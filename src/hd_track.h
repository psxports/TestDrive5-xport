#ifndef TD5_HD_TRACK_H
#define TD5_HD_TRACK_H
#include "xport.h"
void hd_track_reset(void);
void hd_track_prepare(void);
void hd_track_prepare_gpu(void);
sint32 hd_track_ready(void);
void hd_track_draw(uint32 slot);
float hd_track_horizon(void);
#endif
