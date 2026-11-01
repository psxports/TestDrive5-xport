#ifndef CREDITS_H
#define CREDITS_H

#include "psx.h"

typedef struct CREDITS_ROW
{
    uint16 y;
    const char *text;
} CREDITS_ROW;

typedef struct CREDITS_ARCHIVE
{
    uint8 *bytes;
    uint32 size;
    uint32 capacity;
} CREDITS_ARCHIVE;

typedef struct CREDITS_STATE
{
    uint32 viewport_flags;
    uint32 cursor;
    uint32 image_idx;
    uint8 *tex;
    CREDITS_ARCHIVE archive;
    CREDITS_ROW rows[11];
} CREDITS_STATE;

extern CREDITS_STATE credits;

void credits_prepare(void);
void credits_update(void);

#endif
