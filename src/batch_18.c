#include "td5_functions.h"
#include <string.h>
#include <stdio.h>

uint32 td5_800a98f0(uint32 argument)
{
    uint32 previous, index;

    FUNCTION_MARKER(0x800A98F0, "SLUS_006.10");
    // TODO: Restore this entry after the xport_save/load UI migration
    if (!td5_memory_card_enabled && argument == 130)
        return 0;
    previous = r_u32(0x80088620);
    index = r_u32(0x800B02D8);
    w_u8(0x800B0400 + index, previous - 1);
    index = r_u32(0x800B02D8);
    w_u32(0x80088620, argument);
    w_u32(0x800B02D8, index + 1);
    return (uint32)td5_8001a4d8();
}

uint32 td5_800b4db8(void)
{
    uint32 index, bank, packet;

    FUNCTION_MARKER(0x800B4DB8, "SLUS_006.10");
    for (index = 0; index < 8; ++index)
    {
        for (bank = 0; bank < 2; ++bank)
        {
            packet = 0x800BCCC0 + index * 72 + bank * 36;
            w_u8(packet + 3, 8);
            w_u8(packet + 7, 0x38);
            if ((index & 1) == 0)
            {
                w_u8(packet + 4, 255);
                w_u8(packet + 5, 0);
                w_u8(packet + 6, 0);
                w_u8(packet + 20, 255);
                w_u8(packet + 21, 0);
                w_u8(packet + 22, 0);
                w_u8(packet + 12, 0);
                w_u8(packet + 13, 255);
                w_u8(packet + 14, 0);
                w_u8(packet + 28, 0);
                w_u8(packet + 29, 255);
                w_u8(packet + 30, 0);
            }
            else
            {
                w_u8(packet + 12, 255);
                w_u8(packet + 13, 0);
                w_u8(packet + 14, 0);
                w_u8(packet + 28, 255);
                w_u8(packet + 29, 0);
                w_u8(packet + 30, 0);
                w_u8(packet + 4, 0);
                w_u8(packet + 5, 255);
                w_u8(packet + 6, 0);
                w_u8(packet + 20, 0);
                w_u8(packet + 21, 255);
                w_u8(packet + 22, 0);
            }
        }
    }
    for (index = 0; index < 68; ++index)
    {
        for (bank = 0; bank < 2; ++bank)
        {
            packet = 0x800BF428 + index * 32 + bank * 16;
            w_u8(packet + 3, 3);
            w_u8(packet + 7, 0x42);
            w_u8(packet + 4, 255);
            w_u8(packet + 5, 255);
            w_u8(packet + 6, 255);
        }
    }
    for (index = 0; index < 20; ++index)
    {
        for (bank = 0; bank < 2; ++bank)
        {
            packet = 0x800BF118 + index * 32 + bank * 16;
            w_u8(packet + 3, 3);
            w_u8(packet + 7, 0x40);
            if (index % 5 == 2)
            {
                w_u8(packet + 4, 255);
                w_u8(packet + 5, 0);
                w_u8(packet + 6, 255);
            }
            else
            {
                w_u8(packet + 4, 0);
                w_u8(packet + 5, 0);
                w_u8(packet + 6, 0);
            }
        }
    }
    return 0;
}

uint32 td5_800b4f78(void)
{
    uint32 selected, wiggle, row, index, bank, context, packet, head_address, head, tag;
    uint32 x, y, text, length;

    FUNCTION_MARKER(0x800B4F78, "SLUS_006.10");
    selected = r_u32(0x800BCCA4);
    wiggle = r_u32(0x800BC880);
    w_u8(0x800BD070, 1);
    w_u8(0x800BD0E8, 1);
    w_u16(0x800BD07A, selected * 13 + 99);
    w_u16(0x800BD0F2, selected * 13 + 99);
    w_u16(0x800BD078, 99 - wiggle);
    w_u16(0x800BD0F0, wiggle + 413);
    y = 96;
    for (row = 0; row < 4; ++row)
    {
        x = 256 + (uint32)(r_s16(0x800BC88C + row * 2) / 32) - 2;
        bank = r_u32(0x80088104);
        context = r_u32(0x800880F0);
        for (index = 0; index < 5; ++index)
        {
            packet = 0x800BF118 + bank * 16 + (row * 5 + index) * 32;
            w_u16(packet + 8, x);
            w_u16(packet + 10, index == 2 ? y - 1 : y);
            w_u16(packet + 12, x);
            w_u16(packet + 14, index == 2 ? y + 4 : y + 3);
            head_address = r_u32(context + 112);
            tag = r_u32(packet);
            head = r_u32(head_address);
            w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
            head_address = r_u32(context + 112);
            head = r_u32(head_address);
            w_u32(head_address, (head & 0xFF000000) | (packet & 0xFFFFFF));
            ++x;
        }
        bank = r_u32(0x80088104);
        context = r_u32(0x800880F0);
        for (index = 0; index < 17; ++index)
        {
            packet = 0x800BF428 + bank * 16 + (row * 17 + index) * 32;
            x = 128 + index * 16;
            w_u16(packet + 8, x);
            w_u16(packet + 10, y - 1);
            w_u16(packet + 12, x);
            w_u16(packet + 14, y + 4);
            head_address = r_u32(context + 112);
            tag = r_u32(packet);
            head = r_u32(head_address);
            w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
            head_address = r_u32(context + 112);
            head = r_u32(head_address);
            w_u32(head_address, (head & 0xFF000000) | (packet & 0xFFFFFF));
        }
        bank = r_u32(0x80088104);
        context = r_u32(0x800880F0);
        for (index = 0; index < 2; ++index)
        {
            packet = 0x800BCCC0 + bank * 36 + (row * 2 + index) * 72;
            x = index == 0 ? 128 : 256;
            w_u16(packet + 8, x);
            w_u16(packet + 10, y);
            w_u16(packet + 16, x + 128);
            w_u16(packet + 18, y);
            w_u16(packet + 24, x);
            w_u16(packet + 26, y + 4);
            w_u16(packet + 32, x + 128);
            w_u16(packet + 34, y + 4);
            head_address = r_u32(context + 112);
            tag = r_u32(packet);
            head = r_u32(head_address);
            w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
            head_address = r_u32(context + 112);
            head = r_u32(head_address);
            w_u32(head_address, (head & 0xFF000000) | (packet & 0xFFFFFF));
        }
        td5_800b32e4(r_u32(0x800BCCA4) == row ? 1 : 0);
        text = r_u32(0x800BC274 + row * 4);
        length = 0;
        while (r_u8(text + length) != 0)
            ++length;
        td5_800b2790(129, 256, (sint32)(y - 9), (const char *)psx_addr(text, length + 1));
        td5_800b32e4(0);
        text = r_u32(0x800BC284 + row * 8);
        length = 0;
        while (r_u8(text + length) != 0)
            ++length;
        td5_800b2790(129, 122, (sint32)(y - 2), (const char *)psx_addr(text, length + 1));
        text = r_u32(0x800BC288 + row * 8);
        length = 0;
        while (r_u8(text + length) != 0)
            ++length;
        td5_800b2790(129, 390, (sint32)(y - 2), (const char *)psx_addr(text, length + 1));
        y += 13;
    }
    return 0;
}

uint32 td5_800b539c(uint32 values)
{
    sint32 first, second, third, sum, half, selected;
    uint32 index, destination, fallback, candidate;

    FUNCTION_MARKER(0x800B539C, "SLUS_006.10");
    first = r_s16(values);
    second = r_s16(values + 2);
    third = r_s16(values + 4);
    sum = first + second + third;
    if (sum == 0)
        return (uint32)(first + second);
    selected = r_s32(0x800BCCA4);
    if (selected < 0)
        return 1;
    if (selected > 2)
        return 2;
    half = sum / 2;
    for (index = 0; index < 2; ++index)
    {
        if (selected == 0)
            destination = values + (index + 1) * 2;
        else if (selected == 1)
            destination = values + index * 4;
        else
            destination = values + index * 2;
        candidate = (uint32)r_s16(destination) - (uint32)half;
        if (candidate + 4096 < 8193)
            w_u16(destination, candidate);
        else
        {
            fallback = values + (uint32)selected * 2;
            w_u16(fallback, r_u16(fallback) - (uint32)half);
        }
    }
    return 0;
}

uint32 td5_800b5524(uint32 original_stack)
{
    uint32 frame = original_stack - 0x40;
    uint32 values = frame + 0x10;
    uint32 index, target, source, player, selected, value;
    sint32 current, desired;

    FUNCTION_MARKER(0x800B5524, "SLUS_006.10");
    for (index = 0; index < 4; ++index)
    {
        target = 0x800BC884 + index * 2;
        source = 0x800BC88C + index * 2;
        w_u16(values + index * 2, r_u16(target));
        current = r_s16(source);
        desired = r_s16(target);
        if (current < desired)
            w_u16(source, (uint32)current + 32);
        current = r_s16(source);
        desired = r_s16(target);
        if (desired < current)
            w_u16(source, (uint32)current - 32);
    }
    player = r_u32(0x800BC178);
    if ((r_u16(0x800884E8 + player * 2) & 0x10) != 0)
    {
        if (r_s32(0x800BCCA4) > 0)
        {
            td5_80053030();
            w_u32(0x800BCCA4, r_u32(0x800BCCA4) - 1);
        }
    }
    player = r_u32(0x800BC178);
    if ((r_u16(0x800884E8 + player * 2) & 0x40) != 0)
    {
        if (r_s32(0x800BCCA4) < 3)
        {
            td5_80053030();
            w_u32(0x800BCCA4, r_u32(0x800BCCA4) + 1);
        }
    }
    player = r_u32(0x800BC178);
    if ((r_u16(0x800884E8 + player * 2) & 0x80) != 0)
    {
        selected = r_u32(0x800BCCA4);
        if (r_s16(values + selected * 2) >= -4095)
        {
            td5_80053064();
            selected = r_u32(0x800BCCA4);
            target = values + selected * 2;
            w_u16(target, r_u16(target) - 512);
        }
    }
    player = r_u32(0x800BC178);
    if ((r_u16(0x800884E8 + player * 2) & 0x20) != 0)
    {
        selected = r_u32(0x800BCCA4);
        if (r_s16(values + selected * 2) < 4096)
        {
            td5_80053064();
            selected = r_u32(0x800BCCA4);
            target = values + selected * 2;
            w_u16(target, r_u16(target) + 512);
        }
    }
    selected = r_u32(0x800BCCA4);
    target = values + selected * 2;
    current = r_s16(target);
    if (current < -4096)
        current = -4096;
    else if (current > 4096)
        current = 4096;
    w_u16(target, (uint32)current);
    td5_800b539c(values);
    for (index = 0; index < 4; ++index)
    {
        value = r_u16(values + index * 2);
        w_u16(0x800BC884 + index * 2, value);
    }
    return 0;
}

