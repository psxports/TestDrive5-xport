#ifndef RACE_MENU_H
#define RACE_MENU_H

#include "psx.h"

typedef struct RACE_MENU_STATE
{
    uint32 state;
    uint32 return_state;
    uint16 sel;
    uint16 items_y;
    uint16 commands[6];
    uint8 replay_disabled;
    uint8 results_disabled;
    const char *unlock_labels[5];
    uint32 unlock_count;
} RACE_MENU_STATE;

extern RACE_MENU_STATE race_menu;

uint32 race_menu_select_default(void);
sint32 race_menu_prepare(void);
sint32 race_menu_run(void);

#endif
