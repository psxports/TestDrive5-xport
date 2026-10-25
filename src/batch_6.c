#include "td5_functions.h"
#include <string.h>
#include <stdlib.h>

void *td5_800689c0(const char *pattern, void *record)
{
    FUNCTION_MARKER(0x800689C0, "SLUS_006.10");
    return firstfile(pattern, record);
}

uint32 td5_8004af60(uint32 input)
{
    uint32 base;
    uint32 cursor;
    uint32 word;
    uint32 active;
    uint32 count;
    uint32 result;

    FUNCTION_MARKER(0x8004AF60, "SLUS_006.10");
    base = r_u32(0x800882B8);
    cursor = r_u32(0x800882BC);
    if (cursor == base + 0x1BFE)
        td5_debug_log(0x800110D0);
    else
    {
        word = r_u16(cursor);
        active = r_u32(0x80088718);
        count = word & 7;
        if (active != 0 && (input & 0xFFFF) != (word & 0xFFF8))
        {
            w_u32(0x800882BC, cursor + 2);
            w_u16(cursor + 2, input + 1);
        }
        else if (count == 7)
        {
            w_u16(cursor, input + 7);
            w_u32(0x800882BC, cursor + 2);
            w_u16(cursor + 2, input + 1);
        }
        else
            w_u16(cursor, input + count + 1);
    }
    result = r_u32(0x800882BC);
    w_u32(0x800882C4, result);
    return result;
}

sint32 td5_8004adf8(uint32 input)
{
    uint32 mode;
    uint32 base;
    uint32 cursor;
    uint32 word;
    uint32 count;
    uint32 value;

    FUNCTION_MARKER(0x8004ADF8, "SLUS_006.10");
    mode = r_u32(0x80088418);
    value = input;
    if (mode == 0xFFFFFFFF)
        return (sint16)value;
    if (mode == 0)
        td5_8004af60(input & 0xFFFF);
    else
    {
        base = r_u32(0x800882B8);
        cursor = r_u32(0x800882BC);
        if (cursor == base + 0x1BFE || cursor >= r_u32(0x800882C4))
        {
            value = 0;
            td5_debug_log(0x800110C0);
            w_u32(0x800885E0, 1);
        }
        else
        {
            if (r_u32(0x8008851C) == 0)
            {
                word = r_u16(cursor + 2);
                w_u32(0x800882BC, cursor + 2);
                w_u32(0x8008851C, word & 7);
            }
            cursor = r_u32(0x800882BC);
            count = r_u32(0x8008851C);
            word = r_u16(cursor);
            w_u32(0x8008851C, count - 1);
            value = word & 0xFFF8;
        }
    }
    if (r_u32(0x80088718) == 0)
        w_u32(0x80088718, 1);
    return (sint16)value;
}

uint32 td5_80026130(void)
{
    uint32 swaps;
    uint32 index;
    uint32 left;
    uint32 right;
    uint32 offset;
    uint32 first;
    uint32 second;
    sint32 left_position;
    sint32 right_position;

    FUNCTION_MARKER(0x80026130, "SLUS_006.10");
    do
    {
        swaps = 0;
        for (index = 1; index < 6; ++index)
        {
            left = r_u8(0x800883EF + index);
            offset = left * 864;
            if (r_u32(0x800987A0 + offset) == 0)
            {
                right = r_u8(0x800883F0 + index);
                left_position = r_s16(0x80098516 + offset);
                right_position = r_s16(0x80098516 + right * 864);
                if (left_position < right_position)
                {
                    ++swaps;
                    w_u8(0x800883EF + index, right);
                    w_u8(0x800883F0 + index, left);
                }
            }
        }
    } while (swaps != 0);
    if (r_u32(0x80087734) != 0)
    {
        first = r_u32(0x800987A0);
        if (first != 0)
        {
            second = r_u32(0x80098B00);
            if (second != 0)
            {
                first = (first << 8) - (uint32)r_s16(0x800987AE);
                second = (second << 8) - (uint32)r_s16(0x80098B0E);
                if (first < second)
                {
                    w_u8(0x800883F0, 0);
                    w_u8(0x800883F1, 1);
                    w_u8(0x800987FB, 0);
                    w_u8(0x80098B5B, 1);
                }
                else
                {
                    w_u8(0x800883F0, 1);
                    w_u8(0x800883F1, 0);
                    w_u8(0x800987FB, 1);
                    w_u8(0x80098B5B, 0);
                }
                return 1;
            }
        }
    }
    for (index = 0; index < 6; ++index)
    {
        offset = r_u8(0x800883F0 + index) * 864;
        w_u8(0x800987FB + offset, index);
    }
    return 0;
}

uint32 td5_80026590(uint32 slot)
{
    uint32 half_offset;
    uint32 word_offset;
    uint32 output;
    uint32 input;
    uint32 mode;
    uint32 packed;
    uint32 mask;
    uint32 edges;
    uint32 result;
    sint32 center;
    sint32 raw;
    sint32 dead_zone;
    sint32 delta;
    sint32 limit;
    sint32 numerator;
    sint32 divisor;
    sint32 lower;
    sint32 upper;

    FUNCTION_MARKER(0x80026590, "SLUS_006.10");
    half_offset = slot * 2;
    word_offset = slot * 4;
    output = 0x800885DC + half_offset;
    input = 0x800885D0 + half_offset;
    mode = r_u32(0x80088450 + word_offset);
    if (mode == 0x2000 || mode == 0x5000 || mode == 0x7000)
    {
        center = r_s16(0x80087808 + half_offset);
        raw = r_s32(0x800886A0 + word_offset);
        dead_zone = r_s16(0x80087810 + half_offset);
        w_u16(output, 0x8000);
        delta = (sint32)((uint32)center - (uint32)raw);
        if (delta < -dead_zone)
        {
            limit = -r_s16(0x8008780C + half_offset);
            delta = (sint32)((uint32)delta + (uint32)dead_zone);
            if (delta < limit)
                delta = limit;
        }
        else if (dead_zone < delta)
        {
            delta = (sint32)((uint32)delta - (uint32)dead_zone);
            limit = r_s16(0x8008780C + half_offset);
            if (limit < delta)
                delta = limit;
        }
        else
            delta = 0;
        divisor = r_s16(0x8008780C + half_offset);
        numerator = (sint32)(((uint32)delta << 4) - (uint32)delta);
        packed = ((uint32)td5_divide_checked(numerator, divisor) + 16) << 3;
        packed |= r_u16(output);
        w_u16(output, packed);
        upper = r_s16(0x80087818 + half_offset);
        lower = r_s16(0x80087814 + half_offset);
        raw = r_s32(0x800886B0 + word_offset);
        if ((upper - lower) / 2 < raw)
            w_u16(output, packed | 0x100);
        else
        {
            raw = r_s32(0x800886A8 + word_offset);
            lower = r_s16(0x8008781C + half_offset);
            if (raw < lower)
                raw = lower;
            upper = r_s16(0x80087820 + half_offset);
            if (upper < raw)
                raw = upper;
            delta = raw - lower;
            numerator = (sint32)(((uint32)delta << 3) - (uint32)delta);
            divisor = upper - lower;
            result = (uint32)td5_divide_checked(numerator, divisor);
            w_u16(output, packed | (result << 9));
        }
    }
    else if (mode == 0x4000)
    {
        w_u16(output, 0);
        mask = r_u16(input);
        mask &= r_u16(0x800877EC + half_offset);
        if (mask != 0)
            w_u16(output, 8);
        mask = r_u16(input);
        mask &= r_u16(0x800877F0 + half_offset);
        if (mask != 0)
            w_u16(output, r_u16(output) | 0x10);
        mask = r_u16(input);
        mask &= r_u16(0x800877E0 + half_offset);
        if (mask != 0)
            w_u16(output, r_u16(output) | 0x20);
        mask = r_u16(input);
        mask &= r_u16(0x800877E4 + half_offset);
        if (mask != 0)
            w_u16(output, r_u16(output) | 0x40);
    }
    mask = r_u16(input);
    mask &= r_u16(0x800877E8 + half_offset);
    if (mask != 0)
        w_u16(output, r_u16(output) | 0x1000);
    edges = r_u16(0x800884E8 + half_offset);
    mask = r_u16(0x800877F4 + half_offset);
    if ((edges & mask) != 0)
        result = r_u16(output) | 0x2000;
    else
    {
        mask = r_u16(0x800877F8 + half_offset);
        if ((edges & mask) != 0)
            result = r_u16(output) | 0x4000;
        else
        {
            if (r_u8(0x8007EA01) != 0 || r_u32(0x80087724) != 0)
            {
                result = r_u16(input);
                result &= r_u16(0x800877FC + half_offset);
            }
            else
                result = edges & r_u16(0x800877FC + half_offset);
            if (result == 0)
                return 0;
            result = r_u16(output) | 0x6000;
        }
    }
    w_u16(output, result);
    return result;
}

