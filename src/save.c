#include <stdlib.h>
#include <string.h>

#include "game.h"

typedef struct SAVE_WRITE_CURSOR
{
    uint8 *bytes;
    uint32 capacity;
    uint32 offset;
} SAVE_WRITE_CURSOR;

typedef struct SAVE_READ_CURSOR
{
    const uint8 *bytes;
    uint32 capacity;
    uint32 offset;
} SAVE_READ_CURSOR;

static uint32 write_halves(SAVE_WRITE_CURSOR *cursor, const uint16 *src, uint32 count)
{
    uint32 idx;

    FUNCTION_MARKER(0x800BABC8, "SLUS_006.10");
    XASSERT(!(cursor->offset > cursor->capacity || count > (cursor->capacity - cursor->offset) / 2));
    for (idx = 0; idx < count; ++idx)
        xport_store_le16(cursor->bytes + cursor->offset + idx * 2, src[idx]);
    cursor->offset += count * 2;
    return cursor->offset;
}

static uint32 write_bytes(SAVE_WRITE_CURSOR *cursor, const uint8 *src, uint32 bytes)
{
    FUNCTION_MARKER(0x800BAC70, "SLUS_006.10");
    XASSERT(!(cursor->offset > cursor->capacity || bytes > cursor->capacity - cursor->offset));
    memcpy(cursor->bytes + cursor->offset, src, bytes);
    cursor->offset += bytes;
    return cursor->offset;
}

static uint32 write_words(SAVE_WRITE_CURSOR *cursor, const uint32 *src, uint32 count)
{
    FUNCTION_MARKER(0x800BACF0, "SLUS_006.10");
    XASSERT(!(cursor->offset > cursor->capacity || count > (cursor->capacity - cursor->offset) / 4));
    memcpy(cursor->bytes + cursor->offset, src, count * sizeof(*src));
    cursor->offset += count * 4;
    return cursor->offset;
}

uint32 save_encode_progress(uint8 *dst, uint32 capacity)
{
    SAVE_WRITE_CURSOR cursor = {dst, capacity, 0};

    FUNCTION_MARKER(0x800BAD98, "SLUS_006.10");
    XASSERT(!(capacity < 6303));
    write_halves(&cursor, session_cup_models, 13);
    write_words(&cursor, &input_options.profile[0], 2);
    write_words(&cursor, &input_options.dead_zone[0], 2);
    write_words(&cursor, &input_options.sensitivity[0], 2);
    write_words(&cursor, &input_options.madcatz[0], 2);
    write_words(&cursor, &session_options.retained_model, 1);
    write_words(&cursor, &session_options.retained_track, 1);
    write_words(&cursor, &race_view.split_mode, 1);
    write_words(&cursor, &session_flow.player_name_entered, 1);
    write_words(&cursor, &session_flow.card_menu_from_event, 1);
    write_words(&cursor, &session_options.retained_save_word, 1);
    write_words(&cursor, &menu_choices.main, 1);
    write_words(&cursor, &menu_choices.single_event, 1);
    write_words(&cursor, &menu_choices.legacy_general, 1);
    write_words(&cursor, &menu_choices.car_grp, 1);
    write_words(&cursor, &menu_choices.legacy_vehicle, 1);
    write_words(&cursor, &menu_choices.options_page, 1);
    write_words(&cursor, &menu_choices.settings_row, 1);
    write_words(&cursor, &menu_choices.multiplayer, 1);
    write_words(&cursor, &menu_choices.audio_row, 1);
    write_words(&cursor, &menu_choices.cup, 1);
    write_words(&cursor, &game_options.speed_kph, 1);
    write_words(&cursor, &game_options.traffic, 1);
    write_words(&cursor, &race_rules.traffic, 1);
    write_words(&cursor, &game_options.checkpoint_timers, 1);
    write_words(&cursor, &game_options.cops, 1);
    write_words(&cursor, &game_options.difficulty, 1);
    write_words(&cursor, &game_options.simulation, 1);
    write_words(&cursor, &game_options.collisions_3d, 1);
    write_words(&cursor, &game_options.restrict_primary_models, 1);
    write_words(&cursor, &game_options.vibration, 1);
    write_words(&cursor, &race_session.laps, 1);
    write_words(&cursor, &disp_options.scr_x, 1);
    write_words(&cursor, &disp_options.scr_y, 1);
    write_words(&cursor, &audio_options.music_volume, 1);
    write_words(&cursor, &audio_options.effects_volume, 1);
    write_words(&cursor, &audio_options.paired, 1);
    write_words(&cursor, &session_flow.event_round, 1);
    write_words(&cursor, &session_options.model_variant, 1);
    write_words(&cursor, &session_options.sel_track, 1);
    write_words(&cursor, &race_session.track, 1);
    write_words(&cursor, &session_options.beast_cars, 1);
    write_words(&cursor, &race_session.multiplayer_mode, 1);
    write_words(&cursor, &race_session.event_mode, 1);
    write_words(&cursor, &race_session.cop_chase, 1);
    write_words(&cursor, &race_session.cup, 1);
    write_words(&cursor, &race_session.two_players, 1);
    write_words(&cursor, &race_session.linear_course, 1);
    write_words(&cursor, &race_session.drag_race, 1);
    write_words(&cursor, &race_session.demo, 1);
    write_words(&cursor, &game_options.camera_damping, 1);
    write_words(&cursor, &game_options.circuit_laps, 1);
    write_words(&cursor, &game_options.catchup, 1);
    write_words(&cursor, input_calibration.center[0], 2);
    write_words(&cursor, input_calibration.center[1], 2);
    write_words(&cursor, input_calibration.high[0], 2);
    write_words(&cursor, input_calibration.high[1], 2);
    write_words(&cursor, input_calibration.low[0], 2);
    write_words(&cursor, input_calibration.low[1], 2);
    write_words(&cursor, race_results.words, 30);
    write_words(&cursor, session_car_choices.words, 36);
    write_words(&cursor, rec_table.words, 1410);
    return write_bytes(&cursor, (const uint8 *)&progression_profile, 101);
}

