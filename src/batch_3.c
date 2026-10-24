#include "td5_functions.h"

sint32 td5_8004cf8c(sint32 x, sint32 z)
{
    sint32 first;
    sint32 second;
    uint32 value;

    FUNCTION_MARKER(0x8004CF8C, "SLUS_006.10");
    if (x >= 0)
    {
        if (z >= 0)
            return td5_8007b0c0(x, z);
        if (x > 0)
        {
            first = (sint32)(0 - (uint32)z);
            value = (uint32)td5_8007b0c0(first, x);
            return (sint32)(value + 1024);
        }
    }
    if (z < 0)
    {
        first = x < 0 ? (sint32)(0 - (uint32)x) : x;
        second = (sint32)(0 - (uint32)z);
        value = (uint32)td5_8007b0c0(first, second);
        return (sint32)(value + 2048);
    }
    second = x < 0 ? (sint32)(0 - (uint32)x) : x;
    value = (uint32)td5_8007b0c0(z, second);
    return (sint32)(value + 3072);
}

uint32 td5_8004d660(uint32 slot)
{
    uint32 car_offset = slot * 864;
    uint32 state_offset;
    uint32 value;
    sint32 index;
    uint32 table;
    sint32 node;
    uint32 kind;

    FUNCTION_MARKER(0x8004D660, "SLUS_006.10");
    node = r_s16(0x80098510 + car_offset);
    value = td5_8004da54(node, 0x80098664 + car_offset);
    state_offset = slot * 284;
    w_u32(0x800977B4 + state_offset, value);
    index = r_s16(0x80098512 + car_offset);
    table = r_u32(0x80097750 + state_offset);
    node = r_s16(0x80098510 + car_offset);
    kind = r_u8(table + (uint32)index * 3);
    value = td5_8004dbe8(node, value, kind);
    w_u32(0x80097774 + state_offset, value);
    return value;
}

uint32 td5_80024134(uint32 car, uint32 height_address)
{
    uint32 index;
    sint32 orientation;
    sint32 x;
    sint32 y;
    sint32 z;
    sint32 offset;

    FUNCTION_MARKER(0x80024134, "SLUS_006.10");
    w_u32(car + 0x1C8, 0xC0000000);
    w_u8(car + 0x343, 2);
    w_u8(car + 0x34E, 0);
    w_u8(car + 0x351, 0);
    for (index = 0x188; index <= 0x19C; index += 4)
        w_u32(car + index, 0);
    w_u16(car + 0x310, 0);
    w_u8(car + 0x354, 0);
    w_u32(car + 0x2E8, 400);
    for (index = 0; index < 16; index += 4)
    {
        w_u32(car + 0x2C4 + index, 0);
        w_u32(car + 0x2B4 + index, 0);
    }
    orientation = r_s32(car + 0x1BC);
    x = r_s32(car + 0x1C4);
    y = r_s32(car + 0x1C8);
    z = r_s32(car + 0x1CC);
    w_u16(car + 0x1D0, 0);
    w_u32(car + 0x1B8, 0);
    w_u16(car + 0x1D4, 0);
    w_u32(car + 0x1C0, 0);
    w_u32(car + 0x128, y >> 8);
    w_u16(car + 0x1D2, orientation >> 8);
    w_u32(car + 0x124, x >> 8);
    w_u32(car + 0x12C, z >> 8);
    td5_80024220(car, height_address);
    for (offset = 12; offset >= 0; offset -= 4)
        w_u32(car + 0x2D4 + (uint32)offset, 0);
    w_u32(car + 0x188, 0);
    w_u32(car + 0x190, 0);
    w_u32(car + 0x198, 0);
    return car - 4;
}

sint32 td5_8007b0c0(sint32 first, sint32 second)
{
    uint32 numerator;
    sint32 denominator;
    sint32 offset;
    sint32 subtract;
    sint32 index;
    sint32 value;

    FUNCTION_MARKER(0x8007B0C0, "SLUS_006.10");
    if (first == 0 && second == 0)
        return 0;
    if (first >= 0)
    {
        if (second > 0)
        {
            if (first < second)
            {
                numerator = ((uint32)first << 10) + (uint32)(second / 2);
                denominator = second;
                offset = 0;
                subtract = 0;
            }
            else
            {
                numerator = ((uint32)second << 10) + (uint32)(first / 2);
                denominator = first;
                offset = 1024;
                subtract = 1;
            }
        }
        else if (second < (sint32)(0 - (uint32)first))
        {
            numerator = (uint32)(second / 2) - ((uint32)first << 10);
            denominator = second;
            offset = 2048;
            subtract = 1;
        }
        else
        {
            numerator = (uint32)(first / 2) - ((uint32)second << 10);
            denominator = first;
            offset = 1024;
            subtract = 0;
        }
    }
    else if (second > 0)
    {
        if ((sint32)(0 - (uint32)first) < second)
        {
            numerator = (uint32)(second / 2) - ((uint32)first << 10);
            denominator = second;
            offset = 4096;
            subtract = 1;
        }
        else
        {
            numerator = (uint32)(first / 2) - ((uint32)second << 10);
            denominator = first;
            offset = 3072;
            subtract = 0;
        }
    }
    else if (second < first)
    {
        numerator = ((uint32)first << 10) + (uint32)(second / 2);
        denominator = second;
        offset = 2048;
        subtract = 0;
    }
    else
    {
        numerator = ((uint32)second << 10) + (uint32)(first / 2);
        denominator = first;
        offset = 3072;
        subtract = 1;
    }
    index = td5_divide_checked((sint32)numerator, denominator);
    value = r_s16(0x80086DC4 + ((uint32)index << 1));
    return subtract != 0 ? offset - value : offset + value;
}

uint32 td5_8004da54(sint32 node, uint32 position)
{
    uint32 descriptor;
    uint32 index;
    uint32 points;
    uint32 type;
    uint32 lanes;
    uint32 extra;
    uint32 first;
    uint32 second;
    sint32 first_x;
    sint32 second_x;
    sint32 first_z;
    sint32 second_z;
    sint32 dx;
    sint32 dz;
    uint32 base_x;
    uint32 base_z;
    sint32 x;
    sint32 z;
    sint32 distance;
    uint32 product;
    sint32 quotient;

    FUNCTION_MARKER(0x8004DA54, "SLUS_006.10");
    descriptor = r_u32(0x800884C4) + (uint32)node * 24;
    index = r_u16(descriptor + 6);
    points = r_u32(0x8008850C);
    type = r_u8(descriptor);
    lanes = r_u8(descriptor + 3);
    extra = r_u32(0x8007E5AC + type * 8);
    first = points + index * 6;
    index += lanes & 15;
    first_x = r_s16(first);
    index += extra;
    second = points + index * 6;
    second_x = r_s16(second);
    dx = second_x - first_x;
    second_z = r_s16(second + 4);
    first_z = r_s16(first + 4);
    dz = second_z - first_z;
    base_x = r_u32(descriptor + 12);
    x = r_s32(position);
    base_z = r_u32(descriptor + 20);
    z = r_s32(position + 8);
    x = (sint32)((uint32)(x >> 8) - (base_x + (uint32)first_x));
    z = (sint32)((uint32)(z >> 8) - (base_z + (uint32)first_z));
    product = (uint32)dx * (uint32)dx + (uint32)dz * (uint32)dz;
    distance = (sint32)SquareRoot0((sint32)product);
    if (distance == 0)
        return 0;
    product = (uint32)x * (uint32)dx + (uint32)z * (uint32)dz;
    quotient = td5_divide_checked((sint32)product, distance);
    return (uint32)td5_divide_checked((sint32)((uint32)quotient << 8), distance);
}

uint32 td5_8004dbe8(sint32 node, uint32 value, uint32 kind)
{
    uint32 descriptor;
    uint32 index;
    uint32 points;
    uint32 type;
    uint32 lanes;
    uint32 extra;
    uint32 first;
    uint32 second;
    sint32 a;
    sint32 b;
    uint32 difference = value - kind;
    sint32 x;
    sint32 z;
    uint32 result;

    FUNCTION_MARKER(0x8004DBE8, "SLUS_006.10");
    descriptor = r_u32(0x800884C4) + (uint32)node * 24;
    index = r_u16(descriptor + 6);
    points = r_u32(0x8008850C);
    type = r_u8(descriptor);
    lanes = r_u8(descriptor + 3);
    extra = r_u32(0x8007E5AC + type * 8);
    first = points + index * 6;
    second = points + (index + (lanes & 15) + extra) * 6;
    a = r_s16(second);
    b = r_s16(first);
    x = (sint32)((uint32)(a - b) * difference) / 256;
    a = r_s16(second + 4);
    b = r_s16(first + 4);
    z = (sint32)((uint32)(a - b) * difference) / 256;
    result = td5_integer_sqrt((uint32)x * (uint32)x + (uint32)z * (uint32)z);
    return (sint32)value < (sint32)kind ? 0 - result : result;
}

