#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "race_menu.h"
#include "game.h"

static char *const outcome_labels[2] = {
    "SORRY YOU FAILED TO WIN THE RACE",
    "PLEASE RACE THIS TRACK AGAIN",
};

RACE_MENU_STATE race_menu;

static const PSX_RECT menu_pal_rect = {512, 0, 512, 256};
static const VEC3_16 menu_light_dir = {.v = {-2508, 2048, -2508}};

static const char *const command_labels[10] = {
    "RACE AGAIN", "VIEW REPLAY", "NEXT CUP RACE", "QUIT CUP", "QUIT", "RACE SPLIT-SCREEN AGAIN", "SELECT NEW CAR", "SAVE RACE STATUS", "VIEW RACE DATA", "RACE SWITCHED COP",
};

static const sint16 unlock_targets[40] = {7, 8, 9, 10, 11, 12, 7, 8, 9, 10, 11, 12, 18, 19, 20, 21, 22, 18, 19, 20, 21, 22, 24, 24, 24, 22, 28, 26, 18, 0, 0, 0, 0, 0, 0, 0, 30, 0, 27, 0};
static const char unlock_heading[] = "YOU HAVE UNLOCKED";
static const char unlock_bonus_label[] = "FEAR FACTORY VIDEO";

static uint32 collect_unlocks(void)
{
    uint32 idx, val, mission, entry, count;
    const char *pointer;

    FUNCTION_MARKER(0x800B7A8C, "SLUS_006.10");
    for (idx = 0; idx < 5; ++idx)
        race_menu.unlock_labels[4 - idx] = NULL;
    val = race_session.event_mode;
    race_menu.unlock_count = (uint32)(0);
    if (val == 0)
    {
        val = race_session.linear_course;
        if (val == 0)
        {
            val = race_rules.checkpoint_timers;
            if (val == 0)
                return val;
            val = race_rules.traffic;
            if (val == 0)
                return val;
            val = race_rules.cops;
            if (val == 0)
                return val;
        }
    }
    val = race_session.two_players;
    if (val != 0)
        return val;
    val = race_session.event_mode;
    if (val != 0)
        return val;
    val = race_control.exit_reason;
    if (val != 0)
        return val;
    if (car_states[0].place != 0)
        return 36;
    mission = race_session.track;
    if (mission == 36)
    {
        if ((sint8)session_car_choices.cars[0].model[0] == 30 && progression_profile.special_model_30 == 0)
        {
            count = race_menu.unlock_count;
            progression_profile.special_model_30 = (uint8)(1);
            race_menu.unlock_labels[count] = unlock_bonus_label;
            race_menu.unlock_count = (uint32)(count + 1);
        }
        mission = race_session.track;
    }
    entry = (uint32)unlock_targets[mission];
    if (mission < 24)
    {
        if (progression_profile.track_availability[entry] == 2)
            return 2;
        progression_profile.track_availability[entry] = (uint8)(2);
        count = race_menu.unlock_count;
        pointer = ((uint32)(entry) < 48 ? session_event_names[entry] : "");
    }
    else
    {
        if (entry == 0)
            return 2;
        if (progression_profile.model_availability[entry] == 2)
            return 2;
        progression_profile.model_availability[entry] = (uint8)(2);
        count = race_menu.unlock_count;
        pointer = ((uint32)(entry) < 44 ? session_model_names[entry] : "");
    }
    val = count * 4;
    race_menu.unlock_labels[count] = pointer;
    race_menu.unlock_count = (uint32)(count + 1);
    return val;
}

static uint32 draw_unlocks(void)
{
    uint32 count, idx, y;
    const char *text;

    FUNCTION_MARKER(0x800B7CB8, "SLUS_006.10");
    count = race_menu.unlock_count;
    if (count == 0)
        return count;
    presentation_select_pal(3);
    text = unlock_heading;
    presentation_draw_text(129, 256, 170, text);
    presentation_select_pal(0);
    count = race_menu.unlock_count;
    if ((sint32)count <= 0)
        return count;
    idx = 0;
    y = 180;
    do
    {
        text = race_menu.unlock_labels[idx];
        presentation_draw_text(129, 256, (sint32)y, text);
        count = race_menu.unlock_count;
        ++idx;
        y += 10;
    } while ((sint32)idx < (sint32)count);
    return 0;
}

