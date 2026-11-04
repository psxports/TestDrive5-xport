#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "effects.h"
#include "psx_gpu.h"
#include "game.h"

typedef char WHEEL_SKID_SHARED_WORD_LAYOUT_CHECK[sizeof(uint32) == 4 && offsetof(WHEEL_SKID_CONTROL, car_slot) == 0 && offsetof(WHEEL_SKID_CONTROL, wheel) == 1 && offsetof(WHEEL_SKID_CONTROL, active) == 2 && offsetof(WHEEL_SKID_CONTROL, seg) == 3 ? 1 : -1];

static const uint8 wheel_skid_fixed_width = 0;

typedef struct WEATHER_STATE
{
    VEC3_16 coord[128];
    uint8 visible[128];
    XY rain_prev_head[128];
    VEC3 prev_view;
    uint32 active_count;
    uint32 target_count;
    uint32 mode;
} WEATHER_STATE;

static WEATHER_STATE weather;
static const VEC3_16 weather_wind = {.bits = {0, 0xFF06, 0}};

typedef struct WEATHER_PACKETS
{
    LINE_G2 rain[2][128];
    POLY_FT4 snow[2][128];
    POLY_FT3 control[2];
} WEATHER_PACKETS;

static WEATHER_PACKETS weather_packets;
static const uint8 *weather_snow_tex;

typedef char WEATHER_PACKET_LAYOUT_CHECK[sizeof(LINE_G2) == 20 && sizeof(POLY_FT4) == 40 && sizeof(POLY_FT3) == 32 ? 1 : -1];

uint32 wheel_skid_bank;

static uint32 wheel_skid_seg_cursor;

WHEEL_SKID_CONTROL wheel_skid_controls[8];

typedef union EFFECT_QUAD_PACKET
{
    POLY_FT4 sdk;
    uint32 words[10];
} EFFECT_QUAD_PACKET;

typedef struct WHEEL_SKID_SEGMENT
{
    uint8 flags;
    uint8 color;
    uint16 age;
    VEC3 origin;
    uint32 heading;
    VEC3_16 vertex[4];
    EFFECT_QUAD_PACKET packets[2];
} WHEEL_SKID_SEGMENT;

static WHEEL_SKID_SEGMENT wheel_skid_segs[80];

typedef uint32 (*EFFECT_UPDATE)(uint32 slot);

typedef union EFFECT_PHASE
{
    struct
    {
        uint8 frame;
        uint8 prim;
        uint8 lifetime;
        uint8 pal;
    } spr;

    struct
    {
        uint8 prim;
        uint8 lifetime;
    } particle;
} EFFECT_PHASE;

typedef struct EFFECT_BRIGHTNESS
{
    uint16 intensity;
} EFFECT_BRIGHTNESS;

typedef union EFFECT_SCALE
{
    struct
    {
        uint16 size;
        uint16 step;
    } spr;

    uint32 line_velocity_x;
} EFFECT_SCALE;

typedef union EFFECT_MOTION
{
    VEC3 velocity;

    struct
    {
        uint32 y;
        uint32 z;
    } line;
} EFFECT_MOTION;

typedef struct EFFECT_PARAMETERS
{
    EFFECT_PHASE phase;
    EFFECT_BRIGHTNESS brightness;
    uint16 brightness_step;
    EFFECT_SCALE scale;
    EFFECT_MOTION motion;
} EFFECT_PARAMETERS;

typedef intptr (*EFFECT_RENDER)(uint32 slot);

typedef struct EFFECT_RECORD
{
    EFFECT_PARAMETERS params;
    VEC3 pos;
    VEC3 view_pos;
    uint8 flags;
    EFFECT_UPDATE update;
    EFFECT_RENDER render;
} EFFECT_RECORD;

typedef union EFFECT_LINE_PACKET
{
    LINE_G2 sdk;
    uint32 words[5];
} EFFECT_LINE_PACKET;

typedef union EFFECT_VERTEX
{
    SVECTOR sdk;
    uint32 words[2];
} EFFECT_VERTEX;

typedef char EFFECT_QUAD_PACKET_SIZE_CHECK[sizeof(POLY_FT4) == 40 ? 1 : -1];
typedef char EFFECT_LINE_PACKET_SIZE_CHECK[sizeof(LINE_G2) == 20 ? 1 : -1];
typedef char EFFECT_VERTEX_SIZE_CHECK[sizeof(SVECTOR) == 8 ? 1 : -1];

typedef struct EFFECT_PACKETS
{
    EFFECT_QUAD_PACKET quads[2][20];
    EFFECT_LINE_PACKET lines[2][20];
} EFFECT_PACKETS;

static EFFECT_PACKETS effect_packets;

typedef struct EFFECT_STATE
{
    EFFECT_RECORD records[50];
    MODEL_MATRIX rotation[2];
    uint32 angle[2];
    EFFECT_QUAD_PACKET quad_templates[4];
    EFFECT_LINE_PACKET line_templates[4];
    uint16 pal_clut[8];
    const uint8 *scaled_tex;
    sint8 quad_pool[20];
    sint8 line_pool[20];
} EFFECT_STATE;

static EFFECT_STATE effects;
static VEC3 line_hd_previous[50];
static uint8 line_hd_valid[50];

EFFECT_TEXTURE effect_tex[8];

static const EFFECT_VERTEX line_origin = {.words = {0, 0}};

static EFFECT_VERTEX spr_vertices[4] = {{{76, -76, 0, 0}}, {{-76, -76, 0, 0}}, {{76, 76, 0, 0}}, {{-76, 76, 0, 0}}};

static intptr draw_animated_spr(uint32 idx);
static uint32 update_damped_spr(uint32 idx);

static intptr draw_scaled_spr(uint32 idx);
static uint32 update_fading_spr(uint32 idx);
static uint32 update_line(uint32 slot);
static intptr draw_line(uint32 slot);

static uint32 find_free_skid_seg(void);
static uint32 draw_rain(const VECTOR *pos);

static void call_render(EFFECT_RENDER callback, uint32 slot)
{
    XASSERT(callback != NULL);
    callback(slot);
}

static void call_update(EFFECT_UPDATE callback, uint32 slot)
{
    XASSERT(callback != NULL);
    callback(slot);
}

static uint32 update_pool_cooldown(void)
{
    sint8 *const pools[2] = {effects.quad_pool, effects.line_pool};
    uint32 grp;
    uint32 idx;
    sint32 val;

    FUNCTION_MARKER(0x8003BBB0, "SLUS_006.10");
    for (grp = 0; grp < 2; ++grp)
    {
        for (idx = 0; idx < 20; ++idx)
        {
            val = pools[grp][idx];
            if (val < 0)
                pools[grp][idx] = (val + 1);
        }
    }
    return 0;
}

static uint32 update_visibility(void)
{
    VECTOR input;
    VECTOR output;
    uint32 slot;
    uint32 flags;
    uint32 val;
    uint32 origin;
    sint32 depth;
    uint32 visible;

    FUNCTION_MARKER(0x8003B88C, "SLUS_006.10");
    SetRotMatrix(&model_render.view.sdk);
    for (slot = 0; slot < 50; ++slot)
    {
        flags = effects.records[slot].flags;
        if ((flags & 0x80) != 0 && (flags & 0x20) == 0)
        {
            val = effects.records[slot].pos.bits.x;
            origin = model_render.view.sdk.t[0];
            input.vx = (sint32)((uint32)((sint32)val >> 8) - origin);
            val = effects.records[slot].pos.bits.y;
            origin = model_render.view.sdk.t[1];
            input.vy = (sint32)((uint32)((sint32)val >> 8) - origin);
            val = effects.records[slot].pos.bits.z;
            origin = model_render.view.sdk.t[2];
            input.vz = (sint32)((uint32)((sint32)val >> 8) - origin);
            ApplyRotMatrixLV(&input, &output);
            effects.records[slot].view_pos.bits.x = (uint32)output.vx;
            effects.records[slot].view_pos.bits.y = (uint32)output.vy;
            effects.records[slot].view_pos.bits.z = (uint32)output.vz;
            depth = (sint32)effects.records[slot].view_pos.bits.z;
            visible = 0;
            if ((sint32)model_render.near_depth < depth && depth < (sint32)model_render.far_depth)
                visible = 1;
            flags = effects.records[slot].flags;
            effects.records[slot].flags = (uint8)(visible != 0 ? flags | 0x40 : flags & 0xBF);
        }
    }
    return 0;
}

static void draw_hd_animated_spr(uint32 slot)
{
    const EFFECT_RECORD *record = &effects.records[slot];
    uint32 frame = record->params.phase.spr.frame >> 2;
    uint32 prim = record->params.phase.spr.prim;
    uint32 palette = record->params.phase.spr.pal;
    const EFFECT_TEXTURE *tex;
    RGBC color;
    HD_TEXTURE texture;
    if (!hd_capture_active())
        return;
    XASSERT(frame < 8 && prim < 20 && palette < 8);
    tex = &effect_tex[frame];
    color = effect_packets.quads[disp_state.bank][prim].sdk.rgbc;
    color.r = color.g = color.b = (uint8)(record->params.brightness.intensity >> 8);
    texture = (HD_TEXTURE){tex->tpage, effects.pal_clut[palette], tex->u, tex->v, (uint16)(tex->du + 1), (uint16)(tex->dv + 1), HD_TEXTURED | HD_FILTERED};
    if ((color.code & 1) != 0)
        texture.flags |= HD_RAW_TEXTURE;
    if ((color.code & 2) != 0)
        texture.flags |= HD_SEMITRANSPARENT;
    hd_particle_billboard(slot, &record->pos, record->params.scale.spr.size / 48.0f, &texture, &color);
}

static void draw_hd_line(uint32 slot)
{
    const EFFECT_RECORD *record = &effects.records[slot];
    uint32 prim = record->params.phase.particle.prim;
    const LINE_G2 *packet;
    VEC3 points[2];
    RGBC colors[2];
    if (!hd_capture_active() || !line_hd_valid[slot])
        return;
    XASSERT(prim < 20);
    packet = &effect_packets.lines[disp_state.bank][prim].sdk;
    points[0] = record->pos;
    points[1] = line_hd_previous[slot];
    colors[0] = packet->p[1].rgbc;
    colors[1] = packet->p[0].rgbc;
    hd_effect_line(slot, points, colors, disp_state.cur->draw.tpage, (packet->p[0].rgbc.code & 2) != 0);
}

void effect_draw_all(void)
{
    uint32 angle = effects.angle[0];
    uint32 grp, word, rotation_angle;
    uint32 values[4];
    uint32 slot = 0;
    EFFECT_RENDER callback;

    FUNCTION_MARKER(0x8003B9CC, "SLUS_006.10");
    for (grp = 0; grp < 4; ++grp)
    {
        for (word = 0; word < 4; ++word)
            values[word] = model_render.scr.words[(grp & 1) * 4 + word];
        for (word = 0; word < 4; ++word)
            effects.rotation[grp / 2].words[(grp & 1) * 4 + word] = values[word];
    }
    effects.angle[0] = angle + 128;
    angle = effects.angle[1];
    rotation_angle = effects.angle[0];
    effects.angle[1] = angle - 128;
    RotMatrixZ((sint32)rotation_angle, &effects.rotation[0].sdk);
    angle = effects.angle[1];
    RotMatrixZ((sint32)angle, &effects.rotation[1].sdk);
    update_pool_cooldown();
    update_visibility();
    SetRotMatrix(&model_render.scr.sdk);
    hd_effects_begin();
    do
    {
        if ((effects.records[slot].flags & 0x80) != 0 && effects.records[slot].render == draw_animated_spr)
            draw_hd_animated_spr(slot);
        if ((effects.records[slot].flags & 0x80) != 0 && effects.records[slot].render == draw_line)
            draw_hd_line(slot);
        if ((effects.records[slot].flags & 0xC0) == 0xC0)
        {
            callback = effects.records[slot].render;
            call_render(callback, slot);
        }
        ++slot;
    } while (slot < 50);
}

