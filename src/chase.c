#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "game.h"
#include "psx_gpu.h"

typedef char CHASE_FT4_SIZE_CHECK[sizeof(POLY_FT4) == 40 ? 1 : -1];
typedef char CHASE_G4_SIZE_CHECK[sizeof(POLY_G4) == 36 ? 1 : -1];

static POLY_FT4 health_icons[2];
static POLY_G4 health_fill[2];
static POLY_G4 health_empty[2];

static const SVECTOR health_anchor = {0, 120, 0, 0};

CHASE_STATE chase_state;

static const char *const messages[8][2] = {{"SUSPECT IS WANTED FOR", "ARMED ROBBERY."}, {"SUSPECT IS WANTED FOR", "SPEEDING."}, {"SUSPECT IS ARMED", "AND DANGEROUS."}, {"SUSPECT IS WANTED", "FOR 1ST DEGREE MURDER."}, {"SUSPECT IS WANTED", "FOR ILLEGAL LICENSE PLATES."}, {"SUSPECT IS WANTED", "FOR GRAND THEFT AUTO."}, {"SUSPECT IS WANTED", "FOR CHICKEN PLUCKING."}, {"SUSPECT IS WANTED", "FOR SOFTWARE PIRACY."}};

void chase_draw_hd_health(uint32 slot)
{
    const uint8 *tex;
    const POLY_FT4 *icon;
    const POLY_G4 *shade;
    uint32 packet_bank, width, bar;
    if (!hd_capture_active() || (race_session.cop_chase == 0 && race_session.multiplayer_mode != 3) || slot != chase_state.sel_slot)
        return;
    XASSERT(slot < 6);
    packet_bank = disp_state.bank;
    tex = hud_arrow_tex[0];
    icon = &health_icons[packet_bank];
    width = race_session.multiplayer_mode == 3 ? 7 : 10;
    {
        MATRIXF transform = hd_model_transform();
        VECTORF anchor = {(float)health_anchor.vx, (float)health_anchor.vy, (float)health_anchor.vz}, position;
        HD_UI_VERTEX vertices[4];
        HD_TEXTURE texture = {((const TEXTURE_DESCRIPTOR *)tex)->page, ((const TEXTURE_DESCRIPTOR *)tex)->clut, tex[14], tex[15], tex[12], tex[13], HD_TEXTURED};
        uint32 point, first;
        float center_x, center_y;
        RotTransf(&transform, &anchor, &position);
        if (position.vz > 1.0f)
        {
            first = hd_ui_mark();
            float scale = model_render.proj_distance * model_render.scale / 4096.0f / position.vz;
            center_x = (sint32)model_render.view_center_x + position.vx * model_render.x_scale / 4096.0f * scale;
            center_y = (sint32)model_render.view_center_y + position.vy * model_render.y_scale / 4096.0f * scale - 16.0f;
            if ((icon->rgbc.code & 1) != 0)
                texture.flags |= HD_RAW_TEXTURE;
            if ((icon->rgbc.code & 2) != 0)
                texture.flags |= HD_SEMITRANSPARENT;
            for (point = 0; point < 4; ++point)
            {
                vertices[point].x = center_x + ((point & 1) != 0 ? tex[12] - 1.0f : 0.0f);
                vertices[point].y = center_y - ((point & 2) != 0 ? 1.0f : (float)tex[13]);
                vertices[point].u = tex[14] + ((point & 1) != 0 ? tex[12] - 1.0f : 0.0f);
                vertices[point].v = tex[15] + ((point & 2) != 0 ? 0.0f : tex[13] - 1.0f);
                memcpy(vertices[point].color, &icon->rgbc, 4);
            }
            hd_ui_ordered_quad(vertices, &texture, 1.0f, 4);
            for (bar = 0; bar < 2; ++bar)
            {
                HD_TEXTURE solid = {0};
                shade = bar == 0 ? &health_empty[packet_bank] : &health_fill[packet_bank];
                if ((shade->p[0].rgbc.code & 2) != 0)
                    solid.flags = HD_SEMITRANSPARENT;
                for (point = 0; point < 4; ++point)
                {
                    vertices[point].x = center_x + tex[12] + ((point & 1) != 0 ? (float)width : 4.0f);
                    vertices[point].y = center_y - tex[13] + ((point & 2) != 0 ? (bar == 0 ? tex[13] * (4096.0f - (sint16)chase_state.health[slot]) / 4096.0f : (float)tex[13]) : 0.0f);
                    memcpy(vertices[point].color, &shade->p[point].rgbc, 4);
                }
                hd_ui_ordered_quad(vertices, &solid, 1.0f, 4);
            }
            hd_ui_follow_car(first, slot, &health_anchor);
        }
    }
}