void td5_800b633c(uint32 mode, uint32 original_stack)
{
    uint32 frame = original_stack - 0x58;
    uint32 angle, selected, index, count, state, value, height, buttons;
    uint32 text, length, filename_slot, size, allocation;
    sint32 sine, signed_selection;

    FUNCTION_MARKER(0x800B633C, "SLUS_006.10");
    angle = r_u32(0x800BC87C);
    w_u8(0x800BD070, 0);
    w_u8(0x800BD0E8, 0);
    angle += 128;
    w_u32(0x800BC87C, angle);
    sine = rsin((sint32)angle);
    w_u32(0x800BC880, (uint32)(sine / 600));
    td5_800b3c50();
    selected = r_u8(0x800BC89C);
    for (index = 0; index < 44; ++index)
    {
        if (index != selected)
        {
            value = r_u16(0x800BC92A + index * 12);
            w_u16(0x800BC92A + index * 12, value > 0xFFFE ? 0 : value + 1);
        }
    }
    state = r_u32(0x800BC8A0);
    switch (state)
    {
    case 0:
        index = r_u32(0x800BC164);
        w_u8(0x800BCF08, 0);
        w_u8(0x800BCF80, 1);
        w_u8(0x800BCFF8, 1);
        if (index == 0)
            w_u8(0x800BCF80, 0);
        count = r_u32(0x800BF114);
        if (index == count - 1)
            w_u8(0x800BCFF8, 0);
        signed_selection = r_s16(0x800BF3B8 + index * 2);
        w_u32(0x800BC15C, 0);
        w_u32(0x800BC89C, (uint32)signed_selection);
        td5_800b46d8(frame);
        td5_800b44f8(frame);
        td5_800b32e4(3);
        index = r_u32(0x800BC164);
        selected = r_u8(0x800BF3B8 + index * 2);
        height = r_u32(0x8008810C);
        text = r_u32(0x8007EAA0 + selected * 68);
        length = 0;
        while (r_u8(text + length) != 0)
            ++length;
        td5_800b2790(128, 256, (sint32)((height << 3) + 140), (const char *)psx_addr(text, length + 1));
        if ((r_u32(0x800BC89C) & 0x100) != 0)
        {
            td5_800b32e4(3);
            height = r_u32(0x8008810C);
            text = 0x800B0DE0;
            length = 0;
            while (r_u8(text + length) != 0)
                ++length;
            td5_800b2790(128, 256, (sint32)((height << 3) + 148), (const char *)psx_addr(text, length + 1));
        }
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x20) != 0)
        {
            count = r_u32(0x800BF114);
            index = r_u32(0x800BC164);
            if (index < count - 1)
            {
                w_u32(0x800BC16C, 0);
                w_u32(0x800BC164, index + 1);
                td5_80053064();
            }
        }
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x80) != 0)
        {
            index = r_u32(0x800BC164);
            if (index != 0)
            {
                w_u32(0x800BC16C, 0);
                w_u32(0x800BC164, index - 1);
                td5_80053064();
            }
        }
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x1000) != 0)
        {
            td5_800530f0();
            w_u32(0x800BC16C, 0);
            if (mode == 0 || r_u32(0x80087728) != 3 || r_u32(0x80087720) != 2)
            {
                w_u32(0x800BC8A0, 13);
                return;
            }
        }
        index = r_u32(0x800BC164);
        signed_selection = r_s16(0x800BF3B8 + index * 2);
        w_u32(0x800BCB38, 0);
        w_u32(0x800BCB3C, 0);
        selected = (uint32)signed_selection & 255;
        if (r_u8(0x800BC928 + selected * 12) == 0)
        {
            w_u8(0x800BC928 + selected * 12, 1);
            filename_slot = 0x8007E414 + selected * 4;
            text = r_u32(filename_slot);
            length = 0;
            while (r_u8(text + length) != 0)
                ++length;
            size = td5_archive_aligned_size((const char *)psx_addr(text, length + 1), 1, (uint32)signed_selection, 0x8007E414);
            allocation = td5_alloc_memory(size);
            text = r_u32(filename_slot);
            w_u32(0x800BF418, allocation);
            w_u32(0x800BF10C, selected);
            length = 0;
            while (r_u8(text + length) != 0)
                ++length;
            td5_load_archive_file((const char *)psx_addr(text, length + 1), allocation, (uint32)signed_selection);
            value = r_u32(0x800BF10C);
            w_u8(0x800BC928 + value * 12, 2);
            td5_8005a9f0(0);
            td5_debug_log(0x800B0C94);
            td5_800b3c50();
        }
        w_u32(0x800BC89C, (uint32)signed_selection);
        value = r_u32(0x800BC16C) + 1;
        w_u32(0x800BC16C, value);
        /* Populate both retained text buffers before leaving the refresh phase */
        if (value < 3)
            return;
        if (r_u8(0x800BC928 + selected * 12) != 0)
        {
            w_u32(0x800BC8A0, 2);
            w_u8(0x800BCF08, 0);
        }
        return;
    case 1:
        td5_800b46d8(frame);
        value = r_u32(0x800BC15C);
        if ((sint32)value > 0)
            w_u32(0x800BC15C, value - 128);
        value = r_u32(0x800BC158);
        if ((sint32)value < 0x1100)
            w_u32(0x800BC158, value + 128);
        else
        {
            w_u32(0x800BC15C, 0);
            w_u32(0x800BC8A0, 0);
            w_u32(0x800BC168, 4);
        }
        return;
    case 2:
        index = r_u32(0x800BC164);
        w_u8(0x800BCF80, 1);
        w_u8(0x800BCFF8, 1);
        if (index == 0)
            w_u8(0x800BCF80, 0);
        count = r_u32(0x800BF114);
        if (index == count - 1)
            w_u8(0x800BCFF8, 0);
        td5_800b46d8(frame);
        selected = r_u8(0x800BC89C);
        if (r_u8(0x800BC928 + selected * 12) != 3)
            return;
        value = r_u32(0x800BC15C);
        if ((sint32)value > 0)
        {
            w_u32(0x800BC15C, value - 256);
            return;
        }
        value = r_u32(0x800BC178);
        buttons = r_u16(0x800884E8 + value * 2);
        w_u32(0x800BC15C, 0);
        if ((buttons & 0x1000) != 0)
        {
            td5_800530f0();
            if (mode == 0 || r_u32(0x80087728) != 3 || r_u32(0x80087720) != 2)
                w_u32(0x800BC8A0, 13);
        }
        if (r_u32(0x8008772C) == 0 && (r_u32(0x80087720) != 3 || r_u32(0x800BC174) != 2))
        {
            selected = r_u8(0x800BC89C);
            if ((r_u8(0x800BC128 + selected) & 1) != 0)
            {
                value = r_u32(0x800BC178);
                if ((r_u16(0x800884E8 + value * 2) & 0x10) != 0)
                {
                    value = (r_u32(0x800BCB38) + 1) & 3;
                    w_u32(0x800BCB38, value);
                    w_u32(0x800BCB3C, value);
                    td5_800b4cbc();
                    td5_80053030();
                }
                value = r_u32(0x800BC178);
                if ((r_u16(0x800884E8 + value * 2) & 0x40) != 0)
                {
                    value = (r_u32(0x800BCB38) - 1) & 3;
                    w_u32(0x800BCB38, value);
                    w_u32(0x800BCB3C, value);
                    td5_800b4cbc();
                    td5_80053030();
                }
            }
        }
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x4008) != 0)
        {
            if ((r_u32(0x800BC89C) & 0x100) != 0)
                td5_80053124();
            else
            {
                td5_800530f0();
                value = r_u32(0x80087734);
                w_u32(0x800BC8A0, value != 0 ? 12 : 6);
            }
        }
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x20) != 0)
        {
            count = r_u32(0x800BF114);
            index = r_u32(0x800BC164);
            if (index < count - 1)
            {
                td5_80053064();
                index = r_u32(0x800BC164) + 1;
                signed_selection = r_s16(0x800BF3B8 + index * 2);
                w_u32(0x800BC8A0, 0);
                w_u32(0x800BC16C, 0);
                w_u32(0x800BC164, index);
                w_u32(0x800BC89C, (uint32)signed_selection);
                td5_800b44f8(frame);
            }
        }
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x80) != 0 && r_u32(0x800BC164) != 0)
        {
            td5_80053064();
            index = r_u32(0x800BC164) - 1;
            signed_selection = r_s16(0x800BF3B8 + index * 2);
            w_u32(0x800BC8A0, 0);
            w_u32(0x800BC16C, 0);
            w_u32(0x800BC164, index);
            w_u32(0x800BC89C, (uint32)signed_selection);
            td5_800b44f8(frame);
        }
        return;
    case 6:
        td5_800b46d8(frame);
        value = r_u32(0x800BC15C);
        w_u8(0x800BCF08, 0);
        w_u8(0x800BCF80, 0);
        w_u8(0x800BCFF8, 0);
        if ((sint32)value < 160)
        {
            height = r_u32(0x800BC160);
            w_u32(0x800BC15C, value + 32);
            w_u32(0x800BC160, height - 2);
        }
        else
        {
            w_u32(0x800BC15C, 160);
            w_u32(0x800BC8A0, 7);
        }
        return;
    case 7:
        td5_800b46d8(frame);
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x50) != 0)
        {
            td5_80053030();
            w_u32(0x800BC154, r_u32(0x800BC154) ^ 1);
        }
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x1000) != 0)
        {
            w_u32(0x800BC8A0, 8);
            td5_800530f0();
        }
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x4008) == 0)
            return;
        td5_800530f0();
        if (r_u32(0x80087720) == 0)
        {
            w_u32(0x800BC8A0, 9);
            return;
        }
        value = r_u32(0x80087728);
        w_u32(0x800BC8A0, 12);
        if (value == 3)
        {
            selected = r_u8(0x800BC89C);
            for (index = 0; index < 13; ++index)
                if (r_s16(0x800A5098 + index * 2) == (sint32)selected)
                    w_u16(0x800A5098 + index * 2, 0xFFFE);
        }
        return;
    case 8:
        td5_800b46d8(frame);
        value = r_u32(0x800BC15C);
        if ((sint32)value > 0)
        {
            height = r_u32(0x800BC160);
            w_u32(0x800BC15C, value - 32);
            w_u32(0x800BC160, height + 2);
        }
        else
        {
            w_u32(0x800BC160, 0);
            w_u32(0x800BC15C, 0);
            w_u32(0x800BC8A0, 2);
        }
        return;
    case 9:
        td5_800b46d8(frame);
        td5_800b4db8();
        w_u32(0x800BCCA4, 0);
        w_u32(0x800BC8A0, 10);
        return;
    case 10:
        td5_800b46d8(frame);
        td5_800b4f78();
        td5_800b5524(frame);
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x1000) != 0)
        {
            td5_800530f0();
            w_u32(0x800BC8A0, 11);
        }
        value = r_u32(0x800BC178);
        if ((r_u16(0x800884E8 + value * 2) & 0x4008) != 0)
        {
            td5_800530f0();
            w_u32(0x800BC8A0, 12);
        }
        return;
    case 11:
        td5_800b46d8(frame);
        td5_800b4f78();
        w_u32(0x800BC8A0, 6);
        return;
    case 12:
    case 13:
        goto release_carousel;
    default:
        return;
    }