uint32 race_menu_select_default(void)
{
    uint32 val, mode, stage;

    FUNCTION_MARKER(0x800B217C, "SLUS_006.10");
    if (race_session.event_mode == 2 || race_session.multiplayer_mode == 2)
        val = race_session.two_players != 0 ? 80 : 40;
    else
        val = race_session.two_players != 0 ? 70 : 0;
    race_menu.state = (uint32)(val);
    if (race_session.cup == 2 && car_states[0].place != 0)
        race_menu.state = (uint32)(60);
    if (race_session.demo != 0 && race_session.cup == 2)
    {
        val = race_menu.return_state;
        race_menu.state = (uint32)(val);
    }
    mode = race_session.multiplayer_mode;
    if (mode != 3)
        return 3;
    stage = session_flow.event_round;
    if (stage == 0)
        race_menu.state = (uint32)(90);
    if (stage == 1)
        race_menu.state = (uint32)(100);
    return 100;
}

sint32 race_menu_prepare(void)
{
    uint8 *model_file;
    uint8 *cursor;
    PSX_RECT rect;
    char filename[sizeof(presentation_background_format)];
    uint32 flags, mode, val, other;
    uint32 bytes, count, name_len;
    uint8 *vram;
    uint32 vram_size;
    uint8 *tex;
    sint16 width, height;

    FUNCTION_MARKER(0x800575A4, "SLUS_006.10");
    flags = race_control.exit_reason;
    race_menu.sel = (uint16)(0);
    race_menu.replay_disabled = (uint8)(0);
    race_menu.results_disabled = (uint8)(0);
    if (flags != 0 || race_session.demo != 0 || session_flow.load_requested != 0)
    {
        race_menu_select_default();
        goto setup;
    }
    mode = race_session.cup;
    if (mode != 2 || car_states[0].place == 0)
    {
        race_accumulate_results();
        mode = race_session.cup;
    }
    if (mode == 2 && car_states[0].place != 0)
    {
        val = replay_mode;
        race_menu.state = (uint32)(val != 0 ? 60 : 50);
        goto check_time;
    }
    val = replay_mode;
    race_menu.state = (uint32)(30);
    if (val != 0)
        goto check_time;
    if (race_session.event_mode != 2 && race_session.multiplayer_mode != 2)
        goto check_time;
    recs_update_candidate((sint32)race_session.track);
    mode = race_session.cup;
    switch (mode)
    {
        case 0:
        case 4:
            race_rank_scores();
            break;
        case 1:
        case 2:
        case 3:
        case 5:
            race_rank_times();
            break;
    }
    mode = race_session.multiplayer_mode;
    if (mode == 2)
    {
        race_rank_times();
        mode = race_session.multiplayer_mode;
    }
    if (mode != 3)
        goto setup;
    race_rank_scores();
check_time:
    if (race_session.multiplayer_mode == 3 && session_flow.event_round == 0)
        race_menu.state = (uint32)(90);
setup:
    if (session_flow.load_requested != 0)
    {
        race_control.exit_reason = (uint32)(0);
        race_session.demo = (0);
        race_menu.replay_disabled = (uint8)(1);
        race_menu.results_disabled = (uint8)(1);
        session_flow.load_requested = (uint32)(0);
    }
    asset_register_gpu_storage();
    disp_init_ordering_tables(0x1101);
    model_set_ot_shift(2);
    model_set_proj_distance(180);
    model_set_depth_range(0, 0x10000);
    model_set_back_color(50, 50, 50);
    model_set_light(0, &menu_light_dir, 110, 110, 110);
    model_set_light(1, &menu_light_dir, 0, 0, 0);
    model_set_light(2, &menu_light_dir, 0, 0, 0);
    model_set_fog(3000, 16000, 0, 0, 0);
    rect = menu_pal_rect;
    vram = (uint8 *)malloc(0x80000);
    XASSERT(!(vram == NULL));
    vram_size = archive_read_file("INPRSDAT\\VRAM.OUT", vram, 0x80000);
    XASSERT(!(vram_size > 0x80000));
    rect.y = 0;
    width = rect.w;
    height = rect.h;
    bytes = width > 0 && height > 0 ? (uint32)width * (uint32)height * 2 : 0;
    XASSERT(!(bytes > vram_size));
    LoadImagePSX(&rect, (uint32 *)vram);
    DrawSync(0);
    free(vram);
    bytes = archive_aligned_size("INPRSDAT\\INPRES.DAT");
    XASSERT(!(bytes > sizeof(game_resource_storage.file_words)));
    presentation_model_image.bytes = (uint8 *)game_resource_storage.file_words;
    presentation_model_image.size = bytes;
    presentation_model_image.size = archive_read_file("INPRSDAT\\INPRES.DAT", presentation_model_image.bytes, bytes);
    XASSERT(!(presentation_model_image.size > bytes || presentation_model_image.size < 8));
    model_file = presentation_model_image.bytes;
    memcpy(&val, model_file, sizeof(val));
    XASSERT(!(val > (presentation_model_image.size - 4) / 4));
    count = 1;
    cursor = model_file + 4;
    if (val != 0)
    {
        do
        {
            memcpy(&val, cursor, sizeof(val));
            XASSERT(!(val > presentation_model_image.size));
            memcpy(&val, model_file, sizeof(val));
            ++count;
            cursor += 4;
        } while (val >= count);
    }
    val = presentation_background.cycle + 1;
    val = ((sint32)val % 6);
    presentation_background.cycle = (uint32)(val);
    name_len = (uint32)sprintf(filename, presentation_background_format, (sint32)(val + 1));
    {
        sint32 operation_ok = !(archive_read_file(filename, presentation_background.image, (uint32)sizeof(presentation_background.image)) < 20 + 512 * 256 * 2);
        XASSERT(operation_ok);
    }
    model_file = presentation_model_image.bytes;
    memcpy(&val, model_file + 4, sizeof(val));
    XASSERT(!(val > presentation_model_image.size || presentation_model_image.size - val < 4));
    tex_set_table(model_file + val, presentation_model_image.size - val);
    tex = tex_find("Hicar");
    xport_store_le16(tex + 8, (uint16)(152));
    xport_store_le16(tex + 10, (uint16)(48));
    presentation_init_packets();
    presentation_upload_background();
    val = disp_state.bank;
    disp_state.bank = val ^ 1;
    presentation_upload_background();
    val = disp_state.bank;
    disp_state.bank = val ^ 1;
    SetDispMask(1);
    disp_prepare(2);
    val = disp_options.scr_x;
    other = disp_options.scr_y;
    disp_state.frames[0].disp.screen.x = (sint16)(val);
    disp_state.frames[1].disp.screen.x = (sint16)(val);
    disp_state.frames[0].disp.screen.y = (sint16)(other);
    disp_state.frames[1].disp.screen.y = (sint16)(other);
    disp_swap();
    return (sint32)disp_apply();
}

