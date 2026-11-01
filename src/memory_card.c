#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "game.h"

typedef union MEMORY_CARD_FILE_HEADER
{
    uint32 words[64];
    uint8 bytes[256];
} MEMORY_CARD_FILE_HEADER;

typedef struct MEMORY_CARD_BLOCK
{
    MEMORY_CARD_FILE_HEADER header;
    char filename[22];
    char short_label[10];
    sint16 status;
    uint16 head_block;
} MEMORY_CARD_BLOCK;

typedef struct MEMORY_CARD_SLOT
{
    MEMORY_CARD_BLOCK blocks[MEMORY_CARD_BLOCK_COUNT];
    uint32 scan_state;
    uint32 present;
    uint32 formatted;
    uint32 text_updated;
} MEMORY_CARD_SLOT;

/* Keep the unsupported physical-card UI disabled */
const uint32 memory_card_enabled = 0u;

/* Existing SDK entry used by the disabled physical-card path */
uint32 _bu_init(void);

MEMORY_CARD_MENU card_menu;
static MEMORY_CARD_SLOT slots[MEMORY_CARD_SLOT_COUNT];
typedef char MEMORY_CARD_FILE_HEADER_SIZE_CHECK[sizeof(MEMORY_CARD_FILE_HEADER) == 256 ? 1 : -1];

static uint32 memory_card_events[8];

static uint32 scan_slot(uint32 slot);
static uint32 is_full(uint32 slot);
static uint32 reset_slot(uint32 slot);

sint32 memory_card_init(void)
{
    static const sint32 specifications[4] = {4, 0x8000, 0x100, 0x2000};
    uint32 idx;
    uint32 event_class;
    sint32 handle;
    sint32 result;

    FUNCTION_MARKER(0x80055B3C, "SLUS_006.10");
    if (!memory_card_enabled)
    {
        input_init();
        return 0;
    }
    for (idx = 0; idx < 15; ++idx)
        VSync(0);
    for (idx = 0; idx < 8; ++idx)
    {
        event_class = idx < 4 ? 0xF4000001 : 0xF0000011;
        handle = OpenEvent(event_class, specifications[idx & 3], 0x2000, 0);
        memory_card_events[idx] = (uint32)handle;
    }
    InitCARD(0);
    for (idx = 0; idx < 15; ++idx)
        VSync(0);
    StartCARD();
    for (idx = 0; idx < 16; ++idx)
        VSync(0);
    _bu_init();
    for (idx = 0; idx < 16; ++idx)
        VSync(0);
    input_init();
    for (idx = 0; idx < 11; ++idx)
        VSync(0);
    result = 0;
    for (idx = 0; idx < 8; ++idx)
        result = EnableEvent((sint32)memory_card_events[idx]);
    return result;
}

static uint32 request_info(uint32 slot)
{
    uint32 idx;

    FUNCTION_MARKER(0x80056228, "SLUS_006.10");
    _card_info((sint32)(slot << 4));
    for (;;)
        for (idx = 0; idx < 4; ++idx)
            if (TestEvent((sint32)memory_card_events[idx]) == 1)
                return idx;
}

static uint32 memory_card_load(uint32 slot)
{
    uint32 idx;

    FUNCTION_MARKER(0x800561CC, "SLUS_006.10");
    for (idx = 0; idx < 4; ++idx)
        TestEvent((sint32)memory_card_events[idx]);
    return psx_bios_card_load_guest(slot << 4);
}

static uint32 wait_software_completion(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x80056158, "SLUS_006.10");
    for (;;)
        for (idx = 0; idx < 4; ++idx)
            if (TestEvent((sint32)memory_card_events[idx]) == 1)
                return idx;
}

static uint32 wait_hardware_event(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x80056088, "SLUS_006.10");
    for (;;)
        for (idx = 0; idx < 4; ++idx)
            if (TestEvent((sint32)memory_card_events[4 + idx]) == 1)
                return idx;
}

static uint32 consume_hardware_events(uint32 event)
{
    FUNCTION_MARKER(0x80055F88, "SLUS_006.10");
    TestEvent((sint32)event);
    TestEvent((sint32)memory_card_events[5]);
    TestEvent((sint32)memory_card_events[6]);
    return (uint32)TestEvent((sint32)memory_card_events[7]);
}

uint32 memory_card_shutdown(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x80055EFC, "SLUS_006.10");
    for (idx = 0; idx < 8; ++idx)
    {
        if ((sint32)memory_card_events[idx] > 0)
            CloseEvent((sint32)memory_card_events[idx]);
        memory_card_events[idx] = 0;
    }
    PadStopCom();
    return xport_bios_stop_card();
}

static uint32 wait_events(const uint32 events[4])
{
    uint32 idx;

    for (;;)
        for (idx = 0; idx < 4; ++idx)
            if (TestEvent((sint32)events[idx]) == 1)
                return idx;
}

static uint32 menu_update_slot_text(void)
{
    char *dst;
    const char *text;
    uint32 slot, result = 0;

    FUNCTION_MARKER(0x800BA084, "SLUS_006.10");
    for (slot = 0; slot < 2; ++slot)
    {
        slots[slot].text_updated = 1;
        if (slots[slot].present == 0)
            text = presentation_carsel_text.card_src_no_memory_card;
        else if (slots[slot].formatted == 0)
            text = presentation_carsel_text.card_src_unformatted_memory_card;
        else if (is_full(slot) == 1)
            text = presentation_carsel_text.card_src_memory_card_is_full;
        else
            text = presentation_carsel_text.card_src_memory_card_is_ok;
        dst = presentation_items[7 + slot].content.text;
        strcpy(dst, text);
        result = (uint32)strlen(text);
    }
    return result;
}

