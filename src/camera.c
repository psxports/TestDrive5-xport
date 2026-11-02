#include <string.h>

#include <stdlib.h>

#include "game.h"

typedef char CAMERA_ATTACH_OFFSET_SIZE_CHECK[sizeof(CAMERA_ATTACH_OFFSET) == 8 ? 1 : -1];

CAMERA_CUE *camera_cues;
uint32 camera_cue_count;

typedef struct CAMERA_SPLINE_SAMPLE
{
    sint16 node_offset;
    sint16 vertex_offset;
} CAMERA_SPLINE_SAMPLE;

static const CAMERA_SPLINE_SAMPLE spline_samples[5][4] = {{{-1, 0}, {0, 0}, {40, 0}, {41, 0}}, {{-1, 0}, {0, 0}, {40, 4}, {41, 4}}, {{-1, 0}, {0, 0}, {40, -4}, {41, -4}}, {{-1, 0}, {0, 0}, {10, 4}, {11, 4}}, {{-1, 0}, {0, 0}, {10, 4}, {11, 4}}};

typedef struct CAMERA_PRESET
{
    sint16 attached;
    sint16 height;
    sint16 distance;
    sint16 aim_height;
    CAMERA_ATTACH_OFFSET offset;
} CAMERA_PRESET;

static const CAMERA_PRESET camera_presets[14] = {{0, 600, 2100, 510, {.words = {0u, 0u}}}, {0, 550, 1710, 110, {.words = {0u, 0u}}}, {0, 475, 1500, 310, {.words = {0u, 0u}}}, {0, 400, 1350, 110, {.words = {0u, 0u}}}, {0, 325, 1200, 240, {.words = {0u, 0u}}}, {0, 240, 1550, 110, {.words = {0u, 0u}}}, {1, 0, 0, 0, {.words = {4281860096u, 0u}}}, {0, 0, 0, 0, {.words = {0u, 0u}}}, {0, 0, 0, 0, {.words = {0u, 0u}}}, {0, 0, 0, 0, {.words = {0u, 0u}}}, {0, 400, 1600, 310, {.words = {0u, 0u}}}, {0, 2000, 1400, 110, {.words = {0u, 0u}}}, {0, 300, 1600, 310, {.words = {0u, 0u}}}, {0, 550, 3800, 110, {.words = {0u, 0u}}}};

static const uint32 spline_basis[16] = {4294967295u, 3u, 4294967293u, 1u, 2u, 4294967291u, 4u, 4294967295u, 4294967295u, 0u, 1u, 0u, 0u, 2u, 0u, 0u};

CAMERA_VIEW camera_views[2] = {{.prev_mode = 0u, .follow_distance = 0u, .lateral_offset = 0u, .offset_aux = 0u, .target_height = 0u, .target_distance = 0u, .target_aim_height = 0u, .aim_height = 0u, .reverse_heading = 0u, .follow_height = 0u, .look_back_angle = 0u, .attached = 0u, .damping = 0u, .yaw = 0u, .sel_mode = 0u, .roll = 2048u, .cue_idx = 0u, .trackside_x = 0u, .trackside_height = 0u, .trackside_z = 0u, .cue_mode = 0u, .spline_phase = 0u, .spline_step = 0u, .spline_variant = 0u, .target_distance_measured = 0u, .target_scale = 0u, .proj_scale = 0u, .zoom_gain = 4096u},
                               {.prev_mode = 0u, .follow_distance = 0u, .lateral_offset = 0u, .offset_aux = 0u, .target_height = 0u, .target_distance = 0u, .target_aim_height = 0u, .aim_height = 0u, .reverse_heading = 0u, .follow_height = 0u, .look_back_angle = 0u, .attached = 0u, .damping = 0u, .yaw = 0u, .sel_mode = 0u, .roll = 2048u, .cue_idx = 0u, .trackside_x = 0u, .trackside_height = 0u, .trackside_z = 0u, .cue_mode = 0u, .spline_phase = 0u, .spline_step = 0u, .spline_variant = 0u, .target_distance_measured = 0u, .target_scale = 0u, .proj_scale = 0u, .zoom_gain = 0u}};

static uint32 build_spline_coefficients(uint32 coefficients[15], const uint32 points[12]);

static uint32 evaluate_spline(VECTOR *pos, const uint32 coefficients[15], uint32 amount);

static uint32 follow_terrain(CAR_STATE *car, sint32 angle, uint32 view);

