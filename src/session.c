#include <string.h>
#include <stdlib.h>

#include "session.h"
#include "game.h"

typedef char SESSION_CAR_CHOICE_SIZE_CHECK[sizeof(SESSION_CAR_CHOICE) == 24 ? 1 : -1];

typedef char SESSION_CAR_CHOICES_SIZE_CHECK[sizeof(SESSION_CAR_CHOICES) == 144 ? 1 : -1];

typedef char PROGRESSION_PROFILE_SIZE_CHECK[sizeof(PROGRESSION_PROFILE) == 101 ? 1 : -1];

char *const session_place_names[6] = {
    "1ST", "2ND", "3RD", "4TH", "5TH", "6TH",
};

char *const session_model_names[44] = {
    "1998 DODGE VIPER", "1998 CHEVROLET CORVETTE", "1998 ASTON MARTIN V8 VANTAGE", "1998 TVR CERBERA", "1998 JAGUAR XKR", "1998 SALEEN MUSTANG S351", "1997 CHEVROLET CAMARO Z28 SS LT4 ", "1998 NISSAN SKYLINE", "HOT DOG", "1966 SHELBY COBRA 427SC", "1967 PONTIAC GTO", "1968 1/2 FORD MUSTANG 428CJ", "1969 CHEVROLET CAMARO ZL1", "1969 CHEVROLET CORVETTE ZL1", "1970 CHEVROLET CHEVELLE SS 454 LS6", "1971 PLYMOUTH HEMI CUDA", "1969 DODGE CHARGER", "1999 ASTON MARTIN PROJECT VANTAGE", "1998 SHELBY SERIES 1", "1998 TVR SPEED 12", "1998 DODGE VIPER GTS-R", "1994 JAGUAR XJ220", "CATERHAM SUPER 7", "CHRIS'S BEAST", "CATERHAM 21", "POLICE TVR CERBERA", "1969 DODGE CHARGER DAYTONA", "1969 CHEVROLET CAMARO HOTROD", "1968 FORD MUSTANG HOTROD", "BEHOLD THE MIGHTY MAUL!", "FEAR FACTORY WAGON", "1998 FORD MUSTANG GT", "NISSAN R390 GT-1", "1998 POLICE FORD MUSTANG", "1969 POLICE DODGE CHARGER", "1997 POLICE CHEVROLET CAMARO", "PITBULL SPECIAL", "", "", "", "", "", "", "",
};

char *const session_event_names[48] = {
    "",
    "KESWICK, ENGLAND",
    "SAN FRANCISCO, CA, USA",
    "BERN, SWITZERLAND",
    "KYOTO, JAPAN",
    "WASHINGTON, DC, USA",
    "MUNICH, GERMANY",
    "KESWICK, ENGLAND 2",
    "SAN FRANCISCO, CA, USA 2",
    "BERN, SWITZERLAND 2",
    "KYOTO, JAPAN 2",
    "WASHINGTON, DC, USA 2",
    "MUNICH, GERMANY 2",
    "HONOLULU, HAWAII, USA",
    "SYDNEY, AUSTRALIA",
    "TOKYO, JAPAN",
    "EDINBURGH, SCOTLAND",
    "BLUE RIDGE PARKWAY, NC, USA",
    "HONOLULU, HAWAII, USA 2",
    "SYDNEY, AUSTRALIA 2",
    "TOKYO, JAPAN 2",
    "EDINBURGH, SCOTLAND 2",
    "BLUE RIDGE PARKWAY, NC, USA DUSK",
    "MOSCOW, RUSSIA",
    "MOSCOW, RUSSIA NIGHT",
    "CHEDDAR GORGE CIRCUIT, ENGLAND",
    "JARASH CIRCUIT, JORDAN",
    "COURMAYEUR CIRCUIT, ITALY",
    "MAUI CIRCUIT, HAWAII, USA",
    "NEWCASTLE CIRCUIT, ENGLAND",
    "DRAGSTRIP",
    "DUAL DRAGSTRIP",
    "CHEDDAR GORGE CIRCUIT, ENGLAND",
    "JARASH CIRCUIT, JORDAN",
    "COURMAYEUR CIRCUIT, ITALY",
    "MAUI CIRCUIT, HAWAII, USA",
    "NEWCASTLE CIRCUIT, ENGLAND",
    "BEZ'S FLAT CIRCUIT",
    "BEZ'S FLAT CIRCUIT",
    "MONTEGO BAY CIRCUIT, JAMAICA",
    "MONTEGO BAY CIRCUIT, JAMAICA",
    "CHAMPIONSHIP CUP",
    "CHALLENGE CUP",
    "PITBULL CUP",
    "MASTERS CUP",
    "ULTIMATE CUP",
    "ERA CUP",
    "COP CHASE",
};