uint32 card_menu_begin(uint32 mode)
{
    uint32 result;

    FUNCTION_MARKER(0x800B7F0C, "SLUS_006.10");
    menu_choices.panel_selection = (0);
    card_menu.state = (0);
    card_menu.state_frames = (0);
    card_menu.parent_mode = (mode);
    card_menu.message_frames = (0);
    presentation_carsel_recs.card[0].x_flags = (uint16)(presentation_carsel_recs.card[0].x_flags & 0xFFF);
    presentation_items[0].x_flags = (presentation_items[0].x_flags & 0xFFF);
    presentation_carsel_recs.card[1].x_flags = (uint16)(presentation_carsel_recs.card[1].x_flags & 0xFFF);
    presentation_items[1].x_flags = (presentation_items[1].x_flags & 0xFFF);
    presentation_carsel_recs.card[2].x_flags = (uint16)(presentation_carsel_recs.card[2].x_flags & 0xFFF);
    presentation_items[2].x_flags = (presentation_items[2].x_flags & 0xFFF);
    if (session_flow.load_requested != 0)
    {
        presentation_items[1].content.text = "RACE";
        presentation_carsel_recs.card[2].x_flags = (uint16)(presentation_carsel_recs.card[2].x_flags | 0x4000);
        presentation_items[2].x_flags = (presentation_items[2].x_flags | 0x4000);
    }
    if (card_menu.parent_mode == 1)
    {
        presentation_carsel_recs.card[0].x_flags = (uint16)(presentation_carsel_recs.card[0].x_flags | 0x4000);
        presentation_items[0].x_flags = (presentation_items[0].x_flags | 0x4000);
        menu_choices.panel_selection = (1);
        return 1;
    }
    presentation_carsel_recs.card[0].x_flags = (uint16)(presentation_carsel_recs.card[0].x_flags & 0xFFF);
    result = presentation_items[0].x_flags;
    presentation_items[0].x_flags = (result & 0xFFF);
    return result;
}

static uint32 prepare_icon_tex(uint32 first, uint32 count)
{
    char icon_name[22];
    uint32 first_slot, slot_count, slot_idx, icon_block;
    uint32 src_x, src_y, src_pal_x, src_pal_y;
    uint32 dst_x, dst_y, dst_pal_x, dst_pal_y;
    const uint8 *tex, *template_tex, *icon_tex;
    PSX_RECT rect;
    uint32 val, page, coord, end, idx, block;

    FUNCTION_MARKER(0x800BA890, "SLUS_006.10");
    first_slot = (first);
    slot_count = (count);
    template_tex = tex_find("Square1");
    tex = template_tex;
    page = xport_load_le16(tex + 8);
    coord = (page << 6) & 0x3FF;
    tex = template_tex;
    val = tex[14];
    src_x = (coord + ((val >> 2) & 255));
    tex = template_tex;
    page = xport_load_le16(tex + 8);
    coord = (page << 4) & 0x100;
    tex = template_tex;
    page = xport_load_le16(tex + 8);
    coord += (page >> 2) & 0x200;
    tex = template_tex;
    val = tex[15];
    src_y = (coord + val);
    tex = template_tex;
    val = xport_load_le16(tex + 10);
    src_pal_x = ((val & 63) << 4);
    tex = template_tex;
    val = xport_load_le16(tex + 10);
    src_pal_y = ((val >> 6) & 255);
    slot_idx = (first_slot);
    for (;;)
    {
        val = first_slot;
        count = slot_count;
        end = val + count;
        idx = slot_idx;
        if ((sint32)idx >= (sint32)end)
            break;
        icon_block = (1);
        for (;;)
        {
            block = icon_block;
            if ((sint32)block >= 16)
                break;
            idx = slot_idx;
            block = icon_block;
            snprintf(icon_name, sizeof(icon_name), "sq%d%x", (sint32)idx, block);
            icon_tex = tex_find(icon_name);
            tex = icon_tex;
            page = xport_load_le16(tex + 8);
            coord = (page << 6) & 0x3FF;
            tex = icon_tex;
            val = tex[14];
            dst_x = (coord + ((val >> 2) & 255));
            tex = icon_tex;
            page = xport_load_le16(tex + 8);
            coord = (page << 4) & 0x100;
            tex = icon_tex;
            page = xport_load_le16(tex + 8);
            coord += (page >> 2) & 0x200;
            tex = icon_tex;
            val = tex[15];
            dst_y = (coord + val);
            tex = icon_tex;
            val = xport_load_le16(tex + 10);
            dst_pal_x = ((val & 63) << 4);
            tex = icon_tex;
            val = xport_load_le16(tex + 10);
            dst_pal_y = ((val >> 6) & 255);
            rect.x = (sint16)((uint16)src_pal_x);
            rect.y = (sint16)((uint16)src_pal_y);
            rect.w = (sint16)(16);
            rect.h = (sint16)(1);
            val = dst_pal_x;
            coord = dst_pal_y;
            MoveImage(&rect, (sint32)val, (sint32)coord);
            rect.x = (sint16)((uint16)src_x);
            rect.y = (sint16)((uint16)src_y);
            rect.w = (sint16)(4);
            rect.h = (sint16)(16);
            val = dst_x;
            coord = dst_y;
            MoveImage(&rect, (sint32)val, (sint32)coord);
            icon_block = (icon_block + 1);
        }
        slot_idx = (slot_idx + 1);
    }
    return (uint32)DrawSync(0);
}