void effect_update_all(void)
{
    uint32 slot = 0;
    EFFECT_UPDATE callback;

    FUNCTION_MARKER(0x8003BB34, "SLUS_006.10");
    do
    {
        if ((effects.records[slot].flags & 0x80) != 0)
        {
            callback = effects.records[slot].update;
            call_update(callback, slot);
        }
        ++slot;
    } while (slot < 50);
}

uint32 effect_spawn_ballistic_spr(CAR_STATE *car, const VEC3 *point, uint32 style)
{
    EFFECT_QUAD_PACKET *dst;
    uint32 effect, packet, bank, grp, idx, words[4];
    uint32 c, s, horizontal, forward, val, val0, val1, lifetime;
    sint32 random;

    EFFECT_RECORD *control;

    FUNCTION_MARKER(0x8003D288, "SLUS_006.10");
    random = randPSX();
    if (random % 10 < 6)
        return 1;
    for (effect = 0; effect < 50; ++effect)
        if (effects.records[effect].flags == 0)
            break;
    if (effect == 50)
        return 1;
    for (packet = 0; packet < 20; ++packet)
        if (effects.quad_pool[packet] == 0)
            break;
    if (packet == 20)
        return 1;
    effects.quad_pool[packet] = (sint8)(1);
    hd_effect_reset(effect);
    control = effects.records + effect;
    effects.records[effect].params.phase.spr.prim = (uint8)(packet);
    for (bank = 0; bank < 2; ++bank)
    {
        dst = &effect_packets.quads[bank][packet];
        for (grp = 0; grp < 2; ++grp)
        {
            for (idx = 0; idx < 4; ++idx)
                words[idx] = effects.quad_templates[1].words[(grp * 16) / 4 + idx];
            for (idx = 0; idx < 4; ++idx)
                dst->words[(grp * 16) / 4 + idx] = words[idx];
        }
        val0 = effects.quad_templates[1].words[8];
        val1 = effects.quad_templates[1].words[9];
        dst->words[8] = val0;
        dst->words[9] = val1;
    }
    control->flags = (uint8)(0x80);
    control->update = update_damped_spr;
    control->render = draw_animated_spr;
    effects.records[effect].pos.bits.x = point->bits.x;
    effects.records[effect].pos.bits.y = point->bits.y;
    effects.records[effect].pos.bits.z = point->bits.z;
    c = (uint32)rcos(car->orientation.y >> 8);
    s = (uint32)rsin(car->orientation.y >> 8);
    random = randPSX();
    horizontal = (uint32)(random % 24576) - 12288;
    random = randPSX();
    forward = (random % 12288);
    val = horizontal * c + forward * s;
    effects.records[effect].params.motion.velocity.bits.x = (uint32)((uint32)((sint32)val / 4096));
    val = forward * c - horizontal * s;
    effects.records[effect].params.motion.velocity.bits.z = (uint32)((uint32)((sint32)val / 4096));
    effects.records[effect].params.motion.velocity.bits.y = (uint32)(8192);
    random = randPSX();
    effects.records[effect].params.phase.spr.pal = (uint8)(style);
    effects.records[effect].params.phase.spr.frame = (uint8)((uint32)(random % 31));
    random = randPSX();
    lifetime = ((uint32)(random % 4) + 1) * 15;
    val0 = (uint32)fixed_divide_checked(12288, (sint32)(lifetime & 255));
    effects.records[effect].params.phase.spr.lifetime = (uint8)(lifetime);
    lifetime = effects.records[effect].params.phase.spr.lifetime;
    val1 = (uint32)fixed_divide_checked(6400, (sint32)lifetime);
    effects.records[effect].params.brightness.intensity = (uint16)(0x9000);
    effects.records[effect].params.scale.spr.size = (uint16)(8320);
    effects.records[effect].params.brightness_step = (uint16)(0u - val0);
    effects.records[effect].params.scale.spr.step = (uint16)(val1);
    return 8320;
}

static intptr draw_animated_spr(uint32 idx)
{
    DISPLAY_ORDERING ordering;
    EFFECT_QUAD_PACKET *packet;
    uint32 size, frame, buffer, slot, negative;
    const EFFECT_TEXTURE *tex;
    uint32 x, y, z, vertex, val, other, result, shift, depth;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8003C99C, "SLUS_006.10");
    size = effects.records[idx].params.scale.spr.size;
    size = (size >> 4) / 3;
    frame = effects.records[idx].params.phase.spr.frame;
    buffer = disp_state.bank;
    slot = effects.records[idx].params.phase.spr.prim;
    frame >>= 2;
    negative = 0u - size;
    spr_vertices[0].sdk.vx = (sint16)(size);
    spr_vertices[0].sdk.vy = (sint16)(negative);
    spr_vertices[1].sdk.vx = (sint16)(negative);
    spr_vertices[1].sdk.vy = (sint16)(negative);
    spr_vertices[3].sdk.vx = (sint16)(negative);
    spr_vertices[2].sdk.vx = (sint16)(size);
    spr_vertices[2].sdk.vy = (sint16)(size);
    spr_vertices[3].sdk.vy = (sint16)(size);
    packet = &effect_packets.quads[buffer][slot];
    x = effects.records[idx].view_pos.bits.x;
    y = effects.records[idx].view_pos.bits.y;
    z = effects.records[idx].view_pos.bits.z;
    xport_gte_write_control(5, x);
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    for (vertex = 0; vertex < 3; ++vertex)
    {
        xport_gte_write_data(vertex * 2, spr_vertices[vertex].words[0]);
        xport_gte_write_data(vertex * 2 + 1, spr_vertices[vertex].words[1]);
    }
    xport_gte_execute(0x280030);
    tex = &effect_tex[frame];
    setUV0(&packet->sdk, tex->u, tex->v);
    val = tex->u;
    other = tex->du;
    packet->sdk.p[1].uv.u = (uint8)(val + other);
    packet->sdk.p[1].uv.v = (uint8)(tex->v);
    packet->sdk.p[2].uv.u = (uint8)(tex->u);
    val = tex->v;
    other = tex->dv;
    packet->sdk.p[2].uv.v = (uint8)(val + other);
    val = tex->u;
    other = tex->du;
    packet->sdk.p[3].uv.u = (uint8)(val + other);
    val = tex->v;
    other = tex->dv;
    packet->sdk.p[3].uv.v = (uint8)(val + other);
    setRGB0(&packet->sdk, effects.records[idx].params.brightness.intensity >> 8, effects.records[idx].params.brightness.intensity >> 8, effects.records[idx].params.brightness.intensity >> 8);
    packet->sdk.p[0].xy.xy = xport_gte_read_data(12);
    packet->sdk.p[1].xy.xy = xport_gte_read_data(13);
    packet->sdk.p[2].xy.xy = xport_gte_read_data(14);
    result = xport_gte_read_control(31) & 0x1C66000;
    if (result != 0)
        return result;
    xport_gte_write_data(0, spr_vertices[3].words[0]);
    xport_gte_write_data(1, spr_vertices[3].words[1]);
    xport_gte_execute(0x180001);
    packet->sdk.p[1].attr = (uint16)(tex->tpage);
    val = effects.records[idx].params.phase.spr.pal;
    packet->sdk.p[0].attr = (uint16)(effects.pal_clut[val]);
    packet->sdk.p[3].xy.xy = xport_gte_read_data(14);
    xport_gte_execute(0x168002E);
    depth = xport_gte_read_data(7);
    val = model_render.ot_shift;
    other = model_render.coord_shift;
    shift = val - other;
    depth = ((sint32)depth >> (shift & 31));
    if ((sint32)depth <= 0)
        return shift;
    result = (sint32)depth < (sint32)disp_state.ordering_count;
    if (result == 0)
        return result;
    cur_frame = disp_state.cur;
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    other = packet->sdk.tag;
    val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(depth * 4)) / 4];
    packet->sdk.tag = (other & 0xFF000000) | (val & 0xFFFFFF);
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = (uint32)(depth * 4);
    other = ordering.tags[(sint32)ordering.byte_offset / 4];
    result = (other & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    ordering.tags[(sint32)ordering.byte_offset / 4] = result;
    return result;
}

static uint32 update_damped_spr(uint32 idx)
{
    uint32 x, z, val, rate, axis, size_a, step_a, size_b, step_b, frame, life;

    EFFECT_RECORD *control = effects.records + idx;

    FUNCTION_MARKER(0x8003CD14, "SLUS_006.10");
    x = effects.records[idx].params.motion.velocity.bits.x;
    z = effects.records[idx].params.motion.velocity.bits.z;
    effects.records[idx].params.motion.velocity.bits.x = (uint32)(x - (uint32)((sint32)x / 4));
    effects.records[idx].params.motion.velocity.bits.z = (uint32)(z - (uint32)((sint32)z / 4));
    for (axis = 0; axis < 3; ++axis)
    {
        val = effects.records[idx].pos.u[axis];
        rate = effects.records[idx].params.motion.velocity.u[axis];
        effects.records[idx].pos.u[axis] = val + rate;
    }
    size_a = effects.records[idx].params.brightness.intensity;
    step_a = effects.records[idx].params.brightness_step;
    size_b = effects.records[idx].params.scale.spr.size;
    step_b = effects.records[idx].params.scale.spr.step;
    frame = effects.records[idx].params.phase.spr.frame;
    life = effects.records[idx].params.phase.spr.lifetime;
    effects.records[idx].params.phase.spr.frame = (uint8)((frame + 1) & 31);
    val = life + 255;
    effects.records[idx].params.brightness.intensity = (uint16)(size_a + step_a);
    effects.records[idx].params.scale.spr.size = (uint16)(size_b + step_b);
    effects.records[idx].params.phase.spr.lifetime = (uint8)(val);
    if (life == 0)
    {
        hd_effect_retire(idx, &effects.records[idx].pos, NULL);
        idx = effects.records[idx].params.phase.spr.prim;
        val = -3;
        effects.quad_pool[idx] = (sint8)(val);
        control->flags = (uint8)(0);
    }
    return val;
}

uint32 effect_spawn_rear_spr(CAR_STATE *car, const VEC3 *point, uint32 type)
{
    EFFECT_QUAD_PACKET *dst;
    uint32 effect, packet, bank, grp, idx, words[4];
    uint32 c, s, val, val0, val1, lifetime;
    sint32 random;

    EFFECT_RECORD *control;

    FUNCTION_MARKER(0x8003D708, "SLUS_006.10");
    for (effect = 0; effect < 50; ++effect)
        if (effects.records[effect].flags == 0)
            break;
    if (effect == 50)
        return 1;
    for (packet = 0; packet < 20; ++packet)
        if (effects.quad_pool[packet] == 0)
            break;
    if (packet == 20)
        return 1;
    effects.quad_pool[packet] = (sint8)(1);
    hd_effect_reset(effect);
    control = effects.records + effect;
    effects.records[effect].params.phase.spr.prim = (uint8)(packet);
    for (bank = 0; bank < 2; ++bank)
    {
        dst = &effect_packets.quads[bank][packet];
        for (grp = 0; grp < 2; ++grp)
        {
            for (idx = 0; idx < 4; ++idx)
                words[idx] = effects.quad_templates[1].words[(grp * 16) / 4 + idx];
            for (idx = 0; idx < 4; ++idx)
                dst->words[(grp * 16) / 4 + idx] = words[idx];
        }
        val0 = effects.quad_templates[1].words[8];
        val1 = effects.quad_templates[1].words[9];
        dst->words[8] = val0;
        dst->words[9] = val1;
    }
    control->flags = (uint8)(0x80);
    control->update = update_damped_spr;
    control->render = draw_animated_spr;
    effects.records[effect].pos.bits.x = point->bits.x;
    effects.records[effect].pos.bits.y = point->bits.y;
    effects.records[effect].pos.bits.z = point->bits.z;
    c = (uint32)rcos((car->orientation.y >> 8) + 2048);
    s = (uint32)rsin((car->orientation.y >> 8) + 2048);
    val = car->velocity.bits.x;
    effects.records[effect].params.motion.velocity.bits.x = (uint32)(val + (c + s) * 3);
    val = car->velocity.bits.z;
    effects.records[effect].params.motion.velocity.bits.y = (uint32)(512);
    effects.records[effect].params.motion.velocity.bits.z = (uint32)(val + (c - s) * 3);
    random = randPSX();
    effects.records[effect].params.phase.spr.pal = (uint8)(type);
    effects.records[effect].params.phase.spr.frame = (uint8)((uint32)(random % 31));
    random = randPSX();
    lifetime = ((uint32)(random % 4) + 1) * 10;
    val0 = (uint32)fixed_divide_checked(8192, (sint32)(lifetime & 0xFE));
    effects.records[effect].params.phase.spr.lifetime = (uint8)(lifetime);
    lifetime = effects.records[effect].params.phase.spr.lifetime;
    val1 = (uint32)fixed_divide_checked(6400, (sint32)lifetime);
    effects.records[effect].params.brightness.intensity = (uint16)(12288);
    effects.records[effect].params.scale.spr.size = (uint16)(3520);
    effects.records[effect].params.brightness_step = (uint16)(0u - val0);
    effects.records[effect].params.scale.spr.step = (uint16)(val1);
    return 3520;
}