static uint32 update_single(void)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 val, mode;
    sint32 choice;

    FUNCTION_MARKER(0x80058298, "SLUS_006.10");
    race_menu.commands[1] = (uint16)(1);
    race_menu.commands[2] = (uint16)(8);
    race_menu.commands[3] = (uint16)(6);
    race_menu.commands[4] = (uint16)(4);
    race_menu.commands[5] = (uint16)(0xFFFF);
    race_menu.commands[0] = (uint16)(0);
    race_menu.items_y = (uint16)(62);
    draw_unlocks();
    val = input_ports[0].pressed & 0x4000;
    if (val == 0)
        return val;
    audio_play_menu_accept();
    choice = (sint16)(race_menu.sel);
    switch ((uint32)choice)
    {
        case 0:
            race_control.results_finished = (uint16)(1);
            session_flow.phase = (4);
            return 4;
        case 1:
            level_desc = &scene_level_image.desc;
            level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
            val = level_header[30];
            if ((val & 8) == 0)
                return 1;
            race_control.results_finished = (uint16)(1);
            session_flow.phase = (6);
            return 6;
        case 2:
            val = race_control.exit_reason;
            if (val != 0)
                return val;
            val = race_session.demo;
            if (val == 0)
                race_menu.state = (uint32)(30);
            return 30;
        case 3:
            race_menu.state = (uint32)(10);
            return 10;
        case 4:
            mode = race_session.event_mode;
            race_control.results_finished = (uint16)(1);
            session_flow.menu_state = (80);
            val = 4;
            if (mode != 4)
            {
                mode = recs_has_candidate();
                val = 1;
                if (mode == 1)
                {
                    val = race_control.exit_reason;
                    if (val == 0)
                    {
                        mode = session_flow.player_name_entered;
                        val = mode == 1 ? 120 : 190;
                        session_flow.menu_state = (val);
                    }
                }
            }
            session_flow.phase = (0);
            return val;
        default:
            return (uint32)choice << 2;
    }
}

