#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "model_render.h"
#include "psx_gpu.h"
#include "game.h"

typedef char MODEL_MATRIX_SIZE_CHECK[sizeof(MATRIX) == 32 ? 1 : -1];

typedef char MODEL_MATRIX_TRANSLATION_CHECK[offsetof(MATRIX, t) == 20 ? 1 : -1];

static const uint32 prim_codes[2][4] = {
    {0x800715B8, 0x80071670, 0x80071770, 0x80071828},
    {0x800719E0, 0x80071AA8, 0x80071BB8, 0x80071C98},
};

typedef union MODEL_VERTEX
{
    SVECTOR sdk;
    sint16 coordinates[4];
    uint32 words[2];
} MODEL_VERTEX;

typedef char MODEL_VERTEX_SIZE_CHECK[sizeof(MODEL_VERTEX) == sizeof(SVECTOR) && sizeof(SVECTOR) == 8 ? 1 : -1];

static const uint8 *flare_tex[2];
static const char flare_tex_names[2][7] = {"Flare2", "Flare7"};

MODEL_RENDER_STATE model_render = {.width = 320, .height = 256, .far_depth = 0x8000, .proj_distance = 256, .y_scale = 3600, .x_scale = 6554, .scale = 4096, .lod_half_distance = 750, .lod_quarter_distance = 375, .view_center_x = 160, .view_center_y = 128, .reference_offset_y = 4294966996u, .reference_offset_z = 4000, .coord_shift = 2};

typedef struct MODEL_PACKET_POOL
{
    uint8 *buffers[2];
    uint32 capacity;
    uint32 used;
    uint32 limit;
} MODEL_PACKET_POOL;

enum
{
    MODEL_PACKET_CAPACITY = 0xA000
};

typedef union MODEL_PACKET_BUFFER
{
    uint32 words[MODEL_PACKET_CAPACITY / 4];
    POLY_FT3 ft3;
    POLY_FT4 ft4;
    POLY_GT3 gt3;
    POLY_GT4 gt4;
} MODEL_PACKET_BUFFER;

typedef char MODEL_PACKET_FT3_SIZE_CHECK[sizeof(POLY_FT3) == 32 ? 1 : -1];
typedef char MODEL_PACKET_FT4_SIZE_CHECK[sizeof(POLY_FT4) == 40 ? 1 : -1];
typedef char MODEL_PACKET_GT3_SIZE_CHECK[sizeof(POLY_GT3) == 40 ? 1 : -1];
typedef char MODEL_PACKET_GT4_SIZE_CHECK[sizeof(POLY_GT4) == 52 ? 1 : -1];
typedef char MODEL_PACKET_POINT_T_LAYOUT_CHECK[sizeof(POINT_T) == 8 && offsetof(POLY_FT4, p) == 8 ? 1 : -1];
typedef char MODEL_PACKET_POINT_CT_LAYOUT_CHECK[sizeof(POINT_CT) == 12 && offsetof(POLY_GT3, p) == 4 && offsetof(POLY_GT4, p) == 4 ? 1 : -1];

static MODEL_PACKET_BUFFER packet_buffers[2];

static MODEL_PACKET_POOL packet_pool;
static const sint16 flare_factors[2] = {192, 96};

static const struct
{
    sint16 x;
    sint16 y;
    sint16 z;
} near_billboard_center = {0, 0, 0};

static uint32 quad_outside(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 x2, sint32 y2, sint32 x3, sint32 y3);
static uint32 triangle_outside(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 x2, sint32 y2);
static intptr draw_flat_edges2(POLY_FT4 *packet, DISPLAY_ORDERING ordering, const uint32 depths[4]);
static intptr draw_flat_edges4(POLY_FT4 *packet, DISPLAY_ORDERING ordering, const uint32 depths[4]);
static void subdiv_flat_quad(POLY_FT4 *packet, DISPLAY_ORDERING ordering, const MODEL_VERTEX *vertices, sint32 depth);

static intptr draw_subdiv_edges2(POLY_GT4 *packet, DISPLAY_ORDERING ordering_head, const uint32 depths[4]);
static intptr draw_subdiv_edges4(POLY_GT4 *packet, DISPLAY_ORDERING ordering_head, const uint32 depths[4]);

static void project_triangle(const SVECTOR *v0, const SVECTOR *v1, const SVECTOR *v2);
static void subdiv_flat_triangle(POLY_FT3 *packet, DISPLAY_ORDERING ordering, const MODEL_VERTEX *vertices, sint32 depth);

typedef struct MODEL_LIGHTING_STATE
{
    uint32 radius;
    uint32 radius_sq;
    uint32 scratch[256];
    uint32 colors[2][1024];
    uint32 base_color;
    uint8 half_color[3];
} MODEL_LIGHTING_STATE;

static MODEL_LIGHTING_STATE lighting;

static struct
{
    uint32 last_u;
    uint32 last_v;
} tex_uv_state;

static const uint8 light_black[3] = {0, 0, 0};

static uint32 subdiv_lit_triangle(POLY_GT3 *packet, DISPLAY_ORDERING ordering_table, const MODEL_VERTEX *vertices, sint32 depth, const VEC3 *origin, const uint32 *color_table, uint32 color_bias);
static intptr subdiv_lit_quad(POLY_GT4 *packet, DISPLAY_ORDERING ordering_table, const MODEL_VERTEX *vertices, sint32 depth, const VEC3 *origin, const uint32 *color_table, uint32 color_bias);
static uint32 subdiv_color_triangle(POLY_GT3 *packet, DISPLAY_ORDERING ordering_table, const MODEL_VERTEX *vertices, sint32 depth);
static intptr subdiv_color_quad(POLY_GT4 *packet, DISPLAY_ORDERING ordering_table, const MODEL_VERTEX *vertices, sint32 depth);

static intptr draw_format0(const MODEL_RESOURCE *model_input);
static intptr draw_format1(const MODEL_RESOURCE *model_input);
static intptr draw_format4(const MODEL_RESOURCE *model_input);
static intptr draw_format10(const MODEL_RESOURCE *model_input);
static intptr draw_format12(const MODEL_RESOURCE *model_input);
static uint32 used_vertex_count(const MODEL_RESOURCE *model_input);
static intptr draw_format0_near(const MODEL_RESOURCE *model_input);
static intptr draw_format4_near(const MODEL_RESOURCE *model_input);
static intptr draw_format10_near(const MODEL_RESOURCE *model_input);

typedef intptr (*MODEL_RENDERER)(const MODEL_RESOURCE *object);

static const MODEL_RENDERER renderers[14] = {draw_format0, draw_format1, draw_format0, draw_format1, draw_format4, draw_format4, draw_format4, draw_format4, NULL, draw_format1, draw_format10, draw_format10, draw_format12, draw_format10};

static const MODEL_RENDERER near_renderers[14] = {draw_format0_near, draw_format1, draw_format0_near, draw_format1, draw_format4_near, draw_format4, draw_format4_near, draw_format4, NULL, draw_format1, draw_format10_near, draw_format10_near, draw_format12, draw_format10_near};

static uint32 link_packet(uint32 *packet_tag, DISPLAY_ORDERING ordering_table, sint32 depth, uint32 shift);

static intptr call_renderer(MODEL_RENDERER renderer, const MODEL_RESOURCE *object, uint32 published_type)
{
    XASSERT(!(renderer == NULL));
    return renderer(object);
}

static intptr call_lod_renderer(MODEL_RENDERER renderer, const MODEL_RESOURCE *object, uint32 x_sq, uint32 y_sq, uint32 radius_sq)
{
    XASSERT(!(renderer == NULL));
    return renderer(object);
}

intptr model_draw(const MODEL_RESOURCE *object)
{
    uint16 wire_halfword_1, wire_halfword_2;

    uint32 idx, published_type;
    MODEL_RENDERER renderer;

    FUNCTION_MARKER(0x8006E180, "SLUS_006.10");
    hd_draw_model(object);
    XASSERT(!(object->capacity < 2 || (uint32)(0xA) > object->capacity - 2));
    idx = (sint16)(memcpy(&wire_halfword_1, object->bytes + (uint32)(0xA), sizeof(wire_halfword_1)), wire_halfword_1);
    XASSERT(!(object->capacity < 2 || (uint32)(0xA) > object->capacity - 2));
    published_type = (sint16)(memcpy(&wire_halfword_2, object->bytes + (uint32)(0xA), sizeof(wire_halfword_2)), wire_halfword_2);
    renderer = idx < 14 ? renderers[idx] : NULL;
    return call_renderer(renderer, object, published_type);
}

intptr model_draw_lod(const MODEL_RESOURCE *object)
{
    uint16 wire_halfword_1, wire_halfword_2;

    uint32 pos, offset, camera, x, y, z, radius, distance;
    const MODEL_RENDERER *table;
    MODEL_RENDERER renderer;
    uint32 x_sq, y_sq, z_sq, radius_sq, idx;

    FUNCTION_MARKER(0x80071378, "SLUS_006.10");
    hd_draw_model(object);
    XASSERT(!(object->capacity < 4 || (uint32)(0x10) > object->capacity - 4));
    pos = ((const MODEL_HEADER *)object->bytes)->position.bits.x;
    XASSERT(!(object->capacity < 4 || (uint32)(0x1C) > object->capacity - 4));
    offset = ((const MODEL_HEADER *)object->bytes)->origin.bits.x;
    camera = model_render.distance_reference.bits.x;
    x = ((sint32)((uint32)((sint32)pos >> 8) + offset - camera) >> 4);
    x_sq = x * x;
    XASSERT(!(object->capacity < 4 || (uint32)(0x14) > object->capacity - 4));
    pos = ((const MODEL_HEADER *)object->bytes)->position.bits.y;
    XASSERT(!(object->capacity < 4 || (uint32)(0x20) > object->capacity - 4));
    offset = ((const MODEL_HEADER *)object->bytes)->origin.bits.y;
    camera = model_render.distance_reference.bits.y;
    y = ((sint32)((uint32)((sint32)pos >> 8) + offset - camera) >> 4);
    y_sq = y * y;
    XASSERT(!(object->capacity < 4 || (uint32)(0x18) > object->capacity - 4));
    pos = ((const MODEL_HEADER *)object->bytes)->position.bits.z;
    XASSERT(!(object->capacity < 4 || (uint32)(0x24) > object->capacity - 4));
    offset = ((const MODEL_HEADER *)object->bytes)->origin.bits.z;
    camera = model_render.distance_reference.bits.z;
    z = ((sint32)((uint32)((sint32)pos >> 8) + offset - camera) >> 4);
    z_sq = z * z;
    radius = lighting.radius;
    XASSERT(!(object->capacity < 2 || (uint32)(8) > object->capacity - 2));
    offset = (memcpy(&wire_halfword_1, object->bytes + (uint32)(8), sizeof(wire_halfword_1)), wire_halfword_1);
    radius = (uint32)((sint32)(offset << 16) >> 20) + (uint32)((sint32)radius >> 4);
    radius_sq = radius * radius;
    distance = x_sq + y_sq + z_sq;
    table = (sint32)distance < (sint32)radius_sq ? near_renderers : renderers;
    XASSERT(!(object->capacity < 2 || (uint32)(0xA) > object->capacity - 2));
    idx = (sint16)(memcpy(&wire_halfword_2, object->bytes + (uint32)(0xA), sizeof(wire_halfword_2)), wire_halfword_2);
    renderer = idx < 14 ? table[idx] : NULL;
    return call_lod_renderer(renderer, object, x_sq, y_sq, radius_sq);
}

const sint16 *model_transform_matrix(const MATRIX *matrix, const VEC3 *pos)
{
    VECTOR input;
    VECTOR output;
    MATRIX *view;
    uint32 val;
    uint32 origin;
    uint32 column;
    uint32 word0;
    uint32 word1;
    uint32 word2;

    FUNCTION_MARKER(0x800797FC, "SLUS_006.10");
    hd_transform_model(matrix, pos, 0);
    val = pos->bits.x;
    origin = model_render.view.sdk.t[0];
    input.vx = (sint32)((uint32)((sint32)val >> 8) - origin);
    val = pos->bits.y;
    origin = model_render.view.sdk.t[1];
    input.vy = (sint32)((uint32)((sint32)val >> 8) - origin);
    val = pos->bits.z;
    origin = model_render.view.sdk.t[2];
    input.vz = (sint32)((uint32)((sint32)val >> 8) - origin);
    view = &model_render.view.sdk;
    SetRotMatrix(view);
    ApplyMatrixLV(view, &input, &output);
    model_render.matrix.sdk.t[0] = output.vx;
    model_render.matrix.sdk.t[1] = output.vy;
    model_render.matrix.sdk.t[2] = output.vz;
    for (column = 0; column < 3; ++column)
    {
        word0 = (uint16)matrix->m[0][column];
        word1 = (uint16)matrix->m[1][column];
        word2 = (uint16)matrix->m[2][column];
        xport_gte_write_data(9, word0);
        xport_gte_write_data(10, word1);
        xport_gte_write_data(11, word2);
        xport_gte_execute(0x49E012);
        word0 = xport_gte_read_data(9);
        word1 = xport_gte_read_data(10);
        word2 = xport_gte_read_data(11);
        model_render.matrix.sdk.m[0][column] = word0;
        model_render.matrix.sdk.m[1][column] = word1;
        model_render.matrix.sdk.m[2][column] = word2;
    }
    word0 = model_render.matrix.words[0];
    word1 = model_render.matrix.words[1];
    xport_gte_write_control(0, word0);
    xport_gte_write_control(1, word1);
    word0 = model_render.matrix.words[2];
    word1 = model_render.matrix.words[3];
    word2 = model_render.matrix.words[4];
    xport_gte_write_control(2, word0);
    xport_gte_write_control(3, word1);
    xport_gte_write_control(4, word2);
    word0 = model_render.matrix.sdk.t[0];
    word1 = model_render.matrix.sdk.t[1];
    xport_gte_write_control(5, word0);
    word2 = model_render.matrix.sdk.t[2];
    xport_gte_write_control(6, word1);
    xport_gte_write_control(7, word2);
    return &matrix->m[0][1];
}

sint32 *model_transform_active_pos(const VEC3 *pos)
{
    VECTOR input;
    VECTOR output;
    uint32 val;
    uint32 origin;
    uint32 x;
    uint32 y;
    uint32 z;

    FUNCTION_MARKER(0x80079B84, "SLUS_006.10");
    hd_transform_model(NULL, pos, 0);
    val = pos->bits.x;
    origin = model_render.view.sdk.t[0];
    input.vx = (sint32)((uint32)((sint32)val >> 8) - origin);
    val = pos->bits.y;
    origin = model_render.view.sdk.t[1];
    input.vy = (sint32)((uint32)((sint32)val >> 8) - origin);
    val = pos->bits.z;
    origin = model_render.view.sdk.t[2];
    input.vz = (sint32)((uint32)((sint32)val >> 8) - origin);
    ApplyRotMatrixLV(&input, &output);
    model_render.matrix.sdk.t[0] = output.vx;
    model_render.matrix.sdk.t[1] = output.vy;
    model_render.matrix.sdk.t[2] = output.vz;
    x = model_render.matrix.sdk.t[0];
    y = model_render.matrix.sdk.t[1];
    xport_gte_write_control(5, x);
    z = model_render.matrix.sdk.t[2];
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    return model_render.matrix.sdk.t;
}

sint32 *model_transform_pos(const VEC3 *pos)
{
    VECTOR input;
    VECTOR output;
    MATRIX *matrix;
    uint32 x;
    uint32 y;
    uint32 z;

    FUNCTION_MARKER(0x80079C14, "SLUS_006.10");
    hd_transform_model(NULL, pos, 1);
    input.vx = pos->x >> 8;
    input.vy = pos->y >> 8;
    input.vz = pos->z >> 8;
    matrix = &model_render.view.sdk;
    SetRotMatrix(matrix);
    ApplyMatrixLV(matrix, &input, &output);
    model_render.matrix.sdk.t[0] = output.vx;
    model_render.matrix.sdk.t[1] = output.vy;
    model_render.matrix.sdk.t[2] = output.vz;
    x = model_render.matrix.sdk.t[0];
    y = model_render.matrix.sdk.t[1];
    xport_gte_write_control(5, x);
    z = model_render.matrix.sdk.t[2];
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    return model_render.matrix.sdk.t;
}

uint32 model_visible_matrix(const uint8 *model_header)
{
    uint16 wire_halfword_1;

    VECTOR input;
    VECTOR output;
    MATRIX *matrix;
    uint32 val;
    uint32 offset;
    uint32 origin;
    uint32 shift;
    uint32 depth;
    uint32 x;
    uint32 y;
    uint32 coefficient;
    uint32 depth_term;
    sint32 radius;
    sint32 near_plane;
    sint32 far_plane;

    FUNCTION_MARKER(0x80079C90, "SLUS_006.10");
    val = ((const MODEL_HEADER *)model_header)->position.bits.x;
    offset = ((const MODEL_HEADER *)model_header)->origin.bits.x;
    origin = model_render.view.sdk.t[0];
    radius = (sint16)(memcpy(&wire_halfword_1, model_header + 8, sizeof(wire_halfword_1)), wire_halfword_1);
    input.vx = (sint32)((uint32)((sint32)val >> 8) + offset - origin);
    val = ((const MODEL_HEADER *)model_header)->position.bits.y;
    offset = ((const MODEL_HEADER *)model_header)->origin.bits.y;
    origin = model_render.view.sdk.t[1];
    input.vy = (sint32)((uint32)((sint32)val >> 8) + offset - origin);
    val = ((const MODEL_HEADER *)model_header)->position.bits.z;
    offset = ((const MODEL_HEADER *)model_header)->origin.bits.z;
    origin = model_render.view.sdk.t[2];
    shift = model_render.coord_shift;
    input.vz = (sint32)((uint32)((sint32)val >> 8) + offset - origin);
    radius >>= shift & 31;
    matrix = &model_render.view.sdk;
    SetRotMatrix(matrix);
    ApplyMatrixLV(matrix, &input, &output);
    depth = (uint32)output.vz;
    near_plane = (sint32)model_render.near_depth;
    shift = model_render.coord_shift;
    if ((near_plane >> (shift & 31)) >= (sint32)(depth + (uint32)radius))
        return 0;
    far_plane = (sint32)model_render.far_depth >> (shift & 31);
    if ((sint32)(depth - (uint32)radius) >= far_plane)
        return 0;
    coefficient = (uint32)model_render.clip_x.x;
    x = (uint32)output.vx;
    depth_term = depth * (uint32)model_render.clip_x.z;
    val = x * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    val = (0 - x) * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    coefficient = (uint32)model_render.clip_y.y;
    y = (uint32)output.vy;
    depth_term = depth * (uint32)model_render.clip_y.z;
    val = y * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    val = (0 - y) * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    model_render.depth = depth;
    return 1;
}

uint32 model_visible(const uint8 *model_header)
{
    uint16 wire_halfword_1;

    VECTOR input;
    VECTOR output;
    uint32 val;
    uint32 offset;
    uint32 origin;
    uint32 shift;
    uint32 depth;
    uint32 x;
    uint32 y;
    uint32 coefficient;
    uint32 depth_term;
    sint32 radius;
    sint32 near_plane;
    sint32 far_plane;

    FUNCTION_MARKER(0x80079E34, "SLUS_006.10");
    val = ((const MODEL_HEADER *)model_header)->position.bits.x;
    offset = ((const MODEL_HEADER *)model_header)->origin.bits.x;
    origin = model_render.view.sdk.t[0];
    radius = (sint16)(memcpy(&wire_halfword_1, model_header + 8, sizeof(wire_halfword_1)), wire_halfword_1);
    input.vx = (sint32)((uint32)((sint32)val >> 8) + offset - origin);
    val = ((const MODEL_HEADER *)model_header)->position.bits.y;
    offset = ((const MODEL_HEADER *)model_header)->origin.bits.y;
    origin = model_render.view.sdk.t[1];
    input.vy = (sint32)((uint32)((sint32)val >> 8) + offset - origin);
    val = ((const MODEL_HEADER *)model_header)->position.bits.z;
    offset = ((const MODEL_HEADER *)model_header)->origin.bits.z;
    origin = model_render.view.sdk.t[2];
    shift = model_render.coord_shift;
    input.vz = (sint32)((uint32)((sint32)val >> 8) + offset - origin);
    radius >>= shift & 31;
    ApplyRotMatrixLV(&input, &output);
    depth = (uint32)output.vz;
    near_plane = (sint32)model_render.near_depth;
    shift = model_render.coord_shift;
    if ((near_plane >> (shift & 31)) >= (sint32)(depth + (uint32)radius))
        return 0;
    far_plane = (sint32)model_render.far_depth >> (shift & 31);
    if ((sint32)(depth - (uint32)radius) >= far_plane)
        return 0;
    coefficient = (uint32)model_render.clip_x.x;
    x = (uint32)output.vx;
    depth_term = depth * (uint32)model_render.clip_x.z;
    val = x * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    val = (0 - x) * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    coefficient = (uint32)model_render.clip_y.y;
    y = (uint32)output.vy;
    depth_term = depth * (uint32)model_render.clip_y.z;
    val = y * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    val = (0 - y) * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    model_render.depth = depth;
    return 1;
}

uint32 model_visible_origin(const uint8 *model_header)
{
    uint16 wire_halfword_1;

    VECTOR input;
    VECTOR output;
    MATRIX *matrix;
    uint32 val;
    uint32 origin;
    uint32 shift;
    uint32 depth;
    uint32 x;
    uint32 y;
    uint32 coefficient;
    uint32 depth_term;
    sint32 radius;
    sint32 near_plane;
    sint32 far_plane;

    FUNCTION_MARKER(0x80079FD0, "SLUS_006.10");
    val = ((const MODEL_HEADER *)model_header)->position.bits.x;
    origin = ((const MODEL_HEADER *)model_header)->origin.bits.x;
    radius = (sint16)(memcpy(&wire_halfword_1, model_header + 8, sizeof(wire_halfword_1)), wire_halfword_1);
    input.vx = (sint32)((uint32)((sint32)val >> 8) + origin);
    val = ((const MODEL_HEADER *)model_header)->position.bits.y;
    origin = ((const MODEL_HEADER *)model_header)->origin.bits.y;
    input.vy = (sint32)((uint32)((sint32)val >> 8) + origin);
    val = ((const MODEL_HEADER *)model_header)->position.bits.z;
    origin = ((const MODEL_HEADER *)model_header)->origin.bits.z;
    shift = model_render.coord_shift;
    input.vz = (sint32)((uint32)((sint32)val >> 8) + origin);
    radius >>= shift & 31;
    matrix = &model_render.view.sdk;
    SetRotMatrix(matrix);
    ApplyMatrixLV(matrix, &input, &output);
    depth = (uint32)output.vz;
    near_plane = (sint32)model_render.near_depth;
    shift = model_render.coord_shift;
    near_plane >>= shift & 31;
    if (near_plane >= (sint32)(depth + (uint32)radius))
        return 0;
    far_plane = (sint32)model_render.far_depth >> (shift & 31);
    if ((sint32)(depth - (uint32)radius) >= far_plane)
        return 0;
    coefficient = (uint32)model_render.clip_x.x;
    x = (uint32)output.vx;
    depth_term = depth * (uint32)model_render.clip_x.z;
    val = x * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    val = (0 - x) * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    coefficient = (uint32)model_render.clip_y.y;
    y = (uint32)output.vy;
    depth_term = depth * (uint32)model_render.clip_y.z;
    val = y * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    val = (0 - y) * coefficient + depth_term;
    if (((sint32)val >> 12) >= radius)
        return 0;
    model_render.depth = depth;
    return 1;
}

