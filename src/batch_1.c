#include "td5_functions.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Native startup adapter for the original empty constructor table */
void td5_startup_constructors(void)
{
    if (r_u32(0x800861D0) == 0)
        w_u32(0x800861D0, 1);
}

sint32 td5_main(uint32 archive_error_index, uint32 original_stack, uint32 texture_carrier, uint32 tree_descriptor_carrier, uint32 invalid_header_index, uint32 saved_s0_carrier, uint32 audio_angle_carrier)
{
    sint32 random_value;

    FUNCTION_MARKER(0x80015E38, "SLUS_006.10");
    td5_startup_constructors();
    ResetCallback();
    CdInit();
    td5_8001acfc();
    td5_init_graphics();
    td5_80055b3c();
    td5_open_data_archive(0x8001004C);
    td5_8001a0dc(0x800A7A00, archive_error_index);
    td5_load_menu_sounds();
    td5_80078804();
    td5_load_sine_table(archive_error_index);
    td5_set_sound_limit(4096);
    w_u32(0x8008858C, 0);
    w_u32(0x80088620, 80);
    while (r_u32(0x80087638) == 0)
    {
        td5_debug_log(0x8008774C);
        switch (r_u32(0x8008858C))
        {
            case 0:
                if (r_u32(0x80087654) != 0)
                    td5_80019bb8(0x800F1D00, archive_error_index);
                w_u32(0x80087650, 0);
                td5_load_archive_file((const char *)psx_addr(0x80010058, 14), r_u32(0x800132B0), archive_error_index);
                td5_load_archive_file((const char *)psx_addr(0x80010068, 16), r_u32(0x800132B4), archive_error_index);
                w_u32(0x80087644, 0);
                if (r_u32(0x80087654) == 0)
                {
                    td5_800adb84();
                    td5_80016fc4();
                    td5_80016344();
                    td5_80016ef8();
                }
                td5_800a7b84(archive_error_index, original_stack - 0x28, invalid_header_index);
                td5_800b789c(r_s8(0x8008CF72));
                td5_8001a4d8();
                td5_800a90dc();
                w_u32(0x80087654, 1);
                td5_80019bb8(0x800F1D00, archive_error_index);
                w_u32(0x80088668, r_u32(0x80087708));
                break;
            case 1:
                w_u32(0x8008871C, 0);
                w_u32(0x80088418, 0);
                if (r_u32(0x8008772C) != 0)
                    w_u32(0x8008858C, 7);
                else
                {
                    td5_80019bb8(0x800F1D00, archive_error_index);
                    td5_8001492c(archive_error_index, original_stack - 0x28, texture_carrier, tree_descriptor_carrier);
                    w_u32(0x80087644, 0);
                    w_u32(0x8008858C, 2);
                }
                break;
            case 2:
                w_u32(0x80087644, 0);
                td5_80015be8(original_stack - 0x28, invalid_header_index, audio_angle_carrier);
                td5_debug_log(0x80087754);
                break;
            case 3:
                w_u32(0x80087644, 0);
                td5_load_archive_file((const char *)psx_addr(0x80010068, 16), r_u32(0x800132B4), archive_error_index);
                td5_800575a4(archive_error_index, original_stack - 0x28);
                td5_80057b30(original_stack - 0x28, invalid_header_index);
                td5_800557c0();
                break;
            case 4:
                w_u32(0x80087738, 0);
                td5_80018db4(archive_error_index, original_stack - 0x28, saved_s0_carrier);
                break;
            case 5:
                td5_80019bb8(0x800F1D00, archive_error_index);
                td5_80019458(archive_error_index, original_stack - 0x28, saved_s0_carrier);
                break;
            case 6:
                w_u32(0x8008871C, 1);
                w_u32(0x80088418, 1);
                td5_8001a328(archive_error_index, original_stack - 0x28, 0x80016138, saved_s0_carrier);
                if (r_u32(0x8008772C) != 0)
                    w_u32(0x8008858C, 7);
                else
                {
                    td5_80019bb8(0x800F1D00, archive_error_index);
                    w_u32(0x80087644, 0);
                    w_u32(0x80087738, 1);
                    td5_8001492c(archive_error_index, original_stack - 0x28, texture_carrier, tree_descriptor_carrier);
                    w_u32(0x8008858C, 2);
                }
                break;
            case 7:
                td5_80019bb8(0x800F1D00, archive_error_index);
                td5_8001492c(archive_error_index, original_stack - 0x28, texture_carrier, tree_descriptor_carrier);
                w_u32(0x80087644, 0);
                w_u32(0x8008858C, 8);
                break;
            case 8:
                w_u32(0x80087644, 0);
                td5_80015b64(original_stack - 0x28, invalid_header_index);
                break;
            case 9:
                w_u32(0x80087650, 1);
                w_u32(0x80087720, 0);
                w_u32(0x80087728, 0);
                w_u32(0x8008771C, 0);
                w_u32(0x80087738, 0);
                w_u32(0x80087724, 0);
                w_u32(0x8008772C, 0);
                w_u32(0x80087730, 0);
                w_u32(0x80087734, 0);
                w_u32(0x80087710, r_u8(0x8007DADC + r_u32(0x80087614)));
                w_u32(0x80087614, (r_u32(0x80087614) + 1) & 3);
                random_value = randPSX();
                w_u8(0x8008CF72, (uint8)(random_value % 15));
                td5_8001a328(archive_error_index, original_stack - 0x28, 0x8001622C, saved_s0_carrier);
                td5_80048fc8();
                w_u32(0x80087738, 1);
                w_u32(0x8008871C, 0);
                td5_80019bb8(0x800F1D00, archive_error_index);
                w_u32(0x80087644, 0);
                td5_8001492c(archive_error_index, original_stack - 0x28, texture_carrier, tree_descriptor_carrier);
                w_u32(0x8008858C, 2);
                break;
            case 10:
                td5_8001ae4c();
                w_u32(0x8008858C, 0);
                w_u32(0x80088620, 80);
                td5_80019bb8(0x800F1D00, archive_error_index);
                break;
            default:
                break;
        }
    }
    td5_debug_log(0x8008775C);
    DrawSync(0);
    stopIntr();
    PadStopCom();
    ResetGraph(3);
    ResetCallback();
    return 0;
}

uint32 td5_load_cd_file(uint32 filename, uint32 destination)
{
    FUNCTION_MARKER(0x8007B720, "SLUS_006.10");
    return td5_read_data_file(filename, destination);
}

void td5_debug_log(uint32 format, ...)
{
    FUNCTION_MARKER(0x8007A89C, "SLUS_006.10");
    /* The original only spills argument registers to its caller's stack */
    (void)format;
}

void td5_init_graphics(void)
{
    FUNCTION_MARKER(0x80015DD0, "SLUS_006.10");
    InitGeom();
    td5_init_sound();
    ResetGraph(3);
    SetGraphDebug(0);
    DrawSync(0);
    SetDispMask(0);
}

sint32 td5_init_sound(void)
{
    uint32 volume;
    CdlATV mix;
    uint32 index;
    uint32 offset;
    uint32 byte;

    FUNCTION_MARKER(0x800543E0, "SLUS_006.10");
    volume = r_u32(0x80087BDC);
    SpuInit();
    SpuInitMalloc(20, (char *)psx_addr(0x80089B78, 21 * sizeof(SpuMallocRecord)));
    w_u16(0x80089C24, 0x3FFC);
    w_u16(0x80089C26, 0x3FFC);
    w_u32(0x80089C20, 3);
    SpuSetCommonAttr((SpuCommonAttr *)psx_addr(0x80089C20, sizeof(SpuCommonAttr)));
    mix.val0 = (uint8)volume;
    mix.val1 = (uint8)(volume >> 8);
    mix.val2 = (uint8)(volume >> 16);
    mix.val3 = (uint8)(volume >> 24);
    CdMix(&mix);
    for (index = 0; index < 24; ++index)
    {
        offset = index * 92;
        for (byte = 0; byte < 92; ++byte)
            w_u8(0x80091970 + offset + byte, 0);
        w_u32(0x80091970 + offset, 1 << index);
        w_u16(0x8009197C + offset, 0);
        w_u16(0x8009197E + offset, 0);
        w_u32(0x80091974 + offset, 159);
    }
    return 0;
}

sint32 td5_open_data_archive(uint32 filename)
{
    uint32 index, offset;

    FUNCTION_MARKER(0x8007B838, "SLUS_006.10");
    td5_debug_log(0x80013C88, filename);
    if (td5_use_extracted_assets())
        return 0;
    td5_load_archive_directory(filename);
    for (index = 0; index < 512; ++index)
    {
        offset = index * 40;
        w_u32(0x8009266C + offset, r_u32(0x8009266C + offset) + 10);
        if (r_u32(0x80092668 + offset) != 0)
            w_u32(0x8008816C, r_u32(0x8008816C) + 1);
    }
    return 0;
}