static uint32 update_split(void)
{
    uint32 buttons, result, val;
    sint32 choice;

    FUNCTION_MARKER(0x80058474, "SLUS_006.10");
    buttons = input_ports[0].pressed;
    race_menu.commands[0] = (uint16)(5);
    race_menu.commands[1] = (uint16)(8);
    race_menu.commands[2] = (uint16)(4);
    race_menu.commands[3] = (uint16)(0xFFFF);
    race_menu.commands[4] = (uint16)(0xFFFF);
    race_menu.commands[5] = (uint16)(0xFFFF);
    race_menu.items_y = (uint16)(62);
    if ((buttons & 0x4000) == 0)
        return 62;
    audio_play_menu_accept();
    choice = (sint16)(race_menu.sel);
    if (choice == 1)
    {
        result = race_control.exit_reason;
        if (result == 0)
        {
            val = race_session.demo;
            result = 30;
            if (val == 0)
                race_menu.state = (uint32)(result);
        }
        return result;
    }
    if (choice == 0)
    {
        race_control.results_finished = (uint16)(1);
        session_flow.phase = (4);
        return 4;
    }
    if (choice != 2)
        return 1;
    race_control.results_finished = (uint16)(1);
    session_flow.menu_state = (80);
    result = recs_has_candidate();
    if (result == 1)
    {
        result = race_control.exit_reason;
        if (result == 0)
        {
            val = session_flow.player_name_entered;
            result = val == 1 ? 120 : 190;
            session_flow.menu_state = (result);
        }
    }
    session_flow.phase = (0);
    return result;
}

static uint32 update_championship(void)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 buttons, choice, result, val, mode;

    FUNCTION_MARKER(0x800585E4, "SLUS_006.10");
    buttons = input_ports[0].pressed;
    race_menu.commands[0] = (uint16)(2);
    race_menu.commands[1] = (uint16)(1);
    race_menu.commands[2] = (uint16)(8);
    race_menu.commands[3] = (uint16)(7);
    race_menu.commands[4] = (uint16)(4);
    race_menu.commands[5] = (uint16)(0xFFFF);
    race_menu.items_y = (uint16)(72);
    session_flow.card_menu_from_event = 0;
    if ((buttons & 0x4000) == 0)
        return 72;
    audio_play_menu_accept();
    choice = (sint16)(race_menu.sel);
    if (choice >= 5)
        return choice << 2;
    switch (choice)
    {
        case 0:
            val = session_flow.event_round;
            mode = race_session.cup;
            ++val;
            session_flow.event_round = (uint32)(val);
            if (mode == 3 && (sint32)val < 10)
            {
                race_menu.state = (uint32)(10);
                return 10;
            }
            race_control.results_finished = (uint16)(1);
            session_flow.phase = (5);
            return 5;
        case 1:
            level_desc = &scene_level_image.desc;
            level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
            val = level_header[30];
            if ((val & 8) == 0)
                return 1;
            race_control.results_finished = (uint16)(1);
            session_flow.phase = (6);
            return 6;
        case 2:
            result = race_control.exit_reason;
            if (result == 0)
            {
                val = race_session.demo;
                result = 30;
                if (val == 0)
                    race_menu.state = (uint32)(result);
            }
            return result;
        case 3:
            session_flow.card_menu_from_event = 1;
            save_write_game();
            session_flow.card_menu_from_event = 0;
            return race_menu.state;
        default:
            race_control.results_finished = (uint16)(1);
            session_flow.phase = (0);
            session_flow.menu_state = (80);
            return 80;
    }
}

