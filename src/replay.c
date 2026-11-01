#include "replay.h"
#include "game.h"

typedef struct INPUT_REPLAY
{
    uint32 active;
    uint32 remaining;
    sint32 cursor;
    uint16 *end;
    uint16 samples[3584];
} INPUT_REPLAY;

uint32 replay_mode;
static INPUT_REPLAY replay;

void replay_reset(void)
{
    uint32 state, result;
    sint32 idx;

    FUNCTION_MARKER(0x8004AEF4, "SLUS_006.10");
    state = replay_mode;
    replay.cursor = 0;
    if (state == 0)
    {
        for (idx = 3583; idx >= 0; --idx)
            replay.samples[idx] = 0;
        result = replay_mode;
        if (result != 0)
            replay.cursor -= 1;
    }
    else
        replay.cursor -= 1;
    replay.active = 0;
    replay.remaining = 0;
}

static void rec_input(uint32 input)
{
    sint32 cursor;
    uint32 word, active, count;

    FUNCTION_MARKER(0x8004AF60, "SLUS_006.10");
    cursor = replay.cursor;
    if (cursor != 3583)
    {
        word = replay.samples[cursor];
        active = replay.active;
        count = word & 7;
        if (active != 0 && (input & 0xFFFF) != (word & 0xFFF8))
        {
            replay.cursor = cursor + 1;
            replay.samples[cursor + 1] = (uint16)(input + 1);
        }
        else if (count == 7)
        {
            replay.samples[cursor] = (uint16)(input + 7);
            replay.cursor = cursor + 1;
            replay.samples[cursor + 1] = (uint16)(input + 1);
        }
        else
            replay.samples[cursor] = (uint16)(input + count + 1);
    }
    replay.end = replay.samples + replay.cursor;
}

sint32 replay_exchange_input(uint32 input)
{
    uint32 mode, word, count, val;
    sint32 cursor;

    FUNCTION_MARKER(0x8004ADF8, "SLUS_006.10");
    mode = replay_mode;
    val = input;
    if (mode == 0xFFFFFFFF)
        return (sint16)val;
    if (mode == 0)
        rec_input(input & 0xFFFF);
    else
    {
        cursor = replay.cursor;
        if (cursor == 3583 || replay.end == NULL || cursor >= replay.end - replay.samples)
        {
            val = 0;
            race_control.stop_requested = 1;
        }
        else
        {
            if (replay.remaining == 0)
            {
                word = replay.samples[cursor + 1];
                replay.cursor = cursor + 1;
                replay.remaining = word & 7;
            }
            cursor = replay.cursor;
            count = replay.remaining;
            word = replay.samples[cursor];
            replay.remaining = count - 1;
            val = word & 0xFFF8;
        }
    }
    if (replay.active == 0)
        replay.active = 1;
    return (sint16)val;
}