uint32 effect_spawn_col_line(const VEC3 *point, CAR_STATE *car)
{
    EFFECT_LINE_PACKET *dst;
    uint32 speed, effect, packet, copy, idx;
    uint32 words[4];
    sint32 random, result;

    const EFFECT_LINE_PACKET *src;
    EFFECT_RECORD *control;

    FUNCTION_MARKER(0x8003C69C, "SLUS_006.10");
    speed = car->forward_speed;
    if ((sint32)speed < 0)
        speed = 0u - speed;
    if ((sint32)speed <= 65534)
        return 0;
    for (effect = 0; effect < 50; ++effect)
        if (effects.records[effect].flags == 0)
            break;
    if (effect == 50)
        return 1;
    line_hd_valid[effect] = 0;
    for (packet = 0; packet < 20; ++packet)
        if (effects.line_pool[packet] == 0)
            break;
    if (packet == 20)
        return 1;
    effects.line_pool[packet] = (sint8)(1);
    hd_effect_reset(effect);
    control = effects.records + effect;
    effects.records[effect].params.phase.particle.prim = (uint8)(packet);
    random = randPSX();
    src = &effects.line_templates[(uint32)(random % 4)];
    for (copy = 0; copy < 2; ++copy)
    {
        dst = &effect_packets.lines[copy][packet];
        for (idx = 0; idx < 4; ++idx)
            words[idx] = src->words[idx];
        for (idx = 0; idx < 4; ++idx)
            dst->words[idx] = words[idx];
        dst->words[4] = src->words[4];
    }
    control->flags = (uint8)(0x82);
    control->update = update_line;
    control->render = draw_line;
    effects.records[effect].pos.bits.x = point->bits.x;
    effects.records[effect].pos.bits.y = point->bits.y + 30720;
    effects.records[effect].pos.bits.z = point->bits.z;
    effects.records[effect].params.scale.line_velocity_x = (uint32)(car->velocity.bits.x);
    effects.records[effect].params.motion.line.z = (uint32)(car->velocity.bits.z);
    random = randPSX();
    effects.records[effect].params.motion.line.y = (uint32)((uint32)(random % 7000) + 4000);
    result = randPSX() % 2;
    effects.records[effect].params.phase.particle.lifetime = (uint8)((uint32)result * 15);
    return result;
}

static uint32 spawn_wheel_spr(CAR_STATE *car, const VEC3 *pos, uint32 type)
{
    EFFECT_QUAD_PACKET *dst;
    uint32 effect, packet, bank, grp, idx, words[4];
    uint32 c, s, horizontal, forward, val, val0, val1, lifetime;
    sint32 random;

    EFFECT_RECORD *control;

    FUNCTION_MARKER(0x8003CE3C, "SLUS_006.10");
    for (effect = 0; effect < 50; ++effect)
        if (effects.records[effect].flags == 0)
            break;
    if (effect == 50)
        return 1;
    for (packet = 0; packet < 20; ++packet)
        if (effects.quad_pool[packet] == 0)
            break;
    if (packet == 20)
        return 1;
    effects.quad_pool[packet] = (sint8)(1);
    hd_effect_reset(effect);
    control = effects.records + effect;
    effects.records[effect].params.phase.spr.prim = (uint8)(packet);
    for (bank = 0; bank < 2; ++bank)
    {
        dst = &effect_packets.quads[bank][packet];
        for (grp = 0; grp < 2; ++grp)
        {
            for (idx = 0; idx < 4; ++idx)
                words[idx] = effects.quad_templates[1].words[(grp * 16) / 4 + idx];
            for (idx = 0; idx < 4; ++idx)
                dst->words[(grp * 16) / 4 + idx] = words[idx];
        }
        val0 = effects.quad_templates[1].words[8];
        val1 = effects.quad_templates[1].words[9];
        dst->words[8] = val0;
        dst->words[9] = val1;
    }
    control->flags = (uint8)(0x80);
    control->update = update_damped_spr;
    control->render = draw_animated_spr;
    effects.records[effect].pos.bits.x = pos->bits.x;
    effects.records[effect].pos.bits.y = pos->bits.y;
    effects.records[effect].pos.bits.z = pos->bits.z;
    c = (uint32)rcos(car->orientation.y >> 8);
    s = (uint32)rsin(car->orientation.y >> 8);
    random = randPSX();
    horizontal = (uint32)(random % 24576) - 12288;
    random = randPSX();
    forward = 0u - (uint32)(random % 12288);
    val = horizontal * c + forward * s;
    effects.records[effect].params.motion.velocity.bits.x = (uint32)((uint32)((sint32)val / 4096));
    val = forward * c - horizontal * s;
    effects.records[effect].params.motion.velocity.bits.z = (uint32)((uint32)((sint32)val / 4096));
    effects.records[effect].params.motion.velocity.bits.y = (uint32)(6144);
    random = randPSX();
    effects.records[effect].params.phase.spr.pal = (uint8)(type);
    effects.records[effect].params.phase.spr.frame = (uint8)((uint32)(random % 31));
    random = randPSX();
    lifetime = ((uint32)(random % 4) + 1) * 10;
    val0 = (uint32)fixed_divide_checked(12288, (sint32)(lifetime & 0xFE));
    effects.records[effect].params.phase.spr.lifetime = (uint8)(lifetime);
    lifetime = effects.records[effect].params.phase.spr.lifetime;
    val1 = (uint32)fixed_divide_checked(6400, (sint32)lifetime);
    effects.records[effect].params.brightness.intensity = (uint16)(0x7000);
    effects.records[effect].params.scale.spr.size = (uint16)(8320);
    effects.records[effect].params.brightness_step = (uint16)(0u - val0);
    effects.records[effect].params.scale.spr.step = (uint16)(val1);
    return 8320;
}

static uint32 spawn_raindrop(sint32 velocity)
{
    EFFECT_QUAD_PACKET *dst;
    uint32 mag = (velocity >> 13);
    uint32 slot, prim, buffer, chunk, count, i;
    uint32 words[4], flag, life, size, quotient, result;
    sint32 random, denominator;

    EFFECT_RECORD *control;

    FUNCTION_MARKER(0x8003EF50, "SLUS_006.10");
    if ((sint32)mag < 0)
        mag = 0 - mag;
    for (slot = 0; slot < 50; ++slot)
        if (effects.records[slot].flags == 0)
            break;
    if (slot == 50)
        return 1;
    for (prim = 0; prim < 20; ++prim)
        if (effects.quad_pool[prim] == 0)
            break;
    if (prim == 20)
        return 1;
    effects.quad_pool[prim] = (sint8)(1);
    hd_effect_reset(slot);
    control = effects.records + slot;
    effects.records[slot].params.phase.spr.prim = (uint8)(prim);
    for (buffer = 0; buffer < 2; ++buffer)
    {
        dst = &effect_packets.quads[buffer][prim];
        for (chunk = 0; chunk < 40; chunk += count * 4)
        {
            count = chunk < 32 ? 4 : 2;
            for (i = 0; i < count; ++i)
                words[i] = effects.quad_templates[3].words[(chunk) / 4 + i];
            for (i = 0; i < count; ++i)
                dst->words[(chunk) / 4 + i] = words[i];
        }
    }
    control->flags = (uint8)(0x80);
    flag = control->flags;
    control->update = update_fading_spr;
    control->render = draw_scaled_spr;
    control->flags = (uint8)(flag | 0x60);
    random = randPSX();
    denominator = (sint32)model_render.width;
    quotient = (uint32)fixed_divide_checked(random, denominator);
    effects.records[slot].pos.bits.x = (uint32)random - quotient * (uint32)denominator;
    random = randPSX();
    denominator = (sint32)model_render.height;
    quotient = (uint32)fixed_divide_checked(random, denominator);
    effects.records[slot].pos.bits.z = 0;
    effects.records[slot].pos.bits.y = (uint32)random - quotient * (uint32)denominator;
    random = randPSX();
    life = ((uint32)(random % 3) + mag / 8 + 1) * 5;
    effects.records[slot].params.phase.spr.lifetime = (uint8)(life);
    life = effects.records[slot].params.phase.spr.lifetime;
    size = mag * 512 + 0x4000;
    quotient = (uint32)fixed_divide_checked((sint32)((size & 0xFE00) - 0x1000), (sint32)life);
    effects.records[slot].params.brightness.intensity = (uint16)(size);
    effects.records[slot].params.brightness_step = (uint16)(0 - quotient);
    random = randPSX();
    result = (uint32)(random % 6144) + 4096;
    effects.records[slot].params.scale.spr.size = (uint16)(result);
    return result;
}

static intptr draw_scaled_spr(uint32 idx)
{
    DISPLAY_ORDERING ordering;
    EFFECT_QUAD_PACKET *packet;
    uint32 brightness, slot, prim, denominator, width, height;
    uint32 val, tag, shift, other, offset, result;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8003ED48, "SLUS_006.10");
    brightness = effects.records[idx].params.brightness.intensity;
    slot = disp_state.bank;
    prim = effects.records[idx].params.phase.spr.prim;
    packet = &effect_packets.quads[slot][prim];
    setRGB0(&packet->sdk, brightness >> 8, effects.records[idx].params.brightness.intensity >> 8, effects.records[idx].params.brightness.intensity >> 8);
    denominator = effects.records[idx].params.scale.spr.size;
    width = (uint32)fixed_divide_checked(0x18000, (sint32)denominator);
    height = (uint32)fixed_divide_checked(0x10000, (sint32)denominator);
    packet->sdk.p[0].xy.x = (sint16)effects.records[idx].pos.bits.x;
    packet->sdk.p[0].xy.y = (sint16)effects.records[idx].pos.bits.y;
    packet->sdk.p[1].xy.x = (sint16)(effects.records[idx].pos.bits.x + width);
    packet->sdk.p[1].xy.y = (sint16)(effects.records[idx].pos.bits.y);
    packet->sdk.p[2].xy.x = (sint16)(effects.records[idx].pos.bits.x);
    val = effects.records[idx].pos.bits.y;
    tag = packet->sdk.tag;
    packet->sdk.p[2].xy.y = (sint16)(val + height);
    val = effects.records[idx].pos.bits.x;
    shift = model_render.ot_shift;
    packet->sdk.p[3].xy.x = (sint16)(val + width);
    val = effects.records[idx].pos.bits.y;
    other = model_render.coord_shift;
    cur_frame = disp_state.cur;
    shift -= other;
    packet->sdk.p[3].xy.y = (sint16)(val + height);
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    offset = (1 >> (shift & 31)) * 4;
    if (hd_capture_active())
    {
        HD_TEXTURE texture = hd_quad_texture(packet->sdk.p, &packet->sdk.rgbc);
        texture.flags &= ~HD_FILTERED;
        hd_screen_particle(idx, &effects.records[idx].pos, 98304.0f / denominator, 65536.0f / denominator, &texture, &packet->sdk.rgbc, offset / 4);
    }
    val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
    packet->sdk.tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = (uint32)(offset);
    val = ordering.tags[(sint32)ordering.byte_offset / 4];
    result = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    ordering.tags[(sint32)ordering.byte_offset / 4] = result;
    return result;
}

