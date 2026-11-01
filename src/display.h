#ifndef DISPLAY_H
#define DISPLAY_H

#include <stddef.h>

#include "psx.h"

typedef struct DISPLAY_ORDERING
{
    uint32 *tags;
    uint32 byte_offset;
} DISPLAY_ORDERING;

typedef struct DISPLAY_FRAME
{
    DRAWENV draw;
    DISPENV disp;
    uint32 *ordering;
} DISPLAY_FRAME;

typedef struct DISPLAY_STATE
{
    DISPLAY_FRAME frames[2];
    DISPLAY_FRAME *cur;
    uint32 bank;
    uint32 frame_count;
    uint32 ordering_count;
    uint32 field_rate;
    uint32 time_step;
    uint32 pal;
    uint32 mode;
    uint32 pre_sync_ticks;
    uint32 post_sync_ticks;
} DISPLAY_STATE;

typedef struct DISPLAY_OPTIONS
{
    uint32 scr_x;
    uint32 scr_y;
} DISPLAY_OPTIONS;

extern DISPLAY_OPTIONS disp_options;
extern DISPLAY_STATE disp_state;

uint32 disp_prepare(uint32 mode);
DISPLAY_FRAME *disp_swap(void);
uint32 disp_apply(void);
DISPENV *disp_apply_frame(uint32 buffer);

uint32 *disp_init_ordering_tables(uint32 count);
uint32 *disp_clear_ordering_table(void);
void disp_draw_ordering_table(void);

uint32 disp_link_front_packet(DISPLAY_FRAME *frame, uint32 *packet_tag);

void disp_init_graphics(void);

#endif
