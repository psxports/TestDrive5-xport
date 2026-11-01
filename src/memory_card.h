#ifndef MEMORY_CARD_H
#define MEMORY_CARD_H

#include "psx.h"

sint32 memory_card_init(void);
uint32 memory_card_shutdown(void);

typedef struct MEMORY_CARD_MENU
{
    uint32 state_frames;
    uint32 message_frames;
    uint32 sel_block;
    uint32 confirmation;
    uint32 grid_column;
    uint32 grid_row;
    uint32 state;
    uint32 overwrite_prompted;
    uint32 parent_mode;
    uint32 return_state;
    uint32 next_state;
    uint32 sel_slot;
} MEMORY_CARD_MENU;

extern MEMORY_CARD_MENU card_menu;
uint32 card_menu_begin(uint32 mode);
void card_menu_update(uint32 mode);

enum
{
    MEMORY_CARD_SLOT_COUNT = 2,
    MEMORY_CARD_BLOCK_COUNT = 15
};

extern const uint32 memory_card_enabled;

#endif
