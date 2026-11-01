#include "car_random.h"

uint32 car_random_states[14];

static uint32 advance_random_state(uint32 *state)
{
    uint32 val;

    FUNCTION_MARKER(0x8007B3F0, "SLUS_006.10");
    val = *state * 31415821 + 1;
    *state = val;
    return (val << 24) | (val >> 8);
}

uint32 car_random_next(uint32 idx)
{
    FUNCTION_MARKER(0x8004921C, "SLUS_006.10");
    return advance_random_state(&car_random_states[idx]);
}