static intptr draw_format0(const MODEL_RESOURCE *model_input)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;

    const MODEL_VERTEX *vertex_input;
    DISPLAY_ORDERING ordering, ordering_slot;
    uint32 stream_offset, buffer, shift, lod, prim_offset, packet_offset, vertex_offset, offset;
    uint32 limit, result, count, kind, target, prev;
    uint32 offset0, offset1, offset2, offset3, next_offset0, next_offset1, next_offset2;
    uint32 idx0, idx1, idx2, idx3, packet_tag, depth;
    uint32 triangle, colored, packet_step, stream_step, dec_pc;
    sint32 area0, area1;
    sint64 shift_difference;

    DISPLAY_FRAME *cur_frame;
    void *native_packet;
    uint32 *packet_tag_field;
    POINT_T *flat_points;
    POINT_CT *gouraud_points;
    RGBC *flat_color;

    FUNCTION_MARKER(0x80071544, "SLUS_006.10");
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x30) > model_input->capacity - 4));
    stream_offset = ((const MODEL_HEADER *)model_input->bytes)->stream_offset;
    buffer = disp_state.bank;
    shift = model_render.ot_shift;
    lod = model_render.coord_shift;
    prim_offset = stream_offset;
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x34 + buffer * 4) > model_input->capacity - 4));
    memcpy(&offset, model_input->bytes + (uint32)(0x34 + buffer * 4), sizeof(offset));
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x2C) > model_input->capacity - 4));
    vertex_offset = ((const MODEL_HEADER *)model_input->bytes)->vertex_offset;
    packet_offset = offset;
    XASSERT(!(model_input->capacity < 2 || (uint32)(0xC) > model_input->capacity - 2));
    offset = (sint16)(memcpy(&wire_halfword_1, model_input->bytes + (uint32)(0xC), sizeof(wire_halfword_1)), wire_halfword_1);
    cur_frame = disp_state.cur;
    shift_difference = (sint64)(sint32)shift - (sint64)(sint32)lod;
    if (shift_difference < (-2147483647 - (sint64)1) || shift_difference > 2147483647)
        xport_mips_overflow_exception(0x80071588);
    shift = shift_difference;
    offset = ((sint32)offset >> (shift & 31));
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    ordering.byte_offset += offset * 4;
    result = model_render.far_depth >> (lod & 31);
    limit = ((sint32)result >> 2);
    for (;;)
    {
        XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset) > model_input->capacity - 2));
        count = (sint16)(memcpy(&wire_halfword_2, model_input->bytes + (uint32)(prim_offset), sizeof(wire_halfword_2)), wire_halfword_2);
        XASSERT(!(model_input->capacity < 1 || (uint32)(prim_offset + 2) > model_input->capacity - 1));
        kind = (model_input->bytes + (uint32)(prim_offset + 2))[0];
        if (count == 0)
            return result;
        XASSERT(!(kind >= 4));
        target = prim_codes[0][kind];
        result = target;
        prim_offset += 4;
        triangle = target == 0x800715B8 || target == 0x80071770;
        colored = target == 0x80071770 || target == 0x80071828;
        stream_step = triangle ? (colored ? 44 : 20) : (colored ? 56 : 20);
        packet_step = triangle ? (colored ? 40 : 32) : (colored ? 52 : 40);
        packet_tag = triangle ? (colored ? 0x09000000 : 0x07000000) : (colored ? 0x0C000000 : 0x09000000);
        dec_pc = triangle ? (colored ? 0x800717C8 : 0x80071610) : (colored ? 0x80071898 : 0x800716E0);
        if (!triangle)
        {
            XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset) > model_input->capacity - 2));
            idx0 = (sint16)(memcpy(&wire_halfword_3, model_input->bytes + (uint32)(prim_offset), sizeof(wire_halfword_3)), wire_halfword_3);
            XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 2) > model_input->capacity - 2));
            idx1 = (sint16)(memcpy(&wire_halfword_4, model_input->bytes + (uint32)(prim_offset + 2), sizeof(wire_halfword_4)), wire_halfword_4);
            XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 4) > model_input->capacity - 2));
            idx2 = (sint16)(memcpy(&wire_halfword_5, model_input->bytes + (uint32)(prim_offset + 4), sizeof(wire_halfword_5)), wire_halfword_5);
            offset0 = vertex_offset + idx0 * 8;
            offset1 = vertex_offset + idx1 * 8;
            offset2 = vertex_offset + idx2 * 8;
        }
        do
        {
            if (triangle)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset) > model_input->capacity - 2));
                idx0 = (sint16)(memcpy(&wire_halfword_6, model_input->bytes + (uint32)(prim_offset), sizeof(wire_halfword_6)), wire_halfword_6);
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 2) > model_input->capacity - 2));
                idx1 = (sint16)(memcpy(&wire_halfword_7, model_input->bytes + (uint32)(prim_offset + 2), sizeof(wire_halfword_7)), wire_halfword_7);
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 4) > model_input->capacity - 2));
                idx2 = (sint16)(memcpy(&wire_halfword_8, model_input->bytes + (uint32)(prim_offset + 4), sizeof(wire_halfword_8)), wire_halfword_8);
                offset0 = vertex_offset + idx0 * 8;
                offset1 = vertex_offset + idx1 * 8;
                offset2 = vertex_offset + idx2 * 8;
            }
            XASSERT(!((uint32)(offset0) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(offset0)));
            vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(offset0));
            xport_gte_write_data(0, vertex_input->words[0]);
            xport_gte_write_data(1, vertex_input->words[1]);
            XASSERT(!((uint32)(offset1) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(offset1)));
            vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(offset1));
            xport_gte_write_data(2, vertex_input->words[0]);
            xport_gte_write_data(3, vertex_input->words[1]);
            XASSERT(!((uint32)(offset2) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(offset2)));
            vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(offset2));
            xport_gte_write_data(4, vertex_input->words[0]);
            xport_gte_write_data(5, vertex_input->words[1]);
            if (!triangle)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 6) > model_input->capacity - 2));
                idx3 = (sint16)(memcpy(&wire_halfword_9, model_input->bytes + (uint32)(prim_offset + 6), sizeof(wire_halfword_9)), wire_halfword_9);
                prim_offset += stream_step;
            }
            xport_gte_execute(0x280030);
            if (!triangle)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset) > model_input->capacity - 2));
                idx0 = (sint16)(memcpy(&wire_halfword_10, model_input->bytes + (uint32)(prim_offset), sizeof(wire_halfword_10)), wire_halfword_10);
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 2) > model_input->capacity - 2));
                idx1 = (sint16)(memcpy(&wire_halfword_11, model_input->bytes + (uint32)(prim_offset + 2), sizeof(wire_halfword_11)), wire_halfword_11);
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 4) > model_input->capacity - 2));
                idx2 = (sint16)(memcpy(&wire_halfword_12, model_input->bytes + (uint32)(prim_offset + 4), sizeof(wire_halfword_12)), wire_halfword_12);
                next_offset0 = vertex_offset + idx0 * 8;
            }
            xport_gte_execute(0x1400006);
            if (!triangle)
                offset3 = vertex_offset + idx3 * 8;
            area0 = (sint32)xport_gte_read_data(24);
            if (triangle)
            {
                count = render_add_checked(count, (uint32)-1, dec_pc);
                xport_gte_execute(0x158002D);
                if (area0 <= 0)
                {
                    depth = xport_gte_read_data(7);
                    result = packet_tag;
                    if ((sint32)depth > 0)
                    {
                        if (colored)
                        {
                            if (triangle)
                            {
                                XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_GT3) > model_input->capacity - (uint32)(packet_offset)));
                                POLY_GT3 *poly = (POLY_GT3 *)(model_input->bytes + (uint32)(packet_offset));
                                native_packet = poly;
                                packet_tag_field = &poly->tag;
                                gouraud_points = poly->p;
                                flat_points = NULL;
                            }
                            else
                            {
                                XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_GT4) > model_input->capacity - (uint32)(packet_offset)));
                                POLY_GT4 *poly = (POLY_GT4 *)(model_input->bytes + (uint32)(packet_offset));
                                native_packet = poly;
                                packet_tag_field = &poly->tag;
                                gouraud_points = poly->p;
                                flat_points = NULL;
                            }
                        }
                        else
                        {
                            if (triangle)
                            {
                                XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT3) > model_input->capacity - (uint32)(packet_offset)));
                                POLY_FT3 *poly = (POLY_FT3 *)(model_input->bytes + (uint32)(packet_offset));
                                native_packet = poly;
                                packet_tag_field = &poly->tag;
                                flat_points = poly->p;
                                gouraud_points = NULL;
                                flat_color = &poly->rgbc;
                            }
                            else
                            {
                                XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT4) > model_input->capacity - (uint32)(packet_offset)));
                                POLY_FT4 *poly = (POLY_FT4 *)(model_input->bytes + (uint32)(packet_offset));
                                native_packet = poly;
                                packet_tag_field = &poly->tag;
                                flat_points = poly->p;
                                gouraud_points = NULL;
                                flat_color = &poly->rgbc;
                            }
                        }
                        *(colored ? &gouraud_points[0].xy.xy : &flat_points[0].xy.xy) = xport_gte_read_data(12);
                        if ((sint32)depth < (sint32)limit)
                        {
                            *(colored ? &gouraud_points[1].xy.xy : &flat_points[1].xy.xy) = xport_gte_read_data(13);
                            *(colored ? &gouraud_points[2].xy.xy : &flat_points[2].xy.xy) = xport_gte_read_data(14);
                            ordering_slot = ordering;
                            ordering_slot.byte_offset += (uint32)((sint32)depth >> (shift & 31)) * 4;
                            prev = ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4];
                            ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4] = gpu_packet_link((uintptr_t)native_packet);
                            result |= prev & 0xFFFFFF;
                            *packet_tag_field = result;
                        }
                    }
                }
                prim_offset += stream_step;
            }
            else
            {
                if (colored)
                {
                    if (triangle)
                    {
                        XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_GT3) > model_input->capacity - (uint32)(packet_offset)));
                        POLY_GT3 *poly = (POLY_GT3 *)(model_input->bytes + (uint32)(packet_offset));
                        native_packet = poly;
                        packet_tag_field = &poly->tag;
                        gouraud_points = poly->p;
                        flat_points = NULL;
                    }
                    else
                    {
                        XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_GT4) > model_input->capacity - (uint32)(packet_offset)));
                        POLY_GT4 *poly = (POLY_GT4 *)(model_input->bytes + (uint32)(packet_offset));
                        native_packet = poly;
                        packet_tag_field = &poly->tag;
                        gouraud_points = poly->p;
                        flat_points = NULL;
                    }
                }
                else
                {
                    if (triangle)
                    {
                        XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT3) > model_input->capacity - (uint32)(packet_offset)));
                        POLY_FT3 *poly = (POLY_FT3 *)(model_input->bytes + (uint32)(packet_offset));
                        native_packet = poly;
                        packet_tag_field = &poly->tag;
                        flat_points = poly->p;
                        gouraud_points = NULL;
                        flat_color = &poly->rgbc;
                    }
                    else
                    {
                        XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT4) > model_input->capacity - (uint32)(packet_offset)));
                        POLY_FT4 *poly = (POLY_FT4 *)(model_input->bytes + (uint32)(packet_offset));
                        native_packet = poly;
                        packet_tag_field = &poly->tag;
                        flat_points = poly->p;
                        gouraud_points = NULL;
                        flat_color = &poly->rgbc;
                    }
                }
                *(colored ? &gouraud_points[0].xy.xy : &flat_points[0].xy.xy) = xport_gte_read_data(12);
                count = render_add_checked(count, (uint32)-1, dec_pc);
                *(colored ? &gouraud_points[1].xy.xy : &flat_points[1].xy.xy) = xport_gte_read_data(13);
                *(colored ? &gouraud_points[2].xy.xy : &flat_points[2].xy.xy) = xport_gte_read_data(14);
                XASSERT(!((uint32)(offset3) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(offset3)));
                vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(offset3));
                xport_gte_write_data(0, vertex_input->words[0]);
                xport_gte_write_data(1, vertex_input->words[1]);
                result = packet_tag;
                xport_gte_execute(0x180001);
                next_offset1 = vertex_offset + idx1 * 8;
                next_offset2 = vertex_offset + idx2 * 8;
                if (area0 > 0)
                {
                    xport_gte_execute(0x1400006);
                    area1 = (sint32)xport_gte_read_data(24);
                }
                else
                    area1 = 0;
                xport_gte_execute(0x168002E);
                if (area0 <= 0 || area1 >= 0)
                {
                    depth = xport_gte_read_data(7);
                    *(colored ? &gouraud_points[3].xy.xy : &flat_points[3].xy.xy) = xport_gte_read_data(14);
                    if ((sint32)depth > 0 && (sint32)depth < (sint32)limit)
                    {
                        ordering_slot = ordering;
                        ordering_slot.byte_offset += (uint32)((sint32)depth >> (shift & 31)) * 4;
                        prev = ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4];
                        ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4] = gpu_packet_link((uintptr_t)native_packet);
                        result |= prev & 0xFFFFFF;
                        *packet_tag_field = result;
                    }
                }
                offset0 = next_offset0;
                offset1 = next_offset1;
                offset2 = next_offset2;
            }
            packet_offset += packet_step;
        } while (count != 0);
    }
}

static intptr draw_format1(const MODEL_RESOURCE *model_input)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;

    uint32 wire_word_3, wire_word_4, wire_word_5, wire_word_6, wire_word_7, wire_word_8, wire_word_9, wire_word_10, wire_word_11, wire_word_12, wire_word_13, wire_word_14, wire_word_15, wire_word_16, wire_word_17, wire_word_18, wire_word_19, wire_word_20, wire_word_21, wire_word_22, wire_word_23;
    const MODEL_VERTEX *vertex_input;
    DISPLAY_ORDERING ordering, ordering_slot;
    uint32 stream_offset, buffer, shift, lod, prim_offset, packet_offset, vertex_offset, offset;
    uint32 limit, result, count, kind, target, prev;
    uint32 offset0, offset1, offset2, offset3, next_offset0, next_offset1, next_offset2;
    uint32 idx0, idx1, idx2, idx3, packet_tag, depth;
    uint32 triangle, colored, packet_step, stream_step, dec_pc;
    sint32 area0, area1;
    sint64 shift_difference;

    DISPLAY_FRAME *cur_frame;
    void *native_packet;
    uint32 *packet_tag_field;
    POINT_T *flat_points;
    POINT_CT *gouraud_points;
    RGBC *flat_color;

    FUNCTION_MARKER(0x8007196C, "SLUS_006.10");
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x30) > model_input->capacity - 4));
    stream_offset = ((const MODEL_HEADER *)model_input->bytes)->stream_offset;
    buffer = disp_state.bank;
    shift = model_render.ot_shift;
    lod = model_render.coord_shift;
    prim_offset = stream_offset;
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x34 + buffer * 4) > model_input->capacity - 4));
    memcpy(&offset, model_input->bytes + (uint32)(0x34 + buffer * 4), sizeof(offset));
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x2C) > model_input->capacity - 4));
    vertex_offset = ((const MODEL_HEADER *)model_input->bytes)->vertex_offset;
    packet_offset = offset;
    XASSERT(!(model_input->capacity < 2 || (uint32)(0xC) > model_input->capacity - 2));
    offset = (sint16)(memcpy(&wire_halfword_1, model_input->bytes + (uint32)(0xC), sizeof(wire_halfword_1)), wire_halfword_1);
    cur_frame = disp_state.cur;
    shift_difference = (sint64)(sint32)shift - (sint64)(sint32)lod;
    if (shift_difference < (-2147483647 - (sint64)1) || shift_difference > 2147483647)
        xport_mips_overflow_exception(0x800719B0);
    shift = shift_difference;
    offset = ((sint32)offset >> (shift & 31));
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    ordering.byte_offset += offset * 4;
    result = model_render.far_depth >> (lod & 31);
    limit = ((sint32)result >> 2);
    for (;;)
    {
        XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset) > model_input->capacity - 2));
        count = (sint16)(memcpy(&wire_halfword_2, model_input->bytes + (uint32)(prim_offset), sizeof(wire_halfword_2)), wire_halfword_2);
        XASSERT(!(model_input->capacity < 1 || (uint32)(prim_offset + 2) > model_input->capacity - 1));
        kind = (model_input->bytes + (uint32)(prim_offset + 2))[0];
        if (count == 0)
            return result;
        XASSERT(!(kind >= 4));
        target = prim_codes[1][kind];
        result = target;
        prim_offset += 4;
        triangle = target == 0x800719E0 || target == 0x80071BB8;
        colored = target == 0x80071BB8 || target == 0x80071C98;
        stream_step = triangle ? (colored ? 44 : 20) : (colored ? 56 : 20);
        packet_step = triangle ? (colored ? 40 : 32) : (colored ? 52 : 40);
        packet_tag = triangle ? (colored ? 0x09000000 : 0x07000000) : (colored ? 0x0C000000 : 0x09000000);
        dec_pc = triangle ? (colored ? 0x80071C10 : 0x80071A38) : (colored ? 0x80071D08 : 0x80071B18);
        if (!triangle)
        {
            XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset) > model_input->capacity - 2));
            idx0 = (sint16)(memcpy(&wire_halfword_3, model_input->bytes + (uint32)(prim_offset), sizeof(wire_halfword_3)), wire_halfword_3);
            XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 2) > model_input->capacity - 2));
            idx1 = (sint16)(memcpy(&wire_halfword_4, model_input->bytes + (uint32)(prim_offset + 2), sizeof(wire_halfword_4)), wire_halfword_4);
            XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 4) > model_input->capacity - 2));
            idx2 = (sint16)(memcpy(&wire_halfword_5, model_input->bytes + (uint32)(prim_offset + 4), sizeof(wire_halfword_5)), wire_halfword_5);
            offset0 = vertex_offset + idx0 * 8;
            offset1 = vertex_offset + idx1 * 8;
            offset2 = vertex_offset + idx2 * 8;
        }
        do
        {
            if (triangle)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset) > model_input->capacity - 2));
                idx0 = (sint16)(memcpy(&wire_halfword_6, model_input->bytes + (uint32)(prim_offset), sizeof(wire_halfword_6)), wire_halfword_6);
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 2) > model_input->capacity - 2));
                idx1 = (sint16)(memcpy(&wire_halfword_7, model_input->bytes + (uint32)(prim_offset + 2), sizeof(wire_halfword_7)), wire_halfword_7);
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 4) > model_input->capacity - 2));
                idx2 = (sint16)(memcpy(&wire_halfword_8, model_input->bytes + (uint32)(prim_offset + 4), sizeof(wire_halfword_8)), wire_halfword_8);
                offset0 = vertex_offset + idx0 * 8;
                offset1 = vertex_offset + idx1 * 8;
                offset2 = vertex_offset + idx2 * 8;
            }
            XASSERT(!((uint32)(offset0) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(offset0)));
            vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(offset0));
            xport_gte_write_data(0, vertex_input->words[0]);
            xport_gte_write_data(1, vertex_input->words[1]);
            XASSERT(!((uint32)(offset1) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(offset1)));
            vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(offset1));
            xport_gte_write_data(2, vertex_input->words[0]);
            xport_gte_write_data(3, vertex_input->words[1]);
            XASSERT(!((uint32)(offset2) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(offset2)));
            vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(offset2));
            xport_gte_write_data(4, vertex_input->words[0]);
            xport_gte_write_data(5, vertex_input->words[1]);
            if (!triangle)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 6) > model_input->capacity - 2));
                idx3 = (sint16)(memcpy(&wire_halfword_9, model_input->bytes + (uint32)(prim_offset + 6), sizeof(wire_halfword_9)), wire_halfword_9);
                prim_offset += stream_step;
            }
            xport_gte_execute(0x280030);
            if (!triangle)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset) > model_input->capacity - 2));
                idx0 = (sint16)(memcpy(&wire_halfword_10, model_input->bytes + (uint32)(prim_offset), sizeof(wire_halfword_10)), wire_halfword_10);
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 2) > model_input->capacity - 2));
                idx1 = (sint16)(memcpy(&wire_halfword_11, model_input->bytes + (uint32)(prim_offset + 2), sizeof(wire_halfword_11)), wire_halfword_11);
                XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 4) > model_input->capacity - 2));
                idx2 = (sint16)(memcpy(&wire_halfword_12, model_input->bytes + (uint32)(prim_offset + 4), sizeof(wire_halfword_12)), wire_halfword_12);
                next_offset0 = vertex_offset + idx0 * 8;
            }
            xport_gte_execute(0x1400006);
            if (!triangle)
                offset3 = vertex_offset + idx3 * 8;
            area0 = (sint32)xport_gte_read_data(24);
            if (triangle)
            {
                count = render_add_checked(count, (uint32)-1, dec_pc);
                xport_gte_execute(0x158002D);
                if (area0 <= 0)
                {
                    depth = xport_gte_read_data(7);
                    result = packet_tag;
                    if (colored)
                    {
                        if (triangle)
                        {
                            XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_GT3) > model_input->capacity - (uint32)(packet_offset)));
                            POLY_GT3 *poly = (POLY_GT3 *)(model_input->bytes + (uint32)(packet_offset));
                            native_packet = poly;
                            packet_tag_field = &poly->tag;
                            gouraud_points = poly->p;
                            flat_points = NULL;
                            flat_color = NULL;
                        }
                        else
                        {
                            XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_GT4) > model_input->capacity - (uint32)(packet_offset)));
                            POLY_GT4 *poly = (POLY_GT4 *)(model_input->bytes + (uint32)(packet_offset));
                            native_packet = poly;
                            packet_tag_field = &poly->tag;
                            gouraud_points = poly->p;
                            flat_points = NULL;
                            flat_color = NULL;
                        }
                    }
                    else
                    {
                        if (triangle)
                        {
                            XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT3) > model_input->capacity - (uint32)(packet_offset)));
                            POLY_FT3 *poly = (POLY_FT3 *)(model_input->bytes + (uint32)(packet_offset));
                            native_packet = poly;
                            packet_tag_field = &poly->tag;
                            flat_points = poly->p;
                            gouraud_points = NULL;
                            flat_color = &poly->rgbc;
                        }
                        else
                        {
                            XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT4) > model_input->capacity - (uint32)(packet_offset)));
                            POLY_FT4 *poly = (POLY_FT4 *)(model_input->bytes + (uint32)(packet_offset));
                            native_packet = poly;
                            packet_tag_field = &poly->tag;
                            flat_points = poly->p;
                            gouraud_points = NULL;
                            flat_color = &poly->rgbc;
                        }
                    }
                    *(colored ? &gouraud_points[0].xy.xy : &flat_points[0].xy.xy) = xport_gte_read_data(12);
                    if ((sint32)depth < (sint32)limit)
                    {
                        *(colored ? &gouraud_points[1].xy.xy : &flat_points[1].xy.xy) = xport_gte_read_data(13);
                        *(colored ? &gouraud_points[2].xy.xy : &flat_points[2].xy.xy) = xport_gte_read_data(14);
                        ordering_slot = ordering;
                        ordering_slot.byte_offset += (uint32)((sint32)depth >> (shift & 31)) * 4;
                        prev = ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4];
                        XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset + 8) > model_input->capacity - 4));
                        xport_gte_write_data(6, (memcpy(&wire_word_3, model_input->bytes + (uint32)(prim_offset + 8), sizeof(wire_word_3)), wire_word_3));
                        if (colored)
                        {
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset + 20) > model_input->capacity - 4));
                            xport_gte_write_data(0, (memcpy(&wire_word_4, model_input->bytes + (uint32)(prim_offset + 20), sizeof(wire_word_4)), wire_word_4));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset + 24) > model_input->capacity - 4));
                            xport_gte_write_data(1, (memcpy(&wire_word_5, model_input->bytes + (uint32)(prim_offset + 24), sizeof(wire_word_5)), wire_word_5));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset + 28) > model_input->capacity - 4));
                            xport_gte_write_data(2, (memcpy(&wire_word_6, model_input->bytes + (uint32)(prim_offset + 28), sizeof(wire_word_6)), wire_word_6));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset + 32) > model_input->capacity - 4));
                            xport_gte_write_data(3, (memcpy(&wire_word_7, model_input->bytes + (uint32)(prim_offset + 32), sizeof(wire_word_7)), wire_word_7));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset + 36) > model_input->capacity - 4));
                            xport_gte_write_data(4, (memcpy(&wire_word_8, model_input->bytes + (uint32)(prim_offset + 36), sizeof(wire_word_8)), wire_word_8));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset + 40) > model_input->capacity - 4));
                            xport_gte_write_data(5, (memcpy(&wire_word_9, model_input->bytes + (uint32)(prim_offset + 40), sizeof(wire_word_9)), wire_word_9));
                        }
                        else
                        {
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset + 12) > model_input->capacity - 4));
                            xport_gte_write_data(0, (memcpy(&wire_word_10, model_input->bytes + (uint32)(prim_offset + 12), sizeof(wire_word_10)), wire_word_10));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset + 16) > model_input->capacity - 4));
                            xport_gte_write_data(1, (memcpy(&wire_word_11, model_input->bytes + (uint32)(prim_offset + 16), sizeof(wire_word_11)), wire_word_11));
                        }
                        ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4] = gpu_packet_link((uintptr_t)native_packet);
                        xport_gte_execute(colored ? 0x118043F : 0x108041B);
                        result |= prev & 0xFFFFFF;
                        *packet_tag_field = result;
                        if (colored)
                        {
                            *(colored ? &gouraud_points[0].rgbc.rgbc : &flat_color->rgbc) = xport_gte_read_data(20);
                            gouraud_points[1].rgbc.rgbc = xport_gte_read_data(21);
                            gouraud_points[2].rgbc.rgbc = xport_gte_read_data(22);
                        }
                        else
                            *(colored ? &gouraud_points[0].rgbc.rgbc : &flat_color->rgbc) = xport_gte_read_data(22);
                    }
                }
                prim_offset += stream_step;
            }
            else
            {
                if (colored)
                {
                    if (triangle)
                    {
                        XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_GT3) > model_input->capacity - (uint32)(packet_offset)));
                        POLY_GT3 *poly = (POLY_GT3 *)(model_input->bytes + (uint32)(packet_offset));
                        native_packet = poly;
                        packet_tag_field = &poly->tag;
                        gouraud_points = poly->p;
                        flat_points = NULL;
                        flat_color = NULL;
                    }
                    else
                    {
                        XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_GT4) > model_input->capacity - (uint32)(packet_offset)));
                        POLY_GT4 *poly = (POLY_GT4 *)(model_input->bytes + (uint32)(packet_offset));
                        native_packet = poly;
                        packet_tag_field = &poly->tag;
                        gouraud_points = poly->p;
                        flat_points = NULL;
                        flat_color = NULL;
                    }
                }
                else
                {
                    if (triangle)
                    {
                        XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT3) > model_input->capacity - (uint32)(packet_offset)));
                        POLY_FT3 *poly = (POLY_FT3 *)(model_input->bytes + (uint32)(packet_offset));
                        native_packet = poly;
                        packet_tag_field = &poly->tag;
                        flat_points = poly->p;
                        gouraud_points = NULL;
                        flat_color = &poly->rgbc;
                    }
                    else
                    {
                        XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT4) > model_input->capacity - (uint32)(packet_offset)));
                        POLY_FT4 *poly = (POLY_FT4 *)(model_input->bytes + (uint32)(packet_offset));
                        native_packet = poly;
                        packet_tag_field = &poly->tag;
                        flat_points = poly->p;
                        gouraud_points = NULL;
                        flat_color = &poly->rgbc;
                    }
                }
                *(colored ? &gouraud_points[0].xy.xy : &flat_points[0].xy.xy) = xport_gte_read_data(12);
                count = render_add_checked(count, (uint32)-1, dec_pc);
                *(colored ? &gouraud_points[1].xy.xy : &flat_points[1].xy.xy) = xport_gte_read_data(13);
                *(colored ? &gouraud_points[2].xy.xy : &flat_points[2].xy.xy) = xport_gte_read_data(14);
                XASSERT(!((uint32)(offset3) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(offset3)));
                vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(offset3));
                xport_gte_write_data(0, vertex_input->words[0]);
                xport_gte_write_data(1, vertex_input->words[1]);
                result = packet_tag;
                xport_gte_execute(0x180001);
                next_offset1 = vertex_offset + idx1 * 8;
                next_offset2 = vertex_offset + idx2 * 8;
                if (area0 > 0)
                {
                    xport_gte_execute(0x1400006);
                    area1 = (sint32)xport_gte_read_data(24);
                }
                else
                    area1 = 0;
                xport_gte_execute(0x168002E);
                if (area0 <= 0 || area1 >= 0)
                {
                    depth = xport_gte_read_data(7);
                    *(colored ? &gouraud_points[3].xy.xy : &flat_points[3].xy.xy) = xport_gte_read_data(14);
                    if (colored)
                    {
                        XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 48) > model_input->capacity - 4));
                        xport_gte_write_data(6, (memcpy(&wire_word_12, model_input->bytes + (uint32)(prim_offset - 48), sizeof(wire_word_12)), wire_word_12));
                    }
                    if ((sint32)depth < (sint32)limit)
                    {
                        if (colored)
                        {
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 32) > model_input->capacity - 4));
                            xport_gte_write_data(0, (memcpy(&wire_word_13, model_input->bytes + (uint32)(prim_offset - 32), sizeof(wire_word_13)), wire_word_13));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 28) > model_input->capacity - 4));
                            xport_gte_write_data(1, (memcpy(&wire_word_14, model_input->bytes + (uint32)(prim_offset - 28), sizeof(wire_word_14)), wire_word_14));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 24) > model_input->capacity - 4));
                            xport_gte_write_data(2, (memcpy(&wire_word_15, model_input->bytes + (uint32)(prim_offset - 24), sizeof(wire_word_15)), wire_word_15));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 20) > model_input->capacity - 4));
                            xport_gte_write_data(3, (memcpy(&wire_word_16, model_input->bytes + (uint32)(prim_offset - 20), sizeof(wire_word_16)), wire_word_16));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 16) > model_input->capacity - 4));
                            xport_gte_write_data(4, (memcpy(&wire_word_17, model_input->bytes + (uint32)(prim_offset - 16), sizeof(wire_word_17)), wire_word_17));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 12) > model_input->capacity - 4));
                            xport_gte_write_data(5, (memcpy(&wire_word_18, model_input->bytes + (uint32)(prim_offset - 12), sizeof(wire_word_18)), wire_word_18));
                            ordering_slot = ordering;
                            ordering_slot.byte_offset += (uint32)((sint32)depth >> (shift & 31)) * 4;
                            xport_gte_execute(0x118043F);
                            prev = ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4];
                            ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4] = gpu_packet_link((uintptr_t)native_packet);
                            *(colored ? &gouraud_points[0].rgbc.rgbc : &flat_color->rgbc) = xport_gte_read_data(20);
                            gouraud_points[1].rgbc.rgbc = xport_gte_read_data(21);
                            gouraud_points[2].rgbc.rgbc = xport_gte_read_data(22);
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 8) > model_input->capacity - 4));
                            xport_gte_write_data(0, (memcpy(&wire_word_19, model_input->bytes + (uint32)(prim_offset - 8), sizeof(wire_word_19)), wire_word_19));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 4) > model_input->capacity - 4));
                            xport_gte_write_data(1, (memcpy(&wire_word_20, model_input->bytes + (uint32)(prim_offset - 4), sizeof(wire_word_20)), wire_word_20));
                            result |= prev & 0xFFFFFF;
                            *packet_tag_field = result;
                            xport_gte_execute(0x108041B);
                            gouraud_points[3].rgbc.rgbc = xport_gte_read_data(22);
                        }
                        else
                        {
                            ordering_slot = ordering;
                            ordering_slot.byte_offset += (uint32)((sint32)depth >> (shift & 31)) * 4;
                            prev = ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4];
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 12) > model_input->capacity - 4));
                            xport_gte_write_data(6, (memcpy(&wire_word_21, model_input->bytes + (uint32)(prim_offset - 12), sizeof(wire_word_21)), wire_word_21));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 8) > model_input->capacity - 4));
                            xport_gte_write_data(0, (memcpy(&wire_word_22, model_input->bytes + (uint32)(prim_offset - 8), sizeof(wire_word_22)), wire_word_22));
                            XASSERT(!(model_input->capacity < 4 || (uint32)(prim_offset - 4) > model_input->capacity - 4));
                            xport_gte_write_data(1, (memcpy(&wire_word_23, model_input->bytes + (uint32)(prim_offset - 4), sizeof(wire_word_23)), wire_word_23));
                            ordering_slot.tags[(sint32)ordering_slot.byte_offset / 4] = gpu_packet_link((uintptr_t)native_packet);
                            xport_gte_execute(0x108041B);
                            result |= prev & 0xFFFFFF;
                            *packet_tag_field = result;
                            *(colored ? &gouraud_points[0].rgbc.rgbc : &flat_color->rgbc) = xport_gte_read_data(22);
                        }
                    }
                }
                offset0 = next_offset0;
                offset1 = next_offset1;
                offset2 = next_offset2;
            }
            packet_offset += packet_step;
        } while (count != 0);
    }
}

