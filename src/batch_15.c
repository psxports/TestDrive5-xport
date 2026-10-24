#include "td5_functions.h"
#include <stdio.h>

uint32 td5_8003ece8(uint32 index)
{
    uint32 record = 0x8009EFB0 + index * 68;
    uint32 size, step, life, result, primitive;

    FUNCTION_MARKER(0x8003ECE8, "SLUS_006.10");
    size = r_u16(record + 4);
    step = r_u16(record + 6);
    life = r_u8(record + 2);
    w_u16(record + 4, size + step);
    result = life + 255;
    w_u8(record + 2, result);
    if (life == 0)
    {
        primitive = r_u8(record + 1);
        result = 0xFFFFFFFD;
        w_u8(0x8009EC00 + primitive, result);
        w_u8(record + 31, 0);
    }
    return result;
}

uint32 td5_8003ed48(uint32 index)
{
    uint32 record = 0x8009EFB0 + index * 68;
    uint32 brightness, slot, primitive, packet, denominator, width, height;
    uint32 value, tag, shift, other, buffer, ordering, offset, result;

    FUNCTION_MARKER(0x8003ED48, "SLUS_006.10");
    brightness = r_u16(record + 4);
    slot = r_u32(0x80088104);
    primitive = r_u8(record + 1);
    packet = 0x8008C518 + slot * 800 + primitive * 40;
    w_u8(packet + 4, brightness >> 8);
    w_u8(packet + 5, r_u16(record + 4) >> 8);
    w_u8(packet + 6, r_u16(record + 4) >> 8);
    denominator = r_u16(record + 8);
    width = (uint32)td5_divide_checked(0x18000, (sint32)denominator);
    height = (uint32)td5_divide_checked(0x10000, (sint32)denominator);
    w_u16(packet + 8, r_u32(record + 32));
    w_u16(packet + 10, r_u32(record + 36));
    w_u16(packet + 16, r_u32(record + 32) + width);
    w_u16(packet + 18, r_u32(record + 36));
    w_u16(packet + 24, r_u32(record + 32));
    value = r_u32(record + 36);
    tag = r_u32(packet);
    w_u16(packet + 26, value + height);
    value = r_u32(record + 32);
    shift = r_u32(0x80088110);
    w_u16(packet + 32, value + width);
    value = r_u32(record + 36);
    other = r_u32(0x80088164);
    buffer = r_u32(0x800880F0);
    shift -= other;
    w_u16(packet + 34, value + height);
    ordering = r_u32(buffer + 0x70);
    offset = (1 >> (shift & 31)) * 4;
    value = r_u32(ordering + offset);
    w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
    ordering = r_u32(buffer + 0x70) + offset;
    value = r_u32(ordering);
    result = (value & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(ordering, result);
    return result;
}

uint32 td5_8003ef50(sint32 velocity)
{
    uint32 magnitude = (uint32)(velocity >> 13);
    uint32 slot, primitive, record, buffer, destination, chunk, count, i;
    uint32 words[4], flag, life, size, quotient, result;
    sint32 random, denominator;

    FUNCTION_MARKER(0x8003EF50, "SLUS_006.10");
    if ((sint32)magnitude < 0)
        magnitude = 0 - magnitude;
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
                words[i] = r_u32(0x8009D818 + chunk + i * 4);
            for (i = 0; i < count; ++i)
                w_u32(destination + chunk + i * 4, words[i]);
        }
    }
    w_u8(record + 31, 0x80);
    flag = r_u8(record + 31);
    w_u32(record + 64, 0x8003ECE8);
    w_u32(record + 60, 0x8003ED48);
    w_u8(record + 31, flag | 0x60);
    random = randPSX();
    denominator = r_s32(0x800880F4);
    quotient = (uint32)td5_divide_checked(random, denominator);
    w_u32(record + 32, (uint32)random - quotient * (uint32)denominator);
    random = randPSX();
    denominator = r_s32(0x800880F8);
    quotient = (uint32)td5_divide_checked(random, denominator);
    w_u32(record + 40, 0);
    w_u32(record + 36, (uint32)random - quotient * (uint32)denominator);
    random = randPSX();
    life = ((uint32)(random % 3) + magnitude / 8 + 1) * 5;
    w_u8(record + 2, life);
    life = r_u8(record + 2);
    size = magnitude * 512 + 0x4000;
    quotient = (uint32)td5_divide_checked((sint32)((size & 0xFE00) - 0x1000), (sint32)life);
    w_u16(record + 4, size);
    w_u16(record + 6, 0 - quotient);
    random = randPSX();
    result = (uint32)(random % 6144) + 4096;
    w_u16(record + 8, result);
    return result;
}

uint32 td5_8003f3c8(uint32 index, uint32 original_stack)
{
    uint32 frame = original_stack - 0xA8;
    uint32 record = 0x8009EFB0 + index * 68;
    uint32 words[4], matrix, copy, chunk, i, object, x, y, z, value, result;
    sint32 angle;

    FUNCTION_MARKER(0x8003F3C8, "SLUS_006.10");
    for (copy = 0; copy < 3; ++copy)
    {
        matrix = frame + 0x10 + copy * 32;
        for (chunk = 0; chunk < 32; chunk += 16)
        {
            for (i = 0; i < 4; ++i)
                words[i] = r_u32(0x80010D18 + chunk + i * 4);
            for (i = 0; i < 4; ++i)
                w_u32(matrix + chunk + i * 4, words[i]);
        }
    }
    x = r_u32(0x800879AC);
    y = r_u32(0x800879B0);
    /* Aligned SWL/SWR pairs each store the full word */
    w_u32(frame + 0x70, x);
    w_u32(frame + 0x70, x);
    w_u32(frame + 0x74, y);
    w_u32(frame + 0x74, y);
    angle = r_s16(record);
    object = r_u32(0x800886F8);
    matrix = frame + 0x10;
    RotMatrixX(angle, (MATRIX *)psx_addr(matrix, 32));
    RotMatrixZ(r_s16(record + 4), (MATRIX *)psx_addr(matrix, 32));
    RotMatrixY(r_s16(record + 2), (MATRIX *)psx_addr(matrix, 32));
    w_u32(object + 16, r_u32(record + 32));
    w_u32(object + 20, r_u32(record + 36));
    w_u32(object + 24, r_u32(record + 40));
    td5_800797fc(matrix, object);
    td5_8006e180(object, frame);
    xport_gte_write_data(0, r_u32(frame + 0x70));
    xport_gte_write_data(1, r_u32(frame + 0x74));
    xport_gte_execute(0x480012);
    w_u32(frame + 0x78, xport_gte_read_data(25));
    w_u32(frame + 0x7C, xport_gte_read_data(26));
    w_u32(frame + 0x80, xport_gte_read_data(27));
    angle = r_s16(0x80088298);
    object = r_u32(0x80088790);
    matrix = frame + 0x50;
    RotMatrixX(-angle, (MATRIX *)psx_addr(matrix, 32));
    RotMatrixX(r_s16(record), (MATRIX *)psx_addr(matrix, 32));
    RotMatrixZ(r_s16(record + 4), (MATRIX *)psx_addr(matrix, 32));
    RotMatrixY(r_s16(record + 2), (MATRIX *)psx_addr(matrix, 32));
    w_u32(object + 16, r_u32(record + 32));
    w_u32(object + 20, r_u32(record + 36));
    w_u32(object + 24, r_u32(record + 40));
    td5_800797fc(matrix, object);
    x = r_u32(frame + 0x78);
    y = r_u32(frame + 0x7C);
    z = r_u32(frame + 0x80);
    xport_gte_write_control(5, x);
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    x = r_u32(0x80092210);
    y = r_u32(0x80092214);
    xport_gte_write_control(0, x);
    xport_gte_write_control(1, y);
    x = r_u32(0x80092218);
    y = r_u32(0x8009221C);
    z = r_u32(0x80092220);
    xport_gte_write_control(2, x);
    xport_gte_write_control(3, y);
    xport_gte_write_control(4, z);
    td5_8006e180(object, frame);
    object = r_u32(0x80088740);
    value = r_u32(record + 32);
    angle = r_s16(0x80088298);
    w_u32(object + 16, value);
    w_u32(object + 20, r_u32(record + 36));
    w_u32(object + 24, r_u32(record + 40));
    matrix = frame + 0x30;
    RotMatrixY(angle, (MATRIX *)psx_addr(matrix, 32));
    RotMatrixX(r_s16(record), (MATRIX *)psx_addr(matrix, 32));
    RotMatrixZ(r_s16(record + 4), (MATRIX *)psx_addr(matrix, 32));
    RotMatrixY(r_s16(record + 2), (MATRIX *)psx_addr(matrix, 32));
    td5_800797fc(matrix, object);
    td5_8006e180(object, frame);
    result = r_u16(0x80088298) + 0x100;
    w_u16(0x80088298, result);
    SetRotMatrix((MATRIX *)psx_addr(0x8009FCF8, 20));
    return result;
}