uint32 td5_80072140(void)
{
    uint32 value;

    FUNCTION_MARKER(0x80072140, "SLUS_006.10");
    value = r_u32(0x800886D8);
    w_u32(0x80088514, 0);
    w_u32(0x800884FC, value);
    return value;
}

sint32 td5_800720b0(sint32 position, sint32 near_distance, sint32 far_distance)
{
    uint32 magnitude;
    sint32 candidate;
    sint32 result;

    FUNCTION_MARKER(0x800720B0, "SLUS_006.10");
    magnitude = (uint32)position;
    if (position < 0)
        magnitude = 0 - magnitude;
    candidate = (sint32)((uint32)far_distance - ((magnitude << 3) - 500));
    if (candidate < far_distance)
        far_distance = candidate;
    if (near_distance < far_distance)
        near_distance = far_distance;
    w_u32(0x80088134, (uint32)(near_distance >> 1));
    result = near_distance >> 2;
    w_u32(0x80088138, (uint32)result);
    return result;
}

static uint32 td5_carousel_text_half_width(uint32 font, const char *text)
{
    uint32 length;
    uint32 index;
    uint32 character;
    uint32 line;
    uint32 maximum;

    length = (uint32)strlen(text);
    line = 0;
    maximum = 0;
    if ((sint32)length > 0)
    {
        for (index = 0; index < length; ++index)
        {
            character = (uint8)text[index];
            if (character == 10)
            {
                if ((sint32)maximum < (sint32)line)
                    maximum = line;
                line = 0;
            }
            else if (character == 32)
                line += r_u8(font + 12);
            else
                line += 1 + r_u8(font + 12 + ((character - 32) & 255));
        }
    }
    if ((sint32)maximum < (sint32)line)
        maximum = line;
    return (uint32)((sint32)maximum / 2);
}

uint32 td5_800b2790(uint32 mode, uint32 x, sint32 y, const char *text)
{
    uint32 font;
    uint32 initial_x;
    uint32 cursor_x;
    uint32 cursor_y;
    uint32 length;
    uint32 index;
    uint32 character;
    uint32 glyph;
    uint32 glyph_width;
    uint32 counter;
    uint32 base;
    uint32 packet;
    uint32 vertex;
    uint32 component;
    uint32 columns;
    uint32 quotient;
    uint32 cell;
    uint32 product;
    uint32 texture;
    uint32 value;
    uint32 adjustment;
    uint32 display;
    uint32 head;
    uint32 tag;
    uint32 head_tag;
    uint32 count;

    FUNCTION_MARKER(0x800B2790, "SLUS_006.10");
    font = 0x800BBDF8 + (mode & 15) * 140;
    if ((mode & 0x80) != 0)
        x -= td5_carousel_text_half_width(font, text);
    initial_x = x;
    cursor_x = x;
    cursor_y = (uint32)y;
    length = (uint32)strlen(text);
    if ((sint32)length <= 0)
        return length;
    for (index = 0; index < length; ++index)
    {
        character = (uint8)text[index];
        if (character == 10)
        {
            value = r_u8(font + 5);
            cursor_x = initial_x;
            cursor_y += value;
            continue;
        }
        if (character == 13)
        {
            value = td5_carousel_text_half_width(font, text + index + 1);
            cursor_x = r_u32(0x800BF0F0) - (value << 1);
            continue;
        }
        if (character == 125)
        {
            value = td5_carousel_text_half_width(font, text + index + 1);
            cursor_x = initial_x - (value << 1);
            continue;
        }
        if (character == 9)
        {
            cursor_x = r_u32(0x800BF0F0);
            continue;
        }
        glyph = (character - 32) & 255;
        if (character == 32)
        {
            cursor_x += r_u8(font + 12 + glyph);
            continue;
        }
        counter = r_u32(0x800BF398);
        base = r_u32(0x800BF108);
        glyph_width = r_u8(font + 12 + glyph);
        packet = base + counter * 52;
        w_u16(packet + 8, cursor_x);
        w_u16(packet + 10, cursor_y);
        adjustment = r_u8(font + 7);
        w_u16(packet + 22, cursor_y);
        w_u16(packet + 32, cursor_x);
        w_u16(packet + 20, cursor_x + adjustment + glyph_width);
        value = r_u8(font + 5);
        w_u16(packet + 34, cursor_y + value);
        adjustment = r_u8(font + 7);
        w_u16(packet + 44, cursor_x + adjustment + glyph_width);
        value = r_u8(font + 5);
        w_u16(packet + 46, cursor_y + value);
        for (vertex = 0; vertex < 4; ++vertex)
        {
            columns = r_u8(font + 6);
            quotient = (uint32)td5_divide_checked((sint32)glyph, (sint32)columns);
            cell = glyph - quotient * columns;
            value = r_u8(font + 4);
            product = value * cell;
            texture = r_u32(font + 8);
            value = r_u8(texture + 14);
            if ((vertex & 1) != 0)
            {
                adjustment = r_u8(font + 7);
                value += product + adjustment + glyph_width;
            }
            else
                value += product;
            w_u8(packet + 12 + vertex * 12, value);
            columns = r_u8(font + 6);
            quotient = (uint32)td5_divide_checked((sint32)glyph, (sint32)columns);
            cell = r_u8(font + 5);
            product = cell * quotient;
            texture = r_u32(font + 8);
            value = r_u8(texture + 15);
            value += product;
            if ((vertex & 2) != 0)
                value += cell;
            w_u8(packet + 13 + vertex * 12, value);
        }
        for (vertex = 0; vertex < 4; ++vertex)
        {
            for (component = 0; component < 3; ++component)
            {
                value = r_u8(0x800BCCB0 + vertex * 4 + component);
                w_u8(packet + 4 + vertex * 12 + component, value);
            }
        }
        texture = r_u32(font + 8);
        value = r_u16(texture + 8);
        w_u16(packet + 26, value);
        texture = r_u32(font + 8);
        display = r_u32(0x800880F0);
        value = r_u16(texture + 10);
        w_u16(packet + 14, value);
        head = r_u32(display + 0x70);
        tag = r_u32(packet);
        head_tag = r_u32(head);
        w_u32(packet, (tag & 0xFF000000) | (head_tag & 0xFFFFFF));
        head = r_u32(display + 0x70);
        counter = r_u32(0x800BF398);
        count = r_u32(0x800BF410);
        cursor_x += 1 + glyph_width;
        head_tag = r_u32(head);
        w_u32(0x800BF398, counter + 1);
        w_u32(0x800BF410, count + 1);
        w_u32(head, (head_tag & 0xFF000000) | (packet & 0xFFFFFF));
    }
    return 0;
}

