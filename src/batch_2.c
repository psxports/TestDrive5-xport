#include "td5_functions.h"
#include <string.h>
#include <stdio.h>

uint32 td5_archive_aligned_size(const char *filename, sint32 enabled, uint32 missing_index, uint32 prior_result)
{
    uint32 selected = missing_index;
    uint32 index = 0;
    uint32 entry;
    uint32 entry_length;
    sint32 found = 0;

    FUNCTION_MARKER(0x8007BC84, "SLUS_006.10");
    if (enabled == 0)
        return prior_result;
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
    return ((r_u32(0x80092668 + selected * 40) + 2047) >> 11) << 11;
}

sint32 td5_read_timer(sint32 timer, sint32 prior_result)
{
    FUNCTION_MARKER(0x8007AB50, "SLUS_006.10");
    if (timer == 0)
        return GetRCnt(0xF2000002);
    if (timer == 1)
        return GetRCnt(0xF2000001);
    return prior_result;
}

sint32 td5_scale_view_matrices(void)
{
    static const uint32 sources[18] = {
        0x8009D928, 0x8009D92A, 0x8009D92C,
        0x800A3B90, 0x800A3B92, 0x800A3B94,
        0x8009FCF8, 0x8009FCFA, 0x8009FCFC,
        0x8009D92E, 0x8009D930, 0x8009D932,
        0x800A3B96, 0x800A3B98, 0x800A3B9A,
        0x8009FCFE, 0x8009FD00, 0x8009FD02
    };
    static const uint32 destinations[18] = {
        0x80097648, 0x8009764A, 0x8009764C,
        0x800A3B90, 0x800A3B92, 0x800A3B94,
        0x8009FCF8, 0x8009FCFA, 0x8009FCFC,
        0x8009764E, 0x80097650, 0x80097652,
        0x800A3B96, 0x800A3B98, 0x800A3B9A,
        0x8009FCFE, 0x8009FD00, 0x8009FD02
    };
    static const uint32 tail_sources[9] = {
        0x8009D934, 0x8009D936, 0x8009D938,
        0x800A3B9C, 0x800A3B9E, 0x800A3BA0,
        0x8009FD04, 0x8009FD06, 0x8009FD08
    };
    static const uint32 tail_destinations[9] = {
        0x80097654, 0x80097656, 0x80097658,
        0x800A3B9C, 0x800A3B9E, 0x800A3BA0,
        0x8009FD04, 0x8009FD06, 0x8009FD08
    };
    uint32 width;
    uint32 multiplier;
    uint32 height;
    uint32 shift;
    sint32 x_scale;
    sint32 y_scale;
    sint32 product;
    sint32 pending = 0;
    sint32 tail[9];
    uint32 index;

    FUNCTION_MARKER(0x800784C4, "SLUS_006.10");
    width = r_u32(0x8008815C);
    multiplier = r_u32(0x80088160);
    x_scale = (sint32)(width * multiplier) / 4096;
    height = r_u32(0x80088158);
    shift = r_u32(0x80088164) & 31;
    x_scale >>= shift;
    y_scale = ((sint32)(height * multiplier) / 4096) >> shift;
    for (index = 0; index < 18; ++index)
    {
        product = (sint32)((uint32)(sint32)r_s16(sources[index]) * (uint32)(index < 9 ? x_scale : y_scale));
        if (index != 0)
            w_u16(destinations[index - 1], (uint16)pending);
        pending = product / 4096;
    }
    tail[0] = r_s16(tail_sources[0]);
    shift = r_u32(0x80088164) & 31;
    for (index = 1; index < 9; ++index)
        tail[index] = r_s16(tail_sources[index]);
    w_u16(destinations[17], (uint16)pending);
    for (index = 0; index < 9; ++index)
        w_u16(tail_destinations[index], (uint16)(tail[index] >> shift));
    return pending;
}

sint32 td5_set_view_angles(uint32 angles)
{
    uint32 cos_x;
    uint32 sin_x;
    uint32 cos_y;
    uint32 sin_y;
    uint32 cos_z;
    uint32 sin_z;
    uint32 xy;
    uint32 xc;
    uint32 left;
    uint32 right;
    sint32 first;
    sint32 second;

    FUNCTION_MARKER(0x80078DDC, "SLUS_006.10");
    cos_x = (uint32)rcos(r_s16(angles));
    sin_x = (uint32)rsin(r_s16(angles));
    cos_y = (uint32)rcos(r_s16(angles + 2));
    sin_y = (uint32)rsin(r_s16(angles + 2));
    cos_z = (uint32)rcos(r_s16(angles + 4));
    sin_z = (uint32)rsin(r_s16(angles + 4));
    xy = (uint32)((sint32)(sin_x * sin_y + 2048) >> 12);
    xc = (uint32)((sint32)(sin_x * cos_y + 2048) >> 12);
    w_u16(0x8009D936, (uint16)sin_x);
    w_u16(0x800A3B90, (uint16)cos_z);
    w_u16(0x800A3B96, (uint16)sin_z);
    w_u16(0x800A3B9C, 0);
    w_u16(0x800A3B9E, (uint16)sin_x);
    w_u16(0x800A3BA0, (uint16)cos_x);
    w_u16(0x8009FCF8, (uint16)cos_z);
    w_u16(0x8009FCFC, 0);
    w_u16(0x8009FCFE, (uint16)sin_z);
    w_u16(0x8009FD00, (uint16)cos_z);
    w_u16(0x8009FD02, 0);
    w_u16(0x8009FD04, 0);
    w_u16(0x8009FD06, 0);
    w_u16(0x8009FCFA, (uint16)(0 - sin_z));
    w_u16(0x8009FD08, 4096);
    first = (sint32)((0 - cos_x) * sin_z + 2048) >> 12;
    second = (sint32)(cos_x * cos_z + 2048) >> 12;
    w_u16(0x8009D92A, (uint16)first);
    w_u16(0x8009D930, (uint16)second);
    w_u16(0x800A3B92, (uint16)first);
    w_u16(0x800A3B98, (uint16)second);
    w_u16(0x8009D934, (uint16)((sint32)((0 - cos_x) * sin_y + 2048) >> 12));
    w_u16(0x8009D938, (uint16)((sint32)(cos_x * cos_y + 2048) >> 12));
    w_u16(0x800A3B94, (uint16)((sint32)(sin_x * cos_z + 2048) >> 12));
    w_u16(0x800A3B9A, (uint16)((sint32)((0 - sin_x) * cos_z + 2048) >> 12));
    left = cos_y * cos_z;
    right = xy * sin_z;
    w_u16(0x8009D928, (uint16)((sint32)(left - right + 2048) >> 12));
    left = sin_y * cos_z;
    right = xc * sin_z;
    w_u16(0x8009D92C, (uint16)((sint32)(left + right + 2048) >> 12));
    left = cos_y * sin_z;
    right = xy * cos_z;
    w_u16(0x8009D92E, (uint16)((sint32)(left + right + 2048) >> 12));
    left = sin_y * sin_z;
    right = xc * cos_z;
    w_u16(0x8009D932, (uint16)((sint32)(left - right + 2048) >> 12));
    return td5_scale_view_matrices();
}

sint32 td5_move_menu_selection(sint32 original, sint32 count)
{
    uint32 selected = (uint32)original;
    uint32 table;
    uint32 entry;
    sint32 last = (sint32)((uint32)count - 1);
    sint32 result;

    FUNCTION_MARKER(0x8005736C, "SLUS_006.10");
    table = r_u32(0x800BF3A8);
    if ((r_u16(table + selected * 24 + 8) & 0xC000) != 0)
        selected = 0;
    if ((r_u16(0x800884E8) & 0x10) != 0)
    {
        --selected;
        entry = table + selected * 24;
        while ((r_u16(entry + 8) & 0xC000) != 0)
        {
            entry -= 24;
            if ((sint32)selected > 0)
                --selected;
            else
            {
                entry = table + (uint32)original * 24;
                selected = (uint32)original;
            }
        }
    }
    if ((r_u16(0x800884E8) & 0x40) != 0)
    {
        ++selected;
        table = r_u32(0x800BF3A8);
        entry = table + selected * 24;
        while ((r_u16(entry + 8) & 0xC000) != 0)
        {
            entry += 24;
            if ((sint32)selected < last)
                ++selected;
            else
            {
                entry = table + (uint32)original * 24;
                selected = (uint32)original;
            }
        }
    }
    if ((sint32)selected < 0)
        result = 0;
    else if (last < (sint32)selected)
        result = last;
    else
        result = (sint32)selected;
    if (original != result)
        td5_80053030();
    w_u32(0x800887B8, (uint32)result);
    return result;
}

