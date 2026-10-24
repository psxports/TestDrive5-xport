#include "td5_functions.h"
#include <string.h>

// TODO: Replace memory-card file creation with the native xport_save transaction
uint32 td5_80056cfc(uint32 slot, uint32 original_stack)
{
    uint32 buffer = original_stack - 0x38 + 0x10;
    uint32 length;
    sint32 descriptor;

    FUNCTION_MARKER(0x80056CFC, "SLUS_006.10");
    length = sprintfPSX(buffer, 0x8001143C, slot);
    descriptor = openPSX((const char *)psx_addr(buffer, length + 1), 0x10200);
    if (descriptor < 0)
        return 0;
    closePSX(descriptor);
    return 1;
}

static uint32 td5_card_event_index(uint32 events)
{
    uint32 index;

    for (;;)
        for (index = 0; index < 4; ++index)
            if (TestEvent(r_s32(events + index * 4)) == 1)
                return index;
}

// TODO: Remove card formatting when the UI moves to xport_save/load
uint32 td5_8005712c(uint32 slot, uint32 original_stack)
{
    uint32 index, status, length;
    uint32 buffer = original_stack - 0x30 + 0x10;

    FUNCTION_MARKER(0x8005712C, "SLUS_006.10");
    _card_info((sint32)(slot << 4));
    status = td5_card_event_index(0x800882F8);
    td5_debug_log(0x80087C34, status);
    if (status != 0)
    {
        if (status < 3)
            return 0;
        for (index = 0; index < 4; ++index)
            TestEvent(r_s32(0x80088308 + index * 4));
        _card_clear(slot << 4);
        td5_card_event_index(0x80088308);
        status = td5_card_event_index(0x800882F8);
        if (status < 3)
            return 0;
    }
    length = sprintfPSX(buffer, 0x80087C38, slot);
    return format((const char *)psx_addr(buffer, length + 1)) != 0;
}

uint32 td5_80057d74(sint32 incoming_v0, uint32 original_stack)
{
    uint32 frame = original_stack - 0x88;
    uint32 source, destination, index, word, values[4], result;
    uint32 width, height, value, horizontal, vertical;
    sint32 scale, old_width, old_height;

    FUNCTION_MARKER(0x80057D74, "SLUS_006.10");
    if (incoming_v0 > 0)
        w_u32(0x80088448, (uint32)incoming_v0 - 256);
    scale = r_s32(0x80088448);
    if (scale < 0)
        scale = 0;
    else if (scale >= 4097)
        scale = 4096;
    td5_apply_display_buffer();
    source = r_u32(0x800880F0);
    destination = frame + 0x10;
    for (index = 0; index < 5; ++index)
    {
        for (word = 0; word < 4; ++word)
            values[word] = r_u32(source + word * 4);
        for (word = 0; word < 4; ++word)
            w_u32(destination + word * 4, values[word]);
        source += 16;
        destination += 16;
    }
    for (word = 0; word < 3; ++word)
        values[word] = r_u32(source + word * 4);
    for (word = 0; word < 3; ++word)
        w_u32(destination + word * 4, values[word]);
    destination = r_u32(0x800880F0);
    old_height = r_s16(destination + 6);
    vertical = (uint32)old_height * (uint32)scale;
    old_width = r_s16(destination + 4);
    horizontal = (uint32)old_width * (uint32)scale;
    w_u16(destination + 6, (uint32)old_height - (uint32)((sint32)vertical / 4096));
    width = r_u32(0x800880F4);
    horizontal = (uint32)old_width - (uint32)((sint32)horizontal / 4096);
    width = (uint32)((sint32)width / 2) * (uint32)scale;
    w_u16(destination + 4, horizontal);
    height = r_u32(0x800880F8);
    height = (uint32)((sint32)height / 2) * (uint32)scale;
    value = r_u16(destination);
    w_u16(destination, value + (uint32)((sint32)width / 4096));
    value = r_u16(destination + 2);
    w_u16(destination + 2, value + (uint32)((sint32)height / 4096));
    PutDrawEnv((DRAWENV *)psx_addr(destination, sizeof(DRAWENV)));
    destination = r_u32(0x800880F0);
    source = frame + 0x10;
    for (index = 0; index < 5; ++index)
    {
        for (word = 0; word < 4; ++word)
            values[word] = r_u32(source + word * 4);
        for (word = 0; word < 4; ++word)
            w_u32(destination + word * 4, values[word]);
        source += 16;
        destination += 16;
    }
    for (word = 0; word < 3; ++word)
        values[word] = r_u32(source + word * 4);
    result = values[0];
    for (word = 0; word < 3; ++word)
        w_u32(destination + word * 4, values[word]);
    return result;
}

uint32 td5_80058474(void)
{
    uint32 buttons, result, value;
    sint32 choice;

    FUNCTION_MARKER(0x80058474, "SLUS_006.10");
    buttons = r_u16(0x800884E8);
    w_u16(0x8008C508, 5);
    w_u16(0x8008C50A, 8);
    w_u16(0x8008C50C, 4);
    w_u16(0x8008C50E, 0xFFFF);
    w_u16(0x8008C510, 0xFFFF);
    w_u16(0x8008C512, 0xFFFF);
    w_u16(0x800884EC, 62);
    if ((buttons & 0x4000) == 0)
        return 62;
    td5_800530f0();
    choice = r_s16(0x800884E0);
    if (choice == 1)
    {
        result = r_u32(0x80088530);
        if (result == 0)
        {
            value = r_u32(0x80087738);
            result = 30;
            if (value == 0)
                w_u32(0x80088520, result);
        }
        return result;
    }
    if (choice == 0)
    {
        w_u16(0x80088794, 1);
        w_u32(0x8008858C, 4);
        return 4;
    }
    if (choice != 2)
        return 1;
    w_u16(0x80088794, 1);
    w_u32(0x80088620, 80);
    result = td5_80016c2c();
    if (result == 1)
    {
        result = r_u32(0x80088530);
        if (result == 0)
        {
            value = r_u32(0x80087660);
            result = value == 1 ? 120 : 190;
            w_u32(0x80088620, result);
        }
    }
    w_u32(0x8008858C, 0);
    return result;
}

uint32 td5_800585e4(void)
{
    uint32 buttons, choice, result, value, mode, level;

    FUNCTION_MARKER(0x800585E4, "SLUS_006.10");
    buttons = r_u16(0x800884E8);
    w_u16(0x8008C508, 2);
    w_u16(0x8008C50A, 1);
    w_u16(0x8008C50C, 8);
    w_u16(0x8008C50E, 7);
    w_u16(0x8008C510, 4);
    w_u16(0x8008C512, 0xFFFF);
    w_u16(0x800884EC, 72);
    w_u32(0x80087694, 0);
    if ((buttons & 0x4000) == 0)
        return 72;
    td5_800530f0();
    choice = (uint32)r_s16(0x800884E0);
    if (choice >= 5)
        return choice << 2;
    switch (choice)
    {
        case 0:
            value = r_u32(0x800885F0);
            mode = r_u32(0x80087728);
            ++value;
            w_u32(0x800885F0, value);
            if (mode == 3 && (sint32)value < 10)
            {
                w_u32(0x80088520, 10);
                return 10;
            }
            w_u16(0x80088794, 1);
            w_u32(0x8008858C, 5);
            return 5;
        case 1:
            level = r_u32(0x80088770);
            level = r_u32(level);
            value = r_u8(level + 30);
            if ((value & 8) == 0)
                return 1;
            w_u16(0x80088794, 1);
            w_u32(0x8008858C, 6);
            return 6;
        case 2:
            result = r_u32(0x80088530);
            if (result == 0)
            {
                value = r_u32(0x80087738);
                result = 30;
                if (value == 0)
                    w_u32(0x80088520, result);
            }
            return result;
        case 3:
            w_u32(0x80087694, 1);
            w_u32(0x80088520, 20);
            return 20;
        default:
            w_u16(0x80088794, 1);
            w_u32(0x8008858C, 0);
            w_u32(0x80088620, 80);
            return 80;
    }
}

uint32 td5_800587b8(uint32 incoming_v1)
{
    uint32 result, value;
    sint32 choice;

    FUNCTION_MARKER(0x800587B8, "SLUS_006.10");
    w_u16(0x8008C508, 2);
    w_u16(0x8008C50A, 8);
    w_u16(0x8008C50C, 4);
    w_u16(0x8008C50E, 0xFFFF);
    w_u16(0x8008C510, 0xFFFF);
    w_u16(0x8008C512, 0xFFFF);
    w_u16(0x800884EC, 72);
    if ((incoming_v1 & 0x4000) == 0)
        return 72;
    td5_800530f0();
    choice = r_s16(0x800884E0);
    if (choice == 1)
    {
        result = r_u32(0x80088530);
        if (result == 0)
        {
            value = r_u32(0x80087738);
            result = 30;
            if (value == 0)
                w_u32(0x80088520, result);
        }
        return result;
    }
    if (choice == 0)
    {
        result = r_u32(0x800885F0);
        w_u16(0x80088794, 1);
        w_u32(0x8008858C, 5);
        ++result;
        w_u32(0x800885F0, result);
        return result;
    }
    if (choice != 2)
        return 1;
    w_u16(0x80088794, 1);
    w_u32(0x8008858C, 0);
    w_u32(0x80088620, 80);
    return 80;
}

uint32 td5_800588ec(uint32 incoming_v1)
{
    uint32 result, first, second;
    sint32 choice;

    FUNCTION_MARKER(0x800588EC, "SLUS_006.10");
    w_u16(0x8008C508, 9);
    w_u16(0x8008C50A, 4);
    w_u16(0x8008C50C, 0xFFFF);
    w_u16(0x8008C50E, 0xFFFF);
    w_u16(0x8008C510, 0xFFFF);
    w_u16(0x8008C512, 0xFFFF);
    w_u16(0x800884EC, 72);
    if ((incoming_v1 & 0x4000) == 0)
        return 72;
    td5_800530f0();
    choice = r_s16(0x800884E0);
    if (choice == 0)
    {
        result = r_u32(0x80088530);
        w_u16(0x80088794, 1);
        w_u32(0x8008858C, 5);
        if (result != 0)
            return result;
        result = r_u32(0x80087738);
        if (result != 0)
            return result;
        first = r_u8(0x8008CF8A);
        result = r_u32(0x800885F0);
        second = (uint32)r_s8(0x8008CF72);
        result ^= 1;
        w_u8(0x8008CF72, first);
        w_u8(0x8008CF8A, second);
        w_u32(0x800885F0, result);
        return result;
    }
    if (choice != 1)
        return 1;
    w_u16(0x80088794, 1);
    w_u32(0x8008858C, 0);
    w_u32(0x80088620, 80);
    return 80;
}

