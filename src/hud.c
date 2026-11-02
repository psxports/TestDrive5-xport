#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "psx_gpu.h"

typedef struct HUD_GAUGE_PACKETS
{
    POLY_FT4 reverse_turn[2];
    POLY_FT4 numbers[3][2];
    POLY_FT4 speed_units[2];
    POLY_FT4 gears[2];
    POLY_FT4 speedometer[2];
    POLY_FT4 checkpoint[2][3];
} HUD_GAUGE_PACKETS;

typedef struct HUD_GAUGE_TEXTURES
{
    const uint8 *checkpoint_font;
    const uint8 *numbers;
    const uint8 *gears;
    const uint8 *reverse_turn;
    const uint8 *speedometer;
    const uint8 *speed_units;
} HUD_GAUGE_TEXTURES;

typedef struct HUD_NEEDLE_PACKETS
{
    POLY_F3 needle[2][2];
    POLY_F3 shadow[2][2];
} HUD_NEEDLE_PACKETS;

typedef struct HUD_REVERSE_VIEW
{
    uint32 prev_node;
    uint32 reversing_frames;
    uint32 blink_frames;
} HUD_REVERSE_VIEW;

static HUD_REVERSE_VIEW reverse_views[2];
HUD_CHECKPOINT_DISPLAY hud_checkpoint_disp;

enum
{
    HUD_RACE_TEXT_GLYPH_COUNT = 80
};

typedef struct HUD_RACE_TEXT
{
    POLY_FT4 packets[HUD_RACE_TEXT_GLYPH_COUNT][2];
    uint32 packet_idx;
    const uint8 *tex;
} HUD_RACE_TEXT;

static HUD_RACE_TEXT race_text;

static HUD_GAUGE_PACKETS gauge_packets;

typedef char HUD_GAUGE_PACKET_LAYOUT_CHECK[sizeof(POLY_FT4) == 40 && sizeof(HUD_GAUGE_PACKETS) == 800 ? 1 : -1];

static HUD_GAUGE_TEXTURES gauge_tex;

static HUD_NEEDLE_PACKETS needle_packets;

typedef char HUD_NEEDLE_PACKET_LAYOUT_CHECK[sizeof(POLY_F3) == 20 && sizeof(HUD_NEEDLE_PACKETS) == 160 ? 1 : -1];

typedef struct HUD_DRAG_START
{
    const uint8 *panel_tex;
    const uint8 *lamp_tex;
    const uint8 *active_tex;
    uint32 ticks;
    uint32 phase;
    uint32 origin_x;
    uint32 origin_y;
    uint32 contact0_latched;
    uint32 contact1_latched;
} HUD_DRAG_START;

static HUD_DRAG_START drag_start;
static POLY_FT4 drag_packets[2][7];
static const uint8 drag_colors[5][4] = {
    {128, 80, 32, 0}, {128, 80, 32, 0}, {128, 80, 32, 0}, {16, 128, 16, 0}, {128, 16, 16, 0},
};

typedef struct HUD_ROUTE_VIEW
{
    uint32 prev_node;
    uint32 sel_entry;
    uint32 hold_ticks;
    uint32 opacity;
    uint32 reverse;
    uint32 draw_idx;
} HUD_ROUTE_VIEW;

static HUD_ROUTE_VIEW route_views[2];
static uint32 route_count;
static const uint8 *route_entries;
static POLY_FT4 route_packets[4][2];

uint32 hud_countdown_frames[2];
static const uint8 *digit_tex;
static uint32 countdown_aux;

const uint8 *hud_arrow_tex[6];

static const char *const arrow_names[6] = {"Arrow1", "Arrow1", "Arrow2", "Arrow2", "Arrow3", "Arrow4"};

static POLY_FT4 digit_packets[2][2];

typedef char HUD_DIGIT_FT4_SIZE_CHECK[sizeof(POLY_FT4) == 40 ? 1 : -1];

static uint32 draw_status_text(CAR_STATE *car);

uint32 hud_init_scene(void)
{
    uint16 wire_halfword_1, wire_halfword_2;

    const uint8 *found_tex;
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    uint32 entries_address;
    const uint8 *entries;
    uint32 idx;
    const char *name;
    uint32 len;
    uint32 val;
    uint32 row;
    uint32 column;
    POLY_FT4 *prim;

    FUNCTION_MARKER(0x80042C50, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&gauge_packets, (uint32)sizeof(gauge_packets)));
        XASSERT(operation_ok);
    }
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&needle_packets, (uint32)sizeof(needle_packets)));
        XASSERT(operation_ok);
    }
    level_desc = &scene_level_image.desc;
    route_views[1].prev_node = 0;
    route_views[0].prev_node = 0;
    route_views[1].sel_entry = 0;
    route_views[0].sel_entry = 0;
    route_views[1].hold_ticks = 0;
    route_views[0].hold_ticks = 0;
    entries_address = level_desc->route_hints_offset;
    route_count = 0;
    XASSERT(!(entries_address > scene_level_image.size));
    route_entries = entries_address == 0 ? NULL : scene_level_image.bytes + entries_address;
    for (idx = 0; idx < 6; ++idx)
    {
        name = arrow_names[idx];
        len = 0;
        while (name[len] != 0)
            ++len;
        found_tex = tex_find(name);
        hud_arrow_tex[idx] = found_tex;
    }
    {
        sint32 operation_ok = !(!gpu_register_packet_range(digit_packets, (uint32)sizeof(digit_packets)));
        XASSERT(operation_ok);
    }
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    val = level_header[30] & 0x10;
    if (val == 0)
        return val;
    idx = route_count;
    entries = route_entries;
    val = (memcpy(&wire_halfword_1, entries + idx * 6, sizeof(wire_halfword_1)), wire_halfword_1);
    while (val != 0xFFFF)
    {
        ++idx;
        val = (memcpy(&wire_halfword_2, entries + idx * 6, sizeof(wire_halfword_2)), wire_halfword_2);
        route_count = idx;
    }
    for (row = 0; row < 4; ++row)
    {
        for (column = 0; column < 2; ++column)
        {
            prim = &route_packets[row][column];
            prim->tag = (prim->tag & 0x00FFFFFFu) | (9u << 24);
            prim->rgbc.code = 0x2E;
        }
    }
    return 0;
}

uint32 hud_draw_digit(uint32 slot, sint32 digit, uint32 r, uint32 g, uint32 b)
{
    DISPLAY_ORDERING ordering;
    uint32 width;
    uint32 origin_x;
    uint32 height;
    uint32 origin_y;
    uint32 buffer_idx;
    POLY_FT4 *packet;
    uint8 *uv_component;
    const uint8 *tex;
    uint32 corner;
    uint32 val;
    uint32 uv;
    uint32 x;
    uint32 y;
    uint32 attribute;
    uint32 opcode;

    uint32 tag;
    uint32 link;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80047278, "SLUS_006.10");
    b &= 255;
    width = race_view.viewport_width;
    origin_x = race_view.viewport_x;
    height = race_view.viewport_height;
    origin_y = race_view.viewport_y;
    buffer_idx = disp_state.bank;
    packet = &digit_packets[slot][buffer_idx];
    packet->tag = (packet->tag & 0x00FFFFFFu) | ((uint32)(uint8)(9) << 24);
    packet->rgbc.code = (uint8)(44);
    for (corner = 0; corner < 7; ++corner)
    {
        tex = digit_tex;
        if ((corner & 1) == 0)
        {
            val = tex[14] + ((uint32)digit << 4);
            if ((corner & 2) != 0)
                val += 15;
        }
        else
        {
            val = tex[15];
            if (corner >= 5)
                val += 23;
        }
        uv_component = (corner & 1) != 0 ? &packet->p[corner / 2].uv.v : &packet->p[corner / 2].uv.u;
        *uv_component = val;
    }
    tex = digit_tex;
    uv = tex[15];
    y = (uint32)((sint32)height / 3) + origin_y;
    x = (uint32)((sint32)width / 2) + origin_x;
    packet->p[0].xy.x = (sint16)(x - 8);
    packet->p[3].uv.v = (uint8)(uv + 23);
    packet->p[0].xy.y = (sint16)(y - 12);
    tex = digit_tex;
    packet->p[1].xy.x = (sint16)(x + 7);
    packet->p[1].xy.y = (sint16)(y - 12);
    packet->p[2].xy.x = (sint16)(x - 8);
    packet->p[2].xy.y = (sint16)(y + 11);
    packet->p[3].xy.x = (sint16)(x + 7);
    packet->p[3].xy.y = (sint16)(y + 11);
    val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
    packet->p[1].attr = (uint16)(val);
    attribute = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
    opcode = packet->rgbc.code;
    packet->rgbc.r = (uint8)(r);
    packet->rgbc.g = (uint8)(g);
    packet->rgbc.b = (uint8)(b);
    packet->rgbc.code = (uint8)(opcode & 0xFC);
    cur_frame = disp_state.cur;
    packet->p[0].attr = (uint16)(attribute);
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    tag = packet->tag;
    link = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(8)) / 4];
    packet->tag = (tag & 0xFF000000) | (link & 0xFFFFFF);
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    tag = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(8)) / 4];
    hd_ui_flat(packet->p, &packet->rgbc, 2);
    val = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
    ordering.tags[(sint32)(ordering.byte_offset + (uint32)(8)) / 4] = val;
    return val;
}

static void draw_route_symbol(uint32 item, uint32 y, uint32 view, uint32 shade)
{
    FUNCTION_MARKER(0x80042DB0, "SLUS_006.10");
}