uint32 td5_8003f7c4(void)
{
    uint32 slot, record, value;

    FUNCTION_MARKER(0x8003F7C4, "SLUS_006.10");
    slot = td5_8003bcf8();
    if (slot == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    record = 0x8009EFB0 + slot * 68;
    w_u16(record, 0);
    w_u16(record + 2, 0);
    w_u16(record + 4, 0xFFF0);
    w_u8(record + 31, 0x81);
    w_u32(record + 64, 0x8003F724);
    w_u32(record + 60, 0x8003F3C8);
    w_u32(record + 32, r_u32(0x80098664));
    w_u32(record + 36, r_u32(0x80098668) + 0x5DC00);
    w_u32(record + 40, r_u32(0x8009866C));
    value = r_u16(0x80098510);
    w_u16(0x800A5088, value);
    return value;
}

uint32 td5_8003f8a8(uint32 index)
{
    uint32 record = 0x8009EFB0 + index * 68;
    uint32 x, y, z, value, velocity, axis, size_a, step_a, size_b, step_b;
    uint32 frame, life, result, primitive;

    FUNCTION_MARKER(0x8003F8A8, "SLUS_006.10");
    x = r_u32(record + 12);
    y = r_u32(record + 16);
    w_u32(record + 12, (uint32)((sint32)x / 16));
    z = r_u32(record + 20);
    w_u32(record + 16, (uint32)((sint32)y / 16));
    w_u32(record + 20, (uint32)((sint32)z / 16));
    w_u32(record + 12, 0);
    w_u32(record + 16, 0);
    w_u32(record + 20, 0);
    for (axis = 0; axis < 3; ++axis)
    {
        value = r_u32(record + 32 + axis * 4);
        velocity = r_u32(record + 12 + axis * 4);
        w_u32(record + 32 + axis * 4, value + velocity);
    }
    size_a = r_u16(record + 4);
    step_a = r_u16(record + 6);
    size_b = r_u16(record + 8);
    step_b = r_u16(record + 10);
    frame = r_u8(record);
    life = r_u8(record + 2);
    w_u8(record, (frame + 1) & 31);
    result = life + 255;
    w_u16(record + 4, size_a + step_a);
    w_u16(record + 8, size_b + step_b);
    w_u8(record + 2, result);
    if (life == 0)
    {
        primitive = r_u8(record + 1);
        result = 0xFFFFFFFD;
        w_u8(0x8009EC00 + primitive, result);
        w_u8(record + 31, 0);
    }
    return result;
}

uint32 td5_8003fd80(uint32 translation, uint32 object, uint32 incoming_v0, sint32 incoming_v1)
{
    uint32 vertex = object + incoming_v0;
    uint32 index = 0;
    uint32 value, offset;
    sint32 count;

    FUNCTION_MARKER(0x8003FD80, "SLUS_006.10");
    if (incoming_v1 <= 0)
        return incoming_v0;
    do
    {
        value = r_u16(vertex);
        offset = r_u16(translation);
        w_u16(vertex, value + offset);
        value = r_u16(vertex + 2);
        offset = r_u16(translation + 2);
        w_u16(vertex + 2, value + offset);
        value = r_u16(vertex + 4);
        offset = r_u16(translation + 4);
        w_u16(vertex + 4, value + offset);
        count = r_s16(object + 14);
        ++index;
        vertex += 8;
    } while ((sint32)index < count);
    return 0;
}

static uint32 td5_flare_vertical_offset(uint32 product)
{
    sint64 scaled = (sint64)(sint32)product * (sint32)0x80010003;
    uint32 combined = (uint32)(scaled >> 32) + product;

    return (uint32)((sint32)combined >> 14) - (uint32)((sint32)product >> 31);
}

uint32 td5_80041a44(uint32 car)
{
    static const uint32 bases[3] = {0x80091240, 0x80091290, 0x800912C8};
    static const uint32 clear_offsets[6] = {12, 13, 14, 20, 21, 22};
    uint32 index = 0, offset = 0, slot, packet, group, component, phase, color;
    uint32 brightness, rgb[3], screen, depth, raw_depth, flags, x, y, value, other;
    uint32 size, negative_size, angle, cosine, sine, product, width, height;
    uint32 dx, dy, length, bank_offset, packet_offset, buffer, shift, ordering, tag, link;
    sint32 correction, count;

    FUNCTION_MARKER(0x80041A44, "SLUS_006.10");
    (void)car;
    SetRotMatrix((MATRIX *)psx_addr(0x80092210, 20));
    value = r_u32(0x80092224);
    other = r_u32(0x80092228);
    xport_gte_write_control(5, value);
    product = r_u32(0x8009222C);
    xport_gte_write_control(6, other);
    xport_gte_write_control(7, product);
    count = r_s32(0x8007E394);
    if (count <= 0)
        return (uint32)count;
    do
    {
        slot = r_u32(0x80088104);
        phase = r_u8(0x8009130C + offset);
        for (group = 0; group < 2; ++group)
            for (component = 0; component < 6; ++component)
            {
                if (group != 0 || component != 0)
                    slot = r_u32(0x80088104);
                w_u8(bases[group + 1] + slot * 28 + offset + clear_offsets[component], 0);
            }
        product = (uint32)rsin((sint32)(0 - phase * 4)) * 255;
        if ((sint32)product < 0)
            product += 4095;
        brightness = product >> 12;
        color = r_u8(0x8007E398 + index);
        if (color < 4)
        {
            rgb[0] = color == 0 || color == 3 ? brightness : 0;
            rgb[1] = color == 2 ? brightness : color == 3 ? (brightness & 255) >> 1 : 0;
            rgb[2] = color == 1 ? brightness : 0;
            for (group = 0; group < 3; ++group)
                for (component = 0; component < 3; ++component)
                {
                    slot = r_u32(0x80088104);
                    w_u8(bases[group] + slot * (group == 0 ? 40 : 28) + offset + 4 + component, rgb[component]);
                }
        }
        xport_gte_write_data(0, r_u32(0x8007E39C + index * 8));
        xport_gte_write_data(1, r_u32(0x8007E3A0 + index * 8));
        xport_gte_execute(0x180001);
        screen = xport_gte_read_data(14);
        depth = (uint32)((sint32)xport_gte_read_data(19) >> 2);
        raw_depth = xport_gte_read_data(19);
        flags = xport_gte_read_control(31);
        if ((flags & 0x1C66000) == 0 && r_s32(0x8008813C) < (sint32)depth
            && (sint32)depth < r_s32(0x80088140))
        {
            correction = (sint32)((raw_depth - 400) << 12) / 5120;
            value = raw_depth - (correction < 0 ? 0 : (uint32)correction);
            size = (uint32)td5_divide_checked(32768, (sint32)value);
            size *= (uint32)r_s16(0x800885C8);
            size = (uint32)((sint32)size / 4096);
            x = screen & 65535;
            y = screen >> 16;
            width = r_u32(0x800880F4);
            height = r_u32(0x800880F8);
            dx = (uint32)(sint32)(sint16)x - (uint32)((sint32)width / 2);
            dy = (uint32)(sint32)(sint16)y - (uint32)((sint32)height / 2);
            angle = (uint32)td5_8007b0c0((sint32)dx, (sint32)dy) & 4095;
            negative_size = 0 - size;
            for (component = 0; component < 4; ++component)
            {
                cosine = (uint32)rcos((sint32)angle);
                sine = (uint32)rsin((sint32)angle);
                if (component == 0)
                    product = negative_size * cosine + size * (0 - sine);
                else if (component == 1)
                    product = (cosine - sine) * size;
                else if (component == 2)
                    product = (cosine - sine) * negative_size;
                else
                    product = size * cosine + negative_size * (0 - sine);
                slot = r_u32(0x80088104);
                w_u16(0x80091248 + slot * 40 + offset + component * 8,
                    x + (uint32)((sint32)product / 16384));
                sine = (uint32)rsin((sint32)angle);
                cosine = (uint32)rcos((sint32)angle);
                if (component == 0)
                    product = negative_size * sine + size * cosine;
                else if (component == 1)
                    product = (cosine + sine) * size;
                else if (component == 2)
                    product = (cosine + sine) * negative_size;
                else
                    product = size * sine + negative_size * cosine;
                slot = r_u32(0x80088104);
                if (component == 3)
                    width = r_u32(0x800880F4);
                w_u16(0x8009124A + slot * 40 + offset + component * 8,
                    y + td5_flare_vertical_offset(product));
            }
            dx = (uint32)((sint32)width / 2) - (uint32)(sint32)(sint16)x;
            height = r_u32(0x800880F8);
            dy = (uint32)((sint32)height / 2) - (uint32)(sint32)(sint16)y;
            length = (uint32)SquareRoot0((sint32)(dx * dx + dy * dy));
            if (length == 0)
                length = 1;
            slot = r_u32(0x80088104);
            packet_offset = slot * 28 + offset;
            w_u16(0x80091298 + packet_offset, x);
            w_u16(0x8009129A + packet_offset, y);
            for (component = 0; component < 4; ++component)
            {
                angle = component < 2 ? size : negative_size;
                if ((component & 1) == 0)
                {
                    cosine = (uint32)rcos((sint32)angle);
                    sine = (uint32)rsin((sint32)angle);
                    product = dx * cosine + dy * (0 - sine);
                }
                else
                {
                    sine = (uint32)rsin((sint32)angle);
                    cosine = (uint32)rcos((sint32)angle);
                    product = dx * sine + dy * cosine;
                }
                slot = r_u32(0x80088104);
                if (component == 3)
                {
                    bank_offset = slot * 28;
                    packet_offset = bank_offset + offset;
                }
                value = size * ((uint32)((sint32)product / 4096) * 4);
                value = (uint32)td5_divide_checked((sint32)value, (sint32)length);
                w_u16(0x800912A0 + slot * 28 + offset + (component / 2) * 8 + (component & 1) * 2,
                    ((component & 1) == 0 ? x : y) + value);
            }
            w_u16(0x800912D0 + packet_offset, x);
            w_u16(0x800912D2 + packet_offset, y);
            value = r_u16(0x800912A0 + packet_offset);
            w_u16(0x800912D8 + packet_offset, x + x - value);
            value = r_u16(0x800912A2 + packet_offset);
            w_u16(0x800912DA + packet_offset, y + y - value);
            value = r_u16(0x800912A8 + packet_offset);
            w_u16(0x800912E0 + packet_offset, x + x - value);
            value = r_u16(0x800912AA + packet_offset);
            buffer = r_u32(0x800880F0);
            shift = r_u32(0x80088110);
            w_u16(0x800912E2 + packet_offset, y + y - value);
            packet = 0x80091290 + packet_offset;
            tag = r_u32(packet);
            ordering = r_u32(buffer + 0x70);
            link = (uint32)((sint32)depth >> ((shift - 1) & 31)) * 4;
            value = r_u32(ordering + link);
            w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
            ordering = r_u32(buffer + 0x70) + link;
            value = r_u32(ordering);
            w_u32(ordering, (value & 0xFF000000) | (packet & 0xFFFFFF));
            for (group = 1; group < 3; ++group)
            {
                ordering = r_u32(buffer + 0x70);
                packet = group == 1 ? 0x800912C8 + packet_offset : 0x80091240 + slot * 40 + offset;
                tag = r_u32(packet);
                value = r_u32(ordering + link);
                w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
                ordering = r_u32(buffer + 0x70) + link;
                value = r_u32(ordering);
                w_u32(ordering, (value & 0xFF000000) | (packet & 0xFFFFFF));
            }
        }
        count = r_s32(0x8007E394);
        offset += 208;
        ++index;
    } while ((sint32)index < count);
    return 0;
}

uint32 td5_800437ac(uint32 slot, sint32 distance)
{
    uint32 result, owner, step, counter, points, value, mode, arrests;
    sint32 scale, remaining, random;

    FUNCTION_MARKER(0x800437AC, "SLUS_006.10");
    result = r_u8(0x800879B4);
    if (result != 0)
        return result;
    owner = r_u32(0x800879B8);
    if (slot == owner)
        return 0;
    scale = r_s16(0x800885C8);
    result = distance < 10000;
    if (scale == 0)
        return result;
    if (result != 0)
        return (uint32)(distance < 20001);
    step = distance < 20001 ? 512 : 1024;
    result = r_u32(0x800886D0);
    if (result != slot)
    {
        random = randPSX();
        result = (uint32)(random % 8);
        w_u32(0x8008792C, result);
        w_u32(0x80087930, 0);
        w_u32(0x80087934, 0);
        w_u32(0x80087938, 0);
        w_u32(0x800886D0, slot);
        return result;
    }
    counter = 0x8009DBA0 + slot * 2;
    remaining = r_s16(counter);
    if (remaining <= 0)
        return (uint32)remaining;
    value = r_u16(counter);
    w_u32(0x800886D0, slot);
    w_u16(counter, value - step);
    points = 0x80098740 + owner * 864;
    value = r_u32(points);
    w_u32(points, value + (step == 512 ? 10 : 20));
    remaining = r_s16(counter);
    if (remaining <= 0)
    {
        owner = r_u32(0x800879B8);
        points = 0x80098740 + owner * 864;
        arrests = r_u16(0x800883A0);
        value = r_u32(points);
        mode = r_u32(0x8008771C);
        w_u32(points, value + 50);
        w_u16(0x800883A0, arrests + 1);
        if (mode == 3)
            w_u8(0x800879B4, 90);
    }
    remaining = r_s16(counter);
    result = (uint32)remaining;
    if (remaining < 0)
        value = 0;
    else
    {
        result = remaining < 4097;
        value = result != 0 ? (uint32)remaining : 4096;
    }
    w_u16(counter, value);
    return result;
}

uint32 td5_800439c8(void)
{
    sint32 value;

    FUNCTION_MARKER(0x800439C8, "SLUS_006.10");
    value = r_s16(0x800885C8);
    if (value > 0)
        w_u16(0x800885C8, (uint32)value - 512);
    value = r_s16(0x800885C8);
    if (value < 0)
        value = 0;
    else if (value > 4096)
        value = 4096;
    w_u16(0x800885C8, (uint32)value);
    return td5_set_engine_parameter(value);
}

uint32 td5_8004515c(uint32 car, uint32 flags, uint32 original_stack, uint32 incoming_v0)
{
    uint32 point = original_stack - 0x38 + 0x18;
    uint32 handle, slot, selected, value, type, address, result;
    sint32 random, amount;

    FUNCTION_MARKER(0x8004515C, "SLUS_006.10");
    if ((flags & 2) == 0)
        return incoming_v0;
    handle = r_u8(car + 0x349);
    if (handle == 255)
    {
        slot = r_u8(car + 0x34D);
        value = td5_800443ac(0, slot, 0, 55, 26);
        w_u8(car + 0x349, value);
    }
    handle = r_u8(car + 0x34A);
    if (handle == 255)
    {
        slot = r_u8(car + 0x34D);
        value = td5_800443ac(1, slot, 1, 55, 26);
        w_u8(car + 0x34A, value);
    }
    random = randPSX();
    amount = r_s16(car + 0x314);
    if (random % 50 < amount / 2)
    {
        random = randPSX();
        selected = car + ((uint32)random & 1) * 12;
        value = r_u32(selected + 0xE4);
        w_u32(point + 4, value + 0x7800);
        w_u32(point, r_u32(selected + 0xE0));
        w_u32(point + 8, r_u32(selected + 0xE8));
        td5_8003ce3c(car, point, 0);
    }
    handle = r_u8(car + 0x349);
    amount = r_s16(car + 0x314);
    w_u8(0x8009D8D5 + handle * 7, (uint32)(amount / 4));
    handle = r_u8(car + 0x34A);
    amount = r_s16(car + 0x314);
    w_u8(0x8009D8D5 + handle * 7, (uint32)(amount / 4));
    type = r_u8(car + 0x348);
    if (type != 2 && type - 5 >= 2 && type != 4 && type != 9)
        return 9;
    handle = r_u8(car + 0x349);
    address = 0x8009D8D5 + handle * 7;
    value = r_u8(address);
    w_u8(address, value >> 1);
    handle = r_u8(car + 0x34A);
    address = 0x8009D8D5 + handle * 7;
    result = r_u8(address) >> 1;
    w_u8(address, result);
    return result;
}

uint32 td5_80045378(uint32 incoming_v0)
{
    FUNCTION_MARKER(0x80045378, "SLUS_006.10");
    return incoming_v0;
}

uint32 td5_80045ec0(uint32 position)
{
    uint32 rotation[5], local[3], transformed[3], delta[3];
    uint32 axis, value, other, shift, denominator, slot, packet, buffer, ordering;
    uint32 index = 0, point, flag, x, y, z, width, height, projected_x, projected_y;
    uint32 tag, link, texture, u, depth;
    sint32 coordinate, count;

    FUNCTION_MARKER(0x80045EC0, "SLUS_006.10");
    rotation[0] = r_u32(0x80097648);
    rotation[1] = r_u32(0x8009764C);
    xport_gte_write_control(0, rotation[0]);
    xport_gte_write_control(1, rotation[1]);
    rotation[2] = r_u32(0x80097650);
    rotation[3] = r_u32(0x80097654);
    rotation[4] = r_u32(0x80097658);
    xport_gte_write_control(2, rotation[2]);
    xport_gte_write_control(3, rotation[3]);
    xport_gte_write_control(4, rotation[4]);
    for (axis = 0; axis < 3; ++axis)
    {
        value = r_u32(position + axis * 4);
        other = r_u32(0x80089B48 + axis * 4);
        value = (uint32)((sint32)(value - other) / 256);
        other = r_u16(0x800879FC + axis * 2);
        local[axis] = (other - value) & 65535;
    }
    x = r_u32(position);
    y = r_u32(position + 4);
    z = r_u32(position + 8);
    w_u32(0x80089B48, x);
    w_u32(0x80089B4C, y);
    w_u32(0x80089B50, z);
    xport_gte_write_data(0, local[0] | (local[1] << 16));
    xport_gte_write_data(1, local[2]);
    xport_gte_execute(0x486012);
    transformed[0] = xport_gte_read_data(9) & 65535;
    transformed[1] = xport_gte_read_data(10) & 65535;
    transformed[2] = xport_gte_read_data(11) & 65535;
    shift = r_u32(0x80088164);
    denominator = r_u32(0x80088160);
    for (axis = 0; axis < 3; ++axis)
    {
        value = (uint32)(sint32)(sint16)transformed[axis];
        value = (value << (shift & 31)) << 12;
        delta[axis] = (uint32)td5_divide_checked((sint32)value, (sint32)denominator);
    }
    slot = r_u32(0x80088104);
    packet = r_u32(0x800882A8 + slot * 4);
    buffer = r_u32(0x800880F0);
    count = r_s32(0x80087A08);
    ordering = r_u32(buffer + 0x70);
    if (count <= 0)
        return buffer;
    do
    {
        point = 0x80089688 + index * 8;
        flag = 0x80089A88 + index;
        for (axis = 0; axis < 3; ++axis)
            w_u16(point + axis * 2, r_u16(point + axis * 2) + delta[axis]);
        coordinate = r_s16(point);
        if (coordinate < -8000)
        {
            w_u16(point, (uint32)coordinate + 16000);
            w_u8(flag, 0);
        }
        coordinate = r_s16(point);
        if (coordinate >= 8000)
        {
            w_u16(point, (uint32)coordinate - 16000);
            w_u8(flag, 0);
        }
        coordinate = r_s16(point + 2);
        if (coordinate < -8000)
        {
            w_u16(point + 2, (uint32)coordinate + 9000);
            w_u8(flag, 0);
        }
        coordinate = r_s16(point + 2);
        if (coordinate >= 1000)
        {
            w_u16(point + 2, (uint32)coordinate - 9000);
            w_u8(flag, 0);
        }
        coordinate = r_s16(point + 4);
        if (coordinate < 200)
        {
            w_u16(point + 4, (uint32)coordinate + 16183);
            w_u8(flag, 0);
        }
        coordinate = r_s16(point + 4);
        if (coordinate >= 16383)
        {
            w_u16(point + 4, (uint32)coordinate - 16183);
            w_u8(flag, 0);
        }
        x = (uint32)r_s16(point);
        z = (uint32)r_s16(point + 4);
        projected_x = (uint32)td5_divide_checked((sint32)(x * 180), (sint32)z);
        y = (uint32)r_s16(point + 2);
        projected_y = (uint32)td5_divide_checked((sint32)(y * 180), (sint32)z);
        width = r_u32(0x800880F4);
        height = r_u32(0x800880F8);
        x = (uint32)((sint32)width / 2) + projected_x;
        y = (uint32)((sint32)height / 2) + projected_y;
        w_u16(packet + 8, x - 1);
        w_u16(packet + 16, x + 3);
        w_u16(packet + 24, x - 1);
        w_u16(packet + 32, x + 3);
        w_u16(packet + 10, y - 1);
        w_u16(packet + 18, y - 1);
        w_u16(packet + 26, y + 3);
        w_u16(packet + 34, y + 3);
        tag = r_u32(packet);
        link = r_u32(ordering);
        w_u32(packet, (tag & 0xFF000000) | (link & 0xFFFFFF));
        tag = r_u32(ordering);
        w_u32(ordering, (tag & 0xFF000000) | (packet & 0xFFFFFF));
        depth = (uint32)r_s16(point + 4);
        texture = r_u32(0x800882B0);
        u = r_u8(texture + 14);
        value = (uint32)((sint32)depth / 2048) * 4;
        w_u8(packet + 12, u + 28 - value);
        texture = r_u32(0x800882B0);
        w_u8(packet + 13, r_u8(texture + 15));
        texture = r_u32(0x800882B0);
        u = r_u8(texture + 14);
        depth = (uint32)r_s16(point + 4);
        value = (uint32)((sint32)depth / 2048) * 4;
        w_u8(packet + 20, u + 32 - value);
        texture = r_u32(0x800882B0);
        w_u8(packet + 21, r_u8(texture + 15));
        depth = (uint32)r_s16(point + 4);
        texture = r_u32(0x800882B0);
        u = r_u8(texture + 14);
        value = (uint32)((sint32)depth / 2048) * 4;
        w_u8(packet + 28, u + 28 - value);
        texture = r_u32(0x800882B0);
        w_u8(packet + 29, r_u8(texture + 15) + 4);
        texture = r_u32(0x800882B0);
        u = r_u8(texture + 14);
        depth = (uint32)r_s16(point + 4);
        value = (uint32)((sint32)depth / 2048) * 4;
        w_u8(packet + 36, u + 32 - value);
        texture = r_u32(0x800882B0);
        w_u8(packet + 37, r_u8(texture + 15) + 4);
        packet += 40;
        count = r_s32(0x80087A08);
        ++index;
    } while ((sint32)index < count);
    return 0;
}

uint32 td5_800474c8(uint32 slot, uint32 incoming_v0)
{
    uint32 value, result;

    FUNCTION_MARKER(0x800474C8, "SLUS_006.10");
    if (incoming_v0 == 0)
    {
        w_u32(0x80088610 + slot * 4, 0);
        td5_8001e034(0x80098670 + slot * 864, 0, slot);
    }
    value = r_u32(0x800875E8);
    w_u32(0x800884D0 + slot * 4, 1);
    w_u32(0x800883E8 + slot * 4, 0);
    result = value | ((uint32)1 << (slot & 31));
    w_u32(0x800875E8, result);
    return result;
}

uint32 td5_80047bf0(uint32 car)
{
    static const sint32 expected[4] = {0, 1, 3, 7};
    uint32 slot, owner, lap, time, index, address, value, other, flags, stage;
    uint32 progress, interval;
    sint32 segment, amount, denominator;

    FUNCTION_MARKER(0x80047BF0, "SLUS_006.10");
    slot = r_u8(car + 0x34D);
    owner = r_u32(0x800879B8);
    if (slot == owner && r_u32(0x8008771C) == 3)
        return 3;
    lap = r_u8(car + 0x356);
    segment = r_s16(car + 0x72);
    time = r_u16(car + 0x324);
    w_u16(car + 0x326 + lap * 2, time);
    lap = r_u8(car + 0x356);
    index = 0;
    if (lap != 0)
    {
        do
        {
            address = car + 0x326 + lap * 2;
            value = r_u16(address);
            other = r_u16(car + 0x326 + index * 2);
            w_u16(address, value - other);
            lap = r_u8(car + 0x356);
            ++index;
        } while (index < lap);
    }
    if (segment == r_s32(0x8008765C) && r_s16(car + 0x30E) == 15)
    {
        lap = r_u8(car + 0x356) + 1;
        w_u8(car + 0x356, lap);
        value = r_u32(0x800876F4);
        w_u16(car + 0x30E, 0);
        if ((lap & 255) == value)
        {
            if (r_u32(car + 0x300) == 0)
            {
                value = r_u32(car + 0x2EC);
                time = r_u16(car + 0x324);
                w_u32(car + 0x300, time);
                amount = (sint32)value / 256;
                if (amount <= 0)
                    amount = 1;
                slot = r_u8(car + 0x34D);
                w_u16(car + 0x30C, (uint32)amount);
                value = r_u32(0x8009782C + slot * 284);
                denominator = (sint16)amount;
                value = (uint32)td5_divide_checked((sint32)(value * 1500), denominator);
                w_u16(car + 0x30E, value);
            }
            slot = r_u8(car + 0x34D);
            if (slot == r_u32(0x800885F8))
            {
                if (r_u32(0x80087738) == 0)
                {
                    w_u32(0x80088610 + slot * 4, 0);
                    td5_8001e034(0x80098670 + slot * 864, 0, slot);
                }
                flags = r_u32(0x800875E8);
                w_u32(0x800884D0 + slot * 4, 1);
                w_u32(0x800883E8 + slot * 4, 0);
                w_u32(0x800875E8, flags | ((uint32)1 << (slot & 31)));
            }
        }
    }
    value = r_u32(0x8008765C);
    other = r_u32(0x8008841C);
    progress = value * 2;
    interval = (uint32)((sint32)(other - progress) / 5);
    for (stage = 0; stage < 4; ++stage)
    {
        if (segment == (sint32)progress)
        {
            amount = r_s16(car + 0x30E);
            if (amount == expected[stage])
                w_u16(car + 0x30E, (uint32)amount | ((uint32)1 << stage));
        }
        progress += interval;
    }
    return 0;
}

uint32 td5_80048e84(uint32 image, uint32 image_x, uint32 image_y, uint32 palette_x, uint32 palette_y)
{
    PSX_RECT rectangle;
    uint32 block, index, address, value, bytes;

    FUNCTION_MARKER(0x80048E84, "SLUS_006.10");
    if (r_u32(image) != 16)
        return 16;
    block = image + 8;
    rectangle.x = (sint16)palette_x;
    rectangle.y = (sint16)palette_y;
    rectangle.w = (sint16)r_u16(block + 8);
    rectangle.h = (sint16)r_u16(block + 10);
    for (index = 0; index < 16; ++index)
    {
        address = block + 12 + index * 2;
        value = r_u16(address);
        w_u16(address, value | 0x8000);
    }
    bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(block + 12, bytes));
    block += r_u32(block) & 0xFFFFFFFC;
    if (r_u32(block) == 4)
        return 4;
    rectangle.x = (sint16)image_x;
    rectangle.y = (sint16)image_y;
    rectangle.w = (sint16)r_u16(block + 8);
    rectangle.h = (sint16)r_u16(block + 10);
    bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
    return (uint32)LoadImagePSX(&rectangle, (uint32 *)psx_addr(block + 12, bytes));
}