uint32 td5_read_861c0(void)
{
    FUNCTION_MARKER(0x80064124, "SLUS_006.10");
    return (uint32)GetVideoMode();
}

uint32 td5_load_sine_table(uint32 archive_error_index)
{
    FUNCTION_MARKER(0x80077720, "SLUS_006.10");
    return td5_load_archive_file((const char *)psx_addr(0x80013C64, 14), 0x8009B3F8, archive_error_index);
}

sint32 td5_load_menu_sounds(void)
{
    uint32 index;
    uint32 size;
    uint32 filename;
    uint32 name_length;

    FUNCTION_MARKER(0x80052840, "SLUS_006.10");
    for (index = 0; index < 16; ++index)
    {
        filename = r_u32(0x8007E9C0 + index * 4);
        name_length = 0;
        while (r_u8(filename + name_length) != 0)
            ++name_length;
        size = td5_archive_file_size((const char *)psx_addr(filename, name_length + 1), 1, index * 12, 0);
        filename = r_u32(0x8007E9C0 + index * 4);
        name_length = 0;
        while (r_u8(filename + name_length) != 0)
            ++name_length;
        td5_load_archive_file((const char *)psx_addr(filename, name_length + 1), 0x800A7A00, index * 12);
        if (index >= 10 && index <= 14)
            size += 0x2000;
        td5_upload_sound(0x800A7A00, size, 0x8008CD28 + index * 12);
    }
    return 0;
}

sint32 td5_set_sound_limit(sint32 limit)
{
    FUNCTION_MARKER(0x800542F8, "SLUS_006.10");
    if (limit > 4096)
        w_u32(0x800882D8, 4096);
    else if (limit < 0)
        w_u32(0x800882D8, 0);
    return 4096;
}

uint32 td5_upload_sound(uint32 source, uint32 file_size, uint32 description)
{
    uint32 bytes;
    uint32 transfer_bytes;
    uint32 address;
    uint32 sample_rate;

    FUNCTION_MARKER(0x800541AC, "SLUS_006.10");
    bytes = file_size - 80;
    address = (uint32)SpuMalloc((sint32)bytes);
    SpuSetTransferMode(0);
    SpuSetTransferStartAddr(address);
    transfer_bytes = bytes > 0x7EFF0 ? 0x7EFF0 : bytes;
    SpuWrite(psx_addr(source + 80, transfer_bytes), transfer_bytes);
    SpuIsTransferCompleted(1);
    w_u32(description, address);
    w_u32(description + 8, (bytes << 5) / 7);
    sample_rate = ((uint32)r_u8(source + 18) << 8) + r_u8(source + 19);
    w_u32(description + 4, sample_rate);
    return sample_rate;
}

uint32 td5_80016344(void)
{
    uint32 index;
    uint32 offset;

    FUNCTION_MARKER(0x80016344, "SLUS_006.10");
    for (index = 0; index < 47; ++index)
    {
        offset = index * 120;
        w_u32(0x8007C538 + offset, index == 44 ? 0 : UINT32_MAX);
        w_u8(0x8007C53C + offset, 0);
        w_u8(0x8007C53D + offset, 0);
        w_u8(0x8007C546 + offset, 0);
        w_u16(0x8007C548 + offset, 0);
        w_u16(0x8007C54A + offset, 0);
        w_u8(0x8007C547 + offset, 0);
    }
    return 47;
}

uint32 td5_80016ef8(void)
{
    uint32 index;

    FUNCTION_MARKER(0x80016EF8, "SLUS_006.10");
    for (index = 0; index < 47; ++index)
    {
        if (index == 44)
            td5_80017cc0(index);
        else if (index == 40)
            td5_80017f90(index);
        else
            td5_80017940(index);
    }
    return 47;
}

uint32 td5_random_for_slot(uint32 index)
{
    FUNCTION_MARKER(0x8004921C, "SLUS_006.10");
    return td5_random_step(0x8009874C + index * 864);
}

uint32 td5_random_step(uint32 state)
{
    uint32 value;

    FUNCTION_MARKER(0x8007B3F0, "SLUS_006.10");
    value = r_u32(state) * 31415821 + 1;
    w_u32(state, value);
    return (value << 24) | (value >> 8);
}

void td5_80078804(void)
{
    FUNCTION_MARKER(0x80078804, "SLUS_006.10");
}

sint32 td5_upload_tim(uint32 image, sint16 image_x, sint16 image_y, sint16 palette_x, sint16 palette_y)
{
    PSX_RECT rectangle;
    uint32 block;
    uint32 bytes;

    FUNCTION_MARKER(0x80048DB8, "SLUS_006.10");
    if (r_u32(image) != 16)
        return 16;
    block = image + 8;
    rectangle.x = palette_x;
    rectangle.y = palette_y;
    rectangle.w = (sint16)r_u16(block + 8);
    rectangle.h = (sint16)r_u16(block + 10);
    bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(block + 12, bytes));
    block += r_u32(block) & 0xFFFFFFFC;
    if (r_u32(block) != 4)
    {
        rectangle.x = image_x;
        rectangle.y = image_y;
        rectangle.w = (sint16)r_u16(block + 8);
        rectangle.h = (sint16)r_u16(block + 10);
        bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
        LoadImagePSX(&rectangle, (uint32 *)psx_addr(block + 12, bytes));
    }
    return DrawSync(0);
}

void td5_init_loading_primitive(void)
{
    uint32 page;
    uint32 texture_page;

    FUNCTION_MARKER(0x80043A2C, "SLUS_006.10");
    page = r_u32(0x8008847C);
    w_u8(0x800A403B, 9);
    w_u8(0x800A403F, 44);
    w_u8(0x800A4055, 15);
    w_u8(0x800A405D, 15);
    w_u8(0x800A404C, 239);
    w_u8(0x800A405C, 239);
    w_u16(0x800A4042, 16);
    w_u16(0x800A404A, 16);
    w_u16(0x800A4052, 31);
    w_u16(0x800A405A, 31);
    w_u8(0x800A4044, 0);
    w_u8(0x800A4045, 0);
    w_u8(0x800A404D, 0);
    w_u8(0x800A4054, 0);
    w_u16(0x800A4040, 64);
    w_u16(0x800A4048, 303);
    w_u16(0x800A4050, 64);
    w_u16(0x800A4058, 303);
    w_u8(0x800A403C, 128);
    w_u8(0x800A403D, 128);
    w_u8(0x800A403E, 0);
    page <<= 8;
    texture_page = ((page & 0x100) >> 4) | ((page & 0x200) << 2);
    w_u16(0x800A4046, (uint16)((page + 80) << 6));
    w_u16(0x800A404E, (uint16)texture_page);
    DrawPrim(psx_addr(0x800A4038, 40));
}

uint32 td5_80016fc4(void)
{
    uint32 base_speeds[47];
    uint32 pitch_table[16];
    uint32 weights[47];
    uint32 index;
    uint32 variant;
    uint32 offset;
    uint32 value;
    uint32 pitch;
    uint32 denominator;
    uint32 ratio;

    FUNCTION_MARKER(0x80016FC4, "SLUS_006.10");
    for (index = 0; index < 47; ++index)
        base_speeds[index] = r_u32(0x80010078 + index * 4);
    for (index = 0; index < 16; ++index)
        pitch_table[index] = r_u32(0x80010134 + index * 4);
    for (index = 0; index < 47; ++index)
        weights[index] = r_u32(0x80010174 + index * 4);
    w_u32(0x8009874C, 0x3AFE5F64);
    for (index = 0; index < 47; ++index)
    {
        for (variant = 0; variant < 5; ++variant)
        {
            offset = index * 120 + variant * 20;
            value = td5_random_for_slot(0);
            w_u32(0x8007C4D4 + offset, base_speeds[index] + value % 300);
            value = td5_random_for_slot(0);
            w_u8(0x8007C4E2 + offset, (uint8)(value % 14));
            value = td5_random_for_slot(0);
            pitch = pitch_table[r_u8(0x8007C4E2 + offset)] & 0xFFFF;
            w_u16(0x8007C4E6 + offset, (uint16)(pitch - value % 48));
            ratio = (uint32)((sint32)(weights[index] * 61440) / 5280);
            denominator = ((r_u32(0x8007C4D4 + offset) << 12) >> 5) / 3375;
            if (denominator == 0)
                xport_mips_break(7);
            ratio /= denominator;
            pitch = pitch_table[r_u8(0x8007C4E2 + offset)] & 0xFFFF;
            w_u16(0x8007C4E4 + offset, (uint16)(((ratio * 1252) >> 8) - 904 + pitch));
            if (index == 29)
            {
                w_u16(0x8007C4E4 + offset, (uint16)((r_u16(0x8007C4E6 + offset) >> 2) * 2));
                w_u16(0x8007C4E6 + offset, (uint16)(r_u16(0x8007C4E6 + offset) - 244));
                value = td5_random_for_slot(0);
                w_u32(0x8007C4D4 + offset, (value % 5000 + base_speeds[index]) * 24);
            }
            else if (index == 40)
            {
                value = td5_random_for_slot(0);
                w_u8(0x8007C4E3 + offset, (uint8)((value & 3) + variant * 5 + 74));
            }
            else if (index == 44)
            {
                value = td5_random_for_slot(0);
                w_u32(0x8007C4D4 + offset, value % 1000 + variant * 1200 + 58000);
            }
            else if (index >= 41)
            {
                value = td5_random_for_slot(0);
                w_u32(0x8007C4D4 + offset, base_speeds[index] + value % 6000);
                value = td5_random_for_slot(0);
                w_u8(0x8007C4E3 + offset, (uint8)(value % 33 + 10));
            }
        }
    }
    return 47;
}

