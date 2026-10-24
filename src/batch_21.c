#include "td5_functions.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

uint32 td5_800bb948(uint32 invalid_header_index)
{
    uint32 slot, block, count;

    FUNCTION_MARKER(0x800BB948, "SLUS_006.10");
    // TODO: Query native save availability through xport_load
    if (!td5_memory_card_enabled)
        return 0;
    count = 0;
    td5_800562dc(0);
    td5_800562dc(1);
    td5_scan_card_slots(0, 2, invalid_header_index);
    for (slot = 0; slot < 2; ++slot)
    {
        for (block = 0; block < 15; ++block)
        {
            if (r_s16(0x8009FE38 + slot * 4396 + block * 292) == 1)
                ++count;
        }
        if (count != 0)
            return slot + 1;
    }
    return 0;
}

uint32 td5_800bbac8(void)
{
    uint32 block, count, first, second;

    FUNCTION_MARKER(0x800BBAC8, "SLUS_006.10");
    // TODO: Restore the game payload through xport_load
    if (!td5_memory_card_enabled)
        return 0;
    count = 0;
    first = 0;
    second = 0;
    for (block = 0; block < 15; ++block)
    {
        if (r_s16(0x8009FE38 + block * 292) == 1)
            ++count;
    }
    if (count != 0)
        first = 1;
    count = 0;
    for (block = 0; block < 15; ++block)
    {
        if (r_s16(0x800A0F64 + block * 292) == 1)
            ++count;
    }
    if (count != 0)
        second = 1;
    if (first != 0 && td5_80056ff4(0) == 1)
        return 1;
    if (second != 0 && td5_80056ff4(1) == 1)
        return 2;
    return 0;
}

void td5_800adc90(uint32 invalid_header_index)
{
    uint32 ticks, state, value, x, y;

    FUNCTION_MARKER(0x800ADC90, "SLUS_006.10");
    ticks = r_u32(0x800AEA30);
    state = r_u32(0x800B02D0);
    w_u32(0x800AEA30, ticks + 1);
    if (state == 0)
    {
        w_u8(0x800BD340, 1);
        w_u8(0x800BD3B8, 1);
    }
    state = r_u32(0x800B02D0);
    if (state == 1)
    {
        if (r_u32(0x800B02C8) == state)
        {
            w_u8(0x800BD430, 1);
            w_u8(0x800BD4A8, 1);
            w_u8(0x800BD520, 1);
        }
        if (r_u32(0x800B02C8) == 2)
        {
            w_u8(0x800BD430, 1);
            w_u8(0x800BD4A8, 1);
            w_u8(0x800BD598, 1);
        }
        if (r_u32(0x800B02C8) - 1 >= 2)
            w_u32(0x800AEA2C, state);
    }
    if (r_u32(0x800B02D0) == 2)
    {
        x = r_u32(0x800876F8);
        y = r_u32(0x800876FC);
        w_u32(0x800AEA2C, 1);
        w_u16(0x800976CC, x);
        w_u16(0x80097740, x);
        w_u16(0x800976CE, y);
        w_u16(0x80097742, y);
        td5_update_cd_volume();
    }
    if ((r_u32(0x800AEA30) & 15) == 0)
    {
        state = r_u32(0x800B02D0);
        w_u32(0x800AEA30, 0);
        if (state == 1)
        {
            value = td5_800bbac8();
            w_u32(0x800B02C8, value);
            w_u32(0x800B02D0, 2);
        }
        if (r_u32(0x800B02D0) == 0)
        {
            value = td5_800bb948(invalid_header_index);
            w_u32(0x800B02C8, value);
            w_u32(0x800B02D0, 1);
        }
    }
}

static void td5_800ad54c(void)
{
    uint32 index;

    FUNCTION_MARKER(0x800AD54C, "SLUS_006.10");
    for (index = 1; index < 41; ++index)
        w_u8(0x8009DB71 + index, 2);
}

static void td5_800ad578(void)
{
    uint32 index;

    FUNCTION_MARKER(0x800AD578, "SLUS_006.10");
    for (index = 17; index < 37; ++index)
        w_u8(0x8009DB45 + index, 2);
    w_u8(0x8009DB4D, 2);
}

static void td5_800ad5ac(void)
{
    FUNCTION_MARKER(0x800AD5AC, "SLUS_006.10");
    w_u8(0x8009DB3F, 1);
    w_u8(0x8009DB40, 1);
    w_u8(0x8009DB41, 1);
    w_u8(0x8009DB42, 1);
    w_u8(0x8009DB43, 1);
}

static void td5_800ade44(void)
{
    FUNCTION_MARKER(0x800ADE44, "SLUS_006.10");
    w_u8(0x8009DB69, 2);
    w_u8(0x8009DB62, 2);
    w_u8(0x8009DB5C, 2);
}

static void td5_800a6f00(void)
{
    uint32 value;

    FUNCTION_MARKER(0x800A6F00, "SLUS_006.10");
    if (strcmp(td5_menu_string(r_u32(0x8008773C)), td5_menu_string(0x800A52D4)) == 0)
        td5_800ad54c();
    if (strcmp(td5_menu_string(r_u32(0x8008773C)), td5_menu_string(0x800A52DC)) == 0)
        td5_800ad578();
    if (strcmp(td5_menu_string(r_u32(0x8008773C)), td5_menu_string(0x800A52E4)) == 0)
        td5_800ad5ac();
    if (strcmp(td5_menu_string(r_u32(0x8008773C)), td5_menu_string(0x800A52EC)) == 0)
        w_u32(0x800885FC, 1);
    if (strcmp(td5_menu_string(r_u32(0x8008773C)), td5_menu_string(0x800A52F4)) == 0)
        w_u32(0x800877AC, 1);
    if (strcmp(td5_menu_string(r_u32(0x8008773C)), td5_menu_string(0x800A52FC)) == 0)
        td5_800ade44();
    if (strcmp(td5_menu_string(r_u32(0x8008773C)), td5_menu_string(0x800A5304)) == 0)
        w_u32(0x80087714, r_u32(0x80087714) ^ 1);
    if (strcmp(td5_menu_string(r_u32(0x8008773C)), td5_menu_string(0x800A530C)) == 0)
        w_u8(0x8009DB44, 1);
    if (r_u32(0x80087640) == 0)
    {
        value = r_u32(0x800876A0);
        w_u32(0x80087660, 1);
        if (value == 2)
            td5_800a98f0(20);
        else
            td5_800a98f0(10);
    }
    else
        w_u32(0x80088620, 120);
}