void td5_8001a0dc(uint32 buffer, uint32 archive_error_index)
{
    uint32 image_y;
    uint32 palette_y;
    char filename[64];

    FUNCTION_MARKER(0x8001A0DC, "SLUS_006.10");
    td5_show_legal_image(buffer, 0x80010258, 6, archive_error_index);
    td5_show_legal_image(buffer, 0x80010268, 1, archive_error_index);
    sprintf(filename, (const char *)psx_addr(0x80010244, 17));
    td5_load_archive_file(filename, buffer, archive_error_index);
    image_y = r_u32(0x80088104) << 8;
    palette_y = (r_u32(0x80088104) << 8) + 80;
    td5_upload_tim(buffer, 0, (sint16)image_y, 0, (sint16)palette_y);
    SetDispMask(1);
    td5_init_loading_primitive();
}

sint32 td5_8001a4d8(void)
{
    uint32 result;
    uint32 selection;

    FUNCTION_MARKER(0x8001A4D8, "SLUS_006.10");
    w_u32(0x80087720, 0);
    w_u32(0x80087728, 0);
    w_u32(0x8008771C, 0);
    w_u32(0x80087738, 0);
    w_u32(0x80087724, 0);
    w_u32(0x80087734, 0);
    w_u32(0x8008772C, 0);
    result = r_u32(0x8008769C);
    if (result == 0)
    {
        w_u32(0x80087720, 0);
    }
    else if (result == 1)
    {
        result = r_u32(0x800876A0);
        if (result < 6)
        {
            selection = r_u32(0x800876A0);
            result = 0x800107BC + selection * 4;
            switch (selection)
            {
            case 0:
                w_u32(0x80087720, 0);
                break;
            case 1:
                result = 1;
                w_u32(0x80087720, result);
                break;
            case 2:
            case 3:
                w_u32(0x80087720, 2);
                result = r_u32(0x800876BC);
                if (result < 6)
                {
                    selection = r_u32(0x800876BC);
                    result = 0x800107D4 + selection * 4;
                    switch (selection)
                    {
                    case 0:
                        w_u32(0x80087728, 0);
                        break;
                    case 1:
                        result = 5;
                        w_u32(0x80087728, result);
                        break;
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        result = selection - 1;
                        w_u32(0x80087728, result);
                        break;
                    }
                }
                break;
            case 4:
                w_u32(0x80087720, 3);
                result = 1;
                w_u32(0x80087734, result);
                break;
            case 5:
                w_u32(0x80087724, 1);
                result = 4;
                w_u32(0x80087720, result);
                break;
            }
        }
    }
    else if (result == 2)
    {
        w_u32(0x8008772C, 1);
        w_u32(0x80087720, 5);
        result = r_u32(0x800876C0);
        if (result < 3)
        {
            ++result;
            w_u32(0x8008771C, result);
        }
        else if (result == 3)
        {
            w_u32(0x8008771C, 4);
            result = 1;
            w_u32(0x80087734, result);
        }
    }
    return (sint32)result;
}

void td5_80019bb8(uint32 image, uint32 archive_error_index)
{
    uint32 next;
    uint32 environment;
    uint32 other;
    uint32 image_y;
    uint32 palette_y;
    PSX_RECT rectangle;
    char filename[64];

    FUNCTION_MARKER(0x80019BB8, "SLUS_006.10");
    if (r_u32(0x80087644) == 1)
        return;
    w_u32(0x80087644, 1);
    next = r_u32(0x80087610) + 1;
    w_u32(0x80087610, (uint32)((sint32)next % 20));
    VSync(0);
    DrawSync(0);
    SetDispMask(0);
    td5_prepare_display(0);
    td5_swap_display_buffer();
    w_u16(0x800976CC, r_u16(0x800876F8));
    w_u16(0x80097740, r_u16(0x800876F8));
    w_u16(0x800976CE, r_u16(0x800876FC));
    w_u16(0x80097742, r_u16(0x800876FC));
    environment = r_u32(0x800880F0);
    other = (r_u32(0x80088104) ^ 1) * 116;
    w_u16(environment + 2, r_u16(0x8009766A + other));
    environment = r_u32(0x800880F0);
    other = (r_u32(0x80088104) ^ 1) * 116;
    w_u16(environment + 10, r_u16(0x80097672 + other));
    td5_apply_display_buffer();
    sprintf(filename, (const char *)psx_addr(0x80010230, 20), r_u32(0x80087610) + 1);
    td5_load_archive_file(filename, image, archive_error_index);
    w_u32(0x8008847C, r_u32(0x80088104));
    rectangle.w = 320;
    rectangle.h = 256;
    rectangle.x = 0;
    rectangle.y = 0;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(image + 20, 320 * 256 * 2));
    rectangle.w = 320;
    rectangle.h = 256;
    rectangle.x = 0;
    rectangle.y = 256;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(image + 20, 320 * 256 * 2));
    DrawSync(0);
    VSync(0);
    sprintf(filename, (const char *)psx_addr(0x80010244, 17));
    td5_load_archive_file(filename, image, archive_error_index);
    image_y = r_u32(0x80088104) << 8;
    palette_y = (r_u32(0x80088104) << 8) + 80;
    td5_upload_tim(image, 0, (sint16)image_y, 0, (sint16)palette_y);
    SetDispMask(1);
    td5_init_loading_primitive();
}

uint32 td5_find_flare_textures(void)
{
    uint32 texture;

    FUNCTION_MARKER(0x8001AA5C, "SLUS_006.10");
    texture = td5_find_texture((const char *)psx_addr(0x8008776C, 7));
    w_u32(0x80088758, texture);
    texture = td5_find_texture((const char *)psx_addr(0x80087774, 7));
    w_u32(0x8008875C, texture);
    return texture;
}

uint32 td5_rank_race_times(void)
{
    uint32 index;
    uint32 changes = 1;
    uint32 left;
    uint32 right;
    uint32 numerator;
    uint32 denominator;
    sint32 left_time;
    sint32 right_time;
    uint8 temporary;

    FUNCTION_MARKER(0x8001A764, "SLUS_006.10");
    for (index = 0; index < 6; ++index)
        w_u8(0x800883F0 + index, (uint8)index);
    while (changes != 0)
    {
        changes = 0;
        for (index = 0; index < 5; ++index)
        {
            left = r_u8(0x800883F0 + index);
            numerator = r_u32(0x8009DA28 + left * 20) * 100;
            denominator = (uint32)(r_s32(0x800880FC) / 2);
            if (denominator == 0)
                xport_mips_break(7);
            left_time = (sint32)(numerator / denominator);
            right = r_u8(0x800883F1 + index);
            numerator = r_u32(0x8009DA28 + right * 20) * 100;
            denominator = (uint32)(r_s32(0x800880FC) / 2);
            if (denominator == 0)
                xport_mips_break(7);
            right_time = (sint32)(numerator / denominator);
            if (right_time < left_time)
            {
                ++changes;
                temporary = r_u8(0x800883F0 + index);
                w_u8(0x800883F0 + index, r_u8(0x800883F1 + index));
                w_u8(0x800883F1 + index, temporary);
            }
        }
    }
    for (index = 0; index < 6; ++index)
        w_u16(0x8009DA22 + r_u8(0x800883F0 + index) * 20, (uint16)index);
    return 6;
}

uint32 td5_rank_race_scores(void)
{
    uint32 index;
    uint32 changes = 1;
    uint32 left;
    uint32 right;
    uint32 left_score;
    uint32 right_score;
    uint8 temporary;

    FUNCTION_MARKER(0x8001AAB4, "SLUS_006.10");
    for (index = 0; index < 6; ++index)
        w_u8(0x800883F0 + index, (uint8)index);
    while (changes != 0)
    {
        changes = 0;
        for (index = 0; index < 5; ++index)
        {
            left = r_u8(0x800883F0 + index);
            right = r_u8(0x800883F1 + index);
            left_score = r_u32(0x8009DA24 + left * 20);
            right_score = r_u32(0x8009DA24 + right * 20);
            if (left_score < right_score)
            {
                ++changes;
                temporary = r_u8(0x800883F0 + index);
                w_u8(0x800883F0 + index, r_u8(0x800883F1 + index));
                w_u8(0x800883F1 + index, temporary);
            }
        }
    }
    for (index = 0; index < 6; ++index)
        w_u16(0x8009DA22 + r_u8(0x800883F0 + index) * 20, (uint16)index);
    return 6;
}

sint32 td5_80048fc8(void)
{
    uint32 state;
    uint32 first;
    uint32 slot;
    uint32 previous;
    uint32 cycle = 0;
    uint32 table = 0x8007E4C8;
    sint32 candidate;
    sint32 sequential = 0;
    sint32 duplicate;

    FUNCTION_MARKER(0x80048FC8, "SLUS_006.10");
    state = r_u32(0x80087738);
    if (state != 0)
        return (sint32)state;
    first = 2;
    if (r_u32(0x8008772C) == 0 && r_u32(0x80087734) == 0)
        first = 1;
    for (slot = first; slot < 6; ++slot)
        w_u8(0x8008CF72 + slot * 24, 255);
    state = r_u32(0x80088680);
    if (state == 0)
    {
        sequential = 1;
        table = 0x8007E4E8;
    }
    if (state == 1)
    {
        sequential = 1;
        table = 0x8007E4F4;
    }
    if (state == 2)
    {
        sequential = 1;
        table = 0x8007E500;
    }
    if (r_u32(0x80087728) == 5)
    {
        state = r_u32(0x80087718);
        table = state == 0 ? 0x8007DB04 : 0x8007DB28;
        sequential = 0;
    }
    for (slot = first; slot < 6; ++slot)
    {
        for (;;)
        {
            if (sequential != 0)
            {
                candidate = r_s16(table + cycle * 2);
                cycle = (cycle + 1) % 6;
            }
            else
            {
                candidate = r_s16(table + (uint32)(randPSX() % 14) * 2);
            }
            duplicate = 0;
            for (previous = 0; previous < slot; ++previous)
            {
                if (r_s8(0x8008CF72 + previous * 24) == candidate || candidate == 5)
                {
                    duplicate = 1;
                    break;
                }
            }
            if (duplicate == 0)
                break;
        }
        w_u8(0x8008CF74 + slot * 24, 0);
        w_u8(0x8008CF76 + slot * 24, 0);
        w_u8(0x8008CF72 + slot * 24, (uint8)candidate);
    }
    return 0;
}