uint32 td5_80024220(uint32 car, uint32 height_address)
{
    uint32 gravity;
    uint32 time;
    uint32 saved_x;
    uint32 saved_z;
    uint32 alignment;
    uint32 lower;
    sint32 gravity_step;
    sint32 step;
    sint32 next_step;
    uint32 value;
    uint32 flag;
    uint32 snapshot[6];
    uint32 index;
    uint32 mask;
    uint32 previous_mask;
    sint32 factor;
    sint32 delta;
    sint32 velocity;
    sint32 limit_value;
    uint32 model;
    sint32 height;
    uint32 rotation[5];
    uint32 vector;
    uint32 output;
    sint32 suspension;
    uint32 y;
    uint32 rotated[3];
    uint32 total;
    sint32 count;
    sint32 average;

    FUNCTION_MARKER(0x80024220, "SLUS_006.10");
    gravity = r_u32(0x800877A4);
    time = r_u32(0x80088100);
    value = r_u32(car + 0x1C8);
    w_u32(car + 0x2B0, value);
    alignment = car & 3;
    saved_x = r_u32((car + 0x1D3) & 0xFFFFFFFC);
    lower = r_u32((car + 0x1D0) & 0xFFFFFFFC);
    saved_x = alignment == 0 ? lower : (saved_x << ((4 - alignment) * 8)) | (lower >> (alignment * 8));
    saved_z = r_u32((car + 0x1D7) & 0xFFFFFFFC);
    lower = r_u32((car + 0x1D4) & 0xFFFFFFFC);
    saved_z = alignment == 0 ? lower : (saved_z << ((4 - alignment) * 8)) | (lower >> (alignment * 8));
    gravity_step = (sint32)(gravity * time) / 256;
    step = (sint32)(r_u32(car + 0x188) * time) / 256;
    value = r_u32(car + 0x198) - (uint32)gravity_step;
    w_u32(car + 0x198, value);
    next_step = (sint32)(r_u32(car + 0x18C) * time) / 256;
    value = r_u32(car + 0x1B8) + (uint32)step;
    w_u32(car + 0x1B8, value);
    step = (sint32)(r_u32(car + 0x190) * time) / 256;
    value = r_u32(car + 0x1BC) + (uint32)next_step;
    w_u32(car + 0x1BC, value);
    next_step = (sint32)(r_u32(car + 0x198) * time) / 256;
    value = r_u32(car + 0x1C0) + (uint32)step;
    w_u32(car + 0x1C0, value);
    value = r_u32(car + 0x1C8);
    flag = r_u32(0x800883C8);
    w_u32(car + 0x1C8, value + (uint32)next_step);
    if (flag == 0)
    {
        step = (sint32)(r_u32(car + 0x194) * time) / 256;
        next_step = (sint32)(r_u32(car + 0x19C) * time) / 256;
        value = r_u32(car + 0x1C4) + (uint32)step;
        w_u32(car + 0x1C4, value);
        value = r_u32(car + 0x1CC) + (uint32)next_step;
        w_u32(car + 0x1CC, value);
    }
    for (index = 0; index < 6; ++index)
        snapshot[index] = r_u32(car + 0x1B8 + index * 4);
    w_u16(car + 0x1D0, (sint32)snapshot[0] >> 8);
    w_u16(car + 0x1D2, (sint32)snapshot[1] >> 8);
    w_u16(car + 0x1D4, (sint32)snapshot[2] >> 8);
    w_u32(car + 0x124, (sint32)snapshot[3] >> 8);
    w_u32(car + 0x128, (sint32)snapshot[4] >> 8);
    w_u32(car + 0x12C, (sint32)snapshot[5] >> 8);
    td5_80079548(car + 0x110, car + 0x1D0);
    td5_8001b310(car + 0x70, car + 0x1C4);
    td5_8001d8bc(car + 0x70, car + 0x1C4, car + 0x258);
    td5_80020030(car, height_address);
    mask = r_u8(car + 0x354);
    switch (mask)
    {
        case 0:
        case 1:
        case 2:
        case 4:
        case 6:
        case 8:
        case 9:
            td5_8001dcf8(car + 0x1D0, car + 0xE0, car + 0x2B4);
            mask = r_u8(car + 0x354);
            previous_mask = r_u8(car + 0x355);
            if (mask == previous_mask)
            {
                factor = td5_divide_checked(65536, r_s32(0x80088100));
                delta = ((r_s16(car + 0x1D0) - (sint16)saved_x + 2048) & 4095) - 2048;
                velocity = (sint32)((uint32)delta * (uint32)factor);
                w_u32(car + 0x188, delta);
                value = (uint32)r_s16(car + 0x1D4);
                w_u32(car + 0x188, velocity);
                delta = (((sint32)value - (sint16)saved_z + 2048) & 4095) - 2048;
                limit_value = r_s32(car + 0x188);
                w_u32(car + 0x190, (uint32)delta * (uint32)factor);
                if (limit_value > 6000)
                    w_u32(car + 0x188, 6000);
                if (r_s32(car + 0x188) < -6000)
                    w_u32(car + 0x188, -6000);
                if (r_s32(car + 0x190) > 6000)
                    w_u32(car + 0x190, 6000);
                if (r_s32(car + 0x190) < -6000)
                    w_u32(car + 0x190, -6000);
            }
            value = (uint32)r_s16(car + 0x1D0) << 8;
            y = (uint32)r_s16(car + 0x1D4) << 8;
            w_u32(car + 0x1B8, value);
            w_u32(car + 0x1C0, y);
            break;
        case 3:
        case 12:
            td5_8001df60(car + 0x1D0, car + 0xE0, car + 0x2B4);
            mask = r_u8(car + 0x354);
            previous_mask = r_u8(car + 0x355);
            if (mask == previous_mask)
            {
                factor = td5_divide_checked(65536, r_s32(0x80088100));
                delta = ((r_s16(car + 0x1D4) - (sint16)saved_z + 2048) & 4095) - 2048;
                velocity = (sint32)((uint32)delta * (uint32)factor);
                w_u32(car + 0x190, delta);
                w_u32(car + 0x190, velocity);
                if (velocity > 6000)
                    w_u32(car + 0x190, 6000);
                if (r_s32(car + 0x190) < -6000)
                    w_u32(car + 0x190, -6000);
            }
            w_u32(car + 0x1C0, (uint32)r_s16(car + 0x1D4) << 8);
            break;
        case 5:
        case 10:
            td5_8001de8c(car + 0x1D0, car + 0xE0, car + 0x2B4);
            mask = r_u8(car + 0x354);
            previous_mask = r_u8(car + 0x355);
            if (mask == previous_mask)
            {
                factor = td5_divide_checked(65536, r_s32(0x80088100));
                delta = ((r_s16(car + 0x1D0) - (sint16)saved_x + 2048) & 4095) - 2048;
                velocity = (sint32)((uint32)delta * (uint32)factor);
                w_u32(car + 0x188, delta);
                w_u32(car + 0x188, velocity);
                if (velocity > 6000)
                    w_u32(car + 0x188, 6000);
                if (r_s32(car + 0x188) < -6000)
                    w_u32(car + 0x188, -6000);
            }
            w_u32(car + 0x1B8, (uint32)r_s16(car + 0x1D0) << 8);
            break;
        default:
            break;
    }
    td5_80079548(car + 0x110, car + 0x1D0);
    model = r_u32(car + 0x180);
    height = r_s16(model + 0x82) * 181 / 256;
    rotation[0] = r_u32(car + 0x110);
    rotation[1] = r_u32(car + 0x114);
    xport_gte_write_control(0, rotation[0]);
    xport_gte_write_control(1, rotation[1]);
    rotation[2] = r_u32(car + 0x118);
    rotation[3] = r_u32(car + 0x11C);
    rotation[4] = r_u32(car + 0x120);
    xport_gte_write_control(2, rotation[2]);
    xport_gte_write_control(3, rotation[3]);
    xport_gte_write_control(4, rotation[4]);
    total = 0;
    count = 0;
    for (index = 0; index < 4; ++index)
    {
        mask = r_u8(car + 0x354);
        if (((mask >> index) & 1) == 0)
        {
            vector = car + 0x1D8 + index * 8;
            output = car + 0x1F8 + index * 8;
            model = r_u32(car + 0x180);
            suspension = r_s32(car + 0x2B4 + index * 4) / 256;
            y = r_u16(model + 0x42 + index * 8) - (uint32)suspension - (uint32)height;
            w_u16(vector + 2, y);
            xport_gte_write_data(0, r_u32(vector));
            xport_gte_write_data(1, r_u32(vector + 4));
            xport_gte_execute(0x486012);
            rotated[0] = xport_gte_read_data(9);
            rotated[1] = xport_gte_read_data(10);
            rotated[2] = xport_gte_read_data(11);
            w_u16(output, rotated[0]);
            w_u16(output + 2, rotated[1]);
            w_u16(output + 4, rotated[2]);
            y = r_u16(vector + 2);
            w_u16(vector + 2, y + (uint32)height);
            y = (uint32)r_s16(output + 2) << 8;
            value = r_u32(car + 0xE4 + index * 12);
            ++count;
            total += value - y;
        }
    }
    if (count > 0)
    {
        average = td5_divide_checked((sint32)total, count);
        mask = r_u8(car + 0x354);
        w_u32(car + 0x1C8, average);
        if (r_u8(0x8007DE8C + mask) != 0)
        {
            previous_mask = r_u8(car + 0x355);
            if ((previous_mask & (mask ^ 15)) == 0)
            {
                value = r_u32(car + 0x2B0);
                gravity = r_u32(0x800877A4);
                time = r_u32(0x80088100);
                value = ((uint32)average - value - gravity) << 8;
                velocity = td5_divide_checked((sint32)value, (sint32)time);
                w_u32(car + 0x198, velocity);
            }
        }
    }
    else
        w_u16(car + 0x338, r_u16(car + 0x338) + 1);
    return td5_80023998(car);
}

