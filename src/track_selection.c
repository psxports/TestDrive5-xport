#include <string.h>

#include <stdlib.h>

#include "track_selection.h"
#include "psx_gpu.h"
#include "game.h"

TRACK_SELECTION_STATE track_selection;

// Round each native copy window to whole GPU words
static uint32 preview_copies[3][(0x7FFF + 3) / 4];

static const MODEL_MATRIX preview_matrix = {.words = {4096, 0, 4096, 0, 4096, 0, 0, 0}};

typedef struct TRACK_PREVIEW_TARGET
{
    uint16 x;
    uint16 y;
} TRACK_PREVIEW_TARGET;

typedef char TRACK_PREVIEW_TARGET_LAYOUT_CHECK[sizeof(uint16) == 2 && sizeof(TRACK_PREVIEW_TARGET) == 4 && offsetof(TRACK_PREVIEW_TARGET, x) == 0 && offsetof(TRACK_PREVIEW_TARGET, y) == 2 ? 1 : -1];

static const TRACK_PREVIEW_TARGET preview_targets[47] = {
    {3544, 3912}, {2872, 704}, {3584, 3824}, {376, 2248}, {3528, 520}, {3576, 3824}, {3544, 3912}, {2872, 704}, {3584, 3824}, {376, 2248}, {3528, 520}, {3576, 3824}, {288, 1536}, {3776, 2120}, {368, 2208}, {3512, 3928}, {3528, 552}, {288, 1536}, {3776, 2120}, {368, 2208}, {3512, 3928}, {3528, 552}, {3344, 3648}, {3344, 3648}, {3560, 3904}, {3504, 3496}, {3584, 3792}, {272, 1536}, {3520, 3928}, {3176, 504}, {3784, 3920}, {3560, 3904}, {3504, 3496}, {3584, 3792}, {272, 1536}, {3520, 3928}, {2976, 32}, {2976, 32}, {3760, 608}, {3760, 608}, {3544, 3928}, {3608, 3760}, {3392, 3808}, {3680, 3912}, {3576, 3872}, {3616, 3864}, {3592, 3832},
};

static void collect_available_tracks(const sint16 *list, uint32 limit, uint32 category, uint32 flags)
{
    uint32 idx, count, item;
    sint32 val;

    for (idx = 0; idx < limit; ++idx)
    {
        val = list[idx];
        if (val == -1)
            return;
        if (session_track_availability((uint32)val) == category)
        {
            count = track_selection.count;
            item = ((uint32)val - 1) | flags;
            track_selection.entries[count] = (uint32)(item);
            track_selection.count = (uint32)(count + 1);
        }
    }
}

void track_selection_build_list(void)
{
    const sint16 no_tracks[1] = {-1};
    const sint16 *list0 = session_circuit_tracks;
    const sint16 *list1 = session_linear_tracks;
    uint32 mode, idx, count, sel, val;

    FUNCTION_MARKER(0x800AA198, "SLUS_006.10");
    mode = race_session.event_mode;
    track_selection.left_x = (uint32)(0 - 64);
    track_selection.right_x = (uint32)(448);
    menu_choices.car_grp = (0);
    track_selection.count = (uint32)(0);
    if (mode == 1 || mode == 4)
        list1 = no_tracks;
    if (race_session.event_mode == 5)
    {
        list0 = no_tracks;
        list1 = session_multiplayer_tracks;
    }
    for (idx = 0; idx < 46; ++idx)
        track_selection.entries[45 - idx] = (uint32)(0);
    collect_available_tracks(list0, 24, 2, 0);
    collect_available_tracks(list1, 7, 2, 0);
    collect_available_tracks(list0, 24, 1, 0x100);
    collect_available_tracks(list1, 7, 1, 0x100);
    count = track_selection.count;
    if ((sint32)count > 0)
    {
        sel = session_options.retained_track;
        for (idx = 0; (sint32)idx < (sint32)count; ++idx)
        {
            val = (uint8)(track_selection.entries[idx]);
            if (val == sel)
                menu_choices.car_grp = (idx);
        }
    }
}