static uint32 update_route_hint(CAR_STATE *car, uint32 view)
{
    uint16 wire_halfword_1;

    uint32 node;
    uint32 cur;
    uint32 val;
    const uint8 *data;
    uint32 idx;
    sint32 prev;
    sint32 count;

    FUNCTION_MARKER(0x80042DB8, "SLUS_006.10");
    node = car->track_location.n_node;
    prev = (sint32)route_views[view].prev_node;
    cur = car->track_location.node;
    if ((sint32)node < prev)
        route_views[view].reverse = 1;
    else if (prev < (sint32)node)
        route_views[view].reverse = 0;
    if (node != cur)
        node |= 0x8000;
    if (route_views[view].reverse != 0)
        return 0xFFFFFFFF;
    if (route_views[view].hold_ticks == 0)
    {
        val = route_views[view].opacity;
        if ((sint32)val > 0)
            route_views[view].opacity = val - 32;
        if (route_views[view].hold_ticks == 0 && route_views[view].opacity == 0)
        {
            count = (sint32)route_count;
            if (count <= 0)
                return 0xFFFFFFFF;
            data = route_entries;
            for (idx = 0; (sint32)idx < count; ++idx)
            {
                if ((memcpy(&wire_halfword_1, data, sizeof(wire_halfword_1)), wire_halfword_1) == node)
                {
                    route_views[view].sel_entry = idx;
                    route_views[view].hold_ticks = 120;
                    route_views[view].opacity = 224;
                    return 0;
                }
                data += 6;
            }
            return 0xFFFFFFFF;
        }
    }
    val = route_views[view].hold_ticks;
    if ((sint32)val > 0)
        route_views[view].hold_ticks = val - 1;
    return 0;
}

static uint32 draw_route_hints(CAR_STATE *car, uint32 view)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    const uint8 *data;
    uint32 result;
    uint32 row;
    const uint8 *entry;
    uint32 count;
    uint32 y;
    uint32 item;
    uint32 shade;
    sint32 i;

    FUNCTION_MARKER(0x80042F44, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    result = level_header[0x1E] & 0x10;
    if (result == 0)
        return result;
    result = update_route_hint(car, view);
    route_views[view].prev_node = (uint32)(sint16)car->track_location.node;
    if (result == 0xFFFFFFFF)
        return result;
    row = route_views[view].sel_entry;
    route_views[view].draw_idx = 0;
    data = route_entries;
    entry = data + row * 6;
    count = 0;
    for (i = 0; i < 4; ++i)
    {
        if (entry[(uint32)i + 2] != 0)
            count += 12;
    }
    y = model_render.height;
    y = (uint32)((sint32)y / 2) + count - 32;
    for (i = 3; i >= 0; --i)
    {
        result = route_views[view].sel_entry;
        data = route_entries;
        item = data[result * 6 + (uint32)i + 2];
        if (item != 0)
        {
            shade = (uint8)route_views[view].opacity;
            draw_route_symbol(item - 1, y, view, shade);
            y -= 24;
        }
    }
    return result;
}

static void countdown_cue(sint32 val)
{
    FUNCTION_MARKER(0x80053028, "SLUS_006.10");
}

static sint32 play_race_xa(sint32 track)
{
    SpuCommonAttr sound = {0};
    sint32 result = audio_play_xa(track);

    // retain the former race voice gain through the CD mixer
    sound.mask = SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;
    sound.cd.volume.left = sound.cd.volume.right = 0x7FFF;
    SpuSetCommonAttr(&sound);
    return result;
}

const uint8 *hud_start_countdown(void)
{
    const uint8 *tex;

    FUNCTION_MARKER(0x80047468, "SLUS_006.10");
    hud_countdown_frames[0] = 160;
    hud_countdown_frames[1] = 160;
    race_control.players[0].finished = (uint32)(0);
    race_control.players[1].finished = (uint32)(0);
    race_control.players[0].finish_frames = (uint32)(0);
    race_control.players[1].finish_frames = (uint32)(0);
    tex = tex_find("SpFont2");
    digit_tex = tex;
    countdown_aux = 0;
    return tex;
}

uint32 hud_update_countdown(void)
{
    uint32 view;
    uint32 offset;
    uint32 val;
    uint32 r;
    uint32 g;
    sint32 counter;

    FUNCTION_MARKER(0x80047790, "SLUS_006.10");
    view = race_view.active_car;
    offset = view * 4;
    counter = (sint32)hud_countdown_frames[view];
    if (counter == 0)
        return offset;
    val = race_session.drag_race;
    if (val != 0)
        return val;
    if (counter % 40 == 0 && view == 0)
        countdown_cue((sint32)hud_countdown_frames[0] / 40);
    view = race_view.active_car;
    offset = view * 4;
    val = hud_countdown_frames[view] - 1;
    hud_countdown_frames[view] = val;
    if ((sint32)val / 40 == 0)
    {
        r = 32;
        g = 128;
    }
    else
    {
        r = 128;
        g = 32;
    }
    view = race_view.active_car;
    counter = (sint32)hud_countdown_frames[view];
    hud_draw_digit(0, counter / 40, r, g, 32);
    view = race_view.active_car;
    offset = view * 4;
    if ((sint32)hud_countdown_frames[view] >= 40)
        return 0;
    val = hud_countdown_frames[view];
    race_control.start_hold = 0;
    race_control.timing_hold = 0;
    if (val != 0 || view != 0)
        return val;
    val = race_session.linear_course;
    if (val != 0)
        return val;
    val = race_session.two_players;
    if (val != 0)
        return val;
    // rotate full race tracks independently of the selected circuit
    return (uint32)play_race_xa((sint32)((audio_state.race_song_counter - 1) % 8) + 8);
}

uint32 hud_init_drag_start(void)
{
    const uint8 *tex;

    FUNCTION_MARKER(0x8003A384, "SLUS_006.10");
    tex = tex_find("DrgLite2");
    drag_start.panel_tex = tex;
    tex = tex_find("DrgLite1");
    drag_start.lamp_tex = tex;
    tex = tex_find("Drglite3");
    drag_start.active_tex = tex;
    drag_start.ticks = 120;
    drag_start.phase = 0;
    {
        sint32 operation_ok = !(!gpu_register_packet_range(drag_packets, (uint32)sizeof(drag_packets)));
        XASSERT(operation_ok);
    }
    return 120;
}

static void set_drag_start_uv_tail(POLY_FT4 *packet, const uint8 *const *tex_handle, uint32 horizontal_trim)
{
    uint32 u, v;
    const uint8 *tex;

    tex = *tex_handle;
    packet->p[0].uv.v = (uint8)(tex[0xF]);
    tex = *tex_handle;
    u = tex[0xE];
    v = tex[0xC];
    packet->p[1].uv.u = (uint8)(v + u - horizontal_trim);
    tex = *tex_handle;
    packet->p[1].uv.v = (uint8)(tex[0xF]);
    tex = *tex_handle;
    packet->p[2].uv.u = (uint8)(tex[0xE]);
    tex = *tex_handle;
    u = tex[0xF];
    v = tex[0xD];
    packet->p[2].uv.v = (uint8)(v + u - 1);
    tex = *tex_handle;
    u = tex[0xE];
    v = tex[0xC];
    packet->p[3].uv.u = (uint8)(v + u - horizontal_trim);
    tex = *tex_handle;
    u = tex[0xF];
    v = tex[0xD];
    packet->p[3].uv.v = (uint8)(v + u - 1);
}

