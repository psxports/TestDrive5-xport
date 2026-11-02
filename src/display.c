#include <string.h>

#include <stdlib.h>

#include "display.h"
#include "psx_gpu.h"
#include "game.h"

typedef char DISPLAY_DRAWENV_SIZE_CHECK[sizeof(DRAWENV) == 92 ? 1 : -1];

typedef char DISPLAY_SAVED_RESULT_CHECK[offsetof(DRAWENV, dr_env) + offsetof(DR_ENV, code) + 12 * sizeof(uint32) == 80 ? 1 : -1];

enum
{
    DISPLAY_ORDERING_CAPACITY = 0x1201
};

static uint32 ordering_tables[2][DISPLAY_ORDERING_CAPACITY];

typedef struct DISPLAY_MODE
{
    sint16 draw_x0;
    sint16 draw_y0;
    sint16 draw_x1;
    sint16 draw_y1;
    sint16 draw_width;
    sint16 draw_height;
    sint16 disp_width;
    sint16 disp_height;
    sint16 scr_x;
    sint16 scr_y;
    sint16 scr_width;
    sint16 x_scale;
    sint16 y_scale;
    uint16 clear_background;
    uint16 dither;
} DISPLAY_MODE;

static const DISPLAY_MODE pal_modes[4] = {{0, 0, 0, 256, 320, 256, 320, 256, 0, 0, 256, 4096, 4096, 0, 0}, {0, 0, 0, 256, 640, 256, 640, 256, 0, 0, 256, 8192, 4096, 0, 0}, {0, 0, 0, 256, 512, 256, 512, 256, 0, 0, 256, 6554, 4096, 0, 0}, {0, 0, 0, 256, 512, 256, 512, 256, 0, 0, 256, 6554, 4096, 1, 0}};

static const DISPLAY_MODE ntsc_modes[4] = {{0, 8, 0, 264, 320, 240, 320, 240, 5, 0, 256, 4096, 3600, 0, 0}, {0, 8, 0, 264, 640, 240, 640, 240, 5, 0, 256, 8192, 3600, 0, 0}, {0, 8, 0, 264, 512, 240, 512, 240, 5, 0, 256, 6554, 3600, 0, 0}, {0, 8, 0, 264, 512, 240, 512, 240, 5, 0, 256, 6554, 3600, 1, 0}};

DISPLAY_STATE disp_state = {.field_rate = 50, .time_step = 256};
DISPLAY_OPTIONS disp_options = {.scr_x = 2, .scr_y = 1};

static uint32 set_video_mode(sint32 mode);

void disp_clock_init(DISPLAY_CLOCK *clock, uint32 fields, uint32 scene)
{
    XASSERT(fields != 0);
    clock->field = xport_frame();
    clock->fields = fields;
    clock->steps = 0;
    clock->scene = scene;
    clock->mode = xport_hd != 0;
}

sint32 disp_clock_step(DISPLAY_CLOCK *clock, uint32 scene)
{
    uint64 field, horizon = 4 * (uint64)clock->fields;
    // service SDK callbacks without an incidental software presentation
    gpu_frame_begin();
    VSync(-1);
    gpu_frame_end();
    field = xport_frame();
    if (field < clock->field || clock->mode != (xport_hd != 0) || clock->scene != scene)
    {
        clock->field = field;
        clock->steps = 0;
        clock->mode = xport_hd != 0;
        clock->scene = scene;
        hd_reseed_race();
    }
    // retain four recent steps and the substep remainder after a long stall
    if (field - clock->field > horizon)
    {
        clock->field = field - horizon - (field - clock->field) % clock->fields;
        hd_reseed_race();
    }
    if (clock->steps >= 4 || field - clock->field < clock->fields)
    {
        hd_present_frame(clock->field, clock->fields);
        clock->steps = 0;
        return 0;
    }
    clock->field += clock->fields;
    ++clock->steps;
    return 1;
}

void disp_draw_ordering_table(void)
{
    uint32 count;
    uint32 *table;
    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8006C8A4, "SLUS_006.10");
    DrawSync(0);
    cur_frame = disp_state.cur;
    count = disp_state.ordering_count;
    table = cur_frame->ordering;
    DrawOTag(table + count - 1);
}

uint32 *disp_clear_ordering_table(void)
{
    sint32 count;
    uint32 *table;
    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8006C79C, "SLUS_006.10");
    cur_frame = disp_state.cur;
    count = (sint32)disp_state.ordering_count;
    table = cur_frame->ordering;
    ClearOTagR(table, count);
    return table;
}

uint32 *disp_init_ordering_tables(uint32 count)
{
    sint32 entries;

    FUNCTION_MARKER(0x8006C514, "SLUS_006.10");
    DrawSync(0);
    {
        sint32 operation_ok = !(count == 0 || count > DISPLAY_ORDERING_CAPACITY || !gpu_register_packet_range(ordering_tables, (uint32)sizeof(ordering_tables)));
        XASSERT(operation_ok);
    }
    disp_state.ordering_count = count;
    disp_state.frames[0].ordering = ordering_tables[0];
    entries = (sint32)disp_state.ordering_count;
    ClearOTagR(disp_state.frames[0].ordering, entries);
    disp_state.frames[1].ordering = ordering_tables[1];
    entries = (sint32)disp_state.ordering_count;
    ClearOTagR(disp_state.frames[1].ordering, entries);
    return disp_state.frames[1].ordering;
}

