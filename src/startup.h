#ifndef STARTUP_H
#define STARTUP_H

#include "psx.h"

typedef union STARTUP_BUFFER
{
    uint32 alignment;
    uint8 bytes[271040];
} STARTUP_BUFFER;

extern STARTUP_BUFFER startup_buffer;

void startup_show_images(void);

#endif