uint32 hud_draw_drag_start(void)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3;

    static const sint32 full_y[4] = {-4, 34, 72, 110};
    static const sint32 split_y[4] = {-2, 17, 36, 55};
    uint32 slot, x, y, width, height, val;
    const uint8 *tex;
    const CAR_PHYSICS_PROFILE *profile;
    POLY_FT4 *packet;
    const uint8 *color;
    uint32 packet_idx, tag, link;
    uint32 i, mode, left, right_bias, state;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8003A3E4, "SLUS_006.10");
    if (drag_start.phase == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    slot = disp_state.bank;
    x = race_view.viewport_x;
    tex = drag_start.panel_tex;
    drag_start.origin_x = x + 8;
    height = tex[0xD];
    val = race_view.viewport_height;
    packet = &drag_packets[slot][0];
    y = race_view.viewport_y;
    drag_start.origin_y = y + (uint32)((sint32)(val - height) / 2);
    packet->tag = (packet->tag & 0x00FFFFFFu) | ((uint32)(uint8)(9) << 24);
    packet->rgbc.code = (uint8)(0x2D);
    tex = drag_start.panel_tex;
    x = drag_start.origin_x;
    y = drag_start.origin_y;
    packet->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
    val = (memcpy(&wire_halfword_1, tex + 0xA, sizeof(wire_halfword_1)), wire_halfword_1);
    packet->p[0].xy.x = (sint16)(x);
    packet->p[0].xy.y = (sint16)(y);
    packet->p[0].attr = (uint16)(val);
    width = tex[0xC];
    packet->p[1].xy.y = (sint16)(y);
    packet->p[2].xy.x = (sint16)(x);
    packet->p[1].xy.x = (sint16)(x + width - 1);
    packet->p[2].xy.y = (sint16)(y + tex[0xD] - 1);
    packet->p[3].xy.x = (sint16)(x + tex[0xC] - 1);
    packet->p[3].xy.y = (sint16)(y + tex[0xD] - 1);
    packet->p[0].uv.u = (uint8)(tex[0xE]);
    set_drag_start_uv_tail(packet, &drag_start.panel_tex, 1);
    ++packet;
    for (i = 0; i < 5; ++i)
    {
        packet->tag = (packet->tag & 0x00FFFFFFu) | ((uint32)(uint8)(9) << 24);
        packet->rgbc.code = (uint8)(0x2C);
        tex = drag_start.lamp_tex;
        packet->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
        val = (memcpy(&wire_halfword_2, tex + 0xA, sizeof(wire_halfword_2)), wire_halfword_2);
        color = drag_colors[i];
        packet->p[0].attr = (uint16)(val);
        packet->rgbc.r = (uint8)(color[0]);
        packet->rgbc.g = (uint8)(color[1]);
        packet->rgbc.b = (uint8)(color[2]);
        mode = race_session.two_players;
        x = drag_start.origin_x;
        y = drag_start.origin_y;
        tex = drag_start.lamp_tex;
        left = x + (mode == 0 ? 22 : 11);
        y += i * (mode == 0 ? 38 : 19);
        right_bias = mode == 0 ? 20 : 9;
        packet->p[0].xy.x = (sint16)(left);
        packet->p[0].xy.y = (sint16)(y + 3);
        width = tex[0xC];
        packet->p[1].xy.y = (sint16)(y + 3);
        packet->p[2].xy.x = (sint16)(left);
        packet->p[1].xy.x = (sint16)(x + width + right_bias);
        packet->p[2].xy.y = (sint16)(y + tex[0xD] + 2);
        packet->p[3].xy.x = (sint16)(x + tex[0xC] + right_bias);
        packet->p[3].xy.y = (sint16)(y + tex[0xD] + 2);
        tex = drag_start.lamp_tex;
        packet->p[0].uv.u = (uint8)(tex[0xE]);
        set_drag_start_uv_tail(packet, &drag_start.lamp_tex, 2);
        ++packet;
    }
    packet->tag = (packet->tag & 0x00FFFFFFu) | ((uint32)(uint8)(9) << 24);
    packet->rgbc.code = (uint8)(0x2E);
    tex = drag_start.active_tex;
    packet->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
    packet->p[0].attr = (uint16)((memcpy(&wire_halfword_3, tex + 0xA, sizeof(wire_halfword_3)), wire_halfword_3));
    packet->rgbc.r = (uint8)(drag_colors[0][0]);
    packet->rgbc.g = (uint8)(drag_colors[0][1]);
    packet->rgbc.b = (uint8)(drag_colors[0][2]);
    tex = drag_start.active_tex;
    packet->p[0].xy.x = (sint16)(640);
    packet->p[0].xy.y = (sint16)(0);
    width = tex[0xC];
    packet->p[1].xy.y = (sint16)(0);
    packet->p[2].xy.x = (sint16)(640);
    packet->p[1].xy.x = (sint16)(width + 639);
    packet->p[2].xy.y = (sint16)(tex[0xD] - 1);
    packet->p[3].xy.x = (sint16)(tex[0xC] + 639);
    packet->p[3].xy.y = (sint16)(tex[0xD] - 1);
    packet->p[0].uv.u = (uint8)(tex[0xE]);
    set_drag_start_uv_tail(packet, &drag_start.active_tex, 1);
    if (drag_start.contact1_latched == 0)
    {
        for (i = 0; i < 4; ++i)
            if (car_states[0].track_contacts[i].lane >= 2)
                drag_start.contact0_latched = 1;
    }
    if (drag_start.contact0_latched == 0)
    {
        for (i = 0; i < 4; ++i)
            if (car_states[1].track_contacts[i].lane < 2)
                drag_start.contact1_latched = 1;
    }
    if (race_view.active_car == 0)
    {
        val = drag_start.ticks - 1;
        drag_start.ticks = val;
        if (val == 90)
            drag_start.phase = 1;
        if (val == 75)
            drag_start.phase = 2;
        if (val == 60)
            drag_start.phase = 3;
        if (val == 30)
            drag_start.phase = 4;
        if (val == 0)
            drag_start.phase = 0xFFFFFFFF;
    }
    slot = disp_state.bank;
    state = drag_start.phase;
    packet = &drag_packets[slot][6];
    if (state - 1 < 4)
    {
        mode = race_session.two_players;
        color = drag_colors[state - 1];
        x = drag_start.origin_x;
        y = drag_start.origin_y;
        tex = drag_start.active_tex;
        left = x + (mode == 0 ? 10 : 5);
        right_bias = mode == 0 ? 9 : 4;
        val = (mode == 0 ? full_y[state - 1] : split_y[state - 1]);
        packet->p[0].xy.x = (sint16)(left);
        packet->p[0].xy.y = (sint16)(y + val);
        width = tex[0xC];
        packet->p[1].xy.y = (sint16)(y + val);
        packet->p[2].xy.x = (sint16)(left);
        packet->p[1].xy.x = (sint16)(x + width + right_bias);
        packet->p[2].xy.y = (sint16)(y + tex[0xD] + val - 1);
        packet->p[3].xy.x = (sint16)(x + tex[0xC] + right_bias);
        packet->p[3].xy.y = (sint16)(y + tex[0xD] + val - 1);
        packet->rgbc.r = (uint8)(color[0]);
        packet->rgbc.g = (uint8)(color[1]);
        packet->rgbc.b = (uint8)(color[2]);
        if (state == 4)
        {
            mode = race_session.two_players;
            race_control.motion_gate_state = 0xFFFFFFFF;
            race_control.timing_hold = 0;
            race_control.start_hold = 0;
            if (mode == 0)
            {
                profile = car_states[1].physics_profile;
                val = (uint32)(profile->rpm_limit * 2 / 3);
                car_states[1].engine_speed = val;
            }
        }
    }
    cur_frame = disp_state.cur;
    slot = disp_state.bank;
    packet_idx = 6;
    for (i = 0; i < 7; ++i)
    {
        packet = &drag_packets[slot][packet_idx];
        tag = packet->tag;
        link = cur_frame->ordering[0];
        packet->tag = (tag & 0xFF000000) | (link & 0xFFFFFF);
        tag = cur_frame->ordering[0];
        hd_ui_flat(packet->p, &packet->rgbc, 0);
        cur_frame->ordering[0] = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)packet);
        --packet_idx;
    }
    return 0;
}

static uint32 draw_checkpoint_times(CAR_STATE *car)
{
    char text[64];
    uint32 idx;
    uint32 ticks;
    uint32 name;
    sint32 rate;
    sint32 time;
    sint32 remainder;

    FUNCTION_MARKER(0x80038D5C, "SLUS_006.10");
    ticks = car->race_stats.elapsed_ticks;
    rate = (sint32)disp_state.field_rate;
    time = fixed_divide_checked((sint32)(ticks * 100), rate / 2);
    remainder = time % 6000;
    sprintf(text, "%02d:%02d:%02d", time / 6000, remainder / 100, remainder % 100);
    hud_draw_race_text(text, 400, 32, 0);
    hud_draw_race_text("TIME", 356, 32, 0);
    for (idx = 0; idx < 5; ++idx)
    {
        ticks = car->split_ticks[idx];
        if (ticks != 0)
        {
            rate = (sint32)disp_state.field_rate;
            time = fixed_divide_checked((sint32)(ticks * 100), rate / 2);
            remainder = time % 6000;
            sprintf(text, "%02d:%02d:%02d", time / 6000, remainder / 100, remainder % 100);
            hud_draw_race_text(text, 400, 49 + idx * 13, 0);
        }
    }
    name = car->place;
    return hud_draw_race_text(((uint32)(name) < 6 ? session_place_names[name] : ""), 32, 32, 0);
}

