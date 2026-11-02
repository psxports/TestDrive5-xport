#ifndef MODEL_RESOURCE_H
#define MODEL_RESOURCE_H

#include "xport.h"
#include "fixed_math.h"

typedef struct MODEL_HEADER
{
    uint8 format[8];
    uint16 radius;
    uint16 renderer;
    sint16 ordering_bias;
    uint16 visible_vertices;
    VEC3 position;
    VEC3 origin;
    uint8 reserved_28[4];
    uint32 vertex_offset;
    uint32 stream_offset;
    uint32 packet_offsets[2];
} MODEL_HEADER;

typedef char MODEL_HEADER_LAYOUT_CHECK[sizeof(MODEL_HEADER) == 60 && offsetof(MODEL_HEADER, position) == 16 && offsetof(MODEL_HEADER, origin) == 28 && offsetof(MODEL_HEADER, vertex_offset) == 44 && offsetof(MODEL_HEADER, stream_offset) == 48 && offsetof(MODEL_HEADER, packet_offsets) == 52 ? 1 : -1];

typedef struct MODEL_RESOURCE
{
    uint8 *bytes;
    uint32 capacity;
} MODEL_RESOURCE;

#endif
