#ifndef EFFECTS_H
#define EFFECTS_H

#include <stddef.h>

#include "car.h"
#include "psx.h"

typedef struct EFFECT_TEXTURE
{
    uint8 u;
    uint8 v;
    uint8 du;
    uint8 dv;
    uint16 tpage;
    uint16 tpage_blend0;
    uint16 clut;
} EFFECT_TEXTURE;

extern EFFECT_TEXTURE effect_tex[8];

void effect_update_all(void);
void effect_draw_all(void);

uint32 effect_spawn_ballistic_spr(CAR_STATE *car, const VEC3 *point, uint32 style);
uint32 effect_spawn_rear_spr(CAR_STATE *car, const VEC3 *point, uint32 type);
uint32 effect_spawn_col_line(const VEC3 *point, CAR_STATE *car);

uint32 effect_init(void);

uint32 effect_update_rear_slip_skids(CAR_STATE *car, uint32 flags, uint32 incoming_result);

uint32 effect_update_rear_grip_skids(CAR_STATE *car, sint32 amount);

uint32 effect_update_front_grip_skids(CAR_STATE *car, sint32 amount);

uint32 effect_update_front_slip_skids(CAR_STATE *car, uint32 flags, uint32 incoming_v0);

/* Control zero also stores the ninth linear-track section */
typedef struct WHEEL_SKID_CONTROL
{
    union
    {
        uint32 section_word;

        struct
        {
            uint8 car_slot;
            uint8 wheel;
            uint8 active;
            uint8 seg;
        };
    };

    uint8 width;
    uint8 target_color;
    uint8 color;
} WHEEL_SKID_CONTROL;

extern WHEEL_SKID_CONTROL wheel_skid_controls[8];

extern uint32 wheel_skid_bank;

uint32 effect_init_wheel_skids(void);

uint32 effect_update_wheel_skids(void);

uint32 effect_draw_wheel_skids(void);

uint32 effect_try_col_line(CAR_STATE *car, const VEC3 *corner);

uint32 effect_init_weather(void);
intptr effect_update_weather(CAR_STATE *car);

#endif
