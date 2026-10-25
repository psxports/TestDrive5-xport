#include "td5_functions.h"
#include <stdio.h>
#include <string.h>

uint32 td5_80052348(void)
{
    DRAWENV environment;
    uint32 result;
    uint32 allocation;
    uint32 buffer;
    uint32 index;
    uint32 packet;
    uint32 code;
    uint32 value;
    uint32 other;
    uint32 link;
    uint32 count;
    uint32 base;
    uint32 record;
    uint32 start = 0;
    uint32 total = 0;
    uint32 original_total;
    uint32 destination;

    FUNCTION_MARKER(0x80052348, "SLUS_006.10");
    result = r_u32(0x80087730);
    if (result != 0)
        return result;
    allocation = td5_alloc_memory(1152);
    w_u32(0x80088730, allocation);
    allocation = td5_alloc_memory(1152);
    w_u32(0x80088734, allocation);
    td5_find_texture((const char *)psx_addr(0x800111C0, 9));
    for (buffer = 0; buffer < 2; ++buffer)
    {
        packet = r_u32(0x80088730 + buffer * 4);
        for (index = 0; index < 48; ++index)
        {
            w_u8(packet + 3, 5);
            w_u8(packet + 7, 0x2A);
            w_u8(packet + 4, 88);
            w_u8(packet + 5, 88);
            w_u8(packet + 6, 88);
            packet += 24;
        }
        packet = 0x8009D408 + buffer * 16;
        setLineF2((LINE_F2 *)psx_addr(packet, sizeof(LINE_F2)));
        w_u8(packet + 4, 128);
        w_u8(packet + 5, 128);
        w_u8(packet + 6, 0);
        w_u16(packet + 8, 29);
        w_u16(packet + 10, 159);
        w_u16(packet + 12, 131);
        w_u16(packet + 14, 159);
        packet = 0x8009D428 + buffer * 16;
        setLineF2((LINE_F2 *)psx_addr(packet, sizeof(LINE_F2)));
        w_u8(packet + 4, 128);
        w_u8(packet + 5, 128);
        w_u8(packet + 6, 0);
        w_u16(packet + 8, 131);
        w_u16(packet + 10, 159);
        w_u16(packet + 12, 131);
        w_u16(packet + 14, 225);
        packet = 0x8009D448 + buffer * 16;
        setLineF2((LINE_F2 *)psx_addr(packet, sizeof(LINE_F2)));
        w_u8(packet + 4, 128);
        w_u8(packet + 5, 128);
        w_u8(packet + 6, 0);
        w_u16(packet + 8, 29);
        w_u16(packet + 10, 225);
        w_u16(packet + 12, 131);
        w_u16(packet + 14, 225);
        packet = 0x8009D468 + buffer * 16;
        setLineF2((LINE_F2 *)psx_addr(packet, sizeof(LINE_F2)));
        w_u8(packet + 4, 128);
        w_u8(packet + 5, 128);
        w_u8(packet + 6, 0);
        w_u16(packet + 8, 29);
        w_u16(packet + 10, 159);
        w_u16(packet + 12, 29);
        w_u16(packet + 14, 225);
        packet = 0x800A1F80 + buffer * 24;
        w_u8(packet + 7, 0x2A);
        w_u8(packet + 3, 5);
        w_u8(packet + 4, 0);
        w_u8(packet + 5, 0);
        w_u8(packet + 6, 0);
        w_u16(packet + 8, 0);
        w_u16(packet + 10, 0);
        w_u16(packet + 12, 100);
        w_u16(packet + 14, 0);
        w_u16(packet + 16, 0);
        w_u16(packet + 18, 64);
        w_u16(packet + 20, 100);
        w_u16(packet + 22, 64);
        SetDefDrawEnv(&environment, 30, 168 + buffer * 256, 100, 64);
        environment.tpage = 0;
        SetDrawEnv((DR_ENV *)psx_addr(0x8008CE20 + buffer * 64, sizeof(DR_ENV)), &environment);
        packet = 0x8008D168 + buffer * 32;
        w_u8(packet + 3, 3);
        w_u8(packet + 7, 0x42);
        w_u8(packet + 4, 112);
        w_u8(packet + 5, 112);
        w_u8(packet + 6, 112);
        packet += 16;
        w_u8(packet + 3, 3);
        w_u8(packet + 7, 0x42);
        w_u8(packet + 4, 112);
        w_u8(packet + 5, 112);
        w_u8(packet + 6, 112);
        for (index = 0; index < 12; ++index)
        {
            packet = 0x800A3BC8 + buffer * 288 + index * 24;
            w_u8(packet + 3, 5);
            w_u8(packet + 7, 0x28);
            w_u8(packet + 4, r_u8(0x8007E784 + index * 4));
            w_u8(packet + 5, r_u8(0x8007E785 + index * 4));
            code = r_u8(packet + 7);
            value = r_u8(0x8007E786 + index * 4);
            w_u8(packet + 7, code | 2);
            w_u8(packet + 6, value);
        }
    }
    count = r_u32(0x80088424);
    if ((sint32)count > 0)
    {
        base = r_u32(0x800884C4);
        record = base;
        for (index = 0; (sint32)index < (sint32)count; ++index)
        {
            value = r_u8(record);
            if (value == 8)
            {
                w_u16(0x8008D000 + total * 6, start);
                start = index + 1;
                w_u16(0x8008D002 + total * 6, index);
                w_u16(0x8008D004 + total * 6, 0xFFFF);
                ++total;
            }
            else if (value == 11)
            {
                destination = base + start * 24 - 16;
                w_u16(0x8008D000 + total * 6, start);
                start = index;
                w_u16(0x8008D002 + total * 6, index - 1);
                value = r_u16(destination);
                w_u16(0x8008D004 + total * 6, value);
                ++total;
            }
            record += 24;
        }
    }
    value = r_u32(0x8008841C);
    result = total * 6;
    ++total;
    w_u16(0x8008D000 + result, start);
    w_u16(0x8008D004 + result, 0xFFFF);
    w_u32(0x800883DC, total);
    w_u16(0x8008D002 + result, value - 2);
    if ((sint32)total > 0)
    {
        original_total = total;
        destination = total * 6;
        for (index = 0; (sint32)index < (sint32)original_total; ++index)
        {
            link = (uint32)r_s16(0x8008D004 + index * 6);
            if (link != 0xFFFFFFFF)
            {
                w_u16(0x8008D000 + destination, link);
                value = r_u16(0x8008D002 + index * 6);
                other = r_u16(0x8008D000 + index * 6);
                link = r_u16(0x8008D004 + index * 6);
                w_u16(0x8008D002 + destination, link + value - other);
                value = r_u16(0x8008D000 + index * 6);
                ++total;
                w_u16(0x8008D004 + destination, value);
                destination += 6;
            }
            result = (sint32)(index + 1) < (sint32)original_total;
        }
    }
    w_u32(0x80088404, total);
    return result;
}

uint32 td5_80063460(uint32 packet)
{
    FUNCTION_MARKER(0x80063460, "SLUS_006.10");
    setLineF2((LINE_F2 *)psx_addr(packet, sizeof(LINE_F2)));
    return 0x40;
}

uint32 td5_8001e034(uint32 position, sint32 mode, uint32 view)
{
    uint32 offset = view * 4;
    uint32 selected = 0x80088610 + offset;
    uint32 entry;
    uint32 value;
    uint32 first;
    uint32 second;
    uint32 result;
    uint32 reset;

    FUNCTION_MARKER(0x8001E034, "SLUS_006.10");
    entry = r_u32(selected) * 16;
    value = (uint32)r_s16(0x8007DDAE + entry) << 8;
    w_u32(0x800881A0 + offset, value);
    entry = r_u32(selected) * 16;
    value = (uint32)r_s16(0x8007DDB0 + entry) << 8;
    w_u32(0x800881A8 + offset, value);
    entry = r_u32(selected) * 16;
    value = (uint32)r_s16(0x8007DDB2 + entry) << 8;
    w_u32(0x800881B0 + offset, value);
    entry = r_u32(selected) * 16;
    first = r_u32(0x8007DDB4 + entry);
    second = r_u32(0x8007DDB8 + entry);
    w_u32(0x800887FC + view * 8, first);
    w_u32(0x80088800 + view * 8, second);
    reset = mode != 0;
    if (!reset)
    {
        entry = r_u32(selected) * 16;
        first = (uint32)r_s16(0x8007DDAC + entry);
        second = r_u32(0x80088440 + offset);
        reset = second != first;
    }
    if (reset)
    {
        first = r_u32(0x800881C0 + offset);
        second = (uint32)r_s16(position + 2);
        value = ((first << 11) + second) << 8;
        w_u32(0x80088598 + offset, value);
        value = r_u32(0x800881A0 + offset);
        w_u32(0x800883A8 + offset, value);
        value = r_u32(0x800881A8 + offset);
        w_u32(0x80088188 + offset, value);
        value = r_u32(0x800881B0 + offset);
        w_u32(0x800881B8 + offset, value);
    }
    entry = r_u32(0x80088610 + offset) * 16;
    first = (uint32)r_s16(0x8007DDAC + entry);
    result = r_u32(0x80087784);
    w_u32(0x80088190 + offset, 0);
    w_u32(0x80088198 + offset, 0);
    w_u32(0x80088458 + offset, result);
    w_u32(0x80088440 + offset, first);
    return result;
}