uint32 td5_80058a08(void)
{
    uint32 buttons, result, value, first, second;
    sint32 choice;

    FUNCTION_MARKER(0x80058A08, "SLUS_006.10");
    buttons = r_u16(0x800884E8);
    w_u16(0x8008C50A, 8);
    w_u16(0x8008C50C, 4);
    w_u16(0x8008C510, 0xFFFF);
    w_u16(0x8008C50E, 0xFFFF);
    w_u16(0x8008C512, 0xFFFF);
    w_u16(0x8008C508, 0);
    w_u16(0x800884EC, 72);
    if ((buttons & 0x4000) == 0)
        return 72;
    td5_800530f0();
    choice = r_s16(0x800884E0);
    if (choice == 1)
    {
        result = r_u32(0x80088530);
        if (result == 0)
        {
            value = r_u32(0x80087738);
            result = 30;
            if (value == 0)
                w_u32(0x80088520, result);
        }
        return result;
    }
    if (choice == 0)
    {
        result = r_u32(0x80088530);
        w_u16(0x80088794, 1);
        w_u32(0x8008858C, 5);
        if (result != 0)
            return result;
        result = r_u32(0x80087738);
        if (result != 0)
            return result;
        value = r_u32(0x800885F0);
        w_u32(0x800885F0, value ^ 1);
        td5_init_race_results();
        first = r_u8(0x8008CF8A);
        second = (uint32)r_s8(0x8008CF72);
        w_u8(0x8008CF72, first);
        w_u8(0x8008CF8A, second);
        return 0x8008CF72;
    }
    if (choice != 2)
        return 1;
    w_u16(0x80088794, 1);
    w_u32(0x8008858C, 0);
    w_u32(0x80088620, 80);
    return 80;
}

uint32 td5_80058b88(void)
{
    uint32 buttons, value, result, level;
    sint32 choice;

    FUNCTION_MARKER(0x80058B88, "SLUS_006.10");
    buttons = r_u16(0x800884E8);
    w_u16(0x8008C50C, 4);
    w_u16(0x8008C50E, 0xFFFF);
    w_u16(0x8008C510, 0xFFFF);
    w_u16(0x8008C512, 0xFFFF);
    w_u16(0x8008C508, 0);
    w_u16(0x8008C50A, 1);
    w_u16(0x800884EC, 88);
    if ((buttons & 0x4000) == 0)
        return 88;
    td5_800530f0();
    choice = r_s16(0x800884E0);
    if (choice == 1)
    {
        level = r_u32(0x80088770);
        level = r_u32(level);
        if ((r_u8(level + 30) & 8) != 0)
        {
            w_u16(0x80088794, 1);
            w_u32(0x8008858C, 6);
        }
        return 6;
    }
    if (choice >= 2)
    {
        if (choice == 2)
        {
            w_u16(0x80088794, 1);
            w_u32(0x8008858C, 0);
            w_u32(0x80088620, 80);
        }
        return 80;
    }
    if (choice != 0)
        return 3;
    if (r_u32(0x80087728) == 3)
    {
        w_u32(0x80088520, 10);
        return 10;
    }
    value = r_u8(0x800987FB);
    w_u16(0x80088794, 1);
    w_u32(0x8008858C, 5);
    if (value != 0)
        return 5;
    result = r_u32(0x80088530);
    if (result == 0)
    {
        result = r_u32(0x800885F0) + 1;
        w_u32(0x800885F0, result);
    }
    return result;
}

uint32 td5_80059268(uint32 incoming_v0)
{
    uint32 allocation;

    FUNCTION_MARKER(0x80059268, "SLUS_006.10");
    allocation = r_u32(0x800883BC);
    if (incoming_v0 == 0)
        return td5_free_memory(allocation);
    return td5_free_scene_memory(allocation);
}