release_carousel:
    td5_8002f1c4(r_u32(0x800881F4));
    index = 0;
    count = r_u32(0x800BCCA0);
    if ((sint32)count > 0)
    {
        do
        {
            td5_free_memory(r_u32(0x800BCB44 + index * 8));
            count = r_u32(0x800BCCA0);
            ++index;
        } while ((sint32)index < (sint32)count);
    }
    allocation = r_u32(0x800BF418);
    if (allocation != 0)
        td5_free_memory(allocation);
    if (state == 12)
    {
        td5_800b42f0();
        if (mode != 0)
        {
            if (r_u32(0x800BC174) == 1)
            {
                w_u32(0x80088520, 10);
                return;
            }
            value = r_u32(0x80087728);
            w_u32(0x800BC174, 0);
            if (value == 3 && r_u32(0x80087720) == 2)
                goto exit_to_game;
            td5_800b217c();
            goto exit_to_menu;
        }
        if (r_u32(0x800BC174) == 1)
        {
            td5_800a98f0(20);
            return;
        }
        w_u32(0x800BC174, 0);
        w_u32(0x8008858C, 4);
        w_u32(0x800AE0C4, 1);
        return;
    }
    if (mode != 0)
    {
        if (r_u32(0x800BC174) == 2)
        {
            w_u32(0x80088520, 10);
            w_u32(0x800BC174, 0);
            return;
        }
        if (r_u32(0x80087728) == 3 && r_u32(0x80087720) == 2)
            goto exit_to_game;
        td5_800b217c();
        goto exit_to_menu;
    }
    td5_800a99f0();
    if (r_u32(0x800BC174) == 2)
        w_u32(0x800BC174, 0);
    if (r_s32(0x800BC174) >= 2)
        return;
    w_u32(0x800BC174, 0);
exit_to_menu:
    td5_80055a38((sint32)r_u8(0x800884F4));
    td5_stop_sound_voice(20);
    return;
exit_to_game:
    w_u16(0x80088794, 1);
    w_u32(0x8008858C, 5);
}

uint32 td5_800babc8(uint32 source, uint32 count)
{
    uint32 cursor, result, bytes = count << 1;

    FUNCTION_MARKER(0x800BABC8, "SLUS_006.10");
    cursor = r_u32(0x800BF414);
    memcpy(psx_addr(cursor, bytes), psx_addr(source, bytes), bytes);
    w_u32(0x800BF414, r_u32(0x800BF414) + bytes);
    result = r_u32(0x800BCF04) + bytes;
    w_u32(0x800BCF04, result);
    return result;
}

uint32 td5_800bac70(uint32 source, uint32 bytes)
{
    uint32 cursor, result;

    FUNCTION_MARKER(0x800BAC70, "SLUS_006.10");
    cursor = r_u32(0x800BF414);
    memcpy(psx_addr(cursor, bytes), psx_addr(source, bytes), bytes);
    w_u32(0x800BF414, r_u32(0x800BF414) + bytes);
    result = r_u32(0x800BCF04) + bytes;
    w_u32(0x800BCF04, result);
    return result;
}

uint32 td5_800bacf0(uint32 source, uint32 count)
{
    uint32 cursor, result, bytes = count << 2;

    FUNCTION_MARKER(0x800BACF0, "SLUS_006.10");
    cursor = r_u32(0x800BF414);
    memcpy(psx_addr(cursor, bytes), psx_addr(source, bytes), bytes);
    w_u32(0x800BF414, r_u32(0x800BF414) + bytes);
    result = r_u32(0x800BCF04) + bytes;
    w_u32(0x800BCF04, result);
    return result;
}

uint32 td5_800bad98(uint32 destination)
{
    FUNCTION_MARKER(0x800BAD98, "SLUS_006.10");
    w_u32(0x800BCF04, 0);
    w_u32(0x800BF414, destination);
    td5_800babc8(0x800A5098, 13);
    td5_800bacf0(0x8008766C, 2);
    td5_800bacf0(0x80087674, 2);
    td5_800bacf0(0x8008767C, 2);
    td5_800bacf0(0x80087684, 2);
    td5_800bacf0(r_u32(0x8008768C), 1);
    td5_800bacf0(r_u32(0x80087690), 1);
    td5_800bacf0(0x80087668, 1);
    td5_800bacf0(0x80087660, 1);
    td5_800bacf0(0x80087694, 1);
    td5_800bacf0(0x80087698, 1);
    td5_800bacf0(0x8008769C, 1);
    td5_800bacf0(0x800876A0, 1);
    td5_800bacf0(0x800876A4, 1);
    td5_800bacf0(0x800876A8, 1);
    td5_800bacf0(0x800876AC, 1);
    td5_800bacf0(0x800876B0, 1);
    td5_800bacf0(0x800876B4, 1);
    td5_800bacf0(0x800876C0, 1);
    td5_800bacf0(0x800876B8, 1);
    td5_800bacf0(0x800876BC, 1);
    td5_800bacf0(0x800876C4, 1);
    td5_800bacf0(0x800876C8, 1);
    td5_800bacf0(0x800883F8, 1);
    td5_800bacf0(0x800876CC, 1);
    td5_800bacf0(0x800876D0, 1);
    td5_800bacf0(0x800876D4, 1);
    td5_800bacf0(0x800876D8, 1);
    td5_800bacf0(0x800876E8, 1);
    td5_800bacf0(0x800876EC, 1);
    td5_800bacf0(0x800876F0, 1);
    td5_800bacf0(0x800876F4, 1);
    td5_800bacf0(0x800876F8, 1);
    td5_800bacf0(0x800876FC, 1);
    td5_800bacf0(0x80087700, 1);
    td5_800bacf0(0x80087704, 1);
    td5_800bacf0(0x80087708, 1);
    td5_800bacf0(0x800885F0, 1);
    td5_800bacf0(0x8008871C, 1);
    td5_800bacf0(0x8008770C, 1);
    td5_800bacf0(0x80087710, 1);
    td5_800bacf0(0x80087718, 1);
    td5_800bacf0(0x8008771C, 1);
    td5_800bacf0(0x80087720, 1);
    td5_800bacf0(0x80087724, 1);
    td5_800bacf0(0x80087728, 1);
    td5_800bacf0(0x8008772C, 1);
    td5_800bacf0(0x80087730, 1);
    td5_800bacf0(0x80087734, 1);
    td5_800bacf0(0x80087738, 1);
    td5_800bacf0(0x800876DC, 1);
    td5_800bacf0(0x800876E0, 1);
    td5_800bacf0(0x800876E4, 1);
    td5_800bacf0(0x8007DC24, 4);
    td5_800bacf0(0x8007DC34, 4);
    td5_800bacf0(0x8007DC44, 4);
    td5_800bacf0(0x8009DA20, 30);
    td5_800bacf0(0x8008CF70, 36);
    td5_800bacf0(0x8007C4D4, 1410);
    return td5_800bac70(0x8009DB38, 101);
}