MENU_HISTORY menu_history;

static const uint16 rival_models[14] = {0, 1, 2, 3, 4, 6, 7, 9, 10, 12, 13, 14, 15, 16};
static const uint16 easy_rival_models[6] = {14, 12, 4, 7, 15, 16};
static const uint16 medium_rival_models[6] = {16, 2, 1, 13, 9, 0};
static const uint16 hard_rival_models[6] = {32, 26, 17, 18, 20, 3};
uint16 session_cup_models[13];

RACE_RULES race_rules = {.checkpoint_timers = 1, .simulation = 1, .camera_damping = 40, .catchup_scale = {256, 256}};
const uint8 session_attract_tracks[4] = {1, 0, 22, 16};
SESSION_CAR_CHOICES session_car_choices;
PROGRESSION_PROFILE progression_profile;

static const uint32 cup_track_rewards[5] = {2, 1, 5, 3, 4};
static const uint32 cup_model_rewards[5] = {21, 20, 19, 17, 32};
static const uint32 cup_special_model_rewards[6] = {29, 36, 31, 23, 8, 25};
SESSION_FLOW session_flow;
SESSION_OPTIONS session_options = {.retained_model = 0x7FFF, .retained_track = 0x7FFF};
RACE_SESSION race_session = {.laps = 1, .start_node = 100};
MENU_CHOICES menu_choices;
GAME_OPTIONS game_options = {.traffic = 1, .checkpoint_timers = 1, .cops = 1, .difficulty = 1, .camera_damping = 6, .circuit_laps = 3, .catchup = 4, .collisions_3d = 1, .restrict_primary_models = 1, .vibration = 1};

static const sint16 championship_open_tracks[4] = {23, 16, 14, 17};
static const sint16 championship_first_tracks[6] = {23, 16, 14, 17, 6, 13};
static const sint16 championship_second_tracks[8] = {23, 16, 14, 17, 6, 13, 2, 4};
static const sint16 championship_third_tracks[10] = {23, 16, 14, 17, 6, 13, 2, 4, 1, 3};
static const sint16 championship_fourth_tracks[12] = {23, 16, 14, 17, 6, 13, 2, 4, 1, 3, 5, 15};
static const sint16 championship_linear_tracks[6] = {26, 25, 28, 27, 29, 39};

static const sint16 *const championship_tracks[6] = {
    championship_open_tracks, championship_first_tracks, championship_second_tracks, championship_third_tracks, championship_fourth_tracks, championship_linear_tracks,
};

/* Preserve terminal reads from their actual neighboring owners */
static const sint16 *const championship_terminal_tracks[6] = {0, championship_second_tracks, championship_third_tracks, championship_fourth_tracks, session_linear_tracks, championship_first_tracks};

sint32 session_apply_menu_mode(void)
{
    uint32 result;
    uint32 selection;

    FUNCTION_MARKER(0x8001A4D8, "SLUS_006.10");
    race_session.event_mode = (0);
    race_session.cup = (0);
    race_session.multiplayer_mode = (0);
    race_session.demo = (0);
    race_session.cop_chase = (0);
    race_session.drag_race = (0);
    race_session.two_players = (0);
    result = menu_choices.main;
    if (result == 0)
    {
        race_session.event_mode = (0);
    }
    else if (result == 1)
    {
        result = menu_choices.single_event;
        if (result < 6)
        {
            selection = menu_choices.single_event;
            result = 0x800107BC + selection * 4;
            switch (selection)
            {
                case 0:
                    race_session.event_mode = (0);
                    break;
                case 1:
                    result = 1;
                    race_session.event_mode = (result);
                    break;
                case 2:
                case 3:
                    race_session.event_mode = (2);
                    result = menu_choices.cup;
                    if (result < 6)
                    {
                        selection = menu_choices.cup;
                        result = 0x800107D4 + selection * 4;
                        switch (selection)
                        {
                            case 0:
                                race_session.cup = (0);
                                break;
                            case 1:
                                result = 5;
                                race_session.cup = (result);
                                break;
                            case 2:
                            case 3:
                            case 4:
                            case 5:
                                result = selection - 1;
                                race_session.cup = (result);
                                break;
                        }
                    }
                    break;
                case 4:
                    race_session.event_mode = (3);
                    result = 1;
                    race_session.drag_race = (result);
                    break;
                case 5:
                    race_session.cop_chase = (1);
                    result = 4;
                    race_session.event_mode = (result);
                    break;
            }
        }
    }
    else if (result == 2)
    {
        race_session.two_players = (1);
        race_session.event_mode = (5);
        result = menu_choices.multiplayer;
        if (result < 3)
        {
            ++result;
            race_session.multiplayer_mode = (result);
        }
        else if (result == 3)
        {
            race_session.multiplayer_mode = (4);
            result = 1;
            race_session.drag_race = (result);
        }
    }
    return result;
}