void td5_8003b244(void)
{
    FUNCTION_MARKER(0x8003B244, "SLUS_006.10");
}

uint32 td5_80028464(void)
{
    uint32 index;
    uint32 level;
    uint32 first;
    uint32 second;
    uint32 value;
    uint32 result;

    FUNCTION_MARKER(0x80028464, "SLUS_006.10");
    for (index = 0; index < 1024; ++index)
        w_u8(0x8009262F - index, 255);
    for (index = 0; index < 14; ++index)
    {
        w_u8(0x800A4071 + index * 20, 0);
        w_u8(0x800A4070 + index * 20, 255);
    }
    level = r_u32(0x80088770);
    first = r_u32(level);
    second = r_u32(level);
    value = (uint32)r_s16(first + 8);
    result = (uint32)r_s16(second + 10);
    w_u32(0x80087834, 0);
    w_u32(0x80088508, value);
    w_u32(0x80088738, result);
    return result;
}

void td5_8003f3b8(void)
{
    FUNCTION_MARKER(0x8003F3B8, "SLUS_006.10");
}

uint32 td5_8003b264(void)
{
    char filename[8];
    uint32 index;
    uint32 texture;
    uint32 packet;
    uint32 name;
    uint32 color;
    uint32 value;
    uint32 other;

    FUNCTION_MARKER(0x8003B264, "SLUS_006.10");
    td5_8003f3b8();
    for (index = 0; index < 50; ++index)
        w_u8(0x8009EFCF + (49 - index) * 68, 0);
    for (index = 0; index < 20; ++index)
        w_u8(0x8009EC13 - index, 0);
    w_u8(0x8008CB5B, 4);
    w_u8(0x8008CB5F, 0x20);
    for (index = 0; index < 4; ++index)
    {
        if (index == 0)
        {
            name = 0x80087984;
            packet = 0x8009D788;
        }
        else if (index == 1)
        {
            name = 0x8008798C;
            packet = 0x8009EC48;
        }
        else if (index == 2)
        {
            name = 0x8008798C;
            packet = 0x8009D948;
        }
        else
        {
            name = 0x80087994;
            packet = 0x8009D818;
        }
        texture = td5_find_texture((const char *)psx_addr(name, index == 3 ? 5 : (index == 0 ? 6 : 7)));
        if (index == 3)
            w_u32(0x800885EC, texture);
        w_u8(packet + 3, 9);
        w_u8(packet + 7, 0x2E);
        w_u8(packet + 12, r_u8(texture + 14));
        w_u8(packet + 13, r_u8(texture + 15));
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 20, other + value - 1);
        w_u8(packet + 21, r_u8(texture + 15));
        w_u8(packet + 28, r_u8(texture + 14));
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 29, other + value - 1);
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 36, other + value - 1);
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        color = index == 3 ? 255 : 128;
        w_u8(packet + 4, color);
        w_u8(packet + 5, color);
        w_u8(packet + 6, color);
        w_u8(packet + 37, other + value - 1);
        w_u16(packet + 22, r_u16(texture + 8));
        w_u16(packet + 14, r_u16(texture + 10));
    }
    for (index = 0; index < 4; ++index)
    {
        packet = 0x8009D860 + index * 20;
        w_u8(packet + 3, 4);
        w_u8(packet + 7, 0x51);
        w_u16(packet + 8, 0xFC00);
        w_u16(packet + 10, 0xFC00);
        w_u16(packet + 16, 0xFC00);
        w_u16(packet + 18, 0xFC00);
    }
    w_u8(0x8009D880, 230);
    w_u8(0x8009D878, 100);
    w_u8(0x8009D88D, 20);
    w_u8(0x8009D8A9, 190);
    w_u8(0x8009D8A0, 40);
    w_u8(0x8009D86C, 255);
    w_u8(0x8009D86D, 255);
    w_u8(0x8009D86E, 0);
    w_u8(0x8009D864, 90);
    w_u8(0x8009D865, 0);
    w_u8(0x8009D866, 0);
    w_u8(0x8009D881, 200);
    w_u8(0x8009D882, 0);
    w_u8(0x8009D879, 0);
    w_u8(0x8009D87A, 0);
    w_u8(0x8009D894, 255);
    w_u8(0x8009D895, 200);
    w_u8(0x8009D896, 0);
    w_u8(0x8009D88C, 90);
    w_u8(0x8009D88E, 0);
    w_u8(0x8009D8A8, 255);
    w_u8(0x8009D8AA, 0);
    w_u8(0x8009D8A1, 10);
    w_u8(0x8009D8A2, 0);
    for (index = 0; index < 8; ++index)
    {
        sprintf(filename, (const char *)psx_addr(0x8008799C, 8), (sint32)(index + 1));
        texture = td5_find_texture(filename);
        packet = 0x8009D7B0 + index * 10;
        w_u8(packet, r_u8(texture + 14));
        w_u8(packet + 1, r_u8(texture + 15));
        w_u8(packet + 2, r_u8(texture + 12) - 1);
        w_u8(packet + 3, r_u8(texture + 13) - 1);
        w_u16(packet + 4, r_u16(texture + 8));
        w_u16(packet + 6, r_u16(texture + 8) & 0xFF9F);
        w_u16(packet + 8, r_u16(texture + 10));
        value = r_u16(texture + 10);
        w_u16(0x800A1F70 + index * 2, value);
    }
    return 0;
}

uint32 td5_80043b58(void)
{
    uint32 texture;
    uint32 allocation;
    uint32 index;
    uint32 offset;
    uint32 packet;
    uint32 code_packet;
    uint32 value;
    uint32 other;
    uint32 first;
    uint32 second;
    uint32 third;
    uint32 fourth;
    uint32 part;

    FUNCTION_MARKER(0x80043B58, "SLUS_006.10");
    w_u32(0x800883FC, 0);
    texture = td5_find_texture((const char *)psx_addr(0x80010D44, 9));
    allocation = td5_alloc_memory(10560);
    w_u32(0x80088774, allocation);
    w_u32(0x80088604, 0);
    for (index = 0; index < 8; ++index)
        w_u8(0x8009D8D2 + (7 - index) * 7, 0);
    for (index = 0; index < 80; ++index)
    {
        offset = index * 132;
        packet = r_u32(0x80088774) + offset;
        w_u8(packet + 112, 0);
        packet = r_u32(0x80088774) + offset;
        w_u8(packet + 35, 9);
        code_packet = r_u32(0x80088774) + offset;
        w_u16(packet, 0xFED4);
        w_u16(packet + 2, 0);
        w_u16(packet + 4, 300);
        w_u16(packet + 8, 300);
        w_u16(packet + 10, 0);
        w_u16(packet + 12, 300);
        w_u16(packet + 16, 0xFED4);
        w_u16(packet + 18, 0);
        w_u16(packet + 20, 0xFED4);
        w_u16(packet + 24, 800);
        w_u16(packet + 26, 0);
        w_u16(packet + 28, 0xFED4);
        w_u8(code_packet + 39, 0x2C);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(packet + 39);
        w_u8(packet + 39, value | 2);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(packet + 39);
        w_u8(packet + 39, value & 0xFE);
        w_u8(r_u32(0x80088774) + offset + 36, 26);
        w_u8(r_u32(0x80088774) + offset + 37, 26);
        w_u8(r_u32(0x80088774) + offset + 38, 26);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(texture + 14);
        w_u8(packet + 44, value);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(texture + 15);
        w_u8(packet + 45, value);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 52, other + value - 1);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(texture + 15);
        w_u8(packet + 53, value);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(texture + 14);
        w_u8(packet + 60, value);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 61, other + value - 1);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 68, other + value - 1);
        packet = r_u32(0x80088774) + offset;
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 69, other + value - 1);
        packet = r_u32(0x80088774) + offset;
        value = r_u16(texture + 8);
        w_u16(packet + 54, value);
        value = r_u16(texture + 10);
        w_u16(packet + 46, value);
        for (part = 0; part < 32; part += 16)
        {
            first = r_u32(packet + 32 + part);
            second = r_u32(packet + 36 + part);
            third = r_u32(packet + 40 + part);
            fourth = r_u32(packet + 44 + part);
            w_u32(packet + 72 + part, first);
            w_u32(packet + 76 + part, second);
            w_u32(packet + 80 + part, third);
            w_u32(packet + 84 + part, fourth);
        }
        first = r_u32(packet + 64);
        second = r_u32(packet + 68);
        w_u32(packet + 104, first);
        w_u32(packet + 108, second);
    }
    return 0;
}

uint32 td5_80046460(void)
{
    uint32 level;
    uint32 definition;
    sint32 mode;

    FUNCTION_MARKER(0x80046460, "SLUS_006.10");
    level = r_u32(0x80088770);
    definition = r_u32(level + 36);
    mode = r_s16(definition);
    w_u32(0x80087A08, 0);
    w_u32(0x80087A04, 0);
    w_u32(0x80087A0C, (uint32)mode);
    if (mode == 1)
        return td5_800456e4();
    if (mode == 0)
        return td5_80045490();
    return mode < 2;
}

