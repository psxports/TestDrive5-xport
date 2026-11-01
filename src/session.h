#ifndef SESSION_H
#define SESSION_H

#include "psx.h"

typedef struct SESSION_CAR_CHOICE
{
    uint8 rec_tag;
    uint8 manual_transmission;
    uint8 model[2];
    uint8 primary_color[2];
    uint8 secondary_color[2];
    uint16 tuning[2][4];
} SESSION_CAR_CHOICE;

typedef union SESSION_CAR_CHOICES
{
    SESSION_CAR_CHOICE cars[6];
    uint32 words[36];
} SESSION_CAR_CHOICES;

extern SESSION_CAR_CHOICES session_car_choices;

typedef struct PROGRESSION_PROFILE
{
    uint8 cup_completion[7];
    uint8 features[5];
    uint8 special_model_30;
    uint8 model_availability[44];
    uint8 track_availability[44];
} PROGRESSION_PROFILE;

extern PROGRESSION_PROFILE progression_profile;

void session_reset_progression(void);
void session_unlock_bonus_tracks(void);
void session_unlock_bonus_models(void);
void session_unlock_features(void);
void session_unlock_special_models(void);

typedef struct SESSION_OPTIONS
{
    uint32 sel_track;
    uint32 retained_model;
    uint32 retained_track;
    uint32 model_variant;
    uint32 fast_attract;
    uint32 ai_recovery_delay_disabled;
    uint32 retained_save_word;
    uint32 spawn_node_override;
    uint32 beast_cars;
} SESSION_OPTIONS;

typedef struct RACE_SESSION
{
    uint32 track;
    uint32 multiplayer_mode;
    uint32 event_mode;
    uint32 cop_chase;
    uint32 cup;
    uint32 two_players;
    uint32 linear_course;
    uint32 drag_race;
    uint32 demo;
    uint32 laps;
    uint32 time_trial;
    uint32 start_node;
} RACE_SESSION;

typedef struct MENU_CHOICES
{
    uint32 main;
    uint32 single_event;
    uint32 legacy_general;
    uint32 car_grp;
    uint32 legacy_vehicle;
    uint32 options_page;
    uint32 settings_row;
    uint32 audio_row;
    uint32 cup;
    uint32 multiplayer;
    uint32 panel_selection;
} MENU_CHOICES;

typedef struct GAME_OPTIONS
{
    uint32 speed_kph;
    uint32 traffic;
    uint32 checkpoint_timers;
    uint32 cops;
    uint32 difficulty;
    uint32 simulation;
    uint32 camera_damping;
    uint32 circuit_laps;
    uint32 catchup;
    uint32 collisions_3d;
    uint32 restrict_primary_models;
    uint32 vibration;
} GAME_OPTIONS;

typedef struct RACE_RULES
{
    uint32 checkpoint_timers;
    uint32 cops;
    uint32 difficulty;
    uint32 collisions_disabled;
    uint32 simulation;
    uint32 camera_damping;
    uint32 catchup;
    uint32 traffic;
    uint16 catchup_scale[2];
} RACE_RULES;

typedef struct SESSION_FLOW
{
    uint32 phase;
    uint32 menu_state;
    uint32 exit_requested;
    uint32 loading_image_shown;
    uint32 menu_idle_fields;
    uint32 menu_initialized;
    uint32 event_started;
    uint32 player_name_entered;
    uint32 card_menu_from_event;
    uint32 event_success;
    uint32 event_round;
    uint32 load_requested;
    uint32 attract_active;
    uint32 attract_idx;
} SESSION_FLOW;

extern const uint8 session_attract_tracks[4];
extern uint16 session_cup_models[13];
extern uint16 session_multiplayer_routes[5];
extern SESSION_FLOW session_flow;
extern RACE_RULES race_rules;
extern MENU_CHOICES menu_choices;
extern GAME_OPTIONS game_options;
extern SESSION_OPTIONS session_options;
extern RACE_SESSION race_session;

sint32 session_apply_menu_mode(void);

void session_edit_options(void);

void session_start_event(void);
void session_advance_event(void);
sint32 session_choose_rivals(void);

uint32 session_update_catchup(void);

void session_tick_menu_idle(void);
void session_award_cup_rewards(void);

uint32 session_run_single_race(void);
sint32 session_run_two_player_race(void);

extern const sint16 session_circuit_tracks[24];
extern const sint16 session_linear_tracks[7];
extern const sint16 session_multiplayer_tracks[7];

enum
{
    MENU_HISTORY_CAPACITY = 16
};

typedef struct MENU_HISTORY
{
    uint32 depth;
    uint8 states[MENU_HISTORY_CAPACITY];
} MENU_HISTORY;

extern MENU_HISTORY menu_history;
void session_reset_menu_history(uint32 idx);
uint32 session_push_menu(uint32 argument);
void session_pop_menu(void);

extern char *const session_event_names[48];

extern char *const session_model_names[44];

extern char *const session_place_names[6];

#endif
