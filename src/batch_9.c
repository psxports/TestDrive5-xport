#include "td5_functions.h"

typedef struct TD5_MINIMAP_POINT
{
    uint32 x;
    uint32 y;
} TD5_MINIMAP_POINT;

static TD5_MINIMAP_POINT td5_minimap_project(uint32 segment, uint32 vertex, uint32 origin_x, uint32 origin_z, uint32 cosine, uint32 sine, uint32 scale_x, uint32 scale_z)
{
    TD5_MINIMAP_POINT point;
    uint32 x;
    uint32 z;
    uint32 value;
    uint32 component;

    component = (uint32)r_s16(vertex);
    x = r_u32(segment + 12) + component + origin_x;
    component = (uint32)r_s16(vertex + 4);
    z = r_u32(segment + 20) + component + origin_z;
    value = x * cosine + z * sine;
    value = (uint32)((sint32)value / 4096) * scale_x;
    point.x = (uint32)((sint32)value / 65536) + 50;
    value = (0 - x) * sine + z * cosine;
    value = (uint32)((sint32)value / 4096) * scale_z;
    point.y = (uint32)((sint32)value / 65536) + 32;
    return point;
}

uint32 td5_minimap_link(uint32 buffer, uint32 packet)
{
    uint32 ordering;
    uint32 tag;
    uint32 link;

    ordering = r_u32(buffer + 0x70);
    tag = r_u32(packet);
    link = r_u32(ordering);
    w_u32(packet, (tag & 0xFF000000) | (link & 0xFFFFFF));
    ordering = r_u32(buffer + 0x70);
    tag = r_u32(ordering);
    tag = (tag & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(ordering, tag);
    return tag;
}

uint32 td5_80045980(uint32 position)
{
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 value;
    uint32 previous;
    uint32 shift;
    uint32 delta_x;
    uint32 delta_y;
    uint32 delta_z;
    uint32 buffer_index;
    uint32 previous_packet;
    uint32 packet;
    uint32 buffer;
    uint32 ordering;
    uint32 index;
    uint32 point;
    uint32 visible;
    uint32 tag;
    uint32 link;
    uint32 screen_y;
    sint32 count;
    sint32 divisor;
    sint32 coordinate;
    sint32 depth;
    sint32 projected;
    sint32 width;
    sint32 height;
    sint32 screen_x;

    FUNCTION_MARKER(0x80045980, "SLUS_006.10");
    x = r_u32(0x80097648);
    y = r_u32(0x8009764C);
    xport_gte_write_control(0, x);
    xport_gte_write_control(1, y);
    x = r_u32(0x80097650);
    y = r_u32(0x80097654);
    z = r_u32(0x80097658);
    xport_gte_write_control(2, x);
    xport_gte_write_control(3, y);
    xport_gte_write_control(4, z);
    value = r_u32(position);
    previous = r_u32(0x80089B48);
    value = (uint32)((sint32)(value - previous) / 256);
    x = r_u16(0x800879FC) - value;
    value = r_u32(position + 4);
    previous = r_u32(0x80089B4C);
    value = (uint32)((sint32)(value - previous) / 256);
    y = r_u16(0x800879FE) - value;
    value = r_u32(position + 8);
    previous = r_u32(0x80089B50);
    value = (uint32)((sint32)(value - previous) / 256);
    z = r_u16(0x80087A00) - value;
    value = r_u32(position);
    previous = r_u32(position + 4);
    shift = r_u32(position + 8);
    w_u32(0x80089B48, value);
    w_u32(0x80089B4C, previous);
    w_u32(0x80089B50, shift);
    xport_gte_write_data(0, (x & 0xFFFF) | (y << 16));
    xport_gte_write_data(1, z & 0xFFFF);
    xport_gte_execute(0x486012);
    x = xport_gte_read_data(9);
    y = xport_gte_read_data(10);
    z = xport_gte_read_data(11);
    shift = r_u32(0x80088164);
    divisor = r_s32(0x80088160);
    value = ((uint32)(sint32)(sint16)x << (shift & 31)) << 12;
    delta_x = (uint32)td5_divide_checked((sint32)value, divisor);
    value = ((uint32)(sint32)(sint16)y << (shift & 31)) << 12;
    delta_y = (uint32)td5_divide_checked((sint32)value, divisor);
    value = ((uint32)(sint32)(sint16)z << (shift & 31)) << 12;
    delta_z = (uint32)td5_divide_checked((sint32)value, divisor);
    buffer_index = r_u32(0x80088104);
    previous_packet = r_u32(0x800882A0 + (buffer_index ^ 1) * 4);
    buffer = r_u32(0x800880F0);
    packet = r_u32(0x800882A0 + buffer_index * 4);
    count = r_s32(0x80087A08);
    ordering = r_u32(buffer + 0x70);
    index = 0;
    if (count > 0)
    {
        do
        {
            point = 0x80089688 + index * 8;
            visible = 0x80089A88 + index;
            value = r_u16(point);
            w_u16(point, value + delta_x);
            value = r_u16(point + 2);
            w_u16(point + 2, value + delta_y);
            value = r_u16(point + 4);
            w_u16(point + 4, value + delta_z);
            coordinate = r_s16(point);
            if (coordinate < -4000)
            {
                w_u16(point, (uint32)coordinate + 8000);
                w_u8(visible, 0);
            }
            coordinate = r_s16(point);
            if (coordinate >= 4000)
            {
                w_u16(point, (uint32)coordinate - 8000);
                w_u8(visible, 0);
            }
            coordinate = r_s16(point + 2);
            if (coordinate < -4000)
            {
                w_u16(point + 2, (uint32)coordinate + 5000);
                w_u8(visible, 0);
            }
            coordinate = r_s16(point + 2);
            if (coordinate >= 1000)
            {
                w_u16(point + 2, (uint32)coordinate - 5000);
                w_u8(visible, 0);
            }
            coordinate = r_s16(point + 4);
            if (coordinate < 200)
            {
                w_u16(point + 4, (uint32)coordinate + 7991);
                w_u8(visible, 0);
            }
            coordinate = r_s16(point + 4);
            if (coordinate >= 8191)
            {
                w_u16(point + 4, (uint32)coordinate - 7991);
                w_u8(visible, 0);
            }
            coordinate = r_s16(point);
            depth = r_s16(point + 4);
            projected = td5_divide_checked((sint32)((uint32)coordinate * 180), depth);
            width = r_s32(0x800880F4);
            value = (uint32)(width / 2) + (uint32)projected;
            w_u16(packet + 8, value);
            coordinate = r_s16(point + 2);
            depth = r_s16(point + 4);
            projected = td5_divide_checked((sint32)((uint32)coordinate * 180), depth);
            height = r_s32(0x800880F8);
            screen_x = r_s16(packet + 8);
            screen_y = (uint32)(height / 2) + (uint32)projected;
            w_u16(packet + 10, screen_y);
            if (screen_x < -63 || screen_x >= (sint32)((uint32)width + 64) ||
                (sint16)screen_y < -63 || (sint16)screen_y >= (sint32)((uint32)height + 64))
                w_u8(visible, 0);
            else
            {
                if (r_u8(visible) != 0)
                {
                    value = r_u32(previous_packet + 8);
                    w_u32(packet + 16, value);
                    tag = r_u32(packet);
                    link = r_u32(ordering);
                    w_u32(packet, (tag & 0xFF000000) | (link & 0xFFFFFF));
                    tag = r_u32(ordering);
                    w_u32(ordering, (tag & 0xFF000000) | (packet & 0xFFFFFF));
                }
                w_u8(visible, 1);
            }
            previous_packet += 20;
            packet += 20;
            count = r_s32(0x80087A08);
            ++index;
        } while ((sint32)index < count);
    }
    buffer_index = r_u32(0x80088104);
    link = r_u32(ordering);
    packet = 0x80089B08 + buffer_index * 32;
    tag = r_u32(packet);
    w_u32(packet, (tag & 0xFF000000) | (link & 0xFFFFFF));
    tag = r_u32(ordering);
    value = (tag & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(ordering, value);
    return value;
}

uint32 td5_800464cc(uint32 car)
{
    uint32 owner;
    uint32 table;
    uint32 index;
    uint32 value;
    uint32 target;
    uint32 count;
    uint32 random;
    sint32 node;
    sint32 mode;

    FUNCTION_MARKER(0x800464CC, "SLUS_006.10");
    owner = r_u32(0x80088770);
    node = r_s16(car + 0x70);
    table = r_u32(owner + 0x24);
    for (index = 0; index < 8; ++index)
    {
        if (node == r_s16(table + 2))
        {
            value = (uint32)r_s16(table + 4);
            w_u32(0x80087A04, value);
        }
        table += 4;
    }
    target = r_u32(0x80087A04);
    if (target != 0)
    {
        count = r_u32(0x80087A08);
        if ((sint32)count < (sint32)target)
        {
            w_u8(0x80089A88 + count, 0);
            value = r_u32(0x80087A08);
            w_u32(0x80087A08, value + 1);
        }
        target = r_u32(0x80087A04);
    }
    if (target == 0)
    {
        count = r_u32(0x80087A08);
        if ((sint32)count > 0)
            w_u32(0x80087A08, count - 1);
    }
    mode = r_s32(0x80087A0C);
    if (mode == 1)
    {
        index = r_u8(car + 0x34D);
        return td5_80045ec0(0x8009B3E0 + index * 12);
    }
    if (mode >= 2)
        return 0;
    if (mode != 0)
        return 1;
    random = (uint32)randPSX();
    count = r_u32(0x80087A08);
    if ((sint32)count >= (sint32)(random & 127) && (sint32)count > 0)
        td5_8003ef50(r_s32(car + 0x2F0));
    index = r_u8(car + 0x34D);
    return td5_80045980(0x8009B3E0 + index * 12);
}

uint32 td5_80044604(void)
{
    VECTOR input;
    VECTOR output;
    MATRIX *matrix;
    uint32 offset;
    uint32 base;
    uint32 record;
    uint32 value;
    uint32 origin;
    uint32 color;
    uint32 channel;
    uint32 channel_offset;
    uint32 buffer_index;
    uint32 packet;
    uint32 buffer;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 vertex;
    uint32 shift;
    uint32 ordering;
    uint32 tag;
    uint32 link;
    sint32 age;
    sint32 first_clip;
    sint32 second_clip;
    sint32 depth;

    FUNCTION_MARKER(0x80044604, "SLUS_006.10");
    for (offset = 0; offset < 10560; offset += 132)
    {
        base = r_u32(0x80088774);
        record = base + offset;
        if ((r_u8(record + 112) & 2) == 0)
            continue;
        value = r_u16(record + 114) + 1;
        w_u16(record + 114, value);
        age = (sint16)value;
        if (age >= 601)
            w_u8(record + 112, 0);
        record = r_u32(0x80088774) + offset;
        age = r_s16(record + 114);
        if (age >= 301)
        {
            if ((age & 8) == 0)
            {
                color = r_u8(record + 113);
                if (color != 0)
                    w_u8(record + 113, color - 1);
            }
            for (channel = 0; channel < 3; ++channel)
            {
                base = r_u32(0x80088774);
                buffer_index = r_u32(0x80088104);
                record = base + offset;
                color = r_u8(record + 113);
                channel_offset = channel == 0 ? 37 : (channel == 1 ? 36 : 38);
                w_u8(record + buffer_index * 40 + channel_offset, color);
            }
        }
        record = r_u32(0x80088774) + offset;
        value = r_u32(record + 116);
        origin = r_u32(0x8009765C);
        input.vx = (sint32)((uint32)((sint32)value >> 8) - origin);
        value = r_u32(record + 120);
        origin = r_u32(0x80097660);
        input.vy = (sint32)((uint32)((sint32)value >> 8) - origin);
        value = r_u32(record + 124);
        origin = r_u32(0x80097664);
        input.vz = (sint32)((uint32)((sint32)value >> 8) - origin);
        matrix = (MATRIX *)psx_addr(0x80097648, 20);
        SetRotMatrix(matrix);
        ApplyMatrixLV(matrix, &input, &output);
        w_u32(0x80092224, (uint32)output.vx);
        w_u32(0x80092228, (uint32)output.vy);
        w_u32(0x8009222C, (uint32)output.vz);
        x = r_u32(0x80092224);
        y = r_u32(0x80092228);
        xport_gte_write_control(5, x);
        z = r_u32(0x8009222C);
        xport_gte_write_control(6, y);
        xport_gte_write_control(7, z);
        record = r_u32(0x80088774) + offset;
        for (vertex = 0; vertex < 6; ++vertex)
            xport_gte_write_data(vertex, r_u32(record + vertex * 4));
        xport_gte_execute(0x280030);
        xport_gte_execute(0x1400006);
        first_clip = (sint32)xport_gte_read_data(24);
        base = r_u32(0x80088774);
        buffer_index = r_u32(0x80088104);
        packet = base + offset + buffer_index * 40 + 32;
        w_u32(packet + 8, xport_gte_read_data(12));
        w_u32(packet + 16, xport_gte_read_data(13));
        w_u32(packet + 24, xport_gte_read_data(14));
        record = r_u32(0x80088774) + offset;
        xport_gte_write_data(0, r_u32(record + 24));
        xport_gte_write_data(1, r_u32(record + 28));
        xport_gte_execute(0x180001);
        xport_gte_execute(0x1400006);
        second_clip = (sint32)xport_gte_read_data(24);
        xport_gte_execute(0x168002E);
        if (first_clip > 0 && second_clip < 0)
            continue;
        value = xport_gte_read_data(7);
        base = r_u32(0x80088774);
        buffer_index = r_u32(0x80088104);
        w_u32(base + offset + buffer_index * 40 + 64, xport_gte_read_data(14));
        shift = r_u32(0x80088110);
        shift -= r_u32(0x80088164);
        depth = (sint32)(value + 64) >> (shift & 31);
        if (depth < 65)
            continue;
        if (depth >= r_s32(0x8008857C))
            continue;
        buffer_index = r_u32(0x80088104);
        base = r_u32(0x80088774);
        buffer = r_u32(0x800880F0);
        ordering = r_u32(buffer + 0x70);
        packet = base + offset + buffer_index * 40 + 32;
        tag = r_u32(packet);
        link = r_u32(ordering + (uint32)depth * 4);
        w_u32(packet, (tag & 0xFF000000) | (link & 0xFFFFFF));
        ordering = r_u32(buffer + 0x70);
        tag = r_u32(ordering + (uint32)depth * 4);
        w_u32(ordering + (uint32)depth * 4, (tag & 0xFF000000) | (packet & 0xFFFFFF));
    }
    return 0;
}

uint32 td5_800449a0(void)
{
    uint32 index;
    uint32 record;

    FUNCTION_MARKER(0x800449A0, "SLUS_006.10");
    index = r_u32(0x80088604);
    if ((sint32)index < 80)
    {
        record = r_u32(0x80088774) + index * 132;
        do
        {
            if ((r_u8(record + 112) & 1) == 0)
            {
                w_u32(0x80088604, index);
                return index;
            }
            ++index;
            record += 132;
        } while ((sint32)index < 80);
    }
    index = 0;
    record = r_u32(0x80088774);
    do
    {
        if ((r_u8(record + 112) & 1) == 0)
        {
            w_u32(0x80088604, index);
            return index;
        }
        ++index;
        record += 132;
    } while (index < 80);
    return 0;
}

uint32 td5_80043db8(void)
{
    uint32 strip;
    uint32 offset;
    uint32 car_index;
    uint32 wheel;
    uint32 width;
    uint32 language;
    uint32 selected;
    uint32 selected_offset;
    uint32 next;
    uint32 next_offset;
    uint32 car_offset;
    uint32 base;
    uint32 record;
    uint32 old_record;
    uint32 flags;
    uint32 value;
    uint32 origin;
    uint32 component;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 angle;
    uint32 cosine;
    uint32 sine;
    uint32 left_x;
    uint32 left_z;
    uint32 right_x;
    uint32 right_z;
    uint32 saved_angle;
    uint32 current_color;
    uint32 target_color;
    uint32 coordinate;
    uint32 old_origin;
    uint32 new_origin;
    sint32 speed;
    sint32 age_limit;
    sint32 color_difference;

    FUNCTION_MARKER(0x80043DB8, "SLUS_006.10");
    for (strip = 0; strip < 8; ++strip)
    {
        offset = strip * 7;
        if (r_u8(0x8009D8D2 + offset) == 0)
            continue;
        car_index = r_u8(0x8009D8D0 + offset);
        width = r_u8(0x8009D8D4 + offset);
        language = r_u8(0x8007EA00);
        wheel = r_u8(0x8009D8D1 + offset);
        if (language == 1)
            width = 10;
        value = r_u32(0x8009878C + car_index * 864);
        selected = r_u8(0x8009D8D3 + offset);
        if ((sint32)value < 0)
            value = 0 - value;
        speed = (sint32)value >> 15;
        if (speed <= 0)
            speed = 1;
        else if (speed >= 4)
            speed = 3;
        age_limit = 3 - speed;
        base = r_u32(0x80088774);
        car_offset = car_index * 864 + wheel * 12;
        selected_offset = selected * 132;
        record = base + selected_offset;
        value = r_u32(0x80098580 + car_offset);
        origin = r_u32(record + 116);
        x = value - origin;
        value = r_u32(0x80098584 + car_offset);
        origin = r_u32(record + 120);
        y = value - origin;
        value = r_u32(0x80098588 + car_offset);
        origin = r_u32(record + 124);
        z = value - origin;
        x = (uint32)((sint32)x >> 8);
        y = (uint32)((sint32)y >> 8);
        z = (uint32)((sint32)z >> 8);
        angle = (uint32)td5_8007b0c0((sint32)x, (sint32)z);
        cosine = (uint32)rcos((sint32)angle);
        sine = (uint32)rsin((sint32)angle);
        base = r_u32(0x80088774);
        record = base + selected_offset;
        flags = r_u8(record + 112);
        value = (0 - width) * cosine;
        left_x = (uint32)((sint32)value / 4096);
        value = 0 - ((0 - width) * sine);
        left_z = (uint32)((sint32)value / 4096);
        value = width * cosine;
        right_x = (uint32)((sint32)value / 4096);
        value = 0 - width * sine;
        right_z = (uint32)((sint32)value / 4096);
        if ((flags & 2) == 0 && (x != 0 || z != 0))
        {
            flags = r_u8(record + 112);
            w_u8(record + 112, flags | 2);
            base = r_u32(0x80088774);
            w_u32(record + 128, angle);
            record = base + selected_offset;
            w_u16(record + 16, left_x);
            w_u16(record + 20, left_z);
            w_u16(record + 18, 0);
            w_u16(record + 24, right_x);
            w_u16(record + 28, right_z);
            w_u16(record + 26, 0);
        }
        base = r_u32(0x80088774);
        record = base + selected_offset;
        w_u16(record, x + left_x);
        w_u16(record + 4, z + left_z);
        w_u16(record + 2, y);
        w_u16(record + 8, x + right_x);
        saved_angle = r_u32(record + 128);
        w_u16(record + 12, z + right_z);
        w_u16(record + 10, y);
        if (((sint32)saved_angle >> 7) == ((sint32)angle >> 7) && age_limit >= r_s16(record + 114))
            continue;
        next = td5_800449a0();
        w_u8(0x8009D8D3 + offset, next);
        current_color = r_u8(0x8009D8D6 + offset);
        target_color = r_u8(0x8009D8D5 + offset);
        color_difference = (sint32)current_color - (sint32)target_color;
        if (color_difference < 0)
            w_u8(0x8009D8D6 + offset, current_color + 1);
        if (color_difference > 0)
        {
            current_color = r_u8(0x8009D8D6 + offset);
            w_u8(0x8009D8D6 + offset, current_color - 1);
        }
        base = r_u32(0x80088774);
        record = base + selected_offset;
        flags = r_u8(record + 112);
        w_u8(record + 112, flags & 2);
        base = r_u32(0x80088774);
        next_offset = next * 132;
        record = base + next_offset;
        value = r_u32(0x80098580 + car_offset);
        w_u32(record + 116, value);
        value = r_u32(0x80098584 + car_offset);
        w_u32(record + 120, value);
        value = r_u32(0x80098588 + car_offset);
        w_u8(record + 112, 3);
        base = r_u32(0x80088774);
        w_u32(record + 124, value);
        current_color = r_u8(0x8009D8D6 + offset);
        w_u8(base + next_offset + 36, current_color);
        base = r_u32(0x80088774);
        current_color = r_u8(0x8009D8D6 + offset);
        w_u8(base + next_offset + 37, current_color);
        base = r_u32(0x80088774);
        current_color = r_u8(0x8009D8D6 + offset);
        w_u8(base + next_offset + 38, current_color);
        base = r_u32(0x80088774);
        current_color = r_u8(0x8009D8D6 + offset);
        w_u8(base + next_offset + 76, current_color);
        base = r_u32(0x80088774);
        current_color = r_u8(0x8009D8D6 + offset);
        w_u8(base + next_offset + 77, current_color);
        base = r_u32(0x80088774);
        current_color = r_u8(0x8009D8D6 + offset);
        w_u8(base + next_offset + 78, current_color);
        base = r_u32(0x80088774);
        record = base + next_offset;
        w_u32(record + 128, angle);
        current_color = r_u8(0x8009D8D6 + offset);
        w_u8(record + 113, current_color);
        base = r_u32(0x80088774);
        record = base + next_offset;
        old_record = base + selected_offset;
        w_u16(record + 114, 0);
        for (component = 0; component < 4; ++component)
        {
            coordinate = component * 4;
            origin = (component & 1) == 0 ? 116 : 124;
            old_origin = r_u32(old_record + origin);
            value = r_u16(old_record + coordinate);
            new_origin = r_u32(record + origin);
            value += (uint32)((sint32)old_origin >> 8) - (uint32)((sint32)new_origin >> 8);
            w_u16(record + coordinate + 16, value);
            w_u16(record + coordinate, value);
        }
    }
    return 0;
}

uint32 td5_8003f9e4(uint32 position, uint32 original_stack)
{
    uint32 texture_index;
    uint32 effect_index;
    uint32 slot;
    uint32 index;
    uint32 entry;
    uint32 offset;
    uint32 magnitude;
    uint32 angle;
    uint32 x_delta;
    uint32 z_delta;
    uint32 value;
    uint32 copy;
    uint32 chunk;
    uint32 destination;
    uint32 first;
    uint32 second;
    uint32 third;
    uint32 fourth;
    uint32 lifetime;
    sint32 random;
    sint32 horizontal_step;
    sint32 vertical_step;

    FUNCTION_MARKER(0x8003F9E4, "SLUS_006.10");
    texture_index = original_stack - 0x20;
    effect_index = original_stack - 0x1C;
    if (td5_8003c258(texture_index, effect_index) == 1)
    {
        slot = r_u32(effect_index);
        index = r_u32(texture_index);
        entry = 0x8009EFB0 + slot * 68;
        w_u8(entry + 1, index);
        random = randPSX();
        angle = r_u32(0x8008829C);
        magnitude = ((uint32)(random % 768) + 256) << 8;
        value = (uint32)rcos((sint32)angle) * magnitude;
        x_delta = (uint32)((sint32)value / 4096);
        angle = r_u32(0x8008829C);
        value = (uint32)rsin((sint32)angle) * magnitude;
        z_delta = (uint32)((sint32)value / 4096);
        for (copy = 0; copy < 2; ++copy)
        {
            index = r_u32(texture_index);
            destination = (copy == 0 ? 0x8008C518 : 0x8008C838) + index * 40;
            for (chunk = 0; chunk < 32; chunk += 16)
            {
                first = r_u32(0x8009EC48 + chunk);
                second = r_u32(0x8009EC4C + chunk);
                third = r_u32(0x8009EC50 + chunk);
                fourth = r_u32(0x8009EC54 + chunk);
                w_u32(destination + chunk, first);
                w_u32(destination + chunk + 4, second);
                w_u32(destination + chunk + 8, third);
                w_u32(destination + chunk + 12, fourth);
            }
            first = r_u32(0x8009EC68);
            second = r_u32(0x8009EC6C);
            w_u32(destination + 32, first);
            w_u32(destination + 36, second);
        }
        slot = r_u32(effect_index);
        w_u8(0x8009EFCF + slot * 68, 0x80);
        slot = r_u32(effect_index);
        offset = slot * 68;
        w_u32(0x8009EFF0 + offset, 0x8003F8A8);
        w_u32(0x8009EFEC + offset, 0x8003C99C);
        value = r_u32(position);
        w_u32(0x8009EFD0 + offset, value + x_delta);
        value = r_u32(position + 4);
        w_u32(0x8009EFD4 + offset, value);
        value = r_u32(position + 8);
        w_u32(0x8009EFD8 + offset, value + z_delta);
        w_u8(entry + 3, 1);
        random = randPSX();
        lifetime = ((uint32)(random % 5) + 2) * 30;
        horizontal_step = td5_divide_checked(0x7000, (sint32)(lifetime & 0xFE));
        w_u8(entry + 2, lifetime);
        value = r_u8(entry + 2);
        vertical_step = td5_divide_checked(0x2580, (sint32)value);
        w_u16(entry + 4, 0x8000);
        w_u16(entry + 8, 0x1400);
        w_u32(entry + 12, (uint32)((sint32)x_delta >> 3));
        w_u32(entry + 16, 0x4000);
        w_u32(entry + 20, (uint32)((sint32)z_delta >> 3));
        w_u16(entry + 6, 0 - (uint32)horizontal_step);
        w_u16(entry + 10, (uint32)vertical_step);
    }
    random = randPSX();
    value = r_u32(0x8008829C);
    value += 128 + (uint32)(random % 32);
    w_u32(0x8008829C, value);
    return value;
}

uint32 td5_8003c258(uint32 texture_index, uint32 effect_index)
{
    uint32 slot;
    uint32 index;

    FUNCTION_MARKER(0x8003C258, "SLUS_006.10");
    for (slot = 0; slot < 50; ++slot)
    {
        if (r_u8(0x8009EFCF + slot * 68) == 0)
            break;
    }
    if (slot == 50)
        return 0;
    for (index = 0; index < 20; ++index)
    {
        if (r_s8(0x8009EC00 + index) == 0)
            break;
    }
    if (index == 20)
        return 0;
    w_u8(0x8009EC00 + index, 1);
    w_u32(texture_index, index);
    w_u32(effect_index, slot);
    return 1;
}

uint32 td5_8003f724(uint32 slot, uint32 original_stack)
{
    uint32 owner;
    uint32 offset;
    uint32 point;
    uint32 value;

    FUNCTION_MARKER(0x8003F724, "SLUS_006.10");
    owner = r_u32(0x800886F8);
    td5_8001b310(0x800A5088, owner + 0x10);
    value = r_u32(0x80088108);
    if ((value & 15) != 0)
        return slot << 4;
    offset = slot * 68;
    point = original_stack - 0x20;
    value = r_u32(0x8009EFD0 + offset);
    w_u32(point, value);
    value = r_u32(0x8009EFD8 + offset);
    w_u32(point + 8, value);
    td5_8001cd00(0x800A5088, point, point + 4);
    return td5_8003f9e4(point, original_stack - 0x30);
}

uint32 td5_8003c010(uint32 slot)
{
    uint32 buffer_index;
    uint32 offset;
    uint32 index;
    uint32 packet;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 value;
    uint32 shift;
    uint32 buffer;
    uint32 ordering;
    uint32 tag;
    uint32 link;
    sint32 depth;

    FUNCTION_MARKER(0x8003C010, "SLUS_006.10");
    buffer_index = r_u32(0x80088104);
    offset = slot * 68;
    index = r_u8(0x8009EFB0 + offset);
    packet = 0x8009EC70 + buffer_index * 400 + index * 20;
    x = r_u32(0x8009EFDC + offset);
    y = r_u32(0x8009EFE0 + offset);
    z = r_u32(0x8009EFE4 + offset);
    xport_gte_write_control(5, x);
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    xport_gte_write_data(0, r_u32(0x800879A4));
    xport_gte_write_data(1, r_u32(0x800879A8));
    xport_gte_execute(0x180001);
    x = r_u16(packet + 16);
    y = r_u16(packet + 18);
    w_u16(packet + 8, x);
    w_u16(packet + 10, y);
    value = xport_gte_read_control(31);
    if ((value & 0x1C66000) != 0)
        return packet + 16;
    w_u32(packet + 16, xport_gte_read_data(14));
    depth = (sint32)xport_gte_read_data(19) >> 2;
    value = r_u32(0x80088110);
    shift = value - r_u32(0x80088164);
    depth >>= shift & 31;
    if (depth <= 0)
        return shift;
    if (depth >= r_s32(0x8008857C))
        return 0;
    buffer = r_u32(0x800880F0);
    ordering = r_u32(buffer + 0x70);
    tag = r_u32(packet);
    offset = (uint32)depth * 4;
    link = r_u32(ordering + offset);
    w_u32(packet, (tag & 0xFF000000) | (link & 0xFFFFFF));
    ordering = r_u32(buffer + 0x70);
    tag = r_u32(ordering + offset);
    value = (tag & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(ordering + offset, value);
    return value;
}

uint32 td5_8003c190(uint32 slot)
{
    uint32 offset;
    uint32 value;
    uint32 velocity;
    uint32 timer;
    uint32 index;

    FUNCTION_MARKER(0x8003C190, "SLUS_006.10");
    offset = slot * 68;
    value = r_u32(0x8009EFD0 + offset);
    velocity = r_u32(0x8009EFB8 + offset);
    w_u32(0x8009EFD0 + offset, value + velocity);
    value = r_u32(0x8009EFD4 + offset);
    velocity = r_u32(0x8009EFBC + offset);
    w_u32(0x8009EFD4 + offset, value + velocity);
    value = r_u32(0x8009EFD8 + offset);
    velocity = r_u32(0x8009EFC0 + offset);
    w_u32(0x8009EFD8 + offset, value + velocity);
    velocity = r_u32(0x8009EFBC + offset);
    timer = r_u8(0x8009EFB1 + offset);
    w_u32(0x8009EFBC + offset, velocity - 500);
    value = timer + 255;
    w_u8(0x8009EFB1 + offset, value);
    if (timer == 0)
    {
        index = r_u8(0x8009EFB0 + offset);
        w_u8(0x8009D800 + index, 0xFD);
        w_u8(0x8009EFCF + offset, 0);
        return 0xFFFFFFFD;
    }
    return value;
}

uint32 td5_8003b88c(void)
{
    VECTOR input;
    VECTOR output;
    uint32 slot;
    uint32 offset;
    uint32 flags;
    uint32 value;
    uint32 origin;
    sint32 depth;
    uint32 visible;

    FUNCTION_MARKER(0x8003B88C, "SLUS_006.10");
    SetRotMatrix((MATRIX *)psx_addr(0x80097648, 20));
    for (slot = 0; slot < 50; ++slot)
    {
        offset = slot * 68;
        flags = r_u8(0x8009EFCF + offset);
        if ((flags & 0x80) != 0 && (flags & 0x20) == 0)
        {
            value = r_u32(0x8009EFD0 + offset);
            origin = r_u32(0x8009765C);
            input.vx = (sint32)((uint32)((sint32)value >> 8) - origin);
            value = r_u32(0x8009EFD4 + offset);
            origin = r_u32(0x80097660);
            input.vy = (sint32)((uint32)((sint32)value >> 8) - origin);
            value = r_u32(0x8009EFD8 + offset);
            origin = r_u32(0x80097664);
            input.vz = (sint32)((uint32)((sint32)value >> 8) - origin);
            ApplyRotMatrixLV(&input, &output);
            w_u32(0x8009EFDC + offset, (uint32)output.vx);
            w_u32(0x8009EFE0 + offset, (uint32)output.vy);
            w_u32(0x8009EFE4 + offset, (uint32)output.vz);
            depth = r_s32(0x8009EFE4 + offset);
            visible = 0;
            if (r_s32(0x8008813C) < depth && depth < r_s32(0x80088140))
                visible = 1;
            flags = r_u8(0x8009EFCF + offset);
            w_u8(0x8009EFCF + offset, visible != 0 ? flags | 0x40 : flags & 0xBF);
        }
    }
    return 0;
}

uint32 td5_8003bbb0(void)
{
    const uint32 starts[3] = {0x8009EC00, 0x8009D800, 0x8009D970};
    uint32 group;
    uint32 index;
    sint32 value;

    FUNCTION_MARKER(0x8003BBB0, "SLUS_006.10");
    for (group = 0; group < 3; ++group)
    {
        for (index = 0; index < 20; ++index)
        {
            value = r_s8(starts[group] + index);
            if (value < 0)
                w_u8(starts[group] + index, (uint32)(value + 1));
        }
    }
    return 0;
}

void td5_80053028(sint32 value)
{
    FUNCTION_MARKER(0x80053028, "SLUS_006.10");
}

uint32 td5_80047278(uint32 slot, sint32 digit, uint32 red, uint32 green, uint32 blue)
{
    uint32 width;
    uint32 origin_x;
    uint32 height;
    uint32 origin_y;
    uint32 buffer_index;
    uint32 packet;
    uint32 texture;
    uint32 corner;
    uint32 value;
    uint32 uv;
    uint32 x;
    uint32 y;
    uint32 attribute;
    uint32 opcode;
    uint32 buffer;
    uint32 ordering;
    uint32 tag;
    uint32 link;

    FUNCTION_MARKER(0x80047278, "SLUS_006.10");
    blue &= 255;
    width = r_u32(0x80088438);
    origin_x = r_u32(0x80088510);
    height = r_u32(0x800884F8);
    origin_y = r_u32(0x80088518);
    buffer_index = r_u32(0x80088104);
    packet = 0x8009DA98 + slot * 80 + buffer_index * 40;
    w_u8(packet + 3, 9);
    w_u8(packet + 7, 44);
    for (corner = 0; corner < 7; ++corner)
    {
        texture = r_u32(0x80088534);
        if ((corner & 1) == 0)
        {
            value = r_u8(texture + 14) + ((uint32)digit << 4);
            if ((corner & 2) != 0)
                value += 15;
        }
        else
        {
            value = r_u8(texture + 15);
            if (corner >= 5)
                value += 23;
        }
        w_u8(packet + 12 + (corner / 2) * 8 + (corner & 1), value);
    }
    texture = r_u32(0x80088534);
    uv = r_u8(texture + 15);
    y = (uint32)((sint32)height / 3) + origin_y;
    x = (uint32)((sint32)width / 2) + origin_x;
    w_u16(packet + 8, x - 8);
    w_u8(packet + 37, uv + 23);
    w_u16(packet + 10, y - 12);
    texture = r_u32(0x80088534);
    w_u16(packet + 16, x + 7);
    w_u16(packet + 18, y - 12);
    w_u16(packet + 24, x - 8);
    w_u16(packet + 26, y + 11);
    w_u16(packet + 32, x + 7);
    w_u16(packet + 34, y + 11);
    value = r_u16(texture + 8);
    w_u16(packet + 22, value);
    attribute = r_u16(texture + 10);
    opcode = r_u8(packet + 7);
    w_u8(packet + 4, red);
    w_u8(packet + 5, green);
    w_u8(packet + 6, blue);
    w_u8(packet + 7, opcode & 0xFC);
    buffer = r_u32(0x800880F0);
    w_u16(packet + 14, attribute);
    ordering = r_u32(buffer + 0x70);
    tag = r_u32(packet);
    link = r_u32(ordering + 8);
    w_u32(packet, (tag & 0xFF000000) | (link & 0xFFFFFF));
    ordering = r_u32(buffer + 0x70);
    tag = r_u32(ordering + 8);
    value = (tag & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(ordering + 8, value);
    return value;
}

uint32 td5_80047790(void)
{
    uint32 view;
    uint32 offset;
    uint32 value;
    uint32 first;
    uint32 second;
    sint32 counter;

    FUNCTION_MARKER(0x80047790, "SLUS_006.10");
    view = r_u32(0x800885F8);
    offset = view * 4;
    counter = r_s32(0x800886E0 + offset);
    if (counter == 0)
        return offset;
    value = r_u32(0x80087734);
    if (value != 0)
        return value;
    if (counter % 40 == 0 && view == 0)
        td5_80053028(r_s32(0x800886E0) / 40);
    view = r_u32(0x800885F8);
    offset = view * 4;
    value = r_u32(0x800886E0 + offset) - 1;
    w_u32(0x800886E0 + offset, value);
    if ((sint32)value / 40 == 0)
    {
        first = 32;
        second = 128;
    }
    else
    {
        first = 128;
        second = 32;
    }
    view = r_u32(0x800885F8);
    counter = r_s32(0x800886E0 + view * 4);
    td5_80047278(0, counter / 40, first, second, 32);
    view = r_u32(0x800885F8);
    offset = view * 4;
    if (r_s32(0x800886E0 + offset) >= 40)
        return 0;
    value = r_u32(0x800886E0 + offset);
    w_u32(0x8008873C, 0);
    w_u32(0x80088618, 0);
    if (value != 0 || view != 0)
        return value;
    value = r_u32(0x80087730);
    if (value != 0)
        return value;
    value = r_u32(0x8008772C);
    if (value != 0)
        return value;
    return (uint32)td5_8005490c(13, 20, r_s32(0x80087700), 0, 0x1400);
}

uint32 td5_80079c14(uint32 object)
{
    VECTOR input;
    VECTOR output;
    MATRIX *matrix;
    uint32 x;
    uint32 y;
    uint32 z;

    FUNCTION_MARKER(0x80079C14, "SLUS_006.10");
    input.vx = r_s32(object + 0x10) >> 8;
    input.vy = r_s32(object + 0x14) >> 8;
    input.vz = r_s32(object + 0x18) >> 8;
    matrix = (MATRIX *)psx_addr(0x80097648, 20);
    SetRotMatrix(matrix);
    ApplyMatrixLV(matrix, &input, &output);
    w_u32(0x80092224, (uint32)output.vx);
    w_u32(0x80092228, (uint32)output.vy);
    w_u32(0x8009222C, (uint32)output.vz);
    x = r_u32(0x80092224);
    y = r_u32(0x80092228);
    xport_gte_write_control(5, x);
    z = r_u32(0x8009222C);
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    return 0x80092224;
}

uint32 td5_80079fd0(uint32 object)
{
    VECTOR input;
    VECTOR output;
    MATRIX *matrix;
    uint32 value;
    uint32 origin;
    uint32 shift;
    uint32 depth;
    uint32 x;
    uint32 y;
    uint32 coefficient;
    uint32 depth_term;
    sint32 radius;
    sint32 near_plane;
    sint32 far_plane;

    FUNCTION_MARKER(0x80079FD0, "SLUS_006.10");
    value = r_u32(object + 0x10);
    origin = r_u32(object + 0x1C);
    radius = r_s16(object + 8);
    input.vx = (sint32)((uint32)((sint32)value >> 8) + origin);
    value = r_u32(object + 0x14);
    origin = r_u32(object + 0x20);
    input.vy = (sint32)((uint32)((sint32)value >> 8) + origin);
    value = r_u32(object + 0x18);
    origin = r_u32(object + 0x24);
    shift = r_u32(0x80088164);
    input.vz = (sint32)((uint32)((sint32)value >> 8) + origin);
    radius >>= shift & 31;
    matrix = (MATRIX *)psx_addr(0x80097648, 20);
    SetRotMatrix(matrix);
    ApplyMatrixLV(matrix, &input, &output);
    depth = (uint32)output.vz;
    near_plane = r_s32(0x8008813C);
    shift = r_u32(0x80088164);
    near_plane >>= shift & 31;
    if (near_plane >= (sint32)(depth + (uint32)radius))
        return 0;
    far_plane = r_s32(0x80088140) >> (shift & 31);
    if ((sint32)(depth - (uint32)radius) >= far_plane)
        return 0;
    coefficient = (uint32)r_s16(0x80088490);
    x = (uint32)output.vx;
    depth_term = depth * (uint32)r_s16(0x80088494);
    value = x * coefficient + depth_term;
    if (((sint32)value >> 12) >= radius)
        return 0;
    value = (0 - x) * coefficient + depth_term;
    if (((sint32)value >> 12) >= radius)
        return 0;
    coefficient = (uint32)r_s16(0x800884AA);
    y = (uint32)output.vy;
    depth_term = depth * (uint32)r_s16(0x800884AC);
    value = y * coefficient + depth_term;
    if (((sint32)value >> 12) >= radius)
        return 0;
    value = (0 - y) * coefficient + depth_term;
    if (((sint32)value >> 12) >= radius)
        return 0;
    w_u32(0x800883D4, depth);
    return 1;
}

uint32 td5_8003a1b4(sint32 index)
{
    uint32 owner;
    uint32 range;
    uint32 lower;
    uint32 selection;
    uint32 group;
    uint32 item;
    uint32 object;
    uint32 table;
    sint32 first;
    sint32 node;

    FUNCTION_MARKER(0x8003A1B4, "SLUS_006.10");
    if (index < 0)
        selection = 1;
    else
    {
        owner = r_u32(0x80088770);
        range = r_u32(owner + 0x34);
        first = r_s16(range);
        node = r_s32(0x80088720 + (uint32)index * 4);
        lower = r_u16(range);
        selection = 0;
        while (first >= 0)
        {
            if (r_s16(range + 2) < node)
                break;
            if ((sint16)lower < node)
            {
                selection = r_u32(range + 4);
                break;
            }
            range += 8;
            first = r_s16(range);
            lower = r_u16(range);
        }
    }
    for (group = 0; group < 3; ++group)
    {
        table = 0x8007E188 + group * 48 + selection * 16;
        for (item = 0; item < 6; ++item)
        {
            owner = r_u32(0x80088710);
            object = r_u32(owner + 0x10 + group * 24 + item * 4);
            if (td5_80079fd0(object) != 0)
            {
                td5_80079c14(object);
                td5_80071e0c(object, table);
            }
        }
    }
    return 0;
}

uint32 td5_800415d0(uint32 slot, uint32 car_index)
{
    uint32 car;
    uint32 offset;
    uint32 value;
    uint32 first;
    uint32 second;
    sint32 amplitude;
    sint32 divisor;
    sint32 quotient;

    FUNCTION_MARKER(0x800415D0, "SLUS_006.10");
    if (r_u32(0x800876F0) == 0 || r_u32(0x80087738) != 0)
        return td5_8006b9e0(slot, 0, 0);
    value = r_u32(0x80098788 + car_index * 864);
    car = 0x800984A0 + car_index * 864;
    amplitude = 80 - (sint32)value / 128;
    if (amplitude < 0)
        amplitude = 0;
    else if (amplitude >= 256)
        amplitude = 255;
    offset = car_index * 4;
    divisor = r_s32(0x80088480 + offset);
    if (divisor != 0)
    {
        quotient = td5_divide_checked(r_s32(car + 0x2EC), divisor);
        if (amplitude < quotient)
            amplitude = quotient;
    }
    if (amplitude < 0)
        second = 0;
    else if (amplitude >= 256)
        second = 255;
    else
        second = (uint32)amplitude;
    value = r_u32(0x80088480 + offset);
    if ((sint32)value < 8193 && value != 0)
        w_u32(0x80088480 + offset, value + 1024);
    else
        w_u32(0x80088480 + offset, 0);
    first = 0;
    divisor = r_s32(0x800885B0 + offset);
    if (divisor != 0)
    {
        quotient = td5_divide_checked(r_s32(car + 0x2EC), divisor);
        if (quotient < 0)
            first = 0;
        else if (quotient >= 256)
            first = 255;
        else
            first = (uint32)quotient;
        first |= 1;
    }
    value = r_u32(0x800885B0 + offset);
    if ((sint32)value < 8193 && value != 0)
        w_u32(0x800885B0 + offset, value + 1024);
    else
        w_u32(0x800885B0 + offset, 0);
    return td5_8006b9e0(slot, first, second);
}

uint32 td5_80036f48(void)
{
    uint32 selected;
    uint32 value;
    uint32 digits[3];
    uint32 position;
    uint32 corner;
    uint32 texture;
    uint32 offset;
    uint32 coordinate;
    uint32 destination;
    uint32 buffer;
    uint32 packet;
    sint32 timer;

    FUNCTION_MARKER(0x80036F48, "SLUS_006.10");
    selected = r_u32(0x80088498);
    value = r_u16(0x800987BC + selected * 864) >> 8;
    digits[0] = value / 100;
    value %= 100;
    digits[1] = value / 10;
    digits[2] = value % 10;
    for (position = 0; position < 3; ++position)
    {
        for (corner = 0; corner < 8; ++corner)
        {
            if (position == 0 && corner < 7)
            {
                texture = r_u32(0x80088224);
                offset = r_u32(0x80088104) * 120;
            }
            else
            {
                offset = r_u32(0x80088104) * 120;
                texture = r_u32(0x80088224);
            }
            if ((corner & 1) == 0)
            {
                coordinate = r_u8(texture + 14) + digits[position] * 16;
                if ((corner & 2) != 0)
                    coordinate += 15;
            }
            else
            {
                coordinate = r_u8(texture + 15);
                if (corner >= 5)
                    coordinate += 23;
            }
            destination = 0x80088860 + position * 40 + offset + 12 + (corner / 2) * 8 + (corner & 1);
            w_u8(destination, coordinate);
        }
    }
    timer = r_s16(0x800878C8);
    if (timer == 0 || (r_u8(0x80088220) & 4) == 0)
    {
        buffer = r_u32(0x800880F0);
        offset = r_u32(0x80088104) * 120;
        packet = 0x80088860 + offset;
        for (position = 0; position < 3; ++position)
        {
            td5_minimap_link(buffer, packet);
            packet += 40;
        }
    }
    timer = r_s16(0x800878C8);
    if (timer > 0)
        w_u16(0x800878C8, (uint32)timer - 1);
    value = r_u8(0x80088220) + 1;
    w_u8(0x80088220, value);
    return value;
}

uint32 td5_800374b8(uint32 car)
{
    uint32 model;
    uint32 value;
    uint32 angle;
    uint32 offset;
    uint32 next_index;
    uint32 first;
    uint32 second;
    uint32 radius;
    uint32 pass;
    uint32 y0;
    uint32 y1;
    uint32 y2;
    uint32 buffer;
    uint32 ordering;
    uint32 tag;
    uint32 link;
    uint32 texture;
    uint32 gear;
    uint32 u;
    uint32 v;
    uint32 height;
    uint32 packet;
    uint32 position;
    uint32 digit_offset;
    sint32 speed;
    sint32 quotient;
    sint32 digit;

    FUNCTION_MARKER(0x800374B8, "SLUS_006.10");
    model = r_u32(car + 0x184);
    value = r_u32(car + 0x2E8);
    speed = r_s16(model + 0x72);
    angle = (uint32)td5_divide_checked((sint32)(value * 3072), speed);
    offset = r_u32(0x80088104) * 20;
    first = 0x80088950 + offset;
    second = 0x800889A0 + offset;
    for (pass = 0; pass < 2; ++pass)
    {
        radius = pass == 0 ? 56 : 28;
        value = (uint32)rcos((sint32)angle) * radius;
        value = 436 - (uint32)((sint32)value / 4096);
        w_u16(first + 8, value);
        w_u16(second + 8, value);
        value = (uint32)rsin((sint32)angle) * radius;
        value = 187 - (uint32)((sint32)value / 6144);
        w_u16(first + 10, value);
        w_u16(second + 10, value);
        value = (uint32)rsin((sint32)angle) * 3;
        value = (uint32)((sint32)value / 4096) + 436;
        w_u16(first + 12, value);
        w_u16(second + 12, value);
        value = (uint32)rcos((sint32)angle);
        value = 187 - (uint32)((sint32)value / 2048);
        w_u16(first + 14, value);
        w_u16(second + 14, value);
        value = (uint32)rsin((sint32)angle) * 3;
        value = 436 - (uint32)((sint32)value / 4096);
        w_u16(first + 16, value);
        w_u16(second + 16, value);
        value = (uint32)rcos((sint32)angle);
        value = (uint32)((sint32)value / 2048) + 187;
        w_u16(first + 18, value);
        w_u16(second + 18, value);
        y0 = r_u16(second + 10);
        y1 = r_u16(second + 14);
        y2 = r_u16(second + 18);
        buffer = r_u32(0x800880F0);
        w_u16(second + 10, y0 + 2);
        w_u16(second + 14, y1 + 2);
        w_u16(second + 18, y2 + 2);
        td5_minimap_link(buffer, first);
        ordering = r_u32(buffer + 0x70);
        if (pass == 0)
        {
            angle += 2048;
            next_index = r_u32(0x80088104);
        }
        tag = r_u32(second);
        link = r_u32(ordering + 4);
        w_u32(second, (tag & 0xFF000000) | (link & 0xFFFFFF));
        ordering = r_u32(buffer + 0x70);
        tag = r_u32(ordering + 4);
        if (pass == 1)
            next_index = r_u32(0x80088104);
        w_u32(ordering + 4, (tag & 0xFF000000) | (second & 0xFFFFFF));
        offset = next_index * 20;
        first = 0x80088978 + offset;
        second = 0x800889C8 + offset;
    }
    texture = r_u32(0x8008822C);
    gear = r_u8(car + 0x343);
    u = r_u8(texture + 14);
    w_u8(0x80088B3C + next_index * 40, u + gear * 20);
    texture = r_u32(0x8008822C);
    offset = r_u32(0x80088104) * 40;
    v = r_u8(texture + 15);
    w_u8(0x80088B3D + offset, v);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    gear = r_u8(car + 0x343);
    u = r_u8(texture + 14);
    w_u8(0x80088B44 + offset, u + gear * 20 + 19);
    texture = r_u32(0x8008822C);
    offset = r_u32(0x80088104) * 40;
    v = r_u8(texture + 15);
    w_u8(0x80088B45 + offset, v);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    gear = r_u8(car + 0x343);
    u = r_u8(texture + 14);
    w_u8(0x80088B4C + offset, u + gear * 20);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    v = r_u8(texture + 15);
    height = r_u8(texture + 13);
    w_u8(0x80088B4D + offset, v + height + 255);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    gear = r_u8(car + 0x343);
    u = r_u8(texture + 14);
    w_u8(0x80088B54 + offset, u + gear * 20 + 19);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    v = r_u8(texture + 15);
    height = r_u8(texture + 13);
    w_u8(0x80088B55 + offset, v + height + 255);
    offset = r_u32(0x80088104) * 40;
    buffer = r_u32(0x800880F0);
    td5_minimap_link(buffer, 0x80088B30 + offset);
    speed = r_s32(car + 0x2EC) >> 8;
    if (speed < 0)
        speed = 0;
    if (r_u32(0x800876C4) != 0)
    {
        value = ((uint32)speed << 8) + 389;
        speed = (sint32)value / 778;
    }
    else
    {
        value = ((uint32)speed << 8) + 626;
        speed = (sint32)value / 1252;
    }
    position = 0;
    digit_offset = 0;
    do
    {
        quotient = speed / 10;
        digit = speed % 10;
        offset = r_u32(0x80088104) * 40;
        packet = 0x800889F0 + offset + digit_offset;
        if (digit != 0 || quotient != 0 || position == 0)
        {
            texture = r_u32(0x80088228);
            u = r_u8(texture + 14);
            offset = (uint32)digit * 18;
            w_u8(packet + 12, u + offset);
            texture = r_u32(0x80088228);
            v = r_u8(texture + 15);
            w_u8(packet + 13, v);
            texture = r_u32(0x80088228);
            u = r_u8(texture + 14);
            w_u8(packet + 20, u + offset + 18);
            texture = r_u32(0x80088228);
            v = r_u8(texture + 15);
            w_u8(packet + 21, v);
            texture = r_u32(0x80088228);
            u = r_u8(texture + 14);
            w_u8(packet + 28, u + offset);
            texture = r_u32(0x80088228);
            v = r_u8(texture + 15);
            height = r_u8(texture + 13);
            w_u8(packet + 29, v + height);
            texture = r_u32(0x80088228);
            u = r_u8(texture + 14);
            w_u8(packet + 36, u + offset + 18);
            texture = r_u32(0x80088228);
            v = r_u8(texture + 15);
            height = r_u8(texture + 13);
            w_u8(packet + 37, v + height);
            buffer = r_u32(0x800880F0);
            td5_minimap_link(buffer, packet);
        }
        ++position;
        speed = quotient;
        digit_offset += 80;
    } while (position < 3);
    offset = r_u32(0x80088104) * 40;
    buffer = r_u32(0x800880F0);
    td5_minimap_link(buffer, 0x80088AE0 + offset);
    return td5_minimap_link(buffer, 0x80088810 + offset);
}

uint32 td5_80050de8(uint32 car)
{
    uint32 result;
    uint32 angle;
    uint32 cosine;
    uint32 sine;
    uint32 scale_x;
    uint32 scale_z;
    uint32 origin_x;
    uint32 origin_z;
    uint32 value;
    uint32 component;
    uint32 buffer_index;
    uint32 packet;
    uint32 buffer;
    uint32 mode;
    uint32 car_offset;
    uint32 car_index;
    uint32 x;
    uint32 z;
    uint32 owner;
    uint32 data;
    uint32 index;
    uint32 limit;
    uint32 row;
    uint32 row_offset;
    uint32 mapped_row;
    uint32 alternate_packet;
    uint32 special;
    uint32 special_offset;
    uint32 segment_base;
    uint32 segment;
    uint32 vertex_base;
    uint32 vertex;
    uint32 vertex_offset;
    uint32 type;
    uint32 lane;
    uint32 mapped_start;
    uint32 mapped_end;
    uint32 iteration;
    sint32 node;
    sint32 length;
    sint32 half_length;
    sint32 difference;
    sint32 quotient;
    sint32 end_node;
    sint32 alternate;
    sint32 row_start;
    TD5_MINIMAP_POINT first;
    TD5_MINIMAP_POINT second;
    TD5_MINIMAP_POINT third;
    TD5_MINIMAP_POINT fourth;

    FUNCTION_MARKER(0x80050DE8, "SLUS_006.10");
    result = r_u32(0x80087730);
    if (result != 0)
        return result;
    angle = 2048 - (uint32)(r_s32(car + 0x1BC) >> 8);
    cosine = (uint32)rcos((sint32)angle);
    sine = (uint32)rsin((sint32)angle);
    value = r_u32(0x8008815C);
    origin_x = r_u32(0x80098664);
    origin_z = r_u32(0x8009866C);
    origin_x = (uint32)((sint32)(0 - origin_x) >> 8);
    origin_z = (uint32)((sint32)(0 - origin_z) >> 8);
    scale_x = (uint32)((sint32)(value * 56) / 4096);
    value = r_u32(0x80088158);
    scale_z = (uint32)((sint32)(value * 56) / 4096);
    buffer_index = r_u32(0x80088104);
    node = r_s16(0x80098512);
    value = r_u32(0x800875E4);
    packet = 0x800A3BC8 + buffer_index * 288;
    if ((sint32)value > 0)
    {
        mode = r_u32(0x80087720);
        buffer = r_u32(0x800880F0);
        length = r_s32(0x8008841C);
        half_length = length / 2;
        car_offset = 0;
        car_index = 0;
        do
        {
            if (mode != 1 || car_index - 1 >= 5)
            {
                value = r_u32(0x80087730);
                component = (uint32)r_s16(0x80098512 + car_offset);
                difference = (sint32)((uint32)node - component);
                if (value != 0)
                {
                    value = (uint32)difference + (uint32)half_length;
                    quotient = td5_divide_checked((sint32)value, length);
                    value -= (uint32)quotient * (uint32)length;
                    difference = (sint32)(value - (uint32)half_length);
                }
                if ((uint32)difference + 72 < 145)
                {
                    x = (uint32)(r_s32(0x80098664 + car_offset) >> 8) + origin_x;
                    z = (uint32)(r_s32(0x8009866C + car_offset) >> 8) + origin_z;
                    component = (0 - x) * sine + z * cosine;
                    value = x * cosine + z * sine;
                    value = (uint32)((sint32)value / 4096) * scale_x;
                    x = (uint32)((sint32)value / 65536);
                    value = component;
                    value = (uint32)((sint32)value / 4096) * scale_z;
                    z = (uint32)((sint32)value / 65536);
                    w_u16(packet + 8, x + 49);
                    w_u16(packet + 10, z + 31);
                    w_u16(packet + 12, x + 52);
                    w_u16(packet + 14, z + 31);
                    w_u16(packet + 16, x + 49);
                    w_u16(packet + 18, z + 33);
                    w_u16(packet + 20, x + 52);
                    w_u16(packet + 22, z + 33);
                    td5_minimap_link(buffer, packet);
                }
            }
            car_offset += 864;
            value = r_u32(0x800875E4);
            ++car_index;
            packet += 24;
        } while ((sint32)car_index < (sint32)value);
    }
    if (node >= r_s32(0x8008841C))
    {
        index = r_u32(0x800883DC);
        limit = r_u32(0x80088404);
        if ((sint32)index < (sint32)limit)
        {
            row = index * 6;
            do
            {
                end_node = r_s16(0x8008D002 + row);
                ++index;
                if (end_node >= node)
                {
                    row_start = r_s16(0x8008D000 + row);
                    alternate = r_s16(0x8008D004 + row);
                    node = (sint32)((uint32)node - (uint32)row_start + (uint32)alternate);
                    break;
                }
                row += 6;
            } while ((sint32)index < (sint32)limit);
        }
    }
    mode = r_u32(0x80087730);
    node = (sint32)((uint32)(node / 12) * 12 - 72);
    if (node < 0)
    {
        if (mode != 0)
            node = (sint32)((uint32)node + r_u32(0x8008841C));
        else
            node = 0;
    }
    owner = r_u32(0x80088770);
    data = r_u32(owner + 0x2C);
    special = 9999;
    index = 0;
    if (r_u8(data) != 0)
    {
        do
        {
            data = r_u32(owner + 0x2C);
            value = r_u16(data + index * 4 + 4);
            if ((sint32)value >= node && (sint32)value < (sint32)((uint32)node + 144))
            {
                special = value;
                break;
            }
            owner = r_u32(0x80088770);
            data = r_u32(owner + 0x2C);
            value = r_u8(data);
            ++index;
        } while ((sint32)index < (sint32)value);
    }
    buffer_index = r_u32(0x80088104);
    packet = r_u32(0x80088730 + buffer_index * 4);
    alternate_packet = packet + 576;
    row_offset = 0;
    mapped_row = 0;
    iteration = 0;
    do
    {
        if (r_s16(0x8008D002 + row_offset) < node)
        {
            row = mapped_row * 6;
            do
            {
                row += 6;
                end_node = r_s16(0x8008D002 + row);
                row_offset += 6;
                ++mapped_row;
            } while (end_node < node);
        }
        end_node = r_s16(0x8008D002 + row_offset);
        value = (uint32)node + 5;
        if ((sint32)value >= end_node)
        {
            if (r_u32(0x80087730) != 0)
            {
                length = r_s32(0x8008841C);
                end_node = (sint32)value < length ? end_node : 0;
            }
        }
        else
            end_node = (sint32)value;
        if (end_node >= (sint32)special)
        {
            end_node = (sint32)special;
            segment_base = r_u32(0x800884C4);
            special_offset = special * 24;
            segment = segment_base + special_offset;
            index = r_u16(segment + 6);
            vertex_base = r_u32(0x8008850C);
            vertex = vertex_base + index * 6;
            first = td5_minimap_project(segment, vertex, origin_x, origin_z, cosine, sine, scale_x, scale_z);
            type = r_u8(segment);
            lane = r_u8(segment + 3);
            vertex_offset = r_u32(0x8007E728 + type * 8) + (lane & 15);
            second = td5_minimap_project(segment, vertex + vertex_offset * 6, origin_x, origin_z, cosine, sine, scale_x, scale_z);
            buffer_index = r_u32(0x80088104);
            buffer = r_u32(0x800880F0);
            special_offset = buffer_index * 32;
            data = 0x8008D168 + special_offset;
            w_u16(data + 8, first.x);
            w_u16(data + 10, first.y);
            w_u16(data + 12, second.x);
            w_u16(data + 14, second.y);
            td5_minimap_link(buffer, data);
            alternate = r_s16(0x8008D004 + row_offset);
            if (alternate != -1)
            {
                row_start = r_s16(0x8008D000 + row_offset);
                mapped_end = ((uint32)end_node + (uint32)alternate - (uint32)row_start) * 24;
                segment = segment_base + mapped_end;
                index = r_u16(segment + 6);
                vertex = vertex_base + index * 6;
                first = td5_minimap_project(segment, vertex, origin_x, origin_z, cosine, sine, scale_x, scale_z);
                type = r_u8(segment);
                lane = r_u8(segment + 3);
                vertex_offset = r_u32(0x8007E728 + type * 8) + (lane & 15);
                segment_base = r_u32(0x800884C4);
                segment = segment_base + mapped_end;
                second = td5_minimap_project(segment, vertex + vertex_offset * 6, origin_x, origin_z, cosine, sine, scale_x, scale_z);
                data = 0x8008D178 + special_offset;
                w_u16(data + 8, first.x);
                w_u16(data + 10, first.y);
                w_u16(data + 12, second.x);
                w_u16(data + 14, second.y);
                td5_minimap_link(buffer, data);
            }
            special = 9999;
        }
        segment_base = r_u32(0x800884C4);
        segment = segment_base + (uint32)node * 24;
        index = r_u16(segment + 4);
        vertex_base = r_u32(0x8008850C);
        vertex = vertex_base + index * 6;
        first = td5_minimap_project(segment, vertex, origin_x, origin_z, cosine, sine, scale_x, scale_z);
        type = r_u8(segment);
        lane = r_u8(segment + 3);
        vertex_offset = r_u32(0x8007E724 + type * 8) + (lane & 15);
        second = td5_minimap_project(segment, vertex + vertex_offset * 6, origin_x, origin_z, cosine, sine, scale_x, scale_z);
        mapped_end = (uint32)end_node * 24;
        segment = segment_base + mapped_end;
        index = r_u16(segment + 6);
        vertex = vertex_base + index * 6;
        third = td5_minimap_project(segment, vertex, origin_x, origin_z, cosine, sine, scale_x, scale_z);
        type = r_u8(segment);
        lane = r_u8(segment + 3);
        vertex_offset = r_u32(0x8007E728 + type * 8) + (lane & 15);
        segment_base = r_u32(0x800884C4);
        segment = segment_base + mapped_end;
        fourth = td5_minimap_project(segment, vertex + vertex_offset * 6, origin_x, origin_z, cosine, sine, scale_x, scale_z);
        buffer = r_u32(0x800880F0);
        w_u16(alternate_packet - 568, first.x);
        w_u16(alternate_packet - 566, first.y);
        w_u16(alternate_packet - 564, second.x);
        w_u16(alternate_packet - 562, second.y);
        w_u16(alternate_packet - 560, third.x);
        w_u16(alternate_packet - 558, third.y);
        w_u16(alternate_packet - 556, fourth.x);
        w_u16(alternate_packet - 554, fourth.y);
        td5_minimap_link(buffer, packet);
        alternate = r_s16(0x8008D004 + row_offset);
        if (alternate != -1)
        {
            row_start = r_s16(0x8008D000 + row_offset);
            mapped_start = ((uint32)node + (uint32)alternate - (uint32)row_start) * 24;
            segment = segment_base + mapped_start;
            index = r_u16(segment + 4);
            vertex_base = r_u32(0x8008850C);
            vertex = vertex_base + index * 6;
            first = td5_minimap_project(segment, vertex, origin_x, origin_z, cosine, sine, scale_x, scale_z);
            type = r_u8(segment);
            lane = r_u8(segment + 3);
            vertex_offset = r_u32(0x8007E724 + type * 8) + (lane & 15);
            mapped_end = ((uint32)end_node + (uint32)alternate - (uint32)row_start) * 24;
            second = td5_minimap_project(segment, vertex + vertex_offset * 6, origin_x, origin_z, cosine, sine, scale_x, scale_z);
            segment_base = r_u32(0x800884C4);
            segment = segment_base + mapped_end;
            index = r_u16(segment + 6);
            vertex = vertex_base + index * 6;
            third = td5_minimap_project(segment, vertex, origin_x, origin_z, cosine, sine, scale_x, scale_z);
            type = r_u8(segment);
            lane = r_u8(segment + 3);
            vertex_offset = r_u32(0x8007E728 + type * 8) + (lane & 15);
            fourth = td5_minimap_project(segment, vertex + vertex_offset * 6, origin_x, origin_z, cosine, sine, scale_x, scale_z);
            buffer = r_u32(0x800880F0);
            w_u16(alternate_packet + 8, first.x);
            w_u16(alternate_packet + 10, first.y);
            w_u16(alternate_packet + 12, second.x);
            w_u16(alternate_packet + 14, second.y);
            w_u16(alternate_packet + 16, third.x);
            w_u16(alternate_packet + 18, third.y);
            w_u16(alternate_packet + 20, fourth.x);
            w_u16(alternate_packet + 22, fourth.y);
            td5_minimap_link(buffer, alternate_packet);
        }
        value = r_u32(0x8008841C);
        node = (sint32)((uint32)end_node + 1);
        if (node >= (sint32)(value - 2))
        {
            if (r_u32(0x80087730) == 0)
                break;
            node = 0;
        }
        ++iteration;
        alternate_packet += 24;
        packet += 24;
    } while (iteration < 24);
    buffer_index = r_u32(0x80088104);
    buffer = r_u32(0x800880F0);
    td5_minimap_link(buffer, 0x800A1F80 + buffer_index * 24);
    td5_minimap_link(buffer, 0x8008CE20 + buffer_index * 64);
    td5_minimap_link(buffer, 0x8009D408 + buffer_index * 16);
    td5_minimap_link(buffer, 0x8009D428 + buffer_index * 16);
    td5_minimap_link(buffer, 0x8009D448 + buffer_index * 16);
    return td5_minimap_link(buffer, 0x8009D468 + buffer_index * 16);
}

void td5_80042db0(uint32 item, uint32 y, uint32 view, uint32 shade)
{
    FUNCTION_MARKER(0x80042DB0, "SLUS_006.10");
}

uint32 td5_80042db8(uint32 car, uint32 view)
{
    uint32 node;
    uint32 current;
    uint32 offset;
    uint32 value;
    uint32 data;
    uint32 index;
    sint32 previous;
    sint32 count;

    FUNCTION_MARKER(0x80042DB8, "SLUS_006.10");
    node = r_u16(car + 0x72);
    offset = view * 4;
    previous = r_s32(0x80088550 + offset);
    current = r_u16(car + 0x70);
    if ((sint32)node < previous)
        w_u32(0x800887A0 + offset, 1);
    else if (previous < (sint32)node)
        w_u32(0x800887A0 + offset, 0);
    if (node != current)
        node |= 0x8000;
    if (r_u32(0x800887A0 + offset) != 0)
        return 0xFFFFFFFF;
    if (r_u32(0x80088628 + offset) == 0)
    {
        value = r_u32(0x80088558 + offset);
        if ((sint32)value > 0)
            w_u32(0x80088558 + offset, value - 32);
        if (r_u32(0x80088628 + offset) == 0 && r_u32(0x80088558 + offset) == 0)
        {
            count = r_s32(0x80088540);
            if (count <= 0)
                return 0xFFFFFFFF;
            data = r_u32(0x80088650);
            for (index = 0; (sint32)index < count; ++index)
            {
                if (r_u16(data) == node)
                {
                    w_u32(0x800884B8 + offset, index);
                    w_u32(0x80088628 + offset, 120);
                    w_u32(0x80088558 + offset, 224);
                    return 0;
                }
                data += 6;
            }
            return 0xFFFFFFFF;
        }
    }
    value = r_u32(0x80088628 + offset);
    if ((sint32)value > 0)
        w_u32(0x80088628 + offset, value - 1);
    return 0;
}

uint32 td5_80042f44(uint32 car, uint32 view)
{
    uint32 owner;
    uint32 data;
    uint32 offset;
    uint32 result;
    uint32 row;
    uint32 count;
    uint32 y;
    uint32 item;
    uint32 shade;
    sint32 i;

    FUNCTION_MARKER(0x80042F44, "SLUS_006.10");
    owner = r_u32(0x80088770);
    data = r_u32(owner);
    result = r_u8(data + 0x1E) & 0x10;
    if (result == 0)
        return result;
    result = td5_80042db8(car, view);
    offset = view * 4;
    w_u32(0x80088550 + offset, (uint32)r_s16(car + 0x70));
    if (result == 0xFFFFFFFF)
        return result;
    row = r_u32(0x800884B8 + offset);
    w_u32(0x800884D8 + offset, 0);
    data = r_u32(0x80088650);
    row = data + row * 6;
    count = 0;
    for (i = 0; i < 4; ++i)
    {
        if (r_u8(row + (uint32)i + 2) != 0)
            count += 12;
    }
    y = r_u32(0x800880F8);
    y = (uint32)((sint32)y / 2) + count - 32;
    for (i = 3; i >= 0; --i)
    {
        result = r_u32(0x800884B8 + offset);
        data = r_u32(0x80088650);
        item = r_u8(data + result * 6 + (uint32)i + 2);
        if (item != 0)
        {
            shade = r_u8(0x80088558 + offset);
            td5_80042db0(item - 1, y, view, shade);
            y -= 24;
        }
    }
    return result;
}

uint32 td5_8003af84(uint32 car)
{
    uint32 view;
    uint32 offset;
    uint32 value;
    uint32 angle;
    uint32 index;
    uint32 selected;
    uint32 draw;
    uint32 texture;
    uint32 width;
    uint32 height;
    uint32 texture_width;
    uint32 texture_height;
    uint32 x;
    uint32 y;
    uint32 packet;
    uint32 buffer;
    uint32 ordering;
    uint32 tag;
    uint32 link;
    sint32 node;

    FUNCTION_MARKER(0x8003AF84, "SLUS_006.10");
    view = r_u32(0x800885F8);
    node = r_s16(car + 0x70);
    offset = view * 4;
    if (node < r_s32(0x8008825C + offset))
    {
        value = r_u32(0x80088264 + offset);
        w_u32(0x80088264 + offset, value + 1);
    }
    view = r_u32(0x800885F8);
    offset = view * 4;
    if (r_s32(0x8008825C + offset) < node)
        w_u32(0x80088264 + offset, 0);
    view = r_u32(0x800885F8);
    offset = view * 4;
    value = r_u32(0x8008826C + offset);
    w_u32(0x8008826C + offset, value + 1);
    index = r_u8(car + 0x34D);
    angle = td5_8004d19c(0x80097750 + index * 284);
    draw = 1;
    if (r_u32(0x8008771C) == 3)
    {
        index = r_u8(car + 0x34D);
        selected = r_u32(0x800879B8);
        if (selected == index)
            draw = 0;
    }
    if (draw != 0 && angle - 1024 < 2048)
    {
        view = r_u32(0x800885F8);
        offset = view * 4;
        if (r_s32(0x80088264 + offset) >= 3 && r_s32(0x8008826C + offset) % 32 >= 9)
        {
            texture = r_u32(0x80088230);
            width = r_u32(0x80088438);
            texture_width = r_u8(texture + 12);
            x = r_u32(0x80088510);
            texture_height = r_u8(texture + 13);
            x += (uint32)((sint32)(width - texture_width) / 2);
            index = r_u32(0x80088104);
            height = r_u32(0x800884F8);
            packet = 0x80088DD8 + index * 40;
            y = r_u32(0x80088518);
            y += (uint32)((sint32)(height - texture_height) / 2);
            w_u16(packet + 8, x);
            w_u16(packet + 10, y);
            texture_width = r_u8(texture + 12);
            w_u16(packet + 18, y);
            w_u16(packet + 24, x);
            w_u16(packet + 16, texture_width + x);
            texture_height = r_u8(texture + 13);
            w_u16(packet + 26, texture_height + y);
            texture_width = r_u8(texture + 12);
            w_u16(packet + 32, texture_width + x);
            texture_height = r_u8(texture + 13);
            buffer = r_u32(0x800880F0);
            w_u16(packet + 34, texture_height + y);
            ordering = r_u32(buffer + 0x70);
            tag = r_u32(packet);
            link = r_u32(ordering + 8);
            w_u32(packet, (tag & 0xFF000000) | (link & 0xFFFFFF));
            ordering = r_u32(buffer + 0x70);
            tag = r_u32(ordering + 8);
            w_u32(ordering + 8, (tag & 0xFF000000) | (packet & 0xFFFFFF));
        }
    }
    offset = r_u32(0x800885F8) * 4;
    w_u32(0x8008825C + offset, (uint32)node);
    return offset;
}

uint32 td5_8001ebbc(uint32 car, uint32 view, uint32 original_stack)
{
    uint32 offset;
    uint32 value;
    uint32 previous;
    uint32 buttons;
    uint32 mask;
    sint32 counter;
    sint32 phase;

    FUNCTION_MARKER(0x8001EBBC, "SLUS_006.10");
    offset = view * 4;
    if (r_u32(0x800884D0 + offset) != 0)
        return td5_80047564(view, original_stack - 0x20);
    if (r_u32(0x80088418) != 0 || r_u32(0x80087650) != 0)
    {
        td5_8001fa7c(car, view);
        return td5_8001fe94(car, view, original_stack - 0x20);
    }
    counter = r_s32(0x800886E0);
    if (counter > 0)
    {
        phase = counter / 40;
        if (phase <= 3)
        {
            w_u32(0x80088610 + offset, (uint32)(13 - phase));
            if (phase == 0)
            {
                value = r_u32(0x800881A8 + offset);
                w_u32(0x800881A8 + offset, value - 14080);
            }
            else
            {
                value = r_u32(0x80088598 + offset);
                w_u32(0x80088598 + offset, value + (phase == 3 ? 1024 : 4096));
            }
            previous = r_u32(0x80088180 + offset);
            value = r_u32(0x80088610 + offset);
            if (previous != value)
            {
                td5_8001e034(car + 0x1D0, phase == 3 ? 1 : 0, view);
                w_u32(0x800883C0 + offset, 0);
            }
            td5_8001e654(car, phase == 0 ? 1 : 0, view, original_stack - 0x20);
        }
        value = r_u32(0x80088610 + offset);
        w_u32(0x80088180 + offset, value);
        return value;
    }
    buttons = r_u16(0x800885D0 + view * 2);
    mask = r_u16(0x80087804 + view * 2);
    w_u32(0x800883C0 + offset, (buttons & mask) != 0 ? 2048 : 0);
    buttons = r_u16(0x800884E8 + view * 2);
    mask = r_u16(0x80087800 + view * 2);
    if ((buttons & mask) != 0)
    {
        value = r_u32(0x80088610 + offset) + 1;
        w_u32(0x80088610 + offset, (uint32)((sint32)value % 7));
        td5_8001e034(car + 0x1D0, 0, view);
    }
    if (r_u32(0x80088440 + offset) != 0)
        return td5_8001ea78(car, view, original_stack - 0x20);
    return td5_8001e654(car, 1, view, original_stack - 0x20);
}

uint32 td5_8003f33c(uint32 car, uint32 corner)
{
    sint32 random;

    FUNCTION_MARKER(0x8003F33C, "SLUS_006.10");
    random = randPSX();
    if (random % 100 < 50)
        return td5_8003c69c(corner, car);
    return 0;
}