void td5_800a70b4(void)
{
    uint32 buttons, name, selected, cursor, pointer, character, column, row;

    FUNCTION_MARKER(0x800A70B4, "SLUS_006.10");
    buttons = r_u16(0x800884E8);
    name = r_u32(0x8008773C);
    if ((buttons & 0x1000) != 0 && r_u32(0x80087640) == 0)
    {
        td5_800a99f0();
        td5_800530f0();
    }
    if ((r_u16(0x800884E8) & 0x10) != 0 && r_s32(0x800AE0BC) > 0)
    {
        td5_80053064();
        w_u32(0x800AE0BC, r_u32(0x800AE0BC) - 1);
    }
    if ((r_u16(0x800884E8) & 0x40) != 0 && r_s32(0x800AE0BC) < 3)
    {
        td5_80053064();
        w_u32(0x800AE0BC, r_u32(0x800AE0BC) + 1);
    }
    if ((r_u16(0x800884E8) & 0x80) != 0 && r_s32(0x800AE0B8) > 0)
    {
        td5_80053064();
        w_u32(0x800AE0B8, r_u32(0x800AE0B8) - 1);
    }
    if ((r_u16(0x800884E8) & 0x20) != 0 && r_s32(0x800AE0B8) < 7)
    {
        td5_80053064();
        w_u32(0x800AE0B8, r_u32(0x800AE0B8) + 1);
    }
    column = r_u32(0x800AE0BC);
    row = r_u32(0x800AE0B8);
    buttons = r_u16(0x800884E8);
    w_u32(0x800887B8, row + column * 8);
    if ((buttons & 0x4008) != 0)
    {
        td5_80053030();
        selected = r_u32(0x800887B8);
        if (selected == 30)
        {
            cursor = r_u32(0x800AE0C0);
            if ((sint32)cursor > 0)
                w_u32(0x800AE0C0, cursor - 1);
        }
        else if (selected == 31)
        {
            cursor = r_u32(0x800AE0C0);
            w_u8(name + cursor, 0);
            td5_800a6f00();
            return;
        }
        else
        {
            if ((sint32)selected < 31 && selected == 29)
            {
                cursor = r_u32(0x800AE0C0);
                w_u8(name + cursor, 32);
            }
            cursor = r_u32(0x800AE0C0);
            if ((sint32)cursor < 8)
                w_u32(0x800AE0C0, cursor + 1);
            else
            {
                if (cursor == 7)
                    w_u32(0x800AE0C0, 8);
                w_u32(0x800AE0B8, 7);
                w_u32(0x800AE0BC, 3);
            }
        }
    }
    selected = r_u32(0x800887B8);
    pointer = r_u32(0x800BCF0C + selected * 120);
    cursor = r_u32(0x800AE0C0);
    character = r_u8(pointer);
    w_u8(name + cursor, character);
    cursor = r_u32(0x800AE0C0);
    w_u8(name + cursor + 1, 0);
    pointer = r_u32(0x8008773C);
    w_u32(0x800BDE0C, pointer);
}

void td5_800a73cc(uint32 original_stack, uint32 archive_error_index)
{
    uint32 index, resource, size, allocation, source, entry, value, unpacked, result;
    uint32 frame = original_stack - 0x28;

    FUNCTION_MARKER(0x800A73CC, "SLUS_006.10");
    w_u32(0x800B0410, 768);
    for (index = 0; index < 11; ++index)
    {
        w_u16(0x800B0430 + index * 2, index * 24);
        value = r_u32(0x8007F60C);
        w_u32(0x800B04E0 + index * 4, value);
    }
    w_u32(0x800B03E0, 0);
    w_u32(0x800B0690, 0);
    td5_800557c0();
    value = r_u32(0x80087700);
    w_u32(frame + 0x10, 5120);
    result = (uint32)td5_8005490c(13, 20, (sint32)value, 0, 5120);
    size = td5_archive_aligned_size(td5_menu_string(0x800A5314), 1, archive_error_index, result);
    allocation = td5_alloc_memory(size);
    w_u32(0x800B03E4, allocation);
    w_u32(0x800B042C, allocation);
    td5_load_archive_file(td5_menu_string(0x800A5314), allocation, archive_error_index);
    resource = r_u32(0x800B03E4);
    if (r_u32(resource) != 0)
    {
        entry = resource + 4;
        index = 1;
        do
        {
            value = r_u32(entry);
            w_u32(entry, value + resource);
            value = r_u32(resource);
            ++index;
            entry += 4;
        } while (index <= value);
    }
    resource = r_u32(0x800B042C);
    source = r_u32(resource + 4);
    unpacked = td5_80015a3c(source);
    resource = r_u32(0x800B042C);
    allocation = r_u32(0x80088608);
    source = r_u32(resource + 4);
    td5_800158d4(source, allocation, (sint32)unpacked);
    value = td5_find_texture(td5_menu_string(0x800A5324));
    w_u32(0x800B0428, value);
}

static void td5_80048f6c(uint32 image, uint32 width, uint32 height)
{
    uint32 block;

    FUNCTION_MARKER(0x80048F6C, "SLUS_006.10");
    if (r_u32(image) != 16)
        return;
    block = image + 8;
    block += r_u32(block) & 0xFFFFFFFC;
    if (r_u32(block) == 4)
        return;
    w_u32(width, (uint32)r_s16(block + 8) << 2);
    w_u32(height, (uint32)r_s16(block + 10));
}

static void td5_80055a34(void)
{
    FUNCTION_MARKER(0x80055A34, "SLUS_006.10");
    td5_play_xa(r_u8(0x800884F4));
}

static void td5_800a754c(uint32 index, uint32 original_stack)
{
    uint32 frame = original_stack - 0x30;
    uint32 table, source, unpacked, allocation, texture, page, clut;
    uint32 image_x, image_y, palette_x, palette_y, value;

    FUNCTION_MARKER(0x800A754C, "SLUS_006.10");
    table = r_u32(0x800B042C);
    source = r_u32(table + index * 4 + 8);
    unpacked = td5_80015a3c(source);
    allocation = td5_alloc_memory(unpacked);
    table = r_u32(0x800B042C);
    source = r_u32(table + index * 4 + 8);
    td5_800158d4(source, allocation, (sint32)unpacked);
    texture = r_u32(0x800B0428);
    page = r_u16(texture + 8);
    value = r_u8(texture + 14);
    image_x = ((page << 6) & 0x3FF) + (value >> 1);
    image_y = ((page << 4) & 0x100) + ((page >> 2) & 0x200);
    value = r_u8(texture + 15);
    clut = r_u16(texture + 10);
    image_y |= value;
    palette_y = (clut >> 6) & 0xFF;
    palette_x = (clut & 0x3F) << 4;
    w_u32(frame + 0x10, palette_y);
    td5_upload_tim(allocation, (sint16)image_x, (sint16)image_y, (sint16)palette_x, (sint16)palette_y);
    td5_80048f6c(allocation, frame + 0x18, frame + 0x1C);
    td5_free_memory(allocation);
    value = r_u32(frame + 0x18);
    texture = r_u32(0x800B0428);
    w_u8(texture + 12, (uint32)((sint32)value / 2));
    texture = r_u32(0x800B0428);
    value = r_u32(frame + 0x1C);
    w_u8(texture + 13, value);
    w_u32(0x800B0410, 256);
}