uint32 td5_8001de8c(uint32 angles, uint32 positions, uint32 suspension)
{
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;
    uint32 y[4];
    uint32 spring[4];
    sint32 x;
    sint32 z;
    sint32 height;
    uint32 distance;
    uint32 result;

    FUNCTION_MARKER(0x8001DE8C, "SLUS_006.10");
    a = r_u32(positions);
    b = r_u32(positions + 12);
    c = r_u32(positions + 24);
    d = r_u32(positions + 36);
    x = (sint32)(a + b - (c + d)) >> 8;
    a = r_u32(positions + 8);
    b = r_u32(positions + 20);
    c = r_u32(positions + 32);
    d = r_u32(positions + 44);
    z = (sint32)(a + b - (c + d)) >> 8;
    y[0] = r_u32(positions + 4);
    spring[0] = r_u32(suspension);
    y[1] = r_u32(positions + 16);
    y[2] = r_u32(positions + 28);
    spring[2] = r_u32(suspension + 8);
    y[3] = r_u32(positions + 40);
    spring[1] = r_u32(suspension + 4);
    spring[3] = r_u32(suspension + 12);
    height = (sint32)(y[0] + spring[0] + y[1] + spring[1] - (y[2] + spring[2] + y[3] + spring[3])) >> 8;
    distance = SquareRoot0((sint32)((uint32)x * (uint32)x + (uint32)z * (uint32)z));
    result = (uint32)td5_8007b0c0((sint32)(0 - (uint32)height), (sint32)distance);
    w_u16(angles, result);
    return result;
}

uint32 td5_8001df60(uint32 angles, uint32 positions, uint32 suspension)
{
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;
    uint32 y[4];
    uint32 spring[4];
    sint32 x;
    sint32 z;
    sint32 height;
    uint32 distance;
    uint32 result;

    FUNCTION_MARKER(0x8001DF60, "SLUS_006.10");
    a = r_u32(positions);
    b = r_u32(positions + 24);
    c = r_u32(positions + 12);
    d = r_u32(positions + 36);
    x = (sint32)(a + b - (c + d)) >> 8;
    a = r_u32(positions + 8);
    b = r_u32(positions + 32);
    c = r_u32(positions + 20);
    d = r_u32(positions + 44);
    z = (sint32)(a + b - (c + d)) >> 8;
    y[0] = r_u32(positions + 4);
    spring[0] = r_u32(suspension);
    y[2] = r_u32(positions + 28);
    y[1] = r_u32(positions + 16);
    spring[1] = r_u32(suspension + 4);
    y[3] = r_u32(positions + 40);
    spring[2] = r_u32(suspension + 8);
    spring[3] = r_u32(suspension + 12);
    height = (sint32)(y[0] + spring[0] + y[2] + spring[2] - (y[1] + spring[1] + y[3] + spring[3])) >> 8;
    distance = SquareRoot0((sint32)((uint32)x * (uint32)x + (uint32)z * (uint32)z));
    result = (uint32)td5_8007b0c0((sint32)(0 - (uint32)height), (sint32)distance);
    w_u16(angles + 4, result);
    return result;
}

uint32 td5_8001dcf8(uint32 angles, uint32 positions, uint32 suspension)
{
    uint32 step;
    uint32 positive;
    uint32 negative;
    uint32 x[4];
    uint32 z[4];
    uint32 y[4];
    uint32 spring[4];
    sint32 dx;
    sint32 dz;
    sint32 height;
    uint32 distance;
    uint32 result = 0;

    FUNCTION_MARKER(0x8001DCF8, "SLUS_006.10");
    for (step = 0; step < 2; ++step)
    {
        positive = step == 0 ? 24 : 12;
        negative = step == 0 ? 12 : 24;
        x[0] = r_u32(positions);
        x[1] = r_u32(positions + positive);
        x[2] = r_u32(positions + negative);
        x[3] = r_u32(positions + 36);
        dx = (sint32)(x[0] + x[1] - (x[2] + x[3])) >> 8;
        z[1] = r_u32(positions + positive + 8);
        spring[0] = r_u32(suspension);
        z[0] = r_u32(positions + 8);
        z[2] = r_u32(positions + negative + 8);
        z[3] = r_u32(positions + 44);
        y[1] = r_u32(positions + positive + 4);
        y[0] = r_u32(positions + 4);
        dz = (sint32)(z[0] + z[1] - (z[2] + z[3])) >> 8;
        spring[1] = r_u32(suspension + positive / 3);
        y[2] = r_u32(positions + negative + 4);
        spring[2] = r_u32(suspension + negative / 3);
        y[3] = r_u32(positions + 40);
        spring[3] = r_u32(suspension + 12);
        height = (sint32)(y[0] + spring[0] + y[1] + spring[1] - (y[2] + spring[2] + y[3] + spring[3])) >> 8;
        distance = SquareRoot0((sint32)((uint32)dx * (uint32)dx + (uint32)dz * (uint32)dz));
        result = (uint32)td5_8007b0c0((sint32)(0 - (uint32)height), (sint32)distance);
        w_u16(angles + (step == 0 ? 4 : 0), result);
    }
    return result;
}

uint32 td5_80079548(uint32 matrix, uint32 angles)
{
    uint32 cosine_x;
    uint32 sine_x;
    uint32 cosine_y;
    uint32 sine_y;
    uint32 cosine_z;
    uint32 sine_z;
    uint32 xy_sine;
    uint32 xy_cosine;
    sint32 value;

    FUNCTION_MARKER(0x80079548, "SLUS_006.10");
    cosine_x = (uint32)rcos(r_s16(angles));
    sine_x = (uint32)rsin(r_s16(angles));
    cosine_y = (uint32)rcos(r_s16(angles + 2));
    sine_y = (uint32)rsin(r_s16(angles + 2));
    cosine_z = (uint32)rcos(r_s16(angles + 4));
    sine_z = (uint32)rsin(r_s16(angles + 4));
    xy_sine = (uint32)((sint32)(sine_x * sine_y + 2048) >> 12);
    xy_cosine = (uint32)((sint32)(sine_x * cosine_y + 2048) >> 12);
    w_u16(matrix + 10, 0 - sine_x);
    w_u16(matrix + 4, (sint32)(cosine_x * sine_y + 2048) >> 12);
    w_u16(matrix + 6, (sint32)(cosine_x * sine_z + 2048) >> 12);
    w_u16(matrix + 8, (sint32)(cosine_x * cosine_z + 2048) >> 12);
    w_u16(matrix + 16, (sint32)(cosine_x * cosine_y + 2048) >> 12);
    w_u16(matrix, (sint32)(cosine_y * cosine_z + xy_sine * sine_z + 2048) >> 12);
    w_u16(matrix + 2, (sint32)((0 - cosine_y) * sine_z + xy_sine * cosine_z + 2048) >> 12);
    w_u16(matrix + 12, (sint32)((0 - sine_y) * cosine_z + xy_cosine * sine_z + 2048) >> 12);
    value = (sint32)(sine_y * sine_z + xy_cosine * cosine_z + 2048) >> 12;
    w_u16(matrix + 14, value);
    return (uint32)value;
}