uint32 td5_init_race_results(void)
{
    uint32 slot;
    uint32 offset;
    uint32 result;

    FUNCTION_MARKER(0x80018260, "SLUS_006.10");
    for (slot = 0; slot < 6; ++slot)
    {
        offset = slot * 20;
        w_u8(0x8009DA21 + offset, r_u8(0x8008CF72 + slot * 24));
        w_u8(0x8009DA20 + offset, 0);
        w_u32(0x8009DA24 + offset, 0);
        w_u16(0x8009DA22 + offset, 0);
        w_u32(0x8009DA28 + offset, 0);
        w_u16(0x8009DA2C + offset, 0);
        w_u16(0x8009DA2E + offset, 0);
    }
    w_u8(0x8009DA20, 1);
    result = r_u32(0x8008771C);
    if (result == 2)
        w_u8(0x8009DA34, 2);
    return result;
}

sint32 td5_80018db4(uint32 archive_error_index, uint32 original_stack, uint32 saved_s0_carrier)
{
    static const uint32 level_tables[6] = {
        0x8008761C, 0x8007DB64, 0x8007DB70,
        0x8007DB80, 0x8007DB94, 0x8007DB58
    };
    static const uint32 rival_classes[6] = {0, 1, 1, 2, 2, 0};
    static const uint32 loader_returns[6] = {
        0x80019018, 0x8001908C, 0x80019100,
        0x80019174, 0x800191E8, 0x80019258
    };
    uint32 mode;
    uint32 variant;
    uint32 value;

    FUNCTION_MARKER(0x80018DB4, "SLUS_006.10");
    td5_80016344();
    w_u32(0x800885F0, 0);
    w_u32(0x80087640, 1);
    w_u32(0x80088420, 0);
    w_u32(0x80088478, 0xFFFFFFFF);
    w_u32(0x8008871C, 0);
    w_u32(0x80087630, 0);
    w_u32(0x80087738, 0);
    w_u32(0x80088680, r_u32(0x800876D4));
    w_u32(0x80088578, r_u32(0x800876D0));
    w_u32(0x8008763C, r_u32(0x800876CC));
    w_u32(0x80088700, r_u32(0x800876E8) ^ 1);
    w_u32(0x800877A8, r_u32(0x800876D8));
    w_u32(0x800876F4, r_u32(0x800876E0) + 1);
    w_u32(0x80087784, r_u32(0x800876DC) * 5 + 15);
    w_u32(0x80087828, r_u32(0x800876E4) * 7);
    if (r_s32(0x80087828) < 0)
        value = 0;
    else if (r_s32(0x80087828) >= 65)
        value = 64;
    else
        value = r_u32(0x80087828);
    w_u32(0x80087828, value);
    w_u32(0x800883F8, r_u32(0x800876C8));
    mode = r_u32(0x80087720);
    if (mode < 6)
    {
        mode = r_u32(0x80087720);
        switch (mode)
        {
        case 0:
            w_u32(0x80087710, r_u32(0x8008770C));
            td5_8001a328(archive_error_index, original_stack - 0x18, 0x80018F30, saved_s0_carrier);
            td5_80048fc8();
            td5_init_race_results();
            if (r_s32(0x80087710) >= 24)
                w_u32(0x8008763C, 0);
            break;
        case 1:
            w_u32(0x80087630, 1);
            w_u32(0x8008763C, 0);
            w_u32(0x80087710, r_u32(0x8008770C));
            td5_8001a328(archive_error_index, original_stack - 0x18, 0x80018F80, saved_s0_carrier);
            break;
        case 2:
            variant = r_u32(0x80087728);
            if (variant < 6)
            {
                variant = r_u32(0x80087728);
                if (variant < 6)
                {
                    if (variant != 5)
                        w_u32(0x800883F8, 1);
                    w_u32(0x80088578, 1);
                    w_u32(0x80088680, rival_classes[variant]);
                    if (variant == 5)
                        w_u32(0x800876F4, 4);
                    w_u32(0x8008763C, 0);
                    value = r_u32(0x800885F0);
                    w_u32(0x80087710, (uint32)(sint32)r_s16(level_tables[variant] + value * 2) - 1);
                    td5_8001a328(archive_error_index, original_stack - 0x18, loader_returns[variant], saved_s0_carrier);
                    w_u32(0x80088478, variant == 2 ? 1 : 0xFFFFFFFF);
                    td5_80048fc8();
                    td5_init_race_results();
                }
            }
            break;
        case 3:
            w_u32(0x8008763C, 0);
            w_u32(0x80088680, 1);
            w_u32(0x80087710, 29);
            td5_8001a328(archive_error_index, original_stack - 0x18, 0x8001929C, saved_s0_carrier);
            td5_80048fc8();
            break;
        case 4:
            w_u32(0x8008763C, 0);
            w_u32(0x80088578, 0);
            w_u32(0x80087710, r_u32(0x8008770C));
            td5_8001a328(archive_error_index, original_stack - 0x18, 0x800192CC, saved_s0_carrier);
            td5_80048fc8();
            break;
        case 5:
            w_u32(0x80088578, 0);
            variant = r_u32(0x8008771C);
            switch (variant)
            {
            case 1:
                w_u32(0x8008763C, 0);
                w_u32(0x80087710, r_u32(0x8008770C));
                td5_8001a328(archive_error_index, original_stack - 0x18, 0x8001934C, saved_s0_carrier);
                td5_80048fc8();
                break;
            case 2:
                w_u32(0x800876F4, 4);
                w_u32(0x8008763C, 0);
                value = r_u32(0x800885F0);
                w_u32(0x80087710, (uint32)(sint32)r_s16(0x8007DBFC + value * 2));
                td5_8001a328(archive_error_index, original_stack - 0x18, 0x80019398, saved_s0_carrier);
                w_u32(0x80088478, 0xFFFFFFFF);
                td5_80048fc8();
                td5_init_race_results();
                break;
            case 3:
                w_u32(0x80087724, 1);
                w_u32(0x800879B8, r_u32(0x800885F0));
                w_u32(0x800876F4, 4);
                w_u32(0x8008763C, 0);
                w_u32(0x80087710, r_u32(0x8008770C));
                td5_8001a328(archive_error_index, original_stack - 0x18, 0x800193EC, saved_s0_carrier);
                td5_80048fc8();
                td5_init_race_results();
                break;
            case 4:
                w_u32(0x80088680, 1);
                w_u32(0x8008763C, 0);
                w_u32(0x80087710, 30);
                td5_8001a328(archive_error_index, original_stack - 0x18, 0x80019420, saved_s0_carrier);
                td5_80048fc8();
                break;
            }
            break;
        }
    }
    w_u32(0x8008858C, 1);
    return 1;
}