static uint32 update_fading_spr(uint32 idx)
{
    uint32 size, step, life, result, prim;

    EFFECT_RECORD *control = effects.records + idx;

    FUNCTION_MARKER(0x8003ECE8, "SLUS_006.10");
    size = effects.records[idx].params.brightness.intensity;
    step = effects.records[idx].params.brightness_step;
    life = effects.records[idx].params.phase.spr.lifetime;
    effects.records[idx].params.brightness.intensity = (uint16)(size + step);
    result = life + 255;
    effects.records[idx].params.phase.spr.lifetime = (uint8)(result);
    if (life == 0)
    {
        hd_effect_retire(idx, &effects.records[idx].pos, NULL);
        prim = effects.records[idx].params.phase.spr.prim;
        result = 0xFFFFFFFD;
        effects.quad_pool[prim] = (sint8)(result);
        control->flags = (uint8)(0);
    }
    return result;
}

static uint32 update_line(uint32 slot)
{
    uint32 val;
    uint32 velocity;
    uint32 timer;
    uint32 idx;

    FUNCTION_MARKER(0x8003C190, "SLUS_006.10");
    line_hd_previous[slot] = effects.records[slot].pos;
    line_hd_valid[slot] = 1;
    val = effects.records[slot].pos.bits.x;
    velocity = effects.records[slot].params.scale.line_velocity_x;
    effects.records[slot].pos.bits.x = val + velocity;
    val = effects.records[slot].pos.bits.y;
    velocity = effects.records[slot].params.motion.line.y;
    effects.records[slot].pos.bits.y = val + velocity;
    val = effects.records[slot].pos.bits.z;
    velocity = effects.records[slot].params.motion.line.z;
    effects.records[slot].pos.bits.z = val + velocity;
    velocity = effects.records[slot].params.motion.line.y;
    timer = effects.records[slot].params.phase.particle.lifetime;
    effects.records[slot].params.motion.line.y = (uint32)(velocity - 500);
    val = timer + 255;
    effects.records[slot].params.phase.particle.lifetime = (uint8)(val);
    if (timer == 0)
    {
        hd_effect_retire(slot, &effects.records[slot].pos, &line_hd_previous[slot]);
        idx = effects.records[slot].params.phase.particle.prim;
        effects.line_pool[idx] = (sint8)(0xFD);
        effects.records[slot].flags = 0;
        return 0xFFFFFFFD;
    }
    return val;
}

static intptr draw_line(uint32 slot)
{
    DISPLAY_ORDERING ordering;
    EFFECT_LINE_PACKET *packet;
    uint32 buffer_idx;
    uint32 offset;
    uint32 idx;

    uint32 x;
    uint32 y;
    uint32 z;
    uint32 val;
    uint32 shift;

    uint32 tag;
    uint32 link;
    sint32 depth;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8003C010, "SLUS_006.10");
    buffer_idx = disp_state.bank;
    idx = effects.records[slot].params.phase.particle.prim;
    packet = &effect_packets.lines[buffer_idx][idx];
    x = effects.records[slot].view_pos.bits.x;
    y = effects.records[slot].view_pos.bits.y;
    z = effects.records[slot].view_pos.bits.z;
    xport_gte_write_control(5, x);
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    xport_gte_write_data(0, line_origin.words[0]);
    xport_gte_write_data(1, line_origin.words[1]);
    xport_gte_execute(0x180001);
    x = (uint16)packet->sdk.p[1].xy.x;
    y = (uint16)packet->sdk.p[1].xy.y;
    packet->sdk.p[0].xy.x = (sint16)x;
    packet->sdk.p[0].xy.y = (sint16)y;
    val = xport_gte_read_control(31);
    if ((val & 0x1C66000) != 0)
        return (intptr)&packet->sdk.p[1].xy;
    packet->sdk.p[1].xy.xy = xport_gte_read_data(14);
    depth = (sint32)xport_gte_read_data(19) >> 2;
    val = model_render.ot_shift;
    shift = val - model_render.coord_shift;
    depth >>= shift & 31;
    if (depth <= 0)
        return shift;
    if (depth >= (sint32)disp_state.ordering_count)
        return 0;
    cur_frame = disp_state.cur;
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    tag = packet->sdk.tag;
    offset = (uint32)depth * 4;
    link = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
    packet->sdk.tag = (tag & 0xFF000000) | (link & 0xFFFFFF);
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    tag = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
    val = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4] = val;
    return val;
}

uint32 effect_init(void)
{
    char filename[8];
    uint32 idx;
    const uint8 *tex;
    POLY_FT4 *quad;
    LINE_G2 *line;
    EFFECT_TEXTURE *frame;
    const char *name;
    uint32 color;
    uint32 val;
    uint32 other;

    FUNCTION_MARKER(0x8003B264, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&effect_packets, (uint32)sizeof(effect_packets)));
        XASSERT(operation_ok);
    }
    for (idx = 0; idx < 50; ++idx)
    {
        hd_effect_reset(49 - idx);
        line_hd_valid[49 - idx] = 0;
        effects.records[49 - idx].flags = 0;
    }
    for (idx = 0; idx < 20; ++idx)
    {
        effects.quad_pool[19 - idx] = 0;
        effects.line_pool[19 - idx] = 0;
    }
    for (idx = 0; idx < 4; ++idx)
    {
        quad = &effects.quad_templates[idx].sdk;
        if (idx == 0)
        {
            name = "Spark";
        }
        else if (idx == 1)
        {
            name = "Smoke1";
        }
        else if (idx == 2)
        {
            name = "Smoke1";
        }
        else
        {
            name = "drop";
        }
        tex = tex_find(name);
        if (idx == 3)
            effects.scaled_tex = tex;
        setPolyFT4(quad);
        setSemiTrans(quad, 1);
        setUV0(quad, tex[14], tex[15]);
        val = tex[14];
        other = tex[12];
        quad->p[1].uv.u = (uint8)(other + val - 1);
        quad->p[1].uv.v = (uint8)(tex[15]);
        quad->p[2].uv.u = (uint8)(tex[14]);
        val = tex[15];
        other = tex[13];
        quad->p[2].uv.v = (uint8)(other + val - 1);
        val = tex[14];
        other = tex[12];
        quad->p[3].uv.u = (uint8)(other + val - 1);
        val = tex[15];
        other = tex[13];
        color = idx == 3 ? 255 : 128;
        setRGB0(quad, color, color, color);
        quad->p[3].uv.v = (uint8)(other + val - 1);
        quad->p[1].attr = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        quad->p[0].attr = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
    }
    for (idx = 0; idx < 4; ++idx)
    {
        line = &effects.line_templates[idx].sdk;
        setlen(line, 4);
        setcode(line, 0x51);
        line->p[0].xy.x = -1024;
        line->p[0].xy.y = -1024;
        line->p[1].xy.x = -1024;
        line->p[1].xy.y = -1024;
    }
    effects.line_templates[1].sdk.p[1].rgbc.r = (uint8)(230);
    effects.line_templates[1].sdk.p[0].rgbc.r = (uint8)(100);
    effects.line_templates[2].sdk.p[0].rgbc.g = (uint8)(20);
    effects.line_templates[3].sdk.p[1].rgbc.g = (uint8)(190);
    effects.line_templates[3].sdk.p[0].rgbc.r = (uint8)(40);
    setRGB1(&effects.line_templates[0].sdk, 255, 255, 0);
    setRGB0(&effects.line_templates[0].sdk, 90, 0, 0);
    effects.line_templates[1].sdk.p[1].rgbc.g = (uint8)(200);
    effects.line_templates[1].sdk.p[1].rgbc.b = (uint8)(0);
    effects.line_templates[1].sdk.p[0].rgbc.g = (uint8)(0);
    effects.line_templates[1].sdk.p[0].rgbc.b = (uint8)(0);
    setRGB1(&effects.line_templates[2].sdk, 255, 200, 0);
    effects.line_templates[2].sdk.p[0].rgbc.r = (uint8)(90);
    effects.line_templates[2].sdk.p[0].rgbc.b = (uint8)(0);
    effects.line_templates[3].sdk.p[1].rgbc.r = (uint8)(255);
    effects.line_templates[3].sdk.p[1].rgbc.b = (uint8)(0);
    effects.line_templates[3].sdk.p[0].rgbc.g = (uint8)(10);
    effects.line_templates[3].sdk.p[0].rgbc.b = (uint8)(0);
    for (idx = 0; idx < 8; ++idx)
    {
        sprintf(filename, "Smoke%d", (sint32)(idx + 1));
        tex = tex_find(filename);
        frame = &effect_tex[idx];
        frame->u = (uint8)(tex[14]);
        frame->v = (uint8)(tex[15]);
        frame->du = (uint8)(tex[12] - 1);
        frame->dv = (uint8)(tex[13] - 1);
        frame->tpage = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
        frame->tpage_blend0 = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page & 0xFF9F);
        frame->clut = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->clut);
        val = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        effects.pal_clut[idx] = (uint16)val;
    }
    return 0;
}

static uint32 create_wheel_skid(uint32 idx, uint32 slot, uint32 wheel, uint32 rate, uint32 strength)
{
    uint32 handle, sel, val;
    const VEC3 *src;
    WHEEL_SKID_CONTROL *control;

    FUNCTION_MARKER(0x800443AC, "SLUS_006.10");
    handle = idx + wheel_skid_bank * 4;
    control = &wheel_skid_controls[handle];
    control->wheel = (uint8)(wheel);
    control->car_slot = (uint8)(slot);
    control->active = (uint8)(1);
    val = find_free_skid_seg();
    control->seg = (uint8)(val);
    sel = control->seg;
    control->width = (uint8)(rate);
    control->color = (uint8)(strength);
    control->target_color = (uint8)(strength);
    src = &car_states[slot].wheel_contact_pos[wheel];
    val = src->bits.x;
    wheel_skid_segs[sel].origin.bits.x = (uint32)(val);
    sel = control->seg;
    val = src->bits.y;
    wheel_skid_segs[sel].origin.bits.y = (uint32)(val);
    sel = control->seg;
    val = src->bits.z;
    wheel_skid_segs[sel].origin.bits.z = (uint32)(val);
    sel = control->seg;
    wheel_skid_segs[sel].flags = (uint8)(1);
    sel = control->seg;
    wheel_skid_segs[sel].age = (uint16)(0);
    sel = control->seg;
    wheel_skid_segs[sel].color = (uint8)(strength);
    val = wheel_skid_bank;
    return (handle - val * 4) & 255;
}

uint32 effect_update_front_grip_skids(CAR_STATE *car, sint32 amount)
{
    VEC3 spawn_point;
    uint32 wheel, val, type, handle, desc, effect;

    FUNCTION_MARKER(0x80044A2C, "SLUS_006.10");
    for (wheel = 0; wheel < 2; ++wheel)
    {
        if (amount < 15001)
            goto cleanup_gate;
        if (car->motion_mode != 0)
            goto remove_effect;
        if ((car->wheel_slip_flags & 2) != 0 || ((car->wheel_air_mask >> wheel) & 1) != 0)
            goto cleanup_gate;
        amount -= 15000;
        if (amount >= 20481)
        {
            val = car->wheel_contact_pos[wheel].bits.y;
            spawn_point.bits.y = val + 30720;
            val = car->wheel_contact_pos[wheel].bits.x;
            spawn_point.bits.x = val;
            val = car->wheel_contact_pos[wheel].bits.z;
            spawn_point.bits.z = val;
            type = car->surface_type;
            if (type == 1 || type == 10 || type == 3 || type == 5)
                spawn_wheel_spr(car, &spawn_point, 0);
        }
        if (car->wheel_effect_handles[wheel] == 255)
        {
            val = car->slot;
            handle = create_wheel_skid(wheel, val, wheel, 40, 26);
            car->wheel_effect_handles[wheel] = (uint8)(handle);
        }
        handle = car->wheel_effect_handles[wheel];
        wheel_skid_controls[handle + wheel_skid_bank * 4].target_color = (uint8)((uint32)(amount / 2048));
        type = car->surface_type;
        if (type == 2 || type - 5 < 2 || type == 4 || type == 9)
        {
            handle = car->wheel_effect_handles[wheel];
            val = wheel_skid_controls[handle + wheel_skid_bank * 4].target_color;
            wheel_skid_controls[handle + wheel_skid_bank * 4].target_color = (uint8)(val >> 1);
        }
        continue;
    cleanup_gate:
        if (car->motion_mode == 0 && (car->wheel_slip_flags & 2) != 0 && ((car->wheel_air_mask >> wheel) & 1) == 0)
            continue;
    remove_effect:
        handle = car->wheel_effect_handles[wheel];
        if (handle != 255)
        {
            val = wheel_skid_bank;
            desc = (handle + val * 4) & 255;
            effect = wheel_skid_controls[desc].seg;
            wheel_skid_controls[desc].active = (uint8)(0);
            hd_skid_reset(effect);
            wheel_skid_segs[effect].flags = (uint8)(0);
            car->wheel_effect_handles[wheel] = (uint8)(255);
        }
    }
    return 0;
}

