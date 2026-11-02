#include <stdlib.h>
#include <string.h>

#include "race.h"
#include "psx_gpu.h"
#include "game.h"

typedef char RACE_AI_ROUTE_SAMPLE_SIZE_CHECK[sizeof(RACE_AI_ROUTE_SAMPLE) == 3 && offsetof(RACE_AI_ROUTE_SAMPLE, heading) == 1 && offsetof(RACE_AI_ROUTE_SAMPLE, speed_limit) == 2 ? 1 : -1];

typedef char RACE_TRAFFIC_SIZE_CHECK[sizeof(RACE_TRAFFIC_RECORD) == 4 ? 1 : -1];

typedef char RACE_TRAFFIC_FLAGS_CHECK[offsetof(RACE_TRAFFIC_RECORD, flags) == 2 ? 1 : -1];

typedef char RACE_TRAFFIC_SIDE_CHECK[offsetof(RACE_TRAFFIC_RECORD, side) == 3 ? 1 : -1];

typedef char RACE_RESULT_WORD_SIZE_CHECK[sizeof(RACE_RESULT_WORD) == 4 ? 1 : -1];

typedef char RACE_RESULT_SIZE_CHECK[sizeof(RACE_RESULT) == 20 ? 1 : -1];

typedef char RACE_RESULT_TABLE_SIZE_CHECK[sizeof(RACE_RESULT_TABLE) == 120 ? 1 : -1];

static const uint8 championship_place_points[6] = {15, 12, 10, 5, 4, 3};
static const sint16 bonus_championship_place_points[4] = {1000, 500, 250, 0};

RACE_AI_STATE race_ai = {.trailing_gain = 128, .leading_gain = 128, .trailing_limit = 128, .leading_limit = 128};

static uint32 ai_initial_scale[12] = {256, 256, 320, 280, 290, 320, 0, 0, 0, 700, 0, 0};

static const uint32 ai_program_recover[3] = {8, 9, 0};
static const uint32 ai_program_reverse_left[6] = {8, 2, 0xFFFFFFE0, 5, 8, 0};
static const uint32 ai_program_forward_right[5] = {8, 2, 64, 6, 0};
static const uint32 ai_program_reverse_right[6] = {8, 2, 0xFFFFFFE0, 6, 8, 0};
static const uint32 ai_program_forward_left[5] = {8, 2, 64, 5, 0};

RACE_AI_CAR race_ai_cars[12];

RACE_RESULT_TABLE race_results;
uint8 race_order[6];
RACE_VIEW race_view = {.split_mode = 1};
static const uint32 single_lighting_color = 0x00808080u;
static const uint32 split_lighting_color = 0x00D0D0D0u;
static const uint32 primary_view_gate = 0;

RACE_CONTROL race_control = {.car_count = 14};

typedef struct PAUSE_CAPTURE_REGION
{
    uint32 width;
    uint32 height;
    sint32 offset_x;
    sint32 offset_y;
    uint32 *pixels;
} PAUSE_CAPTURE_REGION;

typedef struct PAUSE_PIXEL_BUFFERS
{
    uint32 horizontal[2][232 * 16 / 2];
    uint32 vertical[2][16 * 100 / 2];
} PAUSE_PIXEL_BUFFERS;

static PAUSE_PIXEL_BUFFERS pause_pixels;

static const PAUSE_CAPTURE_REGION pause_capture_regions[4] = {
    {232, 16, 0, -30, pause_pixels.horizontal[0]},
    {232, 16, 0, 60, pause_pixels.horizontal[1]},
    {16, 100, -105, 10, pause_pixels.vertical[0]},
    {16, 100, 105, 10, pause_pixels.vertical[1]},
};
static const PSX_RECT pause_scr_rect = {0, 0, 512, 256};

static const char pause_title[] = "PAUSED";
static const char *const pause_menu_labels[6] = {
    "CONTINUE", "SOUND VOLUME", "MUSIC VOLUME", "RETIRE", "SCREEN LEFT/RIGHT", "SCREEN TOP/BOTTOM",
};
static const char *const pause_confirmation_labels[3] = {"ARE YOU SURE?", "YES", "NO"};

static uint32 pause_wave_ticks;

typedef struct PAUSE_GPU_PACKETS
{
    DR_MODE draw_modes[2];
    POLY_G4 volume_gradient[2][2];
    POLY_G4 volume_mask[2][2];
    POLY_G4 background[2][30];
} PAUSE_GPU_PACKETS;

static PAUSE_GPU_PACKETS pause_packets;

typedef char PAUSE_BACKGROUND_PACKET_LAYOUT_CHECK[sizeof(POLY_G4) == 36 && offsetof(POLY_G4, p) == 4 && sizeof(POINT_C) == 8 && offsetof(POINT_C, xy) == 4 && sizeof(DR_MODE) == 12 ? 1 : -1];

static uint32 set_split_mode(uint32 mode);
static uint32 select_primary_view(uint32 slot);
static uint32 select_secondary_view(uint32 slot);
static uint32 update_pos(void);
static uint32 update_ai_gap_scale(void);
static uint32 update_ai_intervals(uint32 slot);
static uint32 classify_ai_route_offset(uint32 slot, uint32 offset);
static uint32 update_traffic_car(uint32 slot);
static uint32 update_ai_seg(RACE_AI_CAR *owner);
static uint32 update_ai_car(uint32 slot);
static uint32 init_traffic(void);
static uint32 respawn_traffic_car(void);
static uint32 update_checkpoint_laps(CAR_STATE *car);

static uint32 restore_pause_background(void)
{
    uint32 idx;
    uint32 width;
    uint32 offset;
    uint32 scr_height;
    uint32 height;
    uint32 flip;
    uint32 vertical_offset;
    uint32 adjust;
    uint32 val;
    PSX_RECT rect;

    FUNCTION_MARKER(0x80059B38, "SLUS_006.10");
    for (idx = 0; idx < 4; ++idx)
    {
        val = model_render.width;
        width = pause_capture_regions[idx].width;
        offset = (uint32)pause_capture_regions[idx].offset_x;
        rect.x = (sint16)((uint32)((sint32)(val - width) / 2) + offset);
        scr_height = model_render.height;
        height = pause_capture_regions[idx].height;
        val = ((sint32)(scr_height - height) / 2);
        flip = disp_state.bank;
        vertical_offset = (uint32)pause_capture_regions[idx].offset_y;
        adjust = disp_state.pal;
        rect.y = (sint16)(val + (flip << 8) + vertical_offset - adjust * 14);
        rect.w = (sint16)pause_capture_regions[idx].width;
        rect.h = (sint16)pause_capture_regions[idx].height;
        LoadImagePSX(&rect, pause_capture_regions[idx].pixels);
    }
    return 0;
}

static uint32 allocate_pause_resources(void)
{
    POLY_G4 *poly;
    uint32 buffer;
    uint32 item;

    FUNCTION_MARKER(0x800592A4, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&pause_packets, (uint32)sizeof(pause_packets)));
        XASSERT(operation_ok);
    }
    for (buffer = 0; buffer < 2; ++buffer)
    {
        poly = pause_packets.background[buffer];
        for (item = 0; item < 30; ++item)
        {
            setPolyG4(poly);
            ++poly;
        }
        SetDrawMode(&pause_packets.draw_modes[buffer], 0, 1, 0, 0);
    }
    return 0;
}

static uint32 capture_pause_background(void)
{
    uint32 idx;
    uint32 width;
    uint32 offset;
    uint32 scr_height;
    uint32 height;
    uint32 flip;
    uint32 vertical_offset;
    uint32 adjust;
    uint32 val;
    PSX_RECT rect;

    FUNCTION_MARKER(0x800599A0, "SLUS_006.10");
    for (idx = 0; idx < 4; ++idx)
    {
        val = model_render.width;
        width = pause_capture_regions[idx].width;
        offset = (uint32)pause_capture_regions[idx].offset_x;
        rect.x = (sint16)((uint32)((sint32)(val - width) / 2) + offset);
        scr_height = model_render.height;
        height = pause_capture_regions[idx].height;
        val = ((sint32)(scr_height - height) / 2);
        flip = disp_state.bank;
        vertical_offset = (uint32)pause_capture_regions[idx].offset_y;
        adjust = disp_state.pal;
        rect.y = (sint16)(val + (flip << 8) + vertical_offset - adjust * 14);
        rect.w = (sint16)pause_capture_regions[idx].width;
        rect.h = (sint16)pause_capture_regions[idx].height;
        StoreImage(&rect, pause_capture_regions[idx].pixels);
    }
    return 0;
}

static uint32 draw_pause_background(void)
{
    DISPLAY_ORDERING head_address;
    uint16 pos_x[42], pos_y[42];
    uint8 shades[84];
    POLY_G4 *poly;
    DR_MODE *draw_mode;
    uint32 width, bank, ticks, column, row, point, shade, val;
    uint32 tag, head, result;
    sint32 wave, divisor, c;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80059490, "SLUS_006.10");
    width = model_render.width == 512 ? 256 : 160;
    bank = disp_state.bank;
    ticks = pause_wave_ticks;
    poly = pause_packets.background[bank];
    pause_wave_ticks = ticks + 1;
    for (column = 0; column < 7; ++column)
    {
        for (row = 0; row < 6; ++row)
        {
            ticks = pause_wave_ticks;
            wave = rsin((sint32)(ticks * 212 + column * 768 + row * 768)) / 64;
            ticks = pause_wave_ticks;
            c = rcos((sint32)(ticks * 238 + column * 512 + row * 1024));
            divisor = wave + 1280;
            val = (uint32)c + (column * 2 - 6) * 22000;
            val = (uint32)fixed_divide_checked((sint32)val, divisor);
            ticks = pause_wave_ticks;
            point = row * 7 + column;
            pos_x[point] = (val + width);
            c = rcos((sint32)(ticks * 144 + column * 768 + row * 384));
            val = (uint32)c - 60000 + row * 24000;
            pos_y[point] = ((uint32)fixed_divide_checked((sint32)val, divisor) + 128);
            val = 16 - (uint32)((wave - 64) / 4);
            shades[42 + point] = val;
            shades[point] = ((val & 255) >> 1);
        }
    }
    for (column = 0; column < 6; ++column)
    {
        for (row = 0; row < 5; ++row)
        {
            point = row * 7 + column;
            poly->p[0].xy.x = (sint16)(uint16)(pos_x[point]);
            poly->p[0].xy.y = (sint16)(uint16)(pos_y[point]);
            poly->p[1].xy.x = (sint16)(uint16)(pos_x[point + 1]);
            poly->p[1].xy.y = (sint16)(uint16)(pos_y[point + 1]);
            poly->p[2].xy.x = (sint16)(uint16)(pos_x[point + 7]);
            poly->p[2].xy.y = (sint16)(uint16)(pos_y[point + 7]);
            poly->p[3].xy.x = (sint16)(uint16)(pos_x[point + 8]);
            poly->p[3].xy.y = (sint16)(uint16)(pos_y[point + 8]);
            shade = point + ((column + row) & 1) * 42;
            poly->p[0].rgbc.r = (uint8)(shades[shade]);
            poly->p[0].rgbc.g = (uint8)(shades[shade]);
            poly->p[0].rgbc.b = (uint8)(shades[shade]);
            poly->p[1].rgbc.r = (uint8)(shades[shade + 1]);
            poly->p[1].rgbc.g = (uint8)(shades[shade + 1]);
            poly->p[1].rgbc.b = (uint8)(shades[shade + 1]);
            poly->p[2].rgbc.r = (uint8)(shades[shade + 7]);
            poly->p[2].rgbc.g = (uint8)(shades[shade + 7]);
            poly->p[2].rgbc.b = (uint8)(shades[shade + 7]);
            poly->p[3].rgbc.r = (uint8)(shades[shade + 8]);
            poly->p[3].rgbc.g = (uint8)(shades[shade + 8]);
            poly->p[3].rgbc.b = (uint8)(shades[shade + 8]);
            cur_frame = disp_state.cur;
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = (uint32)(800);
            tag = poly->tag;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            poly->tag = (tag & 0xFF000000) | (head & 0xFFFFFF);
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = (uint32)(800);
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            hd_ui_gradient(poly->p, 200);
            head_address.tags[(sint32)head_address.byte_offset / 4] = (head & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
            ++poly;
        }
    }
    bank = disp_state.bank;
    head_address.tags = cur_frame->ordering;
    head_address.byte_offset = (uint32)(800);
    draw_mode = &pause_packets.draw_modes[bank];
    tag = draw_mode->tag;
    head = head_address.tags[(sint32)head_address.byte_offset / 4];
    draw_mode->tag = (tag & 0xFF000000) | (head & 0xFFFFFF);
    head_address.tags = cur_frame->ordering;
    head_address.byte_offset = (uint32)(800);
    head = head_address.tags[(sint32)head_address.byte_offset / 4];
    result = (head & 0xFF000000) | gpu_packet_link((uintptr_t)draw_mode);
    head_address.tags[(sint32)head_address.byte_offset / 4] = result;
    return result;
}