uint32 td5_80045490(void)
{
    uint32 allocation;
    uint32 index;
    uint32 buffer;
    uint32 packet;
    sint32 value;

    FUNCTION_MARKER(0x80045490, "SLUS_006.10");
    allocation = td5_alloc_memory(2560);
    w_u32(0x800882A0, allocation);
    allocation = td5_alloc_memory(2560);
    w_u32(0x800882A4, allocation);
    for (index = 0; index < 128; ++index)
    {
        value = randPSX();
        w_u16(0x80089688 + index * 8, (uint32)(value % 8000 - 4000));
        value = randPSX();
        w_u16(0x8008968A + index * 8, (uint32)(value % 5000 - 4000));
        value = randPSX();
        w_u16(0x8008968C + index * 8, (uint32)(value % 7991 + 200));
        for (buffer = 0; buffer < 2; ++buffer)
        {
            packet = r_u32(0x800882A0 + buffer * 4) + index * 20;
            w_u8(packet + 3, 4);
            w_u8(packet + 7, 0x52);
            w_u8(packet + 4, 48);
            w_u8(packet + 5, 48);
            w_u8(packet + 6, 48);
            w_u8(packet + 12, 8);
            w_u8(packet + 13, 8);
            w_u8(packet + 14, 8);
        }
    }
    for (buffer = 0; buffer < 2; ++buffer)
    {
        packet = 0x80089B08 + buffer * 32;
        w_u8(packet + 3, 7);
        w_u8(packet + 7, 0x24);
        w_u16(packet + 22, 32);
        w_u16(packet + 8, 0xFFFF);
        w_u16(packet + 10, 0xFFFF);
        w_u16(packet + 16, 0xFFFF);
        w_u16(packet + 18, 0xFFFF);
        w_u16(packet + 24, 0xFFFF);
        w_u16(packet + 26, 0xFFFF);
    }
    return 0;
}

uint32 td5_800456e4(void)
{
    uint32 allocation;
    uint32 texture;
    uint32 index;
    uint32 buffer;
    uint32 packet;
    uint32 value;
    sint32 random;

    FUNCTION_MARKER(0x800456E4, "SLUS_006.10");
    allocation = td5_alloc_memory(5120);
    w_u32(0x800882A8, allocation);
    allocation = td5_alloc_memory(5120);
    w_u32(0x800882AC, allocation);
    texture = td5_find_texture((const char *)psx_addr(0x80087A10, 6));
    w_u32(0x800882B0, texture);
    for (index = 0; index < 128; ++index)
    {
        random = randPSX();
        w_u16(0x80089688 + index * 8, (uint32)(random % 16000 - 8000));
        random = randPSX();
        w_u16(0x8008968A + index * 8, (uint32)(random % 9000 - 8000));
        random = randPSX();
        w_u16(0x8008968C + index * 8, (uint32)(random % 16183 + 200));
        for (buffer = 0; buffer < 2; ++buffer)
        {
            packet = r_u32(0x800882A8 + buffer * 4) + index * 40;
            w_u8(packet + 3, 9);
            w_u8(packet + 7, 0x2E);
            w_u8(packet + 12, r_u8(r_u32(0x800882B0) + 14));
            w_u8(packet + 13, r_u8(r_u32(0x800882B0) + 15));
            w_u8(packet + 20, r_u8(r_u32(0x800882B0) + 14) + 4);
            w_u8(packet + 21, r_u8(r_u32(0x800882B0) + 15));
            w_u8(packet + 28, r_u8(r_u32(0x800882B0) + 14));
            w_u8(packet + 29, r_u8(r_u32(0x800882B0) + 15) + 4);
            w_u8(packet + 36, r_u8(r_u32(0x800882B0) + 14) + 4);
            value = r_u8(r_u32(0x800882B0) + 15);
            w_u8(packet + 4, 64);
            w_u8(packet + 5, 64);
            w_u8(packet + 6, 64);
            w_u8(packet + 37, value + 4);
            texture = r_u32(0x800882B0);
            w_u16(packet + 22, r_u16(texture + 8));
            w_u16(packet + 14, r_u16(texture + 10));
        }
    }
    return 0;
}

uint32 td5_8003fd50(void)
{
    uint32 level;
    uint32 result;

    FUNCTION_MARKER(0x8003FD50, "SLUS_006.10");
    level = r_u32(0x80088770);
    result = r_u32(level + 8);
    w_u32(0x80088704, result);
    return result;
}

uint32 td5_8004b3d8(uint32 archive_error_index, uint32 original_stack)
{
    uint32 index;
    uint32 part;
    uint32 pointer;
    uint32 data;
    uint32 offset;
    uint32 value;
    uint32 first;
    uint32 second;
    uint32 third;
    uint32 fourth;
    sint32 scaled;
    sint32 next;
    sint32 difficulty;

    FUNCTION_MARKER(0x8004B3D8, "SLUS_006.10");
    memset(psx_addr(0x80097750, 3408), 0, 3408);
    td5_8004b2bc(archive_error_index);
    for (index = 0; index < 12; ++index)
    {
        pointer = 0x80098624 + index * 864;
        offset = index * 284;
        if ((index == 0 || r_u32(0x8008771C) != 0) && index < 2 && r_u32(0x80087650) == 0)
            goto traffic_record;
        if (index >= 6)
        {
            w_u32(pointer, 0x8007E528);
            goto traffic_record;
        }
        value = r_u32(0x80087734);
        data = r_u32(pointer);
        if (value != 0)
        {
            w_u32(0x80087A80, 0);
            w_u32(0x80087A88, 64);
            w_u32(0x80087A84, 0);
            goto traffic_set_64;
        }
        for (part = 0; part < 128; part += 16)
        {
            first = r_u32(0x8007E6A4 + part);
            second = r_u32(0x8007E6A8 + part);
            third = r_u32(0x8007E6AC + part);
            fourth = r_u32(0x8007E6B0 + part);
            w_u32(data + part, first);
            w_u32(data + part + 4, second);
            w_u32(data + part + 8, third);
            w_u32(data + part + 12, fourth);
        }
        if (r_u32(0x800877AC) != 0)
        {
            scaled = r_s16(data + 104) * 650;
            next = r_s16(data + 44);
            w_u16(data + 104, (uint32)(scaled / 256));
            scaled = next * 380;
            next = r_s16(data + 110);
            w_u16(data + 44, (uint32)(scaled / 256));
            scaled = next * 450;
            next = r_s16(data + 112);
            w_u16(data + 110, (uint32)(scaled / 256));
            w_u16(data + 112, (uint32)(next * 400 / 256));
        }
        else if (r_u32(0x800877A8) == 0)
        {
            scaled = r_s16(data + 104) * 360;
            next = r_s16(data + 44);
            w_u16(data + 104, (uint32)(scaled / 256));
            w_u16(data + 44, (uint32)(next * 300 / 256));
        }
        if (r_u32(0x80087730) != 0)
        {
            difficulty = r_s32(0x80088680);
            if (difficulty == 0)
            {
                scaled = r_s16(data + 104) * 165;
                next = r_s16(data + 44);
                w_u16(data + 104, (uint32)(scaled / 256));
                w_u16(data + 44, (uint32)(next * 220 / 256));
                w_u32(0x80087A80, 140);
                w_u32(0x80087A84, 200);
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u16(data + 116, 929);
                w_u32(0x80087A88, 100);
                w_u32(0x80087A8C, 55);
            }
            else if (difficulty == 1)
            {
                scaled = r_s16(data + 104) * 180;
                value = r_u16(data + 44);
                w_u16(data + 104, (uint32)(scaled / 256));
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u16(data + 116, 929);
                w_u32(0x80087A80, 150);
                w_u32(0x80087A88, 100);
                goto traffic_restore_44;
            }
            else if (difficulty == 2)
                goto traffic_220_280;
        }
        else if (r_u32(0x80087720) == 4)
        {
            difficulty = r_s32(0x80088680);
            if (difficulty == 0)
            {
                scaled = r_s16(data + 104) * 165;
                next = r_s16(data + 44);
                w_u16(data + 104, (uint32)(scaled / 256));
                w_u16(data + 44, (uint32)(next * 220 / 256));
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u16(data + 116, 929);
                w_u32(0x80087A80, 140);
                w_u32(0x80087A88, 100);
                w_u32(0x80087A84, 192);
                goto traffic_set_64;
            }
            else if (difficulty == 1)
            {
                first = r_u16(data + 104);
                second = r_u16(data + 44);
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u16(data + 116, 929);
                w_u32(0x80087A80, 150);
                w_u32(0x80087A88, 100);
                w_u32(0x80087A84, 192);
                w_u32(0x80087A8C, 64);
                w_u16(data + 104, first);
                w_u16(data + 44, second);
            }
            else if (difficulty == 2)
                goto traffic_220_280;
        }
        else if (r_u32(0x800876C8) == 0)
        {
            difficulty = r_s32(0x80088680);
            if (difficulty == 0)
            {
                scaled = r_s16(data + 104) * 200;
                value = r_u16(data + 44);
                w_u16(data + 104, (uint32)(scaled / 256));
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u16(data + 116, 929);
                w_u32(0x80087A80, 160);
                w_u32(0x80087A88, 64);
                goto traffic_restore_44;
            }
            else if (difficulty == 1)
            {
                scaled = r_s16(data + 104) * 185;
                value = r_u16(data + 44);
                w_u16(data + 104, (uint32)(scaled / 256));
                w_u32(0x80087A80, 256);
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u16(data + 116, 929);
                w_u32(0x80087A88, 75);
                w_u32(0x80087A84, 192);
                w_u32(0x80087A8C, 75);
                w_u16(data + 44, value);
            }
            else if (difficulty == 2)
            {
                scaled = r_s16(data + 104) * 220;
                next = r_s16(data + 44);
                w_u16(data + 104, (uint32)(scaled / 256));
                w_u16(data + 44, (uint32)(next * 270 / 256));
                w_u16(data + 116, 1075);
                w_u32(0x80087A80, 270);
                w_u32(0x80087A88, 65);
                w_u32(0x80087A84, 150);
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u32(0x80087A8C, 80);
            }
        }
        else
        {
            difficulty = r_s32(0x80088680);
            if (difficulty == 0)
            {
                scaled = r_s16(data + 104) * 200;
                value = r_u16(data + 44);
                w_u16(data + 104, (uint32)(scaled / 256));
                w_u32(0x80087A80, 180);
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u16(data + 116, 929);
                w_u32(0x80087A88, 64);
                w_u32(0x80087A84, 210);
                w_u32(0x80087A8C, 64);
                w_u16(data + 44, value);
            }
            else if (difficulty == 1)
            {
                scaled = r_s16(data + 104) * 195;
                next = r_s16(data + 44);
                w_u16(data + 104, (uint32)(scaled / 256));
                w_u16(data + 44, (uint32)(next * 270 / 256));
                w_u16(data + 116, 953);
                w_u32(0x80087A80, 310);
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u32(0x80087A88, 60);
                w_u32(0x80087A84, 192);
                w_u32(0x80087A8C, 100);
            }
            else if (difficulty == 2)
            {
                scaled = r_s16(data + 104) * 220;
                next = r_s16(data + 44);
                w_u16(data + 104, (uint32)(scaled / 256));
                w_u16(data + 44, (uint32)(next * 290 / 256));
                w_u16(data + 116, 1075);
                w_u32(0x80087A80, 325);
                w_u16(data + 110, 1000);
                w_u16(data + 112, 1000);
                w_u32(0x80087A88, 60);
                w_u32(0x80087A84, 100);
                goto traffic_set_64;
            }
        }
        goto traffic_record;
    traffic_220_280:
        scaled = r_s16(data + 104) * 220;
        next = r_s16(data + 44);
        w_u16(data + 104, (uint32)(scaled / 256));
        w_u16(data + 44, (uint32)(next * 280 / 256));
        w_u16(data + 116, 1075);
        w_u32(0x80087A80, 200);
        w_u16(data + 110, 1000);
        w_u16(data + 112, 1000);
        w_u32(0x80087A88, 100);
        w_u32(0x80087A84, 120);
        goto traffic_set_64;
    traffic_restore_44:
        w_u32(0x80087A84, 192);
        w_u32(0x80087A8C, 64);
        w_u16(data + 44, value);
        goto traffic_record;
    traffic_set_64:
        w_u32(0x80087A8C, 64);
    traffic_record:
        w_u32(0x80097824 + offset, index);
        if ((index & 1) != 0)
        {
            value = r_u32(0x800886B8);
            w_u32(0x8009775C + offset, 0);
        }
        else
        {
            value = r_u32(0x800887A8);
            w_u32(0x8009775C + offset, 1);
        }
        w_u32(0x80097750 + offset, value);
        value = r_u32(0x8007E66C + index * 4);
        w_u32(0x800977CC + offset, 0xFFFFFFFF);
        w_u32(0x80097760 + offset, value);
    }
    w_u16(r_u32(0x8009A480) + 64, 0xFF10);
    w_u16(r_u32(0x8009A480) + 66, 0xFF79);
    w_u16(r_u32(0x8009A480) + 68, 520);
    w_u16(r_u32(0x8009A480) + 72, 240);
    w_u16(r_u32(0x8009A480) + 74, 0xFF79);
    w_u16(r_u32(0x8009A480) + 76, 520);
    w_u16(r_u32(0x8009A480) + 80, 0xFF00);
    w_u16(r_u32(0x8009A480) + 82, 0xFF79);
    w_u16(r_u32(0x8009A480) + 84, 0xFE52);
    w_u16(r_u32(0x8009A480) + 88, 256);
    w_u16(r_u32(0x8009A480) + 90, 0xFF79);
    w_u16(r_u32(0x8009A480) + 92, 0xFE52);
    w_u16(r_u32(0x8009A480) + 128, 800);
    w_u16(r_u32(0x8009A480) + 130, 140);
    w_u16(r_u32(0x8009A480) + 132, 220);
    w_u16(0x8009A4D8, 0xFF10);
    w_u16(0x8009A4DA, 0xFF79);
    w_u16(0x8009A4DC, 520);
    w_u16(0x8009A4E0, 240);
    w_u16(0x8009A4E2, 0xFF79);
    w_u16(0x8009A4E4, 520);
    w_u16(0x8009A4E8, 0xFF00);
    w_u16(0x8009A4EA, 0xFF79);
    w_u16(0x8009A4EC, 0xFE52);
    w_u16(0x8009A4F0, 256);
    w_u16(0x8009A4F2, 0xFF79);
    w_u16(0x8009A4F4, 0xFE52);
    return td5_8004ee50(original_stack - 0x20);
}