static void td5_800a78c0(uint32 original_stack, uint32 archive_error_index)
{
    uint32 frame = original_stack - 0x58;
    uint32 value, allocation, x, y;
    sint32 remainder, length;

    FUNCTION_MARKER(0x800A78C0, "SLUS_006.10");
    td5_free_memory(r_u32(0x800B03E4));
    value = r_u32(0x80087634) + 1;
    remainder = (sint32)value % 6;
    w_u32(0x80087634, (uint32)remainder);
    length = sprintfPSX(frame + 0x10, 0x800A532C, (uint32)remainder + 1);
    allocation = r_u32(0x80088608);
    td5_load_archive_file((const char *)psx_addr(frame + 0x10, length + 1), allocation, archive_error_index);
    allocation = r_u32(0x800B03E4);
    td5_8001a1bc(allocation, archive_error_index);
    td5_prepare_display(2);
    x = r_u32(0x800876F8);
    y = r_u32(0x800876FC);
    w_u16(0x800976CC, x);
    w_u16(0x80097740, x);
    w_u16(0x800976CE, y);
    w_u16(0x80097742, y);
    td5_80055a34();
    td5_stop_sound_voice(20);
    td5_800a99f0();
}

static void td5_800a7674(uint32 original_stack, uint32 archive_error_index)
{
    uint32 frame = original_stack - 0x28;
    uint32 index, pointer, value, cursor, text, image_index;
    sint32 y;

    FUNCTION_MARKER(0x800A7674, "SLUS_006.10");
    value = r_u32(0x800B0410);
    w_u16(0x800BCF10, value);
    td5_800b32e4(2);
    for (index = 0; index < 11; ++index)
    {
        pointer = r_u32(0x800B04E0 + index * 4);
        value = r_u8(pointer);
        if (value == 'c' || value == 'd')
        {
            td5_800b32e4(3);
            y = r_s16(0x800B0430 + index * 2);
            pointer = r_u32(0x800B04E0 + index * 4);
            td5_800b2790(130, 256, y, td5_menu_string(pointer));
            td5_800b32e4(2);
        }
        else
        {
            y = r_s16(0x800B0430 + index * 2);
            td5_800b2790(128, 256, y, td5_menu_string(pointer));
        }
    }
    if ((r_u32(0x800B02D4) & 1) != 0)
        return;
    for (index = 0; index < 11; ++index)
    {
        pointer = 0x800B0430 + index * 2;
        y = r_s16(pointer);
        if (y >= -23)
            w_u16(pointer, (uint32)y - 1);
        else
        {
            cursor = r_u32(0x800B03E0);
            w_u16(pointer, 240);
            if (r_u32(0x8007F610 + cursor * 4) != 0)
            {
                value = r_u32(0x8007F60C + cursor * 4);
                w_u32(0x800B04E0 + index * 4, value);
                text = r_u32(0x800B04E0 + index * 4);
                w_u32(0x800B03E0, cursor + 1);
                if (r_u8(text) == 'a')
                {
                    image_index = r_u32(0x800B0690);
                    w_u32(0x800B04E0 + index * 4, text + 1);
                    td5_800a754c(image_index, frame);
                    value = r_u32(0x800B0690);
                    w_u32(0x800B0690, value + 1);
                }
                text = r_u32(0x800B04E0 + index * 4);
                if (r_u8(text) == 'e')
                {
                    text = r_u32(0x800B04E0 + index * 4);
                    w_u32(0x800B0410, 768);
                    w_u32(0x800B04E0 + index * 4, text + 1);
                }
                text = r_u32(0x800B04E0 + index * 4);
                if (r_u8(text) == 'b')
                    td5_800a78c0(frame, archive_error_index);
            }
            else
            {
                value = r_u32(0x8007F60C);
                w_u32(0x800B04E0 + index * 4, value);
            }
        }
    }
}

void td5_800a7884(uint32 original_stack, uint32 archive_error_index)
{
    uint32 frame = original_stack - 0x18;

    FUNCTION_MARKER(0x800A7884, "SLUS_006.10");
    td5_800a7674(frame, archive_error_index);
    if (r_s16(0x800884E8) != 0)
        td5_800a78c0(frame, archive_error_index);
}

void td5_800aba04(uint32 original_stack)
{
    uint32 frame = original_stack - 0x48;
    uint32 selected, count, track, value, pointer, state, car, volume;

    FUNCTION_MARKER(0x800ABA04, "SLUS_006.10");
    selected = r_u32(0x800876A8);
    w_u8(0x800BD070, 1);
    w_u8(0x800BD0E8, 1);
    w_u8(0x800BD160, 1);
    w_u8(0x800BD1D8, 1);
    if (selected == 0)
        w_u8(0x800BD070, 0);
    count = r_u32(0x800B03F4);
    if (selected == count - 1)
        w_u8(0x800BD0E8, 0);
    track = r_u32(0x800B04D8);
    if (track == 0)
        w_u8(0x800BD160, 0);
    count = r_u32(0x800BF114);
    if (track == count - 1)
        w_u8(0x800BD1D8, 0);
    td5_800b32e4(2);
    selected = r_u32(0x800876A8);
    car = r_u8(0x800B0328 + selected * 4);
    pointer = r_u32(0x8007DC60 + car * 4);
    td5_800b2790(128, 256, 90, td5_menu_string(pointer));
    selected = r_u32(0x800876A8);
    if ((r_u32(0x800B0328 + selected * 4) & 0x100) != 0)
    {
        td5_800b32e4(3);
        td5_800b2790(128, 256, 98, td5_menu_string(0x800A5E68));
    }
    td5_800b32e4(2);
    track = r_u32(0x800B04D8);
    value = r_u8(0x800BF3B8 + track * 2);
    pointer = r_u32(0x8007EAA0 + value * 68);
    td5_800b2790(128, 256, 130, td5_menu_string(pointer));
    track = r_u32(0x800B04D8);
    if ((r_u16(0x800BF3B8 + track * 2) & 0x100) != 0)
    {
        td5_800b32e4(3);
        td5_800b2790(128, 256, 138, td5_menu_string(0x800A5E68));
    }
    state = r_u32(0x800B02D0);
    if (state == 0)
    {
        if ((r_u16(0x800884E8) & 0x80) != 0)
        {
            selected = r_u32(0x800876A8);
            if ((sint32)selected > 0)
            {
                w_u32(0x800876A8, selected - 1);
                td5_80053064();
            }
        }
        if ((r_u16(0x800884E8) & 0x20) != 0)
        {
            count = r_u32(0x800B03F4);
            selected = r_u32(0x800876A8);
            if ((sint32)selected < (sint32)(count - 1))
            {
                w_u32(0x800876A8, selected + 1);
                td5_80053064();
            }
        }
    }
    else if (state == 1)
    {
        if ((r_u16(0x800884E8) & 0x80) != 0)
        {
            track = r_u32(0x800B04D8);
            if ((sint32)track > 0)
            {
                w_u32(0x800B04D8, track - 1);
                td5_80053064();
            }
        }
        if ((r_u16(0x800884E8) & 0x20) != 0)
        {
            count = r_u32(0x800BF114);
            track = r_u32(0x800B04D8);
            if ((sint32)track < (sint32)(count - 1))
            {
                w_u32(0x800B04D8, track + 1);
                td5_80053064();
            }
        }
    }
    else if (state == 2 && (r_u16(0x800884E8) & 0x4008) != 0)
    {
        selected = r_u32(0x800876A8);
        if ((r_u32(0x800B0328 + selected * 4) & 0x100) != 0)
            td5_80053124();
        else
        {
            track = r_u32(0x800B04D8);
            if ((r_u16(0x800BF3B8 + track * 2) & 0x100) != 0)
                td5_80053124();
            else
            {
                td5_800530f0();
                selected = r_u32(0x800876A8);
                car = r_u8(0x800B0328 + selected * 4);
                track = r_u32(0x800B04D8);
                w_u8(0x8008CF71, 0);
                value = r_u16(0x800BF3B8 + track * 2);
                w_u32(0x8008770C, car);
                w_u32(0x80087690, car);
                w_u8(0x8008CF72, value);
                w_u32(0x8008768C, (uint32)(sint32)(sint8)value);
                value = (uint32)randPSX();
                w_u8(0x8008CF74, value & 3);
                value = (uint32)randPSX();
                w_u8(0x8008CF76, value & 3);
                w_u16(0x8008CF78, 0);
                w_u16(0x8008CF7A, 0);
                w_u16(0x8008CF7C, 0);
                w_u16(0x8008CF7E, 0);
                w_u32(0x8008858C, 4);
                w_u32(0x800AE0C4, 1);
                td5_800557c0();
                volume = r_u32(0x80087700);
                w_u32(frame + 0x10, 5120);
                td5_8005490c(13, 20, (sint32)volume, 0, 5120);
            }
        }
    }
}