void session_edit_options(void)
{
    char *dst;
    const char *format;
    uint32 sel, limit, val;
    uint32 *field;

    FUNCTION_MARKER(0x800AB3B0, "SLUS_006.10");
    if ((input_ports[0].pressed & 0xA0) != 0)
        audio_play_menu_high();
    sel = menu_choices.settings_row;
    field = 0;
    limit = 0;
    switch (sel)
    {
        case 0:
            field = &game_options.speed_kph;
            break;
        case 1:
            field = &game_options.checkpoint_timers;
            break;
        case 2:
            field = &game_options.traffic;
            break;
        case 3:
            field = &game_options.cops;
            break;
        case 4:
            field = &game_options.difficulty;
            limit = 2;
            break;
        case 5:
            field = &game_options.simulation;
            break;
        case 6:
            field = &game_options.collisions_3d;
            break;
        case 7:
            field = &game_options.catchup;
            limit = 9;
            break;
        case 8:
            field = &game_options.vibration;
            break;
        case 9:
            field = &game_options.camera_damping;
            limit = 9;
            break;
        case 10:
            field = &game_options.circuit_laps;
            limit = 3;
            break;
        default:
            break;
    }
    if (field != 0)
    {
        if (limit == 0)
        {
            if ((input_ports[0].pressed & 0xA0) != 0)
                *field = (*field ^ 1);
        }
        else
        {
            if ((input_ports[0].pressed & 0x80) != 0)
            {
                val = *field;
                if ((sint32)val > 0)
                    *field = (val - 1);
            }
            if ((input_ports[0].pressed & 0x20) != 0)
            {
                val = *field;
                if ((sint32)val < (sint32)limit)
                    *field = (val + 1);
            }
        }
    }
    dst = presentation_find_text(presentation_items[0].content.text, " - ");
    format = game_options.speed_kph != 0 ? "KPH" : "MPH";
    presentation_copy_text(dst + 3, format);
    dst = presentation_find_text(presentation_items[1].content.text, " - ");
    format = game_options.checkpoint_timers != 0 ? "ON " : "OFF";
    presentation_copy_text(dst + 3, format);
    dst = presentation_find_text(presentation_items[2].content.text, " - ");
    format = game_options.traffic != 0 ? "ON " : "OFF";
    presentation_copy_text(dst + 3, format);
    dst = presentation_find_text(presentation_items[3].content.text, " - ");
    format = game_options.cops != 0 ? "ON " : "OFF";
    presentation_copy_text(dst + 3, format);
    dst = presentation_find_text(presentation_items[4].content.text, " - ");
    val = game_options.difficulty;
    if (val == 0)
    {
        presentation_copy_text(dst + 3, "EASY  ");
        val = game_options.difficulty;
    }
    if (val == 1)
    {
        presentation_copy_text(dst + 3, "NORMAL");
        val = game_options.difficulty;
    }
    if (val == 2)
        presentation_copy_text(dst + 3, "HARD  ");
    dst = presentation_find_text(presentation_items[5].content.text, " - ");
    format = game_options.simulation != 0 ? "SIMULATION" : "ARCADE    ";
    presentation_copy_text(dst + 3, format);
    dst = presentation_find_text(presentation_items[6].content.text, " - ");
    format = game_options.collisions_3d != 0 ? "ON " : "OFF";
    presentation_copy_text(dst + 3, format);
    dst = presentation_find_text(presentation_items[7].content.text, " - ");
    val = game_options.catchup;
    presentation_format_number(dst + 3, "%d", val + 1);
    dst = presentation_find_text(presentation_items[8].content.text, " - ");
    format = game_options.vibration != 0 ? "ON " : "OFF";
    presentation_copy_text(dst + 3, format);
    dst = presentation_find_text(presentation_items[9].content.text, " - ");
    val = game_options.camera_damping;
    presentation_format_number(dst + 3, "%d", val + 1);
    dst = presentation_find_text(presentation_items[10].content.text, " - ");
    val = game_options.circuit_laps;
    presentation_format_number(dst + 3, "%d", val + 1);
}