static intptr draw_format4(const MODEL_RESOURCE *model_input)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;

    DISPLAY_ORDERING ordering;
    MODEL_VERTEX copied[4][4];
    uint32 radius;
    uint32 val;
    uint32 shift;
    uint32 packet_offset;
    uint32 stream_offset;
    uint32 vertex_offset;

    uint32 result;
    uint32 kind;
    MODEL_VERTEX *copy_vectors;
    const MODEL_VERTEX *vectors;
    uint32 vertex_count;
    uint32 vertex_idx;
    uint32 vertex_src_offset;
    uint32 word_idx;
    uint32 high;
    uint32 low;
    uint32 vertex_word_offset;
    uint32 words[2];
    sint32 count;
    sint32 idx;
    sint32 area0, area1;
    sint32 depth;
    sint32 far_plane;
    sint32 bias;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80076F44, "SLUS_006.10");
    XASSERT(!(model_input->capacity < 2 || (uint32)(8) > model_input->capacity - 2));
    radius = (sint16)(memcpy(&wire_halfword_1, model_input->bytes + (uint32)(8), sizeof(wire_halfword_1)), wire_halfword_1);
    val = model_render.depth;
    far_plane = (sint32)model_render.lod_half_distance;
    if ((sint32)(val - radius) >= far_plane)
        return draw_format0(model_input);
    val = disp_state.bank;
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x34 + val * 4) > model_input->capacity - 4));
    memcpy(&packet_offset, model_input->bytes + (uint32)(0x34 + val * 4), sizeof(packet_offset));
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x30) > model_input->capacity - 4));
    stream_offset = ((const MODEL_HEADER *)model_input->bytes)->stream_offset;
    shift = model_render.coord_shift;
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x2C) > model_input->capacity - 4));
    vertex_offset = ((const MODEL_HEADER *)model_input->bytes)->vertex_offset;
    val = model_render.ot_shift;
    cur_frame = disp_state.cur;
    XASSERT(!(model_input->capacity < 2 || (uint32)(12) > model_input->capacity - 2));
    bias = (sint16)(memcpy(&wire_halfword_2, model_input->bytes + (uint32)(12), sizeof(wire_halfword_2)), wire_halfword_2);
    shift = (val - shift) & 31;
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    XASSERT(!(model_input->capacity < 1 || (uint32)(stream_offset + 3) > model_input->capacity - 1));
    val = (model_input->bytes + (uint32)(stream_offset + 3))[0];
    result = (uint32)(bias >> shift) << 2;
    ordering.byte_offset += result;
    if (val == 0)
        return result;
    do
    {
        XASSERT(!(model_input->capacity < 1 || (uint32)(stream_offset + 2) > model_input->capacity - 1));
        kind = (model_input->bytes + (uint32)(stream_offset + 2))[0];
        XASSERT(!(model_input->capacity < 2 || (uint32)(stream_offset) > model_input->capacity - 2));
        count = (sint16)(memcpy(&wire_halfword_3, model_input->bytes + (uint32)(stream_offset), sizeof(wire_halfword_3)), wire_halfword_3);
        stream_offset += 4;
        if (kind <= 3 && count > 0)
        {
            copy_vectors = copied[kind];
            vertex_count = (kind & 1) != 0 ? 4 : 3;
            for (idx = 0; idx < count; ++idx)
            {
                POINT_T *flat_points = NULL;
                POINT_CT *gouraud_points = NULL;

                for (vertex_idx = 0; vertex_idx < vertex_count; ++vertex_idx)
                {
                    XASSERT(!(model_input->capacity < 2 || (uint32)(stream_offset + vertex_idx * 2) > model_input->capacity - 2));
                    val = (sint16)(memcpy(&wire_halfword_4, model_input->bytes + (uint32)(stream_offset + vertex_idx * 2), sizeof(wire_halfword_4)), wire_halfword_4);
                    vertex_src_offset = vertex_offset + (val << 3);
                    // Complete LWL/LWR pairs retain unaligned vertex words
                    for (word_idx = 0; word_idx < 2; ++word_idx)
                    {
                        vertex_word_offset = vertex_src_offset + word_idx * 4;
                        XASSERT(!(model_input->capacity < 4 || (uint32)((vertex_word_offset + 3) & ~3) > model_input->capacity - 4));
                        memcpy(&high, model_input->bytes + (uint32)((vertex_word_offset + 3) & ~3), sizeof(high));
                        XASSERT(!(model_input->capacity < 4 || (uint32)(vertex_word_offset & ~3) > model_input->capacity - 4));
                        memcpy(&low, model_input->bytes + (uint32)(vertex_word_offset & ~3), sizeof(low));
                        shift = (vertex_word_offset & 3) * 8;
                        words[word_idx] = shift == 0 ? low : (high << (32 - shift)) | (low >> shift);
                    }
                    for (word_idx = 0; word_idx < 2; ++word_idx)
                        copy_vectors[vertex_idx].words[word_idx] = words[word_idx];
                }
                vectors = copy_vectors;
                for (word_idx = 0; word_idx < 6; ++word_idx)
                {
                    val = vectors[word_idx / 2].words[word_idx & 1];
                    xport_gte_write_data(word_idx, val);
                }
                xport_gte_execute(0x280030);
                xport_gte_execute(0x1400006);
                area0 = (sint32)xport_gte_read_data(24);
                if ((kind & 1) != 0)
                {
                    if (kind == 1)
                    {
                        XASSERT(!(packet_offset > model_input->capacity || sizeof(POLY_FT4) > model_input->capacity - packet_offset));
                        POLY_FT4 *poly = (POLY_FT4 *)(model_input->bytes + packet_offset);
                        flat_points = poly->p;
                        gouraud_points = NULL;
                    }
                    else
                    {
                        XASSERT(!(packet_offset > model_input->capacity || sizeof(POLY_GT4) > model_input->capacity - packet_offset));
                        POLY_GT4 *poly = (POLY_GT4 *)(model_input->bytes + packet_offset);
                        gouraud_points = poly->p;
                        flat_points = NULL;
                    }
                    *(kind < 2 ? &flat_points[0].xy.xy : &gouraud_points[0].xy.xy) = xport_gte_read_data(12);
                    *(kind < 2 ? &flat_points[1].xy.xy : &gouraud_points[1].xy.xy) = xport_gte_read_data(13);
                    *(kind < 2 ? &flat_points[2].xy.xy : &gouraud_points[2].xy.xy) = xport_gte_read_data(14);
                    val = copy_vectors[3].words[0];
                    xport_gte_write_data(0, val);
                    val = copy_vectors[3].words[1];
                    xport_gte_write_data(1, val);
                    xport_gte_execute(0x180001);
                    xport_gte_execute(0x1400006);
                    area1 = (sint32)xport_gte_read_data(24);
                }
                if (kind == 2)
                    xport_gte_execute(0x158002D);
                else if (kind == 3)
                    xport_gte_execute(0x168002E);
                if (area0 <= 0 || ((kind & 1) != 0 && area1 >= 0))
                {
                    if (kind == 0)
                        xport_gte_execute(0x158002D);
                    else if (kind == 1)
                        xport_gte_execute(0x168002E);
                    if ((kind & 1) != 0)
                        *(kind < 2 ? &flat_points[3].xy.xy : &gouraud_points[3].xy.xy) = xport_gte_read_data(14);
                    else
                    {
                        if (kind == 0)
                        {
                            XASSERT(!(packet_offset > model_input->capacity || sizeof(POLY_FT3) > model_input->capacity - packet_offset));
                            POLY_FT3 *poly = (POLY_FT3 *)(model_input->bytes + packet_offset);
                            flat_points = poly->p;
                            gouraud_points = NULL;
                        }
                        else
                        {
                            XASSERT(!(packet_offset > model_input->capacity || sizeof(POLY_GT3) > model_input->capacity - packet_offset));
                            POLY_GT3 *poly = (POLY_GT3 *)(model_input->bytes + packet_offset);
                            gouraud_points = poly->p;
                            flat_points = NULL;
                        }
                        *(kind < 2 ? &flat_points[0].xy.xy : &gouraud_points[0].xy.xy) = xport_gte_read_data(12);
                        *(kind < 2 ? &flat_points[1].xy.xy : &gouraud_points[1].xy.xy) = xport_gte_read_data(13);
                        *(kind < 2 ? &flat_points[2].xy.xy : &gouraud_points[2].xy.xy) = xport_gte_read_data(14);
                    }
                    depth = (sint32)xport_gte_read_data(7);
                    if (depth >= 0)
                    {
                        shift = model_render.coord_shift;
                        far_plane = (sint32)model_render.far_depth;
                        if (depth < (far_plane >> ((shift + 2) & 31)))
                        {
                            if (kind == 0)
                                subdiv_flat_triangle((POLY_FT3 *)(model_input->bytes + packet_offset), ordering, vectors, depth);
                            else if (kind == 1)
                                subdiv_flat_quad((POLY_FT4 *)(model_input->bytes + packet_offset), ordering, vectors, depth);
                            else if (kind == 2)
                                subdiv_color_triangle((POLY_GT3 *)(model_input->bytes + packet_offset), ordering, vectors, depth);
                            else
                                subdiv_color_quad((POLY_GT4 *)(model_input->bytes + packet_offset), ordering, vectors, depth);
                        }
                    }
                }
                packet_offset += kind == 0 ? 32 : kind == 3 ? 52 : 40;
                stream_offset += kind <= 1 ? 20 : kind == 2 ? 44 : 56;
            }
        }
        XASSERT(!(model_input->capacity < 1 || (uint32)(stream_offset + 3) > model_input->capacity - 1));
        result = (model_input->bytes + (uint32)(stream_offset + 3))[0];
    } while (result != 0);
    return result;
}

static intptr draw_format10(const MODEL_RESOURCE *model_input)
{
    uint32 saved_rotation[5];
    uint32 word0, word1, word2;
    intptr result;

    FUNCTION_MARKER(0x800714A0, "SLUS_006.10");
    word0 = xport_gte_read_control(0);
    word1 = xport_gte_read_control(1);
    saved_rotation[0] = word0;
    saved_rotation[1] = word1;
    word0 = xport_gte_read_control(2);
    word1 = xport_gte_read_control(3);
    word2 = xport_gte_read_control(4);
    saved_rotation[2] = word0;
    saved_rotation[3] = word1;
    saved_rotation[4] = word2;
    word0 = model_render.billboard.words[0];
    word1 = model_render.billboard.words[1];
    xport_gte_write_control(0, word0);
    xport_gte_write_control(1, word1);
    word0 = model_render.billboard.words[2];
    word1 = model_render.billboard.words[3];
    word2 = model_render.billboard.words[4];
    xport_gte_write_control(2, word0);
    xport_gte_write_control(3, word1);
    xport_gte_write_control(4, word2);
    result = draw_format0(model_input);
    word0 = saved_rotation[0];
    word1 = saved_rotation[1];
    xport_gte_write_control(0, word0);
    xport_gte_write_control(1, word1);
    word0 = saved_rotation[2];
    word1 = saved_rotation[3];
    word2 = saved_rotation[4];
    xport_gte_write_control(2, word0);
    xport_gte_write_control(3, word1);
    xport_gte_write_control(4, word2);
    return result;
}

static intptr draw_format12(const MODEL_RESOURCE *model_input)
{
    uint16 wire_halfword_1, wire_halfword_2;

    uint32 wire_word_28, wire_word_29, wire_word_30, wire_word_31;
    POLY_FT4 *poly;
    POLY_FT4 *billboard_poly;
    uint8 *flare_buffer;
    DISPLAY_ORDERING ordering_cursor;

    MATRIX matrix = {0};
    static uint32 return_depth;
    intptr result;
    uint32 rotation[5], vertex_offsets[3];
    uint32 val0, val1, val2, angle, c, s, buffer, prim_offset, packet_offset, vertex_offset;
    uint32 idx, vertex, depth, shift, offset3, val, other, tag;
    uint32 width, height, used, capacity, brightness, color, factor;
    const uint8 *tex;
    uint32 offset_x, offset_y, center_x, center_y, product_y, half_height;
    sint32 avg_x, avg_y, sum_x, sum_y;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8006DBB4, "SLUS_006.10");
    val0 = model_render.matrix.sdk.t[1];
    val1 = model_render.matrix.sdk.t[0];
    angle = (uint32)fixed_angle_from_vector((sint32)val0, (sint32)val1);
    c = (uint32)rcos((sint32)angle);
    s = (uint32)rsin((sint32)angle);
    matrix.m[0][0] = (sint16)c;
    matrix.m[0][1] = (sint16)(0u - s);
    matrix.m[1][0] = (sint16)s;
    matrix.m[1][1] = (sint16)c;
    matrix.m[2][2] = 4096;
    MulMatrix2(&model_render.billboard.sdk, &matrix);
    memcpy(rotation, &matrix, 20);
    for (idx = 0; idx < 5; ++idx)
        xport_gte_write_control(idx, rotation[idx]);
    buffer = disp_state.bank;
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x30) > model_input->capacity - 4));
    prim_offset = ((const MODEL_HEADER *)model_input->bytes)->stream_offset;
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x34 + buffer * 4) > model_input->capacity - 4));
    memcpy(&packet_offset, model_input->bytes + (uint32)(0x34 + buffer * 4), sizeof(packet_offset));
    XASSERT(!(model_input->capacity < 4 || (uint32)(0x2C) > model_input->capacity - 4));
    vertex_offset = ((const MODEL_HEADER *)model_input->bytes)->vertex_offset;
    XASSERT(!(packet_offset > model_input->capacity || sizeof(POLY_FT4) > model_input->capacity - packet_offset));
    billboard_poly = (POLY_FT4 *)(model_input->bytes + packet_offset);
    // LEGACY FIX
    // restore neutral night-lamp modulation in the mutable packet, retaining its tint
    if (scene_level_image.bytes != NULL && scene_level_image.desc.header_offset < scene_level_image.size && scene_level_image.size - scene_level_image.desc.header_offset > 30 && (scene_level_image.bytes[scene_level_image.desc.header_offset + 30] & 1) != 0)
    {
        uint32 peak = billboard_poly->rgbc.r;
        if (billboard_poly->rgbc.g > peak)
            peak = billboard_poly->rgbc.g;
        if (billboard_poly->rgbc.b > peak)
            peak = billboard_poly->rgbc.b;
        if (peak != 0 && peak < 128)
        {
            billboard_poly->rgbc.r = (uint8)((billboard_poly->rgbc.r * 128u + peak / 2) / peak);
            billboard_poly->rgbc.g = (uint8)((billboard_poly->rgbc.g * 128u + peak / 2) / peak);
            billboard_poly->rgbc.b = (uint8)((billboard_poly->rgbc.b * 128u + peak / 2) / peak);
        }
    }
    for (vertex = 0; vertex < 3; ++vertex)
    {
        XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 4 + vertex * 2) > model_input->capacity - 2));
        vertex_offsets[vertex] = vertex_offset + (uint32)(sint16)(memcpy(&wire_halfword_1, model_input->bytes + (uint32)(prim_offset + 4 + vertex * 2), sizeof(wire_halfword_1)), wire_halfword_1) * 8;
    }
    for (vertex = 0; vertex < 3; ++vertex)
    {
        XASSERT(!(model_input->capacity < 4 || (uint32)(vertex_offsets[vertex]) > model_input->capacity - 4));
        xport_gte_write_data(vertex * 2, (memcpy(&wire_word_28, model_input->bytes + (uint32)(vertex_offsets[vertex]), sizeof(wire_word_28)), wire_word_28));
        XASSERT(!(model_input->capacity < 4 || (uint32)(vertex_offsets[vertex] + 4) > model_input->capacity - 4));
        xport_gte_write_data(vertex * 2 + 1, (memcpy(&wire_word_29, model_input->bytes + (uint32)(vertex_offsets[vertex] + 4), sizeof(wire_word_29)), wire_word_29));
    }
    xport_gte_execute(0x280030);
    billboard_poly->p[0].xy.xy = xport_gte_read_data(12);
    billboard_poly->p[1].xy.xy = xport_gte_read_data(13);
    billboard_poly->p[2].xy.xy = xport_gte_read_data(14);
    XASSERT(!(model_input->capacity < 2 || (uint32)(prim_offset + 10) > model_input->capacity - 2));
    offset3 = vertex_offset + (uint32)(sint16)(memcpy(&wire_halfword_2, model_input->bytes + (uint32)(prim_offset + 10), sizeof(wire_halfword_2)), wire_halfword_2) * 8;
    XASSERT(!(model_input->capacity < 4 || (uint32)(offset3) > model_input->capacity - 4));
    xport_gte_write_data(0, (memcpy(&wire_word_30, model_input->bytes + (uint32)(offset3), sizeof(wire_word_30)), wire_word_30));
    XASSERT(!(model_input->capacity < 4 || (uint32)(offset3 + 4) > model_input->capacity - 4));
    xport_gte_write_data(1, (memcpy(&wire_word_31, model_input->bytes + (uint32)(offset3 + 4), sizeof(wire_word_31)), wire_word_31));
    xport_gte_execute(0x180001);
    xport_gte_execute(0x168002E);
    return_depth = xport_gte_read_data(7);
    result = (intptr)&return_depth;
    depth = return_depth;
    if ((sint32)depth <= 0)
        goto restore_rotation;
    val = ((sint32)model_render.far_depth >> 2);
    result = (intptr)&billboard_poly->p[3].xy.xy;
    if ((sint32)depth >= (sint32)val)
        goto restore_rotation;
    billboard_poly->p[3].xy.xy = xport_gte_read_data(14);
    cur_frame = disp_state.cur;
    val0 = model_render.ot_shift;
    val1 = model_render.coord_shift;
    other = billboard_poly->tag;
    ordering_cursor.tags = cur_frame->ordering;
    ordering_cursor.byte_offset = 0;
    shift = val0 - val1;
    val = (uint32)((sint32)depth >> (shift & 31)) * 4;
    tag = ordering_cursor.tags[(sint32)(ordering_cursor.byte_offset + (uint32)(val)) / 4];
    billboard_poly->tag = (other & 0xFF000000) | (tag & 0xFFFFFF);
    ordering_cursor.tags = cur_frame->ordering;
    ordering_cursor.byte_offset = (uint32)(val);
    other = ordering_cursor.tags[(sint32)ordering_cursor.byte_offset / 4];
    result = (other & 0xFF000000) | gpu_packet_link((uintptr_t)billboard_poly);
    ordering_cursor.tags[(sint32)ordering_cursor.byte_offset / 4] = result;
    if ((sint32)depth >= 2048)
        goto restore_rotation;
    sum_x = billboard_poly->p[0].xy.x;
    sum_x += billboard_poly->p[1].xy.x;
    sum_x += billboard_poly->p[2].xy.x;
    sum_x += billboard_poly->p[3].xy.x;
    avg_x = sum_x / 4;
    sum_y = billboard_poly->p[0].xy.y;
    sum_y += billboard_poly->p[1].xy.y;
    sum_y += billboard_poly->p[2].xy.y;
    sum_y += billboard_poly->p[3].xy.y;
    result = sum_y < 0 ? sum_y + 3 : sum_y;
    avg_y = sum_y / 4;
    if (avg_x < 0)
        goto restore_rotation;
    width = model_render.width;
    result = avg_x < (sint32)width;
    if (result == 0 || avg_y < 0)
        goto restore_rotation;
    height = model_render.height;
    result = avg_y < (sint32)height;
    if (result == 0)
        goto restore_rotation;
    used = packet_pool.used;
    capacity = packet_pool.limit;
    val = used + 80;
    result = width >> 31;
    if (capacity < val)
        goto restore_rotation;
    offset_x = (uint32)avg_x - (uint32)((sint32)width / 2);
    offset_y = (uint32)avg_y - (uint32)((sint32)height / 2);
    buffer = disp_state.bank;
    flare_buffer = packet_pool.buffers[buffer];
    packet_pool.used = val;
    poly = (POLY_FT4 *)(flare_buffer + used);
    SquareRoot0((sint32)(offset_x * offset_x + offset_y * offset_y));
    brightness = ((sint32)(2048u - depth) / 16);
    color = ((brightness << 16) | (brightness << 8) | brightness) & 0xFFFFFF;
    color |= 0x2E000000;
    for (idx = 0; idx < 2; ++idx)
    {
        factor = (uint32)flare_factors[idx];
        val0 = offset_x * factor;
        width = model_render.width;
        tex = flare_tex[idx];
        center_x = (uint32)((sint32)width / 2) + (uint32)((sint32)val0 / 256);
        product_y = offset_y * factor;
        height = model_render.height;
        half_height = ((sint32)height / 2);
        poly->tag = (uint32)(0x09000000);
        poly->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
        val = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        poly->rgbc.rgbc = (uint32)(color);
        poly->p[0].attr = (uint16)(val);
        poly->p[0].uv.u = (uint8)(tex[14]);
        poly->p[0].uv.v = (uint8)(tex[15]);
        val = tex[14];
        other = tex[12];
        poly->p[1].uv.u = (uint8)(other + val + 255);
        poly->p[1].uv.v = (uint8)(tex[15]);
        poly->p[2].uv.u = (uint8)(tex[14]);
        val = tex[15];
        other = tex[13];
        poly->p[2].uv.v = (uint8)(other + val + 255);
        val = tex[14];
        other = tex[12];
        poly->p[3].uv.u = (uint8)(other + val + 255);
        val = tex[15];
        other = tex[13];
        poly->p[3].uv.v = (uint8)(other + val + 255);
        poly->p[0].xy.x = (sint16)(center_x - (tex[12] >> 1));
        val = tex[13];
        center_y = half_height + (uint32)((sint32)product_y / 256);
        poly->p[0].xy.y = (sint16)(center_y - (val >> 1));
        poly->p[1].xy.x = (sint16)(center_x + (tex[12] >> 1) - 1);
        poly->p[1].xy.y = (sint16)(center_y - (tex[13] >> 1));
        poly->p[2].xy.x = (sint16)(center_x - (tex[12] >> 1));
        poly->p[2].xy.y = (sint16)(center_y + (tex[13] >> 1) - 1);
        poly->p[3].xy.x = (sint16)(center_x + (tex[12] >> 1) - 1);
        val = tex[13];
        cur_frame = disp_state.cur;
        poly->p[3].xy.y = (sint16)(center_y + (val >> 1) - 1);
        ordering_cursor.tags = cur_frame->ordering;
        ordering_cursor.byte_offset = 0;
        other = poly->tag;
        tag = ordering_cursor.tags[(sint32)(ordering_cursor.byte_offset + (uint32)(4)) / 4];
        poly->tag = (uint32)((other & 0xFF000000) | (tag & 0xFFFFFF));
        ordering_cursor.tags = cur_frame->ordering;
        ordering_cursor.byte_offset = 0;
        tag = ordering_cursor.tags[(sint32)(ordering_cursor.byte_offset + (uint32)(4)) / 4];
        ordering_cursor.tags[(sint32)(ordering_cursor.byte_offset + (uint32)(4)) / 4] = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
        result = idx + 1 < 2;
        ++poly;
    }
restore_rotation:
    val0 = model_render.view.words[0];
    val1 = model_render.view.words[1];
    xport_gte_write_control(0, val0);
    xport_gte_write_control(1, val1);
    val0 = model_render.view.words[2];
    val1 = model_render.view.words[3];
    val2 = model_render.view.words[4];
    xport_gte_write_control(2, val0);
    xport_gte_write_control(3, val1);
    xport_gte_write_control(4, val2);
    return result;
}

static intptr draw_format0_near(const MODEL_RESOURCE *model_input)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10;

    const MODEL_VERTEX *vertex_input;
    uint32 *output_tag;
    POINT_CT *output_points;
    POLY_GT3 *poly3;
    POLY_GT4 *poly4;
    const POINT_T *template_points;
    DISPLAY_ORDERING ordering_base;
    VECTOR sq;
    const uint32 *colors;
    uint32 *color_output;
    uint32 coord, reference;
    VEC3 delta;
    uint32 bank, shift;
    uint32 template_packet_offset, rec_offset, vertex_offset, count, idx;
    uint32 sum, threshold, val, type, packet_bytes, template_bytes, offset, limit;
    const uint32 *color_base;
    uint32 used, indices[4], offset0, offset1, offset2, offset3, tint;
    sint32 model_depth, far_depth, polygon_count, polygon, clip0, clip1, depth;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80070020, "SLUS_006.10");
    XASSERT(!(model_input->capacity < 1 || (uint32)(0) > model_input->capacity - 1));
    colors = (model_input->bytes + (uint32)(0))[0] == 0 ? lighting.colors[0] : lighting.colors[1];
    XASSERT(!(model_input->capacity < 4 || (uint32)(16) > model_input->capacity - 4));
    coord = ((const MODEL_HEADER *)model_input->bytes)->position.bits.x;
    reference = model_render.distance_reference.bits.x;
    color_output = lighting.scratch;
    delta.bits.x = (uint32)((sint32)coord >> 8) - reference;
    XASSERT(!(model_input->capacity < 4 || (uint32)(20) > model_input->capacity - 4));
    coord = ((const MODEL_HEADER *)model_input->bytes)->position.bits.y;
    reference = model_render.distance_reference.bits.y;
    val = model_render.distance_reference.bits.z;
    delta.bits.y = (uint32)((sint32)coord >> 8) - reference;
    XASSERT(!(model_input->capacity < 4 || (uint32)(24) > model_input->capacity - 4));
    coord = ((const MODEL_HEADER *)model_input->bytes)->position.bits.z;
    bank = disp_state.bank;
    shift = model_render.ot_shift;
    delta.bits.z = (uint32)((sint32)coord >> 8) - val;
    XASSERT(!(model_input->capacity < 4 || (uint32)(52 + bank * 4) > model_input->capacity - 4));
    memcpy(&template_packet_offset, model_input->bytes + (uint32)(52 + bank * 4), sizeof(template_packet_offset));
    XASSERT(!(model_input->capacity < 4 || (uint32)(48) > model_input->capacity - 4));
    rec_offset = ((const MODEL_HEADER *)model_input->bytes)->stream_offset;
    XASSERT(!(model_input->capacity < 4 || (uint32)(44) > model_input->capacity - 4));
    vertex_offset = ((const MODEL_HEADER *)model_input->bytes)->vertex_offset;
    far_depth = (sint32)model_render.far_depth >> 3;
    shift -= model_render.coord_shift;
    cur_frame = disp_state.cur;
    XASSERT(!(model_input->capacity < 2 || (uint32)(12) > model_input->capacity - 2));
    model_depth = (sint16)(memcpy(&wire_halfword_1, model_input->bytes + (uint32)(12), sizeof(wire_halfword_1)), wire_halfword_1);
    ordering_base.tags = cur_frame->ordering;
    ordering_base.byte_offset = (uint32)(((uint32)(model_depth >> (shift & 31)) << 2));
    count = used_vertex_count(model_input);
    XASSERT(!(model_input->capacity < 2 || 14 > model_input->capacity - 2));
    (((MODEL_HEADER *)model_input->bytes)->visible_vertices = (uint16)count);
    if ((sint32)(count << 16) > 0)
    {
        idx = 0;
        do
        {
            XASSERT(!(model_input->capacity < 2 || (uint32)(vertex_offset + idx * 8) > model_input->capacity - 2));
            sq.vx = (sint32)(delta.bits.x + (uint32)(sint16)(memcpy(&wire_halfword_2, model_input->bytes + (uint32)(vertex_offset + idx * 8), sizeof(wire_halfword_2)), wire_halfword_2));
            XASSERT(!(model_input->capacity < 2 || (uint32)(vertex_offset + idx * 8 + 2) > model_input->capacity - 2));
            sq.vy = (sint32)(delta.bits.y + (uint32)(sint16)(memcpy(&wire_halfword_3, model_input->bytes + (uint32)(vertex_offset + idx * 8 + 2), sizeof(wire_halfword_3)), wire_halfword_3));
            XASSERT(!(model_input->capacity < 2 || (uint32)(vertex_offset + idx * 8 + 4) > model_input->capacity - 2));
            sq.vz = (sint32)(delta.bits.z + (uint32)(sint16)(memcpy(&wire_halfword_4, model_input->bytes + (uint32)(vertex_offset + idx * 8 + 4), sizeof(wire_halfword_4)), wire_halfword_4));
            Square0(&sq, &sq);
            threshold = lighting.radius_sq;
            sum = (uint32)sq.vx + (uint32)sq.vy + (uint32)sq.vz;
            if (threshold < sum)
                val = colors[1023];
            else
                val = colors[(uint32)((sint32)sum >> 15)];
            XASSERT(!(idx >= 256));
            *color_output = val;
            ++color_output;
            ++idx;
            XASSERT(!(model_input->capacity < 2 || (uint32)(14) > model_input->capacity - 2));
        } while ((sint32)idx < (sint16)(memcpy(&wire_halfword_5, model_input->bytes + (uint32)(14), sizeof(wire_halfword_5)), wire_halfword_5));
    }
    for (;;)
    {
        XASSERT(!(model_input->capacity < 1 || (uint32)(rec_offset + 3) > model_input->capacity - 1));
        if (model_input->bytes[(uint32)(rec_offset + 3)] == 0)
            break;
        XASSERT(!(model_input->capacity < 1 || (uint32)(rec_offset + 2) > model_input->capacity - 1));
        type = (model_input->bytes + (uint32)(rec_offset + 2))[0];
        XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset) > model_input->capacity - 2));
        polygon_count = (sint16)(memcpy(&wire_halfword_6, model_input->bytes + (uint32)(rec_offset), sizeof(wire_halfword_6)), wire_halfword_6);
        rec_offset += 4;
        if (type > 1)
            continue;
        packet_bytes = type == 0 ? 40 : 52;
        template_bytes = type == 0 ? 32 : 40;
        offset = packet_pool.used;
        limit = packet_pool.limit;
        if (offset + (uint32)polygon_count * packet_bytes >= limit)
            continue;
        bank = disp_state.bank;
        poly3 = type == 0 ? (POLY_GT3 *)(packet_pool.buffers[bank] + limit) : NULL;
        poly4 = type == 0 ? NULL : (POLY_GT4 *)(packet_pool.buffers[bank] + limit);
        used = 0;
        for (polygon = 0; polygon < polygon_count; ++polygon)
        {
            XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset) > model_input->capacity - 2));
            indices[0] = (sint16)(memcpy(&wire_halfword_7, model_input->bytes + (uint32)(rec_offset), sizeof(wire_halfword_7)), wire_halfword_7);
            XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + 2) > model_input->capacity - 2));
            indices[1] = (sint16)(memcpy(&wire_halfword_8, model_input->bytes + (uint32)(rec_offset + 2), sizeof(wire_halfword_8)), wire_halfword_8);
            XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + 4) > model_input->capacity - 2));
            indices[2] = (sint16)(memcpy(&wire_halfword_9, model_input->bytes + (uint32)(rec_offset + 4), sizeof(wire_halfword_9)), wire_halfword_9);
            if (type == 1)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + 6) > model_input->capacity - 2));
                indices[3] = (sint16)(memcpy(&wire_halfword_10, model_input->bytes + (uint32)(rec_offset + 6), sizeof(wire_halfword_10)), wire_halfword_10);
            }
            offset0 = vertex_offset + (indices[0] << 3);
            offset1 = vertex_offset + (indices[1] << 3);
            offset2 = vertex_offset + (indices[2] << 3);
            XASSERT(!(offset0 > model_input->capacity || sizeof(SVECTOR) > model_input->capacity - offset0));
            XASSERT(!(offset1 > model_input->capacity || sizeof(SVECTOR) > model_input->capacity - offset1));
            XASSERT(!(offset2 > model_input->capacity || sizeof(SVECTOR) > model_input->capacity - offset2));
            project_triangle((const SVECTOR *)(model_input->bytes + offset0), (const SVECTOR *)(model_input->bytes + offset1), (const SVECTOR *)(model_input->bytes + offset2));
            xport_gte_execute(0x1400006);
            clip0 = (sint32)xport_gte_read_data(24);
            if (type == 0)
            {
                xport_gte_execute(0x158002D);
                if (clip0 > 0)
                    goto next_colored_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_colored_polygon;
                --poly3;
                output_tag = &poly3->tag;
                output_points = poly3->p;
                ++used;
                XASSERT(!(model_input->capacity < 4 || (uint32)(rec_offset + 8) > model_input->capacity - 4));
                memcpy(&tint, model_input->bytes + (uint32)(rec_offset + 8), sizeof(tint));
                output_points[0].xy.xy = (uint32)(xport_gte_read_data(12));
                output_points[1].xy.xy = (uint32)(xport_gte_read_data(13));
                output_points[2].xy.xy = (uint32)(xport_gte_read_data(14));
            }
            else
            {
                --poly4;
                output_tag = &poly4->tag;
                output_points = poly4->p;
                ++used;
                output_points[0].xy.xy = (uint32)(xport_gte_read_data(12));
                output_points[1].xy.xy = (uint32)(xport_gte_read_data(13));
                output_points[2].xy.xy = (uint32)(xport_gte_read_data(14));
                offset3 = vertex_offset + (indices[3] << 3);
                XASSERT(!((uint32)(offset3) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(offset3)));
                vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(offset3));
                xport_gte_write_data(0, vertex_input->words[0]);
                xport_gte_write_data(1, vertex_input->words[1]);
                xport_gte_execute(0x180001);
                xport_gte_execute(0x1400006);
                clip1 = (sint32)xport_gte_read_data(24);
                xport_gte_execute(0x168002E);
                if (clip0 > 0 && clip1 < 0)
                    goto next_colored_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_colored_polygon;
                XASSERT(!(model_input->capacity < 4 || (uint32)(rec_offset + 8) > model_input->capacity - 4));
                memcpy(&tint, model_input->bytes + (uint32)(rec_offset + 8), sizeof(tint));
                output_points[3].xy.xy = (uint32)(xport_gte_read_data(14));
            }
            color_base = lighting.scratch;
            XASSERT(!(indices[0] >= 256));
            output_points[0].rgbc.rgbc = (uint32)(color_base[indices[0]] + tint);
            XASSERT(!(indices[1] >= 256));
            output_points[1].rgbc.rgbc = (uint32)(color_base[indices[1]] + tint);
            if (type == 1)
            {
                XASSERT(!(indices[2] >= 256));
                output_points[2].rgbc.rgbc = (uint32)(color_base[indices[2]] + tint);
            }
            XASSERT(!(indices[type == 0 ? 2 : 3] >= 256));
            val = color_base[indices[type == 0 ? 2 : 3]];
            setlen(output_tag, type == 0 ? 9 : 12);
            setcode(output_tag, type == 0 ? 52 : 60);
            output_points[type == 0 ? 2 : 3].rgbc.rgbc = (uint32)(val + tint);
            if (type == 0)
            {
                XASSERT(!((uint32)(template_packet_offset) > model_input->capacity || sizeof(POLY_FT3) > model_input->capacity - (uint32)(template_packet_offset)));
                template_points = ((const POLY_FT3 *)(model_input->bytes + (uint32)(template_packet_offset)))->p;
            }
            else
            {
                XASSERT(!((uint32)(template_packet_offset) > model_input->capacity || sizeof(POLY_FT4) > model_input->capacity - (uint32)(template_packet_offset)));
                template_points = ((const POLY_FT4 *)(model_input->bytes + (uint32)(template_packet_offset)))->p;
            }
            output_points[0].uv_attr = (uint32)(template_points[0].uv_attr);
            output_points[1].uv_attr = (uint32)(template_points[1].uv_attr);
            output_points[2].uv_attr = (uint32)(template_points[2].uv_attr);
            if (type == 1)
                output_points[3].uv_attr = (uint32)(template_points[3].uv_attr);
            link_packet(output_tag, ordering_base, depth, shift);
        next_colored_polygon:
            template_packet_offset += template_bytes;
            rec_offset += 20;
        }
        val = packet_pool.limit;
        packet_pool.limit = val - used * packet_bytes;
    }
    return 0;
}