uint32 td5_800b32e4(uint32 palette)
{
    uint32 source;
    uint32 index;
    uint32 words[3];
    uint32 value;

    FUNCTION_MARKER(0x800B32E4, "SLUS_006.10");
    source = 0x800BBD78 + (palette << 4);
    /* Aligned LWL/LWR and SWL/SWR pairs each access the whole word twice */
    for (index = 0; index < 3; ++index)
    {
        (void)r_u32(source + index * 4);
        words[index] = r_u32(source + index * 4);
    }
    for (index = 0; index < 3; ++index)
    {
        w_u32(0x800BCCB0 + index * 4, words[index]);
        w_u32(0x800BCCB0 + index * 4, words[index]);
    }
    (void)r_u32(source + 12);
    value = r_u32(source + 12);
    w_u32(0x800BCCBC, value);
    w_u32(0x800BCCBC, value);
    return value;
}

uint32 td5_800b73ec(void)
{
    uint32 index;
    uint32 object;
    uint32 kind;
    uint32 packet;
    uint32 vertex;
    uint32 component;
    uint32 value;
    uint32 texture;
    uint32 centered;
    uint32 size;
    uint32 coordinate;
    uint32 font;
    uint32 x;
    sint32 y;
    uint32 text;
    uint32 text_length;
    uint32 display;
    uint32 head;
    uint32 tag;
    uint32 head_tag;

    FUNCTION_MARKER(0x800B73EC, "SLUS_006.10");
    for (index = 0; index < 70; ++index)
    {
        object = 0x800BCF08 + index * 120;
        if (r_u8(object) == 0)
            continue;
        if ((r_u16(object + 8) & 0x4000) != 0)
            continue;
        kind = r_u16(object + 2) & 0xFF00;
        if (kind != 0 && kind != 0x8000)
            continue;
        if (r_u32(0x800887B8) == index)
            td5_800b32e4(1);
        else
        {
            td5_800b32e4(r_u8(object + 1));
            if ((r_u16(object + 8) & 0x8000) != 0)
                td5_800b32e4(7);
        }
        if (kind == 0)
        {
            font = r_u8(object + 2);
            x = r_u16(object + 8);
            y = r_s16(object + 10);
            text = r_u32(object + 4);
            text_length = 0;
            do
            {
                value = r_u8(text + text_length);
                ++text_length;
            } while (value != 0);
            td5_800b2790(font, x & 0xFFF, y, (const char *)psx_addr(text, text_length));
            continue;
        }
        value = r_u32(0x80088104);
        packet = object + value * 52 + 16;
        for (vertex = 0; vertex < 3; ++vertex)
        {
            for (component = 0; component < 3; ++component)
            {
                value = r_u8(0x800BCCB0 + vertex * 4 + component);
                w_u8(packet + 4 + vertex * 12 + component, value);
            }
        }
        value = r_u8(0x800BCCBC);
        w_u8(packet + 40, value);
        value = r_u8(0x800BCCBD);
        w_u8(packet + 41, value);
        value = r_u8(0x800BCCBE);
        w_u8(packet + 3, 12);
        w_u8(packet + 7, 0x3E);
        w_u8(packet + 42, value);
        texture = r_u32(object + 4);
        value = r_u16(texture + 8);
        w_u16(packet + 26, value);
        texture = r_u32(object + 4);
        value = r_u16(texture + 10);
        w_u16(packet + 14, value);
        centered = r_u16(object + 2) & 0x80;
        texture = r_u32(object + 4);
        for (vertex = 0; vertex < 4; ++vertex)
        {
            if (centered != 0)
            {
                size = r_u8(texture + 12) >> 1;
                coordinate = r_u16(object + 8) & 0xFFF;
                value = (vertex & 1) != 0 ? coordinate + size - 1 : coordinate - size;
            }
            else
            {
                value = r_u16(object + 8) & 0xFFF;
                if ((vertex & 1) != 0)
                    value += r_u8(texture + 12) + 0xFFFF;
            }
            w_u16(packet + 8 + vertex * 12, value);
            if (centered != 0)
            {
                size = r_u8(texture + 13) >> 1;
                coordinate = r_u16(object + 10);
                value = (vertex & 2) != 0 ? coordinate + size - 1 : coordinate - size;
            }
            else if ((vertex & 2) != 0)
            {
                size = r_u8(texture + 13);
                coordinate = r_u16(object + 10);
                value = coordinate + size + 0xFFFF;
            }
            else
                value = r_u16(object + 10);
            w_u16(packet + 10 + vertex * 12, value);
        }
        for (vertex = 0; vertex < 4; ++vertex)
        {
            texture = r_u32(object + 4);
            value = r_u8(texture + 14);
            if ((vertex & 1) != 0)
                value += r_u8(texture + 12) + 255;
            w_u8(packet + 12 + vertex * 12, value);
            texture = r_u32(object + 4);
            value = r_u8(texture + 15);
            if ((vertex & 2) != 0)
                value += r_u8(texture + 13) + 255;
            w_u8(packet + 13 + vertex * 12, value);
        }
        display = r_u32(0x800880F0);
        head = r_u32(display + 0x70);
        tag = r_u32(packet);
        head_tag = r_u32(head + 0x40);
        w_u32(packet, (tag & 0xFF000000) | (head_tag & 0xFFFFFF));
        head = r_u32(display + 0x70);
        head_tag = r_u32(head + 0x40);
        w_u32(head + 0x40, (head_tag & 0xFF000000) | (packet & 0xFFFFFF));
    }
    return 0;
}

uint32 td5_800b25e0(void)
{
    uint32 packet;
    uint32 index;

    FUNCTION_MARKER(0x800B25E0, "SLUS_006.10");
    td5_800b2438();
    packet = td5_alloc_memory(0xCB20);
    w_u32(0x800BF108, packet);
    for (index = 0; index < 1000; ++index)
    {
        w_u8(packet + 3, 12);
        w_u8(packet + 7, 0x3E);
        packet += 52;
    }
    return 0;
}

uint32 td5_800b2638(uint32 original_stack)
{
    uint32 buffer;

    FUNCTION_MARKER(0x800B2638, "SLUS_006.10");
    buffer = r_u32(0x80088104);
    w_u32(0x800BF410, 0);
    w_u32(0x800BF398, buffer * 500);
    return td5_800b3394(original_stack - 0x18);
}

uint32 td5_800b7294(uint32 records)
{
    uint32 data;
    uint32 indices;
    uint32 index;
    uint32 entry;
    uint32 value;
    uint32 kind;
    uint32 name;
    uint32 length;
    uint32 texture;

    FUNCTION_MARKER(0x800B7294, "SLUS_006.10");
    td5_80057310();
    value = r_u16(records);
    w_u32(0x800BF3A8, records);
    data = records + 4;
    if (value == 0xFFFF)
        return 0xFFFF;
    indices = 0x800BEFD8;
    do
    {
        index = td5_80057334();
        entry = 0x800BCF08 + index * 120;
        w_u32(indices, index);
        w_u8(entry, 1);
        value = r_u16(records);
        w_u16(entry + 2, value);
        value = r_u32(data);
        w_u32(entry + 4, value);
        value = r_u16(data + 4);
        w_u16(entry + 8, value);
        value = r_u16(data + 6);
        w_u16(entry + 10, value);
        value = r_u32(data + 16);
        w_u32(entry + 12, value);
        value = r_u8(data + 8);
        w_u8(entry + 1, value);
        kind = r_u16(records) & 0xFF00;
        if (kind == 0)
        {
            value = r_u8(entry + 2);
            w_u16(entry + 2, value);
            records += 24;
        }
        else if (kind == 0x8000)
        {
            name = r_u32(data);
            records += 24;
            length = 0;
            do
            {
                value = r_u8(name + length);
                ++length;
            } while (value != 0);
            texture = td5_find_texture((const char *)psx_addr(name, length));
            w_u32(entry + 4, texture);
        }
        else
            records += 24;
        value = r_u16(records);
        indices += 4;
        data += 24;
    } while (value != 0xFFFF);
    return 0xFFFF;
}

void td5_play_movies(uint32 mode)
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
    if (!xport_audio_init())
    {
        xport_message_error("Test Drive 5", "Cannot initialize audio for video playback");
        exit(EXIT_FAILURE);
    }
    result = xport_fmv_play(path);

