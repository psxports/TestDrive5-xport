#ifndef RESULTS_MENU_H
#define RESULTS_MENU_H

#include "psx.h"

typedef struct RESULTS_MENU_TEXT
{
    char model[4];
    char driver[4];
    char cup_pos[20];
    char cup_points[24];
    char cup_time[22];
    char avg_speed[25];
    char top_speed[21];
    char race_pos[20];
    char highest_pos[24];
    char starting_pos[24];
    char police_stops[20];
    char race_time[23];
    char checkpoint_time[28];
    char checkpoint_time_2[12];
    char checkpoint_time_3[12];
    char checkpoint_time_4[12];
    char checkpoint_time_5[12];
    char race_points[25];
    char lap_time[20];
    char drag_time[24];
    char arrests[16];
    char points[20];
    char pos[16];
} RESULTS_MENU_TEXT;

typedef struct RESULTS_MENU
{
    uint32 cur_car;
    uint32 displayed_car;
    uint32 target_offset;
    uint32 offset;
    RESULTS_MENU_TEXT text;
    char *text_slots[23];
} RESULTS_MENU;

extern RESULTS_MENU results_menu;
extern const RESULTS_MENU_TEXT results_menu_text_defaults;
extern char *const results_menu_default_slots[23];

uint32 results_menu_begin(void);
uint32 results_menu_update(void);

#endif