static intptr draw_format4_near(const MODEL_RESOURCE *model_input)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;

    const MODEL_VERTEX *vertex_input;
    uint32 *output_tag;
    POINT_CT *output_points;
    POLY_GT3 *poly3;
    POLY_GT4 *poly4;
    const POINT_T *template_points;
    DISPLAY_ORDERING ordering_base;
    VECTOR sq;
    MODEL_VERTEX snapshots[2][4];
    MODEL_VERTEX *snapshot;
    VEC3 light_pos;
    uint32 component, word0, word1, template_src_offset;
    const uint32 *colors;
    uint32 *color_output;
    uint32 coord, reference;
    VEC3 delta;
    uint32 bank, shift;
    uint32 template_packet_offset, rec_offset, vertex_offset, count, idx;
    uint32 sum, threshold, val, type, packet_bytes, template_bytes, offset, limit;
    const uint32 *color_base;
    uint32 used, indices[4], point_offset, tint;
    sint32 model_depth, far_depth, polygon_count, polygon, clip0, clip1, depth;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80070704, "SLUS_006.10");
    XASSERT(!(model_input->capacity < 1 || (uint32)(0) > model_input->capacity - 1));
    colors = (model_input->bytes + (uint32)(0))[0] == 0 ? lighting.colors[0] : lighting.colors[1];
    XASSERT(!(model_input->capacity < 4 || (uint32)(16) > model_input->capacity - 4));
    coord = ((const MODEL_HEADER *)model_input->bytes)->position.bits.x;
    reference = model_render.distance_reference.bits.x;
    color_output = lighting.scratch;
    delta.bits.x = (uint32)((sint32)coord >> 8) - reference;
    XASSERT(!(model_input->capacity < 4 || (uint32)(20) > model_input->capacity - 4));
    coord = ((const MODEL_HEADER *)model_input->bytes)->position.bits.y;
    reference = model_render.distance_reference.bits.y;
    val = model_render.distance_reference.bits.z;
    delta.bits.y = (uint32)((sint32)coord >> 8) - reference;
    XASSERT(!(model_input->capacity < 4 || (uint32)(24) > model_input->capacity - 4));
    coord = ((const MODEL_HEADER *)model_input->bytes)->position.bits.z;
    bank = disp_state.bank;
    delta.bits.z = (uint32)((sint32)coord >> 8) - val;
    XASSERT(!(model_input->capacity < 4 || (uint32)(52 + bank * 4) > model_input->capacity - 4));
    memcpy(&template_packet_offset, model_input->bytes + (uint32)(52 + bank * 4), sizeof(template_packet_offset));
    XASSERT(!(model_input->capacity < 4 || (uint32)(48) > model_input->capacity - 4));
    rec_offset = ((const MODEL_HEADER *)model_input->bytes)->stream_offset;
    XASSERT(!(model_input->capacity < 4 || (uint32)(44) > model_input->capacity - 4));
    vertex_offset = ((const MODEL_HEADER *)model_input->bytes)->vertex_offset;
    cur_frame = disp_state.cur;
    far_depth = (sint32)model_render.far_depth >> 3;
    shift = model_render.ot_shift;
    shift -= model_render.coord_shift;
    XASSERT(!(model_input->capacity < 2 || (uint32)(12) > model_input->capacity - 2));
    model_depth = (sint16)(memcpy(&wire_halfword_1, model_input->bytes + (uint32)(12), sizeof(wire_halfword_1)), wire_halfword_1);
    ordering_base.tags = cur_frame->ordering;
    ordering_base.byte_offset = (uint32)(((uint32)(model_depth >> (shift & 31)) << 2));
    count = used_vertex_count(model_input);
    XASSERT(!(model_input->capacity < 2 || 14 > model_input->capacity - 2));
    (((MODEL_HEADER *)model_input->bytes)->visible_vertices = (uint16)count);
    if ((sint32)(count << 16) > 0)
    {
        idx = 0;
        do
        {
            for (component = 0; component < 3; ++component)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(vertex_offset + idx * 8 + component * 2) > model_input->capacity - 2));
                coord = (sint16)(memcpy(&wire_halfword_2, model_input->bytes + (uint32)(vertex_offset + idx * 8 + component * 2), sizeof(wire_halfword_2)), wire_halfword_2);
                reference = delta.u[component];
                light_pos.u[component] = reference + coord;
            }
            sq.vx = (sint32)light_pos.bits.x;
            sq.vy = (sint32)light_pos.bits.y;
            sq.vz = (sint32)light_pos.bits.z;
            Square0(&sq, &sq);
            sum = (uint32)sq.vx;
            sum += (uint32)sq.vy;
            sum += (uint32)sq.vz;
            threshold = lighting.radius_sq;

            if (threshold < sum)
                val = colors[1023];
            else
                val = colors[(uint32)((sint32)sum >> 15)];
            XASSERT(!(idx >= 256));
            *color_output = val;
            ++color_output;
            ++idx;
            XASSERT(!(model_input->capacity < 2 || (uint32)(14) > model_input->capacity - 2));
        } while ((sint32)idx < (sint16)(memcpy(&wire_halfword_3, model_input->bytes + (uint32)(14), sizeof(wire_halfword_3)), wire_halfword_3));
    }
    for (;;)
    {
        XASSERT(!(model_input->capacity < 1 || (uint32)(rec_offset + 3) > model_input->capacity - 1));
        if (model_input->bytes[(uint32)(rec_offset + 3)] == 0)
            break;
        XASSERT(!(model_input->capacity < 1 || (uint32)(rec_offset + 2) > model_input->capacity - 1));
        type = (model_input->bytes + (uint32)(rec_offset + 2))[0];
        XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset) > model_input->capacity - 2));
        polygon_count = (sint16)(memcpy(&wire_halfword_4, model_input->bytes + (uint32)(rec_offset), sizeof(wire_halfword_4)), wire_halfword_4);
        rec_offset += 4;
        if (type > 1)
            continue;
        packet_bytes = type == 0 ? 40 : 52;
        template_bytes = type == 0 ? 32 : 40;
        offset = packet_pool.used;
        limit = packet_pool.limit;
        if (offset + (uint32)polygon_count * packet_bytes >= limit)
            continue;
        bank = disp_state.bank;
        poly3 = type == 0 ? (POLY_GT3 *)(packet_pool.buffers[bank] + limit) : NULL;
        poly4 = type == 0 ? NULL : (POLY_GT4 *)(packet_pool.buffers[bank] + limit);
        used = 0;
        // Protect pending reverse packets from nested subdivision
        if (polygon_count > 0)
            packet_pool.limit = limit - (uint32)polygon_count * packet_bytes;
        template_src_offset = template_packet_offset;
        for (polygon = 0; polygon < polygon_count; ++polygon)
        {
            XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset) > model_input->capacity - 2));
            indices[0] = (sint16)(memcpy(&wire_halfword_5, model_input->bytes + (uint32)(rec_offset), sizeof(wire_halfword_5)), wire_halfword_5);
            XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + 2) > model_input->capacity - 2));
            indices[1] = (sint16)(memcpy(&wire_halfword_6, model_input->bytes + (uint32)(rec_offset + 2), sizeof(wire_halfword_6)), wire_halfword_6);
            XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + 4) > model_input->capacity - 2));
            indices[2] = (sint16)(memcpy(&wire_halfword_7, model_input->bytes + (uint32)(rec_offset + 4), sizeof(wire_halfword_7)), wire_halfword_7);
            if (type == 1)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + 6) > model_input->capacity - 2));
                indices[3] = (sint16)(memcpy(&wire_halfword_8, model_input->bytes + (uint32)(rec_offset + 6), sizeof(wire_halfword_8)), wire_halfword_8);
            }
            snapshot = snapshots[type];
            for (component = 0; component < (type == 0 ? 3u : 4u); ++component)
            {
                point_offset = vertex_offset + (indices[component] << 3);
                XASSERT(!((uint32)(point_offset) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(point_offset)));
                vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(point_offset));
                word0 = vertex_input->words[0];
                word1 = vertex_input->words[1];
                snapshot[component].words[0] = word0;
                snapshot[component].words[1] = word1;
            }
            project_triangle(&snapshot[0].sdk, &snapshot[1].sdk, &snapshot[2].sdk);
            xport_gte_execute(0x1400006);
            if (type == 0)
            {
                clip0 = (sint32)xport_gte_read_data(24);
                xport_gte_execute(0x158002D);
                if (clip0 > 0)
                    goto next_subdivided_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_subdivided_polygon;
                --poly3;
                output_tag = &poly3->tag;
                output_points = poly3->p;
                ++used;
                XASSERT(!(model_input->capacity < 4 || (uint32)(rec_offset + 8) > model_input->capacity - 4));
                memcpy(&tint, model_input->bytes + (uint32)(rec_offset + 8), sizeof(tint));
                output_points[0].xy.xy = (uint32)(xport_gte_read_data(12));
                output_points[1].xy.xy = (uint32)(xport_gte_read_data(13));
                output_points[2].xy.xy = (uint32)(xport_gte_read_data(14));
            }
            else
            {
                --poly4;
                output_tag = &poly4->tag;
                output_points = poly4->p;
                ++used;
                clip0 = (sint32)xport_gte_read_data(24);
                output_points[0].xy.xy = (uint32)(xport_gte_read_data(12));
                output_points[1].xy.xy = (uint32)(xport_gte_read_data(13));
                output_points[2].xy.xy = (uint32)(xport_gte_read_data(14));
                xport_gte_write_data(0, snapshot[3].words[0]);
                xport_gte_write_data(1, snapshot[3].words[1]);
                xport_gte_execute(0x180001);
                xport_gte_execute(0x1400006);
                clip1 = (sint32)xport_gte_read_data(24);
                xport_gte_execute(0x168002E);
                if (clip0 > 0 && clip1 < 0)
                    goto next_subdivided_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_subdivided_polygon;
                XASSERT(!(model_input->capacity < 4 || (uint32)(rec_offset + 8) > model_input->capacity - 4));
                memcpy(&tint, model_input->bytes + (uint32)(rec_offset + 8), sizeof(tint));
                output_points[3].xy.xy = (uint32)(xport_gte_read_data(14));
            }
            color_base = lighting.scratch;
            XASSERT(!(indices[0] >= 256));
            output_points[0].rgbc.rgbc = (uint32)(color_base[indices[0]] + tint);
            XASSERT(!(indices[1] >= 256));
            output_points[1].rgbc.rgbc = (uint32)(color_base[indices[1]] + tint);
            if (type == 1)
            {
                XASSERT(!(indices[2] >= 256));
                output_points[2].rgbc.rgbc = (uint32)(color_base[indices[2]] + tint);
            }
            XASSERT(!(indices[type == 0 ? 2 : 3] >= 256));
            val = color_base[indices[type == 0 ? 2 : 3]];
            setlen(output_tag, type == 0 ? 9 : 12);
            setcode(output_tag, type == 0 ? 52 : 60);
            output_points[type == 0 ? 2 : 3].rgbc.rgbc = (uint32)(val + tint);
            if (type == 0)
            {
                XASSERT(!((uint32)(template_src_offset) > model_input->capacity || sizeof(POLY_FT3) > model_input->capacity - (uint32)(template_src_offset)));
                template_points = ((const POLY_FT3 *)(model_input->bytes + (uint32)(template_src_offset)))->p;
            }
            else
            {
                XASSERT(!((uint32)(template_src_offset) > model_input->capacity || sizeof(POLY_FT4) > model_input->capacity - (uint32)(template_src_offset)));
                template_points = ((const POLY_FT4 *)(model_input->bytes + (uint32)(template_src_offset)))->p;
            }
            output_points[0].uv_attr = (uint32)(template_points[0].uv_attr);
            output_points[1].uv_attr = (uint32)(template_points[1].uv_attr);
            output_points[2].uv_attr = (uint32)(template_points[2].uv_attr);
            if (type == 1)
                output_points[3].uv_attr = (uint32)(template_points[3].uv_attr);

            if (type == 0)
                subdiv_lit_triangle(poly3, ordering_base, snapshot, depth, &delta, colors, tint);
            else
                subdiv_lit_quad(poly4, ordering_base, snapshot, depth, &delta, colors, tint);

        next_subdivided_polygon:
            template_src_offset += template_bytes;
            template_packet_offset += template_bytes;
            rec_offset += 20;
        }
        val = limit;
        packet_pool.limit = val - used * packet_bytes;
    }
    return 0;
}

static intptr draw_format10_near(const MODEL_RESOURCE *model_input)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6;

    const MODEL_VERTEX *vertex_input;
    DISPLAY_ORDERING head_address, ordering_table;
    POLY_FT3 *poly3;
    POLY_FT4 *poly4;
    POINT_T *packet_points;
    RGBC *packet_color;
    uint32 *packet_tag;
    VECTOR sq;
    const uint32 *colors;
    uint32 val0, val1, val2, val3, coord, reference, z_reference;
    uint32 delta_x, delta_y, delta_z, bank, template_offset, vertices_offset, records_offset;
    uint32 shift, sum, threshold, color, packet_offset, vertex_offset, rec_offset;
    uint32 type, packet_bytes, tint, tag, head, rotation[3];
    sint32 center_y, center_z, center_x, model_depth, far_depth, count, polygon;
    sint32 clip0, clip1, depth;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80070ED0, "SLUS_006.10");
    XASSERT(!(model_input->capacity < 1 || (uint32)(0) > model_input->capacity - 1));
    colors = (model_input->bytes + (uint32)(0))[0] == 0 ? lighting.colors[0] : lighting.colors[1];
    PushMatrix();
    val0 = model_render.billboard.words[0];
    val1 = model_render.billboard.words[1];
    xport_gte_write_control(0, val0);
    xport_gte_write_control(1, val1);
    rotation[0] = model_render.billboard.words[2];
    rotation[1] = model_render.billboard.words[3];
    rotation[2] = model_render.billboard.words[4];
    xport_gte_write_control(2, rotation[0]);
    xport_gte_write_control(3, rotation[1]);
    xport_gte_write_control(4, rotation[2]);
    XASSERT(!(model_input->capacity < 4 || (uint32)(16) > model_input->capacity - 4));
    coord = ((const MODEL_HEADER *)model_input->bytes)->position.bits.x;
    reference = model_render.distance_reference.bits.x;
    delta_x = (uint32)((sint32)coord >> 8) - reference;
    XASSERT(!(model_input->capacity < 4 || (uint32)(20) > model_input->capacity - 4));
    coord = ((const MODEL_HEADER *)model_input->bytes)->position.bits.y;
    reference = model_render.distance_reference.bits.y;
    z_reference = model_render.distance_reference.bits.z;
    delta_y = (uint32)((sint32)coord >> 8) - reference;
    XASSERT(!(model_input->capacity < 4 || (uint32)(24) > model_input->capacity - 4));
    coord = ((const MODEL_HEADER *)model_input->bytes)->position.bits.z;
    center_y = near_billboard_center.y;
    bank = disp_state.bank;
    center_z = near_billboard_center.z;
    delta_z = (uint32)((sint32)coord >> 8) - z_reference;
    XASSERT(!(model_input->capacity < 4 || (uint32)(52 + bank * 4) > model_input->capacity - 4));
    memcpy(&template_offset, model_input->bytes + (uint32)(52 + bank * 4), sizeof(template_offset));
    cur_frame = disp_state.cur;
    XASSERT(!(model_input->capacity < 4 || (uint32)(44) > model_input->capacity - 4));
    vertices_offset = ((const MODEL_HEADER *)model_input->bytes)->vertex_offset;
    XASSERT(!(model_input->capacity < 4 || (uint32)(48) > model_input->capacity - 4));
    records_offset = ((const MODEL_HEADER *)model_input->bytes)->stream_offset;
    XASSERT(!(model_input->capacity < 2 || (uint32)(12) > model_input->capacity - 2));
    model_depth = (sint16)(memcpy(&wire_halfword_1, model_input->bytes + (uint32)(12), sizeof(wire_halfword_1)), wire_halfword_1);
    ordering_table.tags = cur_frame->ordering;
    ordering_table.byte_offset = 0;
    center_x = near_billboard_center.x;
    sq.vy = (sint32)(delta_y + (uint32)center_y);
    shift = model_render.ot_shift;
    sq.vz = (sint32)(delta_z + (uint32)center_z);
    packet_offset = template_offset;
    far_depth = (sint32)model_render.far_depth >> 3;
    sq.vx = (sint32)(delta_x + (uint32)center_x);
    shift -= model_render.coord_shift;
    rec_offset = records_offset;
    vertex_offset = vertices_offset;
    ordering_table.byte_offset += (uint32)(model_depth >> (shift & 31)) << 2;
    Square0(&sq, &sq);
    threshold = lighting.radius_sq;
    sum = (uint32)sq.vx + (uint32)sq.vy + (uint32)sq.vz;
    color = colors[threshold < sum ? 1023 : (uint32)((sint32)sum >> 15)];
    for (;;)
    {
        XASSERT(!(model_input->capacity < 1 || (uint32)(rec_offset + 3) > model_input->capacity - 1));
        if (model_input->bytes[(uint32)(rec_offset + 3)] == 0)
            break;
        XASSERT(!(model_input->capacity < 1 || (uint32)(rec_offset + 2) > model_input->capacity - 1));
        type = (model_input->bytes + (uint32)(rec_offset + 2))[0];
        XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset) > model_input->capacity - 2));
        count = (sint16)(memcpy(&wire_halfword_2, model_input->bytes + (uint32)(rec_offset), sizeof(wire_halfword_2)), wire_halfword_2);
        rec_offset += 4;
        if (type > 1)
            continue;
        packet_bytes = type == 0 ? 32 : 40;
        for (polygon = 0; polygon < count; ++polygon)
        {
            XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset) > model_input->capacity - 2));
            val0 = (sint16)(memcpy(&wire_halfword_3, model_input->bytes + (uint32)(rec_offset), sizeof(wire_halfword_3)), wire_halfword_3);
            XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + 2) > model_input->capacity - 2));
            val1 = (sint16)(memcpy(&wire_halfword_4, model_input->bytes + (uint32)(rec_offset + 2), sizeof(wire_halfword_4)), wire_halfword_4);
            XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + 4) > model_input->capacity - 2));
            val2 = (sint16)(memcpy(&wire_halfword_5, model_input->bytes + (uint32)(rec_offset + 4), sizeof(wire_halfword_5)), wire_halfword_5);
            if (type == 1)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + 6) > model_input->capacity - 2));
                val3 = (sint16)(memcpy(&wire_halfword_6, model_input->bytes + (uint32)(rec_offset + 6), sizeof(wire_halfword_6)), wire_halfword_6);
            }
            val0 = vertex_offset + (val0 << 3);
            val1 = vertex_offset + (val1 << 3);
            val2 = vertex_offset + (val2 << 3);
            XASSERT(!((uint32)(val0) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(val0)));
            vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(val0));
            xport_gte_write_data(0, vertex_input->words[0]);
            xport_gte_write_data(1, vertex_input->words[1]);
            XASSERT(!((uint32)(val1) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(val1)));
            vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(val1));
            xport_gte_write_data(2, vertex_input->words[0]);
            xport_gte_write_data(3, vertex_input->words[1]);
            XASSERT(!((uint32)(val2) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(val2)));
            vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(val2));
            xport_gte_write_data(4, vertex_input->words[0]);
            xport_gte_write_data(5, vertex_input->words[1]);
            xport_gte_execute(0x280030);
            xport_gte_execute(0x1400006);
            clip0 = (sint32)xport_gte_read_data(24);
            if (type == 0)
            {
                xport_gte_execute(0x158002D);
                if (clip0 > 0)
                    goto next_flat_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_flat_polygon;
                XASSERT(!(model_input->capacity < 4 || (uint32)(rec_offset + 8) > model_input->capacity - 4));
                memcpy(&tint, model_input->bytes + (uint32)(rec_offset + 8), sizeof(tint));
                XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT3) > model_input->capacity - (uint32)(packet_offset)));
                poly3 = (POLY_FT3 *)(model_input->bytes + (uint32)(packet_offset));
                packet_tag = &poly3->tag;
                packet_points = poly3->p;
                packet_color = &poly3->rgbc;
                packet_points[0].xy.xy = xport_gte_read_data(12);
                packet_points[1].xy.xy = xport_gte_read_data(13);
                packet_points[2].xy.xy = xport_gte_read_data(14);
            }
            else
            {
                XASSERT(!((uint32)(packet_offset) > model_input->capacity || sizeof(POLY_FT4) > model_input->capacity - (uint32)(packet_offset)));
                poly4 = (POLY_FT4 *)(model_input->bytes + (uint32)(packet_offset));
                packet_tag = &poly4->tag;
                packet_points = poly4->p;
                packet_color = &poly4->rgbc;
                packet_points[0].xy.xy = xport_gte_read_data(12);
                packet_points[1].xy.xy = xport_gte_read_data(13);
                packet_points[2].xy.xy = xport_gte_read_data(14);
                val3 = vertex_offset + (val3 << 3);
                XASSERT(!((uint32)(val3) > model_input->capacity || sizeof(MODEL_VERTEX) > model_input->capacity - (uint32)(val3)));
                vertex_input = (const MODEL_VERTEX *)(model_input->bytes + (uint32)(val3));
                xport_gte_write_data(0, vertex_input->words[0]);
                xport_gte_write_data(1, vertex_input->words[1]);
                xport_gte_execute(0x180001);
                xport_gte_execute(0x1400006);
                clip1 = (sint32)xport_gte_read_data(24);
                xport_gte_execute(0x168002E);
                if (clip0 > 0 && clip1 < 0)
                    goto next_flat_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_flat_polygon;
                XASSERT(!(model_input->capacity < 4 || (uint32)(rec_offset + 8) > model_input->capacity - 4));
                memcpy(&tint, model_input->bytes + (uint32)(rec_offset + 8), sizeof(tint));
                packet_points[3].xy.xy = xport_gte_read_data(14);
            }
            setlen(packet_tag, type == 0 ? 7 : 9);
            packet_color->rgbc = color + tint;
            packet_color->code = (uint8)(type == 0 ? 36 : 44);
            tag = *packet_tag;
            head_address = ordering_table;
            head_address.byte_offset += ((uint32)(depth >> (shift & 31)) << 2);
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            *packet_tag = (tag & 0xFF000000) | (head & 0xFFFFFF);
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            head_address.tags[(sint32)head_address.byte_offset / 4] = (head & 0xFF000000) | gpu_packet_link((uintptr_t)packet_tag);
        next_flat_polygon:
            packet_offset += packet_bytes;
            rec_offset += 20;
        }
    }
    PopMatrix();
    return 0;
}

uint32 render_add_checked(uint32 left, uint32 right, uint32 pc)
{
    sint64 result = (sint64)(sint32)left + (sint64)(sint32)right;

    if (result < (-2147483647 - (sint64)1) || result > 2147483647)
        xport_mips_overflow_exception(pc);
    return result;
}

static uint32 link_packet(uint32 *packet_tag, DISPLAY_ORDERING ordering_table, sint32 depth, uint32 shift)
{
    DISPLAY_ORDERING head_address;
    uint32 tag, head, result;

    tag = *packet_tag;
    head_address = ordering_table;
    head_address.byte_offset += ((uint32)(depth >> (shift & 31)) << 2);
    head = head_address.tags[(sint32)head_address.byte_offset / 4];
    *packet_tag = (tag & 0xFF000000) | (head & 0xFFFFFF);
    head = head_address.tags[(sint32)head_address.byte_offset / 4];
    result = (head & 0xFF000000) | gpu_packet_link((uintptr_t)packet_tag);
    head_address.tags[(sint32)head_address.byte_offset / 4] = result;
    return result;
}

static void project_triangle(const SVECTOR *v0, const SVECTOR *v1, const SVECTOR *v2)
{
    xport_gte_write_data(0, ((uint32)(uint16)v0->vx | ((uint32)(uint16)v0->vy << 16)));
    xport_gte_write_data(1, ((uint32)(uint16)v0->vz | ((uint32)(uint16)v0->pad << 16)));
    xport_gte_write_data(2, ((uint32)(uint16)v1->vx | ((uint32)(uint16)v1->vy << 16)));
    xport_gte_write_data(3, ((uint32)(uint16)v1->vz | ((uint32)(uint16)v1->pad << 16)));
    xport_gte_write_data(4, ((uint32)(uint16)v2->vx | ((uint32)(uint16)v2->vy << 16)));
    xport_gte_write_data(5, ((uint32)(uint16)v2->vz | ((uint32)(uint16)v2->pad << 16)));
    xport_gte_execute(0x280030);
}

typedef struct SUBDIVISION_VERTEX
{
    SVECTOR pos;
    RGBC color;
    UV uv;
} SUBDIVISION_VERTEX;

/* Level 2 emits at most five rows of five vertices */
/* RGB interpolation retains the code byte from earlier lit vertices */
typedef struct SUBDIVISION_STATE
{
    SUBDIVISION_VERTEX vertices[25];
    uint32 depths[4];
} SUBDIVISION_STATE;

static SUBDIVISION_STATE subdiv;

static void subdiv_lit_vertex(SUBDIVISION_VERTEX *vertex, const VEC3 *pos, const uint16 uv[2], uint32 shift, const VEC3 *origin, const uint32 *color_table, uint32 color_bias)
{
    VECTOR sq;
    uint32 coord, base, sum, threshold;
    const uint32 *color_sample;

    vertex->pos.vx = (sint16)(pos->x >> shift);
    vertex->pos.vy = (sint16)(pos->y >> shift);
    vertex->pos.vz = (sint16)(pos->z >> shift);
    vertex->uv.u = (uint8)((sint16)uv[0] >> shift);
    vertex->uv.v = (uint8)((sint16)uv[1] >> shift);
    coord = (uint32)vertex->pos.vx;
    base = origin->bits.x;
    sq.vx = (sint32)(base + coord);
    coord = (uint32)vertex->pos.vy;
    base = origin->bits.y;
    sq.vy = (sint32)(base + coord);
    coord = (uint32)vertex->pos.vz;
    base = origin->bits.z;
    sq.vz = (sint32)(base + coord);
    Square0(&sq, &sq);
    threshold = lighting.radius_sq;
    sum = (uint32)sq.vx + (uint32)sq.vy + (uint32)sq.vz;
    color_sample = color_table + (threshold < sum ? 1023 : (uint32)((sint32)sum >> 15));
    vertex->color.rgbc = *color_sample + color_bias;
}