void track_selection_prepare_previews(void)
{
    uint16 wire_halfword_1, wire_halfword_2;

    uint32 wire_word_4, wire_word_8, wire_word_9, wire_word_10;
    uint8 *owner;
    const uint8 *src;
    uint8 *dst;
    uint32 src_offset, buffer, offset, word, idx, count;
    uint32 words[4];
    sint32 val;

    FUNCTION_MARKER(0x800AA79C, "SLUS_006.10");
    owner = presentation_model_image.bytes;
    XASSERT(!(presentation_model_image.size < 0x68));
    memcpy(&src_offset, owner + 0x64, sizeof(src_offset));
    XASSERT(!(src_offset > presentation_model_image.size || presentation_model_image.size - src_offset < 0x7FFF));
    {
        sint32 operation_ok = !(!gpu_register_packet_range(preview_copies, (uint32)sizeof(preview_copies)));
        XASSERT(operation_ok);
    }
    menu_choices.car_grp = (0);
    track_selection.count = (uint32)(0);
    menu_choices.panel_selection = (0);
    track_selection.left_x = (uint32)(0 - 64);
    track_selection.right_x = (uint32)(448);
    track_selection.preview_buffers[0].bytes = owner + src_offset;
    track_selection.preview_buffers[0].capacity = 0x7FFF;
    for (buffer = 1; buffer < 4; ++buffer)
    {
        owner = presentation_model_image.bytes;
        dst = (uint8 *)preview_copies[buffer - 1];
        track_selection.preview_buffers[buffer].bytes = dst;
        track_selection.preview_buffers[buffer].capacity = 0x7FFF;
        XASSERT(!(presentation_model_image.size < 0x68));
        memcpy(&src_offset, owner + 0x64, sizeof(src_offset));
        XASSERT(!(src_offset > presentation_model_image.size || presentation_model_image.size - src_offset < 0x7FFF));
        src = owner + src_offset;
        for (offset = 0; offset < 0x7FF0; offset += 16)
        {
            for (word = 0; word < 4; ++word)
                memcpy(&words[word], src + offset + word * 4, sizeof(words[word]));
            for (word = 0; word < 4; ++word)
                (wire_word_4 = (uint32)(words[word]), memcpy(dst + offset + word * 4, &wire_word_4, sizeof(wire_word_4)));
        }
        memcpy(&words[0], src + 0x7FF0, sizeof(words[0]));
        memcpy(&words[1], src + 0x7FF4, sizeof(words[1]));
        memcpy(&words[2], src + 0x7FF8, sizeof(words[2]));
        val = (sint16)(memcpy(&wire_halfword_1, src + 0x7FFC, sizeof(wire_halfword_1)), wire_halfword_1);
        (wire_word_8 = (uint32)(words[0]), memcpy(dst + 0x7FF0, &wire_word_8, sizeof(wire_word_8)));
        (wire_word_9 = (uint32)(words[1]), memcpy(dst + 0x7FF4, &wire_word_9, sizeof(wire_word_9)));
        (wire_word_10 = (uint32)(words[2]), memcpy(dst + 0x7FF8, &wire_word_10, sizeof(wire_word_10)));
        (wire_halfword_2 = (uint16)((uint32)val), memcpy(dst + 0x7FFC, &wire_halfword_2, sizeof(wire_halfword_2)));
        val = (sint8)src[0x7FFE];
        dst[0x7FFE] = val;
    }
    for (idx = 0; idx < 46; ++idx)
        track_selection.entries[45 - idx] = (uint32)(0);
    for (idx = 0; idx < 6; ++idx)
    {
        val = (sint16)session_multiplayer_tracks[idx];
        if (session_track_availability((uint32)val) == 2)
        {
            count = track_selection.count;
            track_selection.entries[count] = (uint32)((uint32)val - 1);
            track_selection.count = (uint32)(count + 1);
        }
    }
}

