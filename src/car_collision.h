#ifndef CAR_COLLISION_H
#define CAR_COLLISION_H

#include "psx.h"
#include "car.h"

uint32 car_update_pair_cols(void);
uint32 car_init_pair_cols(void);

typedef struct CAR_PHYSICS_STATE
{
    uint32 gravity;
    uint32 enhanced_mode;
} CAR_PHYSICS_STATE;

extern CAR_PHYSICS_STATE car_physics;

uint32 car_truncate_shift(uint32 val, uint32 bits);

uint32 car_update_grounding(CAR_STATE *car);
void car_reset_grounding(CAR_STATE *car);

uint32 car_load_s_table(void);

intptr_t car_update_motion(uint32 slot);

intptr_t car_update_boundary_motion(uint32 slot);

uint32 car_update_surface_alignment(CAR_STATE *car);

#endif
