#ifndef SCENE_MEMORY_H
#define SCENE_MEMORY_H

#include "xport.h"

typedef enum SCENE_LEVEL_SOURCE
{
    SCENE_LEVEL_FILE,
    SCENE_LEVEL_EMBEDDED
} SCENE_LEVEL_SOURCE;

typedef struct SCENE_LEVEL_DESCRIPTOR
{
    uint32 header_offset;
    uint32 prefix_offset;
    uint32 auxiliary_2_offset;
    uint32 lighting_offset;
    uint32 auxiliary_4_offset;
    uint32 auxiliary_5_offset;
    uint32 auxiliary_6_offset;
    uint32 stream_intervals_offset;
    uint32 environment_offset;
    uint32 effects_offset;
    uint32 camera_offset;
    uint32 checkpoints_offset;
    uint32 route_hints_offset;
    uint32 sky_ranges_offset;
} SCENE_LEVEL_DESCRIPTOR;

typedef struct SCENE_LEVEL_IMAGE
{
    uint8 *bytes;
    SCENE_LEVEL_DESCRIPTOR desc;
    const char *asset_prefix;
    uint32 size;
    uint32 wire_origin;
    SCENE_LEVEL_SOURCE src;
} SCENE_LEVEL_IMAGE;

extern SCENE_LEVEL_IMAGE scene_level_image;

typedef struct SCENE_TRACK_IMAGE
{
    uint8 *bytes;
    uint32 size;
} SCENE_TRACK_IMAGE;

extern SCENE_TRACK_IMAGE scene_track_image;

typedef struct SCENE_BLOCK
{
    struct SCENE_BLOCK *next;
    struct SCENE_BLOCK *prev;
    uint8 **owner;
    uint32 offset;
    uint32 units;
    uint32 allocated;
} SCENE_BLOCK;

typedef struct SCENE_POOL
{
    uint8 *base;
    SCENE_BLOCK *cursor;
    uint32 bytes;
    uint32 free_bytes;
    uint32 compactions;
    uint32 alloc_failed;
    SCENE_BLOCK *blocks;
} SCENE_POOL;

extern SCENE_POOL scene_pool;

uint8 *scene_pool_alloc(uint32 bytes, uint8 **owner);
void scene_pool_free(uint8 *buffer);

void scene_load_level_metadata(void);

sint32 scene_load_race(void);

#endif