static uint32 draw_pause_menu(uint32 player)
{
    DISPLAY_ORDERING head_address;
    POLY_G4 *poly;
    uint32 center, rows, y, idx, half_width, len;
    uint32 left, right, tag, head, val, selection, result;
    const char *text;
    uint16 *pad = &input_ports[player].pressed;
    sint32 volume, quotient;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80059C48, "SLUS_006.10");
    center = model_render.width == 512 ? 256 : 160;
    if (race_control.pause_confirmation != 0)
    {
        text = pause_confirmation_labels[0];
        len = (uint32)strlen(text);
        text = pause_confirmation_labels[0];
        hud_draw_race_text(text, center - ((len * 9) >> 1), 110, 0);
        text = pause_confirmation_labels[1];
        len = (uint32)strlen(text);
        text = pause_confirmation_labels[1];
        selection = race_control.pause_selection;
        hud_draw_race_text(text, center - (len * 9 + 4), 122, selection == 0);
        selection = race_control.pause_selection;
        text = pause_confirmation_labels[2];
        hud_draw_race_text(text, center | 4, 122, selection == 1);
        if ((*pad & 0x80) != 0 && race_control.pause_selection == 1)
        {
            audio_play_menu_move();
            race_control.pause_selection = (uint32)(0);
        }
        result = *pad & 0x20;
        if (result != 0)
        {
            result = race_control.pause_selection;
            if (result == 0)
            {
                audio_play_menu_move();
                result = 1;
                race_control.pause_selection = (uint32)(result);
            }
        }
        return result;
    }
    if (race_session.two_players != 0)
    {
        rows = 5;
        y = 95;
    }
    else
    {
        rows = 4;
        y = 100;
    }
    text = pause_title;
    len = (uint32)strlen(text);
    text = pause_title;
    hud_draw_race_text(text, center - ((len * 9) >> 1), y - 8, 0);
    y += 8;
    for (idx = 0; idx < rows; ++idx)
    {
        text = pause_menu_labels[idx];
        len = (uint32)strlen(text);
        half_width = (len * 9) >> 1;
        if (idx - 1 < 2)
        {
            poly = &pause_packets.volume_gradient[idx - 1][disp_state.bank];
            poly->p[0].rgbc.code = (uint8)(0x38);
            poly->p[0].rgbc.g = (uint8)(255);
            poly->p[1].rgbc.r = (uint8)(255);
            poly->p[2].rgbc.g = (uint8)(255);
            setlen(poly, 8);
            poly->p[0].rgbc.r = (uint8)(0);
            poly->p[0].rgbc.b = (uint8)(0);
            poly->p[1].rgbc.g = (uint8)(0);
            poly->p[1].rgbc.b = (uint8)(0);
            poly->p[2].rgbc.r = (uint8)(0);
            poly->p[2].rgbc.b = (uint8)(0);
            poly->p[3].rgbc.r = (uint8)(255);
            poly->p[3].rgbc.g = (uint8)(0);
            poly->p[3].rgbc.b = (uint8)(0);
            cur_frame = disp_state.cur;
            left = center + half_width - 20;
            right = center + half_width + 20;
            poly->p[0].xy.x = (sint16)(uint16)((uint16)left);
            poly->p[0].xy.y = (sint16)(uint16)((uint16)(y + 6));
            poly->p[1].xy.x = (sint16)(uint16)((uint16)right);
            poly->p[1].xy.y = (sint16)(uint16)((uint16)(y + 6));
            poly->p[2].xy.x = (sint16)(uint16)((uint16)left);
            poly->p[2].xy.y = (sint16)(uint16)((uint16)(y + 10));
            poly->p[3].xy.x = (sint16)(uint16)((uint16)right);
            poly->p[3].xy.y = (sint16)(uint16)((uint16)(y + 10));
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = (uint32)(8);
            tag = poly->tag;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            poly->tag = (tag & 0xFF000000) | (head & 0xFFFFFF);
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = (uint32)(8);
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            hd_ui_gradient(poly->p, 2);
            head_address.tags[(sint32)head_address.byte_offset / 4] = (head & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
            val = idx == 1 ? audio_options.effects_volume : audio_options.music_volume;
            volume = (4096 - val);
            poly = &pause_packets.volume_mask[idx - 1][disp_state.bank];
            quotient = volume / 102;
            setlen(poly, 8);
            poly->p[0].rgbc.r = (uint8)(15);
            poly->p[1].rgbc.r = (uint8)(15);
            poly->p[2].rgbc.r = (uint8)(15);
            poly->p[3].rgbc.r = (uint8)(15);
            poly->p[0].rgbc.code = (uint8)(0x38);
            poly->p[0].rgbc.g = (uint8)(0);
            poly->p[0].rgbc.b = (uint8)(0);
            poly->p[1].rgbc.g = (uint8)(0);
            poly->p[1].rgbc.b = (uint8)(0);
            poly->p[2].rgbc.g = (uint8)(0);
            poly->p[2].rgbc.b = (uint8)(0);
            poly->p[3].rgbc.g = (uint8)(0);
            poly->p[3].rgbc.b = (uint8)(0);
            poly->p[0].xy.y = (sint16)(uint16)((uint16)(y + 6));
            poly->p[1].xy.y = (sint16)(uint16)((uint16)(y + 6));
            poly->p[2].xy.y = (sint16)(uint16)((uint16)(y + 10));
            poly->p[3].xy.y = (sint16)(uint16)((uint16)(y + 10));
            cur_frame = disp_state.cur;
            left = center + half_width - ((uint32)quotient - 20);
            right = left + (uint32)quotient;
            poly->p[0].xy.x = (sint16)(uint16)((uint16)left);
            poly->p[1].xy.x = (sint16)(uint16)((uint16)right);
            poly->p[2].xy.x = (sint16)(uint16)((uint16)left);
            poly->p[3].xy.x = (sint16)(uint16)((uint16)right);
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = 0;
            tag = poly->tag;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            poly->tag = (tag & 0xFF000000) | (head & 0xFFFFFF);
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = 0;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            half_width += 20;
            hd_ui_gradient(poly->p, 0);
            head_address.tags[(sint32)head_address.byte_offset / 4] = (head & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
        }
        if (idx == 4 && race_control.pause_camera_mode == 0)
        {
            selection = race_control.pause_selection;
            text = pause_menu_labels[5];
        }
        else
        {
            selection = race_control.pause_selection;
            text = pause_menu_labels[idx];
        }
        hud_draw_race_text(text, center - half_width, y, selection == idx);
        y += 12;
    }
    if ((*pad & 0x10) != 0)
    {
        val = race_control.pause_selection;
        if ((sint32)val > 0)
        {
            race_control.pause_selection = (uint32)(val - 1);
            audio_play_menu_move();
        }
    }
    if ((*pad & 0x40) != 0 && (sint32)race_control.pause_selection < (sint32)(rows - 1))
    {
        audio_play_menu_move();
        race_control.pause_selection = (uint32)(race_control.pause_selection + 1);
    }
    if ((*pad & 0x80) != 0 && race_control.pause_selection == 1 && (sint32)audio_options.effects_volume > 0)
    {
        audio_play_menu_move();
        audio_options.effects_volume = (audio_options.effects_volume - 512);
    }
    if ((*pad & 0x20) != 0 && race_control.pause_selection == 1 && (sint32)audio_options.effects_volume < 4096)
    {
        audio_play_menu_move();
        audio_options.effects_volume = (audio_options.effects_volume + 512);
    }
    if ((*pad & 0x80) != 0 && race_control.pause_selection == 2 && (sint32)audio_options.music_volume > 0)
    {
        audio_play_menu_move();
        audio_options.music_volume = (audio_options.music_volume - 512);
    }
    if ((*pad & 0x20) != 0 && race_control.pause_selection == 2 && (sint32)audio_options.music_volume < 4096)
    {
        audio_play_menu_move();
        audio_options.music_volume = (audio_options.music_volume + 512);
    }
    if ((*pad & 0xA0) != 0 && race_control.pause_selection == 4)
    {
        audio_play_menu_move();
        race_control.pause_camera_mode = (uint32)(race_control.pause_camera_mode ^ 1);
    }
    volume = (sint32)audio_options.music_volume;
    val = volume < 0 ? 0 : volume < 4097 ? (uint32)volume : 4096;
    volume = (sint32)audio_options.effects_volume;
    audio_options.music_volume = (val);
    result = volume < 4097;
    val = volume < 0 ? 0 : volume < 4097 ? (uint32)volume : 4096;
    audio_options.effects_volume = (val);
    return result;
}

static uint32 update_pause(uint64 *previous_field)
{
    uint32 result;
    uint32 player;
    uint16 *pad;
    uint32 width;
    uint32 height;
    uint32 flip;
    uint32 val;

    uint32 restore;
    PSX_RECT rect;
    DISPLAY_CLOCK pause_clock;

    FUNCTION_MARKER(0x8005A3D0, "SLUS_006.10");
    result = session_flow.attract_active;
    rect = pause_scr_rect;
    if (result != 0)
        return result;
    result = race_control.pause_request_player;
    player = result - 1;
    if (result == 0)
    {
        result = input_ports[0].pressed & 8;
        player = 0;
        if (result == 0)
        {
            result = race_session.two_players;
            if (result == 0)
                return result;
            result = input_ports[1].pressed & 8;
            player = 1;
            if (result == 0)
                return result;
        }
    }
    race_control.pause_request_player = (uint32)(0);
    result = allocate_pause_resources();
    if (result == 1)
        return result;
    hd_pause_begin();
    restore = race_view.split_mode;
    race_control.pause_confirmation = (uint32)(0);
    race_control.pause_camera_mode = (uint32)(restore);
    input_set_motors(0, 0, 0);
    input_set_motors(1, 0, 0);
    width = model_render.width;
    flip = disp_state.bank;
    race_control.pause_selection = (uint32)(0);
    race_control.paused = (uint32)(1);
    rect.w = (sint16)width;
    rect.y = (sint16)((flip ^ 1) << 8);
    MoveImage(&rect, 0, (sint32)(flip << 8));
    audio_mute_voices();
    audio_pause_xa();
    flip = disp_state.pal;
    width = model_render.width;
    height = model_render.height;
    disp_state.frames[0].draw.clip.x = (sint16)(0);
    disp_state.frames[1].draw.clip.x = (sint16)(0);
    val = (flip ^ 1) << 3;
    disp_state.frames[0].draw.clip.y = (sint16)(val);
    val += 256;
    disp_state.frames[0].draw.clip.w = (sint16)(width);
    disp_state.frames[0].draw.clip.h = (sint16)(height);
    disp_state.frames[1].draw.clip.y = (sint16)(val);
    disp_state.frames[1].draw.clip.w = (sint16)(width);
    disp_state.frames[1].draw.clip.h = (sint16)(height);
    capture_pause_background();
    if (race_control.paused == 1)
    {
        pad = &input_ports[player].pressed;
        disp_clock_init(&pause_clock, 2, 0);
        do
        {
            if (!disp_clock_step(&pause_clock, 0))
                continue;
            hd_begin_frame(1);
            input_poll();
            disp_swap();
            hd_menu_view();
            disp_clear_ordering_table();
            draw_pause_background();
            hud_reset_race_text();
            draw_pause_menu(player);
            if (race_control.pause_confirmation == 1)
            {
                result = *pad & 0x4008;
                if (result != 0)
                {
                    val = race_control.pause_selection;
                    result = 3;
                    if (val == 0)
                    {
                        race_control.stop_requested = (uint32)(1);
                        race_control.paused = (uint32)(0);
                        race_control.exit_reason = (uint32)(1);
                    }
                    else
                    {
                        race_control.pause_confirmation = (uint32)(0);
                        race_control.pause_selection = (uint32)(3);
                    }
                }
            }
            else
            {
                result = *pad & 0x4008;
                if (result != 0)
                {
                    if (race_control.pause_selection == 0)
                    {
                        audio_play_menu_accept();
                        race_control.paused = (uint32)(0);
                    }
                    val = *pad & 0x4008;
                    result = 3;
                    if (val != 0 && race_control.pause_selection == 3)
                    {
                        result = (uint32)audio_play_menu_accept();
                        race_control.pause_confirmation = (uint32)(1);
                        race_control.pause_selection = (uint32)(1);
                    }
                }
            }
            hud_link_race_text(result);
            if (race_session.two_players == 0 && race_session.linear_course == 0)
                stream_update();
            disp_apply();
            restore_pause_background();
            disp_draw_ordering_table();
            hd_end_frame();
            if ((*pad & 8) != 0)
                race_control.paused = (uint32)(0);
        } while (race_control.paused == 1);
    }
    hd_pause_end();
    *previous_field = xport_frame();
    DrawSync(0);
    audio_restore_voices();
    audio_resume_xa();
    restore = race_control.pause_camera_mode;
    result = race_session.two_players;
    input_ports[0].pressed = (0);
    input_ports[1].pressed = (0);
    input_ports[0].held = (0);
    input_ports[1].held = (0);
    race_view.split_mode = (uint32)(restore);
    if (result != 0)
        return set_split_mode(restore);
    return result;
}

static uint32 ai_route_speed(uint32 slot)
{
    uint32 car_offset;
    const RACE_AI_ROUTE *route;
    uint32 tag;
    uint32 sum;
    uint32 s_term;
    uint32 c_term;
    sint32 lane;
    sint32 angle;
    sint32 s;
    sint32 c;
    sint32 len;
    sint32 coord;

    FUNCTION_MARKER(0x8004C9EC, "SLUS_006.10");
    car_offset = slot * 864;
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    route = race_ai_cars[slot].route;
    XASSERT(!(route == NULL || (uint32)lane >= route->count));
    tag = route->samples[(uint32)lane].heading;
    angle = (sint32)(tag * 4140) / 256;
    s = rsin(angle);
    c = rcos(angle);
    sum = (uint32)s * (uint32)s + (uint32)c * (uint32)c;
    len = SquareRoot0((sint32)sum);
    coord = car_states[(car_offset) / 864u].velocity.x >> 8;
    s_term = (uint32)coord * (uint32)s;
    coord = car_states[(car_offset) / 864u].velocity.z >> 8;
    c_term = (uint32)coord * (uint32)c;
    return (uint32)fixed_divide_checked((sint32)(s_term + c_term), len);
}

static uint32 ai_route_lateral_offset(uint32 car_offset, uint32 slot, const RACE_AI_ROUTE *route)
{
    sint32 lane;
    sint32 node;
    uint32 offset;
    uint32 tag;

    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    node = (sint16)car_states[(car_offset) / 864u].track_location.node;
    offset = race_ai_cars[slot].lane_parameter;
    XASSERT(!(route == NULL || (uint32)lane >= route->count));
    tag = route->samples[(uint32)lane].lateral;
    return track_lateral_distance(node, offset, tag);
}

static void sync_police_route(void)
{
    uint32 sel;
    const RACE_AI_ROUTE *prev;
    const RACE_AI_ROUTE *route;

    FUNCTION_MARKER(0x8004D918, "SLUS_006.10");
    sel = race_ai_cars[9].chase_target;
    if (sel == 0xFFFFFFFF)
        return;
    prev = race_ai_cars[9].route;
    route = race_ai_cars[sel].route;
    if (prev == route)
        return;
    race_ai_cars[9].route = route;
    race_refresh_ai_route_offset(9);
}

static void update_police_chase(void)
{
    uint32 active;
    uint32 sel;
    uint32 timer;
    uint32 val;
    uint32 variant;
    uint32 expected;
    uint32 mode;
    sint32 lane;
    sint32 target_lane;

    FUNCTION_MARKER(0x8004E5B0, "SLUS_006.10");
    sync_police_route();
    active = race_ai_cars[9].recovery_state;
    sel = race_ai_cars[9].chase_target;
    if (active == 0 && sel == 0xFFFFFFFF)
    {
        timer = race_ai_cars[9].program_argument;
        if (timer != 0)
            return;
        lane = (sint16)car_states[0].track_location.n_node;
        target_lane = (sint16)car_states[9].track_location.n_node;
        if (target_lane != lane - 2)
            return;
        if ((sint32)car_states[0].forward_speed <= 87608)
            return;
        val = race_ai_cars[0].route_speed;
        if ((sint32)val <= 0)
            return;
        variant = race_ai_cars[0].route_variant;
        expected = race_ai_cars[9].route_variant;
        if (variant != expected)
            return;
        race_ai_cars[9].chase_target = 0;
        race_refresh_ai_route_offset(9);
        race_ai_cars[9].route = race_ai_cars[0].route;
        car_reset_grounding(&car_states[9]);
        mode = race_session.cop_chase;
        if (mode != 0)
            return;
        audio_bind_engine_src(9);
        {
            audio_set_engine_volume(4096);
            return;
        }
    }
    if (sel == 0xFFFFFFFF)
        return;
    timer = race_ai_cars[9].program_argument;
    if (timer != 0)
        return;
    sel = race_ai_cars[9].chase_target;
    lane = (sint16)car_states[(sel * 864) / 864u].track_location.n_node;
    target_lane = (sint16)car_states[9].track_location.n_node;
    if (lane - target_lane >= 65 && car_states[9].motion_mode == 0)
    {
        mode = race_session.cop_chase;
        race_ai_cars[9].chase_target = 0xFFFFFFFF;
        race_ai_cars[9].reverse = 0;
        race_ai_cars[9].recovery_state = 0;
        race_ai_cars[9].program_argument = 300;
        if (mode == 0)
            audio_set_engine_volume(0);
        {
            car_reset_grounding(&car_states[9]);
            return;
        }
    }
    sel = race_ai_cars[9].chase_target;
    target_lane = (sint16)car_states[9].track_location.n_node;
    lane = (sint16)car_states[(sel * 864) / 864u].track_location.n_node;
    if (target_lane - lane < 2)
        return;
    variant = race_ai_cars[sel].route_variant;
    expected = race_ai_cars[9].route_variant;
    if (variant != expected)
        return;
    if (car_states[9].motion_mode != 0)
        return;
    mode = race_session.cop_chase;
    race_ai_cars[sel].chase_brake = 1;
    if (mode != 0)
        return;
    {
        audio_set_engine_volume(0);
        return;
    }
}

static void update_ai_and_motion(void)
{
    uint32 idx;
    uint32 count;
    uint32 car_offset;
    uint32 val;
    const RACE_AI_ROUTE *route;

    uint32 tag;
    const RACE_AI_ROUTE *saved;
    uint32 copy;
    sint32 node;
    sint32 lane;
    sint32 threshold;
    sint32 edge;

    FUNCTION_MARKER(0x8004FFE4, "SLUS_006.10");
    update_ai_gap_scale();
    count = race_control.car_count;
    idx = 0;
    if ((sint32)count > 0)
    {
        do
        {
            car_offset = idx * 864;
            node = (sint16)car_states[(car_offset) / 864u].track_location.node;
            threshold = (sint32)track_geometry.main_node_count;
            if (threshold < node)
            {
                route = &race_ai.routes[1];
                race_ai_cars[idx].route_variant = 1;
                race_ai_cars[idx].route = route;
            }
            else if (track_map_branch_node((sint16)car_states[(car_offset) / 864u].track_location.n_node) == 0xFFFFFFFF)
                race_ai_cars[idx].route_variant = 0;
            else
            {
                route = &race_ai.routes[0];
                race_ai_cars[idx].route_variant = 0;
                race_ai_cars[idx].route = route;
            }
            val = ai_route_speed(idx);
            race_ai_cars[idx].route_speed = val;
            val = track_lateral_fraction((sint16)car_states[(car_offset) / 864u].track_location.node, &car_states[(car_offset) / 864u].pos);
            race_ai_cars[idx].lane_parameter = val;
            update_ai_seg(&race_ai_cars[idx]);
            update_ai_intervals(idx);
            val = classify_ai_route_offset(idx, race_ai_cars[idx].route_offset);
            if (val == 1)
                edge = 1;
            else
            {
                val = classify_ai_route_offset(idx, race_ai_cars[idx].route_offset);
                edge = val == 2 ? 2 : 0;
            }
            if (edge != 0)
            {
                lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
                route = race_ai_cars[idx].route;
                node = (sint16)car_states[(car_offset) / 864u].track_location.node;
                XASSERT(!(route == NULL || (uint32)lane >= route->count));
                tag = route->samples[(uint32)lane].lateral;
                val = track_lateral_distance(node, edge == 1 ? 256 : 0, tag);

                if (edge == 1)
                    val += (uint32)car_body_points[car_offset / 864u][0].vx - 32;
                else
                    val += (uint32)car_body_points[car_offset / 864u][1].vx + 32;
                race_ai_cars[idx].route_offset = val;
            }
            node = (sint16)car_states[(car_offset) / 864u].track_location.node;
            tag = car_states[(car_offset) / 864u].track_location.lane;
            if (track_lane_material(node, tag) >= 16)
                race_ai_cars[idx].route_offset = 0;
            tag = car_states[car_offset / 864u].boundary_side;
            if (tag == 1 || tag == 2)
                race_ai_cars[idx].route_offset = 0;
            route = race_ai_cars[idx].route;
            saved = &race_ai.routes[0];
            if (route == saved)
            {
                if ((sint32)idx >= 6 && (idx != 9 || race_ai_cars[9].chase_target == 0xFFFFFFFF))
                {
                    val = ai_route_lateral_offset(car_offset, idx, &race_ai.routes[0]);
                    race_ai_cars[idx].route_offset = val;
                    race_ai_cars[idx].route_offsets[0] = val;
                }
                else
                    race_ai_cars[idx].route_offsets[0] = race_ai_cars[idx].route_offset;
                val = ai_route_lateral_offset(car_offset, idx, &race_ai.routes[1]);
                copy = race_ai_cars[idx].route_bounds[0];
                race_ai_cars[idx].route_offsets[1] = val;
                race_ai_cars[idx].interval_lower = copy;
                copy = race_ai_cars[idx].route_bounds[1];
                race_ai_cars[idx].interval_upper = copy;
            }
            else
            {
                if ((sint32)idx >= 6 && (idx != 9 || race_ai_cars[9].chase_target == 0xFFFFFFFF))
                {
                    val = ai_route_lateral_offset(car_offset, idx, &race_ai.routes[1]);
                    race_ai_cars[idx].route_offset = val;
                }
                else
                    val = race_ai_cars[idx].route_offset;
                race_ai_cars[idx].route_offsets[1] = val;
                val = ai_route_lateral_offset(car_offset, idx, &race_ai.routes[0]);
                race_ai_cars[idx].route_offsets[0] = val;
                copy = race_ai_cars[idx].route_bounds[2];
                race_ai_cars[idx].interval_lower = copy;
                copy = race_ai_cars[idx].route_bounds[3];
                race_ai_cars[idx].interval_upper = copy;
            }
            count = race_control.car_count;
            ++idx;
        } while ((sint32)idx < (sint32)count);
    }
    if ((race_control.stopped_players & 1) != 0)
    {
        update_ai_car(0);
        car_update_motion(0);
    }
    if (race_session.two_players != 0 && (race_control.stopped_players & 2) != 0)
    {
        update_ai_car(1);
        car_update_motion(1);
    }
    if (race_session.time_trial == 0)
    {
        idx = race_session.two_players;
        count = race_control.car_count;
        ++idx;
        while ((sint32)count >= 7 ? (sint32)idx < 6 : (sint32)idx < (sint32)count)
        {
            if (race_session.cop_chase == 0 || (sint16)chase_state.health[idx] != 0)
                update_ai_car(idx);
            car_update_motion(idx);
            count = race_control.car_count;
            ++idx;
        }
        (void)race_session.time_trial;
    }
    count = race_control.car_count;
    if ((sint32)count < 7)
        return;
    idx = 6;
    while ((sint32)count >= 13 ? (sint32)idx < 12 : (sint32)idx < (sint32)count)
    {
        if (idx == 9 && race_ai_cars[9].chase_target != 0xFFFFFFFF)
        {
            update_ai_car(9);
            car_update_motion(9);
        }
        else
        {
            update_traffic_car(idx);
            car_update_boundary_motion(idx);
        }
        count = race_control.car_count;
        ++idx;
    }
    if (race_rules.cops == 1)
    {
        update_police_chase();
        return;
    }
    return;
}

uint32 race_run_single(void)
{
    uint32 wire_word_1;
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 idx;
    uint32 val;

    uint32 result;
    DISPLAY_CLOCK clock;
    uint32 frames = 0;
    uint32 absent_frames = 0;
    uint32 elapsed = 0;
    uint8 color[4];
    sint32 node;
    sint32 lane;
    sint32 pos;

    FUNCTION_MARKER(0x80013F90, "SLUS_006.10");
    race_control.exit_reason = (uint32)(0);
    if (primary_view_gate == 0)
        race_view.primary_car = (uint32)(0);
    race_view.primary_car = (uint32)(0);
    val = race_view.primary_car;
    race_view.tracked_car = (uint32)(val);
    race_control.pause_request_player = (uint32)(0);
    race_control.stopped_players = (uint32)(0);
    race_control.stop_requested = (uint32)(0);
    race_control.frame_counter = (uint32)(0);
    race_control.update_cycles = 0;
    race_control.primary_timer_snapshot = 1;
    race_control.stream_wait_marker = 0;
    race_control.attract_cycles = 0;
    val = race_view.tracked_car;
    audio_set_listener_pos(&car_states[(val * 864) / 864u].pos);
    val = race_view.tracked_car;
    audio_set_listener_velocity(&car_states[(val * 864) / 864u].velocity);
    disp_clock_init(&clock, 2, 0);
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    if ((level_header[30] & 1) != 0)
    {
        val = single_lighting_color;
        (wire_word_1 = (uint32)(val), memcpy(color, &wire_word_1, sizeof(wire_word_1)));
        level_desc = &scene_level_image.desc;
        level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
        model_init_lighting(5200, (uint8 *)(level_header + 26), color, 650);
        model_set_reference_offset(-300, 5000);
    }
    update_pos();
    val = model_render.width;
    race_view.viewport_width = (uint32)(val);
    val = model_render.height;
    race_view.viewport_height = (uint32)(val);
    race_view.viewport_x = (uint32)(0);
    race_view.viewport_y = (uint32)(0);
    while (race_control.stop_requested == 0)
    {
        if (!disp_clock_step(&clock, 0))
            continue;
        hd_begin_race_frame(1);
        if (session_flow.attract_active != 0)
            race_control.stopped_players = (uint32)(3);
        race_view.active_car = (uint32)(0);
        if (session_flow.attract_active != 0)
        {
            val = race_control.attract_cycles;
            race_control.attract_cycles = val + 1;
            if ((sint32)race_control.attract_cycles >= 901)
                race_control.stop_requested = (uint32)(1);
            if ((sint16)input_ports[0].held != 0)
                race_control.stop_requested = (uint32)(1);
            if ((sint16)input_ports[1].held != 0)
                race_control.stop_requested = (uint32)(1);
        }
        input_poll();
        input_pack_controls(0);
        if (session_flow.attract_active == 0)
        {
            if ((input_ports[0].type & 255) != 255 || session_flow.attract_active != 0)
                absent_frames = 0;
            else
            {
                ++absent_frames;
                if ((sint32)absent_frames >= 31)
                {
                    absent_frames = 0;
                    race_control.pause_request_player = (uint32)(1);
                }
            }
        }
        hd_capture_pause(elapsed >= 8 && (race_control.pause_request_player != 0 || (input_ports[0].pressed & 8) != 0));
        disp_swap();
        hd_race_view(0, &disp_state.cur->draw);
        disp_clear_ordering_table();
        model_packet_pool_reset();
        level_desc = &scene_level_image.desc;
        level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
        if ((level_header[30] & 1) != 0)
        {
            pos = (sint32)car_states[0].steered_speed / 256;
            model_update_lod_distances(pos, 1500, 1500);
        }
        else
        {
            pos = (sint32)car_states[0].steered_speed / 256;
            model_update_lod_distances(pos, 2000, 3000);
        }
        if (race_control.stopped_players == 0)
        {
            car_update_player_input(0, 0);
            car_update_motion(0);
        }
        car_update_police_light_phases();
        for (idx = 0; (sint32)idx < (sint32)race_control.car_count; ++idx)
            track_wrap_location(&car_states[(idx * 864) / 864u].track_location);
        if (race_session.cop_chase != 0)
            chase_decay_signal();
        level_desc = &scene_level_image.desc;
        level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
        if ((level_header[30] & 4) != 0)
            update_ai_and_motion();
        audio_update_engines();
        audio_update_spatial();
        car_update_pair_cols();
        update_pos();
        val = race_view.tracked_car;
        camera_update(&car_states[val], 0);
        hd_capture_race_poses();
        val = race_view.tracked_car;
        hud_draw_full_view(&car_states[val]);
        input_update_rumble(0, 0);
        if (car_states[0].wheel_air_mask != 0 || race_session.demo != 0 || session_flow.attract_active != 0)
            sky_draw(-1);
        else
            sky_draw(0);
        if (race_session.drag_race != 0)
            hud_draw_drag_start();
        hud_update_countdown();
        effect_update_all();
        effect_draw_all();
        effect_update_wheel_skids();
        effect_draw_wheel_skids();
        val = race_view.tracked_car;
        effect_update_weather(&car_states[val]);
        if (race_session.linear_course != 0)
        {
            val = race_view.tracked_car;
            node = (sint16)car_states[(val * 864) / 864u].track_location.n_node;
            track_stream_update_linear(0, node);
        }
        else
        {
            val = race_view.tracked_car;
            node = (sint16)car_states[(val * 864) / 864u].track_location.n_node;
            val = race_view.tracked_car;
            lane = (sint16)car_states[(val * 864) / 864u].track_location.node;
            track_stream_update(node, lane);
        }
        if (race_session.linear_course != 0)
            track_stream_draw_linear(0);
        else
            track_stream_draw();
        if (race_control.timing_hold == 0)
        {
            val = race_control.frame_counter;
            if (val != 0xFFFFFFFF)
            {
                val = race_control.frame_counter;
                race_control.frame_counter = (uint32)(val + 1);
            }
        }
        val = race_control.update_cycles;
        race_control.update_cycles = val + 1;
        car_draw_scene();
        DrawSync(0);
        if (race_session.linear_course == 0)
            track_stream_upload_images();
        stream_update();
        timing_reset_counter(1);
        audio_poll_xa();
        disp_apply();
        hd_native_begin(0, &disp_state.cur->draw);
        disp_draw_ordering_table();
        hd_native_end(0);
        ++frames;
        if ((sint32)elapsed <= 65534)
            ++elapsed;
        if ((sint32)elapsed >= 9)
            update_pause(&clock.field);
        if (elapsed == 2)
            audio_prepare_race_voices();
        hd_end_frame();
    }
    DrawSync(0);
    VSync(0);
    audio_key_off_all_voices();
    result = race_session.event_mode;
    if (result == 2)
    {
        result = race_control.exit_reason;
        if (result == 1)
        {
            result = race_session.demo;
            if (result == 0)
            {
                result = 170;
                session_flow.menu_state = (result);
                session_flow.phase = (0);
                session_flow.event_success = 0;
            }
        }
    }
    hd_suspend();
    return result;
}

void race_run_two_player(void)
{
    uint32 wire_word_2, wire_word_3;
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 absent_frames[2];
    uint32 val, mode, buffer, elapsed = 0;
    DISPLAY_CLOCK clock;
    uint32 idx, slot, frame_counter, update_cycles, count;
    CAR_STATE *car;
    uint8 color[4];
    sint32 pos;

    FUNCTION_MARKER(0x8004A3A0, "SLUS_006.10");
    absent_frames[0] = 0;
    absent_frames[1] = 0;
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    val = level_header[26];
    disp_state.frames[1].draw.r0 = (uint8)(val);
    disp_state.frames[0].draw.r0 = (uint8)(val);
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    val = level_header[27];
    disp_state.frames[1].draw.g0 = (uint8)(val);
    disp_state.frames[0].draw.g0 = (uint8)(val);
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    mode = race_view.split_mode;
    val = level_header[28];
    disp_state.frames[1].draw.b0 = (uint8)(val);
    disp_state.frames[0].draw.b0 = (uint8)(val);
    set_split_mode(mode);
    buffer = disp_state.bank;
    race_control.stream_wait_marker = 0xFFFFFFFF;
    race_control.timing_hold = 1;
    disp_clock_init(&clock, 2, 0);
    race_control.exit_reason = (uint32)(0);
    race_view.tracked_car = (uint32)(0);
    race_control.stop_requested = (uint32)(0);
    race_control.stopped_players = (uint32)(0);
    race_control.frame_counter = (uint32)(0);
    race_control.update_cycles = 0;
    camera_reset_follow(&car_states[0].angles, 1, 0);
    camera_reset_follow(&car_states[1].angles, 1, 1);
    slot = race_view.tracked_car;
    audio_set_listener_pos(&car_states[(slot * 864) / 864u].pos);
    slot = race_view.tracked_car;
    audio_set_listener_velocity(&car_states[(slot * 864) / 864u].velocity);
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    if ((level_header[30] & 1) != 0)
    {
        level_desc = &scene_level_image.desc;
        val = split_lighting_color;
        val = split_lighting_color;
        (wire_word_2 = (uint32)(val), memcpy(color, &wire_word_2, sizeof(wire_word_2)));
        (wire_word_3 = (uint32)(val), memcpy(color, &wire_word_3, sizeof(wire_word_3)));
        level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
        model_init_lighting(5200, (uint8 *)(level_header + 26), color, 650);
        model_set_reference_offset(-300, 5000);
    }
    while (race_control.stop_requested == 0)
    {
        if (!disp_clock_step(&clock, 0))
            continue;
        hd_begin_race_frame(2);
        input_poll();
        race_view.active_car = (uint32)(0);
        input_pack_controls(0);
        race_view.active_car = (uint32)(1);
        input_pack_controls(1);
        for (idx = 0; idx < 2; ++idx)
        {
            if ((uint8)input_ports[idx].type == 255)
            {
                val = absent_frames[idx] + 1;
                absent_frames[idx] = val;
                if ((sint32)val >= 31)
                {
                    absent_frames[idx] = 0;
                    race_control.pause_request_player = (uint32)(idx + 1);
                }
            }
            else
                absent_frames[idx] = 0;
        }
        hd_capture_pause(elapsed >= 8 && (race_control.pause_request_player != 0 || (input_ports[0].pressed & 8) != 0 || (input_ports[1].pressed & 8) != 0));
        session_update_catchup();
        val = race_control.stopped_players;
        race_view.active_car = (uint32)(0);
        if ((val & 1) == 0)
            car_update_player_input(0, 0);
        val = race_control.stopped_players;
        race_view.active_car = (uint32)(1);
        if ((val & 2) == 0)
            car_update_player_input(1, 1);
        val = race_control.stopped_players;
        race_view.active_car = (uint32)(0);
        if ((val & 1) == 0)
            car_update_motion(0);
        val = race_control.stopped_players;
        race_view.active_car = (uint32)(1);
        if ((val & 2) == 0)
            car_update_motion(1);
        buffer ^= 1;
        chase_decay_signal();
        car_update_police_light_phases();
        update_pos();
        idx = 0;
        count = race_control.car_count;
        if ((sint32)count > 0)
        {
            car = &car_states[0];
            do
            {
                track_wrap_location(&car->track_location);
                count = race_control.car_count;
                ++idx;
                ++car;
            } while ((sint32)idx < (sint32)count);
        }
        level_desc = &scene_level_image.desc;
        level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
        if ((level_header[30] & 4) != 0 && race_control.start_hold == 0)
            update_ai_and_motion();
        race_view.active_car = (uint32)(0);
        select_primary_view(buffer);
        disp_swap();
        hd_race_view(0, &disp_state.frames[buffer].draw);
        disp_clear_ordering_table();
        model_packet_pool_reset();
        slot = race_view.active_car;
        pos = (sint32)car_states[slot].steered_speed / 256;
        model_update_lod_distances(pos, 1500, 3000);
        slot = race_view.active_car;
        camera_update(&car_states[slot], slot);
        hd_capture_race_poses();
        hud_update_countdown();
        sky_draw((sint32)race_view.active_car);
        effect_draw_wheel_skids();
        slot = race_view.active_car;
        hud_draw_split_view(&car_states[slot], slot);
        input_update_rumble(0, 0);
        effect_draw_all();
        slot = race_view.active_car;
        track_stream_update_linear(0, (sint16)car_states[(slot * 864) / 864u].track_location.n_node);
        track_stream_draw_linear(race_view.active_car);
        car_draw_split_model(race_view.active_car, 0);
        car_draw_split_model(race_view.active_car ^ 1, 1);
        idx = 2;
        count = race_control.car_count;
        while ((sint32)idx < (sint32)count)
        {
            car_draw_racing_model(idx);
            count = race_control.car_count;
            ++idx;
        }
        if (race_session.drag_race != 0)
            hud_draw_drag_start();
        val = (uint32)timing_read_counter(1, 0);
        disp_state.post_sync_ticks = val;
        disp_state.pre_sync_ticks = val;
        timing_reset_counter(1);
        frame_counter = race_control.frame_counter;
        update_cycles = race_control.update_cycles;
        race_control.frame_counter = (uint32)(frame_counter + 1);
        race_control.update_cycles = update_cycles + 1;
        DrawSync(0);
        disp_apply_frame(buffer);
        hd_native_begin(0, &disp_state.frames[buffer].draw);
        disp_draw_ordering_table();
        hd_native_end(0);
        audio_update_split_engines(0);
        car_update_pair_cols();
        effect_update_wheel_skids();
        effect_update_all();
        race_view.active_car = (uint32)(1);
        select_secondary_view(buffer);
        disp_swap();
        hd_race_view(1, &disp_state.frames[buffer].draw);
        disp_clear_ordering_table();
        model_packet_pool_reset();
        slot = race_view.active_car;
        pos = (sint32)car_states[slot].steered_speed / 256;
        model_update_lod_distances(pos, 1500, 3000);
        slot = race_view.active_car;
        camera_update(&car_states[slot], slot);
        hd_capture_race_poses();
        hud_update_countdown();
        sky_draw((sint32)race_view.active_car);
        effect_draw_wheel_skids();
        slot = race_view.active_car;
        hud_draw_split_view(&car_states[slot], slot);
        input_update_rumble(1, 1);
        effect_draw_all();
        slot = race_view.active_car;
        track_stream_update_linear(1, (sint16)car_states[(slot * 864) / 864u].track_location.n_node);
        track_stream_draw_linear(race_view.active_car);
        car_draw_split_model(race_view.active_car ^ 1, 1);
        car_draw_split_model(race_view.active_car, 0);
        idx = 2;
        count = race_control.car_count;
        while ((sint32)idx < (sint32)count)
        {
            car_draw_racing_model(idx);
            count = race_control.car_count;
            ++idx;
        }
        if (race_session.drag_race != 0)
            hud_draw_drag_start();
        audio_poll_xa();
        DrawSync(0);
        disp_apply_frame(buffer);
        hd_native_begin(1, &disp_state.frames[buffer].draw);
        disp_draw_ordering_table();
        hd_native_end(1);
        audio_update_split_engines(1);
        audio_update_spatial();
        if ((sint32)elapsed <= 65534)
            ++elapsed;
        if ((sint32)elapsed >= 9)
            update_pause(&clock.field);
        hd_end_frame();
    }
    DrawSync(0);
    VSync(0);
    audio_key_off_all_voices();
    hd_suspend();
}

static uint32 set_split_mode(uint32 mode)
{
    PSX_RECT rect;
    uint32 dimension, buffer, result;

    FUNCTION_MARKER(0x80049DEC, "SLUS_006.10");
    race_view.split_mode = (uint32)(mode);
    if (mode == 0)
    {
        dimension = model_render.height;
        rect.w = 512;
        rect.h = 1;
        buffer = disp_state.pal;
        rect.x = 0;
        rect.y = (sint16)((uint32)((sint32)dimension / 2) + ((buffer ^ 1) << 3));
        ClearImage(&rect, 0, 0, 0);
        dimension = model_render.height;
        buffer = disp_state.pal;
        rect.y = (sint16)((uint32)((sint32)dimension / 2) + ((buffer ^ 1) << 3) + 256);
        ClearImage(&rect, 0, 0, 0);
        dimension = model_render.height;
        model_render.height = ((uint32)((sint32)dimension / 2));
        model_update_clip_planes();
        result = model_render.height * 2;
        model_render.height = (result);
    }
    else
    {
        dimension = model_render.width;
        rect.w = 2;
        rect.h = 512;
        rect.y = 0;
        rect.x = (sint16)((uint32)((sint32)dimension / 2) - 1);
        ClearImage(&rect, 0, 0, 0);
        dimension = model_render.width;
        model_render.width = ((uint32)((sint32)dimension / 2));
        model_update_clip_planes();
        result = model_render.width * 2;
        model_render.width = (result);
    }
    return result;
}

static uint32 select_primary_view(uint32 slot)
{
    uint32 mode, width, height, buffer, offset, result, scr_offset;
    sint32 signed_width, signed_height;

    FUNCTION_MARKER(0x80049F70, "SLUS_006.10");
    mode = race_view.split_mode;
    race_view.viewport = (uint32)(0);
    if (mode == 0)
    {
        width = model_render.width;
        height = model_render.height;
        model_set_view_center((sint32)width / 2, (sint32)height / 4);
        height = model_render.height;
        buffer = disp_state.pal;
        offset = slot * 116;
        disp_state.frames[slot].draw.clip.x = (sint16)(0);
        race_view.viewport_x = (uint32)(0);
        race_view.viewport_y = (uint32)(0);
        disp_state.frames[slot].draw.clip.h = (sint16)((uint32)((sint32)height / 2));
        width = model_render.width;
        signed_height = (sint16)disp_state.frames[slot].draw.clip.h;
        scr_offset = (buffer ^ 1) << 3;
        disp_state.frames[slot].draw.clip.w = (sint16)(width);
        signed_width = (sint16)width;
        disp_state.frames[slot].draw.clip.y = (sint16)((slot << 8) + scr_offset);
        race_view.viewport_width = (uint32)((uint32)signed_width);
        race_view.viewport_height = (uint32)((uint32)signed_height);
        result = offset;
    }
    else
    {
        width = model_render.width;
        height = model_render.height;
        model_set_view_center((sint32)width / 4, (sint32)height / 2);
        offset = slot * 116;
        width = model_render.width;
        buffer = disp_state.pal;
        height = model_render.height;
        disp_state.frames[slot].draw.clip.w = (sint16)((uint32)((sint32)width / 2) - 1);
        signed_width = (sint16)disp_state.frames[slot].draw.clip.w;
        scr_offset = (buffer ^ 1) << 3;
        disp_state.frames[slot].draw.clip.h = (sint16)(height + scr_offset);
        signed_height = (sint16)disp_state.frames[slot].draw.clip.h;
        disp_state.frames[slot].draw.clip.x = (sint16)(0);
        race_view.viewport_x = (uint32)(0);
        race_view.viewport_y = (uint32)(0);
        result = (slot << 8) + scr_offset;
        disp_state.frames[slot].draw.clip.y = (sint16)(result);
        race_view.viewport_width = (uint32)((uint32)signed_width);
        race_view.viewport_height = (uint32)((uint32)signed_height);
    }
    return result;
}

static uint32 select_secondary_view(uint32 slot)
{
    uint32 mode, width, height, buffer, offset, scr_offset, half, result;
    sint32 signed_width, signed_height;

    FUNCTION_MARKER(0x8004A170, "SLUS_006.10");
    mode = race_view.split_mode;
    race_view.viewport = (uint32)(1);
    if (mode == 0)
    {
        height = model_render.height;
        width = model_render.width;
        model_set_view_center((sint32)width / 2, (sint32)((uint32)((sint32)height / 4) + (uint32)((sint32)height / 2)));
        height = model_render.height;
        buffer = disp_state.pal;
        width = model_render.width;
        offset = slot * 116;
        disp_state.frames[slot].draw.clip.x = (sint16)(0);
        race_view.viewport_x = (uint32)(0);
        half = ((sint32)height / 2);
        disp_state.frames[slot].draw.clip.h = (sint16)(half - 1);
        signed_height = (sint16)disp_state.frames[slot].draw.clip.h;
        scr_offset = (buffer ^ 1) << 3;
        result = (slot << 8) + half + scr_offset + 1;
        disp_state.frames[slot].draw.clip.w = (sint16)(width);
        signed_width = (sint16)width;
        disp_state.frames[slot].draw.clip.y = (sint16)(result);
        race_view.viewport_width = (uint32)((uint32)signed_width);
        race_view.viewport_y = (uint32)(half);
    }
    else
    {
        width = model_render.width;
        height = model_render.height;
        model_set_view_center((sint32)((uint32)((sint32)width / 4) + (uint32)((sint32)width / 2)), (sint32)height / 2);
        width = model_render.width;
        buffer = disp_state.pal;
        height = model_render.height;
        offset = slot * 116;
        race_view.viewport_y = (uint32)(0);
        half = ((sint32)width / 2);
        scr_offset = (buffer ^ 1) << 3;
        disp_state.frames[slot].draw.clip.w = (sint16)(half - 1);
        disp_state.frames[slot].draw.clip.h = (sint16)(height + scr_offset);
        signed_width = (sint16)disp_state.frames[slot].draw.clip.w;
        signed_height = (sint16)disp_state.frames[slot].draw.clip.h;
        ++half;
        result = (slot << 8) + scr_offset;
        disp_state.frames[slot].draw.clip.x = (sint16)(half);
        disp_state.frames[slot].draw.clip.y = (sint16)(result);
        race_view.viewport_x = (uint32)(half);
        race_view.viewport_width = (uint32)((uint32)signed_width);
    }
    race_view.viewport_height = (uint32)((uint32)signed_height);
    return result;
}

static uint32 configure_car_count(void)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 val = 6;

    FUNCTION_MARKER(0x8004B004, "SLUS_006.10");
    if (race_rules.traffic != 0)
    {
        level_desc = &scene_level_image.desc;
        level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
        if ((level_header[30] & 0x20) != 0)
            val = 12;
    }
    race_control.car_count = (uint32)(val);
    if (race_session.linear_course != 0)
        race_control.car_count = (uint32)(6);
    if (race_session.two_players != 0)
        race_control.car_count = (uint32)(4);
    if (race_session.drag_race != 0)
        race_control.car_count = (uint32)(2);
    if (race_session.multiplayer_mode == 3)
        race_control.car_count = (uint32)(2);
    return 2;
}