// TODO: Write the game payload through xport_save and adapt the card-slot UI
uint32 td5_80056d60(sint32 slot)
{
    char filename[32], title[13];
    uint32 words[3], index, value, allocation, character, texture, page, u, v, clut;
    uint32 image_x, image_y, checksum;
    sint32 descriptor, written;
    PSX_RECT rectangle;

    FUNCTION_MARKER(0x80056D60, "SLUS_006.10");
    for (index = 0; index < 3; ++index)
    {
        (void)r_u32(0x80011470 + index * 4);
        words[index] = r_u32(0x80011470 + index * 4);
    }
    for (index = 0; index < 3; ++index)
    {
        title[index * 4] = (char)words[index];
        title[index * 4 + 1] = (char)(words[index] >> 8);
        title[index * 4 + 2] = (char)(words[index] >> 16);
        title[index * 4 + 3] = (char)(words[index] >> 24);
    }
    title[12] = (char)r_s8(0x8001147C);
    sprintf(filename, (const char *)psx_addr(0x8001143C, 19), slot);
    w_u32(0x8008860C, r_u32(0x8008860C) + 1);
    allocation = td5_alloc_memory(8192);
    memset(psx_addr(allocation, 8192), 0, 8192);
    w_u8(allocation, 0x53);
    w_u8(allocation + 1, 0x43);
    w_u8(allocation + 2, 0x11);
    w_u8(allocation + 3, 1);
    for (index = 0; index < strlen(title); ++index)
    {
        character = (uint8)title[index];
        if (character == 32)
        {
            w_u8(allocation + index * 2 + 4, 0x81);
            value = 0x40;
        }
        else
        {
            w_u8(allocation + index * 2 + 4, 0x82);
            value = (uint8)title[index] + 31;
        }
        w_u8(allocation + index * 2 + 5, value);
    }
    texture = td5_find_texture((const char *)psx_addr(0x80087C2C, 8));
    page = r_u16(texture + 8);
    u = r_u8(texture + 14);
    v = r_u8(texture + 15);
    clut = r_u16(texture + 10);
    rectangle.w = 16;
    rectangle.h = 1;
    image_x = ((page << 6) & 0x3FF) + (u >> 2);
    image_y = ((page << 4) & 0x100) + ((page >> 2) & 0x200) + v;
    rectangle.x = (sint16)((clut & 63) << 4);
    rectangle.y = (sint16)((clut >> 6) & 255);
    StoreImage(&rectangle, (uint32 *)psx_addr(allocation + 0x60, 32));
    DrawSync(0);
    rectangle.x = (sint16)image_x;
    rectangle.y = (sint16)image_y;
    rectangle.w = 4;
    rectangle.h = 16;
    StoreImage(&rectangle, (uint32 *)psx_addr(allocation + 0x80, 128));
    DrawSync(0);
    td5_800bad98(allocation + 0x204);
    for (index = 0; index < 7676; ++index)
    {
        value = r_u8(allocation + 0x204 + index);
        checksum = r_u32(allocation + 0x200);
        w_u32(allocation + 0x200, checksum + value);
    }
    descriptor = openPSX(filename, 2);
    if (descriptor < 0)
    {
        td5_free_memory(allocation);
        return 0;
    }
    written = writePSX(descriptor, psx_addr(allocation, 8192), 8192);
    td5_free_memory(allocation);
    closePSX(descriptor);
    return written == 8192;
}

uint32 td5_800bb288(uint32 destination, uint32 count)
{
    uint32 cursor, result, bytes = count << 1;

    FUNCTION_MARKER(0x800BB288, "SLUS_006.10");
    cursor = r_u32(0x800BF414);
    memcpy(psx_addr(destination, bytes), psx_addr(cursor, bytes), bytes);
    w_u32(0x800BF414, r_u32(0x800BF414) + bytes);
    result = r_u32(0x800BCF04) + bytes;
    w_u32(0x800BCF04, result);
    return result;
}

uint32 td5_800bb330(uint32 destination, uint32 bytes)
{
    uint32 cursor, result;

    FUNCTION_MARKER(0x800BB330, "SLUS_006.10");
    cursor = r_u32(0x800BF414);
    memcpy(psx_addr(destination, bytes), psx_addr(cursor, bytes), bytes);
    w_u32(0x800BF414, r_u32(0x800BF414) + bytes);
    result = r_u32(0x800BCF04) + bytes;
    w_u32(0x800BCF04, result);
    return result;
}

uint32 td5_800bb3b0(uint32 destination, uint32 count)
{
    uint32 cursor, result, bytes = count << 2;

    FUNCTION_MARKER(0x800BB3B0, "SLUS_006.10");
    cursor = r_u32(0x800BF414);
    memcpy(psx_addr(destination, bytes), psx_addr(cursor, bytes), bytes);
    w_u32(0x800BF414, r_u32(0x800BF414) + bytes);
    result = r_u32(0x800BCF04) + bytes;
    w_u32(0x800BCF04, result);
    return result;
}

uint32 td5_800bb458(uint32 source)
{
    FUNCTION_MARKER(0x800BB458, "SLUS_006.10");
    w_u32(0x800BCF04, 0);
    w_u32(0x800BF414, source);
    td5_800bb288(0x800A5098, 13);
    td5_800bb3b0(0x8008766C, 2);
    td5_800bb3b0(0x80087674, 2);
    td5_800bb3b0(0x8008767C, 2);
    td5_800bb3b0(0x80087684, 2);
    td5_800bb3b0(r_u32(0x8008768C), 1);
    td5_800bb3b0(r_u32(0x80087690), 1);
    td5_800bb3b0(0x80087668, 1);
    td5_800bb3b0(0x80087660, 1);
    td5_800bb3b0(0x80087694, 1);
    td5_800bb3b0(0x80087698, 1);
    td5_800bb3b0(0x8008769C, 1);
    td5_800bb3b0(0x800876A0, 1);
    td5_800bb3b0(0x800876A4, 1);
    td5_800bb3b0(0x800876A8, 1);
    td5_800bb3b0(0x800876AC, 1);
    td5_800bb3b0(0x800876B0, 1);
    td5_800bb3b0(0x800876B4, 1);
    td5_800bb3b0(0x800876C0, 1);
    td5_800bb3b0(0x800876B8, 1);
    td5_800bb3b0(0x800876BC, 1);
    td5_800bb3b0(0x800876C4, 1);
    td5_800bb3b0(0x800876C8, 1);
    td5_800bb3b0(0x800883F8, 1);
    td5_800bb3b0(0x800876CC, 1);
    td5_800bb3b0(0x800876D0, 1);
    td5_800bb3b0(0x800876D4, 1);
    td5_800bb3b0(0x800876D8, 1);
    td5_800bb3b0(0x800876E8, 1);
    td5_800bb3b0(0x800876EC, 1);
    td5_800bb3b0(0x800876F0, 1);
    td5_800bb3b0(0x800876F4, 1);
    td5_800bb3b0(0x800876F8, 1);
    td5_800bb3b0(0x800876FC, 1);
    td5_800bb3b0(0x80087700, 1);
    td5_800bb3b0(0x80087704, 1);
    td5_800bb3b0(0x80087708, 1);
    td5_800bb3b0(0x800885F0, 1);
    td5_800bb3b0(0x8008871C, 1);
    td5_800bb3b0(0x8008770C, 1);
    td5_800bb3b0(0x80087710, 1);
    td5_800bb3b0(0x80087718, 1);
    td5_800bb3b0(0x8008771C, 1);
    td5_800bb3b0(0x80087720, 1);
    td5_800bb3b0(0x80087724, 1);
    td5_800bb3b0(0x80087728, 1);
    td5_800bb3b0(0x8008772C, 1);
    td5_800bb3b0(0x80087730, 1);
    td5_800bb3b0(0x80087734, 1);
    td5_800bb3b0(0x80087738, 1);
    td5_800bb3b0(0x800876DC, 1);
    td5_800bb3b0(0x800876E0, 1);
    td5_800bb3b0(0x800876E4, 1);
    td5_800bb3b0(0x8007DC24, 4);
    td5_800bb3b0(0x8007DC34, 4);
    td5_800bb3b0(0x8007DC44, 4);
    td5_800bb3b0(0x8009DA20, 30);
    td5_800bb3b0(0x8008CF70, 36);
    td5_800bb3b0(0x8007C4D4, 1410);
    return td5_800bb330(0x8009DB38, 101);
}

/* Native storage uses the original 6303-byte game payload */
#define TD5_SAVE_SIZE 6303

sint32 td5_save_game(void)
{
    uint32 allocation = td5_alloc_memory(TD5_SAVE_SIZE);
    sint32 result;

    td5_800bad98(allocation);
    result = xport_save(psx_addr(allocation, TD5_SAVE_SIZE), TD5_SAVE_SIZE);
    td5_free_memory(allocation);
    if (result != 1)
        xport_message_error("Test Drive 5", "Unable to save game progress");
    return result;
}

sint32 td5_load_game(void)
{
    uint32 allocation = td5_alloc_memory(TD5_SAVE_SIZE);
    sint32 result = xport_load(psx_addr(allocation, TD5_SAVE_SIZE), TD5_SAVE_SIZE);

    if (result == TD5_SAVE_SIZE)
        td5_800bb458(allocation);
    td5_free_memory(allocation);
    if (result != 0 && result != TD5_SAVE_SIZE)
    {
        xport_message_error("Test Drive 5", "Unable to load game progress");
        return -1;
    }
    return result != 0;
}

// TODO: Read the game payload through xport_load and adapt the card-slot UI
uint32 td5_80056ff4(sint32 slot)
{
    char filename[32];
    uint32 allocation, checksum = 0, index, width, height;
    sint32 descriptor;

    FUNCTION_MARKER(0x80056FF4, "SLUS_006.10");
    sprintf(filename, (const char *)psx_addr(0x8001143C, 19), slot);
    allocation = td5_alloc_memory(8192);
    memset(psx_addr(allocation, 8192), 0, 8192);
    descriptor = openPSX(filename, 1);
    if (descriptor < 0)
    {
        td5_free_memory(allocation);
        return 0;
    }
    if (readPSX(descriptor, psx_addr(allocation, 8192), 8192) != 8192)
        goto failed_read;
    for (index = 0; index < 7676; ++index)
        checksum += r_u8(allocation + 0x204 + index);
    if (checksum != r_u32(allocation + 0x200))
        goto failed_read;
    td5_800bb458(allocation + 0x204);
    td5_free_memory(allocation);
    closePSX(descriptor);
    width = r_u32(0x800876F8);
    height = r_u32(0x800876FC);
    w_u16(0x800976CC, width);
    w_u16(0x80097740, width);
    w_u16(0x800976CE, height);
    w_u16(0x80097742, height);
    return 1;

failed_read:
    td5_free_memory(allocation);
    closePSX(descriptor);
    return 0;
}