uint32 td5_80023998(uint32 car)
{
    MATRIX transpose = {0};
    uint32 mask;
    uint32 previous_mask;
    uint32 index;
    uint32 vector;
    sint32 local_x;
    sint32 local_z;
    uint32 rotated[3];
    uint32 gravity;
    sint32 force;
    sint32 a;
    sint32 b;
    uint32 product;
    uint32 dot;
    uint32 gravity_x = 0;
    uint32 gravity_z = 0;
    uint32 impact_x = 0;
    uint32 impact_y = 0;
    uint32 impact_z = 0;
    sint32 contacts = 0;
    sint32 impacts = 0;
    sint32 time;
    sint32 divisor;
    sint32 correction_x;
    sint32 correction_z;
    uint32 velocity_x;
    uint32 velocity_y;
    uint32 velocity_z;
    uint32 result;

    FUNCTION_MARKER(0x80023998, "SLUS_006.10");
    mask = r_u8(car + 0x354);
    previous_mask = r_u8(car + 0x355);
    if (mask == 15)
        return 15;
    TransposeMatrix((MATRIX *)psx_addr(car + 0x110, 18), &transpose);
    for (index = 0; index < 4; ++index)
        xport_gte_write_control(index, xport_load_le32((const uint8 *)&transpose + index * 4));
    xport_gte_write_control(4, xport_load_le16((const uint8 *)&transpose + 16));
    for (index = 0; index < 4; ++index)
    {
        if (((mask >> index) & 1) == 0)
        {
            local_x = r_s16(car + 0x1D8 + index * 8);
            local_z = r_s16(car + 0x1DC + index * 8);
            vector = car + 0x218 + index * 8;
            xport_gte_write_data(0, r_u32(vector));
            xport_gte_write_data(1, r_u32(vector + 4));
            xport_gte_execute(0x486012);
            rotated[0] = xport_gte_read_data(9);
            rotated[1] = xport_gte_read_data(10);
            rotated[2] = xport_gte_read_data(11);
            gravity = r_u32(0x800877A4);
            force = (sint32)((uint32)(sint32)(sint16)rotated[1] * gravity) / 4096;
            gravity_x += (uint32)force * (uint32)local_z;
            ++contacts;
            gravity_z -= (uint32)force * (uint32)local_x;
            if (((previous_mask >> index) & 1) != 0)
            {
                a = r_s16(car + 0x238 + index * 8);
                b = r_s16(vector);
                dot = (uint32)a * (uint32)b;
                a = r_s16(car + 0x23A + index * 8);
                b = r_s16(vector + 2);
                product = (uint32)a * (uint32)b;
                a = r_s16(car + 0x23C + index * 8);
                b = r_s16(vector + 4);
                dot += product + (uint32)a * (uint32)b;
                force = (sint32)dot / 4096;
                ++impacts;
                impact_y += (uint32)(force / 2);
                impact_x -= ((uint32)force * (uint32)local_z) << 8;
                impact_z += ((uint32)force * (uint32)local_x) << 8;
            }
        }
    }
    gravity_x = (uint32)td5_divide_checked((sint32)gravity_x, contacts);
    gravity_z = (uint32)td5_divide_checked((sint32)gravity_z, contacts);
    if (impacts > 0)
    {
        impact_x = (uint32)td5_divide_checked((sint32)impact_x, impacts);
        impact_y = (uint32)td5_divide_checked((sint32)impact_y, impacts);
        impact_z = (uint32)td5_divide_checked((sint32)impact_z, impacts);
    }
    time = r_s32(0x80088100);
    divisor = td5_divide_checked(307200, time);
    correction_x = td5_divide_checked((sint32)(gravity_x + impact_x), divisor);
    divisor = td5_divide_checked(140800, time);
    correction_z = td5_divide_checked((sint32)(gravity_z + impact_z), divisor);
    gravity = r_u32(0x800877A4);
    velocity_z = r_u32(car + 0x190);
    velocity_x = r_u32(car + 0x188);
    velocity_y = r_u32(car + 0x198);
    w_u32(car + 0x188, velocity_x + (uint32)correction_x);
    result = velocity_y + impact_y + gravity;
    w_u32(car + 0x198, result);
    w_u32(car + 0x190, velocity_z + (uint32)correction_z);
    if (impacts <= 0)
        return result;
    if (previous_mask >= 16)
        return previous_mask << 2;
    switch (previous_mask)
    {
        case 0:
        case 1:
        case 2:
        case 4:
        case 6:
        case 8:
        case 9:
            if (r_s32(car + 0x188) > 4000)
                w_u32(car + 0x188, 4000);
            if (r_s32(car + 0x188) < -4000)
                w_u32(car + 0x188, -4000);
            /* Continue into the second rotation clamp */
        case 5:
        case 10:
            if (r_s32(car + 0x190) > 4000)
                w_u32(car + 0x190, 4000);
            if (r_s32(car + 0x190) < -4000)
                w_u32(car + 0x190, -4000);
            return (uint32)-4000;
        case 3:
        case 12:
            if (r_s32(car + 0x188) > 4000)
                w_u32(car + 0x188, 4000);
            if (r_s32(car + 0x188) < -4000)
                w_u32(car + 0x188, -4000);
            return (uint32)-4000;
        default:
            return 0x80023E68;
    }
}

uint32 td5_8001d8bc(uint32 node, uint32 position, uint32 output)
{
    sint32 node_index;
    uint32 descriptor;
    uint32 type;
    uint32 first;
    uint32 second;
    sint32 offset;
    uint32 lane;
    uint32 lanes;
    uint32 value;
    uint32 base;
    uint32 relative_x;
    uint32 relative_y;
    uint32 relative_z;
    sint32 action = 0;
    uint32 points;
    uint32 first_point;
    uint32 second_point;
    sint32 first_z;
    sint32 second_x;
    sint32 first_x;
    sint32 second_z;
    uint32 cross;

    FUNCTION_MARKER(0x8001D8BC, "SLUS_006.10");
    node_index = r_s16(node);
    descriptor = r_u32(0x800884C4) + (uint32)node_index * 24;
    type = r_u8(descriptor);
    first = r_u16(descriptor + 4);
    offset = r_s8(0x8007DD34 + type * 2);
    lane = r_u8(node + 12);
    lanes = r_u8(descriptor + 3);
    first += (uint32)offset + lane;
    w_u16(node + 8, first);
    second = r_u16(descriptor + 6);
    offset = r_s8(0x8007DD35 + type * 2);
    second += (uint32)offset + lane;
    w_u16(node + 10, second);
    value = r_u32(position);
    base = r_u32(descriptor + 12);
    relative_x = (uint32)((sint32)value >> 8) - base;
    value = r_u32(position + 4);
    base = r_u32(descriptor + 16);
    relative_y = (uint32)((sint32)value >> 8) - base;
    value = r_u32(position + 8);
    base = r_u32(descriptor + 20);
    relative_z = (uint32)((sint32)value >> 8) - base;
    lanes &= 15;
    if (lane == 0)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 3 || type == 4)
            return td5_8001d780((sint16)(first + 1), (sint16)second, (sint16)(second + 1), output);
        if (type == 6 || type == 7)
            action = 1;
    }
    else if (lane == lanes - 1)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 2 || type == 4)
            action = -1;
        else if (type == 5 || type == 7)
            return td5_8001d780((sint16)first, (sint16)second, (sint16)(first + 1), output);
    }
    if (action == 0)
    {
        points = r_u32(0x8008850C);
        first_point = points + first * 6;
        second_point = points + second * 6 + 6;
        first_z = r_s16(first_point + 4);
        second_x = r_s16(second_point);
        first_x = r_s16(first_point);
        second_z = r_s16(second_point + 4);
        cross = (relative_z - (uint32)first_z) * (uint32)(second_x - first_x);
        cross -= (relative_x - (uint32)first_x) * (uint32)(second_z - first_z);
        action = (sint32)cross > 0 ? 1 : -1;
    }
    (void)relative_y;
    if (action > 0)
        return td5_8001d780((sint16)first, (sint16)(second + 1), (sint16)(first + 1), output);
    return td5_8001d780((sint16)first, (sint16)second, (sint16)(second + 1), output);
}

uint32 td5_8001d780(sint32 first, sint32 second, sint32 third, uint32 output)
{
    VECTOR left = {0};
    VECTOR right = {0};
    VECTOR normal = {0};
    uint32 points;
    uint32 a;
    uint32 b;
    uint32 c;
    sint32 first_value;
    sint32 other_value;

    FUNCTION_MARKER(0x8001D780, "SLUS_006.10");
    points = r_u32(0x8008850C);
    a = points + (uint32)(sint16)first * 6;
    b = points + (uint32)(sint16)second * 6;
    first_value = r_s16(a);
    other_value = r_s16(b);
    left.vx = first_value - other_value;
    first_value = r_s16(a + 2);
    other_value = r_s16(b + 2);
    left.vy = first_value - other_value;
    first_value = r_s16(a + 4);
    other_value = r_s16(b + 4);
    left.vz = first_value - other_value;
    first_value = r_s16(a);
    c = points + (uint32)(sint16)third * 6;
    other_value = r_s16(c);
    right.vx = first_value - other_value;
    first_value = r_s16(a + 2);
    other_value = r_s16(c + 2);
    right.vy = first_value - other_value;
    first_value = r_s16(a + 4);
    other_value = r_s16(c + 4);
    right.vz = first_value - other_value;
    OuterProduct0(&left, &right, &normal);
    normal.vx >>= 12;
    normal.vy >>= 12;
    normal.vz >>= 12;
    VectorNormalS(&normal, (SVECTOR *)psx_addr(output, 6));
    if (r_s16(output + 2) == 0)
        w_u16(output + 2, 1);
    return 1;
}

uint32 td5_8001d314(uint32 node, uint32 position, uint32 normal, uint32 *height)
{
    VECTOR relative = {0};
    sint32 node_index;
    uint32 descriptor;
    uint32 type;
    uint32 first;
    uint32 second;
    sint32 offset;
    uint32 lane;
    uint32 lanes;
    uint32 value;
    uint32 base;
    sint32 action = 0;
    sint32 triangle_first;
    sint32 triangle_second;
    sint32 triangle_third;
    uint32 points;
    uint32 first_point;
    uint32 second_point;
    sint32 first_z;
    sint32 second_x;
    sint32 first_x;
    sint32 second_z;
    uint32 cross;
    uint32 result;

    FUNCTION_MARKER(0x8001D314, "SLUS_006.10");
    node_index = r_s16(node);
    descriptor = r_u32(0x800884C4) + (uint32)node_index * 24;
    type = r_u8(descriptor);
    first = r_u16(descriptor + 4);
    offset = r_s8(0x8007DD34 + type * 2);
    lane = r_u8(node + 12);
    lanes = r_u8(descriptor + 3);
    first += (uint32)offset + lane;
    w_u16(node + 8, first);
    second = r_u16(descriptor + 6);
    offset = r_s8(0x8007DD35 + type * 2);
    second += (uint32)offset + lane;
    w_u16(node + 10, second);
    value = r_u32(position);
    base = r_u32(descriptor + 12);
    relative.vx = (sint32)((uint32)((sint32)value >> 8) - base);
    value = r_u32(position + 4);
    base = r_u32(descriptor + 16);
    relative.vy = (sint32)((uint32)((sint32)value >> 8) - base);
    value = r_u32(position + 8);
    base = r_u32(descriptor + 20);
    relative.vz = (sint32)((uint32)((sint32)value >> 8) - base);
    lanes &= 15;
    if (lane == 0)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 3 || type == 4)
            action = 2;
        else if (type == 6 || type == 7)
            action = 1;
    }
    else if (lane == lanes - 1)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 2 || type == 4)
            action = -1;
        else if (type == 5 || type == 7)
            action = 3;
    }
    if (action == 0)
    {
        points = r_u32(0x8008850C);
        first_point = points + first * 6;
        second_point = points + second * 6 + 6;
        first_z = r_s16(first_point + 4);
        second_x = r_s16(second_point);
        first_x = r_s16(first_point);
        second_z = r_s16(second_point + 4);
        cross = ((uint32)relative.vz - (uint32)first_z) * (uint32)(second_x - first_x);
        cross -= ((uint32)relative.vx - (uint32)first_x) * (uint32)(second_z - first_z);
        action = (sint32)cross > 0 ? 1 : -1;
    }
    triangle_first = (sint16)(first + (action == 2 ? 1 : 0));
    triangle_second = (sint16)(second + (action == 1 ? 1 : 0));
    triangle_third = (sint16)((action == 1 || action == 3 ? first : second) + 1);
    result = td5_8001d144(triangle_first, triangle_second, triangle_third, &relative, normal);
    base = r_u32(descriptor + 16);
    result += base << 8;
    *height = result;
    return result;
}