uint32 chase_draw_health(uint32 slot)
{
    DISPLAY_ORDERING ordering;
    uint32 sel, val, other, tag;
    const uint8 *tex;
    POLY_FT4 *icon;
    POLY_G4 *shade;
    uint32 x, y, mode, width, product, packet_bank, bar, result;
    sint32 amount;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80043328, "SLUS_006.10");
    if (race_session.cop_chase == 0 && race_session.multiplayer_mode != 3)
        return 3;
    sel = chase_state.sel_slot;
    if (slot != sel)
        return sel;
    val = disp_state.bank;
    tex = hud_arrow_tex[0];
    icon = &health_icons[val];
    SetRotMatrix(&model_render.matrix.sdk);
    val = model_render.matrix.sdk.t[0];
    other = model_render.matrix.sdk.t[1];
    xport_gte_write_control(5, val);
    val = model_render.matrix.sdk.t[2];
    xport_gte_write_control(6, other);
    xport_gte_write_control(7, val);
    val = (uint32)(uint16)health_anchor.vx | ((uint32)(uint16)health_anchor.vy << 16);
    xport_gte_write_data(0, val);
    val = (uint32)(uint16)health_anchor.vz | ((uint32)(uint16)health_anchor.pad << 16);
    xport_gte_write_data(1, val);
    xport_gte_execute(0x180001);
    val = xport_gte_read_data(14);
    x = val & 0xFFFF;
    y = ((val >> 16) - 16) & 0xFFFF;
    val = tex[14];
    icon->p[0].uv.u = (uint8)(val);
    val = tex[15];
    other = tex[13];
    icon->p[0].uv.v = (uint8)(val + other - 1);
    val = tex[14];
    other = tex[12];
    icon->p[1].uv.u = (uint8)(val + other - 1);
    val = tex[15];
    other = tex[13];
    icon->p[1].uv.v = (uint8)(val + other - 1);
    val = tex[14];
    icon->p[2].uv.u = (uint8)(val);
    val = tex[15];
    icon->p[2].uv.v = (uint8)(val);
    val = tex[14];
    other = tex[12];
    icon->p[3].uv.u = (uint8)(val + other - 1);
    val = tex[15];
    icon->p[3].uv.v = (uint8)(val);
    icon->p[0].xy.x = (sint16)(x);
    val = tex[13];
    icon->p[0].xy.y = (sint16)(y - val);
    val = tex[12];
    icon->p[1].xy.x = (sint16)(x + val - 1);
    val = tex[13];
    icon->p[1].xy.y = (sint16)(y - val);
    icon->p[2].xy.x = (sint16)(x);
    icon->p[2].xy.y = (sint16)(y - 1);
    val = tex[12];
    icon->p[3].xy.x = (sint16)(x + val - 1);
    cur_frame = disp_state.cur;
    icon->p[3].xy.y = (sint16)(y - 1);
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    tag = icon->tag;
    val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(16)) / 4];
    icon->tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(16)) / 4];
    mode = race_session.multiplayer_mode;
    ordering.tags[(sint32)(ordering.byte_offset + (uint32)(16)) / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)icon);
    width = mode == 3 ? 7 : 10;
    val = (sint16)chase_state.health[slot];
    other = tex[13];
    product = other * (4096 - val);
    amount = (sint32)product / 4096;
    packet_bank = disp_state.bank;
    for (bar = 0; bar < 2; ++bar)
    {
        val = tex[12];
        shade = bar == 0 ? &health_empty[packet_bank] : &health_fill[packet_bank];
        shade->p[0].xy.x = (sint16)(x + val + 4);
        val = tex[13];
        shade->p[0].xy.y = (sint16)(y - val);
        val = tex[12];
        shade->p[1].xy.x = (sint16)(x + val + width);
        val = tex[13];
        shade->p[1].xy.y = (sint16)(y - val);
        val = tex[12];
        shade->p[2].xy.x = (sint16)(x + val + 4);
        if (bar == 0)
        {
            val = tex[13];
            shade->p[2].xy.y = (sint16)(y - val + (uint32)amount);
        }
        else
            shade->p[2].xy.y = (sint16)(y);
        val = tex[12];
        shade->p[3].xy.x = (sint16)(x + val + width);
        if (bar == 0)
        {
            val = tex[13];
            shade->p[3].xy.y = (sint16)(y - val + (uint32)amount);
        }
        else
            shade->p[3].xy.y = (sint16)(y);
        ordering.tags = cur_frame->ordering;
        ordering.byte_offset = 0;
        tag = shade->tag;
        val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(16)) / 4];
        shade->tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
        ordering.tags = cur_frame->ordering;
        ordering.byte_offset = 0;
        val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(16)) / 4];
        result = (val & 0xFF000000) | gpu_packet_link((uintptr_t)shade);
        ordering.tags[(sint32)(ordering.byte_offset + (uint32)(16)) / 4] = result;
    }
    chase_draw_hd_health(slot);
    return result;
}