uint32 effect_update_rear_grip_skids(CAR_STATE *car, sint32 amount)
{
    VEC3 spawn_point;
    uint32 wheel, val, type, handle, desc, effect;

    FUNCTION_MARKER(0x80044CB4, "SLUS_006.10");
    for (wheel = 2; wheel < 4; ++wheel)
    {
        if (amount < 10001)
            goto cleanup_gate;
        if (car->motion_mode != 0)
            goto remove_effect;
        if ((car->wheel_slip_flags & 1) != 0 || ((car->wheel_air_mask >> wheel) & 1) != 0)
            goto cleanup_gate;
        amount -= 10000;
        if (amount >= 20481)
        {
            val = car->wheel_contact_pos[wheel].bits.y;
            spawn_point.bits.y = val + 30720;
            val = car->wheel_contact_pos[wheel].bits.x;
            spawn_point.bits.x = val;
            val = car->wheel_contact_pos[wheel].bits.z;
            spawn_point.bits.z = val;
            type = car->surface_type;
            if (type == 1 || type == 10 || type == 3 || type == 5)
                spawn_wheel_spr(car, &spawn_point, 0);
        }
        if (car->wheel_effect_handles[wheel] == 255)
        {
            val = car->slot;
            handle = create_wheel_skid(wheel, val, wheel, 55, 26);
            car->wheel_effect_handles[wheel] = (uint8)(handle);
        }
        handle = car->wheel_effect_handles[wheel];
        wheel_skid_controls[handle + wheel_skid_bank * 4].target_color = (uint8)((uint32)(amount / 2048));
        type = car->surface_type;
        if (type == 2 || type - 5 < 2 || type == 4 || type == 9)
        {
            handle = car->wheel_effect_handles[wheel];
            val = wheel_skid_controls[handle + wheel_skid_bank * 4].target_color;
            wheel_skid_controls[handle + wheel_skid_bank * 4].target_color = (uint8)(val >> 1);
        }
        continue;
    cleanup_gate:
        if (car->motion_mode == 0 && (car->wheel_slip_flags & 1) != 0 && ((car->wheel_air_mask >> wheel) & 1) == 0)
            continue;
    remove_effect:
        handle = car->wheel_effect_handles[wheel];
        if (handle != 255)
        {
            val = wheel_skid_bank;
            desc = (handle + val * 4) & 255;
            effect = wheel_skid_controls[desc].seg;
            wheel_skid_controls[desc].active = (uint8)(0);
            hd_skid_reset(effect);
            wheel_skid_segs[effect].flags = (uint8)(0);
            car->wheel_effect_handles[wheel] = (uint8)(255);
        }
    }
    return 0;
}

uint32 effect_update_rear_slip_skids(CAR_STATE *car, uint32 flags, uint32 incoming_result)
{
    VEC3 spawn_point;
    uint32 val, slot, handle, wheel, type, result;
    const VEC3 *point;
    sint32 random_val, strength;

    FUNCTION_MARKER(0x80044F3C, "SLUS_006.10");
    if ((flags & 1) == 0)
        return incoming_result;
    for (wheel = 2; wheel < 4; ++wheel)
    {
        if (car->wheel_effect_handles[wheel] == 255)
        {
            slot = car->slot;
            val = create_wheel_skid(wheel, slot, wheel, 55, 26);
            car->wheel_effect_handles[wheel] = (uint8)(val);
        }
    }
    random_val = randPSX();
    strength = (sint16)car->wheel_slip_amount;
    if (random_val % 50 < strength / 2)
    {
        val = (uint32)randPSX();
        point = &car->wheel_contact_pos[(val & 1) + 2];
        val = point->bits.y;
        spawn_point.bits.y = val + 30720;
        val = point->bits.x;
        spawn_point.bits.x = val;
        val = point->bits.z;
        spawn_point.bits.z = val;
        spawn_wheel_spr(car, &spawn_point, 0);
    }
    for (wheel = 2; wheel < 4; ++wheel)
    {
        handle = car->wheel_effect_handles[wheel];
        strength = (sint16)car->wheel_slip_amount;
        wheel_skid_controls[handle + wheel_skid_bank * 4].target_color = (uint8)((uint32)(strength / 4));
    }
    type = car->surface_type;
    if (type == 2 || type - 5 < 2 || type == 4 || type == 9)
    {
        for (wheel = 2; wheel < 4; ++wheel)
        {
            handle = car->wheel_effect_handles[wheel];
            val = wheel_skid_controls[handle + wheel_skid_bank * 4].target_color;
            result = val >> 1;
            wheel_skid_controls[handle + wheel_skid_bank * 4].target_color = (uint8)(result);
        }
        return result;
    }
    return 9;
}

uint32 effect_update_front_slip_skids(CAR_STATE *car, uint32 flags, uint32 incoming_v0)
{
    VEC3 spawn_point;
    uint32 handle, slot, val, type, result;
    const VEC3 *sel;
    WHEEL_SKID_CONTROL *control;
    sint32 random, amount;

    FUNCTION_MARKER(0x8004515C, "SLUS_006.10");
    if ((flags & 2) == 0)
        return incoming_v0;
    handle = car->wheel_effect_handles[0];
    if (handle == 255)
    {
        slot = car->slot;
        val = create_wheel_skid(0, slot, 0, 55, 26);
        car->wheel_effect_handles[0] = (uint8)(val);
    }
    handle = car->wheel_effect_handles[1];
    if (handle == 255)
    {
        slot = car->slot;
        val = create_wheel_skid(1, slot, 1, 55, 26);
        car->wheel_effect_handles[1] = (uint8)(val);
    }
    random = randPSX();
    amount = (sint16)car->wheel_slip_amount;
    if (random % 50 < amount / 2)
    {
        random = randPSX();
        sel = &car->wheel_contact_pos[(uint32)random & 1];
        val = sel->bits.y;
        spawn_point.bits.y = val + 0x7800;
        spawn_point.bits.x = sel->bits.x;
        spawn_point.bits.z = sel->bits.z;
        spawn_wheel_spr(car, &spawn_point, 0);
    }
    handle = car->wheel_effect_handles[0];
    amount = (sint16)car->wheel_slip_amount;
    wheel_skid_controls[handle + wheel_skid_bank * 4].target_color = (uint8)((uint32)(amount / 4));
    handle = car->wheel_effect_handles[1];
    amount = (sint16)car->wheel_slip_amount;
    wheel_skid_controls[handle + wheel_skid_bank * 4].target_color = (uint8)((uint32)(amount / 4));
    type = car->surface_type;
    if (type != 2 && type - 5 >= 2 && type != 4 && type != 9)
        return 9;
    handle = car->wheel_effect_handles[0];
    control = &wheel_skid_controls[handle + wheel_skid_bank * 4];
    val = control->target_color;
    control->target_color = (uint8)(val >> 1);
    handle = car->wheel_effect_handles[1];
    control = &wheel_skid_controls[handle + wheel_skid_bank * 4];
    result = control->target_color >> 1;
    control->target_color = (uint8)(result);
    return result;
}

uint32 effect_init_wheel_skids(void)
{
    const uint8 *tex;
    uint32 idx;
    EFFECT_QUAD_PACKET *packet;
    uint32 val;
    uint32 other;
    uint32 word0;
    uint32 word1;
    uint32 word2;
    uint32 word3;
    uint32 part;

    FUNCTION_MARKER(0x80043B58, "SLUS_006.10");
    tex = tex_find("Skidmark");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(wheel_skid_segs, (uint32)sizeof(wheel_skid_segs)));
        XASSERT(operation_ok);
    }
    wheel_skid_seg_cursor = (uint32)(0);
    for (idx = 0; idx < 8; ++idx)
        wheel_skid_controls[7 - idx].active = (uint8)(0);
    for (idx = 0; idx < 80; ++idx)
    {
        hd_skid_reset(idx);
        wheel_skid_segs[idx].flags = (uint8)(0);
        packet = wheel_skid_segs[idx].packets;
        packet[0].sdk.tag = (packet[0].sdk.tag & 0x00FFFFFF) | 0x09000000;
        wheel_skid_segs[idx].vertex[0].bits.x = (uint16)(0xFED4);
        wheel_skid_segs[idx].vertex[0].bits.y = (uint16)(0);
        wheel_skid_segs[idx].vertex[0].bits.z = (uint16)(300);
        wheel_skid_segs[idx].vertex[1].bits.x = (uint16)(300);
        wheel_skid_segs[idx].vertex[1].bits.y = (uint16)(0);
        wheel_skid_segs[idx].vertex[1].bits.z = (uint16)(300);
        wheel_skid_segs[idx].vertex[2].bits.x = (uint16)(0xFED4);
        wheel_skid_segs[idx].vertex[2].bits.y = (uint16)(0);
        wheel_skid_segs[idx].vertex[2].bits.z = (uint16)(0xFED4);
        wheel_skid_segs[idx].vertex[3].bits.x = (uint16)(800);
        wheel_skid_segs[idx].vertex[3].bits.y = (uint16)(0);
        wheel_skid_segs[idx].vertex[3].bits.z = (uint16)(0xFED4);
        packet[0].sdk.rgbc.code = (uint8)(0x2C);
        val = packet[0].sdk.rgbc.code;
        packet[0].sdk.rgbc.code = (uint8)(val | 2);
        val = packet[0].sdk.rgbc.code;
        packet[0].sdk.rgbc.code = (uint8)(val & 0xFE);
        packet[0].sdk.rgbc.r = (uint8)(26);
        packet[0].sdk.rgbc.g = (uint8)(26);
        packet[0].sdk.rgbc.b = (uint8)(26);
        val = tex[14];
        packet[0].sdk.p[0].uv.u = (uint8)(val);
        val = tex[15];
        packet[0].sdk.p[0].uv.v = (uint8)(val);
        val = tex[14];
        other = tex[12];
        packet[0].sdk.p[1].uv.u = (uint8)(other + val - 1);
        val = tex[15];
        packet[0].sdk.p[1].uv.v = (uint8)(val);
        val = tex[14];
        packet[0].sdk.p[2].uv.u = (uint8)(val);
        val = tex[15];
        other = tex[13];
        packet[0].sdk.p[2].uv.v = (uint8)(other + val - 1);
        val = tex[14];
        other = tex[12];
        packet[0].sdk.p[3].uv.u = (uint8)(other + val - 1);
        val = tex[15];
        other = tex[13];
        packet[0].sdk.p[3].uv.v = (uint8)(other + val - 1);
        val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        packet[0].sdk.p[1].attr = (uint16)(val);
        val = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        packet[0].sdk.p[0].attr = (uint16)(val);
        for (part = 0; part < 32; part += 16)
        {
            word0 = packet[0].words[0 + part / 4];
            word1 = packet[0].words[1 + part / 4];
            word2 = packet[0].words[2 + part / 4];
            word3 = packet[0].words[3 + part / 4];
            packet[1].words[0 + part / 4] = (uint32)(word0);
            packet[1].words[1 + part / 4] = (uint32)(word1);
            packet[1].words[2 + part / 4] = (uint32)(word2);
            packet[1].words[3 + part / 4] = (uint32)(word3);
        }
        word0 = packet[0].words[8];
        word1 = packet[0].words[9];
        packet[1].words[8] = (uint32)(word0);
        packet[1].words[9] = (uint32)(word1);
    }
    return 0;
}