uint32 td5_80017940(uint32 index)
{
    uint32 changed = 1;
    uint32 variant;
    uint32 address;
    uint32 current;
    uint32 next;
    uint32 denominator;
    uint32 saved[5];
    uint32 moved[4];
    uint32 word;

    FUNCTION_MARKER(0x80017940, "SLUS_006.10");
    while (changed != 0)
    {
        changed = 0;
        for (variant = 0; variant < 5; ++variant)
        {
            address = 0x8007C4D4 + index * 120 + variant * 20;
            current = r_u32(address) * 100;
            denominator = (uint32)(r_s32(0x800880FC) / 2);
            if (denominator == 0)
                xport_mips_break(7);
            current /= denominator;
            next = r_u32(address + 20) * 100;
            denominator = (uint32)(r_s32(0x800880FC) / 2);
            if (denominator == 0)
                xport_mips_break(7);
            next /= denominator;
            if ((sint32)next < (sint32)current)
            {
                ++changed;
                for (word = 0; word < 5; ++word)
                    saved[word] = r_u32(address + word * 4);
                for (word = 0; word < 4; ++word)
                    moved[word] = r_u32(address + 20 + word * 4);
                for (word = 0; word < 4; ++word)
                    w_u32(address + word * 4, moved[word]);
                w_u32(address + 16, r_u32(address + 36));
                for (word = 0; word < 4; ++word)
                    w_u32(address + 20 + word * 4, saved[word]);
                w_u32(address + 36, saved[4]);
            }
        }
    }
    return 0;
}

uint32 td5_80017cc0(uint32 index)
{
    uint32 changed = 1;
    uint32 variant;
    uint32 address;
    uint32 current;
    uint32 next;
    uint32 saved[5];
    uint32 moved[4];
    uint32 word;

    FUNCTION_MARKER(0x80017CC0, "SLUS_006.10");
    while (changed != 0)
    {
        changed = 0;
        for (variant = 0; variant < 5; ++variant)
        {
            address = 0x8007C4D4 + index * 120 + variant * 20;
            current = r_u32(address);
            next = r_u32(address + 20);
            if (current < next)
            {
                ++changed;
                for (word = 0; word < 5; ++word)
                    saved[word] = r_u32(address + word * 4);
                for (word = 0; word < 4; ++word)
                    moved[word] = r_u32(address + 20 + word * 4);
                for (word = 0; word < 4; ++word)
                    w_u32(address + word * 4, moved[word]);
                w_u32(address + 16, r_u32(address + 36));
                for (word = 0; word < 4; ++word)
                    w_u32(address + 20 + word * 4, saved[word]);
                w_u32(address + 36, saved[4]);
            }
        }
    }
    return 0;
}

uint32 td5_80017f90(uint32 index)
{
    uint32 changed = 1;
    uint32 variant;
    uint32 address;
    uint32 current;
    uint32 next;
    uint32 saved[5];
    uint32 moved[4];
    uint32 word;

    FUNCTION_MARKER(0x80017F90, "SLUS_006.10");
    while (changed != 0)
    {
        changed = 0;
        for (variant = 0; variant < 5; ++variant)
        {
            address = 0x8007C4D4 + index * 120 + variant * 20;
            current = r_u8(address + 15);
            next = r_u8(address + 35);
            if (current < next)
            {
                ++changed;
                for (word = 0; word < 5; ++word)
                    saved[word] = r_u32(address + word * 4);
                for (word = 0; word < 4; ++word)
                    moved[word] = r_u32(address + 20 + word * 4);
                for (word = 0; word < 4; ++word)
                    w_u32(address + word * 4, moved[word]);
                w_u32(address + 16, r_u32(address + 36));
                for (word = 0; word < 4; ++word)
                    w_u32(address + 20 + word * 4, saved[word]);
                w_u32(address + 36, saved[4]);
            }
        }
    }
    return 0;
}

uint32 td5_show_legal_image(uint32 buffer, uint32 filename, sint32 seconds, uint32 archive_error_index)
{
    uint32 pointer;
    uint32 other;
    uint32 elapsed;
    uint32 name_length;
    uint32 buttons;
    uint32 skippable;
    uint32 previous_buttons;
    uint32 pressed;
    PSX_RECT rectangle;

    FUNCTION_MARKER(0x80019EA0, "SLUS_006.10");
    VSync(0);
    DrawSync(0);
    SetDispMask(0);
    td5_prepare_display(0);
    td5_swap_display_buffer();
    w_u16(0x800976CC, r_u16(0x800876F8));
    w_u16(0x80097740, r_u16(0x800876F8));
    w_u16(0x800976CE, r_u16(0x800876FC));
    w_u16(0x80097742, r_u16(0x800876FC));
    pointer = r_u32(0x800880F0);
    other = (r_u32(0x80088104) ^ 1) * 116;
    w_u16(pointer + 2, r_u16(0x8009766A + other));
    pointer = r_u32(0x800880F0);
    other = (r_u32(0x80088104) ^ 1) * 116;
    w_u16(pointer + 10, r_u16(0x80097672 + other));
    td5_apply_display_buffer();
    name_length = 0;
    while (r_u8(filename + name_length) != 0)
        ++name_length;
    td5_load_archive_file((const char *)psx_addr(filename, name_length + 1), buffer, archive_error_index);
    rectangle.w = 320;
    rectangle.h = 256;
    rectangle.x = 0;
    rectangle.y = 0;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(buffer + 20, 320 * 256 * 2));
    rectangle.w = 320;
    rectangle.h = 256;
    rectangle.x = 0;
    rectangle.y = 256;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(buffer + 20, 320 * 256 * 2));
    DrawSync(0);
    VSync(0);
    SetDispMask(1);
    elapsed = 0;
    /* Require a fresh Cross/Start press for each startup splash */
    skippable = filename == 0x80010258 || filename == 0x80010268;
    previous_buttons = skippable ? xport_input_read(0) | xport_input_read(1) : 0;
    while ((sint32)elapsed < (sint32)((uint32)seconds * r_u32(0x800880FC)))
    {
        SetDispMask(1);
        VSync(0);
        ++elapsed;
        if (skippable)
        {
            buttons = xport_input_read(0) | xport_input_read(1);
            pressed = buttons & ~previous_buttons;
            previous_buttons = buttons;
            if ((pressed & (PADstart | PADRdown)) != 0)
                break;
        }
    }
    return elapsed;
}

uint32 td5_prepare_display(uint32 mode)
{
    uint32 pal;
    uint32 entry;
    uint32 offset;
    uint32 index;
    sint32 width;
    sint32 height;
    sint32 x;
    sint32 y;

    FUNCTION_MARKER(0x8006C59C, "SLUS_006.10");
    pal = r_u32(0x8008810C);
    w_u32(0x80088160, 4096);
    if (pal != 0)
    {
        entry = 0x80086BEC + mode * 30;
        w_u32(0x800880FC, 50);
        w_u32(0x80088100, 307);
        td5_80064110(1);
    }
    else
    {
        entry = 0x80086C64 + mode * 30;
        w_u32(0x800880FC, 60);
        w_u32(0x80088100, 256);
    }
    x = r_s16(entry);
    y = r_s16(entry + 2);
    width = r_s16(entry + 8);
    height = r_s16(entry + 10);
    SetDefDrawEnv((DRAWENV *)psx_addr(0x80097668, sizeof(DRAWENV)), x, y, width, height);
    x = r_s16(entry + 4);
    y = r_s16(entry + 6);
    width = r_s16(entry + 8);
    height = r_s16(entry + 10);
    SetDefDrawEnv((DRAWENV *)psx_addr(0x800976DC, sizeof(DRAWENV)), x, y, width, height);
    x = r_s16(entry + 4);
    y = r_s16(entry + 6);
    width = r_s16(entry + 12);
    height = r_s16(entry + 14);
    SetDefDispEnv((DISPENV *)psx_addr(0x800976C4, sizeof(DISPENV)), x, y, width, height);
    x = r_s16(entry);
    y = r_s16(entry + 2);
    width = r_s16(entry + 12);
    height = r_s16(entry + 14);
    SetDefDispEnv((DISPENV *)psx_addr(0x80097738, sizeof(DISPENV)), x, y, width, height);
    width = r_s16(entry + 8);
    height = r_s16(entry + 10);
    x = r_s16(entry + 22);
    y = r_s16(entry + 24);
    w_u32(0x800880F4, (uint32)width);
    w_u32(0x800880F8, (uint32)height);
    w_u32(0x8008815C, (uint32)x);
    w_u32(0x80088158, (uint32)y);
    for (index = 0; index < 2; ++index)
    {
        offset = index * 116;
        w_u8(0x80097680 + offset, (uint8)r_u16(entry + 26));
        w_u8(0x80097681 + offset, 0);
        w_u8(0x80097682 + offset, 0);
        w_u8(0x80097683 + offset, 0);
        w_u8(0x8009767E + offset, (uint8)r_u16(entry + 28));
        w_u16(0x800976CC + offset, r_u16(entry + 16));
        w_u16(0x800976CE + offset, r_u16(entry + 18));
        w_u16(0x800976D0 + offset, r_u16(entry + 20));
        w_u16(0x800976D2 + offset, r_u16(entry + 10));
    }
    width = r_s32(0x800880F4);
    height = r_s32(0x800880F8);
    w_u32(0x80088114, mode);
    td5_set_view_center(width / 2, height / 2);
    return td5_8007880c();
}