uint32 hud_init_gauges(void)
{
    uint32 buffer;
    uint32 idx;
    const uint8 *tex;
    POLY_FT4 *poly;
    uint32 val;
    uint32 other;
    uint32 code;
    uint32 clut;
    const char *name;
    uint32 left;
    uint32 right;

    FUNCTION_MARKER(0x80036778, "SLUS_006.10");
    reverse_views[0].reversing_frames = (uint32)(0);
    reverse_views[1].reversing_frames = (uint32)(0);
    for (buffer = 0; buffer < 2; ++buffer)
    {
        minimap_init();
        tex = tex_find("UTurnL");
        poly = &gauge_packets.reverse_turn[buffer];
        gauge_tex.reverse_turn = tex;
        setPolyFT4(poly);
        poly->p[0].uv.u = (uint8)(tex[14]);
        poly->p[0].uv.v = (uint8)(tex[15]);
        val = tex[14];
        other = tex[12];
        poly->p[1].uv.u = (uint8)(other + val - 1);
        poly->p[1].uv.v = (uint8)(tex[15]);
        poly->p[2].uv.u = (uint8)(tex[14]);
        val = tex[15];
        other = tex[13];
        poly->p[2].uv.v = (uint8)(other + val - 1);
        val = tex[14];
        other = tex[12];
        poly->p[3].uv.u = (uint8)(other + val - 1);
        val = tex[15];
        other = tex[13];
        poly->p[3].uv.v = (uint8)(other + val - 1);
        val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        code = poly->rgbc.code;
        poly->p[1].attr = (uint16)(val);
        clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        poly->rgbc.code = (uint8)(code | 3);
        poly->p[0].attr = (uint16)(clut);
        for (idx = 0; idx < 2; ++idx)
        {
            POLY_F3 *needle_poly;
            needle_poly = &needle_packets.needle[idx][buffer];
            needle_poly->rgbc.code = (uint8)(0x20);
            code = needle_poly->rgbc.code;
            setlen(needle_poly, 4);
            needle_poly->rgbc.r = (uint8)(255);
            needle_poly->rgbc.g = (uint8)(255);
            needle_poly->rgbc.b = (uint8)(0);
            needle_poly->rgbc.code = (uint8)(code & 0xFD);
            needle_poly = &needle_packets.shadow[idx][buffer];
            needle_poly->rgbc.code = (uint8)(0x20);
            code = needle_poly->rgbc.code;
            setlen(needle_poly, 4);
            needle_poly->rgbc.r = (uint8)(1);
            needle_poly->rgbc.g = (uint8)(1);
            needle_poly->rgbc.b = (uint8)(1);
            needle_poly->rgbc.code = (uint8)(code & 0xFD);
        }
        for (idx = 0; idx < 3; ++idx)
        {
            poly = &gauge_packets.numbers[idx][buffer];
            tex = tex_find("Numbers");
            gauge_tex.numbers = tex;
            setPolyFT4(poly);
            poly->p[0].uv.u = (uint8)(tex[14]);
            poly->p[0].uv.v = (uint8)(tex[15]);
            poly->p[1].uv.u = (uint8)(tex[14] + 18);
            poly->p[1].uv.v = (uint8)(tex[15]);
            poly->p[2].uv.u = (uint8)(tex[14]);
            val = tex[15];
            other = tex[13];
            poly->p[2].uv.v = (uint8)(other + val - 1);
            poly->p[3].uv.u = (uint8)(tex[14] + 18);
            val = tex[15];
            other = tex[13];
            left = 19 * (2 - idx) + 350;
            right = 19 * (2 - idx) + 368;
            poly->p[0].xy.x = (sint16)(uint16)(left);
            poly->p[0].xy.y = (sint16)(uint16)(200);
            poly->p[1].xy.x = (sint16)(uint16)(right);
            poly->p[1].xy.y = (sint16)(uint16)(200);
            poly->p[2].xy.x = (sint16)(uint16)(left);
            poly->p[3].uv.v = (uint8)(other + val - 1);
            val = tex[13];
            poly->p[3].xy.x = (sint16)(uint16)(right);
            poly->p[2].xy.y = (sint16)(uint16)(val + 199);
            poly->p[3].xy.y = (sint16)(uint16)(tex[13] + 199);
            poly->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
            poly->p[0].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->clut);
            code = poly->rgbc.code;
            poly->rgbc.code = (uint8)(code | 3);
        }
        name = game_options.speed_kph != 0 ? "Kph" : "Mph";
        tex = tex_find(name);
        gauge_tex.speed_units = tex;
        poly = &gauge_packets.speed_units[buffer];
        setPolyFT4(poly);
        poly->p[0].uv.u = (uint8)(tex[14]);
        poly->p[0].uv.v = (uint8)(tex[15]);
        val = tex[14];
        other = tex[12];
        poly->p[1].uv.u = (uint8)(other + val - 1);
        poly->p[1].uv.v = (uint8)(tex[15]);
        poly->p[2].uv.u = (uint8)(tex[14]);
        val = tex[15];
        other = tex[13];
        poly->p[2].uv.v = (uint8)(other + val - 1);
        val = tex[14];
        other = tex[12];
        poly->p[3].uv.u = (uint8)(other + val - 1);
        val = tex[15];
        other = tex[13];
        poly->p[0].xy.x = (sint16)(uint16)(409);
        poly->p[0].xy.y = (sint16)(uint16)(212);
        poly->p[3].uv.v = (uint8)(other + val - 1);
        val = tex[12];
        poly->p[1].xy.y = (sint16)(uint16)(212);
        poly->p[2].xy.x = (sint16)(uint16)(409);
        poly->p[1].xy.x = (sint16)(uint16)(val + 408);
        poly->p[2].xy.y = (sint16)(uint16)(tex[13] + 211);
        poly->p[3].xy.x = (sint16)(uint16)(tex[12] + 408);
        poly->p[3].xy.y = (sint16)(uint16)(tex[13] + 211);
        poly->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
        code = poly->rgbc.code;
        clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        poly->rgbc.code = (uint8)(code | 3);
        poly->p[0].attr = (uint16)(clut);
        tex = tex_find("gears");
        poly = &gauge_packets.gears[buffer];
        gauge_tex.gears = tex;
        setPolyFT4(poly);
        poly->p[0].uv.u = (uint8)(tex[14]);
        poly->p[0].uv.v = (uint8)(tex[15]);
        poly->p[1].uv.u = (uint8)(tex[14] + 19);
        poly->p[1].uv.v = (uint8)(tex[15]);
        poly->p[2].uv.u = (uint8)(tex[14]);
        val = tex[15];
        other = tex[13];
        poly->p[2].uv.v = (uint8)(other + val - 1);
        poly->p[3].uv.u = (uint8)(tex[14] + 19);
        val = tex[15];
        other = tex[13];
        poly->p[0].xy.x = (sint16)(uint16)(409);
        poly->p[0].xy.y = (sint16)(uint16)(190);
        poly->p[1].xy.x = (sint16)(uint16)(428);
        poly->p[1].xy.y = (sint16)(uint16)(190);
        poly->p[2].xy.x = (sint16)(uint16)(409);
        poly->p[3].uv.v = (uint8)(other + val - 1);
        val = tex[13];
        poly->p[3].xy.x = (sint16)(uint16)(428);
        poly->p[2].xy.y = (sint16)(uint16)(val + 189);
        poly->p[3].xy.y = (sint16)(uint16)(tex[13] + 189);
        poly->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
        poly->p[0].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->clut);
        code = poly->rgbc.code;
        poly->rgbc.code = (uint8)(code | 3);
        name = race_session.two_players != 0 ? "SmSpdo1" : "Speedo1";
        tex = tex_find(name);
        gauge_tex.speedometer = tex;
        poly = &gauge_packets.speedometer[buffer];
        setPolyFT4(poly);
        poly->p[0].uv.u = (uint8)(tex[14]);
        poly->p[0].uv.v = (uint8)(tex[15]);
        val = tex[14];
        other = tex[12];
        poly->p[1].uv.u = (uint8)(other + val - 1);
        poly->p[1].uv.v = (uint8)(tex[15]);
        poly->p[2].uv.u = (uint8)(tex[14]);
        val = tex[15];
        other = tex[13];
        poly->p[2].uv.v = (uint8)(other + val - 1);
        val = tex[14];
        other = tex[12];
        poly->p[3].uv.u = (uint8)(other + val - 1);
        val = tex[15];
        other = tex[13];
        poly->p[0].xy.x = (sint16)(uint16)(380);
        poly->p[0].xy.y = (sint16)(uint16)(150);
        poly->p[3].uv.v = (uint8)(other + val - 1);
        val = tex[12];
        poly->p[1].xy.y = (sint16)(uint16)(150);
        poly->p[2].xy.x = (sint16)(uint16)(380);
        poly->p[1].xy.x = (sint16)(uint16)(val + 379);
        poly->p[2].xy.y = (sint16)(uint16)(tex[13] + 149);
        poly->p[3].xy.x = (sint16)(uint16)(tex[12] + 379);
        poly->p[3].xy.y = (sint16)(uint16)(tex[13] + 149);
        poly->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
        code = poly->rgbc.code;
        clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        poly->rgbc.code = (uint8)(code | 3);
        poly->p[0].attr = (uint16)(clut);
        tex = tex_find("SpFont2");
        gauge_tex.checkpoint_font = tex;
        for (idx = 0; idx < 3; ++idx)
        {
            poly = &gauge_packets.checkpoint[buffer][idx];
            setPolyFT4(poly);
            poly->p[0].uv.u = (uint8)(gauge_tex.checkpoint_font[14]);
            poly->p[0].uv.v = (uint8)(gauge_tex.checkpoint_font[15]);
            poly->p[1].uv.u = (uint8)(gauge_tex.checkpoint_font[14] + 15);
            poly->p[1].uv.v = (uint8)(gauge_tex.checkpoint_font[15]);
            poly->p[2].uv.u = (uint8)(gauge_tex.checkpoint_font[14]);
            poly->p[2].uv.v = (uint8)(gauge_tex.checkpoint_font[15] + 23);
            poly->p[3].uv.u = (uint8)(gauge_tex.checkpoint_font[14] + 15);
            val = gauge_tex.checkpoint_font[15];
            poly->p[0].xy.x = (sint16)(uint16)(232 + idx * 16);
            poly->p[0].xy.y = (sint16)(uint16)(24);
            poly->p[1].xy.x = (sint16)(uint16)(247 + idx * 16);
            poly->p[1].xy.y = (sint16)(uint16)(24);
            poly->p[2].xy.x = (sint16)(uint16)(232 + idx * 16);
            poly->p[2].xy.y = (sint16)(uint16)(47);
            poly->p[3].xy.x = (sint16)(uint16)(247 + idx * 16);
            poly->p[3].uv.v = (uint8)(val + 23);
            poly->p[3].xy.y = (sint16)(uint16)(47);
            tex = gauge_tex.checkpoint_font;
            code = poly->rgbc.code;
            val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
            poly->p[1].attr = (uint16)(val);
            clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
            poly->rgbc.code = (uint8)((code | 1) & 0xFD);
            poly->p[0].attr = (uint16)(clut);
        }
    }
    return 0;
}

