#include <stdio.h>
#include <stdlib.h>

#include "results_menu.h"
#include "game.h"

RESULTS_MENU results_menu;
const RESULTS_MENU_TEXT results_menu_text_defaults = {
    .model = "ZZ",
    .driver = "XX",
    .cup_pos = "CUP POSITION \tXXST",
    .cup_points = "CUP POINTS \tXXXXX",
    .cup_time = "CUP TIME \tXX:XX:XX",
    .avg_speed = "AVERAGE SPEED \tXXXMPH",
    .top_speed = "TOP SPEED \tXXXMPH",
    .race_pos = "RACE POSITION \tXXST",
    .highest_pos = "HIGHEST POSITION \tXXST",
    .starting_pos = "STARTING POSITION \tXXST",
    .police_stops = "POLICE STOPS \tXXX",
    .race_time = "RACE TIME \tXX:XX:XX",
    .checkpoint_time = "CHECKPOINT TIMES \tXX:XX:XX",
    .checkpoint_time_2 = " \tYY:XX:XX",
    .checkpoint_time_3 = " \tZZ:XX:XX",
    .checkpoint_time_4 = " \tWW:XX:XX",
    .checkpoint_time_5 = " \tVV:XX:XX",
    .race_points = "RACE POINTS \tXXXXX",
    .lap_time = "LAP TIMES \tXX:XX:XX",
    .drag_time = "DRAG TIME \tXX:XX:XXX",
    .arrests = "ARRESTS \tXXXXX",
    .points = "POINTS \tXXXXX",
    .pos = "POSITION \tXXXXX",
};
char *const results_menu_default_slots[23] = {
    results_menu.text.model, results_menu.text.driver, results_menu.text.cup_pos, results_menu.text.cup_points, results_menu.text.cup_time, results_menu.text.avg_speed, results_menu.text.top_speed, results_menu.text.race_pos, results_menu.text.highest_pos, results_menu.text.starting_pos, results_menu.text.police_stops, results_menu.text.race_time, results_menu.text.checkpoint_time, results_menu.text.checkpoint_time_2, results_menu.text.checkpoint_time_3, results_menu.text.checkpoint_time_4, results_menu.text.checkpoint_time_5, results_menu.text.race_points, results_menu.text.lap_time, results_menu.text.drag_time, results_menu.text.arrests, results_menu.text.points, results_menu.text.pos,
};

static const char prefix[] = " \t";
static const uint8 rows_race_points[] = {9, 2, 17, 3, 5, 6, 7, 8, 9, 11};
static const uint8 rows_cup_points[] = {13, 2, 3, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16};
static const uint8 rows_cup_time[] = {13, 2, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16};
static const uint8 rows_race[] = {11, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16};
static const uint8 rows_cup_laps[] = {12, 2, 4, 5, 6, 7, 8, 9, 11, 18, 13, 14, 15};
static const uint8 rows_two_player[] = {8, 2, 4, 5, 6, 7, 8, 9, 11};
static const uint8 rows_drag[] = {4, 19, 5, 6, 7};
static const uint8 rows_arrests[] = {4, 20, 21, 5, 6};
static const uint8 rows_police[] = {3, 20, 5, 6};

static char *find_result_value(char *const *pointer_slot)
{
    char *dst = presentation_find_text(*pointer_slot, prefix);

    XASSERT(!(dst == NULL));
    return dst + 2;
}

