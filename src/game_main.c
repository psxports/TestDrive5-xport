#include "td5_functions.h"
#include "psx_gpu.h"
#include <stdio.h>

#if !defined(LOCKSTEP_DEBUG)
#if AP_WEB
#include <emscripten.h>

EM_JS(sint32, td5_random_seed, (uint32 *seed), {
    try {
        const words = new Uint32Array(1);
        globalThis.crypto.getRandomValues(words);
        HEAPU32[seed >> 2] = words[0];
        return 1;
    } catch (error) {
        return 0;
    }
});
#else
#include <windows.h>
#include <bcrypt.h>

static sint32 td5_random_seed(uint32 *seed)
{
    return BCryptGenRandom(NULL, (PUCHAR)seed, sizeof(*seed), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
}
#endif
#endif

// TODO: Migrate the remaining card manager UI to native storage
const uint32 td5_memory_card_enabled = 0u;

/* Zero selects the existing native GPU owner */
const uint32 xport_gpu_graph_type_address = 0u;

/* Unresolved CD bindings until original storage is audited */
const uint32 xport_cd_sync_callback_address = 0x8007FCC8;
const uint32 xport_cd_ready_callback_address = 0u;
const uint32 xport_cd_status_address = 0u;
const uint32 xport_cd_setloc_table_address = 0u;

void xport_main(void)
{
    CdlFILE initial_xa;
#if !defined(LOCKSTEP_DEBUG)
    uint32 random_seed;
#endif
    static const PSX_EXE executable = {
        "DATA/SLUS_006.10", 0x80010000, 0x80088170, 0x800A50B8,
        0x800861D4, 0x800A50B8, 0x752F40};

    gpu_set_frame_skip(0);
    if (!xport_window_init())
    {
        xport_message_error("Test Drive 5", "Cannot initialize the game window");
        xport_set_exit_code(1);
        return;
    }
    if (!xport_psx_exe_load(&executable))
    {
        xport_message_error("Test Drive 5", "Cannot load DATA/SLUS_006.10");
        xport_set_exit_code(1);
        return;
    }
#if !defined(LOCKSTEP_DEBUG)
    if (!td5_random_seed(&random_seed))
    {
        xport_message_error("Test Drive 5", "Cannot initialize the random seed");
        xport_set_exit_code(1);
        return;
    }
    srandPSX(random_seed);
    /* Randomize the 13-song native and eight-track Web playlist starts */
    w_u32(0x8008762C, random_seed % (13 * 8));
#ifdef _DEBUG
    printf("td5_random_seed=%08X\n", random_seed);
#endif
#endif
    InitHeap(r_u32(0x800861D4) + 4, r_u32(0x800861D8));
#if AP_WEB
    SpuInit();
#else
    if (!td5_mount_xa_file(r_u32(0x80087BE4), &initial_xa))
    {
        xport_message_error("Test Drive 5", "Cannot open the initial DATA XA stream");
        xport_set_exit_code(1);
        return;
    }
#endif
#ifdef _DEBUG
    printf("td5_boot bss_first=%08X bss_last=%08X heap_start=%08X heap_size=%08X bios_heap=%08X bios_end=%08X\n",
        r_u32(0x80088170), r_u32(0x800A50B4), r_u32(0x800861D4),
        r_u32(0x800861D8), r_u32(0xA0009000), r_u32(0xA0009008));
#endif
    /* Entry carriers from the reviewed BIOS boot capture at 80015E38 */
    xport_set_exit_code(td5_main(0u, 0x807FFFF8u, 0u, 0u, 0u,
        0xA000B870u, 0u));
}