void td5_800ab3b0(void)
{
    uint32 selected, field, limit, value, destination, format;

    FUNCTION_MARKER(0x800AB3B0, "SLUS_006.10");
    if ((r_u16(0x800884E8) & 0xA0) != 0)
        td5_80053064();
    selected = r_u32(0x800876B4);
    field = 0;
    limit = 0;
    switch (selected)
    {
        case 0:
            field = 0x800876C4;
            break;
        case 1:
            field = 0x800876CC;
            break;
        case 2:
            field = 0x800876C8;
            break;
        case 3:
            field = 0x800876D0;
            break;
        case 4:
            field = 0x800876D4;
            limit = 2;
            break;
        case 5:
            field = 0x800876D8;
            break;
        case 6:
            field = 0x800876E8;
            break;
        case 7:
            field = 0x800876E4;
            limit = 9;
            break;
        case 8:
            field = 0x800876F0;
            break;
        case 9:
            field = 0x800876DC;
            limit = 9;
            break;
        case 10:
            field = 0x800876E0;
            limit = 3;
            break;
        default:
            break;
    }
    if (field != 0)
    {
        if (limit == 0)
        {
            if ((r_u16(0x800884E8) & 0xA0) != 0)
                w_u32(field, r_u32(field) ^ 1);
        }
        else
        {
            if ((r_u16(0x800884E8) & 0x80) != 0)
            {
                value = r_u32(field);
                if ((sint32)value > 0)
                    w_u32(field, value - 1);
            }
            if ((r_u16(0x800884E8) & 0x20) != 0)
            {
                value = r_u32(field);
                if ((sint32)value < (sint32)limit)
                    w_u32(field, value + 1);
            }
        }
    }
    destination = td5_menu_copy_string(r_u32(0x800BCF0C), 0x800A5E70);
    format = r_u32(0x800876C4) != 0 ? 0x800A5E80 : 0x800A5E7C;
    td5_menu_copy_string(destination + 3, format);
    destination = td5_menu_copy_string(r_u32(0x800BCF84), 0x800A5E70);
    format = r_u32(0x800876CC) != 0 ? 0x800A5E78 : 0x800A5E74;
    td5_menu_copy_string(destination + 3, format);
    destination = td5_menu_copy_string(r_u32(0x800BCFFC), 0x800A5E70);
    format = r_u32(0x800876C8) != 0 ? 0x800A5E78 : 0x800A5E74;
    td5_menu_copy_string(destination + 3, format);
    destination = td5_menu_copy_string(r_u32(0x800BD074), 0x800A5E70);
    format = r_u32(0x800876D0) != 0 ? 0x800A5E78 : 0x800A5E74;
    td5_menu_copy_string(destination + 3, format);
    destination = td5_menu_copy_string(r_u32(0x800BD0EC), 0x800A5E70);
    value = r_u32(0x800876D4);
    if (value == 0)
    {
        td5_menu_copy_string(destination + 3, 0x800A5E84);
        value = r_u32(0x800876D4);
    }
    if (value == 1)
    {
        td5_menu_copy_string(destination + 3, 0x800A5E8C);
        value = r_u32(0x800876D4);
    }
    if (value == 2)
        td5_menu_copy_string(destination + 3, 0x800A5E94);
    destination = td5_menu_copy_string(r_u32(0x800BD164), 0x800A5E70);
    format = r_u32(0x800876D8) != 0 ? 0x800A5EA8 : 0x800A5E9C;
    td5_menu_copy_string(destination + 3, format);
    destination = td5_menu_copy_string(r_u32(0x800BD1DC), 0x800A5E70);
    format = r_u32(0x800876E8) != 0 ? 0x800A5E78 : 0x800A5E74;
    td5_menu_copy_string(destination + 3, format);
    destination = td5_menu_copy_string(r_u32(0x800BD254), 0x800A5E70);
    value = r_u32(0x800876E4);
    td5_menu_format_number(destination + 3, 0x800A5EB4, value + 1);
    destination = td5_menu_copy_string(r_u32(0x800BD2CC), 0x800A5E70);
    format = r_u32(0x800876F0) != 0 ? 0x800A5E78 : 0x800A5E74;
    td5_menu_copy_string(destination + 3, format);
    destination = td5_menu_copy_string(r_u32(0x800BD344), 0x800A5E70);
    value = r_u32(0x800876DC);
    td5_menu_format_number(destination + 3, 0x800A5EB4, value + 1);
    destination = td5_menu_copy_string(r_u32(0x800BD3BC), 0x800A5E70);
    value = r_u32(0x800876E0);
    td5_menu_format_number(destination + 3, 0x800A5EB4, value + 1);
}