static uint32 poll_slot(uint32 slot)
{
    uint32 status_val, wait_result;
    uint32 status, result, offset;

    FUNCTION_MARKER(0x800BA234, "SLUS_006.10");
    XASSERT(!(slot >= MEMORY_CARD_SLOT_COUNT));
    if ((input_ports[0].type & 255) == 255)
    {
        input_poll();
        VSync(0);
        VSync(0);
        input_poll();
        status_val = (request_info(slot));
        VSync(0);
        VSync(0);
        status_val = (request_info(slot));
        VSync(0);
        VSync(0);
        VSync(0);
        input_poll();
        status_val = (request_info(slot));
        VSync(0);
        VSync(0);
        input_poll();
        status_val = (request_info(slot));
        VSync(0);
        VSync(0);
    }
    status_val = (request_info(slot));
    status = status_val;
    switch (status)
    {
        case 1:
            offset = slot;
            slots[offset].present = (0);
            offset = slot;
            slots[offset].formatted = (0);
            return slot * 4396;
        case 2:
            reset_slot(slot);
            offset = slot;
            slots[offset].present = (0);
            prepare_icon_tex(slot, 1);
            return menu_update_slot_text();
        case 3:
            consume_hardware_events(memory_card_events[4]);
            _card_clear(slot << 4);
            wait_result = (wait_hardware_event());
            prepare_icon_tex(slot, 1);
            memory_card_load(slot);
            wait_result = (wait_software_completion());
            prepare_icon_tex(slot, 1);
            result = wait_result;
            if (result == 0)
            {
                scan_slot(slot);
                offset = slot;
                slots[offset].present = (1);
                offset = slot;
                slots[offset].formatted = (1);
            }
            else if (result == 1 || result == 2)
            {
                offset = slot;
                slots[offset].present = (0);
                offset = slot;
                slots[offset].formatted = (0);
            }
            else if (result == 3)
            {
                offset = slot;
                slots[offset].present = (1);
                offset = slot;
                slots[offset].formatted = (0);
            }
            return menu_update_slot_text();
        default:
            return status;
    }
}

static uint32 create_save_file(uint32 slot)
{
    char path[sizeof("bu%d0:BASLUS-00610") + 9];
    sint32 desc;

    FUNCTION_MARKER(0x80056CFC, "SLUS_006.10");
    snprintf(path, sizeof(path), "bu%d0:BASLUS-00610", (sint32)slot);
    desc = openPSX(path, 0x10200);
    if (desc < 0)
        return 0;
    closePSX(desc);
    return 1;
}

static uint32 format_medium(uint32 slot)
{
    uint32 idx, status, result;
    char path[sizeof("bu%d0:") + 9];

    FUNCTION_MARKER(0x8005712C, "SLUS_006.10");
    _card_info((sint32)(slot << 4));
    status = wait_events(memory_card_events);
    (void)(status);
    if (status != 0)
    {
        if (status < 3)
            return 0;
        for (idx = 0; idx < 4; ++idx)
            TestEvent((sint32)memory_card_events[4 + idx]);
        _card_clear(slot << 4);
        wait_events(memory_card_events + 4);
        status = wait_events(memory_card_events);
        if (status < 3)
            return 0;
    }
    snprintf(path, sizeof(path), "bu%d0:", (sint32)slot);
    result = format(path) != 0;
    return result;
}

static uint32 delete_file(const char *filename, sint32 slot)
{
    const char *format_string;
    char *path;
    sint32 bytes;
    uint32 result;

    FUNCTION_MARKER(0x800562A4, "SLUS_006.10");
    format_string = "bu%d0:%s";
    bytes = snprintf(0, 0, format_string, slot, filename);
    XASSERT(!(bytes < 0));
    path = (char *)malloc((size_t)bytes + 1);
    XASSERT(!(path == NULL));
    sprintf(path, format_string, slot, filename);
    result = erase(path) != 0;
    free(path);
    return result;
}

static uint32 read_icon(const char *path, uint32 slot, uint32 block)
{
    MEMORY_CARD_BLOCK *rec;
    sint32 desc;
    const uint8 *tex;
    uint32 page, u, v, clut;
    PSX_RECT rect;
    char name[32];

    FUNCTION_MARKER(0x800563C8, "SLUS_006.10");
    XASSERT(!(slot >= MEMORY_CARD_SLOT_COUNT || block >= MEMORY_CARD_BLOCK_COUNT));
    rec = &slots[slot].blocks[block];
    desc = openPSX(path, 1);
    if (desc < 0)
        return 0;
    if (readPSX(desc, rec->header.bytes, 256) != 256)
    {
        closePSX(desc);
        return 0;
    }
    closePSX(desc);
    if (rec->header.bytes[0] != 'S' || rec->header.bytes[1] != 'C')
    {
        memcpy(rec->header.bytes + 4, "Empty.", 7);
        rec->status = 0;
        rec->filename[0] = 0;
        rec->short_label[0] = 0;
        return 0;
    }
    sprintf(name, "sq%d%x", (sint32)slot, block + 1);
    tex = tex_find(name);
    page = xport_load_le16(tex + 8);
    u = tex[14];
    v = tex[15];
    clut = xport_load_le16(tex + 10);
    rect.x = (sint16)((clut & 63) << 4);
    rect.y = (sint16)((clut >> 6) & 255);
    rect.w = 16;
    rect.h = 1;
    LoadImagePSX(&rect, rec->header.words + 24);
    DrawSync(0);
    rect.x = (sint16)(((page << 6) & 1023) + (u >> 2));
    rect.y = (sint16)(((page << 4) & 256) + ((page >> 2) & 512) + v);
    rect.w = 4;
    rect.h = 16;
    LoadImagePSX(&rect, rec->header.words + 32);
    DrawSync(0);
    return 1;
}