#if !defined(AP_WEB)
    if ((result == 1 || result == 2) && mode == 0)
    {
        path = "DATA/INTO.STR";
        result = xport_fmv_play(path);
    }
#endif
    if (result != 1 && result != 2)
    {
        xport_message_error("Test Drive 5: video playback failed", path);
        exit(EXIT_FAILURE);
    }
#if !AP_WEB
    /* The fullscreen player releases its CD source on return */
    if (!td5_mount_xa_file(r_u32(0x80087BE4), &music))
    {
        xport_message_error("Test Drive 5", "Cannot restore the DATA XA stream after video playback");
        exit(EXIT_FAILURE);
    }
#endif
}

sint32 td5_8001acfc(void)
{
    uint32 value;

    FUNCTION_MARKER(0x8001ACFC, "SLUS_006.10");
    /* Movie playback no longer loads or executes the PS1 player */
    DrawSync(0);
    StopRCnt(0xF2000000, 0x80086B0C, 0x80086B14);
    StopRCnt(0xF2000001, 0x80086B0C, 0x80086B14);
    StopRCnt(0xF2000002, 0x80086B0C, 0x80086B14);
    StopRCnt(0xF2000003, 0x80086B0C, 0x80086B14);
    ResetGraph(3);
    stopIntr();
    EnterCriticalSectionPSX();
    value = r_u32(0x80087618);
    w_u8(value, 0);
    td5_play_movies(r_u8(r_u32(0x80087618)));
    ResetCallback();
    return CdInit();
}

void td5_800680a0(uint32 mode)
{
    FUNCTION_MARKER(0x800680A0, "SLUS_006.10");
    ChangeClearPAD(0);
    EnterCriticalSectionPSX();
    if (ReadInitPadFlag() == 0)
        mode = 0;
    td5_80068170(mode);
    td5_card_copy_patch();
    td5_card_patch_first(0x800680EC);
    td5_card_patch_second(0x800680F4);
    ExitCriticalSection();
}

void td5_8006810c(void)
{
    FUNCTION_MARKER(0x8006810C, "SLUS_006.10");
    EnterCriticalSectionPSX();
    td5_80068180();
    ChangeClearPAD(0);
    ExitCriticalSection();
}

uint32 td5_8006961c(void)
{
    FUNCTION_MARKER(0x8006961C, "SLUS_006.10");
    PadStartCom();
    return 1;
}

uint32 td5_800672f0(uint32 source, uint32 bytes)
{
    FUNCTION_MARKER(0x800672F0, "SLUS_006.10");
    if (bytes > 0x7EFF0)
        bytes = 0x7EFF0;
    SpuWrite(psx_addr(source, bytes), bytes);
    return bytes;
}

uint32 td5_800388a0(uint32 car, uint32 original_stack)
{
    uint32 result;

    FUNCTION_MARKER(0x800388A0, "SLUS_006.10");
    if (r_u32(0x80087738) == 0)
    {
        if (r_u32(0x80087734) == 0)
        {
            td5_8003af84(car);
            td5_80042f44(car, 0);
            td5_80050de8(car);
        }
        td5_800374b8(car);
        if (r_u32(0x8008763C) != 0)
            td5_80036f48();
    }
    result = td5_80039a24(car, original_stack - 0x18);
    return td5_80039f80(result);
}

uint32 td5_80038934(uint32 car, uint32 parameter, uint32 original_stack)
{
    uint32 result;

    FUNCTION_MARKER(0x80038934, "SLUS_006.10");
    if (r_u32(0x80087738) == 0)
    {
        td5_8003af84(car);
        td5_80037e04(car, parameter);
    }
    result = td5_80039a24(car, original_stack - 0x20);
    return td5_80039f80(result);
}

uint32 td5_80039f80(uint32 prior_result)
{
    uint32 count;
    uint32 index;
    uint32 base;
    uint32 display;
    uint32 buffer;
    uint32 packet;
    uint32 head;
    uint32 packet_tag;
    uint32 head_tag;

    FUNCTION_MARKER(0x80039F80, "SLUS_006.10");
    count = r_u32(0x80088504);
    if ((sint32)count <= 0)
        return prior_result;
    base = r_u32(0x80088468);
    display = r_u32(0x800880F0);
    buffer = r_u32(0x80088104);
    for (index = 0; (sint32)index < (sint32)count; ++index)
    {
        packet = base + buffer * 40;
        head = r_u32(display + 0x70);
        packet_tag = r_u32(packet);
        head_tag = r_u32(head);
        w_u32(packet, (packet_tag & 0xFF000000) | (head_tag & 0xFFFFFF));
        head = r_u32(display + 0x70);
        head_tag = r_u32(head);
        w_u32(head, (head_tag & 0xFF000000) | (packet & 0xFFFFFF));
        buffer += 2;
    }
    return 0;
}

uint32 td5_80059b38(void)
{
    uint32 index;
    uint32 width;
    uint32 offset;
    uint32 screen_height;
    uint32 height;
    uint32 flip;
    uint32 vertical_offset;
    uint32 adjustment;
    uint32 source;
    uint32 bytes;
    uint32 value;
    PSX_RECT rectangle;

    FUNCTION_MARKER(0x80059B38, "SLUS_006.10");
    for (index = 0; index < 4; ++index)
    {
        value = r_u32(0x800880F4);
        width = r_u32(0x8007FBCC + index * 4);
        offset = r_u32(0x8007FBEC + index * 4);
        rectangle.x = (sint16)((uint32)((sint32)(value - width) / 2) + offset);
        screen_height = r_u32(0x800880F8);
        height = r_u32(0x8007FBDC + index * 4);
        value = (uint32)((sint32)(screen_height - height) / 2);
        flip = r_u32(0x80088104);
        vertical_offset = r_u32(0x8007FBFC + index * 4);
        adjustment = r_u32(0x8008810C);
        rectangle.y = (sint16)(value + (flip << 8) + vertical_offset - adjustment * 14);
        rectangle.w = (sint16)r_u32(0x8007FBCC + index * 4);
        rectangle.h = (sint16)r_u32(0x8007FBDC + index * 4);
        source = r_u32(0x8009D918 + index * 4);
        bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
        LoadImagePSX(&rectangle, (uint32 *)psx_addr(source, bytes));
    }
    return 0;
}

uint32 td5_800592a4(void)
{
    uint32 required;
    uint32 index;
    uint32 width;
    uint32 height;
    uint32 owner;
    uint32 allocation;
    uint32 packet;
    uint32 buffer;
    uint32 item;

    FUNCTION_MARKER(0x800592A4, "SLUS_006.10");
    required = 2160;
    for (index = 0; index < 4; ++index)
    {
        width = r_u32(0x8007FBCC + index * 4);
        height = r_u32(0x8007FBDC + index * 4);
        required += (width * height) << 1;
    }
    if (r_s32(0x800883B8) < (sint32)required)
    {
        if (r_s32(0x800A3BC0) < (sint32)required)
            return 1;
        w_u32(0x80088750, 0);
    }
    else
        w_u32(0x80088750, 1);
    if (r_u32(0x80088750) == 0)
    {
        allocation = td5_alloc_memory(2160);
        w_u32(0x800883BC, allocation);
    }
    else
        td5_alloc_scene_memory(2160, 0x800883BC);
    for (index = 0; index < 4; ++index)
    {
        buffer = r_u32(0x80088750);
        owner = 0x8009D918 + index * 4;
        width = r_u32(0x8007FBCC + index * 4);
        height = r_u32(0x8007FBDC + index * 4);
        if (buffer == 0)
        {
            allocation = td5_alloc_memory((width * height) << 1);
            w_u32(owner, allocation);
        }
        else
            td5_alloc_scene_memory((width * height) << 1, owner);
    }
    packet = r_u32(0x800883BC);
    for (buffer = 0; buffer < 2; ++buffer)
    {
        for (item = 0; item < 30; ++item)
        {
            w_u8(packet + 3, 8);
            w_u8(packet + 7, 0x38);
            packet += 36;
        }
        SetDrawMode((DR_MODE *)psx_addr(0x800911E0 + buffer * 12, 12), 0, 1, 0, 0);
    }
    return 0;
}