uint32 td5_800560fc(uint32 slot)
{
    uint32 index, offset = slot * 4396;

    FUNCTION_MARKER(0x800560FC, "SLUS_006.10");
    for (index = 0; index < 15; ++index)
        if (r_s16(0x8009FE38 + offset + index * 292) == 0)
            return 0;
    return 1;
}

uint32 td5_800562dc(uint32 slot)
{
    uint32 index, offset, destination, word, half, byte, merged;
    uint32 slot_offset = slot * 4396;

    FUNCTION_MARKER(0x800562DC, "SLUS_006.10");
    for (index = 0; index < 15; ++index)
    {
        offset = slot_offset + index * 292;
        destination = 0x8009FD1C + offset;
        (void)r_u32(0x80087BF4);
        word = r_u32(0x80087BF4);
        half = (uint32)r_s16(0x80087BF8);
        byte = (uint32)r_s8(0x80087BFA);
        w_u32(destination, word);
        w_u32(destination, word);
        w_u16(destination + 4, half);
        w_u8(destination + 6, byte);
        destination = 0x8009FE2E + offset;
        (void)r_u32(0x80087BF4);
        word = r_u32(0x80087BF4);
        half = (uint32)r_s16(0x80087BF8);
        byte = (uint32)r_s8(0x80087BFA);
        merged = r_u32(destination + 2);
        w_u32(destination + 2, (merged & 0xFFFF0000) | (word >> 16));
        merged = r_u32(destination - 2);
        w_u32(destination - 2, (merged & 0xFFFF) | (word << 16));
        w_u16(destination + 4, half);
        w_u8(destination + 6, byte);
        w_u16(0x8009FE3A + offset, index);
        w_u8(0x8009FE18 + offset, 0);
        w_u16(0x8009FE38 + offset, 0);
    }
    return slot_offset + 4088;
}

uint32 td5_800ba084(void)
{
    uint32 slot, text, destination, length, result = 0;

    FUNCTION_MARKER(0x800BA084, "SLUS_006.10");
    for (slot = 0; slot < 2; ++slot)
    {
        w_u32(0x80088630 + slot * 4, 1);
        if (r_u32(0x800A0E3C + slot * 4396) == 0)
            text = 0x800B12F0;
        else if (r_u32(0x800A0E40 + slot * 4396) == 0)
            text = 0x800B1300;
        else if (td5_800560fc(slot) == 1)
            text = 0x800B131C;
        else
            text = 0x800B1334;
        destination = r_u32(0x800BD254 + slot * 120);
        length = 0;
        while (r_u8(text + length) != 0)
            ++length;
        result = (uint32)sprintf((char *)psx_addr(destination, length + 1), (const char *)psx_addr(text, length + 1));
    }
    return result;
}

uint32 td5_800ba700(uint32 slot)
{
    uint32 flags = 0, index, selected, count;

    FUNCTION_MARKER(0x800BA700, "SLUS_006.10");
    if (r_u32(0x800A0E3C + slot * 4396) == 0)
        flags |= 1;
    if (td5_800560fc(slot) == 1)
    {
        count = 0;
        for (index = 0; index < 15; ++index)
        {
            selected = r_u32(0x800BF424);
            if (r_s16(0x8009FE38 + selected * 4396 + index * 292) == 1)
                ++count;
        }
        if (count == 0)
            flags |= 2;
    }
    return flags;
}

uint32 td5_800ba890(uint32 first, uint32 count, uint32 original_stack)
{
    uint32 frame = original_stack - 0x58;
    uint32 texture, value, page, coordinate, end, index, block;

    FUNCTION_MARKER(0x800BA890, "SLUS_006.10");
    w_u32(frame + 0x58, first);
    w_u32(frame + 0x5C, count);
    texture = td5_find_texture((const char *)psx_addr(0x800B1348, 8));
    w_u32(frame + 0x20, texture);
    texture = r_u32(frame + 0x20);
    page = r_u16(texture + 8);
    coordinate = (page << 6) & 0x3FF;
    texture = r_u32(frame + 0x20);
    value = r_u8(texture + 14);
    w_u32(frame + 0x30, coordinate + ((value >> 2) & 255));
    texture = r_u32(frame + 0x20);
    page = r_u16(texture + 8);
    coordinate = (page << 4) & 0x100;
    texture = r_u32(frame + 0x20);
    page = r_u16(texture + 8);
    coordinate += (page >> 2) & 0x200;
    texture = r_u32(frame + 0x20);
    value = r_u8(texture + 15);
    w_u32(frame + 0x34, coordinate + value);
    texture = r_u32(frame + 0x20);
    value = r_u16(texture + 10);
    w_u32(frame + 0x38, (value & 63) << 4);
    texture = r_u32(frame + 0x20);
    value = r_u16(texture + 10);
    w_u32(frame + 0x3C, (value >> 6) & 255);
    w_u32(frame + 0x18, r_u32(frame + 0x58));
    for (;;)
    {
        value = r_u32(frame + 0x58);
        count = r_u32(frame + 0x5C);
        end = value + count;
        index = r_u32(frame + 0x18);
        if ((sint32)index >= (sint32)end)
            break;
        w_u32(frame + 0x1C, 1);
        for (;;)
        {
            block = r_u32(frame + 0x1C);
            if ((sint32)block >= 16)
                break;
            index = r_u32(frame + 0x18);
            block = r_u32(frame + 0x1C);
            sprintf((char *)psx_addr(frame + 0x28, 22), (const char *)psx_addr(0x800B1350, 7), (sint32)index, block);
            texture = td5_find_texture((const char *)psx_addr(frame + 0x28, 8));
            w_u32(frame + 0x24, texture);
            texture = r_u32(frame + 0x24);
            page = r_u16(texture + 8);
            coordinate = (page << 6) & 0x3FF;
            texture = r_u32(frame + 0x24);
            value = r_u8(texture + 14);
            w_u32(frame + 0x40, coordinate + ((value >> 2) & 255));
            texture = r_u32(frame + 0x24);
            page = r_u16(texture + 8);
            coordinate = (page << 4) & 0x100;
            texture = r_u32(frame + 0x24);
            page = r_u16(texture + 8);
            coordinate += (page >> 2) & 0x200;
            texture = r_u32(frame + 0x24);
            value = r_u8(texture + 15);
            w_u32(frame + 0x44, coordinate + value);
            texture = r_u32(frame + 0x24);
            value = r_u16(texture + 10);
            w_u32(frame + 0x48, (value & 63) << 4);
            texture = r_u32(frame + 0x24);
            value = r_u16(texture + 10);
            w_u32(frame + 0x4C, (value >> 6) & 255);
            w_u16(frame + 0x10, r_u16(frame + 0x38));
            w_u16(frame + 0x12, r_u16(frame + 0x3C));
            w_u16(frame + 0x14, 16);
            w_u16(frame + 0x16, 1);
            value = r_u32(frame + 0x48);
            coordinate = r_u32(frame + 0x4C);
            MoveImage((PSX_RECT *)psx_addr(frame + 0x10, 8), (sint32)value, (sint32)coordinate);
            w_u16(frame + 0x10, r_u16(frame + 0x30));
            w_u16(frame + 0x12, r_u16(frame + 0x34));
            w_u16(frame + 0x14, 4);
            w_u16(frame + 0x16, 16);
            value = r_u32(frame + 0x40);
            coordinate = r_u32(frame + 0x44);
            MoveImage((PSX_RECT *)psx_addr(frame + 0x10, 8), (sint32)value, (sint32)coordinate);
            w_u32(frame + 0x1C, r_u32(frame + 0x1C) + 1);
        }
        w_u32(frame + 0x18, r_u32(frame + 0x18) + 1);
    }
    return (uint32)DrawSync(0);
}