uint32 td5_swap_display_buffer(void)
{
    uint32 index;

    FUNCTION_MARKER(0x8006C868, "SLUS_006.10");
    index = r_u32(0x80088104) ^ 1;
    w_u32(0x80088104, index);
    w_u32(0x800880F0, 0x80097668 + index * 116);
    return 0x80097668;
}

uint32 td5_apply_display_buffer(void)
{
    uint32 count;

    FUNCTION_MARKER(0x8006C7C8, "SLUS_006.10");
    DrawSync(0);
    PutDrawEnv((DRAWENV *)psx_addr(r_u32(0x800880F0), sizeof(DRAWENV)));
    PutDispEnv((DISPENV *)psx_addr(r_u32(0x800880F0) + 92, sizeof(DISPENV)));
    count = r_u32(0x80088108) + 1;
    w_u32(0x80088108, count);
    return count;
}

void td5_set_view_center(sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x80078AFC, "SLUS_006.10");
    SetGeomOffset(x, y);
    w_u32(0x80088148, (uint32)x);
    w_u32(0x8008814C, (uint32)y);
}

uint32 td5_integer_sqrt(uint32 value)
{
    uint32 estimate = 0;
    uint32 bit;
    uint32 trial;

    FUNCTION_MARKER(0x800782A0, "SLUS_006.10");
    if ((sint32)value > 0xFFFF)
        bit = (sint32)value > 0xFFFFFF ? 0x40000000 : 0x400000;
    else
        bit = 0x4000;
    while (bit > 1)
    {
        trial = estimate + bit;
        if (value >= trial)
        {
            value -= trial;
            estimate = (trial + bit) >> 1;
        }
        else
            estimate >>= 1;
        bit >>= 2;
    }
    if (value >= estimate + 1)
        estimate += 2;
    return estimate >> 1;
}

/* Native arithmetic helper preserves the original explicit BREAK paths */
sint32 td5_divide_checked(sint32 numerator, sint32 denominator)
{
    if (denominator == 0)
        xport_mips_break(7);
    if (denominator == -1 && numerator == INT32_MIN)
        xport_mips_break(6);
    return numerator / denominator;
}

uint32 td5_8007880c(void)
{
    uint32 scale;
    uint32 x_product;
    uint32 y_product;
    sint32 half_width;
    sint32 half_height;
    uint32 distance;
    uint32 distance_squared;
    uint32 distance_fixed;
    sint32 root;
    sint32 square;
    sint32 ratio;
    sint32 offset;
    sint32 divisor;
    sint32 horizontal;
    sint32 vertical;
    uint32 centered;

    FUNCTION_MARKER(0x8007880C, "SLUS_006.10");
    x_product = r_u32(0x800880F4) * r_u32(0x8008815C);
    ratio = (sint32)x_product / 4096;
    scale = r_u32(0x80088160);
    x_product = (uint32)ratio * scale;
    if ((sint32)x_product < 0)
        x_product += 4095;
    y_product = r_u32(0x800880F8) * r_u32(0x80088158);
    half_width = (sint32)x_product >> 12;
    ratio = (sint32)y_product / 4096;
    y_product = (uint32)ratio * scale;
    if ((sint32)y_product < 0)
        y_product += 4095;
    distance = r_u32(0x80088144);
    square = (sint32)((uint32)half_width * (uint32)half_width);
    half_height = (sint32)y_product >> 12;
    distance_squared = distance * distance;
    root = (sint32)td5_integer_sqrt((uint32)(square / 4) + distance_squared);
    distance_fixed = distance << 12;
    ratio = td5_divide_checked((sint32)distance_fixed, root);
    centered = (uint32)((sint32)((uint32)half_width + (x_product >> 31)) >> 1);
    offset = td5_divide_checked((sint32)(0 - (centered << 12)), root);
    divisor = r_s32(0x80088160);
    horizontal = td5_divide_checked((sint32)((uint32)ratio << 12), divisor);
    vertical = td5_divide_checked((sint32)((uint32)offset << 12), divisor);
    square = (sint32)((uint32)half_height * (uint32)half_height);
    w_u16(0x80088492, 0);
    w_u16(0x80088490, (uint16)horizontal);
    w_u16(0x80088494, (uint16)vertical);
    root = (sint32)td5_integer_sqrt((uint32)(square / 4) + distance_squared);
    ratio = td5_divide_checked((sint32)distance_fixed, root);
    centered = (uint32)((sint32)((uint32)half_height + (y_product >> 31)) >> 1);
    offset = td5_divide_checked((sint32)(0 - (centered << 12)), root);
    divisor = r_s32(0x80088160);
    horizontal = td5_divide_checked((sint32)((uint32)ratio << 12), divisor);
    vertical = td5_divide_checked((sint32)((uint32)offset << 12), divisor);
    w_u16(0x800884A8, 0);
    w_u16(0x800884AA, (uint16)horizontal);
    w_u16(0x800884AC, (uint16)vertical);
    return (uint32)vertical;
}

void td5_800b789c(sint32 selection)
{
    uint32 source;
    uint32 destination;
    uint32 words[4];
    uint32 group;
    uint32 word;

    FUNCTION_MARKER(0x800B789C, "SLUS_006.10");
    if (r_u32(0x8008771C) == 3 || r_u32(0x80087724) != 0 || r_u32(0x80087720) == 4)
    {
        switch (selection)
        {
            case 25:
                source = 0x800BC2F0;
                break;
            case 33:
                source = 0x800BC318;
                break;
            case 34:
                source = 0x800BC340;
                break;
            case 35:
                source = 0x800BC368;
                break;
            default:
                return;
        }
    }
    else
        source = 0x800BC2C8;
    destination = 0x8007E394;
    for (group = 0; group < 2; ++group)
    {
        for (word = 0; word < 4; ++word)
            words[word] = r_u32(source + word * 4);
        for (word = 0; word < 4; ++word)
            w_u32(destination + word * 4, words[word]);
        source += 16;
        destination += 16;
    }
    words[0] = r_u32(source);
    words[1] = r_u32(source + 4);
    w_u32(destination, words[0]);
    w_u32(destination + 4, words[1]);
}

// TODO: Replace memory-card startup and UI integration with xport_save/load
sint32 td5_80055b3c(void)
{
    static const sint32 specifications[4] = {4, 0x8000, 0x100, 0x2000};
    uint32 index;
    uint32 event_class;
    sint32 handle;
    sint32 result;

    FUNCTION_MARKER(0x80055B3C, "SLUS_006.10");
    if (!td5_memory_card_enabled)
    {
        td5_init_pad();
        return 0;
    }
    for (index = 0; index < 15; ++index)
        VSync(0);
    for (index = 0; index < 8; ++index)
    {
        event_class = index < 4 ? 0xF4000001 : 0xF0000011;
        handle = OpenEvent(event_class, specifications[index & 3], 0x2000, 0);
        w_u32(0x800882F8 + index * 4, (uint32)handle);
    }
    InitCARD(0);
    for (index = 0; index < 15; ++index)
        VSync(0);
    StartCARD();
    for (index = 0; index < 16; ++index)
        VSync(0);
    _bu_init();
    for (index = 0; index < 16; ++index)
        VSync(0);
    td5_init_pad();
    for (index = 0; index < 11; ++index)
        VSync(0);
    result = 0;
    for (index = 0; index < 8; ++index)
        result = EnableEvent(r_s32(0x800882F8 + index * 4));
    return result;
}