uint32 race_configure_car_count(void)
{
    FUNCTION_MARKER(0x8004B0B0, "SLUS_006.10");
    return configure_car_count();
}

uint32 race_update_finish_wait(uint32 view)
{
    uint32 sel, val, count, rank;
    uint32 *counter;
    int ready;

    FUNCTION_MARKER(0x80047564, "SLUS_006.10");
    sel = race_view.active_car;
    counter = &race_control.players[sel].finish_frames;
    val = *counter;
    if ((sint32)val < 30000)
        *counter = (val + 1);
    if (race_session.two_players != 0 && race_session.multiplayer_mode != 3)
    {
        sel = race_view.active_car;
        count = race_control.players[sel].finish_frames;
        ready = 0;
        if ((sint32)count >= 61)
        {
            count = race_control.players[(sel ^ 1)].finish_frames;
            ready = (sint32)count >= 61;
        }
    }
    else
    {
        if (race_session.event_mode == 3)
        {
            if (car_states[0].finish_time == 0 || car_states[1].finish_time == 0)
            {
                sel = race_view.active_car;
                counter = &race_control.players[sel].finish_frames;
                val = *counter;
                *counter = (val - 1);
            }
        }
        sel = race_view.active_car;
        count = race_control.players[sel].finish_frames;
        ready = (sint32)count >= 61;
    }
    if (ready)
        race_control.stop_requested = (uint32)(1);
    sel = race_view.active_car;
    rank = car_states[sel].place;
    if (rank < 3 && race_session.drag_race == 0 && race_session.multiplayer_mode != 3)
        hud_draw_digit(view, (sint32)(rank + 1), 255, 255, 0);
    sel = race_view.active_car;
    camera_views[view].follow_height = 0x7D000;
    val = camera_views[view].look_back_angle;
    camera_views[view].look_back_angle = val - 8;
    return camera_follow(&car_states[sel], 1, view);
}