uint32 td5_800ba234(uint32 slot, uint32 original_stack, uint32 invalid_header_index)
{
    uint32 frame = original_stack - 0x20;
    uint32 status, result, offset;

    FUNCTION_MARKER(0x800BA234, "SLUS_006.10");
    w_u32(frame + 0x20, slot);
    if ((r_u32(0x80088450) & 255) == 255)
    {
        td5_poll_pads();
        VSync(0);
        VSync(0);
        td5_poll_pads();
        w_u32(frame + 0x10, td5_80056228(r_u32(frame + 0x20)));
        VSync(0);
        VSync(0);
        w_u32(frame + 0x10, td5_80056228(r_u32(frame + 0x20)));
        VSync(0);
        VSync(0);
        VSync(0);
        td5_poll_pads();
        w_u32(frame + 0x10, td5_80056228(r_u32(frame + 0x20)));
        VSync(0);
        VSync(0);
        td5_poll_pads();
        w_u32(frame + 0x10, td5_80056228(r_u32(frame + 0x20)));
        VSync(0);
        VSync(0);
    }
    w_u32(frame + 0x10, td5_80056228(r_u32(frame + 0x20)));
    status = r_u32(frame + 0x10);
    switch (status)
    {
    case 1:
        offset = r_u32(frame + 0x20) * 4396;
        w_u32(0x800A0E3C + offset, 0);
        offset = r_u32(frame + 0x20) * 4396;
        w_u32(0x800A0E40 + offset, 0);
        return offset;
    case 2:
        td5_800562dc(r_u32(frame + 0x20));
        offset = r_u32(frame + 0x20) * 4396;
        w_u32(0x800A0E3C + offset, 0);
        td5_800ba890(r_u32(frame + 0x20), 1, frame);
        return td5_800ba084();
    case 3:
        td5_80055f88(r_u32(0x80088308));
        _card_clear(r_u32(frame + 0x20) << 4);
        w_u32(frame + 0x14, td5_80056088());
        td5_800ba890(r_u32(frame + 0x20), 1, frame);
        td5_800561cc(r_u32(frame + 0x20));
        w_u32(frame + 0x14, td5_80056158());
        td5_800ba890(r_u32(frame + 0x20), 1, frame);
        result = r_u32(frame + 0x14);
        if (result == 0)
        {
            td5_scan_card(r_u32(frame + 0x20), invalid_header_index);
            offset = r_u32(frame + 0x20) * 4396;
            w_u32(0x800A0E3C + offset, 1);
            offset = r_u32(frame + 0x20) * 4396;
            w_u32(0x800A0E40 + offset, 1);
        }
        else if (result == 1 || result == 2)
        {
            offset = r_u32(frame + 0x20) * 4396;
            w_u32(0x800A0E3C + offset, 0);
            offset = r_u32(frame + 0x20) * 4396;
            w_u32(0x800A0E40 + offset, 0);
        }
        else if (result == 3)
        {
            offset = r_u32(frame + 0x20) * 4396;
            w_u32(0x800A0E3C + offset, 1);
            offset = r_u32(frame + 0x20) * 4396;
            w_u32(0x800A0E40 + offset, 0);
        }
        return td5_800ba084();
    default:
        return status;
    }
}

static void td5_card_menu_set_text(uint32 destination_pointer, uint32 text)
{
    uint32 destination = r_u32(destination_pointer);
    uint32 length = 0;

    while (r_u8(text + length) != 0)
        ++length;
    sprintf((char *)psx_addr(destination, length + 1), (const char *)psx_addr(text, length + 1));
}

static void td5_card_menu_clear_text(void)
{
    td5_card_menu_set_text(0x800BD074, 0x800B1070);
    td5_card_menu_set_text(0x800BD0EC, 0x800B1070);
    td5_card_menu_set_text(0x800BD344, 0x800B1070);
    td5_card_menu_set_text(0x800BD3BC, 0x800B1070);
}

static void td5_card_menu_count_saved(uint32 frame, uint32 base, uint32 use_selected)
{
    uint32 index, offset;

    w_u32(frame + 0x28, 0);
    w_u32(frame + 0x24, 0);
    while ((sint32)r_u32(frame + 0x24) < 15)
    {
        index = r_u32(frame + 0x24);
        offset = index * 292;
        if (use_selected != 0)
            offset += r_u32(0x800BF424) * 4396;
        if (r_s16(base + offset) == 1)
            w_u32(frame + 0x28, r_u32(frame + 0x28) + 1);
        w_u32(frame + 0x24, r_u32(frame + 0x24) + 1);
    }
}

