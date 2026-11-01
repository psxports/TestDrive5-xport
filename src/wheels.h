#ifndef WHEELS_H
#define WHEELS_H

#include "psx.h"

extern const char wheel_standard_tex_name[8];

uint32 wheels_init_race(void);
uint32 wheels_init_preview(void);

uint32 wheels_draw_race(uint32 slot);
uint32 wheels_draw_preview(void);

uint32 wheels_init_sides(uint32 slot, sint32 wheel_radius, sint32 wheel_width);

#endif