uint32 camera_follow(CAR_STATE *car, uint32 update, uint32 view)
{
    uint32 offset = view * 4;
    VECTOR *displacement = &camera_views[view].displacement;
    VECTOR *eye = &camera_views[view].eye;
    VECTOR target;
    uint32 val;
    uint32 other;
    uint32 product;
    uint32 base;
    uint32 heading;
    uint32 phase;
    sint32 counter;
    sint32 angle;
    sint32 c;
    sint32 s;
    sint32 scaled;
    sint32 error;
    sint32 speed;
    sint32 limit;

    FUNCTION_MARKER(0x8001E654, "SLUS_006.10");
    if (car->motion_mode != 0)
    {
        counter = (sint32)camera_views[offset / 4].damping;
        if (counter >= 6)
            camera_views[offset / 4].damping = (uint32)counter - 1;
    }
    else
    {
        counter = (sint32)camera_views[offset / 4].damping;
        if (counter < (sint32)race_rules.camera_damping)
        {
            camera_views[offset / 4].damping = (uint32)counter + 1;
            counter = (sint32)camera_views[offset / 4].damping;
        }
        if ((sint32)race_rules.camera_damping < counter)
            camera_views[offset / 4].damping = (uint32)counter - 1;
    }
    camera_views[offset / 4].reverse_heading = 0;
    phase = camera_views[offset / 4].yaw;
    base = camera_views[offset / 4].look_back_angle;
    angle = (base + (uint32)((sint32)(0 - phase) >> 8));
    c = rcos(angle);
    s = rsin(angle);
    scaled = (sint32)camera_views[offset / 4].follow_distance / 16;
    product = (uint32)scaled * (uint32)s;
    val = camera_views[offset / 4].follow_height;
    displacement->vx = (sint32)((uint32)((sint32)product / 256));
    displacement->vy = (sint32)(val);
    val = camera_views[offset / 4].follow_distance;
    scaled = (sint32)(0 - val) / 16;
    product = (uint32)scaled * (uint32)c;
    displacement->vz = (sint32)((uint32)((sint32)product / 256));
    val = camera_views[offset / 4].lateral_offset;
    product = (uint32)((sint32)val >> 8) * (uint32)c;
    val = car->pos.bits.x;
    other = (uint32)displacement->vx;
    eye->vx = (sint32)(val + other + (uint32)((sint32)product / 256));
    val = car->pos.bits.y;
    other = (uint32)displacement->vy;
    eye->vy = (sint32)(val + other);
    val = camera_views[offset / 4].lateral_offset;
    product = (uint32)((sint32)val >> 8) * (uint32)s;
    val = car->pos.bits.z;
    other = (uint32)displacement->vz;
    eye->vz = (sint32)(val + other + (uint32)((sint32)product / 256));
    val = camera_views[offset / 4].reverse_heading;
    phase = camera_views[offset / 4].yaw;
    angle = ((val << 11) + (uint32)((sint32)phase >> 8));
    follow_terrain(car, angle, view);
    if (update != 0)
    {
        val = camera_views[offset / 4].reverse_heading;
        heading = (uint32)car->angles.vy;
        phase = camera_views[offset / 4].yaw;
        val = (((val << 11) + heading) << 8) - phase;
        val &= 0xFFFFF;
        if (val > 0x7FFFF)
            val -= 0x100000;
        error = val;
        counter = (sint32)camera_views[offset / 4].damping;
        speed = error >> 11;
        if (speed < 0)
            speed = -speed;
        limit = speed + 5;
        if (limit < 15)
            limit = 15;
        if (counter < limit)
        {
            counter = (sint32)camera_views[offset / 4].damping;
            product = val * (uint32)counter;
        }
        else
            product = val * (uint32)limit;
        other = disp_state.time_step;
        product = (uint32)((sint32)product >> 8) * other;
        val = camera_views[offset / 4].yaw;
        camera_views[offset / 4].yaw = val + (uint32)((sint32)product / 256);
    }
    val = car->pos.bits.x;
    target.vx = (sint32)(val);
    val = car->pos.bits.y;
    other = camera_views[offset / 4].aim_height;
    target.vy = (sint32)(val + other);
    val = car->pos.bits.z;
    target.vz = (sint32)(val);
    model_set_view_pos(eye);
    angle = (sint32)camera_views[offset / 4].roll;
    model_aim_view(&target, angle);
    val = (sint16)car->track_location.node;
    camera_views[offset / 4].track_node = val;
    return val;
}

static uint32 attach_car(CAR_STATE *car, uint32 view)
{
    SVECTOR angles;
    VECTOR *camera = &camera_views[view].eye;
    uint32 val, coord;

    FUNCTION_MARKER(0x8001F700, "SLUS_006.10");
    val = (uint16)car->angles.vx;
    angles.vx = (sint16)(val + 0x800);
    val = (uint16)car->angles.vy;
    angles.vy = (sint16)(0x800 - val);
    val = (uint16)car->angles.vz;
    angles.vz = (sint16)(0 - val);
    model_set_view_angles(&angles);
    ApplyMatrix(&car->transform, &camera_views[view].attach_offset.sdk, camera);
    val = (uint32)camera->vx;
    coord = car->pos.bits.x;
    camera->vx = (sint32)((val << 8) + coord);
    val = (uint32)camera->vy;
    coord = car->pos.bits.y;
    camera->vy = (sint32)((val << 8) + coord);
    val = (uint32)camera->vz;
    coord = car->pos.bits.z;
    camera->vz = (sint32)((val << 8) + coord);
    model_set_view_pos(camera);
    model_set_view_angles(&angles);
    val = (sint16)car->track_location.node;
    camera_views[view].track_node = val;
    return val;
}

