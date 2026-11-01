#ifndef HUD_H
#define HUD_H

#include "car.h"
#include "xport.h"
#include "psx.h"

typedef struct HUD_CHECKPOINT_DISPLAY
{
    uint16 blink_frames;
    uint8 blink_phase;
} HUD_CHECKPOINT_DISPLAY;

extern HUD_CHECKPOINT_DISPLAY hud_checkpoint_disp;

extern const uint8 *hud_arrow_tex[6];
extern uint32 hud_countdown_frames[2];

uint32 hud_init_scene(void);

uint32 hud_init_race_text(void);
void hud_reset_race_text(void);
uint32 hud_draw_race_text(const char *text, uint32 x, uint32 y, uint32 color);
uint32 hud_link_race_text(uint32 prev_result);
uint32 hud_draw_digit(uint32 slot, sint32 digit, uint32 r, uint32 g, uint32 b);

const uint8 *hud_start_countdown(void);
uint32 hud_update_countdown(void);

uint32 hud_init_drag_start(void);
uint32 hud_draw_drag_start(void);

uint32 hud_init_gauges(void);

uint32 hud_draw_full_view(CAR_STATE *car);

uint32 hud_draw_split_view(CAR_STATE *car, uint32 parameter);

#endif
