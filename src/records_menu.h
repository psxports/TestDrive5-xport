#ifndef RECORDS_MENU_H
#define RECORDS_MENU_H

#include "psx.h"

typedef struct RECORDS_MENU_STATE
{
    uint8 groups[47];
    uint8 count;
    uint32 sel;
} RECORDS_MENU_STATE;

extern RECORDS_MENU_STATE recs_menu;

void recs_menu_begin(void);
void recs_menu_update(void);

#endif