static uint32 select_result_car(uint32 selection)
{
    char *dst;
    uint32 car, rec, val, sel, divisor, time, idx, rate;

    FUNCTION_MARKER(0x800B187C, "SLUS_006.10");
    val = session_options.model_variant;
    results_menu.displayed_car = (uint32)(selection);
    sel = (sint8)session_car_choices.cars[selection].model[val];
    results_menu.text_slots[0] = ((uint32)(sel) < 44 ? session_model_names[sel] : "");
    results_menu.text_slots[1] = selection == 0 ? presentation_player_name : "";
    val = race_session.multiplayer_mode;
    if (val - 2 < 2 && (sint32)selection < 2)
        results_menu.text_slots[1] = (selection == 0 ? "PLAYER 1" : "PLAYER 2");
    dst = find_result_value(&results_menu.text_slots[2]);
    rec = selection * 20;
    val = (sint16)race_results.entries[selection].place;
    val = (uint32)sprintf(dst, "%s", ((uint32)(val) < 6 ? session_place_names[val] : ""));
    dst = find_result_value(&results_menu.text_slots[22]);
    val = (sint16)race_results.entries[selection].place;
    val = (uint32)sprintf(dst, "%s", ((uint32)(val) < 6 ? session_place_names[val] : ""));
    dst = find_result_value(&results_menu.text_slots[3]);
    presentation_format_number(dst, "%d", race_results.entries[selection].points.val);
    val = race_results.entries[selection].total_time * 100;
    divisor = ((sint32)disp_state.field_rate / 2);
    time = presentation_time_from_ticks(val, divisor);
    dst = find_result_value(&results_menu.text_slots[4]);
    presentation_format_time(dst, "%02d:%02d:%02d", time, 100);
    dst = find_result_value(&results_menu.text_slots[5]);
    car = selection * 864;
    val = game_options.speed_kph;
    sel = (uint32)(sint32)(sint16)car_states[car / 864u].race_stats.avg_speed << 8;
    if (val == 0)
        presentation_format_number(dst, "%dMPH", presentation_result_speed(sel, 0xD161543F, 10));
    else
        presentation_format_number(dst, "%dKPH", presentation_result_speed(sel, 0xA8791709, 9));
    dst = find_result_value(&results_menu.text_slots[6]);
    val = game_options.speed_kph;
    sel = (uint32)(sint32)(sint16)car_states[car / 864u].race_stats.top_speed << 8;
    if (val == 0)
        presentation_format_number(dst, "%dMPH", presentation_result_speed(sel, 0xD161543F, 10));
    else
        presentation_format_number(dst, "%dKPH", presentation_result_speed(sel, 0xA8791709, 9));
    dst = find_result_value(&results_menu.text_slots[7]);
    val = car_states[car / 864u].place;
    sprintf(dst, "%s", ((uint32)(val) < 6 ? session_place_names[val] : ""));
    dst = find_result_value(&results_menu.text_slots[8]);
    val = car_states[car / 864u].best_place;
    sprintf(dst, "%s", ((uint32)(val) < 6 ? session_place_names[val] : ""));
    dst = find_result_value(&results_menu.text_slots[9]);
    val = car_states[car / 864u].start_place;
    sprintf(dst, "%s", ((uint32)(val) < 6 ? session_place_names[val] : ""));
    dst = find_result_value(&results_menu.text_slots[10]);
    presentation_format_number(dst, "%d", car_states[car / 864u].result_detail);
    val = car_states[car / 864u].finish_time * 100;
    divisor = ((sint32)disp_state.field_rate / 2);
    time = presentation_time_from_ticks(val, divisor);
    dst = find_result_value(&results_menu.text_slots[11]);
    presentation_format_time(dst, "%02d:%02d:%02d", time, 100);
    for (idx = 0; idx < 5; ++idx)
    {
        val = car_states[car / 864u].split_ticks[idx] * 100;
        divisor = ((sint32)disp_state.field_rate / 2);
        time = (uint32)fixed_divide_checked((sint32)val, (sint32)divisor);
        dst = find_result_value(&results_menu.text_slots[12 + idx]);
        presentation_format_time(dst, "%02d:%02d:%02d", time, 100);
        if (idx == 0)
        {
            dst = find_result_value(&results_menu.text_slots[18]);
            presentation_format_time(dst, "%02d:%02d:%02d", time, 100);
        }
    }
    dst = find_result_value(&results_menu.text_slots[17]);
    presentation_format_number(dst, "%d", car_states[car / 864u].points);
    rate = disp_state.field_rate;
    val = car_states[car / 864u].finish_time;
    divisor = ((sint32)rate / 2);
    time = presentation_time_from_ticks(val, divisor);
    dst = find_result_value(&results_menu.text_slots[19]);
    presentation_format_time(dst, "%02d:%02d:%03d", time, 1000);
    dst = find_result_value(&results_menu.text_slots[20]);
    presentation_format_number(dst, "%d", race_results.entries[selection].finish_data.bytes[0]);
    dst = find_result_value(&results_menu.text_slots[21]);
    return presentation_format_number(dst, "%d", race_results.entries[selection].points.val);
}

uint32 results_menu_begin(void)
{
    uint32 mode;

    FUNCTION_MARKER(0x800B1358, "SLUS_006.10");
    results_menu.offset = (uint32)(0);
    results_menu.cur_car = (uint32)(0);
    results_menu.target_offset = (uint32)(0);
    select_result_car(0);
    mode = race_session.event_mode;
    if (mode == 1)
        race_control.car_count = (uint32)(mode);
    return 1;
}