uint32 td5_8001d144(sint32 first, sint32 second, sint32 third, const VECTOR *position, uint32 output)
{
    VECTOR left = {0};
    VECTOR right = {0};
    VECTOR normal = {0};
    uint32 points;
    uint32 first_offset = (uint32)(sint16)first * 6;
    uint32 a;
    uint32 b;
    uint32 c;
    sint32 first_value;
    sint32 other_value;
    sint32 position_value;
    sint32 normal_x;
    sint32 normal_z;
    sint32 normal_y;
    uint32 dot;
    sint32 quotient;

    FUNCTION_MARKER(0x8001D144, "SLUS_006.10");
    points = r_u32(0x8008850C);
    a = points + first_offset;
    b = points + (uint32)(sint16)second * 6;
    first_value = r_s16(a);
    other_value = r_s16(b);
    left.vx = first_value - other_value;
    first_value = r_s16(a + 2);
    other_value = r_s16(b + 2);
    left.vy = first_value - other_value;
    first_value = r_s16(a + 4);
    other_value = r_s16(b + 4);
    left.vz = first_value - other_value;
    first_value = r_s16(a);
    c = points + (uint32)(sint16)third * 6;
    other_value = r_s16(c);
    right.vx = first_value - other_value;
    first_value = r_s16(a + 2);
    other_value = r_s16(c + 2);
    right.vy = first_value - other_value;
    first_value = r_s16(a + 4);
    other_value = r_s16(c + 4);
    right.vz = first_value - other_value;
    OuterProduct0(&left, &right, &normal);
    normal.vx >>= 12;
    normal.vy >>= 12;
    normal.vz >>= 12;
    VectorNormalS(&normal, (SVECTOR *)psx_addr(output, 6));
    if (r_s16(output + 2) == 0)
        w_u16(output + 2, 1);
    points = r_u32(0x8008850C);
    position_value = position->vx;
    a = points + first_offset;
    first_value = r_s16(a);
    normal_x = r_s16(output);
    dot = ((uint32)first_value - (uint32)position_value) * (uint32)normal_x;
    first_value = r_s16(a + 4);
    position_value = position->vz;
    normal_z = r_s16(output + 4);
    dot += ((uint32)first_value - (uint32)position_value) * (uint32)normal_z;
    normal_y = r_s16(output + 2);
    quotient = td5_divide_checked((sint32)dot, normal_y);
    first_value = r_s16(a + 2);
    return ((uint32)quotient + (uint32)first_value) << 8;
}

uint32 td5_8001b1b8(uint32 mask, const VECTOR *position, uint32 first, uint32 second, uint32 third, uint32 fourth)
{
    uint32 starts[4];
    uint32 ends[4];
    uint32 index;
    uint32 bit;
    sint32 start_z;
    sint32 end_x;
    sint32 start_x;
    sint32 end_z;
    uint32 z;
    uint32 x;
    uint32 cross;
    uint32 result = 0;

    FUNCTION_MARKER(0x8001B1B8, "SLUS_006.10");
    starts[0] = fourth;
    starts[1] = third;
    starts[2] = second;
    starts[3] = first;
    ends[0] = first;
    ends[1] = fourth;
    ends[2] = third;
    ends[3] = second;
    for (index = 0; index < 4; ++index)
    {
        bit = 1 << index;
        if ((mask & bit) != 0)
        {
            start_z = r_s16(starts[index] + 4);
            z = (uint32)position->vz;
            end_x = r_s16(ends[index]);
            start_x = r_s16(starts[index]);
            cross = (z - (uint32)start_z) * (uint32)(end_x - start_x);
            x = (uint32)position->vx;
            end_z = r_s16(ends[index] + 4);
            cross -= (x - (uint32)start_x) * (uint32)(end_z - start_z);
            if ((sint32)cross > 0)
                result |= bit;
        }
    }
    return result;
}

/* Native views preserve the repeated cell and edge calculations */
typedef struct TD5_TRACK_CELL
{
    uint32 type;
    uint32 lanes;
    uint32 left;
    uint32 right;
    VECTOR relative;
} TD5_TRACK_CELL;

typedef struct TD5_TRACK_TRANSITION
{
    uint32 node;
    uint32 lane;
} TD5_TRACK_TRANSITION;

typedef struct TD5_TRACK_EDGE
{
    uint32 points;
    uint32 start;
    uint32 end;
    sint32 start_x;
    sint32 start_z;
    sint32 end_x;
    sint32 end_z;
    sint32 cross;
} TD5_TRACK_EDGE;

static TD5_TRACK_CELL td5_track_read_cell(uint32 descriptor, uint32 position, sint32 read_lanes)
{
    TD5_TRACK_CELL cell = {0};
    uint32 value;
    uint32 base;
    sint32 left_offset;
    sint32 right_offset;

    cell.type = r_u8(descriptor);
    if (read_lanes != 0)
        cell.lanes = r_u8(descriptor + 3) & 15;
    cell.left = r_u16(descriptor + 4);
    cell.right = r_u16(descriptor + 6);
    value = r_u32(position);
    base = r_u32(descriptor + 12);
    left_offset = r_s8(0x8007DD34 + cell.type * 2);
    right_offset = r_s8(0x8007DD35 + cell.type * 2);
    cell.relative.vx = (sint32)((uint32)((sint32)value >> 8) - base);
    value = r_u32(position + 4);
    base = r_u32(descriptor + 16);
    cell.relative.vy = (sint32)((uint32)((sint32)value >> 8) - base);
    value = r_u32(position + 8);
    base = r_u32(descriptor + 20);
    cell.left += (uint32)left_offset;
    cell.right += (uint32)right_offset;
    cell.relative.vz = (sint32)((uint32)((sint32)value >> 8) - base);
    return cell;
}

static TD5_TRACK_EDGE td5_track_read_edge(uint32 points, uint32 start, uint32 end, const VECTOR *relative)
{
    TD5_TRACK_EDGE edge;
    uint32 cross;

    edge.points = points;
    edge.start = points + start * 6;
    edge.end = points + end * 6;
    edge.start_z = r_s16(edge.start + 4);
    edge.end_x = r_s16(edge.end);
    edge.start_x = r_s16(edge.start);
    edge.end_z = r_s16(edge.end + 4);
    cross = ((uint32)relative->vz - (uint32)edge.start_z) * (uint32)(edge.end_x - edge.start_x);
    cross -= ((uint32)relative->vx - (uint32)edge.start_x) * (uint32)(edge.end_z - edge.start_z);
    edge.cross = (sint32)cross;
    return edge;
}

static TD5_TRACK_TRANSITION td5_track_transition(uint32 node, uint32 type, uint32 lane, sint32 direction)
{
    TD5_TRACK_TRANSITION next;
    uint32 base;
    uint32 current;
    uint32 target_lanes;
    uint32 current_lanes;
    uint32 target_header;
    uint32 current_header;
    uint32 link = direction > 0 ? 8 : 10;

    next.node = node + (uint32)direction;
    next.lane = lane;
    base = r_u32(0x800884C4);
    current = base + node * 24;
    if (type == (direction > 0 ? 10 : 9))
    {
        next.node = (uint32)r_s16(current + link);
        target_lanes = r_u8(base + next.node * 24 + 3);
        current_lanes = r_u8(current + 3);
        next.lane += (target_lanes & 15) - (current_lanes & 15);
    }
    else if (type == (direction > 0 ? 8 : 11))
    {
        target_lanes = r_u8(current + (direction > 0 ? 27 : (uint32)-21)) & 15;
        if ((sint32)lane >= (sint32)target_lanes)
        {
            next.node = (uint32)r_s16(current + link);
            current_lanes = r_u8(current + 3);
            target_lanes = r_u8(base + next.node * 24 + 3);
            next.lane -= (current_lanes & 15) - (target_lanes & 15);
        }
    }
    else
    {
        target_header = r_u32(base + next.node * 24);
        current_header = r_u32(current);
        next.lane -= (target_header >> 28) - (current_header >> 28);
    }
    return next;
}

