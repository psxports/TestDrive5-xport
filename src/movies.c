#include <stdlib.h>

#include "movies.h"
#include "game.h"

static void play_sequence(uint32 mode)
{
#if !defined(AP_WEB)
    CdlFILE music;
#endif
#if defined(AP_WEB)
    const char *path = mode ? "DATA/FEAR.STR" : "DATA/INTO.STR";
#else
    const char *path = mode ? "DATA/FEAR.STR" : "DATA/LOGO.STR";
#endif
    sint32 result;

    /* The replaced executable normally restarts callbacks before playback */
    ResetCallback();
    /* Startup movies precede the game's sound initialization */
    {
        sint32 operation_ok = !(!xport_audio_init());
        XASSERT(operation_ok);
    }
    result = xport_fmv_play(path);

#if !defined(AP_WEB)
    if ((result == 1 || result == 2) && mode == 0)
    {
        path = "DATA/INTO.STR";
        result = xport_fmv_play(path);
    }
#endif
    XASSERT(!(result != 1 && result != 2));
#if !AP_WEB
    /* The fullscreen player releases its CD source on return */
    {
        sint32 operation_ok = !(!asset_mount_xa_file(audio_xa_files[0], &music));
        XASSERT(operation_ok);
    }
#endif
}

sint32 movie_play_startup(void)
{
    FUNCTION_MARKER(0x8001ACFC, "SLUS_006.10");
    /* Movie playback no longer loads or executes the PS1 player */
    DrawSync(0);
    psx_root_counter_stop(0xF2000000);
    psx_root_counter_stop(0xF2000001);
    psx_root_counter_stop(0xF2000002);
    psx_root_counter_stop(0xF2000003);
    ResetGraph(3);
    stopIntr();
    EnterCriticalSectionPSX();
    play_sequence(0);
    ResetCallback();
    return CdInit();
}

sint32 movie_play_fear(void)
{
    FUNCTION_MARKER(0x8001AE4C, "SLUS_006.10");
    /* Movie playback no longer loads or executes the PS1 player */
    DrawSync(0);
    memory_card_shutdown();
    psx_root_counter_stop(0xF2000000);
    psx_root_counter_stop(0xF2000001);
    psx_root_counter_stop(0xF2000002);
    psx_root_counter_stop(0xF2000003);
    audio_shutdown();
    ResetGraph(3);
    stopIntr();
    EnterCriticalSectionPSX();
    play_sequence(1);
    ResetCallback();
    CdInit();
    ResetGraph(3);
    SetGraphDebug(0);
    DrawSync(0);
    SetDispMask(0);
    audio_init();
    memory_card_init();
    return (sint32)audio_load_menu_samples(startup_buffer.bytes, sizeof(startup_buffer.bytes));
}