static void subdiv_attributes(POLY_GT4 *poly, const SUBDIVISION_VERTEX *v0, const SUBDIVISION_VERTEX *v1, const SUBDIVISION_VERTEX *v2, const SUBDIVISION_VERTEX *v3, uint16 clut, uint16 page)
{
    poly->p[1].attr = clut;
    poly->p[0].attr = page;
    poly->p[0].rgbc.rgbc = v0->color.rgbc;
    poly->p[1].rgbc.rgbc = v1->color.rgbc;
    poly->p[2].rgbc.rgbc = v2->color.rgbc;
    poly->p[3].rgbc.rgbc = v3->color.rgbc;
    poly->p[0].uv.u = v0->uv.u;
    poly->p[0].uv.v = v0->uv.v;
    poly->p[1].uv.u = v1->uv.u;
    poly->p[1].uv.v = v1->uv.v;
    poly->p[2].uv.u = v2->uv.u;
    poly->p[2].uv.v = v2->uv.v;
    poly->p[3].uv.u = v3->uv.u;
    poly->p[3].uv.v = v3->uv.v;
}

static void subdiv_color_vertex(SUBDIVISION_VERTEX *vertex, const VEC3 *pos, const uint16 uv[2], const uint16 color[3], uint32 shift)
{
    vertex->pos.vx = (sint16)(pos->x >> shift);
    vertex->pos.vy = (sint16)(pos->y >> shift);
    vertex->pos.vz = (sint16)(pos->z >> shift);
    vertex->uv.u = (uint8)((sint16)uv[0] >> shift);
    vertex->uv.v = (uint8)((sint16)uv[1] >> shift);
    vertex->color.r = (uint8)((sint16)color[0] >> shift);
    vertex->color.g = (uint8)((sint16)color[1] >> shift);
    vertex->color.b = (uint8)((sint16)color[2] >> shift);
}

static void subdiv_flat_triangle(POLY_FT3 *packet, DISPLAY_ORDERING ordering, const MODEL_VERTEX *vertices, sint32 depth)
{
    uint8 *packet_base;
    POLY_FT4 *poly4_cursor;
    POLY_FT3 *poly3_cursor;
    DISPLAY_ORDERING bucket;
    uint32 shift, other_shift, level, scale, allocation, limit, reserved;
    uint32 triangle_offset, buffer_idx;
    uint32 header, color, tex_22, tex_14;
    VEC3 edge, vertical, across, cur;
    uint32 edge_uv[2], vertical_uv[2], across_uv[2], cur_uv[2];
    VEC2 scr[3];
    uint32 clip[3];
    uint32 side, component, divisions, row, column, row_width;
    SUBDIVISION_VERTEX *cursor, *row_base, *prev_row = NULL, *point, *old_left, *old_right, *new_left;
    uint32 val, other, tag;
    sint32 threshold, width, height, proj_depth;

    POLY_FT4 *poly4;
    POLY_FT3 *poly3;

    FUNCTION_MARKER(0x800724AC, "SLUS_006.10");
    shift = model_render.ot_shift;
    other_shift = model_render.coord_shift;
    threshold = (sint32)model_render.lod_half_distance;
    shift = (shift - other_shift) & 31;
    if (depth >= threshold / 4)
        goto link_original;
    threshold = (sint32)model_render.lod_quarter_distance;
    level = depth < threshold / 4 ? 2 : 1;
    scale = level * 2;
    allocation = packet_pool.used;
    limit = packet_pool.limit;
    reserved = allocation + (40 << scale) - (8 << level);
    if (limit < reserved)
        goto link_original;
    for (side = 0; side < 3; ++side)
    {
        scr[side].x = packet->p[side].xy.x;
        scr[side].y = packet->p[side].xy.y;
    }
    width = (sint32)model_render.width;
    height = (sint32)model_render.height;
    for (side = 0; side < 3; ++side)
    {
        clip[side] = scr[side].x < 0 ? 1 : 0;
        if (width < scr[side].x)
            clip[side] |= 2;
        if (scr[side].y < 0)
            clip[side] |= 4;
        if (height < scr[side].y)
            clip[side] |= 8;
    }
    if ((clip[0] & clip[1] & clip[2]) != 0)
        return;
    allocation = packet_pool.used;
    triangle_offset = allocation + (40 << scale) - (40 << level);
    buffer_idx = disp_state.bank;
    packet_pool.used = triangle_offset;
    packet_pool.used = triangle_offset + (32 << level);
    color = packet->rgbc.rgbc;
    packet_base = packet_pool.buffers[buffer_idx];
    header = packet->tag;
    tex_22 = packet->p[1].attr;
    val = (uint32)vertices[0].sdk.vx;
    tex_14 = packet->p[0].attr;
    edge.bits.x = val << scale;
    for (component = 1; component < 3; ++component)
        edge.u[component] = (uint32)vertices[0].coordinates[component] << scale;
    for (component = 0; component < 2; ++component)
        edge_uv[component] = (uint16)((component == 0 ? packet->p[0].uv.u : packet->p[0].uv.v) << scale);
    for (component = 0; component < 3; ++component)
    {
        val = (uint32)vertices[2].coordinates[component];
        other = (uint32)vertices[0].coordinates[component];
        vertical.u[component] = (val - other) << level;
    }
    for (component = 0; component < 2; ++component)
    {
        val = (component == 0 ? packet->p[2].uv.u : packet->p[2].uv.v);
        other = (component == 0 ? packet->p[0].uv.u : packet->p[0].uv.v);
        vertical_uv[component] = (uint16)((val - other) << level);
    }
    for (component = 0; component < 3; ++component)
    {
        val = (uint32)vertices[1].coordinates[component];
        other = (uint32)vertices[0].coordinates[component];
        across.u[component] = (val - other) << level;
    }
    for (component = 0; component < 2; ++component)
    {
        val = (component == 0 ? packet->p[1].uv.u : packet->p[1].uv.v);
        other = (component == 0 ? packet->p[0].uv.u : packet->p[0].uv.v);
        across_uv[component] = (uint16)((val - other) << level);
    }
    poly4_cursor = (POLY_FT4 *)(packet_base + allocation);
    poly3_cursor = (POLY_FT3 *)(packet_base + triangle_offset);
    divisions = 1 << level;
    cursor = subdiv.vertices;
    for (row = 0; row <= divisions; ++row)
    {
        if (row != 0)
        {
            for (component = 0; component < 3; ++component)
                edge.u[component] += vertical.u[component];
            for (component = 0; component < 2; ++component)
                edge_uv[component] = (uint16)(edge_uv[component] + vertical_uv[component]);
        }
        for (component = 0; component < 3; ++component)
            cur.u[component] = edge.u[component];
        for (component = 0; component < 2; ++component)
            cur_uv[component] = edge_uv[component];
        row_base = cursor;
        row_width = divisions - row + 1;
        for (column = 0; column < row_width; ++column)
        {
            if (column != 0)
            {
                for (component = 0; component < 3; ++component)
                    cur.u[component] += across.u[component];
                for (component = 0; component < 2; ++component)
                    cur_uv[component] = (uint16)(cur_uv[component] + across_uv[component]);
            }
            point = cursor;
            point->pos.vx = (sint16)(cur.x >> scale);
            point->pos.vy = (sint16)(cur.y >> scale);
            point->pos.vz = (sint16)(cur.z >> scale);
            point->uv.u = (uint8)((sint32)(sint16)cur_uv[0] >> scale);
            point->uv.v = (uint8)((sint32)(sint16)cur_uv[1] >> scale);
            ++cursor;
            if (row == 0 || column == 0)
                continue;
            old_left = prev_row + (column - 1);
            old_right = old_left + 1;
            new_left = point - 1;
            poly4 = poly4_cursor;
            poly4->p[1].attr = tex_22;
            poly4->p[0].attr = tex_14;
            poly4->rgbc.rgbc = color;
            val = old_left->uv.u;
            poly4->p[0].uv.u = val;
            val = old_left->uv.v;
            poly4->p[0].uv.v = val;
            val = old_right->uv.u;
            poly4->p[1].uv.u = val;
            val = old_right->uv.v;
            poly4->p[1].uv.v = val;
            val = new_left->uv.u;
            poly4->p[2].uv.u = val;
            val = new_left->uv.v;
            poly4->p[2].uv.v = val;
            val = point->uv.u;
            poly4->p[3].uv.u = val;
            val = point->uv.v;
            poly4->p[3].uv.v = val;
            val = ((uint32)(uint16)old_left->pos.vx | ((uint32)(uint16)old_left->pos.vy << 16));
            xport_gte_write_data(0, val);
            val = ((uint32)(uint16)old_left->pos.vz | ((uint32)(uint16)old_left->pos.pad << 16));
            xport_gte_write_data(1, val);
            val = ((uint32)(uint16)old_right->pos.vx | ((uint32)(uint16)old_right->pos.vy << 16));
            xport_gte_write_data(2, val);
            val = ((uint32)(uint16)old_right->pos.vz | ((uint32)(uint16)old_right->pos.pad << 16));
            xport_gte_write_data(3, val);
            val = ((uint32)(uint16)new_left->pos.vx | ((uint32)(uint16)new_left->pos.vy << 16));
            xport_gte_write_data(4, val);
            val = ((uint32)(uint16)new_left->pos.vz | ((uint32)(uint16)new_left->pos.pad << 16));
            xport_gte_write_data(5, val);
            xport_gte_execute(0x280030);
            poly4->p[0].xy.xy = xport_gte_read_data(12);
            poly4->p[1].xy.xy = xport_gte_read_data(13);
            poly4->p[2].xy.xy = xport_gte_read_data(14);
            val = ((uint32)(uint16)point->pos.vx | ((uint32)(uint16)point->pos.vy << 16));
            xport_gte_write_data(0, val);
            val = ((uint32)(uint16)point->pos.vz | ((uint32)(uint16)point->pos.pad << 16));
            xport_gte_write_data(1, val);
            xport_gte_execute(0x180001);
            xport_gte_execute(0x168002E);
            proj_depth = (sint32)xport_gte_read_data(7);
            if (proj_depth > 0)
            {
                poly4->p[3].xy.xy = xport_gte_read_data(14);
                setlen(poly4, 9);
                poly4->rgbc.code = 0x2C;
                bucket = ordering;
                bucket.byte_offset += ((uint32)proj_depth << 2);
                tag = poly4->tag;
                val = bucket.tags[(sint32)bucket.byte_offset / 4];
                poly4->tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
                val = bucket.tags[(sint32)bucket.byte_offset / 4];
                bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)poly4_cursor);
            }
            ++poly4_cursor;
        }
        if (row != 0)
        {
            old_left = prev_row + (row_width - 1);
            old_right = old_left + 1;
            point = row_base + (row_width - 1);
            poly3 = poly3_cursor;
            poly3->p[1].attr = tex_22;
            poly3->p[0].attr = tex_14;
            poly3->tag = header;
            poly3->rgbc.rgbc = color;
            val = old_left->uv.u;
            poly3->p[0].uv.u = val;
            val = old_left->uv.v;
            poly3->p[0].uv.v = val;
            val = old_right->uv.u;
            poly3->p[1].uv.u = val;
            val = old_right->uv.v;
            poly3->p[1].uv.v = val;
            val = point->uv.u;
            poly3->p[2].uv.u = val;
            val = point->uv.v;
            poly3->p[2].uv.v = val;
            val = ((uint32)(uint16)old_left->pos.vx | ((uint32)(uint16)old_left->pos.vy << 16));
            xport_gte_write_data(0, val);
            val = ((uint32)(uint16)old_left->pos.vz | ((uint32)(uint16)old_left->pos.pad << 16));
            xport_gte_write_data(1, val);
            val = ((uint32)(uint16)old_right->pos.vx | ((uint32)(uint16)old_right->pos.vy << 16));
            xport_gte_write_data(2, val);
            val = ((uint32)(uint16)old_right->pos.vz | ((uint32)(uint16)old_right->pos.pad << 16));
            xport_gte_write_data(3, val);
            val = ((uint32)(uint16)point->pos.vx | ((uint32)(uint16)point->pos.vy << 16));
            xport_gte_write_data(4, val);
            val = ((uint32)(uint16)point->pos.vz | ((uint32)(uint16)point->pos.pad << 16));
            xport_gte_write_data(5, val);
            xport_gte_execute(0x280030);
            xport_gte_execute(0x158002D);
            proj_depth = (sint32)xport_gte_read_data(7);
            if (proj_depth > 0)
            {
                poly3->p[0].xy.xy = xport_gte_read_data(12);
                poly3->p[1].xy.xy = xport_gte_read_data(13);
                poly3->p[2].xy.xy = xport_gte_read_data(14);
                bucket = ordering;
                bucket.byte_offset += ((uint32)(proj_depth >> shift) << 2);
                tag = poly3->tag;
                val = bucket.tags[(sint32)bucket.byte_offset / 4];
                poly3->tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
                val = bucket.tags[(sint32)bucket.byte_offset / 4];
                bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)poly3_cursor);
            }
            ++poly3_cursor;
        }
        prev_row = row_base;
    }
    return;

link_original:
    bucket = ordering;
    bucket.byte_offset += ((uint32)(depth >> shift) << 2);
    tag = packet->tag;
    val = bucket.tags[(sint32)bucket.byte_offset / 4];
    packet->tag = (uint32)((tag & 0xFF000000) | (val & 0xFFFFFF));
    val = bucket.tags[(sint32)bucket.byte_offset / 4];
    bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
}

static uint32 subdiv_lit_triangle(POLY_GT3 *packet, DISPLAY_ORDERING ordering_table, const MODEL_VERTEX *vertices, sint32 depth, const VEC3 *origin, const uint32 *color_table, uint32 color_bias)
{
    POLY_GT4 *poly4_cursor;
    POLY_GT3 *poly3_cursor;
    uint32 shift, level, precision, required, offset, triangle_offset, bank;
    uint32 original_tag, row, column, axis, count;
    VEC3 base, horizontal, vertical, cur;
    uint32 val0, val1;
    uint16 base_uv[2], horizontal_uv[2], vertical_uv[2], cur_uv[2], clut, page;
    sint32 cutoff, x0, y0, x1, y1, x2, y2, proj_depth;

    SUBDIVISION_VERTEX *cursor, *v0, *v1, *v2;

    POLY_GT4 *poly4;
    POLY_GT3 *poly3;

    FUNCTION_MARKER(0x8006E934, "SLUS_006.10");
    cutoff = (sint32)model_render.lod_half_distance;
    shift = model_render.ot_shift;
    shift -= model_render.coord_shift;
    if (depth >= cutoff / 4)
        return link_packet(&packet->tag, ordering_table, depth, shift);
    cutoff = (sint32)model_render.lod_quarter_distance;
    level = depth < cutoff / 4 ? 2 : 1;
    offset = packet_pool.used;
    precision = level * 2;
    required = 52 << precision;
    if (packet_pool.limit < offset + required - (12 << level))
        return link_packet(&packet->tag, ordering_table, depth, shift);
    x2 = packet->p[2].xy.x;
    x0 = packet->p[0].xy.x;
    y0 = packet->p[0].xy.y;
    x1 = packet->p[1].xy.x;
    y1 = packet->p[1].xy.y;
    y2 = packet->p[2].xy.y;
    if (triangle_outside(x0, y0, x1, y1, x2, y2) != 0)
        return 1;
    offset = packet_pool.used;
    bank = disp_state.bank;
    triangle_offset = offset + required - (52 << level);
    packet_pool.used = triangle_offset;
    packet_pool.used = triangle_offset + (40 << level);
    original_tag = packet->tag;
    poly4_cursor = (POLY_GT4 *)(packet_pool.buffers[bank] + offset);
    poly3_cursor = (POLY_GT3 *)(packet_pool.buffers[bank] + triangle_offset);
    clut = packet->p[1].attr;
    base.bits.x = (uint32)vertices[0].sdk.vx << precision;
    page = packet->p[0].attr;
    base.bits.y = (uint32)vertices[0].sdk.vy << precision;
    base.bits.z = (uint32)vertices[0].sdk.vz << precision;
    base_uv[0] = (uint16)(packet->p[0].uv.u << precision);
    base_uv[1] = (uint16)(packet->p[0].uv.v << precision);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (uint32)vertices[2].coordinates[axis];
        val1 = (uint32)vertices[0].coordinates[axis];
        vertical.u[axis] = (val0 - val1) << level;
    }
    val0 = packet->p[2].uv.u;
    val1 = packet->p[0].uv.u;
    vertical_uv[0] = ((val0 - val1) << level);
    val0 = packet->p[2].uv.v;
    val1 = packet->p[0].uv.v;
    vertical_uv[1] = ((val0 - val1) << level);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (uint32)vertices[1].coordinates[axis];
        val1 = (uint32)vertices[0].coordinates[axis];
        horizontal.u[axis] = (val0 - val1) << level;
    }
    val0 = packet->p[1].uv.u;
    val1 = packet->p[0].uv.u;
    horizontal_uv[0] = ((val0 - val1) << level);
    val0 = packet->p[1].uv.v;
    val1 = packet->p[0].uv.v;
    horizontal_uv[1] = ((val0 - val1) << level);
    cursor = subdiv.vertices;
    count = 1 << level;
    for (axis = 0; axis < 3; ++axis)
        cur.u[axis] = base.u[axis];
    cur_uv[0] = base_uv[0];
    cur_uv[1] = base_uv[1];
    for (column = 0; column <= count; ++column)
    {
        subdiv_lit_vertex(cursor, &cur, cur_uv, precision, origin, color_table, color_bias);
        ++cursor;
        for (axis = 0; axis < 3; ++axis)
            cur.u[axis] += horizontal.u[axis];
        cur_uv[0] = (cur_uv[0] + horizontal_uv[0]);
        cur_uv[1] = (cur_uv[1] + horizontal_uv[1]);
    }
    for (row = 1; row <= count; ++row)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            base.u[axis] += vertical.u[axis];
            cur.u[axis] = base.u[axis];
        }
        base_uv[0] = (base_uv[0] + vertical_uv[0]);
        base_uv[1] = (base_uv[1] + vertical_uv[1]);
        cur_uv[0] = base_uv[0];
        cur_uv[1] = base_uv[1];
        subdiv_lit_vertex(cursor, &cur, cur_uv, precision, origin, color_table, color_bias);
        ++cursor;
        for (column = 1; column <= count - row; ++column)
        {
            for (axis = 0; axis < 3; ++axis)
                cur.u[axis] += horizontal.u[axis];
            cur_uv[0] = (cur_uv[0] + horizontal_uv[0]);
            cur_uv[1] = (cur_uv[1] + horizontal_uv[1]);
            subdiv_lit_vertex(cursor, &cur, cur_uv, precision, origin, color_table, color_bias);
            v1 = cursor - (count - row + 2);
            v0 = v1 - 1;
            v2 = cursor - 1;
            poly4 = poly4_cursor;
            subdiv_attributes(poly4, v0, v1, v2, cursor, clut, page);
            project_triangle(&v0->pos, &v1->pos, &v2->pos);
            poly4->p[0].xy.xy = xport_gte_read_data(12);
            poly4->p[1].xy.xy = xport_gte_read_data(13);
            poly4->p[2].xy.xy = xport_gte_read_data(14);
            xport_gte_write_data(0, ((uint32)(uint16)cursor->pos.vx | ((uint32)(uint16)cursor->pos.vy << 16)));
            xport_gte_write_data(1, ((uint32)(uint16)cursor->pos.vz | ((uint32)(uint16)cursor->pos.pad << 16)));
            xport_gte_execute(0x180001);
            xport_gte_execute(0x168002E);
            proj_depth = (sint32)xport_gte_read_data(7);
            if (proj_depth > 0)
            {
                poly4->p[3].xy.xy = xport_gte_read_data(14);
                setlen(poly4, 12);
                poly4->p[0].rgbc.code = 60;
                link_packet(&poly4_cursor->tag, ordering_table, proj_depth, shift);
            }
            ++poly4_cursor;
            ++cursor;
        }
        v1 = cursor - (count - row + 2);
        v0 = v1 - 1;
        v2 = cursor - 1;
        poly3 = poly3_cursor;
        poly3->p[1].attr = clut;
        poly3->p[0].attr = page;
        poly3->tag = original_tag;
        poly3->p[0].rgbc.rgbc = v0->color.rgbc;
        poly3->p[1].rgbc.rgbc = v1->color.rgbc;
        poly3->p[2].rgbc.rgbc = v2->color.rgbc;
        poly3->p[0].uv.u = v0->uv.u;
        poly3->p[0].uv.v = v0->uv.v;
        poly3->p[1].uv.u = v1->uv.u;
        poly3->p[1].uv.v = v1->uv.v;
        poly3->p[2].uv.u = v2->uv.u;
        poly3->p[2].uv.v = v2->uv.v;
        project_triangle(&v0->pos, &v1->pos, &v2->pos);
        xport_gte_execute(0x158002D);
        proj_depth = (sint32)xport_gte_read_data(7);
        if (proj_depth > 0)
        {
            poly3->p[0].xy.xy = xport_gte_read_data(12);
            poly3->p[1].xy.xy = xport_gte_read_data(13);
            poly3->p[2].xy.xy = xport_gte_read_data(14);
            setlen(poly3, 9);
            poly3->p[0].rgbc.code = 52;
            link_packet(&poly3_cursor->tag, ordering_table, proj_depth, shift);
        }
        ++poly3_cursor;
    }
    return 1;
}

static intptr subdiv_lit_quad(POLY_GT4 *packet, DISPLAY_ORDERING ordering_table, const MODEL_VERTEX *vertices, sint32 depth, const VEC3 *origin, const uint32 *color_table, uint32 color_bias)
{
    POLY_GT4 *poly4_cursor;
    DISPLAY_ORDERING ordering_head;
    uint32 shift, level, precision, required, offset, bank, original_tag;
    uint32 count, axis, component, row, column, val0, val1;
    VEC3 left, right, left_step, right_step, horizontal, cur;
    uint16 left_uv[2], right_uv[2], left_uv_step[2], right_uv_step[2], horizontal_uv[2], cur_uv[2], clut, page;
    uint32 *depths = subdiv.depths;

    sint32 cutoff, x0, y0, x1, y1, x2, y2, x3, y3, proj_depth;

    SUBDIVISION_VERTEX *cursor, *v0, *v1, *v2;

    POLY_GT4 *poly4;

    FUNCTION_MARKER(0x8006F450, "SLUS_006.10");
    cutoff = (sint32)model_render.lod_half_distance;
    shift = model_render.ot_shift;
    shift -= model_render.coord_shift;
    if (depth >= cutoff / 4)
        return link_packet(&packet->tag, ordering_table, depth, shift);
    cutoff = (sint32)model_render.lod_quarter_distance;
    level = depth < cutoff / 4 ? 2 : 1;
    offset = packet_pool.used;
    bank = packet_pool.limit;
    precision = level * 2;
    required = 52 << precision;
    if (bank < offset + required)
        return link_packet(&packet->tag, ordering_table, depth, shift);
    x2 = packet->p[2].xy.x;
    x0 = packet->p[0].xy.x;
    y0 = packet->p[0].xy.y;
    x1 = packet->p[1].xy.x;
    y1 = packet->p[1].xy.y;
    y2 = packet->p[2].xy.y;
    x3 = packet->p[3].xy.x;
    y3 = packet->p[3].xy.y;
    if (quad_outside(x0, y0, x1, y1, x2, y2, x3, y3) != 0)
        return (intptr)depths;
    depths[0] = xport_gte_read_data(16);
    depths[1] = xport_gte_read_data(17);
    depths[2] = xport_gte_read_data(18);
    depths[3] = xport_gte_read_data(19);
    offset = packet_pool.used;
    bank = disp_state.bank;
    packet_pool.used = offset + required;
    original_tag = packet->tag;
    poly4_cursor = (POLY_GT4 *)(packet_pool.buffers[bank] + offset);
    clut = packet->p[1].attr;
    left.bits.x = (uint32)vertices[0].sdk.vx << precision;
    page = packet->p[0].attr;
    left.bits.y = (uint32)vertices[0].sdk.vy << precision;
    left.bits.z = (uint32)vertices[0].sdk.vz << precision;
    left_uv[0] = (uint16)(packet->p[0].uv.u << precision);
    left_uv[1] = (uint16)(packet->p[0].uv.v << precision);
    for (axis = 0; axis < 3; ++axis)
        right.u[axis] = (uint32)vertices[1].coordinates[axis] << precision;
    right_uv[0] = (uint16)(packet->p[1].uv.u << precision);
    right_uv[1] = (uint16)(packet->p[1].uv.v << precision);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (uint32)vertices[2].coordinates[axis];
        val1 = (uint32)vertices[0].coordinates[axis];
        left_step.u[axis] = (val0 - val1) << level;
    }
    val0 = packet->p[2].uv.u;
    val1 = packet->p[0].uv.u;
    left_uv_step[0] = ((val0 - val1) << level);
    val0 = packet->p[2].uv.v;
    val1 = packet->p[0].uv.v;
    left_uv_step[1] = ((val0 - val1) << level);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (uint32)vertices[3].coordinates[axis];
        val1 = (uint32)vertices[1].coordinates[axis];
        right_step.u[axis] = (val0 - val1) << level;
    }
    val0 = packet->p[3].uv.u;
    val1 = packet->p[1].uv.u;
    right_uv_step[0] = ((val0 - val1) << level);
    val0 = packet->p[3].uv.v;
    val1 = packet->p[1].uv.v;
    right_uv_step[1] = ((val0 - val1) << level);
    cursor = subdiv.vertices;
    count = 1 << level;
    for (row = 0; row <= count; ++row)
    {
        if (row != 0)
        {
            for (axis = 0; axis < 3; ++axis)
                left.u[axis] += left_step.u[axis];
            left_uv[0] = (left_uv[0] + left_uv_step[0]);
            left_uv[1] = (left_uv[1] + left_uv_step[1]);
            for (axis = 0; axis < 3; ++axis)
                right.u[axis] += right_step.u[axis];
            right_uv[0] = (right_uv[0] + right_uv_step[0]);
            right_uv[1] = (right_uv[1] + right_uv_step[1]);
        }
        for (axis = 0; axis < 3; ++axis)
        {
            cur.u[axis] = left.u[axis];
            horizontal.u[axis] = (uint32)((sint32)(right.u[axis] - left.u[axis]) >> level);
        }
        for (component = 0; component < 2; ++component)
        {
            cur_uv[component] = left_uv[component];
            horizontal_uv[component] = (((sint32)(sint16)right_uv[component] - (sint32)(sint16)left_uv[component]) >> level);
        }
        subdiv_lit_vertex(cursor, &cur, cur_uv, precision, origin, color_table, color_bias);
        ++cursor;
        for (column = 1; column <= count; ++column)
        {
            for (axis = 0; axis < 3; ++axis)
                cur.u[axis] += horizontal.u[axis];
            cur_uv[0] = (cur_uv[0] + horizontal_uv[0]);
            cur_uv[1] = (cur_uv[1] + horizontal_uv[1]);
            subdiv_lit_vertex(cursor, &cur, cur_uv, precision, origin, color_table, color_bias);
            if (row != 0)
            {
                v1 = cursor - (count + 1);
                v0 = v1 - 1;
                v2 = cursor - 1;
                poly4 = poly4_cursor;
                poly4->p[1].attr = clut;
                poly4->p[0].attr = page;
                poly4->tag = original_tag;
                poly4->p[0].rgbc.rgbc = v0->color.rgbc;
                poly4->p[1].rgbc.rgbc = v1->color.rgbc;
                poly4->p[2].rgbc.rgbc = v2->color.rgbc;
                poly4->p[3].rgbc.rgbc = cursor->color.rgbc;
                poly4->p[0].uv.u = v0->uv.u;
                poly4->p[0].uv.v = v0->uv.v;
                poly4->p[1].uv.u = v1->uv.u;
                poly4->p[1].uv.v = v1->uv.v;
                poly4->p[2].uv.u = v2->uv.u;
                poly4->p[2].uv.v = v2->uv.v;
                poly4->p[3].uv.u = cursor->uv.u;
                poly4->p[3].uv.v = cursor->uv.v;
                project_triangle(&v0->pos, &v1->pos, &v2->pos);
                poly4->p[0].xy.xy = xport_gte_read_data(12);
                poly4->p[1].xy.xy = xport_gte_read_data(13);
                poly4->p[2].xy.xy = xport_gte_read_data(14);
                xport_gte_write_data(0, ((uint32)(uint16)cursor->pos.vx | ((uint32)(uint16)cursor->pos.vy << 16)));
                xport_gte_write_data(1, ((uint32)(uint16)cursor->pos.vz | ((uint32)(uint16)cursor->pos.pad << 16)));
                xport_gte_execute(0x180001);
                poly4->p[3].xy.xy = xport_gte_read_data(14);
                xport_gte_execute(0x168002E);
                proj_depth = (sint32)xport_gte_read_data(7);
                if (proj_depth > 0)
                {
                    setlen(poly4, 12);
                    poly4->p[0].rgbc.code = 60;
                    link_packet(&poly4_cursor->tag, ordering_table, proj_depth, shift);
                }
                ++poly4_cursor;
            }
            ++cursor;
        }
    }
    ordering_head = ordering_table;
    ordering_head.byte_offset += ((uint32)((sint32)((uint32)depth + 256) >> (shift & 31)) << 2);
    if (level == 1)
        return draw_subdiv_edges2(packet, ordering_head, depths);
    return draw_subdiv_edges4(packet, ordering_head, depths);
}