uint32 td5_80049258(uint32 car)
{
    uint32 result, reduction, state;
    sint32 speed, amount = 0;
    sint32 segment, next_segment;

    FUNCTION_MARKER(0x80049258, "SLUS_006.10");
    result = r_u32(car + 0x300);
    if (result != 0)
        return result;
    if (r_u8(car + 0x34E) == 0)
    {
        speed = r_s32(car + 0x2F0);
        if (speed > 0 && (r_u32(0x800885E8) & 3) == 0)
        {
            reduction = r_u8(car + 0x35B);
            state = r_u8(car + 0x348);
            amount = (speed >> 15) - (sint32)(reduction >> 1);
            if (state >= 16)
                amount = 0;
            if (amount < 0)
                amount = 0;
            if (r_u8(car + 0x34D) == 0)
            {
                segment = r_s16(car + 0x72);
                next_segment = r_s16(car + 0x76);
                if (segment < next_segment)
                    amount = 0;
            }
        }
    }
    result = r_u32(car + 0x2A0) + (uint32)amount;
    w_u32(car + 0x2A0, result);
    return result;
}

uint32 td5_80049538(sint32 node, uint32 slot)
{
    sint32 count;
    uint32 index, matches = 0, first = 0xFFFFFFFF;
    uint32 address, state;
    sint32 candidate;

    FUNCTION_MARKER(0x80049538, "SLUS_006.10");
    count = r_s32(0x80088428);
    if (count > 0)
    {
        address = slot * 12;
        index = 0;
        do
        {
            state = r_u8(0x800A4D10 + address);
            if (state == 0 && first == 0xFFFFFFFF)
                first = index;
            candidate = r_s16(0x800A4D0E + address);
            if (candidate == node && r_u8(0x800A4D10 + address) == 2)
                ++matches;
            ++index;
            address += 24;
        } while ((sint32)index < count);
    }
    return matches != 0 ? 0xFFFFFFFF : first;
}

