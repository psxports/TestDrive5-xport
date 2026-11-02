#include <string.h>

#include <stdlib.h>
#include <math.h>

#include "psx_gpu.h"
#include "wheels.h"
#include "game.h"

typedef char WHEEL_FT4_SIZE_CHECK[sizeof(POLY_FT4) == 40 ? 1 : -1];

typedef struct WHEEL_TEXTURE
{
    uint16 page;
    uint16 clut;
    uint8 u;
    uint8 v;
} WHEEL_TEXTURE;

static WHEEL_TEXTURE wheel_tex[8];

static POLY_FT4 wheel_packets[2][48];

typedef union WHEEL_SIDE_VERTEX
{
    SVECTOR sdk;
    uint32 words[2];
} WHEEL_SIDE_VERTEX;

typedef char WHEEL_SIDE_VERTEX_SIZE_CHECK[sizeof(WHEEL_SIDE_VERTEX) == 8 ? 1 : -1];
typedef char WHEEL_SIDE_PACKET_SIZE_CHECK[sizeof(POLY_F4) == 24 ? 1 : -1];

const char wheel_standard_tex_name[8] = "stanhub";
static const char race_tex_name[8] = "racehub";
static const uint32 light_rgbc = 0x2C808080;
static const WHEEL_SIDE_VERTEX light_n[4] = {{.words = {0x0000F000, 0}}, {.words = {0x00001000, 0}}, {.words = {0x0000F000, 0}}, {.words = {0x00001000, 0}}};

static WHEEL_SIDE_VERTEX side_vertices[6][18];
static POLY_F4 side_packets[2][6][4][8];

static uint32 draw_sides(uint32 slot, uint32 wheel);

static void draw_hd_wheel(uint32 slot, uint32 wheel, const SVECTOR *center, float steering, float spin, float radius, float width, const WHEEL_TEXTURE *tex, float spin_period)
{
    MATRIXF base = hd_model_transform(), transform, turn;
    VECTORF local_center = {(float)center->vx, (float)center->vy, (float)center->vz}, translated;
    HD_TEXTURE texture = {tex->page, tex->clut, tex->u, tex->v, 32, 32, HD_TEXTURED | HD_FILTERED};
    float c = cosf(spin) * radius, s = sinf(spin) * radius;
    uint32 face, point, segment;
    if (!hd_capture_active())
        return;
    hd_race_wheel_texture(slot, (uint32)(tex - wheel_tex) & 3, &texture);
    hd_wheel_begin(wheel, center, steering, spin, spin_period);
    IdentityMatrixf(&turn);
    RotMatrixYf(steering, &turn);
    MulMatrix0f(&base, &turn, &transform);
    RotTransf(&base, &local_center, &translated);
    transform.t[0] = translated.vx;
    transform.t[1] = translated.vy;
    transform.t[2] = translated.vz;
    for (segment = 0; segment < 8; ++segment)
    {
        HD_TEXTURE solid = {0};
        RGBC black = {.rgbc = 0x28000000};
        RGBC colors[4] = {black, black, black, black};
        VECTORF vertices[4];
        for (point = 0; point < 4; ++point)
        {
            const SVECTOR *source = &side_vertices[slot][segment * 2 + point].sdk;
            VECTORF local = {(float)source->vx, (float)source->vy, (float)source->vz};
            RotTransf(&transform, &local, &vertices[point]);
        }
        hd_solid_quad(vertices, &solid, colors, 1);
    }
    hd_wheel_hub();
    for (face = 0; face < 2; ++face)
    {
        VECTORF vertices[4], local[4];
        RGBC color = {.rgbc = light_rgbc}, colors[4];
        float side = (wheel & 1) != 0 ? width : -width;
        float determinant;
        if (face != 0)
            side = -side;
        for (point = 0; point < 4; ++point)
            local[point].vx = side;
        local[0].vy = s;
        local[0].vz = c;
        local[1].vy = (wheel & 1) != 0 ? -c : c;
        local[1].vz = (wheel & 1) != 0 ? s : -s;
        local[2].vy = (wheel & 1) != 0 ? c : -c;
        local[2].vz = (wheel & 1) != 0 ? -s : s;
        local[3].vy = -s;
        local[3].vz = -c;
        for (point = 0; point < 4; ++point)
            RotTransf(&transform, &local[point], &vertices[point]);
        determinant = vertices[0].vx * (vertices[1].vy * vertices[2].vz - vertices[1].vz * vertices[2].vy) + vertices[0].vy * (vertices[1].vz * vertices[2].vx - vertices[1].vx * vertices[2].vz) + vertices[0].vz * (vertices[1].vx * vertices[2].vy - vertices[1].vy * vertices[2].vx);
        if (face != 0 && determinant <= 0.0f)
            continue;
        if (face == 0)
        {
            if (determinant >= 0.0f)
                color.rgbc = 0x2C000000;
        }
        else
            color = wheel_packets[disp_state.bank][slot * 8 + wheel * 2 + 1].rgbc;
        for (point = 0; point < 4; ++point)
            colors[point] = color;
        if (face == 0 && determinant < 0.0f)
            hd_lit_wheel_quad(vertices, &texture, colors, &light_n[wheel].sdk);
        else
            hd_solid_quad(vertices, &texture, colors, 1);
    }
    hd_wheel_end();
}