static uint32 td5_track_increment_count(uint32 node, sint32 reload_base)
{
    uint32 count;
    uint32 maximum;
    sint32 signed_maximum;
    uint32 base = 0;

    count = r_u16(node + 4) + 1;
    maximum = r_u16(node + 6);
    signed_maximum = r_s16(node + 6);
    w_u16(node + 4, count);
    if (signed_maximum < (sint16)count)
        maximum = count;
    if (reload_base != 0)
        base = r_u32(0x800884C4);
    w_u16(node + 6, maximum);
    return base;
}

uint32 td5_8001b310(uint32 node, uint32 position)
{
    uint32 current;
    uint32 base;
    uint32 lane;
    uint32 mask;
    uint32 points;
    uint32 left;
    uint32 right;
    uint32 classification;
    uint32 count;
    uint32 cross;
    sint32 previous_z;
    sint32 previous_x;
    sint32 next_x;
    sint32 next_z;
    TD5_TRACK_CELL cell;
    TD5_TRACK_TRANSITION next;
    TD5_TRACK_EDGE edge;
    TD5_TRACK_EDGE adjacent;

    FUNCTION_MARKER(0x8001B310, "SLUS_006.10");
    current = (uint32)r_s16(node);
    base = r_u32(0x800884C4);
    lane = r_u8(node + 12);
    cell = td5_track_read_cell(base + current * 24, position, 1);
    mask = 15;
    if (lane == 0)
        mask = r_u8(0x8007DD1C + cell.type * 2) & 15;
    if (lane == cell.lanes - 1)
        mask &= r_u8(0x8007DD1D + cell.type * 2);
    points = r_u32(0x8008850C);
    right = points + (cell.right + lane) * 6;
    left = points + (cell.left + lane) * 6;
    classification = td5_8001b1b8(mask, &cell.relative, left, left + 6, right + 6, right);
    if (classification == 1 || classification == 4)
    {
        mask = 15;
        if (classification == 1)
        {
            if ((sint32)(lane - 1) <= 0)
                mask = r_u8(0x8007DD1C + cell.type * 2);
        }
        else if ((sint32)(lane + 1) >= (sint32)(cell.lanes - 1))
            mask = r_u8(0x8007DD1D + cell.type * 2);
        if ((mask & 2) != 0)
        {
            points = r_u32(0x8008850C);
            right = cell.right + lane;
            edge = td5_track_read_edge(points, right + (classification == 1 ? 0 : 2), right + (classification == 1 ? (uint32)-1 : 1), &cell.relative);
            if (edge.cross > 0)
            {
                next = td5_track_transition(current, cell.type, lane, 1);
                lane = next.lane;
                td5_track_increment_count(node, 0);
                w_u16(node, next.node);
                w_u8(node + 12, lane + (classification == 1 ? (uint32)-1 : 1));
                return classification;
            }
        }
        if ((mask & 8) != 0)
        {
            points = r_u32(0x8008850C);
            left = cell.left + lane;
            edge = td5_track_read_edge(points, left + (classification == 1 ? (uint32)-1 : 1), left + (classification == 1 ? 0 : 2), &cell.relative);
            if (edge.cross > 0)
            {
                next = td5_track_transition(current, cell.type, lane, -1);
                lane = next.lane;
                count = r_u16(node + 4);
                w_u16(node, next.node);
                w_u16(node + 4, count - 1);
            }
        }
        w_u8(node + 12, lane + (classification == 1 ? (uint32)-1 : 1));
        return classification;
    }
    if (classification != 2 && classification != 3 && classification != 6 && classification != 8 && classification != 9 && classification != 12)
        return classification;
    next = td5_track_transition(current, cell.type, lane, classification < 8 ? 1 : -1);
    lane = next.lane;
    if (classification < 8)
        base = td5_track_increment_count(node, 1);
    else
    {
        count = r_u16(node + 4);
        w_u16(node + 4, count - 1);
        base = r_u32(0x800884C4);
    }
    cell = td5_track_read_cell(base + next.node * 24, position, classification == 3 || classification == 9 ? 0 : 1);
    mask = 15;
    if (classification == 2 || classification == 8)
    {
        if ((sint32)lane <= 0)
            mask = r_u8(0x8007DD1C + cell.type * 2);
        if ((sint32)lane >= (sint32)(cell.lanes - 1))
            mask &= r_u8(0x8007DD1D + cell.type * 2);
        if ((mask & 1) != 0)
        {
            points = r_u32(0x8008850C);
            edge = td5_track_read_edge(points, cell.right + lane, cell.left + lane, &cell.relative);
            if (edge.cross > 0)
            {
                w_u8(node + 12, lane - 1);
                w_u16(node, next.node);
                return classification;
            }
        }
        if ((mask & 4) != 0)
        {
            points = r_u32(0x8008850C);
            edge = td5_track_read_edge(points, cell.left + lane + 1, cell.right + lane + 1, &cell.relative);
            if (edge.cross > 0)
            {
                w_u8(node + 12, lane + 1);
                w_u16(node, next.node);
                return classification;
            }
        }
        w_u8(node + 12, lane);
        w_u16(node, next.node);
        return classification;
    }
    if (classification == 3 || classification == 9)
    {
        if ((sint32)lane <= 0)
            mask = r_u8(0x8007DD1C + cell.type * 2);
        if ((mask & 1) != 0)
        {
            points = r_u32(0x8008850C);
            edge = td5_track_read_edge(points, cell.right + lane, cell.left + lane, &cell.relative);
            if (edge.cross > 0)
            {
                mask = 15;
                if ((sint32)(lane - 1) <= 0)
                    mask = r_u8(0x8007DD1C + cell.type * 2);
                if ((mask & (classification == 3 ? 8 : 2)) != 0)
                {
                    if (classification == 3)
                    {
                        previous_z = r_s16(edge.end - 2);
                        previous_x = r_s16(edge.end - 6);
                        cross = ((uint32)cell.relative.vz - (uint32)previous_z) * (uint32)(edge.end_x - previous_x);
                        cross -= ((uint32)cell.relative.vx - (uint32)previous_x) * (uint32)(edge.end_z - previous_z);
                    }
                    else
                    {
                        next_x = r_s16(edge.start - 6);
                        next_z = r_s16(edge.start - 2);
                        cross = ((uint32)cell.relative.vz - (uint32)edge.start_z) * (uint32)(next_x - edge.start_x);
                        cross -= ((uint32)cell.relative.vx - (uint32)edge.start_x) * (uint32)(next_z - edge.start_z);
                    }
                    if ((sint32)cross > 0)
                    {
                        w_u8(node + 12, r_u8(node + 12) - 1);
                        return classification;
                    }
                }
                if (classification == 3)
                {
                    w_u16(node, next.node);
                    w_u8(node + 12, lane - 1);
                }
                else
                {
                    w_u8(node + 12, lane - 1);
                    w_u16(node, next.node);
                }
                return classification;
            }
        }
        if (classification == 3)
        {
            w_u16(node, next.node);
            w_u8(node + 12, lane);
        }
        else
        {
            w_u8(node + 12, lane);
            w_u16(node, next.node);
        }
        return classification;
    }
    if ((sint32)lane >= (sint32)(cell.lanes - 1))
        mask = r_u8(0x8007DD1D + cell.type * 2);
    if ((mask & 4) != 0)
    {
        points = r_u32(0x8008850C);
        edge = td5_track_read_edge(points, cell.left + lane + 1, cell.right + lane + 1, &cell.relative);
        if (edge.cross > 0)
        {
            mask = 15;
            if ((sint32)(lane + 1) >= (sint32)(cell.lanes - 1))
                mask = r_u8(0x8007DD1D + cell.type * 2);
            if ((mask & (classification == 6 ? 8 : 2)) != 0)
            {
                if (classification == 6)
                    adjacent = td5_track_read_edge(edge.points, cell.left + lane + 1, cell.left + lane + 2, &cell.relative);
                else
                    adjacent = td5_track_read_edge(edge.points, cell.right + lane + 2, cell.right + lane + 1, &cell.relative);
                if (adjacent.cross > 0)
                {
                    w_u8(node + 12, r_u8(node + 12) + 1);
                    return classification;
                }
            }
            w_u8(node + 12, lane + 1);
            w_u16(node, next.node);
            return classification;
        }
    }
    w_u8(node + 12, lane);
    w_u16(node, next.node);
    return classification;
}