uint32 td5_80059490(void)
{
    uint16 positions_x[42], positions_y[42];
    uint8 shades[84];
    uint32 width, packet, bank, ticks, column, row, point, shade, value;
    uint32 buffer, head_address, tag, head, result;
    sint32 wave, divisor, cosine;

    FUNCTION_MARKER(0x80059490, "SLUS_006.10");
    width = r_u32(0x800880F4) == 512 ? 256 : 160;
    packet = r_u32(0x800883BC);
    bank = r_u32(0x80088104);
    ticks = r_u32(0x80088318);
    packet += bank * 1080;
    w_u32(0x80088318, ticks + 1);
    for (column = 0; column < 7; ++column)
    {
        for (row = 0; row < 6; ++row)
        {
            ticks = r_u32(0x80088318);
            wave = rsin((sint32)(ticks * 212 + column * 768 + row * 768)) / 64;
            ticks = r_u32(0x80088318);
            cosine = rcos((sint32)(ticks * 238 + column * 512 + row * 1024));
            divisor = wave + 1280;
            value = (uint32)cosine + (column * 2 - 6) * 22000;
            value = (uint32)td5_divide_checked((sint32)value, divisor);
            ticks = r_u32(0x80088318);
            point = row * 7 + column;
            positions_x[point] = (uint16)(value + width);
            cosine = rcos((sint32)(ticks * 144 + column * 768 + row * 384));
            value = (uint32)cosine - 60000 + row * 24000;
            positions_y[point] = (uint16)((uint32)td5_divide_checked((sint32)value, divisor) + 128);
            value = 16 - (uint32)((wave - 64) / 4);
            shades[42 + point] = (uint8)value;
            shades[point] = (uint8)((value & 255) >> 1);
        }
    }
    for (column = 0; column < 6; ++column)
    {
        for (row = 0; row < 5; ++row)
        {
            point = row * 7 + column;
            w_u16(packet + 8, positions_x[point]);
            w_u16(packet + 10, positions_y[point]);
            w_u16(packet + 16, positions_x[point + 1]);
            w_u16(packet + 18, positions_y[point + 1]);
            w_u16(packet + 24, positions_x[point + 7]);
            w_u16(packet + 26, positions_y[point + 7]);
            w_u16(packet + 32, positions_x[point + 8]);
            w_u16(packet + 34, positions_y[point + 8]);
            shade = point + ((column + row) & 1) * 42;
            w_u8(packet + 4, shades[shade]);
            w_u8(packet + 5, shades[shade]);
            w_u8(packet + 6, shades[shade]);
            w_u8(packet + 12, shades[shade + 1]);
            w_u8(packet + 13, shades[shade + 1]);
            w_u8(packet + 14, shades[shade + 1]);
            w_u8(packet + 20, shades[shade + 7]);
            w_u8(packet + 21, shades[shade + 7]);
            w_u8(packet + 22, shades[shade + 7]);
            w_u8(packet + 28, shades[shade + 8]);
            w_u8(packet + 29, shades[shade + 8]);
            w_u8(packet + 30, shades[shade + 8]);
            buffer = r_u32(0x800880F0);
            head_address = r_u32(buffer + 0x70) + 800;
            tag = r_u32(packet);
            head = r_u32(head_address);
            w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
            head_address = r_u32(buffer + 0x70) + 800;
            head = r_u32(head_address);
            w_u32(head_address, (head & 0xFF000000) | (packet & 0xFFFFFF));
            packet += 36;
        }
    }
    bank = r_u32(0x80088104);
    head_address = r_u32(buffer + 0x70) + 800;
    packet = 0x800911E0 + bank * 12;
    tag = r_u32(packet);
    head = r_u32(head_address);
    w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
    head_address = r_u32(buffer + 0x70) + 800;
    head = r_u32(head_address);
    result = (head & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(head_address, result);
    return result;
}

uint32 td5_80059ab4(uint32 incoming_v0)
{
    uint32 index, allocation;

    FUNCTION_MARKER(0x80059AB4, "SLUS_006.10");
    for (index = 0; index < 4; ++index)
    {
        allocation = r_u32(0x8009D918 + index * 4);
        if (incoming_v0 == 0)
            td5_free_memory(allocation);
        else
            td5_free_scene_memory(allocation);
    }
    return 0;
}

uint32 td5_80059c48(uint32 player)
{
    uint32 center, rows, y, index, half_width, length, packet, buffer;
    uint32 left, right, head_address, tag, head, value, selection, text, result;
    uint32 pad = 0x800884E8 + player * 2;
    sint32 volume, quotient;

    FUNCTION_MARKER(0x80059C48, "SLUS_006.10");
    center = r_u32(0x800880F4) == 512 ? 256 : 160;
    if (r_u32(0x800880A0) != 0)
    {
        text = r_u32(0x8007FC24);
        length = (uint32)strlen(td5_hud_string(text));
        text = r_u32(0x8007FC24);
        td5_80038994(td5_hud_string(text), center - ((length * 9) >> 1), 110, 0);
        text = r_u32(0x8007FC28);
        length = (uint32)strlen(td5_hud_string(text));
        text = r_u32(0x8007FC28);
        selection = r_u32(0x8008809C);
        td5_80038994(td5_hud_string(text), center - (length * 9 + 4), 122, selection == 0);
        selection = r_u32(0x8008809C);
        text = r_u32(0x8007FC2C);
        td5_80038994(td5_hud_string(text), center | 4, 122, selection == 1);
        if ((r_u16(pad) & 0x80) != 0 && r_u32(0x8008809C) == 1)
        {
            td5_80053030();
            w_u32(0x8008809C, 0);
        }
        result = r_u16(pad) & 0x20;
        if (result != 0)
        {
            result = r_u32(0x8008809C);
            if (result == 0)
            {
                td5_80053030();
                result = 1;
                w_u32(0x8008809C, result);
            }
        }
        return result;
    }
    if (r_u32(0x8008772C) != 0)
    {
        rows = 5;
        y = 95;
    }
    else
    {
        rows = 4;
        y = 100;
    }
    text = r_u32(0x800880A4);
    length = (uint32)strlen(td5_hud_string(text));
    text = r_u32(0x800880A4);
    td5_80038994(td5_hud_string(text), center - ((length * 9) >> 1), y - 8, 0);
    y += 8;
    for (index = 0; index < rows; ++index)
    {
        text = r_u32(0x8007FC0C + index * 4);
        length = (uint32)strlen(td5_hud_string(text));
        half_width = (length * 9) >> 1;
        if (index - 1 < 2)
        {
            packet = 0x8008CE58 + r_u32(0x80088104) * 36 + index * 72;
            w_u8(packet + 7, 0x38);
            w_u8(packet + 5, 255);
            w_u8(packet + 12, 255);
            w_u8(packet + 21, 255);
            w_u8(packet + 3, 8);
            w_u8(packet + 4, 0);
            w_u8(packet + 6, 0);
            w_u8(packet + 13, 0);
            w_u8(packet + 14, 0);
            w_u8(packet + 20, 0);
            w_u8(packet + 22, 0);
            w_u8(packet + 28, 255);
            w_u8(packet + 29, 0);
            w_u8(packet + 30, 0);
            buffer = r_u32(0x800880F0);
            left = center + half_width - 20;
            right = center + half_width + 20;
            w_u16(packet + 8, (uint16)left);
            w_u16(packet + 10, (uint16)(y + 6));
            w_u16(packet + 16, (uint16)right);
            w_u16(packet + 18, (uint16)(y + 6));
            w_u16(packet + 24, (uint16)left);
            w_u16(packet + 26, (uint16)(y + 10));
            w_u16(packet + 32, (uint16)right);
            w_u16(packet + 34, (uint16)(y + 10));
            head_address = r_u32(buffer + 0x70) + 8;
            tag = r_u32(packet);
            head = r_u32(head_address);
            w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
            head_address = r_u32(buffer + 0x70) + 8;
            head = r_u32(head_address);
            w_u32(head_address, (head & 0xFF000000) | (packet & 0xFFFFFF));
            value = index == 1 ? r_u32(0x80087704) : r_u32(0x80087700);
            volume = (sint32)(4096 - value);
            packet = 0x800A4FA0 + r_u32(0x80088104) * 36 + index * 72;
            quotient = volume / 102;
            w_u8(packet + 3, 8);
            w_u8(packet + 4, 15);
            w_u8(packet + 12, 15);
            w_u8(packet + 20, 15);
            w_u8(packet + 28, 15);
            w_u8(packet + 7, 0x38);
            w_u8(packet + 5, 0);
            w_u8(packet + 6, 0);
            w_u8(packet + 13, 0);
            w_u8(packet + 14, 0);
            w_u8(packet + 21, 0);
            w_u8(packet + 22, 0);
            w_u8(packet + 29, 0);
            w_u8(packet + 30, 0);
            w_u16(packet + 10, (uint16)(y + 6));
            w_u16(packet + 18, (uint16)(y + 6));
            w_u16(packet + 26, (uint16)(y + 10));
            w_u16(packet + 34, (uint16)(y + 10));
            buffer = r_u32(0x800880F0);
            left = center + half_width - ((uint32)quotient - 20);
            right = left + (uint32)quotient;
            w_u16(packet + 8, (uint16)left);
            w_u16(packet + 16, (uint16)right);
            w_u16(packet + 24, (uint16)left);
            w_u16(packet + 32, (uint16)right);
            head_address = r_u32(buffer + 0x70);
            tag = r_u32(packet);
            head = r_u32(head_address);
            w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
            head_address = r_u32(buffer + 0x70);
            head = r_u32(head_address);
            half_width += 20;
            w_u32(head_address, (head & 0xFF000000) | (packet & 0xFFFFFF));
        }
        if (index == 4 && r_u32(0x80088698) == 0)
        {
            selection = r_u32(0x8008809C);
            text = r_u32(0x8007FC20);
        }
        else
        {
            selection = r_u32(0x8008809C);
            text = r_u32(0x8007FC0C + index * 4);
        }
        td5_80038994(td5_hud_string(text), center - half_width, y, selection == index);
        y += 12;
    }
    if ((r_u16(pad) & 0x10) != 0)
    {
        value = r_u32(0x8008809C);
        if ((sint32)value > 0)
        {
            w_u32(0x8008809C, value - 1);
            td5_80053030();
        }
    }
    if ((r_u16(pad) & 0x40) != 0 && r_s32(0x8008809C) < (sint32)(rows - 1))
    {
        td5_80053030();
        w_u32(0x8008809C, r_u32(0x8008809C) + 1);
    }
    if ((r_u16(pad) & 0x80) != 0 && r_u32(0x8008809C) == 1 && r_s32(0x80087704) > 0)
    {
        td5_80053030();
        w_u32(0x80087704, r_u32(0x80087704) - 512);
    }
    if ((r_u16(pad) & 0x20) != 0 && r_u32(0x8008809C) == 1 && r_s32(0x80087704) < 4096)
    {
        td5_80053030();
        w_u32(0x80087704, r_u32(0x80087704) + 512);
    }
    if ((r_u16(pad) & 0x80) != 0 && r_u32(0x8008809C) == 2 && r_s32(0x80087700) > 0)
    {
        td5_80053030();
        w_u32(0x80087700, r_u32(0x80087700) - 512);
    }
    if ((r_u16(pad) & 0x20) != 0 && r_u32(0x8008809C) == 2 && r_s32(0x80087700) < 4096)
    {
        td5_80053030();
        w_u32(0x80087700, r_u32(0x80087700) + 512);
    }
    if ((r_u16(pad) & 0xA0) != 0 && r_u32(0x8008809C) == 4)
    {
        td5_80053030();
        w_u32(0x80088698, r_u32(0x80088698) ^ 1);
    }
    volume = r_s32(0x80087700);
    value = volume < 0 ? 0 : volume < 4097 ? (uint32)volume : 4096;
    volume = r_s32(0x80087704);
    w_u32(0x80087700, value);
    result = volume < 4097;
    value = volume < 0 ? 0 : volume < 4097 ? (uint32)volume : 4096;
    w_u32(0x80087704, value);
    return result;
}

uint32 td5_8005a9f0(uint32 slot)
{
    uint32 entry = 0x800A4E28 + slot * 24;

    FUNCTION_MARKER(0x8005A9F0, "SLUS_006.10");
    memset(psx_addr(entry, 24), 0, 24);
    return entry;
}

uint32 td5_8005b470(uint32 slot)
{
    FUNCTION_MARKER(0x8005B470, "SLUS_006.10");
    if (td5_8005aac8() == 1)
        return 1;
    if (r_u32(0x800880D4) != 0)
        return 1;
    return r_u16(0x800A4E28 + slot * 24) != 0;
}

uint32 td5_80064260(uint32 incoming_v0)
{
    FUNCTION_MARKER(0x80064260, "SLUS_006.10");
    return incoming_v0;
}

uint32 td5_80068580(const char *path)
{
    FUNCTION_MARKER(0x80068580, "SLUS_006.10");
    return (uint32)erase(path);
}

uint32 td5_80068cd0(uint32 buffer0, uint32 size0, uint32 buffer1, uint32 size1)
{
    FUNCTION_MARKER(0x80068CD0, "SLUS_006.10");
    return InitPAD(buffer0, size0, buffer1, size1);
}

uint32 td5_80068ce0(void)
{
    FUNCTION_MARKER(0x80068CE0, "SLUS_006.10");
    return StartPAD();
}

uint32 td5_80068cf0(void)
{
    FUNCTION_MARKER(0x80068CF0, "SLUS_006.10");
    return xport_bios_stop_pad();
}

uint32 td5_80068d00(uint32 type, uint32 destination)
{
    FUNCTION_MARKER(0x80068D00, "SLUS_006.10");
    return PAD_init2(type, destination);
}

uint32 td5_8006b074(void)
{
    uint32 destination, previous, block, next, flags, owner, bytes, result;

    FUNCTION_MARKER(0x8006B074, "SLUS_006.10");
    DrawSync(0);
    destination = r_u32(0x80088328);
    previous = 0;
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
                owner = r_u32(destination + 8);
                w_u32(destination, destination + ((flags >> 1) << 4));
                w_u32(owner, destination + 16);
            }
            flags = r_u32(destination + 12);
            w_u32(destination + 4, previous);
            previous = destination;
            destination += (flags >> 1) << 4;
        }
        block = next;
    } while (block != r_u32(0x80088328));
    flags = r_u32(destination + 12);
    bytes = r_u32(0x80088330);
    w_u32(destination, block);
    w_u32(destination + 4, previous);
    w_u32(0x8008832C, destination);
    result = ((bytes - (destination - block)) >> 4) << 1;
    w_u32(destination + 12, flags & 0xFFFFFFFE);
    w_u32(destination + 12, result);
    w_u32(block + 4, destination);
    return result;
}

uint32 td5_8006b4b0(uint32 port)
{
    uint32 actuator = 0x80088688 + (uint32)((sint32)port >> 4) * 4;
    uint32 state, result;

    FUNCTION_MARKER(0x8006B4B0, "SLUS_006.10");
    state = (uint32)PadGetState((sint32)port);
    if (state == 1)
        w_u8(actuator + 2, 0);
    result = r_u8(actuator + 2);
    if (result != 0)
        return result;
    PadSetAct((sint32)port, (uint8 *)psx_addr(actuator, 2), 2);
    if (state == 2)
    {
        w_u8(actuator + 2, 1);
        return 1;
    }
    if (state != 6)
        return 6;
    result = (uint32)PadSetActAlign((sint32)port, (uint8 *)psx_addr(0x800880E0, 6));
    if (result != 0)
    {
        w_u8(actuator + 2, 1);
        result = 1;
    }
    return result;
}

uint32 td5_8006c810(uint32 buffer)
{
    uint32 draw = 0x80097668 + buffer * 116;
    uint32 display = draw + 92;

    FUNCTION_MARKER(0x8006C810, "SLUS_006.10");
    PutDrawEnv((DRAWENV *)psx_addr(draw, sizeof(DRAWENV)));
    PutDispEnv((DISPENV *)psx_addr(display, sizeof(DISPENV)));
    return display;
}

void td5_800696e8(void)
{
    FUNCTION_MARKER(0x800696E8, "SLUS_006.10");
    PadStopCom();
}

uint32 td5_8006c964(uint32 model)
{
    uint32 record, type, stride, vertices, vertex;
    sint32 count, index, value, maximum = 0;

    FUNCTION_MARKER(0x8006C964, "SLUS_006.10");
    record = model + r_u32(model + 48);
    while (r_u8(record + 3) != 0)
    {
        type = r_u8(record + 2);
        count = r_s16(record);
        record += 4;
        if (type > 3)
            continue;
        stride = type < 2 ? 20 : type == 2 ? 44 : 56;
        vertices = (type & 1) != 0 ? 4 : 3;
        for (index = 0; index < count; ++index)
        {
            for (vertex = 0; vertex < vertices; ++vertex)
            {
                value = r_s16(record + vertex * 2);
                if (maximum < value)
                    maximum = value;
            }
            record += stride;
        }
    }
    return (uint32)maximum + 1;
}