static void draw_hd_race_wheel(uint32 slot, uint32 wheel)
{
    const CAR_STATE *car;
    const WHEEL_TEXTURE *texture;
    sint32 speed, spin;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 6 && wheel < 4);
    car = &car_states[slot];
    speed = (sint32)(wheel < 2 ? car->forward_speed : car->steered_speed) >> 8;
    if (speed < 0)
        speed = -speed;
    speed /= 64;
    texture = wheel_tex + (speed < 4 ? speed + 4 : 7);
    if (slot != 0)
        texture -= 4;
    spin = -4 * (sint16)car->wheel_rotation[wheel < 2 ? 1 : 0];
    draw_hd_wheel(slot, wheel, &car->suspension_local[(wheel + 2) & 3], wheel >= 2 ? (sint32)car->steering_angle / 256.0f * RAD12 : 0.0f, spin * RAD12, (float)(sint16)car_geometry_metrics[slot].wheel_radius, (float)(sint16)car_geometry_metrics[slot].wheel_width, texture, 262144.0f * RAD12);
}

void wheels_draw_hd_race(uint32 slot)
{
    uint32 wheel;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 6);
    for (wheel = 0; wheel < 4; ++wheel)
        draw_hd_race_wheel(slot, wheel);
}

uint32 wheels_init_race(void)
{
    const uint8 *standard;
    const uint8 *race;
    const uint8 *tex;
    WHEEL_TEXTURE *rec;
    uint32 idx;
    uint32 buffer;
    POLY_FT4 *packet;
    uint32 val;

    FUNCTION_MARKER(0x8003320C, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(wheel_packets, (uint32)sizeof(wheel_packets)));
        XASSERT(operation_ok);
    }
    standard = tex_find(wheel_standard_tex_name);
    race = tex_find(race_tex_name);
    for (idx = 0; idx < 8; ++idx)
    {
        tex = idx < 4 ? standard : race;
        rec = wheel_tex + idx;
        val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        rec->page = (uint16)(val);
        val = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        rec->clut = (uint16)(val);
        val = tex[14];
        rec->u = (uint8)(val + ((idx & 1) != 0 ? 32 : 0));
        val = tex[15];
        rec->v = (uint8)(val + ((idx & 2) != 0 ? 32 : 0));
    }
    for (idx = 0; idx < 48; ++idx)
    {
        for (buffer = 0; buffer < 2; ++buffer)
        {
            packet = &wheel_packets[buffer][idx];
            packet->tag = (packet->tag & 0xFFFFFF) | ((uint32)(uint8)(9) << 24);
            packet->rgbc.code = (uint8)(0x2C);
            packet->rgbc.r = (uint8)(32);
            packet->rgbc.g = (uint8)(32);
            packet->rgbc.b = (uint8)(32);
        }
    }
    return 0;
}