uint32 td5_8004b2bc(uint32 archive_error_index)
{
    char filename[64];
    uint32 index;
    uint32 prefix;
    uint32 length;
    uint32 format;
    uint32 destination;
    uint32 log;
    uint32 size;
    uint32 buffer;
    uint32 result;
    sint32 formatted;

    FUNCTION_MARKER(0x8004B2BC, "SLUS_006.10");
    for (index = 0; index < 3; ++index)
    {
        prefix = r_u32(0x800885C4);
        length = 0;
        while (r_u8(prefix + length) != 0)
            ++length;
        if (index == 0)
        {
            format = 0x800110E0;
            destination = 0x800886B8;
            log = 0x800110EC;
        }
        else if (index == 1)
        {
            format = 0x80011104;
            destination = 0x800887A8;
            log = 0x80011110;
        }
        else
        {
            format = 0x80011128;
            destination = 0x800883E0;
            log = 0x80011138;
        }
        formatted = sprintf(filename, (const char *)psx_addr(format, index == 0 ? 11 : (index == 1 ? 12 : 14)), (const char *)psx_addr(prefix, length + 1));
        size = td5_archive_aligned_size(filename, 1, archive_error_index, (uint32)formatted);
        buffer = td5_alloc_memory(size);
        w_u32(destination, buffer);
        result = td5_load_archive_file(filename, buffer, archive_error_index);
        td5_debug_log(log);
    }
    return result;
}

uint32 td5_8001affc(uint32 car)
{
    sint32 node;
    sint32 count;
    sint32 quotient;
    uint32 remainder;

    FUNCTION_MARKER(0x8001AFFC, "SLUS_006.10");
    node = r_s16(car + 116);
    count = r_s32(0x8008841C);
    quotient = td5_divide_checked(node, count);
    remainder = (uint32)node - (uint32)quotient * (uint32)count;
    if (node < 0)
        remainder += (uint32)count;
    w_u16(car + 114, remainder);
    return (uint32)count;
}

uint32 td5_80050704(sint32 node)
{
    uint32 base;
    uint32 count;
    uint32 index = 0;
    uint32 offset = 0;
    uint32 entry;
    uint32 start;
    uint32 end;
    uint32 source;

    FUNCTION_MARKER(0x80050704, "SLUS_006.10");
    base = r_u32(0x80088584);
    count = r_u32(base + 20);
    if (count == 0)
        return 0xFFFFFFFF;
    for (;;)
    {
        entry = base + offset;
        start = r_u16(entry + 28);
        if (node >= (sint32)start)
        {
            end = r_u16(entry + 26);
            source = r_u16(entry + 24);
            if ((sint32)(start + end - source - 1) >= node)
                return (uint32)node - start + source;
        }
        base = r_u32(0x80088584);
        count = r_u32(base + 20);
        ++index;
        offset += 6;
        if (index >= count)
            return 0xFFFFFFFF;
    }
}

uint32 td5_8004ee50(uint32 original_stack)
{
    uint32 index;
    uint32 offset;
    uint32 record;
    uint32 base;
    uint32 flags;
    uint32 sides;
    uint32 side;
    uint32 node;
    uint32 value;
    uint32 height_address = original_stack - 0x20 - 0x28 - 0x18 - 0x30 - 0x98 + 0x20;
    sint32 count;

    FUNCTION_MARKER(0x8004EE50, "SLUS_006.10");
    count = r_s32(0x800875E4);
    if (count < 7)
        return 1;
    for (index = 6;; ++index)
    {
        if (count >= 13)
        {
            if (index >= 12)
                return 0;
        }
        else if ((sint32)index >= count)
            return 0;
        record = r_u32(0x800883E0);
        flags = r_u8(record + 2);
        offset = index * 284;
        w_u32(0x800977B8 + offset, index * 4 + 16);
        w_u32(0x800977D8 + offset, 0);
        w_u32(0x80097840 + offset, 0);
        w_u32(0x8009784C + offset, flags & 1);
        node = (uint32)r_s16(record);
        base = r_u32(0x800884C4);
        sides = r_u8(base + node * 24 + 3);
        side = r_u8(record + 3);
        if (side < (sides & 15))
        {
            w_u32(0x8009775C + offset, 0);
            node = (uint32)r_s16(record);
            flags = r_u8(record + 2);
            side = r_u8(record + 3);
            td5_8004d984(index, node, side, flags & 1, height_address);
            td5_8001affc(0x800984A0 + index * 864);
            record = r_u32(0x800883E0);
            w_u32(0x800883E0, record + 4);
        }
        else
        {
            w_u32(0x8009775C + offset, 1);
            node = (uint32)r_s16(record);
            node = td5_80050704((sint32)node);
            record = r_u32(0x800883E0);
            value = (uint32)r_s16(record);
            base = r_u32(0x800884C4);
            flags = r_u8(record + 2);
            sides = r_u8(base + value * 24 + 3);
            side = r_u8(record + 3);
            td5_8004d984(index, node, side - (sides & 15), flags & 1, height_address);
            td5_8001affc(0x800984A0 + index * 864);
            record = r_u32(0x800883E0);
            value = r_u16(record);
            w_u32(0x800883E0, record + 4);
            w_u16(0x80098512 + index * 864, value);
        }
        count = r_s32(0x800875E4);
    }
}