static uint32 subdiv_color_triangle(POLY_GT3 *packet, DISPLAY_ORDERING ordering_table, const MODEL_VERTEX *vertices, sint32 depth)
{
    POLY_GT4 *poly4_cursor;
    POLY_GT3 *poly3_cursor;
    uint32 shift, level, precision, required, offset, triangle_offset, bank;
    uint32 original_tag, row, column, axis, count;
    VEC3 base, horizontal, vertical, cur;
    uint32 val0, val1;
    uint16 base_uv[2], horizontal_uv[2], vertical_uv[2], cur_uv[2], clut, page;
    uint16 base_color[3], horizontal_color[3], vertical_color[3], cur_color[3];
    uint32 flags[3], rejected;
    VEC2 scr[3];
    sint32 width, height;
    sint32 cutoff, proj_depth;

    SUBDIVISION_VERTEX *cursor, *v0, *v1, *v2;

    POLY_GT4 *poly4;
    POLY_GT3 *poly3;

    FUNCTION_MARKER(0x8007460C, "SLUS_006.10");
    shift = model_render.ot_shift;
    shift -= model_render.coord_shift;
    cutoff = (sint32)model_render.lod_half_distance;
    if (depth >= cutoff / 4)
        return link_packet(&packet->tag, ordering_table, depth, shift);
    cutoff = (sint32)model_render.lod_quarter_distance;
    level = depth < cutoff / 4 ? 2 : 1;
    offset = packet_pool.used;
    precision = level * 2;
    required = 52 << precision;
    if (packet_pool.limit < offset + required - (12 << level))
        return link_packet(&packet->tag, ordering_table, depth, shift);
    scr[0].x = packet->p[0].xy.x;
    scr[0].y = packet->p[0].xy.y;
    scr[1].x = packet->p[1].xy.x;
    scr[1].y = packet->p[1].xy.y;
    scr[2].x = packet->p[2].xy.x;
    scr[2].y = packet->p[2].xy.y;
    flags[0] = scr[0].x < 0 ? 1 : 0;
    width = (sint32)model_render.width;
    if (width < scr[0].x)
        flags[0] |= 2;
    if (scr[0].y < 0)
        flags[0] |= 4;
    height = (sint32)model_render.height;
    if (height < scr[0].y)
        flags[0] |= 8;
    for (axis = 1; axis < 3; ++axis)
    {
        flags[axis] = scr[axis].x < 0 ? 1 : 0;
        if (width < scr[axis].x)
            flags[axis] |= 2;
        if (scr[axis].y < 0)
            flags[axis] |= 4;
        if (height < scr[axis].y)
            flags[axis] |= 8;
    }
    rejected = flags[0] & flags[1] & flags[2];
    if (rejected != 0)
        return rejected;
    offset = packet_pool.used;
    bank = disp_state.bank;
    triangle_offset = offset + required - (52 << level);
    packet_pool.used = triangle_offset;
    packet_pool.used = triangle_offset + (40 << level);
    original_tag = packet->tag;
    poly4_cursor = (POLY_GT4 *)(packet_pool.buffers[bank] + offset);
    poly3_cursor = (POLY_GT3 *)(packet_pool.buffers[bank] + triangle_offset);
    clut = packet->p[1].attr;
    base.bits.x = (uint32)vertices[0].sdk.vx << precision;
    page = packet->p[0].attr;
    base.bits.y = (uint32)vertices[0].sdk.vy << precision;
    base.bits.z = (uint32)vertices[0].sdk.vz << precision;
    base_uv[0] = (uint16)(packet->p[0].uv.u << precision);
    base_uv[1] = (uint16)(packet->p[0].uv.v << precision);
    for (axis = 0; axis < 3; ++axis)
        base_color[axis] = (uint16)((axis == 0 ? packet->p[0].rgbc.r : axis == 1 ? packet->p[0].rgbc.g : packet->p[0].rgbc.b) << precision);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (uint32)vertices[2].coordinates[axis];
        val1 = (uint32)vertices[0].coordinates[axis];
        vertical.u[axis] = (val0 - val1) << level;
    }
    val0 = packet->p[2].uv.u;
    val1 = packet->p[0].uv.u;
    vertical_uv[0] = ((val0 - val1) << level);
    val0 = packet->p[2].uv.v;
    val1 = packet->p[0].uv.v;
    vertical_uv[1] = ((val0 - val1) << level);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (axis == 0 ? packet->p[2].rgbc.r : axis == 1 ? packet->p[2].rgbc.g : packet->p[2].rgbc.b);
        val1 = (axis == 0 ? packet->p[0].rgbc.r : axis == 1 ? packet->p[0].rgbc.g : packet->p[0].rgbc.b);
        vertical_color[axis] = ((val0 - val1) << level);
    }
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (uint32)vertices[1].coordinates[axis];
        val1 = (uint32)vertices[0].coordinates[axis];
        horizontal.u[axis] = (val0 - val1) << level;
    }
    val0 = packet->p[1].uv.u;
    val1 = packet->p[0].uv.u;
    horizontal_uv[0] = ((val0 - val1) << level);
    val0 = packet->p[1].uv.v;
    val1 = packet->p[0].uv.v;
    horizontal_uv[1] = ((val0 - val1) << level);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (axis == 0 ? packet->p[1].rgbc.r : axis == 1 ? packet->p[1].rgbc.g : packet->p[1].rgbc.b);
        val1 = (axis == 0 ? packet->p[0].rgbc.r : axis == 1 ? packet->p[0].rgbc.g : packet->p[0].rgbc.b);
        horizontal_color[axis] = ((val0 - val1) << level);
    }
    cursor = subdiv.vertices;
    count = 1 << level;
    for (axis = 0; axis < 3; ++axis)
    {
        cur.u[axis] = base.u[axis];
        cur_color[axis] = base_color[axis];
    }
    cur_uv[0] = base_uv[0];
    cur_uv[1] = base_uv[1];
    for (column = 0; column <= count; ++column)
    {
        subdiv_color_vertex(cursor, &cur, cur_uv, cur_color, precision);
        ++cursor;
        for (axis = 0; axis < 3; ++axis)
        {
            cur.u[axis] += horizontal.u[axis];
            cur_color[axis] = (cur_color[axis] + horizontal_color[axis]);
        }
        cur_uv[0] = (cur_uv[0] + horizontal_uv[0]);
        cur_uv[1] = (cur_uv[1] + horizontal_uv[1]);
    }
    for (row = 1; row <= count; ++row)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            base.u[axis] += vertical.u[axis];
            cur.u[axis] = base.u[axis];
            base_color[axis] = (base_color[axis] + vertical_color[axis]);
            cur_color[axis] = base_color[axis];
        }
        base_uv[0] = (base_uv[0] + vertical_uv[0]);
        base_uv[1] = (base_uv[1] + vertical_uv[1]);
        cur_uv[0] = base_uv[0];
        cur_uv[1] = base_uv[1];
        subdiv_color_vertex(cursor, &cur, cur_uv, cur_color, precision);
        ++cursor;
        for (column = 1; column <= count - row; ++column)
        {
            for (axis = 0; axis < 3; ++axis)
            {
                cur.u[axis] += horizontal.u[axis];
                cur_color[axis] = (cur_color[axis] + horizontal_color[axis]);
            }
            cur_uv[0] = (cur_uv[0] + horizontal_uv[0]);
            cur_uv[1] = (cur_uv[1] + horizontal_uv[1]);
            subdiv_color_vertex(cursor, &cur, cur_uv, cur_color, precision);
            v1 = cursor - (count - row + 2);
            v0 = v1 - 1;
            v2 = cursor - 1;
            poly4 = poly4_cursor;
            subdiv_attributes(poly4, v0, v1, v2, cursor, clut, page);
            project_triangle(&v0->pos, &v1->pos, &v2->pos);
            poly4->p[0].xy.xy = xport_gte_read_data(12);
            poly4->p[1].xy.xy = xport_gte_read_data(13);
            poly4->p[2].xy.xy = xport_gte_read_data(14);
            xport_gte_write_data(0, ((uint32)(uint16)cursor->pos.vx | ((uint32)(uint16)cursor->pos.vy << 16)));
            xport_gte_write_data(1, ((uint32)(uint16)cursor->pos.vz | ((uint32)(uint16)cursor->pos.pad << 16)));
            xport_gte_execute(0x180001);
            xport_gte_execute(0x168002E);
            proj_depth = (sint32)xport_gte_read_data(7);
            if (proj_depth > 0)
            {
                poly4->p[3].xy.xy = xport_gte_read_data(14);
                setlen(poly4, 12);
                poly4->p[0].rgbc.code = 60;
                link_packet(&poly4_cursor->tag, ordering_table, proj_depth, shift);
            }
            ++poly4_cursor;
            ++cursor;
        }
        v1 = cursor - (count - row + 2);
        v0 = v1 - 1;
        v2 = cursor - 1;
        poly3 = poly3_cursor;
        poly3->p[1].attr = clut;
        poly3->p[0].attr = page;
        poly3->tag = original_tag;
        poly3->p[0].rgbc.rgbc = v0->color.rgbc;
        poly3->p[1].rgbc.rgbc = v1->color.rgbc;
        poly3->p[2].rgbc.rgbc = v2->color.rgbc;
        poly3->p[0].uv.u = v0->uv.u;
        poly3->p[0].uv.v = v0->uv.v;
        poly3->p[1].uv.u = v1->uv.u;
        poly3->p[1].uv.v = v1->uv.v;
        poly3->p[2].uv.u = v2->uv.u;
        poly3->p[2].uv.v = v2->uv.v;
        project_triangle(&v0->pos, &v1->pos, &v2->pos);
        xport_gte_execute(0x158002D);
        proj_depth = (sint32)xport_gte_read_data(7);
        if (proj_depth > 0)
        {
            poly3->p[0].xy.xy = xport_gte_read_data(12);
            poly3->p[1].xy.xy = xport_gte_read_data(13);
            poly3->p[2].xy.xy = xport_gte_read_data(14);
            setlen(poly3, 9);
            poly3->p[0].rgbc.code = 52;
            link_packet(&poly3_cursor->tag, ordering_table, proj_depth, shift);
        }
        ++poly3_cursor;
    }
    return 1;
}

static intptr subdiv_color_quad(POLY_GT4 *packet, DISPLAY_ORDERING ordering_table, const MODEL_VERTEX *vertices, sint32 depth)
{
    POLY_GT4 *poly4_cursor;
    DISPLAY_ORDERING ordering_head;
    uint32 shift, level, precision, required, offset, bank, original_tag;
    uint32 count, axis, component, row, column, val0, val1;
    VEC3 left, right, left_step, right_step, horizontal, cur;
    uint16 left_uv[2], right_uv[2], left_uv_step[2], right_uv_step[2], horizontal_uv[2], cur_uv[2], clut, page;
    uint16 left_color[3], right_color[3], left_color_step[3], right_color_step[3], horizontal_color[3], cur_color[3];
    uint32 flag;
    VEC2 scr[4];
    sint32 width, height;
    uint32 *depths = subdiv.depths;

    sint32 cutoff, proj_depth;

    SUBDIVISION_VERTEX *cursor, *v0, *v1, *v2;

    POLY_GT4 *poly4;

    FUNCTION_MARKER(0x800760B8, "SLUS_006.10");
    shift = model_render.ot_shift;
    shift -= model_render.coord_shift;
    cutoff = (sint32)model_render.lod_half_distance;
    if (depth >= cutoff / 4)
        return link_packet(&packet->tag, ordering_table, depth, shift);
    cutoff = (sint32)model_render.lod_quarter_distance;
    level = depth < cutoff / 4 ? 2 : 1;
    offset = packet_pool.used;
    bank = packet_pool.limit;
    precision = level * 2;
    required = 52 << precision;
    if (bank < offset + required)
        return link_packet(&packet->tag, ordering_table, depth, shift);
    for (axis = 0; axis < 4; ++axis)
    {
        scr[axis].x = packet->p[axis].xy.x;
        scr[axis].y = packet->p[axis].xy.y;
    }
    depths[0] = 0;
    if (scr[0].x < 0)
        depths[0] = 1;
    width = (sint32)model_render.width;
    if (width < scr[0].x)
        depths[0] |= 2;
    if (scr[0].y < 0)
        depths[0] |= 4;
    height = (sint32)model_render.height;
    if (height < scr[0].y)
        depths[0] |= 8;
    for (axis = 1; axis < 4; ++axis)
    {
        depths[axis] = 0;
        if (scr[axis].x < 0)
            depths[axis] = 1;
        if (axis == 3)
            width = (sint32)model_render.width;
        if (width < scr[axis].x)
            depths[axis] |= 2;
        if (scr[axis].y < 0)
            depths[axis] |= 4;
        if (axis == 3)
            height = (sint32)model_render.height;
        if (height < scr[axis].y)
            depths[axis] |= 8;
    }
    flag = depths[0];
    flag &= depths[1];
    flag &= depths[2];
    flag &= depths[3];
    if (flag != 0)
        return (intptr)depths;
    depths[0] = xport_gte_read_data(16);
    depths[1] = xport_gte_read_data(17);
    depths[2] = xport_gte_read_data(18);
    depths[3] = xport_gte_read_data(19);
    bank = disp_state.bank;
    offset = packet_pool.used;
    packet_pool.used = offset + required;
    original_tag = packet->tag;
    poly4_cursor = (POLY_GT4 *)(packet_pool.buffers[bank] + offset);
    clut = packet->p[1].attr;
    left.bits.x = (uint32)vertices[0].sdk.vx << precision;
    page = packet->p[0].attr;
    left.bits.y = (uint32)vertices[0].sdk.vy << precision;
    left.bits.z = (uint32)vertices[0].sdk.vz << precision;
    left_uv[0] = (uint16)(packet->p[0].uv.u << precision);
    left_uv[1] = (uint16)(packet->p[0].uv.v << precision);
    for (axis = 0; axis < 3; ++axis)
        left_color[axis] = (uint16)((axis == 0 ? packet->p[0].rgbc.r : axis == 1 ? packet->p[0].rgbc.g : packet->p[0].rgbc.b) << precision);
    for (axis = 0; axis < 3; ++axis)
        right.u[axis] = (uint32)vertices[1].coordinates[axis] << precision;
    right_uv[0] = (uint16)(packet->p[1].uv.u << precision);
    right_uv[1] = (uint16)(packet->p[1].uv.v << precision);
    for (axis = 0; axis < 3; ++axis)
        right_color[axis] = (uint16)((axis == 0 ? packet->p[1].rgbc.r : axis == 1 ? packet->p[1].rgbc.g : packet->p[1].rgbc.b) << precision);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (uint32)vertices[2].coordinates[axis];
        val1 = (uint32)vertices[0].coordinates[axis];
        left_step.u[axis] = (val0 - val1) << level;
    }
    val0 = packet->p[2].uv.u;
    val1 = packet->p[0].uv.u;
    left_uv_step[0] = ((val0 - val1) << level);
    val0 = packet->p[2].uv.v;
    val1 = packet->p[0].uv.v;
    left_uv_step[1] = ((val0 - val1) << level);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (axis == 0 ? packet->p[2].rgbc.r : axis == 1 ? packet->p[2].rgbc.g : packet->p[2].rgbc.b);
        val1 = (axis == 0 ? packet->p[0].rgbc.r : axis == 1 ? packet->p[0].rgbc.g : packet->p[0].rgbc.b);
        left_color_step[axis] = ((val0 - val1) << level);
    }
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (uint32)vertices[3].coordinates[axis];
        val1 = (uint32)vertices[1].coordinates[axis];
        right_step.u[axis] = (val0 - val1) << level;
    }
    val0 = packet->p[3].uv.u;
    val1 = packet->p[1].uv.u;
    right_uv_step[0] = ((val0 - val1) << level);
    val0 = packet->p[3].uv.v;
    val1 = packet->p[1].uv.v;
    right_uv_step[1] = ((val0 - val1) << level);
    for (axis = 0; axis < 3; ++axis)
    {
        val0 = (axis == 0 ? packet->p[3].rgbc.r : axis == 1 ? packet->p[3].rgbc.g : packet->p[3].rgbc.b);
        val1 = (axis == 0 ? packet->p[1].rgbc.r : axis == 1 ? packet->p[1].rgbc.g : packet->p[1].rgbc.b);
        right_color_step[axis] = ((val0 - val1) << level);
    }
    cursor = subdiv.vertices;
    count = 1 << level;
    for (row = 0; row <= count; ++row)
    {
        if (row != 0)
        {
            for (axis = 0; axis < 3; ++axis)
                left.u[axis] += left_step.u[axis];
            left_uv[0] = (left_uv[0] + left_uv_step[0]);
            left_uv[1] = (left_uv[1] + left_uv_step[1]);
            for (axis = 0; axis < 3; ++axis)
            {
                right.u[axis] += right_step.u[axis];
                left_color[axis] = (left_color[axis] + left_color_step[axis]);
                right_color[axis] = (right_color[axis] + right_color_step[axis]);
            }
            right_uv[0] = (right_uv[0] + right_uv_step[0]);
            right_uv[1] = (right_uv[1] + right_uv_step[1]);
        }
        for (axis = 0; axis < 3; ++axis)
        {
            cur.u[axis] = left.u[axis];
            horizontal.u[axis] = (uint32)((sint32)(right.u[axis] - left.u[axis]) >> level);
            cur_color[axis] = left_color[axis];
            horizontal_color[axis] = (((sint32)(sint16)right_color[axis] - (sint32)(sint16)left_color[axis]) >> level);
        }
        for (component = 0; component < 2; ++component)
        {
            cur_uv[component] = left_uv[component];
            horizontal_uv[component] = (((sint32)(sint16)right_uv[component] - (sint32)(sint16)left_uv[component]) >> level);
        }
        subdiv_color_vertex(cursor, &cur, cur_uv, cur_color, precision);
        ++cursor;
        for (column = 1; column <= count; ++column)
        {
            for (axis = 0; axis < 3; ++axis)
            {
                cur.u[axis] += horizontal.u[axis];
                cur_color[axis] = (cur_color[axis] + horizontal_color[axis]);
            }
            cur_uv[0] = (cur_uv[0] + horizontal_uv[0]);
            cur_uv[1] = (cur_uv[1] + horizontal_uv[1]);
            subdiv_color_vertex(cursor, &cur, cur_uv, cur_color, precision);
            if (row != 0)
            {
                v1 = cursor - (count + 1);
                v0 = v1 - 1;
                v2 = cursor - 1;
                poly4 = poly4_cursor;
                poly4->p[1].attr = clut;
                poly4->p[0].attr = page;
                poly4->tag = original_tag;
                poly4->p[0].rgbc.rgbc = v0->color.rgbc;
                poly4->p[1].rgbc.rgbc = v1->color.rgbc;
                poly4->p[2].rgbc.rgbc = v2->color.rgbc;
                poly4->p[3].rgbc.rgbc = cursor->color.rgbc;
                poly4->p[0].uv.u = v0->uv.u;
                poly4->p[0].uv.v = v0->uv.v;
                poly4->p[1].uv.u = v1->uv.u;
                poly4->p[1].uv.v = v1->uv.v;
                poly4->p[2].uv.u = v2->uv.u;
                poly4->p[2].uv.v = v2->uv.v;
                poly4->p[3].uv.u = cursor->uv.u;
                poly4->p[3].uv.v = cursor->uv.v;
                project_triangle(&v0->pos, &v1->pos, &v2->pos);
                poly4->p[0].xy.xy = xport_gte_read_data(12);
                poly4->p[1].xy.xy = xport_gte_read_data(13);
                poly4->p[2].xy.xy = xport_gte_read_data(14);
                xport_gte_write_data(0, ((uint32)(uint16)cursor->pos.vx | ((uint32)(uint16)cursor->pos.vy << 16)));
                xport_gte_write_data(1, ((uint32)(uint16)cursor->pos.vz | ((uint32)(uint16)cursor->pos.pad << 16)));
                xport_gte_execute(0x180001);
                poly4->p[3].xy.xy = xport_gte_read_data(14);
                xport_gte_execute(0x168002E);
                proj_depth = (sint32)xport_gte_read_data(7);
                if (proj_depth > 0)
                {
                    setlen(poly4, 12);
                    poly4->p[0].rgbc.code = 60;
                    link_packet(&poly4_cursor->tag, ordering_table, proj_depth, shift);
                }
                ++poly4_cursor;
            }
            ++cursor;
        }
    }
    ordering_head = ordering_table;
    ordering_head.byte_offset += ((uint32)((sint32)((uint32)depth + 256) >> (shift & 31)) << 2);
    if (level == 1)
        return draw_subdiv_edges2(packet, ordering_head, depths);
    return draw_subdiv_edges4(packet, ordering_head, depths);
}

static uint32 emit_boundary_packet(uint32 *packet_tag, POINT_CT *points, DISPLAY_ORDERING ordering_head, const POINT_CT *const sources[4], uint16 clut, uint16 page, uint32 color, uint32 quad)
{
    uint32 vertex, val, tag = 0, head, result;
    points[1].attr = clut;
    points[0].attr = page;
    *packet_tag = quad == 0 ? 0x09000000 : 0x0C000000;
    for (vertex = 0; vertex < (quad == 0 ? 3u : 4u); ++vertex)
    {
        val = sources[vertex]->rgbc.rgbc;
        if ((quad == 0 && vertex == 0) || (quad != 0 && vertex == 3))
            tag = *packet_tag;
        if (vertex == 0)
            val = (val & 0xFFFFFF) | color;
        points[vertex].rgbc.rgbc = val;
        points[vertex].uv.u = sources[vertex]->uv.u;
        points[vertex].uv.v = sources[vertex]->uv.v;
        points[vertex].xy.x = sources[vertex]->xy.x;
        points[vertex].xy.y = sources[vertex]->xy.y;
    }
    head = ordering_head.tags[(sint32)ordering_head.byte_offset / 4];
    *packet_tag = (tag & 0xFF000000) | (head & 0xFFFFFF);
    head = ordering_head.tags[(sint32)ordering_head.byte_offset / 4];
    result = (head & 0xFF000000) | gpu_packet_link((uintptr_t)packet_tag);
    ordering_head.tags[(sint32)ordering_head.byte_offset / 4] = result;
    return result;
}

static intptr draw_subdiv_edges2(POLY_GT4 *packet, DISPLAY_ORDERING ordering_head, const uint32 depths[4])
{
    uint8 *base;
    POLY_GT3 *poly3;
    const POLY_GT4 *src_polys;
    static const uint32 depth_indices[4] = {1, 3, 2, 0};
    static const uint8 src_vertices[4][3][2] = {{{0, 0}, {1, 1}, {0, 1}}, {{1, 1}, {3, 3}, {1, 3}}, {{2, 2}, {2, 3}, {3, 3}}, {{0, 0}, {0, 2}, {2, 2}}};
    uint32 offset, limit, bank, color, edge, vertex, result;
    const POINT_CT *sources[4];
    uint16 clut, page;
    sint32 cur_depth, next_depth;

    FUNCTION_MARKER(0x8007526C, "SLUS_006.10");
    offset = packet_pool.used;
    limit = packet_pool.limit;
    if (limit < offset + 160)
        return 1;
    bank = disp_state.bank;
    clut = packet->p[1].attr;
    page = packet->p[0].attr;
    color = packet->p[0].rgbc.rgbc;
    packet_pool.used = offset + 160;
    cur_depth = depths[0];
    base = packet_pool.buffers[bank];
    color &= 0xF7FFFFFF;
    poly3 = (POLY_GT3 *)(base + offset);
    src_polys = (const POLY_GT4 *)((const uint8 *)poly3 - 208);
    for (edge = 0; edge < 4; ++edge)
    {
        if (cur_depth > 0)
        {
            next_depth = depths[depth_indices[edge]];
            if (next_depth > 0)
            {
                for (vertex = 0; vertex < 3; ++vertex)
                    sources[vertex] = &src_polys[src_vertices[edge][vertex][0]].p[src_vertices[edge][vertex][1]];
                result = emit_boundary_packet(&poly3->tag, poly3->p, ordering_head, sources, clut, page, color, 0);
                if (edge == 3)
                    return result;
                cur_depth = depths[depth_indices[edge]];
            }
            else
            {
                cur_depth = next_depth;
                if (edge == 3)
                    return cur_depth;
            }
        }
        else
        {
            if (edge == 3)
                return cur_depth;
            cur_depth = depths[depth_indices[edge]];
        }
        ++poly3;
    }
    return cur_depth;
}

static intptr draw_subdiv_edges4(POLY_GT4 *packet, DISPLAY_ORDERING ordering_head, const uint32 depths[4])
{
    uint8 *quad_base, *triangle_base;
    POLY_GT3 *poly3;
    POLY_GT4 *poly4;
    const POLY_GT4 *src_polys;
    const POINT_CT *sources[4];
    static const uint32 depth_indices[4] = {1, 3, 2, 0};
    static const uint8 triangle_vertices[4][3][2] = {{{0, 0}, {3, 1}, {0, 1}}, {{3, 1}, {15, 3}, {3, 3}}, {{15, 3}, {12, 2}, {15, 2}}, {{12, 2}, {0, 0}, {12, 0}}};
    static const uint8 quad_vertices[4][4][2] = {{{0, 1}, {3, 1}, {2, 1}, {1, 1}}, {{3, 3}, {15, 3}, {11, 3}, {7, 3}}, {{15, 2}, {12, 2}, {13, 2}, {14, 2}}, {{12, 0}, {0, 0}, {4, 0}, {8, 0}}};
    uint32 offset, limit, bank, color;
    uint32 edge, vertex, result;
    uint16 clut, page;
    sint32 cur_depth, next_depth;

    FUNCTION_MARKER(0x80075728, "SLUS_006.10");
    offset = packet_pool.used;
    limit = packet_pool.limit;
    if (limit < offset + 368)
        return 1;
    clut = packet->p[1].attr;
    bank = disp_state.bank;
    page = packet->p[0].attr;
    color = packet->p[0].rgbc.rgbc;
    triangle_base = packet_pool.buffers[bank];
    quad_base = packet_pool.buffers[bank];
    color &= 0xF7FFFFFF;
    packet_pool.used = offset + 160;
    packet_pool.used = offset + 368;
    poly3 = (POLY_GT3 *)(triangle_base + offset);
    poly4 = (POLY_GT4 *)(quad_base + offset + 160);
    cur_depth = depths[0];
    src_polys = (const POLY_GT4 *)(triangle_base + offset - 832);
    for (edge = 0; edge < 4; ++edge)
    {
        if (cur_depth > 0)
        {
            next_depth = depths[depth_indices[edge]];
            if (next_depth > 0)
            {
                for (vertex = 0; vertex < 3; ++vertex)
                    sources[vertex] = &src_polys[triangle_vertices[edge][vertex][0]].p[triangle_vertices[edge][vertex][1]];
                emit_boundary_packet(&poly3->tag, poly3->p, ordering_head, sources, clut, page, color, 0);
                for (vertex = 0; vertex < 4; ++vertex)
                    sources[vertex] = &src_polys[quad_vertices[edge][vertex][0]].p[quad_vertices[edge][vertex][1]];
                result = emit_boundary_packet(&poly4->tag, poly4->p, ordering_head, sources, clut, page, color | 0x08000000, 1);
                if (edge == 3)
                    return result;
                cur_depth = depths[depth_indices[edge]];
            }
            else
            {
                cur_depth = next_depth;
                if (edge == 3)
                    return cur_depth;
            }
        }
        else
        {
            if (edge == 3)
                return cur_depth;
            cur_depth = depths[depth_indices[edge]];
        }
        ++poly3;
        ++poly4;
    }
    return cur_depth;
}

static uint32 quad_outside(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 x2, sint32 y2, sint32 x3, sint32 y3)
{
    uint32 clip0, clip1, clip2, clip3;
    sint32 width, height;

    FUNCTION_MARKER(0x80072154, "SLUS_006.10");
    clip0 = x0 < 0;
    width = (sint32)model_render.width;
    if (width < x0)
        clip0 |= 2;
    if (y0 < 0)
        clip0 |= 4;
    height = (sint32)model_render.height;
    if (height < y0)
        clip0 |= 8;
    clip1 = x1 < 0;
    if (width < x1)
        clip1 |= 2;
    if (y1 < 0)
        clip1 |= 4;
    if (height < y1)
        clip1 |= 8;
    clip2 = x2 < 0;
    if (width < x2)
        clip2 |= 2;
    if (y2 < 0)
        clip2 |= 4;
    if (height < y2)
        clip2 |= 8;
    clip3 = x3 < 0;
    width = (sint32)model_render.width;
    if (width < x3)
        clip3 |= 2;
    if (y3 < 0)
        clip3 |= 4;
    height = (sint32)model_render.height;
    if (height < y3)
        clip3 |= 8;
    return (clip0 & clip1 & clip2 & clip3) != 0;
}

static uint32 triangle_outside(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 x2, sint32 y2)
{
    uint32 clip0, clip1, clip2;
    sint32 width, height;

    FUNCTION_MARKER(0x80072344, "SLUS_006.10");
    clip0 = x0 < 0;
    width = (sint32)model_render.width;
    if (width < x0)
        clip0 |= 2;
    if (y0 < 0)
        clip0 |= 4;
    height = (sint32)model_render.height;
    if (height < y0)
        clip0 |= 8;
    clip1 = x1 < 0;
    if (width < x1)
        clip1 |= 2;
    if (y1 < 0)
        clip1 |= 4;
    if (height < y1)
        clip1 |= 8;
    clip2 = x2 < 0;
    if (width < x2)
        clip2 |= 2;
    if (y2 < 0)
        clip2 |= 4;
    if (height < y2)
        clip2 |= 8;
    return (clip0 & clip1 & clip2) != 0;
}

