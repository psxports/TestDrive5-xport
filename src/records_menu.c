#include "records_menu.h"
#include "game.h"

static const sint16 column_x[6] = {30, 90, 185, 300, 415, 470};

static char *const heading_labels[3][6] = {{"POS", "NAME", "TIME", "CAR", "AVG SPD", "TOP SPD"}, {"POS", "NAME", "PTS", "CAR", "AVG SPD", "TOP SPD"}, {"POS", "NAME", "BEST LAP", "CAR", "AVG SPD", "TOP SPD"}};

static char *const model_labels[38] = {
    "VIPER", "98' CORVETTE", "VANTAGE", "CERBERA", "XKR", "98' MUSTANG", "98' CAMARO", "SKYLINE", "HOT DOG", "COBRA", "GTO", "69' MUSTANG", "69' CAMARO", "69' CORVETTE", "70' CHEVELLE", "71' CUDA", "CHARGER", "PROJECT VANTAGE", "SERIES 1", "SPEED 12", "GTS-R", "XJ220", "SUPER 7", "CHRIS'S BEAST", "CATERHAM 21", "CERBERA POLICE", "DAYTONA", "CAMARO HOTROD", "MUSTANG HOTROD", "MAUL", "FEAR FACTORY", "MUSTANG GT", "390 GT", "MUSTANG POLICE", "CHARGER POLICE", "CAMARO POLICE", "PITBULL SPECIAL", "SP9",
};

RECORDS_MENU_STATE recs_menu;

static void collect_rec_candidates(void)
{
    uint32 idx, offset, val, prev, skip, count;

    FUNCTION_MARKER(0x800A6318, "SLUS_006.10");
    recs_menu.count = (uint8)(0);
    for (idx = 0, offset = 0; idx < 47; ++idx, offset += 120)
    {
        if (idx == 40)
        {
            val = rec_table.rows[40][5].data.detail;
            prev = rec_table.rows[40][4].data.detail;
            skip = val < prev;
        }
        else if (idx == 44)
        {
            val = rec_table.rows[44][5].data.val;
            prev = rec_table.rows[44][4].data.val;
            skip = val < prev;
        }
        else
        {
            val = rec_table.rows[idx][5].data.val;
            prev = rec_table.rows[idx][4].data.val;
            skip = prev < val;
        }
        if (skip == 0 && val != 0)
        {
            count = recs_menu.count;
            recs_menu.groups[count] = (uint8)(idx);
            count = recs_menu.count;
            recs_menu.count = (uint8)(count + 1);
        }
    }
}

void recs_menu_begin(void)
{
    uint32 idx, count, item;
    sint32 entry;

    FUNCTION_MARKER(0x800A63E4, "SLUS_006.10");
    recs_menu.count = (uint8)(0);
    count = 0;
    for (idx = 0; idx < 24; ++idx)
    {
        entry = (sint16)session_circuit_tracks[idx];
        if (session_track_availability((uint32)entry) == 2)
        {
            recs_menu.groups[count] = (uint8)((uint32)entry - 1);
            item = recs_menu.count;
            ++count;
            recs_menu.count = (uint8)(item + 1);
        }
    }
    for (idx = 0; idx < 7; ++idx)
    {
        entry = (sint16)session_linear_tracks[idx];
        if (session_track_availability((uint32)entry) == 2)
        {
            recs_menu.groups[count] = (uint8)((uint32)entry - 1);
            item = recs_menu.count;
            ++count;
            recs_menu.count = (uint8)(item + 1);
        }
    }
    recs_menu.groups[count] = (uint8)(29);
    item = recs_menu.count;
    ++count;
    recs_menu.count = (uint8)(item + 1);
    for (idx = 40; idx < 46; ++idx, ++count)
    {
        recs_menu.groups[count] = (uint8)(idx);
        item = recs_menu.count;
        recs_menu.count = (uint8)(item + 1);
    }
    if (session_flow.event_started != 0)
    {
        recs_clear_name_states();
        collect_rec_candidates();
        recs_set_candidate_names();
        recs_sort();
        save_autosave();
        presentation_items[2].active = (0);
        presentation_items[3].active = (0);
    }
    else
    {
        recs_clear_name_states();
        presentation_items[2].active = (1);
        presentation_items[3].active = (1);
    }
    recs_menu.sel = (uint32)(0);
}

