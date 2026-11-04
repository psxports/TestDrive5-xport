#ifndef TD5_HD_POST_H
#define TD5_HD_POST_H

#include "vid.h"

void hd_post_prepare(void);
void hd_post_reflection_prepare(sint32 width, sint32 height);
sint32 hd_post_reflection_mips(VID_TARGET *target, sint32 width, sint32 height, uint32 levels);
void hd_post_reset(void);
sint32 hd_post_world_begin(sint32 width, sint32 height, const float clear[3]);
sint32 hd_post_world_end(const VID_RECT *views, uint32 count);
void hd_post_submit(VID_RECT view, uint32 index);

#endif