uint32 race_rank_times(void)
{
    uint32 idx;
    uint32 changes = 1;
    uint32 left;
    uint32 right;
    uint32 numerator;
    uint32 denominator;
    sint32 left_time;
    sint32 right_time;
    uint8 tmp;

    FUNCTION_MARKER(0x8001A764, "SLUS_006.10");
    for (idx = 0; idx < 6; ++idx)
        race_order[idx] = (uint8)((uint8)idx);
    while (changes != 0)
    {
        changes = 0;
        for (idx = 0; idx < 5; ++idx)
        {
            left = race_order[idx];
            numerator = race_results.entries[left].total_time * 100;
            denominator = ((sint32)disp_state.field_rate / 2);
            if (denominator == 0)
                xport_mips_break(7);
            left_time = (numerator / denominator);
            right = race_order[idx + 1];
            numerator = race_results.entries[right].total_time * 100;
            denominator = ((sint32)disp_state.field_rate / 2);
            if (denominator == 0)
                xport_mips_break(7);
            right_time = (numerator / denominator);
            if (right_time < left_time)
            {
                ++changes;
                tmp = race_order[idx];
                race_order[idx] = (uint8)(race_order[idx + 1]);
                race_order[idx + 1] = (uint8)(tmp);
            }
        }
    }
    for (idx = 0; idx < 6; ++idx)
        race_results.entries[race_order[idx]].place = (uint16)((uint16)idx);
    return 6;
}

uint32 race_rank_scores(void)
{
    uint32 idx;
    uint32 changes = 1;
    uint32 left;
    uint32 right;
    uint32 left_score;
    uint32 right_score;
    uint8 tmp;

    FUNCTION_MARKER(0x8001AAB4, "SLUS_006.10");
    for (idx = 0; idx < 6; ++idx)
        race_order[idx] = (uint8)((uint8)idx);
    while (changes != 0)
    {
        changes = 0;
        for (idx = 0; idx < 5; ++idx)
        {
            left = race_order[idx];
            right = race_order[idx + 1];
            left_score = race_results.entries[left].points.val;
            right_score = race_results.entries[right].points.val;
            if (left_score < right_score)
            {
                ++changes;
                tmp = race_order[idx];
                race_order[idx] = (uint8)(race_order[idx + 1]);
                race_order[idx + 1] = (uint8)(tmp);
            }
        }
    }
    for (idx = 0; idx < 6; ++idx)
        race_results.entries[race_order[idx]].place = (uint16)((uint16)idx);
    return 6;
}

uint32 race_reset_results(void)
{
    uint32 slot;
    uint32 offset;
    uint32 result;

    FUNCTION_MARKER(0x80018260, "SLUS_006.10");
    for (slot = 0; slot < 6; ++slot)
    {
        offset = slot * 20;
        race_results.entries[slot].model = (uint8)(session_car_choices.cars[slot].model[0]);
        race_results.entries[slot].player_tag = (uint8)(0);
        race_results.entries[slot].points.val = (uint32)(0);
        race_results.entries[slot].place = (uint16)(0);
        race_results.entries[slot].total_time = (uint32)(0);
        race_results.entries[slot].speed_sum = (uint16)(0);
        race_results.entries[slot].top_speed = (uint16)(0);
    }
    race_results.entries[0].player_tag = (uint8)(1);
    result = race_session.multiplayer_mode;
    if (result == 2)
        race_results.entries[1].player_tag = (uint8)(2);
    return result;
}

uint32 race_accumulate_results(void)
{
    uint32 maximum = 0;
    uint32 slot;
    uint32 rank;
    uint32 lap;
    uint32 val;
    uint32 other;
    uint32 remaining;
    uint32 jitter;
    sint32 random_val;
    sint32 denominator;
    sint32 quotient;

    FUNCTION_MARKER(0x80018408, "SLUS_006.10");
    for (slot = 0; slot < 6; ++slot)
    {
        if (car_states[slot].finish_time != 0)
        {
            if (maximum < car_states[slot].finish_time)
                maximum = car_states[slot].finish_time;
        }
    }
    for (slot = 0; slot < 6; ++slot)
    {
        for (rank = 0; rank < 6; ++rank)
        {
            if (slot == race_order[rank])
                break;
        }
        if (car_states[slot].finish_time == 0)
        {
            if (race_session.linear_course != 0 || race_session.two_players != 0)
            {
                random_val = randPSX();
                val = (sint16)car_states[0].track_location.max_path_count;
                other = (sint16)car_states[slot].track_location.max_path_count;
                car_states[slot].finish_time = (uint32)((val - other) * 2 + maximum + (uint32)(random_val % 32));
            }
            else
            {
                random_val = randPSX();
                val = car_states[slot].place;
                car_states[slot].finish_time = (uint32)(val * 30 + maximum + (uint32)(random_val % 29));
            }
            XASSERT(!(race_session.laps == 0 || race_session.laps > CAR_TIMING_HISTORY_COUNT));
            remaining = car_states[slot].finish_time;
            for (lap = 0; (sint32)lap < (sint32)race_session.laps; ++lap)
            {
                if (lap == race_session.laps - 1)
                    jitter = 0;
                else
                    jitter = 50 - (uint32)(randPSX() % 150);
                denominator = (sint32)(race_session.laps - lap);
                if (denominator == 0)
                    xport_mips_break(7);
                if (denominator == -1 && remaining == 0x80000000)
                    xport_mips_break(6);
                quotient = (sint32)remaining / denominator;
                car_states[slot].split_ticks[lap] = (uint16)((uint32)quotient + (uint16)jitter);
                remaining -= car_states[slot].split_ticks[lap];
            }
        }
        if (race_session.cup == 0 && race_session.event_mode == 2)
        {
            val = rank < 6 ? championship_place_points[rank] : 0;
            other = car_states[slot].points;
            car_states[slot].points = (uint32)(other + val);
        }
        if (race_session.cup == 4 && race_session.event_mode == 2)
        {
            val = rank < 4 ? (uint32)(sint32)bonus_championship_place_points[rank] : 0;
            other = car_states[slot].points;
            car_states[slot].points = (uint32)(other + val);
        }
        race_results.entries[slot].place = (uint16)((uint16)rank);
        val = race_results.entries[slot].total_time;
        other = car_states[slot].finish_time;
        race_results.entries[slot].total_time = (uint32)(val + other);
        val = race_results.entries[slot].points.val;
        other = car_states[slot].points;
        race_results.entries[slot].points.val = (uint32)(val + other);
        if (race_session.cop_chase != 0)
        {
            if (chase_state.pursuer_slot == slot)
                race_results.entries[slot].finish_data.bytes[0] = (uint8)((uint8)chase_state.arrests);
        }
        else
        {
            race_results.entries[slot].finish_data.bytes[0] = (uint8)(car_states[slot].chase_events);
        }
        val = race_results.entries[slot].speed_sum;
        other = car_states[slot].race_stats.avg_speed;
        race_results.entries[slot].speed_sum = (uint16)((uint16)(val + other));
        val = race_results.entries[slot].top_speed;
        if ((sint32)val < (sint16)car_states[slot].race_stats.top_speed)
            race_results.entries[slot].top_speed = (uint16)(car_states[slot].race_stats.top_speed);
    }
    return 6;
}

static uint32 update_pos(void)
{
    uint32 swaps;
    uint32 idx;
    uint32 left;
    uint32 right;
    uint32 finish_score0;
    uint32 finish_score1;
    sint32 left_pos;
    sint32 right_pos;

    FUNCTION_MARKER(0x80026130, "SLUS_006.10");
    do
    {
        swaps = 0;
        for (idx = 1; idx < 6; ++idx)
        {
            left = race_order[idx - 1];
            if (car_states[left].finish_time == 0)
            {
                right = race_order[idx];
                left_pos = (sint16)car_states[left].track_location.max_path_count;
                right_pos = (sint16)car_states[right].track_location.max_path_count;
                if (left_pos < right_pos)
                {
                    ++swaps;
                    race_order[idx - 1] = (uint8)(right);
                    race_order[idx] = (uint8)(left);
                }
            }
        }
    } while (swaps != 0);
    if (race_session.drag_race != 0)
    {
        finish_score0 = car_states[0].finish_time;
        if (finish_score0 != 0)
        {
            finish_score1 = car_states[1].finish_time;
            if (finish_score1 != 0)
            {
                finish_score0 = (finish_score0 << 8) - (uint32)(sint16)car_states[0].checkpoint_progress;
                finish_score1 = (finish_score1 << 8) - (uint32)(sint16)car_states[1].checkpoint_progress;
                if (finish_score0 < finish_score1)
                {
                    race_order[0] = (uint8)(0);
                    race_order[1] = (uint8)(1);
                    car_states[0].place = (uint8)0;
                    car_states[1].place = (uint8)1;
                }
                else
                {
                    race_order[0] = (uint8)(1);
                    race_order[1] = (uint8)(0);
                    car_states[0].place = (uint8)1;
                    car_states[1].place = (uint8)0;
                }
                return 1;
            }
        }
    }
    for (idx = 0; idx < 6; ++idx)
    {
        car_states[race_order[idx]].place = (uint8)idx;
    }
    return 0;
}

uint32 race_update_checkpoint_timer(CAR_STATE *car)
{
    uint32 slot;
    uint32 result;
    uint32 state;
    uint32 packed;
    uint32 old_mask;
    sint32 low;
    sint32 high;
    sint32 speed;
    sint32 divisor;
    sint32 numerator;

    FUNCTION_MARKER(0x80047970, "SLUS_006.10");
    slot = car->slot;
    result = race_view.primary_car;
    if (slot != result)
        return result;
    result = race_control.timing_hold;
    if (result != 0)
        return result;
    result = session_flow.attract_active;
    if (result != 0)
        return result;
    result = race_session.demo;
    if (result != 0)
        return result;
    packed = car->checkpoint_timer;
    state = car->finish_time;
    low = packed & 255;
    high = packed >> 8;
    race_control.primary_timer_snapshot = packed;
    if (state != 0)
        return packed;
    if (race_rules.checkpoint_timers != 0)
    {
        low -= 2;
        if (low < 0)
        {
            state = disp_state.field_rate;
            --high;
            low = ((uint32)(low - 1) + state);
            if (high < 0)
            {
                speed = (sint32)car->forward_speed;
                state = car->race_stats.elapsed_ticks;
                low = 0;
                high = 0;
                car->finish_time = (uint32)(state);
                speed /= 256;
                divisor = speed > 0 ? speed : 1;
                slot = car->slot;
                car->finish_speed_divisor = (uint16)((uint32)divisor);
                numerator = (sint32)(race_ai_cars[slot].seg_u * 1500);
                result = (uint32)fixed_divide_checked(numerator, (sint16)divisor);
                slot = car->slot;
                state = race_view.active_car;
                car->checkpoint_progress = (uint16)(result);
                if (slot == state)
                {
                    state = race_session.demo;
                    race_control.exit_reason = (uint32)(2);
                    if (state == 0)
                    {
                        camera_views[slot].sel_mode = 0;
                        camera_reset_follow(&car_states[(slot * 864) / 864u].angles, 0, slot);
                    }
                    old_mask = race_control.stopped_players;
                    race_control.players[slot].finished = (uint32)(1);
                    race_control.players[slot].finish_frames = (uint32)(0);
                    race_control.stopped_players = (uint32)(old_mask | ((uint32)1 << (slot & 31)));
                }
            }
        }
    }
    result = ((uint32)high & 255) << 8;
    packed = ((uint32)low & 255) | result;
    car->checkpoint_timer = (uint16)(packed);
    race_control.primary_timer_snapshot = packed;
    if (packed < 0xA00)
    {
        state = (sint16)(hud_checkpoint_disp.blink_frames);
        result = 2;
        if ((sint32)state < 2)
            hud_checkpoint_disp.blink_frames = (uint16)(2);
    }
    return result;
}

uint32 race_award_speed_points(CAR_STATE *car)
{
    uint32 result, reduction, state;
    sint32 speed, amount = 0;
    sint32 seg, next_seg;

    FUNCTION_MARKER(0x80049258, "SLUS_006.10");
    result = car->finish_time;
    if (result != 0)
        return result;
    if (car->wheel_slip_flags == 0)
    {
        speed = (sint32)car->steered_speed;
        if (speed > 0 && (race_control.update_cycles & 3) == 0)
        {
            reduction = car->place;
            state = car->surface_type;
            amount = (speed >> 15) - (sint32)(reduction >> 1);
            if (state >= 16)
                amount = 0;
            if (amount < 0)
                amount = 0;
            if (car->slot == 0)
            {
                seg = (sint16)car->track_location.n_node;
                next_seg = (sint16)car->track_location.max_path_count;
                if (seg < next_seg)
                    amount = 0;
            }
        }
    }
    result = car->points + (uint32)amount;
    car->points = (uint32)(result);
    return result;
}

static uint32 update_ai_gap_scale(void)
{
    uint32 idx;
    uint32 count;
    uint32 val;
    uint32 factor;
    sint32 pos;
    sint32 player_pos;
    sint32 difference;
    sint32 divisor;
    sint32 quotient;

    FUNCTION_MARKER(0x8004B0D0, "SLUS_006.10");
    if (race_session.two_players != 0)
    {
        count = race_control.car_count;
        idx = 2;
        while ((sint32)count >= 7 ? (sint32)idx < 6 : (sint32)idx < (sint32)count)
        {
            count = race_control.car_count;
            ai_initial_scale[idx] = 256;
            race_ai_cars[idx].throttle_scale = 256;
            ++idx;
        }
        return 0;
    }
    count = race_control.car_count;
    idx = 1;
    while ((sint32)count >= 7 ? (sint32)idx < 6 : (sint32)idx < (sint32)count)
    {
        pos = (sint16)car_states[(idx * 864) / 864u].track_location.path_count;
        player_pos = (sint16)car_states[0].track_location.path_count;
        difference = pos - player_pos;
        race_ai.gap_work = (uint32)difference;
        if (difference < 0)
        {
            divisor = (sint32)race_ai.trailing_limit;
            if (divisor < difference)
                race_ai.gap_work = (uint32)divisor;
            val = race_ai.gap_work;
            factor = race_ai.trailing_gain;
        }
        else
        {
            divisor = (sint32)race_ai.leading_limit;
            if (divisor < difference)
                race_ai.gap_work = (uint32)divisor;
            val = race_ai.gap_work;
            factor = race_ai.leading_gain;
        }
        quotient = fixed_divide_checked((sint32)(val * factor), divisor);
        race_ai.gap_work = (uint32)quotient;
        val = race_ai.gap_work;
        count = race_control.car_count;
        race_ai_cars[idx].throttle_scale = 256 - val;
        ++idx;
    }
    return idx << 3;
}

static uint32 apply_ai_throttle(uint32 slot)
{
    uint32 sel;
    uint32 result;
    uint32 car_offset;
    const RACE_AI_ROUTE *route;
    uint32 tag;
    uint32 throttle;
    sint32 lane;
    sint32 speed;
    sint32 threshold;

    FUNCTION_MARKER(0x8004E29C, "SLUS_006.10");
    if (slot == 9)
    {
        sel = race_ai_cars[9].chase_target;
        if (sel != 0xFFFFFFFF)
        {
            result = race_ai_cars[sel].chase_brake;
            if (result != 0)
                return result;
        }
    }
    car_offset = slot * 864;
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    route = race_ai_cars[slot].route;
    XASSERT(!(route == NULL || (uint32)lane >= route->count));
    tag = route->samples[(uint32)lane].speed_limit;
    throttle = race_ai_cars[slot].throttle_scale;
    if (tag != 0)
    {
        speed = (sint32)race_ai_cars[slot].route_speed;
        threshold = ((tag << 10) / 255);
        car_states[car_offset / 864u].pedal = (uint16)(speed < threshold ? throttle : 0);
        car_states[slot].braking = (uint8)(0);
        car_states[slot].brake_latch = (uint8)(0);
        return 0;
    }
    speed = (sint32)race_ai_cars[slot].route_speed;
    if (speed >= 129 && (sint32)car_states[slot].body_grip_excess <= 65535)
    {
        car_states[car_offset / 864u].pedal = (uint16)(0xFF00);
        car_states[slot].braking = (uint8)(1);
        car_states[slot].brake_latch = (uint8)(1);
        return 1;
    }
    car_states[car_offset / 864u].pedal = (uint16)(throttle);
    car_states[slot].braking = (uint8)(0);
    car_states[slot].brake_latch = (uint8)(0);
    return 0;
}

uint32 race_ai_heading_error(const RACE_AI_CAR *rec)
{
    uint32 car_offset;
    const RACE_AI_ROUTE *route;
    uint32 heading;
    uint32 tag;
    uint32 angle;
    uint32 difference;
    sint32 lane;

    FUNCTION_MARKER(0x8004D19C, "SLUS_006.10");
    car_offset = rec->car_slot * 864;
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    route = rec->route;
    heading = car_states[(car_offset) / 864u].orientation.bits.y;
    XASSERT(!(route == NULL || (uint32)lane >= route->count));
    tag = route->samples[(uint32)lane].heading;
    angle = tag * 4140 / 256;
    difference = ((uint32)((sint32)heading >> 8) & 0xFFF) - 2048;
    difference = (difference - angle) & 0xFFF;
    difference = (difference + 2048) & 0xFFF;
    return (0 - difference) & 0xFFF;
}

static void filter_traffic_spawns(void)
{
    uint32 source, count = 0;
    RACE_TRAFFIC_LIST *traffic = &race_ai.traffic;
    for (source = 0; source + 1 < traffic->count; ++source)
    {
        RACE_TRAFFIC_RECORD record = traffic->records[source];
        uint32 node = (uint32)(sint32)record.node;
        uint32 lane = record.side;
        uint32 lanes;
        if (node >= track_geometry.node_count)
            continue;
        lanes = track_geometry.node_bytes[node * 24 + 3] & 15;
        if (lane >= lanes)
        {
            node = track_map_branch_node(record.node);
            lane -= lanes;
        }
        if (node >= track_geometry.node_count)
            continue;
        lanes = track_geometry.node_bytes[node * 24 + 3] & 15;
        if (lane >= lanes)
            continue;
        traffic->records[count++] = record;
    }
    traffic->records[count] = traffic->records[traffic->count - 1];
    traffic->count = count + 1;
}