uint32 disp_apply(void)
{
    uint32 count;

    FUNCTION_MARKER(0x8006C7C8, "SLUS_006.10");
    DrawSync(0);
    PutDrawEnv(&disp_state.cur->draw);
    PutDispEnv(&disp_state.cur->disp);
    count = disp_state.frame_count + 1;
    disp_state.frame_count = count;
    return count;
}

DISPLAY_FRAME *disp_swap(void)
{
    FUNCTION_MARKER(0x8006C868, "SLUS_006.10");
    disp_state.bank ^= 1;
    disp_state.cur = disp_state.frames + disp_state.bank;
    return disp_state.frames;
}

uint32 disp_prepare(uint32 mode)
{
    uint32 pal;
    const DISPLAY_MODE *entry;
    uint32 idx;
    sint32 width;
    sint32 height;
    sint32 x;
    sint32 y;

    FUNCTION_MARKER(0x8006C59C, "SLUS_006.10");
    pal = disp_state.pal;
    model_render.scale = (4096);
    if (pal != 0)
    {
        entry = pal_modes + mode;
        disp_state.field_rate = 50;
        disp_state.time_step = 307;
        set_video_mode(1);
    }
    else
    {
        entry = ntsc_modes + mode;
        disp_state.field_rate = 60;
        disp_state.time_step = 256;
    }
    x = entry->draw_x0;
    y = entry->draw_y0;
    width = entry->draw_width;
    height = entry->draw_height;
    SetDefDrawEnv(&disp_state.frames[0].draw, x, y, width, height);
    x = entry->draw_x1;
    y = entry->draw_y1;
    width = entry->draw_width;
    height = entry->draw_height;
    SetDefDrawEnv(&disp_state.frames[1].draw, x, y, width, height);
    x = entry->draw_x1;
    y = entry->draw_y1;
    width = entry->disp_width;
    height = entry->disp_height;
    SetDefDispEnv(&disp_state.frames[0].disp, x, y, width, height);
    x = entry->draw_x0;
    y = entry->draw_y0;
    width = entry->disp_width;
    height = entry->disp_height;
    SetDefDispEnv(&disp_state.frames[1].disp, x, y, width, height);
    width = entry->draw_width;
    height = entry->draw_height;
    x = entry->x_scale;
    y = entry->y_scale;
    model_render.width = ((uint32)width);
    model_render.height = ((uint32)height);
    model_render.x_scale = ((uint32)x);
    model_render.y_scale = ((uint32)y);
    for (idx = 0; idx < 2; ++idx)
    {
        disp_state.frames[idx].draw.isbg = (uint8)((uint8)(uint16)entry->clear_background);
        disp_state.frames[idx].draw.r0 = (uint8)(0);
        disp_state.frames[idx].draw.g0 = (uint8)(0);
        disp_state.frames[idx].draw.b0 = (uint8)(0);
        disp_state.frames[idx].draw.dtd = (uint8)((uint8)(uint16)entry->dither);
        disp_state.frames[idx].disp.screen.x = (sint16)((uint16)entry->scr_x);
        disp_state.frames[idx].disp.screen.y = (sint16)((uint16)entry->scr_y);
        disp_state.frames[idx].disp.screen.w = (sint16)((uint16)entry->scr_width);
        disp_state.frames[idx].disp.screen.h = (sint16)((uint16)entry->draw_height);
    }
    width = (sint32)model_render.width;
    height = (sint32)model_render.height;
    disp_state.mode = mode;
    model_set_view_center(width / 2, height / 2);
    return model_update_clip_planes();
}

DISPENV *disp_apply_frame(uint32 buffer)
{
    DISPLAY_FRAME *frame = disp_state.frames + buffer;

    FUNCTION_MARKER(0x8006C810, "SLUS_006.10");
    PutDrawEnv(&frame->draw);
    PutDispEnv(&frame->disp);
    return &frame->disp;
}

uint32 disp_link_front_packet(DISPLAY_FRAME *frame, uint32 *packet_tag)
{
    DISPLAY_ORDERING ordering;

    uint32 tag;
    uint32 link;

    ordering.tags = frame->ordering;
    ordering.byte_offset = 0;
    memcpy(&tag, packet_tag, sizeof(tag));
    link = ordering.tags[(sint32)ordering.byte_offset / 4];
    (*packet_tag = (tag & 0xFF000000) | (link & 0xFFFFFF));
    ordering.tags = frame->ordering;
    ordering.byte_offset = 0;
    tag = ordering.tags[(sint32)ordering.byte_offset / 4];
    tag = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)packet_tag);
    ordering.tags[(sint32)ordering.byte_offset / 4] = tag;
    return tag;
}

void disp_init_graphics(void)
{
    FUNCTION_MARKER(0x80015DD0, "SLUS_006.10");
    InitGeom();
    audio_init();
    ResetGraph(3);
    SetGraphDebug(0);
    DrawSync(0);
    SetDispMask(0);
}

static uint32 set_video_mode(sint32 mode)
{
    FUNCTION_MARKER(0x80064110, "SLUS_006.10");
    return (uint32)SetVideoMode(mode);
}