uint32 td5_accumulate_race_results(void)
{
    uint32 maximum = 0;
    uint32 slot;
    uint32 rank;
    uint32 lap;
    uint32 offset;
    uint32 result_offset;
    uint32 value;
    uint32 other;
    uint32 remaining;
    uint32 jitter;
    sint32 random_value;
    sint32 denominator;
    sint32 quotient;

    FUNCTION_MARKER(0x80018408, "SLUS_006.10");
    for (slot = 0; slot < 6; ++slot)
    {
        offset = slot * 864;
        if (r_u32(0x800987A0 + offset) != 0)
        {
            if (maximum < r_u32(0x800987A0 + offset))
                maximum = r_u32(0x800987A0 + offset);
        }
    }
    for (slot = 0; slot < 6; ++slot)
    {
        offset = slot * 864;
        result_offset = slot * 20;
        for (rank = 0; rank < 6; ++rank)
        {
            if (slot == r_u8(0x800883F0 + rank))
                break;
        }
        if (r_u32(0x800987A0 + offset) == 0)
        {
            if (r_u32(0x80087730) != 0 || r_u32(0x8008772C) != 0)
            {
                random_value = randPSX();
                value = (uint32)(sint32)r_s16(0x80098516);
                other = (uint32)(sint32)r_s16(0x80098516 + offset);
                w_u32(0x800987A0 + offset, (value - other) * 2 + maximum + (uint32)(random_value % 32));
            }
            else
            {
                random_value = randPSX();
                value = r_u8(0x800987FB + offset);
                w_u32(0x800987A0 + offset, value * 30 + maximum + (uint32)(random_value % 29));
            }
            remaining = r_u32(0x800987A0 + offset);
            for (lap = 0; (sint32)lap < r_s32(0x800876F4); ++lap)
            {
                if (lap == r_u32(0x800876F4) - 1)
                    jitter = 0;
                else
                    jitter = 50 - (uint32)(randPSX() % 150);
                denominator = (sint32)(r_u32(0x800876F4) - lap);
                if (denominator == 0)
                    xport_mips_break(7);
                if (denominator == -1 && remaining == 0x80000000)
                    xport_mips_break(6);
                quotient = (sint32)remaining / denominator;
                w_u16(0x800987C6 + offset + lap * 2, (uint16)((uint32)quotient + (uint16)jitter));
                remaining -= r_u16(0x800987C6 + offset + lap * 2);
            }
        }
        if (r_u32(0x80087728) == 0 && r_u32(0x80087720) == 2)
        {
            value = r_u8(0x80087764 + rank);
            other = r_u32(0x80098740 + offset);
            w_u32(0x80098740 + offset, other + value);
        }
        if (r_u32(0x80087728) == 4 && r_u32(0x80087720) == 2)
        {
            value = (uint32)(sint32)r_s16(0x8007DC54 + rank * 2);
            other = r_u32(0x80098740 + offset);
            w_u32(0x80098740 + offset, other + value);
        }
        w_u16(0x8009DA22 + result_offset, (uint16)rank);
        value = r_u32(0x8009DA28 + result_offset);
        other = r_u32(0x800987A0 + offset);
        w_u32(0x8009DA28 + result_offset, value + other);
        value = r_u32(0x8009DA24 + result_offset);
        other = r_u32(0x80098740 + offset);
        w_u32(0x8009DA24 + result_offset, value + other);
        if (r_u32(0x80087724) != 0)
        {
            if (r_u32(0x800879B8) == slot)
                w_u8(0x8009DA30 + result_offset, r_u8(0x800883A0));
        }
        else
        {
            w_u8(0x8009DA30 + result_offset, r_u8(0x800987FC + offset));
        }
        value = r_u16(0x8009DA2C + result_offset);
        other = r_u16(0x800987AA + offset);
        w_u16(0x8009DA2C + result_offset, (uint16)(value + other));
        value = r_u16(0x8009DA2E + result_offset);
        if ((sint32)value < r_s16(0x800987A8 + offset))
            w_u16(0x8009DA2E + result_offset, r_u16(0x800987A8 + offset));
    }
    return 6;
}

uint32 td5_8004b004(void)
{
    uint32 value = 6;
    uint32 descriptor;

    FUNCTION_MARKER(0x8004B004, "SLUS_006.10");
    if (r_u32(0x800883F8) != 0)
    {
        descriptor = r_u32(0x80088770);
        descriptor = r_u32(descriptor);
        if ((r_u8(descriptor + 30) & 0x20) != 0)
            value = 12;
    }
    w_u32(0x800875E4, value);
    if (r_u32(0x80087730) != 0)
        w_u32(0x800875E4, 6);
    if (r_u32(0x8008772C) != 0)
        w_u32(0x800875E4, 4);
    if (r_u32(0x80087734) != 0)
        w_u32(0x800875E4, 2);
    if (r_u32(0x8008771C) == 3)
        w_u32(0x800875E4, 2);
    return 2;
}

uint32 td5_8004b0b0(void)
{
    FUNCTION_MARKER(0x8004B0B0, "SLUS_006.10");
    return td5_8004b004();
}

uint32 td5_80030d48(void)
{
    uint32 descriptor;
    uint32 value;

    FUNCTION_MARKER(0x80030D48, "SLUS_006.10");
    descriptor = r_u32(0x80088770);
    value = r_u32(descriptor + 12);
    w_u32(0x80088488, value);
    return value;
}

sint32 td5_80019458(uint32 archive_error_index, uint32 original_stack, uint32 saved_s0_carrier)
{
    static const uint32 level_tables[6] = {
        0x8008761C, 0x8007DB64, 0x8007DB70,
        0x8007DB80, 0x8007DB94, 0x8007DB58
    };
    static const uint32 rival_classes[6] = {0, 1, 1, 2, 2, 0};
    static const sint32 stage_counts[6] = {4, 6, 8, 10, 12, 6};
    uint32 mode;
    uint32 variant;
    uint32 stage;
    uint32 winner;
    uint32 result;

    FUNCTION_MARKER(0x80019458, "SLUS_006.10");
    w_u32(0x80087738, 0);
    w_u32(0x80087640, 1);
    w_u32(0x8008858C, 1);
    mode = r_u32(0x80087720);
    if (mode < 6)
    {
        mode = r_u32(0x80087720);
        switch (mode)
        {
        case 2:
            variant = r_u32(0x80087728);
            if (variant < 6)
            {
                variant = r_u32(0x80087728);
                w_u32(0x800883F8, 1);
                w_u32(0x80088578, 1);
                w_u32(0x80088680, rival_classes[variant]);
                w_u32(0x8008763C, 0);
                if (variant != 5)
                    w_u32(0x80088478, 0xFFFFFFFF);
                stage = r_u32(0x800885F0);
                w_u32(0x80087710, (uint32)(sint32)r_s16(level_tables[variant] + stage * 2) - 1);
                if (r_s32(0x800885F0) >= stage_counts[variant])
                {
                    if (variant != 0)
                        w_u32(0x80087710, 0);
                    w_u16(0x8009DA2C, (uint16)(r_u16(0x8009DA2C) / (uint32)stage_counts[variant]));
                    td5_800164e0((sint32)(40 + variant));
                    if (variant == 0 || variant == 4)
                        td5_rank_race_scores();
                    else
                        td5_rank_race_times();
                    w_u32(0x80088620, 170);
                    w_u32(0x8008858C, 0);
                    if (variant == 0)
                    {
                        if (r_s16(0x8009DA22) == 0)
                            w_u32(0x80088420, 1);
                    }
                    else
                    {
                        winner = r_u8(0x800883F0);
                        if (r_u8(0x8009DA20 + winner * 20) == 1)
                            w_u32(0x80088420, 1);
                    }
                }
            }
            break;
        case 4:
            w_u32(0x80088578, 0);
            break;
        case 5:
            w_u32(0x80088578, 0);
            variant = r_u32(0x8008771C);
            if (variant == 3)
            {
                w_u32(0x800879B8, r_u32(0x800885F0));
                td5_8001a328(archive_error_index, original_stack - 0x18, 0x80019AD8, saved_s0_carrier);
                td5_80048fc8();
            }
            else if (variant == 2)
            {
                w_u32(0x8008763C, 0);
                stage = r_u32(0x800885F0);
                w_u32(0x80087710, (uint32)(sint32)r_s16(0x8007DBFC + stage * 2));
                if (r_s32(0x800885F0) >= 4)
                {
                    td5_rank_race_times();
                    w_u32(0x80088620, 240);
                    w_u32(0x8008858C, 0);
                    winner = r_u8(0x800883F0);
                    if (r_u8(0x8009DA20 + winner * 20) != 0)
                        w_u32(0x80088420, 1);
                }
            }
            break;
        }
    }
    result = r_u32(0x8008858C);
    if (result == 1)
        return td5_8001a328(archive_error_index, original_stack - 0x18, 0x80019BA0, saved_s0_carrier);
    return (sint32)result;
}

uint32 td5_8001b128(void)
{
    uint32 owner;
    uint32 first_base;
    uint32 saved_base;
    uint32 second_base;
    uint32 count_base;
    uint32 extra_base;
    uint32 last_base;
    uint32 first_offset;
    uint32 second_offset;
    uint32 count;
    uint32 extra;
    uint32 last;
    uint32 first;
    uint32 result;

    FUNCTION_MARKER(0x8001B128, "SLUS_006.10");
    owner = r_u32(0x80088710);
    first_base = r_u32(owner + 4);
    saved_base = r_u32(owner + 4);
    second_base = r_u32(owner + 4);
    count_base = r_u32(owner + 4);
    extra_base = r_u32(owner + 4);
    last_base = r_u32(owner + 4);
    first_offset = r_u32(first_base);
    second_offset = r_u32(second_base + 8);
    count = r_u32(count_base + 4);
    extra = r_u32(extra_base + 16);
    last = r_u32(last_base + 12);
    w_u32(0x80088584, saved_base);
    first = first_base + first_offset;
    w_u32(0x800885E4, last);
    w_u32(0x800884C4, first);
    w_u32(0x8008850C, second_base + second_offset);
    w_u32(0x8008841C, count);
    w_u32(0x80088424, extra);
    w_u8(first, 9);
    result = r_u32(0x8008841C);
    w_u16(first + 10, (uint16)(count - 1));
    first = r_u32(0x800884C4);
    result = result * 24 + first;
    w_u16(result - 16, 0);
    w_u8(result - 24, 10);
    return result;
}