static uint32 draw_sel_track(uint32 selection)
{
    VEC3 model_pos;
    MODEL_RESOURCE render_model;
    uint8 *model_data;
    MODEL_MATRIX matrix = preview_matrix;
    uint32 offset = selection * 4;
    uint8 *owner;
    uint32 target_x, target_y, cur, val, delta, object;

    sint32 changed;

    FUNCTION_MARKER(0x800A9B44, "SLUS_006.10");
    target_x = (sint16)(preview_targets[selection].x);
    val = (sint16)(track_selection.preview.target_x);
    changed = val != target_x;
    if (!changed)
    {
        val = (sint16)(track_selection.preview.target_y);
        target_y = (sint16)(preview_targets[selection].y);
        changed = val != target_y;
    }
    if (changed)
    {
        target_y = preview_targets[selection].y;
        cur = track_selection.preview.angle_x;
        val = (sint16)(preview_targets[selection].x);
        track_selection.preview.target_x = (uint16)(target_x);
        cur &= 0xFFF00;
        track_selection.preview.angle_x = (uint32)(cur);
        delta = val - (uint32)((sint32)cur >> 8);
        track_selection.preview.target_y = (uint16)(target_y);
        track_selection.preview.step_x = (uint32)(delta);
        if ((sint32)delta < -2048)
            track_selection.preview.step_x = (uint32)(delta + 4096);
        delta = track_selection.preview.step_x;
        if ((sint32)delta > 2048)
            track_selection.preview.step_x = (uint32)(delta - 4096);
        delta = track_selection.preview.step_x << 8;
        cur = track_selection.preview.angle_y;
        val = (sint16)(preview_targets[selection].y);
        track_selection.preview.step_x = (uint32)((uint32)((sint32)delta / 64));
        cur &= 0xFFF00;
        track_selection.preview.angle_y = (uint32)(cur);
        delta = val - (uint32)((sint32)cur >> 8);
        track_selection.preview.step_y = (uint32)(delta);
        if ((sint32)delta < -2048)
            track_selection.preview.step_y = (uint32)(delta + 4096);
        delta = track_selection.preview.step_y;
        if ((sint32)delta > 2048)
            track_selection.preview.step_y = (uint32)(delta - 4096);
        delta = track_selection.preview.step_y << 8;
        track_selection.preview.step_y = (uint32)((uint32)((sint32)delta / 64));
    }
    cur = track_selection.preview.angle_x;
    val = (sint16)(track_selection.preview.target_x);
    if ((uint32)((sint32)cur >> 8) != val)
        track_selection.preview.angle_x = (uint32)(cur + track_selection.preview.step_x);
    val = track_selection.preview.angle_x;
    cur = track_selection.preview.angle_y;
    target_y = (sint16)(track_selection.preview.target_y);
    track_selection.preview.angle_x = (uint32)(val & 0xFFFFF);
    if ((uint32)((sint32)cur >> 8) != target_y)
        track_selection.preview.angle_y = (uint32)(cur + track_selection.preview.step_y);
    val = track_selection.preview.angle_y;
    track_selection.preview.angle_y = (uint32)(val & 0xFFFFF);
    model_set_view_center(256, 118);
    owner = presentation_model_image.bytes;
    val = track_selection.preview.angle_x;
    XASSERT(!(presentation_model_image.size < 0x68));
    memcpy(&object, owner + 0x64, sizeof(object));
    {
        uint32 model_offset = object;
        XASSERT(!(model_offset > presentation_model_image.size || presentation_model_image.size - model_offset < 28));
        model_data = owner + model_offset;
        render_model.bytes = model_data;
        render_model.capacity = presentation_model_image.size - model_offset;
    }
    ((MODEL_HEADER *)model_data)->position.bits.x = 0;
    ((MODEL_HEADER *)model_data)->position.bits.y = 0;
    ((MODEL_HEADER *)model_data)->position.bits.z = 0x9C400;
    RotMatrixX((sint32)val >> 8, &matrix.sdk);
    val = track_selection.preview.angle_y;
    RotMatrixY((sint32)val >> 8, &matrix.sdk);
    model_apply_light_matrix(&matrix.sdk);
    model_pos.bits.x = ((const MODEL_HEADER *)model_data)->position.bits.x;
    model_pos.bits.y = ((const MODEL_HEADER *)model_data)->position.bits.y;
    model_pos.bits.z = ((const MODEL_HEADER *)model_data)->position.bits.z;
    model_transform_matrix(&matrix.sdk, &model_pos);
    model_draw(&render_model);
    presentation_select_pal(3);
    return presentation_draw_text(128, 256, 114, "0");
}