uint32 td5_800495e8(uint32 object, sint32 count)
{
    uint32 index = 0, matches = 0;
    sint32 node;

    FUNCTION_MARKER(0x800495E8, "SLUS_006.10");
    if (count <= 0)
        return 0;
    node = r_s16(object + 6);
    do
    {
        if (node == r_s32(0x8009D8B0 + index * 4))
            ++matches;
        ++index;
    } while ((sint32)index < count);
    return matches;
}

uint32 td5_80049638(uint32 slot, sint32 node)
{
    sint32 track, groups, quotient, current, end, start, count, candidate;
    uint32 centered, radius, lower, upper, list_count = 0, address, index;
    uint32 state, matches, first, offset, entry, source, length, destination;
    uint32 result;

    FUNCTION_MARKER(0x80049638, "SLUS_006.10");
    track = r_s32(0x8008841C);
    groups = (sint32)((uint32)track + 11) / 12;
    quotient = td5_divide_checked(node, track);
    current = (sint32)((uint32)node - (uint32)quotient * (uint32)track);
    if (node < 0)
        current = (sint32)((uint32)current + (uint32)track);
    centered = (uint32)(current / 12) * 12 + 6;
    radius = r_u32(0x80087A74);
    track = r_s32(0x8008841C);
    lower = centered - radius;
    upper = centered + radius;
    quotient = td5_divide_checked((sint32)upper, track);
    current = (sint32)(upper - (uint32)quotient * (uint32)track);
    if ((sint32)lower < 0)
        lower += (uint32)track;
    current /= 12;
    quotient = td5_divide_checked(current, groups);
    end = (sint32)((uint32)current - (uint32)quotient * (uint32)groups);
    start = (sint32)lower / 12;
    current = start;
    address = 0x8009D8B0;
    while (current != end)
    {
        w_u32(address, (uint32)current);
        current = (sint32)((uint32)current + 1);
        quotient = td5_divide_checked(current, groups);
        current = (sint32)((uint32)current - (uint32)quotient * (uint32)groups);
        address += 4;
        ++list_count;
    }
    count = r_s32(0x80088428);
    index = 0;
    if (count > 0)
    {
        entry = 0x800A4D08 + slot * 12;
        do
        {
            state = r_u8(entry + 8);
            if (state == 3)
                w_u8(entry + 8, 0);
            if (state == 2)
            {
                matches = 0;
                if ((sint32)list_count > 0)
                {
                    candidate = r_s16(entry + 6);
                    offset = 0;
                    do
                    {
                        if (candidate == r_s32(0x8009D8B0 + offset * 4))
                            ++matches;
                        ++offset;
                    } while ((sint32)offset < (sint32)list_count);
                }
                if (matches == 0)
                    w_u8(entry + 8, 3);
            }
            count = r_s32(0x80088428);
            ++index;
            entry += 24;
        } while ((sint32)index < count);
    }
    result = slot * 2;
    current = start;
    while (current != end)
    {
        count = r_s32(0x80088428);
        first = 0xFFFFFFFF;
        matches = 0;
        index = 0;
        if (count > 0)
        {
            offset = slot * 12;
            do
            {
                state = r_u8(0x800A4D10 + offset);
                if (state == 0 && first == 0xFFFFFFFF)
                    first = index;
                candidate = r_s16(0x800A4D0E + offset);
                if (candidate == current && r_u8(0x800A4D10 + offset) == 2)
                    ++matches;
                ++index;
                offset += 24;
            } while ((sint32)index < count);
        }
        if (matches != 0)
            first = 0xFFFFFFFF;
        result = first * 2;
        if (first != 0xFFFFFFFF)
        {
            entry = 0x800A4D08 + slot * 12 + first * 24;
            result = r_u8(entry + 8);
            if (result == 0)
            {
                destination = r_u32(entry);
                if (destination != 0)
                    td5_free_scene_memory(destination);
                source = r_u32(0x80088400) + (uint32)current * 4;
                source = r_u32(source + 4);
                length = td5_80015a3c(source);
                td5_debug_log(0x80011004, length);
                td5_alloc_scene_memory(length, entry);
                w_u8(entry + 8, 1);
                source = r_u32(0x80088400) + (uint32)current * 4;
                destination = r_u32(entry);
                source = r_u32(source + 4);
                td5_800158d4(source, destination, (sint32)length);
                w_u8(entry + 8, 2);
                w_u16(entry + 6, (uint32)current);
                w_u16(entry + 4, centered);
                result = 2;
            }
        }
        current = (sint32)((uint32)current + 1);
        quotient = td5_divide_checked(current, groups);
        current = (sint32)((uint32)current - (uint32)quotient * (uint32)groups);
    }
    return result;
}