static uint32 draw_large_gauges(CAR_STATE *car)
{
    DISPLAY_ORDERING ordering;
    const CAR_PHYSICS_PROFILE *model;
    uint32 val;
    uint32 angle;
    uint32 offset;
    uint32 next_idx;
    POLY_F3 *needle_poly;
    POLY_F3 *shadow_poly;
    uint32 radius;
    uint32 pass;
    uint32 y0;
    uint32 y1;
    uint32 y2;

    uint32 tag;
    uint32 link;
    const uint8 *tex;
    uint32 gear;
    uint32 u;
    uint32 v;
    uint32 height;
    POLY_FT4 *poly;
    uint32 pos;
    uint32 digit_offset;
    sint32 speed;
    sint32 quotient;
    sint32 digit;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x800374B8, "SLUS_006.10");
    model = car->physics_profile;
    val = car->engine_speed;
    speed = model->rpm_limit;
    angle = (uint32)fixed_divide_checked((sint32)(val * 3072), speed);
    offset = disp_state.bank * 20;
    needle_poly = &needle_packets.needle[0][offset / 20];
    shadow_poly = &needle_packets.shadow[0][offset / 20];
    for (pass = 0; pass < 2; ++pass)
    {
        radius = pass == 0 ? 56 : 28;
        val = (uint32)rcos((sint32)angle) * radius;
        val = 436 - (uint32)((sint32)val / 4096);
        needle_poly->p[0].x = (sint16)(uint16)(val);
        shadow_poly->p[0].x = (sint16)(uint16)(val);
        val = (uint32)rsin((sint32)angle) * radius;
        val = 187 - (uint32)((sint32)val / 6144);
        needle_poly->p[0].y = (sint16)(uint16)(val);
        shadow_poly->p[0].y = (sint16)(uint16)(val);
        val = (uint32)rsin((sint32)angle) * 3;
        val = (uint32)((sint32)val / 4096) + 436;
        needle_poly->p[1].x = (sint16)(uint16)(val);
        shadow_poly->p[1].x = (sint16)(uint16)(val);
        val = (uint32)rcos((sint32)angle);
        val = 187 - (uint32)((sint32)val / 2048);
        needle_poly->p[1].y = (sint16)(uint16)(val);
        shadow_poly->p[1].y = (sint16)(uint16)(val);
        val = (uint32)rsin((sint32)angle) * 3;
        val = 436 - (uint32)((sint32)val / 4096);
        needle_poly->p[2].x = (sint16)(uint16)(val);
        shadow_poly->p[2].x = (sint16)(uint16)(val);
        val = (uint32)rcos((sint32)angle);
        val = (uint32)((sint32)val / 2048) + 187;
        needle_poly->p[2].y = (sint16)(uint16)(val);
        shadow_poly->p[2].y = (sint16)(uint16)(val);
        y0 = (uint16)(shadow_poly->p[0].y);
        y1 = (uint16)(shadow_poly->p[1].y);
        y2 = (uint16)(shadow_poly->p[2].y);
        cur_frame = disp_state.cur;
        shadow_poly->p[0].y = (sint16)(uint16)(y0 + 2);
        shadow_poly->p[1].y = (sint16)(uint16)(y1 + 2);
        shadow_poly->p[2].y = (sint16)(uint16)(y2 + 2);
        hd_gauge_needle(car, pass, 436.0f, 187.0f, (float)radius, 3.0f, 2.0f / 3.0f, &needle_poly->rgbc, 0);
        disp_link_front_packet(cur_frame, &needle_poly->tag);
        ordering.tags = cur_frame->ordering;
        ordering.byte_offset = 0;
        if (pass == 0)
        {
            angle += 2048;
            next_idx = disp_state.bank;
        }
        tag = shadow_poly->tag;
        link = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(4)) / 4];
        shadow_poly->tag = (uint32)((tag & 0xFF000000) | (link & 0xFFFFFF));
        ordering.tags = cur_frame->ordering;
        ordering.byte_offset = 0;
        tag = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(4)) / 4];
        if (pass == 1)
            next_idx = disp_state.bank;
        hd_gauge_needle(car, pass, 436.0f, 189.0f, (float)radius, 3.0f, 2.0f / 3.0f, &shadow_poly->rgbc, 1);
        ordering.tags[(sint32)(ordering.byte_offset + (uint32)(4)) / 4] = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)shadow_poly);
        offset = next_idx * 20;
        needle_poly = &needle_packets.needle[1][offset / 20];
        shadow_poly = &needle_packets.shadow[1][offset / 20];
    }
    tex = gauge_tex.gears;
    gear = car->gear;
    u = tex[14];
    gauge_packets.gears[next_idx].p[0].uv.u = (uint8)(u + gear * 20);
    tex = gauge_tex.gears;
    offset = disp_state.bank * 40;
    v = tex[15];
    gauge_packets.gears[offset / 40].p[0].uv.v = (uint8)(v);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    gear = car->gear;
    u = tex[14];
    gauge_packets.gears[offset / 40].p[1].uv.u = (uint8)(u + gear * 20 + 19);
    tex = gauge_tex.gears;
    offset = disp_state.bank * 40;
    v = tex[15];
    gauge_packets.gears[offset / 40].p[1].uv.v = (uint8)(v);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    gear = car->gear;
    u = tex[14];
    gauge_packets.gears[offset / 40].p[2].uv.u = (uint8)(u + gear * 20);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    v = tex[15];
    height = tex[13];
    gauge_packets.gears[offset / 40].p[2].uv.v = (uint8)(v + height + 255);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    gear = car->gear;
    u = tex[14];
    gauge_packets.gears[offset / 40].p[3].uv.u = (uint8)(u + gear * 20 + 19);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    v = tex[15];
    height = tex[13];
    gauge_packets.gears[offset / 40].p[3].uv.v = (uint8)(v + height + 255);
    offset = disp_state.bank * 40;
    cur_frame = disp_state.cur;
    hd_ui_flat(gauge_packets.gears[offset / 40].p, &gauge_packets.gears[offset / 40].rgbc, 0);
    disp_link_front_packet(cur_frame, &gauge_packets.gears[offset / 40].tag);
    speed = (sint32)car->forward_speed >> 8;
    if (speed < 0)
        speed = 0;
    if (game_options.speed_kph != 0)
    {
        val = ((uint32)speed << 8) + 389;
        speed = (sint32)val / 778;
    }
    else
    {
        val = ((uint32)speed << 8) + 626;
        speed = (sint32)val / 1252;
    }
    pos = 0;
    digit_offset = 0;
    do
    {
        quotient = speed / 10;
        digit = speed % 10;
        offset = disp_state.bank * 40;
        poly = &gauge_packets.numbers[digit_offset / 80][offset / 40];
        if (digit != 0 || quotient != 0 || pos == 0)
        {
            tex = gauge_tex.numbers;
            u = tex[14];
            offset = (uint32)digit * 18;
            poly->p[0].uv.u = (uint8)(u + offset);
            tex = gauge_tex.numbers;
            v = tex[15];
            poly->p[0].uv.v = (uint8)(v);
            tex = gauge_tex.numbers;
            u = tex[14];
            poly->p[1].uv.u = (uint8)(u + offset + 18);
            tex = gauge_tex.numbers;
            v = tex[15];
            poly->p[1].uv.v = (uint8)(v);
            tex = gauge_tex.numbers;
            u = tex[14];
            poly->p[2].uv.u = (uint8)(u + offset);
            tex = gauge_tex.numbers;
            v = tex[15];
            height = tex[13];
            poly->p[2].uv.v = (uint8)(v + height);
            tex = gauge_tex.numbers;
            u = tex[14];
            poly->p[3].uv.u = (uint8)(u + offset + 18);
            tex = gauge_tex.numbers;
            v = tex[15];
            height = tex[13];
            poly->p[3].uv.v = (uint8)(v + height);
            cur_frame = disp_state.cur;
            hd_ui_flat(poly->p, &poly->rgbc, 0);
            disp_link_front_packet(cur_frame, &poly->tag);
        }
        ++pos;
        speed = quotient;
        digit_offset += 80;
    } while (pos < 3);
    offset = disp_state.bank * 40;
    cur_frame = disp_state.cur;
    hd_ui_flat(gauge_packets.speed_units[offset / 40].p, &gauge_packets.speed_units[offset / 40].rgbc, 0);
    disp_link_front_packet(cur_frame, &gauge_packets.speed_units[offset / 40].tag);
    hd_ui_flat(gauge_packets.speedometer[offset / 40].p, &gauge_packets.speedometer[offset / 40].rgbc, 0);
    return disp_link_front_packet(cur_frame, &gauge_packets.speedometer[offset / 40].tag);
}