static sint32 td5_reflection_project(uint32 record, uint32 vertices)
{
    uint32 first, second, third;

    first = vertices + ((uint32)r_s16(record) << 3);
    second = vertices + ((uint32)r_s16(record + 2) << 3);
    third = vertices + ((uint32)r_s16(record + 4) << 3);
    xport_gte_write_data(0, r_u32(first));
    xport_gte_write_data(1, r_u32(first + 4));
    xport_gte_write_data(2, r_u32(second));
    xport_gte_write_data(3, r_u32(second + 4));
    xport_gte_write_data(4, r_u32(third));
    xport_gte_write_data(5, r_u32(third + 4));
    xport_gte_execute(0x280030);
    xport_gte_execute(0x1400006);
    return (sint32)xport_gte_read_data(24);
}

static void td5_reflection_uv(uint32 packet, uint32 command, uint32 texture_u, uint32 texture_v, uint32 offset_x, uint32 offset_y)
{
    sint32 normal_x, normal_y, screen_x, screen_y;
    uint32 u;

    xport_gte_execute(command);
    normal_x = (sint16)xport_gte_read_data(9);
    normal_y = (sint16)xport_gte_read_data(10);
    xport_gte_read_data(11);
    screen_x = (sint32)((uint32)r_s16(packet + 8) - offset_x);
    u = (uint32)(normal_x / 128) + (uint32)(screen_x / 2) + texture_u + 64;
    screen_y = (sint32)((uint32)r_s16(packet + 10) - offset_y);
    w_u8(packet + 12, (uint8)u);
    w_u8(packet + 13, (uint8)((uint32)(normal_y / 128) + (uint32)(screen_y / 2) + texture_v + 64));
}

uint32 td5_8006cbd4(uint32 model, const char *texture_name, uint32 ordering_table, uint32 offset_x, uint32 offset_y)
{
    uint32 texture, bank, record_offset, packet_offset, texture_u, texture_v;
    uint32 result, record, packet, vertices, shift, type, stride, packet_stride;
    uint32 fourth, head_address, tag, head, normal, component;
    sint32 count, index, first_clip, second_clip, depth, far_depth;

    FUNCTION_MARKER(0x8006CBD4, "SLUS_006.10");
    texture = td5_find_texture(texture_name);
    bank = r_u32(0x80088104);
    record_offset = r_u32(model + 48);
    texture_u = r_u8(texture + 14);
    packet_offset = r_u32(model + 52 + bank * 4);
    texture_v = r_u8(texture + 15);
    result = r_u32(0x80088140);
    record = model + record_offset;
    packet = model + packet_offset;
    vertices = r_u32(model + 44);
    type = r_u8(record + 3);
    shift = r_u32(0x80088110);
    far_depth = (sint32)result >> 2;
    vertices += model;
    if (type == 0)
        return result;
    do
    {
        type = r_u8(record + 2);
        count = r_s16(record);
        record += 4;
        if (type == 2 || type == 3)
        {
            stride = type == 2 ? 44 : 56;
            packet_stride = type == 2 ? 40 : 52;
            for (index = 0; index < count; ++index)
            {
                first_clip = td5_reflection_project(record, vertices);
                if (type == 2)
                {
                    xport_gte_execute(0x158002D);
                    if (first_clip > 0)
                        goto next_reflection_polygon;
                    depth = (sint32)xport_gte_read_data(7);
                    if (depth >= far_depth)
                        goto next_reflection_polygon;
                    w_u32(packet + 8, xport_gte_read_data(12));
                    w_u32(packet + 20, xport_gte_read_data(13));
                    w_u32(packet + 32, xport_gte_read_data(14));
                }
                else
                {
                    w_u32(packet + 8, xport_gte_read_data(12));
                    w_u32(packet + 20, xport_gte_read_data(13));
                    w_u32(packet + 32, xport_gte_read_data(14));
                    fourth = vertices + ((uint32)r_s16(record + 6) << 3);
                    xport_gte_write_data(0, r_u32(fourth));
                    xport_gte_write_data(1, r_u32(fourth + 4));
                    xport_gte_execute(0x180001);
                    xport_gte_execute(0x1400006);
                    second_clip = (sint32)xport_gte_read_data(24);
                    xport_gte_execute(0x168002E);
                    if (first_clip > 0 && second_clip < 0)
                        goto next_reflection_polygon;
                    depth = (sint32)xport_gte_read_data(7);
                    if (depth >= far_depth)
                        goto next_reflection_polygon;
                    w_u32(packet + 44, xport_gte_read_data(14));
                }
                tag = r_u32(packet);
                head_address = ordering_table + ((uint32)(depth >> (shift & 31)) << 2);
                head = r_u32(head_address);
                w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
                head = r_u32(head_address);
                w_u32(head_address, (head & 0xFF000000) | (packet & 0xFFFFFF));
                xport_gte_write_data(6, r_u32(record + 8));
                normal = record + (type == 2 ? 20 : 24);
                for (component = 0; component < 6; ++component)
                    xport_gte_write_data(component, r_u32(normal + component * 4));
                xport_gte_execute(0x118043F);
                w_u32(packet + 4, xport_gte_read_data(20));
                w_u32(packet + 16, xport_gte_read_data(21));
                w_u32(packet + 28, xport_gte_read_data(22));
                td5_reflection_uv(packet, 0x486012, texture_u, texture_v, offset_x, offset_y);
                td5_reflection_uv(packet + 12, 0x48E012, texture_u, texture_v, offset_x, offset_y);
                td5_reflection_uv(packet + 24, 0x496012, texture_u, texture_v, offset_x, offset_y);
                if (type == 3)
                {
                    normal = record + 48;
                    xport_gte_write_data(0, r_u32(normal));
                    xport_gte_write_data(1, r_u32(normal + 4));
                    xport_gte_execute(0x108041B);
                    w_u32(packet + 40, xport_gte_read_data(22));
                    td5_reflection_uv(packet + 36, 0x486012, texture_u, texture_v, offset_x, offset_y);
                }
            next_reflection_polygon:
                packet += packet_stride;
                record += stride;
            }
        }
        result = r_u8(record + 3);
    } while (result != 0);
    return result;
}

static void td5_reflection_uv_quarter(uint32 packet, uint32 command, uint32 texture_u, uint32 texture_v, uint32 offset_x, uint32 offset_y, sint32 normal_y_divisor, uint32 late_y)
{
    sint32 normal_x, normal_y, screen_x, screen_y;
    uint32 u;

    xport_gte_execute(command);
    normal_x = (sint16)xport_gte_read_data(9);
    normal_y = (sint16)xport_gte_read_data(10);
    xport_gte_read_data(11);
    screen_x = (sint32)((uint32)r_s16(packet + 8) - offset_x);
    u = (uint32)(normal_x / 128) + (uint32)(screen_x / 4) + texture_u + 64;
    if (late_y != 0)
    {
        w_u8(packet + 12, (uint8)u);
        screen_y = (sint32)((uint32)r_s16(packet + 10) - offset_y);
    }
    else
    {
        screen_y = (sint32)((uint32)r_s16(packet + 10) - offset_y);
        w_u8(packet + 12, (uint8)u);
    }
    w_u8(packet + 13, (uint8)((uint32)(normal_y / normal_y_divisor) + (uint32)(screen_y / 2) + texture_v + 64));
}

uint32 td5_8006d398(uint32 model, const char *texture_name, uint32 ordering_table, uint32 offset_x, uint32 offset_y)
{
    uint32 texture, bank, record_offset, packet_offset, texture_u, texture_v;
    uint32 result, record, packet, vertices, shift, type, stride, packet_stride;
    uint32 fourth, head_address, tag, head, normal, component;
    sint32 count, index, first_clip, second_clip, depth, far_depth;

    FUNCTION_MARKER(0x8006D398, "SLUS_006.10");
    texture = td5_find_texture(texture_name);
    bank = r_u32(0x80088104);
    record_offset = r_u32(model + 48);
    texture_u = r_u8(texture + 14);
    packet_offset = r_u32(model + 52 + bank * 4);
    texture_v = r_u8(texture + 15);
    result = r_u32(0x80088140);
    record = model + record_offset;
    packet = model + packet_offset;
    vertices = r_u32(model + 44);
    type = r_u8(record + 3);
    shift = r_u32(0x80088110);
    far_depth = (sint32)result >> 2;
    vertices += model;
    if (type == 0)
        return result;
    do
    {
        type = r_u8(record + 2);
        count = r_s16(record);
        record += 4;
        if (type == 2 || type == 3)
        {
            stride = type == 2 ? 44 : 56;
            packet_stride = type == 2 ? 40 : 52;
            for (index = 0; index < count; ++index)
            {
                first_clip = td5_reflection_project(record, vertices);
                if (type == 2)
                {
                    xport_gte_execute(0x158002D);
                    if (first_clip > 0)
                        goto next_reflection_polygon;
                    depth = (sint32)xport_gte_read_data(7);
                    if (depth >= far_depth)
                        goto next_reflection_polygon;
                    w_u32(packet + 8, xport_gte_read_data(12));
                    w_u32(packet + 20, xport_gte_read_data(13));
                    w_u32(packet + 32, xport_gte_read_data(14));
                }
                else
                {
                    w_u32(packet + 8, xport_gte_read_data(12));
                    w_u32(packet + 20, xport_gte_read_data(13));
                    w_u32(packet + 32, xport_gte_read_data(14));
                    fourth = vertices + ((uint32)r_s16(record + 6) << 3);
                    xport_gte_write_data(0, r_u32(fourth));
                    xport_gte_write_data(1, r_u32(fourth + 4));
                    xport_gte_execute(0x180001);
                    xport_gte_execute(0x1400006);
                    second_clip = (sint32)xport_gte_read_data(24);
                    xport_gte_execute(0x168002E);
                    if (first_clip > 0 && second_clip < 0)
                        goto next_reflection_polygon;
                    depth = (sint32)xport_gte_read_data(7);
                    if (depth >= far_depth)
                        goto next_reflection_polygon;
                    w_u32(packet + 44, xport_gte_read_data(14));
                }
                tag = r_u32(packet);
                head_address = ordering_table + ((uint32)(depth >> (shift & 31)) << 2);
                head = r_u32(head_address);
                w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
                head = r_u32(head_address);
                w_u32(head_address, (head & 0xFF000000) | (packet & 0xFFFFFF));
                xport_gte_write_data(6, r_u32(record + 8));
                normal = record + (type == 2 ? 20 : 24);
                for (component = 0; component < 6; ++component)
                    xport_gte_write_data(component, r_u32(normal + component * 4));
                xport_gte_execute(0x118043F);
                w_u32(packet + 4, xport_gte_read_data(20));
                w_u32(packet + 16, xport_gte_read_data(21));
                w_u32(packet + 28, xport_gte_read_data(22));
                td5_reflection_uv_quarter(packet, 0x486012, texture_u, texture_v, offset_x, offset_y, type == 3 ? 256 : 128, 0);
                td5_reflection_uv_quarter(packet + 12, 0x48E012, texture_u, texture_v, offset_x, offset_y, type == 3 ? 256 : 128, 0);
                td5_reflection_uv_quarter(packet + 24, 0x496012, texture_u, texture_v, offset_x, offset_y, type == 3 ? 256 : 128, 0);
                if (type == 3)
                {
                    normal = record + 48;
                    xport_gte_write_data(0, r_u32(normal));
                    xport_gte_write_data(1, r_u32(normal + 4));
                    xport_gte_execute(0x108041B);
                    w_u32(packet + 40, xport_gte_read_data(22));
                    td5_reflection_uv_quarter(packet + 36, 0x486012, texture_u, texture_v, offset_x, offset_y, 128, 1);
                }
            next_reflection_polygon:
                packet += packet_stride;
                record += stride;
            }
        }
        result = r_u8(record + 3);
    } while (result != 0);
    return result;
}