uint32 wheels_init_preview(void)
{
    uint32 idx, val, bank;
    const uint8 *tex;
    POLY_FT4 *packet;
    WHEEL_TEXTURE *dst;

    FUNCTION_MARKER(0x800B5800, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(wheel_packets, (uint32)sizeof(wheel_packets)));
        XASSERT(operation_ok);
    }
    tex = tex_find(wheel_standard_tex_name);
    for (idx = 0; idx < 4; ++idx)
    {
        dst = wheel_tex + idx;
        dst->page = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
        dst->clut = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->clut);
        val = tex[14];
        if (idx & 1)
            val += 32;
        dst->u = (uint8)(val);
        val = tex[15];
        if (idx >= 2)
            val += 32;
        dst->v = (uint8)(val);
    }
    for (idx = 0; idx < 48; ++idx)
    {
        for (bank = 0; bank < 2; ++bank)
        {
            packet = &wheel_packets[bank][idx];
            packet->tag = (packet->tag & 0xFFFFFF) | ((uint32)(uint8)(9) << 24);
            packet->rgbc.code = (uint8)(44);
        }
    }
    return 0;
}

static void preview_set_uv(POLY_FT4 *packet)
{
    packet->p[0].uv.u = (uint8)(wheel_tex[0].u);
    packet->p[0].uv.v = (uint8)(wheel_tex[0].v);
    packet->p[1].uv.u = (uint8)(wheel_tex[0].u + 31);
    packet->p[1].uv.v = (uint8)(wheel_tex[0].v);
    packet->p[2].uv.u = (uint8)(wheel_tex[0].u);
    packet->p[2].uv.v = (uint8)(wheel_tex[0].v + 31);
    packet->p[3].uv.u = (uint8)(wheel_tex[0].u + 31);
    packet->p[3].uv.v = (uint8)(wheel_tex[0].v + 31);
    packet->p[1].attr = (uint16)(wheel_tex[0].page);
    packet->p[0].attr = (uint16)(wheel_tex[0].clut);
}