static uint32 read_halves(SAVE_READ_CURSOR *cursor, uint16 *dst, uint32 count)
{
    uint32 idx;

    FUNCTION_MARKER(0x800BB288, "SLUS_006.10");
    XASSERT(!(cursor->offset > cursor->capacity || count > (cursor->capacity - cursor->offset) / 2));
    for (idx = 0; idx < count; ++idx)
        dst[idx] = xport_load_le16(cursor->bytes + cursor->offset + idx * 2);
    cursor->offset += count * 2;
    return cursor->offset;
}

static uint32 read_bytes(SAVE_READ_CURSOR *cursor, uint8 *dst, uint32 bytes)
{
    FUNCTION_MARKER(0x800BB330, "SLUS_006.10");
    XASSERT(!(cursor->offset > cursor->capacity || bytes > cursor->capacity - cursor->offset));
    memcpy(dst, cursor->bytes + cursor->offset, bytes);
    cursor->offset += bytes;
    return cursor->offset;
}

static uint32 read_words(SAVE_READ_CURSOR *cursor, uint32 *dst, uint32 count)
{
    FUNCTION_MARKER(0x800BB3B0, "SLUS_006.10");
    XASSERT(!(cursor->offset > cursor->capacity || count > (cursor->capacity - cursor->offset) / 4));
    memcpy(dst, cursor->bytes + cursor->offset, count * sizeof(*dst));
    cursor->offset += count * 4;
    return cursor->offset;
}

