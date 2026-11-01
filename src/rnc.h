#ifndef RNC_H
#define RNC_H

#include "xport.h"

uint32 rnc_allocation_size(const uint8 *src);
void rnc_decode(const uint8 *src, uint32 src_capacity, uint8 *dst, sint32 capacity);

#endif