uint32 td5_80020030(uint32 car, uint32 height_address)
{
    uint32 saved_model;
    sint32 model_height;
    sint32 saved_node;
    uint32 saved_lane;
    sint32 height;
    uint32 old_mask;
    uint32 index;
    uint32 world;
    uint32 node;
    uint32 vector;
    uint32 old_position[3];
    uint32 value;
    uint32 model;
    sint32 suspension;
    uint32 transformed;
    uint32 coordinates[3];
    uint32 ground;
    uint32 gravity;
    uint32 deviation;
    uint32 mask = 0;
    sint32 step;

    FUNCTION_MARKER(0x80020030, "SLUS_006.10");
    saved_model = r_u32(car + 0x180);
    model_height = r_s16(saved_model + 0x82);
    saved_node = r_s16(car + 0x70);
    saved_lane = r_u8(car + 0x7C);
    height = model_height * 181 / 256;
    SetRotMatrix((MATRIX *)psx_addr(car + 0x110, 20));
    SetTransMatrix((MATRIX *)psx_addr(car + 0x110, 32));
    old_mask = r_u8(car + 0x354);
    w_u8(car + 0x355, old_mask);
    for (index = 0; index < 4; ++index)
    {
        world = car + 0xE0 + index * 12;
        old_position[0] = r_u32(world);
        old_position[1] = r_u32(world + 4);
        old_position[2] = r_u32(world + 8);
        node = car + 0x38 + index * 14;
        w_u16(node, saved_node);
        w_u8(node + 12, saved_lane);
        model = r_u32(car + 0x180);
        suspension = r_s32(car + 0x2B4 + index * 4) / 256;
        value = r_u16(model + 0x42 + index * 8) - (uint32)suspension - (uint32)height;
        vector = car + 0x1D8 + index * 8;
        w_u16(vector + 2, value);
        xport_gte_write_data(0, r_u32(vector));
        xport_gte_write_data(1, r_u32(vector + 4));
        xport_gte_execute(0x480012);
        w_u32(world, xport_gte_read_data(25));
        w_u32(world + 4, xport_gte_read_data(26));
        w_u32(world + 8, xport_gte_read_data(27));
        value = r_u16(vector + 2);
        w_u16(vector + 2, value + (uint32)height);
        xport_gte_write_data(0, r_u32(vector));
        xport_gte_write_data(1, r_u32(vector + 4));
        xport_gte_execute(0x480012);
        transformed = car + 0x260 + index * 16;
        w_u32(transformed, xport_gte_read_data(25));
        w_u32(transformed + 4, xport_gte_read_data(26));
        w_u32(transformed + 8, xport_gte_read_data(27));
        coordinates[0] = r_u32(world);
        coordinates[1] = r_u32(world + 4);
        coordinates[2] = r_u32(world + 8);
        w_u32(world, coordinates[0] << 8);
        w_u32(world + 4, coordinates[1] << 8);
        w_u32(world + 8, coordinates[2] << 8);
        td5_8001b310(node, world);
        td5_8001d314(node, world, car + 0x218 + index * 8, (uint32 *)psx_addr(height_address, 4));
        value = r_u32(world + 4);
        ground = r_u32(height_address);
        gravity = r_u32(0x800877A4);
        deviation = car + 0x2D4 + index * 4;
        w_u32(deviation, value - ground + gravity);
        value = r_u32(world);
        step = (sint32)(value - old_position[0]) / 256;
        w_u16(car + 0x238 + index * 8, step);
        value = r_u32(world + 4);
        step = (sint32)(value - old_position[1]) / 256;
        w_u16(car + 0x23A + index * 8, step);
        value = r_u32(world + 8);
        step = (sint32)(value - old_position[2]) / 256;
        w_u16(car + 0x23C + index * 8, step);
        value = r_u32(deviation);
        if ((sint32)value < 0)
            value = 0 - value;
        if ((sint32)value < 512)
            w_u32(deviation, 0);
        if (r_s32(deviation) >= 2049)
        {
            mask |= 1 << index;
            w_u32(deviation, 12000);
        }
        else
            w_u32(world + 4, r_u32(height_address));
    }
    w_u8(car + 0x354, mask);
    for (index = 0; index < 4; ++index)
    {
        node = car + index * 14;
        w_u16(node, saved_node);
        w_u8(node + 12, saved_lane);
        vector = saved_model + index * 8;
        world = car + 0x80 + index * 12;
        xport_gte_write_data(0, r_u32(vector));
        xport_gte_write_data(1, r_u32(vector + 4));
        xport_gte_execute(0x480012);
        w_u32(world, xport_gte_read_data(25));
        w_u32(world + 4, xport_gte_read_data(26));
        w_u32(world + 8, xport_gte_read_data(27));
        coordinates[0] = r_u32(world);
        coordinates[1] = r_u32(world + 4);
        coordinates[2] = r_u32(world + 8);
        w_u32(world, coordinates[0] << 8);
        w_u32(world + 4, coordinates[1] << 8);
        w_u32(world + 8, coordinates[2] << 8);
        td5_8001b310(node, world);
        td5_8001cd00(node, world, world + 4);
    }
    return 0;
}

uint32 td5_8001cd00(uint32 node, uint32 position, uint32 height_address)
{
    VECTOR relative = {0};
    sint32 node_index;
    uint32 descriptor;
    uint32 type;
    uint32 first;
    uint32 second;
    sint32 offset;
    uint32 lane;
    uint32 lanes;
    uint32 value;
    uint32 base;
    sint32 action = 0;
    sint32 triangle_first;
    sint32 triangle_second;
    sint32 triangle_third;
    uint32 points;
    uint32 first_point;
    uint32 second_point;
    sint32 first_z;
    sint32 second_x;
    sint32 first_x;
    sint32 second_z;
    uint32 cross;
    uint32 result;

    FUNCTION_MARKER(0x8001CD00, "SLUS_006.10");
    node_index = r_s16(node);
    descriptor = r_u32(0x800884C4) + (uint32)node_index * 24;
    type = r_u8(descriptor);
    first = r_u16(descriptor + 4);
    offset = r_s8(0x8007DD34 + type * 2);
    lane = r_u8(node + 12);
    lanes = r_u8(descriptor + 3);
    first += (uint32)offset + lane;
    w_u16(node + 8, first);
    second = r_u16(descriptor + 6);
    offset = r_s8(0x8007DD35 + type * 2);
    second += (uint32)offset + lane;
    w_u16(node + 10, second);
    value = r_u32(position);
    base = r_u32(descriptor + 12);
    relative.vx = (sint32)((uint32)((sint32)value >> 8) - base);
    value = r_u32(position + 4);
    base = r_u32(descriptor + 16);
    relative.vy = (sint32)((uint32)((sint32)value >> 8) - base);
    value = r_u32(position + 8);
    base = r_u32(descriptor + 20);
    relative.vz = (sint32)((uint32)((sint32)value >> 8) - base);
    lanes &= 15;
    if (lane == 0)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 3 || type == 4)
            action = 2;
        else if (type == 6 || type == 7)
            action = 1;
    }
    else if (lane == lanes - 1)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 2 || type == 4)
            action = -1;
        else if (type == 5 || type == 7)
            action = 3;
    }
    if (action == 0)
    {
        points = r_u32(0x8008850C);
        first_point = points + first * 6;
        second_point = points + second * 6 + 6;
        first_z = r_s16(first_point + 4);
        second_x = r_s16(second_point);
        first_x = r_s16(first_point);
        second_z = r_s16(second_point + 4);
        cross = ((uint32)relative.vz - (uint32)first_z) * (uint32)(second_x - first_x);
        cross -= ((uint32)relative.vx - (uint32)first_x) * (uint32)(second_z - first_z);
        action = (sint32)cross > 0 ? 1 : -1;
    }
    triangle_first = (sint16)(first + (action == 2 ? 1 : 0));
    triangle_second = (sint16)(second + (action == 1 ? 1 : 0));
    triangle_third = (sint16)((action == 1 || action == 3 ? first : second) + 1);
    result = td5_8001cb58(triangle_first, triangle_second, triangle_third, &relative);
    base = r_u32(descriptor + 16);
    result += base << 8;
    w_u32(height_address, result);
    return result;
}

uint32 td5_8001cb58(sint32 first, sint32 second, sint32 third, const VECTOR *position)
{
    VECTOR left = {0};
    VECTOR right = {0};
    VECTOR normal = {0};
    uint32 points;
    uint32 first_offset = (uint32)(sint16)first * 6;
    uint32 a;
    uint32 b;
    uint32 c;
    sint32 first_value;
    sint32 other_value;
    sint32 position_value;
    sint32 normal_x;
    sint32 normal_y;
    sint32 normal_z;
    uint32 dot;
    sint32 quotient;

    FUNCTION_MARKER(0x8001CB58, "SLUS_006.10");
    points = r_u32(0x8008850C);
    a = points + first_offset;
    b = points + (uint32)(sint16)second * 6;
    first_value = r_s16(a);
    other_value = r_s16(b);
    left.vx = first_value - other_value;
    first_value = r_s16(a + 2);
    other_value = r_s16(b + 2);
    left.vy = first_value - other_value;
    first_value = r_s16(a + 4);
    other_value = r_s16(b + 4);
    left.vz = first_value - other_value;
    first_value = r_s16(a);
    c = points + (uint32)(sint16)third * 6;
    other_value = r_s16(c);
    right.vx = first_value - other_value;
    first_value = r_s16(a + 2);
    other_value = r_s16(c + 2);
    right.vy = first_value - other_value;
    first_value = r_s16(a + 4);
    other_value = r_s16(c + 4);
    right.vz = first_value - other_value;
    OuterProduct0(&left, &right, &normal);
    normal_x = normal.vx >> 4;
    normal_y = normal.vy >> 4;
    normal_z = normal.vz >> 4;
    if (normal_y == 0)
        normal_y = 1;
    points = r_u32(0x8008850C);
    position_value = position->vx;
    a = points + first_offset;
    first_value = r_s16(a);
    dot = ((uint32)first_value - (uint32)position_value) * (uint32)normal_x;
    first_value = r_s16(a + 4);
    position_value = position->vz;
    dot += ((uint32)first_value - (uint32)position_value) * (uint32)normal_z;
    quotient = td5_divide_checked((sint32)dot, normal_y);
    first_value = r_s16(a + 2);
    return ((uint32)quotient + (uint32)first_value) << 8;
}

void td5_80068500(void)
{
    FUNCTION_MARKER(0x80068500, "SLUS_006.10");
    ExitCriticalSection();
}

sint32 td5_800684f0(void)
{
    FUNCTION_MARKER(0x800684F0, "SLUS_006.10");
    return EnterCriticalSectionPSX();
}