// TODO: Adapt memory-card menus to xport_save/load storage
void td5_800b80c8(uint32 mode, uint32 original_stack, uint32 invalid_header_index)
{
    uint32 frame = original_stack - 0x38;
    uint32 state, value, selection, index, slot, result, filename;

    FUNCTION_MARKER(0x800B80C8, "SLUS_006.10");
    w_u32(frame + 0x38, mode);
    w_u32(frame + 0x18, r_u32(0x800BF110));
    w_u32(0x800BF3A0, r_u32(frame + 0x38));
    td5_800ba234(0, frame, invalid_header_index);
    td5_800ba234(1, frame, invalid_header_index);
    w_u32(0x800BC3F8, r_u32(0x800BC3F8) + 1);
    if (r_u32(0x800BF110) >= 12)
        goto finished;
    state = r_u32(0x800BF110);
    switch (state)
    {
    case 0:
        td5_card_menu_set_text(0x800BD074, 0x800B1048);
        td5_card_menu_set_text(0x800BD0EC, 0x800B1060);
        if (r_u32(0x800BC3F8) == 3)
        {
            td5_800562dc(0);
            td5_800562dc(1);
            td5_800ba890(0, 2, frame);
            td5_scan_card_slots(0, 2, invalid_header_index);
            td5_800ba084();
            w_u32(0x800BF110, 2);
            td5_card_menu_set_text(0x800BD074, 0x800B1070);
            td5_card_menu_set_text(0x800BD0EC, 0x800B1070);
        }
        break;
    case 2:
        td5_card_menu_set_text(0x800BD0EC, 0x800B1070);
        if (r_u32(0x800875D0) == 0)
        {
            value = r_u32(0x800B02D0);
            w_u32(0x800B02D0, (uint32)td5_move_menu_selection((sint32)value, 3));
            selection = r_u32(0x800B02D0);
            switch (selection)
            {
            case 0:
                if ((r_u16(0x800884E8) & 0x4008) != 0)
                {
                    td5_800530f0();
                    w_u32(0x800BF424, 0);
                    w_u32(0x800BF110, 6);
                    w_u32(0x800BF41C, 8);
                    w_u32(0x800BF3B0, 2);
                    w_u32(0x800887B8, 0xFFFFFFFF);
                }
                td5_card_menu_set_text(0x800BD074, 0x800B1074);
                break;
            case 1:
                if ((r_u16(0x800884E8) & 0x4008) != 0)
                {
                    td5_800530f0();
                    w_u32(0x800BF424, 0);
                    w_u32(0x800BF39C, 0);
                    w_u32(0x800BF110, 6);
                    w_u32(0x800BF41C, 7);
                    w_u32(0x800BF3B0, 2);
                    w_u32(0x800887B8, 0xFFFFFFFF);
                }
                td5_card_menu_set_text(0x800BD074, 0x800B1090);
                break;
            case 2:
                if ((r_u16(0x800884E8) & 0x4008) != 0)
                {
                    td5_800530f0();
                    w_u32(0x800BF0F4, 1);
                    w_u32(0x800BF424, 0);
                    w_u32(0x800BF110, 1);
                    w_u32(0x800BF41C, 3);
                    w_u32(0x800BF3B0, 2);
                    w_u32(0x800887B8, 0xFFFFFFFF);
                }
                td5_card_menu_set_text(0x800BD074, 0x800B10AC);
                break;
            }
        }
        else
        {
            value = r_u32(0x800B02D0);
            w_u32(0x800B02D0, (uint32)td5_move_menu_selection((sint32)value, 2));
            selection = r_u32(0x800B02D0);
            if (selection == 0)
            {
                td5_card_menu_set_text(0x800BD074, 0x800B1074);
                if ((r_u16(0x800884E8) & 0x4008) != 0)
                {
                    td5_800530f0();
                    w_u32(0x800BF424, 0);
                    w_u32(0x800BF110, 6);
                    w_u32(0x800BF41C, 8);
                    w_u32(0x800BF3B0, 2);
                    w_u32(0x800887B8, 0xFFFFFFFF);
                }
            }
            else if (selection == 1)
            {
                td5_card_menu_set_text(0x800BD074, 0x800B10BC);
                if ((r_u16(0x800884E8) & 0x4008) != 0)
                {
                    td5_800530f0();
                    if (r_u32(0x80087694) != 0)
                    {
                        w_u32(0x80087694, 0);
                        w_u32(0x8008858C, 3);
                        w_u32(0x800AE0C4, 1);
                        td5_800557c0();
                        w_u32(frame + 0x10, 0x1400);
                        value = r_u32(0x80087700);
                        td5_8005490c(13, 20, (sint32)value, 0, 0x1400);
                    }
                    else
                    {
                        td5_card_menu_set_text(0x800BD074, 0x800B10C4);
                        td5_card_menu_set_text(0x800BD0EC, 0x800B10D4);
                        w_u32(0x800BF110, 4);
                        w_u32(0x800BF41C, 2);
                    }
                }
            }
        }
        if ((r_u16(0x800884E8) & 0x1000) != 0)
        {
            td5_800530f0();
            w_u32(0x80087694, 0);
            w_u32(0x800875D0, 0);
            if (r_u32(frame + 0x38) == 1)
                td5_800b217c();
            else
                td5_800a99f0();
        }
        break;
    case 6:
        w_u32(frame + 0x1C, td5_800ba700(0));
        w_u32(frame + 0x20, td5_800ba700(1));
        if (r_u32(0x800B02D0) == 0)
        {
            td5_card_menu_count_saved(frame, 0x8009FE38, 0);
            if (r_u32(frame + 0x28) == 0)
                w_u32(frame + 0x1C, 1);
            td5_card_menu_count_saved(frame, 0x800A0F64, 0);
            if (r_u32(frame + 0x28) == 0)
                w_u32(frame + 0x20, 1);
        }
        if (r_u32(frame + 0x1C) != 0 && r_u32(frame + 0x20) != 0)
        {
            w_u32(0x800BF110, r_u32(0x800BF3B0));
            break;
        }
        value = r_u32(0x800B02D0);
        td5_card_menu_set_text(0x800BD074, 0x800B10E0);
        td5_card_menu_set_text(0x800BD0EC, value == 0 ? 0x800B10F8 : 0x800B1104);
        if (r_u32(frame + 0x1C) != 0)
            w_u32(0x800BF424, 1);
        if (r_u32(frame + 0x20) != 0)
            w_u32(0x800BF424, 0);
        if ((r_u32(0x80088108) & 1) == 0)
            w_u32(0x800887B8, r_u32(0x800BF424) + 5);
        else
            w_u32(0x800887B8, 0xFFFFFFFF);
        if ((r_u16(0x800884E8) & 0x1000) != 0)
            w_u32(0x800BF110, r_u32(0x800BF3B0));
        if ((r_u16(0x800884E8) & 0x4008) != 0)
            w_u32(0x800BF110, r_u32(0x800BF41C));
        if ((r_u16(0x800884E8) & 0x80) != 0 && r_s32(0x800BF424) > 0 && r_u32(frame + 0x1C) == 0)
        {
            w_u32(0x800BF424, r_u32(0x800BF424) - 1);
            td5_80053030();
        }
        if ((r_u16(0x800884E8) & 0x20) != 0 && r_s32(0x800BF424) <= 0 && r_u32(frame + 0x20) == 0)
        {
            w_u32(0x800BF424, r_u32(0x800BF424) + 1);
            td5_80053030();
        }
        break;
    case 7:
        w_u32(frame + 0x1C, td5_800ba700(r_u32(0x800BF424)));
        if (r_u32(frame + 0x1C) != 0)
        {
            w_u32(0x800BF110, r_u32(0x800BF3B0));
            break;
        }
        w_u32(0x800BF0F4, 1);
        slot = r_u32(0x800BF424);
        if (r_u32(0x800A0E40 + slot * 4396) == 0)
        {
            w_u32(0x800BF0F4, 1);
            w_u32(0x800BF110, 9);
            break;
        }
        if (r_u32(0x800BF39C) == 0)
        {
            td5_card_menu_count_saved(frame, 0x8009FE38, 1);
            if (r_u32(frame + 0x28) != 0)
            {
                w_u32(0x800BF0F4, 1);
                w_u32(0x800BF39C, 1);
                w_u32(0x800BF110, 11);
                break;
            }
        }
        td5_card_menu_set_text(0x800BD074, 0x800B1110);
        td5_card_menu_set_text(0x800BD0EC, 0x800B1124);
        if (r_u32(0x800BC3F8) == 3)
        {
            td5_card_menu_count_saved(frame, 0x8009FE38, 1);
            if (r_u32(frame + 0x28) == 0 && td5_80056cfc(r_u32(0x800BF424), frame) != 1)
            {
                td5_800ba890(r_u32(0x800BF424), 1, frame);
                td5_scan_card_slots((sint32)r_u32(0x800BF424), 1, invalid_header_index);
                td5_800ba084();
                td5_card_menu_set_text(0x800BD074, 0x800B112C);
                td5_card_menu_set_text(0x800BD0EC, 0x800B113C);
                w_u32(0x800BF110, 4);
                w_u32(0x800BF41C, r_u32(0x800BF3B0));
                break;
            }
            result = td5_80056d60((sint32)r_u32(0x800BF424));
            td5_800ba890(r_u32(0x800BF424), 1, frame);
            td5_scan_card_slots((sint32)r_u32(0x800BF424), 1, invalid_header_index);
            td5_800ba084();
            td5_card_menu_set_text(0x800BD074, result == 1 ? 0x800B1150 : 0x800B1160);
            td5_card_menu_set_text(0x800BD0EC, 0x800B1070);
            w_u32(0x800BF110, 4);
            w_u32(0x800BF41C, r_u32(0x800BF3B0));
        }
        break;
    case 9:
    case 11:
        w_u32(0x800887B8, 0xFFFFFFFF);
        if ((r_u32(0x80088108) & 1) == 0)
            w_u32(0x800887B8, r_u32(0x800BF0F4) + 9);
        td5_card_menu_set_text(0x800BD074, state == 9 ? 0x800B1178 : 0x800B1250);
        td5_card_menu_set_text(0x800BD0EC, state == 9 ? 0x800B1194 : 0x800B1264);
        td5_card_menu_set_text(0x800BD344, 0x800B11A0);
        td5_card_menu_set_text(0x800BD3BC, 0x800B11A4);
        if (state == 9)
        {
            slot = r_u32(0x800BF424);
            if (r_u32(0x800A0E40 + slot * 4396) != 0)
            {
                td5_card_menu_clear_text();
                w_u32(0x800BF110, r_u32(0x800BF3B0));
                return;
            }
        }
        if ((r_u16(0x800884E8) & 0x80) != 0 && r_s32(0x800BF0F4) > 0)
        {
            w_u32(0x800BF0F4, r_u32(0x800BF0F4) - 1);
            td5_80053030();
        }
        if ((r_u16(0x800884E8) & 0x20) != 0 && r_s32(0x800BF0F4) <= 0)
        {
            w_u32(0x800BF0F4, r_u32(0x800BF0F4) + 1);
            td5_80053030();
        }
        if ((r_u16(0x800884E8) & 0x4008) != 0)
        {
            if (state == 9)
            {
                if (r_u32(0x800BF0F4) == 0)
                    w_u32(0x800BF110, 10);
                else
                {
                    td5_card_menu_set_text(0x800BD074, 0x800B1070);
                    td5_card_menu_set_text(0x800BD0EC, 0x800B1070);
                    w_u32(0x800BF110, r_u32(0x800BF3B0));
                }
                td5_card_menu_set_text(0x800BD344, 0x800B1070);
                td5_card_menu_set_text(0x800BD3BC, 0x800B1070);
            }
            else if (r_u32(0x800BF0F4) == 0)
            {
                w_u32(0x800BF110, 7);
                td5_card_menu_clear_text();
            }
            else
            {
                td5_card_menu_clear_text();
                w_u32(0x800BF110, r_u32(0x800BF3B0));
            }
        }
        break;
    case 10:
        w_u32(0x800BF0F4, 0);
        td5_card_menu_set_text(0x800BD074, 0x800B11A8);
        td5_card_menu_set_text(0x800BD0EC, 0x800B1060);
        if (r_u32(0x800BC3F8) == 3)
        {
            result = td5_8005712c(r_u32(0x800BF424), frame);
            td5_800ba890(r_u32(0x800BF424), 1, frame);
            td5_scan_card_slots((sint32)r_u32(0x800BF424), 1, invalid_header_index);
            td5_800ba084();
            td5_card_menu_set_text(0x800BD074, result == 1 ? 0x800B11C0 : 0x800B11E0);
            td5_card_menu_set_text(0x800BD0EC, result == 1 ? 0x800B11D0 : 0x800B11F4);
            w_u32(0x800BF110, 4);
            if (result == 1)
                w_u32(0x800BF41C, 7);
            else
                w_u32(0x800BF41C, r_u32(0x800BF3B0));
        }
        break;
    case 4:
        w_u32(0x800BC3FC, r_u32(0x800BC3FC) + 1);
        if ((r_u16(0x800884E8) & 0x4008) != 0 || r_s32(0x800BC3FC) >= 31)
        {
            w_u32(0x800BC3FC, 0);
            td5_card_menu_clear_text();
            w_u32(0x800BF110, r_u32(0x800BF41C));
        }
        break;
    case 8:
        td5_card_menu_set_text(0x800BD074, 0x800B1204);
        td5_card_menu_set_text(0x800BD0EC, 0x800B121C);
        if (r_u32(0x800BC3F8) == 3)
        {
            result = td5_80056ff4((sint32)r_u32(0x800BF424));
            td5_800ba890(r_u32(0x800BF424), 1, frame);
            td5_scan_card_slots((sint32)r_u32(0x800BF424), 1, invalid_header_index);
            td5_800ba084();
            td5_card_menu_set_text(0x800BD074, result == 1 ? 0x800B1224 : 0x800B1238);
            td5_card_menu_set_text(0x800BD0EC, 0x800B1070);
            w_u32(0x800BF110, 4);
            w_u32(0x800BF41C, r_u32(0x800BF3B0));
        }
        break;
    case 1:
        w_u32(frame + 0x1C, td5_800ba700(0));
        w_u32(frame + 0x20, td5_800ba700(1));
        w_u32(frame + 0x1C, r_u32(frame + 0x1C) & 1);
        w_u32(frame + 0x20, r_u32(frame + 0x20) & 1);
        if (r_u32(frame + 0x1C) != 0 && r_u32(frame + 0x20) != 0)
            w_u32(0x800BF110, r_u32(0x800BF3B0));
        if (r_u32(frame + 0x1C) != 0)
        {
            value = r_u32(0x800BF0FC);
            w_u32(0x800BF0FC, (uint32)((sint32)value % 5) + 5);
        }
        if (r_u32(frame + 0x20) != 0)
        {
            value = r_u32(0x800BF0FC);
            w_u32(0x800BF0FC, (uint32)((sint32)value % 5));
        }
        if ((r_u32(0x80088108) & 1) == 0)
            w_u32(0x800887B8, 0xFFFFFFFF);
        if ((r_u32(0x80088108) & 1) == 0)
        {
            value = r_u32(0x800BF0FC);
            selection = (uint32)((sint32)value % 10);
            value = r_u32(0x800BF100);
            w_u32(0x800887B8, selection + value * 10 + 11);
        }
        if ((r_u16(0x800884E8) & 0x80) != 0 && r_s32(0x800BF0FC) > 0)
        {
            w_u32(0x800BF0FC, r_u32(0x800BF0FC) - 1);
            td5_80053030();
        }
        if ((r_u16(0x800884E8) & 0x20) != 0 && r_s32(0x800BF0FC) < 9)
        {
            w_u32(0x800BF0FC, r_u32(0x800BF0FC) + 1);
            td5_80053030();
        }
        if ((r_u16(0x800884E8) & 0x10) != 0 && r_s32(0x800BF100) > 0)
        {
            w_u32(0x800BF100, r_u32(0x800BF100) - 1);
            td5_80053030();
        }
        if ((r_u16(0x800884E8) & 0x40) != 0 && r_s32(0x800BF100) < 2)
        {
            w_u32(0x800BF100, r_u32(0x800BF100) + 1);
            td5_80053030();
        }
        value = r_u32(0x800BF0FC);
        w_u32(0x800BF424, (uint32)((sint32)value / 5));
        value = r_u32(0x800BF0FC);
        selection = (uint32)((sint32)value % 5);
        value = r_u32(0x800BF100);
        w_u32(0x800BCCA8, selection + value * 5);
        if ((r_u16(0x800884E8) & 0x4008) != 0)
        {
            td5_800530f0();
            index = r_u32(0x800BCCA8);
            slot = r_u32(0x800BF424);
            if (r_s16(0x8009FE38 + index * 292 + slot * 4396) != 0)
                w_u32(0x800BF110, r_u32(0x800BF41C));
            else
                w_u32(0x800BF110, r_u32(0x800BF3B0));
        }
        if ((r_u16(0x800884E8) & 0x1000) != 0)
        {
            td5_800530f0();
            w_u32(0x800BF110, r_u32(0x800BF3B0));
        }
        break;
    case 3:
        w_u32(0x800887B8, 0xFFFFFFFF);
        if ((r_u32(0x80088108) & 1) == 0)
            w_u32(0x800887B8, r_u32(0x800BF0F4) + 9);
        td5_card_menu_set_text(0x800BD074, 0x800B1278);
        td5_card_menu_set_text(0x800BD0EC, 0x800B121C);
        td5_card_menu_set_text(0x800BD344, 0x800B11A0);
        td5_card_menu_set_text(0x800BD3BC, 0x800B11A4);
        if ((r_u16(0x800884E8) & 0x80) != 0 && r_s32(0x800BF0F4) > 0)
        {
            w_u32(0x800BF0F4, r_u32(0x800BF0F4) - 1);
            td5_80053030();
        }
        if ((r_u16(0x800884E8) & 0x20) != 0 && r_s32(0x800BF0F4) <= 0)
        {
            w_u32(0x800BF0F4, r_u32(0x800BF0F4) + 1);
            td5_80053030();
        }
        if ((r_u16(0x800884E8) & 0x4008) != 0)
        {
            td5_card_menu_set_text(0x800BD344, 0x800B1070);
            td5_card_menu_set_text(0x800BD3BC, 0x800B1070);
            if (r_u32(0x800BF0F4) == 0)
            {
                slot = r_u32(0x800BF424);
                index = r_u32(0x800BCCA8);
                filename = 0x8009FE18 + slot * 4396 + index * 292;
                result = td5_800562a4(filename, (sint32)r_u32(0x800BF424), frame);
                td5_800ba890(r_u32(0x800BF424), 1, frame);
                td5_scan_card_slots((sint32)r_u32(0x800BF424), 1, invalid_header_index);
                td5_800ba084();
                td5_card_menu_set_text(0x800BD074, result != 0 ? 0x800B128C : 0x800B12AC);
                td5_card_menu_set_text(0x800BD0EC, result != 0 ? 0x800B129C : 0x800B121C);
                w_u32(0x800BF110, 4);
                w_u32(0x800BF41C, r_u32(0x800BF3B0));
            }
            else
            {
                w_u32(0x800BF110, r_u32(0x800BF3B0));
                td5_card_menu_set_text(0x800BD074, 0x800B1070);
                td5_card_menu_set_text(0x800BD0EC, 0x800B1070);
            }
        }
        break;
    }

finished:
    value = r_u32(frame + 0x18);
    selection = r_u32(0x800BF110);
    if (value != selection)
        w_u32(0x800BC3F8, 0);
}