uint32 td5_8006e1c8(uint32 matrix, uint32 position)
{
    uint32 scale_z, scale_y, first, second, base, row, sums[3], result;

    FUNCTION_MARKER(0x8006E1C8, "SLUS_006.10");
    first = (uint32)r_s16(matrix + 4);
    scale_z = r_u32(0x80088120);
    first = (uint32)((sint32)(first * scale_z) / 16);
    second = (uint32)r_s16(matrix + 2);
    scale_y = r_u32(0x8008811C);
    second = (uint32)((sint32)(second * scale_y) / 16);
    base = r_u32(position);
    sums[0] = base + first + second;
    for (row = 1; row < 3; ++row)
    {
        first = (uint32)r_s16(matrix + row * 6 + 4);
        first = (uint32)((sint32)(first * scale_z) / 16);
        second = (uint32)r_s16(matrix + row * 6 + 2);
        second = (uint32)((sint32)(second * scale_y) / 16);
        base = r_u32(position + row * 4);
        sums[row] = base + first + second;
    }
    w_u32(0x8008C4F8, (uint32)((sint32)sums[0] >> 8));
    result = (uint32)((sint32)sums[2] >> 8);
    w_u32(0x8008C4FC, (uint32)((sint32)sums[1] >> 8));
    w_u32(0x8008C500, result);
    return result;
}

uint32 td5_8006e6dc(uint32 phase_r, uint32 incoming_v0, uint32 incoming_v1)
{
    uint32 phase_g, phase_b, index, fade, red, green, blue, destination;

    FUNCTION_MARKER(0x8006E6DC, "SLUS_006.10");
    phase_r += 0x2200;
    phase_g = incoming_v0 + 0x1200;
    phase_b = incoming_v1 - 0x1600;
    w_u32(0x80088338, phase_r);
    w_u32(0x8008833C, phase_g);
    w_u32(0x80088340, phase_b);
    for (index = 0; index < 1024; ++index)
    {
        fade = index < 512 ? 256 : (1024 - index) / 2;
        red = (uint32)(rsin((sint32)phase_r / 256) / 64) + 64;
        if (index >= 512)
            red = (uint32)((sint32)(red * fade) / 256);
        green = (uint32)(rsin((sint32)phase_g / 256) / 64) + 64;
        if (index >= 512)
            green = (uint32)((sint32)(green * fade) / 256);
        blue = (uint32)(rsin((sint32)phase_b / 256) / 64) + 64;
        if (index >= 512)
            blue = (uint32)((sint32)(blue * fade) / 256);
        phase_r += 526;
        phase_g += 959;
        phase_b += 679;
        destination = r_u32(0x80088350);
        w_u32(destination + index * 4, red | (green << 8) | (blue << 16));
    }
    return 0;
}

void td5_8006e92c(void)
{
    FUNCTION_MARKER(0x8006E92C, "SLUS_006.10");
}

uint32 td5_80072344(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 x2, sint32 y2)
{
    uint32 first, second, third;
    sint32 width, height;

    FUNCTION_MARKER(0x80072344, "SLUS_006.10");
    first = x0 < 0;
    width = r_s32(0x800880F4);
    if (width < x0)
        first |= 2;
    if (y0 < 0)
        first |= 4;
    height = r_s32(0x800880F8);
    if (height < y0)
        first |= 8;
    second = x1 < 0;
    if (width < x1)
        second |= 2;
    if (y1 < 0)
        second |= 4;
    if (height < y1)
        second |= 8;
    third = x2 < 0;
    if (width < x2)
        third |= 2;
    if (y2 < 0)
        third |= 4;
    if (height < y2)
        third |= 8;
    return (first & second & third) != 0;
}