void track_selection_update(void)
{
    uint32 buttons, sel, last, val, rec, count, history, state;
    sint32 left, right;

    FUNCTION_MARKER(0x800AA45C, "SLUS_006.10");
    buttons = input_ports[0].pressed;
    presentation_menu.sel = (0xFFFFFFFF);
    if ((buttons & 0x80) != 0)
    {
        val = menu_choices.car_grp;
        if ((sint32)val > 0)
        {
            menu_choices.car_grp = (val - 1);
            audio_play_menu_high();
        }
    }
    if ((input_ports[0].pressed & 0x20) != 0)
    {
        count = track_selection.count;
        val = menu_choices.car_grp;
        if ((sint32)val < (sint32)(count - 1))
        {
            menu_choices.car_grp = (val + 1);
            audio_play_menu_high();
        }
    }
    sel = menu_choices.car_grp;
    left = (sint32)sel > 0 ? 64 : -64;
    last = track_selection.count - 1;
    right = (sint32)sel < (sint32)last ? 448 : 576;
    val = track_selection.left_x;
    if ((sint32)val < left)
    {
        track_selection.left_x = (uint32)(val + 16);
        val = track_selection.left_x;
    }
    if (left < (sint32)val)
        track_selection.left_x = (uint32)(val - 16);
    val = track_selection.right_x;
    if ((sint32)val < right)
    {
        track_selection.right_x = (uint32)(val + 16);
        val = track_selection.right_x;
    }
    if (right < (sint32)val)
        track_selection.right_x = (uint32)(val - 16);
    presentation_items[3].active = (1);
    presentation_items[4].active = (1);
    if (sel == 0)
        presentation_items[3].active = (0);
    if (sel == last)
        presentation_items[4].active = (0);
    val = (uint8)(track_selection.entries[sel]);
    draw_sel_track(val);
    if ((input_ports[0].pressed & 0x4008) != 0)
    {
        sel = menu_choices.car_grp;
        rec = track_selection.entries[sel];
        if ((rec & 0x100) != 0)
            audio_play_menu_error();
        else
        {
            history = menu_history.depth;
            state = session_flow.menu_state;
            session_options.sel_track = (rec & 255);
            session_options.retained_track = (uint32)(rec & 255);
            XASSERT(!(history >= MENU_HISTORY_CAPACITY));
            menu_history.states[history] = (uint8)(state - 1);
            history = menu_history.depth;
            session_flow.menu_state = (20);
            menu_history.depth = history + 1;
            session_apply_menu_mode();
        }
    }
    if ((input_ports[0].pressed & 0x1000) != 0)
    {
        session_pop_menu();
        audio_play_menu_accept();
    }
    sel = menu_choices.car_grp;
    rec = track_selection.entries[sel];
    if ((rec & 0x100) != 0)
    {
        presentation_select_pal(3);
        presentation_draw_text(128, 256, 180, "LOCKED");
    }
    presentation_select_pal(2);
    sel = menu_choices.car_grp;
    val = (uint8)(track_selection.entries[sel]);
    presentation_draw_text(128, 256, 170, ((uint32)(val + 1) < 48 ? session_event_names[val + 1] : ""));
}

