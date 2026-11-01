#include "game.h"

sint32 timing_reset_counter(sint32 timer)
{
    FUNCTION_MARKER(0x8007AB08, "SLUS_006.10");
    if (timer == 0)
        return ResetRCnt(0xF2000002);
    if (timer == 1)
        return ResetRCnt(0xF2000001);
    return 1;
}

sint32 timing_read_counter(sint32 timer, sint32 prev_result)
{
    FUNCTION_MARKER(0x8007AB50, "SLUS_006.10");
    if (timer == 0)
        return GetRCnt(0xF2000002);
    if (timer == 1)
        return GetRCnt(0xF2000001);
    return prev_result;
}