uint32 save_decode_progress(const uint8 *src, uint32 capacity)
{
    uint32 wire_word_3, wire_word_10, wire_word_11;
    SAVE_READ_CURSOR cursor = {src, capacity, 0};
    uint32 port, dead_zone, sensitivity, center, high, low, laps;
    uint32 record, character;
    sint32 volume;
    const uint8 *name;

    FUNCTION_MARKER(0x800BB458, "SLUS_006.10");
    XASSERT(!(capacity < 6303));
    /* Reject invalid input settings before changing any game state */
    for (port = 0; port < 2; ++port)
    {
        if ((memcpy(&wire_word_3, src + 26 + port * 4, sizeof(wire_word_3)), wire_word_3) > 4)
            return 0;
        memcpy(&dead_zone, src + 34 + port * 4, sizeof(dead_zone));
        memcpy(&sensitivity, src + 42 + port * 4, sizeof(sensitivity));
        if (dead_zone > 4096 || (dead_zone & 511) != 0 || sensitivity > 4096 || (sensitivity & 511) != 0)
            return 0;
        memcpy(&center, src + 250 + port * 8, sizeof(center));
        memcpy(&high, src + 266 + port * 8, sizeof(high));
        memcpy(&low, src + 282 + port * 8, sizeof(low));
        if (center < 48 || center > 208 || high > 255 || low > center - 32 || high < center + 32)
            return 0;
    }
    /* Keep imported names within the native string and font domains */
    for (record = 0; record < 47 * 6; ++record)
    {
        name = src + 562 + record * 20 + 5;
        if (name[8] != 0)
            return 0;
        for (character = 0; character < 8 && name[character] != 0; ++character)
        {
            if (name[character] >= 160 && name[character] <= 171)
                return 0;
        }
    }
    /* Reject saved lap counts outside the game-authored range */
    memcpy(&laps, src + 162, sizeof(laps));
    if (laps == 0 || laps > 4 || (memcpy(&wire_word_10, src + 242, sizeof(wire_word_10)), wire_word_10) > 3)
        return 0;
    /* Reject response tuning that would make the gear divisor zero */
    for (port = 0; port < 2; ++port)
    {
        if (xport_load_le16(src + 428 + port * 24) == 0x8000 || xport_load_le16(src + 436 + port * 24) == 0x8000)
            return 0;
    }
    /* Preserve bounded audio settings before publishing saved state */
    for (port = 0; port < 2; ++port)
    {
        volume = (sint32)(memcpy(&wire_word_11, src + 174 + port * 4, sizeof(wire_word_11)), wire_word_11);
        if (volume < -511 || volume > 4607)
            return 0;
    }
    read_halves(&cursor, session_cup_models, 13);
    read_words(&cursor, &input_options.profile[0], 2);
    read_words(&cursor, &input_options.dead_zone[0], 2);
    read_words(&cursor, &input_options.sensitivity[0], 2);
    read_words(&cursor, &input_options.madcatz[0], 2);
    read_words(&cursor, &session_options.retained_model, 1);
    read_words(&cursor, &session_options.retained_track, 1);
    read_words(&cursor, &race_view.split_mode, 1);
    read_words(&cursor, &session_flow.player_name_entered, 1);
    read_words(&cursor, &session_flow.card_menu_from_event, 1);
    read_words(&cursor, &session_options.retained_save_word, 1);
    read_words(&cursor, &menu_choices.main, 1);
    read_words(&cursor, &menu_choices.single_event, 1);
    read_words(&cursor, &menu_choices.legacy_general, 1);
    read_words(&cursor, &menu_choices.car_grp, 1);
    read_words(&cursor, &menu_choices.legacy_vehicle, 1);
    read_words(&cursor, &menu_choices.options_page, 1);
    read_words(&cursor, &menu_choices.settings_row, 1);
    read_words(&cursor, &menu_choices.multiplayer, 1);
    read_words(&cursor, &menu_choices.audio_row, 1);
    read_words(&cursor, &menu_choices.cup, 1);
    read_words(&cursor, &game_options.speed_kph, 1);
    read_words(&cursor, &game_options.traffic, 1);
    read_words(&cursor, &race_rules.traffic, 1);
    read_words(&cursor, &game_options.checkpoint_timers, 1);
    read_words(&cursor, &game_options.cops, 1);
    read_words(&cursor, &game_options.difficulty, 1);
    read_words(&cursor, &game_options.simulation, 1);
    read_words(&cursor, &game_options.collisions_3d, 1);
    read_words(&cursor, &game_options.restrict_primary_models, 1);
    read_words(&cursor, &game_options.vibration, 1);
    read_words(&cursor, &race_session.laps, 1);
    read_words(&cursor, &disp_options.scr_x, 1);
    read_words(&cursor, &disp_options.scr_y, 1);
    read_words(&cursor, &audio_options.music_volume, 1);
    read_words(&cursor, &audio_options.effects_volume, 1);
    read_words(&cursor, &audio_options.paired, 1);
    read_words(&cursor, &session_flow.event_round, 1);
    read_words(&cursor, &session_options.model_variant, 1);
    read_words(&cursor, &session_options.sel_track, 1);
    read_words(&cursor, &race_session.track, 1);
    read_words(&cursor, &session_options.beast_cars, 1);
    read_words(&cursor, &race_session.multiplayer_mode, 1);
    read_words(&cursor, &race_session.event_mode, 1);
    read_words(&cursor, &race_session.cop_chase, 1);
    read_words(&cursor, &race_session.cup, 1);
    read_words(&cursor, &race_session.two_players, 1);
    read_words(&cursor, &race_session.linear_course, 1);
    read_words(&cursor, &race_session.drag_race, 1);
    read_words(&cursor, &race_session.demo, 1);
    read_words(&cursor, &game_options.camera_damping, 1);
    read_words(&cursor, &game_options.circuit_laps, 1);
    read_words(&cursor, &game_options.catchup, 1);
    read_words(&cursor, input_calibration.center[0], 2);
    read_words(&cursor, input_calibration.center[1], 2);
    read_words(&cursor, input_calibration.high[0], 2);
    read_words(&cursor, input_calibration.high[1], 2);
    read_words(&cursor, input_calibration.low[0], 2);
    read_words(&cursor, input_calibration.low[1], 2);
    read_words(&cursor, race_results.words, 30);
    read_words(&cursor, session_car_choices.words, 36);
    read_words(&cursor, rec_table.words, 1410);
    return read_bytes(&cursor, (uint8 *)&progression_profile, 101);
}

sint32 save_write_game(void)
{
    uint8 *image = (uint8 *)malloc(SAVE_PAYLOAD_BYTES);
    sint32 result;

    XASSERT(!(image == NULL));
    save_encode_progress(image, SAVE_PAYLOAD_BYTES);
    result = xport_save(image, SAVE_PAYLOAD_BYTES);
    free(image);
    XASSERT(!(result != 1));
    return result;
}

sint32 save_load_game(void)
{
    uint8 *image = (uint8 *)malloc(SAVE_PAYLOAD_BYTES);
    sint32 result;

    XASSERT(!(image == NULL));
    result = xport_load(image, SAVE_PAYLOAD_BYTES);

    if (result == SAVE_PAYLOAD_BYTES && save_decode_progress(image, SAVE_PAYLOAD_BYTES) != SAVE_PAYLOAD_BYTES)
        result = -1;
    free(image);
    XASSERT(!(result != 0 && result != SAVE_PAYLOAD_BYTES));
    return result == 0 ? 0 : result == SAVE_PAYLOAD_BYTES ? 1 : -1;
}