uint32 td5_80049a90(uint32 slot, uint32 original_stack)
{
    uint32 rotation0, rotation1, rotation2, rotation3, rotation4;
    uint32 entry, resource, state, index, object_index, offset, object;
    sint32 count, object_count;
    uint32 frame = original_stack - 0x50;

    FUNCTION_MARKER(0x80049A90, "SLUS_006.10");
    td5_8006e92c();
    td5_8006e1c8(0x800985B0, 0x80098664);
    rotation0 = r_u32(0x80097648);
    rotation1 = r_u32(0x8009764C);
    xport_gte_write_control(0, rotation0);
    xport_gte_write_control(1, rotation1);
    rotation2 = r_u32(0x80097650);
    rotation3 = r_u32(0x80097654);
    rotation4 = r_u32(0x80097658);
    xport_gte_write_control(2, rotation2);
    xport_gte_write_control(3, rotation3);
    xport_gte_write_control(4, rotation4);
    count = r_s32(0x80088428);
    if (count <= 0)
        return (uint32)count;
    entry = 0x800A4D08 + slot * 12;
    index = 0;
    do
    {
        resource = r_u32(entry);
        state = r_u8(entry + 8);
        object_count = r_s32(resource);
        if (state == 2 && object_count > 0)
        {
            object_index = 0;
            do
            {
                offset = r_u32(resource + 4 + object_index * 4);
                object = resource + offset;
                if (td5_80079e34(object) != 0)
                {
                    td5_80079b84(object);
                    td5_80071378(object, frame);
                }
                ++object_index;
            } while ((sint32)object_index < object_count);
        }
        count = r_s32(0x80088428);
        ++index;
        entry += 24;
    } while ((sint32)index < count);
    return 0;
}

uint32 td5_80049bc4(uint32 slot, uint32 original_stack)
{
    uint32 level, header, mode, rotation0, rotation1, rotation2, rotation3, rotation4;
    uint32 entry, resource, state, index, object_index, object;
    sint32 count, object_count;
    uint32 frame = original_stack - 0x60;

    FUNCTION_MARKER(0x80049BC4, "SLUS_006.10");
    level = r_u32(0x80088770);
    header = r_u32(level);
    mode = r_u8(header + 30) & 1;
    if (mode != 0)
    {
        td5_8006e92c();
        td5_8006e1c8(0x800985B0, 0x80098664);
    }
    rotation0 = r_u32(0x80097648);
    rotation1 = r_u32(0x8009764C);
    xport_gte_write_control(0, rotation0);
    xport_gte_write_control(1, rotation1);
    rotation2 = r_u32(0x80097650);
    rotation3 = r_u32(0x80097654);
    rotation4 = r_u32(0x80097658);
    xport_gte_write_control(2, rotation2);
    xport_gte_write_control(3, rotation3);
    xport_gte_write_control(4, rotation4);
    count = r_s32(0x80088428);
    if (count <= 0)
        return mode != 0 ? slot * 2 : (uint32)count;
    entry = 0x800A4D08 + slot * 12;
    index = 0;
    do
    {
        resource = r_u32(entry);
        state = r_u8(entry + 8);
        object_count = r_s32(resource);
        if (state == 2 && object_count > 0)
        {
            object_index = 0;
            do
            {
                object = resource + r_u32(resource + 4 + object_index * 4);
                if (td5_80079e34(object) != 0)
                {
                    td5_80079b84(object);
                    if (mode != 0)
                        td5_80071378(object, frame);
                    else
                        td5_8006e180(object, frame);
                }
                ++object_index;
            } while ((sint32)object_index < object_count);
        }
        count = r_s32(0x80088428);
        ++index;
        entry += 24;
    } while ((sint32)index < count);
    return 0;
}