static uint32 td5_subdivision_link(uint32 packet, uint32 ordering_table, sint32 depth, uint32 shift)
{
    uint32 tag, head_address, head, result;

    tag = r_u32(packet);
    head_address = ordering_table + ((uint32)(depth >> (shift & 31)) << 2);
    head = r_u32(head_address);
    w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
    head = r_u32(head_address);
    result = (head & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(head_address, result);
    return result;
}

static void td5_subdivision_vertex(uint32 vertex, const uint32 position[3], const uint16 uv[2], uint32 shift, uint32 origin, uint32 color_table, uint32 color_bias)
{
    VECTOR square;
    uint32 coordinate, base, sum, threshold, color_address;

    w_u16(vertex, (uint16)((sint32)position[0] >> shift));
    w_u16(vertex + 2, (uint16)((sint32)position[1] >> shift));
    w_u16(vertex + 4, (uint16)((sint32)position[2] >> shift));
    w_u8(vertex + 12, (uint8)((sint16)uv[0] >> shift));
    w_u8(vertex + 13, (uint8)((sint16)uv[1] >> shift));
    coordinate = (uint32)r_s16(vertex);
    base = r_u32(origin);
    square.vx = (sint32)(base + coordinate);
    coordinate = (uint32)r_s16(vertex + 2);
    base = r_u32(origin + 4);
    square.vy = (sint32)(base + coordinate);
    coordinate = (uint32)r_s16(vertex + 4);
    base = r_u32(origin + 8);
    square.vz = (sint32)(base + coordinate);
    Square0(&square, &square);
    threshold = r_u32(0x80088348);
    sum = (uint32)square.vx + (uint32)square.vy + (uint32)square.vz;
    color_address = (sint32)threshold < (sint32)sum ? color_table + 4092 : color_table + ((uint32)((sint32)sum >> 15) << 2);
    w_u32(vertex + 8, r_u32(color_address) + color_bias);
}

static void td5_subdivision_project(uint32 first, uint32 second, uint32 third)
{
    xport_gte_write_data(0, r_u32(first));
    xport_gte_write_data(1, r_u32(first + 4));
    xport_gte_write_data(2, r_u32(second));
    xport_gte_write_data(3, r_u32(second + 4));
    xport_gte_write_data(4, r_u32(third));
    xport_gte_write_data(5, r_u32(third + 4));
    xport_gte_execute(0x280030);
}

static void td5_subdivision_attributes(uint32 packet, uint32 first, uint32 second, uint32 third, uint32 fourth, uint16 clut, uint16 page, uint32 quad)
{
    w_u16(packet + 26, clut);
    w_u16(packet + 14, page);
    w_u32(packet + 4, r_u32(first + 8));
    w_u32(packet + 16, r_u32(second + 8));
    w_u32(packet + 28, r_u32(third + 8));
    if (quad != 0)
        w_u32(packet + 40, r_u32(fourth + 8));
    w_u8(packet + 12, r_u8(first + 12));
    w_u8(packet + 13, r_u8(first + 13));
    w_u8(packet + 24, r_u8(second + 12));
    w_u8(packet + 25, r_u8(second + 13));
    w_u8(packet + 36, r_u8(third + 12));
    w_u8(packet + 37, r_u8(third + 13));
    if (quad != 0)
    {
        w_u8(packet + 48, r_u8(fourth + 12));
        w_u8(packet + 49, r_u8(fourth + 13));
    }
}

uint32 td5_8006e934(uint32 packet, uint32 ordering_table, uint32 vertices, sint32 depth, uint32 origin, uint32 color_table, uint32 color_bias)
{
    uint32 shift, level, precision, required, offset, triangle_offset, bank;
    uint32 quad_packet, triangle_packet, original_tag, cursor, row, column, axis, count;
    uint32 base[3], horizontal[3], vertical[3], current[3], first, second, third;
    uint16 base_uv[2], horizontal_uv[2], vertical_uv[2], current_uv[2], clut, page;
    sint32 cutoff, x0, y0, x1, y1, x2, y2, projected_depth;

    FUNCTION_MARKER(0x8006E934, "SLUS_006.10");
    cutoff = r_s32(0x80088134);
    shift = r_u32(0x80088110);
    shift -= r_u32(0x80088164);
    if (depth >= cutoff / 4)
        return td5_subdivision_link(packet, ordering_table, depth, shift);
    cutoff = r_s32(0x80088138);
    level = depth < cutoff / 4 ? 2 : 1;
    offset = r_u32(0x80088514);
    precision = level * 2;
    required = 52 << precision;
    if (r_u32(0x800884FC) < offset + required - (12 << level))
        return td5_subdivision_link(packet, ordering_table, depth, shift);
    x2 = r_s16(packet + 32);
    x0 = r_s16(packet + 8);
    y0 = r_s16(packet + 10);
    x1 = r_s16(packet + 20);
    y1 = r_s16(packet + 22);
    y2 = r_s16(packet + 34);
    if (td5_80072344(x0, y0, x1, y1, x2, y2) != 0)
        return 1;
    offset = r_u32(0x80088514);
    bank = r_u32(0x80088104);
    triangle_offset = offset + required - (52 << level);
    w_u32(0x80088514, triangle_offset);
    w_u32(0x80088514, triangle_offset + (40 << level));
    original_tag = r_u32(packet);
    quad_packet = r_u32(0x80088370 + bank * 4) + offset;
    triangle_packet = r_u32(0x80088370 + bank * 4) + triangle_offset;
    clut = r_u16(packet + 26);
    base[0] = (uint32)r_s16(vertices) << precision;
    page = r_u16(packet + 14);
    base[1] = (uint32)r_s16(vertices + 2) << precision;
    base[2] = (uint32)r_s16(vertices + 4) << precision;
    base_uv[0] = (uint16)(r_u8(packet + 12) << precision);
    base_uv[1] = (uint16)(r_u8(packet + 13) << precision);
    for (axis = 0; axis < 3; ++axis)
    {
        first = (uint32)r_s16(vertices + 16 + axis * 2);
        second = (uint32)r_s16(vertices + axis * 2);
        vertical[axis] = (first - second) << level;
    }
    first = r_u8(packet + 36);
    second = r_u8(packet + 12);
    vertical_uv[0] = (uint16)((first - second) << level);
    first = r_u8(packet + 37);
    second = r_u8(packet + 13);
    vertical_uv[1] = (uint16)((first - second) << level);
    for (axis = 0; axis < 3; ++axis)
    {
        first = (uint32)r_s16(vertices + 8 + axis * 2);
        second = (uint32)r_s16(vertices + axis * 2);
        horizontal[axis] = (first - second) << level;
    }
    first = r_u8(packet + 24);
    second = r_u8(packet + 12);
    horizontal_uv[0] = (uint16)((first - second) << level);
    first = r_u8(packet + 25);
    second = r_u8(packet + 13);
    horizontal_uv[1] = (uint16)((first - second) << level);
    cursor = 0x800A4E58;
    count = 1 << level;
    for (axis = 0; axis < 3; ++axis)
        current[axis] = base[axis];
    current_uv[0] = base_uv[0];
    current_uv[1] = base_uv[1];
    for (column = 0; column <= count; ++column)
    {
        td5_subdivision_vertex(cursor, current, current_uv, precision, origin, color_table, color_bias);
        cursor += 16;
        for (axis = 0; axis < 3; ++axis)
            current[axis] += horizontal[axis];
        current_uv[0] = (uint16)(current_uv[0] + horizontal_uv[0]);
        current_uv[1] = (uint16)(current_uv[1] + horizontal_uv[1]);
    }
    for (row = 1; row <= count; ++row)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            base[axis] += vertical[axis];
            current[axis] = base[axis];
        }
        base_uv[0] = (uint16)(base_uv[0] + vertical_uv[0]);
        base_uv[1] = (uint16)(base_uv[1] + vertical_uv[1]);
        current_uv[0] = base_uv[0];
        current_uv[1] = base_uv[1];
        td5_subdivision_vertex(cursor, current, current_uv, precision, origin, color_table, color_bias);
        cursor += 16;
        for (column = 1; column <= count - row; ++column)
        {
            for (axis = 0; axis < 3; ++axis)
                current[axis] += horizontal[axis];
            current_uv[0] = (uint16)(current_uv[0] + horizontal_uv[0]);
            current_uv[1] = (uint16)(current_uv[1] + horizontal_uv[1]);
            td5_subdivision_vertex(cursor, current, current_uv, precision, origin, color_table, color_bias);
            second = cursor - (count - row + 2) * 16;
            first = second - 16;
            third = cursor - 16;
            td5_subdivision_attributes(quad_packet, first, second, third, cursor, clut, page, 1);
            td5_subdivision_project(first, second, third);
            w_u32(quad_packet + 8, xport_gte_read_data(12));
            w_u32(quad_packet + 20, xport_gte_read_data(13));
            w_u32(quad_packet + 32, xport_gte_read_data(14));
            xport_gte_write_data(0, r_u32(cursor));
            xport_gte_write_data(1, r_u32(cursor + 4));
            xport_gte_execute(0x180001);
            xport_gte_execute(0x168002E);
            projected_depth = (sint32)xport_gte_read_data(7);
            if (projected_depth > 0)
            {
                w_u32(quad_packet + 44, xport_gte_read_data(14));
                w_u8(quad_packet + 3, 12);
                w_u8(quad_packet + 7, 60);
                td5_subdivision_link(quad_packet, ordering_table, projected_depth, shift);
            }
            quad_packet += 52;
            cursor += 16;
        }
        second = cursor - (count - row + 2) * 16;
        first = second - 16;
        third = cursor - 16;
        w_u16(triangle_packet + 26, clut);
        w_u16(triangle_packet + 14, page);
        w_u32(triangle_packet, original_tag);
        w_u32(triangle_packet + 4, r_u32(first + 8));
        w_u32(triangle_packet + 16, r_u32(second + 8));
        w_u32(triangle_packet + 28, r_u32(third + 8));
        w_u8(triangle_packet + 12, r_u8(first + 12));
        w_u8(triangle_packet + 13, r_u8(first + 13));
        w_u8(triangle_packet + 24, r_u8(second + 12));
        w_u8(triangle_packet + 25, r_u8(second + 13));
        w_u8(triangle_packet + 36, r_u8(third + 12));
        w_u8(triangle_packet + 37, r_u8(third + 13));
        td5_subdivision_project(first, second, third);
        xport_gte_execute(0x158002D);
        projected_depth = (sint32)xport_gte_read_data(7);
        if (projected_depth > 0)
        {
            w_u32(triangle_packet + 8, xport_gte_read_data(12));
            w_u32(triangle_packet + 20, xport_gte_read_data(13));
            w_u32(triangle_packet + 32, xport_gte_read_data(14));
            w_u8(triangle_packet + 3, 9);
            w_u8(triangle_packet + 7, 52);
            td5_subdivision_link(triangle_packet, ordering_table, projected_depth, shift);
        }
        triangle_packet += 40;
    }
    return 1;
}

uint32 td5_80072154(sint32 x0, sint32 y0, sint32 x1, sint32 y1, sint32 x2, sint32 y2, sint32 x3, sint32 y3)
{
    uint32 first, second, third, fourth;
    sint32 width, height;

    FUNCTION_MARKER(0x80072154, "SLUS_006.10");
    first = x0 < 0;
    width = r_s32(0x800880F4);
    if (width < x0)
        first |= 2;
    if (y0 < 0)
        first |= 4;
    height = r_s32(0x800880F8);
    if (height < y0)
        first |= 8;
    second = x1 < 0;
    if (width < x1)
        second |= 2;
    if (y1 < 0)
        second |= 4;
    if (height < y1)
        second |= 8;
    third = x2 < 0;
    if (width < x2)
        third |= 2;
    if (y2 < 0)
        third |= 4;
    if (height < y2)
        third |= 8;
    fourth = x3 < 0;
    width = r_s32(0x800880F4);
    if (width < x3)
        fourth |= 2;
    if (y3 < 0)
        fourth |= 4;
    height = r_s32(0x800880F8);
    if (height < y3)
        fourth |= 8;
    return (first & second & third & fourth) != 0;
}

static void td5_quad_subdivision_vertex(uint32 vertex, const uint32 position[3], const uint16 uv[2], uint32 shift, uint32 original_stack)
{
    VECTOR square;
    uint32 origin, coordinate, base, threshold, sum, table, color, bias;

    w_u16(vertex, (uint16)((sint32)position[0] >> shift));
    w_u16(vertex + 2, (uint16)((sint32)position[1] >> shift));
    w_u16(vertex + 4, (uint16)((sint32)position[2] >> shift));
    w_u8(vertex + 12, (uint8)((sint16)uv[0] >> shift));
    w_u8(vertex + 13, (uint8)((sint16)uv[1] >> shift));
    origin = r_u32(original_stack + 16);
    coordinate = (uint32)r_s16(vertex);
    base = r_u32(origin);
    square.vx = (sint32)(base + coordinate);
    coordinate = (uint32)r_s16(vertex + 2);
    base = r_u32(origin + 4);
    square.vy = (sint32)(base + coordinate);
    coordinate = (uint32)r_s16(vertex + 4);
    base = r_u32(origin + 8);
    square.vz = (sint32)(base + coordinate);
    Square0(&square, &square);
    threshold = r_u32(0x80088348);
    sum = (uint32)square.vx + (uint32)square.vy + (uint32)square.vz;
    table = r_u32(original_stack + 20);
    if ((sint32)threshold < (sint32)sum)
        color = r_u32(table + 4092);
    else
        color = r_u32(table + ((uint32)((sint32)sum >> 15) << 2));
    bias = r_u32(original_stack + 24);
    w_u32(vertex + 8, color + bias);
}