static intptr draw_flat_edges2(POLY_FT4 *packet, DISPLAY_ORDERING ordering, const uint32 depths[4])
{
    uint8 *buffer;
    const POLY_FT4 *src_polys;
    POLY_FT3 *poly3_cursor;
    const POINT_T *point;
    static const uint8 triangle_vertices[4][3][2] = {{{0, 0}, {1, 1}, {0, 1}}, {{1, 1}, {3, 3}, {1, 3}}, {{2, 2}, {2, 3}, {3, 3}}, {{0, 0}, {0, 2}, {2, 2}}};
    static const uint32 neighbor[4] = {1, 3, 2, 0};
    uint32 allocation, limit, reserved, buffer_idx, color;
    uint32 tex_22, tex_14, grp, corner;
    uint32 val, tag = 0, result;
    sint32 cur;
    POLY_FT3 *poly3;

    FUNCTION_MARKER(0x80072E84, "SLUS_006.10");
    allocation = packet_pool.used;
    limit = packet_pool.limit;
    reserved = allocation + 128;
    result = limit < reserved;
    if (result != 0)
        return result;
    buffer_idx = disp_state.bank;
    color = packet->rgbc.rgbc;
    tex_22 = packet->p[1].attr;
    tex_14 = packet->p[0].attr;
    packet_pool.used = reserved;
    buffer = packet_pool.buffers[buffer_idx];
    color &= 0xF7FFFFFF;
    cur = depths[0];
    poly3_cursor = (POLY_FT3 *)(buffer + allocation);
    src_polys = (const POLY_FT4 *)(buffer + allocation - 160);
    for (grp = 0; grp < 4; ++grp, ++poly3_cursor)
    {
        if (cur <= 0)
        {
            if (grp == 3)
                return cur;
            cur = depths[neighbor[grp]];
            continue;
        }
        cur = depths[neighbor[grp]];
        if (cur <= 0)
        {
            if (grp == 3)
                return cur;
            continue;
        }
        poly3 = poly3_cursor;
        poly3->tag = 0x07000000;
        poly3->p[1].attr = tex_22;
        poly3->p[0].attr = tex_14;
        poly3->rgbc.rgbc = color;
        for (corner = 0; corner < 3; ++corner)
        {
            point = &src_polys[triangle_vertices[grp][corner][0]].p[triangle_vertices[grp][corner][1]];
            val = point->uv.u;
            if (corner == 0)
                tag = poly3->tag;
            poly3->p[corner].uv.u = val;
            val = point->uv.v;
            poly3->p[corner].uv.v = val;
            val = (uint16)point->xy.x;
            poly3->p[corner].xy.x = (sint16)val;
            val = (uint16)point->xy.y;
            poly3->p[corner].xy.y = (sint16)val;
        }
        val = ordering.tags[(sint32)ordering.byte_offset / 4];
        poly3->tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
        val = ordering.tags[(sint32)ordering.byte_offset / 4];
        result = (val & 0xFF000000) | gpu_packet_link((uintptr_t)poly3_cursor);
        ordering.tags[(sint32)ordering.byte_offset / 4] = result;
        if (grp == 3)
            return result;
        cur = depths[neighbor[grp]];
    }
    return result;
}

static intptr draw_flat_edges4(POLY_FT4 *packet, DISPLAY_ORDERING ordering, const uint32 depths[4])
{
    uint8 *packet_base;
    const POLY_FT4 *src_polys;
    POLY_FT3 *poly3_cursor;
    POLY_FT4 *poly4_cursor;
    const POINT_T *point;
    static const uint8 triangle_vertices[4][3][2] = {{{0, 0}, {3, 1}, {0, 1}}, {{3, 1}, {15, 3}, {3, 3}}, {{15, 3}, {12, 2}, {15, 2}}, {{12, 2}, {0, 0}, {12, 0}}};
    static const uint8 quad_vertices[4][4][2] = {{{0, 1}, {3, 1}, {2, 1}, {1, 1}}, {{3, 3}, {15, 3}, {11, 3}, {7, 3}}, {{15, 2}, {12, 2}, {13, 2}, {14, 2}}, {{12, 0}, {0, 0}, {4, 0}, {8, 0}}};
    static const uint32 neighbor[4] = {1, 3, 2, 0};
    uint32 allocation, limit, reserved, buffer_idx;
    uint32 tex_22, tex_14, color, quad_color;
    uint32 grp, corner, val, tag = 0, result;
    sint32 cur;
    POLY_FT3 *poly3;
    POLY_FT4 *poly4;

    FUNCTION_MARKER(0x800732A0, "SLUS_006.10");
    allocation = packet_pool.used;
    limit = packet_pool.limit;
    reserved = allocation + 288;
    result = limit < reserved;
    if (result != 0)
        return result;
    color = packet->rgbc.rgbc;
    buffer_idx = disp_state.bank;
    tex_22 = packet->p[1].attr;
    tex_14 = packet->p[0].attr;
    packet_base = packet_pool.buffers[buffer_idx];
    color &= 0xF7FFFFFF;
    packet_pool.used = allocation + 128;
    packet_pool.used = reserved;
    poly3_cursor = (POLY_FT3 *)(packet_base + allocation);
    poly4_cursor = (POLY_FT4 *)(packet_base + allocation + 128);
    cur = depths[0];
    quad_color = color | 0x08000000;
    src_polys = (const POLY_FT4 *)(packet_base + allocation - 640);
    for (grp = 0; grp < 4; ++grp, ++poly3_cursor, ++poly4_cursor)
    {
        if (cur <= 0)
        {
            if (grp == 3)
                return cur;
            cur = depths[neighbor[grp]];
            continue;
        }
        cur = depths[neighbor[grp]];
        if (cur <= 0)
        {
            if (grp == 3)
                return cur;
            continue;
        }
        poly3 = poly3_cursor;
        poly3->tag = 0x07000000;
        poly3->p[1].attr = tex_22;
        poly3->p[0].attr = tex_14;
        poly3->rgbc.rgbc = color;
        for (corner = 0; corner < 3; ++corner)
        {
            point = &src_polys[triangle_vertices[grp][corner][0]].p[triangle_vertices[grp][corner][1]];
            val = point->uv.u;
            if (corner == 0)
                tag = poly3->tag;
            poly3->p[corner].uv.u = val;
            val = point->uv.v;
            poly3->p[corner].uv.v = val;
            val = (uint16)point->xy.x;
            poly3->p[corner].xy.x = (sint16)val;
            val = (uint16)point->xy.y;
            poly3->p[corner].xy.y = (sint16)val;
        }
        val = ordering.tags[(sint32)ordering.byte_offset / 4];
        poly3->tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
        val = ordering.tags[(sint32)ordering.byte_offset / 4];
        ordering.tags[(sint32)ordering.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)poly3_cursor);
        poly4 = poly4_cursor;
        poly4->p[1].attr = tex_22;
        poly4->p[0].attr = tex_14;
        poly4->tag = 0x09000000;
        poly4->rgbc.rgbc = quad_color;
        for (corner = 0; corner < 4; ++corner)
        {
            point = &src_polys[quad_vertices[grp][corner][0]].p[quad_vertices[grp][corner][1]];
            val = point->uv.u;
            poly4->p[corner].uv.u = val;
            val = point->uv.v;
            poly4->p[corner].uv.v = val;
            val = (uint16)point->xy.x;
            poly4->p[corner].xy.x = (sint16)val;
            val = (uint16)point->xy.y;
            poly4->p[corner].xy.y = (sint16)val;
        }
        tag = poly4->tag;
        val = ordering.tags[(sint32)ordering.byte_offset / 4];
        poly4->tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
        val = ordering.tags[(sint32)ordering.byte_offset / 4];
        result = (val & 0xFF000000) | gpu_packet_link((uintptr_t)poly4_cursor);
        ordering.tags[(sint32)ordering.byte_offset / 4] = result;
        if (grp == 3)
            return result;
        cur = depths[neighbor[grp]];
    }
    return result;
}

static void subdiv_flat_quad(POLY_FT4 *packet, DISPLAY_ORDERING ordering, const MODEL_VERTEX *vertices, sint32 depth)
{
    uint8 *packet_buffer;
    POLY_FT4 *poly4_cursor;
    DISPLAY_ORDERING bucket;
    SUBDIVISION_VERTEX rows[2][5] = {0};
    uint32 depths[4];
    uint32 shift, other_shift, level, scale, reserve, allocation, limit;
    uint32 buffer, header, color, tex_22, tex_14;
    VEC3 edge[2], edge_step[2];
    uint32 edge_uv[2][2], edge_uv_step[2][2];
    VEC3 cur, across;
    uint32 cur_uv[2], across_uv[2];
    VEC2 scr[4];
    uint32 clip[4];
    uint32 side, component, row, column, divisions;
    SUBDIVISION_VERTEX *row_base, *prev_row, *point, *old_left, *old_right, *new_left;
    uint32 val, other, tag;
    sint32 width, height, threshold, proj_depth;
    POLY_FT4 *poly;

    FUNCTION_MARKER(0x80073AD0, "SLUS_006.10");
    shift = model_render.ot_shift;
    other_shift = model_render.coord_shift;
    threshold = (sint32)model_render.lod_half_distance;
    shift = (shift - other_shift) & 31;
    if (depth >= threshold / 4)
        goto link_original;
    threshold = (sint32)model_render.lod_quarter_distance;
    level = depth < threshold / 4 ? 2 : 1;
    scale = level * 2;
    allocation = packet_pool.used;
    limit = packet_pool.limit;
    reserve = 40 << scale;
    if (limit < allocation + reserve)
        goto link_original;
    for (side = 0; side < 4; ++side)
    {
        scr[side].x = packet->p[side].xy.x;
        scr[side].y = packet->p[side].xy.y;
    }
    width = (sint32)model_render.width;
    height = (sint32)model_render.height;
    for (side = 0; side < 4; ++side)
    {
        if (side == 3)
            width = (sint32)model_render.width;
        clip[side] = scr[side].x < 0 ? 1 : 0;
        if (width < scr[side].x)
            clip[side] |= 2;
        if (scr[side].y < 0)
            clip[side] |= 4;
        if (side == 3)
            height = (sint32)model_render.height;
        if (height < scr[side].y)
            clip[side] |= 8;
    }
    if ((clip[0] & clip[1] & clip[2] & clip[3]) != 0)
        return;
    for (component = 0; component < 4; ++component)
        depths[component] = xport_gte_read_data(16 + component);
    buffer = disp_state.bank;
    allocation = packet_pool.used;
    packet_pool.used = allocation + reserve;
    color = packet->rgbc.rgbc;
    packet_buffer = packet_pool.buffers[buffer];
    header = packet->tag;
    tex_22 = packet->p[1].attr;
    val = (uint32)vertices[0].sdk.vx;
    tex_14 = packet->p[0].attr;
    edge[0].bits.x = val << scale;
    for (component = 1; component < 3; ++component)
        edge[0].u[component] = (uint32)vertices[0].coordinates[component] << scale;
    for (component = 0; component < 2; ++component)
        edge_uv[0][component] = (uint16)((component == 0 ? packet->p[0].uv.u : packet->p[0].uv.v) << scale);
    for (component = 0; component < 3; ++component)
        edge[1].u[component] = (uint32)vertices[1].coordinates[component] << scale;
    for (component = 0; component < 2; ++component)
        edge_uv[1][component] = (uint16)((component == 0 ? packet->p[1].uv.u : packet->p[1].uv.v) << scale);
    for (side = 0; side < 2; ++side)
    {
        for (component = 0; component < 3; ++component)
        {
            val = (uint32)vertices[2 + side].coordinates[component];
            other = (uint32)vertices[side].coordinates[component];
            edge_step[side].u[component] = (val - other) << level;
        }
        for (component = 0; component < 2; ++component)
        {
            val = (component == 0 ? packet->p[2 + side].uv.u : packet->p[2 + side].uv.v);
            other = (component == 0 ? packet->p[side].uv.u : packet->p[side].uv.v);
            edge_uv_step[side][component] = (uint16)((val - other) << level);
        }
    }
    poly4_cursor = (POLY_FT4 *)(packet_buffer + allocation);
    divisions = 1 << level;
    for (row = 0; row <= divisions; ++row)
    {
        if (row != 0)
        {
            for (side = 0; side < 2; ++side)
            {
                for (component = 0; component < 3; ++component)
                    edge[side].u[component] += edge_step[side].u[component];
                for (component = 0; component < 2; ++component)
                    edge_uv[side][component] = (uint16)(edge_uv[side][component] + edge_uv_step[side][component]);
            }
        }
        for (component = 0; component < 3; ++component)
        {
            cur.u[component] = edge[0].u[component];
            across.u[component] = (uint32)((sint32)(edge[1].u[component] - edge[0].u[component]) >> level);
        }
        for (component = 0; component < 2; ++component)
        {
            cur_uv[component] = edge_uv[0][component];
            val = (sint16)edge_uv[1][component];
            other = (sint16)edge_uv[0][component];
            across_uv[component] = (uint16)((sint32)(val - other) >> level);
        }
        row_base = rows[row & 1];
        prev_row = rows[(row - 1) & 1];
        for (column = 0; column <= divisions; ++column)
        {
            if (column != 0)
            {
                for (component = 0; component < 3; ++component)
                    cur.u[component] += across.u[component];
                for (component = 0; component < 2; ++component)
                    cur_uv[component] = (uint16)(cur_uv[component] + across_uv[component]);
            }
            point = row_base + column;
            point->pos.vx = (sint16)(cur.x >> scale);
            point->pos.vy = (sint16)(cur.y >> scale);
            point->pos.vz = (sint16)(cur.z >> scale);
            point->uv.u = (uint8)((sint32)(sint16)cur_uv[0] >> scale);
            point->uv.v = (uint8)((sint32)(sint16)cur_uv[1] >> scale);
            if (row == 0 || column == 0)
                continue;
            old_left = prev_row + column - 1;
            old_right = old_left + 1;
            new_left = point - 1;
            poly = poly4_cursor;
            poly->p[1].attr = tex_22;
            poly->p[0].attr = tex_14;
            poly->tag = header;
            poly->rgbc.rgbc = color;
            val = old_left->uv.u;
            poly->p[0].uv.u = val;
            val = old_left->uv.v;
            poly->p[0].uv.v = val;
            val = old_right->uv.u;
            poly->p[1].uv.u = val;
            val = old_right->uv.v;
            poly->p[1].uv.v = val;
            val = new_left->uv.u;
            poly->p[2].uv.u = val;
            val = new_left->uv.v;
            poly->p[2].uv.v = val;
            val = point->uv.u;
            poly->p[3].uv.u = val;
            val = point->uv.v;
            poly->p[3].uv.v = val;
            val = ((uint32)(uint16)old_left->pos.vx | ((uint32)(uint16)old_left->pos.vy << 16));
            xport_gte_write_data(0, val);
            val = (uint16)old_left->pos.vz;
            xport_gte_write_data(1, val);
            val = ((uint32)(uint16)old_right->pos.vx | ((uint32)(uint16)old_right->pos.vy << 16));
            xport_gte_write_data(2, val);
            val = (uint16)old_right->pos.vz;
            xport_gte_write_data(3, val);
            val = ((uint32)(uint16)new_left->pos.vx | ((uint32)(uint16)new_left->pos.vy << 16));
            xport_gte_write_data(4, val);
            val = (uint16)new_left->pos.vz;
            xport_gte_write_data(5, val);
            xport_gte_execute(0x280030);
            poly->p[0].xy.xy = xport_gte_read_data(12);
            poly->p[1].xy.xy = xport_gte_read_data(13);
            poly->p[2].xy.xy = xport_gte_read_data(14);
            val = ((uint32)(uint16)point->pos.vx | ((uint32)(uint16)point->pos.vy << 16));
            xport_gte_write_data(0, val);
            val = (uint16)point->pos.vz;
            xport_gte_write_data(1, val);
            xport_gte_execute(0x180001);
            xport_gte_execute(0x168002E);
            proj_depth = (sint32)xport_gte_read_data(7);
            if (proj_depth > 0)
            {
                poly->p[3].xy.xy = xport_gte_read_data(14);
                bucket = ordering;
                bucket.byte_offset += ((uint32)(proj_depth >> shift) << 2);
                tag = poly->tag;
                val = bucket.tags[(sint32)bucket.byte_offset / 4];
                poly->tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
                val = bucket.tags[(sint32)bucket.byte_offset / 4];
                bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)poly4_cursor);
            }
            ++poly4_cursor;
        }
    }
    bucket = ordering;
    bucket.byte_offset += ((uint32)((sint32)((uint32)depth + 256) >> shift) << 2);
    if (level == 1)
        draw_flat_edges2(packet, bucket, depths);
    else
        draw_flat_edges4(packet, bucket, depths);
    return;

link_original:
    bucket = ordering;
    bucket.byte_offset += ((uint32)(depth >> shift) << 2);
    tag = packet->tag;
    val = bucket.tags[(sint32)bucket.byte_offset / 4];
    packet->tag = (uint32)((tag & 0xFF000000) | (val & 0xFFFFFF));
    val = bucket.tags[(sint32)bucket.byte_offset / 4];
    bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
}

uint8 *model_packet_pool_init(uint32 bytes)
{
    FUNCTION_MARKER(0x80072100, "SLUS_006.10");
    {
        sint32 operation_ok = !(bytes == 0 || bytes > MODEL_PACKET_CAPACITY || (bytes & 3) != 0 || !gpu_register_packet_range(packet_buffers, (uint32)sizeof(packet_buffers)));
        XASSERT(operation_ok);
    }
    packet_pool.buffers[0] = (uint8 *)&packet_buffers[0];
    packet_pool.buffers[1] = (uint8 *)&packet_buffers[1];
    packet_pool.capacity = bytes;
    return packet_pool.buffers[1];
}

uint32 model_packet_pool_reset(void)
{
    uint32 val;

    FUNCTION_MARKER(0x80072140, "SLUS_006.10");
    val = packet_pool.capacity;
    packet_pool.used = 0;
    packet_pool.limit = val;
    return val;
}

sint32 model_set_view_pos(const VECTOR *pos)
{
    sint32 result;

    FUNCTION_MARKER(0x80078C18, "SLUS_006.10");
    hd_camera_position(pos);
    model_render.view_base.sdk.t[0] = pos->vx >> 8;
    model_render.view_base.sdk.t[1] = pos->vy >> 8;
    model_render.view_base.sdk.t[2] = pos->vz >> 8;
    model_render.view.sdk.t[0] = pos->vx >> 8;
    model_render.view.sdk.t[1] = pos->vy >> 8;
    result = pos->vz >> 8;
    model_render.view.sdk.t[2] = result;
    return result;
}

static sint32 scale_view_matrices(void)
{
    static const sint16 *const sources[18] = {&model_render.view_base.sdk.m[0][0], &model_render.view_base.sdk.m[0][1], &model_render.view_base.sdk.m[0][2], &model_render.billboard.sdk.m[0][0], &model_render.billboard.sdk.m[0][1], &model_render.billboard.sdk.m[0][2], &model_render.scr.sdk.m[0][0], &model_render.scr.sdk.m[0][1], &model_render.scr.sdk.m[0][2], &model_render.view_base.sdk.m[1][0], &model_render.view_base.sdk.m[1][1], &model_render.view_base.sdk.m[1][2], &model_render.billboard.sdk.m[1][0], &model_render.billboard.sdk.m[1][1], &model_render.billboard.sdk.m[1][2], &model_render.scr.sdk.m[1][0], &model_render.scr.sdk.m[1][1], &model_render.scr.sdk.m[1][2]};
    static sint16 *const destinations[18] = {&model_render.view.sdk.m[0][0], &model_render.view.sdk.m[0][1], &model_render.view.sdk.m[0][2], &model_render.billboard.sdk.m[0][0], &model_render.billboard.sdk.m[0][1], &model_render.billboard.sdk.m[0][2], &model_render.scr.sdk.m[0][0], &model_render.scr.sdk.m[0][1], &model_render.scr.sdk.m[0][2], &model_render.view.sdk.m[1][0], &model_render.view.sdk.m[1][1], &model_render.view.sdk.m[1][2], &model_render.billboard.sdk.m[1][0], &model_render.billboard.sdk.m[1][1], &model_render.billboard.sdk.m[1][2], &model_render.scr.sdk.m[1][0], &model_render.scr.sdk.m[1][1], &model_render.scr.sdk.m[1][2]};
    static const sint16 *const tail_sources[9] = {&model_render.view_base.sdk.m[2][0], &model_render.view_base.sdk.m[2][1], &model_render.view_base.sdk.m[2][2], &model_render.billboard.sdk.m[2][0], &model_render.billboard.sdk.m[2][1], &model_render.billboard.sdk.m[2][2], &model_render.scr.sdk.m[2][0], &model_render.scr.sdk.m[2][1], &model_render.scr.sdk.m[2][2]};
    static sint16 *const tail_destinations[9] = {&model_render.view.sdk.m[2][0], &model_render.view.sdk.m[2][1], &model_render.view.sdk.m[2][2], &model_render.billboard.sdk.m[2][0], &model_render.billboard.sdk.m[2][1], &model_render.billboard.sdk.m[2][2], &model_render.scr.sdk.m[2][0], &model_render.scr.sdk.m[2][1], &model_render.scr.sdk.m[2][2]};
    uint32 width;
    uint32 multiplier;
    uint32 height;
    uint32 shift;
    sint32 x_scale;
    sint32 y_scale;
    sint32 product;
    sint32 pending = 0;
    sint32 tail[9];
    uint32 idx;

    FUNCTION_MARKER(0x800784C4, "SLUS_006.10");
    width = model_render.x_scale;
    multiplier = model_render.scale;
    x_scale = (sint32)(width * multiplier) / 4096;
    height = model_render.y_scale;
    shift = model_render.coord_shift & 31;
    x_scale >>= shift;
    y_scale = ((sint32)(height * multiplier) / 4096) >> shift;
    for (idx = 0; idx < 18; ++idx)
    {
        product = ((uint32)(sint32)*sources[idx] * (uint32)(idx < 9 ? x_scale : y_scale));
        if (idx != 0)
            *destinations[idx - 1] = pending;
        pending = product / 4096;
    }
    tail[0] = *tail_sources[0];
    shift = model_render.coord_shift & 31;
    for (idx = 1; idx < 9; ++idx)
        tail[idx] = *tail_sources[idx];
    *destinations[17] = pending;
    for (idx = 0; idx < 9; ++idx)
        *tail_destinations[idx] = (tail[idx] >> shift);
    return pending;
}

uint32 model_aim_view(const VECTOR *pos, sint32 roll)
{
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 val;
    uint32 origin;
    uint32 absolute;
    uint32 half;
    uint32 n_x;
    uint32 n_y;
    uint32 n_z;
    uint32 m00;
    uint32 m02;
    uint32 m10;
    uint32 m12;
    uint32 c;
    uint32 s;
    sint32 len;
    sint32 horizontal;
    MATRIX rotation = {0};
    MATRIX flip = {0};
    MATRIX *matrix;

    FUNCTION_MARKER(0x80079094, "SLUS_006.10");
    hd_camera_aim(pos, roll);
    val = (uint32)pos->vx;
    origin = model_render.view_base.sdk.t[0];
    x = (uint32)((sint32)val >> 8) - origin;
    val = (uint32)pos->vy;
    origin = model_render.view_base.sdk.t[1];
    y = (uint32)((sint32)val >> 8) - origin;
    val = (uint32)pos->vz;
    origin = model_render.view_base.sdk.t[2];
    z = (uint32)((sint32)val >> 8) - origin;
    for (;;)
    {
        absolute = (sint32)x < 0 ? 0 - x : x;
        if ((sint32)absolute < 16385)
        {
            absolute = (sint32)y < 0 ? 0 - y : y;
            if ((sint32)absolute < 16385)
            {
                absolute = (sint32)z < 0 ? 0 - z : z;
                if ((sint32)absolute < 16385)
                    break;
            }
        }
        x = ((sint32)x / 2);
        y = ((sint32)y / 2);
        z = ((sint32)z / 2);
    }
    len = (sint32)fixed_sqrt(x * x + y * y + z * z);
    val = x << 12;
    if (len <= 0)
        return val;
    half = (len / 2);
    n_x = (uint32)fixed_divide_checked((sint32)(val + half), len);
    n_z = (uint32)fixed_divide_checked((sint32)((z << 12) + half), len);
    val = n_x * n_x + n_z * n_z;
    n_y = (uint32)fixed_divide_checked((sint32)((y << 12) + half), len);
    horizontal = (sint32)fixed_sqrt(val);
    if (horizontal > 0)
    {
        half = (horizontal / 2);
        m00 = (uint32)fixed_divide_checked((sint32)((n_z << 12) + half), horizontal);
        val = 0 - ((n_x << 12) + half);
        m02 = (uint32)fixed_divide_checked((sint32)val, horizontal);
        val = 0 - (n_x * n_y + half);
        m10 = (uint32)fixed_divide_checked((sint32)val, horizontal);
        val = 0 - (n_y * n_z + half);
        m12 = (uint32)fixed_divide_checked((sint32)val, horizontal);
        model_render.view_base.sdk.m[0][1] = 0;
        model_render.view_base.sdk.m[1][1] = (sint16)(horizontal);
        model_render.view_base.sdk.m[2][0] = (sint16)(n_x);
        model_render.view_base.sdk.m[2][1] = (sint16)(n_y);
        model_render.view_base.sdk.m[2][2] = (sint16)(n_z);
        model_render.billboard.sdk.m[0][1] = 0;
        model_render.billboard.sdk.m[0][2] = 0;
        model_render.billboard.sdk.m[1][0] = 0;
        model_render.billboard.sdk.m[1][1] = (sint16)(horizontal);
        model_render.billboard.sdk.m[2][0] = 0;
        model_render.billboard.sdk.m[2][1] = (sint16)(n_y);
        model_render.billboard.sdk.m[2][2] = (sint16)(horizontal);
        model_render.billboard.sdk.m[0][0] = ONE;
        model_render.billboard.sdk.m[1][2] = (sint16)(0 - n_y);
        model_render.view_base.sdk.m[0][0] = (sint16)(m00);
        model_render.view_base.sdk.m[0][2] = (sint16)(m02);
        model_render.view_base.sdk.m[1][0] = (sint16)(m10);
        model_render.view_base.sdk.m[1][2] = (sint16)(m12);
    }
    else
    {
        model_render.view_base.sdk.m[0][0] = ONE;
        model_render.view_base.sdk.m[0][1] = 0;
        model_render.view_base.sdk.m[0][2] = 0;
        model_render.view_base.sdk.m[1][0] = 0;
        model_render.view_base.sdk.m[1][1] = 0;
        model_render.view_base.sdk.m[1][2] = (sint16)(0 - n_y);
        model_render.view_base.sdk.m[2][0] = 0;
        model_render.view_base.sdk.m[2][1] = (sint16)(n_y);
        model_render.view_base.sdk.m[2][2] = 0;
    }
    c = (uint32)rcos(roll);
    s = (uint32)rsin(roll);
    rotation.m[0][0] = (sint16)c;
    rotation.m[1][1] = (sint16)c;
    rotation.m[0][1] = (sint16)(0 - s);
    rotation.m[0][2] = 0;
    rotation.m[1][0] = (sint16)s;
    rotation.m[1][2] = 0;
    rotation.m[2][0] = 0;
    rotation.m[2][1] = 0;
    rotation.m[2][2] = ONE;
    matrix = &model_render.view_base.sdk;
    MulMatrix2(&rotation, matrix);
    flip.m[0][0] = -ONE;
    flip.m[0][1] = 0;
    flip.m[0][2] = 0;
    flip.m[1][0] = 0;
    flip.m[1][1] = ONE;
    flip.m[1][2] = 0;
    flip.m[2][0] = 0;
    flip.m[2][1] = 0;
    flip.m[2][2] = -ONE;
    matrix = &model_render.billboard.sdk;
    MulMatrix2(&rotation, matrix);
    MulMatrix0(matrix, &flip, matrix);
    MulMatrix0(&rotation, &flip, &model_render.scr.sdk);
    return (uint32)scale_view_matrices();
}