uint32 td5_80049dec(uint32 mode)
{
    PSX_RECT rectangle;
    uint32 dimension, buffer, result;

    FUNCTION_MARKER(0x80049DEC, "SLUS_006.10");
    w_u32(0x80087668, mode);
    if (mode == 0)
    {
        dimension = r_u32(0x800880F8);
        rectangle.w = 512;
        rectangle.h = 1;
        buffer = r_u32(0x8008810C);
        rectangle.x = 0;
        rectangle.y = (sint16)((uint32)((sint32)dimension / 2) + ((buffer ^ 1) << 3));
        ClearImage(&rectangle, 0, 0, 0);
        dimension = r_u32(0x800880F8);
        buffer = r_u32(0x8008810C);
        rectangle.y = (sint16)((uint32)((sint32)dimension / 2) + ((buffer ^ 1) << 3) + 256);
        ClearImage(&rectangle, 0, 0, 0);
        dimension = r_u32(0x800880F8);
        w_u32(0x800880F8, (uint32)((sint32)dimension / 2));
        td5_8007880c();
        result = r_u32(0x800880F8) * 2;
        w_u32(0x800880F8, result);
    }
    else
    {
        dimension = r_u32(0x800880F4);
        rectangle.w = 2;
        rectangle.h = 512;
        rectangle.y = 0;
        rectangle.x = (sint16)((uint32)((sint32)dimension / 2) - 1);
        ClearImage(&rectangle, 0, 0, 0);
        dimension = r_u32(0x800880F4);
        w_u32(0x800880F4, (uint32)((sint32)dimension / 2));
        td5_8007880c();
        result = r_u32(0x800880F4) * 2;
        w_u32(0x800880F4, result);
    }
    return result;
}

uint32 td5_80049f70(uint32 slot)
{
    uint32 mode, width, height, buffer, offset, result, screen_offset;
    sint32 signed_width, signed_height;

    FUNCTION_MARKER(0x80049F70, "SLUS_006.10");
    mode = r_u32(0x80087668);
    w_u32(0x80087A78, 0);
    if (mode == 0)
    {
        width = r_u32(0x800880F4);
        height = r_u32(0x800880F8);
        td5_set_view_center((sint32)width / 2, (sint32)height / 4);
        height = r_u32(0x800880F8);
        buffer = r_u32(0x8008810C);
        offset = slot * 116;
        w_u16(0x80097668 + offset, 0);
        w_u32(0x80088510, 0);
        w_u32(0x80088518, 0);
        w_u16(0x8009766E + offset, (uint32)((sint32)height / 2));
        width = r_u32(0x800880F4);
        signed_height = r_s16(0x8009766E + offset);
        screen_offset = (buffer ^ 1) << 3;
        w_u16(0x8009766C + offset, width);
        signed_width = (sint16)width;
        w_u16(0x8009766A + offset, (slot << 8) + screen_offset);
        w_u32(0x80088438, (uint32)signed_width);
        w_u32(0x800884F8, (uint32)signed_height);
        result = offset;
    }
    else
    {
        width = r_u32(0x800880F4);
        height = r_u32(0x800880F8);
        td5_set_view_center((sint32)width / 4, (sint32)height / 2);
        offset = slot * 116;
        width = r_u32(0x800880F4);
        buffer = r_u32(0x8008810C);
        height = r_u32(0x800880F8);
        w_u16(0x8009766C + offset, (uint32)((sint32)width / 2) - 1);
        signed_width = r_s16(0x8009766C + offset);
        screen_offset = (buffer ^ 1) << 3;
        w_u16(0x8009766E + offset, height + screen_offset);
        signed_height = r_s16(0x8009766E + offset);
        w_u16(0x80097668 + offset, 0);
        w_u32(0x80088510, 0);
        w_u32(0x80088518, 0);
        result = (slot << 8) + screen_offset;
        w_u16(0x8009766A + offset, result);
        w_u32(0x80088438, (uint32)signed_width);
        w_u32(0x800884F8, (uint32)signed_height);
    }
    return result;
}

uint32 td5_8004a170(uint32 slot)
{
    uint32 mode, width, height, buffer, offset, screen_offset, half, result;
    sint32 signed_width, signed_height;

    FUNCTION_MARKER(0x8004A170, "SLUS_006.10");
    mode = r_u32(0x80087668);
    w_u32(0x80087A78, 1);
    if (mode == 0)
    {
        height = r_u32(0x800880F8);
        width = r_u32(0x800880F4);
        td5_set_view_center((sint32)width / 2, (sint32)((uint32)((sint32)height / 4) + (uint32)((sint32)height / 2)));
        height = r_u32(0x800880F8);
        buffer = r_u32(0x8008810C);
        width = r_u32(0x800880F4);
        offset = slot * 116;
        w_u16(0x80097668 + offset, 0);
        w_u32(0x80088510, 0);
        half = (uint32)((sint32)height / 2);
        w_u16(0x8009766E + offset, half - 1);
        signed_height = r_s16(0x8009766E + offset);
        screen_offset = (buffer ^ 1) << 3;
        result = (slot << 8) + half + screen_offset + 1;
        w_u16(0x8009766C + offset, width);
        signed_width = (sint16)width;
        w_u16(0x8009766A + offset, result);
        w_u32(0x80088438, (uint32)signed_width);
        w_u32(0x80088518, half);
    }
    else
    {
        width = r_u32(0x800880F4);
        height = r_u32(0x800880F8);
        td5_set_view_center((sint32)((uint32)((sint32)width / 4) + (uint32)((sint32)width / 2)), (sint32)height / 2);
        width = r_u32(0x800880F4);
        buffer = r_u32(0x8008810C);
        height = r_u32(0x800880F8);
        offset = slot * 116;
        w_u32(0x80088518, 0);
        half = (uint32)((sint32)width / 2);
        screen_offset = (buffer ^ 1) << 3;
        w_u16(0x8009766C + offset, half - 1);
        w_u16(0x8009766E + offset, height + screen_offset);
        signed_width = r_s16(0x8009766C + offset);
        signed_height = r_s16(0x8009766E + offset);
        ++half;
        result = (slot << 8) + screen_offset;
        w_u16(0x80097668 + offset, half);
        w_u16(0x8009766A + offset, result);
        w_u32(0x80088510, half);
        w_u32(0x80088438, (uint32)signed_width);
    }
    w_u32(0x800884F8, (uint32)signed_height);
    return result;
}