static uint32 trackside_target_height(CAR_STATE *car, uint32 view)
{
    uint16 wire_halfword_1, wire_halfword_2;

    VECTOR target;
    uint32 offset = view * 4;
    VECTOR *camera = &camera_views[view].eye;
    const uint8 *rec, *pool, *vertices;
    uint32 rec_idx, idx, base_vertex, val, other, height;
    sint32 roll;

    FUNCTION_MARKER(0x8001F5C4, "SLUS_006.10");
    rec_idx = camera_views[offset / 4].track_node;
    pool = track_geometry.node_bytes;
    idx = camera_views[offset / 4].trackside_vertex;
    XASSERT(!(rec_idx >= track_geometry.node_count));
    rec = pool + rec_idx * 24;
    base_vertex = (memcpy(&wire_halfword_1, rec + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    XASSERT(!(base_vertex >= track_geometry.vertex_count || idx >= track_geometry.vertex_count - base_vertex));
    val = camera_views[offset / 4].trackside_x;
    vertices = track_geometry.vertex_bytes;
    camera->vx = (sint32)(val);
    val = (sint16)(memcpy(&wire_halfword_2, vertices + base_vertex * 6 + idx * 6 + 2, sizeof(wire_halfword_2)), wire_halfword_2);
    memcpy(&other, rec + 16, sizeof(other));
    height = camera_views[offset / 4].trackside_height;
    camera->vy = (sint32)((other + val + height) << 8);
    val = camera_views[offset / 4].trackside_z;
    camera->vz = (sint32)(val);
    val = car->pos.bits.x;
    target.vx = (sint32)(val);
    val = car->pos.bits.y;
    height = camera_views[offset / 4].aim_height;
    target.vy = (sint32)(val + height);
    val = car->pos.bits.z;
    target.vz = (sint32)(val);
    model_set_view_pos(camera);
    roll = (sint32)camera_views[offset / 4].roll;
    return model_aim_view(&target, roll);
}

static uint32 spline(CAR_STATE *car, uint32 view, uint32 offset)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6;

    uint32 samples[12];
    VECTOR target;
    uint32 stride = view * 4;
    VECTOR *camera = &camera_views[view].eye;
    uint32 i, a, val, other, step, counter;
    const uint8 *records, *vertices, *rec;
    uint32 idx, base_vertex;
    const CAMERA_SPLINE_SAMPLE *table;
    uint32 *output;
    uint32 *coefficients = camera_views[view].spline_coefficients;
    uint32 sq, distance, product;
    sint32 amount;

    FUNCTION_MARKER(0x8001F1E4, "SLUS_006.10");
    step = camera_views[stride / 4].spline_step;
    counter = camera_views[stride / 4].spline_phase;
    val = (sint32)counter < (sint32)(4095 - step) ? counter + step : 4095;
    camera_views[stride / 4].spline_phase = val;
    records = track_geometry.node_bytes;
    vertices = track_geometry.vertex_bytes;
    XASSERT(!(offset >= 5));
    table = spline_samples[offset];
    for (i = 0; i < 4; ++i)
    {
        val = (uint32)table->node_offset;
        other = camera_views[stride / 4].track_node;
        a = (uint32)table->vertex_offset;
        idx = camera_views[stride / 4].cue_vertex + a;
        XASSERT(!(other + val >= track_geometry.node_count));
        rec = records + (other + val) * 24;
        base_vertex = (memcpy(&wire_halfword_1, rec + 4, sizeof(wire_halfword_1)), wire_halfword_1);
        XASSERT(!(base_vertex + idx >= track_geometry.vertex_count));
        val = (sint16)(memcpy(&wire_halfword_2, vertices + (base_vertex + idx) * 6, sizeof(wire_halfword_2)), wire_halfword_2);
        memcpy(&other, rec + 12, sizeof(other));
        output = samples + i * 3;
        output[0] = (val + other) << 8;
        base_vertex = (memcpy(&wire_halfword_3, rec + 4, sizeof(wire_halfword_3)), wire_halfword_3);
        val = (sint16)(memcpy(&wire_halfword_4, vertices + (base_vertex + idx) * 6 + 4, sizeof(wire_halfword_4)), wire_halfword_4);
        memcpy(&other, rec + 20, sizeof(other));
        output[2] = (val + other) << 8;
        base_vertex = (memcpy(&wire_halfword_5, rec + 4, sizeof(wire_halfword_5)), wire_halfword_5);
        ++table;
        val = (sint16)(memcpy(&wire_halfword_6, vertices + (base_vertex + idx) * 6 + 2, sizeof(wire_halfword_6)), wire_halfword_6);
        memcpy(&other, rec + 16, sizeof(other));
        a = camera_views[stride / 4].trackside_height;
        output[1] = (val + other + a) << 8;
    }
    build_spline_coefficients(coefficients, samples);
    val = camera_views[stride / 4].spline_phase;
    evaluate_spline(camera, coefficients, val);
    val = car->pos.bits.x;
    target.vx = (sint32)(val);
    val = car->pos.bits.y;
    target.vy = (sint32)(val);
    val = car->pos.bits.z;
    target.vz = (sint32)(val);
    model_set_view_pos(camera);
    amount = (sint32)camera_views[stride / 4].roll;
    model_aim_view(&target, amount);
    val = (uint32)camera->vx;
    other = (uint32)target.vx;
    val = ((sint32)(val - other) >> 10);
    sq = val * val;
    val = (uint32)camera->vy;
    other = (uint32)target.vy;
    val = ((sint32)(val - other) >> 10);
    sq += val * val;
    other = (uint32)target.vz;
    val = (uint32)camera->vz;
    val = ((sint32)(val - other) >> 10);
    sq += val * val;
    distance = (uint32)SquareRoot0((sint32)sq) << 2;
    val = camera_views[stride / 4].zoom_gain;
    product = distance * val;
    camera_views[stride / 4].target_distance_measured = distance;
    amount = (sint32)product / 4096;
    camera_views[stride / 4].target_scale = (uint32)amount;
    if (amount < 4096)
        amount = 4096;
    else if (amount >= 20001)
        amount = 20000;
    camera_views[stride / 4].target_scale = (uint32)amount;
    val = camera_views[stride / 4].target_scale;
    model_render.scale = (val);
    return val;
}

static uint32 attach_view(CAR_STATE *car, uint32 view)
{
    SVECTOR angles;
    VECTOR *camera = &camera_views[view].eye;
    uint32 val, rotation, coord;

    FUNCTION_MARKER(0x8001EA78, "SLUS_006.10");
    val = (uint16)car->angles.vx;
    angles.vx = (sint16)(val + 0x800);
    val = camera_views[view].look_back_angle;
    rotation = (uint16)car->angles.vy;
    angles.vy = (sint16)(val + 0x800 - rotation);
    val = (uint16)car->angles.vz;
    angles.vz = (sint16)(0 - val);
    model_set_view_angles(&angles);
    ApplyMatrix(&model_render.view_base.sdk, &camera_views[view].attach_offset.sdk, camera);
    val = (uint32)camera->vx;
    coord = car->pos.bits.x;
    camera->vx = (sint32)((val << 8) + coord);
    val = (uint32)camera->vy;
    coord = car->pos.bits.y;
    camera->vy = (sint32)((val << 8) + coord);
    val = (uint32)camera->vz;
    coord = car->pos.bits.z;
    camera->vz = (sint32)((val << 8) + coord);
    model_set_view_pos(camera);
    model_set_view_angles(&angles);
    val = (sint16)car->track_location.node;
    camera_views[view].track_node = val;
    return val;
}

static uint32 trackside_zoom(CAR_STATE *car, uint32 view)
{
    uint16 wire_halfword_1, wire_halfword_2;

    VECTOR target;
    uint32 offset = view * 4;
    VECTOR *camera = &camera_views[view].eye;
    const uint8 *rec, *pool, *vertices;
    uint32 rec_idx, idx, base_vertex, x, val, other, sq;
    uint32 distance, coefficient, product;
    sint32 roll, amount;

    FUNCTION_MARKER(0x8001EFAC, "SLUS_006.10");
    rec_idx = camera_views[offset / 4].track_node;
    pool = track_geometry.node_bytes;
    idx = camera_views[offset / 4].trackside_vertex;
    XASSERT(!(rec_idx >= track_geometry.node_count));
    rec = pool + rec_idx * 24;
    base_vertex = (memcpy(&wire_halfword_1, rec + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    XASSERT(!(base_vertex >= track_geometry.vertex_count || idx >= track_geometry.vertex_count - base_vertex));
    x = camera_views[offset / 4].trackside_x;
    vertices = track_geometry.vertex_bytes;
    camera->vx = (sint32)(x);
    val = (sint16)(memcpy(&wire_halfword_2, vertices + (base_vertex + idx) * 6 + 2, sizeof(wire_halfword_2)), wire_halfword_2);
    memcpy(&other, rec + 16, sizeof(other));
    x = camera_views[offset / 4].trackside_height;
    camera->vy = (sint32)((other + val + x) << 8);
    val = camera_views[offset / 4].trackside_z;
    camera->vz = (sint32)(val);
    val = car->pos.bits.x;
    target.vx = (sint32)(val);
    val = car->pos.bits.y;
    target.vy = (sint32)(val);
    val = car->pos.bits.z;
    target.vz = (sint32)(val);
    model_set_view_pos(camera);
    roll = (sint32)camera_views[offset / 4].roll;
    model_aim_view(&target, roll);
    val = (uint32)camera->vx;
    other = (uint32)target.vx;
    val = ((sint32)(val - other) >> 10);
    sq = val * val;
    val = (uint32)camera->vy;
    other = (uint32)target.vy;
    val = ((sint32)(val - other) >> 10);
    sq += val * val;
    other = (uint32)target.vz;
    val = (uint32)camera->vz;
    val = ((sint32)(val - other) >> 10);
    sq += val * val;
    distance = (uint32)SquareRoot0((sint32)sq) << 2;
    coefficient = camera_views[offset / 4].zoom_gain;
    product = distance * coefficient;
    camera_views[offset / 4].target_distance_measured = distance;
    amount = (sint32)product / 4096;
    camera_views[offset / 4].target_scale = (uint32)amount;
    if (amount < 4096)
        amount = 4096;
    else if (amount >= 20001)
        amount = 20000;
    camera_views[offset / 4].target_scale = (uint32)amount;
    val = camera_views[offset / 4].target_scale;
    model_render.scale = (val);
    return val;
}

static uint32 evaluate_spline(VECTOR *pos, const uint32 coefficients[15], uint32 amount)
{
    uint32 sq, cubed, axis, cubic_term, quadratic_term, linear_term, constant, base, val;
    sint32 *components[3] = {&pos->vx, &pos->vy, &pos->vz};

    FUNCTION_MARKER(0x8007A708, "SLUS_006.10");
    sq = ((sint32)(amount * amount) >> 12);
    cubed = ((sint32)(sq * amount) >> 12);
    for (axis = 0; axis < 3; ++axis)
    {
        cubic_term = coefficients[3 + axis];
        cubic_term *= cubed;
        quadratic_term = coefficients[6 + axis];
        quadratic_term *= sq;
        linear_term = coefficients[9 + axis];
        linear_term *= amount;
        constant = coefficients[12 + axis];
        base = coefficients[axis];
        val = cubic_term + quadratic_term + linear_term;
        val = ((sint32)val >> 12);
        val = ((val + constant) << 8) + base;
        *components[axis] = val;
    }
    return val;
}

static uint32 follow_terrain(CAR_STATE *car, sint32 angle, uint32 view)
{
    VEC3 points[3];
    TRACK_LOCATION locations[3] = {0};
    SVECTOR angles;
    SVECTOR displacement;
    MODEL_MATRIX rotation;
    uint32 offset = view * 4;
    uint32 c;
    uint32 s;
    uint32 val;
    uint32 val0;
    uint32 val1;
    uint32 val2;
    uint32 product;
    uint32 rate;
    uint32 prev;
    sint32 scaled;
    sint32 dir;

    FUNCTION_MARKER(0x8001E22C, "SLUS_006.10");
    c = (uint32)rcos(angle);
    s = (uint32)rsin(angle) << 5;
    val = car->pos.bits.x;
    points[0].bits.x = val + s;
    val = car->pos.bits.z;
    c <<= 5;
    points[0].bits.z = val + c;
    val = car->pos.bits.x;
    points[1].bits.x = val - c;
    val = car->pos.bits.z;
    points[1].bits.z = val + s;
    val = car->pos.bits.x;
    points[2].bits.x = val + c;
    val = car->pos.bits.z;
    points[2].bits.z = val - s;
    val = car->track_location.node;
    locations[2].node = (uint16)val;
    locations[1].node = (uint16)val;
    locations[0].node = (uint16)val;
    val = car->track_location.lane;
    locations[2].lane = (uint8)val;
    locations[1].lane = (uint8)val;
    locations[0].lane = (uint8)val;
    points[0].bits.y = car->pos.bits.y;
    points[1].bits.y = car->pos.bits.y;
    points[2].bits.y = car->pos.bits.y;
    track_update_location(&locations[0], &points[0]);
    track_surface_height(&locations[0], &points[0], &points[0].bits.y);
    track_update_location(&locations[1], &points[1]);
    track_surface_height(&locations[1], &points[1], &points[1].bits.y);
    track_update_location(&locations[2], &points[2]);
    track_surface_height(&locations[2], &points[2], &points[2].bits.y);
    val1 = points[1].bits.y;
    val2 = points[2].bits.y;
    val0 = points[0].bits.y;
    val = val0 - (uint32)((sint32)(val1 + val2) / 2);
    dir = fixed_angle_from_vector((sint32)(0 - (uint32)((sint32)val >> 8)), 512);
    val1 = points[1].bits.y;
    val2 = points[2].bits.y;
    angles.vx = (sint16)(dir);
    angles.vy = (sint16)(angle);
    val = val1 - val2;
    dir = fixed_angle_from_vector((sint32)(0 - (uint32)((sint32)val >> 8)), 1024);
    angles.vz = (sint16)(dir);
    model_build_rotation(&rotation.sdk, &angles);
    val = camera_views[offset / 4].yaw;
    val0 = camera_views[offset / 4].look_back_angle;
    val1 = (uint32)car->angles.vy;
    dir = (val0 + (uint32)((sint32)val >> 8) - val1);
    s = (uint32)rsin(dir);
    val = camera_views[offset / 4].target_distance;
    product = (uint32)((sint32)val / 256) * s;
    displacement.vx = (sint16)((sint32)product / 4096);
    val = camera_views[offset / 4].target_height;
    displacement.vy = (sint16)((sint32)val / 256);
    val = camera_views[offset / 4].yaw;
    val0 = camera_views[offset / 4].look_back_angle;
    val1 = (uint32)car->angles.vy;
    dir = (val0 + (uint32)((sint32)val >> 8) - val1);
    c = (uint32)rcos(dir);
    val = camera_views[offset / 4].target_distance;
    product = (uint32)((sint32)(0 - val) / 256) * c;
    displacement.vz = (sint16)((sint32)product / 4096);
    val0 = rotation.words[0];
    val1 = rotation.words[1];
    xport_gte_write_control(0, val0);
    xport_gte_write_control(1, val1);
    val0 = rotation.words[2];
    val1 = rotation.words[3];
    val2 = (uint16)rotation.sdk.m[2][2];
    xport_gte_write_control(2, val0);
    xport_gte_write_control(3, val1);
    xport_gte_write_control(4, val2);
    val = (uint32)(uint16)displacement.vx | ((uint32)(uint16)displacement.vy << 16);
    xport_gte_write_data(0, val);
    val = (uint16)displacement.vz;
    xport_gte_write_data(1, val);
    xport_gte_execute(0x486012);
    val0 = xport_gte_read_data(9);
    val1 = xport_gte_read_data(10);
    val2 = xport_gte_read_data(11);
    displacement.vx = (sint16)(val0);
    displacement.vy = (sint16)(val1);
    displacement.vz = (sint16)(val2);
    val = (uint32)displacement.vy;
    prev = camera_views[offset / 4].follow_height;
    val = (val << 8) - prev;
    rate = disp_state.time_step;
    product = (uint32)((sint32)val / 8) * rate;
    camera_views[offset / 4].follow_height = prev + (uint32)((sint32)product / 256);
    val0 = (uint32)displacement.vx;
    val1 = (uint32)displacement.vz;
    val = fixed_sqrt(val0 * val0 + val1 * val1);
    prev = camera_views[offset / 4].follow_distance;
    val = (val << 8) - prev;
    rate = disp_state.time_step;
    product = (uint32)((sint32)val / 8) * rate;
    camera_views[offset / 4].follow_distance = prev + (uint32)((sint32)product / 256);
    val = camera_views[offset / 4].target_aim_height;
    prev = camera_views[offset / 4].aim_height;
    scaled = (sint32)(val - prev) / 8;
    product = (uint32)scaled * rate;
    val = prev + (uint32)((sint32)product / 256);
    camera_views[offset / 4].aim_height = val;
    return val;
}

static uint32 update_cue(CAR_STATE *car, uint32 view)
{
    uint32 offset = view * 4, mode, val, prev;

    FUNCTION_MARKER(0x8001FE94, "SLUS_006.10");
    mode = camera_views[offset / 4].cue_mode;
    switch (mode)
    {
        case 0:
            trackside_zoom(car, view);
            break;
        case 1:
        case 2:
            attach_car(car, view);
            break;
        case 3:
            trackside_target_height(car, view);
            break;
        case 4:
            val = camera_views[offset / 4].look_back_angle;
            camera_views[offset / 4].look_back_angle = val + 8;
            camera_follow(car, 1, view);
            break;
        case 5:
            val = camera_views[offset / 4].look_back_angle;
            camera_views[offset / 4].look_back_angle = val - 8;
            camera_follow(car, 1, view);
            break;
        case 6:
            val = camera_views[offset / 4].spline_variant;
            spline(car, view, val);
            break;
        case 7:
            attach_view(car, view);
            break;
        case 8:
            camera_views[offset / 4].look_back_angle = 2048;
            attach_view(car, view);
            break;
        case 9:
            camera_views[offset / 4].look_back_angle = 2048;
            camera_follow(car, 1, view);
            break;
        case 10:
            camera_views[offset / 4].look_back_angle = 0;
            camera_follow(car, 1, view);
            break;
    }
    prev = camera_views[offset / 4].proj_scale;
    val = model_render.scale;
    if (prev != val)
    {
        model_update_clip_planes();
        val = model_render.scale;
    }
    camera_views[offset / 4].proj_scale = val;
    return val;
}

static uint32 select_cue(CAR_STATE *car, uint32 view)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6;

    uint32 offset = view * 4, prev, sel, idx;
    const CAMERA_CUE *table, *cursor;
    const uint8 *rec, *vertices;
    uint32 val0, val1, point, val, other, kind, sample;
    uint32 sample_node, sample_vertex;
    sint32 count, node;

    FUNCTION_MARKER(0x8001FA7C, "SLUS_006.10");
    count = (sint32)camera_cue_count;
    node = (sint16)car->track_location.node;
    prev = camera_views[offset / 4].cue_idx;
    if (count > 0)
    {
        cursor = camera_cues;
        for (idx = 0; (sint32)idx < count; ++idx)
        {
            val0 = cursor->node_start;
            if (node >= (sint32)val0)
            {
                val1 = cursor->node_end;
                if ((sint32)val1 >= node)
                {
                    camera_views[offset / 4].cue_idx = idx;
                    break;
                }
            }
            ++cursor;
        }
    }
    sel = camera_views[offset / 4].cue_idx;
    XASSERT(!(sel >= (camera_cue_count != 0 ? camera_cue_count : 1)));
    if (prev == sel)
        return sel * 16;
    table = camera_cues;
    cursor = table + sel;
    point = cursor->vertex;
    val1 = cursor->node;
    camera_views[offset / 4].cue_vertex = point;
    camera_views[offset / 4].track_node = val1;
    kind = cursor->kind;
    if (kind == 0 || kind == 3 || kind == 6)
    {
        if (val1 >= track_geometry.node_count)
            kind = 10;
        else
        {
            rec = track_geometry.node_bytes + val1 * 24;
            val0 = (memcpy(&wire_halfword_1, rec + 4, sizeof(wire_halfword_1)), wire_halfword_1);
            if (val0 + point >= track_geometry.vertex_count)
                kind = 10;
        }
        if (kind == 6)
        {
            XASSERT(!(cursor->spline_variant >= 5));
            for (sample = 0; sample < 4; ++sample)
            {
                sample_node = val1 + (uint32)spline_samples[cursor->spline_variant][sample].node_offset;
                if (sample_node >= track_geometry.node_count)
                {
                    kind = 10;
                    break;
                }
                sample_vertex = (memcpy(&wire_halfword_2, track_geometry.node_bytes + sample_node * 24 + 4, sizeof(wire_halfword_2)), wire_halfword_2) + point + (uint32)spline_samples[cursor->spline_variant][sample].vertex_offset;
                if (sample_vertex >= track_geometry.vertex_count)
                {
                    kind = 10;
                    break;
                }
            }
        }
        if (kind != 10)
        {
            rec = track_geometry.node_bytes + val1 * 24;
            val0 = (memcpy(&wire_halfword_3, rec + 4, sizeof(wire_halfword_3)), wire_halfword_3);
            vertices = track_geometry.vertex_bytes;
            val = (sint16)(memcpy(&wire_halfword_4, vertices + (val0 + point) * 6, sizeof(wire_halfword_4)), wire_halfword_4);
            memcpy(&other, rec + 12, sizeof(other));
            camera_views[offset / 4].trackside_x = (val + other) << 8;
            val0 = (memcpy(&wire_halfword_5, rec + 4, sizeof(wire_halfword_5)), wire_halfword_5);
            val = (sint16)(memcpy(&wire_halfword_6, vertices + (val0 + point) * 6 + 4, sizeof(wire_halfword_6)), wire_halfword_6);
            memcpy(&other, rec + 20, sizeof(other));
            camera_views[offset / 4].trackside_z = (val + other) << 8;
        }
    }
    sel = camera_views[offset / 4].cue_idx;
    val = table[sel].height;
    camera_views[offset / 4].attached = 0;
    camera_views[offset / 4].trackside_height = val;
    sel = camera_views[offset / 4].cue_idx;
    camera_views[offset / 4].cue_mode = kind;
    switch (kind)
    {
        case 0:
            camera_views[offset / 4].look_back_angle = 0;
            camera_views[offset / 4].zoom_gain = 20000;
            return 20000;
        case 1:
        case 2:
            model_render.scale = (4096);
            camera_views[offset / 4].look_back_angle = 0;
            camera_views[view].attach_offset.sdk.vx = (sint16)(kind == 1 ? 512 : 0xFE00);
            camera_views[view].attach_offset.sdk.vy = (sint16)(196);
            camera_views[view].attach_offset.sdk.vz = (sint16)(0xFC00);
            return 0xFFFFFC00;
        case 3:
            model_render.scale = (4096);
            camera_views[offset / 4].look_back_angle = 0;
            return offset;
        case 4:
        case 5:
        case 9:
        case 10:
            sel = camera_views[offset / 4].cue_idx;
            table = camera_cues;
            val = table[sel].height;
            model_render.scale = (4096);
            camera_views[offset / 4].trackside_height = val;
            return 4096;
        case 6:
            camera_views[offset / 4].look_back_angle = 0;
            camera_views[offset / 4].zoom_gain = 20000;
            camera_views[offset / 4].spline_phase = 0;
            sel = camera_views[offset / 4].cue_idx;
            table = camera_cues;
            val = table[sel].spline_step;
            camera_views[offset / 4].spline_step = val;
            sel = camera_views[offset / 4].cue_idx;
            val = table[sel].spline_variant;
            camera_views[offset / 4].spline_variant = val;
            return val;
        case 7:
            model_render.scale = (4096);
            camera_views[offset / 4].look_back_angle = 0;
            camera_views[view].attach_offset.sdk.vx = (sint16)(0);
            camera_views[view].attach_offset.sdk.vy = (sint16)(0xFF38);
            camera_views[view].attach_offset.sdk.vz = (sint16)(0);
            camera_views[offset / 4].attached = 1;
            return 1;
        case 8:
            model_render.scale = (4096);
            camera_views[view].attach_offset.sdk.vy = (sint16)(0xFF38);
            camera_views[view].attach_offset.sdk.vx = (sint16)(0);
            camera_views[view].attach_offset.sdk.vz = (sint16)(0);
            camera_views[offset / 4].attached = 1;
            return 1;
        default:
            return kind << 2;
    }
}

uint32 camera_init_cues(void)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;
    uint16 wire_halfword_13;

    const SCENE_LEVEL_IMAGE *image = &scene_level_image;
    const uint8 *wire;
    CAMERA_CUE *decoded;
    uint32 cue_offset, cursor, decode_count;
    const CAMERA_CUE *records;
    uint32 sentinel;
    uint32 count;
    uint32 node;
    const uint8 *base;
    uint32 offset;
    const uint8 *track;
    uint32 vertex;
    uint32 val;
    uint32 other;
    sint32 random;

    FUNCTION_MARKER(0x8001F844, "SLUS_006.10");
    camera_views[0].cue_idx = 0;
    camera_views[1].cue_idx = 0;
    camera_views[0].proj_scale = 0xFFFFFFFF;
    camera_views[1].proj_scale = 0xFFFFFFFF;
    XASSERT(!(image->bytes == NULL || image->size < 16));
    cue_offset = image->desc.camera_offset;
    XASSERT(!(cue_offset > image->size - 2));
    cursor = cue_offset;
    count = 0;
    for (;;)
    {
        XASSERT(!(cursor > image->size - 2));
        wire = image->bytes + cursor;
        sentinel = (memcpy(&wire_halfword_1, wire, sizeof(wire_halfword_1)), wire_halfword_1);
        if (sentinel == 0xFFFF)
            break;
        XASSERT(!(cursor > image->size - 16));
        ++count;
        cursor += 16;
    }
    decode_count = count != 0 ? count : 1;
    XASSERT(!(decode_count > (image->size - cue_offset) / 16));
    decoded = (CAMERA_CUE *)malloc((size_t)decode_count * sizeof(*decoded));
    XASSERT(!(decoded == NULL));
    for (cursor = 0; cursor < decode_count; ++cursor)
    {
        wire = image->bytes + cue_offset + cursor * 16;
        decoded[cursor].kind = (memcpy(&wire_halfword_2, wire, sizeof(wire_halfword_2)), wire_halfword_2);
        decoded[cursor].node_start = (memcpy(&wire_halfword_3, wire + 2, sizeof(wire_halfword_3)), wire_halfword_3);
        decoded[cursor].node_end = (memcpy(&wire_halfword_4, wire + 4, sizeof(wire_halfword_4)), wire_halfword_4);
        decoded[cursor].node = (memcpy(&wire_halfword_5, wire + 6, sizeof(wire_halfword_5)), wire_halfword_5);
        decoded[cursor].vertex = (memcpy(&wire_halfword_6, wire + 8, sizeof(wire_halfword_6)), wire_halfword_6);
        decoded[cursor].height = (memcpy(&wire_halfword_7, wire + 10, sizeof(wire_halfword_7)), wire_halfword_7);
        decoded[cursor].spline_variant = (memcpy(&wire_halfword_8, wire + 12, sizeof(wire_halfword_8)), wire_halfword_8);
        decoded[cursor].spline_step = (memcpy(&wire_halfword_9, wire + 14, sizeof(wire_halfword_9)), wire_halfword_9);
        XASSERT(!(decoded[cursor].kind == 6 && decoded[cursor].spline_variant >= 5));
    }
    free(camera_cues);
    camera_cues = decoded;
    camera_cue_count = count;
    records = camera_cues;
    node = records->node;
    base = track_geometry.node_bytes;
    offset = records->vertex;
    XASSERT(!(node >= track_geometry.node_count));
    track = base + node * 24;
    camera_views[1].cue_vertex = offset;
    camera_views[0].cue_vertex = offset;
    camera_views[1].track_node = node;
    camera_views[0].track_node = node;
    vertex = (memcpy(&wire_halfword_10, track + 4, sizeof(wire_halfword_10)), wire_halfword_10);
    base = track_geometry.vertex_bytes;
    XASSERT(!(vertex + offset >= track_geometry.vertex_count));
    val = (sint16)(memcpy(&wire_halfword_11, base + (vertex + offset) * 6, sizeof(wire_halfword_11)), wire_halfword_11);
    memcpy(&other, track + 12, sizeof(other));
    val = (val + other) << 8;
    camera_views[1].trackside_x = val;
    camera_views[0].trackside_x = val;
    vertex = (memcpy(&wire_halfword_12, track + 4, sizeof(wire_halfword_12)), wire_halfword_12);
    XASSERT(!(vertex + offset >= track_geometry.vertex_count));
    val = (sint16)(memcpy(&wire_halfword_13, base + (vertex + offset) * 6 + 4, sizeof(wire_halfword_13)), wire_halfword_13);
    memcpy(&other, track + 20, sizeof(other));
    val = (val + other) << 8;
    camera_views[1].trackside_z = val;
    camera_views[0].trackside_z = val;
    val = records->height;
    camera_views[1].cue_mode = 0;
    camera_views[0].cue_mode = 0;
    camera_views[1].trackside_height = val;
    camera_views[0].trackside_height = val;
    random = randPSX();
    camera_views[0].zoom_gain = (uint32)(random % 10000 + 10000);
    random = randPSX();
    camera_views[0].spline_step = 8;
    camera_views[1].spline_step = 8;
    camera_views[0].spline_phase = 0;
    camera_views[1].spline_phase = 0;
    count = camera_cue_count;
    camera_views[1].zoom_gain = (uint32)(random % 10000 + 10000);
    if ((sint32)count < 2 || (sint32)race_session.track >= 24)
    {
        camera_views[0].cue_mode = 4;
        camera_views[1].cue_mode = 4;
    }
    return 4;
}

uint32 camera_reset_follow(const SVECTOR *angles, sint32 mode, uint32 view)
{
    uint32 offset = view * 4;
    uint32 *sel = &camera_views[offset / 4].sel_mode;
    uint32 entry;
    uint32 val;
    uint32 val0;
    uint32 val1;
    uint32 result;
    uint32 reset;

    FUNCTION_MARKER(0x8001E034, "SLUS_006.10");
    hd_camera_reset(view);
    entry = *sel * 16;
    val = (uint32)camera_presets[entry / 16].height << 8;
    camera_views[offset / 4].target_height = val;
    entry = *sel * 16;
    val = (uint32)camera_presets[entry / 16].distance << 8;
    camera_views[offset / 4].target_distance = val;
    entry = *sel * 16;
    val = (uint32)camera_presets[entry / 16].aim_height << 8;
    camera_views[offset / 4].target_aim_height = val;
    entry = *sel * 16;
    val0 = camera_presets[entry / 16].offset.words[0];
    val1 = camera_presets[entry / 16].offset.words[1];
    camera_views[view].attach_offset.words[0] = val0;
    camera_views[view].attach_offset.words[1] = val1;
    reset = mode != 0;
    if (!reset)
    {
        entry = *sel * 16;
        val0 = (uint32)camera_presets[entry / 16].attached;
        val1 = camera_views[offset / 4].attached;
        reset = val1 != val0;
    }
    if (reset)
    {
        val0 = camera_views[offset / 4].reverse_heading;
        val1 = (uint32)angles->vy;
        val = ((val0 << 11) + val1) << 8;
        camera_views[offset / 4].yaw = val;
        val = camera_views[offset / 4].target_height;
        camera_views[offset / 4].follow_height = val;
        val = camera_views[offset / 4].target_distance;
        camera_views[offset / 4].follow_distance = val;
        val = camera_views[offset / 4].target_aim_height;
        camera_views[offset / 4].aim_height = val;
    }
    entry = camera_views[offset / 4].sel_mode * 16;
    val0 = (uint32)camera_presets[entry / 16].attached;
    result = race_rules.camera_damping;
    camera_views[offset / 4].lateral_offset = 0;
    camera_views[offset / 4].offset_aux = 0;
    camera_views[offset / 4].damping = result;
    camera_views[offset / 4].attached = val0;
    return result;
}

uint32 camera_update(CAR_STATE *car, uint32 view)
{
    uint32 offset;
    uint32 val;
    uint32 prev;
    uint32 buttons;
    uint32 mask;
    sint32 counter;
    sint32 phase;

    FUNCTION_MARKER(0x8001EBBC, "SLUS_006.10");
    offset = view * 4;
    if (race_control.players[view].finished != 0)
        return race_update_finish_wait(view);
    if (replay_mode != 0 || session_flow.attract_active != 0)
    {
        select_cue(car, view);
        return update_cue(car, view);
    }
    counter = (sint32)hud_countdown_frames[0];
    if (counter > 0)
    {
        phase = counter / 40;
        if (phase <= 3)
        {
            camera_views[offset / 4].sel_mode = (uint32)(13 - phase);
            if (phase == 0)
            {
                val = camera_views[offset / 4].target_distance;
                camera_views[offset / 4].target_distance = val - 14080;
            }
            else
            {
                val = camera_views[offset / 4].yaw;
                camera_views[offset / 4].yaw = val + (phase == 3 ? 1024 : 4096);
            }
            prev = camera_views[offset / 4].prev_mode;
            val = camera_views[offset / 4].sel_mode;
            if (prev != val)
            {
                camera_reset_follow(&car->angles, phase == 3 ? 1 : 0, view);
                camera_views[offset / 4].look_back_angle = 0;
            }
            camera_follow(car, phase == 0 ? 1 : 0, view);
        }
        val = camera_views[offset / 4].sel_mode;
        camera_views[offset / 4].prev_mode = val;
        return val;
    }
    buttons = input_ports[view].held;
    mask = input_ports[view].config.camera_held;
    val = (buttons & mask) != 0 ? 2048 : 0;
    if (camera_views[offset / 4].look_back_angle != val)
        hd_camera_reset(view);
    camera_views[offset / 4].look_back_angle = val;
    buttons = input_ports[view].pressed;
    mask = input_ports[view].config.camera_cycle;
    if ((buttons & mask) != 0)
    {
        val = camera_views[offset / 4].sel_mode + 1;
        camera_views[offset / 4].sel_mode = (uint32)((sint32)val % 7);
        camera_reset_follow(&car->angles, 0, view);
    }
    if (camera_views[offset / 4].attached != 0)
        return attach_view(car, view);
    return camera_follow(car, 1, view);
}

static uint32 build_spline_coefficients(uint32 coefficients[15], const uint32 points[12])
{
    uint32 relative[12], point, axis, row, inner, val0, val1, sum;
    uint32 *address;
    sint32 x, y, z;

    FUNCTION_MARKER(0x8007A570, "SLUS_006.10");
    coefficients[0] = points[3];
    coefficients[1] = points[4];
    coefficients[2] = points[5];
    for (point = 0; point < 4; ++point)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            val0 = points[point * 3 + axis];
            val1 = points[3 + axis];
            relative[point * 3 + axis] = ((sint32)(val0 - val1) >> 8);
        }
    }
    for (row = 0; row < 4; ++row)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            sum = 0;
            for (inner = 0; inner < 4; ++inner)
            {
                val0 = spline_basis[row * 4 + inner];
                sum += val0 * relative[inner * 3 + axis];
            }
            coefficients[3 + row * 3 + axis] = sum;
        }
    }
    for (row = 0; row < 4; ++row)
    {
        address = coefficients + row * 3;
        x = address[3];
        y = address[4];
        address[3] = (x / 2);
        z = address[5];
        address[4] = (y / 2);
        address[5] = (z / 2);
    }
    return 0;
}
