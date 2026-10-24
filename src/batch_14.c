#include "td5_functions.h"
#include <stdio.h>
#include <string.h>

uint32 td5_800284f0(uint32 incoming_v1, uint32 original_stack)
{
    uint32 counter, slot, count, car, value, next, pool, record, type, side, i;
    uint32 height_address = original_stack - 0x20 - 0x28 - 0x18 - 0x30 - 0x98 + 0x20;
    sint32 current;

    FUNCTION_MARKER(0x800284F0, "SLUS_006.10");
    if (incoming_v1 == 1)
        return 1;
    counter = r_u32(0x80087834);
    w_u32(0x80087834, counter + 1);
    if ((sint32)counter < 91)
        return 1;
    value = r_u32(0x8008772C);
    count = r_u32(0x800875E4);
    w_u32(0x80087834, 0);
    slot = value + 1;
    for (;;)
    {
        if ((sint32)count >= 7)
        {
            if ((sint32)slot >= 6)
                return slot << 3;
        }
        else if ((sint32)slot >= (sint32)count)
            return slot << 3;
        car = 0x800984A0 + slot * 864;
        value = r_u8(car + 0x35D);
        if (value == 0)
        {
            current = r_s16(car + 0x74);
            next = (uint32)r_s16(car + 0x33A);
            if (current + 1 >= (sint32)next)
            {
                pool = r_u32(0x800884C4);
                for (i = 0; i < 4; ++i)
                {
                    record = pool + next * 24;
                    type = r_u8(record);
                    ++next;
                    if (type == 10)
                        next = (uint32)r_s16(record + 8);
                }
                pool = r_u32(0x800884C4);
                side = r_u8(pool + next * 24 + 3);
                td5_8004d984(slot, next, (side & 15) >> 1, 0, height_address);
            }
        }
        value = r_u16(car + 0x74);
        count = r_u32(0x800875E4);
        ++slot;
        w_u16(car + 0x33A, value);
    }
}

uint32 td5_80029388(uint32 car, sint32 angle, sint32 distance)
{
    uint32 cosine, sine, scale, x, z, position_x, position_z, value, result;
    sint32 forward, sideways, reflected, excess;

    FUNCTION_MARKER(0x80029388, "SLUS_006.10");
    cosine = (uint32)rcos(angle);
    sine = (uint32)rsin(angle);
    scale = (uint32)(distance / 4096) - 4;
    position_x = (uint32)((sint32)((0 - scale) * sine) / 16);
    position_z = (uint32)((sint32)(scale * cosine) / 16);
    x = r_u32(car + 0x194);
    z = r_u32(car + 0x19C);
    value = r_u32(car + 0x1C4);
    result = r_u32(car + 0x1CC);
    w_u32(car + 0x1C4, value + position_x);
    w_u32(car + 0x1CC, result + position_z);
    forward = (sint32)(x * cosine + z * sine) / 4096;
    sideways = (sint32)(z * cosine - x * sine) / 4096;
    result = 0 - (uint32)sideways;
    if (sideways < 0)
        return result;
    reflected = (sint32)result / 2;
    if (forward > 384)
        excess = forward - 384;
    else if (forward < -384)
        excess = forward + 384;
    else
        excess = 0;
    value = (uint32)excess * cosine - (uint32)reflected * sine;
    w_u32(car + 0x194, (uint32)((sint32)value / 4096));
    value = (uint32)reflected * cosine + (uint32)excess * sine;
    result = (uint32)((sint32)value / 4096);
    w_u32(car + 0x19C, result);
    w_u8(car + 0x353, 0);
    return result;
}

uint32 td5_8002c60c(uint32 car, uint32 angles)
{
    uint32 cosine, sine, first, second, value, result, square;
    sint32 angle;
    sint16 x, y, z;

    FUNCTION_MARKER(0x8002C60C, "SLUS_006.10");
    angle = r_s16(car + 0x1D2);
    cosine = (uint32)rcos(angle);
    sine = (uint32)rsin(angle);
    first = (uint32)r_s16(car + 0x258);
    second = (uint32)r_s16(car + 0x25C);
    value = first * cosine - second * sine;
    x = (sint16)((sint32)value / 4096);
    y = (sint16)r_u16(car + 0x25A);
    first = (uint32)r_s16(car + 0x258);
    second = (uint32)r_s16(car + 0x25C);
    value = first * sine + second * cosine;
    z = (sint16)((sint32)value / 4096);
    result = (uint32)td5_8007b0c0(-(sint32)z, -(sint32)y);
    w_u16(angles, result);
    square = (uint32)(sint32)y * (uint32)(sint32)y;
    square += (uint32)(sint32)z * (uint32)(sint32)z;
    value = td5_integer_sqrt(square);
    result = (uint32)td5_8007b0c0(x, (sint32)value);
    w_u16(angles + 4, result);
    return result;
}

uint32 td5_8002c73c(uint32 car, uint32 original_stack)
{
    uint32 height_address = original_stack - 0x18 - 0x18 - 0x30 - 0x98 + 0x20;
    sint32 counter;

    FUNCTION_MARKER(0x8002C73C, "SLUS_006.10");
    counter = r_s16(car + 0x310);
    if (counter < 60)
        return 1;
    return td5_80024134(car, height_address);
}

uint32 td5_8002ca98(uint32 car, sint32 angle, uint32 distance)
{
    uint32 cosine, sine, move_x, move_z, x, z, value, other, y, result, i;
    sint32 forward, sideways, reflected, slowed;

    FUNCTION_MARKER(0x8002CA98, "SLUS_006.10");
    cosine = (uint32)rcos(angle);
    sine = (uint32)rsin(angle);
    move_x = (uint32)((sint32)((0 - distance) * sine) / 16);
    move_z = (uint32)((sint32)(distance * cosine) / 16);
    x = r_u32(car + 0x1C4);
    z = r_u32(car + 0x1CC);
    w_u32(car + 0x1C4, x + move_x);
    w_u32(car + 0x1CC, z + move_z);
    for (i = 0; i < 8; ++i)
    {
        x = r_u32(car + 0x80 + i * 12);
        z = r_u32(car + 0x88 + i * 12);
        w_u32(car + 0x80 + i * 12, x + move_x);
        w_u32(car + 0x88 + i * 12, z + move_z);
    }
    x = r_u32(car + 0x194);
    z = r_u32(car + 0x19C);
    value = r_u32(car + 0x1C4);
    y = r_u32(car + 0x1C8);
    other = r_u32(car + 0x1CC);
    w_u32(car + 0x124, (uint32)((sint32)value >> 8));
    w_u32(car + 0x128, (uint32)((sint32)y >> 8));
    w_u32(car + 0x12C, (uint32)((sint32)other >> 8));
    forward = (sint32)(x * cosine + z * sine) / 4096;
    sideways = (sint32)(z * cosine - x * sine) / 4096;
    result = 0 - (uint32)sideways;
    if (sideways < 0)
        return result;
    reflected = (sint32)result / 16;
    slowed = forward - forward / 128;
    if ((forward < 0 && slowed >= 0) || (forward >= 0 && slowed < 0))
        slowed = 0;
    value = (uint32)slowed * cosine - (uint32)reflected * sine;
    w_u32(car + 0x194, (uint32)((sint32)value / 4096));
    value = (uint32)reflected * cosine + (uint32)slowed * sine;
    w_u32(car + 0x19C, (uint32)((sint32)value / 4096));
    if (sideways < 8193)
        return 1;
    value = r_u8(car + 0x34D);
    if (value >= 6)
        return 0;
    return (uint32)td5_play_positioned_sound(7, -1, 4096, 4096, car + 0x1C4, 0);
}

uint32 td5_8007a374(uint32 first, uint32 second)
{
    uint64 product;

    FUNCTION_MARKER(0x8007A374, "SLUS_006.10");
    product = (uint64)((sint64)(sint32)first * (sint32)second);
    return (uint32)((product + 0x7FFF) >> 16);
}

uint32 td5_8002d9c0(sint32 first, sint32 second, sint32 third, uint32 output)
{
    uint32 cx, sx, cy, sy, cz, sz, value, left, right;

    FUNCTION_MARKER(0x8002D9C0, "SLUS_006.10");
    first /= 2;
    cx = (uint32)rcos(first) << 4;
    sx = (uint32)rsin(first) << 4;
    second /= 2;
    cy = (uint32)rcos(second) << 4;
    sy = (uint32)rsin(second) << 4;
    third /= 2;
    cz = (uint32)rcos(third) << 4;
    sz = (uint32)rsin(third) << 4;
    value = td5_8007a374(cx, sy);
    left = td5_8007a374(value, cz);
    value = td5_8007a374(sx, cy);
    right = td5_8007a374(value, sz);
    w_u32(output + 4, left - right);
    value = td5_8007a374(cx, sy);
    left = td5_8007a374(value, sz);
    value = td5_8007a374(sx, cy);
    right = td5_8007a374(value, cz);
    w_u32(output, left + right);
    value = td5_8007a374(cx, cy);
    left = td5_8007a374(value, sz);
    value = td5_8007a374(sx, sy);
    right = td5_8007a374(value, cz);
    w_u32(output + 8, left - right);
    value = td5_8007a374(cx, cy);
    left = td5_8007a374(value, cz);
    value = td5_8007a374(sx, sy);
    right = td5_8007a374(value, sz);
    w_u32(output + 12, left + right);
    return right;
}