void td5_init_pad(void)
{
    uint32 index;

    FUNCTION_MARKER(0x8006B470, "SLUS_006.10");
    PadInitDirect((uint8 *)psx_addr(0x8009D9D8, 34), (uint8 *)psx_addr(0x8009D9FA, 34));
    PadStartCom();
    for (index = 0; index < 8; ++index)
        w_u8(0x80088688 + index, 0);
}

void td5_wait_loading_screen(void)
{
    uint32 elapsed, buttons, previous;
    uint32 fields = r_u32(0x800880FC) * 4;

    previous = (xport_input_read(0) | xport_input_read(1)) & (PADRdown | PADstart);
    for (elapsed = 0; elapsed < fields; ++elapsed)
    {
        VSync(0);
        buttons = (xport_input_read(0) | xport_input_read(1)) & (PADRdown | PADstart);
        if ((buttons & ~previous) != 0)
            break;
        previous = buttons;
    }
}

void td5_fade_audio(void)
{
    uint32 initial;
    uint32 current;
    uint32 volume;

    FUNCTION_MARKER(0x80054108, "SLUS_006.10");
    initial = r_u32(0x80087700);
    current = initial;
    while ((sint32)current >= 0)
    {
        w_u32(0x80087700, current - 64);
        td5_update_cd_volume();
        volume = r_u32(0x80087700) << 2;
        w_u16(0x800920A8, (uint16)volume);
        w_u16(0x800920AA, (uint16)volume);
        SpuSetVoiceAttr((SpuVoiceAttr *)psx_addr(0x800920A0, sizeof(SpuVoiceAttr)));
        VSync(0);
        current = r_u32(0x80087700);
    }
    td5_800557c0();
    td5_stop_sound_voice(20);
    w_u32(0x80087700, initial);
}

sint32 td5_update_cd_volume(void)
{
    sint32 volume;
    uint32 initial;
    CdlATV mix;

    FUNCTION_MARKER(0x800556AC, "SLUS_006.10");
    volume = r_s32(0x80087700);
    initial = r_u32(0x80087BEC);
    volume /= 16;
    if (volume < 0)
        volume = 0;
    else if (volume > 255)
        volume = 255;
    mix.val0 = (uint8)volume;
    mix.val1 = (uint8)(initial >> 8);
    mix.val2 = (uint8)volume;
    mix.val3 = (uint8)(initial >> 24);
    CdMix(&mix);
    return 1;
}

sint32 td5_800557c0(void)
{
    uint8 mode = 128;

    FUNCTION_MARKER(0x800557C0, "SLUS_006.10");
#if AP_WEB
    CdControl(CdlStop, 0, 0);
#endif
    return CdControlB(14, &mode, 0);
}

void td5_stop_sound_voice(uint32 voice)
{
    FUNCTION_MARKER(0x80054288, "SLUS_006.10");
    SpuSetKey(0, (uint32)1 << (voice & 31));
    if (r_u32(0x80088668) != 0)
        SpuSetKey(0, (uint32)1 << ((voice + 1) & 31));
}

sint32 td5_play_xa(sint32 track)
{
    uint32 duration;
    uint32 filename;
    sint32 channel;
    sint32 sector;
    sint32 volume;
    uint32 initial;
    CdlFILE file;
    CdlLOC position;
    uint8 mode;
    uint8 filter[2];
    CdlATV mix;

    FUNCTION_MARKER(0x800558C8, "SLUS_006.10");
#if AP_WEB
    {
        char path[64];
        sint32 result;
        SpuCommonAttr sound = {0};
        w_u8(0x800884F4, (uint8)track);
        sound.mask = SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR | SPU_COMMON_CDMIX;
        sound.cd.volume.left = sound.cd.volume.right = 0x3FFF;
        sound.cd.mix = SPU_ON;
        SpuSetCommonAttr(&sound);
        snprintf(path, sizeof(path), "MUSIC/%s_%d.XA", track < 8 ? "PRES" : "GAME",
            track < 8 ? track : track - 8);
        result = xport_xa_play_file(path, 1);
        if (!result)
        {
            xport_message_error("Test Drive 5 - Music error", path);
            exit(EXIT_FAILURE);
        }
        CdControl(CdlDemute, 0, 0);
        td5_update_cd_volume();
        return result;
    }
#endif
    duration = r_u32(0x8007EA20 + (uint32)track * 4);
    w_u8(0x800884F4, (uint8)track);
    duration <<= 3;
    channel = track;
    if (track < 8)
        filename = r_u32(0x80087BE4);
    else
    {
        filename = r_u32(0x80087BE8);
        channel = (sint32)((uint32)track - 8);
    }
    CdInit();
    if (td5_mount_xa_file(filename, &file) == 0)
    {
        td5_debug_log(0x80011418, filename);
        return 0;
    }
    sector = CdPosToInt(&file.pos);
    mode = 0xCC;
    CdControlB(14, &mode, 0);
    w_u32(0x800882EC, (uint32)sector + duration);
    w_u32(0x800882E8, (uint32)sector);
    w_u32(0x800882F4, (uint32)sector);
    w_u32(0x800882F0, (uint32)channel);
    filter[0] = 1;
    filter[1] = (uint8)channel;
    CdControlF(13, filter);
    CdIntToPos(r_s32(0x800882E8), &position);
    CdControlF(27, (uint8 *)&position);
    volume = r_s32(0x80087700);
    initial = r_u32(0x80087BEC);
    volume /= 16;
    if (volume < 0)
        volume = 0;
    else if (volume > 255)
        volume = 255;
    mix.val0 = (uint8)volume;
    mix.val1 = (uint8)(initial >> 8);
    mix.val2 = (uint8)volume;
    mix.val3 = (uint8)(initial >> 24);
    CdMix(&mix);
    return 1;
}

void td5_80013cd8(void)
{
    FUNCTION_MARKER(0x80013CD8, "SLUS_006.10");
}

sint32 td5_upload_presentation_image(void)
{
    uint32 image;
    uint32 index;
    PSX_RECT rectangle;

    FUNCTION_MARKER(0x80057AE0, "SLUS_006.10");
    image = r_u32(0x80088608);
    index = r_u32(0x80088104);
    rectangle.h = 256;
    rectangle.w = 512;
    rectangle.x = 0;
    rectangle.y = (sint16)(index << 8);
    return LoadImagePSX(&rectangle, (uint32 *)psx_addr(image + 20, 512 * 256 * 2));
}

sint32 td5_80053030(void)
{
    FUNCTION_MARKER(0x80053030, "SLUS_006.10");
    return td5_8005490c(0, 23, 4096, 0, 4096);
}

sint32 td5_8005490c(sint32 sample, sint32 voice, sint32 volume, sint32 pan, sint32 pitch)
{
    FUNCTION_MARKER(0x8005490C, "SLUS_006.10");
    return td5_play_sound(sample, voice, volume, pan, pitch);
}

sint32 td5_800530f0(void)
{
    FUNCTION_MARKER(0x800530F0, "SLUS_006.10");
    return td5_8005490c(1, 23, 4096, 0, 5120);
}

sint32 td5_80053064(void)
{
    FUNCTION_MARKER(0x80053064, "SLUS_006.10");
    return td5_8005490c(0, 23, 4096, 0, 6144);
}

sint32 td5_play_sound(sint32 sample, sint32 voice, sint32 volume, sint32 pan, sint32 pitch)
{
    uint32 voice_offset;
    uint32 sample_offset;
    uint32 rate;
    uint32 product;
    sint32 normalized;
    sint32 ratio;
    sint32 frequency;

    FUNCTION_MARKER(0x80054720, "SLUS_006.10");
    if (voice == -1)
    {
        voice = td5_find_sound_voice();
        if (voice == -1)
            return -1;
    }
    voice_offset = (uint32)voice * 92;
    w_u32(0x800919B0 + voice_offset, (uint32)sample);
    w_u32(0x800919C4 + voice_offset, 0);
    w_u32(0x800919BC + voice_offset, (uint32)pitch);
    if (volume != -1)
        w_u32(0x800919B8 + voice_offset, (uint32)volume);
    else
        volume = r_s32(0x800919B8 + voice_offset);
    sample_offset = (uint32)sample * 12;
    rate = r_u32(0x8008CD2C + sample_offset);
    product = rate * (uint32)pitch;
    normalized = (sint32)product / 4096;
    ratio = td5_divide_checked(44100, (sint32)rate);
    product = ((uint32)normalized * (uint32)ratio) << 12;
    frequency = (sint32)product / 44100;
    if (frequency >= 16384)
        frequency = 16383;
    SpuSetKey(0, r_u32(0x80091970 + voice_offset));
    w_u32(0x8009198C + voice_offset, r_u32(0x8008CD28 + sample_offset));
    td5_update_sound_voice(voice, frequency, pan, volume);
    SpuSetKey(1, r_u32(0x80091970 + voice_offset));
    return voice;
}