void session_start_event(void)
{
    static const uint32 rival_classes[6] = {0, 1, 1, 2, 2, 0};
    uint32 mode;
    uint32 variant;
    uint32 val;

    FUNCTION_MARKER(0x80018DB4, "SLUS_006.10");
    recs_reset_candidates();
    session_flow.event_round = (uint32)(0);
    session_flow.event_started = (1);
    session_flow.event_success = 0;
    session_options.model_variant = (uint32)(0);
    race_session.time_trial = (0);
    race_session.demo = (0);
    race_rules.difficulty = (game_options.difficulty);
    race_rules.cops = (game_options.cops);
    race_rules.checkpoint_timers = (game_options.checkpoint_timers);
    race_rules.collisions_disabled = (game_options.collisions_3d ^ 1);
    race_rules.simulation = (game_options.simulation);
    race_session.laps = (game_options.circuit_laps + 1);
    race_rules.camera_damping = (game_options.camera_damping * 5 + 15);
    race_rules.catchup = (game_options.catchup * 7);
    if ((sint32)race_rules.catchup < 0)
        val = 0;
    else if ((sint32)race_rules.catchup >= 65)
        val = 64;
    else
        val = race_rules.catchup;
    race_rules.catchup = (val);
    race_rules.traffic = (game_options.traffic);
    mode = race_session.event_mode;
    if (mode < 6)
    {
        mode = race_session.event_mode;
        switch (mode)
        {
            case 0:
                race_session.track = (session_options.sel_track);
                scene_load_level_metadata();
                session_choose_rivals();
                race_reset_results();
                if ((sint32)race_session.track >= 24)
                    race_rules.checkpoint_timers = (0);
                break;
            case 1:
                race_session.time_trial = (1);
                race_rules.checkpoint_timers = (0);
                race_session.track = (session_options.sel_track);
                scene_load_level_metadata();
                break;
            case 2:
                variant = race_session.cup;
                if (variant < 6)
                {
                    variant = race_session.cup;
                    if (variant < 6)
                    {
                        if (variant != 5)
                            race_rules.traffic = (1);
                        race_rules.cops = (1);
                        race_rules.difficulty = (rival_classes[variant]);
                        if (variant == 5)
                            race_session.laps = (4);
                        race_rules.checkpoint_timers = (0);
                        val = session_flow.event_round;
                        race_session.track = ((uint32)(sint32)championship_tracks[variant][val] - 1);
                        scene_load_level_metadata();
                        session_choose_rivals();
                        race_reset_results();
                    }
                }
                break;
            case 3:
                race_rules.checkpoint_timers = (0);
                race_rules.difficulty = (1);
                race_session.track = (29);
                scene_load_level_metadata();
                session_choose_rivals();
                break;
            case 4:
                race_rules.checkpoint_timers = (0);
                race_rules.cops = (0);
                race_session.track = (session_options.sel_track);
                scene_load_level_metadata();
                session_choose_rivals();
                break;
            case 5:
                race_rules.cops = (0);
                variant = race_session.multiplayer_mode;
                switch (variant)
                {
                    case 1:
                        race_rules.checkpoint_timers = (0);
                        race_session.track = (session_options.sel_track);
                        scene_load_level_metadata();
                        session_choose_rivals();
                        break;
                    case 2:
                        race_session.laps = (4);
                        race_rules.checkpoint_timers = (0);
                        val = session_flow.event_round;
                        race_session.track = ((uint32)(sint32)(sint16)(session_multiplayer_routes[val]));
                        scene_load_level_metadata();
                        session_choose_rivals();
                        race_reset_results();
                        break;
                    case 3:
                        race_session.cop_chase = (1);
                        chase_state.pursuer_slot = session_flow.event_round;
                        race_session.laps = (4);
                        race_rules.checkpoint_timers = (0);
                        race_session.track = (session_options.sel_track);
                        scene_load_level_metadata();
                        session_choose_rivals();
                        race_reset_results();
                        break;
                    case 4:
                        race_rules.difficulty = (1);
                        race_rules.checkpoint_timers = (0);
                        race_session.track = (30);
                        scene_load_level_metadata();
                        session_choose_rivals();
                        break;
                }
                break;
        }
    }
    session_flow.phase = (1);
    return;
}