static uint32 draw_small_gauges(CAR_STATE *car, uint32 parameter)
{
    DISPLAY_ORDERING ordering;
    uint32 val, angle, offset, next_idx, pass;
    POLY_F3 *shadow_poly;
    POLY_F3 *needle_poly;
    const CAR_PHYSICS_PROFILE *model;
    uint32 y0, y1, y2, tag, link, gear, u, v, height;
    const uint8 *tex;
    uint32 pos, digit_offset, base_x, base_y, center_x, center_y, origin_x, origin_y;
    POLY_FT4 *poly;
    sint32 speed, quotient, digit;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80037E04, "SLUS_006.10");
    model = car->physics_profile;
    val = car->engine_speed;
    speed = model->rpm_limit;
    angle = (uint32)fixed_divide_checked((sint32)(val * 3072), speed);
    offset = disp_state.bank * 20;
    origin_x = race_view.viewport_width;
    origin_y = race_view.viewport_height;
    needle_poly = &needle_packets.needle[0][offset / 20];
    shadow_poly = &needle_packets.shadow[0][offset / 20];
    val = race_view.viewport_x;
    height = race_view.viewport_y;
    origin_x += val;
    origin_y += height;
    base_x = origin_x - 64;
    base_y = origin_y - 72;
    center_x = origin_x - 40;
    center_y = origin_y - 48;
    for (pass = 0; pass < 2; ++pass)
    {
        val = (uint32)rcos((sint32)angle);
        if (pass == 0)
            val = center_x - (uint32)((sint32)(val * 24) / 4096);
        else
            val = center_x - (uint32)((sint32)val / 512);
        needle_poly->p[0].x = (sint16)(uint16)(val);
        shadow_poly->p[0].x = (sint16)(uint16)(val);
        val = (uint32)rsin((sint32)angle);
        if (pass == 0)
            val = center_y - (uint32)((sint32)(val * 24) / 4096);
        else
            val = center_y - (uint32)((sint32)val / 512);
        needle_poly->p[0].y = (sint16)(uint16)(val);
        shadow_poly->p[0].y = (sint16)(uint16)(val);
        val = (uint32)rsin((sint32)angle);
        val = center_x + (uint32)((sint32)val / 2048);
        needle_poly->p[1].x = (sint16)(uint16)(val);
        shadow_poly->p[1].x = (sint16)(uint16)(val);
        val = (uint32)rcos((sint32)angle);
        val = center_y - (uint32)((sint32)val / 2048);
        needle_poly->p[1].y = (sint16)(uint16)(val);
        shadow_poly->p[1].y = (sint16)(uint16)(val);
        val = (uint32)rsin((sint32)angle);
        val = center_x - (uint32)((sint32)val / 2048);
        needle_poly->p[2].x = (sint16)(uint16)(val);
        shadow_poly->p[2].x = (sint16)(uint16)(val);
        val = (uint32)rcos((sint32)angle);
        val = center_y + (uint32)((sint32)val / 2048);
        needle_poly->p[2].y = (sint16)(uint16)(val);
        shadow_poly->p[2].y = (sint16)(uint16)(val);
        y0 = (uint16)(shadow_poly->p[0].y);
        y1 = (uint16)(shadow_poly->p[1].y);
        y2 = (uint16)(shadow_poly->p[2].y);
        cur_frame = disp_state.cur;
        shadow_poly->p[0].y = (sint16)(uint16)(y0 + 2);
        shadow_poly->p[1].y = (sint16)(uint16)(y1 + 2);
        shadow_poly->p[2].y = (sint16)(uint16)(y2 + 2);
        hd_gauge_needle(car, pass, (float)center_x, (float)center_y, pass == 0 ? 24.0f : 8.0f, 2.0f, 1.0f, &needle_poly->rgbc, 0);
        disp_link_front_packet(cur_frame, &needle_poly->tag);
        ordering.tags = cur_frame->ordering;
        ordering.byte_offset = 0;
        if (pass == 0)
        {
            angle += 2048;
            next_idx = disp_state.bank;
        }
        tag = shadow_poly->tag;
        link = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(4)) / 4];
        shadow_poly->tag = (uint32)((tag & 0xFF000000) | (link & 0xFFFFFF));
        ordering.tags = cur_frame->ordering;
        ordering.byte_offset = 0;
        tag = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(4)) / 4];
        if (pass == 1)
        {
            next_idx = disp_state.bank;
        }
        hd_gauge_needle(car, pass, (float)center_x, (float)center_y + 2.0f, pass == 0 ? 24.0f : 8.0f, 2.0f, 1.0f, &shadow_poly->rgbc, 1);
        ordering.tags[(sint32)(ordering.byte_offset + (uint32)(4)) / 4] = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)shadow_poly);
        offset = next_idx * 20;
        needle_poly = &needle_packets.needle[1][offset / 20];
        shadow_poly = &needle_packets.shadow[1][offset / 20];
    }
    tex = gauge_tex.gears;
    poly = &gauge_packets.gears[next_idx];
    poly->p[0].xy.y = (sint16)(uint16)(base_y + 29);
    poly->p[1].xy.y = (sint16)(uint16)(base_y + 29);
    poly->p[0].xy.x = (sint16)(uint16)(base_x + 10);
    poly->p[1].xy.x = (sint16)(uint16)(base_x + 19);
    poly->p[2].xy.x = (sint16)(uint16)(base_x + 10);
    height = tex[13];
    poly->p[3].xy.x = (sint16)(uint16)(base_x + 19);
    poly->p[2].xy.y = (sint16)(uint16)(base_y + height + 28);
    height = tex[13];
    poly->p[3].xy.y = (sint16)(uint16)(base_y + height + 28);
    gear = car->gear;
    u = tex[14];
    poly->p[0].uv.u = (uint8)(u + gear * 10);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    v = tex[15];
    gauge_packets.gears[offset / 40].p[0].uv.v = (uint8)(v);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    gear = car->gear;
    u = tex[14];
    gauge_packets.gears[offset / 40].p[1].uv.u = (uint8)(u + gear * 10 + 9);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    v = tex[15];
    gauge_packets.gears[offset / 40].p[1].uv.v = (uint8)(v);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    gear = car->gear;
    u = tex[14];
    gauge_packets.gears[offset / 40].p[2].uv.u = (uint8)(u + gear * 10);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    v = tex[15];
    height = tex[13];
    gauge_packets.gears[offset / 40].p[2].uv.v = (uint8)(v + height + 255);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    gear = car->gear;
    u = tex[14];
    gauge_packets.gears[offset / 40].p[3].uv.u = (uint8)(u + gear * 10 + 9);
    offset = disp_state.bank * 40;
    tex = gauge_tex.gears;
    v = tex[15];
    height = tex[13];
    gauge_packets.gears[offset / 40].p[3].uv.v = (uint8)(v + height + 255);
    offset = disp_state.bank * 40;
    cur_frame = disp_state.cur;
    hd_ui_flat(gauge_packets.gears[offset / 40].p, &gauge_packets.gears[offset / 40].rgbc, 0);
    disp_link_front_packet(cur_frame, &gauge_packets.gears[offset / 40].tag);
    speed = (sint32)car->forward_speed >> 8;
    if (speed < 0)
        speed = 0;
    if (game_options.speed_kph != 0)
    {
        val = ((uint32)speed << 8) + 389;
        speed = (sint32)val / 778;
    }
    else
    {
        val = ((uint32)speed << 8) + 626;
        speed = (sint32)val / 1252;
    }
    pos = 0;
    digit_offset = 0;
    do
    {
        quotient = speed / 10;
        digit = speed % 10;
        offset = disp_state.bank * 40;
        poly = &gauge_packets.numbers[digit_offset / 80][offset / 40];
        if (digit != 0 || quotient != 0 || pos == 0)
        {
            tex = gauge_tex.numbers;
            val = base_x + (2 - pos) * 10 - 24;
            poly->p[0].xy.y = (sint16)(uint16)(base_y + 36);
            poly->p[1].xy.y = (sint16)(uint16)(base_y + 36);
            poly->p[0].xy.x = (sint16)(uint16)(val);
            poly->p[1].xy.x = (sint16)(uint16)(val + 9);
            poly->p[2].xy.x = (sint16)(uint16)(val);
            height = tex[13];
            poly->p[3].xy.x = (sint16)(uint16)(val + 9);
            poly->p[2].xy.y = (sint16)(uint16)(base_y + height + 35);
            height = tex[13];
            poly->p[3].xy.y = (sint16)(uint16)(base_y + height + 35);
            u = tex[14];
            offset = (uint32)digit * 9;
            poly->p[0].uv.u = (uint8)(u + offset);
            tex = gauge_tex.numbers;
            v = tex[15];
            poly->p[0].uv.v = (uint8)(v);
            tex = gauge_tex.numbers;
            u = tex[14];
            poly->p[1].uv.u = (uint8)(u + offset + 9);
            tex = gauge_tex.numbers;
            v = tex[15];
            poly->p[1].uv.v = (uint8)(v);
            tex = gauge_tex.numbers;
            u = tex[14];
            poly->p[2].uv.u = (uint8)(u + offset);
            tex = gauge_tex.numbers;
            v = tex[15];
            height = tex[13];
            poly->p[2].uv.v = (uint8)(v + height);
            tex = gauge_tex.numbers;
            u = tex[14];
            poly->p[3].uv.u = (uint8)(u + offset + 9);
            tex = gauge_tex.numbers;
            v = tex[15];
            height = tex[13];
            poly->p[3].uv.v = (uint8)(v + height);
            cur_frame = disp_state.cur;
            hd_ui_flat(poly->p, &poly->rgbc, 0);
            disp_link_front_packet(cur_frame, &poly->tag);
        }
        ++pos;
        speed = quotient;
        digit_offset += 80;
    } while (pos < 3);
    offset = disp_state.bank * 40;
    tex = gauge_tex.speed_units;
    poly = &gauge_packets.speed_units[offset / 40];
    poly->p[0].xy.x = (sint16)(uint16)(base_x + 6);
    poly->p[0].xy.y = (sint16)(uint16)(base_y + 40);
    val = tex[12];
    poly->p[1].xy.y = (sint16)(uint16)(base_y + 40);
    poly->p[2].xy.x = (sint16)(uint16)(base_x + 6);
    poly->p[1].xy.x = (sint16)(uint16)(base_x + val + 5);
    val = tex[13];
    poly->p[2].xy.y = (sint16)(uint16)(base_y + val + 39);
    val = tex[12];
    poly->p[3].xy.x = (sint16)(uint16)(base_x + val + 5);
    val = tex[13];
    cur_frame = disp_state.cur;
    poly->p[3].xy.y = (sint16)(uint16)(base_y + val + 39);
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    tag = poly->tag;
    link = ordering.tags[(sint32)ordering.byte_offset / 4];
    poly->tag = (uint32)((tag & 0xFF000000) | (link & 0xFFFFFF));
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    tag = ordering.tags[(sint32)ordering.byte_offset / 4];
    hd_ui_flat(poly->p, &poly->rgbc, 0);
    val = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
    tex = gauge_tex.speedometer;
    ordering.tags[(sint32)ordering.byte_offset / 4] = val;
    poly = &gauge_packets.speedometer[offset / 40];
    poly->p[0].xy.y = (sint16)(uint16)(base_y);
    poly->p[0].xy.x = (sint16)(uint16)(base_x);
    val = tex[12];
    poly->p[1].xy.y = (sint16)(uint16)(base_y);
    poly->p[2].xy.x = (sint16)(uint16)(base_x);
    poly->p[1].xy.x = (sint16)(uint16)(base_x + val - 1);
    val = tex[13];
    poly->p[2].xy.y = (sint16)(uint16)(base_y + val - 1);
    val = tex[12];
    poly->p[3].xy.x = (sint16)(uint16)(base_x + val - 1);
    val = tex[13];
    poly->p[3].xy.y = (sint16)(uint16)(base_y + val - 1);
    hd_ui_flat(poly->p, &poly->rgbc, 0);
    return disp_link_front_packet(cur_frame, &poly->tag);
}