void td5_update_sound_voice(sint32 voice, sint32 pitch, sint32 pan, sint32 volume)
{
    uint32 offset;
    uint32 sample;
    uint32 product;
    sint32 normalized;
    sint32 frequency;
    sint32 delta;
    uint32 left;
    uint32 right;

    FUNCTION_MARKER(0x800553DC, "SLUS_006.10");
    offset = (uint32)voice * 92;
    sample = r_u32(0x800919B0 + offset);
    product = r_u32(0x8008CD2C + sample * 12) * (uint32)pitch;
    normalized = (sint32)product / 4096;
    frequency = (sint32)((uint32)normalized << 12) / 44100;
    if (frequency >= 16384)
        frequency = 16383;
    w_u16(0x80091984 + offset, (uint16)frequency);
    if (pan < 0)
    {
        if (pan < -4096)
            pan = -4096;
        left = (uint32)volume;
        delta = (sint32)((uint32)volume * (uint32)pan) / 4096;
        right = (uint32)volume + (uint32)delta;
    }
    else
    {
        if (pan > 4096)
            pan = 4096;
        delta = (sint32)((uint32)volume * (uint32)pan) / 4096;
        left = (uint32)volume - (uint32)delta;
        right = (uint32)volume;
    }
    w_u16(0x80091978 + offset, (uint16)(left << 2));
    w_u16(0x8009197A + offset, (uint16)(right << 2));
    SpuSetVoiceAttr((SpuVoiceAttr *)psx_addr(0x80091970 + offset, sizeof(SpuVoiceAttr)));
}

sint32 td5_find_sound_voice(void)
{
    uint32 stereo;
    uint32 cursor;
    uint32 step;
    uint32 index;
    uint32 limit;

    FUNCTION_MARKER(0x800552AC, "SLUS_006.10");
    stereo = r_u32(0x80088668);
    cursor = r_u32(0x80087BE0);
    if ((sint32)cursor < 24)
        cursor = 24;
    if (stereo != 0)
    {
        step = 2;
        cursor &= 0xFFFE;
    }
    else
        step = 1;
    w_u32(0x80087BE0, cursor);
    if (r_u32(0x80088668) != 0)
        return -1;
    index = r_u32(0x80087BE0);
    if ((sint32)index < 24)
    {
        do
        {
            if (r_u32(0x800919B4 + index * 92) == 0)
            {
                w_u32(0x80087BE0, index + 1);
                return (sint32)index;
            }
            index += step;
        } while ((sint32)index < 24);
    }
    limit = r_u32(0x80087BE0);
    if ((sint32)limit > 0)
    {
        index = 0;
        do
        {
            if (r_u32(0x800919B4 + index * 92) == 0)
            {
                w_u32(0x80087BE0, index + 1);
                return (sint32)index;
            }
            index += step;
        } while ((sint32)index < (sint32)limit);
    }
    return -1;
}

uint32 td5_init_memory_pool(uint32 block, uint32 bytes)
{
    uint32 context;
    uint32 pointer;

    FUNCTION_MARKER(0x8007B41C, "SLUS_006.10");
    context = r_u32(0x80088168);
    w_u32(context, block);
    w_u32(block, block);
    pointer = r_u32(context);
    w_u32(pointer + 4, bytes >> 4);
    pointer = r_u32(context);
    w_u32(pointer + 8, 0x12345678);
    w_u32(context + 4, bytes);
    w_u32(context + 8, bytes);
    return 0x12345678;
}

uint32 td5_alloc_memory(uint32 bytes)
{
    uint32 context;
    uint32 previous;
    uint32 block;
    uint32 units;
    uint32 allocation_bytes;
    uint32 available;
    uint32 remaining;

    FUNCTION_MARKER(0x8007B454, "SLUS_006.10");
    context = r_u32(0x80088168);
    bytes += 15;
    previous = r_u32(context);
    units = (bytes >> 4) + 1;
    allocation_bytes = units << 4;
    block = r_u32(previous);
    for (;;)
    {
        available = r_u32(block + 4);
        if ((sint32)available >= (sint32)units)
            break;
        context = r_u32(0x80088168);
        context = r_u32(context);
        previous = block;
        if (block == context)
        {
            for (;;)
            {
                SetDispMask(0);
                SetDispMask(1);
            }
        }
        block = r_u32(block);
    }
    remaining = available - units;
    if (available == units)
        w_u32(previous, r_u32(block));
    else
    {
        w_u32(block + 4, remaining);
        block += remaining << 4;
        w_u32(block + 4, units);
    }
    context = r_u32(0x80088168);
    available = r_u32(context + 8);
    w_u32(context, previous);
    w_u32(context + 8, available - allocation_bytes);
    w_u32(block + 8, 0x12345678);
    return block + 16;
}

uint32 td5_init_ordering_tables(uint32 count)
{
    uint32 bytes;
    uint32 table;
    sint32 entries;

    FUNCTION_MARKER(0x8006C514, "SLUS_006.10");
    DrawSync(0);
    bytes = count << 2;
    w_u32(0x8008857C, count);
    table = td5_alloc_memory(bytes);
    entries = r_s32(0x8008857C);
    w_u32(0x800976D8, table);
    ClearOTagR((uint32 *)psx_addr(table, entries > 0 ? (uint32)entries * 4 : 4), entries);
    table = td5_alloc_memory(bytes);
    entries = r_s32(0x8008857C);
    w_u32(0x8009774C, table);
    ClearOTagR((uint32 *)psx_addr(table, entries > 0 ? (uint32)entries * 4 : 4), entries);
    return table;
}

void td5_8006c590(uint32 value)
{
    FUNCTION_MARKER(0x8006C590, "SLUS_006.10");
    w_u32(0x80088110, value);
}

uint32 td5_set_projection_distance(sint32 distance)
{
    FUNCTION_MARKER(0x80078B38, "SLUS_006.10");
    SetGeomScreen(distance);
    w_u32(0x80088144, (uint32)distance);
    return td5_8007880c();
}

void td5_80078b6c(uint32 first, uint32 second)
{
    FUNCTION_MARKER(0x80078B6C, "SLUS_006.10");
    w_u32(0x8008813C, first);
    w_u32(0x80088140, second);
}

void td5_set_back_color(sint32 red, sint32 green, sint32 blue)
{
    FUNCTION_MARKER(0x80078C98, "SLUS_006.10");
    SetBackColor((sint32)((uint32)red << 1), (sint32)((uint32)green << 1), (sint32)((uint32)blue << 1));
}

uint32 td5_set_light(uint32 index, uint32 vector, uint32 red, uint32 green, uint32 blue)
{
    uint32 result;

    FUNCTION_MARKER(0x80078CC0, "SLUS_006.10");
    w_u16(0x8009D840 + index * 6, r_u16(vector));
    w_u16(0x8009D842 + index * 6, r_u16(vector + 2));
    w_u16(0x8009D844 + index * 6, r_u16(vector + 4));
    w_u16(0x8009EF90 + index * 2, (uint16)(red << 5));
    result = 0x8009EF96 + index * 2;
    w_u16(result, (uint16)(green << 5));
    w_u16(0x8009EF9C + index * 2, (uint16)(blue << 5));
    return result;
}

uint32 td5_set_fog(sint32 near_distance, sint32 far_distance, sint32 red, sint32 green, sint32 blue)
{
    FUNCTION_MARKER(0x80078D8C, "SLUS_006.10");
    SetFarColor(red, green, blue);
    return SetFogNearFar(near_distance, far_distance, r_s32(0x80088144));
}

void td5_set_texture_table(uint32 table)
{
    FUNCTION_MARKER(0x8006C8E0, "SLUS_006.10");
    w_u32(0x800885D8, table);
}

uint32 td5_find_texture(const char *name)
{
    uint32 entry;

    FUNCTION_MARKER(0x8006C8EC, "SLUS_006.10");
    entry = r_u32(0x800885D8);
    for (;;)
    {
        if (strncmp((const char *)psx_addr(entry, 8), name, 8) == 0)
            return entry;
        if (r_u8(entry) == 0)
        {
            td5_debug_log(0x80013C48, name);
            return entry - 16;
        }
        entry += 16;
    }
}

uint32 td5_clear_ordering_table(void)
{
    uint32 environment;
    sint32 count;
    uint32 table;

    FUNCTION_MARKER(0x8006C79C, "SLUS_006.10");
    environment = r_u32(0x800880F0);
    count = r_s32(0x8008857C);
    table = r_u32(environment + 112);
    ClearOTagR((uint32 *)psx_addr(table, count > 0 ? (uint32)count * 4 : 4), count);
    return table;
}