uint32 td5_8002db80(uint32 axis, sint32 angle, uint32 output)
{
    uint32 value, square, cosine, sine, result, i;
    sint32 length, quotient;

    FUNCTION_MARKER(0x8002DB80, "SLUS_006.10");
    value = (uint32)r_s16(axis);
    square = value * value;
    value = (uint32)r_s16(axis + 2);
    square += value * value;
    value = (uint32)r_s16(axis + 4);
    square += value * value;
    length = (sint32)SquareRoot0((sint32)square);
    if (length == 0)
        length = 1;
    angle /= 2;
    cosine = (uint32)rcos(angle) << 4;
    sine = (uint32)rsin(angle) << 4;
    for (i = 0; i < 3; ++i)
    {
        value = (uint32)r_s16(axis + i * 2) << 16;
        value += (uint32)(length / 2);
        quotient = (sint32)value / length;
        result = td5_8007a374((uint32)quotient, sine);
        w_u32(output + i * 4, result);
    }
    w_u32(output + 12, cosine);
    return result;
}

uint32 td5_8002dd2c(uint32 first, uint32 second, uint32 output)
{
    static const uint8 first_offsets[4][4] = {
        {12, 0, 4, 8}, {12, 4, 8, 0}, {12, 8, 0, 4}, {12, 0, 4, 8}
    };
    static const uint8 second_offsets[4][4] = {
        {0, 12, 8, 4}, {4, 12, 0, 8}, {8, 12, 4, 0}, {12, 0, 4, 8}
    };
    uint32 row, term, left, right, sum, result;

    FUNCTION_MARKER(0x8002DD2C, "SLUS_006.10");
    for (row = 0; row < 4; ++row)
    {
        for (term = 0; term < 4; ++term)
        {
            left = r_u32(first + first_offsets[row][term]);
            right = r_u32(second + second_offsets[row][term]);
            result = td5_8007a374(left, right);
            if (term == 0)
                sum = result;
            else if (row == 3 || term == 3)
                sum -= result;
            else
                sum += result;
        }
        w_u32(output + row * 4, sum);
    }
    return result;
}

uint32 td5_8002debc(uint32 quaternion, uint32 matrix)
{
    uint32 square = 0, value, length, q[4], a, b, c, d, i;

    FUNCTION_MARKER(0x8002DEBC, "SLUS_006.10");
    for (i = 0; i < 4; ++i)
    {
        value = (uint32)(r_s32(quaternion + i * 4) >> 1);
        square += value * value;
    }
    length = SquareRoot0((sint32)square);
    for (i = 0; i < 4; ++i)
    {
        value = r_u32(quaternion + i * 4);
        value = (value << 15) + length / 2;
        if (length == 0)
            xport_mips_break(7);
        q[i] = (uint32)((sint32)value / (sint32)length);
    }
    a = td5_8007a374(q[3], q[3]);
    b = td5_8007a374(q[0], q[0]);
    c = td5_8007a374(q[1], q[1]);
    d = td5_8007a374(q[2], q[2]);
    w_u16(matrix, (uint32)((sint32)(a + b - c - d + 8) >> 4));
    a = td5_8007a374(q[0], q[1]);
    b = td5_8007a374(q[3], q[2]);
    w_u16(matrix + 6, (uint32)((sint32)(a + b + 4) >> 3));
    a = td5_8007a374(q[0], q[2]);
    b = td5_8007a374(q[3], q[1]);
    w_u16(matrix + 12, (uint32)((sint32)(a - b + 4) >> 3));
    a = td5_8007a374(q[0], q[1]);
    b = td5_8007a374(q[3], q[2]);
    w_u16(matrix + 2, (uint32)((sint32)(a - b + 4) >> 3));
    a = td5_8007a374(q[3], q[3]);
    b = td5_8007a374(q[0], q[0]);
    c = td5_8007a374(q[1], q[1]);
    d = td5_8007a374(q[2], q[2]);
    w_u16(matrix + 8, (uint32)((sint32)(a - b + c - d + 8) >> 4));
    a = td5_8007a374(q[1], q[2]);
    b = td5_8007a374(q[3], q[0]);
    w_u16(matrix + 14, (uint32)((sint32)(a + b + 4) >> 3));
    a = td5_8007a374(q[0], q[2]);
    b = td5_8007a374(q[3], q[1]);
    w_u16(matrix + 4, (uint32)((sint32)(a + b + 4) >> 3));
    a = td5_8007a374(q[1], q[2]);
    b = td5_8007a374(q[3], q[0]);
    w_u16(matrix + 10, (uint32)((sint32)(a - b + 4) >> 3));
    a = td5_8007a374(q[3], q[3]);
    b = td5_8007a374(q[0], q[0]);
    c = td5_8007a374(q[1], q[1]);
    d = td5_8007a374(q[2], q[2]);
    w_u16(matrix + 16, (uint32)((sint32)(a - b - c + d + 8) >> 4));
    return d;
}

uint32 td5_8002e3fc(uint32 slot, uint32 original_stack, uint32 texture_v_carrier, uint32 texture_u_carrier)
{
    uint32 car = 0x800984A0 + slot * 864;
    uint32 frame = original_stack - 0x20;
    uint32 object, x, y, z, value, header;

    FUNCTION_MARKER(0x8002E3FC, "SLUS_006.10");
    object = r_u32(car + 0x178);
    x = r_u32(car + 0x1C4);
    y = r_u32(car + 0x1C8);
    z = r_u32(car + 0x1CC);
    w_u32(object + 0x10, x);
    w_u32(object + 0x14, y);
    w_u32(object + 0x18, z);
    value = r_u32(0x80088440);
    w_u32(0x800885A8, 0);
    if (value == 0)
    {
        td5_800314b8(car, frame);
        value = td5_80079c90(object);
        if (value != 0)
        {
            td5_800797fc(car + 0x110, object);
            header = r_u32(0x80088770);
            header = r_u32(header);
            value = r_u8(header + 0x1E);
            if ((value & 1) == 0)
                td5_8002fe8c(object, car, frame, car, slot, texture_v_carrier, texture_u_carrier);
            td5_8006e180(object, frame);
            if (r_u32(0x80087724) != 0 && r_u32(0x800885C8) != 0)
                td5_80041a44(car);
            td5_80040060(slot, frame);
            td5_8004099c(slot, frame);
            td5_80041114(slot);
            td5_80040ea4(car, frame);
            td5_80033610(slot);
            td5_80040e94();
            if (r_u8(car + 0x351) == 0)
                td5_800321d0(slot, frame);
            else
                td5_80032984(car, frame);
        }
    }
    return td5_80045388(car, frame);
}

uint32 td5_8002e568(uint32 slot, uint32 alternate, uint32 original_stack)
{
    uint32 car = 0x800984A0 + slot * 864;
    uint32 frame = original_stack - 0x20;
    uint32 object, x, y, z, value, mode;

    FUNCTION_MARKER(0x8002E568, "SLUS_006.10");
    object = r_u32(car + (alternate == 0 ? 0x178 : 0x17C));
    x = r_u32(car + 0x1C4);
    y = r_u32(car + 0x1C8);
    z = r_u32(car + 0x1CC);
    w_u32(object + 0x10, x);
    w_u32(object + 0x14, y);
    w_u32(object + 0x18, z);
    value = r_u32(0x80088440 + slot * 4);
    w_u32(0x800885A8, slot);
    if (value == 0 || r_u32(0x80087A78) != slot)
    {
        td5_800314b8(car, frame);
        value = td5_80079c90(object);
        if (value != 0)
        {
            td5_800797fc(car + 0x110, object);
            td5_8006e180(object, frame);
            mode = r_u32(0x8008771C);
            if (mode == 3)
            {
                if (slot != r_u32(0x800879B8))
                    td5_80043328(slot);
                if (r_u32(0x8008771C) == mode && r_u32(0x800885C8) != 0
                    && slot == r_u32(0x800879B8))
                    td5_80041a44(car);
            }
            td5_80040060(slot, frame);
            td5_8004099c(slot, frame);
            td5_80041114(slot);
            td5_80033610(slot);
            td5_80040e94();
            if (r_u8(car + 0x351) == 0)
                td5_800321d0(slot, frame);
            else
                td5_80032984(car, frame);
        }
    }
    return td5_80045388(car, frame);
}

uint32 td5_8002f1c4(uint32 allocation)
{
    uint32 value;

    FUNCTION_MARKER(0x8002F1C4, "SLUS_006.10");
    td5_free_memory(allocation);
    value = r_u32(0x800881F8);
    td5_free_memory(value);
    value = r_u32(0x800881FC);
    td5_free_memory(value);
    value = r_u32(0x80088200);
    return td5_free_memory(value);
}