void td5_8004a3a0(uint32 original_stack, uint32 original_frame, uint32 audio_pitch_carrier)
{
    uint32 frame = original_stack - 0x48;
    uint32 level, header, value, mode, buffer, previous_sync, elapsed = 0;
    uint32 index, address, slot, car, first, second, count;
    uint32 color = frame + 0x18;
    sint32 position;

    FUNCTION_MARKER(0x8004A3A0, "SLUS_006.10");
    w_u32(frame + 0x10, 0);
    w_u32(frame + 0x14, 0);
    td5_80013cd8();
    level = r_u32(0x80088770);
    header = r_u32(level);
    value = r_u8(header + 26);
    w_u8(0x800976F5, value);
    w_u8(0x80097681, value);
    header = r_u32(level);
    value = r_u8(header + 27);
    w_u8(0x800976F6, value);
    w_u8(0x80097682, value);
    header = r_u32(level);
    mode = r_u32(0x80087668);
    value = r_u8(header + 28);
    w_u8(0x800976F7, value);
    w_u8(0x80097683, value);
    td5_80049dec(mode);
    buffer = r_u32(0x80088104);
    w_u32(0x800884E4, 0xFFFFFFFF);
    w_u32(0x80088618, 1);
    previous_sync = (uint32)VSync(-1);
    w_u32(0x80088530, 0);
    w_u32(0x800875F0, 0);
    w_u32(0x800885E0, 0);
    w_u32(0x800875E8, 0);
    w_u32(0x800875D8, 0);
    w_u32(0x800885E8, 0);
    td5_8001e034(0x80098670, 1, 0);
    td5_8001e034(0x800989D0, 1, 1);
    slot = r_u32(0x800875F0);
    td5_800542ec(0x80098664 + slot * 864);
    slot = r_u32(0x800875F0);
    td5_800542e0(0x80098634 + slot * 864);
    level = r_u32(0x80088770);
    header = r_u32(level);
    if ((r_u8(header + 30) & 1) != 0)
    {
        level = r_u32(0x80088770);
        value = r_u32(0x80087A7C);
        value = r_u32(0x80087A7C);
        w_u32(color, value);
        w_u32(color, value);
        header = r_u32(level);
        td5_8006e2f4(5200, header + 26, color, 650);
        td5_8006e2e4(-300, 5000);
    }
    while (r_u32(0x800885E0) == 0)
    {
        td5_poll_pads();
        w_u32(0x800885F8, 0);
        td5_80026590(0);
        w_u32(0x800885F8, 1);
        td5_80026590(1);
        for (index = 0; index < 2; ++index)
        {
            address = frame + 0x10 + index * 4;
            if (r_u8(0x80088450 + index * 4) == 255)
            {
                value = r_u32(address) + 1;
                w_u32(address, value);
                if ((sint32)value >= 31)
                {
                    w_u32(address, 0);
                    w_u32(0x8008805C, index + 1);
                }
            }
            else
                w_u32(address, 0);
        }
        td5_8002740c();
        value = r_u32(0x800875E8);
        w_u32(0x800885F8, 0);
        if ((value & 1) == 0)
            td5_800269dc(0, 0);
        value = r_u32(0x800875E8);
        w_u32(0x800885F8, 1);
        if ((value & 2) == 0)
            td5_800269dc(1, 1);
        value = r_u32(0x800875E8);
        w_u32(0x800885F8, 0);
        if ((value & 1) == 0)
            td5_80024c14(0, frame - 0xD8, audio_pitch_carrier);
        value = r_u32(0x800875E8);
        w_u32(0x800885F8, 1);
        if ((value & 2) == 0)
            td5_80024c14(1, frame - 0xD8, audio_pitch_carrier);
        buffer ^= 1;
        td5_800439c8();
        td5_800419f4();
        td5_80026130();
        index = 0;
        count = r_u32(0x800875E4);
        if ((sint32)count > 0)
        {
            car = 0x800984A0;
            do
            {
                td5_8001affc(car);
                count = r_u32(0x800875E4);
                ++index;
                car += 864;
            } while ((sint32)index < (sint32)count);
        }
        level = r_u32(0x80088770);
        header = r_u32(level);
        if ((r_u8(header + 30) & 4) != 0 && r_u32(0x8008873C) == 0)
            td5_8004ffe4(frame, audio_pitch_carrier);
        w_u32(0x800885F8, 0);
        td5_80049f70(buffer);
        td5_swap_display_buffer();
        td5_clear_ordering_table();
        td5_80072140();
        slot = r_u32(0x800885F8);
        position = r_s32(0x80098790 + slot * 864) / 256;
        td5_800720b0(position, 1500, 3000);
        slot = r_u32(0x800885F8);
        td5_8001ebbc(0x800984A0 + slot * 864, slot, frame);
        td5_80047790();
        td5_8003a1b4((sint32)r_u32(0x800885F8));
        td5_80044604();
        slot = r_u32(0x800885F8);
        td5_80038934(0x800984A0 + slot * 864, slot, frame);
        td5_800415d0(0, 0);
        td5_8003b9cc(frame);
        slot = r_u32(0x800885F8);
        td5_80049638(0, r_s16(0x80098512 + slot * 864));
        td5_80049bc4(r_u32(0x800885F8), frame);
        td5_8002e568(r_u32(0x800885F8), 0, frame);
        td5_8002e568(r_u32(0x800885F8) ^ 1, 1, frame);
        index = 2;
        count = r_u32(0x800875E4);
        while ((sint32)index < (sint32)count)
        {
            td5_8002e70c(index, frame);
            count = r_u32(0x800875E4);
            ++index;
        }
        if (r_u32(0x80087734) != 0)
            td5_8003a3e4();
        value = (uint32)td5_read_timer(1, 0);
        w_u32(0x80087600, value);
        w_u32(0x800875FC, value);
        td5_80013cf8();
        while ((sint32)((uint32)VSync(-1) - previous_sync) < 2)
        {
        }
        previous_sync = (uint32)VSync(-1);
        td5_reset_timer(1);
        first = r_u32(0x800875D8);
        second = r_u32(0x800885E8);
        w_u32(0x800875D8, first + 1);
        w_u32(0x800885E8, second + 1);
        DrawSync(0);
        td5_8006c810(buffer);
        td5_draw_ordering_table();
        td5_80053aac(0, frame, audio_pitch_carrier);
        td5_8002c30c(frame);
        td5_80043db8();
        td5_8003bb34(frame);
        w_u32(0x800885F8, 1);
        td5_8004a170(buffer);
        td5_swap_display_buffer();
        td5_clear_ordering_table();
        td5_80072140();
        slot = r_u32(0x800885F8);
        position = r_s32(0x80098790 + slot * 864) / 256;
        td5_800720b0(position, 1500, 3000);
        slot = r_u32(0x800885F8);
        td5_8001ebbc(0x800984A0 + slot * 864, slot, frame);
        td5_80047790();
        td5_8003a1b4((sint32)r_u32(0x800885F8));
        td5_80044604();
        slot = r_u32(0x800885F8);
        td5_80038934(0x800984A0 + slot * 864, slot, frame);
        td5_800415d0(1, 1);
        td5_8003b9cc(frame);
        slot = r_u32(0x800885F8);
        td5_80049638(1, r_s16(0x80098512 + slot * 864));
        td5_80049bc4(r_u32(0x800885F8), frame);
        td5_8002e568(r_u32(0x800885F8) ^ 1, 1, frame);
        td5_8002e568(r_u32(0x800885F8), 0, frame);
        index = 2;
        count = r_u32(0x800875E4);
        while ((sint32)index < (sint32)count)
        {
            td5_8002e70c(index, frame);
            count = r_u32(0x800875E4);
            ++index;
        }
        if (r_u32(0x80087734) != 0)
            td5_8003a3e4();
        td5_800557f0(frame);
        DrawSync(0);
        td5_8006c810(buffer);
        td5_draw_ordering_table();
        td5_80053aac(1, frame, audio_pitch_carrier);
        td5_80054c58(frame, 0x80088450, audio_pitch_carrier, original_frame);
        if ((sint32)elapsed <= 65534)
            ++elapsed;
        if ((sint32)elapsed >= 9)
            td5_8005a3d0();
    }
    DrawSync(0);
    VSync(0);
    td5_80054260();
}

uint32 td5_8004ac1c(uint32 incoming_v0)
{
    FUNCTION_MARKER(0x8004AC1C, "SLUS_006.10");
    return incoming_v0;
}

uint32 td5_8004ac24(uint32 archive_error_index)
{
    uint32 selected, random, filename, length, index, source, destination;
    uint32 values[8];

    FUNCTION_MARKER(0x8004AC24, "SLUS_006.10");
    selected = r_u32(0x80087614);
    random = (uint32)randPSX();
    filename = r_u32(0x8007E50C + selected * 4);
    w_u32(0x80087614, random & 1);
    length = 0;
    while (r_u8(filename + length) != 0)
        ++length;
    td5_load_archive_file((const char *)psx_addr(filename, length + 1), 0x800A7A00, archive_error_index);
    for (index = 0; index < 8; ++index)
        values[index] = r_u32(0x800A7A00 + index * 4);
    w_u32(0x80088718, values[0]);
    w_u32(0x8008851C, values[1]);
    w_u32(0x80088780, values[2]);
    w_u32(0x80087710, values[3]);
    w_u32(0x800882B8, 0x8008F5E0 + values[4]);
    w_u32(0x800882BC, 0x8008F5E0 + values[5]);
    w_u32(0x800882C0, 0x8008F5E0 + values[6]);
    w_u32(0x800882C4, 0x8008F5E0 + values[7]);
    source = 0x800A7A20;
    destination = 0x8008CF70;
    do
    {
        for (index = 0; index < 4; ++index)
            values[index] = r_u32(source + index * 4);
        for (index = 0; index < 4; ++index)
            w_u32(destination + index * 4, values[index]);
        source += 16;
        destination += 16;
    } while (source != 0x800A7AB0);
    for (index = 0; index < 3584; ++index)
    {
        values[0] = r_u16(0x800A7AB0 + index * 2);
        w_u16(0x8008F5E0 + index * 2, values[0]);
    }
    filename = r_u32(0x8007E50C + selected * 4);
    td5_debug_log(0x800110AC, filename);
    return selected * 4;
}

uint32 td5_8004d728(uint32 slot)
{
    uint32 car_offset = slot * 864;
    uint32 first_value, second_value, first_distance, second_distance;
    uint32 table, kind, offset, current, first_abs, second_abs;
    sint32 node, segment;

    FUNCTION_MARKER(0x8004D728, "SLUS_006.10");
    node = r_s16(0x80098510 + car_offset);
    first_value = td5_8004da54(node, 0x80098664 + car_offset);
    segment = r_s16(0x80098512 + car_offset);
    table = r_u32(0x800886B8);
    node = r_s16(0x80098510 + car_offset);
    kind = r_u8(table + (uint32)segment * 3);
    first_distance = td5_8004dbe8(node, first_value, kind);
    node = r_s16(0x80098510 + car_offset);
    second_value = td5_8004da54(node, 0x80098664 + car_offset);
    segment = r_s16(0x80098512 + car_offset);
    table = r_u32(0x800887A8);
    node = r_s16(0x80098510 + car_offset);
    kind = r_u8(table + (uint32)segment * 3);
    second_distance = td5_8004dbe8(node, second_value, kind);
    first_abs = (sint32)first_distance < 0 ? 0 - first_distance : first_distance;
    second_abs = (sint32)second_distance < 0 ? 0 - second_distance : second_distance;
    offset = slot * 284;
    if ((sint32)first_abs < (sint32)second_abs)
    {
        current = r_u32(0x80097750 + offset);
        table = r_u32(0x800886B8);
        if (current != table)
        {
            w_u32(0x80097750 + offset, table);
            w_u32(0x800977B4 + offset, first_value);
            w_u32(0x80097774 + offset, first_distance);
            return current;
        }
    }
    if ((sint32)second_abs < (sint32)first_abs)
    {
        current = r_u32(0x80097750 + offset);
        table = r_u32(0x800887A8);
        if (current != table)
        {
            w_u32(0x80097750 + offset, table);
            w_u32(0x800977B4 + offset, second_value);
            w_u32(0x80097774 + offset, second_distance);
        }
        return current;
    }
    return slot << 3;
}