uint32 td5_800544f8(void)
{
    uint32 index;
    uint32 voice;
    uint32 saved;
    uint32 value;

    FUNCTION_MARKER(0x800544F8, "SLUS_006.10");
    for (index = 0; index < 24; ++index)
    {
        voice = 0x80091970 + index * 92;
        saved = 0x800A1FB0 + index * 4;
        value = r_u16(voice + 8);
        w_u16(saved, value);
        value = r_u16(voice + 10);
        w_u16(saved + 2, value);
        w_u16(voice + 8, 0);
        w_u16(voice + 10, 0);
        SpuSetVoiceAttr((SpuVoiceAttr *)psx_addr(voice, sizeof(SpuVoiceAttr)));
    }
    return 0;
}

void td5_800545a4(void)
{
    uint32 index;
    uint32 voice;
    uint32 saved;
    uint32 value;

    FUNCTION_MARKER(0x800545A4, "SLUS_006.10");
    for (index = 0; index < 24; ++index)
    {
        voice = 0x80091970 + index * 92;
        saved = 0x800A1FB0 + index * 4;
        value = r_u16(saved);
        w_u16(voice + 8, value);
        value = r_u16(saved + 2);
        w_u16(voice + 10, value);
        SpuSetVoiceAttr((SpuVoiceAttr *)psx_addr(voice, sizeof(SpuVoiceAttr)));
    }
    value = r_u32(0x80087700) << 2;
    w_u16(0x800920A8, value);
    w_u16(0x800920AA, value);
    SpuSetVoiceAttr((SpuVoiceAttr *)psx_addr(0x800920A0, sizeof(SpuVoiceAttr)));
}

uint32 td5_800599a0(void)
{
    uint32 index;
    uint32 width;
    uint32 offset;
    uint32 screen_height;
    uint32 height;
    uint32 flip;
    uint32 vertical_offset;
    uint32 adjustment;
    uint32 destination;
    uint32 bytes;
    uint32 value;
    PSX_RECT rectangle;

    FUNCTION_MARKER(0x800599A0, "SLUS_006.10");
    for (index = 0; index < 4; ++index)
    {
        value = r_u32(0x800880F4);
        width = r_u32(0x8007FBCC + index * 4);
        offset = r_u32(0x8007FBEC + index * 4);
        rectangle.x = (sint16)((uint32)((sint32)(value - width) / 2) + offset);
        screen_height = r_u32(0x800880F8);
        height = r_u32(0x8007FBDC + index * 4);
        value = (uint32)((sint32)(screen_height - height) / 2);
        flip = r_u32(0x80088104);
        vertical_offset = r_u32(0x8007FBFC + index * 4);
        adjustment = r_u32(0x8008810C);
        rectangle.y = (sint16)(value + (flip << 8) + vertical_offset - adjustment * 14);
        rectangle.w = (sint16)r_u32(0x8007FBCC + index * 4);
        rectangle.h = (sint16)r_u32(0x8007FBDC + index * 4);
        destination = r_u32(0x8009D918 + index * 4);
        bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
        StoreImage(&rectangle, (uint32 *)psx_addr(destination, bytes));
    }
    return 0;
}

uint32 td5_8005a3d0(void)
{
    uint32 result;
    uint32 first;
    uint32 second;
    uint32 player;
    uint32 pad;
    uint32 width;
    uint32 height;
    uint32 flip;
    uint32 value;
    uint32 index;
    uint32 allocation;
    uint32 restore;
    PSX_RECT rectangle;

    FUNCTION_MARKER(0x8005A3D0, "SLUS_006.10");
    result = r_u32(0x80087650);
    first = r_u32(0x800880C0);
    second = r_u32(0x800880C4);
    rectangle.x = (sint16)first;
    rectangle.y = (sint16)(first >> 16);
    rectangle.w = (sint16)second;
    rectangle.h = (sint16)(second >> 16);
    if (result != 0)
        return result;
    result = r_u32(0x8008805C);
    player = result - 1;
    if (result == 0)
    {
        result = r_u16(0x800884E8) & 8;
        player = 0;
        if (result == 0)
        {
            result = r_u32(0x8008772C);
            if (result == 0)
                return result;
            result = r_u16(0x800884EA) & 8;
            player = 1;
            if (result == 0)
                return result;
        }
    }
    w_u32(0x8008805C, 0);
    result = td5_800592a4();
    if (result == 1)
        return result;
    restore = r_u32(0x80087668);
    w_u32(0x800880A0, 0);
    w_u32(0x80088698, restore);
    td5_8006b9e0(0, 0, 0);
    td5_8006b9e0(1, 0, 0);
    width = r_u32(0x800880F4);
    flip = r_u32(0x80088104);
    w_u32(0x8008809C, 0);
    w_u32(0x8008831C, 1);
    rectangle.w = (sint16)width;
    rectangle.y = (sint16)((flip ^ 1) << 8);
    MoveImage(&rectangle, 0, (sint32)(flip << 8));
    td5_800544f8();
#if AP_WEB
    td5_80055a58();
#else
    if (r_u32(0x80087730) != 0)
        td5_80055a58();
#endif
    flip = r_u32(0x8008810C);
    width = r_u32(0x800880F4);
    height = r_u32(0x800880F8);
    w_u16(0x80097668, 0);
    w_u16(0x800976DC, 0);
    value = (flip ^ 1) << 3;
    w_u16(0x8009766A, value);
    value += 256;
    w_u16(0x8009766C, width);
    w_u16(0x8009766E, height);
    w_u16(0x800976DE, value);
    w_u16(0x800976E0, width);
    w_u16(0x800976E2, height);
    td5_800599a0();
    if (r_u32(0x8008831C) == 1)
    {
        pad = 0x800884E8 + player * 2;
        do
        {
            td5_poll_pads();
            td5_swap_display_buffer();
            td5_clear_ordering_table();
            td5_80059490();
            td5_80039dec();
            td5_80059c48(player);
            if (r_u32(0x800880A0) == 1)
            {
                result = r_u16(pad) & 0x4008;
                if (result != 0)
                {
                    value = r_u32(0x8008809C);
                    result = 3;
                    if (value == 0)
                    {
                        w_u32(0x800885E0, 1);
                        w_u32(0x8008831C, 0);
                        w_u32(0x80088530, 1);
                    }
                    else
                    {
                        w_u32(0x800880A0, 0);
                        w_u32(0x8008809C, 3);
                    }
                }
            }
            else
            {
                result = r_u16(pad) & 0x4008;
                if (result != 0)
                {
                    if (r_u32(0x8008809C) == 0)
                    {
                        td5_800530f0();
                        w_u32(0x8008831C, 0);
                    }
                    value = r_u16(pad) & 0x4008;
                    result = 3;
                    if (value != 0 && r_u32(0x8008809C) == 3)
                    {
                        result = (uint32)td5_800530f0();
                        w_u32(0x800880A0, 1);
                        w_u32(0x8008809C, 1);
                    }
                }
            }
            td5_80039f80(result);
            if (r_u32(0x8008772C) == 0 && r_u32(0x80087730) == 0)
                td5_8005ae14();
            VSync(2);
            td5_apply_display_buffer();
            td5_80059b38();
            td5_draw_ordering_table();
            if ((r_u16(pad) & 8) != 0)
                w_u32(0x8008831C, 0);
        } while (r_u32(0x8008831C) == 1);
    }
    VSync(0);
    DrawSync(0);
    if (r_u32(0x80088750) == 0)
        td5_free_memory(r_u32(0x800883BC));
    else
        td5_free_scene_memory(r_u32(0x800883BC));
    if (r_u32(0x80088750) == 0)
    {
        for (index = 0; index < 4; ++index)
        {
            allocation = r_u32(0x8009D918 + index * 4);
            td5_free_memory(allocation);
        }
    }
    else
    {
        for (index = 0; index < 4; ++index)
        {
            allocation = r_u32(0x8009D918 + index * 4);
            td5_free_scene_memory(allocation);
        }
    }
    td5_800545a4();
#if AP_WEB
    td5_80055a80();
#else
    if (r_u32(0x80087730) != 0)
        td5_80055a80();
#endif
    restore = r_u32(0x80088698);
    result = r_u32(0x8008772C);
    w_u16(0x800884E8, 0);
    w_u16(0x800884EA, 0);
    w_u16(0x800885D0, 0);
    w_u16(0x800885D2, 0);
    w_u32(0x80087668, restore);
    if (result != 0)
        return td5_80049dec(restore);
    return result;
}

