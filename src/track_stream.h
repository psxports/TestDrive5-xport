#ifndef TRACK_STREAM_H
#define TRACK_STREAM_H

#include "xport.h"

enum
{
    TRACK_LINEAR_PAIR_CAPACITY = 9,
    TRACK_LINEAR_WINDOW_CAPACITY = 9
};

typedef struct TRACK_LINEAR_ENTRY
{
    uint8 *buffer;
    uint32 buffer_bytes;
    uint16 center;
    uint16 section;
    uint8 state;
} TRACK_LINEAR_ENTRY;

typedef struct TRACK_LINEAR_IMAGE
{
    uint8 *bytes;
    uint32 size;
} TRACK_LINEAR_IMAGE;

extern TRACK_LINEAR_IMAGE track_linear_image;

uint32 track_stream_update_linear(uint32 slot, sint32 node);
uint32 track_stream_init_linear(void);

uint32 track_stream_draw_linear(uint32 slot);

uint32 track_stream_draw(void);
uint32 track_stream_init(void);
void track_stream_upload_images(void);
uint32 track_stream_release_pending(void);
uint32 track_stream_wait_for_load(void);
uint32 track_stream_update(sint32 node, sint32 lane);

#endif