sint32 td5_set_view_position(uint32 position)
{
    sint32 result;

    FUNCTION_MARKER(0x80078C18, "SLUS_006.10");
    w_u32(0x8009D93C, (uint32)(r_s32(position) >> 8));
    w_u32(0x8009D940, (uint32)(r_s32(position + 4) >> 8));
    w_u32(0x8009D944, (uint32)(r_s32(position + 8) >> 8));
    w_u32(0x8009765C, (uint32)(r_s32(position) >> 8));
    w_u32(0x80097660, (uint32)(r_s32(position + 4) >> 8));
    result = r_s32(position + 8) >> 8;
    w_u32(0x80097664, (uint32)result);
    return result;
}

void td5_apply_light_matrix(uint32 matrix)
{
    MATRIX result;

    FUNCTION_MARKER(0x80078D48, "SLUS_006.10");
    MulMatrix0((MATRIX *)psx_addr(0x8009D840, sizeof(MATRIX)), (MATRIX *)psx_addr(matrix, sizeof(MATRIX)), &result);
    SetLightMatrix(&result);
    SetColorMatrix((MATRIX *)psx_addr(0x8009EF90, sizeof(MATRIX)));
}

sint32 td5_reset_timer(sint32 timer)
{
    FUNCTION_MARKER(0x8007AB08, "SLUS_006.10");
    if (timer == 0)
        return ResetRCnt(0xF2000002);
    if (timer == 1)
        return ResetRCnt(0xF2000001);
    return 1;
}

void td5_draw_ordering_table(void)
{
    uint32 environment;
    uint32 count;
    uint32 table;

    FUNCTION_MARKER(0x8006C8A4, "SLUS_006.10");
    DrawSync(0);
    environment = r_u32(0x800880F0);
    count = r_u32(0x8008857C);
    table = r_u32(environment + 112);
    DrawOTag((uint32 *)psx_addr(table + (count << 2) - 4, 4));
}

uint32 td5_poll_pads(void)
{
    uint32 port;
    uint32 packet;
    uint32 offset;
    uint32 held_address;
    uint32 pressed_address;
    uint32 previous_address;
    uint32 actuator;
    uint32 type;
    uint32 held;
    uint32 previous;
    uint32 axis;
    sint32 state;

    FUNCTION_MARKER(0x8006B560, "SLUS_006.10");
    for (port = 0; port < 2; ++port)
    {
        offset = port * 4;
        packet = 0x8009D9D8 + port * 34;
        held_address = 0x800885D0 + port * 2;
        pressed_address = 0x800884E8 + port * 2;
        previous_address = 0x800880E8 + offset;
        type = r_u16(packet) & 0xF0FF;
        w_u32(0x80088450 + offset, type);
        if (type == 0x4000 || type == 0x5000 || type == 0x7000)
        {
            actuator = 0x80088688 + offset;
            state = PadGetState((sint32)(port * 16));
            if (state == 1)
                w_u8(actuator + 2, 0);
            if (r_u8(actuator + 2) == 0)
            {
                PadSetAct((sint32)(port * 16), (uint8 *)psx_addr(actuator, 2), 2);
                if (state == 2 || (state == 6 && PadSetActAlign((sint32)(port * 16), (uint8 *)psx_addr(0x800880E0, 6)) != 0))
                    w_u8(actuator + 2, 1);
            }
            if (type == 0x4000)
            {
                held = ~(uint32)r_u16(packet + 2);
                previous = r_u32(previous_address);
                w_u16(pressed_address, (uint16)(~previous & held));
                w_u16(held_address, (uint16)held);
                w_u32(previous_address, held);
            }
            else
            {
                w_u32(0x800886A0 + offset, r_u16(packet + 6) & 255);
                held = ~(uint32)r_u16(packet + 2);
                previous = r_u32(previous_address);
                w_u16(pressed_address, (uint16)(~previous & held));
                w_u16(held_address, (uint16)held);
                w_u32(previous_address, held);
                if (r_u32(0x80088380 + offset) != 0)
                {
                    if (((uint32)r_s16(held_address) & r_u32(0x800887B0 + offset)) != 0)
                        w_u32(0x800886A8 + offset, 256);
                    else
                        w_u32(0x800886A8 + offset, 0);
                    if (((uint32)r_s16(held_address) & r_u32(0x80088380 + offset)) != 0)
                        w_u32(0x800886B0 + offset, 256);
                    else
                        w_u32(0x800886B0 + offset, 0);
                }
                else
                {
                    axis = r_u16(packet + 4) >> 8;
                    if (axis >= 128)
                    {
                        w_u32(0x800886A8 + offset, 0);
                        w_u32(0x800886B0 + offset, (axis - 127) << 1);
                    }
                    else
                    {
                        w_u32(0x800886A8 + offset, (128 - axis) << 1);
                        w_u32(0x800886B0 + offset, 0);
                    }
                }
            }
        }
        else if (type == 0x2000)
        {
            w_u32(0x800886A0 + offset, r_u16(packet + 4) & 255);
            w_u32(0x800886A8 + offset, r_u16(packet + 4) >> 8);
            w_u32(0x800886B0 + offset, r_u16(packet + 6) & 255);
            held = ~(uint32)r_u16(packet + 2);
            if (r_s32(0x800886A8 + offset) >= 65)
                held |= 0x4000;
            if ((r_u16(packet + 6) >> 8) >= 65)
                held |= 0x400;
            previous = r_u32(previous_address);
            w_u16(pressed_address, (uint16)(~previous & held));
            w_u16(held_address, (uint16)held);
            w_u32(previous_address, held);
        }
        else
        {
            w_u16(held_address, 0);
            w_u16(pressed_address, 0);
        }
    }
    return 0;
}

uint32 td5_load_archive_file(const char *filename, uint32 destination, uint32 missing_index)
{
    uint32 selected = missing_index;
    uint32 index, entry, entry_length, offset, sectors;
    sint32 found = 0;

    FUNCTION_MARKER(0x8007BA58, "SLUS_006.10");
    if (td5_use_extracted_assets())
        return td5_read_extracted_file(filename, destination);
    if (r_s32(0x8008816C) > 0)
    {
        entry = 0x80092648;
        for (index = 0; (sint32)index < r_s32(0x8008816C); ++index, entry += 40)
        {
            entry_length = 0;
            while (r_u8(entry + entry_length) != 0)
                ++entry_length;
            if (strcmp(filename, (const char *)psx_addr(entry, entry_length + 1)) == 0)
            {
                selected = index;
                found = 1;
                break;
            }
        }
    }
    if (found == 0)
    {
        td5_debug_log(0x80013C74, filename);
        xport_message_error("Test Drive 5 - File error", filename);
        exit(EXIT_FAILURE);
    }
    offset = selected * 40;
    sectors = (r_u32(0x80092668 + offset) + 2047) >> 11;
    td5_read_archive_sectors(r_u32(0x8009266C + offset), sectors, destination);
    return r_u32(0x80092668 + selected * 40);
}

uint32 td5_archive_file_size(const char *filename, sint32 enabled, uint32 missing_index, uint32 prior_result)
{
    uint32 selected = missing_index;
    uint32 index = 0;
    uint32 entry;
    uint32 entry_length;
    sint32 found = 0;

    FUNCTION_MARKER(0x8007BD44, "SLUS_006.10");
    if (enabled == 0)
        return prior_result;
    if (td5_use_extracted_assets())
        return td5_extracted_file_size(filename);
    if (r_s32(0x8008816C) > 0)
    {
        entry = 0x80092648;
        for (;;)
        {
            entry_length = 0;
            while (r_u8(entry + entry_length) != 0)
                ++entry_length;
            if (strcmp(filename, (const char *)psx_addr(entry, entry_length + 1)) == 0)
            {
                selected = index;
                found = 1;
                break;
            }
            ++index;
            if ((sint32)index >= r_s32(0x8008816C))
                break;
            entry += 40;
        }
    }
    if (found == 0)
        td5_debug_log(0x80013C74, filename);
    return r_u32(0x80092668 + selected * 40);
}

uint32 td5_read_card_icon(const char *path, uint32 slot, uint32 block, uint32 invalid_header_index)
{
    sint32 descriptor;
    uint32 slot_offset;
    uint32 offset;
    uint32 buffer;
    uint32 texture;
    uint32 page;
    uint32 u;
    uint32 v;
    uint32 clut;
    uint32 error_offset;
    PSX_RECT rectangle;
    char name[32];

    FUNCTION_MARKER(0x800563C8, "SLUS_006.10");
    descriptor = openPSX(path, 1);
    if (descriptor < 0)
        return 0;
    slot_offset = slot * 4396;
    offset = slot_offset + block * 292;
    buffer = 0x8009FD18 + offset;
    if (readPSX(descriptor, psx_addr(buffer, 256), 256) != 256)
    {
        closePSX(descriptor);
        return 0;
    }
    closePSX(descriptor);
    if (r_u8(buffer) != 'S' || r_u8(buffer + 1) != 'C')
    {
        sprintf((char *)psx_addr(buffer + 4, 252), (const char *)psx_addr(0x80087BF4, 7));
        error_offset = (uint32)(sint32)(sint16)invalid_header_index * 292 + slot_offset;
        w_u16(0x8009FE38 + offset, 0);
        w_u8(0x8009FE18 + error_offset + block, 0);
        w_u8(0x8009FE2E + error_offset + block, 0);
        return 0;
    }
    sprintf(name, (const char *)psx_addr(0x80087BFC, 7), (sint32)slot, block + 1);
    texture = td5_find_texture(name);
    page = r_u16(texture + 8);
    u = r_u8(texture + 14);
    v = r_u8(texture + 15);
    clut = r_u16(texture + 10);
    rectangle.x = (sint16)((clut & 63) << 4);
    rectangle.y = (sint16)((clut >> 6) & 255);
    rectangle.w = 16;
    rectangle.h = 1;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(buffer + 96, 32));
    DrawSync(0);
    rectangle.x = (sint16)(((page << 6) & 1023) + (u >> 2));
    rectangle.y = (sint16)(((page << 4) & 256) + ((page >> 2) & 512) + v);
    rectangle.w = 4;
    rectangle.h = 16;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(buffer + 128, 128));
    DrawSync(0);
    return 1;
}