uint32 td5_80063440(uint32 packet)
{
    FUNCTION_MARKER(0x80063440, "SLUS_006.10");
    setPolyG3((POLY_G3 *)psx_addr(packet, sizeof(POLY_G3)));
    return 0x30;
}

uint32 td5_800417e0(void)
{
    uint32 texture;
    uint32 group;
    uint32 index;
    uint32 packet;
    uint32 value;
    uint32 other;

    FUNCTION_MARKER(0x800417E0, "SLUS_006.10");
    texture = td5_find_texture((const char *)psx_addr(0x80010D38, 9));
    for (group = 0; group < 4; ++group)
    {
        for (index = 0; index < 2; ++index)
        {
            packet = 0x80091240 + group * 208 + index * 40;
            w_u8(packet + 3, 9);
            w_u8(packet + 7, 0x2C);
            w_u8(packet + 12, r_u8(texture + 14));
            w_u8(packet + 13, r_u8(texture + 15));
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 20, other + value - 1);
            w_u8(packet + 21, r_u8(texture + 15));
            w_u8(packet + 28, r_u8(texture + 14));
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 29, other + value - 1);
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 36, other + value - 1);
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 37, other + value - 1);
            w_u16(packet + 22, r_u16(texture + 8));
            w_u16(packet + 14, r_u16(texture + 10));
            value = r_u8(packet + 7);
            w_u8(packet + 7, (value & 0xFE) | 2);
            packet = 0x80091290 + group * 208 + index * 28;
            td5_80063440(packet);
            value = r_u8(packet + 7);
            w_u8(packet + 7, (value & 0xFE) | 2);
            packet = 0x800912C8 + group * 208 + index * 28;
            td5_80063440(packet);
            value = r_u8(packet + 7);
            w_u8(packet + 7, (value & 0xFE) | 2);
        }
    }
    w_u32(0x800913DC, 128);
    w_u32(0x800914AC, 64);
    w_u32(0x8009130C, 0);
    w_u32(0x8009157C, 192);
    return 192;
}

uint32 td5_8003fdf4(void)
{
    uint32 texture;
    uint32 allocation;
    uint32 result;
    uint32 index;
    uint32 packet;
    uint32 value;
    uint32 other;
    uint32 code;
    uint32 clut;
    sint32 count;

    FUNCTION_MARKER(0x8003FDF4, "SLUS_006.10");
    texture = td5_find_texture((const char *)psx_addr(0x800879BC, 7));
    allocation = td5_alloc_memory(r_u32(0x800875E4) * 160);
    count = (sint32)(r_u32(0x800875E4) << 2);
    w_u32(0x80088410, allocation);
    index = 0;
    if (count > 0)
    {
        for (;;)
        {
            packet = r_u32(0x80088410) + index * 40;
            w_u8(0x8009D3F8 + (uint32)((sint32)index / 4), 0);
            w_u8(packet + 3, 9);
            w_u8(packet + 7, 0x2C);
            w_u8(packet + 12, r_u8(texture + 14));
            w_u8(packet + 13, r_u8(texture + 15));
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 20, other + value - 1);
            w_u8(packet + 21, r_u8(texture + 15));
            w_u8(packet + 28, r_u8(texture + 14));
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 29, other + value - 1);
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 36, other + value - 1);
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 37, other + value - 1);
            value = r_u16(texture + 8);
            code = r_u8(packet + 7);
            w_u16(packet + 22, value);
            clut = r_u16(texture + 10);
            w_u8(packet + 7, (code & 0xFE) | 2);
            count = (sint32)(r_u32(0x800875E4) << 2);
            ++index;
            w_u16(packet + 14, clut);
            if ((sint32)index >= count)
                break;
        }
    }
    allocation = td5_alloc_memory(r_u32(0x800875E4) * 208);
    count = (sint32)(r_u32(0x800875E4) << 2);
    w_u32(0x80088538, allocation);
    result = allocation;
    index = 0;
    if (count > 0)
    {
        for (;;)
        {
            packet = r_u32(0x80088538) + index * 52;
            w_u8(packet + 3, 12);
            w_u8(packet + 7, 0x3C);
            w_u8(packet + 12, r_u8(texture + 14));
            w_u8(packet + 13, r_u8(texture + 15));
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 24, other + value - 1);
            w_u8(packet + 25, r_u8(texture + 15));
            w_u8(packet + 36, r_u8(texture + 14));
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 37, other + value - 1);
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 48, other + value - 1);
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 49, other + value - 1);
            value = r_u16(texture + 8);
            code = r_u8(packet + 7);
            w_u16(packet + 26, value);
            clut = r_u16(texture + 10);
            w_u8(packet + 7, (code & 0xFE) | 2);
            count = (sint32)(r_u32(0x800875E4) << 2);
            ++index;
            result = (sint32)index < count;
            w_u16(packet + 14, clut);
            if (result == 0)
                break;
        }
    }
    return result;
}

uint32 td5_80040fb0(void)
{
    uint32 level;
    uint32 header;
    uint32 texture;
    uint32 result;
    uint32 index = 0;
    uint32 packet;
    uint32 value;
    uint32 other;
    uint32 code;
    uint32 clut;
    sint32 count;

    FUNCTION_MARKER(0x80040FB0, "SLUS_006.10");
    level = r_u32(0x80088770);
    header = r_u32(level);
    result = r_u8(header + 30) & 2;
    if (result == 0)
        return result;
    texture = td5_find_texture((const char *)psx_addr(0x800879CC, 7));
    result = td5_alloc_memory(r_u32(0x800875E4) * 160);
    count = (sint32)(r_u32(0x800875E4) << 2);
    w_u32(0x800885D4, result);
    if (count <= 0)
        return result;
    for (;;)
    {
        packet = r_u32(0x800885D4) + index * 40;
        w_u8(packet + 3, 9);
        w_u8(packet + 7, 0x2C);
        w_u8(packet + 12, r_u8(texture + 14));
        w_u8(packet + 13, r_u8(texture + 15));
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 20, other + value - 1);
        w_u8(packet + 21, r_u8(texture + 15));
        w_u8(packet + 28, r_u8(texture + 14));
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 29, other + value - 1);
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 36, other + value - 1);
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 37, other + value - 1);
        value = r_u16(texture + 8);
        code = r_u8(packet + 7);
        w_u16(packet + 22, value);
        clut = r_u16(texture + 10);
        w_u8(packet + 7, (code & 0xFE) | 2);
        count = (sint32)(r_u32(0x800875E4) << 2);
        ++index;
        result = (sint32)index < count;
        w_u16(packet + 14, clut);
        if (result == 0)
            return result;
    }
}

uint32 td5_8004070c(void)
{
    uint32 texture;
    uint32 allocation;
    uint32 result;
    uint32 index;
    uint32 packet;
    uint32 value;
    uint32 other;
    uint32 code;
    uint32 clut;
    sint32 count;

    FUNCTION_MARKER(0x8004070C, "SLUS_006.10");
    texture = td5_find_texture((const char *)psx_addr(0x800879C4, 6));
    allocation = td5_alloc_memory(r_u32(0x800875E4) * 160);
    count = (sint32)(r_u32(0x800875E4) << 2);
    w_u32(0x8008876C, allocation);
    index = 0;
    if (count > 0)
    {
        for (;;)
        {
            packet = r_u32(0x8008876C) + index * 40;
            w_u8(packet + 4, 64);
            w_u8(packet + 5, 64);
            w_u8(packet + 6, 64);
            w_u8(packet + 3, 9);
            w_u8(packet + 7, 0x2C);
            w_u8(packet + 12, r_u8(texture + 14));
            w_u8(packet + 13, r_u8(texture + 15));
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 20, other + value - 1);
            w_u8(packet + 21, r_u8(texture + 15));
            w_u8(packet + 28, r_u8(texture + 14));
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 29, other + value - 1);
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 36, other + value - 1);
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 37, other + value - 1);
            value = r_u16(texture + 8);
            code = r_u8(packet + 7);
            w_u16(packet + 22, value);
            clut = r_u16(texture + 10);
            w_u8(packet + 7, (code & 0xFE) | 2);
            count = (sint32)(r_u32(0x800875E4) << 2);
            ++index;
            w_u16(packet + 14, clut);
            if ((sint32)index >= count)
                break;
        }
    }
    allocation = td5_alloc_memory(r_u32(0x800875E4) * 208);
    count = (sint32)(r_u32(0x800875E4) << 2);
    w_u32(0x8008843C, allocation);
    result = allocation;
    index = 0;
    if (count > 0)
    {
        for (;;)
        {
            packet = r_u32(0x8008843C) + index * 52;
            w_u8(packet + 3, 12);
            w_u8(packet + 4, 0);
            w_u8(packet + 5, 0);
            w_u8(packet + 6, 0);
            w_u8(packet + 16, 0);
            w_u8(packet + 17, 0);
            w_u8(packet + 18, 0);
            w_u8(packet + 28, 32);
            w_u8(packet + 29, 32);
            w_u8(packet + 30, 32);
            w_u8(packet + 40, 32);
            w_u8(packet + 41, 32);
            w_u8(packet + 42, 32);
            w_u8(packet + 7, 0x3C);
            w_u8(packet + 12, r_u8(texture + 14));
            w_u8(packet + 13, r_u8(texture + 15));
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 24, other + value - 1);
            w_u8(packet + 25, r_u8(texture + 15));
            w_u8(packet + 36, r_u8(texture + 14));
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 37, other + value - 1);
            value = r_u8(texture + 14);
            other = r_u8(texture + 12);
            w_u8(packet + 48, other + value - 1);
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 49, other + value - 1);
            value = r_u16(texture + 8);
            code = r_u8(packet + 7);
            w_u16(packet + 26, value);
            clut = r_u16(texture + 10);
            w_u8(packet + 7, (code & 0xFE) | 2);
            count = (sint32)(r_u32(0x800875E4) << 2);
            ++index;
            result = (sint32)index < count;
            w_u16(packet + 14, clut);
            if (result == 0)
                break;
        }
    }
    return result;
}