uint32 td5_8006f450(uint32 packet, uint32 ordering_table, uint32 vertices, sint32 depth, uint32 original_stack)
{
    uint32 shift, level, precision, required, offset, bank, output, original_tag;
    uint32 cursor, count, axis, component, row, column, first, second, third;
    uint32 left[3], right[3], left_step[3], right_step[3], horizontal[3], current[3];
    uint16 left_uv[2], right_uv[2], left_uv_step[2], right_uv_step[2], horizontal_uv[2], current_uv[2], clut, page;
    uint32 depths = original_stack - 0x188 + 0xE0;
    uint32 ordering_head;
    sint32 cutoff, x0, y0, x1, y1, x2, y2, x3, y3, projected_depth;

    FUNCTION_MARKER(0x8006F450, "SLUS_006.10");
    cutoff = r_s32(0x80088134);
    shift = r_u32(0x80088110);
    shift -= r_u32(0x80088164);
    if (depth >= cutoff / 4)
        return td5_subdivision_link(packet, ordering_table, depth, shift);
    cutoff = r_s32(0x80088138);
    level = depth < cutoff / 4 ? 2 : 1;
    offset = r_u32(0x80088514);
    bank = r_u32(0x800884FC);
    precision = level * 2;
    required = 52 << precision;
    if (bank < offset + required)
        return td5_subdivision_link(packet, ordering_table, depth, shift);
    x2 = r_s16(packet + 32);
    x0 = r_s16(packet + 8);
    y0 = r_s16(packet + 10);
    x1 = r_s16(packet + 20);
    y1 = r_s16(packet + 22);
    y2 = r_s16(packet + 34);
    x3 = r_s16(packet + 44);
    y3 = r_s16(packet + 46);
    if (td5_80072154(x0, y0, x1, y1, x2, y2, x3, y3) != 0)
        return depths;
    w_u32(depths, xport_gte_read_data(16));
    w_u32(depths + 4, xport_gte_read_data(17));
    w_u32(depths + 8, xport_gte_read_data(18));
    w_u32(depths + 12, xport_gte_read_data(19));
    offset = r_u32(0x80088514);
    bank = r_u32(0x80088104);
    w_u32(0x80088514, offset + required);
    original_tag = r_u32(packet);
    output = r_u32(0x80088370 + bank * 4) + offset;
    clut = r_u16(packet + 26);
    left[0] = (uint32)r_s16(vertices) << precision;
    page = r_u16(packet + 14);
    left[1] = (uint32)r_s16(vertices + 2) << precision;
    left[2] = (uint32)r_s16(vertices + 4) << precision;
    left_uv[0] = (uint16)(r_u8(packet + 12) << precision);
    left_uv[1] = (uint16)(r_u8(packet + 13) << precision);
    for (axis = 0; axis < 3; ++axis)
        right[axis] = (uint32)r_s16(vertices + 8 + axis * 2) << precision;
    right_uv[0] = (uint16)(r_u8(packet + 24) << precision);
    right_uv[1] = (uint16)(r_u8(packet + 25) << precision);
    for (axis = 0; axis < 3; ++axis)
    {
        first = (uint32)r_s16(vertices + 16 + axis * 2);
        second = (uint32)r_s16(vertices + axis * 2);
        left_step[axis] = (first - second) << level;
    }
    first = r_u8(packet + 36);
    second = r_u8(packet + 12);
    left_uv_step[0] = (uint16)((first - second) << level);
    first = r_u8(packet + 37);
    second = r_u8(packet + 13);
    left_uv_step[1] = (uint16)((first - second) << level);
    for (axis = 0; axis < 3; ++axis)
    {
        first = (uint32)r_s16(vertices + 24 + axis * 2);
        second = (uint32)r_s16(vertices + 8 + axis * 2);
        right_step[axis] = (first - second) << level;
    }
    first = r_u8(packet + 48);
    second = r_u8(packet + 24);
    right_uv_step[0] = (uint16)((first - second) << level);
    first = r_u8(packet + 49);
    second = r_u8(packet + 25);
    right_uv_step[1] = (uint16)((first - second) << level);
    cursor = 0x800A4E58;
    count = 1 << level;
    for (row = 0; row <= count; ++row)
    {
        if (row != 0)
        {
            for (axis = 0; axis < 3; ++axis)
                left[axis] += left_step[axis];
            left_uv[0] = (uint16)(left_uv[0] + left_uv_step[0]);
            left_uv[1] = (uint16)(left_uv[1] + left_uv_step[1]);
            for (axis = 0; axis < 3; ++axis)
                right[axis] += right_step[axis];
            right_uv[0] = (uint16)(right_uv[0] + right_uv_step[0]);
            right_uv[1] = (uint16)(right_uv[1] + right_uv_step[1]);
        }
        for (axis = 0; axis < 3; ++axis)
        {
            current[axis] = left[axis];
            horizontal[axis] = (uint32)((sint32)(right[axis] - left[axis]) >> level);
        }
        for (component = 0; component < 2; ++component)
        {
            current_uv[component] = left_uv[component];
            horizontal_uv[component] = (uint16)(((sint32)(sint16)right_uv[component] - (sint32)(sint16)left_uv[component]) >> level);
        }
        td5_quad_subdivision_vertex(cursor, current, current_uv, precision, original_stack);
        cursor += 16;
        for (column = 1; column <= count; ++column)
        {
            for (axis = 0; axis < 3; ++axis)
                current[axis] += horizontal[axis];
            current_uv[0] = (uint16)(current_uv[0] + horizontal_uv[0]);
            current_uv[1] = (uint16)(current_uv[1] + horizontal_uv[1]);
            td5_quad_subdivision_vertex(cursor, current, current_uv, precision, original_stack);
            if (row != 0)
            {
                second = cursor - (count + 1) * 16;
                first = second - 16;
                third = cursor - 16;
                w_u16(output + 26, clut);
                w_u16(output + 14, page);
                w_u32(output, original_tag);
                w_u32(output + 4, r_u32(first + 8));
                w_u32(output + 16, r_u32(second + 8));
                w_u32(output + 28, r_u32(third + 8));
                w_u32(output + 40, r_u32(cursor + 8));
                w_u8(output + 12, r_u8(first + 12));
                w_u8(output + 13, r_u8(first + 13));
                w_u8(output + 24, r_u8(second + 12));
                w_u8(output + 25, r_u8(second + 13));
                w_u8(output + 36, r_u8(third + 12));
                w_u8(output + 37, r_u8(third + 13));
                w_u8(output + 48, r_u8(cursor + 12));
                w_u8(output + 49, r_u8(cursor + 13));
                td5_subdivision_project(first, second, third);
                w_u32(output + 8, xport_gte_read_data(12));
                w_u32(output + 20, xport_gte_read_data(13));
                w_u32(output + 32, xport_gte_read_data(14));
                xport_gte_write_data(0, r_u32(cursor));
                xport_gte_write_data(1, r_u32(cursor + 4));
                xport_gte_execute(0x180001);
                w_u32(output + 44, xport_gte_read_data(14));
                xport_gte_execute(0x168002E);
                projected_depth = (sint32)xport_gte_read_data(7);
                if (projected_depth > 0)
                {
                    w_u8(output + 3, 12);
                    w_u8(output + 7, 60);
                    td5_subdivision_link(output, ordering_table, projected_depth, shift);
                }
                output += 52;
            }
            cursor += 16;
        }
    }
    ordering_head = ordering_table + ((uint32)((sint32)((uint32)depth + 256) >> (shift & 31)) << 2);
    if (level == 1)
        return td5_8007526c(packet, ordering_head, depths);
    return td5_80075728(packet, ordering_head, depths);
}

uint32 td5_80070020(uint32 model)
{
    VECTOR square;
    uint32 colors, color_output, coordinate, reference, delta[3], bank, shift;
    uint32 template_packet, record, vertices, buffer, ordering_base, count, index;
    uint32 sum, threshold, value, type, packet_bytes, template_bytes, offset, limit;
    uint32 output, used, indices[4], first, second, third, fourth, tint, color_base;
    sint32 model_depth, far_depth, polygon_count, polygon, first_clip, second_clip, depth;

    FUNCTION_MARKER(0x80070020, "SLUS_006.10");
    colors = r_u8(model) == 0 ? r_u32(0x80088350) : r_u32(0x80088354);
    coordinate = r_u32(model + 16);
    reference = r_u32(0x8008C4F8);
    color_output = r_u32(0x8008834C);
    delta[0] = (uint32)((sint32)coordinate >> 8) - reference;
    coordinate = r_u32(model + 20);
    reference = r_u32(0x8008C4FC);
    value = r_u32(0x8008C500);
    delta[1] = (uint32)((sint32)coordinate >> 8) - reference;
    coordinate = r_u32(model + 24);
    bank = r_u32(0x80088104);
    shift = r_u32(0x80088110);
    delta[2] = (uint32)((sint32)coordinate >> 8) - value;
    template_packet = model + r_u32(model + 52 + bank * 4);
    record = model + r_u32(model + 48);
    vertices = model + r_u32(model + 44);
    far_depth = r_s32(0x80088140) >> 3;
    shift -= r_u32(0x80088164);
    buffer = r_u32(0x800880F0);
    model_depth = r_s16(model + 12);
    ordering_base = r_u32(buffer + 0x70) + ((uint32)(model_depth >> (shift & 31)) << 2);
    count = td5_8006c964(model);
    w_u16(model + 14, (uint16)count);
    if ((sint32)(count << 16) > 0)
    {
        index = 0;
        do
        {
            square.vx = (sint32)(delta[0] + (uint32)r_s16(vertices + index * 8));
            square.vy = (sint32)(delta[1] + (uint32)r_s16(vertices + index * 8 + 2));
            square.vz = (sint32)(delta[2] + (uint32)r_s16(vertices + index * 8 + 4));
            Square0(&square, &square);
            threshold = r_u32(0x80088348);
            sum = (uint32)square.vx + (uint32)square.vy + (uint32)square.vz;
            if ((sint32)threshold < (sint32)sum)
                value = r_u32(colors + 4092);
            else
                value = r_u32(colors + ((uint32)((sint32)sum >> 15) << 2));
            w_u32(color_output, value);
            color_output += 4;
            ++index;
        } while ((sint32)index < r_s16(model + 14));
    }
    while (r_u8(record + 3) != 0)
    {
        type = r_u8(record + 2);
        polygon_count = r_s16(record);
        record += 4;
        if (type > 1)
            continue;
        packet_bytes = type == 0 ? 40 : 52;
        template_bytes = type == 0 ? 32 : 40;
        offset = r_u32(0x80088514);
        limit = r_u32(0x800884FC);
        if (offset + (uint32)polygon_count * packet_bytes >= limit)
            continue;
        bank = r_u32(0x80088104);
        output = r_u32(0x80088370 + bank * 4) + limit;
        used = 0;
        for (polygon = 0; polygon < polygon_count; ++polygon)
        {
            indices[0] = (uint32)r_s16(record);
            indices[1] = (uint32)r_s16(record + 2);
            indices[2] = (uint32)r_s16(record + 4);
            if (type == 1)
                indices[3] = (uint32)r_s16(record + 6);
            first = vertices + (indices[0] << 3);
            second = vertices + (indices[1] << 3);
            third = vertices + (indices[2] << 3);
            td5_subdivision_project(first, second, third);
            xport_gte_execute(0x1400006);
            first_clip = (sint32)xport_gte_read_data(24);
            if (type == 0)
            {
                xport_gte_execute(0x158002D);
                if (first_clip > 0)
                    goto next_colored_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_colored_polygon;
                output -= 40;
                ++used;
                tint = r_u32(record + 8);
                w_u32(output + 8, xport_gte_read_data(12));
                w_u32(output + 20, xport_gte_read_data(13));
                w_u32(output + 32, xport_gte_read_data(14));
            }
            else
            {
                output -= 52;
                ++used;
                w_u32(output + 8, xport_gte_read_data(12));
                w_u32(output + 20, xport_gte_read_data(13));
                w_u32(output + 32, xport_gte_read_data(14));
                fourth = vertices + (indices[3] << 3);
                xport_gte_write_data(0, r_u32(fourth));
                xport_gte_write_data(1, r_u32(fourth + 4));
                xport_gte_execute(0x180001);
                xport_gte_execute(0x1400006);
                second_clip = (sint32)xport_gte_read_data(24);
                xport_gte_execute(0x168002E);
                if (first_clip > 0 && second_clip < 0)
                    goto next_colored_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_colored_polygon;
                tint = r_u32(record + 8);
                w_u32(output + 44, xport_gte_read_data(14));
            }
            color_base = r_u32(0x8008834C);
            w_u32(output + 4, r_u32(color_base + (indices[0] << 2)) + tint);
            w_u32(output + 16, r_u32(color_base + (indices[1] << 2)) + tint);
            if (type == 1)
                w_u32(output + 28, r_u32(color_base + (indices[2] << 2)) + tint);
            value = r_u32(color_base + (indices[type == 0 ? 2 : 3] << 2));
            w_u8(output + 3, type == 0 ? 9 : 12);
            w_u8(output + 7, type == 0 ? 52 : 60);
            w_u32(output + (type == 0 ? 28 : 40), value + tint);
            w_u32(output + 12, r_u32(template_packet + 12));
            w_u32(output + 24, r_u32(template_packet + 20));
            w_u32(output + 36, r_u32(template_packet + 28));
            if (type == 1)
                w_u32(output + 48, r_u32(template_packet + 36));
            td5_subdivision_link(output, ordering_base, depth, shift);
        next_colored_polygon:
            template_packet += template_bytes;
            record += 20;
        }
        value = r_u32(0x800884FC);
        w_u32(0x800884FC, value - used * packet_bytes);
    }
    return 0;
}

