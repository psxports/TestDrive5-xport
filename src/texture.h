#ifndef TEXTURE_H
#define TEXTURE_H

#include "xport.h"

void tex_set_table(uint8 *table, uint32 capacity);
uint8 *tex_find(const char *name);

typedef struct TEXTURE_DESCRIPTOR
{
    char name[8];
    uint16 page;
    uint16 clut;
    uint8 width, height, u, v;
} TEXTURE_DESCRIPTOR;

typedef char TEXTURE_DESCRIPTOR_LAYOUT_CHECK[sizeof(TEXTURE_DESCRIPTOR) == 16 && offsetof(TEXTURE_DESCRIPTOR, page) == 8 && offsetof(TEXTURE_DESCRIPTOR, clut) == 10 ? 1 : -1];

typedef struct TEXTURE_REGION
{
    uint16 u, v, width, height;
} TEXTURE_REGION;

sint32 tex_find_region(uint16 page, uint16 clut, uint16 min_u, uint16 min_v, uint16 max_u, uint16 max_v, TEXTURE_REGION *region);

sint32 tex_upload_tim(const uint8 *image, uint32 capacity, sint16 image_x, sint16 image_y, sint16 pal_x, sint16 pal_y);

#endif
