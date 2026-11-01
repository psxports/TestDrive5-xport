#ifndef CAR_RANDOM_H
#define CAR_RANDOM_H

#include "psx.h"

extern uint32 car_random_states[14];

uint32 car_random_next(uint32 idx);

#endif