void session_advance_event(void)
{
    static const uint32 rival_classes[6] = {0, 1, 1, 2, 2, 0};
    static const sint32 stage_counts[6] = {4, 6, 8, 10, 12, 6};
    uint32 mode;
    uint32 variant;
    uint32 stage;
    uint32 winner;
    uint32 result;

    FUNCTION_MARKER(0x80019458, "SLUS_006.10");
    race_session.demo = (0);
    session_flow.event_started = (1);
    session_flow.phase = (1);
    mode = race_session.event_mode;
    if (mode < 6)
    {
        mode = race_session.event_mode;
        switch (mode)
        {
            case 2:
                variant = race_session.cup;
                if (variant < 6)
                {
                    variant = race_session.cup;
                    race_rules.traffic = (1);
                    race_rules.cops = (1);
                    race_rules.difficulty = (rival_classes[variant]);
                    race_rules.checkpoint_timers = (0);
                    stage = session_flow.event_round;
                    if (stage == (uint32)stage_counts[variant])
                    {
                        if (variant == 0)
                            race_session.track = (uint32)(sint32)(sint16)(uint16)session_options.ai_recovery_delay_disabled - 1;
                        else
                            race_session.track = (uint32)(sint32)*championship_terminal_tracks[variant] - 1;
                    }
                    else
                        race_session.track = (uint32)(sint32)championship_tracks[variant][stage] - 1;
                    if ((sint32)(session_flow.event_round) >= stage_counts[variant])
                    {
                        if (variant != 0)
                            race_session.track = (0);
                        race_results.entries[0].speed_sum = (uint16)((uint16)(race_results.entries[0].speed_sum / (uint32)stage_counts[variant]));
                        recs_update_candidate((sint32)(40 + variant));
                        if (variant == 0 || variant == 4)
                            race_rank_scores();
                        else
                            race_rank_times();
                        session_flow.menu_state = (170);
                        session_flow.phase = (0);
                        if (variant == 0)
                        {
                            if ((sint16)race_results.entries[0].place == 0)
                                session_flow.event_success = 1;
                        }
                        else
                        {
                            winner = race_order[0];
                            if (race_results.entries[winner].player_tag == 1)
                                session_flow.event_success = 1;
                        }
                    }
                }
                break;
            case 4:
                race_rules.cops = (0);
                break;
            case 5:
                race_rules.cops = (0);
                variant = race_session.multiplayer_mode;
                if (variant == 3)
                {
                    chase_state.pursuer_slot = session_flow.event_round;
                    scene_load_level_metadata();
                    session_choose_rivals();
                }
                else if (variant == 2)
                {
                    race_rules.checkpoint_timers = (0);
                    stage = session_flow.event_round;
                    race_session.track = ((uint32)(sint32)(sint16)(session_multiplayer_routes[stage]));
                    if ((sint32)(session_flow.event_round) >= 4)
                    {
                        race_rank_times();
                        session_flow.menu_state = (240);
                        session_flow.phase = (0);
                        winner = race_order[0];
                        if (race_results.entries[winner].player_tag != 0)
                            session_flow.event_success = 1;
                    }
                }
                break;
        }
    }
    result = session_flow.phase;
    if (result == 1)
        scene_load_level_metadata();
}

sint32 session_choose_rivals(void)
{
    uint32 state;
    uint32 first_slot;
    uint32 slot;
    uint32 prev;
    uint32 cycle = 0;
    const uint16 *table = rival_models;
    sint32 candidate;
    sint32 sequential = 0;
    sint32 duplicate;

    FUNCTION_MARKER(0x80048FC8, "SLUS_006.10");
    state = race_session.demo;
    if (state != 0)
        return state;
    first_slot = 2;
    if (race_session.two_players == 0 && race_session.drag_race == 0)
        first_slot = 1;
    for (slot = first_slot; slot < 6; ++slot)
        session_car_choices.cars[slot].model[0] = (uint8)(255);
    state = race_rules.difficulty;
    if (state == 0)
    {
        sequential = 1;
        table = easy_rival_models;
    }
    if (state == 1)
    {
        sequential = 1;
        table = medium_rival_models;
    }
    if (state == 2)
    {
        sequential = 1;
        table = hard_rival_models;
    }
    if (race_session.cup == 5)
    {
        state = session_options.beast_cars;
        table = state == 0 ? car_selection_standard_models : car_selection_beast_models;
        sequential = 0;
    }
    for (slot = first_slot; slot < 6; ++slot)
    {
        for (;;)
        {
            if (sequential != 0)
            {
                candidate = (sint16)table[cycle];
                cycle = (cycle + 1) % 6;
            }
            else
            {
                candidate = (sint16)table[(uint32)(randPSX() % 14)];
            }
            duplicate = 0;
            for (prev = 0; prev < slot; ++prev)
            {
                if ((sint8)session_car_choices.cars[prev].model[0] == candidate || candidate == 5)
                {
                    duplicate = 1;
                    break;
                }
            }
            if (duplicate == 0)
                break;
        }
        session_car_choices.cars[slot].primary_color[0] = (uint8)(0);
        session_car_choices.cars[slot].secondary_color[0] = (uint8)(0);
        session_car_choices.cars[slot].model[0] = (uint8)((uint8)candidate);
    }
    return 0;
}