uint32 td5_8004e46c(void)
{
    uint32 first, second, count, result;
    sint32 segment, player_segment;

    FUNCTION_MARKER(0x8004E46C, "SLUS_006.10");
    first = td5_8004d19c(0x80097750);
    second = td5_8004d19c(0x8009814C);
    count = r_u32(0x800977B0);
    w_u8(0x8009A645, 1);
    w_u16(0x8009A616, 0xFF00);
    if ((sint32)count < 9 || first - 1024 < 2049 || second - 1024 < 2049)
    {
        result = r_u8(0x800987FC);
        w_u32(0x800981C8, 0xFFFFFFFF);
        w_u32(0x800977D0, 0);
        w_u32(0x8009784C, 0);
        w_u32(0x8009823C, 300);
        w_u32(0x800981B4, 0);
        w_u32(0x800981D4, 0);
        w_u16(0x8009A616, 0);
        w_u8(0x8009A645, 0);
        ++result;
        w_u8(0x800987FC, result);
        return result;
    }
    result = (sint32)count < r_s32(0x800981AC);
    if (result == 0)
    {
        segment = r_s16(0x8009A372);
        player_segment = r_s16(0x80098512);
        result = (sint32)((uint32)segment - (uint32)player_segment) < 3;
        if (result != 0)
        {
            w_u8(0x800987E5, 1);
            w_u16(0x800987B6, 0xFF00);
            return result;
        }
    }
    w_u16(0x800987B6, 0);
    return result;
}

uint32 td5_80052c60(void)
{
    uint32 index, count, offset;

    FUNCTION_MARKER(0x80052C60, "SLUS_006.10");
    td5_play_positioned_sound(10, 0, 4096, 4096, 0x80098664, 0x80098634);
    td5_play_positioned_sound(10, 2, 4096, 4096, 0x800989C4, 0x80098994);
    count = r_u32(0x800875E4);
    index = 2;
    while ((sint32)count < 5 ? (sint32)index < (sint32)count : (sint32)index < 4)
    {
        offset = index * 864;
        td5_play_positioned_sound(11, (sint32)(index * 2), 4096, 4096, 0x80098664 + offset, 0x80098634 + offset);
        count = r_u32(0x800875E4);
        ++index;
    }
    for (index = 0; index < 21; ++index)
        td5_80054674((sint32)index);
    return 0;
}

uint32 td5_80053124(void)
{
    FUNCTION_MARKER(0x80053124, "SLUS_006.10");
    return (uint32)td5_8005490c(1, 23, 4096, 0, 1024);
}

uint32 td5_80054324(void)
{
    SpuReverbAttr attribute = {0};

    FUNCTION_MARKER(0x80054324, "SLUS_006.10");
    attribute.mask = 7;
    attribute.mode = 0x105;
    attribute.depth.left = 0x1FFF;
    attribute.depth.right = 0x1FFF;
    SpuSetReverbModeParam(&attribute);
    SpuSetReverbDepth(&attribute);
    SpuSetReverbVoice(1, 0);
    SpuSetReverb(1);
    return (uint32)SpuReserveReverbWorkArea(1);
}

uint32 td5_80054384(void)
{
    FUNCTION_MARKER(0x80054384, "SLUS_006.10");
    SpuSetReverbVoice(1, 0xFFFFFF);
    return (uint32)SpuSetReverb(1);
}

uint32 td5_800543b4(void)
{
    FUNCTION_MARKER(0x800543B4, "SLUS_006.10");
    SpuSetReverbVoice(1, 0);
    return (uint32)SpuSetReverb(0);
}

uint32 td5_80055728(void)
{
    uint8 mode = 0xCC;

    FUNCTION_MARKER(0x80055728, "SLUS_006.10");
    return (uint32)CdControlB(14, &mode, 0);
}

uint32 td5_80055758(uint32 sector, uint32 length, uint32 channel)
{
    uint8 filter[2];
    CdlLOC position;

    FUNCTION_MARKER(0x80055758, "SLUS_006.10");
    w_u32(0x800882EC, length);
    w_u32(0x800882E8, sector);
    w_u32(0x800882F4, sector);
    w_u32(0x800882F0, channel);
    filter[0] = 1;
    filter[1] = (uint8)channel;
    CdControlF(13, filter);
    CdIntToPos(r_s32(0x800882E8), &position);
    return (uint32)CdControlF(27, (uint8 *)&position);
}

uint32 td5_80055a38(sint32 track)
{
    FUNCTION_MARKER(0x80055A38, "SLUS_006.10");
    return (uint32)td5_play_xa(track);
}

uint32 td5_80055a58(void)
{
    FUNCTION_MARKER(0x80055A58, "SLUS_006.10");
    return (uint32)CdControl(11, 0, 0);
}

uint32 td5_80055a80(void)
{
    uint32 template_word;
    sint32 volume;
    CdlATV mix;

    FUNCTION_MARKER(0x80055A80, "SLUS_006.10");
    CdControl(12, 0, 0);
    volume = r_s32(0x80087700);
    template_word = r_u32(0x80087BEC);
    template_word = r_u32(0x80087BEC);
    volume /= 16;
    if (volume < 0)
        volume = 0;
    else if (volume >= 256)
        volume = 255;
    mix.val0 = (uint8)volume;
    mix.val1 = (uint8)(template_word >> 8);
    mix.val2 = (uint8)volume;
    mix.val3 = (uint8)(template_word >> 24);
    CdMix(&mix);
    return 1;
}

uint32 td5_80055efc(void)
{
    uint32 index;

    FUNCTION_MARKER(0x80055EFC, "SLUS_006.10");
    for (index = 0; index < 8; ++index)
        CloseEventPSX(r_u32(0x800882F8 + index * 4));
    PadStopCom();
    return td5_80068144();
}

uint32 td5_80055f88(uint32 event)
{
    FUNCTION_MARKER(0x80055F88, "SLUS_006.10");
    TestEvent((sint32)event);
    TestEvent(r_s32(0x8008830C));
    TestEvent(r_s32(0x80088310));
    return (uint32)TestEvent(r_s32(0x80088314));
}

uint32 td5_80055fd0(uint32 event)
{
    FUNCTION_MARKER(0x80055FD0, "SLUS_006.10");
    TestEvent((sint32)event);
    TestEvent(r_s32(0x800882FC));
    TestEvent(r_s32(0x80088300));
    return (uint32)TestEvent(r_s32(0x80088304));
}

uint32 td5_80056014(void)
{
    uint32 index;

    FUNCTION_MARKER(0x80056014, "SLUS_006.10");
    for (;;)
        for (index = 0; index < 4; ++index)
            if (TestEvent(r_s32(0x800882F8 + index * 4)) == 1)
                return index;
}

uint32 td5_80056088(void)
{
    uint32 index;

    FUNCTION_MARKER(0x80056088, "SLUS_006.10");
    for (;;)
        for (index = 0; index < 4; ++index)
            if (TestEvent(r_s32(0x80088308 + index * 4)) == 1)
                return index;
}

uint32 td5_80056158(void)
{
    uint32 index;

    FUNCTION_MARKER(0x80056158, "SLUS_006.10");
    for (;;)
        for (index = 0; index < 4; ++index)
            if (TestEvent(r_s32(0x800882F8 + index * 4)) == 1)
                return index;
}

uint32 td5_800561cc(uint32 slot)
{
    uint32 index;

    FUNCTION_MARKER(0x800561CC, "SLUS_006.10");
    for (index = 0; index < 4; ++index)
        TestEvent(r_s32(0x800882F8 + index * 4));
    return psx_bios_card_load_guest(slot << 4);
}

uint32 td5_80056228(uint32 slot)
{
    uint32 index;

    FUNCTION_MARKER(0x80056228, "SLUS_006.10");
    _card_info((sint32)(slot << 4));
    for (;;)
        for (index = 0; index < 4; ++index)
            if (TestEvent(r_s32(0x800882F8 + index * 4)) == 1)
                return index;
}

uint32 td5_800562a4(uint32 filename, sint32 slot, uint32 original_stack)
{
    uint32 buffer = original_stack - 0x38 + 0x10;
    uint32 length = 0;
    const char *name, *format_string;
    char *path;
    sint32 bytes;

    FUNCTION_MARKER(0x800562A4, "SLUS_006.10");
    while (r_u8(filename + length) != 0)
        ++length;
    name = (const char *)psx_addr(filename, length + 1);
    format_string = (const char *)psx_addr(0x80011430, 9);
    bytes = snprintf(0, 0, format_string, slot, name);
    path = (char *)psx_addr(buffer, (uint32)bytes + 1);
    sprintf(path, format_string, slot, name);
    return erase(path) != 0;
}

uint32 td5_80056614(uint32 destination, uint32 slot)
{
    FUNCTION_MARKER(0x80056614, "SLUS_006.10");
    return sprintfPSX(destination, 0x8001143C, slot);
}