uint32 chase_draw_message(void)
{
    char buffer[80];
    uint32 counter, idx, count, len0, len1;
    const char *src;
    char *dst;
    const char *input;

    FUNCTION_MARKER(0x8003A028, "SLUS_006.10");
    counter = chase_state.phrase_ticks;
    if ((sint32)counter >= 300)
        return 0;
    ++counter;
    chase_state.phrase_ticks = counter;
    if ((sint32)counter >= 270 && (counter & 1) != 0)
        return 1;
    idx = chase_state.phrase_idx;
    src = messages[idx][0];
    len0 = (uint32)strlen(src);
    idx = chase_state.phrase_idx;
    count = chase_state.chars0;
    src = messages[idx][0];
    dst = buffer;
    input = src;
    memcpy(dst, input, count);
    count = chase_state.chars0;
    buffer[count] = 0;
    count = chase_state.chars0;
    if ((sint32)count < (sint32)len0)
        chase_state.chars0 = count + 1;
    count = chase_state.chars0;
    hud_draw_race_text(buffer, 480 - count * 9, 12, 0);
    idx = chase_state.phrase_idx;
    src = messages[idx][1];
    len1 = (uint32)strlen(src);
    idx = chase_state.phrase_idx;
    count = chase_state.chars1;
    src = messages[idx][1];
    dst = buffer;
    input = src;
    memcpy(dst, input, count);
    count = chase_state.chars1;
    buffer[count] = 0;
    count = chase_state.chars0;
    if (count == len0)
    {
        count = chase_state.chars1;
        if ((sint32)count < (sint32)len1)
            chase_state.chars1 = count + 1;
    }
    count = chase_state.chars1;
    return hud_draw_race_text(buffer, 480 - count * 9, 24, 0);
}