uint32 td5_80039df8(uint32 original_stack)
{
    uint32 allocation;
    uint32 texture;
    uint32 index;
    uint32 packet;
    uint32 value;
    uint32 other;

    FUNCTION_MARKER(0x80039DF8, "SLUS_006.10");
    allocation = td5_alloc_memory(6400);
    w_u32(0x80088468, allocation);
    texture = td5_find_texture((const char *)psx_addr(0x80087924, 5));
    w_u32(0x800883D0, texture);
    if (r_u8(texture + 12) == 0)
        w_u8(texture + 12, 255);
    for (index = 0; index < 160; ++index)
    {
        packet = r_u32(0x80088468) + index * 40;
        w_u8(packet + 3, 9);
        w_u8(packet + 7, 0x2E);
        w_u8(packet + 12, r_u8(texture + 14));
        w_u8(packet + 13, r_u8(texture + 15));
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 20, other + value - 1);
        w_u8(packet + 21, r_u8(texture + 15));
        w_u8(packet + 28, r_u8(texture + 14));
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 29, other + value - 1);
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 36, other + value - 1);
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u16(packet + 8, 96);
        w_u16(packet + 10, 200);
        w_u8(packet + 37, other + value - 1);
        value = r_u8(texture + 12);
        w_u16(packet + 18, 200);
        w_u16(packet + 24, 96);
        w_u16(packet + 16, value + 95);
        w_u16(packet + 26, r_u8(texture + 13) + 199);
        w_u16(packet + 32, r_u8(texture + 12) + 95);
        w_u16(packet + 34, r_u8(texture + 13) + 199);
        w_u8(packet + 4, 255);
        w_u8(packet + 5, 255);
        w_u8(packet + 6, 0);
        w_u16(packet + 22, r_u16(texture + 8));
        w_u16(packet + 14, r_u16(texture + 10));
    }
    return td5_80039a24(0x800984A0, original_stack - 0x18);
}

const char *td5_hud_string(uint32 address)
{
    uint32 length = 0;
    while (r_u8(address + length) != 0)
        ++length;
    return (const char *)psx_addr(address, length + 1);
}

void td5_80039dec(void)
{
    FUNCTION_MARKER(0x80039DEC, "SLUS_006.10");
    w_u32(0x80088504, 0);
}

uint32 td5_80038994(const char *text, uint32 x, uint32 y, uint32 color)
{
    uint32 length;
    uint32 small;
    uint32 index;
    uint32 character;
    uint32 glyph;
    uint32 current;
    uint32 buffer;
    uint32 pool;
    uint32 packet;
    uint32 texture;
    uint32 u;
    uint32 v;
    uint32 width;
    uint32 height;
    uint32 right;

    FUNCTION_MARKER(0x80038994, "SLUS_006.10");
    length = (uint32)strlen(text);
    small = r_u32(0x8008772C);
    if ((sint32)length <= 0)
        return length;
    width = small != 0 ? 8 : 7;
    height = small != 0 ? 8 : 16;
    for (index = 0; index < length; ++index)
    {
        character = (uint8)text[index];
        glyph = (character - 32) & 255;
        if (character == 32)
        {
            x += small != 0 ? 8 : 9;
            continue;
        }
        current = r_u32(0x80088504);
        buffer = r_u32(0x80088104);
        pool = r_u32(0x80088468);
        packet = pool + ((current << 1) + buffer) * 40;
        w_u32(0x80088504, current + 1);
        w_u8(packet + 4, 255);
        w_u8(packet + 5, color != 0 ? 0 : 255);
        w_u8(packet + 6, 0);
        texture = r_u32(0x800883D0);
        right = x + width;
        w_u16(packet + 8, x);
        w_u16(packet + 10, y);
        w_u16(packet + 16, right);
        w_u16(packet + 18, y);
        w_u16(packet + 24, x);
        w_u16(packet + 26, y + height);
        w_u16(packet + 32, right);
        w_u16(packet + 34, y + height);
        if (small != 0)
        {
            u = (glyph % 21) * 8;
            v = (glyph / 21) * 8;
        }
        else
        {
            u = (glyph & 31) * 8;
            v = (glyph >> 5) * 16;
        }
        w_u8(packet + 12, r_u8(texture + 14) + u);
        w_u8(packet + 13, r_u8(r_u32(0x800883D0) + 15) + v);
        w_u8(packet + 20, r_u8(r_u32(0x800883D0) + 14) + u + width);
        w_u8(packet + 21, r_u8(r_u32(0x800883D0) + 15) + v);
        w_u8(packet + 28, r_u8(r_u32(0x800883D0) + 14) + u);
        w_u8(packet + 29, r_u8(r_u32(0x800883D0) + 15) + v + height);
        w_u8(packet + 36, r_u8(r_u32(0x800883D0) + 14) + u + width);
        w_u8(packet + 37, r_u8(r_u32(0x800883D0) + 15) + v + height);
        if (small != 0)
            x = (uint8)text[index] == ':' ? x + 4 : right;
        else
            x += 9;
    }
    return 0;
}

uint32 td5_80039a24(uint32 car, uint32 original_stack)
{
    char text[72];
    uint32 title;
    uint32 level;
    uint32 length;
    uint32 name;
    uint32 x;
    uint32 y;
    sint32 width;
    sint32 height;
    sint32 lap;
    sint32 laps;
    sint32 total;

    FUNCTION_MARKER(0x80039A24, "SLUS_006.10");
    td5_80039dec();
    if (r_u32(0x80087738) != 0)
    {
        if (r_u32(0x80087650) != 0)
            td5_80038994((const char *)psx_addr(0x80087914, 5), 32, 200, 0);
        else if (r_u32(0x80087664) == 0)
            td5_80038994((const char *)psx_addr(0x8008791C, 7), 32, 200, 0);
        if (r_s32(0x800885E8) >= 60)
            return 0;
        level = r_u32(0x80087710);
        title = r_u32(0x8007DC60 + level * 4);
        length = (uint32)strlen(td5_hud_string(title));
        level = r_u32(0x80087710);
        width = r_s32(0x800880F4);
        height = r_s32(0x800880F8);
        title = r_u32(0x8007DC60 + level * 4);
        x = (uint32)(width / 2) - ((length >> 1) << 3);
        y = (uint32)(height / 2);
        return td5_80038994(td5_hud_string(title), x, y, 0);
    }
    if (r_u32(0x8008772C) != 0)
    {
        if (r_u32(0x80087734) != 0)
            return td5_800396dc(car);
        if (r_u32(0x8008771C) == 3)
            return td5_800394c8(car);
        lap = r_u8(car + 854) + 1;
        laps = r_s32(0x800876F4);
        if (laps >= lap)
            laps = lap;
        total = r_s32(0x800876F4);
        sprintf(text, (const char *)psx_addr(0x80010B84, 10), laps, total);
        x = r_u32(0x80088510);
        y = r_u32(0x80088518);
        td5_80038994(text, x + 16, y + 16, 0);
        name = r_u8(car + 859);
        x = r_u32(0x80088510);
        y = r_u32(0x80088518);
        name = r_u32(0x8007E0F0 + name * 4);
        return td5_80038994(td5_hud_string(name), x + 16, y + 28, 0);
    }
    if (r_u32(0x80087730) != 0)
    {
        if (r_u32(0x80087734) != 0)
            return td5_800396dc(car);
        lap = r_u8(car + 854) + 1;
        laps = r_s32(0x800876F4);
        if (laps >= lap)
            laps = lap;
        total = r_s32(0x800876F4);
        sprintf(text, (const char *)psx_addr(0x80010B84, 10), laps, total);
        td5_80038994(text, 32, 12, 0);
    }
    else
    {
        if (r_u32(0x80087720) == 1)
            return td5_80039208(car);
        if (r_u32(0x80087728) == 4)
        {
            sprintf(text, (const char *)psx_addr(0x80010B78, 10), r_s32(car + 672));
            td5_80038994(text, 32, 12, 0);
            name = r_u32(0x8007E0F0 + r_u8(car + 859) * 4);
            return td5_80038994(td5_hud_string(name), 32, 32, 0);
        }
        if (r_u32(0x80087724) != 0)
        {
            sprintf(text, (const char *)psx_addr(0x80010B6C, 11), r_s16(0x800883A0));
            td5_80038994(text, 32, 12, 0);
            sprintf(text, (const char *)psx_addr(0x80010B78, 10), r_s32(car + 672));
            td5_80038994(text, 32, 28, 0);
            return td5_8003a028(original_stack - 0x60);
        }
    }
    return td5_80038d5c(car);
}