void td5_80030d68(uint32 car, uint32 descriptor, sint32 width)
{
    SVECTOR normal;
    uint32 record, first, last, pool, value, other, origin, product, near_weight, far_weight;
    uint32 color[3], light[3], i;
    sint32 distance, coordinate;
    sint16 dx, dz;

    FUNCTION_MARKER(0x80030D68, "SLUS_006.10");
    value = (uint32)r_s16(descriptor);
    pool = r_u32(0x800884C4);
    record = pool + value * 24;
    last = r_u16(record + 6);
    first = r_u16(record + 4);
    pool = r_u32(0x8008850C);
    last = pool + (last - 1) * 6;
    first = pool + first * 6;
    value = r_u16(first + 4);
    other = r_u16(last + 4);
    normal.vy = 0;
    normal.vx = (sint16)(value - other);
    value = r_u16(last);
    other = r_u16(first);
    normal.vz = (sint16)(value - other);
    normal.pad = 0;
    VectorNormalSS(&normal, &normal);
    coordinate = r_s32(car + 0x1C4);
    value = r_u16(last);
    origin = r_u32(record + 12);
    dx = (sint16)((uint32)(coordinate / 256) - value - origin);
    coordinate = r_s32(car + 0x1CC);
    product = (uint32)(sint32)normal.vx * (uint32)(sint32)dx;
    value = r_u16(last + 4);
    origin = r_u32(record + 20);
    dz = (sint16)((uint32)(coordinate / 256) - value - origin);
    product += (uint32)(sint32)normal.vz * (uint32)(sint32)dz;
    distance = (sint32)(0 - product) / 4096;
    if (distance < 0)
        distance = 0;
    if (width < distance)
        distance = width;
    near_weight = (uint32)distance;
    far_weight = (uint32)width - near_weight;
    for (i = 0; i < 3; ++i)
    {
        value = r_u8(descriptor + 16 + i);
        other = r_u8(descriptor - 16 + i);
        product = value * near_weight + other * far_weight;
        color[i] = (uint32)td5_divide_checked((sint32)product, width);
    }
    td5_set_back_color((sint32)color[0], (sint32)color[1], (sint32)color[2]);
    for (i = 0; i < 3; ++i)
    {
        value = r_u8(descriptor - 20 + i);
        product = value * far_weight;
        light[i] = (uint32)td5_divide_checked((sint32)product, width);
    }
    td5_set_light(0, descriptor - 28, light[0], light[1], light[2]);
    td5_set_light(1, descriptor + 4, 0, 0, 0);
    td5_set_light(2, descriptor + 4, 0, 0, 0);
    td5_apply_light_matrix(car + 0x110);
}

void td5_8003110c(uint32 car, uint32 descriptor, sint32 width)
{
    SVECTOR normal;
    uint32 record, first, last, pool, value, other, origin, product, near_weight, far_weight;
    uint32 color[3], light[3], i;
    sint32 distance, coordinate;
    sint16 dx, dz;

    FUNCTION_MARKER(0x8003110C, "SLUS_006.10");
    value = (uint32)r_s16(descriptor + 2);
    pool = r_u32(0x800884C4);
    record = pool + value * 24 + 24;
    last = r_u16(record + 6);
    first = r_u16(record + 4);
    pool = r_u32(0x8008850C);
    last = pool + (last - 1) * 6;
    first = pool + first * 6;
    value = r_u16(first + 4);
    other = r_u16(last + 4);
    normal.vy = 0;
    normal.vx = (sint16)(value - other);
    value = r_u16(last);
    other = r_u16(first);
    normal.vz = (sint16)(value - other);
    normal.pad = 0;
    VectorNormalSS(&normal, &normal);
    coordinate = r_s32(car + 0x1C4);
    value = r_u16(last);
    origin = r_u32(record + 12);
    dx = (sint16)((uint32)(coordinate / 256) - value - origin);
    coordinate = r_s32(car + 0x1CC);
    product = (uint32)(sint32)normal.vx * (uint32)(sint32)dx;
    value = r_u16(last + 4);
    origin = r_u32(record + 20);
    dz = (sint16)((uint32)(coordinate / 256) - value - origin);
    product += (uint32)(sint32)normal.vz * (uint32)(sint32)dz;
    distance = (sint32)product / 4096;
    if (distance < 0)
        distance = 0;
    if (width < distance)
        distance = width;
    near_weight = (uint32)distance;
    far_weight = (uint32)width - near_weight;
    for (i = 0; i < 3; ++i)
    {
        value = r_u8(descriptor + 16 + i);
        other = r_u8(descriptor + 48 + i);
        product = value * near_weight + other * far_weight;
        color[i] = (uint32)td5_divide_checked((sint32)product, width);
    }
    td5_set_back_color((sint32)color[0], (sint32)color[1], (sint32)color[2]);
    for (i = 0; i < 3; ++i)
    {
        value = r_u8(descriptor + 44 + i);
        product = value * far_weight;
        light[i] = (uint32)td5_divide_checked((sint32)product, width);
    }
    td5_set_light(0, descriptor + 36, light[0], light[1], light[2]);
    td5_set_light(1, descriptor + 4, 0, 0, 0);
    td5_set_light(2, descriptor + 4, 0, 0, 0);
    td5_apply_light_matrix(car + 0x110);
}

uint32 td5_800340fc(sint32 node)
{
    uint32 owner, table, index = 0, value;
    sint32 count;

    FUNCTION_MARKER(0x800340FC, "SLUS_006.10");
    owner = r_u32(0x80088584);
    count = r_s32(owner + 20);
    owner = r_u32(0x80088770);
    table = r_u32(owner + 28);
    while ((sint32)index < count)
    {
        value = r_u16(table);
        if (node >= (sint32)value)
        {
            value = r_u16(table + 2);
            if ((sint32)value >= node)
                return 1;
        }
        ++index;
        table += 6;
    }
    return 0;
}

uint64 td5_80035024(uint32 original_stack)
{
    uint32 frame = original_stack - 0x50;
    uint32 row, record, table, variant_table, count, index, variant, object;
    uint32 x, y, z, value;

    FUNCTION_MARKER(0x80035024, "SLUS_006.10");
    td5_8006e92c();
    td5_8006e1c8(0x800985B0, 0x80098664);
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
    for (row = 0; row < 7; ++row)
    {
        record = 0x8009DBD0 + row * 36;
        table = r_u32(record);
        count = r_u8(table);
        value = (uint32)r_s16(record + 30);
        variant_table = r_u32(0x800884C0) + value * 18;
        variant = 0;
        if (r_u8(record + 32) == 2)
        {
            if ((r_u8(record + 34) & 1) != 0)
            {
                for (index = 0; index < count; ++index)
                {
                    value = r_u32(table + index * 4 + 0x2A0);
                    object = table + value;
                    if (td5_80079e34(object) != 0)
                    {
                        td5_80079b84(object);
                        td5_80071378(object, frame);
                    }
                }
            }
            if (r_u8(table + 1) != 0 && (r_u8(record + 34) & 2) != 0)
            {
                if (r_u8(record + 34) == 2)
                    variant = 0;
                else
                    variant = (uint32)r_u8(variant_table) << 11;
                value = r_u8(table + 1);
                count = r_u8(table);
                count += value;
                index = r_u8(table);
                for (; index < count; ++index)
                {
                    value = r_u32(table + index * 4 + 0x2A0);
                    object = table + value + variant;
                    if (td5_80079e34(object) != 0)
                    {
                        td5_80079b84(object);
                        td5_80071378(object, frame);
                    }
                }
            }
        }
    }
    return 7;
}

uint32 td5_80035da4(sint32 segment)
{
    uint32 row, item, record, field, value, base;
    sint32 selected;

    FUNCTION_MARKER(0x80035DA4, "SLUS_006.10");
    for (row = 0; row < 7; ++row)
    {
        record = 0x8009DBD0 + row * 36;
        selected = r_s16(record + 30);
        if (selected == segment)
        {
            for (item = 0; item < 6; ++item)
            {
                field = record + 4 + item * 4;
                value = r_u32(field);
                if (value != 0)
                {
                    base = r_u32(record);
                    value = r_u32(field);
                    value += base;
                    w_u32(field, 0);
                    return value;
                }
            }
        }
    }
    return 0;
}

uint32 td5_8003608c(sint32 node)
{
    uint32 rounded, value, status;
    sint32 index;

    FUNCTION_MARKER(0x8003608C, "SLUS_006.10");
    rounded = (uint32)node + 10 - (uint32)(node % 20);
    value = r_u32(0x800880D4);
    if (value != 0)
        return value;
    index = (sint32)rounded / 20;
    status = 0x8009DBF1 + (uint32)index * 36;
    if (r_u8(status) == 0)
        td5_80034804((uint32)index);
    while (r_u8(status) != 4)
    {
        VSync(0);
        td5_800341fc();
        DrawSync(0);
    }
    return 4;
}

uint32 td5_80036278(sint32 node)
{
    uint32 value, status;
    sint32 index;

    FUNCTION_MARKER(0x80036278, "SLUS_006.10");
    value = r_u32(0x800880D4);
    if (value != 0)
        return value;
    index = node / 20;
    status = 0x8009DBF0 + (uint32)index * 36;
    if (r_u8(status + 1) == 4 && r_u8(status) == 0)
        td5_80034ae8((uint32)index, (uint32)node, node);
    while (r_u8(status) != 2)
        VSync(0);
    return 2;
}

uint32 td5_80036458(uint32 slot)
{
    uint32 selected = 0, row, record, value;
    sint32 segment, angle, candidate;

    FUNCTION_MARKER(0x80036458, "SLUS_006.10");
    segment = (sint16)(r_s16(0x80098512 + slot * 864) / 20);
    angle = (sint32)td5_8004d19c(0x80097750 + slot * 284);
    for (row = 0; row < 7; ++row)
    {
        record = 0x8009DBD0 + row * 36;
        if (r_u8(record + 32) == 2)
        {
            candidate = r_s16(record + 30);
            if ((angle >= 1024 && angle < 3072 && segment < candidate)
                || ((angle < 1024 || angle >= 3072) && candidate < segment))
            {
                segment = r_s16(record + 30);
                selected = row;
            }
        }
    }
    record = 0x8009DBD0 + selected * 36;
    if (r_u8(record + 32) == 2)
    {
        w_u8(record + 32, 0);
        value = r_u32(record);
        if (value != 0)
        {
            value = r_u32(record);
            td5_free_scene_memory(value);
            w_u32(record, 0);
        }
    }
    if (r_u8(record + 33) == 1)
    {
        w_u8(record + 33, 0);
        value = r_u32(0x80088548);
        return td5_free_scene_memory(value);
    }
    return 1;
}