static uint32 draw_checkpoint_timer(void)
{
    uint32 sel;
    uint32 val;
    uint32 digits[3];
    uint32 pos;
    uint32 corner;
    const uint8 *tex;
    uint32 offset;
    uint32 coord;
    uint8 *uv_component;
    POLY_FT4 *poly;
    sint32 timer;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80036F48, "SLUS_006.10");
    sel = race_view.primary_car;
    val = car_states[sel].checkpoint_timer >> 8;
    digits[0] = val / 100;
    val %= 100;
    digits[1] = val / 10;
    digits[2] = val % 10;
    for (pos = 0; pos < 3; ++pos)
    {
        for (corner = 0; corner < 8; ++corner)
        {
            if (pos == 0 && corner < 7)
            {
                tex = gauge_tex.checkpoint_font;
                offset = disp_state.bank * 120;
            }
            else
            {
                offset = disp_state.bank * 120;
                tex = gauge_tex.checkpoint_font;
            }
            if ((corner & 1) == 0)
            {
                coord = tex[14] + digits[pos] * 16;
                if ((corner & 2) != 0)
                    coord += 15;
            }
            else
            {
                coord = tex[15];
                if (corner >= 5)
                    coord += 23;
            }
            uv_component = (corner & 1) != 0 ? &gauge_packets.checkpoint[offset / 120][pos].p[corner / 2].uv.v : &gauge_packets.checkpoint[offset / 120][pos].p[corner / 2].uv.u;
            *uv_component = (coord);
        }
    }
    timer = (sint16)(hud_checkpoint_disp.blink_frames);
    if (timer == 0 || (hud_checkpoint_disp.blink_phase & 4) == 0)
    {
        cur_frame = disp_state.cur;
        offset = disp_state.bank * 120;
        poly = &gauge_packets.checkpoint[offset / 120][0];
        for (pos = 0; pos < 3; ++pos)
        {
            hd_ui_flat(poly->p, &poly->rgbc, 0);
            disp_link_front_packet(cur_frame, &poly->tag);
            ++poly;
        }
    }
    timer = (sint16)(hud_checkpoint_disp.blink_frames);
    if (timer > 0)
        hud_checkpoint_disp.blink_frames = (uint16)((uint32)timer - 1);
    val = hud_checkpoint_disp.blink_phase + 1;
    hud_checkpoint_disp.blink_phase = (uint8)(val);
    return val;
}

static uint32 draw_reverse_indicator(CAR_STATE *car)
{
    DISPLAY_ORDERING ordering;
    uint32 view;
    uint32 offset;
    uint32 val;
    uint32 angle;
    uint32 idx;
    uint32 sel;
    uint32 draw;
    const uint8 *tex;
    uint32 width;
    uint32 height;
    uint32 tex_width;
    uint32 tex_height;
    uint32 x;
    uint32 y;
    POLY_FT4 *poly;

    uint32 tag;
    uint32 link;
    sint32 node;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8003AF84, "SLUS_006.10");
    view = race_view.active_car;
    node = (sint16)car->track_location.node;
    offset = view * 4;
    if (node < (sint32)(reverse_views[offset / 4].prev_node))
    {
        val = reverse_views[offset / 4].reversing_frames;
        reverse_views[offset / 4].reversing_frames = (uint32)(val + 1);
    }
    view = race_view.active_car;
    offset = view * 4;
    if ((sint32)(reverse_views[offset / 4].prev_node) < node)
        reverse_views[offset / 4].reversing_frames = (uint32)(0);
    view = race_view.active_car;
    offset = view * 4;
    val = reverse_views[offset / 4].blink_frames;
    reverse_views[offset / 4].blink_frames = (uint32)(val + 1);
    idx = car->slot;
    angle = race_ai_heading_error(&race_ai_cars[idx]);
    draw = 1;
    if (race_session.multiplayer_mode == 3)
    {
        idx = car->slot;
        sel = chase_state.pursuer_slot;
        if (sel == idx)
            draw = 0;
    }
    if (draw != 0 && angle - 1024 < 2048)
    {
        view = race_view.active_car;
        offset = view * 4;
        if ((sint32)(reverse_views[offset / 4].reversing_frames) >= 3 && (sint32)(reverse_views[offset / 4].blink_frames) % 32 >= 9)
        {
            tex = gauge_tex.reverse_turn;
            width = race_view.viewport_width;
            tex_width = tex[12];
            x = race_view.viewport_x;
            tex_height = tex[13];
            x += (uint32)((sint32)(width - tex_width) / 2);
            idx = disp_state.bank;
            height = race_view.viewport_height;
            poly = &gauge_packets.reverse_turn[idx];
            y = race_view.viewport_y;
            y += (uint32)((sint32)(height - tex_height) / 2);
            poly->p[0].xy.x = (sint16)(uint16)(x);
            poly->p[0].xy.y = (sint16)(uint16)(y);
            tex_width = tex[12];
            poly->p[1].xy.y = (sint16)(uint16)(y);
            poly->p[2].xy.x = (sint16)(uint16)(x);
            poly->p[1].xy.x = (sint16)(uint16)(tex_width + x);
            tex_height = tex[13];
            poly->p[2].xy.y = (sint16)(uint16)(tex_height + y);
            tex_width = tex[12];
            poly->p[3].xy.x = (sint16)(uint16)(tex_width + x);
            tex_height = tex[13];
            cur_frame = disp_state.cur;
            poly->p[3].xy.y = (sint16)(uint16)(tex_height + y);
            ordering.tags = cur_frame->ordering;
            ordering.byte_offset = 0;
            tag = poly->tag;
            link = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(8)) / 4];
            poly->tag = (uint32)((tag & 0xFF000000) | (link & 0xFFFFFF));
            ordering.tags = cur_frame->ordering;
            ordering.byte_offset = 0;
            tag = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(8)) / 4];
            hd_ui_flat(poly->p, &poly->rgbc, 2);
            ordering.tags[(sint32)(ordering.byte_offset + (uint32)(8)) / 4] = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
        }
    }
    offset = race_view.active_car * 4;
    reverse_views[offset / 4].prev_node = (uint32)((uint32)node);
    return offset;
}

uint32 hud_init_race_text(void)
{
    uint8 *tex_rec;
    uint8 *tex;
    uint32 idx;
    POLY_FT4 *poly;
    uint32 val;
    uint32 other;

    FUNCTION_MARKER(0x80039DF8, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(race_text.packets, (uint32)sizeof(race_text.packets)));
        XASSERT(operation_ok);
    }
    tex_rec = tex_find("Text");
    tex = tex_rec;
    race_text.tex = tex;
    if (tex[12] == 0)
        tex[12] = (255);
    for (idx = 0; idx < 160; ++idx)
    {
        poly = &race_text.packets[idx / 2][idx & 1];
        poly->tag = (poly->tag & 0xFFFFFF) | ((uint32)(uint8)(9) << 24);
        poly->rgbc.code = (uint8)(0x2E);
        poly->p[0].uv.u = (uint8)(tex[14]);
        poly->p[0].uv.v = (uint8)(tex[15]);
        val = tex[14];
        other = tex[12];
        poly->p[1].uv.u = (uint8)(other + val - 1);
        poly->p[1].uv.v = (uint8)(tex[15]);
        poly->p[2].uv.u = (uint8)(tex[14]);
        val = tex[15];
        other = tex[13];
        poly->p[2].uv.v = (uint8)(other + val - 1);
        val = tex[14];
        other = tex[12];
        poly->p[3].uv.u = (uint8)(other + val - 1);
        val = tex[15];
        other = tex[13];
        poly->p[0].xy.x = (sint16)(96);
        poly->p[0].xy.y = (sint16)(200);
        poly->p[3].uv.v = (uint8)(other + val - 1);
        val = tex[12];
        poly->p[1].xy.y = (sint16)(200);
        poly->p[2].xy.x = (sint16)(96);
        poly->p[1].xy.x = (sint16)(val + 95);
        poly->p[2].xy.y = (sint16)(tex[13] + 199);
        poly->p[3].xy.x = (sint16)(tex[12] + 95);
        poly->p[3].xy.y = (sint16)(tex[13] + 199);
        poly->rgbc.r = (uint8)(255);
        poly->rgbc.g = (uint8)(255);
        poly->rgbc.b = (uint8)(0);
        poly->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
        poly->p[0].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->clut);
    }
    return draw_status_text(&car_states[0]);
}

void hud_reset_race_text(void)
{
    FUNCTION_MARKER(0x80039DEC, "SLUS_006.10");
    race_text.packet_idx = 0;
}

uint32 hud_draw_race_text(const char *text, uint32 x, uint32 y, uint32 color)
{
    uint32 len;
    uint32 small;
    uint32 idx;
    uint32 character;
    uint32 glyph;
    uint32 cur;
    uint32 buffer;
    POLY_FT4 *poly;
    const uint8 *tex;
    uint32 u;
    uint32 v;
    uint32 width;
    uint32 height;
    uint32 right;

    FUNCTION_MARKER(0x80038994, "SLUS_006.10");
    len = (uint32)strlen(text);
    small = race_session.two_players;
    if ((sint32)len <= 0)
        return len;
    width = small != 0 ? 8 : 7;
    height = small != 0 ? 8 : 16;
    for (idx = 0; idx < len; ++idx)
    {
        character = (uint8)text[idx];
        glyph = (character - 32) & 255;
        if (character == 32)
        {
            x += small != 0 ? 8 : 9;
            continue;
        }
        cur = race_text.packet_idx;
        buffer = disp_state.bank;
        XASSERT(!(cur >= HUD_RACE_TEXT_GLYPH_COUNT));
        poly = &race_text.packets[cur][buffer];
        race_text.packet_idx = cur + 1;
        poly->rgbc.r = (uint8)(255);
        poly->rgbc.g = (uint8)(color != 0 ? 0 : 255);
        poly->rgbc.b = (uint8)(0);
        tex = race_text.tex;
        right = x + width;
        poly->p[0].xy.x = (sint16)(x);
        poly->p[0].xy.y = (sint16)(y);
        poly->p[1].xy.x = (sint16)(right);
        poly->p[1].xy.y = (sint16)(y);
        poly->p[2].xy.x = (sint16)(x);
        poly->p[2].xy.y = (sint16)(y + height);
        poly->p[3].xy.x = (sint16)(right);
        poly->p[3].xy.y = (sint16)(y + height);
        if (small != 0)
        {
            u = (glyph % 21) * 8;
            v = (glyph / 21) * 8;
        }
        else
        {
            u = (glyph & 31) * 8;
            v = (glyph >> 5) * 16;
        }
        poly->p[0].uv.u = (uint8)(tex[14] + u);
        poly->p[0].uv.v = (uint8)(race_text.tex[15] + v);
        poly->p[1].uv.u = (uint8)(race_text.tex[14] + u + width);
        poly->p[1].uv.v = (uint8)(race_text.tex[15] + v);
        poly->p[2].uv.u = (uint8)(race_text.tex[14] + u);
        poly->p[2].uv.v = (uint8)(race_text.tex[15] + v + height);
        poly->p[3].uv.u = (uint8)(race_text.tex[14] + u + width);
        poly->p[3].uv.v = (uint8)(race_text.tex[15] + v + height);
        if (small != 0)
            x = (uint8)text[idx] == ':' ? x + 4 : right;
        else
            x += 9;
    }
    return 0;
}