void td5_800ad5e0(uint32 original_stack)
{
    uint32 frame = original_stack - 0x58;
    uint32 first, second, third, mode, count, slot, value, other;
    uint32 index, items, local, flag_base, name_base, stride, offset, flag;

    FUNCTION_MARKER(0x800AD5E0, "SLUS_006.10");
    first = r_u32(0x800A62C0);
    second = r_u32(0x800A62C4);
    third = r_u32(0x800A62C8);
    w_u32(frame, first);
    w_u32(frame + 4, second);
    w_u32(frame + 8, third);
    first = r_u32(0x800A62CC);
    second = r_u32(0x800A62D0);
    w_u32(frame + 12, first);
    w_u32(frame + 16, second);
    mode = r_u32(0x80087728);
    first = r_u32(0x800A62D4);
    second = r_u32(0x800A62D8);
    third = r_u32(0x800A62DC);
    w_u32(frame + 0x18, first);
    w_u32(frame + 0x1C, second);
    w_u32(frame + 0x20, third);
    first = r_u32(0x800A62E0);
    second = r_u32(0x800A62E4);
    w_u32(frame + 0x24, first);
    w_u32(frame + 0x28, second);
    w_u32(frame + 0x30, 15);
    w_u32(frame + 0x34, 13);
    first = r_u32(0x800A62E8);
    second = r_u32(0x800A62EC);
    third = r_u32(0x800A62F0);
    w_u32(frame + 0x38, first);
    w_u32(frame + 0x3C, second);
    w_u32(frame + 0x40, third);
    first = r_u32(0x800A62F4);
    second = r_u32(0x800A62F8);
    third = r_u32(0x800A62FC);
    w_u32(frame + 0x44, first);
    w_u32(frame + 0x48, second);
    w_u32(frame + 0x4C, third);
    w_u8(0x8009DB38 + mode, 1);
    mode = r_u32(0x80087728);
    count = 0;
    slot = 5;
    if ((mode == 0 || mode == 5) && r_u8(0x8009DB38) != 0 && r_u8(0x8009DB3D) != 0)
    {
        first = r_u32(0x8007DD04);
        second = r_u32(0x8007DD08);
        count = 2;
        slot = 7;
        w_u8(0x8009DB3F, 1);
        w_u8(0x8009DB40, 1);
        w_u32(0x800BD164, first);
        w_u32(0x800BD1DC, second);
    }
    if (r_u32(0x80087728) - 1 < 2 && r_u8(0x8009DB39) != 0 && r_u8(0x8009DB3A) != 0)
    {
        offset = slot * 120;
        value = r_u32(0x8007DD0C);
        ++slot;
        count += 2;
        w_u8(0x8009DB41, 1);
        w_u32(0x800BCF0C + offset, value);
        value = r_u32(0x8007DD10);
        offset = slot * 120;
        ++slot;
        w_u8(0x8009DB42, 1);
        w_u32(0x800BCF0C + offset, value);
    }
    mode = r_u32(0x80087728);
    items = 0;
    local = 0;
    flag_base = 0;
    name_base = 0;
    stride = 0;
    switch (mode)
    {
        case 0:
            if (r_u8(0x8009DB43) == 0)
            {
                w_u8(0x8009DB43, 1);
                value = r_u32(0x8007DD18);
                ++count;
                w_u32(0x800BCF0C + slot * 120, value);
            }
            break;
        case 1:
            items = 5;
            local = frame;
            flag_base = 0x8009DB71;
            name_base = 0x8007DC5C;
            stride = 4;
            break;
        case 2:
            items = 5;
            local = frame + 0x18;
            flag_base = 0x8009DB45;
            name_base = 0x8007EAA0;
            stride = 68;
            break;
        case 3:
            items = 2;
            local = frame + 0x30;
            flag_base = 0x8009DB71;
            name_base = 0x8007DC5C;
            stride = 4;
            break;
        case 4:
            if (r_u8(0x8009DB77) != 2)
            {
                ++count;
                value = r_u32(0x8007DC5C);
                offset = slot * 120;
                ++slot;
                w_u8(0x8009DB77, 2);
                w_u32(0x800BCF0C + offset, value);
            }
            if (r_u8(0x8009DB96) != 2)
            {
                ++count;
                value = r_u32(0x8007DCF0);
                offset = slot * 120;
                ++slot;
                w_u8(0x8009DB96, 2);
                w_u8(0x8009DB97, 2);
                w_u32(0x800BCF0C + offset, value);
            }
            items = 6;
            local = frame + 0x38;
            flag_base = 0x8009DB45;
            name_base = 0x8007EAA0;
            stride = 68;
            break;
        case 5:
            if (r_u8(0x8009DB8A) != 2)
            {
                ++count;
                value = r_u32(0x8007DCC0);
                offset = slot * 120;
                ++slot;
                w_u8(0x8009DB8A, 2);
                w_u8(0x8009DB91, 2);
                w_u32(0x800BCF0C + offset, value);
            }
            if (r_u8(0x8009DB98) != 2)
            {
                value = r_u32(0x8007DCF8);
                ++count;
                w_u8(0x8009DB98, 2);
                w_u8(0x8009DB99, 2);
                w_u32(0x800BCF0C + slot * 120, value);
            }
            break;
        default:
            break;
    }
    offset = slot * 120;
    for (index = 0; index < items; ++index)
    {
        value = r_u32(local + index * 4);
        flag = flag_base + value;
        if (r_u8(flag) != 2)
        {
            w_u8(flag, 2);
            other = r_u32(local + index * 4);
            value = r_u32(name_base + other * stride);
            ++count;
            w_u32(0x800BCF0C + offset, value);
            offset += 120;
        }
    }
    if (count == 0)
        w_u8(0x800BD0E8, 0);
}

