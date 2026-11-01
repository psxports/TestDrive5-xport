#ifndef TEXTURE_H
#define TEXTURE_H

#include "xport.h"

void tex_set_table(uint8 *table, uint32 capacity);
uint8 *tex_find(const char *name);

sint32 tex_upload_tim(const uint8 *image, uint32 capacity, sint16 image_x, sint16 image_y, sint16 pal_x, sint16 pal_y);

#endif