uint32 effect_update_wheel_skids(void)
{
    WHEEL_SKID_SEGMENT *seg;
    uint32 strip;
    uint32 offset;
    uint32 car_idx;
    uint32 wheel;
    uint32 width;
    uint32 fixed_width;
    uint32 sel;
    uint32 next;
    uint32 car_offset;
    uint32 flags;
    uint32 val;
    uint32 origin;
    uint32 component;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 angle;
    uint32 c;
    uint32 s;
    uint32 left_x;
    uint32 left_z;
    uint32 right_x;
    uint32 right_z;
    uint32 saved_angle;
    uint32 cur_color;
    uint32 target_color;
    uint32 old_origin;
    uint32 new_origin;
    sint32 speed;
    sint32 age_limit;
    sint32 color_difference;

    FUNCTION_MARKER(0x80043DB8, "SLUS_006.10");
    for (strip = 0; strip < 8; ++strip)
    {
        offset = strip * 7;
        if (wheel_skid_controls[strip].active == 0)
            continue;
        car_idx = wheel_skid_controls[strip].car_slot;
        width = wheel_skid_controls[strip].width;
        fixed_width = wheel_skid_fixed_width;
        wheel = wheel_skid_controls[strip].wheel;
        if (fixed_width == 1)
            width = 10;
        val = car_states[car_idx].forward_speed;
        sel = wheel_skid_controls[strip].seg;
        if ((sint32)val < 0)
            val = 0 - val;
        speed = (sint32)val >> 15;
        if (speed <= 0)
            speed = 1;
        else if (speed >= 4)
            speed = 3;
        age_limit = 3 - speed;
        car_offset = car_idx * 864 + wheel * 12;
        seg = &wheel_skid_segs[sel];
        val = car_states[car_idx].wheel_contact_pos[wheel].bits.x;
        origin = seg->origin.bits.x;
        x = val - origin;
        val = car_states[car_idx].wheel_contact_pos[wheel].bits.y;
        origin = seg->origin.bits.y;
        y = val - origin;
        val = car_states[car_idx].wheel_contact_pos[wheel].bits.z;
        origin = seg->origin.bits.z;
        z = val - origin;
        x = ((sint32)x >> 8);
        y = ((sint32)y >> 8);
        z = ((sint32)z >> 8);
        angle = (uint32)fixed_angle_from_vector((sint32)x, (sint32)z);
        c = (uint32)rcos((sint32)angle);
        s = (uint32)rsin((sint32)angle);
        flags = seg->flags;
        val = (0 - width) * c;
        left_x = ((sint32)val / 4096);
        val = 0 - ((0 - width) * s);
        left_z = ((sint32)val / 4096);
        val = width * c;
        right_x = ((sint32)val / 4096);
        val = 0 - width * s;
        right_z = ((sint32)val / 4096);
        if ((flags & 2) == 0 && (x != 0 || z != 0))
        {
            flags = seg->flags;
            seg->flags = (uint8)(flags | 2);
            seg->heading = (uint32)(angle);
            seg->vertex[2].u[0] = (uint16)(left_x);
            seg->vertex[2].u[2] = (uint16)(left_z);
            seg->vertex[2].u[1] = (uint16)(0);
            seg->vertex[3].u[0] = (uint16)(right_x);
            seg->vertex[3].u[2] = (uint16)(right_z);
            seg->vertex[3].u[1] = (uint16)(0);
        }
        seg->vertex[0].u[0] = (uint16)(x + left_x);
        seg->vertex[0].u[2] = (uint16)(z + left_z);
        seg->vertex[0].u[1] = (uint16)(y);
        seg->vertex[1].u[0] = (uint16)(x + right_x);
        if ((flags & 2) == 0 && x == 0 && z == 0)
            seg->heading = (uint32)(angle);
        saved_angle = seg->heading;
        seg->vertex[1].u[2] = (uint16)(z + right_z);
        seg->vertex[1].u[1] = (uint16)(y);
        if (((sint32)saved_angle >> 7) == ((sint32)angle >> 7) && age_limit >= (sint16)seg->age)
            continue;
        next = find_free_skid_seg();
        wheel_skid_controls[strip].seg = (uint8)(next);
        cur_color = wheel_skid_controls[strip].color;
        target_color = wheel_skid_controls[strip].target_color;
        color_difference = (sint32)cur_color - (sint32)target_color;
        if (color_difference < 0)
            wheel_skid_controls[strip].color = (uint8)(cur_color + 1);
        if (color_difference > 0)
        {
            cur_color = wheel_skid_controls[strip].color;
            wheel_skid_controls[strip].color = (uint8)(cur_color - 1);
        }
        flags = seg->flags;
        seg->flags = (uint8)(flags & 2);
        seg = &wheel_skid_segs[next];
        val = car_states[car_idx].wheel_contact_pos[wheel].bits.x;
        seg->origin.bits.x = (uint32)(val);
        val = car_states[car_idx].wheel_contact_pos[wheel].bits.y;
        seg->origin.bits.y = (uint32)(val);
        val = car_states[car_idx].wheel_contact_pos[wheel].bits.z;
        seg->flags = (uint8)(3);
        seg->origin.bits.z = (uint32)(val);
        cur_color = wheel_skid_controls[strip].color;
        seg->packets[0].sdk.rgbc.r = (uint8)(cur_color);
        cur_color = wheel_skid_controls[strip].color;
        seg->packets[0].sdk.rgbc.g = (uint8)(cur_color);
        cur_color = wheel_skid_controls[strip].color;
        seg->packets[0].sdk.rgbc.b = (uint8)(cur_color);
        cur_color = wheel_skid_controls[strip].color;
        seg->packets[1].sdk.rgbc.r = (uint8)(cur_color);
        cur_color = wheel_skid_controls[strip].color;
        seg->packets[1].sdk.rgbc.g = (uint8)(cur_color);
        cur_color = wheel_skid_controls[strip].color;
        seg->packets[1].sdk.rgbc.b = (uint8)(cur_color);
        seg->heading = (uint32)(angle);
        cur_color = wheel_skid_controls[strip].color;
        seg->color = (uint8)(cur_color);
        seg->age = (uint16)(0);
        for (component = 0; component < 4; ++component)
        {
            old_origin = wheel_skid_segs[sel].origin.u[(component & 1) * 2];
            val = wheel_skid_segs[sel].vertex[component / 2].u[(component & 1) * 2];
            new_origin = seg->origin.u[(component & 1) * 2];
            val += (uint32)((sint32)old_origin >> 8) - (uint32)((sint32)new_origin >> 8);
            seg->vertex[component / 2 + 2].u[(component & 1) * 2] = (uint16)(val);
            seg->vertex[component / 2].u[(component & 1) * 2] = (uint16)(val);
        }
    }
    return 0;
}

uint32 effect_draw_wheel_skids(void)
{
    WHEEL_SKID_SEGMENT *seg;
    DISPLAY_ORDERING ordering;
    VECTOR input;
    VECTOR output;
    MATRIX *matrix;
    uint32 offset;
    uint32 val;
    uint32 origin;
    uint32 color;
    uint32 channel;
    uint32 buffer_idx;
    EFFECT_QUAD_PACKET *packet;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 vertex;
    uint32 shift;

    uint32 tag;
    uint32 link;
    sint32 age;
    sint32 clip0;
    sint32 clip1;
    sint32 depth;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80044604, "SLUS_006.10");
    hd_skids_begin();
    for (offset = 0; offset < 10560; offset += 132)
    {
        seg = &wheel_skid_segs[offset / 132];
        if ((seg->flags & 2) == 0)
            continue;
        val = seg->age + 1;
        seg->age = (uint16)(val);
        age = (sint16)val;
        if (age >= 601)
            seg->flags = (uint8)(0);
        age = (sint16)seg->age;
        if (age >= 301)
        {
            if ((age & 8) == 0)
            {
                color = seg->color;
                if (color != 0)
                    seg->color = (uint8)(color - 1);
            }
            for (channel = 0; channel < 3; ++channel)
            {
                buffer_idx = disp_state.bank;
                color = seg->color;
                packet = &seg->packets[buffer_idx];
                if (channel == 0)
                    packet->sdk.rgbc.g = (uint8)(color);
                else if (channel == 1)
                    packet->sdk.rgbc.r = (uint8)(color);
                else
                    packet->sdk.rgbc.b = (uint8)(color);
            }
        }
        if (hd_capture_active())
        {
            VECTORF points[4];
            HD_TEXTURE texture;
            buffer_idx = disp_state.bank;
            packet = &seg->packets[buffer_idx];
            texture = hd_quad_texture(packet->sdk.p, &packet->sdk.rgbc);
            for (vertex = 0; vertex < 4; ++vertex)
            {
                points[vertex].vx = seg->origin.x / 256.0f + seg->vertex[vertex].x;
                points[vertex].vy = seg->origin.y / 256.0f + seg->vertex[vertex].y;
                points[vertex].vz = seg->origin.z / 256.0f + seg->vertex[vertex].z;
            }
            hd_skid_quad(offset / 132, points, &texture, &packet->sdk.rgbc, (seg->flags & 2) == 0);
        }
        val = seg->origin.bits.x;
        origin = model_render.view.sdk.t[0];
        input.vx = (sint32)((uint32)((sint32)val >> 8) - origin);
        val = seg->origin.bits.y;
        origin = model_render.view.sdk.t[1];
        input.vy = (sint32)((uint32)((sint32)val >> 8) - origin);
        val = seg->origin.bits.z;
        origin = model_render.view.sdk.t[2];
        input.vz = (sint32)((uint32)((sint32)val >> 8) - origin);
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
        for (vertex = 0; vertex < 6; ++vertex)
            xport_gte_write_data(vertex, (vertex & 1) != 0 ? seg->vertex[vertex / 2].u[2] : seg->vertex[vertex / 2].u[0] | ((uint32)seg->vertex[vertex / 2].u[1] << 16));
        xport_gte_execute(0x280030);
        xport_gte_execute(0x1400006);
        clip0 = (sint32)xport_gte_read_data(24);
        buffer_idx = disp_state.bank;
        packet = &seg->packets[buffer_idx];
        packet->sdk.p[0].xy.xy = xport_gte_read_data(12);
        packet->sdk.p[1].xy.xy = xport_gte_read_data(13);
        packet->sdk.p[2].xy.xy = xport_gte_read_data(14);
        xport_gte_write_data(0, seg->vertex[3].u[0] | ((uint32)seg->vertex[3].u[1] << 16));
        xport_gte_write_data(1, seg->vertex[3].u[2]);
        xport_gte_execute(0x180001);
        xport_gte_execute(0x1400006);
        clip1 = (sint32)xport_gte_read_data(24);
        xport_gte_execute(0x168002E);
        if (clip0 > 0 && clip1 < 0)
            continue;
        val = xport_gte_read_data(7);
        buffer_idx = disp_state.bank;
        seg->packets[buffer_idx].sdk.p[3].xy.xy = xport_gte_read_data(14);
        shift = model_render.ot_shift;
        shift -= model_render.coord_shift;
        depth = (sint32)(val + 64) >> (shift & 31);
        if (depth < 65)
            continue;
        if (depth >= (sint32)disp_state.ordering_count)
            continue;
        buffer_idx = disp_state.bank;
        cur_frame = disp_state.cur;
        ordering.tags = cur_frame->ordering;
        ordering.byte_offset = 0;
        packet = &seg->packets[buffer_idx];
        tag = packet->sdk.tag;
        link = ordering.tags[(sint32)(ordering.byte_offset + (uint32)((uint32)depth * 4)) / 4];
        packet->sdk.tag = (tag & 0xFF000000) | (link & 0xFFFFFF);
        ordering.tags = cur_frame->ordering;
        ordering.byte_offset = 0;
        tag = ordering.tags[(sint32)(ordering.byte_offset + (uint32)((uint32)depth * 4)) / 4];
        ordering.tags[(sint32)(ordering.byte_offset + (uint32)((uint32)depth * 4)) / 4] = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    }
    return 0;
}