void td5_800a6594(uint32 original_stack)
{
    uint32 frame = original_stack - 0x80;
    uint32 count, selected, item, special, heading_table, pointer, index, row;
    uint32 record, value, numerator, rate, time, format, multiplier, shift;
    uint32 remainder, fraction, fixed_speed;
    sint32 x, y, seconds_unit, minutes, seconds;

    FUNCTION_MARKER(0x800A6594, "SLUS_006.10");
    count = r_u8(0x800B0414);
    w_u8(0x800BCF08, 1);
    w_u8(0x800BCF80, 1);
    if (count == 0)
    {
        w_u8(0x800BCF08, 0);
        w_u8(0x800BCF80, 0);
    }
    selected = r_u32(0x800B0698);
    if (selected == 0)
        w_u8(0x800BCF08, 0);
    if (selected == count - 1)
        w_u8(0x800BCF80, 0);
    if ((r_u16(0x800884E8) & 0x80) != 0 && (sint32)selected > 0)
    {
        td5_80053064();
        w_u32(0x800B0698, r_u32(0x800B0698) - 1);
    }
    if ((r_u16(0x800884E8) & 0x20) != 0)
    {
        count = r_u8(0x800B0414);
        selected = r_u32(0x800B0698);
        if ((sint32)selected < (sint32)(count - 1))
        {
            td5_80053064();
            w_u32(0x800B0698, r_u32(0x800B0698) + 1);
        }
    }
    selected = r_u32(0x800B0698);
    item = r_u8(0x800B02E0 + selected);
    special = item == 44 || item == 40;
    td5_800b32e4(0);
    pointer = r_u32(0x8007DC60 + item * 4);
    td5_800b2790(129, 256, 64, td5_menu_string(pointer));
    td5_800b32e4(3);
    if (special != 0)
        heading_table = 0x800ADFE4;
    else if (item - 24 < 5 || item == 36 || item == 38)
        heading_table = 0x800ADFFC;
    else
        heading_table = 0x800ADFCC;
    for (index = 0; index < 6; ++index)
    {
        x = r_s16(0x800AE0AC + index * 2);
        pointer = r_u32(heading_table + index * 4);
        td5_800b2790(129, x, 96, td5_menu_string(pointer));
        if (index >= 4)
        {
            pointer = r_u32(0x800876C4) == 0 ? 0x800A52A0 : 0x800A52A8;
            td5_800b2790(129, x, 104, td5_menu_string(pointer));
        }
    }
    y = 112;
    for (row = 0; row < 5; ++row, y += 20)
    {
        record = item * 120 + row * 20;
        td5_800b32e4(row == 0 ? 4 : 6);
        if (special == 0 && r_u8(0x8007C4D8 + record) != 0)
            td5_800b32e4(1);
        x = r_s16(0x800AE0AC);
        td5_menu_format_number(frame + 0x18, 0x800A52B0, row + 1);
        td5_800b2790(128, x, y, td5_menu_string(frame + 0x18));
        x = r_s16(0x800AE0AE);
        td5_800b2790(128, x, y, td5_menu_string(0x8007C4D9 + record));
        if (special != 0)
        {
            x = r_s16(0x800AE0B0);
            if (item == 44)
                value = r_u32(0x8007D974 + row * 20);
            else
                value = r_u8(0x8007C4E3 + record);
            td5_menu_format_number(frame + 0x18, 0x800A52B0, value);
        }
        else
        {
            if (item == 29)
            {
                rate = r_u32(0x800880FC);
                numerator = r_u32(0x8007D26C + row * 20);
                rate = (uint32)((sint32)rate / 2);
                time = td5_menu_unsigned_time(numerator, rate);
                seconds_unit = 1000;
                format = 0x800A52B4;
            }
            else
            {
                numerator = r_u32(0x8007C4D4 + record);
                rate = r_u32(0x800880FC);
                numerator *= 100;
                rate = (uint32)((sint32)rate / 2);
                time = td5_menu_unsigned_time(numerator, rate);
                seconds_unit = 100;
                format = 0x800A52C4;
            }
            minutes = (sint32)time / (seconds_unit * 60);
            remainder = time - (uint32)minutes * (uint32)(seconds_unit * 60);
            seconds = (sint32)remainder / seconds_unit;
            x = r_s16(0x800AE0B0);
            fraction = remainder - (uint32)seconds * (uint32)seconds_unit;
            w_u32(frame + 0x10, fraction);
            td5_menu_format_time(frame + 0x18, format, time, seconds_unit);
        }
        td5_800b2790(128, x, y, td5_menu_string(frame + 0x18));
        value = r_u8(0x8007C4E2 + record);
        x = r_s16(0x800AE0B2);
        pointer = r_u32(0x800AE014 + value * 4);
        td5_800b2790(128, x, y, td5_menu_string(pointer));
        value = r_u32(0x800876C4);
        x = r_s16(0x800AE0B4);
        fixed_speed = (uint32)r_u16(0x8007C4E4 + record) << 8;
        multiplier = value == 0 ? 0xD161543F : 0xA8791709;
        shift = value == 0 ? 10 : 9;
        value = td5_menu_result_speed(fixed_speed, multiplier, shift);
        td5_menu_format_number(frame + 0x18, 0x800A52B0, value);
        td5_800b2790(128, x, y, td5_menu_string(frame + 0x18));
        value = r_u32(0x800876C4);
        x = r_s16(0x800AE0B6);
        fixed_speed = (uint32)r_u16(0x8007C4E6 + record) << 8;
        multiplier = value == 0 ? 0xD161543F : 0xA8791709;
        shift = value == 0 ? 10 : 9;
        value = td5_menu_result_speed(fixed_speed, multiplier, shift);
        td5_menu_format_number(frame + 0x18, 0x800A52B0, value);
        td5_800b2790(128, x, y, td5_menu_string(frame + 0x18));
    }
}

static void td5_800a8ba8(uint32 slot)
{
    uint32 base = slot * 29;
    uint32 index, state, previous, type, source, value, flag;

    FUNCTION_MARKER(0x800A8BA8, "SLUS_006.10");
    w_u8(0x800BCF08 + (base + 5) * 120, 0);
    for (index = 0; index < 4; ++index)
        w_u8(0x800BCF08 + (base + index) * 120, 0);
    state = r_u32(0x800AE0F8 + slot * 4);
    w_u8(0x800BCF08 + (base + 4) * 120, 0);
    previous = state - 1;
    for (index = 0; index < 10; ++index)
    {
        w_u8(0x800BCF08 + (base + index + 6) * 120, 0);
        w_u8(0x800BCF08 + (base + index + 16) * 120, 0);
    }
    type = r_u32(0x80088450 + slot * 4);
    w_u8(0x800BCF08 + (base + 26) * 120, 1);
    w_u8(0x800BCF08 + (base + 27) * 120, 1);
    w_u8(0x800BCF08 + (base + 28) * 120, 1);
    if (type == 0x4000)
        w_u32(0x800AE0F8 + slot * 4, 0);
    else if (type == 0x2000)
    {
        flag = r_u32(0x80087684 + slot * 4);
        source = (flag == 0 ? 0x800AE328 : 0x800AE340) + previous * 12;
        for (index = 0; index < 3; ++index)
        {
            value = r_u32(source + index * 4);
            w_u32(0x800BCF0C + (base + 26 + index) * 120, value);
        }
    }
    else if (type == 0x5000 || type == 0x7000)
    {
        source = 0x800AE358 + previous * 12;
        for (index = 0; index < 3; ++index)
        {
            value = r_u32(source + index * 4);
            w_u32(0x800BCF0C + (base + 26 + index) * 120, value);
        }
    }
}

static void td5_800a8edc(uint32 slot)
{
    uint32 state_address = 0x800AE0F8 + slot * 4;
    uint32 center_address = 0x800B0288 + slot * 8;
    uint32 high_address = 0x800B0298 + slot * 8;
    uint32 low_address = 0x800B02A8 + slot * 8;
    uint32 value, center, high, low;

    FUNCTION_MARKER(0x800A8EDC, "SLUS_006.10");
    if (r_u32(state_address) == 1)
    {
        if ((r_u16(0x800884E8 + slot * 2) & 8) != 0)
        {
            td5_80053030();
            value = r_u32(0x800886A0 + slot * 4);
            w_u32(center_address, value);
            value = r_u32(state_address);
            w_u32(state_address, value + 1);
            center = r_u32(center_address);
            if ((sint32)center < 48)
                center = 48;
            else if ((sint32)center >= 209)
                center = 208;
            w_u32(center_address, center);
            high = r_u32(center_address);
            low = r_u32(center_address);
            w_u32(high_address, high + 32);
            w_u32(low_address, low - 32);
        }
    }
    else
    {
        value = r_u32(0x800886A0 + slot * 4);
        high = r_u32(high_address);
        if ((sint32)high < (sint32)value)
        {
            w_u32(high_address, value);
            value = r_u32(0x800886A0 + slot * 4);
        }
        low = r_u32(low_address);
        if ((sint32)value < (sint32)low)
            w_u32(low_address, value);
        if ((r_u16(0x800884E8 + slot * 2) & 8) != 0)
        {
            td5_80053030();
            center = r_u32(center_address);
            high = r_u32(high_address);
            low = r_u32(low_address);
            w_u32(0x8007DC24 + slot * 8, center);
            w_u32(0x8007DC34 + slot * 8, high);
            w_u32(0x8007DC44 + slot * 8, low);
            w_u32(state_address, 0);
        }
    }
}

