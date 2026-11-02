#ifndef SAVE_H
#define SAVE_H

#include "psx.h"

enum
{
    SAVE_PAYLOAD_BYTES = 6303
};

uint32 save_encode_progress(uint8 *dst, uint32 capacity);

uint32 save_decode_progress(const uint8 *src, uint32 capacity);

sint32 save_write_game(void);

void save_autosave(void);

sint32 save_load_game(void);

#endif