uint32 td5_8005aa54(uint32 value)
{
    FUNCTION_MARKER(0x8005AA54, "SLUS_006.10");
    w_u32(0x80088368, value);
    CdInit();
    CdSetDebug(0);
    memset(psx_addr(0x800A4E28, 48), 0, 48);
    w_u32(0x800880D0, 0);
    w_u32(0x800880CC, 0);
    w_u32(0x800880D4, 0);
    w_u32(0x800880C8, 0);
    return 0x800A4E28;
}

uint32 td5_alloc_display_buffers(uint32 bytes)
{
    uint32 buffer;

    FUNCTION_MARKER(0x80072100, "SLUS_006.10");
    buffer = td5_alloc_memory(bytes);
    w_u32(0x80088370, buffer);
    buffer = td5_alloc_memory(bytes);
    w_u32(0x80088374, buffer);
    w_u32(0x800886D8, bytes);
    return buffer;
}

uint32 td5_init_scene_pool(uint32 buffer, uint32 bytes)
{
    uint32 units = (bytes >> 4) << 1;

    FUNCTION_MARKER(0x8006B000, "SLUS_006.10");
    w_u32(0x80088328, buffer);
    w_u32(0x8008832C, buffer);
    w_u32(buffer, buffer);
    w_u32(buffer + 4, buffer);
    w_u32(buffer + 12, units);
    w_u32(0x80088664, bytes);
    w_u32(0x80088544, 0);
    w_u32(0x80088330, bytes);
    w_u32(0x800883B8, bytes);
    w_u32(0x800880D8, 0);
    return units;
}

uint32 td5_set_engine_parameter(uint32 value)
{
    uint32 stereo;

    FUNCTION_MARKER(0x80052EF4, "SLUS_006.10");
    stereo = r_u32(0x80088668);
    w_u32(0x80091F78, value);
    if (stereo != 0)
        w_u32(0x80091FD4, value);
    return stereo;
}

sint32 td5_start_engine_sound(uint32 position, uint32 velocity)
{
    FUNCTION_MARKER(0x80052F1C, "SLUS_006.10");
    return td5_play_positioned_sound(8, 16, 0, 5120, position, velocity);
}

void td5_800556a4(void)
{
    FUNCTION_MARKER(0x800556A4, "SLUS_006.10");
}

sint32 td5_play_positioned_sound(sint32 sample, sint32 voice, uint32 pan, sint32 pitch, uint32 position, uint32 velocity)
{
    uint32 coordinate;
    uint32 view;
    uint32 difference;
    uint32 offset;
    uint32 sample_offset;
    uint32 product;
    sint32 rate;
    sint32 step;
    sint32 frequency;
    uint32 next_voice;
    uint32 key;

    FUNCTION_MARKER(0x80054978, "SLUS_006.10");
    if (voice == -1)
    {
        voice = td5_find_sound_voice();
        if (voice == -1)
            return -1;
        coordinate = (uint32)(r_s32(position) >> 8);
        view = r_u32(0x8009765C);
        difference = view - coordinate;
        if ((sint32)difference < 0)
            difference = coordinate - view;
        if ((sint32)difference > 32767)
            return -1;
        coordinate = (uint32)(r_s32(position + 8) >> 8);
        view = r_u32(0x80097664);
        difference = view - coordinate;
        if ((sint32)difference < 0)
            difference = coordinate - view;
        if ((sint32)difference > 32767)
            return -1;
    }
    offset = (uint32)voice * 92;
    w_u32(0x800919C8 + offset, velocity);
    sample_offset = (uint32)sample * 12;
    w_u32(0x800919B0 + offset, (uint32)sample);
    w_u32(0x800919C4 + offset, position);
    w_u32(0x800919B8 + offset, pan);
    w_u32(0x800919BC + offset, (uint32)pitch);
    rate = r_s32(0x8008CD2C + sample_offset);
    product = (uint32)rate * (uint32)pitch;
    step = (sint32)product / 4096;
    if (rate == 0)
        xport_mips_break(7);
    product = (uint32)step * (uint32)(44100 / rate);
    frequency = (sint32)(product << 12) / 44100;
    if (frequency >= 16384)
        frequency = 16383;
    SpuSetKey(0, r_u32(0x80091970 + offset));
    w_u32(0x8009198C + offset, r_u32(0x8008CD28 + sample_offset));
    td5_8005552c(voice, frequency, 0, 0);
    SpuSetKey(1, r_u32(0x80091970 + offset));
    next_voice = (uint32)voice + 1;
    if (r_u32(0x80088668) != 0)
    {
        offset = next_voice * 92;
        key = r_u32(0x80091970 + offset);
        w_u32(0x800919B0 + offset, (uint32)sample);
        w_u32(0x800919B8 + offset, 0xFFFFF000);
        w_u32(0x800919BC + offset, (uint32)pitch);
        SpuSetKey(0, key);
        w_u32(0x8009198C + offset, r_u32(0x8008CD28 + sample_offset));
        td5_8005552c((sint32)next_voice, frequency, 0, 0);
        SpuSetKey(1, r_u32(0x80091970 + offset));
    }
    return voice;
}

void td5_8005552c(sint32 voice, sint32 pitch, sint32 pan, sint32 volume)
{
    uint32 product;
    uint32 offset;
    uint32 sample;
    sint32 scaled_volume;
    sint32 normalized;
    sint32 frequency;
    sint32 delta;
    uint32 left;
    uint32 right;

    FUNCTION_MARKER(0x8005552C, "SLUS_006.10");
    product = (uint32)volume * r_u32(0x80087704);
    scaled_volume = (sint32)product / 4096;
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
        left = (uint32)scaled_volume;
        delta = (sint32)((uint32)scaled_volume * (uint32)pan) / 4096;
        right = (uint32)scaled_volume + (uint32)delta;
    }
    else
    {
        if (pan > 4096)
            pan = 4096;
        delta = (sint32)((uint32)scaled_volume * (uint32)pan) / 4096;
        left = (uint32)scaled_volume - (uint32)delta;
        right = (uint32)scaled_volume;
    }
    w_u16(0x80091978 + offset, (uint16)(left << 2));
    w_u16(0x8009197A + offset, (uint16)(right << 2));
    SpuSetVoiceAttr((SpuVoiceAttr *)psx_addr(0x80091970 + offset, sizeof(SpuVoiceAttr)));
}