static uint32 load_ai_routes(void)
{
    char filename[64];
    uint32 idx, len, size, result, sample;
    const char *prefix;
    const char *format;
    RACE_AI_ROUTE *dst;
    RACE_AI_ROUTE_SAMPLE *samples;
    sint32 formatted;

    FUNCTION_MARKER(0x8004B2BC, "SLUS_006.10");
    for (idx = 0; idx < 3; ++idx)
    {
        prefix = scene_level_image.asset_prefix;
        len = 0;
        while (prefix[len] != 0)
            ++len;
        if (idx == 0)
        {
            format = "%sLEFT.TRK";
            dst = &race_ai.routes[0];
        }
        else if (idx == 1)
        {
            format = "%sRIGHT.TRK";
            dst = &race_ai.routes[1];
        }
        else
        {
            format = "%sTRAFFIC.BUS";
            dst = NULL;
        }
        formatted = sprintf(filename, format, prefix);
        size = archive_aligned_size(filename);
        if (idx < 2)
        {
            free(dst->samples);
            dst->samples = (RACE_AI_ROUTE_SAMPLE *)malloc(size ? size : 1);
            dst->count = 0;
            XASSERT(!(dst->samples == NULL));
            result = archive_read_file(filename, dst->samples, size);
            XASSERT(!(result % sizeof(RACE_AI_ROUTE_SAMPLE) != 0));
            dst->count = result / sizeof(RACE_AI_ROUTE_SAMPLE);
            if (strcmp(prefix, "LEVEL16\\") == 0 && dst->count == 2661 && track_geometry.main_node_count == 2756)
            {
                XASSERT(!(dst->samples[2660].lateral != 127 || dst->samples[2660].heading != 127 || dst->samples[2660].speed_limit != 255));
                samples = (RACE_AI_ROUTE_SAMPLE *)realloc(dst->samples, 2756 * sizeof(*samples));
                XASSERT(!(samples == NULL));
                dst->samples = samples;
                // Extend the supported straight section through the service junction
                for (sample = 2661; sample < 2756; ++sample)
                {
                    samples[sample].lateral = 127;
                    samples[sample].heading = 127;
                    samples[sample].speed_limit = 255;
                }
                dst->count = 2756;
            }
            XASSERT(!(dst->count < track_geometry.main_node_count));
        }
        else
        {
            free(race_ai.traffic.records);
            race_ai.traffic.records = (RACE_TRAFFIC_RECORD *)malloc(size ? size : 1);
            race_ai.traffic.count = 0;
            race_ai.traffic.next = 0;
            XASSERT(!(race_ai.traffic.records == NULL));
            result = archive_read_file(filename, race_ai.traffic.records, size);
            XASSERT(!(result == 0 || result % sizeof(RACE_TRAFFIC_RECORD) != 0));
            race_ai.traffic.count = result / sizeof(RACE_TRAFFIC_RECORD);
            for (sample = 0; sample + 1 < race_ai.traffic.count; ++sample)
                XASSERT(!(race_ai.traffic.records[sample].node < 0));
            XASSERT(!(race_ai.traffic.records[sample].node != -1));
            filter_traffic_spawns();
        }
    }
    return result;
}

uint32 race_refresh_ai_route_offset(uint32 slot)
{
    uint32 car_offset = slot * 864;
    uint32 val;
    sint32 idx;
    const RACE_AI_ROUTE *table;
    sint32 node;
    uint32 kind;

    FUNCTION_MARKER(0x8004D660, "SLUS_006.10");
    node = (sint16)car_states[(car_offset) / 864u].track_location.node;
    val = track_lateral_fraction(node, &car_states[(car_offset) / 864u].pos);
    race_ai_cars[slot].lane_parameter = val;
    idx = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    table = race_ai_cars[slot].route;
    node = (sint16)car_states[(car_offset) / 864u].track_location.node;
    XASSERT(!(table == NULL || (uint32)idx >= table->count));
    kind = table->samples[(uint32)idx].lateral;
    val = track_lateral_distance(node, val, kind);
    race_ai_cars[slot].route_offset = val;
    return val;
}

static uint32 ai_intercept_time(const RACE_AI_CAR *owner, const RACE_AI_CAR *other)
{
    uint32 other_slot;
    uint32 own_slot;
    uint32 other_offset;
    uint32 own_offset;
    uint32 other_pos;
    uint32 own_pos;
    uint32 distance;
    uint32 scaled;
    uint32 numerator;
    uint32 own_speed;
    uint32 other_speed;
    sint32 other_node;
    sint32 own_node;
    sint32 quotient;

    FUNCTION_MARKER(0x8004C868, "SLUS_006.10");
    other_slot = other->car_slot;
    other_offset = other->seg_u;
    own_speed = owner->route_speed;
    own_slot = owner->car_slot;
    other_speed = other->route_speed;
    other_node = (sint16)car_states[(other_slot * 864) / 864u].track_location.node;
    own_node = (sint16)car_states[(own_slot * 864) / 864u].track_location.node;
    own_offset = owner->seg_u;
    other_pos = ((uint32)other_node << 8) + other_offset;
    own_pos = ((uint32)own_node << 8) + own_offset;
    if (own_speed == other_speed || own_speed == 0)
        return 192000;
    distance = other_pos - own_pos;
    scaled = distance * 1500;
    numerator = other_speed * scaled;
    quotient = fixed_divide_checked((sint32)numerator, (sint32)(own_speed - other_speed));
    numerator = scaled + (uint32)quotient;
    quotient = fixed_divide_checked((sint32)numerator, (sint32)own_speed);
    if (quotient < 0)
        quotient = ((uint32)quotient + 255);
    return (quotient >> 8);
}

static uint32 update_ai_intervals(uint32 slot)
{
    uint32 car_offset;
    uint32 idx;
    uint32 val;
    uint32 edge0;
    uint32 edge1;
    uint32 offset;
    const RACE_AI_ROUTE *route;
    uint32 tag;
    sint32 minimum;
    sint32 maximum;
    sint32 sample;
    sint32 lane;
    sint32 node;

    FUNCTION_MARKER(0x8004FA10, "SLUS_006.10");
    car_offset = slot * 864;
    for (idx = 0; idx < 4; ++idx)
    {
        node = (sint16)car_states[(car_offset) / 864u].track_location.node;
        val = track_lateral_fraction(node, &car_states[car_offset / 864u].body_contact_pos[idx]);
        race_ai_cars[slot].corner_parameters[idx] = val;
    }
    minimum = 65536;
    maximum = -65536;
    for (idx = 0; idx < 4; ++idx)
    {
        sample = (sint32)race_ai_cars[slot].corner_parameters[idx];
        if (sample < minimum)
            minimum = sample;
        if (maximum < sample)
            maximum = sample;
    }
    edge0 = race_ai_cars[slot].corner_parameters[1];
    edge1 = race_ai_cars[slot].corner_parameters[0];
    race_ai_cars[slot].footprint_bounds[0] = (uint32)minimum;
    race_ai_cars[slot].footprint_bounds[1] = (uint32)maximum;
    race_ai_cars[slot].edge_parameters[0] = edge0;
    race_ai_cars[slot].edge_parameters[1] = edge1;
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    offset = race_ai_cars[slot].footprint_bounds[0];
    route = &race_ai.routes[0];
    node = (sint16)car_states[(car_offset) / 864u].track_location.node;
    XASSERT(!(route == NULL || (uint32)lane >= route->count));
    tag = route->samples[(uint32)lane].lateral;
    val = track_lateral_distance(node, offset, tag);
    race_ai_cars[slot].route_bounds[0] = val;
    for (idx = 0; idx < 3; ++idx)
    {
        lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
        node = (sint16)car_states[(car_offset) / 864u].track_location.node;
        route = &race_ai.routes[idx == 0 ? 0 : 1];
        offset = race_ai_cars[slot].footprint_bounds[idx == 1 ? 0 : 1];
        XASSERT(!(route == NULL || (uint32)lane >= route->count));
        tag = route->samples[(uint32)lane].lateral;
        val = track_lateral_distance(node, offset, tag);
        race_ai_cars[slot].route_bounds[idx + 1] = val;
    }
    return val;
}

static uint32 ai_target_side_factor(sint32 node, uint32 slot, const VEC3 *point)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3;

    const uint8 *seg;
    const uint8 *vertices;
    uint32 type;
    uint32 count;
    uint32 vertex;
    uint32 extension;
    const uint8 *endpoint;
    uint32 endpoint_idx;
    uint32 point_z;
    uint32 point_x;
    uint32 origin_x;
    uint32 origin_z;
    uint32 world_x;
    uint32 world_z;
    uint32 x;
    uint32 z;
    uint32 target_x;
    uint32 target_z;
    uint32 dot;
    uint32 denominator;
    uint32 numerator;

    FUNCTION_MARKER(0x8004DD0C, "SLUS_006.10");
    XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
    seg = track_geometry.node_bytes + (uint32)node * 24;
    type = seg[0];
    vertices = track_geometry.vertex_bytes;
    count = seg[3];
    vertex = (memcpy(&wire_halfword_1, seg + 6, sizeof(wire_halfword_1)), wire_halfword_1);
    XASSERT(!(type >= 12));
    extension = track_boundary_offsets[type].vertex_offset1;
    point_z = point->bits.z;
    endpoint_idx = vertex + (count & 15) + extension;
    XASSERT(!(endpoint_idx >= track_geometry.vertex_count));
    endpoint = vertices + endpoint_idx * 6;
    x = (sint16)(memcpy(&wire_halfword_2, endpoint, sizeof(wire_halfword_2)), wire_halfword_2);
    memcpy(&origin_x, seg + 12, sizeof(origin_x));
    world_x = car_states[(slot * 864) / 864u].pos.bits.x;
    world_z = car_states[(slot * 864) / 864u].pos.bits.z;
    x = origin_x + x - (uint32)((sint32)world_x >> 8);
    target_z = ((sint32)(0 - point_z - world_z) >> 8);
    dot = x * target_z;
    point_x = point->bits.x;
    z = (sint16)(memcpy(&wire_halfword_3, endpoint + 4, sizeof(wire_halfword_3)), wire_halfword_3);
    memcpy(&origin_z, seg + 20, sizeof(origin_z));
    target_x = ((sint32)(point_x - world_x) >> 8);
    z = origin_z + z - (uint32)((sint32)world_z >> 8);
    dot += z * target_x;
    if ((sint32)dot <= 0)
        return 0;
    denominator = x + z;
    numerator = target_z - target_x;
    if (denominator == 0)
    {
        x = 0 - z + 1;
        denominator = x + z;
    }
    return (uint32)fixed_divide_checked((sint32)numerator, (sint32)denominator);
}