uint32 session_update_catchup(void)
{
    sint32 path0, path1, limit, difference;
    uint32 val;

    FUNCTION_MARKER(0x8002740C, "SLUS_006.10");
    path0 = (sint16)car_states[0].track_location.path_count;
    path1 = (sint16)car_states[1].track_location.path_count;
    if (path0 < path1)
    {
        limit = (sint32)race_rules.catchup;
        difference = (path1 - path0) * 2;
        if (difference < limit)
            limit = difference;
        race_rules.catchup_scale[0] = ((uint32)limit + 256);
        val = 256 - (uint32)limit;
    }
    else
    {
        limit = (sint32)race_rules.catchup;
        difference = (path0 - path1) * 2;
        if (difference < limit)
            limit = difference;
        race_rules.catchup_scale[0] = (256 - (uint32)limit);
        val = (uint32)limit + 256;
    }
    race_rules.catchup_scale[1] = (val);
    return val;
}

void session_tick_menu_idle(void)
{
    FUNCTION_MARKER(0x800A994C, "SLUS_006.10");
    session_flow.menu_idle_fields = (session_flow.menu_idle_fields + 1);
    if ((sint16)input_ports[0].held != 0)
        session_flow.menu_idle_fields = (0);
    if ((sint16)input_ports[1].held != 0)
        session_flow.menu_idle_fields = (0);
    if (session_options.fast_attract != 0)
        session_flow.menu_idle_fields = (session_flow.menu_idle_fields + 120);
    if ((sint32)session_flow.menu_idle_fields >= 7200)
    {
        session_flow.menu_idle_fields = (0);
        session_flow.menu_state = (140);
    }
}

uint32 session_run_single_race(void)
{
    uint32 result;

    FUNCTION_MARKER(0x80015BE8, "SLUS_006.10");
    session_flow.phase = (3);
    if (race_session.linear_course == 0)
        track_stream_wait_for_load();
    race_run_single();
    input_set_motors(0, 0, 0);
    input_set_motors(1, 0, 0);
    if (session_flow.attract_active != 0)
    {
        session_flow.menu_state = (80);
        session_flow.phase = (0);
        session_flow.attract_active = (0);
        return 80;
    }
    result = replay_mode;
    if (result != 0)
        return result;
    result = race_control.exit_reason;
    if (result != 0)
        return result;
    result = race_session.event_mode;
    if (result != 2)
        return recs_update_candidate((sint32)race_session.track);
    return result;
}

sint32 session_run_two_player_race(void)
{
    FUNCTION_MARKER(0x80015B64, "SLUS_006.10");
    session_flow.phase = (3);
    race_run_two_player();
    input_set_motors(0, 0, 0);
    return (sint32)input_set_motors(1, 0, 0);
}

void session_reset_progression(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x800ADB84, "SLUS_006.10");
    memset(&progression_profile, 0, sizeof(progression_profile));
    for (idx = 0; idx < 6; ++idx)
        progression_profile.model_availability[17 + idx] = (uint8)(1);
    progression_profile.model_availability[32] = (uint8)(1);
    progression_profile.track_availability[23] = (uint8)(2);
    progression_profile.track_availability[16] = (uint8)(2);
    progression_profile.track_availability[14] = (uint8)(2);
    progression_profile.track_availability[17] = (uint8)(2);
    progression_profile.track_availability[26] = (uint8)(2);
    progression_profile.track_availability[29] = (uint8)(2);
    progression_profile.track_availability[28] = (uint8)(2);
    progression_profile.track_availability[27] = (uint8)(2);
    progression_profile.track_availability[33] = (uint8)(2);
    progression_profile.track_availability[36] = (uint8)(2);
    progression_profile.track_availability[35] = (uint8)(2);
    progression_profile.track_availability[34] = (uint8)(2);
    for (idx = 0; idx < 6; ++idx)
        progression_profile.track_availability[1 + idx] = (uint8)(1);
    progression_profile.track_availability[13] = (uint8)(1);
    progression_profile.track_availability[15] = (uint8)(1);
}

uint32 session_track_availability(uint32 track)
{
    XASSERT(track < sizeof(progression_profile.track_availability));
#if defined(_DEBUG) && !defined(LOCKSTEP_DEBUG)
    switch (track)
    {
        case 1:
        case 2:
        case 6:
        case 22:
        case 24:
        case 37:
            return 2;
    }
#endif
    return progression_profile.track_availability[track];
}

