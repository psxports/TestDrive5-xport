#ifndef PRESENTATION_RENDER_H
#define PRESENTATION_RENDER_H

#include "psx.h"
#include "fixed_math.h"

typedef struct PRESENTATION_PACKET_STORAGE
{
    POLY_GT4 text[1000];
    POLY_F4 menu_frame[5][2];
    POLY_G4 volume_background[2][2];
    POLY_G4 volume_mask[2][2];
} PRESENTATION_PACKET_STORAGE;

extern PRESENTATION_PACKET_STORAGE presentation_packets;

typedef struct PRESENTATION_MODEL_IMAGE
{
    uint8 *bytes;
    uint32 size;
} PRESENTATION_MODEL_IMAGE;

extern PRESENTATION_MODEL_IMAGE presentation_model_image;

typedef union PRESENTATION_COLOR
{
    RGBC sdk;
    uint32 word;
    uint8 channels[4];
} PRESENTATION_COLOR;

typedef struct PRESENTATION_FONT
{
    const char *tex_name;
    const uint8 *tex;
    uint8 cell_width;
    uint8 cell_height;
    uint8 columns;
    uint8 spacing;
    uint8 widths[128];
} PRESENTATION_FONT;

typedef struct PRESENTATION_RENDER_STATE
{
    PRESENTATION_COLOR pals[8][4];
    PRESENTATION_COLOR colors[4];
    VEC3_16 angles;
    uint32 tab_stop;
    uint32 packet_idx;
    uint32 packet_count;
    PRESENTATION_FONT fonts[5];
} PRESENTATION_RENDER_STATE;

typedef struct PRESENTATION_ITEM
{
    uint8 active;
    uint8 pal;
    uint16 kind;

    union
    {
        char *text;
        const uint8 *tex;
    } content;

    uint16 x_flags;
    sint16 y;
    uint32 param;
    POLY_GT4 packets[2];
} PRESENTATION_ITEM;