static uint32 scan_slot(uint32 slot)
{
    uint32 wire_word_1;
    uint32 block = 0, idx, name_len, head;
    sint32 bytes, blocks, continuation;
    uint8 directory[40];
    char path[40];
    void *entry;
    MEMORY_CARD_BLOCK *rec;

    FUNCTION_MARKER(0x80056640, "SLUS_006.10");
    XASSERT(!(slot >= MEMORY_CARD_SLOT_COUNT));
    for (idx = 0; idx < MEMORY_CARD_BLOCK_COUNT; ++idx)
    {
        rec = &slots[slot].blocks[idx];
        rec->filename[0] = 0;
        rec->short_label[0] = 0;
        rec->status = 0;
        rec->head_block = (uint16)idx;
    }
    entry = firstfile(slot == 0 ? "bu00:*" : "bu10:*", directory);
    while (entry != 0)
    {
        if (block >= MEMORY_CARD_BLOCK_COUNT)
            break;
        for (name_len = 0; name_len < 20 && directory[name_len] != 0; ++name_len)
        {
        }
        snprintf(path, sizeof(path), "bu%d0:%.*s", (sint32)slot, (int)name_len, (const char *)directory);
        if (strncmp((const char *)directory, "BASLUS-00610", 12) == 0)
        {
            read_icon(path, slot, block);
            rec = &slots[slot].blocks[block];
            for (idx = 0; idx < 8; ++idx)
                rec->short_label[idx] = (char)directory[12 + idx];
            rec->short_label[8] = 0;
            memcpy(rec->filename, directory, name_len);
            rec->filename[name_len] = 0;
            rec->head_block = (uint16)block;
            rec->status = 1;
            ++block;
        }
        else
        {
            bytes = (sint32)(memcpy(&wire_word_1, directory + 24, sizeof(wire_word_1)), wire_word_1);
            blocks = bytes / 8192;
            XASSERT(!(bytes < 0 || (uint32)blocks > MEMORY_CARD_BLOCK_COUNT - block));
            read_icon(path, slot, block);
            rec = &slots[slot].blocks[block];
            rec->status = 2;
            rec->head_block = (uint16)block;
            memcpy(rec->filename, directory, name_len);
            rec->filename[name_len] = 0;
            memcpy(rec->short_label, "USED", 5);
            head = block;
            ++block;
            for (continuation = 1; continuation < blocks; ++continuation)
            {
                read_icon(path, slot, block);
                rec = &slots[slot].blocks[block];
                rec->status = 2;
                rec->head_block = (uint16)head;
                ++block;
            }
        }
        entry = nextfile(directory);
    }
    return 0;
}

static uint32 scan_slots(sint32 first, sint32 count)
{
    uint32 cur = first;
    sint32 end = ((uint32)first + (uint32)count);
    uint32 channel;
    uint32 offset;
    sint32 status;

    FUNCTION_MARKER(0x800569E4, "SLUS_006.10");
    while ((sint32)(sint16)cur < end)
    {
        channel = (uint32)(sint32)(sint16)cur << 4;
        _card_info((sint32)channel);
        for (;;)
        {
            status = 0;
            if (TestEvent((sint32)memory_card_events[0]) == 1)
                break;
            status = 1;
            if (TestEvent((sint32)memory_card_events[1]) == 1)
                break;
            status = 2;
            if (TestEvent((sint32)memory_card_events[2]) == 1)
                break;
            status = 3;
            if (TestEvent((sint32)memory_card_events[3]) == 1)
                break;
        }
        (void)(status);
        if (status == 3)
        {
            TestEvent((sint32)memory_card_events[4]);
            TestEvent((sint32)memory_card_events[5]);
            TestEvent((sint32)memory_card_events[6]);
            TestEvent((sint32)memory_card_events[7]);
            _card_clear(channel);
            while (TestEvent((sint32)memory_card_events[4]) != 1 && TestEvent((sint32)memory_card_events[5]) != 1 && TestEvent((sint32)memory_card_events[6]) != 1 && TestEvent((sint32)memory_card_events[7]) != 1)
            {
            }
        }
        TestEvent((sint32)memory_card_events[0]);
        TestEvent((sint32)memory_card_events[1]);
        TestEvent((sint32)memory_card_events[2]);
        TestEvent((sint32)memory_card_events[3]);
        psx_bios_card_load_guest(channel);
        for (;;)
        {
            status = 0;
            if (TestEvent((sint32)memory_card_events[0]) == 1)
                break;
            status = 1;
            if (TestEvent((sint32)memory_card_events[1]) == 1)
                break;
            status = 2;
            if (TestEvent((sint32)memory_card_events[2]) == 1)
                break;
            status = 3;
            if (TestEvent((sint32)memory_card_events[3]) == 1)
                break;
        }
        offset = (sint16)cur;
        XASSERT(!(offset >= MEMORY_CARD_SLOT_COUNT));
        if (status == 0 || status == 3)
        {
            slots[offset].present = (1);
            slots[offset].formatted = (status == 0 ? 1 : 0);
        }
        slots[offset].scan_state = (0);
        ++cur;
    }
    scan_slot(0);
    return scan_slot(1);
}

static uint32 is_full(uint32 slot)
{
    uint32 idx;

    FUNCTION_MARKER(0x800560FC, "SLUS_006.10");
    XASSERT(!(slot >= MEMORY_CARD_SLOT_COUNT));
    for (idx = 0; idx < MEMORY_CARD_BLOCK_COUNT; ++idx)
        if (slots[slot].blocks[idx].status == 0)
            return 0;
    return 1;
}

static uint32 reset_slot(uint32 slot)
{
    uint32 idx;
    MEMORY_CARD_BLOCK *rec;

    FUNCTION_MARKER(0x800562DC, "SLUS_006.10");
    XASSERT(!(slot >= MEMORY_CARD_SLOT_COUNT));
    for (idx = 0; idx < MEMORY_CARD_BLOCK_COUNT; ++idx)
    {
        rec = &slots[slot].blocks[idx];
        memcpy(rec->header.bytes + 4, "Empty.", 7);
        memcpy(rec->short_label, "Empty.", 7);
        rec->head_block = (uint16)idx;
        rec->filename[0] = 0;
        rec->status = 0;
    }
    return slot * 4396 + 4088;
}