static intptr update_switched_race(void)
{
    uint32 buttons, result, val, model1, model0;
    sint32 choice;

    FUNCTION_MARKER(0x80058A08, "SLUS_006.10");
    buttons = input_ports[0].pressed;
    race_menu.commands[1] = (uint16)(8);
    race_menu.commands[2] = (uint16)(4);
    race_menu.commands[4] = (uint16)(0xFFFF);
    race_menu.commands[3] = (uint16)(0xFFFF);
    race_menu.commands[5] = (uint16)(0xFFFF);
    race_menu.commands[0] = (uint16)(0);
    race_menu.items_y = (uint16)(72);
    if ((buttons & 0x4000) == 0)
        return 72;
    audio_play_menu_accept();
    choice = (sint16)(race_menu.sel);
    if (choice == 1)
    {
        result = race_control.exit_reason;
        if (result == 0)
        {
            val = race_session.demo;
            result = 30;
            if (val == 0)
                race_menu.state = (uint32)(result);
        }
        return result;
    }
    if (choice == 0)
    {
        result = race_control.exit_reason;
        race_control.results_finished = (uint16)(1);
        session_flow.phase = (5);
        if (result != 0)
            return result;
        result = race_session.demo;
        if (result != 0)
            return result;
        val = session_flow.event_round;
        session_flow.event_round = (uint32)(val ^ 1);
        race_reset_results();
        model1 = session_car_choices.cars[1].model[0];
        model0 = (sint8)session_car_choices.cars[0].model[0];
        session_car_choices.cars[0].model[0] = (uint8)(model1);
        session_car_choices.cars[1].model[0] = (uint8)(model0);
        return (intptr)&session_car_choices.cars[0].model[0];
    }
    if (choice != 2)
        return 1;
    race_control.results_finished = (uint16)(1);
    session_flow.phase = (0);
    session_flow.menu_state = (80);
    return 80;
}

static uint32 update_cup_finish(void)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 buttons, val, result;
    sint32 choice;

    FUNCTION_MARKER(0x80058B88, "SLUS_006.10");
    buttons = input_ports[0].pressed;
    race_menu.commands[2] = (uint16)(4);
    race_menu.commands[3] = (uint16)(0xFFFF);
    race_menu.commands[4] = (uint16)(0xFFFF);
    race_menu.commands[5] = (uint16)(0xFFFF);
    race_menu.commands[0] = (uint16)(0);
    race_menu.commands[1] = (uint16)(1);
    race_menu.items_y = (uint16)(88);
    if ((buttons & 0x4000) == 0)
        return 88;
    audio_play_menu_accept();
    choice = (sint16)(race_menu.sel);
    if (choice == 1)
    {
        level_desc = &scene_level_image.desc;
        level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
        if ((level_header[30] & 8) != 0)
        {
            race_control.results_finished = (uint16)(1);
            session_flow.phase = (6);
        }
        return 6;
    }
    if (choice >= 2)
    {
        if (choice == 2)
        {
            race_control.results_finished = (uint16)(1);
            session_flow.phase = (0);
            session_flow.menu_state = (80);
        }
        return 80;
    }
    if (choice != 0)
        return 3;
    if (race_session.cup == 3)
    {
        race_menu.state = (uint32)(10);
        return 10;
    }
    val = car_states[0].place;
    race_control.results_finished = (uint16)(1);
    session_flow.phase = (5);
    if (val != 0)
        return 5;
    result = race_control.exit_reason;
    if (result == 0)
    {
        result = session_flow.event_round + 1;
        session_flow.event_round = (uint32)(result);
    }
    return result;
}