static uint32 draw_multiplayer_track(uint32 slot, uint32 selection)
{
    VEC3 model_pos;
    MODEL_RESOURCE render_model;
    uint8 *model_data;
    MODEL_MATRIX matrix = preview_matrix;
    uint32 offset = selection * 4, slot_word = slot * 4, slot_half = slot * 2;
    uint32 target_x, target_y, val, cur, delta;

    sint32 changed;

    FUNCTION_MARKER(0x800A9E54, "SLUS_006.10");
    target_x = (sint16)(preview_targets[selection].x);
    val = (sint16)(track_selection.previews[slot].target_x);
    changed = val != target_x;
    if (!changed)
    {
        val = (sint16)(track_selection.previews[slot].target_y);
        target_y = (sint16)(preview_targets[selection].y);
        changed = val != target_y;
    }
    if (changed)
    {
        track_selection.previews[slot].target_x = (uint16)(target_x);
        target_y = preview_targets[selection].y;
        track_selection.previews[slot].target_y = (uint16)(target_y);
        cur = track_selection.previews[slot].angle_x & 0xFFF00;
        track_selection.previews[slot].angle_x = (uint32)(cur);
        val = (sint16)(preview_targets[selection].x);
        delta = val - (uint32)((sint32)cur >> 8);
        track_selection.previews[slot].step_x = (uint32)(delta);
        if ((sint32)delta < -2048)
            track_selection.previews[slot].step_x = (uint32)(delta + 4096);
        delta = track_selection.previews[slot].step_x;
        if ((sint32)delta > 2048)
            track_selection.previews[slot].step_x = (uint32)(delta - 4096);
        delta = track_selection.previews[slot].step_x << 8;
        track_selection.previews[slot].step_x = (uint32)((uint32)((sint32)delta / 64));
        cur = track_selection.previews[slot].angle_y & 0xFFF00;
        track_selection.previews[slot].angle_y = (uint32)(cur);
        val = (sint16)(preview_targets[selection].y);
        delta = val - (uint32)((sint32)cur >> 8);
        track_selection.previews[slot].step_y = (uint32)(delta);
        if ((sint32)delta < -2048)
            track_selection.previews[slot].step_y = (uint32)(delta + 4096);
        delta = track_selection.previews[slot].step_y;
        if ((sint32)delta > 2048)
            track_selection.previews[slot].step_y = (uint32)(delta - 4096);
        delta = track_selection.previews[slot].step_y << 8;
        track_selection.previews[slot].step_y = (uint32)((uint32)((sint32)delta / 64));
    }
    cur = track_selection.previews[slot].angle_x;
    val = (sint16)(track_selection.previews[slot].target_x);
    if ((uint32)((sint32)cur >> 8) != val)
        track_selection.previews[slot].angle_x = (uint32)(cur + track_selection.previews[slot].step_x);
    val = track_selection.previews[slot].angle_x;
    track_selection.previews[slot].angle_x = (uint32)(val & 0xFFFFF);
    cur = track_selection.previews[slot].angle_y;
    val = (sint16)(track_selection.previews[slot].target_y);
    if ((uint32)((sint32)cur >> 8) != val)
        track_selection.previews[slot].angle_y = (uint32)(cur + track_selection.previews[slot].step_y);
    val = track_selection.previews[slot].angle_y;
    track_selection.previews[slot].angle_y = (uint32)(val & 0xFFFFF);
    model_set_view_center(196, (sint32)(slot * 40 + 72));
    model_data = track_selection.preview_buffers[slot].bytes;
    render_model.bytes = model_data;
    render_model.capacity = track_selection.preview_buffers[slot].capacity;
    XASSERT(!(render_model.bytes == NULL || render_model.capacity < 28));
    ((MODEL_HEADER *)model_data)->position.bits.x = 0;
    ((MODEL_HEADER *)model_data)->position.bits.y = 0;
    ((MODEL_HEADER *)model_data)->position.bits.z = 0x213400;
    val = track_selection.previews[slot].angle_x;
    RotMatrixX((sint32)val >> 8, &matrix.sdk);
    val = track_selection.previews[slot].angle_y;
    RotMatrixY((sint32)val >> 8, &matrix.sdk);
    model_apply_light_matrix(&matrix.sdk);
    model_pos.bits.x = ((const MODEL_HEADER *)model_data)->position.bits.x;
    model_pos.bits.y = ((const MODEL_HEADER *)model_data)->position.bits.y;
    model_pos.bits.z = ((const MODEL_HEADER *)model_data)->position.bits.z;
    model_transform_matrix(&matrix.sdk, &model_pos);
    model_draw(&render_model);
    presentation_select_pal(3);
    return presentation_draw_text(128, 196, (sint32)(slot * 40 + 62), presentation_pres_text.player_name_empty_4);
}