static uint32 menu_slot_flags(uint32 slot)
{
    uint32 flags = 0, idx, count;

    FUNCTION_MARKER(0x800BA700, "SLUS_006.10");
    XASSERT(!(slot >= MEMORY_CARD_SLOT_COUNT));
    if (slots[slot].present == 0)
        flags |= 1;
    if (is_full(slot) == 1)
    {
        count = 0;
        for (idx = 0; idx < MEMORY_CARD_BLOCK_COUNT; ++idx)
            if (slots[slot].blocks[idx].status == 1)
                ++count;
        if (count == 0)
            flags |= 2;
    }
    return flags;
}

static uint32 write_save_file(sint32 slot)
{
    uint32 wire_word_3;
    uint8 *image;
    static const char title[13] = "TESTDRIVE 5 ";
    char filename[32];
    uint32 idx, val, character, page, u, v, clut;
    const uint8 *tex;
    uint32 image_x, image_y, checksum;
    sint32 desc, written;
    PSX_RECT rect;

    FUNCTION_MARKER(0x80056D60, "SLUS_006.10");
    sprintf(filename, "bu%d0:BASLUS-00610", slot);
    image = (uint8 *)calloc(8192, 1);
    XASSERT(!(image == NULL));
    image[0] = (0x53);
    image[1] = (0x43);
    image[2] = (0x11);
    image[3] = (1);
    for (idx = 0; idx < strlen(title); ++idx)
    {
        character = (uint8)title[idx];
        if (character == 32)
        {
            image[idx * 2 + 4] = (0x81);
            val = 0x40;
        }
        else
        {
            image[idx * 2 + 4] = (0x82);
            val = (uint8)title[idx] + 31;
        }
        image[idx * 2 + 5] = (val);
    }
    tex = tex_find("Icon");
    page = xport_load_le16(tex + 8);
    u = tex[14];
    v = tex[15];
    clut = xport_load_le16(tex + 10);
    rect.w = 16;
    rect.h = 1;
    image_x = ((page << 6) & 0x3FF) + (u >> 2);
    image_y = ((page << 4) & 0x100) + ((page >> 2) & 0x200) + v;
    rect.x = (sint16)((clut & 63) << 4);
    rect.y = (sint16)((clut >> 6) & 255);
    StoreImage(&rect, (uint32 *)(image + 0x60));
    DrawSync(0);
    rect.x = (sint16)image_x;
    rect.y = (sint16)image_y;
    rect.w = 4;
    rect.h = 16;
    StoreImage(&rect, (uint32 *)(image + 0x80));
    DrawSync(0);
    save_encode_progress(image + 0x204, 7676);
    for (idx = 0; idx < 7676; ++idx)
    {
        val = image[0x204 + idx];
        memcpy(&checksum, image + 0x200, sizeof(checksum));
        (wire_word_3 = (uint32)(checksum + val), memcpy(image + 0x200, &wire_word_3, sizeof(wire_word_3)));
    }
    desc = openPSX(filename, 2);
    if (desc < 0)
    {
        free(image);
        return 0;
    }
    written = writePSX(desc, image, 8192);
    free(image);
    closePSX(desc);
    return written == 8192;
}

static uint32 read_save_file(sint32 slot)
{
    uint32 wire_word_4;
    uint8 *image;
    char filename[32];
    uint32 checksum = 0, idx, width, height;
    sint32 desc;

    FUNCTION_MARKER(0x80056FF4, "SLUS_006.10");
    sprintf(filename, "bu%d0:BASLUS-00610", slot);
    image = (uint8 *)calloc(8192, 1);
    XASSERT(!(image == NULL));
    desc = openPSX(filename, 1);
    if (desc < 0)
    {
        free(image);
        return 0;
    }
    if (readPSX(desc, image, 8192) != 8192)
        goto failed_read;
    for (idx = 0; idx < 7676; ++idx)
        checksum += image[0x204 + idx];
    if (checksum != (memcpy(&wire_word_4, image + 0x200, sizeof(wire_word_4)), wire_word_4))
        goto failed_read;
    if (save_decode_progress(image + 0x204, 7676) != SAVE_PAYLOAD_BYTES)
        goto failed_read;
    free(image);
    closePSX(desc);
    width = disp_options.scr_x;
    height = disp_options.scr_y;
    disp_state.frames[0].disp.screen.x = (sint16)(width);
    disp_state.frames[1].disp.screen.x = (sint16)(width);
    disp_state.frames[0].disp.screen.y = (sint16)(height);
    disp_state.frames[1].disp.screen.y = (sint16)(height);
    return 1;

failed_read:
    free(image);
    closePSX(desc);
    return 0;
}

static void set_menu_text(char *const *dst_pointer, const char *text)
{
    strcpy(*dst_pointer, text);
}

static void clear_menu_text(void)
{
    set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_empty);
    set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_empty);
    set_menu_text(&presentation_items[9].content.text, presentation_carsel_text.card_src_empty);
    set_menu_text(&presentation_items[10].content.text, presentation_carsel_text.card_src_empty);
}

static uint32 count_saved(uint32 slot, uint32 use_sel)
{
    uint32 idx, sel, count = 0;

    for (idx = 0; idx < MEMORY_CARD_BLOCK_COUNT; ++idx)
    {
        sel = use_sel != 0 ? card_menu.sel_slot : slot;
        XASSERT(!(sel >= MEMORY_CARD_SLOT_COUNT));
        if (slots[sel].blocks[idx].status == 1)
            ++count;
    }
    return count;
}