uint32 td5_80070704(uint32 model, uint32 original_stack)
{
    VECTOR square;
    uint32 frame = original_stack - 0xF8;
    uint32 snapshot, component, word0, word1, template_source;
    uint32 colors, color_output, coordinate, reference, delta[3], bank, shift;
    uint32 template_packet, record, vertices, buffer, ordering_base, count, index;
    uint32 sum, threshold, value, type, packet_bytes, template_bytes, offset, limit;
    uint32 output, used, indices[4], first, fourth, tint, color_base;
    sint32 model_depth, far_depth, polygon_count, polygon, first_clip, depth;

    FUNCTION_MARKER(0x80070704, "SLUS_006.10");
    colors = r_u8(model) == 0 ? r_u32(0x80088350) : r_u32(0x80088354);
    w_u32(frame + 0x98, colors);
    coordinate = r_u32(model + 16);
    reference = r_u32(0x8008C4F8);
    color_output = r_u32(0x8008834C);
    delta[0] = (uint32)((sint32)coordinate >> 8) - reference;
    w_u32(frame + 0x20, delta[0]);
    coordinate = r_u32(model + 20);
    reference = r_u32(0x8008C4FC);
    value = r_u32(0x8008C500);
    delta[1] = (uint32)((sint32)coordinate >> 8) - reference;
    w_u32(frame + 0x24, delta[1]);
    coordinate = r_u32(model + 24);
    bank = r_u32(0x80088104);
    delta[2] = (uint32)((sint32)coordinate >> 8) - value;
    w_u32(frame + 0x28, delta[2]);
    template_packet = model + r_u32(model + 52 + bank * 4);
    record = model + r_u32(model + 48);
    vertices = model + r_u32(model + 44);
    buffer = r_u32(0x800880F0);
    far_depth = r_s32(0x80088140) >> 3;
    shift = r_u32(0x80088110);
    shift -= r_u32(0x80088164);
    model_depth = r_s16(model + 12);
    ordering_base = r_u32(buffer + 0x70) + ((uint32)(model_depth >> (shift & 31)) << 2);
    w_u32(frame + 0x88, (uint32)far_depth);
    w_u32(frame + 0xC8, template_packet);
    w_u32(frame + 0x90, ordering_base);
    count = td5_8006c964(model);
    w_u16(model + 14, (uint16)count);
    template_packet = r_u32(frame + 0xC8);
    if ((sint32)(count << 16) > 0)
    {
        index = 0;
        do
        {
            for (component = 0; component < 3; ++component)
            {
                coordinate = (uint32)r_s16(vertices + index * 8 + component * 2);
                reference = r_u32(frame + 0x20 + component * 4);
                if (component == 2)
                    w_u32(frame + 0xC8, template_packet);
                w_u32(frame + 0x30 + component * 4, reference + coordinate);
            }
            square.vx = r_s32(frame + 0x30);
            square.vy = r_s32(frame + 0x34);
            square.vz = r_s32(frame + 0x38);
            Square0(&square, &square);
            w_u32(frame + 0x30, (uint32)square.vx);
            w_u32(frame + 0x34, (uint32)square.vy);
            w_u32(frame + 0x38, (uint32)square.vz);
            sum = r_u32(frame + 0x30);
            sum += r_u32(frame + 0x34);
            sum += r_u32(frame + 0x38);
            threshold = r_u32(0x80088348);
            template_packet = r_u32(frame + 0xC8);
            colors = r_u32(frame + 0x98);
            if ((sint32)threshold < (sint32)sum)
                value = r_u32(colors + 4092);
            else
                value = r_u32(colors + ((uint32)((sint32)sum >> 15) << 2));
            w_u32(color_output, value);
            color_output += 4;
            ++index;
        } while ((sint32)index < r_s16(model + 14));
    }
    if (r_u8(record + 3) == 0)
        return 0;
    w_u32(frame + 0xA0, frame + 0x40);
    w_u32(frame + 0xA8, frame + 0x60);
    do
    {
        type = r_u8(record + 2);
        polygon_count = r_s16(record);
        record += 4;
        if (type > 1)
            continue;
        packet_bytes = type == 0 ? 40 : 52;
        template_bytes = type == 0 ? 32 : 40;
        offset = r_u32(0x80088514);
        limit = r_u32(0x800884FC);
        if (offset + (uint32)polygon_count * packet_bytes >= limit)
            continue;
        bank = r_u32(0x80088104);
        output = r_u32(0x80088370 + bank * 4) + limit;
        used = 0;
        template_source = template_packet;
        for (polygon = 0; polygon < polygon_count; ++polygon)
        {
            indices[0] = (uint32)r_s16(record);
            indices[1] = (uint32)r_s16(record + 2);
            indices[2] = (uint32)r_s16(record + 4);
            if (type == 1)
                indices[3] = (uint32)r_s16(record + 6);
            snapshot = frame + (type == 0 ? 0x40 : 0x60);
            for (component = 0; component < (type == 0 ? 3u : 4u); ++component)
            {
                first = vertices + (indices[component] << 3);
                word0 = r_u32(first);
                word1 = r_u32(first + 4);
                w_u32(snapshot + component * 8, word0);
                w_u32(snapshot + component * 8 + 4, word1);
            }
            snapshot = r_u32(frame + (type == 0 ? 0xA0 : 0xA8));
            td5_subdivision_project(snapshot, snapshot + 8, snapshot + 16);
            xport_gte_execute(0x1400006);
            if (type == 0)
            {
                w_u32(frame + 0x58, xport_gte_read_data(24));
                xport_gte_execute(0x158002D);
                first_clip = r_s32(frame + 0x58);
                if (first_clip > 0)
                    goto next_subdivided_polygon;
                w_u32(frame + 0x5C, xport_gte_read_data(7));
                depth = r_s32(frame + 0x5C);
                far_depth = r_s32(frame + 0x88);
                if (depth >= far_depth)
                    goto next_subdivided_polygon;
                output -= 40;
                ++used;
                tint = r_u32(record + 8);
                w_u32(output + 8, xport_gte_read_data(12));
                w_u32(output + 20, xport_gte_read_data(13));
                w_u32(output + 32, xport_gte_read_data(14));
            }
            else
            {
                output -= 52;
                ++used;
                w_u32(frame + 0x58, xport_gte_read_data(24));
                w_u32(output + 8, xport_gte_read_data(12));
                w_u32(output + 20, xport_gte_read_data(13));
                w_u32(output + 32, xport_gte_read_data(14));
                fourth = frame + 0x78;
                xport_gte_write_data(0, r_u32(fourth));
                xport_gte_write_data(1, r_u32(fourth + 4));
                xport_gte_execute(0x180001);
                xport_gte_execute(0x1400006);
                w_u32(frame + 0x80, xport_gte_read_data(24));
                xport_gte_execute(0x168002E);
                first_clip = r_s32(frame + 0x58);
                if (first_clip > 0 && r_s32(frame + 0x80) < 0)
                    goto next_subdivided_polygon;
                w_u32(frame + 0x5C, xport_gte_read_data(7));
                depth = r_s32(frame + 0x5C);
                far_depth = r_s32(frame + 0x88);
                if (depth >= far_depth)
                    goto next_subdivided_polygon;
                tint = r_u32(record + 8);
                w_u32(output + 44, xport_gte_read_data(14));
            }
            color_base = r_u32(0x8008834C);
            w_u32(output + 4, r_u32(color_base + (indices[0] << 2)) + tint);
            w_u32(output + 16, r_u32(color_base + (indices[1] << 2)) + tint);
            if (type == 1)
                w_u32(output + 28, r_u32(color_base + (indices[2] << 2)) + tint);
            value = r_u32(color_base + (indices[type == 0 ? 2 : 3] << 2));
            w_u8(output + 3, type == 0 ? 9 : 12);
            w_u8(output + 7, type == 0 ? 52 : 60);
            w_u32(output + (type == 0 ? 28 : 40), value + tint);
            w_u32(output + 12, r_u32(template_source + 12));
            w_u32(output + 24, r_u32(template_source + 20));
            w_u32(output + 36, r_u32(template_source + 28));
            if (type == 1)
                w_u32(output + 48, r_u32(template_source + 36));
            colors = r_u32(frame + 0x98);
            ordering_base = r_u32(frame + 0x90);
            snapshot = r_u32(frame + (type == 0 ? 0xA0 : 0xA8));
            w_u32(frame + 0x10, frame + 0x20);
            w_u32(frame + 0x18, tint);
            w_u32(frame + 0x14, colors);
            depth = r_s32(frame + 0x5C);
            w_u32(frame + 0xC8, template_packet);
            if (type == 0)
                td5_8006e934(output, ordering_base, snapshot, depth, frame + 0x20, colors, tint);
            else
                td5_8006f450(output, ordering_base, snapshot, depth, frame);
            template_packet = r_u32(frame + 0xC8);
        next_subdivided_polygon:
            template_source += template_bytes;
            template_packet += template_bytes;
            record += 20;
        }
        value = r_u32(0x800884FC);
        w_u32(0x800884FC, value - used * packet_bytes);
    } while (r_u8(record + 3) != 0);
    return 0;
}