sint32 td5_8001492c(uint32 archive_error_index, uint32 original_stack, uint32 texture_carrier, uint32 tree_descriptor_carrier)
{
    uint32 descriptor;
    uint32 pointer;
    uint32 value;
    uint32 second;
    uint32 prefix;
    uint32 length;
    uint32 buffer;
    uint32 size;
    uint32 base;
    uint32 index;
    uint32 span;
    uint32 sound_name_address = original_stack - 0xC8 - 0x140 + 0x18;
    uint32 wheel_height_address = original_stack - 0xC8 - 0x28 - 0x18 - 0x30 - 0x98 + 0x20;
    sint32 near_distance;
    sint32 far_distance;
    sint32 red;
    sint32 green;
    sint32 blue;
    PSX_RECT rectangle;
    char filename[64];

    FUNCTION_MARKER(0x8001492C, "SLUS_006.10");
    w_u32(0x80087A78, 0);
    w_u32(0x80088498, 0);
    if (r_u32(0x80087730) != 0 || r_u32(0x8008772C) != 0)
    {
        descriptor = r_u32(0x80088770);
        pointer = r_u32(descriptor + 44);
        w_u32(0x8008765C, r_u16(pointer + 4));
    }
    else
    {
        value = r_u32(0x80087710);
        w_u32(0x8008765C, r_u8(0x8007C4BC + value));
    }
    td5_init_memory_pool(0x800A7A00, 0x157A00);
    td5_8004b0b0();
    td5_8005aa54(0);
    td5_init_ordering_tables(0x1201);
    descriptor = r_u32(0x80088770);
    pointer = r_u32(descriptor);
    if ((r_u8(pointer + 30) & 1) != 0)
        td5_alloc_display_buffers(0xA000);
    else
        td5_alloc_display_buffers(0x8000);
    td5_8006c590(2);
    if (r_u32(0x8008772C) != 0)
        td5_prepare_display(0);
    else
        td5_prepare_display(2);
    if (r_u32(0x800876EC) == 0 && r_u32(0x80087738) != 0)
        td5_prepare_display(0);
    w_u16(0x800976CC, r_u16(0x800876F8));
    w_u16(0x80097740, r_u16(0x800876F8));
    w_u16(0x800976CE, r_u16(0x800876FC));
    w_u16(0x80097742, r_u16(0x800876FC));
    td5_set_projection_distance(180);
    td5_80078b6c(0, 65536);
    td5_set_back_color(50, 50, 50);
    td5_set_light(0, 0x800875DC, 110, 110, 110);
    td5_set_light(1, 0x800875DC, 0, 0, 0);
    td5_set_light(2, 0x800875DC, 0, 0, 0);
    if (r_u32(0x800885FC) != 0)
    {
        for (index = 0; index < 6; ++index)
        {
            descriptor = r_u32(0x80088770);
            pointer = r_u32(descriptor + 44);
            w_u16(pointer + (index + 1) * 4, (uint16)(150 + index * 10));
        }
    }
    descriptor = r_u32(0x80088770);
    pointer = r_u32(descriptor);
    near_distance = r_s16(pointer + 4);
    descriptor = r_u32(0x80088770);
    pointer = r_u32(descriptor);
    far_distance = r_s16(pointer + 6);
    descriptor = r_u32(0x80088770);
    pointer = r_u32(descriptor);
    red = r_u8(pointer);
    descriptor = r_u32(0x80088770);
    pointer = r_u32(descriptor);
    green = r_u8(pointer + 1);
    descriptor = r_u32(0x80088770);
    pointer = r_u32(descriptor);
    blue = r_u8(pointer + 2);
    td5_set_fog(near_distance, far_distance, red, green, blue);
    value = r_u32(0x80087608);
    second = r_u32(0x8008760C);
    rectangle.x = (sint16)value;
    rectangle.y = (sint16)(value >> 16);
    rectangle.w = (sint16)second;
    rectangle.h = (sint16)(second >> 16);
    buffer = td5_alloc_memory(0xB0000);
    prefix = r_u32(0x800885C4);
    length = 0;
    while (r_u8(prefix + length) != 0)
        ++length;
    sprintf(filename, (const char *)psx_addr(0x80010000, 11), (const char *)psx_addr(prefix, length + 1));
    td5_load_archive_file(filename, buffer, archive_error_index);
    rectangle.y = 0;
    span = (uint32)(uint16)rectangle.w * (uint16)rectangle.h * 2;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(buffer, span));
    rectangle.y = 256;
    span = (uint32)(uint16)rectangle.w * (uint16)rectangle.h * 2;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(buffer + 0x40000, span));
    DrawSync(0);
    td5_free_memory(buffer);
    prefix = r_u32(0x800885C4);
    length = 0;
    while (r_u8(prefix + length) != 0)
        ++length;
    sprintf(filename, (const char *)psx_addr(0x8001000C, 12), (const char *)psx_addr(prefix, length + 1));
    size = td5_archive_aligned_size(filename, 1, archive_error_index, 0);
    w_u32(0x80088568, td5_alloc_memory(size));
    w_u32(0x80088710, r_u32(0x80088568));
    td5_load_archive_file(filename, r_u32(0x80088568), archive_error_index);
    base = r_u32(0x80088568);
    for (index = 1; index <= r_u32(base); ++index)
        w_u32(base + index * 4, r_u32(base + index * 4) + base);
    descriptor = r_u32(0x80088770);
    pointer = r_u32(descriptor);
    w_u32(0x800883E4, td5_alloc_memory(r_u32(pointer + 12)));
    descriptor = r_u32(0x80088770);
    pointer = r_u32(descriptor);
    buffer = r_u32(0x800883E4);
    size = r_u32(pointer + 12);
    td5_init_scene_pool(buffer, size);
    td5_80030d48();
    td5_8001b128();
    pointer = r_u32(0x80088710);
    td5_set_texture_table(r_u32(pointer + 8));
    td5_80042c50();
    td5_800430b0();
    td5_find_flare_textures();
    td5_80046638(archive_error_index, original_stack - 0xC8, texture_carrier);
    td5_80025960(original_stack - 0xC8);
    td5_8002eb88(original_stack - 0xC8, tree_descriptor_carrier);
    td5_8003320c();
    for (index = 0; index < 6; ++index)
        td5_80033030(index, r_u32(0x80098620 + index * 864));
    td5_80034d90();
    td5_80036778();
    w_u32(0x80088610, 2);
    w_u32(0x80088614, 2);
    td5_8001e034(0x80098670, 1, 0);
    td5_8001e034(0x800989D0, 1, 1);
    td5_8003b244();
    td5_80028464();
    td5_8003b264();
    td5_80043b58();
    td5_80046460();
    td5_8003fd50();
    descriptor = r_u32(0x80088770);
    pointer = r_u32(descriptor);
    if ((r_u8(pointer + 30) & 4) != 0)
        td5_8004b3d8(archive_error_index, original_stack - 0xC8);
    td5_800556a4();
    td5_800417e0();
    td5_8004aef4(0);
    td5_8003fdf4();
    td5_80040fb0();
    td5_8004070c();
    td5_80039df8(original_stack - 0xC8);
    w_u32(0x80088108, 0);
    td5_80049364(archive_error_index);
    if (r_u32(0x80087734) != 0)
    {
        w_u32(0x800884D0, 0);
        w_u32(0x800884D4, 0);
        w_u32(0x800875E8, 0);
        w_u32(0x80088360, 0);
        w_u32(0x80088408, 0xFFFFFFFF);
        w_u32(0x800886E0, 0);
        w_u32(0x800886E4, 0);
        w_u32(0x80088360, 4);
        w_u32(0x80088618, 1);
        td5_8003a384();
    }
    else
        td5_80047468();
    w_u32(0x8008873C, 1);
    w_u32(0x80088618, 1);
    w_u32(0x80088360, 0xFFFFFFFF);
    w_u32(0x80088408, 0xFFFFFFFF);
    for (index = 0; index < 14; ++index)
        w_u32(0x8009874C + index * 864, 0x3AFE5F64);
    w_u32(0x80088610, 2);
    td5_8001e034(0x80098670 + r_u32(0x80088498) * 864, 1, 0);
    if (r_u32(0x80087724) != 0)
    {
        if (r_u32(0x8008771C) == 3)
        {
            if (r_u32(0x800885F0) != 0)
            {
                td5_8004d984(0, r_u32(0x8008765C) + 23, 2, 0, wheel_height_address);
                td5_8004d984(1, r_u32(0x8008765C) + 3, 2, 0, wheel_height_address);
            }
            else
            {
                td5_8004d984(1, r_u32(0x8008765C) + 23, 2, 0, wheel_height_address);
                td5_8004d984(0, r_u32(0x8008765C) + 3, 2, 0, wheel_height_address);
            }
        }
        else
        {
            td5_8004d984(0, r_u32(0x8008765C) - 3, 2, 0, wheel_height_address);
            td5_8004d984(1, r_u32(0x8008765C) + 25, 2, 0, wheel_height_address);
            td5_8004d984(2, r_u32(0x8008765C) + 50, 3, 0, wheel_height_address);
            td5_8004d984(3, r_u32(0x8008765C) + 75, 3, 0, wheel_height_address);
            td5_8004d984(4, r_u32(0x8008765C) + 100, 2, 0, wheel_height_address);
            td5_8004d984(5, r_u32(0x8008765C) + 125, 3, 0, wheel_height_address);
        }
    }
    else if (r_u32(0x80087734) != 0)
    {
        td5_8004d984(0, 115, 1, 0, wheel_height_address);
        td5_8004d984(1, 115, 2, 0, wheel_height_address);
    }
    else
    {
        if (r_u32(0x80087730) != 0)
        {
            td5_8004d984(0, r_u32(0x8008765C) - 6, 1, 0, wheel_height_address);
            td5_8004d984(1, r_u32(0x8008765C) - 6, 2, 0, wheel_height_address);
            td5_8004d984(2, r_u32(0x8008765C) - 12, 1, 0, wheel_height_address);
            td5_8004d984(3, r_u32(0x8008765C) - 12, 2, 0, wheel_height_address);
            td5_8004d984(4, r_u32(0x8008765C) - 18, 1, 0, wheel_height_address);
        }
        else
        {
            td5_8004d984(2, r_u32(0x8008765C) - 3, 1, 0, wheel_height_address);
            td5_8004d984(1, r_u32(0x8008765C) - 6, 2, 0, wheel_height_address);
            td5_8004d984(0, r_u32(0x8008765C) - 9, 1, 0, wheel_height_address);
            td5_8004d984(3, r_u32(0x8008765C) - 12, 2, 0, wheel_height_address);
            td5_8004d984(4, r_u32(0x8008765C) - 15, 1, 0, wheel_height_address);
        }
        td5_8004d984(5, r_u32(0x8008765C) - 18, 2, 0, wheel_height_address);
    }
    if (r_u32(0x80087630) == 1)
    {
        for (index = 1; index < 6; ++index)
            td5_8004d984(index, 1, index - 1, 0, wheel_height_address);
    }
    td5_8001f844();
    w_u32(0x800884E4, 1);
    td5_fade_audio();
    for (index = 0; index < 5; ++index)
        VSync(0);
    td5_80052924(0, sound_name_address);
    td5_80052924(1, sound_name_address);
    td5_80052b74(r_s32(0x8008762C) % 13, archive_error_index, original_stack - 0xC8);
    for (index = 0; index < 8; ++index)
        VSync(0);
    td5_set_engine_parameter(0);
    td5_start_engine_sound(0x80098664, 0x80098634);
    td5_set_engine_parameter(0);
    td5_800532a8();
    td5_80052d80();
    w_u32(0x8008762C, r_u32(0x8008762C) + 1);
    if (r_u32(0x80087730) != 0)
        return td5_play_xa(r_s32(0x80087710) % 7 + 9);
    value = r_u32(0x8008772C);
    if (value != 0)
        return td5_play_xa(r_s32(0x80087710) % 7 + 9);
    return (sint32)value;
}