uint32 chase_draw_status(CAR_STATE *car)
{
    char text[72];
    uint32 slot, owner, x, y, count, val, origin_x, origin_y;
    uint16 *address;
    sint32 number, total, opacity;

    FUNCTION_MARKER(0x800394C8, "SLUS_006.10");
    slot = car->slot;
    owner = chase_state.pursuer_slot;
    if (slot == owner)
    {
        number = (sint16)chase_state.arrests;
        sprintf(text, "ARRESTS %d", number);
        x = race_view.viewport_x;
        y = race_view.viewport_y;
        hud_draw_race_text(text, x + 16, y + 16, 0);
        number = (sint32)car->points;
        sprintf(text, "POINTS %d", number);
        x = race_view.viewport_x;
        y = race_view.viewport_y;
        return hud_draw_race_text(text, x + 16, y + 28, 0);
    }
    number = (sint32)car->completed_laps + 1;
    total = (sint32)race_session.laps;
    if (total < number)
        number = total;
    total = (sint32)race_session.laps;
    sprintf(text, "LAP %d/%d", number, total);
    x = race_view.viewport_x;
    y = race_view.viewport_y;
    hud_draw_race_text(text, x + 16, y + 16, 0);
    count = chase_state.arrest_notice_ticks;
    if (count == 0)
        return 0x20000;
    val = count * 0x2D82D;
    slot = car->slot;
    chase_state.health[slot] = (uint16)(4096 - val / 4096);
    slot = car->slot;
    address = &chase_state.health[slot];
    opacity = (sint16)*address;
    if (opacity < 0)
        opacity = 0;
    else if (opacity > 4096)
        opacity = 4096;
    count = chase_state.arrest_notice_ticks;
    x = race_view.viewport_width;
    *address = opacity;
    origin_x = race_view.viewport_x;
    y = race_view.viewport_height;
    x = origin_x + (uint32)((sint32)(x - 56) / 2);
    origin_y = race_view.viewport_y;
    y = origin_y + (uint32)((sint32)y / 2);
    chase_state.arrest_notice_ticks = (uint8)(count - 1);
    return hud_draw_race_text("BUSTED!", x, y, 0);
}

uint32 chase_decay_signal(void)
{
    sint32 val;

    FUNCTION_MARKER(0x800439C8, "SLUS_006.10");
    val = (sint16)chase_state.signal_level;
    if (val > 0)
        chase_state.signal_level = (chase_state.signal_level & 0xFFFF0000u) | (uint16)((uint32)val - 512);
    val = (sint16)chase_state.signal_level;
    if (val < 0)
        val = 0;
    else if (val > 4096)
        val = 4096;
    chase_state.signal_level = (chase_state.signal_level & 0xFFFF0000u) | (uint16)((uint32)val);
    return audio_set_engine_volume(val);
}

uint32 chase_apply_contact(uint32 slot, sint32 distance)
{
    uint32 result, owner, step, val, mode, arrests;
    CAR_STATE *point_car;
    uint16 *counter;
    sint32 scale, remaining, random;

    FUNCTION_MARKER(0x800437AC, "SLUS_006.10");
    result = chase_state.arrest_notice_ticks;
    if (result != 0)
        return result;
    owner = chase_state.pursuer_slot;
    if (slot == owner)
        return 0;
    scale = (sint16)chase_state.signal_level;
    result = distance < 10000;
    if (scale == 0)
        return result;
    if (result != 0)
        return (distance < 20001);
    step = distance < 20001 ? 512 : 1024;
    result = chase_state.sel_slot;
    if (result != slot)
    {
        random = randPSX();
        result = (random % 8);
        chase_state.phrase_idx = result;
        chase_state.phrase_ticks = 0;
        chase_state.chars0 = 0;
        chase_state.chars1 = 0;
        chase_state.sel_slot = slot;
        return result;
    }
    counter = &chase_state.health[slot];
    remaining = (sint16)*counter;
    if (remaining <= 0)
        return remaining;
    val = *counter;
    chase_state.sel_slot = slot;
    *counter = (val - step);
    point_car = &car_states[owner];
    val = point_car->points;
    point_car->points = (uint32)(val + (step == 512 ? 10 : 20));
    remaining = (sint16)*counter;
    if (remaining <= 0)
    {
        owner = chase_state.pursuer_slot;
        point_car = &car_states[owner];
        arrests = chase_state.arrests;
        val = point_car->points;
        mode = race_session.multiplayer_mode;
        point_car->points = (uint32)(val + 50);
        chase_state.arrests = (uint16)(arrests + 1);
        if (mode == 3)
            chase_state.arrest_notice_ticks = (uint8)(90);
    }
    remaining = (sint16)*counter;
    result = remaining;
    if (remaining < 0)
        val = 0;
    else
    {
        result = remaining < 4097;
        val = result != 0 ? (uint32)remaining : 4096;
    }
    *counter = (val);
    return result;
}

