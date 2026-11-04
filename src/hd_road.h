#ifndef TD5_HD_ROAD_H
#define TD5_HD_ROAD_H

#include "fixed_math.h"

void hd_road_begin(void);
void hd_road_end(void);
sint32 hd_road_point(VEC3F point);
sint32 hd_road_face(const VEC3F *points, uint32 count);
sint32 hd_road_active(void);

#endif