uint32 td5_8004aef4(uint32 value)
{
    uint32 state;
    uint32 result;
    sint32 offset;

    FUNCTION_MARKER(0x8004AEF4, "SLUS_006.10");
    state = r_u32(0x80088418);
    w_u32(0x800882B8, 0x8008F5E0);
    w_u32(0x800882BC, 0x8008F5E0);
    w_u32(0x800882C0, 0x8008F5E0);
    if (state == 0)
    {
        for (offset = 0x1BFE; offset >= 0; offset -= 2)
            w_u16(0x8008F5E0 + (uint32)offset, 0);
        result = r_u32(0x80088418);
        if (result != 0)
        {
            result = r_u32(0x800882BC) - 2;
            w_u32(0x800882BC, result);
        }
    }
    else
    {
        result = r_u32(0x800882BC) - 2;
        w_u32(0x800882BC, result);
    }
    w_u32(0x80088780, value);
    w_u32(0x80088718, 0);
    w_u32(0x8008851C, 0);
    return result;
}

uint32 td5_alloc_scene_memory(uint32 bytes, uint32 owner)
{
    uint32 block = r_u32(0x8008832C);
    uint32 units = ((bytes + 15) >> 4) + 1;
    uint32 flags;
    uint32 available;
    uint32 allocation;
    uint32 next;
    uint32 destination;
    uint32 previous;
    uint32 update_owner;
    uint32 pool_bytes;

    FUNCTION_MARKER(0x8006B15C, "SLUS_006.10");
    do
    {
        flags = r_u32(block + 12);
        available = flags >> 1;
        if ((flags & 1) == 0 && (sint32)available >= (sint32)units)
        {
            if (available == units)
            {
                allocation = block;
                w_u32(block + 12, flags | 1);
                w_u32(block + 8, owner);
            }
            else
            {
                next = r_u32(block);
                available = (available - units) << 1;
                allocation = block + (available << 3);
                w_u32(block + 12, available);
                w_u32(allocation + 4, block);
                w_u32(allocation, next);
                w_u32(block, allocation);
                w_u32(r_u32(allocation) + 4, allocation);
                w_u32(allocation + 12, (units << 1) | 1);
                w_u32(allocation + 8, owner);
            }
            pool_bytes = r_u32(0x800883B8);
            w_u32(0x8008832C, block);
            w_u32(0x800883B8, pool_bytes - (units << 4));
            w_u32(owner, allocation + 16);
            w_u32(0x800880DC, 0);
            return allocation + 16;
        }
        block = r_u32(block);
    } while (block != r_u32(0x8008832C));

    if (r_u32(0x800880DC) != 0)
    {
        for (;;)
        {
            SetDispMask(0);
            SetDispMask(1);
        }
    }
    w_u32(0x800880DC, 1);
    w_u32(0x800880D8, r_u32(0x800880D8) + 1);
    td5_debug_log(0x80013C28);
    previous = 0;
    DrawSync(0);
    destination = r_u32(0x80088328);
    block = destination;
    do
    {
        flags = r_u32(block + 12);
        next = r_u32(block);
        if ((flags & 1) != 0)
        {
            if (block != destination)
            {
                td5_8007a3a8(destination, block, (flags >> 1) << 4);
                flags = r_u32(destination + 12);
                update_owner = r_u32(destination + 8);
                w_u32(destination, destination + ((flags >> 1) << 4));
                w_u32(update_owner, destination + 16);
            }
            flags = r_u32(destination + 12);
            w_u32(destination + 4, previous);
            previous = destination;
            destination += (flags >> 1) << 4;
        }
        block = next;
    } while (block != r_u32(0x80088328));
    flags = r_u32(destination + 12);
    pool_bytes = r_u32(0x80088330);
    w_u32(destination, block);
    w_u32(destination + 4, previous);
    w_u32(0x8008832C, destination);
    w_u32(destination + 12, flags & 0xFFFFFFFE);
    w_u32(destination + 12, ((pool_bytes - (destination - block)) >> 4) << 1);
    w_u32(block + 4, destination);
    return td5_alloc_scene_memory(bytes, owner);
}

uint32 td5_free_scene_memory(uint32 buffer)
{
    uint32 flags;
    uint32 next;
    uint32 previous;
    uint32 free_bytes;
    uint32 next_flags;
    uint32 block = buffer - 16;
    uint32 result;

    FUNCTION_MARKER(0x8006B380, "SLUS_006.10");
    flags = r_u32(buffer - 4);
    next = r_u32(buffer - 16);
    previous = r_u32(buffer - 12);
    flags &= 0xFFFFFFFE;
    free_bytes = r_u32(0x800883B8);
    w_u32(buffer - 4, flags);
    next_flags = r_u32(next + 12);
    w_u32(0x8008832C, previous);
    w_u32(0x800883B8, free_bytes + (flags << 3));
    if ((next_flags & 1) == 0 && block < next)
    {
        flags = r_u32(buffer - 4);
        next = r_u32(buffer - 16);
        w_u32(buffer - 4, (flags & 1) | (((flags >> 1) + (next_flags >> 1)) << 1));
        w_u32(r_u32(next) + 4, block);
        next = r_u32(buffer - 16);
        w_u32(buffer - 16, r_u32(next));
    }
    previous = r_u32(block + 4);
    flags = r_u32(previous + 12);
    result = previous < block;
    if ((flags & 1) == 0)
    {
        result = flags >> 1;
        if (previous < block)
        {
            result = (result + (r_u32(block + 12) >> 1)) << 1;
            w_u32(previous + 12, result);
            next = r_u32(block);
            previous = r_u32(block + 4);
            w_u32(next + 4, previous);
            previous = r_u32(block + 4);
            result = r_u32(block);
            w_u32(previous, result);
        }
    }
    return result;
}

uint32 td5_80052924(sint32 car, uint32 original_name_address)
{
    uint32 owner = original_name_address - 8;
    uint32 selection;
    sint32 car_id;
    uint32 descriptor;
    uint32 suffix;
    uint32 length;
    uint32 file_size;
    uint32 sound;
    uint32 transfer_address;
    uint32 payload;
    uint32 index;
    char filename[256];

    FUNCTION_MARKER(0x80052924, "SLUS_006.10");
    for (index = 0; index < 5; ++index)
        VSync(0);
    td5_alloc_scene_memory(0x20000, owner);
    selection = 0x8008CF72 + (uint32)car * 24 + r_u32(0x8008871C);
    car_id = r_s8(selection);
    if (car_id < 0)
        return selection;
    td5_debug_log(0x800113B4, car);
    descriptor = (uint32)car_id * 12;
    suffix = r_u32(0x8007E7B4 + descriptor);
    for (length = 0; r_u8(suffix + length) != 0; ++length) {}
    sprintf(filename, (const char *)psx_addr(0x800113C8, 13), (const char *)psx_addr(suffix, length + 1));
    file_size = td5_archive_file_size(filename, 1, original_name_address, 0);
    td5_load_archive_file(filename, r_u32(owner), original_name_address);
    sound = ((uint32)car + 10) * 12;
    transfer_address = r_u32(0x8008CD28 + sound);
    SpuSetTransferMode(0);
    SpuSetTransferStartAddr(transfer_address);
    payload = file_size - 80;
    SpuWrite(psx_addr(r_u32(owner) + 80, payload), payload);
    SpuIsTransferCompleted(1);
    w_u32(0x8008CD30 + sound, (sint32)(payload << 5) / 7);
    if (car == 0)
    {
        td5_debug_log(0x800113D8, 0);
        suffix = r_u32(0x8007E7BC + descriptor);
        for (length = 0; r_u8(suffix + length) != 0; ++length) {}
        sprintf(filename, (const char *)psx_addr(0x800113C8, 13), (const char *)psx_addr(suffix, length + 1));
        file_size = td5_archive_file_size(filename, 1, original_name_address, 0);
        td5_load_archive_file(filename, r_u32(owner), original_name_address);
        transfer_address = r_u32(0x8008CDD0);
        SpuSetTransferMode(0);
        SpuSetTransferStartAddr(transfer_address);
        payload = file_size - 80;
        SpuWrite(psx_addr(r_u32(owner) + 80, payload), payload);
        SpuIsTransferCompleted(1);
        w_u32(0x8008CDD8, (sint32)(payload << 5) / 7);
    }
    return td5_free_scene_memory(r_u32(owner));
}