uint32 chase_init(void)
{
    uint32 idx;
    POLY_FT4 *prim;
    const uint8 *tex;
    uint32 val;
    uint32 extent;
    POLY_G4 *shade;

    FUNCTION_MARKER(0x800430B0, "SLUS_006.10");
    val = race_session.cop_chase;
    if (val == 0 && race_session.multiplayer_mode != 3)
        return 3;
    chase_state.sel_slot = 0xFFFFFFFF;
    chase_state.signal_level = (chase_state.signal_level & 0xFFFF0000u) | (uint16)(0);
    chase_state.arrests = (uint16)(0);
    chase_state.phrase_idx = 0;
    chase_state.phrase_ticks = 0xFFFF;
    chase_state.chars0 = 0;
    chase_state.chars1 = 0;
    chase_state.arrest_notice_ticks = (uint8)(0);
    for (idx = 0; idx < 6; ++idx)
        chase_state.health[5 - idx] = 4096;
    {
        sint32 operation_ok = !(!gpu_register_packet_range(health_icons, (uint32)sizeof(health_icons)) || !gpu_register_packet_range(health_fill, (uint32)sizeof(health_fill)) || !gpu_register_packet_range(health_empty, (uint32)sizeof(health_empty)));
        XASSERT(operation_ok);
    }
    for (idx = 0; idx < 2; ++idx)
    {
        prim = &health_icons[idx];
        prim->tag = (prim->tag & 0x00FFFFFFu) | ((uint32)(uint8)(9) << 24);
        prim->rgbc.code = (uint8)(0x2C);
        tex = hud_arrow_tex[0];
        val = tex[14];
        prim->p[0].uv.u = (uint8)(val);
        tex = hud_arrow_tex[0];
        val = tex[15];
        prim->p[0].uv.v = (uint8)(val);
        tex = hud_arrow_tex[0];
        val = tex[14];
        extent = tex[12];
        prim->p[1].uv.u = (uint8)(val + extent + 255);
        tex = hud_arrow_tex[0];
        val = tex[15];
        prim->p[1].uv.v = (uint8)(val);
        tex = hud_arrow_tex[0];
        val = tex[14];
        prim->p[2].uv.u = (uint8)(val);
        tex = hud_arrow_tex[0];
        val = tex[15];
        extent = tex[13];
        prim->p[2].uv.v = (uint8)(val + extent + 255);
        tex = hud_arrow_tex[0];
        val = tex[14];
        extent = tex[12];
        prim->p[3].uv.u = (uint8)(val + extent + 255);
        tex = hud_arrow_tex[0];
        val = tex[15];
        extent = tex[13];
        prim->p[3].uv.v = (uint8)(val + extent + 255);
        tex = hud_arrow_tex[0];
        val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        prim->p[1].attr = (uint16)(val);
        tex = hud_arrow_tex[0];
        val = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        prim->p[0].attr = (uint16)(val);
        val = prim->rgbc.code;
        prim->rgbc.code = (uint8)((val | 1) & 0xFD);
        shade = &health_fill[idx];
        shade->tag = (shade->tag & 0x00FFFFFFu) | ((uint32)(uint8)(8) << 24);
        shade->p[0].rgbc.code = (uint8)(0x38);
        shade->p[0].rgbc.r = (uint8)(0);
        shade->p[0].rgbc.g = (uint8)(255);
        shade->p[0].rgbc.b = (uint8)(0);
        shade->p[1].rgbc.r = (uint8)(0);
        shade->p[1].rgbc.g = (uint8)(255);
        shade->p[1].rgbc.b = (uint8)(0);
        shade->p[2].rgbc.r = (uint8)(255);
        shade->p[2].rgbc.g = (uint8)(0);
        shade->p[2].rgbc.b = (uint8)(0);
        shade->p[3].rgbc.r = (uint8)(255);
        shade->p[3].rgbc.g = (uint8)(0);
        shade->p[3].rgbc.b = (uint8)(0);
        shade = &health_empty[idx];
        shade->tag = (shade->tag & 0x00FFFFFFu) | ((uint32)(uint8)(8) << 24);
        shade->p[0].rgbc.code = (uint8)(0x38);
        shade->p[0].rgbc.r = (uint8)(32);
        shade->p[0].rgbc.g = (uint8)(0);
        shade->p[0].rgbc.b = (uint8)(0);
        shade->p[1].rgbc.r = (uint8)(96);
        shade->p[1].rgbc.g = (uint8)(0);
        shade->p[1].rgbc.b = (uint8)(0);
        shade->p[2].rgbc.r = (uint8)(32);
        shade->p[2].rgbc.g = (uint8)(0);
        shade->p[2].rgbc.b = (uint8)(0);
        shade->p[3].rgbc.r = (uint8)(96);
        shade->p[3].rgbc.g = (uint8)(0);
        shade->p[3].rgbc.b = (uint8)(0);
    }
    return 0;
}