void td5_8006e2e4(sint32 lower, sint32 upper)
{
    FUNCTION_MARKER(0x8006E2E4, "SLUS_006.10");
    w_u32(0x8008811C, (uint32)lower);
    w_u32(0x80088120, (uint32)upper);
}

void td5_80054260(void)
{
    FUNCTION_MARKER(0x80054260, "SLUS_006.10");
    SpuSetKey(0, 0xFFFFFF);
}

uint32 td5_8006e2f4(uint32 radius, uint32 destination, uint32 color, uint32 height)
{
    uint32 index;
    uint32 channel;
    uint32 root;
    uint32 product;
    uint32 quotient;
    uint32 weight;
    uint32 from;
    uint32 to;
    uint32 candidate;
    uint32 lower;
    uint32 upper;
    uint32 table;
    uint32 packed;
    uint32 components[3];

    FUNCTION_MARKER(0x8006E2F4, "SLUS_006.10");
    w_u8(destination, 0);
    w_u8(destination + 1, 0);
    w_u8(destination + 2, 0);
    w_u8(0x80088128, r_u8(color) >> 1);
    w_u8(0x80088129, r_u8(color + 1) >> 1);
    w_u8(0x8008812A, r_u8(color + 2) >> 1);
    table = td5_alloc_memory(1024);
    w_u32(0x8008834C, table);
    table = td5_alloc_memory(4096);
    w_u32(0x80088350, table);
    table = td5_alloc_memory(4096);
    w_u32(0x80088354, table);
    w_u32(0x80088344, radius);
    w_u32(0x80088348, radius * radius);
    packed = r_u8(destination + 1) << 8;
    packed |= r_u8(destination);
    packed |= r_u8(destination + 2) << 16;
    w_u32(0x80088358, packed);
    for (index = 0; index < 1024; ++index)
    {
        root = SquareRoot0((sint32)(index << 15));
        product = height * root;
        quotient = (uint32)td5_divide_checked((sint32)product, (sint32)radius);
        weight = height - quotient;
        for (channel = 0; channel < 3; ++channel)
        {
            if (channel == 0)
            {
                from = r_u8(destination);
                to = r_u8(color);
            }
            else
            {
                to = r_u8(color + channel);
                from = r_u8(destination + channel);
            }
            product = weight * (to - from);
            candidate = from + (uint32)((sint32)product / 256);
            lower = r_u8(destination + channel);
            if ((sint32)lower < (sint32)candidate)
                lower = candidate;
            upper = r_u8(color + channel);
            if ((sint32)lower < (sint32)upper)
                upper = lower;
            components[channel] = upper;
        }
        table = r_u32(0x80088350);
        packed = components[0] | (components[1] << 8) | (components[2] << 16);
        w_u32(table + index * 4, packed);
        for (channel = 0; channel < 3; ++channel)
        {
            if (channel == 1)
            {
                product = height * root;
                quotient = (uint32)td5_divide_checked((sint32)product, (sint32)radius);
                weight = height - quotient;
            }
            to = r_u8(0x80088128 + channel);
            from = r_u8(0x80088124 + channel);
            product = weight * (to - from);
            candidate = from + (uint32)((sint32)product / 256);
            lower = from;
            if ((sint32)lower < (sint32)candidate)
                lower = candidate;
            upper = to;
            if ((sint32)lower < (sint32)upper)
                upper = lower;
            components[channel] = upper;
        }
        table = r_u32(0x80088354);
        packed = components[0] | (components[1] << 8) | (components[2] << 16);
        w_u32(table + index * 4, packed);
    }
    return 0;
}

static void td5_upload_stream_image(uint32 rectangle_address, uint32 pixels)
{
    sint32 width;
    sint32 height;
    uint32 bytes;

    width = r_s16(rectangle_address + 4);
    height = r_s16(rectangle_address + 6);
    bytes = width > 0 && height > 0 ? (uint32)width * (uint32)height * 2 : 0;
    LoadImagePSX((PSX_RECT *)psx_addr(rectangle_address, 8), (uint32 *)psx_addr(pixels, bytes));
}

void td5_800542ec(uint32 position)
{
    FUNCTION_MARKER(0x800542EC, "SLUS_006.10");
    w_u32(0x800882E0, position);
}

void td5_800542e0(uint32 velocity)
{
    FUNCTION_MARKER(0x800542E0, "SLUS_006.10");
    w_u32(0x800882DC, velocity);
}

uint32 td5_800341fc(void)
{
    uint32 data;
    uint32 entry;
    uint32 index;
    uint32 value;
    uint32 width;
    uint32 height;
    uint32 pixels;
    uint32 size;
    uint32 result;

    FUNCTION_MARKER(0x800341FC, "SLUS_006.10");
    if (r_u32(0x8008786C) == 1)
    {
        entry = r_u32(0x80088590);
        if (r_u8(entry + 33) == 2)
        {
            w_u32(0x8008786C, 0);
            data = r_u32(0x80088548);
            w_u32(0x80088210, r_u32(0x80088548) + 0x5AC);
            entry = r_u32(0x80088590);
            w_u8(entry + 33, 4);
            for (index = 0; (sint32)index < r_u8(data + 2); ++index)
            {
                value = r_u8(data + index + 0x84);
                w_u16(0x80087874, (value & 15) * 2);
                value = r_u8(data + index + 0x84);
                w_u16(0x80087876, (value >> 4) * 8);
                value = r_u8(data + index + 4);
                w_u16(0x80087870, value * 4);
                value = r_u8(data + index + 0x44);
                w_u16(0x80087872, value * 4);
                pixels = r_u32(0x80088210);
                td5_upload_stream_image(0x80087870, pixels);
                width = (uint32)r_s16(0x80087874);
                height = (uint32)r_s16(0x80087876);
                w_u32(0x80088214, width * (height * 2));
                pixels = r_u32(0x80088210);
                size = r_u32(0x80088214);
                w_u32(0x80088210, pixels + size);
            }
            for (index = 0; (sint32)index < r_u8(data + 3); ++index)
            {
                value = r_u8(data + index + 0xC4);
                w_u16(0x80087878, value * 4);
                value = r_u8(data + index + 0x104);
                w_u16(0x8008787A, value);
                pixels = r_u32(0x80088210);
                td5_upload_stream_image(0x80087878, pixels);
                pixels = r_u32(0x80088210);
                w_u32(0x80088210, pixels + 32);
            }
            td5_free_scene_memory(r_u32(0x80088548));
        }
    }
    result = r_u32(0x8008786C);
    if (result != 0xFFFFFFFF)
        return result;
    entry = r_u32(0x80088654);
    if (r_u8(entry + 33) != 2)
        return 2;
    w_u32(0x8008786C, 0);
    data = r_u32(0x80088548);
    w_u32(0x80088210, r_u32(0x80088548) + 0x5AC);
    entry = r_u32(0x80088654);
    w_u8(entry + 33, 4);
    for (index = 0; (sint32)index < r_u8(data + 0x144); ++index)
    {
        value = r_u8(data + index + 0x1C6);
        w_u16(0x80087874, (value & 15) * 2);
        value = r_u8(data + index + 0x1C6);
        w_u16(0x80087876, (value >> 4) * 8);
        value = r_u8(data + index + 0x146);
        w_u16(0x80087870, value * 4);
        value = r_u8(data + index + 0x186);
        w_u16(0x80087872, value * 4);
        pixels = r_u32(0x80088210);
        td5_upload_stream_image(0x80087870, pixels);
        width = (uint32)r_s16(0x80087874);
        height = (uint32)r_s16(0x80087876);
        w_u32(0x80088214, width * (height * 2));
        pixels = r_u32(0x80088210);
        size = r_u32(0x80088214);
        w_u32(0x80088210, pixels + size);
    }
    for (index = 0; (sint32)index < r_u8(data + 0x145); ++index)
    {
        value = r_u8(data + index + 0x206);
        w_u16(0x80087878, value * 4);
        value = r_u8(data + index + 0x246);
        w_u16(0x8008787A, value);
        pixels = r_u32(0x80088210);
        td5_upload_stream_image(0x80087878, pixels);
        pixels = r_u32(0x80088210);
        w_u32(0x80088210, pixels + 32);
    }
    return td5_free_scene_memory(r_u32(0x80088548));
}