uint32 td5_80052b74(sint32 song, uint32 archive_error_index, uint32 original_stack)
{
    uint32 owner = original_stack - 0x28 + 0x10;
    uint32 entry;
    uint32 filename;
    uint32 length;
    uint32 file_size;
    uint32 transfer_address;
    uint32 payload;

    FUNCTION_MARKER(0x80052B74, "SLUS_006.10");
    td5_alloc_scene_memory(0x4B000, owner);
    td5_debug_log(0x800113EC, song);
    entry = 0x8007E988 + ((uint32)song << 2);
    filename = r_u32(entry);
    for (length = 0; r_u8(filename + length) != 0; ++length) {}
    file_size = td5_archive_file_size((const char *)psx_addr(filename, length + 1), 1, archive_error_index, 0);
    filename = r_u32(entry);
    for (length = 0; r_u8(filename + length) != 0; ++length) {}
    td5_load_archive_file((const char *)psx_addr(filename, length + 1), r_u32(owner), archive_error_index);
    transfer_address = r_u32(0x8008CDC4);
    SpuSetTransferMode(0);
    SpuSetTransferStartAddr(transfer_address);
    payload = file_size - 80;
    SpuWrite(psx_addr(r_u32(owner) + 80, payload), payload);
    SpuIsTransferCompleted(1);
    w_u32(0x8008CDCC, (sint32)(payload << 5) / 7);
    return td5_free_scene_memory(r_u32(owner));
}

uint32 td5_8007a3a8(uint32 destination, uint32 source, uint32 bytes)
{
    sint32 words = (sint32)(bytes + 3) >> 2;
    sint32 groups = words / 16;
    sint32 remaining = words - groups * 16;
    sint32 group;
    uint32 pair;
    uint32 first;
    uint32 second;
    sint32 index;

    FUNCTION_MARKER(0x8007A3A8, "SLUS_006.10");
    for (group = 0; group < groups; ++group)
    {
        for (pair = 0; pair < 64; pair += 8)
        {
            first = r_u32(source + pair);
            second = r_u32(source + pair + 4);
            w_u32(destination + pair, first);
            w_u32(destination + pair + 4, second);
        }
        source += 64;
        destination += 64;
    }
    for (index = 0; index < remaining; ++index)
    {
        first = r_u32(source);
        source += 4;
        w_u32(destination, first);
        destination += 4;
    }
    return 0;
}

uint32 td5_800532a8(void)
{
    sint32 index;
    sint32 voice = 8;

    FUNCTION_MARKER(0x800532A8, "SLUS_006.10");
    for (index = 0; index < 2; ++index)
    {
        td5_play_positioned_sound(2, voice, 0, 4096, 0x80098664 + (uint32)(index / 2) * 864, 0);
        voice += 2;
    }
    return 0;
}

uint32 td5_80052d80(void)
{
    uint32 offset;
    sint32 index;
    sint32 count;

    FUNCTION_MARKER(0x80052D80, "SLUS_006.10");
    offset = r_u32(0x80088498) * 864;
    td5_play_positioned_sound(10, 0, 0, 4096, 0x80098664 + offset, 0x80098634 + offset);
    count = r_s32(0x800875E4);
    index = 1;
    for (;;)
    {
        if (count >= 5)
        {
            if (index >= 4)
                break;
        }
        else if (index >= count)
            break;
        offset = (uint32)index * 864;
        td5_play_positioned_sound(11, index * 2, 0, 4096, 0x80098664 + offset, 0x80098634 + offset);
        count = r_s32(0x800875E4);
        ++index;
    }
    for (index = 0; index < 21; ++index)
        td5_80054674(index);
    return 0;
}

uint32 td5_80054674(sint32 voice)
{
    uint32 offset = (uint32)voice * 92;
    uint32 paired;

    FUNCTION_MARKER(0x80054674, "SLUS_006.10");
    paired = r_u32(0x80088668);
    w_u32(0x800919B4 + offset, 1);
    if (paired != 0)
    {
        offset = ((uint32)voice + 1) * 92;
        w_u32(0x800919B4 + offset, 1);
    }
    return offset;
}

TD5_NODE_POSITION_RESULT td5_8001dbc0(uint32 node, uint32 position)
{
    TD5_NODE_POSITION_RESULT result;
    sint32 index;
    uint32 descriptor;
    uint32 type;
    uint32 lanes;
    uint32 first_index;
    uint32 second_index;
    sint32 first_offset;
    sint32 second_offset;
    uint32 base;
    uint32 points;
    uint32 first;
    uint32 second;
    sint32 a;
    sint32 b;
    sint32 c;
    sint32 d;
    uint32 delta;

    FUNCTION_MARKER(0x8001DBC0, "SLUS_006.10");
    index = r_s16(node);
    descriptor = r_u32(0x800884C4) + (uint32)index * 24;
    w_u16(node + 4, index);
    w_u16(node + 6, index);
    type = r_u8(descriptor);
    lanes = r_u8(descriptor + 3);
    first_index = r_u16(descriptor + 4);
    second_index = r_u16(descriptor + 6);
    first_offset = r_s8(0x8007DD34 + type * 2);
    second_offset = r_s8(0x8007DD35 + type * 2);
    result.lane = r_u8(node + 12);
    lanes &= 15;
    first_index += (uint32)first_offset;
    second_index += (uint32)second_offset;
    if (result.lane >= (sint32)lanes)
    {
        result.lane = (sint32)lanes - 1;
        w_u8(node + 12, result.lane);
    }
    base = r_u32(descriptor + 12);
    points = r_u32(0x8008850C);
    first = points + (first_index + (uint32)result.lane) * 6;
    second = points + (second_index + (uint32)result.lane) * 6;
    a = r_s16(first);
    b = r_s16(first + 6);
    c = r_s16(second);
    d = r_s16(second + 6);
    delta = (uint32)(a + b + c + d) << 6;
    w_u32(position, (base << 8) + delta);
    base = r_u32(descriptor + 16);
    a = r_s16(first + 2);
    b = r_s16(first + 8);
    result.second_y = r_s16(second + 2);
    d = r_s16(second + 8);
    delta = (uint32)(a + b + result.second_y + d) << 6;
    w_u32(position + 4, (base << 8) + delta);
    base = r_u32(descriptor + 20);
    a = r_s16(first + 4);
    b = r_s16(first + 10);
    c = r_s16(second + 4);
    d = r_s16(second + 10);
    result.z_delta = (uint32)(a + b + c + d) << 6;
    w_u32(position + 8, (base << 8) + result.z_delta);
    return result;
}

sint32 td5_8004d030(uint32 car, sint32 fallback_lane, sint32 fallback_y)
{
    sint32 node;
    uint32 descriptor;
    uint32 first_index;
    uint32 second_index;
    uint32 points;
    uint32 first;
    uint32 second;
    uint32 type;
    sint32 a;
    sint32 b;
    sint32 c;
    sint32 d;
    sint32 x;
    sint32 z;

    FUNCTION_MARKER(0x8004D030, "SLUS_006.10");
    node = r_s16(car + 112);
    descriptor = r_u32(0x800884C4) + (uint32)node * 24;
    first_index = r_u16(descriptor + 4);
    points = r_u32(0x8008850C);
    second_index = r_u16(descriptor + 6);
    type = r_u8(descriptor);
    first = points + first_index * 6;
    second = points + second_index * 6;
    switch (type)
    {
    case 1:
    case 2:
    case 5:
        a = r_s16(first);
        b = r_s16(second);
        c = r_s16(first + 6);
        d = r_s16(second + 6);
        x = a - b + c - d;
        a = r_s16(first + 4);
        b = r_s16(second + 4);
        c = r_s16(first + 10);
        d = r_s16(second + 10);
        z = a - b + c - d;
        break;
    case 3:
    case 4:
        a = r_s16(first);
        b = r_s16(second + 6);
        c = r_s16(first + 6);
        d = r_s16(second + 12);
        x = a - b + c - d;
        a = r_s16(first + 4);
        b = r_s16(second + 10);
        c = r_s16(first + 10);
        d = r_s16(second + 16);
        z = a - b + c - d;
        break;
    case 6:
    case 7:
        a = r_s16(first + 6);
        b = r_s16(second);
        c = r_s16(first + 12);
        d = r_s16(second + 6);
        x = a - b + c - d;
        a = r_s16(first + 10);
        b = r_s16(second + 4);
        c = r_s16(first + 16);
        d = r_s16(second + 10);
        z = a - b + c - d;
        break;
    default:
        return td5_8004cf8c(fallback_lane, fallback_y);
    }
    return td5_8004cf8c(x / 4, z / 4);
}

uint32 td5_8004d984(uint32 slot, uint32 node, uint32 side, uint32 raised, uint32 height_address)
{
    uint32 offset = slot * 864;
    uint32 car = 0x800984A0 + offset;
    uint32 height;
    TD5_NODE_POSITION_RESULT position;

    FUNCTION_MARKER(0x8004D984, "SLUS_006.10");
    w_u16(0x80098510 + offset, node);
    w_u8(0x8009851C + offset, side);
    position = td5_8001dbc0(0x80098510 + offset, 0x80098664 + offset);
    height = ((uint32)td5_8004d030(car, position.lane, position.second_y) + 2048) << 8;
    w_u32(0x8009865C + offset, height);
    if (raised != 0)
        w_u32(0x8009865C + offset, height + 0x80000);
    td5_80024134(car, height_address);
    return td5_8004d660(slot);
}