void card_menu_update(uint32 mode)
{
    uint32 prev_state, flags0, flags1, saved_count;
    const char *filename;
    uint32 state, val, selection, idx, slot, result;

    FUNCTION_MARKER(0x800B80C8, "SLUS_006.10");
    prev_state = (card_menu.state);
    card_menu.parent_mode = (mode);
    poll_slot(0);
    poll_slot(1);
    card_menu.state_frames = (card_menu.state_frames + 1);
    if (card_menu.state >= 12)
        goto finished;
    state = card_menu.state;
    switch (state)
    {
        case 0:
            set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_reading_memory_cards);
            set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_please_wait);
            if (card_menu.state_frames == 3)
            {
                reset_slot(0);
                reset_slot(1);
                prepare_icon_tex(0, 2);
                scan_slots(0, 2);
                menu_update_slot_text();
                card_menu.state = (2);
                set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_empty);
                set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_empty);
            }
            break;
        case 2:
            set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_empty);
            if (session_flow.load_requested == 0)
            {
                val = menu_choices.panel_selection;
                menu_choices.panel_selection = ((uint32)presentation_move_menu_selection((sint32)val, 3));
                selection = menu_choices.panel_selection;
                switch (selection)
                {
                    case 0:
                        if ((input_ports[0].pressed & 0x4008) != 0)
                        {
                            audio_play_menu_accept();
                            card_menu.sel_slot = (0);
                            card_menu.state = (6);
                            card_menu.next_state = (8);
                            card_menu.return_state = (2);
                            presentation_menu.sel = (0xFFFFFFFF);
                        }
                        set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_load_test_drive_5_block);
                        break;
                    case 1:
                        if ((input_ports[0].pressed & 0x4008) != 0)
                        {
                            audio_play_menu_accept();
                            card_menu.sel_slot = (0);
                            card_menu.overwrite_prompted = (0);
                            card_menu.state = (6);
                            card_menu.next_state = (7);
                            card_menu.return_state = (2);
                            presentation_menu.sel = (0xFFFFFFFF);
                        }
                        set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_save_test_drive_5_block);
                        break;
                    case 2:
                        if ((input_ports[0].pressed & 0x4008) != 0)
                        {
                            audio_play_menu_accept();
                            card_menu.confirmation = (1);
                            card_menu.sel_slot = (0);
                            card_menu.state = (1);
                            card_menu.next_state = (3);
                            card_menu.return_state = (2);
                            presentation_menu.sel = (0xFFFFFFFF);
                        }
                        set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_delete_block);
                        break;
                }
            }
            else
            {
                val = menu_choices.panel_selection;
                menu_choices.panel_selection = ((uint32)presentation_move_menu_selection((sint32)val, 2));
                selection = menu_choices.panel_selection;
                if (selection == 0)
                {
                    set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_load_test_drive_5_block);
                    if ((input_ports[0].pressed & 0x4008) != 0)
                    {
                        audio_play_menu_accept();
                        card_menu.sel_slot = (0);
                        card_menu.state = (6);
                        card_menu.next_state = (8);
                        card_menu.return_state = (2);
                        presentation_menu.sel = (0xFFFFFFFF);
                    }
                }
                else if (selection == 1)
                {
                    set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_race);
                    if ((input_ports[0].pressed & 0x4008) != 0)
                    {
                        audio_play_menu_accept();
                        if (session_flow.card_menu_from_event != 0)
                        {
                            session_flow.card_menu_from_event = 0;
                            session_flow.phase = (3);
                            presentation_menu.finished = (1);
                            audio_stop_xa();
                            val = audio_options.music_volume;
                            audio_play_request(13, 20, (sint32)val, 0, 0x1400);
                        }
                        else
                        {
                            set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_no_cup_race);
                            set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_loaded);
                            card_menu.state = (4);
                            card_menu.next_state = (2);
                        }
                    }
                }
            }
            if ((input_ports[0].pressed & 0x1000) != 0)
            {
                audio_play_menu_accept();
                session_flow.card_menu_from_event = 0;
                session_flow.load_requested = (uint32)(0);
                if (mode == 1)
                    race_menu_select_default();
                else
                    session_pop_menu();
            }
            break;
        case 6:
            flags0 = (menu_slot_flags(0));
            flags1 = (menu_slot_flags(1));
            if (menu_choices.panel_selection == 0)
            {
                saved_count = count_saved(0, 0);
                if (saved_count == 0)
                    flags0 = (1);
                saved_count = count_saved(1, 0);
                if (saved_count == 0)
                    flags1 = (1);
            }
            if (flags0 != 0 && flags1 != 0)
            {
                card_menu.state = (card_menu.return_state);
                break;
            }
            val = menu_choices.panel_selection;
            set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_select_memory_card_to);
            set_menu_text(&presentation_items[4].content.text, val == 0 ? presentation_carsel_text.card_src_load_from : presentation_carsel_text.card_src_save_to);
            if (flags0 != 0)
                card_menu.sel_slot = (1);
            if (flags1 != 0)
                card_menu.sel_slot = (0);
            if ((disp_state.frame_count & 1) == 0)
                presentation_menu.sel = (card_menu.sel_slot + 5);
            else
                presentation_menu.sel = (0xFFFFFFFF);
            if ((input_ports[0].pressed & 0x1000) != 0)
                card_menu.state = (card_menu.return_state);
            if ((input_ports[0].pressed & 0x4008) != 0)
                card_menu.state = (card_menu.next_state);
            if ((input_ports[0].pressed & 0x80) != 0 && (sint32)card_menu.sel_slot > 0 && flags0 == 0)
            {
                card_menu.sel_slot = (card_menu.sel_slot - 1);
                audio_play_menu_move();
            }
            if ((input_ports[0].pressed & 0x20) != 0 && (sint32)card_menu.sel_slot <= 0 && flags1 == 0)
            {
                card_menu.sel_slot = (card_menu.sel_slot + 1);
                audio_play_menu_move();
            }
            break;
        case 7:
            flags0 = (menu_slot_flags(card_menu.sel_slot));
            if (flags0 != 0)
            {
                card_menu.state = (card_menu.return_state);
                break;
            }
            card_menu.confirmation = (1);
            slot = card_menu.sel_slot;
            if (slots[slot].formatted == 0)
            {
                card_menu.confirmation = (1);
                card_menu.state = (9);
                break;
            }
            if (card_menu.overwrite_prompted == 0)
            {
                saved_count = count_saved(0, 1);
                if (saved_count != 0)
                {
                    card_menu.confirmation = (1);
                    card_menu.overwrite_prompted = (1);
                    card_menu.state = (11);
                    break;
                }
            }
            set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_saving_memory_card);
            set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_block);
            if (card_menu.state_frames == 3)
            {
                saved_count = count_saved(0, 1);
                if (saved_count == 0 && create_save_file(card_menu.sel_slot) != 1)
                {
                    prepare_icon_tex(card_menu.sel_slot, 1);
                    scan_slots((sint32)card_menu.sel_slot, 1);
                    menu_update_slot_text();
                    set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_error_creating);
                    set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_memory_card_block);
                    card_menu.state = (4);
                    card_menu.next_state = (card_menu.return_state);
                    break;
                }
                result = write_save_file((sint32)card_menu.sel_slot);
                prepare_icon_tex(card_menu.sel_slot, 1);
                scan_slots((sint32)card_menu.sel_slot, 1);
                menu_update_slot_text();
                set_menu_text(&presentation_items[3].content.text, result == 1 ? presentation_carsel_text.card_src_block_saved_ok : presentation_carsel_text.card_src_error_writing_block);
                set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_empty);
                card_menu.state = (4);
                card_menu.next_state = (card_menu.return_state);
            }
            break;
        case 9:
        case 11:
            presentation_menu.sel = (0xFFFFFFFF);
            if ((disp_state.frame_count & 1) == 0)
                presentation_menu.sel = (card_menu.confirmation + 9);
            set_menu_text(&presentation_items[3].content.text, state == 9 ? presentation_carsel_text.card_src_memory_card_is_unformatted : presentation_carsel_text.card_src_overwrite_existing);
            set_menu_text(&presentation_items[4].content.text, state == 9 ? presentation_carsel_text.card_src_format_it : presentation_carsel_text.card_src_test_drive_block);
            set_menu_text(&presentation_items[9].content.text, presentation_carsel_text.card_src_yes);
            set_menu_text(&presentation_items[10].content.text, presentation_carsel_text.card_src_no);
            if (state == 9)
            {
                slot = card_menu.sel_slot;
                if (slots[slot].formatted != 0)
                {
                    clear_menu_text();
                    card_menu.state = (card_menu.return_state);
                    return;
                }
            }
            if ((input_ports[0].pressed & 0x80) != 0 && (sint32)card_menu.confirmation > 0)
            {
                card_menu.confirmation = (card_menu.confirmation - 1);
                audio_play_menu_move();
            }
            if ((input_ports[0].pressed & 0x20) != 0 && (sint32)card_menu.confirmation <= 0)
            {
                card_menu.confirmation = (card_menu.confirmation + 1);
                audio_play_menu_move();
            }
            if ((input_ports[0].pressed & 0x4008) != 0)
            {
                if (state == 9)
                {
                    if (card_menu.confirmation == 0)
                        card_menu.state = (10);
                    else
                    {
                        set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_empty);
                        set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_empty);
                        card_menu.state = (card_menu.return_state);
                    }
                    set_menu_text(&presentation_items[9].content.text, presentation_carsel_text.card_src_empty);
                    set_menu_text(&presentation_items[10].content.text, presentation_carsel_text.card_src_empty);
                }
                else if (card_menu.confirmation == 0)
                {
                    card_menu.state = (7);
                    clear_menu_text();
                }
                else
                {
                    clear_menu_text();
                    card_menu.state = (card_menu.return_state);
                }
            }
            break;
        case 10:
            card_menu.confirmation = (0);
            set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_formating_memory_card);
            set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_please_wait);
            if (card_menu.state_frames == 3)
            {
                result = format_medium(card_menu.sel_slot);
                prepare_icon_tex(card_menu.sel_slot, 1);
                scan_slots((sint32)card_menu.sel_slot, 1);
                menu_update_slot_text();
                set_menu_text(&presentation_items[3].content.text, result == 1 ? presentation_carsel_text.card_src_memory_card : presentation_carsel_text.card_src_error_formating);
                set_menu_text(&presentation_items[4].content.text, result == 1 ? presentation_carsel_text.card_src_formatted_ok : presentation_carsel_text.card_src_memory_card_2);
                card_menu.state = (4);
                if (result == 1)
                    card_menu.next_state = (7);
                else
                    card_menu.next_state = (card_menu.return_state);
            }
            break;
        case 4:
            card_menu.message_frames = (card_menu.message_frames + 1);
            if ((input_ports[0].pressed & 0x4008) != 0 || (sint32)card_menu.message_frames >= 31)
            {
                card_menu.message_frames = (0);
                clear_menu_text();
                card_menu.state = (card_menu.next_state);
            }
            break;
        case 8:
            set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_loading_memory_card);
            set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_block_2);
            if (card_menu.state_frames == 3)
            {
                result = read_save_file((sint32)card_menu.sel_slot);
                prepare_icon_tex(card_menu.sel_slot, 1);
                scan_slots((sint32)card_menu.sel_slot, 1);
                menu_update_slot_text();
                set_menu_text(&presentation_items[3].content.text, result == 1 ? presentation_carsel_text.card_src_block_loaded_ok : presentation_carsel_text.card_src_error_loading_block);
                set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_empty);
                card_menu.state = (4);
                card_menu.next_state = (card_menu.return_state);
            }
            break;
        case 1:
            flags0 = (menu_slot_flags(0));
            flags1 = (menu_slot_flags(1));
            flags0 = (flags0 & 1);
            flags1 = (flags1 & 1);
            if (flags0 != 0 && flags1 != 0)
                card_menu.state = (card_menu.return_state);
            if (flags0 != 0)
            {
                val = card_menu.grid_column;
                card_menu.grid_column = ((uint32)((sint32)val % 5) + 5);
            }
            if (flags1 != 0)
            {
                val = card_menu.grid_column;
                card_menu.grid_column = ((uint32)((sint32)val % 5));
            }
            if ((disp_state.frame_count & 1) == 0)
                presentation_menu.sel = (0xFFFFFFFF);
            if ((disp_state.frame_count & 1) == 0)
            {
                val = card_menu.grid_column;
                selection = ((sint32)val % 10);
                val = card_menu.grid_row;
                presentation_menu.sel = (selection + val * 10 + 11);
            }
            if ((input_ports[0].pressed & 0x80) != 0 && (sint32)card_menu.grid_column > 0)
            {
                card_menu.grid_column = (card_menu.grid_column - 1);
                audio_play_menu_move();
            }
            if ((input_ports[0].pressed & 0x20) != 0 && (sint32)card_menu.grid_column < 9)
            {
                card_menu.grid_column = (card_menu.grid_column + 1);
                audio_play_menu_move();
            }
            if ((input_ports[0].pressed & 0x10) != 0 && (sint32)card_menu.grid_row > 0)
            {
                card_menu.grid_row = (card_menu.grid_row - 1);
                audio_play_menu_move();
            }
            if ((input_ports[0].pressed & 0x40) != 0 && (sint32)card_menu.grid_row < 2)
            {
                card_menu.grid_row = (card_menu.grid_row + 1);
                audio_play_menu_move();
            }
            val = card_menu.grid_column;
            card_menu.sel_slot = ((uint32)((sint32)val / 5));
            val = card_menu.grid_column;
            selection = ((sint32)val % 5);
            val = card_menu.grid_row;
            card_menu.sel_block = (selection + val * 5);
            if ((input_ports[0].pressed & 0x4008) != 0)
            {
                audio_play_menu_accept();
                idx = card_menu.sel_block;
                slot = card_menu.sel_slot;
                if (slots[slot].blocks[idx].status != 0)
                    card_menu.state = (card_menu.next_state);
                else
                    card_menu.state = (card_menu.return_state);
            }
            if ((input_ports[0].pressed & 0x1000) != 0)
            {
                audio_play_menu_accept();
                card_menu.state = (card_menu.return_state);
            }
            break;
        case 3:
            presentation_menu.sel = (0xFFFFFFFF);
            if ((disp_state.frame_count & 1) == 0)
                presentation_menu.sel = (card_menu.confirmation + 9);
            set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_delete_memory_card);
            set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_block_2);
            set_menu_text(&presentation_items[9].content.text, presentation_carsel_text.card_src_yes);
            set_menu_text(&presentation_items[10].content.text, presentation_carsel_text.card_src_no);
            if ((input_ports[0].pressed & 0x80) != 0 && (sint32)card_menu.confirmation > 0)
            {
                card_menu.confirmation = (card_menu.confirmation - 1);
                audio_play_menu_move();
            }
            if ((input_ports[0].pressed & 0x20) != 0 && (sint32)card_menu.confirmation <= 0)
            {
                card_menu.confirmation = (card_menu.confirmation + 1);
                audio_play_menu_move();
            }
            if ((input_ports[0].pressed & 0x4008) != 0)
            {
                set_menu_text(&presentation_items[9].content.text, presentation_carsel_text.card_src_empty);
                set_menu_text(&presentation_items[10].content.text, presentation_carsel_text.card_src_empty);
                if (card_menu.confirmation == 0)
                {
                    slot = card_menu.sel_slot;
                    idx = card_menu.sel_block;
                    XASSERT(!(slot >= MEMORY_CARD_SLOT_COUNT || idx >= MEMORY_CARD_BLOCK_COUNT));
                    idx = slots[slot].blocks[idx].head_block;
                    XASSERT(!(idx >= MEMORY_CARD_BLOCK_COUNT));
                    filename = slots[slot].blocks[idx].filename;
                    result = delete_file(filename, (sint32)card_menu.sel_slot);
                    prepare_icon_tex(card_menu.sel_slot, 1);
                    scan_slots((sint32)card_menu.sel_slot, 1);
                    menu_update_slot_text();
                    set_menu_text(&presentation_items[3].content.text, result != 0 ? presentation_carsel_text.card_src_block_deleted : presentation_carsel_text.card_src_error_deleting);
                    set_menu_text(&presentation_items[4].content.text, result != 0 ? presentation_carsel_text.card_src_successfully : presentation_carsel_text.card_src_block_2);
                    card_menu.state = (4);
                    card_menu.next_state = (card_menu.return_state);
                }
                else
                {
                    card_menu.state = (card_menu.return_state);
                    set_menu_text(&presentation_items[3].content.text, presentation_carsel_text.card_src_empty);
                    set_menu_text(&presentation_items[4].content.text, presentation_carsel_text.card_src_empty);
                }
            }
            break;
    }

finished:
    val = prev_state;
    selection = card_menu.state;
    if (val != selection)
        card_menu.state_frames = (0);
}