uint32 td5_scan_card(uint32 slot, uint32 invalid_header_index)
{
    uint32 slot_offset = slot * 4396;
    uint32 block = 0;
    uint32 byte_offset = 0;
    uint32 head = invalid_header_index;
    uint32 offset;
    uint32 index;
    uint32 prefix;
    uint32 word;
    uint32 tail;
    uint32 destination;
    sint32 bytes;
    sint32 blocks;
    sint32 continuation;
    uint8 record[40];
    char path[40];
    void *entry;

    FUNCTION_MARKER(0x80056640, "SLUS_006.10");
    for (index = 0; index < 15; ++index)
    {
        offset = slot_offset + index * 292;
        w_u8(0x8009FE18 + offset, 0);
        w_u8(0x8009FE2E + offset, 0);
        w_u16(0x8009FE38 + offset, 0);
        w_u16(0x8009FE3A + offset, (uint16)index);
    }
    entry = firstfile((const char *)psx_addr(slot == 0 ? 0x80087C04 : 0x80087C0C, 7), record);
    while (entry != 0)
    {
        prefix = slot == 0 ? 0x80087C14 : 0x80087C1C;
        word = r_u32(prefix);
        tail = r_u16(prefix + 4);
        for (index = 0; index < 4; ++index)
            path[index] = (char)(word >> (index * 8));
        path[4] = (char)tail;
        path[5] = (char)(tail >> 8);
        strcat(path, (const char *)record);
        if (strncmp((const char *)record, (const char *)psx_addr(0x80011450, 12), 12) == 0)
        {
            td5_read_card_icon(path, slot, block, head);
            offset = slot_offset + byte_offset;
            for (index = 0; index < 8; ++index)
                w_u8(0x8009FE2E + offset + index, record[12 + index]);
            w_u8(0x8009FE36 + offset, 0);
            index = 0;
            do
            {
                w_u8(0x8009FE18 + offset + index, record[index]);
            } while (record[index++] != 0);
            byte_offset += 292;
            w_u16(0x8009FE3A + offset, (uint16)block);
            w_u16(0x8009FE38 + offset, 1);
            ++block;
        }
        else
        {
            memcpy(&bytes, record + 24, 4);
            blocks = bytes / 8192;
            td5_read_card_icon(path, slot, block, head);
            offset = slot_offset + byte_offset;
            w_u16(0x8009FE38 + offset, 2);
            w_u16(0x8009FE3A + offset, (uint16)block);
            index = 0;
            do
            {
                w_u8(0x8009FE18 + offset + index, record[index]);
            } while (record[index++] != 0);
            word = r_u32(0x80087C24);
            tail = r_u8(0x80087C28);
            destination = 0x8009FE2E + offset;
            /* Preserve the two original unaligned merge stores */
            w_u32(destination + 2, (r_u32(destination + 2) & 0xFFFF0000) | (word >> 16));
            w_u32(destination - 2, (r_u32(destination - 2) & 0xFFFF) | (word << 16));
            w_u8(destination + 4, (uint8)tail);
            byte_offset += 292;
            head = block;
            ++block;
            if (blocks >= 2)
            {
                offset = slot_offset + block * 292;
                for (continuation = 1; continuation < blocks - 1; ++continuation)
                {
                    td5_read_card_icon(path, slot, block, head);
                    w_u16(0x8009FE38 + offset, 2);
                    w_u16(0x8009FE3A + offset, (uint16)head);
                    offset += 292;
                    byte_offset += 292;
                    ++block;
                }
                td5_read_card_icon(path, slot, block, head);
                offset = slot_offset + byte_offset;
                byte_offset += 292;
                ++block;
                w_u16(0x8009FE38 + offset, 2);
                w_u16(0x8009FE3A + offset, (uint16)head);
            }
        }
        entry = nextfile(record);
    }
    return 0;
}

// TODO: Replace card enumeration and completion polling with native save availability
uint32 td5_scan_card_slots(sint32 first, sint32 count, uint32 invalid_header_index)
{
    uint32 current = (uint32)first;
    sint32 end = (sint32)((uint32)first + (uint32)count);
    uint32 channel;
    uint32 offset;
    sint32 status;

    FUNCTION_MARKER(0x800569E4, "SLUS_006.10");
    while ((sint32)(sint16)current < end)
    {
        channel = (uint32)(sint32)(sint16)current << 4;
        _card_info((sint32)channel);
        for (;;)
        {
            status = 0;
            if (TestEvent(r_s32(0x800882F8)) == 1)
                break;
            status = 1;
            if (TestEvent(r_s32(0x800882FC)) == 1)
                break;
            status = 2;
            if (TestEvent(r_s32(0x80088300)) == 1)
                break;
            status = 3;
            if (TestEvent(r_s32(0x80088304)) == 1)
                break;
        }
        td5_debug_log(0x80011460, status);
        if (status == 3)
        {
            TestEvent(r_s32(0x80088308));
            TestEvent(r_s32(0x8008830C));
            TestEvent(r_s32(0x80088310));
            TestEvent(r_s32(0x80088314));
            _card_clear(channel);
            while (TestEvent(r_s32(0x80088308)) != 1 && TestEvent(r_s32(0x8008830C)) != 1 && TestEvent(r_s32(0x80088310)) != 1 && TestEvent(r_s32(0x80088314)) != 1)
            {
            }
        }
        TestEvent(r_s32(0x800882F8));
        TestEvent(r_s32(0x800882FC));
        TestEvent(r_s32(0x80088300));
        TestEvent(r_s32(0x80088304));
        psx_bios_card_load_guest(channel);
        for (;;)
        {
            status = 0;
            if (TestEvent(r_s32(0x800882F8)) == 1)
                break;
            status = 1;
            if (TestEvent(r_s32(0x800882FC)) == 1)
                break;
            status = 2;
            if (TestEvent(r_s32(0x80088300)) == 1)
                break;
            status = 3;
            if (TestEvent(r_s32(0x80088304)) == 1)
                break;
        }
        offset = (uint32)(sint32)(sint16)current * 4396;
        if (status == 0 || status == 3)
        {
            w_u32(0x800A0E3C + offset, 1);
            w_u32(0x800A0E40 + offset, status == 0 ? 1 : 0);
        }
        w_u32(0x800A0E38 + offset, 0);
        ++current;
    }
    td5_scan_card(0, invalid_header_index);
    return td5_scan_card(1, invalid_header_index);
}

uint32 td5_free_memory(uint32 allocation)
{
    uint32 pool;
    uint32 units;
    uint32 available;
    uint32 block;
    uint32 previous;
    uint32 next;
    uint32 previous_units;
    uint32 result;

    FUNCTION_MARKER(0x8007B52C, "SLUS_006.10");
    pool = r_u32(0x80088168);
    units = r_u32(allocation - 12);
    available = r_u32(pool + 8);
    previous = r_u32(pool);
    block = allocation - 16;
    w_u32(pool + 8, available + (units << 4));
    for (;;)
    {
        if (previous < block)
        {
            next = r_u32(previous);
            if (block < next)
                break;
        }
        next = r_u32(previous);
        if (previous >= next && (previous < block || block < next))
            break;
        previous = next;
    }
    units = r_u32(block + 4);
    next = r_u32(previous);
    if (block + (units << 4) == next)
    {
        w_u32(block + 4, units + r_u32(next + 4));
        next = r_u32(previous);
        w_u32(block, r_u32(next));
    }
    else
    {
        w_u32(block, next);
    }
    previous_units = r_u32(previous + 4);
    if (previous + (previous_units << 4) == block)
    {
        w_u32(previous + 4, previous_units + r_u32(block + 4));
        w_u32(previous, r_u32(block));
    }
    else
    {
        w_u32(previous, block);
    }
    result = r_u32(0x80088168);
    w_u32(result, previous);
    return result;
}