uint32 td5_80037e04(uint32 car, uint32 parameter)
{
    uint32 model, value, angle, offset, next_index, first, second, pass;
    uint32 y0, y1, y2, buffer, ordering, tag, link, texture, gear, u, v, height;
    uint32 packet, position, digit_offset, base_x, base_y, center_x, center_y, origin_x, origin_y;
    sint32 speed, quotient, digit;

    FUNCTION_MARKER(0x80037E04, "SLUS_006.10");
    model = r_u32(car + 0x184);
    value = r_u32(car + 0x2E8);
    speed = r_s16(model + 0x72);
    angle = (uint32)td5_divide_checked((sint32)(value * 3072), speed);
    offset = r_u32(0x80088104) * 20;
    origin_x = r_u32(0x80088438);
    origin_y = r_u32(0x800884F8);
    first = 0x80088950 + offset;
    second = 0x800889A0 + offset;
    value = r_u32(0x80088510);
    height = r_u32(0x80088518);
    origin_x += value;
    origin_y += height;
    base_x = origin_x - 64;
    base_y = origin_y - 72;
    center_x = origin_x - 40;
    center_y = origin_y - 48;
    for (pass = 0; pass < 2; ++pass)
    {
        value = (uint32)rcos((sint32)angle);
        if (pass == 0)
            value = center_x - (uint32)((sint32)(value * 24) / 4096);
        else
            value = center_x - (uint32)((sint32)value / 512);
        w_u16(first + 8, value);
        w_u16(second + 8, value);
        value = (uint32)rsin((sint32)angle);
        if (pass == 0)
            value = center_y - (uint32)((sint32)(value * 24) / 4096);
        else
            value = center_y - (uint32)((sint32)value / 512);
        w_u16(first + 10, value);
        w_u16(second + 10, value);
        value = (uint32)rsin((sint32)angle);
        value = center_x + (uint32)((sint32)value / 2048);
        w_u16(first + 12, value);
        w_u16(second + 12, value);
        value = (uint32)rcos((sint32)angle);
        value = center_y - (uint32)((sint32)value / 2048);
        w_u16(first + 14, value);
        w_u16(second + 14, value);
        value = (uint32)rsin((sint32)angle);
        value = center_x - (uint32)((sint32)value / 2048);
        w_u16(first + 16, value);
        w_u16(second + 16, value);
        value = (uint32)rcos((sint32)angle);
        value = center_y + (uint32)((sint32)value / 2048);
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
        {
            texture = r_u32(0x8008822C);
            next_index = r_u32(0x80088104);
        }
        w_u32(ordering + 4, (tag & 0xFF000000) | (second & 0xFFFFFF));
        offset = next_index * 20;
        first = 0x80088978 + offset;
        second = 0x800889C8 + offset;
    }
    packet = 0x80088B30 + next_index * 40;
    w_u16(packet + 10, base_y + 29);
    w_u16(packet + 18, base_y + 29);
    w_u16(packet + 8, base_x + 10);
    w_u16(packet + 16, base_x + 19);
    w_u16(packet + 24, base_x + 10);
    height = r_u8(texture + 13);
    w_u16(packet + 32, base_x + 19);
    w_u16(packet + 26, base_y + height + 28);
    height = r_u8(texture + 13);
    w_u16(packet + 34, base_y + height + 28);
    gear = r_u8(car + 0x343);
    u = r_u8(texture + 14);
    w_u8(packet + 12, u + gear * 10);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    v = r_u8(texture + 15);
    w_u8(0x80088B3D + offset, v);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    gear = r_u8(car + 0x343);
    u = r_u8(texture + 14);
    w_u8(0x80088B44 + offset, u + gear * 10 + 9);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    v = r_u8(texture + 15);
    w_u8(0x80088B45 + offset, v);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    gear = r_u8(car + 0x343);
    u = r_u8(texture + 14);
    w_u8(0x80088B4C + offset, u + gear * 10);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    v = r_u8(texture + 15);
    height = r_u8(texture + 13);
    w_u8(0x80088B4D + offset, v + height + 255);
    offset = r_u32(0x80088104) * 40;
    texture = r_u32(0x8008822C);
    gear = r_u8(car + 0x343);
    u = r_u8(texture + 14);
    w_u8(0x80088B54 + offset, u + gear * 10 + 9);
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
            value = base_x + (2 - position) * 10 - 24;
            w_u16(packet + 10, base_y + 36);
            w_u16(packet + 18, base_y + 36);
            w_u16(packet + 8, value);
            w_u16(packet + 16, value + 9);
            w_u16(packet + 24, value);
            height = r_u8(texture + 13);
            w_u16(packet + 32, value + 9);
            w_u16(packet + 26, base_y + height + 35);
            height = r_u8(texture + 13);
            w_u16(packet + 34, base_y + height + 35);
            u = r_u8(texture + 14);
            offset = (uint32)digit * 9;
            w_u8(packet + 12, u + offset);
            texture = r_u32(0x80088228);
            v = r_u8(texture + 15);
            w_u8(packet + 13, v);
            texture = r_u32(0x80088228);
            u = r_u8(texture + 14);
            w_u8(packet + 20, u + offset + 9);
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
            w_u8(packet + 36, u + offset + 9);
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
    texture = r_u32(0x800885C0);
    packet = 0x80088AE0 + offset;
    w_u16(packet + 8, base_x + 6);
    w_u16(packet + 10, base_y + 40);
    value = r_u8(texture + 12);
    w_u16(packet + 18, base_y + 40);
    w_u16(packet + 24, base_x + 6);
    w_u16(packet + 16, base_x + value + 5);
    value = r_u8(texture + 13);
    w_u16(packet + 26, base_y + value + 39);
    value = r_u8(texture + 12);
    w_u16(packet + 32, base_x + value + 5);
    value = r_u8(texture + 13);
    buffer = r_u32(0x800880F0);
    w_u16(packet + 34, base_y + value + 39);
    ordering = r_u32(buffer + 0x70);
    tag = r_u32(packet);
    link = r_u32(ordering);
    w_u32(packet, (tag & 0xFF000000) | (link & 0xFFFFFF));
    ordering = r_u32(buffer + 0x70);
    tag = r_u32(ordering);
    value = (tag & 0xFF000000) | (packet & 0xFFFFFF);
    texture = r_u32(0x80088254);
    w_u32(ordering, value);
    packet = 0x80088810 + offset;
    w_u16(packet + 10, base_y);
    w_u16(packet + 8, base_x);
    value = r_u8(texture + 12);
    w_u16(packet + 18, base_y);
    w_u16(packet + 24, base_x);
    w_u16(packet + 16, base_x + value - 1);
    value = r_u8(texture + 13);
    w_u16(packet + 26, base_y + value - 1);
    value = r_u8(texture + 12);
    w_u16(packet + 32, base_x + value - 1);
    value = r_u8(texture + 13);
    w_u16(packet + 34, base_y + value - 1);
    return td5_minimap_link(buffer, packet);
}

uint32 td5_80038fe0(void)
{
    char text[64];
    uint32 result, player, ticks, divisor, base, format;
    sint32 rate, time, remainder;

    FUNCTION_MARKER(0x80038FE0, "SLUS_006.10");
    result = r_u32(0x8008772C);
    if (result != 0)
        return result;
    result = r_u32(0x8008873C);
    if (result != 0)
        return result;
    for (player = 0; player < 2; ++player)
    {
        base = 0x800987A0 + player * 864;
        ticks = r_u32(base);
        if (ticks != 0)
        {
            rate = r_s32(0x800880FC);
            divisor = (uint32)(rate / 2);
            if (divisor == 0)
                xport_mips_break(7);
            time = (sint32)(ticks / divisor);
            remainder = time % 60000;
            format = player == 0 ? 0x80010B28 : 0x80010B44;
            sprintf(text, (const char *)psx_addr(format, 25), time / 60000,
                remainder / 1000, remainder % 1000);
            td5_80038994(text, 160, player == 0 ? 64 : 80, 0);
            w_u16(base + 22, 0xFC00);
            result = 1;
            w_u32(base - 28, 0);
            w_u8(base + 69, 1);
        }
    }
    return result;
}

uint32 td5_80039208(uint32 car)
{
    char text[64];
    uint32 ticks, level, divisor;
    sint32 rate, time, remainder;

    FUNCTION_MARKER(0x80039208, "SLUS_006.10");
    ticks = r_u16(car + 0x324);
    rate = r_s32(0x800880FC);
    time = td5_divide_checked((sint32)(ticks * 100), rate / 2);
    remainder = time % 6000;
    sprintf(text, (const char *)psx_addr(0x80010B18, 15), time / 6000,
        remainder / 100, remainder % 100);
    td5_80038994((const char *)psx_addr(0x80087904, 5), 46, 12, 0);
    td5_80038994(text, 28, 24, 0);
    level = r_u32(0x80087710);
    ticks = r_u32(0x8007C4D4 + level * 120);
    rate = r_s32(0x800880FC);
    divisor = (uint32)(rate / 2);
    if (divisor == 0)
        xport_mips_break(7);
    time = (sint32)((ticks * 100) / divisor);
    remainder = time % 6000;
    sprintf(text, (const char *)psx_addr(0x80010B18, 15), time / 6000,
        remainder / 100, remainder % 100);
    td5_80038994((const char *)psx_addr(0x80010B60, 10), 408, 12, 0);
    return td5_80038994(text, 412, 24, 0);
}