void track_selection_update_multiplayer(void)
{
    char *name;
    uint32 buttons, val, count, sel, slot, idx, history, state;
    uint32 left_pos, right_pos, rotation, rec;
    sint32 wanted, left, right;

    FUNCTION_MARKER(0x800AA920, "SLUS_006.10");
    buttons = input_ports[0].pressed;
    presentation_menu.sel = (0xFFFFFFFF);
    if ((buttons & 0x10) != 0 && (sint32)menu_choices.panel_selection > 0)
    {
        audio_play_menu_move();
        menu_choices.panel_selection = (menu_choices.panel_selection - 1);
    }
    if ((input_ports[0].pressed & 0x40) != 0 && (sint32)menu_choices.panel_selection < 3)
    {
        audio_play_menu_move();
        menu_choices.panel_selection = (menu_choices.panel_selection + 1);
    }
    count = track_selection.count;
    idx = 0;
    if ((sint32)count > 0)
    {
        slot = menu_choices.panel_selection;
        wanted = (sint16)(session_multiplayer_routes[slot]);
        for (; (sint32)idx < (sint32)count; ++idx)
            if (track_selection.entries[idx] == (uint32)wanted)
                break;
    }
    buttons = input_ports[0].pressed;
    menu_choices.car_grp = (idx);
    if ((buttons & 0x80) != 0 && (sint32)idx > 0)
        menu_choices.car_grp = (idx - 1);
    if ((input_ports[0].pressed & 0x20) != 0)
    {
        count = track_selection.count;
        sel = menu_choices.car_grp;
        if ((sint32)sel < (sint32)(count - 1))
        {
            menu_choices.car_grp = (sel + 1);
            sel = menu_choices.car_grp;
        }
    }
    else
        sel = menu_choices.car_grp;
    left = (sint32)sel > 0 ? 64 : -64;
    count = track_selection.count;
    right = (sint32)sel < (sint32)(count - 1) ? 448 : 576;
    val = track_selection.left_x;
    if ((sint32)val < left)
    {
        track_selection.left_x = (uint32)(val + 16);
        val = track_selection.left_x;
    }
    if (left < (sint32)val)
        track_selection.left_x = (uint32)(val - 16);
    val = track_selection.right_x;
    if ((sint32)val < right)
    {
        track_selection.right_x = (uint32)(val + 16);
        val = track_selection.right_x;
    }
    if (right < (sint32)val)
        track_selection.right_x = (uint32)(val - 16);
    left_pos = track_selection.left_x;
    rotation = track_selection.banner_angles.bits.y;
    right_pos = track_selection.right_x;
    buttons = input_ports[0].pressed;
    track_selection.banner_angles.bits.x = (uint16)(0);
    track_selection.banner_angles.bits.z = (uint16)(1024);
    track_selection.banner_angles.bits.y = (uint16)(rotation + 32);
    if ((buttons & 0x4008) != 0)
    {
        wanted = (sint16)(session_multiplayer_routes[0]);
        state = session_flow.menu_state;
        history = menu_history.depth;
        session_options.sel_track = ((uint32)wanted);
        XASSERT(!(history >= MENU_HISTORY_CAPACITY));
        menu_history.states[history] = (uint8)(state - 1);
        history = menu_history.depth;
        session_flow.menu_state = (20);
        menu_history.depth = history + 1;
        session_apply_menu_mode();
    }
    if ((input_ports[0].pressed & 0x1000) != 0)
        session_pop_menu();
    for (slot = 0; slot < 4; ++slot)
    {
        val = 2;
        if (menu_choices.panel_selection == slot)
        {
            sel = menu_choices.car_grp;
            rec = track_selection.entries[sel];
            val = 1;
            session_multiplayer_routes[slot] = (uint16)(rec);
        }
        presentation_select_pal(val);
        wanted = (sint16)(session_multiplayer_routes[slot]);
        name = ((uint32)((uint32)wanted + 1) < 48 ? session_event_names[(uint32)wanted + 1] : "");
        presentation_draw_text(0, 232, (sint32)(64 + slot * 40), name);
        wanted = (sint16)(session_multiplayer_routes[slot]);
        draw_multiplayer_track(slot, (uint32)wanted);
    }
}