typedef struct PRESENTATION_PRES_TEXT
{
    char controls_empty[1];
    char controls_pad_cfg[25];
    char controls_back[5];
    char controls_triangle[9];
    char controls_chris2[7];
    char controls_chris1[7];
    char controls_pad_2[13];
    char controls_pad_1[13];
    char controls_catzmad[8];
    char controls_senistivity_1[24];
    char controls_deadzone_1[28];
    char controls_cfg_1[48];
    char controls_camera[8];
    char controls_horn[6];
    char controls_rear_view[11];
    char controls_hand_brake[12];
    char controls_gear_down[11];
    char controls_gear_up[9];
    char controls_steer_right[13];
    char controls_steer_left[12];
    char controls_brake[7];
    char controls_accelerate[12];
    char controls_no_pad_present[22];
    char controls_ctazmad[8];
    char controls_calibrate[10];
    char controls_senistivity_0[24];
    char controls_deadzone_0[28];
    char controls_cfg_0[47];
    char quick_quick_race[11];
    char quick_back[5];
    char quick_triangle[9];
    char quick_select[7];
    char quick_cross[6];
    char quick_toss[5];
    char quick_chris2[7];
    char quick_chris1[7];
    char quick_srrorght[9];
    char quick_srroleft[9];
    char quick_race[5];
    char quick_car[4];
    char quick_track[6];
    char main_main_menu[10];
    char main_error_loading_prev[23];
    char main_from_memory_card_slot_2[25];
    char main_from_memory_card_slot_1[25];
    char main_testdrive_5_savegame[21];
    char main_loading_prev[17];
    char main_testdrive_5_savegame_2[22];
    char main_searching_for_prev[23];
    char main_empty[1];
    char main_now_playing[12];
    char main_fear_factory_video[19];
    char main_credits[8];
    char main_high_scores[12];
    char main_options[8];
    char main_two_player_race[16];
    char main_full_race[10];
    char multiplayer_two_player_race_type[21];
    char multiplayer_drag_race[10];
    char multiplayer_cop_chase[10];
    char multiplayer_quad_race[10];
    char multiplayer_single_race[12];
    char single_event_select_race_type[17];
    char single_event_continue_cup[13];
    char single_event_cup_race[9];
    char single_event_time_trials[12];
    char cup_choose_cup_to_enter[20];
    char cup_ultimate_cup[13];
    char cup_masters_cup[12];
    char cup_pitbull_cup[12];
    char cup_challenge_cup[14];
    char cup_era_cup_beauty[18];
    char cup_championship_cup[17];
    char options_main_options[13];
    char options_game_options[13];
    char options_scr_cfg[21];
    char options_memory_card_manager[20];
    char options_audio_cfg[20];
    char options_pad_cfg[25];
    char audio_surround_on[15];
    char audio_effect_volume[14];
    char audio_music_volume[13];
    char scr_right[6];
    char scr_left[5];
    char scr_down[5];
    char scr_up[3];
    char settings_circuit_laps_000[27];
    char settings_camera_damping_000[29];
    char settings_vibration_off[17];
    char settings_two_player_catchup_000[33];
    char settings_3d_collisions_on[20];
    char settings_dynamics_simulation[22];
    char settings_difficulty_n[21];
    char settings_cops_on[11];
    char settings_traffic_on[14];
    char settings_checkpoint_timers_on[24];
    char settings_speed_readout_mph[20];
    char track_select_track[13];
    char track_arrorght[9];
    char track_arroleft[9];
    char multiplayer_track_select_tracks[14];
    char credits_hicar[6];
    char records_highscores[11];
    char player_name_enter_your_name[16];
    char player_name_a[2];
    char player_name_b[2];
    char player_name_empty[2];
    char player_name_empty_2[2];
    char player_name_empty_3[2];
    char player_name_empty_4[2];
    char player_name_z[2];
    char player_name_y[2];
    char player_name_x[2];
    char player_name_w[2];
    char player_name_v[2];
    char player_name_u[2];
    char player_name_t[2];
    char player_name_s[2];
    char player_name_r[2];
    char player_name_q[2];
    char player_name_p[2];
    char player_name_o[2];
    char player_name_n[2];
    char player_name_m[2];
    char player_name_l[2];
    char player_name_k[2];
    char player_name_j[2];
    char player_name_i[2];
    char player_name_h[2];
    char player_name_g[2];
    char player_name_f[2];
    char player_name_e[2];
    char player_name_d[2];
    char player_name_c[2];
    char player_name_b_2[2];
    char player_name_a_2[2];
    char event_success_advance[8];
    char event_success_quad_cup[9];
    char event_success_test_drive_5[13];
    char event_success_you_have_won_the[17];
    char event_success_congratulations[16];
    char event_success_player_1[13];
    char event_failure_the_test_drive_5[17];
    char event_failure_you_have_failed_to_win[23];
    char event_failure_sorry[6];
    char cup_success_you_have_unlocked[18];
} PRESENTATION_PRES_TEXT;

extern PRESENTATION_PRES_TEXT presentation_pres_text;
extern const PRESENTATION_PRES_TEXT presentation_pres_text_defaults;