void session_unlock_bonus_models(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x800AD578, "SLUS_006.10");
    for (idx = 17; idx < 37; ++idx)
        progression_profile.model_availability[idx] = (uint8)(2);
    progression_profile.model_availability[8] = (uint8)(2);
}

void session_unlock_bonus_tracks(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x800AD54C, "SLUS_006.10");
    for (idx = 1; idx < 41; ++idx)
        progression_profile.track_availability[idx] = (uint8)(2);
}

void session_unlock_features(void)
{
    FUNCTION_MARKER(0x800AD5AC, "SLUS_006.10");
    progression_profile.features[0] = (uint8)(1);
    progression_profile.features[1] = (uint8)(1);
    progression_profile.features[2] = (uint8)(1);
    progression_profile.features[3] = (uint8)(1);
    progression_profile.features[4] = (uint8)(1);
}

void session_unlock_special_models(void)
{
    FUNCTION_MARKER(0x800ADE44, "SLUS_006.10");
    progression_profile.model_availability[36] = (uint8)(2);
    progression_profile.model_availability[29] = (uint8)(2);
    progression_profile.model_availability[23] = (uint8)(2);
}

const sint16 session_circuit_tracks[24] = {23, 16, 14, 17, 13, 15, 1, 2, 3, 4, 5, 6, 24, 21, 19, 22, 18, 20, 7, 8, 9, 10, 11, 12};

const sint16 session_linear_tracks[7] = {26, 29, 28, 27, 25, 39, 37};

const sint16 session_multiplayer_tracks[7] = {33, 36, 35, 34, 32, 40, 38};

/* Keep the terminal route read before the event-end check */
uint16 session_multiplayer_routes[5] = {32, 33, 34, 35, 65535};