uint32 hud_link_race_text(uint32 prev_result)
{
    DISPLAY_ORDERING head;
    uint32 count;
    uint32 idx;

    uint32 buffer;
    POLY_FT4 *poly;

    uint32 packet_tag;
    uint32 head_tag;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80039F80, "SLUS_006.10");
    count = race_text.packet_idx;
    if ((sint32)count <= 0)
        return prev_result;
    XASSERT(!(count > HUD_RACE_TEXT_GLYPH_COUNT));
    cur_frame = disp_state.cur;
    buffer = disp_state.bank;
    for (idx = 0; (sint32)idx < (sint32)count; ++idx)
    {
        poly = &race_text.packets[buffer / 2][buffer & 1];
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        packet_tag = poly->tag;
        head_tag = head.tags[(sint32)head.byte_offset / 4];
        poly->tag = (uint32)((packet_tag & 0xFF000000) | (head_tag & 0xFFFFFF));
        head.tags = cur_frame->ordering;
        head.byte_offset = 0;
        head_tag = head.tags[(sint32)head.byte_offset / 4];
        hd_ui_flat(poly->p, &poly->rgbc, 0);
        head.tags[(sint32)head.byte_offset / 4] = (head_tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
        buffer += 2;
    }
    return 0;
}

static uint32 draw_finish_times(void)
{
    char text[64];
    const char *format;
    uint32 result, player, ticks, divisor;
    sint32 rate, time, remainder;

    FUNCTION_MARKER(0x80038FE0, "SLUS_006.10");
    result = race_session.two_players;
    if (result != 0)
        return result;
    result = race_control.start_hold;
    if (result != 0)
        return result;
    for (player = 0; player < 2; ++player)
    {
        ticks = car_states[player].finish_time;
        if (ticks != 0)
        {
            rate = (sint32)disp_state.field_rate;
            divisor = (rate / 2);
            if (divisor == 0)
                xport_mips_break(7);
            time = (ticks / divisor);
            remainder = time % 60000;
            format = player == 0 ? "PLAYER    %02d:%02d:%03d" : "COMPUTER  %02d:%02d:%03d";
            sprintf(text, format, time / 60000, remainder / 1000, remainder % 1000);
            hud_draw_race_text(text, 160, player == 0 ? 64 : 80, 0);
            car_states[player].pedal = (uint16)(0xFC00);
            result = 1;
            car_states[player].steering_angle = 0;
            car_states[player].braking = (uint8)(1);
        }
    }
    return result;
}

static uint32 draw_race_times(CAR_STATE *car)
{
    char text[64];
    uint32 ticks, level, divisor;
    sint32 rate, time, remainder;

    FUNCTION_MARKER(0x80039208, "SLUS_006.10");
    ticks = car->race_stats.elapsed_ticks;
    rate = (sint32)disp_state.field_rate;
    time = fixed_divide_checked((sint32)(ticks * 100), rate / 2);
    remainder = time % 6000;
    sprintf(text, "%02d:%02d:%02d", time / 6000, remainder / 100, remainder % 100);
    hud_draw_race_text("TIME", 46, 12, 0);
    hud_draw_race_text(text, 28, 24, 0);
    level = race_session.track;
    ticks = rec_table.rows[level][0].data.val;
    rate = (sint32)disp_state.field_rate;
    divisor = (rate / 2);
    if (divisor == 0)
        xport_mips_break(7);
    time = ((ticks * 100) / divisor);
    remainder = time % 6000;
    sprintf(text, "%02d:%02d:%02d", time / 6000, remainder / 100, remainder % 100);
    hud_draw_race_text("BEST TIME", 408, 12, 0);
    return hud_draw_race_text(text, 412, 24, 0);
}

static uint32 draw_linear_times(CAR_STATE *car)
{
    char text[64];
    uint32 ticks, x, y, len;
    sint32 adjust, rate, time, remainder;

    FUNCTION_MARKER(0x800396DC, "SLUS_006.10");
    adjust = (sint16)car->checkpoint_progress;
    ticks = car->race_stats.elapsed_ticks;
    adjust = adjust * 1000 / 256;
    rate = (sint32)disp_state.field_rate;
    time = fixed_divide_checked((sint32)(ticks * 1000 - (uint32)adjust), rate / 2);
    remainder = time % 60000;
    sprintf(text, "TIME %02d:%02d:%03d", time / 60000, remainder / 1000, remainder % 1000);
    if (race_session.two_players != 0)
    {
        x = race_view.viewport_x;
        y = race_view.viewport_y;
        x += 16;
    }
    else
    {
        len = (uint32)strlen(text);
        y = race_view.viewport_y;
        x = 256 - len * 4;
    }
    hud_draw_race_text(text, x, y + 20, 0);
    return draw_finish_times();
}

static uint32 draw_status_text(CAR_STATE *car)
{
    char text[72];
    const char *title;
    uint32 level;
    uint32 len;
    uint32 name;
    uint32 x;
    uint32 y;
    sint32 width;
    sint32 height;
    sint32 lap;
    sint32 laps;
    sint32 total;

    FUNCTION_MARKER(0x80039A24, "SLUS_006.10");
    hud_reset_race_text();
    if (race_session.demo != 0)
    {
        if (session_flow.attract_active != 0)
            hud_draw_race_text("DEMO", 32, 200, 0);
        else
            hud_draw_race_text("REPLAY", 32, 200, 0);
        if ((sint32)race_control.update_cycles >= 60)
            return 0;
        level = race_session.track;
        title = ((uint32)(level + 1) < 48 ? session_event_names[level + 1] : "");
        len = (uint32)strlen(title);
        level = race_session.track;
        width = (sint32)model_render.width;
        height = (sint32)model_render.height;
        title = ((uint32)(level + 1) < 48 ? session_event_names[level + 1] : "");
        x = (uint32)(width / 2) - ((len >> 1) << 3);
        y = (height / 2);
        return hud_draw_race_text(title, x, y, 0);
    }
    if (race_session.two_players != 0)
    {
        if (race_session.drag_race != 0)
            return draw_linear_times(car);
        if (race_session.multiplayer_mode == 3)
            return chase_draw_status(car);
        lap = car->completed_laps + 1;
        laps = (sint32)race_session.laps;
        if (laps >= lap)
            laps = lap;
        total = (sint32)race_session.laps;
        sprintf(text, "LAP %d/%d", laps, total);
        x = race_view.viewport_x;
        y = race_view.viewport_y;
        hud_draw_race_text(text, x + 16, y + 16, 0);
        name = car->place;
        x = race_view.viewport_x;
        y = race_view.viewport_y;
        return hud_draw_race_text(((uint32)(name) < 6 ? session_place_names[name] : ""), x + 16, y + 28, 0);
    }
    if (race_session.linear_course != 0)
    {
        if (race_session.drag_race != 0)
            return draw_linear_times(car);
        lap = car->completed_laps + 1;
        laps = (sint32)race_session.laps;
        if (laps >= lap)
            laps = lap;
        total = (sint32)race_session.laps;
        sprintf(text, "LAP %d/%d", laps, total);
        hud_draw_race_text(text, 32, 12, 0);
    }
    else
    {
        if (race_session.event_mode == 1)
            return draw_race_times(car);
        if (race_session.cup == 4)
        {
            sprintf(text, "POINTS %d", (sint32)car->points);
            hud_draw_race_text(text, 32, 12, 0);
            name = car->place;
            return hud_draw_race_text(((uint32)(name) < 6 ? session_place_names[name] : ""), 32, 32, 0);
        }
        if (race_session.cop_chase != 0)
        {
            sprintf(text, "ARRESTS %d", (sint16)chase_state.arrests);
            hud_draw_race_text(text, 32, 12, 0);
            sprintf(text, "POINTS %d", (sint32)car->points);
            hud_draw_race_text(text, 32, 28, 0);
            return chase_draw_message();
        }
    }
    return draw_checkpoint_times(car);
}

uint32 hud_draw_full_view(CAR_STATE *car)
{
    uint32 result;

    FUNCTION_MARKER(0x800388A0, "SLUS_006.10");
    if (race_session.demo == 0)
    {
        if (race_session.drag_race == 0)
        {
            draw_reverse_indicator(car);
            draw_route_hints(car, 0);
            minimap_draw(car);
        }
        hd_ui_anchor(1);
        draw_large_gauges(car);
        hd_ui_anchor(0);
        if (race_rules.checkpoint_timers != 0)
            draw_checkpoint_timer();
    }
    result = draw_status_text(car);
    return hud_link_race_text(result);
}

uint32 hud_draw_split_view(CAR_STATE *car, uint32 parameter)
{
    uint32 result;

    FUNCTION_MARKER(0x80038934, "SLUS_006.10");
    if (race_session.demo == 0)
    {
        draw_reverse_indicator(car);
        hd_ui_anchor(1);
        draw_small_gauges(car, parameter);
        hd_ui_anchor(0);
    }
    result = draw_status_text(car);
    return hud_link_race_text(result);
}