static uint32 find_free_skid_seg(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x800449A0, "SLUS_006.10");
    idx = wheel_skid_seg_cursor;
    if ((sint32)idx < 80)
    {
        do
        {
            if ((wheel_skid_segs[idx].flags & 1) == 0)
            {
                wheel_skid_seg_cursor = (uint32)(idx);
                hd_skid_reset(idx);
                return idx;
            }
            ++idx;
        } while ((sint32)idx < 80);
    }
    idx = 0;
    do
    {
        if ((wheel_skid_segs[idx].flags & 1) == 0)
        {
            wheel_skid_seg_cursor = (uint32)(idx);
            hd_skid_reset(idx);
            return idx;
        }
        ++idx;
    } while (idx < 80);
    hd_skid_reset(0);
    return 0;
}

uint32 effect_try_col_line(CAR_STATE *car, const VEC3 *corner)
{
    sint32 random;

    FUNCTION_MARKER(0x8003F33C, "SLUS_006.10");
    random = randPSX();
    if (random % 100 < 50)
        return effect_spawn_col_line(corner, car);
    return 0;
}

static intptr draw_snow(const VECTOR *pos)
{
    DISPLAY_ORDERING ordering;
    uint32 rotation[5];
    VEC3 local, t, delta;
    const sint32 *components[3] = {&pos->vx, &pos->vy, &pos->vz};
    uint32 axis, val, other, shift, denominator, slot;
    POLY_FT4 *poly;
    uint32 idx = 0, x, y, z, width, height, proj_x, proj_y;
    VEC3_16 *point;
    uint8 *flag;
    uint32 tag, link, u, depth;
    const uint8 *tex;
    sint32 coord, count;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80045EC0, "SLUS_006.10");
    rotation[0] = model_render.view.words[0];
    rotation[1] = model_render.view.words[1];
    xport_gte_write_control(0, rotation[0]);
    xport_gte_write_control(1, rotation[1]);
    rotation[2] = model_render.view.words[2];
    rotation[3] = model_render.view.words[3];
    rotation[4] = model_render.view.words[4];
    xport_gte_write_control(2, rotation[2]);
    xport_gte_write_control(3, rotation[3]);
    xport_gte_write_control(4, rotation[4]);
    for (axis = 0; axis < 3; ++axis)
    {
        val = *components[axis];
        other = weather.prev_view.u[axis];
        val = ((sint32)(val - other) / 256);
        other = weather_wind.u[axis];
        local.u[axis] = (other - val) & 65535;
    }
    x = (uint32)pos->vx;
    y = (uint32)pos->vy;
    z = (uint32)pos->vz;
    weather.prev_view.bits.x = (uint32)(x);
    weather.prev_view.bits.y = (uint32)(y);
    weather.prev_view.bits.z = (uint32)(z);
    xport_gte_write_data(0, local.bits.x | (local.bits.y << 16));
    xport_gte_write_data(1, local.bits.z);
    xport_gte_execute(0x486012);
    t.bits.x = xport_gte_read_data(9) & 65535;
    t.bits.y = xport_gte_read_data(10) & 65535;
    t.bits.z = xport_gte_read_data(11) & 65535;
    shift = model_render.coord_shift;
    denominator = model_render.scale;
    for (axis = 0; axis < 3; ++axis)
    {
        val = (sint16)t.u[axis];
        val = (val << (shift & 31)) << 12;
        delta.u[axis] = (uint32)fixed_divide_checked((sint32)val, (sint32)denominator);
    }
    slot = disp_state.bank;
    poly = weather_packets.snow[slot];
    cur_frame = disp_state.cur;
    count = (sint32)weather.active_count;
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    if (count <= 0)
        return (intptr)cur_frame;
    do
    {
        point = &weather.coord[idx];
        flag = &weather.visible[idx];
        for (axis = 0; axis < 3; ++axis)
            point->u[axis] = (uint16)(point->u[axis] + delta.u[axis]);
        coord = point->x;
        if (coord < -8000)
        {
            point->bits.x = (uint16)((uint32)coord + 16000);
            *flag = (0);
            hd_weather_reset(idx);
        }
        coord = point->x;
        if (coord >= 8000)
        {
            point->bits.x = (uint16)((uint32)coord - 16000);
            *flag = (0);
            hd_weather_reset(idx);
        }
        coord = point->y;
        if (coord < -8000)
        {
            point->bits.y = (uint16)((uint32)coord + 9000);
            *flag = (0);
            hd_weather_reset(idx);
        }
        coord = point->y;
        if (coord >= 1000)
        {
            point->bits.y = (uint16)((uint32)coord - 9000);
            *flag = (0);
            hd_weather_reset(idx);
        }
        coord = point->z;
        if (coord < 200)
        {
            point->bits.z = (uint16)((uint32)coord + 16183);
            *flag = (0);
            hd_weather_reset(idx);
        }
        coord = point->z;
        if (coord >= 16383)
        {
            point->bits.z = (uint16)((uint32)coord - 16183);
            *flag = (0);
            hd_weather_reset(idx);
        }
        x = (uint32)point->x;
        z = (uint32)point->z;
        proj_x = (uint32)fixed_divide_checked((sint32)(x * 180), (sint32)z);
        y = (uint32)point->y;
        proj_y = (uint32)fixed_divide_checked((sint32)(y * 180), (sint32)z);
        width = model_render.width;
        height = model_render.height;
        x = (uint32)((sint32)width / 2) + proj_x;
        y = (uint32)((sint32)height / 2) + proj_y;
        poly->p[0].xy.x = (sint16)(uint16)(x - 1);
        poly->p[1].xy.x = (sint16)(uint16)(x + 3);
        poly->p[2].xy.x = (sint16)(uint16)(x - 1);
        poly->p[3].xy.x = (sint16)(uint16)(x + 3);
        poly->p[0].xy.y = (sint16)(uint16)(y - 1);
        poly->p[1].xy.y = (sint16)(uint16)(y - 1);
        poly->p[2].xy.y = (sint16)(uint16)(y + 3);
        poly->p[3].xy.y = (sint16)(uint16)(y + 3);
        tag = poly->tag;
        link = ordering.tags[(sint32)ordering.byte_offset / 4];
        poly->tag = (uint32)((tag & 0xFF000000) | (link & 0xFFFFFF));
        tag = ordering.tags[(sint32)ordering.byte_offset / 4];
        ordering.tags[(sint32)ordering.byte_offset / 4] = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
        depth = (uint32)point->z;
        tex = weather_snow_tex;
        u = tex[14];
        val = (uint32)((sint32)depth / 2048) * 4;
        poly->p[0].uv.u = (uint8)(u + 28 - val);
        tex = weather_snow_tex;
        poly->p[0].uv.v = (uint8)(tex[15]);
        tex = weather_snow_tex;
        u = tex[14];
        depth = (uint32)point->z;
        val = (uint32)((sint32)depth / 2048) * 4;
        poly->p[1].uv.u = (uint8)(u + 32 - val);
        tex = weather_snow_tex;
        poly->p[1].uv.v = (uint8)(tex[15]);
        depth = (uint32)point->z;
        tex = weather_snow_tex;
        u = tex[14];
        val = (uint32)((sint32)depth / 2048) * 4;
        poly->p[2].uv.u = (uint8)(u + 28 - val);
        tex = weather_snow_tex;
        poly->p[2].uv.v = (uint8)(tex[15] + 4);
        tex = weather_snow_tex;
        u = tex[14];
        depth = (uint32)point->z;
        val = (uint32)((sint32)depth / 2048) * 4;
        poly->p[3].uv.u = (uint8)(u + 32 - val);
        tex = weather_snow_tex;
        poly->p[3].uv.v = (uint8)(tex[15] + 4);
        if (hd_capture_active())
        {
            HD_UI_VERTEX vertices[4];
            HD_TEXTURE texture = hd_quad_texture(poly->p, &poly->rgbc);
            float center_x = model_render.width * 0.5f + point->x * 180.0f / point->z;
            float center_y = model_render.height * 0.5f + point->y * 180.0f / point->z;
            uint32 corner;
            texture.width = texture.height = 4;
            for (corner = 0; corner < 4; ++corner)
            {
                vertices[corner].x = center_x + ((corner & 1) != 0 ? 3.0f : -1.0f);
                vertices[corner].y = center_y + ((corner & 2) != 0 ? 3.0f : -1.0f);
                vertices[corner].u = (float)poly->p[corner].uv.u;
                vertices[corner].v = (float)poly->p[corner].uv.v;
                memcpy(vertices[corner].color, &poly->rgbc, 4);
            }
            hd_weather_snow(idx, vertices, &texture);
        }
        ++poly;
        count = (sint32)weather.active_count;
        ++idx;
    } while ((sint32)idx < count);
    return 0;
}

static uint32 init_snow(void)
{
    const uint8 *tex_rec;
    const uint8 *tex;
    uint32 idx;
    uint32 buffer;
    POLY_FT4 *poly;
    uint32 val;
    sint32 random;

    FUNCTION_MARKER(0x800456E4, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&weather_packets, (uint32)sizeof(weather_packets)));
        XASSERT(operation_ok);
    }
    tex_rec = tex_find("Snow1");
    weather_snow_tex = tex_rec;
    for (idx = 0; idx < 128; ++idx)
    {
        random = randPSX();
        weather.coord[idx].bits.x = (uint16)((uint32)(random % 16000 - 8000));
        random = randPSX();
        weather.coord[idx].bits.y = (uint16)((uint32)(random % 9000 - 8000));
        random = randPSX();
        weather.coord[idx].bits.z = (uint16)((uint32)(random % 16183 + 200));
        for (buffer = 0; buffer < 2; ++buffer)
        {
            poly = &weather_packets.snow[buffer][idx];
            setlen(poly, 9);
            poly->rgbc.code = (uint8)(0x2E);
            poly->p[0].uv.u = (uint8)(weather_snow_tex[14]);
            poly->p[0].uv.v = (uint8)(weather_snow_tex[15]);
            poly->p[1].uv.u = (uint8)(weather_snow_tex[14] + 4);
            poly->p[1].uv.v = (uint8)(weather_snow_tex[15]);
            poly->p[2].uv.u = (uint8)(weather_snow_tex[14]);
            poly->p[2].uv.v = (uint8)(weather_snow_tex[15] + 4);
            poly->p[3].uv.u = (uint8)(weather_snow_tex[14] + 4);
            val = weather_snow_tex[15];
            poly->rgbc.r = (uint8)(64);
            poly->rgbc.g = (uint8)(64);
            poly->rgbc.b = (uint8)(64);
            poly->p[3].uv.v = (uint8)(val + 4);
            tex = weather_snow_tex;
            poly->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
            poly->p[0].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->clut);
        }
    }
    return 0;
}

static uint32 init_rain(void)
{
    uint32 idx;
    uint32 buffer;
    LINE_G2 *line;
    POLY_FT3 *poly;
    sint32 val;

    FUNCTION_MARKER(0x80045490, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&weather_packets, (uint32)sizeof(weather_packets)));
        XASSERT(operation_ok);
    }
    for (idx = 0; idx < 128; ++idx)
    {
        val = randPSX();
        weather.coord[idx].bits.x = (uint16)((uint32)(val % 8000 - 4000));
        val = randPSX();
        weather.coord[idx].bits.y = (uint16)((uint32)(val % 5000 - 4000));
        val = randPSX();
        weather.coord[idx].bits.z = (uint16)((uint32)(val % 7991 + 200));
        for (buffer = 0; buffer < 2; ++buffer)
        {
            line = &weather_packets.rain[buffer][idx];
            setlen(line, 4);
            line->p[0].rgbc.code = (uint8)(0x52);
            line->p[0].rgbc.r = (uint8)(48);
            line->p[0].rgbc.g = (uint8)(48);
            line->p[0].rgbc.b = (uint8)(48);
            line->p[1].rgbc.r = (uint8)(8);
            line->p[1].rgbc.g = (uint8)(8);
            line->p[1].rgbc.b = (uint8)(8);
        }
    }
    for (buffer = 0; buffer < 2; ++buffer)
    {
        poly = &weather_packets.control[buffer];
        setPolyFT3(poly);
        poly->p[1].attr = (uint16)(32);
        poly->p[0].xy.x = (sint16)(uint16)(0xFFFF);
        poly->p[0].xy.y = (sint16)(uint16)(0xFFFF);
        poly->p[1].xy.x = (sint16)(uint16)(0xFFFF);
        poly->p[1].xy.y = (sint16)(uint16)(0xFFFF);
        poly->p[2].xy.x = (sint16)(uint16)(0xFFFF);
        poly->p[2].xy.y = (sint16)(uint16)(0xFFFF);
    }
    return 0;
}