typedef struct PRESENTATION_CARSEL_TEXT
{
    char car_back[5];
    char car_triangle[9];
    char car_select[7];
    char car_cross[6];
    char car_srroleft[9];
    char car_srrorght[9];
    char car_arrorght[9];
    char car_arroleft[9];
    char car_cd[3];
    char card_memory_card_manager[20];
    char card_back[5];
    char card_triangle[9];
    char card_select[7];
    char card_cross[6];
    char card_sq1f[5];
    char card_sq1e[5];
    char card_sq1d[5];
    char card_sq1c[5];
    char card_sq1b[5];
    char card_sq0f[5];
    char card_sq0e[5];
    char card_sq0d[5];
    char card_sq0c[5];
    char card_sq0b[5];
    char card_sq1a[5];
    char card_sq19[5];
    char card_sq18[5];
    char card_sq17[5];
    char card_sq16[5];
    char card_sq0a[5];
    char card_sq09[5];
    char card_sq08[5];
    char card_sq07[5];
    char card_sq06[5];
    char card_sq15[5];
    char card_sq14[5];
    char card_sq13[5];
    char card_sq12[5];
    char card_sq11[5];
    char card_sq05[5];
    char card_sq04[5];
    char card_sq03[5];
    char card_sq02[5];
    char card_sq01[5];
    char card_empty[3];
    char card_empty_2[4];
    char card_2memory_card_status[37];
    char card_1memory_card_status[37];
    char card_memory_card_2[14];
    char card_memory_card_1[14];
    char card_message_window_2[34];
    char card_message_window_1[34];
    char card_delete[7];
    char card_save[5];
    char card_load[5];
    char card_src_reading_memory_cards[21];
    char card_src_please_wait[13];
    char card_src_empty[1];
    char card_src_load_test_drive_5_block[24];
    char card_src_save_test_drive_5_block[24];
    char card_src_delete_block[13];
    char card_src_race[5];
    char card_src_no_cup_race[12];
    char card_src_loaded[8];
    char card_src_select_memory_card_to[22];
    char card_src_load_from[11];
    char card_src_save_to[9];
    char card_src_saving_memory_card[19];
    char card_src_block[7];
    char card_src_error_creating[15];
    char card_src_memory_card_block[19];
    char card_src_block_saved_ok[15];
    char card_src_error_writing_block[20];
    char card_src_memory_card_is_unformatted[27];
    char card_src_format_it[11];
    char card_src_yes[4];
    char card_src_no[3];
    char card_src_formating_memory_card[22];
    char card_src_memory_card[12];
    char card_src_formatted_ok[14];
    char card_src_error_formating[16];
    char card_src_memory_card_2[13];
    char card_src_loading_memory_card[20];
    char card_src_block_2[7];
    char card_src_block_loaded_ok[17];
    char card_src_error_loading_block[21];
    char card_src_overwrite_existing[19];
    char card_src_test_drive_block[18];
    char card_src_delete_memory_card[19];
    char card_src_block_deleted[14];
    char card_src_successfully[14];
    char card_src_error_deleting[15];
    char card_src_no_memory_card[15];
    char card_src_unformatted_memory_card[24];
    char card_src_memory_card_is_full[20];
    char card_src_memory_card_is_ok[18];
} PRESENTATION_CARSEL_TEXT;

extern PRESENTATION_CARSEL_TEXT presentation_carsel_text;
extern const PRESENTATION_CARSEL_TEXT presentation_carsel_text_defaults;

typedef struct PRESENTATION_RECORD
{
    uint16 kind;
    char *content;
    uint16 x_flags;
    sint16 y;
    uint8 pal;
    uint32 action;
    uint32 param;
} PRESENTATION_RECORD;

typedef struct PRESENTATION_RECORD_LIST
{
    PRESENTATION_RECORD *entries;
    uint32 count;
} PRESENTATION_RECORD_LIST;

typedef struct PRESENTATION_PRES_RECORDS
{
    PRESENTATION_RECORD controls[65];
    PRESENTATION_RECORD quick[15];
    PRESENTATION_RECORD main[22];
    PRESENTATION_RECORD multiplayer[12];
    PRESENTATION_RECORD single_event[14];
    PRESENTATION_RECORD cup[14];
    PRESENTATION_RECORD options[13];
    PRESENTATION_RECORD audio[9];
    PRESENTATION_RECORD scr[12];
    PRESENTATION_RECORD settings[17];
    PRESENTATION_RECORD track[10];
    PRESENTATION_RECORD multiplayer_track[7];
    PRESENTATION_RECORD credits[2];
    PRESENTATION_RECORD records[5];
    PRESENTATION_RECORD player_name[41];
    PRESENTATION_RECORD event_success[7];
    PRESENTATION_RECORD event_failure[6];
    PRESENTATION_RECORD cup_success[16];
    PRESENTATION_RECORD cup_failure[6];
} PRESENTATION_PRES_RECORDS;

extern PRESENTATION_PRES_RECORDS presentation_pres_recs;
extern const PRESENTATION_PRES_RECORDS presentation_pres_rec_defaults;

typedef struct PRESENTATION_CARSEL_RECORDS
{
    PRESENTATION_RECORD car[9];
    PRESENTATION_RECORD card[46];
} PRESENTATION_CARSEL_RECORDS;