/* Preserve the original untouched distance and identifier words on the guest stack */
uint32 td5_80053aac(uint32 view, uint32 original_stack, uint32 speed_carrier)
{
    uint32 frame = original_stack - 0xB0;
    uint32 offset = view * 32;
    uint32 first, second, third, fourth, value, other, dx, dz;
    uint32 count, filled = 0, car = 0, car_offset = 0;
    uint32 pass, position, distance_pointer, id_pointer, bound, inner_bound;
    uint32 output, companion, selected, selected_offset, result, saved_speed = speed_carrier;

    FUNCTION_MARKER(0x80053AAC, "SLUS_006.10");
    first = r_u32(0x80097648);
    second = r_u32(0x8009764C);
    third = r_u32(0x80097650);
    fourth = r_u32(0x80097654);
    w_u32(0x800A2010 + offset, first);
    w_u32(0x800A2014 + offset, second);
    w_u32(0x800A2018 + offset, third);
    w_u32(0x800A201C + offset, fourth);
    first = r_u32(0x80097658);
    second = r_u32(0x8009765C);
    third = r_u32(0x80097660);
    fourth = r_u32(0x80097664);
    w_u32(0x800A2020 + offset, first);
    w_u32(0x800A2024 + offset, second);
    w_u32(0x800A2028 + offset, third);
    w_u32(0x800A202C + offset, fourth);
    count = r_u32(0x800875E4);
    if ((sint32)count > 0)
    {
        do
        {
            if (car == view)
            {
                distance_pointer = frame + 0x10 + filled * 4;
                w_u32(distance_pointer + 0x38, view);
                value = r_u32(0x80098664 + car_offset);
                other = r_u32(0x8009765C);
                value = (uint32)((sint32)value >> 8);
                dx = other - value;
                if ((sint32)dx < 0)
                    dx = value - other;
                value = r_u32(0x8009866C + car_offset);
                other = r_u32(0x80097664);
                value = (uint32)((sint32)value >> 8);
                dz = other - value;
                if ((sint32)dz < 0)
                    dz = value - other;
                value = 0x10000;
                if ((sint32)dx <= 0x7FFF && (sint32)dz <= 0x7FFF)
                    value = SquareRoot0((sint32)(dx * dx + dz * dz));
                w_u32(distance_pointer, value);
                ++filled;
            }
            count = r_u32(0x800875E4);
            ++car;
            car_offset += 864;
        } while ((sint32)car < (sint32)count);
        count = r_u32(0x800875E4);
    }
    bound = count - 2;
    for (pass = 0; pass < 6; ++pass)
    {
        position = 1;
        if ((sint32)position < (sint32)bound)
        {
            value = r_u32(0x800875E4);
            distance_pointer = frame + 0x14;
            id_pointer = frame + 0x50;
            inner_bound = value - 2;
            do
            {
                first = r_u32(distance_pointer);
                second = r_u32(distance_pointer + 4);
                if ((sint32)second < (sint32)first)
                {
                    first = r_u32(id_pointer);
                    second = r_u32(distance_pointer + 0x38);
                    w_u32(distance_pointer + 0x38, first);
                    w_u32(id_pointer, second);
                    first = r_u32(distance_pointer + 4);
                    second = r_u32(distance_pointer);
                    w_u32(distance_pointer, first);
                    w_u32(distance_pointer + 4, second);
                }
                distance_pointer += 4;
                ++position;
                id_pointer += 4;
            } while ((sint32)position < (sint32)inner_bound);
        }
    }
    car_offset = view * 864;
    first = (uint32)r_s16(0x800987B6 + car_offset);
    second = r_u32(0x80098788 + car_offset);
    value = 256 - first;
    other = second - 4000;
    value = other - (uint32)((sint32)value / 4);
    value <<= 12;
    output = view * 184;
    w_u32(0x800919BC + output, (uint32)((sint32)value / 5000) + 4096);
    first = r_u32(0x80098788);
    second = (uint32)r_s16(0x800987B6 + car_offset);
    value = (256 - second) * 2;
    other = (uint32)((sint32)first / 4);
    if ((sint32)(other + 3072) < 4097)
        value = other - (value - 3072);
    else
        value = 4096 - value;
    other = r_u32(0x80088668);
    w_u32(0x800919B8 + output, value);
    if (other != 0)
    {
        value = r_u32(0x800919BC + output);
        companion = (view * 2 + 1) * 92;
        w_u32(0x800919B8 + companion, 0xFFFFF000);
        w_u32(0x800919BC + companion, value);
    }
    selected = r_u32(frame + 0x48);
    output = (view + 2) * 184;
    companion = ((view + 2) * 2 + 1) * 92;
    if ((sint32)selected < 6)
    {
        if (r_u32(0x8008873C) != 0)
        {
            selected_offset = selected * 864;
            saved_speed = r_u32(0x80098788 + selected_offset);
            w_u32(0x80098788 + selected_offset, 2000);
        }
        selected_offset = selected * 864;
        value = r_u32(0x80098788 + selected_offset);
        value = (value - 4000) << 12;
        w_u32(0x800919BC + output, (uint32)((sint32)value / 5000) + 4096);
        value = r_u32(0x80098788 + selected_offset);
        value = (uint32)((sint32)value / 4) + 3072;
        if ((sint32)value > 4096)
            value = 4096;
        other = r_u32(0x80088668);
        w_u32(0x800919B8 + output, value);
        if (other != 0)
        {
            value = r_u32(0x800919BC + output);
            w_u32(0x800919B8 + companion, 0xFFFFF000);
            w_u32(0x800919C4 + companion, 0x80098664 + selected_offset);
            w_u32(0x800919C8 + companion, 0x80098634 + selected_offset);
            w_u32(0x800919BC + companion, value);
        }
        result = r_u32(0x8008873C);
        w_u32(0x800919C4 + output, 0x80098664 + selected_offset);
        w_u32(0x800919C8 + output, 0x80098634 + selected_offset);
        if (result != 0)
            w_u32(0x80098788 + selected_offset, saved_speed);
        return result;
    }
    other = r_u32(0x80088668);
    w_u32(0x800919BC + output, 4096);
    w_u32(0x800919B8 + output, 4096);
    if (other != 0)
    {
        value = r_u32(0x800919BC + output);
        w_u32(0x800919B8 + companion, 0xFFFFF000);
        selected_offset = selected * 864;
        w_u32(0x800919C4 + companion, 0x80098664 + selected_offset);
        w_u32(0x800919C8 + companion, 0x80098634 + selected_offset);
        w_u32(0x800919BC + companion, value);
    }
    selected_offset = selected * 864;
    result = 0x80098634 + selected_offset;
    w_u32(0x800919C4 + output, 0x80098664 + selected_offset);
    w_u32(0x800919C8 + output, result);
    return result;
}