static void preview_link(POLY_FT4 *packet, uint32 bucket)
{
    DISPLAY_ORDERING head;
    DISPLAY_FRAME *cur_frame;
    uint32 tag, prev;

    cur_frame = disp_state.cur;
    head.tags = cur_frame->ordering;
    head.byte_offset = (uint32)(bucket * 4);
    tag = packet->tag;
    prev = head.tags[(sint32)head.byte_offset / 4];
    packet->tag = (uint32)((tag & 0xFF000000) | (prev & 0xFFFFFF));
    head.tags = cur_frame->ordering;
    head.byte_offset = (uint32)(bucket * 4);
    prev = head.tags[(sint32)head.byte_offset / 4];
    head.tags[(sint32)head.byte_offset / 4] = (prev & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
}

static sint32 preview_project(POLY_FT4 *packet, const SVECTOR *vertices, uint32 *area, uint32 *depth)
{
    uint32 word, flags;

    for (word = 0; word < 6; ++word)
        xport_gte_write_data(word, ((word & 1) != 0 ? (uint32)(sint32)vertices[word >> 1].vz : (uint16)vertices[word >> 1].vx | ((uint32)(uint16)vertices[word >> 1].vy << 16)));
    xport_gte_execute(0x280030);
    packet->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
    packet->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
    packet->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
    flags = xport_gte_read_control(31);
    if ((flags & 0x1C66000) != 0)
        return 0;
    xport_gte_execute(0x1400006);
    *area = xport_gte_read_data(24);
    xport_gte_write_data(0, ((uint16)vertices[3].vx | ((uint32)(uint16)vertices[3].vy << 16)));
    xport_gte_write_data(1, (uint32)(sint32)vertices[3].vz);
    xport_gte_execute(0x180001);
    packet->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
    xport_gte_execute(0x168002E);
    *depth = xport_gte_read_data(7);
    return 1;
}

uint32 wheels_draw_race(uint32 slot)
{
    DISPLAY_ORDERING bucket, ordering;
    CAR_STATE *car = &car_states[slot];
    uint32 val, product, wheel, pair, vertex, word, face;
    VEC3 translation[4];
    uint32 tag, offset;
    POLY_FT4 *packet;
    WHEEL_TEXTURE *tex;
    uint32 flags, shift, cur_buffer;
    sint32 angle_a, angle_b, scale, width, radii[4], angle, speed;
    sint32 area, depth, limit, depth_idx;
    VEC3_16 points[4];
    MATRIX turn = {0};

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80033610, "SLUS_006.10");
    angle_a = -4 * (sint16)car->wheel_rotation[1];
    angle_b = -4 * (sint16)car->wheel_rotation[0];

    scale = (sint16)car_geometry_metrics[slot].wheel_radius;
    width = (sint16)car_geometry_metrics[slot].wheel_width;
    product = (uint32)rcos(angle_a) * (uint32)scale;
    radii[0] = (sint32)product / 4096;
    product = (uint32)rsin(angle_a) * (uint32)scale;
    radii[2] = (sint32)product / 4096;
    product = (uint32)rcos(angle_b) * (uint32)scale;
    radii[1] = (sint32)product / 4096;
    product = (uint32)rsin(angle_b) * (uint32)scale;
    radii[3] = (sint32)product / 4096;
    for (wheel = 0; wheel < 4; ++wheel)
    {
        val = ((uint32)(uint16)car->suspension_local[wheel].vx | ((uint32)(uint16)car->suspension_local[wheel].vy << 16));
        xport_gte_write_data(0, val);
        val = ((uint32)(uint16)car->suspension_local[wheel].vz | ((uint32)(uint16)car->suspension_local[wheel].pad << 16));
        xport_gte_write_data(1, val);
        xport_gte_execute(0x480012);
        translation[wheel].bits.x = xport_gte_read_data(25);
        translation[wheel].bits.y = xport_gte_read_data(26);
        translation[wheel].bits.z = xport_gte_read_data(27);
    }
    for (wheel = 0; wheel < 4; ++wheel)
    {
        pair = wheel >> 1;
        if (wheel == 2)
        {
            angle = (sint32)car->steering_angle / 256;
            turn.m[0][0] = rcos(angle);
            turn.m[0][1] = 0;
            turn.m[0][2] = rsin(angle);
            turn.m[1][0] = 0;
            turn.m[1][1] = 4096;
            turn.m[1][2] = 0;
            turn.m[2][0] = -rsin(angle);
            turn.m[2][1] = 0;
            turn.m[2][2] = rcos(angle);
            MulMatrix2(&model_render.matrix.sdk, &turn);
            SetRotMatrix(&turn);
        }
        for (word = 0; word < 3; ++word)
            xport_gte_write_control(5 + word, translation[(wheel + 2) & 3].u[word]);
        draw_sides(slot, wheel);
        for (vertex = 0; vertex < 4; ++vertex)
            points[vertex].x = (sint16)((wheel & 1) != 0 ? width : -width);
        points[0].y = (sint16)radii[pair + 2];
        points[0].z = (sint16)radii[pair];
        points[1].y = (sint16)((wheel & 1) != 0 ? -radii[pair] : radii[pair]);
        points[1].z = (sint16)((wheel & 1) != 0 ? radii[pair + 2] : -radii[pair + 2]);
        points[2].y = (sint16)((wheel & 1) != 0 ? radii[pair] : -radii[pair]);
        points[2].z = (sint16)((wheel & 1) != 0 ? -radii[pair + 2] : radii[pair + 2]);
        points[3].y = (sint16)-radii[pair + 2];
        points[3].z = (sint16)-radii[pair];
        cur_buffer = disp_state.bank;
        packet = &wheel_packets[cur_buffer][wheel * 2 + slot * 8];
        speed = (sint32)(pair == 0 ? car->forward_speed : car->steered_speed) >> 8;
        if (speed < 0)
            speed = -speed;
        speed /= 64;
        tex = speed < 4 ? wheel_tex + (uint32)(speed + 4) : wheel_tex + 7;
        if (slot != 0)
            tex -= 4;
        draw_hd_race_wheel(slot, wheel);
        for (face = 0; face < 2; ++face)
        {
            if (face != 0)
            {
                cur_buffer = disp_state.bank;
                for (vertex = 0; vertex < 4; ++vertex)
                    points[vertex].x = (sint16)-points[vertex].x;
                packet = &wheel_packets[cur_buffer][wheel * 2 + slot * 8 + 1];
            }
            val = tex->u;
            packet->p[0].uv.u = (uint8)(val);
            val = tex->v;
            packet->p[0].uv.v = (uint8)(val);
            val = tex->u;
            packet->p[1].uv.u = (uint8)(val + 31);
            val = tex->v;
            packet->p[1].uv.v = (uint8)(val);
            val = tex->u;
            packet->p[2].uv.u = (uint8)(val);
            val = tex->v;
            packet->p[2].uv.v = (uint8)(val + 31);
            val = tex->u;
            packet->p[3].uv.u = (uint8)(val + 31);
            val = tex->v;
            packet->p[3].uv.v = (uint8)(val + 31);
            val = tex->page;
            packet->p[1].attr = (uint16)(val);
            val = tex->clut;
            packet->p[0].attr = (uint16)(val);
            for (vertex = 0; vertex < 3; ++vertex)
            {
                val = (uint16)points[vertex].x | ((uint32)(uint16)points[vertex].y << 16);
                xport_gte_write_data(vertex * 2, val);
                xport_gte_write_data(vertex * 2 + 1, (uint32)(sint32)points[vertex].z);
            }
            xport_gte_execute(0x280030);
            packet->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
            packet->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
            packet->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
            flags = xport_gte_read_control(31);
            if ((flags & 0x1C66000) != 0)
                continue;
            xport_gte_execute(0x1400006);
            area = (sint32)xport_gte_read_data(24);
            val = (uint16)points[3].x | ((uint32)(uint16)points[3].y << 16);
            xport_gte_write_data(0, val);
            xport_gte_write_data(1, (uint32)(sint32)points[3].z);
            xport_gte_execute(0x180001);
            packet->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
            xport_gte_execute(0x168002E);
            depth = (sint32)xport_gte_read_data(7);
            if (face != 0 && area <= 0)
                continue;
            if (face == 0 && area < 0)
            {
                shift = model_render.ot_shift;
                shift = (shift - model_render.coord_shift) & 31;
                depth_idx = (sint32)((uint32)depth - 8) >> shift;
                if (depth_idx < 0)
                    depth_idx = 0;
            }
            else
            {
                limit = (sint32)(disp_state.ordering_count - 1);
                shift = model_render.ot_shift;
                shift = (shift - model_render.coord_shift) & 31;
                depth_idx = (sint32)((uint32)depth + 8) >> shift;
                if (depth_idx >= limit)
                    depth_idx = limit;
            }
            cur_frame = disp_state.cur;
            ordering.tags = cur_frame->ordering;
            ordering.byte_offset = 0;
            tag = packet->tag;
            offset = (uint32)depth_idx << 2;
            val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
            packet->tag = (uint32)((tag & 0xFF000000) | (val & 0xFFFFFF));
            ordering.tags = cur_frame->ordering;
            ordering.byte_offset = 0;
            bucket = ordering;
            bucket.byte_offset += offset;
            val = bucket.tags[(sint32)bucket.byte_offset / 4];
            bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
            if (face == 0)
            {
                if (area < 0)
                {
                    val = light_rgbc;
                    xport_gte_write_data(6, val);
                    val = light_n[wheel].words[0];
                    xport_gte_write_data(0, val);
                    val = light_n[wheel].words[1];
                    xport_gte_write_data(1, val);
                    xport_gte_execute(0xE80413);
                    packet->rgbc.rgbc = (uint32)(xport_gte_read_data(22));
                }
                else
                    packet->rgbc.rgbc = (uint32)(0x2C000000);
            }
        }
    }
    return 0;
}