void track_selection_update_quick(void)
{
    char *pointer;
    uint32 sel, count, track, val, state, car, volume;

    FUNCTION_MARKER(0x800ABA04, "SLUS_006.10");
    sel = menu_choices.car_grp;
    presentation_items[3].active = (1);
    presentation_items[4].active = (1);
    presentation_items[5].active = (1);
    presentation_items[6].active = (1);
    if (sel == 0)
        presentation_items[3].active = (0);
    count = track_selection.count;
    if (sel == count - 1)
        presentation_items[4].active = (0);
    track = track_selection.quick_model_idx;
    if (track == 0)
        presentation_items[5].active = (0);
    count = car_selection.list_count;
    if (track == count - 1)
        presentation_items[6].active = (0);
    presentation_select_pal(2);
    sel = menu_choices.car_grp;
    car = (uint8)(track_selection.entries[sel]);
    pointer = ((uint32)(car + 1) < 48 ? session_event_names[car + 1] : "");
    presentation_draw_text(128, 256, 90, pointer);
    sel = menu_choices.car_grp;
    if ((track_selection.entries[sel] & 0x100) != 0)
    {
        presentation_select_pal(3);
        presentation_draw_text(128, 256, 98, "LOCKED");
    }
    presentation_select_pal(2);
    track = track_selection.quick_model_idx;
    val = (uint8)car_selection.entries[track];
    pointer = ((uint32)(val) < 44 ? session_model_names[val] : "");
    presentation_draw_text(128, 256, 130, pointer);
    track = track_selection.quick_model_idx;
    if ((car_selection.entries[track] & 0x100) != 0)
    {
        presentation_select_pal(3);
        presentation_draw_text(128, 256, 138, "LOCKED");
    }
    state = menu_choices.panel_selection;
    if (state == 0)
    {
        if ((input_ports[0].pressed & 0x80) != 0)
        {
            sel = menu_choices.car_grp;
            if ((sint32)sel > 0)
            {
                menu_choices.car_grp = (sel - 1);
                audio_play_menu_high();
            }
        }
        if ((input_ports[0].pressed & 0x20) != 0)
        {
            count = track_selection.count;
            sel = menu_choices.car_grp;
            if ((sint32)sel < (sint32)(count - 1))
            {
                menu_choices.car_grp = (sel + 1);
                audio_play_menu_high();
            }
        }
    }
    else if (state == 1)
    {
        if ((input_ports[0].pressed & 0x80) != 0)
        {
            track = track_selection.quick_model_idx;
            if ((sint32)track > 0)
            {
                track_selection.quick_model_idx = (uint32)(track - 1);
                audio_play_menu_high();
            }
        }
        if ((input_ports[0].pressed & 0x20) != 0)
        {
            count = car_selection.list_count;
            track = track_selection.quick_model_idx;
            if ((sint32)track < (sint32)(count - 1))
            {
                track_selection.quick_model_idx = (uint32)(track + 1);
                audio_play_menu_high();
            }
        }
    }
    else if (state == 2 && (input_ports[0].pressed & 0x4008) != 0)
    {
        sel = menu_choices.car_grp;
        if ((track_selection.entries[sel] & 0x100) != 0)
            audio_play_menu_error();
        else
        {
            track = track_selection.quick_model_idx;
            if ((car_selection.entries[track] & 0x100) != 0)
                audio_play_menu_error();
            else
            {
                audio_play_menu_accept();
                sel = menu_choices.car_grp;
                car = (uint8)(track_selection.entries[sel]);
                track = track_selection.quick_model_idx;
                session_car_choices.cars[0].manual_transmission = (uint8)(0);
                val = car_selection.entries[track];
                session_options.sel_track = (car);
                session_options.retained_track = (uint32)(car);
                session_car_choices.cars[0].model[0] = (uint8)(val);
                session_options.retained_model = (uint32)((uint32)(sint32)(sint8)val);
                val = (uint32)randPSX();
                session_car_choices.cars[0].primary_color[0] = (uint8)(val & 3);
                val = (uint32)randPSX();
                session_car_choices.cars[0].secondary_color[0] = (uint8)(val & 3);
                session_car_choices.cars[0].tuning[0][0] = (uint16)(0);
                session_car_choices.cars[0].tuning[0][1] = (uint16)(0);
                session_car_choices.cars[0].tuning[0][2] = (uint16)(0);
                session_car_choices.cars[0].tuning[0][3] = (uint16)(0);
                session_flow.phase = (4);
                presentation_menu.finished = (1);
                audio_stop_xa();
                volume = audio_options.music_volume;
                audio_play_request(13, 20, (sint32)volume, 0, 5120);
            }
        }
    }
}