uint32 chase_raise_signal(void)
{
    uint32 val;

    FUNCTION_MARKER(0x800439B0, "SLUS_006.10");
    val = (uint16)chase_state.signal_level + 1024;
    chase_state.signal_level = (chase_state.signal_level & 0xFFFF0000u) | (uint16)(val);
    return val;
}

uint32 chase_update_braking(void)
{
    uint32 car0_error, car9_error, count, result;
    sint32 seg, player_seg;

    FUNCTION_MARKER(0x8004E46C, "SLUS_006.10");
    car0_error = race_ai_heading_error(&race_ai_cars[0]);
    car9_error = race_ai_heading_error(&race_ai_cars[9]);
    count = race_ai_cars[0].route_speed;
    car_states[9].braking = (uint8)(1);
    car_states[9].pedal = (uint16)(0xFF00);
    if ((sint32)count < 9 || car0_error - 1024 < 2049 || car9_error - 1024 < 2049)
    {
        result = car_states[0].chase_events;
        race_ai_cars[9].chase_target = 0xFFFFFFFF;
        race_ai_cars[0].chase_brake = 0;
        race_ai_cars[0].reverse = 0;
        race_ai_cars[9].program_argument = 300;
        race_ai_cars[9].traffic_selection_offset = 0;
        race_ai_cars[9].recovery_state = 0;
        car_states[9].pedal = (uint16)(0);
        car_states[9].braking = (uint8)(0);
        ++result;
        car_states[0].chase_events = (uint8)result;
        return result;
    }
    result = (sint32)count < (sint32)race_ai_cars[9].route_speed;
    if (result == 0)
    {
        seg = (sint16)car_states[9].track_location.n_node;
        player_seg = (sint16)car_states[0].track_location.n_node;
        result = (sint32)((uint32)seg - (uint32)player_seg) < 3;
        if (result != 0)
        {
            car_states[0].braking = (uint8)(1);
            car_states[0].pedal = (uint16)(0xFF00);
            return result;
        }
    }
    car_states[0].pedal = (uint16)(0);
    return result;
}