extern PRESENTATION_CARSEL_RECORDS presentation_carsel_recs;
extern const PRESENTATION_CARSEL_RECORDS presentation_carsel_rec_defaults;

enum
{
    PRESENTATION_LIST_CONTROLS,
    PRESENTATION_LIST_DEBUG,
    PRESENTATION_LIST_QUICK,
    PRESENTATION_LIST_MAIN,
    PRESENTATION_LIST_MULTIPLAYER,
    PRESENTATION_LIST_SINGLE_EVENT,
    PRESENTATION_LIST_CUP,
    PRESENTATION_LIST_OPTIONS,
    PRESENTATION_LIST_AUDIO,
    PRESENTATION_LIST_SCREEN,
    PRESENTATION_LIST_SETTINGS,
    PRESENTATION_LIST_TRACK,
    PRESENTATION_LIST_MULTIPLAYER_TRACK,
    PRESENTATION_LIST_CREDITS,
    PRESENTATION_LIST_RECORDS,
    PRESENTATION_LIST_PLAYER_NAME,
    PRESENTATION_LIST_EVENT_SUCCESS,
    PRESENTATION_LIST_EVENT_FAILURE,
    PRESENTATION_LIST_CUP_SUCCESS,
    PRESENTATION_LIST_CUP_FAILURE,
    PRESENTATION_LIST_CAR,
    PRESENTATION_LIST_CARD,
    PRESENTATION_LIST_COUNT
};

extern const PRESENTATION_RECORD_LIST presentation_rec_lists[PRESENTATION_LIST_COUNT];
sint32 presentation_move_menu_selection(sint32 original, sint32 count);

typedef struct PRESENTATION_MENU_STATE
{
    uint32 sel;
    const PRESENTATION_RECORD_LIST *records;
    uint32 indices[70];
    uint32 finished;
    uint32 banner_x_angle;
    uint32 banner_y_angle;
    uint32 frame_counter;
    uint32 save_load_attempted;
} PRESENTATION_MENU_STATE;

typedef struct PRESENTATION_NAME_EDITOR
{
    uint32 row;
    uint32 column;
    uint32 cursor;
} PRESENTATION_NAME_EDITOR;

extern char presentation_player_name[10];
extern PRESENTATION_NAME_EDITOR presentation_name_editor;

extern PRESENTATION_ITEM presentation_items[70];
extern PRESENTATION_MENU_STATE presentation_menu;

typedef struct PRESENTATION_BACKGROUND_STATE
{
    uint32 image[0x40814 / 4];
    uint32 cycle;
} PRESENTATION_BACKGROUND_STATE;

extern PRESENTATION_BACKGROUND_STATE presentation_background;

void presentation_show_loading(void);

extern const char presentation_background_format[16];
sint32 presentation_upload_background(void);
void presentation_show_credits_legal_images(uint8 *buffer, uint32 capacity);

uint32 presentation_load_items(const PRESENTATION_RECORD_LIST *records);
uint32 presentation_draw_items(void);

extern PRESENTATION_RENDER_STATE presentation_render;
extern const PRESENTATION_RENDER_STATE presentation_render_defaults;

uint32 presentation_init_packets(void);
sint16 *presentation_begin_frame(void);
uint32 presentation_select_pal(uint32 pal);
uint32 presentation_draw_text(uint32 mode, uint32 x, sint32 y, const char *text);
uint32 presentation_draw_image(uint32 mode, uint32 x, sint32 y, const char *name);

uint32 presentation_show_image(uint8 *buffer, uint32 capacity, const char *filename, sint32 seconds, uint32 skippable);

void presentation_load_menu(void);

char *presentation_find_text(char *text, const char *needle);
char *presentation_copy_text(char *dst, const char *src);
uint32 presentation_format_number(char *dst, const char *format, uint32 val);
uint32 presentation_format_time(char *dst, const char *format, uint32 time, sint32 seconds_unit);

extern const VECTOR presentation_menu_view_pos;
extern const SVECTOR presentation_menu_view_angles;

void presentation_wait_loading(void);

uint32 presentation_time_from_ticks(uint32 ticks, uint32 divisor);

uint32 presentation_result_speed(uint32 fixed_speed, uint32 multiplier, uint32 shift);

#endif