uint32 td5_80039450(uint32 car, sint32 arrests, uint32 original_stack)
{
    char text[64];
    sint32 points;

    FUNCTION_MARKER(0x80039450, "SLUS_006.10");
    sprintf(text, (const char *)psx_addr(0x80010B6C, 11), arrests);
    td5_80038994(text, 32, 12, 0);
    points = r_s32(car + 0x2A0);
    sprintf(text, (const char *)psx_addr(0x80010B78, 10), points);
    td5_80038994(text, 32, 28, 0);
    return td5_8003a028(original_stack - 0x58);
}

uint32 td5_800394c8(uint32 car)
{
    char text[72];
    uint32 slot, owner, x, y, count, value, address, origin_x, origin_y;
    sint32 number, total, opacity;

    FUNCTION_MARKER(0x800394C8, "SLUS_006.10");
    slot = r_u8(car + 0x34D);
    owner = r_u32(0x800879B8);
    if (slot == owner)
    {
        number = r_s16(0x800883A0);
        sprintf(text, (const char *)psx_addr(0x80010B6C, 11), number);
        x = r_u32(0x80088510);
        y = r_u32(0x80088518);
        td5_80038994(text, x + 16, y + 16, 0);
        number = r_s32(car + 0x2A0);
        sprintf(text, (const char *)psx_addr(0x80010B78, 10), number);
        x = r_u32(0x80088510);
        y = r_u32(0x80088518);
        return td5_80038994(text, x + 16, y + 28, 0);
    }
    number = (sint32)r_u8(car + 0x356) + 1;
    total = r_s32(0x800876F4);
    if (total < number)
        number = total;
    total = r_s32(0x800876F4);
    sprintf(text, (const char *)psx_addr(0x80010B84, 10), number, total);
    x = r_u32(0x80088510);
    y = r_u32(0x80088518);
    td5_80038994(text, x + 16, y + 16, 0);
    count = r_u8(0x800879B4);
    if (count == 0)
        return 0x20000;
    value = count * 0x2D82D;
    slot = r_u8(car + 0x34D);
    w_u16(0x8009DBA0 + slot * 2, 4096 - value / 4096);
    slot = r_u8(car + 0x34D);
    address = 0x8009DBA0 + slot * 2;
    opacity = r_s16(address);
    if (opacity < 0)
        opacity = 0;
    else if (opacity > 4096)
        opacity = 4096;
    count = r_u8(0x800879B4);
    x = r_u32(0x80088438);
    w_u16(address, opacity);
    origin_x = r_u32(0x80088510);
    y = r_u32(0x800884F8);
    x = origin_x + (uint32)((sint32)(x - 56) / 2);
    origin_y = r_u32(0x80088518);
    y = origin_y + (uint32)((sint32)y / 2);
    w_u8(0x800879B4, count - 1);
    return td5_80038994((const char *)psx_addr(0x8008790C, 8), x, y, 0);
}

uint32 td5_800396dc(uint32 car)
{
    char text[64];
    uint32 ticks, x, y, length;
    sint32 adjustment, rate, time, remainder;

    FUNCTION_MARKER(0x800396DC, "SLUS_006.10");
    adjustment = r_s16(car + 0x30E);
    ticks = r_u16(car + 0x324);
    adjustment = adjustment * 1000 / 256;
    rate = r_s32(0x800880FC);
    time = td5_divide_checked((sint32)(ticks * 1000 - (uint32)adjustment), rate / 2);
    remainder = time % 60000;
    sprintf(text, (const char *)psx_addr(0x80010B90, 20), time / 60000,
        remainder / 1000, remainder % 1000);
    if (r_u32(0x8008772C) != 0)
    {
        x = r_u32(0x80088510);
        y = r_u32(0x80088518);
        x += 16;
    }
    else
    {
        length = (uint32)strlen(text);
        y = r_u32(0x80088518);
        x = 256 - length * 4;
    }
    td5_80038994(text, x, y + 20, 0);
    return td5_80038fe0();
}

uint32 td5_8003985c(uint32 car)
{
    char text[64];
    sint32 lap, total;

    FUNCTION_MARKER(0x8003985C, "SLUS_006.10");
    lap = (sint32)r_u8(car + 0x356) + 1;
    total = r_s32(0x800876F4);
    if (total < lap)
        lap = total;
    total = r_s32(0x800876F4);
    sprintf(text, (const char *)psx_addr(0x80010B84, 10), lap, total);
    td5_80038994(text, 32, 12, 0);
    return td5_80038d5c(car);
}

uint32 td5_800398e8(uint32 car)
{
    char text[64];
    uint32 x, y, index, label;
    sint32 lap, total;

    FUNCTION_MARKER(0x800398E8, "SLUS_006.10");
    lap = (sint32)r_u8(car + 0x356) + 1;
    total = r_s32(0x800876F4);
    if (total < lap)
        lap = total;
    total = r_s32(0x800876F4);
    sprintf(text, (const char *)psx_addr(0x80010B84, 10), lap, total);
    x = r_u32(0x80088510);
    y = r_u32(0x80088518);
    td5_80038994(text, x + 16, y + 16, 0);
    index = r_u8(car + 0x35B);
    x = r_u32(0x80088510);
    y = r_u32(0x80088518);
    label = r_u32(0x8007E0F0 + index * 4);
    return td5_80038994(td5_hud_string(label), x + 16, y + 28, 0);
}

uint32 td5_800399b0(uint32 car)
{
    char text[64];
    uint32 index, label;
    sint32 points;

    FUNCTION_MARKER(0x800399B0, "SLUS_006.10");
    points = r_s32(car + 0x2A0);
    sprintf(text, (const char *)psx_addr(0x80010B78, 10), points);
    td5_80038994(text, 32, 12, 0);
    index = r_u8(car + 0x35B);
    label = r_u32(0x8007E0F0 + index * 4);
    return td5_80038994(td5_hud_string(label), 32, 32, 0);
}

uint32 td5_8003a028(uint32 original_stack)
{
    uint32 buffer = original_stack - 0x70 + 0x10;
    uint32 counter, index, source, count, first_length, second_length;
    char *destination;
    const char *input;

    FUNCTION_MARKER(0x8003A028, "SLUS_006.10");
    counter = r_u32(0x80087930);
    if ((sint32)counter >= 300)
        return 0;
    ++counter;
    w_u32(0x80087930, counter);
    if ((sint32)counter >= 270 && (counter & 1) != 0)
        return 1;
    index = r_u32(0x8008792C);
    source = r_u32(0x8007E140 + index * 8);
    first_length = (uint32)strlen(td5_hud_string(source));
    index = r_u32(0x8008792C);
    count = r_u32(0x80087934);
    source = r_u32(0x8007E140 + index * 8);
    destination = (char *)psx_addr(buffer, count);
    input = (const char *)psx_addr(source, count);
    memcpy(destination, input, count);
    count = r_u32(0x80087934);
    w_u8(buffer + count, 0);
    count = r_u32(0x80087934);
    if ((sint32)count < (sint32)first_length)
        w_u32(0x80087934, count + 1);
    count = r_u32(0x80087934);
    td5_80038994(td5_hud_string(buffer), 480 - count * 9, 12, 0);
    index = r_u32(0x8008792C);
    source = r_u32(0x8007E144 + index * 8);
    second_length = (uint32)strlen(td5_hud_string(source));
    index = r_u32(0x8008792C);
    count = r_u32(0x80087938);
    source = r_u32(0x8007E144 + index * 8);
    destination = (char *)psx_addr(buffer, count);
    input = (const char *)psx_addr(source, count);
    memcpy(destination, input, count);
    count = r_u32(0x80087938);
    w_u8(buffer + count, 0);
    count = r_u32(0x80087934);
    if (count == first_length)
    {
        count = r_u32(0x80087938);
        if ((sint32)count < (sint32)second_length)
            w_u32(0x80087938, count + 1);
    }
    count = r_u32(0x80087938);
    return td5_80038994(td5_hud_string(buffer), 480 - count * 9, 24, 0);
}

uint32 td5_8003a384(void)
{
    uint32 texture;

    FUNCTION_MARKER(0x8003A384, "SLUS_006.10");
    texture = td5_find_texture((const char *)psx_addr(0x80010C8C, 9));
    w_u32(0x80088798, texture);
    texture = td5_find_texture((const char *)psx_addr(0x80010C98, 9));
    w_u32(0x800884C8, texture);
    texture = td5_find_texture((const char *)psx_addr(0x80010CA4, 9));
    w_u32(0x80088764, texture);
    w_u32(0x80088784, 120);
    w_u32(0x800886FC, 0);
    return 120;
}