uint32 wheels_draw_preview(void)
{
    uint16 wire_halfword_1, wire_halfword_2;

    uint32 wire_word_2, wire_word_3;
    const CAR_SELECTION_BUFFER *asset;
    const uint8 *model;
    const uint8 *points;
    uint32 model_offset, angle, product, idx, wheel, half;
    uint32 radial[4];
    VEC3 t[4];
    VEC3 *translation;
    uint32 *radial_coord;
    uint32 proj_area[2], proj_depth[2];
    SVECTOR vertices[4] = {0};
    MATRIX turn = {0};
    uint32 val, x, z, coord, shift, depth, bucket, x0;
    const WHEEL_SIDE_VERTEX *vertex;
    POLY_FT4 *packet;
    sint32 radius, side, area, limit;

    FUNCTION_MARKER(0x800B5970, "SLUS_006.10");
    asset = &car_selection.active_buffer;
    model = asset->bytes;
    angle = car_selection.wheel_angles[0];
    XASSERT(!(asset->capacity < 0x58));
    memcpy(&model_offset, model + 0x54, sizeof(model_offset));
    XASSERT(!(model_offset > asset->capacity || asset->capacity - model_offset < 134));
    model = asset->bytes + model_offset;
    radius = (sint16)(memcpy(&wire_halfword_1, model + 0x82, sizeof(wire_halfword_1)), wire_halfword_1);
    side = (sint16)(memcpy(&wire_halfword_2, model + 0x84, sizeof(wire_halfword_2)), wire_halfword_2);
    points = model + 0x40;
    product = (uint32)rcos((sint32)(angle << 2)) * (uint32)radius;
    angle = car_selection.wheel_angles[0];
    radial[0] = (((sint32)product / 4096));
    product = (uint32)rsin((sint32)(angle << 2)) * (uint32)radius;
    angle = car_selection.wheel_angles[1];
    radial[2] = (((sint32)product / 4096));
    product = (uint32)rcos((sint32)(angle << 2)) * (uint32)radius;
    angle = car_selection.wheel_angles[1];
    radial[1] = (((sint32)product / 4096));
    product = (uint32)rsin((sint32)(angle << 2)) * (uint32)radius;
    angle = car_selection.wheel_angles[0];
    val = car_selection.wheel_angles[1];
    radial[3] = (((sint32)product / 4096));
    car_selection.wheel_angles[0] = (uint32)(angle - 32);
    car_selection.wheel_angles[1] = (uint32)(val - 32);
    for (idx = 0; idx < 4; ++idx)
    {
        xport_gte_write_data(0, (memcpy(&wire_word_2, points + idx * 8, sizeof(wire_word_2)), wire_word_2));
        xport_gte_write_data(1, (memcpy(&wire_word_3, points + idx * 8 + 4, sizeof(wire_word_3)), wire_word_3));
        xport_gte_execute(0x480012);
        t[idx].bits.x = (uint32)(xport_gte_read_data(25));
        t[idx].bits.y = (uint32)(xport_gte_read_data(26));
        t[idx].bits.z = (uint32)(xport_gte_read_data(27));
    }
    for (wheel = 0; wheel < 4; ++wheel)
    {
        half = wheel >> 1;
        if (wheel == 2)
        {
            turn.m[0][0] = (sint16)(uint16)(rcos(0));
            turn.m[0][1] = (sint16)(uint16)(0);
            turn.m[0][2] = (sint16)(uint16)(rsin(0));
            turn.m[1][0] = (sint16)(uint16)(0);
            turn.m[1][1] = (sint16)(uint16)(4096);
            turn.m[1][2] = (sint16)(uint16)(0);
            turn.m[2][0] = (sint16)(uint16)(0 - (uint32)rsin(0));
            turn.m[2][1] = (sint16)(uint16)(0);
            turn.m[2][2] = (sint16)(uint16)(rcos(0));
            MulMatrix2(&model_render.matrix.sdk, &turn);
            SetRotMatrix(&turn);
        }
        translation = &t[(wheel + 2) & 3];
        xport_gte_write_control(5, translation->bits.x);
        xport_gte_write_control(6, translation->bits.y);
        xport_gte_write_control(7, translation->bits.z);
        draw_sides(0, wheel);
        radial_coord = radial + half;
        val = (wheel & 1) != 0 ? (uint32)side : 0 - (uint32)side;
        vertices[0].vx = (sint16)(uint16)(val);
        z = radial_coord[2];
        vertices[0].vy = (sint16)(uint16)(z);
        x = radial_coord[0];
        vertices[1].vx = (sint16)(uint16)(val);
        vertices[0].vz = (sint16)(uint16)(x);
        x = radial_coord[0];
        vertices[1].vy = (sint16)(uint16)((wheel & 1) != 0 ? 0 - x : x);
        z = radial_coord[2];
        vertices[2].vx = (sint16)(uint16)(val);
        vertices[1].vz = (sint16)(uint16)((wheel & 1) != 0 ? z : 0 - z);
        x = radial_coord[0];
        vertices[2].vy = (sint16)(uint16)((wheel & 1) != 0 ? x : 0 - x);
        z = radial_coord[2];
        vertices[3].vx = (sint16)(uint16)(val);
        vertices[2].vz = (sint16)(uint16)((wheel & 1) != 0 ? 0 - z : z);
        z = radial_coord[2];
        vertices[3].vy = (sint16)(uint16)(0 - z);
        x = radial_coord[0];
        vertices[3].vz = (sint16)(uint16)(0 - x);
        packet = &wheel_packets[disp_state.bank][wheel * 2];
        preview_set_uv(packet);
        if (hd_capture_active())
        {
            SVECTOR center;
            memcpy(&center, points + ((wheel + 2) & 3) * sizeof(center), sizeof(center));
            draw_hd_wheel(0, wheel, &center, 0.0f, (((car_selection.wheel_angles[half] + 32) << 2) & 4095) * RAD12, (float)radius, (float)side, &wheel_tex[0], 4096.0f * RAD12);
        }
        if (preview_project(packet, vertices, &proj_area[0], &proj_depth[0]) != 0)
        {
            area = proj_area[0];
            depth = proj_depth[0];
            if (area < 0)
            {
                shift = model_render.ot_shift;
                val = model_render.coord_shift;
                bucket = ((sint32)(depth - 8) >> ((shift - val) & 31));
                if ((sint32)bucket < 0)
                    bucket = 0;
                preview_link(packet, bucket);
                xport_gte_write_data(6, light_rgbc);
                vertex = &light_n[wheel];
                xport_gte_write_data(0, vertex->words[0]);
                xport_gte_write_data(1, vertex->words[1]);
                xport_gte_execute(0xE80413);
                packet->rgbc.rgbc = (uint32)(xport_gte_read_data(22));
            }
            else
            {
                limit = (sint32)(disp_state.ordering_count - 1);
                shift = model_render.ot_shift;
                val = model_render.coord_shift;
                bucket = ((sint32)(depth + 8) >> ((shift - val) & 31));
                if ((sint32)bucket >= limit)
                    bucket = limit;
                preview_link(packet, bucket);
                packet->rgbc.rgbc = (uint32)(0x2C000000);
            }
        }
        val = disp_state.bank;
        packet = &wheel_packets[val][wheel * 2 + 1];
        x = (uint16)vertices[1].vx;
        z = (uint16)vertices[2].vx;
        coord = (uint16)vertices[3].vx;
        x0 = (uint16)vertices[0].vx;
        vertices[1].vx = (sint16)(uint16)(0 - x);
        vertices[2].vx = (sint16)(uint16)(0 - z);
        vertices[3].vx = (sint16)(uint16)(0 - coord);
        vertices[0].vx = (sint16)(uint16)(0 - x0);
        preview_set_uv(packet);
        if (preview_project(packet, vertices, &proj_area[1], &proj_depth[1]) != 0 && (sint32)proj_area[1] > 0)
        {
            depth = proj_depth[1];
            limit = (sint32)(disp_state.ordering_count - 1);
            shift = model_render.ot_shift;
            val = model_render.coord_shift;
            bucket = ((sint32)(depth + 8) >> ((shift - val) & 31));
            if ((sint32)bucket >= limit)
                bucket = limit;
            preview_link(packet, bucket);
        }
    }
    return 0;
}