void session_award_cup_rewards(void)
{
    uint32 track_rewards[5];
    uint32 model_rewards[5];
    uint32 route_rewards[2];
    uint32 special_model_rewards[6];
    uint32 val0, val1, val2, mode, count, slot, val, other;
    char *reward_text, *label0, *label1;
    char *const *name_base;
    uint32 idx, items, name_count, offset;
    const uint32 *local;
    uint8 *flag_base, *flag;

    FUNCTION_MARKER(0x800AD5E0, "SLUS_006.10");
    val0 = cup_track_rewards[0];
    val1 = cup_track_rewards[1];
    val2 = cup_track_rewards[2];
    track_rewards[0] = val0;
    track_rewards[1] = val1;
    track_rewards[2] = val2;
    val0 = cup_track_rewards[3];
    val1 = cup_track_rewards[4];
    track_rewards[3] = val0;
    track_rewards[4] = val1;
    mode = race_session.cup;
    val0 = cup_model_rewards[0];
    val1 = cup_model_rewards[1];
    val2 = cup_model_rewards[2];
    model_rewards[0] = val0;
    model_rewards[1] = val1;
    model_rewards[2] = val2;
    val0 = cup_model_rewards[3];
    val1 = cup_model_rewards[4];
    model_rewards[3] = val0;
    model_rewards[4] = val1;
    route_rewards[0] = 15;
    route_rewards[1] = 13;
    val0 = cup_special_model_rewards[0];
    val1 = cup_special_model_rewards[1];
    val2 = cup_special_model_rewards[2];
    special_model_rewards[0] = val0;
    special_model_rewards[1] = val1;
    special_model_rewards[2] = val2;
    val0 = cup_special_model_rewards[3];
    val1 = cup_special_model_rewards[4];
    val2 = cup_special_model_rewards[5];
    special_model_rewards[3] = val0;
    special_model_rewards[4] = val1;
    special_model_rewards[5] = val2;
    progression_profile.cup_completion[mode] = (uint8)(1);
    mode = race_session.cup;
    count = 0;
    slot = 5;
    if ((mode == 0 || mode == 5) && progression_profile.cup_completion[0] != 0 && progression_profile.cup_completion[5] != 0)
    {
        label0 = session_event_names[42];
        label1 = session_event_names[43];
        count = 2;
        slot = 7;
        progression_profile.features[0] = (uint8)(1);
        progression_profile.features[1] = (uint8)(1);
        presentation_items[5].content.text = label0;
        presentation_items[6].content.text = label1;
    }
    if (race_session.cup - 1 < 2 && progression_profile.cup_completion[1] != 0 && progression_profile.cup_completion[2] != 0)
    {
        offset = slot;
        reward_text = session_event_names[44];
        ++slot;
        count += 2;
        progression_profile.features[2] = (uint8)(1);
        presentation_items[offset].content.text = reward_text;
        reward_text = session_event_names[45];
        offset = slot;
        ++slot;
        progression_profile.features[3] = (uint8)(1);
        presentation_items[offset].content.text = reward_text;
    }
    mode = race_session.cup;
    items = 0;
    local = 0;
    flag_base = 0;
    name_base = NULL;
    name_count = 0;
    switch (mode)
    {
        case 0:
            if (progression_profile.features[4] == 0)
            {
                progression_profile.features[4] = (uint8)(1);
                reward_text = session_event_names[47];
                ++count;
                presentation_items[slot].content.text = reward_text;
            }
            break;
        case 1:
            items = 5;
            local = track_rewards;
            flag_base = progression_profile.track_availability;
            name_base = session_event_names;
            name_count = 48;
            break;
        case 2:
            items = 5;
            local = model_rewards;
            flag_base = progression_profile.model_availability;
            name_base = session_model_names;
            name_count = 44;
            break;
        case 3:
            items = 2;
            local = route_rewards;
            flag_base = progression_profile.track_availability;
            name_base = session_event_names;
            name_count = 48;
            break;
        case 4:
            if (progression_profile.track_availability[6] != 2)
            {
                ++count;
                reward_text = session_event_names[0];
                offset = slot;
                ++slot;
                progression_profile.track_availability[6] = (uint8)(2);
                presentation_items[offset].content.text = reward_text;
            }
            if (progression_profile.track_availability[37] != 2)
            {
                ++count;
                reward_text = session_event_names[37];
                offset = slot;
                ++slot;
                progression_profile.track_availability[37] = (uint8)(2);
                progression_profile.track_availability[38] = (uint8)(2);
                presentation_items[offset].content.text = reward_text;
            }
            items = 6;
            local = special_model_rewards;
            flag_base = progression_profile.model_availability;
            name_base = session_model_names;
            name_count = 44;
            break;
        case 5:
            if (progression_profile.track_availability[25] != 2)
            {
                ++count;
                reward_text = session_event_names[25];
                offset = slot;
                ++slot;
                progression_profile.track_availability[25] = (uint8)(2);
                progression_profile.track_availability[32] = (uint8)(2);
                presentation_items[offset].content.text = reward_text;
            }
            if (progression_profile.track_availability[39] != 2)
            {
                reward_text = session_event_names[39];
                ++count;
                progression_profile.track_availability[39] = (uint8)(2);
                progression_profile.track_availability[40] = (uint8)(2);
                presentation_items[slot].content.text = reward_text;
            }
            break;
        default:
            break;
    }
    offset = slot;
    for (idx = 0; idx < items; ++idx)
    {
        val = local[idx];
        flag = flag_base + val;
        if (*flag != 2)
        {
            *flag = (2);
            other = local[idx];
            reward_text = other < name_count ? name_base[other] : "";
            ++count;
            presentation_items[offset].content.text = reward_text;
            ++offset;
        }
    }
    if (count == 0)
        presentation_items[4].active = (0);
}

void session_reset_menu_history(uint32 idx)
{
    FUNCTION_MARKER(0x800A98B4, "SLUS_006.10");
    XASSERT(!(idx > MENU_HISTORY_CAPACITY));
    menu_history.depth = 0;
    while ((sint32)idx < MENU_HISTORY_CAPACITY)
        menu_history.states[idx++] = 255;
}

uint32 session_push_menu(uint32 argument)
{
    uint32 prev, idx;

    FUNCTION_MARKER(0x800A98F0, "SLUS_006.10");
    // TODO: Restore this entry after the xport_save/load UI migration
    if (!memory_card_enabled && argument == 130)
        return 0;
    prev = session_flow.menu_state;
    idx = menu_history.depth;
    XASSERT(!(idx >= MENU_HISTORY_CAPACITY));
    menu_history.states[idx] = (uint8)(prev - 1);
    idx = menu_history.depth;
    session_flow.menu_state = (argument);
    menu_history.depth = idx + 1;
    return (uint32)session_apply_menu_mode();
}

void session_pop_menu(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x800A99F0, "SLUS_006.10");
    if (menu_history.depth == 0)
        return;
    XASSERT(!(menu_history.depth > MENU_HISTORY_CAPACITY));
    idx = menu_history.depth - 1;
    menu_history.depth = idx;
    session_flow.menu_state = (menu_history.states[idx]);
    session_apply_menu_mode();
}