static uint32 update_commands(void)
{
    uint32 buttons, val, prev, idx, pal;
    const uint16 *entry;
    const char *text;
    sint32 choice, item, y;

    FUNCTION_MARKER(0x80057F44, "SLUS_006.10");
    buttons = input_ports[0].pressed;
    if ((buttons & 0x10) != 0)
    {
        choice = (sint16)(race_menu.sel);
        if (choice > 0 && (sint16)(race_menu.commands[choice - 1]) != -1)
        {
            audio_play_menu_move();
            val = race_menu.sel - 1;
            item = (sint16)(race_menu.commands[(sint16)val]);
            race_menu.sel = (uint16)(val);
            if (item == 8 && (race_control.exit_reason != 0 || race_session.demo != 0 || race_menu.results_disabled != 0))
            {
                val = race_menu.sel;
                race_menu.sel = (uint16)(val - 1);
            }
            choice = (sint16)(race_menu.sel);
            item = (sint16)(race_menu.commands[choice]);
            if (item == 1 && race_menu.replay_disabled != 0)
                race_menu.sel = (uint16)((uint32)choice - 1);
        }
    }
    buttons = input_ports[0].pressed;
    if ((buttons & 0x40) != 0)
    {
        choice = (sint16)(race_menu.sel);
        if (choice < 5 && (sint16)(race_menu.commands[choice + 1]) != -1)
        {
            audio_play_menu_move();
            prev = race_menu.sel;
            val = prev + 1;
            item = (sint16)(race_menu.commands[(sint16)val]);
            race_menu.sel = (uint16)(val);
            if (item == 1 && race_menu.replay_disabled != 0)
                race_menu.sel = (uint16)(prev + 2);
            choice = (sint16)(race_menu.sel);
            item = (sint16)(race_menu.commands[choice]);
            if (item == 8 && (race_control.exit_reason != 0 || race_session.demo != 0 || race_menu.results_disabled != 0))
            {
                val = race_menu.sel;
                race_menu.sel = (uint16)(val + 1);
            }
        }
    }
    presentation_select_pal(0);
    presentation_draw_image(128, 104, 206, "cross");
    presentation_select_pal(2);
    presentation_draw_text(0, 117, 198, "SELECT");
    for (idx = 0; idx < 6; ++idx)
    {
        entry = &race_menu.commands[idx];
        item = (sint16)(*entry);
        if (item == -1)
            continue;
        choice = (sint16)(race_menu.sel);
        pal = choice == (sint32)idx ? 3 : 2;
        presentation_select_pal(pal);
        item = (sint16)(*entry);
        if (item == 8 && (race_control.exit_reason != 0 || race_session.demo != 0 || race_menu.results_disabled != 0))
        {
            presentation_select_pal(7);
            item = (sint16)(*entry);
        }
        if (item == 1 && race_menu.replay_disabled != 0)
            presentation_select_pal(7);
        item = (sint16)(*entry);
        y = (sint16)(race_menu.items_y);
        text = command_labels[(uint32)item];
        presentation_draw_text(128, 256, (sint32)(idx * 20) + y, text);
    }
    return 0;
}

static void update_race_menu(void)
{
    const char *outcome;
    uint32 buttons, state, val, other, text;
    sint32 offset, choice;

    FUNCTION_MARKER(0x80058D0C, "SLUS_006.10");
    buttons = input_ports[0].pressed;
    /* The original 4AC1C button hook is empty */
    (void)buttons;
    state = race_menu.state;
    if (state >= 102)
        return;
    // TODO: Restore the in-game storage menu after xport_save/load integration
    if (!memory_card_enabled && (state == 20 || state == 21))
    {
        session_flow.card_menu_from_event = 0;
        session_flow.load_requested = (uint32)(0);
        race_menu_select_default();
        return;
    }
    switch (state)
    {
        case 0:
        case 10:
        case 20:
        case 30:
        case 40:
        case 50:
        case 60:
        case 70:
        case 80:
        case 90:
        case 100:
            for (offset = 69; offset >= 0; --offset)
                presentation_items[offset].active = (0);
            if (state == 10)
                car_selection_begin(1);
            else if (state == 20)
            {
                presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_CARD]);
                race_menu.state = (uint32)(21);
                card_menu_begin(1);
                return;
            }
            else if (state == 30)
                results_menu_begin();
            race_menu.state = (uint32)(state + 1);
            return;
        case 1:
            update_single();
            update_commands();
            return;
        case 11:
            car_selection_update(1);
            return;
        case 21:
            card_menu_update(1);
            return;
        case 31:
            results_menu_update();
            return;
        case 41:
            update_championship();
            update_commands();
            race_menu.return_state = (uint32)(40);
            return;
        case 51:
            presentation_select_pal(2);
            outcome = outcome_labels[0];
            presentation_draw_text(131, 256, 100, outcome);
            outcome = outcome_labels[1];
            presentation_draw_text(131, 256, 120, outcome);
            buttons = input_ports[0].pressed;
            if ((buttons & 0x4000) != 0)
                race_menu.state = (uint32)(60);
            return;
        case 61:
            update_cup_finish();
            update_commands();
            race_menu.return_state = (uint32)(60);
            return;
        case 71:
            update_split();
            update_commands();
            return;
        case 81:
            buttons = input_ports[0].pressed;
            race_menu.commands[0] = (uint16)(2);
            race_menu.commands[1] = (uint16)(8);
            race_menu.commands[2] = (uint16)(4);
            race_menu.commands[3] = (uint16)(0xFFFF);
            race_menu.commands[4] = (uint16)(0xFFFF);
            race_menu.commands[5] = (uint16)(0xFFFF);
            race_menu.items_y = (uint16)(72);
            if ((buttons & 0x4000) != 0)
            {
                audio_play_menu_accept();
                choice = (sint16)(race_menu.sel);
                if (choice == 0)
                {
                    val = session_flow.event_round;
                    race_control.results_finished = (uint16)(1);
                    session_flow.phase = (5);
                    session_flow.event_round = (uint32)(val + 1);
                }
                else if (choice == 1)
                {
                    if (race_control.exit_reason == 0 && race_session.demo == 0)
                        race_menu.state = (uint32)(30);
                }
                else if (choice == 2)
                {
                    race_control.results_finished = (uint16)(1);
                    session_flow.phase = (0);
                    session_flow.menu_state = (80);
                }
            }
            update_commands();
            return;
        case 91:
            buttons = input_ports[0].pressed;
            race_menu.commands[0] = (uint16)(9);
            race_menu.commands[1] = (uint16)(4);
            race_menu.commands[2] = (uint16)(0xFFFF);
            race_menu.commands[3] = (uint16)(0xFFFF);
            race_menu.commands[4] = (uint16)(0xFFFF);
            race_menu.commands[5] = (uint16)(0xFFFF);
            race_menu.items_y = (uint16)(72);
            if ((buttons & 0x4000) != 0)
            {
                audio_play_menu_accept();
                choice = (sint16)(race_menu.sel);
                if (choice == 0)
                {
                    val = race_control.exit_reason;
                    race_control.results_finished = (uint16)(1);
                    session_flow.phase = (5);
                    if (val == 0 && race_session.demo == 0)
                    {
                        val = session_car_choices.cars[1].model[0];
                        other = session_flow.event_round;
                        text = (sint8)session_car_choices.cars[0].model[0];
                        session_car_choices.cars[0].model[0] = (uint8)(val);
                        session_car_choices.cars[1].model[0] = (uint8)(text);
                        session_flow.event_round = (uint32)(other ^ 1);
                    }
                }
                else if (choice == 1)
                {
                    race_control.results_finished = (uint16)(1);
                    session_flow.phase = (0);
                    session_flow.menu_state = (80);
                }
            }
            update_commands();
            return;
        case 101:
            update_switched_race();
            update_commands();
            return;
        default:
            return;
    }
}