uint32 wheels_init_sides(uint32 slot, sint32 wheel_radius, sint32 wheel_width)
{
    uint32 buffer;
    uint32 grp;
    uint32 idx;
    POLY_F4 *packet;
    WHEEL_SIDE_VERTEX *vertex;
    uint32 angle;
    sint32 radius;
    sint32 width;
    sint32 trig;
    uint32 product;

    FUNCTION_MARKER(0x80033030, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(side_packets, (uint32)sizeof(side_packets)));
        XASSERT(operation_ok);
    }
    for (buffer = 0; buffer < 2; ++buffer)
    {
        for (grp = 0; grp < 4; ++grp)
        {
            for (idx = 0; idx < 8; ++idx)
            {
                packet = &side_packets[buffer][slot][grp][idx];
                packet->tag = (packet->tag & 0xFFFFFF) | ((uint32)(uint8)(5) << 24);
                packet->rgbc.code = (uint8)(0x28);
                packet->rgbc.r = (uint8)(0);
                packet->rgbc.g = (uint8)(0);
                packet->rgbc.b = (uint8)(0);
            }
        }
    }
    radius = wheel_radius * 195 / 256;
    width = wheel_width + 4;
    vertex = side_vertices[slot];
    for (idx = 0; idx < 18; idx += 2)
    {
        vertex[0].sdk.vx = (sint16)(uint16)((uint32)-width);
        angle = idx << 8;
        trig = rsin((sint32)angle);
        product = (uint32)trig * (uint32)radius;
        vertex[0].sdk.vy = (sint16)(uint16)((uint32)((sint32)product / 4096));
        trig = rcos((sint32)angle);
        product = (uint32)trig * (uint32)radius;
        vertex[0].sdk.vz = (sint16)(uint16)((uint32)((sint32)product / 4096));
        vertex[1].sdk.vx = (sint16)(uint16)((uint32)width);
        trig = rsin((sint32)angle);
        product = (uint32)trig * (uint32)radius;
        vertex[1].sdk.vy = (sint16)(uint16)((uint32)((sint32)product / 4096));
        trig = rcos((sint32)angle);
        product = (uint32)trig * (uint32)radius;
        vertex[1].sdk.vz = (sint16)(uint16)((uint32)((sint32)product / 4096));
        vertex += 2;
    }
    return 0;
}