static void td5_start_light_uv_tail(uint32 packet, uint32 texture_global, uint32 horizontal_trim)
{
    uint32 texture, u, v;

    texture = r_u32(texture_global);
    w_u8(packet + 0xD, r_u8(texture + 0xF));
    texture = r_u32(texture_global);
    u = r_u8(texture + 0xE);
    v = r_u8(texture + 0xC);
    w_u8(packet + 0x14, v + u - horizontal_trim);
    texture = r_u32(texture_global);
    w_u8(packet + 0x15, r_u8(texture + 0xF));
    texture = r_u32(texture_global);
    w_u8(packet + 0x1C, r_u8(texture + 0xE));
    texture = r_u32(texture_global);
    u = r_u8(texture + 0xF);
    v = r_u8(texture + 0xD);
    w_u8(packet + 0x1D, v + u - 1);
    texture = r_u32(texture_global);
    u = r_u8(texture + 0xE);
    v = r_u8(texture + 0xC);
    w_u8(packet + 0x24, v + u - horizontal_trim);
    texture = r_u32(texture_global);
    u = r_u8(texture + 0xF);
    v = r_u8(texture + 0xD);
    w_u8(packet + 0x25, v + u - 1);
}

uint32 td5_8003a3e4(void)
{
    static const sint32 full_y[4] = {-4, 34, 72, 110};
    static const sint32 split_y[4] = {-2, 17, 36, 55};
    uint32 slot, packet, texture, x, y, width, height, value;
    uint32 i, mode, left, right_bias, color, state, buffer;

    FUNCTION_MARKER(0x8003A3E4, "SLUS_006.10");
    if (r_u32(0x800886FC) == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    slot = r_u32(0x80088104);
    x = r_u32(0x80088510);
    texture = r_u32(0x80088798);
    w_u32(0x800885B8, x + 8);
    height = r_u8(texture + 0xD);
    value = r_u32(0x800884F8);
    packet = 0x800A3E08 + slot * 280;
    y = r_u32(0x80088518);
    w_u32(0x800885BC, y + (uint32)((sint32)(value - height) / 2));
    w_u8(packet + 3, 9);
    w_u8(packet + 7, 0x2D);
    texture = r_u32(0x80088798);
    x = r_u32(0x800885B8);
    y = r_u32(0x800885BC);
    w_u16(packet + 0x16, r_u16(texture + 8));
    value = r_u16(texture + 0xA);
    w_u16(packet + 8, x);
    w_u16(packet + 0xA, y);
    w_u16(packet + 0xE, value);
    width = r_u8(texture + 0xC);
    w_u16(packet + 0x12, y);
    w_u16(packet + 0x18, x);
    w_u16(packet + 0x10, x + width - 1);
    w_u16(packet + 0x1A, y + r_u8(texture + 0xD) - 1);
    w_u16(packet + 0x20, x + r_u8(texture + 0xC) - 1);
    w_u16(packet + 0x22, y + r_u8(texture + 0xD) - 1);
    w_u8(packet + 0xC, r_u8(texture + 0xE));
    td5_start_light_uv_tail(packet, 0x80088798, 1);
    packet += 40;
    for (i = 0; i < 5; ++i)
    {
        w_u8(packet + 3, 9);
        w_u8(packet + 7, 0x2C);
        texture = r_u32(0x800884C8);
        w_u16(packet + 0x16, r_u16(texture + 8));
        value = r_u16(texture + 0xA);
        color = 0x8007E218 + i * 4;
        w_u16(packet + 0xE, value);
        w_u8(packet + 4, r_u8(color));
        w_u8(packet + 5, r_u8(color + 1));
        w_u8(packet + 6, r_u8(color + 2));
        mode = r_u32(0x8008772C);
        x = r_u32(0x800885B8);
        y = r_u32(0x800885BC);
        texture = r_u32(0x800884C8);
        left = x + (mode == 0 ? 22 : 11);
        y += i * (mode == 0 ? 38 : 19);
        right_bias = mode == 0 ? 20 : 9;
        w_u16(packet + 8, left);
        w_u16(packet + 0xA, y + 3);
        width = r_u8(texture + 0xC);
        w_u16(packet + 0x12, y + 3);
        w_u16(packet + 0x18, left);
        w_u16(packet + 0x10, x + width + right_bias);
        w_u16(packet + 0x1A, y + r_u8(texture + 0xD) + 2);
        w_u16(packet + 0x20, x + r_u8(texture + 0xC) + right_bias);
        w_u16(packet + 0x22, y + r_u8(texture + 0xD) + 2);
        texture = r_u32(0x800884C8);
        w_u8(packet + 0xC, r_u8(texture + 0xE));
        td5_start_light_uv_tail(packet, 0x800884C8, 2);
        packet += 40;
    }
    w_u8(packet + 3, 9);
    w_u8(packet + 7, 0x2E);
    texture = r_u32(0x80088764);
    w_u16(packet + 0x16, r_u16(texture + 8));
    w_u16(packet + 0xE, r_u16(texture + 0xA));
    w_u8(packet + 4, r_u8(0x8007E218));
    w_u8(packet + 5, r_u8(0x8007E219));
    w_u8(packet + 6, r_u8(0x8007E21A));
    texture = r_u32(0x80088764);
    w_u16(packet + 8, 640);
    w_u16(packet + 0xA, 0);
    width = r_u8(texture + 0xC);
    w_u16(packet + 0x12, 0);
    w_u16(packet + 0x18, 640);
    w_u16(packet + 0x10, width + 639);
    w_u16(packet + 0x1A, r_u8(texture + 0xD) - 1);
    w_u16(packet + 0x20, r_u8(texture + 0xC) + 639);
    w_u16(packet + 0x22, r_u8(texture + 0xD) - 1);
    w_u8(packet + 0xC, r_u8(texture + 0xE));
    td5_start_light_uv_tail(packet, 0x80088764, 1);
    if (r_u32(0x80088574) == 0)
    {
        for (i = 0; i < 4; ++i)
            if (r_u8(0x800984A0 + i * 14 + 12) >= 2)
                w_u32(0x80088570, 1);
    }
    if (r_u32(0x80088570) == 0)
    {
        for (i = 0; i < 4; ++i)
            if (r_u8(0x80098800 + i * 14 + 12) < 2)
                w_u32(0x80088574, 1);
    }
    if (r_u32(0x800885F8) == 0)
    {
        value = r_u32(0x80088784) - 1;
        w_u32(0x80088784, value);
        if (value == 90)
            w_u32(0x800886FC, 1);
        if (value == 75)
            w_u32(0x800886FC, 2);
        if (value == 60)
            w_u32(0x800886FC, 3);
        if (value == 30)
            w_u32(0x800886FC, 4);
        if (value == 0)
            w_u32(0x800886FC, 0xFFFFFFFF);
    }
    slot = r_u32(0x80088104);
    state = r_u32(0x800886FC);
    packet = 0x800A3EF8 + slot * 280;
    if (state - 1 < 4)
    {
        mode = r_u32(0x8008772C);
        color = 0x8007E218 + (state - 1) * 4;
        x = r_u32(0x800885B8);
        y = r_u32(0x800885BC);
        texture = r_u32(0x80088764);
        left = x + (mode == 0 ? 10 : 5);
        right_bias = mode == 0 ? 9 : 4;
        value = (uint32)(mode == 0 ? full_y[state - 1] : split_y[state - 1]);
        w_u16(packet + 8, left);
        w_u16(packet + 0xA, y + value);
        width = r_u8(texture + 0xC);
        w_u16(packet + 0x12, y + value);
        w_u16(packet + 0x18, left);
        w_u16(packet + 0x10, x + width + right_bias);
        w_u16(packet + 0x1A, y + r_u8(texture + 0xD) + value - 1);
        w_u16(packet + 0x20, x + r_u8(texture + 0xC) + right_bias);
        w_u16(packet + 0x22, y + r_u8(texture + 0xD) + value - 1);
        w_u8(packet + 4, r_u8(color));
        w_u8(packet + 5, r_u8(color + 1));
        w_u8(packet + 6, r_u8(color + 2));
        if (state == 4)
        {
            mode = r_u32(0x8008772C);
            w_u32(0x80088360, 0xFFFFFFFF);
            w_u32(0x80088618, 0);
            w_u32(0x8008873C, 0);
            if (mode == 0)
            {
                texture = r_u32(0x80098984);
                value = (uint32)(r_s16(texture + 0x72) * 2 / 3);
                w_u32(0x80098AE8, value);
            }
        }
    }
    buffer = r_u32(0x800880F0);
    slot = r_u32(0x80088104);
    packet = 0x800A3EF8 + slot * 280;
    for (i = 0; i < 7; ++i)
    {
        td5_minimap_link(buffer, packet);
        packet -= 40;
    }
    return 0;
}

uint32 td5_8003b254(uint32 incoming_v0)
{
    FUNCTION_MARKER(0x8003B254, "SLUS_006.10");
    return incoming_v0;
}

uint32 td5_8003bba8(uint32 incoming_v0)
{
    FUNCTION_MARKER(0x8003BBA8, "SLUS_006.10");
    return incoming_v0;
}

uint32 td5_8003bcf8(void)
{
    uint32 index;

    FUNCTION_MARKER(0x8003BCF8, "SLUS_006.10");
    for (index = 0; index < 50; ++index)
        if (r_u8(0x8009EFCF + index * 68) == 0)
            return index;
    return 0xFFFFFFFF;
}

uint32 td5_8003bd34(uint32 index)
{
    uint32 slot, record, packet, x, y, z, vertex, result, depth;
    uint32 value, other, shift, buffer, ordering;

    FUNCTION_MARKER(0x8003BD34, "SLUS_006.10");
    slot = r_u32(0x80088104);
    record = index * 68;
    value = r_u8(0x8009EFB1 + record);
    packet = 0x8008C518 + slot * 800 + value * 40;
    record += 0x8009EFDC;
    x = r_u32(record);
    y = r_u32(record + 4);
    z = r_u32(record + 8);
    xport_gte_write_control(5, x);
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    for (vertex = 0; vertex < 3; ++vertex)
    {
        xport_gte_write_data(vertex * 2, r_u32(0x8007E27C + vertex * 8));
        xport_gte_write_data(vertex * 2 + 1, r_u32(0x8007E280 + vertex * 8));
    }
    xport_gte_execute(0x280030);
    w_u32(packet + 8, xport_gte_read_data(12));
    w_u32(packet + 16, xport_gte_read_data(13));
    w_u32(packet + 24, xport_gte_read_data(14));
    result = xport_gte_read_control(31) & 0x1C66000;
    if (result != 0)
        return result;
    xport_gte_write_data(0, r_u32(0x8007E294));
    xport_gte_write_data(1, r_u32(0x8007E298));
    xport_gte_execute(0x180001);
    w_u32(packet + 32, xport_gte_read_data(14));
    xport_gte_execute(0x168002E);
    depth = xport_gte_read_data(7);
    value = r_u32(0x80088110);
    other = r_u32(0x80088164);
    shift = value - other;
    depth = (uint32)((sint32)depth >> (shift & 31));
    if ((sint32)depth <= 0)
        return shift;
    result = (sint32)depth < r_s32(0x8008857C);
    if (result == 0)
        return 0;
    buffer = r_u32(0x800880F0);
    ordering = r_u32(buffer + 0x70);
    other = r_u32(packet);
    value = r_u32(ordering + depth * 4);
    w_u32(packet, (other & 0xFF000000) | (value & 0xFFFFFF));
    ordering = r_u32(buffer + 0x70) + depth * 4;
    value = r_u32(ordering);
    result = (value & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(ordering, result);
    return result;
}

uint32 td5_8003bef0(uint32 index, uint32 original_stack)
{
    uint32 matrix = original_stack - 0x48 + 0x10;
    uint32 words[3];
    uint32 offset, count, i, record, object, result;
    sint32 angle;

    FUNCTION_MARKER(0x8003BEF0, "SLUS_006.10");
    for (offset = 0; offset < 32; offset += count * 4)
    {
        count = offset < 24 ? 3 : 2;
        for (i = 0; i < count; ++i)
            words[i] = r_u32(0x80010CC8 + offset + i * 4);
        for (i = 0; i < count; ++i)
            w_u32(matrix + offset + i * 4, words[i]);
    }
    offset = index * 68;
    record = 0x8009EFB0 + offset;
    angle = r_s16(record);
    object = r_u32(record + 8);
    RotMatrixX(angle, (MATRIX *)psx_addr(matrix, 32));
    angle = r_s16(record + 4);
    RotMatrixZ(angle, (MATRIX *)psx_addr(matrix, 32));
    angle = r_s16(record + 2);
    RotMatrixY(angle, (MATRIX *)psx_addr(matrix, 32));
    w_u32(object + 0x10, r_u32(0x8009EFD0 + offset));
    w_u32(object + 0x14, r_u32(0x8009EFD4 + offset));
    w_u32(object + 0x18, r_u32(0x8009EFD8 + offset));
    td5_800797fc(matrix, object);
    result = td5_8006e180(object, original_stack - 0x48);
    SetRotMatrix((MATRIX *)psx_addr(0x8009FCF8, 20));
    return result;
}

uint32 td5_8003c450(void)
{
    uint32 slot, primitive, offset, buffer, destination, chunk, count, i;
    uint32 words[4];

    FUNCTION_MARKER(0x8003C450, "SLUS_006.10");
    for (slot = 0; slot < 50; ++slot)
        if (r_u8(0x8009EFCF + slot * 68) == 0)
            break;
    if (slot == 50)
        return 1;
    for (primitive = 0; primitive < 20; ++primitive)
        if (r_s8(0x8009EC00 + primitive) == 0)
            break;
    if (primitive == 20)
        return 1;
    w_u8(0x8009EC00 + primitive, 1);
    offset = slot * 68;
    w_u8(0x8009EFB1 + offset, primitive);
    for (buffer = 0; buffer < 2; ++buffer)
    {
        destination = 0x8008C518 + buffer * 800 + primitive * 40;
        for (chunk = 0; chunk < 40; chunk += count * 4)
        {
            count = chunk < 32 ? 4 : 2;
            for (i = 0; i < count; ++i)
                words[i] = r_u32(0x8009D788 + chunk + i * 4);
            for (i = 0; i < count; ++i)
                w_u32(destination + chunk + i * 4, words[i]);
        }
    }
    w_u8(0x8009EFCF + offset, 0x80);
    w_u32(0x8009EFF0 + offset, 0x8003BBA8);
    w_u32(0x8009EFEC + offset, 0x8003BD34);
    w_u32(0x8009EFD0 + offset, r_u32(0x80098664));
    w_u32(0x8009EFD4 + offset, r_u32(0x80098668) + 0x12C00);
    w_u32(0x8009EFD8 + offset, r_u32(0x8009866C));
    return offset;
}

uint32 td5_8003dac4(uint32 position, uint32 type)
{
    uint32 slot, primitive, record, buffer, destination, chunk, count, i;
    uint32 words[4], lifetime, first, second;
    sint32 random;

    FUNCTION_MARKER(0x8003DAC4, "SLUS_006.10");
    for (slot = 0; slot < 50; ++slot)
        if (r_u8(0x8009EFCF + slot * 68) == 0)
            break;
    if (slot == 50)
        return 1;
    for (primitive = 0; primitive < 20; ++primitive)
        if (r_s8(0x8009EC00 + primitive) == 0)
            break;
    if (primitive == 20)
        return 1;
    w_u8(0x8009EC00 + primitive, 1);
    record = 0x8009EFB0 + slot * 68;
    w_u8(record + 1, primitive);
    for (buffer = 0; buffer < 2; ++buffer)
    {
        destination = 0x8008C518 + buffer * 800 + primitive * 40;
        for (chunk = 0; chunk < 40; chunk += count * 4)
        {
            count = chunk < 32 ? 4 : 2;
            for (i = 0; i < count; ++i)
                words[i] = r_u32(0x8009EC48 + chunk + i * 4);
            for (i = 0; i < count; ++i)
                w_u32(destination + chunk + i * 4, words[i]);
        }
    }
    w_u8(record + 31, 0x80);
    w_u32(record + 64, 0x8003CD14);
    w_u32(record + 60, 0x8003C99C);
    w_u32(record + 32, r_u32(position));
    w_u32(record + 36, r_u32(position + 4));
    w_u32(record + 40, r_u32(position + 8));
    random = randPSX();
    w_u32(record + 12, (uint32)(random % 512) - 256);
    w_u32(record + 16, 1536);
    random = randPSX();
    w_u32(record + 20, (uint32)(random % 512) - 256);
    random = randPSX();
    w_u8(record + 3, type);
    w_u8(record, (uint32)(random % 31));
    random = randPSX();
    lifetime = ((uint32)(random % 4) + 1) * 10;
    first = (uint32)td5_divide_checked(12288, (sint32)(lifetime & 0xFE));
    w_u8(record + 2, lifetime);
    lifetime = r_u8(record + 2);
    second = (uint32)td5_divide_checked(6400, (sint32)lifetime);
    w_u16(record + 4, 0x4000);
    w_u16(record + 8, 0x2080);
    w_u16(record + 6, 0 - first);
    w_u16(record + 10, second);
    return 0x2080;
}

uint32 td5_8003de68(uint32 position)
{
    uint32 slot, primitive, record, buffer, destination, chunk, count, i;
    uint32 words[4], lifetime, first, second;
    sint32 random;

    FUNCTION_MARKER(0x8003DE68, "SLUS_006.10");
    for (slot = 0; slot < 50; ++slot)
        if (r_u8(0x8009EFCF + slot * 68) == 0)
            break;
    if (slot == 50)
        return 1;
    for (primitive = 0; primitive < 20; ++primitive)
        if (r_s8(0x8009EC00 + primitive) == 0)
            break;
    if (primitive == 20)
        return 1;
    w_u8(0x8009EC00 + primitive, 1);
    record = 0x8009EFB0 + slot * 68;
    w_u8(record + 1, primitive);
    for (buffer = 0; buffer < 2; ++buffer)
    {
        destination = 0x8008C518 + buffer * 800 + primitive * 40;
        for (chunk = 0; chunk < 40; chunk += count * 4)
        {
            count = chunk < 32 ? 4 : 2;
            for (i = 0; i < count; ++i)
                words[i] = r_u32(0x8009EC48 + chunk + i * 4);
            for (i = 0; i < count; ++i)
                w_u32(destination + chunk + i * 4, words[i]);
        }
    }
    w_u8(record + 31, 0x80);
    w_u32(record + 64, 0x8003CD14);
    w_u32(record + 60, 0x8003C99C);
    w_u32(record + 32, r_u32(position));
    w_u32(record + 36, r_u32(position + 4));
    w_u32(record + 40, r_u32(position + 8));
    w_u32(record + 12, 0);
    w_u32(record + 16, 0);
    w_u32(record + 20, 0);
    random = randPSX();
    w_u8(record + 3, 0);
    w_u8(record, (uint32)(random % 31));
    random = randPSX();
    lifetime = ((uint32)(random % 4) + 1) * 500;
    first = (uint32)td5_divide_checked(12288, (sint32)(lifetime & 0xFC));
    w_u8(record + 2, lifetime);
    lifetime = r_u8(record + 2);
    second = (uint32)td5_divide_checked(6400, (sint32)lifetime);
    w_u16(record + 4, 0x4000);
    w_u16(record + 8, 0x2080);
    w_u16(record + 6, 0 - first);
    w_u16(record + 10, second);
    return 0x2080;
}

uint32 td5_8003e1b8(uint32 index)
{
    uint32 slot, record, packet, value, x, y, z, vertex, result, depth;
    uint32 other, shift, buffer, ordering;

    FUNCTION_MARKER(0x8003E1B8, "SLUS_006.10");
    slot = r_u32(0x80088104);
    record = 0x8009EFB0 + index * 68;
    value = r_u8(record);
    packet = 0x800A2050 + slot * 400 + value * 20;
    x = r_u32(record + 44);
    y = r_u32(record + 48);
    z = r_u32(record + 52);
    xport_gte_write_control(5, x);
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    value = r_u8(record + 5);
    SetRotMatrix((MATRIX *)psx_addr(0x8008CF30 + value * 32, 20));
    for (vertex = 0; vertex < 3; ++vertex)
    {
        xport_gte_write_data(vertex * 2, r_u32(0x8007E2BC + vertex * 8));
        xport_gte_write_data(vertex * 2 + 1, r_u32(0x8007E2C0 + vertex * 8));
    }
    xport_gte_execute(0x280030);
    w_u32(packet + 8, xport_gte_read_data(12));
    w_u32(packet + 12, xport_gte_read_data(13));
    w_u32(packet + 16, xport_gte_read_data(14));
    result = xport_gte_read_control(31) & 0x1C66000;
    if (result == 0)
    {
        xport_gte_execute(0x158002D);
        depth = xport_gte_read_data(7);
        value = r_u32(0x80088110);
        other = r_u32(0x80088164);
        shift = value - other;
        depth = (uint32)((sint32)depth >> (shift & 31));
        result = shift;
        if ((sint32)depth > 0)
        {
            result = (sint32)depth < r_s32(0x8008857C);
            if (result != 0)
            {
                buffer = r_u32(0x800880F0);
                ordering = r_u32(buffer + 0x70);
                other = r_u32(packet);
                value = r_u32(ordering + depth * 4);
                w_u32(packet, (other & 0xFF000000) | (value & 0xFFFFFF));
                ordering = r_u32(buffer + 0x70) + depth * 4;
                value = r_u32(ordering);
                result = (value & 0xFF000000) | (packet & 0xFFFFFF);
                w_u32(ordering, result);
            }
        }
    }
    SetRotMatrix((MATRIX *)psx_addr(0x8009FCF8, 20));
    return result;
}

uint32 td5_8003e38c(uint32 index)
{
    uint32 record = 0x8009EFB0 + index * 68;
    uint32 velocity, x_velocity, value, life, result;

    FUNCTION_MARKER(0x8003E38C, "SLUS_006.10");
    velocity = r_u32(record + 16);
    x_velocity = r_u32(record + 12);
    w_u32(record + 16, velocity - 800);
    value = r_u32(record + 32);
    w_u32(record + 32, value + x_velocity);
    value = r_u32(record + 36);
    velocity = r_u32(record + 16);
    w_u32(record + 36, value + velocity);
    value = r_u32(record + 40);
    velocity = r_u32(record + 20);
    w_u32(record + 40, value + velocity);
    life = r_u8(record + 1);
    result = life + 255;
    w_u8(record + 1, result);
    if (life == 0)
    {
        value = r_u8(record);
        result = 0xFFFFFFFD;
        w_u8(0x8009D970 + value, result);
        w_u8(record + 31, 0);
    }
    return result;
}

uint32 td5_8003e458(uint32 car, uint32 point_index, uint32 red, uint32 green, uint32 blue)
{
    uint32 slot, primitive, record, buffer, destination, point, value, result, i;
    uint32 words[4];
    sint32 threshold, random;

    FUNCTION_MARKER(0x8003E458, "SLUS_006.10");
    threshold = r_s32(car + 0x2EC) / 200;
    random = randPSX();
    if (random % 1000 >= threshold)
        return 0;
    for (slot = 0; slot < 50; ++slot)
        if (r_u8(0x8009EFCF + slot * 68) == 0)
            break;
    if (slot == 50)
        return 1;
    for (primitive = 0; primitive < 20; ++primitive)
        if (r_s8(0x8009D970 + primitive) == 0)
            break;
    if (primitive == 20)
        return 1;
    w_u8(0x8009D970 + primitive, 1);
    record = 0x8009EFB0 + slot * 68;
    w_u8(record, primitive);
    for (buffer = 0; buffer < 2; ++buffer)
    {
        destination = 0x800A2050 + buffer * 400 + primitive * 20;
        for (i = 0; i < 4; ++i)
            words[i] = r_u32(0x8008CB58 + i * 4);
        for (i = 0; i < 4; ++i)
            w_u32(destination + i * 4, words[i]);
        w_u32(destination + 16, r_u32(0x8008CB68));
    }
    for (buffer = 0; buffer < 2; ++buffer)
    {
        destination = 0x800A2050 + buffer * 400 + primitive * 20;
        w_u8(destination + 4, red);
        w_u8(destination + 5, green);
        w_u8(destination + 6, blue);
    }
    w_u8(record + 31, 0x83);
    w_u32(record + 64, 0x8003E38C);
    w_u32(record + 60, 0x8003E1B8);
    random = randPSX();
    point = car + point_index * 12;
    value = r_u32(point + 0x80);
    w_u32(record + 32, value + (uint32)(random % 4096) - 2048);
    w_u32(record + 36, r_u32(point + 0x84));
    random = randPSX();
    value = r_u32(point + 0x88);
    w_u32(record + 40, value + (uint32)(random % 4096) - 2048);
    random = randPSX();
    value = r_u32(car + 0x194);
    w_u32(record + 12, value + 4096 - (uint32)(random % 8192));
    random = randPSX();
    w_u32(record + 16, (uint32)(random % 4096) + 4096);
    random = randPSX();
    value = r_u32(car + 0x19C);
    w_u32(record + 20, value + 4096 - (uint32)(random % 8192));
    random = randPSX();
    w_u8(record + 5, (uint32)random & 1);
    random = randPSX();
    result = (uint32)(random % 5) + 10;
    w_u8(record + 1, result);
    return result;
}

uint32 td5_8003e904(uint32 car, uint32 point_index, uint32 type)
{
    uint32 slot, primitive, record, buffer, destination, chunk, count, i, point, value;
    uint32 words[4], lifetime, first, second;
    sint32 threshold, random;

    FUNCTION_MARKER(0x8003E904, "SLUS_006.10");
    threshold = r_s32(car + 0x2EC) / 200;
    random = randPSX();
    if (random % 1000 >= threshold)
        return 0;
    for (slot = 0; slot < 50; ++slot)
        if (r_u8(0x8009EFCF + slot * 68) == 0)
            break;
    if (slot == 50)
        return 1;
    for (primitive = 0; primitive < 20; ++primitive)
        if (r_s8(0x8009EC00 + primitive) == 0)
            break;
    if (primitive == 20)
        return 1;
    w_u8(0x8009EC00 + primitive, 1);
    record = 0x8009EFB0 + slot * 68;
    w_u8(record + 1, primitive);
    for (buffer = 0; buffer < 2; ++buffer)
    {
        destination = 0x8008C518 + buffer * 800 + primitive * 40;
        for (chunk = 0; chunk < 40; chunk += count * 4)
        {
            count = chunk < 32 ? 4 : 2;
            for (i = 0; i < count; ++i)
                words[i] = r_u32(0x8009D948 + chunk + i * 4);
            for (i = 0; i < count; ++i)
                w_u32(destination + chunk + i * 4, words[i]);
        }
    }
    w_u8(record + 31, 0x80);
    w_u32(record + 64, 0x8003CD14);
    w_u32(record + 60, 0x8003C99C);
    point = car + point_index * 12;
    w_u32(record + 32, r_u32(point + 0x80));
    w_u32(record + 36, r_u32(point + 0x84));
    w_u32(record + 40, r_u32(point + 0x88));
    value = r_u32(car + 0x194);
    w_u32(record + 16, 1536);
    w_u32(record + 12, value);
    w_u32(record + 20, r_u32(car + 0x19C));
    random = randPSX();
    w_u8(record + 3, type);
    w_u8(record, (uint32)(random % 31));
    random = randPSX();
    lifetime = ((uint32)(random % 4) + 1) * 10;
    first = (uint32)td5_divide_checked(12288, (sint32)(lifetime & 0xFE));
    w_u8(record + 2, lifetime);
    lifetime = r_u8(record + 2);
    second = (uint32)td5_divide_checked(6400, (sint32)lifetime);
    w_u16(record + 4, 0x4000);
    w_u16(record + 8, 0x26C0);
    w_u16(record + 6, 0 - first);
    w_u16(record + 10, second);
    return 0x26C0;
}