uint32 results_menu_update(void)
{
    uint32 maximum, cur, rotation, target, sel, x, mode, count;
    const uint8 *list, *pointer, *end;
    const char *text;
    uint32 buttons, choice, row = 82;

    FUNCTION_MARKER(0x800B13AC, "SLUS_006.10");
    maximum = race_control.car_count;
    if ((sint32)maximum >= 7)
        maximum = 6;
    maximum -= 1;
    buttons = input_ports[0].pressed;
    if (buttons & 0x4008)
    {
        race_menu_select_default();
        audio_play_menu_accept();
    }
    if (input_ports[0].pressed & 0x80)
    {
        cur = results_menu.cur_car;
        if ((sint32)cur > 0 && results_menu.offset == 0)
        {
            results_menu.target_offset = (uint32)(416);
            results_menu.cur_car = (uint32)(cur - 1);
            audio_play_menu_move();
        }
    }
    if (input_ports[0].pressed & 0x20)
    {
        cur = results_menu.cur_car;
        if ((sint32)cur < (sint32)maximum && results_menu.offset == 0)
        {
            results_menu.target_offset = (uint32)(0 - 416);
            results_menu.cur_car = (uint32)(cur + 1);
            audio_play_menu_move();
        }
    }
    target = results_menu.target_offset;
    rotation = results_menu.offset;
    if (target == rotation)
    {
        sel = results_menu.displayed_car;
        cur = results_menu.cur_car;
        if (sel != cur)
        {
            select_result_car(cur);
            rotation = results_menu.offset;
            results_menu.target_offset = (uint32)(0);
            results_menu.offset = (uint32)(0 - rotation);
        }
    }
    rotation = results_menu.offset;
    target = results_menu.target_offset;
    if ((sint32)target < (sint32)rotation)
    {
        results_menu.offset = (uint32)(rotation - 32);
        rotation = results_menu.offset;
    }
    if ((sint32)rotation < (sint32)target)
        results_menu.offset = (uint32)(rotation + 32);
    presentation_select_pal(0);
    presentation_draw_image(128, 104, 206, "cross");
    presentation_select_pal(2);
    presentation_draw_text(0, 117, 198, "ADVANCE");
    if (results_menu.offset == 0)
    {
        cur = results_menu.cur_car;
        if (cur != 0)
        {
            presentation_draw_image(128, 64, 128, "ArroLeft");
            cur = results_menu.cur_car;
        }
        if (cur != maximum)
            presentation_draw_image(128, 448, 128, "ArroRght");
    }
    presentation_select_pal(2);
    presentation_draw_text(131, 256, 22, "RACE RESULTS");
    rotation = results_menu.offset;
    x = rotation + 128;
    presentation_render.tab_stop = rotation + 308;
    presentation_select_pal(3);
    text = results_menu.text_slots[0];
    presentation_draw_text(1, x, 48, text);
    if (race_session.multiplayer_mode == 2 && (race_control.frame_counter & 8) == 0)
    {
        text = results_menu.text_slots[1];
        presentation_draw_text(1, x, 57, text);
    }
    presentation_select_pal(0);
    mode = race_session.event_mode;
    list = rows_race;
    if (mode == 2)
    {
        choice = race_session.cup;
        if (choice < 6)
        {
            switch (choice)
            {
                case 0:
                    list = rows_cup_points;
                    break;
                case 1:
                case 2:
                case 3:
                    list = rows_cup_time;
                    break;
                case 4:
                    list = rows_race_points;
                    break;
                case 5:
                    list = rows_cup_laps;
                    break;
            }
        }
    }
    mode = race_session.multiplayer_mode;
    if (mode == 2)
        list = rows_two_player;
    if (race_session.cop_chase != 0)
        list = rows_police;
    if (mode == 3)
        list = rows_arrests;
    if (race_session.drag_race != 0)
        list = rows_drag;
    count = *list + 1;
    if (race_session.cup == 5 || race_session.linear_course != 0 || race_session.two_players != 0)
    {
        if (race_session.multiplayer_mode - 3 >= 2)
            count = count - 5 + race_session.laps;
    }
    pointer = list + 1;
    end = list + count;
    if ((sint32)count > 1)
    {
        do
        {
            choice = *pointer;
            text = results_menu.text_slots[choice];
            ++pointer;
            presentation_draw_text(1, x, (sint32)row, text);
            row += 9;
        } while (pointer < end);
    }
    return 0;
}