sint32 race_menu_run(void)
{
    uint32 val, tick, buttons, buffer;
    const uint8 *image;
    PSX_RECT rect;
    sint32 offset, random;

    FUNCTION_MARKER(0x80057B30, "SLUS_006.10");
    race_control.frame_counter = (uint32)(0);
    race_control.results_finished = (uint16)(0);
    for (offset = 69; offset >= 0; --offset)
        presentation_items[offset].active = (0);
    random = randPSX();
    audio_play_xa(random % 8);
    tick = (uint32)VSync(-1);
    if (race_session.demo == 0)
        collect_unlocks();
    while ((sint16)race_control.results_finished == 0)
    {
        input_poll();
        disp_swap();
        disp_clear_ordering_table();
        model_set_view_pos(&presentation_menu_view_pos);
        model_set_view_angles(&presentation_menu_view_angles);
        presentation_begin_frame();
        update_race_menu();
        val = presentation_draw_items();
        val = (uint32)timing_read_counter(1, (sint32)val);
        disp_state.pre_sync_ticks = val;
        audio_poll_xa();
        val = (uint32)DrawSync(0);
        val = (uint32)timing_read_counter(1, (sint32)val);
        buttons = input_ports[0].held;
        disp_state.post_sync_ticks = val;
        /* The button hook 13CF8 has no observable body */
        (void)buttons;
        do
        {
            val = (uint32)VSync(-1) - tick;
        } while ((sint32)val <= 0);
        tick = (uint32)VSync(-1);
        timing_reset_counter(1);
        disp_apply();
        if (race_menu.state == 11)
            presentation_upload_car_background();
        else
        {
            image = (const uint8 *)presentation_background.image;
            buffer = disp_state.bank;
            rect.h = 256;
            rect.w = 512;
            rect.x = 0;
            rect.y = (sint16)(buffer << 8);
            LoadImagePSX(&rect, (uint32 *)(image + 20));
        }
        val = race_control.frame_counter;
        race_control.frame_counter = (uint32)(val + 1);
        disp_draw_ordering_table();
    }
    audio_stop_xa();
    return audio_play_request(13, 20, (sint32)audio_options.music_volume, 0, 5120);
}