void recs_menu_update(void)
{
    char text[64];
    char *const *heading_table;
    const char *pointer;
    uint32 count, sel, item, special, idx, row;
    const char *format;
    uint32 rec, val, numerator, rate, time, multiplier, shift;
    uint32 remainder, fraction, fixed_speed;
    sint32 x, y, seconds_unit, minutes, seconds;

    FUNCTION_MARKER(0x800A6594, "SLUS_006.10");
    count = recs_menu.count;
    presentation_items[0].active = (1);
    presentation_items[1].active = (1);
    if (count == 0)
    {
        presentation_items[0].active = (0);
        presentation_items[1].active = (0);
    }
    sel = recs_menu.sel;
    if (sel == 0)
        presentation_items[0].active = (0);
    if (sel == count - 1)
        presentation_items[1].active = (0);
    if ((input_ports[0].pressed & 0x80) != 0 && (sint32)sel > 0)
    {
        audio_play_menu_high();
        recs_menu.sel = (uint32)(recs_menu.sel - 1);
    }
    if ((input_ports[0].pressed & 0x20) != 0)
    {
        count = recs_menu.count;
        sel = recs_menu.sel;
        if ((sint32)sel < (sint32)(count - 1))
        {
            audio_play_menu_high();
            recs_menu.sel = (uint32)(recs_menu.sel + 1);
        }
    }
    sel = recs_menu.sel;
    item = recs_menu.groups[sel];
    special = item == 44 || item == 40;
    presentation_select_pal(0);
    pointer = ((uint32)(item + 1) < 48 ? session_event_names[item + 1] : "");
    presentation_draw_text(129, 256, 64, pointer);
    presentation_select_pal(3);
    if (special != 0)
        heading_table = heading_labels[1];
    else if (item - 24 < 5 || item == 36 || item == 38)
        heading_table = heading_labels[2];
    else
        heading_table = heading_labels[0];
    for (idx = 0; idx < 6; ++idx)
    {
        x = column_x[idx];
        pointer = heading_table[idx];
        presentation_draw_text(129, x, 96, pointer);
        if (idx >= 4)
        {
            pointer = game_options.speed_kph == 0 ? "(MPH)" : "(KPH)";
            presentation_draw_text(129, x, 104, pointer);
        }
    }
    y = 112;
    for (row = 0; row < 5; ++row, y += 20)
    {
        rec = item * 120 + row * 20;
        presentation_select_pal(row == 0 ? 4 : 6);
        if (special == 0 && rec_table.rows[item][row].data.name_state != 0)
            presentation_select_pal(1);
        x = column_x[0];
        presentation_format_number(text, "%d", row + 1);
        presentation_draw_text(128, x, y, text);
        x = column_x[1];
        presentation_draw_text(128, x, y, (const char *)rec_table.rows[item][row].data.name);
        if (special != 0)
        {
            x = column_x[2];
            if (item == 44)
                val = rec_table.rows[44][row].data.val;
            else
                val = rec_table.rows[item][row].data.detail;
            presentation_format_number(text, "%d", val);
        }
        else
        {
            if (item == 29)
            {
                rate = disp_state.field_rate;
                numerator = rec_table.rows[29][row].data.val;
                rate = ((sint32)rate / 2);
                time = presentation_time_from_ticks(numerator, rate);
                seconds_unit = 1000;
                format = "%02d:%02d.%03d";
            }
            else
            {
                numerator = rec_table.rows[item][row].data.val;
                rate = disp_state.field_rate;
                numerator *= 100;
                rate = ((sint32)rate / 2);
                time = presentation_time_from_ticks(numerator, rate);
                seconds_unit = 100;
                format = "%01d:%02d.%02d";
            }
            minutes = (sint32)time / (seconds_unit * 60);
            remainder = time - (uint32)minutes * (uint32)(seconds_unit * 60);
            seconds = (sint32)remainder / seconds_unit;
            x = column_x[2];
            fraction = remainder - (uint32)seconds * (uint32)seconds_unit;
            presentation_format_time(text, format, time, seconds_unit);
        }
        presentation_draw_text(128, x, y, text);
        val = rec_table.rows[item][row].data.model;
        x = column_x[3];
        pointer = ((uint32)(val) < 38 ? model_labels[val] : "");
        presentation_draw_text(128, x, y, pointer);
        val = game_options.speed_kph;
        x = column_x[4];
        fixed_speed = (uint32)rec_table.rows[item][row].data.speed << 8;
        multiplier = val == 0 ? 0xD161543F : 0xA8791709;
        shift = val == 0 ? 10 : 9;
        val = presentation_result_speed(fixed_speed, multiplier, shift);
        presentation_format_number(text, "%d", val);
        presentation_draw_text(128, x, y, text);
        val = game_options.speed_kph;
        x = column_x[5];
        fixed_speed = (uint32)rec_table.rows[item][row].data.peak_speed << 8;
        multiplier = val == 0 ? 0xD161543F : 0xA8791709;
        shift = val == 0 ? 10 : 9;
        val = presentation_result_speed(fixed_speed, multiplier, shift);
        presentation_format_number(text, "%d", val);
        presentation_draw_text(128, x, y, text);
    }
}
