#ifndef CHASE_H
#define CHASE_H

#include "car.h"
#include "xport.h"

typedef struct CHASE_STATE
{
    uint32 phrase_idx;
    uint32 phrase_ticks;
    uint32 chars0;
    uint32 chars1;
    uint8 arrest_notice_ticks;
    uint32 pursuer_slot;
    uint32 sel_slot;
    uint32 signal_level;
    uint16 arrests;
    uint16 health[6];
} CHASE_STATE;

extern CHASE_STATE chase_state;

uint32 chase_draw_health(uint32 slot);
void chase_draw_hd_health(uint32 slot);
uint32 chase_draw_message(void);
uint32 chase_draw_status(CAR_STATE *car);
uint32 chase_decay_signal(void);
uint32 chase_apply_contact(uint32 slot, sint32 distance);
uint32 chase_init(void);
uint32 chase_raise_signal(void);

uint32 chase_update_braking(void);

#endif