static void td5_800a7ef8(uint32 slot, uint32 configuration, uint32 inherited_s3)
{
    uint32 base = slot * 29;
    uint32 mapping = inherited_s3;
    uint32 destination, value, type, index, entry, flag, pointer;
    sint32 parameter;

    FUNCTION_MARKER(0x800A7EF8, "SLUS_006.10");
    if (r_u32(0x800AE0F8 + slot * 4) != 0)
    {
        td5_800a8ba8(slot);
        return;
    }
    destination = r_u32(0x800BCF0C + base * 120);
    td5_menu_format_number(destination, 0x800A57D4, configuration + 1);
    parameter = r_s32(0x80087674 + slot * 4);
    destination = r_u32(0x800BCF0C + (base + 1) * 120);
    td5_menu_format_number(destination, 0x800A57E8, (uint32)(parameter / 512) + 1);
    parameter = r_s32(0x8008767C + slot * 4);
    destination = r_u32(0x800BCF0C + (base + 2) * 120);
    td5_menu_format_number(destination, 0x800A57F4, (uint32)(parameter / 512) + 1);
    value = r_u32(0x80087684 + slot * 4);
    destination = r_u32(0x800BCF0C + (base + 4) * 120);
    td5_menu_copy_string(destination, value != 0 ? 0x800A5804 : 0x800A580C);
    type = r_u32(0x80088450 + slot * 4);
    if (type == 0x4000)
        mapping = 0x800AE100;
    else if (type == 0x2000)
    {
        value = r_u32(0x80087684 + slot * 4);
        mapping = value != 0 ? 0x800AE164 : 0x800AE1C8;
    }
    else if (type == 0x5000 || type == 0x7000)
        mapping = 0x800AE22C;
    w_u8(0x800BCF08 + (base + 26) * 120, 0);
    w_u8(0x800BCF08 + (base + 27) * 120, 0);
    type = r_u32(0x80088450 + slot * 4);
    w_u8(0x800BCF08 + (base + 28) * 120, 0);
    if (type != 255)
    {
        w_u8(0x800BCF08 + (base + 5) * 120, 0);
        for (index = 0; index < 5; ++index)
            w_u8(0x800BCF08 + (base + index) * 120, 1);
        for (index = 0; index < 10; ++index)
        {
            w_u8(0x800BCF08 + (base + index + 6) * 120, 1);
            entry = 0x800BCF08 + (base + index + 16) * 120;
            w_u8(entry, 1);
            value = r_u16(mapping + configuration * 2);
            pointer = r_u32(0x800AE290 + value * 4);
            w_u32(entry + 4, pointer);
            value = r_u16(mapping + configuration * 2);
            if (value < 4)
            {
                w_u16(entry + 2, 0x8000);
                value = r_u16(mapping + configuration * 2);
                pointer = r_u32(0x800AE290 + value * 4);
                mapping += 10;
                pointer = td5_find_texture(td5_menu_string(pointer));
                w_u32(entry + 4, pointer);
            }
            else
            {
                w_u16(entry + 2, 1);
                mapping += 10;
            }
        }
    }
    else
    {
        w_u8(0x800BCF08 + (base + 5) * 120, 1);
        for (index = 0; index < 5; ++index)
            w_u8(0x800BCF08 + (base + index) * 120, 0);
        for (index = 0; index < 10; ++index)
        {
            w_u8(0x800BCF08 + (base + index + 6) * 120, 0);
            w_u8(0x800BCF08 + (base + index + 16) * 120, 0);
        }
    }
    type = r_u32(0x80088450 + slot * 4);
    if (type == 0x4000)
    {
        for (index = 1; index < 5; ++index)
            w_u8(0x800BCF08 + (base + index) * 120, 0);
    }
    else if (type == 0x5000 || type == 0x7000)
    {
        flag = r_u32(0x800B0420 + slot * 4);
        w_u8(0x800BCF08 + (base + 3) * 120, 0);
        if (flag == 0)
            w_u8(0x800BCF08 + (base + 3) * 120, 1);
        w_u8(0x800BCF08 + (base + 4) * 120, 0);
    }
}

static void td5_800a8588(uint32 slot)
{
    uint32 base = slot * 29;
    uint32 index, type, limit, value, selection, field;
    uint32 pad = 0x800884E8 + slot * 2;
    uint32 selection_address = 0x800AE0F0 + slot * 4;

    FUNCTION_MARKER(0x800A8588, "SLUS_006.10");
    if (r_u32(0x800AE0F8 + slot * 4) != 0)
    {
        td5_800a8edc(slot);
        return;
    }
    for (index = 0; index < 5; ++index)
        w_u8(0x800BCF09 + (base + index) * 120, 0);
    limit = 0;
    type = r_u32(0x80088450 + slot * 4);
    if (type == 0x2000)
        limit = 4;
    else if (type == 0x5000 || type == 0x7000)
        limit = r_u32(0x800B0420 + slot * 4) != 0 ? 2 : 3;
    if ((r_u16(pad) & 0x10) != 0 && r_s32(selection_address) > 0)
    {
        td5_80053030();
        w_u32(selection_address, r_u32(selection_address) - 1);
    }
    if ((r_u16(pad) & 0x40) != 0 && r_s32(selection_address) < (sint32)limit)
    {
        td5_80053030();
        w_u32(selection_address, r_u32(selection_address) + 1);
    }
    selection = r_u32(selection_address);
    w_u8(0x800BCF09 + (base + selection) * 120, 1);
    selection = r_u32(selection_address);
    switch (selection)
    {
        case 0:
            field = 0x8008766C + slot * 4;
            if ((r_u16(pad) & 0x80) != 0 && r_s32(field) > 0)
            {
                td5_80053030();
                w_u32(field, r_u32(field) - 1);
            }
            if ((r_u16(pad) & 0x20) != 0 && r_s32(field) < 4)
            {
                td5_80053030();
                w_u32(field, r_u32(field) + 1);
            }
            break;
        case 1:
        case 2:
            field = (selection == 1 ? 0x80087674 : 0x8008767C) + slot * 4;
            if ((r_u16(pad) & 0x80) != 0 && r_s32(field) >= 512)
            {
                td5_80053030();
                w_u32(field, r_u32(field) - 512);
            }
            if ((r_u16(pad) & 0x20) != 0 && r_s32(field) < 4096)
            {
                td5_80053030();
                w_u32(field, r_u32(field) + 512);
            }
            break;
        case 3:
            if ((r_u16(pad) & 0x4008) != 0)
                w_u32(0x800AE0F8 + slot * 4, 1);
            break;
        case 4:
            if ((r_u16(pad) & 0x4008) != 0)
            {
                field = 0x80087684 + slot * 4;
                value = r_u32(field);
                w_u32(field, value ^ 1);
            }
            break;
        default:
            break;
    }
}

void td5_800a8a48(uint32 slot, uint32 inherited_s3)
{
    uint32 type, previous, ticks, available, configuration;
    sint32 state;

    FUNCTION_MARKER(0x800A8A48, "SLUS_006.10");
    type = r_u32(0x80088450 + slot * 4);
    previous = r_u32(0x800B03F8 + slot * 4);
    w_u32(0x800887B8, 0xFFFFFFFF);
    if (type != previous)
    {
        w_u32(0x800B0420 + slot * 4, 0);
        type = r_u32(0x80088450 + slot * 4);
        if (type == 0x5000 || type == 0x7000)
        {
            ticks = 0;
            for (;;)
            {
                state = PadGetState((sint32)(slot << 4));
                if (state == 6 || ticks >= 31)
                    break;
                VSync(0);
                ++ticks;
            }
            available = 0;
            if (ticks < 30)
                available = (uint32)PadInfoMode((sint32)(slot << 4), 2, 0) != 0;
            w_u32(0x800B0420 + slot * 4, available);
        }
    }
    type = r_u32(0x80088450 + slot * 4);
    w_u32(0x800B03F8 + slot * 4, type);
    configuration = r_u32(0x8008766C + slot * 4);
    td5_800a7ef8(slot, configuration, inherited_s3);
    (void)r_u32(0x8008766C + slot * 4);
    td5_800a8588(slot);
    if ((r_u16(0x800884E8 + slot * 2) & 0x1000) != 0 && slot == 0)
        td5_800a99f0();
}