uint32 td5_80038d5c(uint32 car)
{
    char text[64];
    uint32 index;
    uint32 ticks;
    uint32 name;
    sint32 rate;
    sint32 time;
    sint32 remainder;

    FUNCTION_MARKER(0x80038D5C, "SLUS_006.10");
    ticks = r_u16(car + 804);
    rate = r_s32(0x800880FC);
    time = td5_divide_checked((sint32)(ticks * 100), rate / 2);
    remainder = time % 6000;
    sprintf(text, (const char *)psx_addr(0x80010B18, 15), time / 6000, remainder / 100, remainder % 100);
    td5_80038994(text, 400, 32, 0);
    td5_80038994((const char *)psx_addr(0x80087904, 5), 356, 32, 0);
    for (index = 0; index < 5; ++index)
    {
        ticks = r_u16(car + 806 + index * 2);
        if (ticks != 0)
        {
            rate = r_s32(0x800880FC);
            time = td5_divide_checked((sint32)(ticks * 100), rate / 2);
            remainder = time % 6000;
            sprintf(text, (const char *)psx_addr(0x80010B18, 15), time / 6000, remainder / 100, remainder % 100);
            td5_80038994(text, 400, 49 + index * 13, 0);
        }
    }
    name = r_u32(0x8007E0F0 + r_u8(car + 859) * 4);
    return td5_80038994(td5_hud_string(name), 32, 32, 0);
}

uint32 td5_80049364(uint32 archive_error_index)
{
    char filename[72];
    uint32 result;
    uint32 flag;
    uint32 prefix;
    uint32 level;
    uint32 size;
    uint32 buffer;
    uint32 index;
    uint32 entry;
    uint32 value;
    uint32 count;
    uint32 offset;
    uint32 numerator;
    sint32 formatted;

    FUNCTION_MARKER(0x80049364, "SLUS_006.10");
    result = r_u32(0x80087730);
    if (result == 0)
        return result;
    flag = r_u32(0x8008772C);
    w_u32(0x80087A74, 47);
    if (flag != 0)
        w_u32(0x80087A74, 35);
    td5_debug_log(0x80010FDC);
    prefix = r_u32(0x800885C4);
    level = r_u32(0x80087710);
    w_u32(0x80087730, 1);
    formatted = sprintf(filename, (const char *)psx_addr(0x80010FF4, 14), td5_hud_string(prefix), (sint32)(level + 1));
    size = td5_archive_aligned_size(filename, 1, archive_error_index, (uint32)formatted);
    buffer = td5_alloc_memory(size);
    w_u32(0x800886E8, buffer);
    w_u32(0x80088400, buffer);
    td5_load_archive_file(filename, buffer, archive_error_index);
    buffer = r_u32(0x800886E8);
    if (r_u32(buffer) != 0)
    {
        index = 1;
        entry = buffer + 4;
        for (;;)
        {
            value = r_u32(entry);
            w_u32(entry, value + buffer);
            count = r_u32(buffer);
            ++index;
            entry += 4;
            if (count < index)
                break;
        }
    }
    size = r_u32(0x80088428) * 24;
    memset(psx_addr(0x800A4D08, size), 0, size);
    numerator = r_u32(0x8008841C) + 11;
    count = r_u32(0x80087A74) + 1;
    value = (uint32)((sint32)numerator / 12);
    w_u32(0x800886CC, value);
    result = (uint32)((sint32)count / 6) + 1;
    w_u32(0x80088428, result);
    if ((sint32)result > 0)
    {
        index = 0;
        offset = 0;
        for (;;)
        {
            w_u8(0x800A4D10 + offset, 0);
            w_u8(0x800A4D1C + offset, 0);
            count = r_u32(0x80088428);
            w_u32(0x800A4D08 + offset, 0);
            w_u32(0x800A4D14 + offset, 0);
            w_u16(0x800A4D0E + offset, 0xFFFF);
            w_u16(0x800A4D1A + offset, 0xFFFF);
            ++index;
            result = (sint32)index < (sint32)count;
            offset += 24;
            if (result == 0)
                break;
        }
    }
    return result;
}

uint32 td5_80047468(void)
{
    uint32 texture;

    FUNCTION_MARKER(0x80047468, "SLUS_006.10");
    w_u32(0x800886E0, 160);
    w_u32(0x800886E4, 160);
    w_u32(0x800884D0, 0);
    w_u32(0x800884D4, 0);
    w_u32(0x800883E8, 0);
    w_u32(0x800883EC, 0);
    texture = td5_find_texture((const char *)psx_addr(0x80087A68, 8));
    w_u32(0x80088534, texture);
    w_u32(0x80087A18, 0);
    return texture;
}

uint32 td5_8001f844(void)
{
    uint32 level;
    uint32 records;
    uint32 sentinel;
    uint32 count;
    uint32 node;
    uint32 base;
    uint32 offset;
    uint32 track;
    uint32 vertex;
    uint32 value;
    uint32 other;
    sint32 random;

    FUNCTION_MARKER(0x8001F844, "SLUS_006.10");
    level = r_u32(0x80088770);
    w_u32(0x80087798, 0);
    w_u32(0x8008779C, 0);
    w_u32(0x800884A0, 0xFFFFFFFF);
    w_u32(0x800884A4, 0xFFFFFFFF);
    records = r_u32(level + 40);
    sentinel = r_u16(records);
    w_u32(0x80088788, 0);
    w_u32(0x800886EC, records);
    while (sentinel != 0xFFFF)
    {
        count = r_u32(0x80088788) + 1;
        sentinel = r_u16(records + count * 16);
        w_u32(0x80088788, count);
    }
    records = r_u32(0x800886EC);
    node = r_u16(records + 6);
    base = r_u32(0x800884C4);
    offset = r_u16(records + 8);
    track = base + node * 24;
    w_u32(0x80088474, offset);
    w_u32(0x80088470, offset);
    w_u32(0x80088724, node);
    w_u32(0x80088720, node);
    vertex = r_u16(track + 4);
    base = r_u32(0x8008850C);
    value = (uint32)r_s16(base + (vertex + offset) * 6);
    other = r_u32(track + 12);
    value = (value + other) << 8;
    w_u32(0x8008838C, value);
    w_u32(0x80088388, value);
    vertex = r_u16(track + 4);
    value = (uint32)r_s16(base + (vertex + offset) * 6 + 4);
    other = r_u32(track + 20);
    value = (value + other) << 8;
    w_u32(0x8008839C, value);
    w_u32(0x80088398, value);
    value = r_u16(records + 10);
    w_u32(0x8008865C, 0);
    w_u32(0x80088658, 0);
    w_u32(0x80088394, value);
    w_u32(0x80088390, value);
    random = randPSX();
    w_u32(0x80087788, (uint32)(random % 10000 + 10000));
    random = randPSX();
    w_u32(0x80088560, 8);
    w_u32(0x80088564, 8);
    w_u32(0x80088748, 0);
    w_u32(0x8008874C, 0);
    count = r_u32(0x80088788);
    w_u32(0x8008778C, (uint32)(random % 10000 + 10000));
    if ((sint32)count < 2 || r_s32(0x80087710) >= 24)
    {
        w_u32(0x80088658, 4);
        w_u32(0x8008865C, 4);
    }
    return 4;
}

uint32 td5_80013d20(void)
{
    uint32 index;
    uint32 value;
    uint32 result;
    sint32 node;
    sint32 lane;

    FUNCTION_MARKER(0x80013D20, "SLUS_006.10");
    w_u32(0x80088498, 0);
    value = r_u32(0x80088498);
    w_u32(0x800875F0, value);
    w_u32(0x800884E4, 1);
    for (index = 0; (sint32)index < r_s32(0x800875E4); ++index)
        td5_8001affc(0x800984A0 + index * 864);
    value = (uint32)r_s16(0x80098512);
    w_u32(0x80088170, value);
    for (;;)
    {
        result = td5_80035bd4();
        if (result != 0)
            return result;
        for (index = 0; (sint32)index < r_s32(0x800875E4); ++index)
            td5_8001affc(0x800984A0 + index * 864);
        value = r_u32(0x800875F0);
        node = r_s16(0x80098512 + value * 864);
        value = r_u32(0x800875F0);
        lane = r_s16(0x80098510 + value * 864);
        td5_800357e4(node, lane);
        DrawSync(0);
        td5_8005ae14();
        td5_800341fc();
    }
}

uint32 td5_8006b9e0(uint32 slot, uint32 first, uint32 second)
{
    uint32 entry = 0x80088688 + slot * 4;

    FUNCTION_MARKER(0x8006B9E0, "SLUS_006.10");
    if (first != 0xFFFFFFFF)
        w_u8(entry, first);
    if (second != 0xFFFFFFFF)
        w_u8(entry + 1, second);
    return 0xFFFFFFFF;
}

