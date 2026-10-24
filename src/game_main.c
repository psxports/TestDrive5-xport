#include "td5_functions.h"
#include <stdio.h>

// TODO: Enable storage after migrating the card UI to xport_save/load
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
    static const PSX_EXE executable = {
        "DATA/SLUS_006.10", 0x80010000, 0x80088170, 0x800A50B8,
        0x800861D4, 0x800A50B8, 0x752F40};

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
    InitHeap(r_u32(0x800861D4) + 4, r_u32(0x800861D8));
    if (!td5_mount_xa_file(r_u32(0x80087BE4), &initial_xa))
    {
        xport_message_error("Test Drive 5", "Cannot open the initial DATA XA stream");
        xport_set_exit_code(1);
        return;
    }
#ifdef _DEBUG
    printf("td5_boot bss_first=%08X bss_last=%08X heap_start=%08X heap_size=%08X bios_heap=%08X bios_end=%08X\n",
        r_u32(0x80088170), r_u32(0x800A50B4), r_u32(0x800861D4),
        r_u32(0x800861D8), r_u32(0xA0009000), r_u32(0xA0009008));
#endif
    /* Entry carriers from the reviewed BIOS boot capture at 80015E38 */
    xport_set_exit_code(td5_main(0u, 0x807FFFF8u, 0u, 0u, 0u,
        0xA000B870u, 0u));
}