uint32 td5_8005aca4(void)
{
    uint32 result;
    uint32 index;
    uint32 entry;

    FUNCTION_MARKER(0x8005ACA4, "SLUS_006.10");
    result = td5_8005aac8();
    if (result == 1)
        return result;
    w_u32(0x800880D4, 0);
    w_u32(0x800880C8, 0);
    if (r_u32(0x80088368) == 0)
        td5_80035f2c();
    else
        td5_800b3bec();
    for (index = 0; index < 2; ++index)
    {
        entry = 0x800A4E28 + index * 24;
        w_u16(entry, 1);
        w_u16(entry + 2, 0);
        w_u32(entry + 8, 0);
        w_u32(entry + 16, 0);
        w_u32(entry + 20, 0x8005A840);
    }
    return index;
}

uint32 td5_8005ae14(void)
{
    uint32 result;
    uint32 index;
    uint32 entry;
    uint32 source;
    uint32 flags;
    uint32 mask;
    uint32 sector;
    uint32 count;
    uint32 destination;

    FUNCTION_MARKER(0x8005AE14, "SLUS_006.10");
    if (r_u32(0x800880D4) != 0)
        td5_8005aca4();
    while (r_s32(0x800880D4) >= 50)
    {
        VSync(0);
        td5_8005aca4();
    }
    result = td5_8005aac8();
    if (result == 1)
        return result;
    if (r_u32(0x800880C8) != 1)
        goto scan_start;
    result = 2;
    if (result != 2)
        return result;
    if (r_u32(0x800880D0) == 0xFFFFFFFB)
    {
        result = 0;
        if (result != 0)
        {
            if (result == 0xFFFFFFFF)
                td5_debug_log(0x8001327C);
            return result;
        }
        td5_debug_log(0x8001326C, r_u32(0x800880CC));
        entry = 0x800A4E28 + r_u32(0x800880CC) * 24;
        td5_call_cd_request_callback(r_u32(entry + 20));
    }
    td5_debug_log(0x80013288, r_u32(0x800880CC));
    entry = 0x800A4E28 + r_u32(0x800880CC) * 24;
    source = 0x800A4E28 + r_u32(0x800880CC) * 24;
    flags = r_u16(source);
    mask = r_u16(0x800880D0);
    w_u16(entry, flags & mask);
    entry = 0x800A4E28 + r_u32(0x800880CC) * 24;
    if ((r_u16(entry) & 0x10) == 0)
        goto scan_start;
    w_u32(0x800880C8, 0);
    index = r_u32(0x800880CC);
    goto process_request;

scan_start:
    w_u32(0x800880C8, 0);
    index = 0;
scan_check:
    if ((sint32)index >= 2)
    {
        result = r_u32(0x800880C8);
        if (result == 1)
            goto scan_start;
        return result;
    }
process_request:
    entry = 0x800A4E28 + index * 24;
    if ((r_u16(entry) & 0x10) != 0)
    {
        w_u32(0x800880C8, 1);
        w_u32(0x800880CC, index);
        if ((r_u16(entry) & 2) != 0)
        {
            td5_debug_log(0x80013294, index);
            w_u32(0x800880D0, 0xFFFFFFFD);
            return 1;
        }
        if ((r_u16(entry) & 4) != 0)
        {
            td5_debug_log(0x800132A0, index, r_u32(entry + 16));
            count = r_u32(entry + 8);
            destination = r_u32(entry + 16);
            if (td5_use_extracted_assets())
                td5_read_stream_asset(index, r_u16(entry + 2) * 2048, count * 2048, destination);
            else
            {
                sector = r_u16(entry + 2) + r_u32(entry + 4);
                td5_read_archive_sectors(sector, count, destination);
            }
            w_u32(0x800880D0, 0xFFFFFFFB);
            return 0xFFFFFFFB;
        }
        flags = r_u16(entry);
        w_u16(entry, flags & 0xFFEF);
    }
    ++index;
    goto scan_check;
}

uint32 td5_8007b628(uint32 sector, uint8 *location)
{
    uint32 minutes;
    uint32 seconds;
    uint32 frames;
    uint32 tens;

    FUNCTION_MARKER(0x8007B628, "SLUS_006.10");
    minutes = sector / 4500;
    seconds = (sector % 4500) / 75;
    frames = sector % 75;
    location[3] = 0;
    location[2] = (frames / 10) * 16 + frames % 10;
    location[0] = (minutes / 10) * 16 + minutes % 10;
    tens = seconds / 10;
    location[1] = tens * 16 + seconds % 10;
    return tens * 10;
}

uint32 td5_80035f2c(void)
{
    uint32 index;
    uint32 entry;

    FUNCTION_MARKER(0x80035F2C, "SLUS_006.10");
    w_u32(0x8008786C, 0);
    w_u32(0x80087868, 0);
    for (index = 0; index < 7; ++index)
    {
        entry = 0x8009DBD0 + index * 36;
        if (r_u8(entry + 32) == 1 || r_u8(entry + 32) == 3)
        {
            w_u8(entry + 32, 0);
            if (r_u32(entry) != 0)
            {
                td5_free_scene_memory(r_u32(entry));
                w_u32(entry, 0);
            }
        }
        if (r_u8(entry + 33) == 1 || r_u8(entry + 33) == 3)
        {
            w_u8(entry + 33, 0);
            td5_free_scene_memory(r_u32(0x80088548));
        }
    }
    return index;
}

uint32 td5_800346dc(void)
{
    uint32 entry;

    FUNCTION_MARKER(0x800346DC, "SLUS_006.10");
    entry = r_u32(0x80088590);
    w_u8(entry + 33, 2);
    return entry;
}

uint32 td5_80034708(void)
{
    uint32 entry;

    FUNCTION_MARKER(0x80034708, "SLUS_006.10");
    entry = r_u32(0x80088654);
    w_u8(entry + 33, 2);
    return entry;
}

uint32 td5_80034734(void)
{
    uint32 entry;
    uint32 buffer;
    uint32 index;
    uint32 value;

    FUNCTION_MARKER(0x80034734, "SLUS_006.10");
    entry = r_u32(0x8008861C);
    buffer = r_u32(entry);
    entry = r_u32(0x8008861C);
    w_u8(entry + 32, 2);
    for (index = 0; index < 6; ++index)
    {
        entry = r_u32(0x8008861C);
        value = r_u32(buffer + 0x288 + index * 4);
        w_u32(entry + 4 + index * 4, value);
    }
    return entry;
}

uint32 td5_8005aac8(void)
{
    FUNCTION_MARKER(0x8005AAC8, "SLUS_006.10");
    return 0;
}