static char td5_archive_path[512];

static void td5_file_error(const char *path, const char *operation)
{
    char message[768];

    snprintf(message, sizeof(message), "%s: %s", operation, path);
    xport_message_error("Test Drive 5 - File error", message);
    exit(EXIT_FAILURE);
}

static void td5_data_path(char *path, uint32 filename)
{
    const char *name = td5_menu_string(filename);
    size_t index = 5;

    memcpy(path, "DATA/", 5);
    while (*name == '\\' || *name == '/')
        ++name;
    while (*name != 0 && *name != ';')
    {
        if (index + 1 >= 512)
            td5_file_error("DATA", "Asset path is too long");
        path[index++] = *name == '\\' ? '/' : *name;
        ++name;
    }
    path[index] = 0;
}

static void td5_read_data_range(const char *path, uint32 offset, uint32 bytes, uint32 destination)
{
    FILE *file;
    long length = 0;
    size_t count;

    file = xport_fopen(path, "rb");
    if (file == 0)
        td5_file_error(path, "Cannot open asset");
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0)
    {
        fclose(file);
        td5_file_error(path, "Cannot determine asset size");
    }
    if ((uint64)offset + bytes > (uint64)length || offset > 0x7FFFFFFF || fseek(file, (long)offset, SEEK_SET) != 0)
    {
        fclose(file);
        td5_file_error(path, "Asset read is outside the file");
    }
    count = xport_fread(psx_addr(destination, bytes), 1, bytes, file);
    if (count != bytes)
    {
        fclose(file);
        td5_file_error(path, "Asset read failed or ended early");
    }
    fclose(file);
}

uint32 td5_read_data_file(uint32 filename, uint32 destination)
{
    char path[512];
    FILE *file;
    long length = 0;
    uint32 bytes, padded;
    size_t count;

    td5_data_path(path, filename);
    file = xport_fopen(path, "rb");
    if (file == 0)
        td5_file_error(path, "Cannot open asset");
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        td5_file_error(path, "Cannot determine asset size");
    }
    bytes = (uint32)length;
    padded = (bytes + 2047) & 0xFFFFF800;
    count = xport_fread(psx_addr(destination, padded), 1, bytes, file);
    if (count != bytes)
    {
        fclose(file);
        td5_file_error(path, "Asset read failed or ended early");
    }
    fclose(file);
    if (padded != bytes)
        memset(psx_addr(destination + bytes, padded - bytes), 0, padded - bytes);
    return bytes;
}

void td5_load_archive_directory(uint32 filename)
{
    td5_data_path(td5_archive_path, filename);
    td5_read_data_range(td5_archive_path, 0, 10 * 2048, 0x80092648);
}

void td5_read_archive_sectors(uint32 sector, uint32 sectors, uint32 destination)
{
    uint64 offset = (uint64)sector * 2048;
    uint64 bytes = (uint64)sectors * 2048;

    if (td5_archive_path[0] == 0)
        td5_file_error("TD5.DAT", "Archive has not been opened");
    if (offset > 0x7FFFFFFF || bytes > 0xFFFFFFFF)
        td5_file_error(td5_archive_path, "Archive sector range is too large");
    td5_read_data_range(td5_archive_path, (uint32)offset, (uint32)bytes, destination);
}

void td5_read_data_prefix(uint32 filename, uint32 destination, uint32 bytes)
{
    char path[512];

    td5_data_path(path, filename);
    td5_read_data_range(path, 0, bytes, destination);
}

typedef struct TD5_XA_FILE
{
    FILE *file;
    uint32 first_sector;
    uint32 sectors;
} TD5_XA_FILE;

static sint32 td5_xa_read_sector(void *context, uint32 sector,
    uint8 buffer[2352], uint32 *sector_bytes)
{
    TD5_XA_FILE *source = (TD5_XA_FILE *)context;
    uint32 relative;

    if (sector < source->first_sector)
        return 0;
    relative = sector - source->first_sector;
    if (relative >= source->sectors ||
        fseek(source->file, (long)(relative * 2352u), SEEK_SET) != 0 ||
        xport_fread(buffer, 1, 2352, source->file) != 2352)
        return 0;
    *sector_bytes = 2352;
    return 1;
}

static void td5_xa_close(void *context)
{
    TD5_XA_FILE *source = (TD5_XA_FILE *)context;

    fclose(source->file);
    free(source);
}

sint32 td5_mount_xa_file(uint32 filename, CdlFILE *result)
{
    char path[512];
    uint8 header[16];
    TD5_XA_FILE *owner;
    XPORT_CD_SOURCE source;
    CdlLOC position;
    FILE *file;
    long bytes;
    sint32 first;
    uint32 index;
    const char *name;

    td5_data_path(path, filename);
    file = xport_fopen(path, "rb");
    if (file == NULL)
        return 0;
    if (fseek(file, 0, SEEK_END) != 0 || (bytes = ftell(file)) <= 0 ||
        bytes % 2352 != 0 || fseek(file, 0, SEEK_SET) != 0 ||
        xport_fread(header, 1, sizeof(header), file) != sizeof(header))
        goto failed;
    if (header[0] != 0 || header[11] != 0 || header[15] != 2)
        goto failed;
    for (index = 1; index < 11; ++index)
        if (header[index] != 0xFF)
            goto failed;
    for (index = 12; index < 15; ++index)
        if ((header[index] & 15) > 9 || (header[index] >> 4) > 9)
            goto failed;
    if (header[13] >= 0x60 || header[14] >= 0x75)
        goto failed;
    position.minute = header[12];
    position.second = header[13];
    position.sector = header[14];
    position.track = 0;
    first = CdPosToInt(&position);
    if (first < 0)
        goto failed;
    owner = (TD5_XA_FILE *)malloc(sizeof(*owner));
    if (owner == NULL)
        goto failed;
    owner->file = file;
    owner->first_sector = (uint32)first;
    owner->sectors = (uint32)bytes / 2352;
    memset(&source, 0, sizeof(source));
    source.context = owner;
    source.sectors = owner->first_sector + owner->sectors;
    source.read_sector = td5_xa_read_sector;
    source.close = td5_xa_close;
    if (!xport_cd_mount_source(&source))
    {
        free(owner);
        goto failed;
    }
    memset(result, 0, sizeof(*result));
    result->pos = position;
    result->size = owner->sectors * 2048;
    name = strrchr(path, '/');
    name = name != NULL ? name + 1 : path;
    for (index = 0; index < sizeof(result->name) - 1 && name[index] != 0; ++index)
        result->name[index] = name[index];
    return 1;

failed:
    fclose(file);
    return 0;
}