TD5_STACK_EXCHANGE_RESULT td5_80068510(uint32 replacement_stack, uint32 current_stack)
{
    TD5_STACK_EXCHANGE_RESULT result;

    FUNCTION_MARKER(0x80068510, "SLUS_006.10");
    result.previous_stack = current_stack;
    result.replacement_stack = replacement_stack;
    return result;
}

uint32 td5_80015be8(uint32 original_stack, uint32 texture_v_carrier, uint32 audio_angle_carrier)
{
    TD5_STACK_EXCHANGE_RESULT stack;
    uint32 result;

    FUNCTION_MARKER(0x80015BE8, "SLUS_006.10");
    stack = td5_80068510(0x1F800400, original_stack - 0x18);
    w_u32(0x80088714, stack.previous_stack);
    w_u32(0x8008858C, 3);
    if (r_u32(0x80087730) == 0)
        td5_80013d20();
    td5_80013f90(stack.replacement_stack, texture_v_carrier, audio_angle_carrier);
    stack = td5_80068510(r_u32(0x80088714), stack.replacement_stack);
    td5_8006b9e0(0, 0, 0);
    td5_8006b9e0(1, 0, 0);
    if (r_u32(0x80087650) != 0)
    {
        w_u32(0x80088620, 80);
        w_u32(0x8008858C, 0);
        w_u32(0x80087650, 0);
        return 80;
    }
    result = r_u32(0x80088418);
    if (result != 0)
        return result;
    result = r_u32(0x80088530);
    if (result != 0)
        return result;
    result = r_u32(0x80087720);
    if (result != 2)
        return td5_800164e0(r_s32(0x80087710));
    return result;
}

uint32 td5_80013f90(uint32 original_stack, uint32 texture_v_carrier, uint32 audio_angle_carrier)
{
    uint32 index;
    uint32 value;
    uint32 level;
    uint32 header;
    uint32 result;
    uint32 previous_sync;
    uint32 frames = 0;
    uint32 absent_frames = 0;
    uint32 elapsed = 0;
    uint32 color = original_stack - 0x40 + 0x28;
    sint32 node;
    sint32 lane;
    sint32 position;

    FUNCTION_MARKER(0x80013F90, "SLUS_006.10");
    w_u32(0x8008866C, 0);
    w_u32(0x80088544, 0);
    td5_80013cd8();
    w_u32(0x80088530, 0);
    if (r_u32(0x80088460) == 0)
        w_u32(0x80088498, 0);
    w_u32(0x80088498, 0);
    value = r_u32(0x80088498);
    w_u32(0x800875F0, value);
    w_u32(0x8008805C, 0);
    w_u32(0x800875E8, 0);
    w_u32(0x800885E0, 0);
    w_u32(0x800875D8, 0);
    w_u32(0x800885E8, 0);
    w_u32(0x800875D4, 0);
    w_u32(0x8008869C, 1);
    w_u32(0x800884E4, 0);
    w_u32(0x800885CC, 0);
    value = r_u32(0x800875F0);
    td5_800542ec(0x80098664 + value * 864);
    value = r_u32(0x800875F0);
    td5_800542e0(0x80098634 + value * 864);
    previous_sync = (uint32)VSync(-1);
    level = r_u32(0x80088770);
    header = r_u32(level);
    if ((r_u8(header + 30) & 1) != 0)
    {
        value = r_u32(0x80087604);
        w_u32(color, value);
        level = r_u32(0x80088770);
        header = r_u32(level);
        td5_8006e2f4(5200, header + 26, color, 650);
        td5_8006e2e4(-300, 5000);
    }
    td5_80026130();
    value = r_u32(0x800880F4);
    w_u32(0x80088438, value);
    value = r_u32(0x800880F8);
    w_u32(0x800884F8, value);
    w_u32(0x80088510, 0);
    w_u32(0x80088518, 0);
    while (r_u32(0x800885E0) == 0)
    {
        if (r_u32(0x80087650) != 0)
            w_u32(0x800875E8, 3);
        w_u32(0x800885F8, 0);
        if (r_u32(0x80087650) != 0)
        {
            value = r_u32(0x800885CC);
            w_u32(0x800885CC, value + 1);
            if (r_s32(0x800885CC) >= 901)
                w_u32(0x800885E0, 1);
            if (r_s16(0x800885D0) != 0)
                w_u32(0x800885E0, 1);
            if (r_s16(0x800885D2) != 0)
                w_u32(0x800885E0, 1);
        }
        td5_poll_pads();
        td5_80026590(0);
        if (r_u32(0x80087650) == 0)
        {
            if ((r_u32(0x80088450) & 255) != 255 || r_u32(0x80087650) != 0)
                absent_frames = 0;
            else
            {
                ++absent_frames;
                if ((sint32)absent_frames >= 31)
                {
                    absent_frames = 0;
                    w_u32(0x8008805C, 1);
                }
            }
        }
        td5_swap_display_buffer();
        td5_clear_ordering_table();
        td5_80072140();
        level = r_u32(0x80088770);
        header = r_u32(level);
        if ((r_u8(header + 30) & 1) != 0)
        {
            position = r_s32(0x80098790) / 256;
            td5_800720b0(position, 1500, 1500);
        }
        else
        {
            position = r_s32(0x80098790) / 256;
            td5_800720b0(position, 2000, 3000);
        }
        if (r_u32(0x800875E8) == 0)
        {
            td5_800269dc(0, 0);
            td5_80024c14(0, original_stack - 0x118, texture_v_carrier);
        }
        td5_800419f4();
        for (index = 0; (sint32)index < r_s32(0x800875E4); ++index)
            td5_8001affc(0x800984A0 + index * 864);
        if (r_u32(0x80087724) != 0)
            td5_800439c8();
        level = r_u32(0x80088770);
        header = r_u32(level);
        if ((r_u8(header + 30) & 4) != 0)
            td5_8004ffe4(original_stack - 0x40, texture_v_carrier);
        td5_80053348(original_stack - 0x40);
        td5_80054c58(original_stack - 0x40, audio_angle_carrier, texture_v_carrier, original_stack - 0x40);
        td5_8002c30c(original_stack - 0x40);
        td5_80026130();
        value = r_u32(0x800875F0);
        td5_8001ebbc(0x800984A0 + value * 864, 0, original_stack - 0x40);
        value = r_u32(0x800875F0);
        td5_800388a0(0x800984A0 + value * 864, original_stack - 0x40);
        td5_800415d0(0, 0);
        if (r_u8(0x800987F4) != 0 || r_u32(0x80087738) != 0 || r_u32(0x80087650) != 0)
            td5_8003a1b4(-1);
        else
            td5_8003a1b4(0);
        if (r_u32(0x80087734) != 0)
            td5_8003a3e4();
        td5_80047790();
        td5_8003a020();
        td5_8003bb34(original_stack - 0x40);
        td5_8003b9cc(original_stack - 0x40);
        td5_80043db8();
        td5_80044604();
        value = r_u32(0x800875F0);
        td5_800464cc(0x800984A0 + value * 864);
        if (r_u32(0x80087730) != 0)
        {
            value = r_u32(0x800875F0);
            node = r_s16(0x80098512 + value * 864);
            td5_80049638(0, node);
        }
        else
        {
            value = r_u32(0x800875F0);
            node = r_s16(0x80098512 + value * 864);
            value = r_u32(0x800875F0);
            lane = r_s16(0x80098510 + value * 864);
            td5_800357e4(node, lane);
        }
        if (r_u32(0x80087730) != 0)
            td5_80049bc4(0, original_stack - 0x40);
        else
            td5_80035390(original_stack - 0x40);
        if (r_u32(0x80088618) == 0)
        {
            value = r_u32(0x800875D8);
            if (value != 0xFFFFFFFF)
            {
                value = r_u32(0x800875D8);
                w_u32(0x800875D8, value + 1);
            }
        }
        value = r_u32(0x800885E8);
        w_u32(0x800885E8, value + 1);
        td5_8002ecf4(original_stack - 0x40, texture_v_carrier, original_stack - 0x40);
        DrawSync(0);
        if (r_u32(0x80087730) == 0)
            td5_800341fc();
        td5_8005ae14();
        if (r_u32(0x80088498) == 0 || r_u32(0x80088460) == 0)
        {
            while ((sint32)((uint32)VSync(-1) - previous_sync) < 2)
            {
            }
            previous_sync = (uint32)VSync(-1);
        }
        td5_reset_timer(1);
        if (r_u32(0x80087730) != 0)
            td5_800557f0(original_stack - 0x40);
        td5_apply_display_buffer();
        td5_draw_ordering_table();
        ++frames;
        if ((sint32)elapsed <= 65534)
            ++elapsed;
        if ((sint32)elapsed >= 9)
            td5_8005a3d0();
        if (elapsed == 2)
            td5_80052d80();
    }
    DrawSync(0);
    VSync(0);
    td5_80054260();
    result = r_u32(0x80087720);
    if (result == 2)
    {
        result = r_u32(0x80088530);
        if (result == 1)
        {
            result = r_u32(0x80087738);
            if (result == 0)
            {
                result = 170;
                w_u32(0x80088620, result);
                w_u32(0x8008858C, 0);
                w_u32(0x80088420, 0);
            }
        }
    }
    return result;
}