sint32 td5_8005bd80(uint32 position)
{
    FUNCTION_MARKER(0x8005BD80, "SLUS_006.10");
    return CdPosToInt((CdlLOC *)psx_addr(position, 3));
}

sint32 td5_8005bbf8(uint32 destination, uint32 words)
{
    FUNCTION_MARKER(0x8005BBF8, "SLUS_006.10");
    return CdGetSector(psx_addr(destination, words * 4), (sint32)words);
}

uint32 td5_8005ded0(void)
{
    uint32 callback;
    uint32 previous;

    FUNCTION_MARKER(0x8005DED0, "SLUS_006.10");
    callback = r_u32(0x80080018);
    previous = CdSyncCallbackPSX(callback);
    w_u32(0x80080014, 0);
    return previous;
}

uint32 td5_80064ca0(void)
{
    FUNCTION_MARKER(0x80064CA0, "SLUS_006.10");
    return _SpuInit(0);
}

sint32 td5_8005bbd8(uint32 volume)
{
    FUNCTION_MARKER(0x8005BBD8, "SLUS_006.10");
    CdMix((CdlATV *)psx_addr(volume, 4));
    return 1;
}

uint32 td5_800574f0(sint32 selected, sint32 count)
{
    sint32 index;
    uint32 value;
    uint32 item;
    uint32 argument;

    FUNCTION_MARKER(0x800574F0, "SLUS_006.10");
    if (selected < 0)
        index = 0;
    else
    {
        index = (sint32)((uint32)count - 1);
        if (index >= selected)
            index = selected;
    }
    value = r_u16(0x800884E8);
    if ((value & 0x4008) != 0)
    {
        item = r_u32(0x800BF3A8) + (uint32)index * 24;
        argument = r_u32(item + 16);
        if (argument != 0)
        {
            td5_800a98f0(argument);
            td5_800530f0();
        }
    }
    value = r_u16(0x800884E8) & 0x1000;
    if (value != 0)
    {
        td5_800a99f0();
        return (uint32)td5_800530f0();
    }
    return value;
}

sint32 td5_800557f0(uint32 original_stack)
{
    uint32 counter;
    uint32 response = original_stack - 0x38 + 0x18;
    CdlLOC position = {0};
    uint32 parameters = original_stack - 0x38 + 0x20;
    uint32 seek_position = original_stack - 0x38 + 0x28;
    sint32 sector;
    sint32 end;
    sint32 begin;
    uint32 mode;

    FUNCTION_MARKER(0x800557F0, "SLUS_006.10");
    counter = r_u32(0x80087BF0);
    w_u32(0x80087BF0, counter + 1);
    if ((sint32)counter < 11)
        return 1;
    w_u32(0x80087BF0, 0);
    CdControl(0x10, 0, (uint8 *)psx_addr(response, 8));
    position.minute = r_u8(response);
    position.second = r_u8(response + 1);
    position.sector = r_u8(response + 2);
    position.track = 0;
    sector = CdPosToInt(&position);
    end = r_s32(0x800882EC);
    w_u32(0x800882F4, (uint32)sector);
    if (end >= sector)
    {
        begin = r_s32(0x800882E8);
        if (sector >= begin)
            return 0;
    }
    begin = r_s32(0x800882E8);
    mode = r_u32(0x800882F0);
    w_u32(0x800882EC, (uint32)end);
    w_u8(parameters, 1);
    w_u32(0x800882E8, (uint32)begin);
    w_u32(0x800882F4, (uint32)begin);
    w_u32(0x800882F0, mode);
    w_u8(parameters + 1, mode);
    CdControlF(0x0D, (uint8 *)psx_addr(parameters, 8));
    begin = r_s32(0x800882E8);
    CdIntToPos(begin, (CdlLOC *)psx_addr(seek_position, 3));
    return CdControlF(0x1B, (uint8 *)psx_addr(seek_position, 8));
}

uint32 td5_80042c50(void)
{
    uint32 level;
    uint32 entries;
    uint32 names = 0x8007E3BC;
    uint32 textures = 0x800A3B78;
    uint32 index;
    uint32 name;
    uint32 length;
    uint32 value;
    uint32 row;
    uint32 column;
    uint32 primitive;

    FUNCTION_MARKER(0x80042C50, "SLUS_006.10");
    level = r_u32(0x80088770);
    w_u32(0x80088554, 0);
    w_u32(0x80088550, 0);
    w_u32(0x800884BC, 0);
    w_u32(0x800884B8, 0);
    w_u32(0x8008862C, 0);
    w_u32(0x80088628, 0);
    entries = r_u32(level + 48);
    w_u32(0x80088540, 0);
    w_u32(0x80088650, entries);
    for (index = 0; index < 6; ++index)
    {
        name = r_u32(names);
        names += 4;
        length = 0;
        while (r_u8(name + length) != 0)
            ++length;
        value = td5_find_texture((const char *)psx_addr(name, length + 1));
        w_u32(textures, value);
        textures += 4;
    }
    level = r_u32(0x80088770);
    level = r_u32(level);
    value = r_u8(level + 30) & 0x10;
    if (value == 0)
        return value;
    index = r_u32(0x80088540);
    entries = r_u32(0x80088650);
    value = r_u16(entries + index * 6);
    while (value != 0xFFFF)
    {
        ++index;
        value = r_u16(entries + index * 6);
        w_u32(0x80088540, index);
    }
    for (row = 0; row < 4; ++row)
    {
        for (column = 0; column < 2; ++column)
        {
            primitive = 0x8008CB70 + row * 80 + column * 40;
            w_u8(primitive + 3, 9);
            w_u8(primitive + 7, 0x2E);
        }
    }
    return 0;
}

uint32 td5_800430b0(void)
{
    uint32 index;
    uint32 primitive;
    uint32 texture;
    uint32 value;
    uint32 extent;
    uint32 shade;

    FUNCTION_MARKER(0x800430B0, "SLUS_006.10");
    value = r_u32(0x80087724);
    if (value == 0 && r_u32(0x8008771C) != 3)
        return 3;
    w_u32(0x800886D0, 0xFFFFFFFF);
    w_u16(0x800885C8, 0);
    w_u16(0x800883A0, 0);
    w_u32(0x8008792C, 0);
    w_u32(0x80087930, 0xFFFF);
    w_u32(0x80087934, 0);
    w_u32(0x80087938, 0);
    w_u8(0x800879B4, 0);
    for (index = 0; index < 6; ++index)
        w_u16(0x8009DBAA - index * 2, 4096);
    for (index = 0; index < 2; ++index)
    {
        primitive = 0x8009D988 + index * 40;
        w_u8(primitive + 3, 9);
        w_u8(primitive + 7, 0x2C);
        texture = r_u32(0x800A3B78);
        value = r_u8(texture + 14);
        w_u8(primitive + 12, value);
        texture = r_u32(0x800A3B78);
        value = r_u8(texture + 15);
        w_u8(primitive + 13, value);
        texture = r_u32(0x800A3B78);
        value = r_u8(texture + 14);
        extent = r_u8(texture + 12);
        w_u8(primitive + 20, value + extent + 255);
        texture = r_u32(0x800A3B78);
        value = r_u8(texture + 15);
        w_u8(primitive + 21, value);
        texture = r_u32(0x800A3B78);
        value = r_u8(texture + 14);
        w_u8(primitive + 28, value);
        texture = r_u32(0x800A3B78);
        value = r_u8(texture + 15);
        extent = r_u8(texture + 13);
        w_u8(primitive + 29, value + extent + 255);
        texture = r_u32(0x800A3B78);
        value = r_u8(texture + 14);
        extent = r_u8(texture + 12);
        w_u8(primitive + 36, value + extent + 255);
        texture = r_u32(0x800A3B78);
        value = r_u8(texture + 15);
        extent = r_u8(texture + 13);
        w_u8(primitive + 37, value + extent + 255);
        texture = r_u32(0x800A3B78);
        value = r_u16(texture + 8);
        w_u16(primitive + 22, value);
        texture = r_u32(0x800A3B78);
        value = r_u16(texture + 10);
        w_u16(primitive + 14, value);
        value = r_u8(primitive + 7);
        w_u8(primitive + 7, (value | 1) & 0xFD);
        shade = 0x80091580 + index * 36;
        w_u8(shade + 3, 8);
        w_u8(shade + 7, 0x38);
        w_u8(shade + 4, 0);
        w_u8(shade + 5, 255);
        w_u8(shade + 6, 0);
        w_u8(shade + 12, 0);
        w_u8(shade + 13, 255);
        w_u8(shade + 14, 0);
        w_u8(shade + 20, 255);
        w_u8(shade + 21, 0);
        w_u8(shade + 22, 0);
        w_u8(shade + 28, 255);
        w_u8(shade + 29, 0);
        w_u8(shade + 30, 0);
        shade = 0x800915C8 + index * 36;
        w_u8(shade + 3, 8);
        w_u8(shade + 7, 0x38);
        w_u8(shade + 4, 32);
        w_u8(shade + 5, 0);
        w_u8(shade + 6, 0);
        w_u8(shade + 12, 96);
        w_u8(shade + 13, 0);
        w_u8(shade + 14, 0);
        w_u8(shade + 20, 32);
        w_u8(shade + 21, 0);
        w_u8(shade + 22, 0);
        w_u8(shade + 28, 96);
        w_u8(shade + 29, 0);
        w_u8(shade + 30, 0);
    }
    return 0;
}