static uint32 compute_ai_target(sint32 node, uint32 factor, VEC3 *output, uint32 offset)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5;

    SVECTOR n;
    const uint8 *seg;
    const uint8 *vertices;
    uint32 vertex;
    uint32 type;
    uint32 count;
    uint32 origin_x;
    uint32 origin_z;
    uint32 extension;
    const uint8 *point0;
    const uint8 *point1;
    uint32 idx1;
    uint32 start_x;
    uint32 start_z;
    uint32 delta_x;
    uint32 delta_z;
    uint32 val;
    uint32 result;
    sint32 x;
    sint32 z;
    sint32 step;

    FUNCTION_MARKER(0x8004DE3C, "SLUS_006.10");
    seg = track_geometry.node_bytes;
    vertices = track_geometry.vertex_bytes;
    XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
    seg += (uint32)node * 24;
    vertex = (memcpy(&wire_halfword_1, seg + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    type = seg[0];
    count = seg[3];
    memcpy(&origin_x, seg + 12, sizeof(origin_x));
    XASSERT(!(type >= 12));
    extension = track_boundary_offsets[type].vertex_offset0;
    XASSERT(!(vertex >= track_geometry.vertex_count));
    point0 = vertices + vertex * 6;
    x = (sint16)(memcpy(&wire_halfword_2, point0, sizeof(wire_halfword_2)), wire_halfword_2);
    idx1 = vertex + (count & 15) + extension;
    XASSERT(!(idx1 >= track_geometry.vertex_count));
    point1 = vertices + idx1 * 6;
    z = (sint16)(memcpy(&wire_halfword_3, point1, sizeof(wire_halfword_3)), wire_halfword_3);
    start_x = origin_x + (uint32)x;
    delta_x = origin_x + (uint32)z - start_x;
    memcpy(&origin_z, seg + 20, sizeof(origin_z));
    x = (sint16)(memcpy(&wire_halfword_4, point0 + 4, sizeof(wire_halfword_4)), wire_halfword_4);
    z = (sint16)(memcpy(&wire_halfword_5, point1 + 4, sizeof(wire_halfword_5)), wire_halfword_5);
    start_z = origin_z + (uint32)x;
    delta_z = origin_z + (uint32)z - start_z;
    output->bits.x = (start_x << 8) + factor * delta_x;
    output->bits.z = (start_z << 8) + factor * delta_z;
    n.vx = (sint16)delta_x;
    n.vy = 0;
    n.vz = (sint16)delta_z;
    VectorNormalSS(&n, &n);
    step = (sint32)((uint32)(sint32)n.vx * offset) / 4096;
    val = output->bits.x;
    output->bits.x = val + ((uint32)step << 8);
    step = (sint32)((uint32)(sint32)n.vz * offset) / 4096;
    val = output->bits.z;
    result = (uint32)step << 8;
    output->bits.z = val + result;
    return result;
}

static uint32 classify_ai_route_offset(uint32 slot, uint32 offset)
{
    uint32 car_offset;
    VEC3 point;
    const RACE_AI_ROUTE *route;
    const RACE_AI_ROUTE *primary;

    uint32 tag;
    uint32 width;
    uint32 mapped;
    sint32 lane;
    sint32 threshold;
    sint32 lookahead;
    sint32 sel;

    FUNCTION_MARKER(0x8004FCE4, "SLUS_006.10");
    car_offset = slot * 864;
    route = race_ai_cars[slot].route;
    primary = &race_ai.routes[0];
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    threshold = (sint32)track_geometry.main_node_count;
    sel = lane + 4;
    if (sel >= threshold)
        sel = ((uint32)lane - ((uint32)threshold - 4));
    if (route != primary)
    {
        mapped = track_map_branch_node(sel);
        sel = mapped;
        if (mapped == 0xFFFFFFFF)
        {
            lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
            threshold = (sint32)track_geometry.main_node_count;
            sel = lane + 4;
            if (sel >= threshold)
                sel = ((uint32)lane - ((uint32)threshold - 4));
        }
    }
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    threshold = (sint32)track_geometry.main_node_count;
    route = race_ai_cars[slot].route;
    lookahead = lane + 4;
    if (lookahead >= threshold)
        lookahead = ((uint32)lane - ((uint32)threshold - 4));

    width = (uint32)car_body_points[car_offset / 864u][1].vx;
    XASSERT(!(route == NULL || (uint32)lookahead >= route->count));
    tag = route->samples[(uint32)lookahead].lateral;
    compute_ai_target(sel, tag, &point, offset + width);
    if ((sint32)track_lateral_fraction(sel, &point) >= 255)
        return 1;
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    threshold = (sint32)track_geometry.main_node_count;
    route = race_ai_cars[slot].route;
    lookahead = lane + 4;
    if (lookahead >= threshold)
        lookahead = ((uint32)lane - ((uint32)threshold - 4));

    width = (uint32)car_body_points[car_offset / 864u][0].vx;
    XASSERT(!(route == NULL || (uint32)lookahead >= route->count));
    tag = route->samples[(uint32)lookahead].lateral;
    compute_ai_target(sel, tag, &point, offset + width);
    return (sint32)track_lateral_fraction(sel, &point) < 1 ? 2 : 0;
}

static uint32 ai_candidate_interval(RACE_AI_CAR *owner, uint32 idx, sint32 *gap, uint32 *last_offset)
{
    uint32 car_offset;
    const RACE_AI_ROUTE *route;
    const RACE_AI_ROUTE *candidate_route;
    const RACE_AI_ROUTE *primary;
    uint32 val;
    uint32 interval_lower;
    uint32 interval_upper;
    uint32 own;

    uint32 edge;
    uint32 tag;
    sint32 lane;
    sint32 node;
    sint32 bound;
    sint32 candidate;
    sint32 own_lane;

    car_offset = idx * 864;
    route = owner->route;
    candidate_route = race_ai_cars[idx].route;
    if (route != candidate_route)
    {
        primary = &race_ai.routes[0];
        if (route == primary)
        {
            val = owner->route_offsets[1];
            route = &race_ai.routes[1];
            owner->route_offset = val;
            owner->route = route;
        }
        else
        {
            val = owner->route_offsets[0];
            owner->route = primary;
            owner->route_offset = val;
        }
    }
    route = owner->route;
    candidate_route = &race_ai.routes[1];
    if (route == candidate_route)
    {
        interval_lower = owner->route_bounds[2];
        interval_upper = owner->route_bounds[3];
    }
    else
    {
        interval_lower = owner->route_bounds[0];
        interval_upper = owner->route_bounds[1];
    }
    owner->interval_lower = interval_lower;
    owner->interval_upper = interval_upper;
    val = owner->route_offset;
    edge = classify_ai_route_offset(idx, val);
    if (edge == 1 || edge == 2)
    {
        lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
        route = race_ai_cars[idx].route;
        node = (sint16)car_states[(car_offset) / 864u].track_location.node;
        XASSERT(!(route == NULL || (uint32)lane >= route->count));
        tag = route->samples[(uint32)lane].lateral;
        val = track_lateral_distance(node, edge == 1 ? 256 : 0, tag);
        own = owner->car_slot;

        bound = car_body_points[own][edge == 1 ? 0 : 1].vx;
        *last_offset = val + (uint32)bound - 32;
    }
    else
        *last_offset = owner->route_offset;
    own = owner->car_slot;
    car_offset = own * 864;

    bound = car_body_points[own][0].vx;
    candidate = (sint32)race_ai_cars[idx].interval_upper;
    if (candidate < (sint32)((uint32)bound + *last_offset - 32))
        return 0;
    bound = car_body_points[own][1].vx;
    candidate = (sint32)race_ai_cars[idx].interval_lower;
    if ((sint32)((uint32)bound + *last_offset + 32) < candidate)
        return 0;
    lane = (sint16)car_states[(idx * 864) / 864u].track_location.n_node;
    own_lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    *gap = lane - own_lane;
    return 1;
}

static uint32 ai_finish_selection(uint32 sel, uint32 own, uint32 last_offset)
{
    uint32 interval_upper;
    uint32 interval_lower;

    uint32 midpoint;
    uint32 lower;
    uint32 upper;
    uint32 lower_distance;
    uint32 upper_distance;

    interval_upper = race_ai_cars[sel].interval_upper;
    interval_lower = race_ai_cars[sel].interval_lower;

    lower = (uint32)car_body_points[own][0].vx;
    midpoint = ((sint32)(interval_upper + interval_lower) / 2);
    lower += last_offset - 32;
    lower_distance = lower - midpoint;
    if ((sint32)lower_distance < 0)
        lower_distance = midpoint - lower;
    upper = (uint32)car_body_points[own][1].vx;
    upper += last_offset + 32;
    upper_distance = upper - midpoint;
    if ((sint32)upper_distance < 0)
        upper_distance = midpoint - upper;
    race_ai.selection_side = (sint32)lower_distance < (sint32)upper_distance ? 0 : 1;
    return sel;
}

static uint32 select_ai_peer(RACE_AI_CAR *owner)
{
    uint32 count;
    uint32 sel;
    uint32 idx;
    uint32 own;
    uint32 own_variant;
    uint32 peer_variant;
    uint32 last_offset;
    sint32 best_gap;
    sint32 gap;
    sint32 lane;
    sint32 candidate_lane;
    sint32 speed;
    sint32 candidate_speed;

    FUNCTION_MARKER(0x8004C0A4, "SLUS_006.10");
    count = race_control.car_count;
    sel = owner->car_slot;
    best_gap = 192000;
    idx = 6;
    if ((sint32)count >= 6 && (sint32)idx < (sint32)count)
    {
        do
        {
            own_variant = owner->route_variant;
            peer_variant = race_ai_cars[idx].route_variant;
            if (own_variant == peer_variant)
            {
                own = owner->car_slot;
                lane = (sint16)car_states[(own * 864) / 864u].track_location.n_node;
                candidate_lane = (sint16)car_states[(idx * 864) / 864u].track_location.n_node;
                if (candidate_lane >= lane)
                {
                    if (ai_candidate_interval(owner, idx, &gap, &last_offset) != 0 && gap < best_gap)
                    {
                        best_gap = gap;
                        sel = idx;
                    }
                }
            }
            count = race_control.car_count;
            ++idx;
        } while ((sint32)idx < (sint32)count);
    }
    if ((uint32)(best_gap - 1) < 39)
    {
        own = owner->car_slot;
        if (sel != own)
            return ai_finish_selection(sel, own, last_offset);
    }
    sel = owner->car_slot;
    best_gap = 192000;
    idx = 0;
    for (;;)
    {
        count = race_control.car_count;
        if ((sint32)count >= 7)
            count = 6;
        if ((sint32)idx >= (sint32)count)
            break;
        own = owner->car_slot;
        if (idx != own)
        {
            own_variant = owner->route_variant;
            peer_variant = race_ai_cars[idx].route_variant;
            if (own_variant == peer_variant)
            {
                lane = (sint16)car_states[(own * 864) / 864u].track_location.n_node;
                candidate_lane = (sint16)car_states[(idx * 864) / 864u].track_location.n_node;
                if (candidate_lane >= lane)
                {
                    candidate_speed = (sint32)race_ai_cars[idx].route_speed;
                    speed = (sint32)owner->route_speed;
                    if (speed >= (sint32)((uint32)candidate_speed - 16))
                    {
                        if (ai_candidate_interval(owner, idx, &gap, &last_offset) != 0 && gap < best_gap)
                        {
                            best_gap = gap;
                            sel = idx;
                        }
                    }
                }
            }
        }
        ++idx;
    }
    if ((uint32)best_gap < 41)
    {
        own = owner->car_slot;
        if (sel != own)
            return ai_finish_selection(sel, own, last_offset);
    }
    return owner->car_slot;
}

static uint32 update_ai_peer_offset(uint32 slot)
{
    uint32 state_offset;
    uint32 sel;
    uint32 other_offset;
    uint32 flag;
    uint32 val;
    uint32 result;
    const uint8 *seg;
    uint32 count;
    uint32 lane;
    sint32 node;
    sint32 other_lane;
    sint32 own_lane;
    sint32 gap;

    FUNCTION_MARKER(0x8004DFA4, "SLUS_006.10");
    state_offset = slot * 284;
    sel = select_ai_peer(&race_ai_cars[slot]);
    if (sel == slot)
    {
        val = race_ai_cars[slot].route_offset;
        if ((sint32)val < 0)
        {
            result = val + 8;
            race_ai_cars[slot].route_offset = result;
            if ((sint32)result <= 0)
                return result;
            race_ai_cars[slot].route_offset = 0;
            val = race_ai_cars[slot].route_offset;
        }
        result = val - 8;
        if ((sint32)val > 0)
        {
            race_ai_cars[slot].route_offset = result;
            if ((sint32)result < 0)
                race_ai_cars[slot].route_offset = 0;
        }
        return result;
    }
    ai_intercept_time(&race_ai_cars[slot], &race_ai_cars[sel]);
    other_offset = sel * 864;
    flag = race_ai.selection_side;
    if (flag == 1 && car_states[(other_offset) / 864u].track_location.lane == 0)
    {
        race_ai.selection_side = 0;
        flag = race_ai.selection_side;
    }
    else
    {
        flag = race_ai.selection_side;
        if (flag == 0)
        {
            node = (sint16)car_states[(other_offset) / 864u].track_location.node;
            XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
            seg = track_geometry.node_bytes + (uint32)node * 24;
            count = seg[3];
            lane = car_states[(other_offset) / 864u].track_location.lane;
            if (lane == (count & 15))
                race_ai.selection_side = 1;
            flag = race_ai.selection_side;
        }
    }
    other_lane = (sint16)car_states[(other_offset) / 864u].track_location.n_node;
    own_lane = (sint16)car_states[(slot * 864) / 864u].track_location.n_node;
    gap = other_lane - own_lane;
    if (gap >= 41)
        gap = 40;
    else if (gap <= 0)
        gap = 1;
    val = race_ai_cars[slot].route_offset;
    if (flag == 0)
        val = val + 41 - (uint32)gap;
    else
        val = val - 41 + (uint32)gap;
    race_ai_cars[slot].route_offset = val;
    return state_offset;
}

static uint32 update_ai_steering_target(RACE_AI_CAR *owner, const VEC3 *point)
{
    uint32 slot;
    CAR_STATE *car;
    uint32 coord;
    uint32 pos;
    uint32 angle;
    uint32 steering;
    uint32 result;
    sint32 x;
    sint32 z;
    sint32 difference;
    sint32 mag;

    FUNCTION_MARKER(0x8004D268, "SLUS_006.10");
    slot = owner->car_slot;
    coord = point->bits.x;
    car = &car_states[slot];
    pos = car->pos.bits.x;
    x = (sint32)(coord - pos) >> 8;
    coord = point->bits.z;
    pos = car->pos.bits.z;
    z = (sint32)(coord - pos) >> 8;
    angle = (uint32)car_heading_from_components(x, z);
    pos = car->orientation.bits.y;
    steering = car->steering_angle;
    difference = (((uint32)((sint32)(pos + steering) >> 8) & 0xFFF) - (angle & 0xFFF));
    mag = difference < 0 ? -difference : difference;
    result = 4095 - (uint32)mag;
    if (difference < 0)
    {
        result = mag;
        difference += 4095;
    }
    owner->steering_error[0] = result;
    owner->steering_error[1] = (uint32)difference;
    return result;
}

static uint32 step_ai_steering(const RACE_AI_CAR *owner, uint32 step)
{
    CAR_STATE *car;
    uint32 sq;
    uint32 val;
    uint32 delta;
    uint32 limit;
    sint32 yaw;
    sint32 speed;
    sint32 rate;
    sint32 val0;
    sint32 val1;
    sint32 angle;
    sint32 counter;
    sint32 product;
    uint32 subtract;

    FUNCTION_MARKER(0x8004D330, "SLUS_006.10");
    car = &car_states[owner->car_slot];
    yaw = (sint32)car->steered_grip_excess >> 8;
    sq = (uint32)yaw * (uint32)yaw;
    speed = (sint32)car->forward_speed >> 8;
    if (speed < 0)
        speed = -speed;
    val0 = fixed_divide_checked((sint32)((uint32)speed << 10), (sint32)(sq + 1024));
    val1 = fixed_divide_checked((sint32)((uint32)speed << 16), (sint32)(sq + 65536));
    rate = fixed_divide_checked(0xC0000, (sint32)((uint32)val0 + 64));
    val0 = (sint32)owner->steering_error[0];
    val1 = ((uint32)val1 + 256);
    angle = (sint32)owner->steering_error[1];
    limit = (uint32)fixed_divide_checked(0x1800000, val1);
    subtract = val0 >= 2048;
    if (subtract == 0)
        angle = val0;
    if (angle >= 1025)
    {
        val = car->steering_angle;
        val = subtract != 0 ? val - 0x4000 : val + 0x4000;
        car->steering_angle = val;
    }
    else if (step != 0)
    {
        delta = step;
        if (angle < 256)
        {
            product = (step * (uint32)rsin(angle));
            if (product < 0)
                product = ((uint32)product + 4095);
            delta = (product >> 12);
        }
        val = car->steering_angle;
        val = subtract != 0 ? val - delta : val + delta;
        car->steering_angle = val;
    }
    else
    {
        counter = (sint16)car->steering_ramp;
        if (counter < 256)
            car->steering_ramp = (uint16)((uint32)counter + 64);
        counter = (sint16)car->steering_ramp;
        product = ((uint32)rate * (uint32)counter);
        if (product < 0)
            product = ((uint32)product + 255);
        val = car->steering_angle;
        delta = (product >> 8);
        val = subtract != 0 ? val - delta : val + delta;
        car->steering_angle = val;
        if (subtract != 0)
        {
            limit = 0 - limit;
            if ((sint32)val < (sint32)limit)
                car->steering_angle = limit;
        }
        else if ((sint32)limit < (sint32)val)
            car->steering_angle = limit;
    }
    val = car->steering_angle;
    if ((sint32)val > 98304)
    {
        car->steering_angle = 98304;
        return 1;
    }
    if ((sint32)val < -98304)
    {
        car->steering_angle = (uint32)-98304;
        return 1;
    }
    return 0;
}

static uint32 run_ai_program(RACE_AI_CAR *owner)
{
    uint32 timer, slot, flags, val, angle, argument, command;
    CAR_STATE *car;
    const uint32 *program, *cursor, *next;
    const RACE_AI_ROUTE *route;
    const uint8 *node_table;
    uint32 node, sample, steering, heading, threshold, speed, middle, lane;

    FUNCTION_MARKER(0x80050798, "SLUS_006.10");
    timer = owner->program_timer;
    slot = owner->car_slot;
    timer -= 1;
    car = &car_states[slot];
    owner->program_timer = timer;
    if ((sint32)timer < 0)
    {
        program = owner->program;
        owner->program_timer = 150;
        if (program == ai_program_reverse_left)
            program = ai_program_forward_right;
        else if (program == ai_program_forward_right)
            program = ai_program_reverse_left;
        else if (program == ai_program_reverse_right)
            program = ai_program_forward_left;
        else if (program == ai_program_forward_left)
            program = ai_program_reverse_right;
        else
            goto steering_flags;
        owner->program = program;
        owner->program_cursor = program;
    }
steering_flags:
    flags = owner->program_flags;
    if ((flags & 4) != 0)
    {
        angle = race_ai_heading_error(owner);
        if ((sint32)angle >= 513)
        {
            val = car->steering_angle + 0x4000;
            car->steering_angle = val;
            if ((sint32)val > 102400)
                car->steering_angle = 102400;
            goto velocity_flags;
        }
        if ((sint32)owner->program_speed < 0)
        {
            car->steering_angle = 0;
            flags = owner->program_flags;
            owner->program_flags = flags ^ 4;
        }
        goto follow_course;
    }
    if ((flags & 8) != 0)
    {
        angle = race_ai_heading_error(owner);
        if ((sint32)angle < 3584)
        {
            val = car->steering_angle - 0x4000;
            car->steering_angle = val;
            if ((sint32)val < -102400)
                car->steering_angle = (uint32)-102400;
            goto velocity_flags;
        }
        if ((sint32)owner->program_speed < 0)
        {
            car->steering_angle = 0;
            flags = owner->program_flags;
            owner->program_flags = flags ^ 8;
        }
        goto follow_course;
    }
    goto velocity_flags;
follow_course:
    node = (sint16)car->track_location.n_node;
    route = owner->route;
    XASSERT(!(route == NULL || (uint32)node >= route->count));
    sample = route->samples[(uint32)node].heading;
    steering = car->steering_angle;
    heading = car->orientation.bits.y;
    val = (uint32)((sint32)(heading + steering) >> 8) & 4095;
    val = (val - sample * 4140 / 256 + 2048) & 4095;
    val = fixed_reflect_heading((sint32)val);
    owner->steering_error[0] = val;
    owner->steering_error[1] = 4095 - val;
    step_ai_steering(owner, 0x4000);
velocity_flags:
    flags = owner->program_flags;
    if ((flags & 0x10) != 0)
    {
        val = car->forward_speed + 255;
        if (val < 511)
        {
            owner->program_flags = flags ^ 0x10;
            car->braking = (uint8)(0);
            car->pedal = (uint16)(0);
        }
        else
        {
            car->braking = (uint8)(1);
            car->pedal = (uint16)(0xFF00);
        }
        return 0;
    }
    if ((flags & 2) != 0)
    {
        car->braking = (uint8)(0);
        val = owner->program_speed;
        if ((sint32)val > 0)
        {
            car->pedal = (uint16)(255);
            val = owner->program_speed * 262920;
            threshold = ((sint32)val / 256);
            speed = car->forward_speed;
            if ((sint32)speed < (sint32)threshold)
                car->pedal = (uint16)(0);
        }
        else
        {
            car->pedal = (uint16)(0xFF00);
            val = owner->program_speed * 262920;
            threshold = ((sint32)val / 256);
            speed = car->forward_speed;
            if ((sint32)threshold > (sint32)speed)
                car->pedal = (uint16)(0);
        }
    }
    cursor = owner->program_cursor;
    command = cursor[0];
    switch (command)
    {
        case 0:
            angle = race_ai_heading_error(owner);
            if ((sint32)angle >= 64 && (sint32)race_ai_heading_error(owner) < 4033)
                return 0;
            if ((sint32)owner->program_speed < 0)
                car->steering_angle = 0;
            owner->program = 0;
            owner->program_flags = 0;
            owner->program_completion = 0;
            owner->program_aux_completion = 0;
            car->steering_angle = 0;
            car->angular_velocity.bits.y = (uint32)(0);
            return 1;
        case 1:
            cursor = owner->program_cursor;
            flags = owner->program_flags;
            next = owner->program_cursor;
            argument = cursor[1];
            owner->program_flags = flags | 1;
            owner->program_cursor = next + 2;
            owner->program_argument = argument;
            return 0;
        case 2:
            flags = owner->program_flags;
            cursor = owner->program_cursor;
            owner->program_flags = flags | 2;
            next = owner->program_cursor;
            argument = cursor[1];
            owner->program_cursor = next + 2;
            owner->program_speed = argument;
            return 0;
        case 3:
            cursor = owner->program_cursor;
            flags = owner->program_flags;
            next = owner->program_cursor;
            argument = cursor[1];
            owner->program_cursor = next + 2;
            owner->program_flags = flags | argument;
            return 0;
        case 4:
            cursor = owner->program_cursor;
            argument = cursor[1];
            next = owner->program_cursor;
            flags = owner->program_flags;
            owner->program_cursor = next + 2;
            owner->program_flags = flags & ~argument;
            return 0;
        case 5:
        case 6:
        case 8:
        case 10:
        case 11:
            flags = owner->program_flags;
            cursor = owner->program_cursor;
            switch (command)
            {
                case 5:
                    flags |= 4;
                    break;
                case 6:
                    flags |= 8;
                    break;
                case 8:
                    flags |= 0x10;
                    break;
                case 10:
                    flags |= 0x40;
                    break;
                default:
                    flags |= 0x80;
                    break;
            }
            owner->program_flags = flags;
            owner->program_cursor = cursor + 1;
            return 0;
        case 7:
            car->braking = (uint8)(1);
            car->pedal = (uint16)(0xFF00);
            cursor = owner->program_cursor;
            owner->program_cursor = cursor + 1;
            return 0;
        case 9:
            angle = race_ai_heading_error(owner);
            owner->program_timer = 250;
            if (angle - 2305 < 1535)
            {
                slot = owner->car_slot;
                node = (sint16)car_states[(slot * 864) / 864u].track_location.node;
                node_table = track_geometry.node_bytes;
                XASSERT(!(node >= track_geometry.node_count));
                middle = (node_table[node * 24 + 3] & 15) >> 1;
                lane = car->track_location.lane;
                if (middle >= lane)
                {
                    program = ai_program_reverse_left;
                    goto select_program;
                }
            }
            if ((sint32)angle >= 1792)
            {
                slot = owner->car_slot;
                node = (sint16)car_states[(slot * 864) / 864u].track_location.node;
                node_table = track_geometry.node_bytes;
                XASSERT(!(node >= track_geometry.node_count));
                middle = (node_table[node * 24 + 3] & 15) >> 1;
                lane = car->track_location.lane;
                if (middle < lane)
                {
                    program = ai_program_forward_right;
                    goto select_program;
                }
            }
            program = ai_program_forward_left;
            if (angle - 256 < 1537)
            {
                slot = owner->car_slot;
                node = (sint16)car_states[(slot * 864) / 864u].track_location.node;
                node_table = track_geometry.node_bytes;
                XASSERT(!(node >= track_geometry.node_count));
                middle = (node_table[node * 24 + 3] & 15) >> 1;
                lane = car->track_location.lane;
                if (lane >= middle)
                    program = ai_program_reverse_right;
            }
        select_program:
            owner->program = program;
            owner->program_cursor = program;
            return 0;
        default:
            return 0;
    }
}

static uint32 select_traffic_peer(const RACE_AI_CAR *owner)
{
    uint32 mode;
    uint32 own;
    uint32 sel;
    uint32 idx;
    uint32 val0;
    uint32 val1;
    sint32 own_node;
    sint32 candidate_node;
    sint32 gap;
    sint32 best_gap;

    FUNCTION_MARKER(0x8004BEA8, "SLUS_006.10");
    mode = owner->reverse;
    own = owner->car_slot;
    sel = own;
    best_gap = 192000;
    for (idx = 0; idx < 12; ++idx)
    {
        if (idx == own)
            continue;
        if (mode == 0)
        {
            own_node = (sint16)car_states[(own * 864) / 864u].track_location.node;
            candidate_node = (sint16)car_states[(idx * 864) / 864u].track_location.node;
            if (candidate_node < own_node)
                continue;
            val0 = car_states[(own * 864) / 864u].track_location.lane;
            val1 = car_states[(idx * 864) / 864u].track_location.lane;
            if (val0 != val1)
                continue;
            gap = candidate_node - own_node;
        }
        else
        {
            val0 = car_states[(idx * 864) / 864u].track_location.lane;
            val1 = car_states[(own * 864) / 864u].track_location.lane;
            if (val0 != val1)
                continue;
            candidate_node = (sint16)car_states[(idx * 864) / 864u].track_location.node;
            own_node = (sint16)car_states[(own * 864) / 864u].track_location.node;
            if (own_node < candidate_node)
                continue;
            gap = own_node - candidate_node;
        }
        val0 = race_ai_cars[own].route_variant;
        val1 = race_ai_cars[idx].route_variant;
        if (val0 == val1 && gap < best_gap)
        {
            best_gap = gap;
            sel = idx;
        }
    }
    if (best_gap == 0 || best_gap >= 33)
        return owner->car_slot;
    return sel;
}

static void follow_traffic_seg(uint32 car_offset, uint32 reverse, TRACK_LOCATION *location)
{
    uint32 mapped;
    const uint8 *base;
    const uint8 *cur;
    const uint8 *adjacent;
    ptrdiff_t offset;
    uint32 val0;
    uint32 val1;
    uint32 lane_idx;
    uint32 type;
    sint32 lane;
    sint32 node;
    sint32 threshold;
    sint32 dir;

    dir = reverse != 0 ? -1 : 1;
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    mapped = (uint32)track_map_branch_node(lane + dir);
    if (reverse == 0 && mapped != 0xFFFFFFFF)
    {
        node = (sint16)car_states[(car_offset) / 864u].track_location.node;
        threshold = (sint32)track_geometry.main_node_count;
        if (node < threshold && threshold < (sint32)mapped)
        {
            lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
            base = track_geometry.node_bytes;
            offset = base - scene_track_image.bytes + (ptrdiff_t)lane * 24 + 27;
            XASSERT(!(offset < 0 || scene_track_image.size < 1 || (size_t)offset > scene_track_image.size - 1));
            val0 = base[(ptrdiff_t)lane * 24 + 27] & 15;
            lane_idx = car_states[(car_offset) / 864u].track_location.lane;
            if (lane_idx >= val0)
            {
                lane_idx -= val0;
                location->n_node = (uint16)((uint32)lane + 1);
                goto selected_segment;
            }
        }
    }
    else if (reverse != 0 && mapped != 0xFFFFFFFF)
    {
        lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
        base = track_geometry.node_bytes;
        offset = base - scene_track_image.bytes + (ptrdiff_t)lane * 24 - 21;
        XASSERT(!(offset < 0 || scene_track_image.size < 1 || (size_t)offset > scene_track_image.size - 1));
        val0 = base[(ptrdiff_t)lane * 24 - 21] & 15;
        lane_idx = car_states[(car_offset) / 864u].track_location.lane;
        if (lane_idx >= val0)
        {
            node = (sint16)car_states[(car_offset) / 864u].track_location.node;
            offset = base - scene_track_image.bytes + (ptrdiff_t)node * 24 + 3;
            XASSERT(!(offset < 0 || scene_track_image.size < 1 || (size_t)offset > scene_track_image.size - 1));
            val1 = base[(ptrdiff_t)node * 24 + 3];
            location->n_node = (uint16)((uint32)lane - 1);
            lane_idx += val0 - (val1 & 15);
            goto selected_segment;
        }
    }
    if (mapped == 0xFFFFFFFF)
    {
        node = (sint16)car_states[(car_offset) / 864u].track_location.node;
        threshold = (sint32)track_geometry.main_node_count;
        if (threshold < node)
        {
            lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
            lane_idx = car_states[(car_offset) / 864u].track_location.lane;
            base = track_geometry.node_bytes;
            offset = base - scene_track_image.bytes + (ptrdiff_t)lane * 24 + (reverse != 0 ? -21 : 27);
            XASSERT(!(offset < 0 || scene_track_image.size < 1 || (size_t)offset > scene_track_image.size - 1));
            val0 = base[(ptrdiff_t)lane * 24 + (reverse != 0 ? -21 : 27)];
            offset = base - scene_track_image.bytes + (ptrdiff_t)node * 24 + 3;
            XASSERT(!(offset < 0 || scene_track_image.size < 1 || (size_t)offset > scene_track_image.size - 1));
            val1 = base[(ptrdiff_t)node * 24 + 3];
            mapped = (uint32)lane + (uint32)dir;
            location->n_node = (uint16)mapped;
            lane_idx += (val0 & 15) - (val1 & 15);
            goto selected_segment;
        }
    }
    node = (sint16)car_states[(car_offset) / 864u].track_location.node;
    base = track_geometry.node_bytes;
    mapped = (uint32)node + (uint32)dir;
    offset = base - scene_track_image.bytes + (ptrdiff_t)node * 24;
    XASSERT(!(offset < 0 || scene_track_image.size < 4 || (size_t)offset > scene_track_image.size - 4));
    cur = base + (ptrdiff_t)node * 24;
    memcpy(&val0, cur, sizeof(val0));
    offset = base - scene_track_image.bytes + (ptrdiff_t)(sint32)mapped * 24;
    XASSERT(!(offset < 0 || scene_track_image.size < 4 || (size_t)offset > scene_track_image.size - 4));
    memcpy(&val1, base + (ptrdiff_t)(sint32)mapped * 24, sizeof(val1));
    if ((val0 & 0x0F000000) == (val1 & 0x0F000000))
        lane_idx = car_states[(car_offset) / 864u].track_location.lane;
    else
    {
        type = cur[0];
        lane_idx = car_states[(car_offset) / 864u].track_location.lane;
        if (reverse != 0 ? (type >= 3 && type < 5) : (type >= 6 && type < 8))
            --lane_idx;
        base = track_geometry.node_bytes;
        offset = base - scene_track_image.bytes + (ptrdiff_t)(sint32)mapped * 24;
        XASSERT(!(offset < 0 || scene_track_image.size < 1 || (size_t)offset > scene_track_image.size - 1));
        adjacent = base + (ptrdiff_t)(sint32)mapped * 24;
        type = adjacent[0];
        if (reverse != 0 ? (type >= 6 && type < 8) : (type >= 3 && type < 5))
            ++lane_idx;
    }
    lane = car_states[(car_offset) / 864u].track_location.n_node;
    location->n_node = (uint16)((uint32)lane + (uint32)dir);
selected_segment:
    location->lane = (uint8)lane_idx;
    location->node = (uint16)mapped;
}

static uint32 update_traffic_car(uint32 slot)
{
    uint32 car_offset;
    RACE_AI_CAR *owner;
    uint32 mode;
    uint32 angle;
    uint32 val;
    uint32 valid;
    TRACK_LOCATION location;
    VEC3 point;
    uint32 sel;
    uint32 result;
    sint32 node;
    sint32 threshold;
    sint32 speed;
    sint32 other_speed;
    sint32 other_node;
    sint32 gap;
    sint32 time;
    sint32 quotient;

    FUNCTION_MARKER(0x8004F03C, "SLUS_006.10");
    car_offset = slot * 864;
    owner = &race_ai_cars[slot];
    respawn_traffic_car();
    angle = race_ai_heading_error(&race_ai_cars[slot]);
    mode = race_ai_cars[slot].reverse;
    if (mode == 0)
    {
        if (angle - 1025 < 2047)
        {
            val = car_states[0].pos.bits.z & 7;
            race_ai_cars[slot].recovery_state = val;
            if (val == 0)
                race_ai_cars[slot].recovery_state = 1;
            if (slot == 9)
            {
                if (race_session.cop_chase == 0)
                    audio_set_engine_volume(0);
                race_ai_cars[9].chase_target = 0xFFFFFFFF;
            }
        }
    }
    else if (((angle + 2048) & 0xFFF) - 1025 < 2047)
    {
        if (race_ai_cars[slot].recovery_state == 0)
            race_ai_cars[slot].recovery_state = 1;
        if (slot == 9 && race_session.cop_chase == 0)
            audio_set_engine_volume(0);
    }
    node = (sint16)car_states[(car_offset) / 864u].track_location.node;
    valid = 0;
    if (node >= 3)
    {
        threshold = (sint32)track_geometry.main_node_count;
        valid = node < (sint32)((uint32)threshold - 8);
    }
    if (valid == 0)
        valid = race_ai_cars[slot].route_variant != 0;
    if (valid == 0 || race_ai_cars[slot].program_argument != 0 || race_ai_cars[slot].recovery_state != 0)
    {
        car_states[slot].braking = (uint8)(1);
        car_states[car_offset / 864u].pedal = (uint16)(0);
        return car_offset;
    }
    car_states[slot].braking = (uint8)(0);
    val = session_options.ai_recovery_delay_disabled;
    car_states[car_offset / 864u].pedal = (uint16)(60);
    if (val != 0)
        car_states[car_offset / 864u].pedal = (uint16)(0);
    mode = race_ai_cars[slot].reverse;
    follow_traffic_seg(car_offset, mode, &location);
    track_place_location(&location, &point);
    update_ai_steering_target(&race_ai_cars[slot], &point);
    step_ai_steering(&race_ai_cars[slot], 0x8000);
    mode = owner->reverse;
    sel = select_traffic_peer(owner);
    if (mode != 0)
    {
        val = owner->car_slot;
        if (sel == val)
            return sel;
    }
    else if (sel == slot)
        return sel;
    speed = (sint32)owner->route_speed;
    other_speed = (sint32)race_ai_cars[sel].route_speed;
    if (mode != 0 ? speed >= other_speed : other_speed >= speed)
        return slot * 7;
    node = (sint16)car_states[(car_offset) / 864u].track_location.node;
    other_node = (sint16)car_states[(sel * 864) / 864u].track_location.node;
    gap = node - other_node;
    if (mode != 0)
    {
        if (gap >= 25)
            return gap < 4;
        valid = gap < 4;
    }
    else
    {
        if (gap < -24)
            return gap < -3;
        valid = gap >= -3;
    }
    if (valid != 0 && (sint32)car_states[car_offset / 864u].forward_speed > 0)
    {
        car_states[slot].braking = (uint8)(1);
        car_states[car_offset / 864u].pedal = (uint16)(0xFF00);
    }
    time = (sint32)ai_intercept_time(&race_ai_cars[slot], &race_ai_cars[sel]);
    speed = (sint32)car_states[car_offset / 864u].forward_speed;
    quotient = speed / 1024;
    result = quotient < (sint32)((uint32)time - 8);
    if (result != 0 || time < 0)
        return result;
    if (speed <= 0)
        return 1;
    car_states[slot].braking = (uint8)(1);
    car_states[car_offset / 864u].pedal = (uint16)(0xFF00);
    return 0xFFFFFF00;
}

static uint32 update_ai_seg(RACE_AI_CAR *owner)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;

    uint32 car_offset;
    const uint8 *seg;
    const uint8 *vertices;
    uint32 vertex;
    uint32 other_vertex;
    uint32 lane;
    uint32 origin_x;
    uint32 origin_z;
    uint32 type;
    const uint8 *point0;
    const uint8 *other;
    const uint8 *point1;
    uint32 lateral_x;
    uint32 lateral_z;
    uint32 longitudinal_x;
    uint32 longitudinal_z;
    uint32 relative_x;
    uint32 relative_z;
    uint32 coord;
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;
    uint32 product;
    uint32 determinant;
    uint32 numerator;
    uint32 result;
    sint32 node;
    sint32 left;
    sint32 right;
    sint32 difference;

    FUNCTION_MARKER(0x8004CB38, "SLUS_006.10");
    car_offset = owner->car_slot * 864;
    node = (sint16)car_states[(car_offset) / 864u].track_location.node;
    seg = track_geometry.node_bytes;
    vertices = track_geometry.vertex_bytes;
    XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
    seg += (uint32)node * 24;
    vertex = (memcpy(&wire_halfword_1, seg + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    lane = car_states[(car_offset) / 864u].track_location.lane;
    memcpy(&origin_x, seg + 12, sizeof(origin_x));
    other_vertex = (memcpy(&wire_halfword_2, seg + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    type = seg[0];
    memcpy(&origin_z, seg + 20, sizeof(origin_z));
    XASSERT(!(type < 1 || type > 11));
    {
        if ((type == 3 || type == 4 || type == 6 || type == 7) && lane == 0)
            lane = 1;
        XASSERT(!(track_geometry.vertex_count < 2 || vertex + lane >= track_geometry.vertex_count || other_vertex + lane >= track_geometry.vertex_count || ((type == 3 || type == 4) && vertex + lane == 0) || ((type != 3 && type != 4) && vertex + lane >= track_geometry.vertex_count - 1) || ((type == 6 || type == 7) && other_vertex + lane == 0)));
        point0 = vertices + vertex * 6 + lane * 6;
        other = vertices + other_vertex * 6 + lane * 6;
        if (type == 3 || type == 4)
        {
            point1 = point0;
            point0 -= 6;
        }
        else
        {
            point1 = point0 + 6;
            if (type == 6 || type == 7)
                other -= 6;
        }
        left = (sint16)(memcpy(&wire_halfword_3, other, sizeof(wire_halfword_3)), wire_halfword_3);
        right = (sint16)(memcpy(&wire_halfword_4, point0, sizeof(wire_halfword_4)), wire_halfword_4);
        lateral_x = ((left - right) / 2);
        left = (sint16)(memcpy(&wire_halfword_5, other + 4, sizeof(wire_halfword_5)), wire_halfword_5);
        right = (sint16)(memcpy(&wire_halfword_6, point0 + 4, sizeof(wire_halfword_6)), wire_halfword_6);
        lateral_z = ((left - right) / 2);
        car_offset = owner->car_slot * 864;
        coord = (uint32)(car_states[(car_offset) / 864u].pos.x >> 8);
        right = (sint16)(memcpy(&wire_halfword_7, point0, sizeof(wire_halfword_7)), wire_halfword_7);
        difference = (coord - (origin_x + (uint32)right));
        relative_x = (difference / 2);
        car_offset = owner->car_slot * 864;
        coord = (uint32)(car_states[(car_offset) / 864u].pos.z >> 8);
        right = (sint16)(memcpy(&wire_halfword_8, point0 + 4, sizeof(wire_halfword_8)), wire_halfword_8);
        difference = (coord - (origin_z + (uint32)right));
        relative_z = (difference / 2);
        left = (sint16)(memcpy(&wire_halfword_9, point1, sizeof(wire_halfword_9)), wire_halfword_9);
        right = (sint16)(memcpy(&wire_halfword_10, point0, sizeof(wire_halfword_10)), wire_halfword_10);
        longitudinal_x = ((left - right) / 2);
        left = (sint16)(memcpy(&wire_halfword_11, point1 + 4, sizeof(wire_halfword_11)), wire_halfword_11);
        right = (sint16)(memcpy(&wire_halfword_12, point0 + 4, sizeof(wire_halfword_12)), wire_halfword_12);
        longitudinal_z = ((left - right) / 2);
    }
    a = lateral_x;
    b = longitudinal_z;
    product = a * b;
    c = longitudinal_x;
    d = lateral_z;
    result = d * c;
    determinant = product - result;
    if (determinant == 0)
        return result;
    a = relative_x;
    product = a * b;
    a = relative_z;
    numerator = (product - a * c) << 8;
    result = (uint32)fixed_divide_checked((sint32)numerator, (sint32)determinant);
    owner->seg_u = result;
    a = relative_z;
    b = lateral_x;
    product = a * b;
    a = relative_x;
    b = lateral_z;
    numerator = (product - a * b) << 8;
    result = (uint32)fixed_divide_checked((sint32)numerator, (sint32)determinant);
    owner->seg_v = result;
    return result;
}

static uint32 update_ai_car(uint32 slot)
{
    uint32 car_offset;
    VEC3 point;
    RACE_AI_CAR *owner;
    uint32 val;
    const RACE_AI_ROUTE *route;
    const RACE_AI_ROUTE *primary;
    uint32 mapped;
    uint32 tag;
    uint32 offset;
    uint32 step;
    sint32 lane;
    sint32 threshold;
    sint32 sel;
    sint32 lookup;

    FUNCTION_MARKER(0x8004E838, "SLUS_006.10");
    car_offset = slot * 864;
    owner = &race_ai_cars[slot];
    if (race_ai_cars[slot].program != 0)
    {
        if (run_ai_program(&race_ai_cars[slot]) == 0)
            return race_refresh_ai_route_offset(slot);
    }
    else
    {
        val = race_ai_heading_error(&race_ai_cars[slot]);
        if (val - 801 < 0x9BF)
        {
            race_ai_cars[slot].program = ai_program_recover;
            race_ai_cars[slot].program_cursor = ai_program_recover;
        }
    }
    route = owner->route;
    primary = &race_ai.routes[0];
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    threshold = (sint32)track_geometry.main_node_count;
    sel = lane + 4;
    if (sel >= threshold)
        sel = ((uint32)lane - ((uint32)threshold - 4));
    if (route != primary)
    {
        mapped = track_map_branch_node(sel);
        sel = mapped;
        if (mapped == 0xFFFFFFFF)
        {
            lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
            threshold = (sint32)track_geometry.main_node_count;
            sel = lane + 4;
            if (sel >= threshold)
                sel = ((uint32)lane - ((uint32)threshold - 4));
        }
    }
    update_ai_peer_offset(slot);
    lane = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
    threshold = (sint32)track_geometry.main_node_count;
    route = owner->route;
    lookup = lane + 4;
    if (lookup >= threshold)
        lookup = ((uint32)lane - ((uint32)threshold - 4));
    XASSERT(!(route == NULL || (uint32)lookup >= route->count));
    tag = route->samples[(uint32)lookup].lateral;
    offset = race_ai_cars[slot].route_offset;
    compute_ai_target(sel, tag, &point, offset);
    ai_target_side_factor(sel, slot, &point);
    update_ai_steering_target(&race_ai_cars[slot], &point);
    val = apply_ai_throttle(slot);
    step = val != 0 ? 0x10000 : 0x20000;
    return step_ai_steering(&race_ai_cars[slot], step);
}

uint32 race_init_ai(void)
{
    const RACE_AI_ROUTE *route;
    uint32 idx;
    const CAR_PHYSICS_PROFILE **pointer;
    CAR_PHYSICS_PROFILE *data;
    uint32 val;
    sint32 scaled;
    sint32 next;
    sint32 difficulty;

    FUNCTION_MARKER(0x8004B3D8, "SLUS_006.10");
    memset(race_ai_cars, 0, sizeof(race_ai_cars));
    load_ai_routes();
    for (idx = 0; idx < 12; ++idx)
    {
        pointer = &car_states[idx].physics_profile;
        if ((idx == 0 || race_session.multiplayer_mode != 0) && idx < 2 && session_flow.attract_active == 0)
            goto traffic_record;
        if (idx >= 6)
        {
            *pointer = &car_traffic_profile;
            goto traffic_record;
        }
        val = race_session.drag_race;
        data = (CAR_PHYSICS_PROFILE *)*pointer;
        if (val != 0)
        {
            race_ai.trailing_gain = 0;
            race_ai.trailing_limit = 64;
            race_ai.leading_gain = 0;
            goto traffic_set_64;
        }
        *data = car_ai_profile;
        if (car_physics.enhanced_mode != 0)
        {
            scaled = data->drive_coefficient * 650;
            next = data->grip_strength;
            data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
            scaled = next * 380;
            next = data->front_force_weight;
            data->grip_strength = (sint16)(uint16)((uint32)(scaled / 256));
            scaled = next * 450;
            next = data->rear_force_weight;
            data->front_force_weight = (sint16)(uint16)((uint32)(scaled / 256));
            data->rear_force_weight = (sint16)(uint16)((uint32)(next * 400 / 256));
        }
        else if (race_rules.simulation == 0)
        {
            scaled = data->drive_coefficient * 360;
            next = data->grip_strength;
            data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
            data->grip_strength = (sint16)(uint16)((uint32)(next * 300 / 256));
        }
        if (race_session.linear_course != 0)
        {
            difficulty = (sint32)race_rules.difficulty;
            if (difficulty == 0)
            {
                scaled = data->drive_coefficient * 165;
                next = data->grip_strength;
                data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
                data->grip_strength = (sint16)(uint16)((uint32)(next * 220 / 256));
                race_ai.trailing_gain = 140;
                race_ai.leading_gain = 200;
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                data->motion_coefficient_116 = (sint16)(uint16)(929);
                race_ai.trailing_limit = 100;
                race_ai.leading_limit = 55;
            }
            else if (difficulty == 1)
            {
                scaled = data->drive_coefficient * 180;
                data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                data->motion_coefficient_116 = (sint16)(uint16)(929);
                race_ai.trailing_gain = 150;
                race_ai.trailing_limit = 100;
                goto traffic_default_lead;
            }
            else if (difficulty == 2)
                goto traffic_220_280;
        }
        else if (race_session.event_mode == 4)
        {
            difficulty = (sint32)race_rules.difficulty;
            if (difficulty == 0)
            {
                scaled = data->drive_coefficient * 165;
                next = data->grip_strength;
                data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
                data->grip_strength = (sint16)(uint16)((uint32)(next * 220 / 256));
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                data->motion_coefficient_116 = (sint16)(uint16)(929);
                race_ai.trailing_gain = 140;
                race_ai.trailing_limit = 100;
                race_ai.leading_gain = 192;
                goto traffic_set_64;
            }
            else if (difficulty == 1)
            {
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                data->motion_coefficient_116 = (sint16)(uint16)(929);
                race_ai.trailing_gain = 150;
                race_ai.trailing_limit = 100;
                race_ai.leading_gain = 192;
                race_ai.leading_limit = 64;
            }
            else if (difficulty == 2)
                goto traffic_220_280;
        }
        else if (game_options.traffic == 0)
        {
            difficulty = (sint32)race_rules.difficulty;
            if (difficulty == 0)
            {
                scaled = data->drive_coefficient * 200;
                data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                data->motion_coefficient_116 = (sint16)(uint16)(929);
                race_ai.trailing_gain = 160;
                race_ai.trailing_limit = 64;
                goto traffic_default_lead;
            }
            else if (difficulty == 1)
            {
                scaled = data->drive_coefficient * 185;
                data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
                race_ai.trailing_gain = 256;
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                data->motion_coefficient_116 = (sint16)(uint16)(929);
                race_ai.trailing_limit = 75;
                race_ai.leading_gain = 192;
                race_ai.leading_limit = 75;
            }
            else if (difficulty == 2)
            {
                scaled = data->drive_coefficient * 220;
                next = data->grip_strength;
                data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
                data->grip_strength = (sint16)(uint16)((uint32)(next * 270 / 256));
                data->motion_coefficient_116 = (sint16)(uint16)(1075);
                race_ai.trailing_gain = 270;
                race_ai.trailing_limit = 65;
                race_ai.leading_gain = 150;
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                race_ai.leading_limit = 80;
            }
        }
        else
        {
            difficulty = (sint32)race_rules.difficulty;
            if (difficulty == 0)
            {
                scaled = data->drive_coefficient * 200;
                data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
                race_ai.trailing_gain = 180;
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                data->motion_coefficient_116 = (sint16)(uint16)(929);
                race_ai.trailing_limit = 64;
                race_ai.leading_gain = 210;
                race_ai.leading_limit = 64;
            }
            else if (difficulty == 1)
            {
                scaled = data->drive_coefficient * 195;
                next = data->grip_strength;
                data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
                data->grip_strength = (sint16)(uint16)((uint32)(next * 270 / 256));
                data->motion_coefficient_116 = (sint16)(uint16)(953);
                race_ai.trailing_gain = 310;
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                race_ai.trailing_limit = 60;
                race_ai.leading_gain = 192;
                race_ai.leading_limit = 100;
            }
            else if (difficulty == 2)
            {
                scaled = data->drive_coefficient * 220;
                next = data->grip_strength;
                data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
                data->grip_strength = (sint16)(uint16)((uint32)(next * 290 / 256));
                data->motion_coefficient_116 = (sint16)(uint16)(1075);
                race_ai.trailing_gain = 325;
                data->front_force_weight = (sint16)(uint16)(1000);
                data->rear_force_weight = (sint16)(uint16)(1000);
                race_ai.trailing_limit = 60;
                race_ai.leading_gain = 100;
                goto traffic_set_64;
            }
        }
        goto traffic_record;
    traffic_220_280:
        scaled = data->drive_coefficient * 220;
        next = data->grip_strength;
        data->drive_coefficient = (sint16)(uint16)((uint32)(scaled / 256));
        data->grip_strength = (sint16)(uint16)((uint32)(next * 280 / 256));
        data->motion_coefficient_116 = (sint16)(uint16)(1075);
        race_ai.trailing_gain = 200;
        data->front_force_weight = (sint16)(uint16)(1000);
        data->rear_force_weight = (sint16)(uint16)(1000);
        race_ai.trailing_limit = 100;
        race_ai.leading_gain = 120;
        goto traffic_set_64;
    traffic_default_lead:
        race_ai.leading_gain = 192;
        race_ai.leading_limit = 64;
        goto traffic_record;
    traffic_set_64:
        race_ai.leading_limit = 64;
    traffic_record:
        race_ai_cars[idx].car_slot = idx;
        if ((idx & 1) != 0)
        {
            route = &race_ai.routes[0];
            race_ai_cars[idx].route_variant = 0;
        }
        else
        {
            route = &race_ai.routes[1];
            race_ai_cars[idx].route_variant = 1;
        }
        race_ai_cars[idx].route = route;
        val = ai_initial_scale[idx];
        race_ai_cars[idx].chase_target = 0xFFFFFFFF;
        race_ai_cars[idx].throttle_scale = val;
    }
    car_suspension_points[9][0].vx = (sint16)(uint16)(0xFF10);
    car_suspension_points[9][0].vy = (sint16)(uint16)(0xFF79);
    car_suspension_points[9][0].vz = (sint16)(uint16)(520);
    car_suspension_points[9][1].vx = (sint16)(uint16)(240);
    car_suspension_points[9][1].vy = (sint16)(uint16)(0xFF79);
    car_suspension_points[9][1].vz = (sint16)(uint16)(520);
    car_suspension_points[9][2].vx = (sint16)(uint16)(0xFF00);
    car_suspension_points[9][2].vy = (sint16)(uint16)(0xFF79);
    car_suspension_points[9][2].vz = (sint16)(uint16)(0xFE52);
    car_suspension_points[9][3].vx = (sint16)(uint16)(256);
    car_suspension_points[9][3].vy = (sint16)(uint16)(0xFF79);
    car_suspension_points[9][3].vz = (sint16)(uint16)(0xFE52);
    car_geometry_metrics[9].col_radius = (uint16)(800);
    car_geometry_metrics[9].wheel_radius = (uint16)(140);
    car_geometry_metrics[9].wheel_width = (uint16)(220);
    car_states[9].suspension_local[0].vx = (sint16)(uint16)(0xFF10);
    car_states[9].suspension_local[0].vy = (sint16)(uint16)(0xFF79);
    car_states[9].suspension_local[0].vz = (sint16)(uint16)(520);
    car_states[9].suspension_local[1].vx = (sint16)(uint16)(240);
    car_states[9].suspension_local[1].vy = (sint16)(uint16)(0xFF79);
    car_states[9].suspension_local[1].vz = (sint16)(uint16)(520);
    car_states[9].suspension_local[2].vx = (sint16)(uint16)(0xFF00);
    car_states[9].suspension_local[2].vy = (sint16)(uint16)(0xFF79);
    car_states[9].suspension_local[2].vz = (sint16)(uint16)(0xFE52);
    car_states[9].suspension_local[3].vx = (sint16)(uint16)(256);
    car_states[9].suspension_local[3].vy = (sint16)(uint16)(0xFF79);
    car_states[9].suspension_local[3].vz = (sint16)(uint16)(0xFE52);
    return init_traffic();
}

static uint32 init_traffic(void)
{
    uint32 idx;
    const RACE_TRAFFIC_RECORD *rec;
    const uint8 *base;
    uint32 flags;
    uint32 sides;
    uint32 side;
    uint32 node;
    uint32 val;
    sint32 count;

    FUNCTION_MARKER(0x8004EE50, "SLUS_006.10");
    count = (sint32)race_control.car_count;
    if (count < 7)
        return 1;
    for (idx = 6;; ++idx)
    {
        if (count >= 13)
        {
            if (idx >= 12)
                return 0;
        }
        else if ((sint32)idx >= count)
            return 0;
        XASSERT(!(race_ai.traffic.next >= race_ai.traffic.count));
        rec = &race_ai.traffic.records[race_ai.traffic.next];
        flags = rec->flags;
        race_ai_cars[idx].traffic_selection_offset = idx * 4 + 16;
        race_ai_cars[idx].recovery_state = 0;
        race_ai_cars[idx].program_argument = 0;
        race_ai_cars[idx].reverse = flags & 1;
        node = (uint32)rec->node;
        base = track_geometry.node_bytes;
        XASSERT(!(node >= track_geometry.node_count));
        sides = base[node * 24 + 3];
        side = rec->side;
        if (side < (sides & 15))
        {
            race_ai_cars[idx].route_variant = 0;
            node = (uint32)rec->node;
            flags = rec->flags;
            side = rec->side;
            car_place_on_track(idx, node, side, flags & 1);
            track_wrap_location(&car_states[(idx * 864) / 864u].track_location);
            XASSERT(!(race_ai.traffic.next >= race_ai.traffic.count));
            rec = &race_ai.traffic.records[race_ai.traffic.next];
            ++race_ai.traffic.next;
        }
        else
        {
            race_ai_cars[idx].route_variant = 1;
            node = (uint32)rec->node;
            node = track_map_branch_node((sint32)node);
            XASSERT(!(race_ai.traffic.next >= race_ai.traffic.count));
            rec = &race_ai.traffic.records[race_ai.traffic.next];
            val = (uint32)rec->node;
            base = track_geometry.node_bytes;
            flags = rec->flags;
            XASSERT(!(val >= track_geometry.node_count));
            sides = base[val * 24 + 3];
            side = rec->side;
            car_place_on_track(idx, node, side - (sides & 15), flags & 1);
            track_wrap_location(&car_states[(idx * 864) / 864u].track_location);
            XASSERT(!(race_ai.traffic.next >= race_ai.traffic.count));
            rec = &race_ai.traffic.records[race_ai.traffic.next];
            val = (uint16)rec->node;
            ++race_ai.traffic.next;
            car_states[(idx * 864) / 864u].track_location.n_node = (uint16)(val);
        }
        count = (sint32)race_control.car_count;
    }
}

static uint32 respawn_traffic_car(void)
{
    uint32 count;
    const RACE_TRAFFIC_RECORD *cursor;
    uint32 idx;
    uint32 sel;
    uint32 val;
    uint32 car_offset;
    uint32 flags;
    const uint8 *base;
    uint32 sides;
    uint32 side;
    uint32 mapped;
    sint32 node;
    sint32 next_node;
    sint32 player_node;
    sint32 car_node;
    sint32 gap;
    sint32 best_gap;

    FUNCTION_MARKER(0x8004EAD8, "SLUS_006.10");
    count = race_control.car_count;
    if ((sint32)count < 7)
        return 1;
    XASSERT(!(race_ai.traffic.next >= race_ai.traffic.count));
    cursor = &race_ai.traffic.records[race_ai.traffic.next];
    node = cursor->node;
    if (node != -1)
    {
        player_node = (sint16)car_states[0].track_location.n_node;
        do
        {
            node = cursor->node;
            if (node >= player_node && node - player_node >= 40)
                break;
            XASSERT(!(race_ai.traffic.next + 1 >= race_ai.traffic.count));
            next_node = cursor[1].node;
            cursor += 1;
            ++race_ai.traffic.next;
        } while (next_node != -1);
    }
    count = race_control.car_count;
    best_gap = 0;
    idx = 6;
    while ((sint32)count >= 13 ? (sint32)idx < 12 : (sint32)idx < (sint32)count)
    {
        player_node = (sint16)car_states[0].track_location.n_node;
        car_node = (sint16)car_states[(idx * 864) / 864u].track_location.n_node;
        gap = player_node - car_node;
        if (gap > best_gap)
        {
            sel = idx;
            best_gap = gap;
        }
        count = race_control.car_count;
        ++idx;
    }
    if (best_gap < 41)
        return 1;
    XASSERT(!(race_ai.traffic.next >= race_ai.traffic.count));
    cursor = &race_ai.traffic.records[race_ai.traffic.next];
    node = cursor->node;
    if (node == -1)
        return 9;
    if (sel == 9)
    {
        val = race_ai_cars[9].chase_target;
        if (val != 0xFFFFFFFF)
            return val;
    }
    car_offset = sel * 864;
    flags = cursor->flags;
    race_ai_cars[sel].traffic_selection_offset = sel * 4 + 16;
    race_ai_cars[sel].reverse = flags & 1;
    node = cursor->node;
    base = track_geometry.node_bytes;
    XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
    sides = base[(uint32)node * 24 + 3] & 15;
    side = cursor->side;
    if (side < sides)
    {
        race_ai_cars[sel].route_variant = 0;
        node = cursor->node;
        flags = cursor->flags;
        side = cursor->side;
        car_place_on_track(sel, (uint32)node, side, flags & 1);
        track_wrap_location(&car_states[(car_offset) / 864u].track_location);
    }
    else
    {
        race_ai_cars[sel].route_variant = 1;
        node = cursor->node;
        mapped = track_map_branch_node(node);
        XASSERT(!(race_ai.traffic.next >= race_ai.traffic.count));
        cursor = &race_ai.traffic.records[race_ai.traffic.next];
        node = cursor->node;
        base = track_geometry.node_bytes;
        flags = cursor->flags;
        XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
        sides = base[(uint32)node * 24 + 3];
        side = cursor->side;
        car_place_on_track(sel, mapped, side - (sides & 15), flags & 1);
        track_normalize_location(&car_states[(car_offset) / 864u].track_location);
    }
    XASSERT(!(race_ai.traffic.next >= race_ai.traffic.count));
    cursor = &race_ai.traffic.records[race_ai.traffic.next];
    ++race_ai.traffic.next;
    race_ai_cars[sel].recovery_state = 0;
    race_ai_cars[sel].program_argument = 0;
    race_ai_cars[sel].chase_target = 0xFFFFFFFF;
    car_states[car_offset / 864u].forward_speed = 0;
    car_states[sel].steering_angle = 0;
    car_states[car_offset / 864u].motion_mode = (uint8)(0);
    car_states[(car_offset) / 864u].orientation.bits.x = (uint32)(0);
    car_states[(car_offset) / 864u].orientation.bits.z = (uint32)(0);
    car_states[car_offset / 864u].angular_velocity.bits.x = (uint32)(0);
    car_states[car_offset / 864u].angular_velocity.bits.y = (uint32)(0);
    car_states[car_offset / 864u].angular_velocity.bits.z = (uint32)(0);
    car_states[(car_offset) / 864u].velocity.bits.x = (uint32)(0);
    car_states[(car_offset) / 864u].velocity.bits.y = (uint32)(0);
    car_states[(car_offset) / 864u].velocity.bits.z = (uint32)(0);
    car_states[sel].motion_ticks = (uint16)(0);
    return car_offset;
}

uint32 race_update_lap_timer(CAR_STATE *car)
{
    uint16 wire_halfword_1, wire_halfword_2;

    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 result;
    uint32 checkpoint_count;
    uint32 state;
    uint32 lap;
    uint32 timer;
    uint32 dst;
    uint32 idx;
    uint32 val0;
    uint32 val1;
    const uint8 *desc;
    uint32 desc_address;
    uint32 node;
    uint32 bonus;
    uint32 incremented;
    uint32 slot;
    uint32 old_mask;
    sint32 speed;
    sint32 divisor;
    sint32 quotient;
    sint32 correction;

    FUNCTION_MARKER(0x80047F48, "SLUS_006.10");
    result = session_flow.attract_active;
    if (result != 0)
        return result;
    result = race_session.demo;
    if (result != 0)
        return result;
    if (race_session.linear_course != 0 || race_session.two_players != 0)
        if (race_session.drag_race == 0)
            return update_checkpoint_laps(car);
    result = race_control.stopped_players;
    if (result != 0)
    {
        result = race_session.two_players;
        if (result == 0)
        {
            result = race_session.drag_race;
            if (result == 0)
                return result;
        }
    }
    level_desc = &scene_level_image.desc;
    desc_address = level_desc->checkpoints_offset;
    XASSERT(!(desc_address > scene_level_image.size || scene_level_image.size - desc_address < 4));
    checkpoint_count = scene_level_image.bytes[desc_address];
    XASSERT(!(checkpoint_count == 0 || checkpoint_count > CAR_TIMING_HISTORY_COUNT || checkpoint_count > (scene_level_image.size - desc_address - 4) / 4));
    lap = car->completed_laps;
    if (lap >= checkpoint_count)
        return 0;
    timer = car->race_stats.elapsed_ticks;
    car->split_ticks[lap] = (uint16)timer;
    lap = car->completed_laps;
    idx = 0;
    while (idx < lap)
    {
        dst = lap;
        val0 = car->split_ticks[dst];
        val1 = car->split_ticks[idx];
        car->split_ticks[dst] = (uint16)(val0 - val1);
        lap = car->completed_laps;
        ++idx;
    }
    level_desc = &scene_level_image.desc;
    lap = car->completed_laps;
    desc_address = level_desc->checkpoints_offset;
    XASSERT(!(desc_address >= scene_level_image.size));
    desc = scene_level_image.bytes + (desc_address);
    node = (sint16)car->track_location.n_node;
    XASSERT(!(scene_level_image.size < 6 || (size_t)(desc - scene_level_image.bytes) > scene_level_image.size - 6 || lap > (scene_level_image.size - 6 - (size_t)(desc - scene_level_image.bytes)) / 4));
    desc += lap * 4;
    result = (memcpy(&wire_halfword_1, desc + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    if (result != node)
        return result;
    lap = car->completed_laps;
    XASSERT(!(scene_level_image.size < 8 || (size_t)(desc - scene_level_image.bytes) > scene_level_image.size - 8));
    bonus = (memcpy(&wire_halfword_2, desc + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    timer = car->checkpoint_timer;
    incremented = lap + 1;
    car->completed_laps = (uint8)(incremented);
    level_desc = &scene_level_image.desc;
    car->checkpoint_timer = (uint16)(timer + bonus);
    desc_address = level_desc->checkpoints_offset;
    XASSERT(!(desc_address >= scene_level_image.size));
    desc = scene_level_image.bytes + (desc_address);
    result = desc[0];
    if ((incremented & 255) == result)
    {
        if (car->finish_time == 0)
        {
            speed = (sint32)car->forward_speed;
            timer = car->race_stats.elapsed_ticks;
            car->finish_time = (uint32)(timer);
            speed /= 256;
            divisor = speed > 0 ? speed : 1;
            slot = car->slot;
            car->finish_speed_divisor = (uint16)((uint32)divisor);
            val0 = race_ai_cars[slot].seg_u * 1500;
            quotient = fixed_divide_checked((sint32)val0, (sint16)divisor);
            state = race_session.drag_race;
            car->checkpoint_progress = (uint16)((uint32)quotient);
            if (state != 0)
            {
                timer = car->finish_time;
                correction = (sint16)quotient * 1000 / 256;
                car->finish_time = (uint32)(timer * 1000 - (uint32)correction);
            }
        }
        slot = car->slot;
        result = race_view.active_car;
        if (slot == result)
        {
            if (race_session.demo == 0)
            {
                camera_views[slot].sel_mode = 0;
                camera_reset_follow(&car_states[(slot * 864) / 864u].angles, 0, slot);
            }
            old_mask = race_control.stopped_players;
            race_control.players[slot].finished = (uint32)(1);
            race_control.players[slot].finish_frames = (uint32)(0);
            race_control.stopped_players = (uint32)(old_mask | ((uint32)1 << (slot & 31)));
        }
        slot = car->slot;
        for (idx = 0; idx < 6; ++idx)
        {
            result = race_order[idx];
            if (result == slot)
                return result;
        }
        return 0;
    }
    slot = car->slot;
    if (slot == race_view.active_car)
    {
        hud_checkpoint_disp.blink_frames = (uint16)(60);
        slot = car->slot;
    }
    result = race_order[0];
    if (slot == result)
    {
        result = race_session.cup;
        if (result == 0)
        {
            result = car->points + 1;
            car->points = (uint32)(result);
        }
    }
    return result;
}

static uint32 update_checkpoint_laps(CAR_STATE *car)
{
    static const sint32 expected[4] = {0, 1, 3, 7};
    uint32 slot, owner, lap, time, idx, address, val, other, flags, stage;
    uint32 progress, interval;
    sint32 seg, amount, denominator;

    FUNCTION_MARKER(0x80047BF0, "SLUS_006.10");
    slot = car->slot;
    owner = chase_state.pursuer_slot;
    if (slot == owner && race_session.multiplayer_mode == 3)
        return 3;
    XASSERT(!(race_session.laps == 0 || race_session.laps > CAR_TIMING_HISTORY_COUNT));
    lap = car->completed_laps;
    if (lap >= race_session.laps)
        return 0;
    seg = (sint16)car->track_location.n_node;
    time = car->race_stats.elapsed_ticks;
    car->split_ticks[lap] = (uint16)time;
    lap = car->completed_laps;
    idx = 0;
    if (lap != 0)
    {
        do
        {
            address = lap;
            val = car->split_ticks[address];
            other = car->split_ticks[idx];
            car->split_ticks[address] = (uint16)(val - other);
            lap = car->completed_laps;
            ++idx;
        } while (idx < lap);
    }
    if (seg == (sint32)race_session.start_node && (sint16)car->checkpoint_progress == 15)
    {
        lap = car->completed_laps + 1;
        car->completed_laps = (uint8)(lap);
        val = race_session.laps;
        car->checkpoint_progress = (uint16)(0);
        if ((lap & 255) == val)
        {
            if (car->finish_time == 0)
            {
                val = car->forward_speed;
                time = car->race_stats.elapsed_ticks;
                car->finish_time = (uint32)(time);
                amount = (sint32)val / 256;
                if (amount <= 0)
                    amount = 1;
                slot = car->slot;
                car->finish_speed_divisor = (uint16)((uint32)amount);
                val = race_ai_cars[slot].seg_u;
                denominator = (sint16)amount;
                val = (uint32)fixed_divide_checked((sint32)(val * 1500), denominator);
                car->checkpoint_progress = (uint16)(val);
            }
            slot = car->slot;
            if (slot == race_view.active_car)
            {
                if (race_session.demo == 0)
                {
                    camera_views[slot].sel_mode = 0;
                    camera_reset_follow(&car_states[(slot * 864) / 864u].angles, 0, slot);
                }
                flags = race_control.stopped_players;
                race_control.players[slot].finished = (uint32)(1);
                race_control.players[slot].finish_frames = (uint32)(0);
                race_control.stopped_players = (uint32)(flags | ((uint32)1 << (slot & 31)));
            }
        }
    }
    val = race_session.start_node;
    other = track_geometry.main_node_count;
    progress = val * 2;
    interval = ((sint32)(other - progress) / 5);
    for (stage = 0; stage < 4; ++stage)
    {
        if (seg == (sint32)progress)
        {
            amount = (sint16)car->checkpoint_progress;
            if (amount == expected[stage])
                car->checkpoint_progress = (uint16)((uint32)amount | ((uint32)1 << stage));
        }
        progress += interval;
    }
    return 0;
}

uint32 race_apply_point_penalty(CAR_STATE *car, uint32 mode)
{
    uint32 val;

    FUNCTION_MARKER(0x80049310, "SLUS_006.10");
    val = car->finish_time;
    if (val != 0)
        return 4;
    if (race_session.cup != 4)
        return 4;
    val = car->points;
    if ((sint32)val > 0)
        car->points = (uint32)(val - mode);
    val = car->points;
    if ((sint32)val < 0)
        car->points = (uint32)(0);
    return val;
}