static uint32 draw_sides(uint32 slot, uint32 wheel)
{
    DISPLAY_ORDERING bucket, ordering;
    WHEEL_SIDE_VERTEX *src = side_vertices[slot];
    uint32 shift, other_shift, cur_buffer;
    POLY_F4 *packet;
    uint32 idx, word, val, tag;
    sint32 far_plane, area0, area1, depth;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80033454, "SLUS_006.10");
    shift = model_render.ot_shift;
    other_shift = model_render.coord_shift;
    far_plane = (sint32)model_render.far_depth;
    cur_buffer = disp_state.bank;
    shift -= other_shift;
    far_plane >>= (shift + 2) & 31;
    packet = side_packets[cur_buffer][slot][wheel];
    cur_frame = disp_state.cur;
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = (uint32)(128);
    shift &= 31;
    for (idx = 0; idx < 8; ++idx)
    {
        for (word = 0; word < 6; ++word)
        {
            val = src[word >> 1].words[word & 1];
            xport_gte_write_data(word, val);
        }
        xport_gte_execute(0x280030);
        xport_gte_execute(0x1400006);
        area0 = (sint32)xport_gte_read_data(24);
        packet->p[0].xy = (uint32)(xport_gte_read_data(12));
        packet->p[1].xy = (uint32)(xport_gte_read_data(13));
        packet->p[2].xy = (uint32)(xport_gte_read_data(14));
        val = src[3].words[0];
        xport_gte_write_data(0, val);
        val = src[3].words[1];
        xport_gte_write_data(1, val);
        xport_gte_execute(0x180001);
        xport_gte_execute(0x1400006);
        area1 = (sint32)xport_gte_read_data(24);
        xport_gte_execute(0x168002E);
        if (area0 <= 0 || area1 >= 0)
        {
            depth = (sint32)xport_gte_read_data(7);
            if (depth < far_plane && depth > 0)
            {
                packet->p[3].xy = (uint32)(xport_gte_read_data(14));
                tag = packet->tag;
                bucket = ordering;
                bucket.byte_offset += ((uint32)(depth >> shift) << 2);
                val = bucket.tags[(sint32)bucket.byte_offset / 4];
                packet->tag = (uint32)((tag & 0xFF000000) | (val & 0xFFFFFF));
                val = bucket.tags[(sint32)bucket.byte_offset / 4];
                bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
            }
        }
        ++packet;
        src += 2;
    }
    return 0;
}