uint32 td5_8005b300(uint32 slot, uint32 buffer, uint32 sectors, uint32 callback, uint32 sector)
{
    uint32 entry;
    uint32 flags;

    FUNCTION_MARKER(0x8005B300, "SLUS_006.10");
    if (td5_8005aac8() == 1)
        return 1;
    if (r_u32(0x800880D4) != 0)
        return 1;
    entry = 0x800A4E28 + slot * 24;
    flags = r_u16(entry);
    w_u16(entry, flags | 0x16);
    w_u32(entry + 8, sectors);
    w_u32(entry + 16, buffer);
    w_u32(entry + 20, callback);
    w_u16(entry + 2, sector);
    return 0;
}

uint32 td5_80034804(uint32 record)
{
    uint32 data;
    uint32 entry;
    uint32 sector;
    uint32 size;
    uint32 result;
    uint32 buffer;

    FUNCTION_MARKER(0x80034804, "SLUS_006.10");
    data = r_u32(0x800884C0) + record * 18;
    (void)r_u16(data + 10);
    entry = 0x8009DBD0 + (uint32)((sint32)record % 7) * 36;
    if (r_u32(entry) != 0)
        td5_free_scene_memory(r_u32(entry));
    w_u32(0x80087864, 0);
    if (r_s32(0x80087860) < (sint32)record)
        w_u32(0x80087864, 1);
    if ((sint32)record < r_s32(0x80087860))
        w_u32(0x80087864, 0xFFFFFFFF);
    w_u32(0x80087860, record);
    w_u8(entry + 33, 4);
    sector = r_u16(data + 10);
    sector += r_u16(data + 4);
    size = r_u8(data + 2);
    if (size != 0 && r_u32(0x80087864) == 1)
    {
        w_u32(0x8008786C, r_u32(0x80087864));
        w_u8(entry + 33, 1);
        w_u32(0x80088590, entry);
        td5_alloc_scene_memory(size << 11, 0x80088548);
        buffer = r_u32(0x80088548);
        td5_8005b300(1, buffer, size, 0x800346DC, sector);
        w_u32(0x80087868, 1);
    }
    sector = r_u16(data + 10);
    sector += r_u16(data + 6);
    size = r_u8(data + 3);
    result = size;
    if (size != 0)
    {
        result = r_u32(0x80087864);
        if (result == 0xFFFFFFFF)
        {
            w_u32(0x8008786C, r_u32(0x80087864));
            w_u8(entry + 33, 1);
            w_u32(0x80088654, entry);
            td5_alloc_scene_memory(size << 11, 0x80088548);
            buffer = r_u32(0x80088548);
            td5_8005b300(1, buffer, size, 0x80034708, sector);
            result = 1;
            w_u32(0x80087868, 1);
        }
    }
    return result;
}

uint32 td5_80034ae8(uint32 record, uint32 node, sint32 lane)
{
    uint32 variant;
    uint32 data;
    uint32 entry;
    uint32 sector;
    uint32 size;
    uint32 buffer;
    uint32 value;

    FUNCTION_MARKER(0x80034AE8, "SLUS_006.10");
    variant = td5_80033fa4((sint32)node, lane);
    data = r_u32(0x800884C0) + record * 18;
    entry = 0x8009DBD0 + (uint32)((sint32)record % 7) * 36;
    if (r_u8(data + 1) == 0)
        variant = 3;
    if (variant == 2)
    {
        sector = r_u16(data + 10);
        sector += r_u8(data);
        size = r_u8(data + 1);
        w_u8(entry + 34, 2);
    }
    else if (variant == 1)
    {
        sector = r_u16(data + 10);
        size = r_u8(data);
        w_u8(entry + 34, 1);
    }
    else
    {
        /* The audited selector returns only 1, 2 or 3 */
        sector = r_u16(data + 10);
        size = r_u8(data);
        size += r_u8(data + 1);
        w_u8(entry + 34, 3);
    }
    td5_alloc_scene_memory(size << 11, entry);
    buffer = r_u32(entry);
    w_u16(entry + 30, record);
    value = r_u16(data + 8);
    w_u16(entry + 28, value);
    w_u8(entry + 32, 1);
    w_u32(0x8008861C, entry);
    td5_8005b300(0, buffer, size, 0x80034734, sector);
    w_u32(0x80087868, 1);
    return 1;
}

uint32 td5_80033fa4(sint32 node, sint32 lane)
{
    uint32 base;
    uint32 entry;
    uint32 index;

    FUNCTION_MARKER(0x80033FA4, "SLUS_006.10");
    base = r_u32(0x80088584);
    (void)r_u32(base + 20);
    base = r_u32(0x80088770);
    entry = r_u32(base + 28);
    for (index = 0; (sint32)index < r_s32(0x800886D4); ++index)
    {
        if (node >= r_u16(entry) && node <= r_u16(entry + 2))
        {
            if (r_u16(entry + 4) == 0)
                return 3;
            if (lane < r_s32(0x8008841C))
                return 1;
            return 2;
        }
        entry += 6;
    }
    return 3;
}

void td5_8003a020(void)
{
    FUNCTION_MARKER(0x8003A020, "SLUS_006.10");
}

uint32 td5_80035bd4(void)
{
    uint32 value;
    uint32 index;
    sint32 lower;
    sint32 upper;
    sint32 last;

    FUNCTION_MARKER(0x80035BD4, "SLUS_006.10");
    value = r_u32(0x80088674);
    w_u32(0x80088674, value + 1);
    if (r_s32(0x80088674) >= 1200)
        return 1;
    lower = (sint32)(r_u32(0x80088218) - 60);
    upper = (sint32)(r_u32(0x80088218) + 60);
    lower /= 20;
    if (lower < 0)
        lower = 0;
    upper /= 20;
    last = (sint32)(r_u32(0x800885F4) - 1);
    if (upper < last)
        last = upper;
    for (index = (uint32)lower; (sint32)index <= last; ++index)
    {
        value = (uint32)((sint32)index % 7);
        if (r_u8(0x8009DBF0 + value * 36) != 2)
            return 0;
    }
    return 1;
}

uint32 td5_800357e4(sint32 node, sint32 lane)
{
    uint32 result;
    uint32 aligned;
    uint32 index;
    uint32 entry;
    uint32 state;
    sint32 lower;
    sint32 upper;
    sint32 last;
    sint32 record;

    FUNCTION_MARKER(0x800357E4, "SLUS_006.10");
    result = r_u32(0x800880D4);
    if (result != 0)
        return result;
    result = r_u32(0x800880C8);
    if (result != 0)
        return result;
    w_u32(0x80087868, 0);
    aligned = (uint32)node + 10 - (uint32)(node % 20);
    w_u32(0x80088218, aligned);
    w_u32(0x800878A4, aligned);
    lower = (sint32)(aligned - 60) / 20;
    if (lower < 0)
        lower = 0;
    upper = (sint32)(aligned + 60) / 20;
    last = (sint32)(r_u32(0x800885F4) - 1);
    if (upper < last)
        last = upper;
    for (index = 0; index < 7; ++index)
    {
        entry = 0x8009DBD0 + index * 36;
        state = r_u8(entry + 32);
        if (state == 3)
        {
            w_u8(entry + 32, 0);
            w_u8(entry + 33, 0);
        }
        if (state == 2)
        {
            record = r_s16(entry + 30);
            if (record < lower || last < record)
                w_u8(entry + 32, 3);
        }
    }
    for (index = (uint32)lower;; ++index)
    {
        if (last < (sint32)index)
            return 1;
        entry = 0x8009DBD0 + (uint32)((sint32)index % 7) * 36;
        if (r_u8(entry + 33) == 0 && r_u32(0x8008786C) == 0)
            td5_80034804(index);
        result = r_u32(0x80087868);
        if (result != 0)
            return result;
        if (r_u8(entry + 33) == 4 && r_u8(entry + 32) == 0 && r_u32(0x8008786C) == 0)
            td5_80034ae8(index, index * 20, lane);
        result = r_u32(0x80087868);
        if (result != 0)
            return result;
    }
}