uint32 effect_init_weather(void)
{
    uint16 wire_halfword_1;

    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    const uint8 *definition;
    sint32 mode;

    FUNCTION_MARKER(0x80046460, "SLUS_006.10");
    {
        uint32 slot;
        for (slot = 0; slot < 128; ++slot)
            hd_weather_reset(slot);
    }
    level_desc = &scene_level_image.desc;
    XASSERT(!(scene_level_image.bytes == NULL || scene_level_image.size < 2 || level_desc->effects_offset > scene_level_image.size - 2));
    definition = scene_level_image.bytes + (level_desc->effects_offset);
    mode = (sint16)(memcpy(&wire_halfword_1, definition, sizeof(wire_halfword_1)), wire_halfword_1);
    weather.active_count = (uint32)(0);
    weather.target_count = (uint32)(0);
    weather.mode = (uint32)((uint32)mode);
    if (mode == 1)
        return init_snow();
    if (mode == 0)
        return init_rain();
    return mode < 2;
}

sint32 effect_has_rain(void)
{
    const SCENE_LEVEL_DESCRIPTOR *desc = &scene_level_image.desc;
    sint16 mode;
    XASSERT(scene_level_image.bytes != NULL && desc->effects_offset <= scene_level_image.size && scene_level_image.size - desc->effects_offset >= 34);
    const uint8 *definition = scene_level_image.bytes + desc->effects_offset;
    memcpy(&mode, definition, sizeof(mode));
    if (mode != 0)
        return 0;
    for (uint32 index = 0; index < 8; ++index)
    {
        sint16 node, count;
        memcpy(&node, definition + 2 + index * 4, sizeof(node));
        memcpy(&count, definition + 4 + index * 4, sizeof(count));
        if (node >= 0 && (uint32)node < track_geometry.node_count && count > 0)
            return 1;
    }
    return 0;
}

float effect_rain_amount(void)
{
    return weather.mode == 0 ? weather.active_count / 128.0f : 0.0f;
}

intptr effect_update_weather(CAR_STATE *car)
{
    uint16 wire_halfword_1, wire_halfword_2;

    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    const uint8 *table;
    uint32 idx;
    uint32 val;
    uint32 target;
    uint32 count;
    uint32 random;
    uint32 retiring_slot = 128;
    intptr result;
    sint32 node;
    sint32 mode;

    FUNCTION_MARKER(0x800464CC, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    XASSERT(!(scene_level_image.bytes == NULL || scene_level_image.size < 34 || level_desc->effects_offset > scene_level_image.size - 34));
    node = (sint16)car->track_location.node;
    table = scene_level_image.bytes + (level_desc->effects_offset);
    for (idx = 0; idx < 8; ++idx)
    {
        if (node == (sint16)(memcpy(&wire_halfword_1, table + 2, sizeof(wire_halfword_1)), wire_halfword_1))
        {
            val = (sint16)(memcpy(&wire_halfword_2, table + 4, sizeof(wire_halfword_2)), wire_halfword_2);
            weather.target_count = (uint32)(val);
        }
        table += 4;
    }
    target = weather.target_count;
    XASSERT(!((sint32)target > (sint32)(sizeof(weather.visible) / sizeof(weather.visible[0]))));
    if (target != 0)
    {
        count = weather.active_count;
        if ((sint32)count < (sint32)target)
        {
            weather.visible[count] = (uint8)(0);
            hd_weather_reset(count);
            val = weather.active_count;
            weather.active_count = (uint32)(val + 1);
        }
        target = weather.target_count;
    }
    if (target == 0)
    {
        count = weather.active_count;
        if ((sint32)count > 0)
        {
            retiring_slot = count - 1;
            weather.active_count = (uint32)(count - 1);
        }
    }
    mode = (sint32)weather.mode;
    if (mode == 1)
    {
        idx = car->slot;
        result = draw_snow(&camera_views[idx].eye);
        if (retiring_slot < 128)
            hd_weather_retire(retiring_slot, weather.mode);
        return result;
    }
    if (mode >= 2)
        return 0;
    if (mode != 0)
        return 1;
    random = (uint32)randPSX();
    count = weather.active_count;
    if ((sint32)count >= (sint32)(random & 127) && (sint32)count > 0)
        spawn_raindrop((sint32)car->steered_speed);
    idx = car->slot;
    result = (intptr)draw_rain(&camera_views[idx].eye);
    if (retiring_slot < 128)
        hd_weather_retire(retiring_slot, weather.mode);
    return result;
}

static uint32 draw_rain(const VECTOR *pos)
{
    DISPLAY_ORDERING ordering;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 val;
    uint32 prev;
    uint32 shift;
    uint32 delta_x;
    uint32 delta_y;
    uint32 delta_z;
    uint32 buffer_idx;
    LINE_G2 *line;
    POLY_FT3 *poly;

    uint32 idx;
    VEC3_16 *point;
    uint8 *visible;
    uint32 tag;
    uint32 link;
    uint32 scr_y;
    sint32 count;
    sint32 divisor;
    sint32 coord;
    sint32 depth;
    sint32 proj;
    sint32 width;
    sint32 height;
    sint32 scr_x;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80045980, "SLUS_006.10");
    x = model_render.view.words[0];
    y = model_render.view.words[1];
    xport_gte_write_control(0, x);
    xport_gte_write_control(1, y);
    x = model_render.view.words[2];
    y = model_render.view.words[3];
    z = model_render.view.words[4];
    xport_gte_write_control(2, x);
    xport_gte_write_control(3, y);
    xport_gte_write_control(4, z);
    val = (uint32)pos->vx;
    prev = weather.prev_view.bits.x;
    val = ((sint32)(val - prev) / 256);
    x = weather_wind.bits.x - val;
    val = (uint32)pos->vy;
    prev = weather.prev_view.bits.y;
    val = ((sint32)(val - prev) / 256);
    y = weather_wind.bits.y - val;
    val = (uint32)pos->vz;
    prev = weather.prev_view.bits.z;
    val = ((sint32)(val - prev) / 256);
    z = weather_wind.bits.z - val;
    val = (uint32)pos->vx;
    prev = (uint32)pos->vy;
    shift = (uint32)pos->vz;
    weather.prev_view.bits.x = (uint32)(val);
    weather.prev_view.bits.y = (uint32)(prev);
    weather.prev_view.bits.z = (uint32)(shift);
    xport_gte_write_data(0, (x & 0xFFFF) | (y << 16));
    xport_gte_write_data(1, z & 0xFFFF);
    xport_gte_execute(0x486012);
    x = xport_gte_read_data(9);
    y = xport_gte_read_data(10);
    z = xport_gte_read_data(11);
    shift = model_render.coord_shift;
    divisor = (sint32)model_render.scale;
    val = ((uint32)(sint32)(sint16)x << (shift & 31)) << 12;
    delta_x = (uint32)fixed_divide_checked((sint32)val, divisor);
    val = ((uint32)(sint32)(sint16)y << (shift & 31)) << 12;
    delta_y = (uint32)fixed_divide_checked((sint32)val, divisor);
    val = ((uint32)(sint32)(sint16)z << (shift & 31)) << 12;
    delta_z = (uint32)fixed_divide_checked((sint32)val, divisor);
    buffer_idx = disp_state.bank;
    cur_frame = disp_state.cur;
    line = weather_packets.rain[buffer_idx];
    count = (sint32)weather.active_count;
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    idx = 0;
    if (count > 0)
    {
        do
        {
            point = &weather.coord[idx];
            visible = &weather.visible[idx];
            val = point->bits.x;
            point->bits.x = (uint16)(val + delta_x);
            val = point->bits.y;
            point->bits.y = (uint16)(val + delta_y);
            val = point->bits.z;
            point->bits.z = (uint16)(val + delta_z);
            coord = point->x;
            if (coord < -4000)
            {
                point->bits.x = (uint16)((uint32)coord + 8000);
                *visible = (0);
                hd_weather_reset(idx);
            }
            coord = point->x;
            if (coord >= 4000)
            {
                point->bits.x = (uint16)((uint32)coord - 8000);
                *visible = (0);
                hd_weather_reset(idx);
            }
            coord = point->y;
            if (coord < -4000)
            {
                point->bits.y = (uint16)((uint32)coord + 5000);
                *visible = (0);
                hd_weather_reset(idx);
            }
            coord = point->y;
            if (coord >= 1000)
            {
                point->bits.y = (uint16)((uint32)coord - 5000);
                *visible = (0);
                hd_weather_reset(idx);
            }
            coord = point->z;
            if (coord < 200)
            {
                point->bits.z = (uint16)((uint32)coord + 7991);
                *visible = (0);
                hd_weather_reset(idx);
            }
            coord = point->z;
            if (coord >= 8191)
            {
                point->bits.z = (uint16)((uint32)coord - 7991);
                *visible = (0);
                hd_weather_reset(idx);
            }
            {
                HD_UI_VERTEX head = {0};
                head.x = model_render.width * 0.5f + point->x * 180.0f / point->z;
                head.y = model_render.height * 0.5f + point->y * 180.0f / point->z;
                memcpy(head.color, &line->p[0].rgbc, 4);
                hd_weather_rain(idx, &head, &line->p[1].rgbc);
            }
            coord = point->x;
            depth = point->z;
            proj = fixed_divide_checked((sint32)((uint32)coord * 180), depth);
            width = (sint32)model_render.width;
            val = (uint32)(width / 2) + (uint32)proj;
            line->p[0].xy.x = (sint16)(uint16)(val);
            coord = point->y;
            depth = point->z;
            proj = fixed_divide_checked((sint32)((uint32)coord * 180), depth);
            height = (sint32)model_render.height;
            scr_x = line->p[0].xy.x;
            scr_y = (uint32)(height / 2) + (uint32)proj;
            line->p[0].xy.y = (sint16)(uint16)(scr_y);
            if (scr_x < -63 || scr_x >= (sint32)((uint32)width + 64) || (sint16)scr_y < -63 || (sint16)scr_y >= (sint32)((uint32)height + 64))
                *visible = (0);
            else
            {
                if (*visible != 0)
                {
                    val = weather.rain_prev_head[idx].xy;
                    line->p[1].xy.xy = (uint32)(val);
                    tag = line->tag;
                    link = ordering.tags[(sint32)ordering.byte_offset / 4];
                    line->tag = (uint32)((tag & 0xFF000000) | (link & 0xFFFFFF));
                    tag = ordering.tags[(sint32)ordering.byte_offset / 4];
                    ordering.tags[(sint32)ordering.byte_offset / 4] = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)line);
                }
                *visible = (1);
            }
            weather.rain_prev_head[idx].xy = line->p[0].xy.xy;
            ++line;
            count = (sint32)weather.active_count;
            ++idx;
        } while ((sint32)idx < count);
    }
    buffer_idx = disp_state.bank;
    link = ordering.tags[(sint32)ordering.byte_offset / 4];
    poly = &weather_packets.control[buffer_idx];
    tag = poly->tag;
    poly->tag = (uint32)((tag & 0xFF000000) | (link & 0xFFFFFF));
    tag = ordering.tags[(sint32)ordering.byte_offset / 4];
    val = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
    ordering.tags[(sint32)ordering.byte_offset / 4] = val;
    return val;
}