sint32 model_set_view_angles(const SVECTOR *angles)
{
    uint32 cos_x;
    uint32 sin_x;
    uint32 cos_y;
    uint32 sin_y;
    uint32 cos_z;
    uint32 sin_z;
    uint32 xy;
    uint32 xc;
    uint32 left;
    uint32 right;
    sint32 m01;
    sint32 m11;

    FUNCTION_MARKER(0x80078DDC, "SLUS_006.10");
    hd_camera_angles(angles);
    cos_x = (uint32)rcos(angles->vx);
    sin_x = (uint32)rsin(angles->vx);
    cos_y = (uint32)rcos(angles->vy);
    sin_y = (uint32)rsin(angles->vy);
    cos_z = (uint32)rcos(angles->vz);
    sin_z = (uint32)rsin(angles->vz);
    xy = ((sint32)(sin_x * sin_y + 2048) >> 12);
    xc = ((sint32)(sin_x * cos_y + 2048) >> 12);
    model_render.view_base.sdk.m[2][1] = (sint16)((uint16)sin_x);
    model_render.billboard.sdk.m[0][0] = (sint16)((uint16)cos_z);
    model_render.billboard.sdk.m[1][0] = (sint16)((uint16)sin_z);
    model_render.billboard.sdk.m[2][0] = 0;
    model_render.billboard.sdk.m[2][1] = (sint16)((uint16)sin_x);
    model_render.billboard.sdk.m[2][2] = (sint16)((uint16)cos_x);
    model_render.scr.sdk.m[0][0] = (sint16)((uint16)cos_z);
    model_render.scr.sdk.m[0][2] = 0;
    model_render.scr.sdk.m[1][0] = (sint16)((uint16)sin_z);
    model_render.scr.sdk.m[1][1] = (sint16)((uint16)cos_z);
    model_render.scr.sdk.m[1][2] = 0;
    model_render.scr.sdk.m[2][0] = 0;
    model_render.scr.sdk.m[2][1] = 0;
    model_render.scr.sdk.m[0][1] = (sint16)((uint16)(0 - sin_z));
    model_render.scr.sdk.m[2][2] = ONE;
    m01 = (sint32)((0 - cos_x) * sin_z + 2048) >> 12;
    m11 = (sint32)(cos_x * cos_z + 2048) >> 12;
    model_render.view_base.sdk.m[0][1] = (sint16)((uint16)m01);
    model_render.view_base.sdk.m[1][1] = (sint16)((uint16)m11);
    model_render.billboard.sdk.m[0][1] = (sint16)((uint16)m01);
    model_render.billboard.sdk.m[1][1] = (sint16)((uint16)m11);
    model_render.view_base.sdk.m[2][0] = (sint16)((uint16)((sint32)((0 - cos_x) * sin_y + 2048) >> 12));
    model_render.view_base.sdk.m[2][2] = (sint16)((uint16)((sint32)(cos_x * cos_y + 2048) >> 12));
    model_render.billboard.sdk.m[0][2] = (sint16)((uint16)((sint32)(sin_x * cos_z + 2048) >> 12));
    model_render.billboard.sdk.m[1][2] = (sint16)((uint16)((sint32)((0 - sin_x) * cos_z + 2048) >> 12));
    left = cos_y * cos_z;
    right = xy * sin_z;
    model_render.view_base.sdk.m[0][0] = (sint16)((uint16)((sint32)(left - right + 2048) >> 12));
    left = sin_y * cos_z;
    right = xc * sin_z;
    model_render.view_base.sdk.m[0][2] = (sint16)((uint16)((sint32)(left + right + 2048) >> 12));
    left = cos_y * sin_z;
    right = xy * cos_z;
    model_render.view_base.sdk.m[1][0] = (sint16)((uint16)((sint32)(left + right + 2048) >> 12));
    left = sin_y * sin_z;
    right = xc * cos_z;
    model_render.view_base.sdk.m[1][2] = (sint16)((uint16)((sint32)(left - right + 2048) >> 12));
    return scale_view_matrices();
}

uint32 model_update_clip_planes(void)
{
    uint32 scale;
    uint32 x_product;
    uint32 y_product;
    sint32 half_width;
    sint32 half_height;
    uint32 distance;
    uint32 distance_sq;
    uint32 distance_fixed;
    sint32 root;
    sint32 sq;
    sint32 ratio;
    sint32 offset;
    sint32 divisor;
    sint32 horizontal;
    sint32 vertical;
    uint32 centered;

    FUNCTION_MARKER(0x8007880C, "SLUS_006.10");
    x_product = model_render.width * model_render.x_scale;
    ratio = (sint32)x_product / 4096;
    scale = model_render.scale;
    x_product = (uint32)ratio * scale;
    if ((sint32)x_product < 0)
        x_product += 4095;
    y_product = model_render.height * model_render.y_scale;
    half_width = (sint32)x_product >> 12;
    ratio = (sint32)y_product / 4096;
    y_product = (uint32)ratio * scale;
    if ((sint32)y_product < 0)
        y_product += 4095;
    distance = model_render.proj_distance;
    sq = ((uint32)half_width * (uint32)half_width);
    half_height = (sint32)y_product >> 12;
    distance_sq = distance * distance;
    root = (sint32)fixed_sqrt((uint32)(sq / 4) + distance_sq);
    distance_fixed = distance << 12;
    ratio = fixed_divide_checked((sint32)distance_fixed, root);
    centered = ((sint32)((uint32)half_width + (x_product >> 31)) >> 1);
    offset = fixed_divide_checked((sint32)(0 - (centered << 12)), root);
    divisor = (sint32)model_render.scale;
    horizontal = fixed_divide_checked((sint32)((uint32)ratio << 12), divisor);
    vertical = fixed_divide_checked((sint32)((uint32)offset << 12), divisor);
    sq = ((uint32)half_height * (uint32)half_height);
    model_render.clip_x.y = (sint16)(0);
    model_render.clip_x.x = (sint16)((uint16)horizontal);
    model_render.clip_x.z = (sint16)((uint16)vertical);
    root = (sint32)fixed_sqrt((uint32)(sq / 4) + distance_sq);
    ratio = fixed_divide_checked((sint32)distance_fixed, root);
    centered = ((sint32)((uint32)half_height + (y_product >> 31)) >> 1);
    offset = fixed_divide_checked((sint32)(0 - (centered << 12)), root);
    divisor = (sint32)model_render.scale;
    horizontal = fixed_divide_checked((sint32)((uint32)ratio << 12), divisor);
    vertical = fixed_divide_checked((sint32)((uint32)offset << 12), divisor);
    model_render.clip_y.x = (sint16)(0);
    model_render.clip_y.y = (sint16)((uint16)horizontal);
    model_render.clip_y.z = (sint16)((uint16)vertical);
    return vertical;
}

void model_set_ot_shift(uint32 shift)
{
    FUNCTION_MARKER(0x8006C590, "SLUS_006.10");
    model_render.ot_shift = shift;
}

uint32 model_set_proj_distance(sint32 distance)
{
    FUNCTION_MARKER(0x80078B38, "SLUS_006.10");
    SetGeomScreen(distance);
    model_render.proj_distance = ((uint32)distance);
    return model_update_clip_planes();
}

void model_set_depth_range(uint32 near_depth, uint32 far_depth)
{
    FUNCTION_MARKER(0x80078B6C, "SLUS_006.10");
    model_render.near_depth = near_depth;
    model_render.far_depth = far_depth;
}

void model_set_back_color(sint32 r, sint32 g, sint32 b)
{
    FUNCTION_MARKER(0x80078C98, "SLUS_006.10");
    hd_set_back_color(r, g, b);
    SetBackColor((sint32)((uint32)r << 1), (sint32)((uint32)g << 1), (sint32)((uint32)b << 1));
}

sint16 *model_set_light(uint32 idx, const VEC3_16 *dir, uint32 r, uint32 g, uint32 b)
{
    sint16 *result;

    FUNCTION_MARKER(0x80078CC0, "SLUS_006.10");
    model_render.light.m[idx][0] = dir->x;
    model_render.light.m[idx][1] = dir->y;
    model_render.light.m[idx][2] = dir->z;
    model_render.color.m[0][idx] = (sint16)(r << 5);
    result = &model_render.color.m[1][idx];
    *result = (g << 5);
    model_render.color.m[2][idx] = (sint16)(b << 5);
    return result;
}

uint32 model_set_fog(sint32 near_distance, sint32 far_distance, sint32 r, sint32 g, sint32 b)
{
    FUNCTION_MARKER(0x80078D8C, "SLUS_006.10");
    SetFarColor(r, g, b);
    return SetFogNearFar(near_distance, far_distance, (sint32)model_render.proj_distance);
}

void model_apply_light_matrix(const MATRIX *matrix)
{
    MATRIX result;

    FUNCTION_MARKER(0x80078D48, "SLUS_006.10");
    hd_set_lighting(matrix);
    MulMatrix0(&model_render.light, (MATRIX *)matrix, &result);
    SetLightMatrix(&result);
    SetColorMatrix(&model_render.color);
}

void model_set_view_center(sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x80078AFC, "SLUS_006.10");
    SetGeomOffset(x, y);
    model_render.view_center_x = (uint32)x;
    model_render.view_center_y = (uint32)y;
}

sint32 model_update_lod_distances(sint32 pos, sint32 near_distance, sint32 far_distance)
{
    uint32 mag;
    sint32 candidate;
    sint32 result;

    FUNCTION_MARKER(0x800720B0, "SLUS_006.10");
    mag = pos;
    if (pos < 0)
        mag = 0 - mag;
    candidate = ((uint32)far_distance - ((mag << 3) - 500));
    if (candidate < far_distance)
        far_distance = candidate;
    if (near_distance < far_distance)
        near_distance = far_distance;
    model_render.lod_half_distance = (uint32)(near_distance >> 1);
    result = near_distance >> 2;
    model_render.lod_quarter_distance = (uint32)result;
    return result;
}

uint32 model_update_distance_reference(const MATRIX *rotation, const VEC3 *pos)
{
    uint32 scale_z, scale_y, z_term, y_term, base, row, result;
    VEC3 ref_pos;

    FUNCTION_MARKER(0x8006E1C8, "SLUS_006.10");
    z_term = (uint32)rotation->m[0][2];
    scale_z = model_render.reference_offset_z;
    z_term = ((sint32)(z_term * scale_z) / 16);
    y_term = (uint32)rotation->m[0][1];
    scale_y = model_render.reference_offset_y;
    y_term = ((sint32)(y_term * scale_y) / 16);
    base = pos->bits.x;
    ref_pos.bits.x = base + z_term + y_term;
    for (row = 1; row < 3; ++row)
    {
        z_term = (uint32)rotation->m[row][2];
        z_term = ((sint32)(z_term * scale_z) / 16);
        y_term = (uint32)rotation->m[row][1];
        y_term = ((sint32)(y_term * scale_y) / 16);
        base = pos->u[row];
        ref_pos.u[row] = base + z_term + y_term;
    }
    model_render.distance_reference.bits.x = (uint32)(ref_pos.x >> 8);
    result = (uint32)(ref_pos.z >> 8);
    model_render.distance_reference.bits.y = (uint32)(ref_pos.y >> 8);
    model_render.distance_reference.bits.z = result;
    return result;
}

void model_set_reference_offset(sint32 lower, sint32 upper)
{
    FUNCTION_MARKER(0x8006E2E4, "SLUS_006.10");
    model_render.reference_offset_y = (uint32)lower;
    model_render.reference_offset_z = (uint32)upper;
}

uint32 model_build_rotation(MATRIX *matrix, const SVECTOR *angles)
{
    uint32 c_x;
    uint32 s_x;
    uint32 c_y;
    uint32 s_y;
    uint32 c_z;
    uint32 s_z;
    uint32 xy_s;
    uint32 xy_c;
    sint32 val;

    FUNCTION_MARKER(0x80079548, "SLUS_006.10");
    c_x = (uint32)rcos(angles->vx);
    s_x = (uint32)rsin(angles->vx);
    c_y = (uint32)rcos(angles->vy);
    s_y = (uint32)rsin(angles->vy);
    c_z = (uint32)rcos(angles->vz);
    s_z = (uint32)rsin(angles->vz);
    xy_s = ((sint32)(s_x * s_y + 2048) >> 12);
    xy_c = ((sint32)(s_x * c_y + 2048) >> 12);
    matrix->m[1][2] = (sint16)(0 - s_x);
    matrix->m[0][2] = (sint16)((sint32)(c_x * s_y + 2048) >> 12);
    matrix->m[1][0] = (sint16)((sint32)(c_x * s_z + 2048) >> 12);
    matrix->m[1][1] = (sint16)((sint32)(c_x * c_z + 2048) >> 12);
    matrix->m[2][2] = (sint16)((sint32)(c_x * c_y + 2048) >> 12);
    matrix->m[0][0] = (sint16)((sint32)(c_y * c_z + xy_s * s_z + 2048) >> 12);
    matrix->m[0][1] = (sint16)((sint32)((0 - c_y) * s_z + xy_s * c_z + 2048) >> 12);
    matrix->m[2][0] = (sint16)((sint32)((0 - s_y) * c_z + xy_c * s_z + 2048) >> 12);
    val = (sint32)(s_y * s_z + xy_c * c_z + 2048) >> 12;
    matrix->m[2][1] = (sint16)(val);
    return val;
}

uint32 model_init_lighting(uint32 radius, uint8 *dst, const uint8 *color, uint32 height)
{
    uint32 idx;
    uint32 channel;
    uint32 root;
    uint32 product;
    uint32 quotient;
    uint32 weight;
    uint32 from;
    uint32 to;
    uint32 candidate;
    uint32 lower;
    uint32 upper;
    uint32 *table;
    uint32 packed;
    uint32 components[3];

    FUNCTION_MARKER(0x8006E2F4, "SLUS_006.10");
    hd_distance_lighting(radius, color, height);
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    lighting.half_color[0] = (uint8)(color[0] >> 1);
    lighting.half_color[1] = (uint8)(color[1] >> 1);
    lighting.half_color[2] = (uint8)(color[2] >> 1);
    lighting.radius = radius;
    lighting.radius_sq = radius * radius;
    packed = dst[1] << 8;
    packed |= dst[0];
    packed |= dst[2] << 16;
    lighting.base_color = packed;
    for (idx = 0; idx < 1024; ++idx)
    {
        root = SquareRoot0((sint32)(idx << 15));
        product = height * root;
        quotient = (uint32)fixed_divide_checked((sint32)product, (sint32)radius);
        weight = height - quotient;
        for (channel = 0; channel < 3; ++channel)
        {
            if (channel == 0)
            {
                from = dst[0];
                to = color[0];
            }
            else
            {
                to = color[channel];
                from = dst[channel];
            }
            product = weight * (to - from);
            candidate = from + (uint32)((sint32)product / 256);
            lower = dst[channel];
            if ((sint32)lower < (sint32)candidate)
                lower = candidate;
            upper = color[channel];
            if ((sint32)lower < (sint32)upper)
                upper = lower;
            components[channel] = upper;
        }
        table = lighting.colors[0];
        packed = components[0] | (components[1] << 8) | (components[2] << 16);
        table[idx] = packed;
        for (channel = 0; channel < 3; ++channel)
        {
            if (channel == 1)
            {
                product = height * root;
                quotient = (uint32)fixed_divide_checked((sint32)product, (sint32)radius);
                weight = height - quotient;
            }
            to = lighting.half_color[channel];
            from = light_black[channel];
            product = weight * (to - from);
            candidate = from + (uint32)((sint32)product / 256);
            lower = from;
            if ((sint32)lower < (sint32)candidate)
                lower = candidate;
            upper = to;
            if ((sint32)lower < (sint32)upper)
                upper = lower;
            components[channel] = upper;
        }
        table = lighting.colors[1];
        packed = components[0] | (components[1] << 8) | (components[2] << 16);
        table[idx] = packed;
    }
    return 0;
}

static uint32 used_vertex_count(const MODEL_RESOURCE *model_input)
{
    uint16 wire_halfword_1, wire_halfword_2;

    uint32 rec_offset, type, stride, vertices, vertex;
    sint32 count, idx, val, maximum = 0;

    FUNCTION_MARKER(0x8006C964, "SLUS_006.10");
    XASSERT(!(model_input->capacity < 4 || (uint32)(48) > model_input->capacity - 4));
    rec_offset = ((const MODEL_HEADER *)model_input->bytes)->stream_offset;
    for (;;)
    {
        XASSERT(!(model_input->capacity < 1 || (uint32)(rec_offset + 3) > model_input->capacity - 1));
        if (model_input->bytes[(uint32)(rec_offset + 3)] == 0)
            break;
        XASSERT(!(model_input->capacity < 1 || (uint32)(rec_offset + 2) > model_input->capacity - 1));
        type = model_input->bytes[(uint32)(rec_offset + 2)];
        XASSERT(!(model_input->capacity < 2 || rec_offset > model_input->capacity - 2));
        count = (sint16)(memcpy(&wire_halfword_1, model_input->bytes + rec_offset, sizeof(wire_halfword_1)), wire_halfword_1);
        rec_offset += 4;
        if (type > 3)
            continue;
        stride = type < 2 ? 20 : type == 2 ? 44 : 56;
        vertices = (type & 1) != 0 ? 4 : 3;
        for (idx = 0; idx < count; ++idx)
        {
            for (vertex = 0; vertex < vertices; ++vertex)
            {
                XASSERT(!(model_input->capacity < 2 || (uint32)(rec_offset + vertex * 2) > model_input->capacity - 2));
                val = (sint16)(memcpy(&wire_halfword_2, model_input->bytes + (uint32)(rec_offset + vertex * 2), sizeof(wire_halfword_2)), wire_halfword_2);
                if (maximum < val)
                    maximum = val;
            }
            rec_offset += stride;
        }
    }
    return (uint32)maximum + 1;
}

uint32 model_reset_prim_colors(uint8 *groups)
{
    uint16 wire_halfword_1;

    uint32 flag;
    uint32 type;
    sint32 count;
    sint32 idx;
    uint32 vertex;
    uint32 vertices;
    uint32 stride;
    uint8 *color;

    FUNCTION_MARKER(0x8002E28C, "SLUS_006.10");
    flag = groups[3];
    while (flag != 0)
    {
        type = groups[2];
        count = (sint16)(memcpy(&wire_halfword_1, groups, sizeof(wire_halfword_1)), wire_halfword_1);
        groups += 4;
        if (type <= 3)
        {
            vertices = type < 2 ? 1 : (type == 2 ? 3 : 4);
            stride = type < 2 ? 20 : (type == 2 ? 44 : 56);
            for (idx = 0; idx < count; ++idx)
            {
                for (vertex = 0; vertex < vertices; ++vertex)
                {
                    color = groups + 8 + vertex * 4;
                    color[0] = 128;
                    color[1] = 128;
                    color[2] = 128;
                }
                groups += stride;
            }
        }
        flag = groups[3];
    }
    return flag;
}

uint32 model_set_gouraud_tex(const MODEL_PRIMITIVE_BUFFERS *streams, const MODEL_TEXTURE_SOURCE *tex, uint32 u_offset)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3;

    const uint8 *groups;
    uint8 *packet0;
    uint8 *packet1;
    uint32 flag;
    uint32 type;
    sint32 count;
    sint32 idx;
    uint32 vertices;
    POINT_CT *gouraud_points[2];
    uint32 buffer;
    uint32 vertex;
    POINT_CT *packet_points;
    uint32 val;
    uint32 coordinates[4];

    FUNCTION_MARKER(0x80048B94, "SLUS_006.10");
    groups = streams->groups;
    flag = groups[3];
    packet1 = streams->packets[1];
    packet0 = streams->packets[0];
    if (flag == 0)
        return streams->packet_offset;
    do
    {
        type = groups[2];
        count = (sint16)(memcpy(&wire_halfword_1, groups, sizeof(wire_halfword_1)), wire_halfword_1);
        groups += 4;
        if (type <= 1)
        {
            for (idx = 0; idx < count; ++idx)
            {
                if (type == 0)
                {
                    packet0 = (uint8 *)((POLY_FT3 *)packet0 + 1);
                    packet1 = (uint8 *)((POLY_FT3 *)packet1 + 1);
                }
                else
                {
                    packet0 = (uint8 *)((POLY_FT4 *)packet0 + 1);
                    packet1 = (uint8 *)((POLY_FT4 *)packet1 + 1);
                }
                groups += 20;
            }
        }
        else if (type == 2 || type == 3)
        {
            vertices = type == 2 ? 3 : 4;
            for (idx = 0; idx < count; ++idx)
            {
                if (type == 2)
                {
                    gouraud_points[0] = ((POLY_GT3 *)packet0)->p;
                    gouraud_points[1] = ((POLY_GT3 *)packet1)->p;
                }
                else
                {
                    gouraud_points[0] = ((POLY_GT4 *)packet0)->p;
                    gouraud_points[1] = ((POLY_GT4 *)packet1)->p;
                }
                val = tex->attributes != NULL ? tex->attributes->page : (memcpy(&wire_halfword_2, tex->resource + 8, sizeof(wire_halfword_2)), wire_halfword_2);
                gouraud_points[0][1].attr = (uint16)val;
                gouraud_points[1][1].attr = (uint16)val;
                val = tex->attributes != NULL ? tex->attributes->clut : (memcpy(&wire_halfword_3, tex->resource + 10, sizeof(wire_halfword_3)), wire_halfword_3);
                gouraud_points[0][0].attr = (uint16)val;
                gouraud_points[1][0].attr = (uint16)val;
                for (buffer = 0; buffer < 2; ++buffer)
                {
                    packet_points = gouraud_points[buffer];
                    for (vertex = 0; vertex < vertices; ++vertex)
                        coordinates[vertex] = packet_points[vertex].uv.u;
                    for (vertex = 0; vertex < vertices; ++vertex)
                        packet_points[vertex].uv.u = (uint8)(coordinates[vertex] + u_offset);
                }
                if (type == 2)
                {
                    packet0 = (uint8 *)((POLY_GT3 *)packet0 + 1);
                    packet1 = (uint8 *)((POLY_GT3 *)packet1 + 1);
                }
                else
                {
                    packet0 = (uint8 *)((POLY_GT4 *)packet0 + 1);
                    packet1 = (uint8 *)((POLY_GT4 *)packet1 + 1);
                }
                groups += type == 2 ? 44 : 56;
            }
        }
        flag = groups[3];
    } while (flag != 0);
    return 0;
}

uint32 model_replace_tex_region(const MODEL_RESOURCE *model, const char *old_name, const char *new_name)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;

    const uint8 *old_tex_rec;
    const uint8 *old_tex;
    const uint8 *new_tex_rec;
    const uint8 *new_tex;
    uint32 old_u;
    uint32 old_v;
    uint32 width;
    uint32 height;
    const uint8 *groups;
    uint8 *packet0;
    uint8 *packet1;
    uint32 groups_offset;
    uint32 packet_offset0;
    uint32 packet_offset1;
    uint32 page;
    uint32 flag;
    uint32 type;
    sint32 count;
    sint32 idx;
    uint32 vertices;
    POINT_T *flat_points[2];
    POINT_CT *gouraud_points[2];
    UV *uv0;
    UV *uv1;
    UV *probe_uv;
    uint16 *page_fields[2];
    uint16 *clut_fields[2];
    uint32 rec_stride;
    uint32 probe_vertex;
    uint32 probe_u;
    uint32 probe_v;
    sint32 left;
    sint32 right;
    sint32 top;
    sint32 bottom;
    sint32 x;
    sint32 y;
    uint32 vertex;
    uint32 original_u;
    uint32 original_v;
    uint32 cur;
    uint32 replacement;
    uint32 val;
    uint32 last_v = 0;

    FUNCTION_MARKER(0x800484AC, "SLUS_006.10");
    old_tex_rec = tex_find(old_name);
    new_tex_rec = tex_find(new_name);
    old_tex = old_tex_rec;
    old_u = old_tex[14];
    width = old_tex[12];
    XASSERT(!(model->capacity < 60));
    groups_offset = ((const MODEL_HEADER *)model->bytes)->stream_offset;
    packet_offset0 = ((const MODEL_HEADER *)model->bytes)->packet_offsets[0];
    packet_offset1 = ((const MODEL_HEADER *)model->bytes)->packet_offsets[1];
    XASSERT(!(groups_offset > model->capacity || packet_offset0 - groups_offset > model->capacity - groups_offset || model->capacity - groups_offset < 4));
    groups = model->bytes + groups_offset;
    page = (memcpy(&wire_halfword_1, old_tex + 8, sizeof(wire_halfword_1)), wire_halfword_1);
    left = ((old_u >> 2) + ((page << 6) & 0x3FF));
    old_v = old_tex[15];
    height = old_tex[13];
    flag = groups[3];
    top = (old_v + ((page << 4) & 0x100) + ((page >> 2) & 0x200));
    right = left + (sint32)(width >> 2);
    if (flag == 0)
        return right;
    XASSERT(!(packet_offset0 > model->capacity || packet_offset1 - packet_offset0 > model->capacity - packet_offset0));
    packet0 = model->bytes + packet_offset0;
    XASSERT(!(packet_offset1 > model->capacity || packet_offset1 - packet_offset0 > model->capacity - packet_offset1));
    packet1 = model->bytes + packet_offset1;
    bottom = (sint16)((uint32)top + height);
    do
    {
        type = groups[2];
        count = (sint16)(memcpy(&wire_halfword_2, groups, sizeof(wire_halfword_2)), wire_halfword_2);
        groups += 4;
        if (type <= 3)
        {
            vertices = (type & 1) == 0 ? 3 : 4;
            rec_stride = type < 2 ? 20 : (type == 2 ? 44 : 56);
            probe_vertex = (type & 1) == 0 ? 1 : 0;
            for (idx = 0; idx < count; ++idx)
            {
                if (type == 0)
                {
                    flat_points[0] = ((POLY_FT3 *)packet0)->p;
                    flat_points[1] = ((POLY_FT3 *)packet1)->p;
                }
                else if (type == 1)
                {
                    flat_points[0] = ((POLY_FT4 *)packet0)->p;
                    flat_points[1] = ((POLY_FT4 *)packet1)->p;
                }
                else if (type == 2)
                {
                    gouraud_points[0] = ((POLY_GT3 *)packet0)->p;
                    gouraud_points[1] = ((POLY_GT3 *)packet1)->p;
                }
                else
                {
                    gouraud_points[0] = ((POLY_GT4 *)packet0)->p;
                    gouraud_points[1] = ((POLY_GT4 *)packet1)->p;
                }
                if (type < 2)
                {
                    page_fields[0] = &flat_points[0][1].attr;
                    page_fields[1] = &flat_points[1][1].attr;
                    clut_fields[0] = &flat_points[0][0].attr;
                    clut_fields[1] = &flat_points[1][0].attr;
                }
                else
                {
                    page_fields[0] = &gouraud_points[0][1].attr;
                    page_fields[1] = &gouraud_points[1][1].attr;
                    clut_fields[0] = &gouraud_points[0][0].attr;
                    clut_fields[1] = &gouraud_points[1][0].attr;
                }
                probe_uv = (type < 2 ? &flat_points[0][probe_vertex].uv : &gouraud_points[0][probe_vertex].uv);
                probe_u = probe_uv->u;
                page = *page_fields[0];
                probe_v = probe_uv->v;
                x = ((probe_u >> 2) | ((page << 6) & 0x3FF));
                y = (((page << 4) & 0x100) + ((page >> 2) & 0x200) + probe_v);
                if (x >= left && x < right && y >= top && y < bottom)
                {
                    uv0 = (type < 2 ? &flat_points[0][0].uv : &gouraud_points[0][0].uv);
                    uv1 = (type < 2 ? &flat_points[1][0].uv : &gouraud_points[1][0].uv);
                    cur = (type & 1) != 0 ? probe_u : uv0->u;
                    original_u = old_tex[14];
                    new_tex = new_tex_rec;
                    replacement = new_tex[14];
                    original_v = old_tex[15];
                    val = replacement + cur - original_u;
                    uv0->u = (uint8)val;
                    uv1->u = (uint8)val;
                    if (type == 3)
                        tex_uv_state.last_u = val & 255;
                    for (vertex = 0; vertex < vertices; ++vertex)
                    {
                        uv0 = (type < 2 ? &flat_points[0][vertex].uv : &gouraud_points[0][vertex].uv);
                        uv1 = (type < 2 ? &flat_points[1][vertex].uv : &gouraud_points[1][vertex].uv);
                        if (vertex != 0)
                        {
                            cur = uv0->u;
                            replacement = new_tex[14];
                            val = replacement + cur - original_u;
                            uv0->u = (uint8)val;
                            uv1->u = (uint8)val;
                            if (type == 3)
                                tex_uv_state.last_u = val & 255;
                        }
                        cur = uv0->v;
                        replacement = new_tex[15];
                        val = replacement + cur - original_v;
                        uv0->v = (uint8)val;
                        uv1->v = (uint8)val;
                        if (type == 3)
                        {
                            last_v = val & 255;
                            if (vertex != 3)
                                tex_uv_state.last_v = last_v;
                        }
                    }
                    val = (memcpy(&wire_halfword_3, new_tex + 8, sizeof(wire_halfword_3)), wire_halfword_3);
                    *page_fields[0] = val;
                    *page_fields[1] = val;
                    val = (memcpy(&wire_halfword_4, new_tex + 10, sizeof(wire_halfword_4)), wire_halfword_4);
                    if (type == 3)
                        tex_uv_state.last_v = last_v;
                    *clut_fields[0] = val;
                    *clut_fields[1] = val;
                }
                if (type == 0)
                {
                    packet0 = (uint8 *)((POLY_FT3 *)packet0 + 1);
                    packet1 = (uint8 *)((POLY_FT3 *)packet1 + 1);
                }
                else if (type == 1)
                {
                    packet0 = (uint8 *)((POLY_FT4 *)packet0 + 1);
                    packet1 = (uint8 *)((POLY_FT4 *)packet1 + 1);
                }
                else if (type == 2)
                {
                    packet0 = (uint8 *)((POLY_GT3 *)packet0 + 1);
                    packet1 = (uint8 *)((POLY_GT3 *)packet1 + 1);
                }
                else
                {
                    packet0 = (uint8 *)((POLY_GT4 *)packet0 + 1);
                    packet1 = (uint8 *)((POLY_GT4 *)packet1 + 1);
                }
                groups += rec_stride;
            }
        }
        flag = groups[3];
    } while (flag != 0);
    return flag;
}

const uint8 *model_init_flare_tex(void)
{
    const uint8 *tex_rec;
    const uint8 *tex;

    FUNCTION_MARKER(0x8001AA5C, "SLUS_006.10");
    tex_rec = tex_find(flare_tex_names[0]);
    tex = tex_rec;
    flare_tex[0] = tex;
    tex_rec = tex_find(flare_tex_names[1]);
    tex = tex_rec;
    flare_tex[1] = tex;
    hd_flare_textures(flare_tex);
    return tex;
}
