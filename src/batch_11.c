#include "td5_functions.h"

uint32 td5_8002f384(uint32 object, uint32 mapping, uint32 distance)
{
    static const uint32 normal_commands[4] = {0x486012, 0x48E012, 0x496012, 0x486012};
    uint32 value, product, first_scale, second_scale, normal_scale;
    uint32 shift, other_shift, texture, texture_u, texture_v, stream, vertex_base;
    uint32 buffer, ordering, result, kind, packet, vertex_addresses[3];
    uint32 vertex, word_index, corner, address, normals, bucket, tag;
    uint32 normal_product, y_product, coordinate_u, coordinate_v;
    sint32 projection, multiplier, quotient_x, quotient_y;
    sint32 center_x, center_y, texture_bias, far_plane, area, second_area, depth;
    sint32 count, index, pixel_x, pixel_y;
    sint16 normal_x, normal_y;

    FUNCTION_MARKER(0x8002F384, "SLUS_006.10");
    value = r_u32(mapping + 4);
    product = value * distance;
    projection = r_s32(0x80088144);
    quotient_x = td5_divide_checked((sint32)product, projection);
    value = r_u32(mapping + 8);
    quotient_y = td5_divide_checked((sint32)(value * distance), projection);
    multiplier = r_s32(0x80088160);
    first_scale = (uint32)td5_divide_checked((sint32)((uint32)quotient_x << 12), multiplier);
    second_scale = (uint32)td5_divide_checked((sint32)((uint32)quotient_y << 12), multiplier);
    texture_bias = r_s16(mapping + 16);
    center_x = r_s16(mapping);
    center_y = r_s16(mapping + 2);
    shift = r_u32(0x80088110);
    stream = object + r_u32(object + 0x30);
    value = r_u32(mapping + 12);
    texture = r_u32(0x800884F0);
    far_plane = r_s32(0x80088140);
    other_shift = r_u32(0x80088164);
    texture_u = r_u8(texture + 14);
    texture_v = r_u8(texture + 15);
    vertex_base = object + r_u32(object + 0x2C);
    shift = (shift - other_shift) & 31;
    buffer = r_u32(0x800880F0);
    quotient_x = r_s16(object + 12);
    far_plane >>= (other_shift + 2) & 31;
    ordering = r_u32(buffer + 0x70);
    kind = r_u8(stream + 3);
    result = (uint32)(quotient_x >> shift) << 2;
    ordering += result;
    normal_scale = (uint32)td5_divide_checked((sint32)(value << 12), multiplier);
    if (kind == 0)
        return result;
    do
    {
        kind = r_u8(stream + 2);
        count = r_s16(stream);
        stream += 4;
        if (kind <= 1)
            stream += (uint32)count * 20;
        else if (kind == 2 || kind == 3)
        {
            value = r_u32(0x80088104);
            packet = r_u32((kind == 2 ? 0x800881F4 : 0x800881FC) + value * 4);
            for (index = 0; index < count; ++index)
            {
                for (vertex = 0; vertex < 3; ++vertex)
                {
                    value = (uint32)r_s16(stream + vertex * 2);
                    vertex_addresses[vertex] = vertex_base + (value << 3);
                }
                for (vertex = 0; vertex < 3; ++vertex)
                {
                    value = r_u32(vertex_addresses[vertex]);
                    xport_gte_write_data(vertex * 2, value);
                    value = r_u32(vertex_addresses[vertex] + 4);
                    xport_gte_write_data(vertex * 2 + 1, value);
                }
                xport_gte_execute(0x280030);
                xport_gte_execute(0x1400006);
                area = (sint32)xport_gte_read_data(24);
                second_area = -1;
                if (kind == 3)
                {
                    w_u32(packet + 8, xport_gte_read_data(12));
                    w_u32(packet + 16, xport_gte_read_data(13));
                    w_u32(packet + 24, xport_gte_read_data(14));
                    value = (uint32)r_s16(stream + 6);
                    address = vertex_base + (value << 3);
                    value = r_u32(address);
                    xport_gte_write_data(0, value);
                    value = r_u32(address + 4);
                    xport_gte_write_data(1, value);
                    xport_gte_execute(0x180001);
                    xport_gte_execute(0x1400006);
                    second_area = (sint32)xport_gte_read_data(24);
                    xport_gte_execute(0x168002E);
                }
                else
                    xport_gte_execute(0x158002D);
                if (area <= 0 || (kind == 3 && second_area >= 0))
                {
                    depth = (sint32)xport_gte_read_data(7);
                    if (depth < far_plane)
                    {
                        if (kind == 3)
                            w_u32(packet + 32, xport_gte_read_data(14));
                        else
                        {
                            w_u32(packet + 8, xport_gte_read_data(12));
                            w_u32(packet + 16, xport_gte_read_data(13));
                            w_u32(packet + 24, xport_gte_read_data(14));
                        }
                        bucket = ordering + ((uint32)(depth >> shift) << 2);
                        tag = r_u32(packet);
                        value = r_u32(bucket);
                        w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
                        value = r_u32(bucket);
                        w_u32(bucket, (value & 0xFF000000) | (packet & 0xFFFFFF));
                        normals = stream + (kind == 2 ? 20 : 24);
                        for (word_index = 0; word_index < 6; ++word_index)
                        {
                            value = r_u32(normals + word_index * 4);
                            xport_gte_write_data(word_index, value);
                        }
                        for (corner = 0; corner < (kind == 2 ? 3u : 4u); ++corner)
                        {
                            if (corner == 3)
                            {
                                value = r_u32(stream + 48);
                                xport_gte_write_data(0, value);
                                value = r_u32(stream + 52);
                                xport_gte_write_data(1, value);
                            }
                            xport_gte_execute(normal_commands[corner]);
                            normal_x = (sint16)xport_gte_read_data(9);
                            normal_y = (sint16)xport_gte_read_data(10);
                            xport_gte_read_data(11);
                            pixel_x = r_s16(packet + 8 + corner * 8);
                            product = ((uint32)pixel_x - (uint32)center_x) * first_scale;
                            normal_product = (uint32)(sint32)normal_x * normal_scale;
                            coordinate_u = (uint32)((sint32)product / 4096);
                            coordinate_u += (uint32)((sint32)normal_product / 4096) + texture_u + (uint32)texture_bias;
                            if (corner < 2)
                            {
                                pixel_y = r_s16(packet + 10 + corner * 8);
                                y_product = ((uint32)pixel_y - (uint32)center_y) * second_scale;
                                w_u8(packet + 12 + corner * 8, coordinate_u);
                            }
                            else
                            {
                                w_u8(packet + 12 + corner * 8, coordinate_u);
                                pixel_y = r_s16(packet + 10 + corner * 8);
                                y_product = ((uint32)pixel_y - (uint32)center_y) * second_scale;
                            }
                            normal_product = (uint32)(sint32)normal_y * normal_scale;
                            coordinate_v = (uint32)((sint32)y_product / 4096);
                            coordinate_v += (uint32)((sint32)normal_product / 4096) + texture_v + (uint32)texture_bias;
                            w_u8(packet + 13 + corner * 8, coordinate_v);
                        }
                        value = r_u32(0x80087838);
                        w_u32(packet + 4, value | (kind == 2 ? 0x26000000 : 0x2E000000));
                    }
                }
                packet += kind == 2 ? 32 : 40;
                stream += kind == 2 ? 44 : 56;
            }
        }
        result = r_u8(stream + 3);
    } while (result != 0);
    return result;
}

uint32 td5_80040060(uint32 slot, uint32 original_stack)
{
    uint32 car = 0x800984A0 + slot * 864;
    uint32 frame = original_stack - 0x68;
    uint32 active = 0, color = 32, fade, flags, value, level;
    uint32 stage, index, packet, source, address, word, words[2];
    uint32 high, low, shift, translation[3], vertex, stride, geometry;
    uint32 buffer, tag, bucket, offset, ordering;
    sint32 intensity = -64, depth;

    FUNCTION_MARKER(0x80040060, "SLUS_006.10");
    if (slot == 0 && (r_u16(0x800885DC) & 0x1000) == 0)
        active = r_u8(car + 0x345) != 0;
    if (slot == 1 && r_u32(0x8008772C) != 0 && (r_u16(0x800885DE) & 0x1000) == 0 && r_u8(car + 0x345) != 0)
        active = 1;
    if (active == 0)
    {
        level = r_u32(0x80088770);
        level = r_u32(level);
        if ((r_u8(level + 30) & 3) == 0)
            return 0;
    }
    level = r_u32(0x80088770);
    level = r_u32(level);
    flags = r_u8(level + 30);
    if ((flags & 1) != 0)
    {
        color = 96;
        intensity = 80;
    }
    else if ((flags & 2) != 0)
    {
        color = 48;
        intensity = 32;
    }
    if (active != 0)
    {
        color += 48;
        intensity += 48;
    }
    if ((r_u8(car + 0x354) & 15) != 0)
    {
        value = r_u8(0x8009D3F8 + slot);
        if (value < 128)
            w_u8(0x8009D3F8 + slot, value + 8);
    }
    else
    {
        value = r_u8(0x8009D3F8 + slot);
        if (value != 0)
            w_u8(0x8009D3F8 + slot, value >> 1);
    }
    value = r_u8(0x8009D3F8 + slot);
    fade = (uint32)intensity - value;
    if ((sint16)fade < 0)
        fade = 0;
    else if ((sint16)fade > 128)
        fade = 128;
    td5_8001cd00(car + 0x70, car + 0x1C4, frame + 0x20);
    for (stage = 0; stage < 2; ++stage)
    {
        if (stage == 1 && (fade & 0xFFFF) == 0)
            break;
        stride = stage == 0 ? 8 : 12;
        geometry = stage == 0 ? 0x8007E2F4 : 0x8007E314;
        for (index = 0; index < 2; ++index)
        {
            value = r_u32(0x80088104);
            offset = slot * 4 + index * 2 + value;
            packet = r_u32(stage == 0 ? 0x80088410 : 0x80088538);
            packet += offset * (stage == 0 ? 40 : 52);
            w_u8(packet + 4, stage == 0 ? color : 0);
            w_u8(packet + 5, stage == 0 ? color : 0);
            w_u8(packet + 6, stage == 0 ? color : 0);
            if (stage != 0)
            {
                w_u8(packet + 16, 0);
                w_u8(packet + 17, 0);
                w_u8(packet + 18, 0);
                w_u8(packet + 28, fade);
                w_u8(packet + 29, fade);
                w_u8(packet + 30, fade);
                w_u8(packet + 40, fade);
                w_u8(packet + 41, fade);
                w_u8(packet + 42, fade);
            }
            PushMatrix();
            source = r_u32(car + 0x180) + 96 + index * 8;
            if (stage == 0)
            {
                value = r_u32(source);
                xport_gte_write_data(0, value);
                value = r_u32(source + 4);
                xport_gte_write_data(1, value);
            }
            else
            {
                for (word = 0; word < 2; ++word)
                {
                    address = source + word * 4;
                    high = r_u32((address + 3) & ~3);
                    low = r_u32(address & ~3);
                    shift = (address & 3) * 8;
                    words[word] = shift == 0 ? low : (high << (32 - shift)) | (low >> shift);
                }
                value = r_u32(frame + 0x20);
                low = r_u32(car + 0x1C8);
                address = r_u32(car + 0x180) + index * 8;
                high = r_u16(address + 98);
                low = (uint32)((sint32)(value - low) >> 8);
                value = r_u16(car + ((index ^ 1) + 2) * 8 + 0x1DA);
                value = low + value - (high - 120);
                words[0] = (words[0] & 0xFFFF) | (value << 16);
                xport_gte_write_data(0, words[0]);
                xport_gte_write_data(1, words[1]);
            }
            xport_gte_execute(0x480012);
            translation[0] = xport_gte_read_data(25);
            translation[1] = xport_gte_read_data(26);
            translation[2] = xport_gte_read_data(27);
            SetRotMatrix((MATRIX *)psx_addr(0x8009FCF8, 20));
            for (word = 0; word < 3; ++word)
                xport_gte_write_control(5 + word, translation[word]);
            for (vertex = 0; vertex < 3; ++vertex)
            {
                value = r_u32(geometry + vertex * 8);
                xport_gte_write_data(vertex * 2, value);
                value = r_u32(geometry + vertex * 8 + 4);
                xport_gte_write_data(vertex * 2 + 1, value);
            }
            xport_gte_execute(0x280030);
            w_u32(packet + 8, xport_gte_read_data(12));
            w_u32(packet + 8 + stride, xport_gte_read_data(13));
            w_u32(packet + 8 + stride * 2, xport_gte_read_data(14));
            flags = xport_gte_read_control(31);
            if ((flags & 0x1C66000) == 0)
            {
                value = r_u32(geometry + 24);
                xport_gte_write_data(0, value);
                value = r_u32(geometry + 28);
                xport_gte_write_data(1, value);
                xport_gte_execute(0x180001);
                w_u32(packet + 8 + stride * 3, xport_gte_read_data(14));
                xport_gte_execute(0x168002E);
                depth = (sint32)(xport_gte_read_data(7) - 16);
                if (depth < 0)
                    depth = 0;
                buffer = r_u32(0x800880F0);
                shift = r_u32(0x80088110);
                shift = (shift - r_u32(0x80088164)) & 31;
                tag = r_u32(packet);
                ordering = r_u32(buffer + 0x70);
                offset = (uint32)(depth >> shift) << 2;
                value = r_u32(ordering + offset);
                w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
                ordering = r_u32(buffer + 0x70);
                bucket = ordering + offset;
                value = r_u32(bucket);
                w_u32(bucket, (value & 0xFF000000) | (packet & 0xFFFFFF));
            }
            PopMatrix();
        }
    }
    return 0;
}

uint32 td5_8004099c(uint32 slot, uint32 original_stack)
{
    uint32 level, car = 0x800984A0 + slot * 864;
    uint32 frame = original_stack - 0x60;
    uint32 stage, index, packet, source, address, word, words[2];
    uint32 high, low, shift, translation[3], vertex, stride, geometry;
    uint32 value, flags, buffer, tag, bucket, offset, ordering;
    sint32 area, depth;

    FUNCTION_MARKER(0x8004099C, "SLUS_006.10");
    level = r_u32(0x80088770);
    level = r_u32(level);
    if ((r_u8(level + 30) & 1) == 0)
        return slot * 864;
    td5_8001cd00(car + 0x70, car + 0x1C4, frame + 0x20);
    for (stage = 0; stage < 2; ++stage)
    {
        stride = stage == 0 ? 8 : 12;
        geometry = stage == 0 ? 0x8007E334 : 0x8007E354;
        for (index = 0; index < 2; ++index)
        {
            value = r_u32(0x80088104);
            offset = slot * 4 + index * 2 + value;
            packet = r_u32(stage == 0 ? 0x8008876C : 0x8008843C);
            packet += offset * (stage == 0 ? 40 : 52);
            PushMatrix();
            source = r_u32(car + 0x180) + 112 + index * 8;
            if (stage == 0)
            {
                value = r_u32(source);
                xport_gte_write_data(0, value);
                value = r_u32(source + 4);
                xport_gte_write_data(1, value);
            }
            else
            {
                for (word = 0; word < 2; ++word)
                {
                    address = source + word * 4;
                    high = r_u32((address + 3) & ~3);
                    low = r_u32(address & ~3);
                    shift = (address & 3) * 8;
                    words[word] = shift == 0 ? low : (high << (32 - shift)) | (low >> shift);
                }
                value = r_u32(frame + 0x20);
                low = r_u32(car + 0x1C8);
                address = r_u32(car + 0x180) + index * 8;
                high = r_u16(address + 114);
                low = (uint32)((sint32)(value - low) >> 8);
                value = r_u16(car + ((index ^ 1) + 2) * 8 + 0x1DA);
                value = low + value - (high - 120);
                words[0] = (words[0] & 0xFFFF) | (value << 16);
                xport_gte_write_data(0, words[0]);
                xport_gte_write_data(1, words[1]);
            }
            xport_gte_execute(0x480012);
            translation[0] = xport_gte_read_data(25);
            translation[1] = xport_gte_read_data(26);
            translation[2] = xport_gte_read_data(27);
            for (word = 0; word < 3; ++word)
                xport_gte_write_control(5 + word, translation[word]);
            for (vertex = 0; vertex < 3; ++vertex)
            {
                value = r_u32(geometry + vertex * 8);
                xport_gte_write_data(vertex * 2, value);
                value = r_u32(geometry + vertex * 8 + 4);
                xport_gte_write_data(vertex * 2 + 1, value);
            }
            xport_gte_execute(0x280030);
            w_u32(packet + 8, xport_gte_read_data(12));
            w_u32(packet + 8 + stride, xport_gte_read_data(13));
            w_u32(packet + 8 + stride * 2, xport_gte_read_data(14));
            flags = xport_gte_read_control(31);
            if ((flags & 0x1C66000) == 0)
            {
                xport_gte_execute(0x1400006);
                area = (sint32)xport_gte_read_data(24);
                if (area >= 0)
                {
                    value = r_u32(geometry + 24);
                    xport_gte_write_data(0, value);
                    value = r_u32(geometry + 28);
                    xport_gte_write_data(1, value);
                    xport_gte_execute(0x180001);
                    w_u32(packet + 8 + stride * 3, xport_gte_read_data(14));
                    xport_gte_execute(0x168002E);
                    depth = (sint32)(xport_gte_read_data(7) - 16);
                    if (depth < 0)
                        depth = 0;
                    buffer = r_u32(0x800880F0);
                    shift = r_u32(0x80088110);
                    shift = (shift - r_u32(0x80088164)) & 31;
                    tag = r_u32(packet);
                    ordering = r_u32(buffer + 0x70);
                    offset = (uint32)(depth >> shift) << 2;
                    value = r_u32(ordering + offset);
                    w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
                    ordering = r_u32(buffer + 0x70);
                    bucket = ordering + offset;
                    value = r_u32(bucket);
                    w_u32(bucket, (value & 0xFF000000) | (packet & 0xFFFFFF));
                }
            }
            PopMatrix();
        }
    }
    return 0;
}

uint32 td5_80041114(uint32 slot)
{
    uint32 car, level, color, animation, index, packet, source;
    uint32 value, other, address, shift, high, low, words[2], word;
    uint32 translation[3], vertex, flags, texture, buffer, tag, ordering, offset;
    sint32 intensity, random_value, depth;

    FUNCTION_MARKER(0x80041114, "SLUS_006.10");
    level = r_u32(0x80088770);
    level = r_u32(level);
    if ((r_u8(level + 30) & 2) == 0)
        return 0;
    car = 0x800984A0 + slot * 864;
    intensity = r_s32(car + 0x2EC) / 4096;
    if ((sint16)intensity < 0)
        intensity = 0;
    else if ((sint16)intensity > 24)
        intensity = 24;
    color = (uint32)intensity;
    if ((color << 16) == 0)
        return 0;
    random_value = randPSX();
    value = r_u32(0x80088108);
    animation = (uint32)((sint32)(value + (uint32)(random_value % 3)) >> 1) & 3;
    for (index = 0; index < 2; ++index)
    {
        value = r_u32(0x80088104);
        offset = slot * 4 + index * 2 + value;
        packet = r_u32(0x800885D4) + offset * 40;
        w_u8(packet + 4, color);
        w_u8(packet + 5, color);
        w_u8(packet + 6, color);
        PushMatrix();
        source = r_u32(car + 0x180) + index * 8 + 16;
        for (word = 0; word < 2; ++word)
        {
            address = source + word * 4;
            high = r_u32((address + 3) & ~3);
            low = r_u32(address & ~3);
            shift = (address & 3) * 8;
            words[word] = shift == 0 ? low : (high << (32 - shift)) | (low >> shift);
        }
        value = ((words[0] & 0xFFFF) + (index == 0 ? 72 : (uint32)-72)) & 0xFFFF;
        words[0] = value | ((words[0] + 0x1000000) & 0xFFFF0000);
        words[1] = (words[1] & 0xFFFF0000) | ((words[1] - 64) & 0xFFFF);
        xport_gte_write_data(0, words[0]);
        xport_gte_write_data(1, words[1]);
        xport_gte_execute(0x480012);
        translation[0] = xport_gte_read_data(25);
        translation[1] = xport_gte_read_data(26);
        translation[2] = xport_gte_read_data(27);
        SetRotMatrix((MATRIX *)psx_addr(0x8009FCF8, 20));
        for (word = 0; word < 3; ++word)
            xport_gte_write_control(5 + word, translation[word]);
        for (vertex = 0; vertex < 3; ++vertex)
        {
            value = r_u32(0x8007E374 + vertex * 8);
            xport_gte_write_data(vertex * 2, value);
            value = r_u32(0x8007E378 + vertex * 8);
            xport_gte_write_data(vertex * 2 + 1, value);
        }
        xport_gte_execute(0x280030);
        w_u32(packet + 8, xport_gte_read_data(12));
        w_u32(packet + 16, xport_gte_read_data(13));
        w_u32(packet + 24, xport_gte_read_data(14));
        flags = xport_gte_read_control(31);
        if ((flags & 0x1C66000) == 0)
        {
            value = r_u32(0x8007E38C);
            xport_gte_write_data(0, value);
            value = r_u32(0x8007E390);
            xport_gte_write_data(1, value);
            xport_gte_execute(0x180001);
            texture = 0x8009D7B0 + animation * 10;
            value = r_u8(texture);
            w_u8(packet + 12, value);
            value = r_u8(texture + 1);
            w_u8(packet + 13, value);
            value = r_u8(texture);
            other = r_u8(texture + 2);
            w_u8(packet + 20, other + value + 255);
            value = r_u8(texture + 1);
            w_u8(packet + 21, value);
            value = r_u8(texture);
            w_u8(packet + 28, value);
            value = r_u8(texture + 1);
            other = r_u8(texture + 3);
            w_u8(packet + 29, other + value + 255);
            value = r_u8(texture);
            other = r_u8(texture + 2);
            w_u8(packet + 36, other + value + 255);
            value = r_u8(texture + 1);
            other = r_u8(texture + 3);
            w_u8(packet + 37, other + value + 255);
            value = r_u16(texture + 4);
            w_u16(packet + 22, value);
            value = r_u16(texture + 8);
            w_u16(packet + 14, value);
            w_u32(packet + 32, xport_gte_read_data(14));
            xport_gte_execute(0x168002E);
            depth = (sint32)xport_gte_read_data(7);
            buffer = r_u32(0x800880F0);
            shift = r_u32(0x80088110);
            shift = (shift - r_u32(0x80088164)) & 31;
            tag = r_u32(packet);
            ordering = r_u32(buffer + 0x70);
            offset = (uint32)(depth >> shift) << 2;
            value = r_u32(ordering + offset);
            w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
            ordering = r_u32(buffer + 0x70);
            value = r_u32(ordering + offset);
            w_u32(ordering + offset, (value & 0xFF000000) | (packet & 0xFFFFFF));
        }
        PopMatrix();
    }
    return 0;
}

uint32 td5_80040ea4(uint32 car, uint32 original_stack)
{
    uint32 frame = original_stack - 0x28;
    uint32 first, second, result;
    sint32 strength, denominator, quotient, remainder;

    FUNCTION_MARKER(0x80040EA4, "SLUS_006.10");
    strength = r_s32(car + 0x2E8);
    if (strength >= 4000)
        return 0;
    if (r_s16(car + 0x316) < 201)
        return 1;
    if (strength <= 0)
        return 0;
    result = (uint32)randPSX();
    denominator = r_s32(car + 0x2E8);
    quotient = td5_divide_checked((sint32)result, denominator);
    remainder = (sint32)(result - (uint32)quotient * (uint32)denominator);
    if (remainder >= 500)
        return result;
    first = r_u32(car + 0x9C);
    second = r_u32(car + 0xA8);
    first = (uint32)((sint32)(first + second) / 2) + 30720;
    w_u32(frame + 0x14, first);
    first = r_u32(car + 0x98);
    second = r_u32(car + 0xA4);
    w_u32(frame + 0x10, (uint32)((sint32)(first + second) / 2));
    first = r_u32(car + 0xA0);
    second = r_u32(car + 0xAC);
    w_u32(frame + 0x18, (uint32)((sint32)(first + second) / 2));
    return td5_8003d708(car, frame + 0x10, 0);
}

uint32 td5_80033610(uint32 slot)
{
    uint32 car = 0x800984A0 + slot * 864;
    uint32 model, value, product, wheel, pair, vertex, word, face;
    uint32 translation[4][3], packet, texture, buffer, ordering, tag, offset;
    uint32 flags, shift, bucket, current_buffer;
    sint32 angle_a, angle_b, scale, width, radii[4], angle, speed;
    sint32 area, depth, limit, depth_index;
    sint16 points[4][3];
    MATRIX turn = {0};

    FUNCTION_MARKER(0x80033610, "SLUS_006.10");
    angle_a = -4 * r_s16(car + 0x31A);
    angle_b = -4 * r_s16(car + 0x318);
    model = r_u32(car + 0x180);
    scale = r_s16(model + 130);
    width = r_s16(model + 132);
    product = (uint32)rcos(angle_a) * (uint32)scale;
    radii[0] = (sint32)product / 4096;
    product = (uint32)rsin(angle_a) * (uint32)scale;
    radii[2] = (sint32)product / 4096;
    product = (uint32)rcos(angle_b) * (uint32)scale;
    radii[1] = (sint32)product / 4096;
    product = (uint32)rsin(angle_b) * (uint32)scale;
    radii[3] = (sint32)product / 4096;
    for (wheel = 0; wheel < 4; ++wheel)
    {
        value = r_u32(car + 0x1D8 + wheel * 8);
        xport_gte_write_data(0, value);
        value = r_u32(car + 0x1DC + wheel * 8);
        xport_gte_write_data(1, value);
        xport_gte_execute(0x480012);
        translation[wheel][0] = xport_gte_read_data(25);
        translation[wheel][1] = xport_gte_read_data(26);
        translation[wheel][2] = xport_gte_read_data(27);
    }
    for (wheel = 0; wheel < 4; ++wheel)
    {
        pair = wheel >> 1;
        if (wheel == 2)
        {
            angle = r_s32(car + 0x2E4) / 256;
            turn.m[0][0] = rcos(angle);
            turn.m[0][1] = 0;
            turn.m[0][2] = rsin(angle);
            turn.m[1][0] = 0;
            turn.m[1][1] = 4096;
            turn.m[1][2] = 0;
            turn.m[2][0] = -rsin(angle);
            turn.m[2][1] = 0;
            turn.m[2][2] = rcos(angle);
            MulMatrix2((MATRIX *)psx_addr(0x80092210, 20), &turn);
            SetRotMatrix(&turn);
        }
        for (word = 0; word < 3; ++word)
            xport_gte_write_control(5 + word, translation[(wheel + 2) & 3][word]);
        td5_80033454(slot, wheel);
        for (vertex = 0; vertex < 4; ++vertex)
            points[vertex][0] = (sint16)((wheel & 1) != 0 ? width : -width);
        points[0][1] = (sint16)radii[pair + 2];
        points[0][2] = (sint16)radii[pair];
        points[1][1] = (sint16)((wheel & 1) != 0 ? -radii[pair] : radii[pair]);
        points[1][2] = (sint16)((wheel & 1) != 0 ? radii[pair + 2] : -radii[pair + 2]);
        points[2][1] = (sint16)((wheel & 1) != 0 ? radii[pair] : -radii[pair]);
        points[2][2] = (sint16)((wheel & 1) != 0 ? -radii[pair + 2] : radii[pair + 2]);
        points[3][1] = (sint16)-radii[pair + 2];
        points[3][2] = (sint16)-radii[pair];
        current_buffer = r_u32(0x80088104);
        packet = 0x8009DD00 + current_buffer * 1920 + (wheel * 2 + slot * 8) * 40;
        speed = r_s32(car + (pair == 0 ? 0x2EC : 0x2F0)) >> 8;
        if (speed < 0)
            speed = -speed;
        speed /= 64;
        texture = speed < 4 ? 0x8008F5A8 + (uint32)(speed + 4) * 6 : 0x8008F5D2;
        if (slot != 0)
            texture -= 24;
        for (face = 0; face < 2; ++face)
        {
            if (face != 0)
            {
                current_buffer = r_u32(0x80088104);
                for (vertex = 0; vertex < 4; ++vertex)
                    points[vertex][0] = (sint16)-points[vertex][0];
                packet = 0x8009DD00 + current_buffer * 1920 + (wheel * 2 + slot * 8 + 1) * 40;
            }
            value = r_u8(texture + 4);
            w_u8(packet + 12, value);
            value = r_u8(texture + 5);
            w_u8(packet + 13, value);
            value = r_u8(texture + 4);
            w_u8(packet + 20, value + 31);
            value = r_u8(texture + 5);
            w_u8(packet + 21, value);
            value = r_u8(texture + 4);
            w_u8(packet + 28, value);
            value = r_u8(texture + 5);
            w_u8(packet + 29, value + 31);
            value = r_u8(texture + 4);
            w_u8(packet + 36, value + 31);
            value = r_u8(texture + 5);
            w_u8(packet + 37, value + 31);
            value = r_u16(texture);
            w_u16(packet + 22, value);
            value = r_u16(texture + 2);
            w_u16(packet + 14, value);
            for (vertex = 0; vertex < 3; ++vertex)
            {
                value = (uint16)points[vertex][0] | ((uint32)(uint16)points[vertex][1] << 16);
                xport_gte_write_data(vertex * 2, value);
                xport_gte_write_data(vertex * 2 + 1, (uint32)(sint32)points[vertex][2]);
            }
            xport_gte_execute(0x280030);
            w_u32(packet + 8, xport_gte_read_data(12));
            w_u32(packet + 16, xport_gte_read_data(13));
            w_u32(packet + 24, xport_gte_read_data(14));
            flags = xport_gte_read_control(31);
            if ((flags & 0x1C66000) != 0)
                continue;
            xport_gte_execute(0x1400006);
            area = (sint32)xport_gte_read_data(24);
            value = (uint16)points[3][0] | ((uint32)(uint16)points[3][1] << 16);
            xport_gte_write_data(0, value);
            xport_gte_write_data(1, (uint32)(sint32)points[3][2]);
            xport_gte_execute(0x180001);
            w_u32(packet + 32, xport_gte_read_data(14));
            xport_gte_execute(0x168002E);
            depth = (sint32)xport_gte_read_data(7);
            if (face != 0 && area <= 0)
                continue;
            if (face == 0 && area < 0)
            {
                shift = r_u32(0x80088110);
                shift = (shift - r_u32(0x80088164)) & 31;
                depth_index = (sint32)((uint32)depth - 8) >> shift;
                if (depth_index < 0)
                    depth_index = 0;
            }
            else
            {
                limit = (sint32)(r_u32(0x8008857C) - 1);
                shift = r_u32(0x80088110);
                shift = (shift - r_u32(0x80088164)) & 31;
                depth_index = (sint32)((uint32)depth + 8) >> shift;
                if (depth_index >= limit)
                    depth_index = limit;
            }
            buffer = r_u32(0x800880F0);
            ordering = r_u32(buffer + 0x70);
            tag = r_u32(packet);
            offset = (uint32)depth_index << 2;
            value = r_u32(ordering + offset);
            w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
            ordering = r_u32(buffer + 0x70);
            bucket = ordering + offset;
            value = r_u32(bucket);
            w_u32(bucket, (value & 0xFF000000) | (packet & 0xFFFFFF));
            if (face == 0)
            {
                if (area < 0)
                {
                    value = r_u32(0x8008785C);
                    xport_gte_write_data(6, value);
                    value = r_u32(0x8007E0B0 + wheel * 8);
                    xport_gte_write_data(0, value);
                    value = r_u32(0x8007E0B4 + wheel * 8);
                    xport_gte_write_data(1, value);
                    xport_gte_execute(0xE80413);
                    w_u32(packet + 4, xport_gte_read_data(22));
                }
                else
                    w_u32(packet + 4, 0x2C000000);
            }
        }
    }
    return 0;
}

uint32 td5_80033454(uint32 slot, uint32 wheel)
{
    uint32 source = 0x80091610 + slot * 144;
    uint32 shift, other_shift, current_buffer, packet, buffer, ordering;
    uint32 index, word, value, bucket, tag;
    sint32 far_plane, first_area, second_area, depth;

    FUNCTION_MARKER(0x80033454, "SLUS_006.10");
    shift = r_u32(0x80088110);
    other_shift = r_u32(0x80088164);
    far_plane = r_s32(0x80088140);
    current_buffer = r_u32(0x80088104);
    shift -= other_shift;
    far_plane >>= (shift + 2) & 31;
    packet = 0x8008D1A8 + current_buffer * 4608 + slot * 768 + wheel * 192;
    buffer = r_u32(0x800880F0);
    ordering = r_u32(buffer + 0x70) + 128;
    shift &= 31;
    for (index = 0; index < 8; ++index)
    {
        for (word = 0; word < 6; ++word)
        {
            value = r_u32(source + word * 4);
            xport_gte_write_data(word, value);
        }
        xport_gte_execute(0x280030);
        xport_gte_execute(0x1400006);
        first_area = (sint32)xport_gte_read_data(24);
        w_u32(packet + 8, xport_gte_read_data(12));
        w_u32(packet + 12, xport_gte_read_data(13));
        w_u32(packet + 16, xport_gte_read_data(14));
        value = r_u32(source + 24);
        xport_gte_write_data(0, value);
        value = r_u32(source + 28);
        xport_gte_write_data(1, value);
        xport_gte_execute(0x180001);
        xport_gte_execute(0x1400006);
        second_area = (sint32)xport_gte_read_data(24);
        xport_gte_execute(0x168002E);
        if (first_area <= 0 || second_area >= 0)
        {
            depth = (sint32)xport_gte_read_data(7);
            if (depth < far_plane && depth > 0)
            {
                w_u32(packet + 20, xport_gte_read_data(14));
                tag = r_u32(packet);
                bucket = ordering + ((uint32)(depth >> shift) << 2);
                value = r_u32(bucket);
                w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
                value = r_u32(bucket);
                w_u32(bucket, (value & 0xFF000000) | (packet & 0xFFFFFF));
            }
        }
        packet += 24;
        source += 16;
    }
    return 0;
}

uint32 td5_800321d0(uint32 slot, uint32 original_stack)
{
    uint32 car = 0x800984A0 + slot * 864;
    uint32 value, packet, current_buffer, vertex, axis, phase, first;
    uint32 buffer, ordering, tag, offset, shift, other_shift, result;
    sint32 world[4][3], center[3], depth, far_plane;
    sint16 points[6][3];
    VECTOR relative = {0}, transformed = {0};

    FUNCTION_MARKER(0x800321D0, "SLUS_006.10");
    current_buffer = r_u32(0x80088104);
    world[0][0] = r_s32(car + 0x80) >> 8;
    packet = r_u32(0x80088208 + current_buffer * 4) + slot * 80;
    for (axis = 1; axis < 3; ++axis)
        world[0][axis] = r_s32(car + 0x80 + axis * 4) >> 8;
    for (vertex = 1; vertex < 4; ++vertex)
        for (axis = 0; axis < 3; ++axis)
            world[vertex][axis] = r_s32(car + 0x80 + vertex * 12 + axis * 4) >> 8;
    for (axis = 0; axis < 3; ++axis)
    {
        value = (uint32)world[0][axis] + (uint32)world[1][axis];
        value += (uint32)world[2][axis] + (uint32)world[3][axis];
        center[axis] = (sint32)value >> 2;
        points[0][axis] = (sint16)((uint32)world[0][axis] - (uint32)center[axis]);
        points[1][axis] = (sint16)((uint32)world[1][axis] - (uint32)center[axis]);
        points[4][axis] = (sint16)((uint32)world[2][axis] - (uint32)center[axis]);
        points[5][axis] = (sint16)((uint32)world[3][axis] - (uint32)center[axis]);
        points[2][axis] = (sint16)(((sint32)points[0][axis] + points[4][axis]) / 2);
        points[3][axis] = (sint16)(((sint32)points[1][axis] + points[5][axis]) / 2);
    }
    value = r_u32(0x8009765C);
    relative.vx = (sint32)((uint32)center[0] - value);
    value = r_u32(0x80097660);
    relative.vy = (sint32)((uint32)center[1] - value);
    value = r_u32(0x80097664);
    relative.vz = (sint32)((uint32)center[2] - value);
    SetRotMatrix((MATRIX *)psx_addr(0x80097648, 20));
    ApplyMatrixLV((MATRIX *)psx_addr(0x80097648, 20), &relative, &transformed);
    w_u32(0x80092224, (uint32)transformed.vx);
    w_u32(0x80092228, (uint32)transformed.vy);
    w_u32(0x8009222C, (uint32)transformed.vz);
    value = r_u32(0x80092224);
    tag = r_u32(0x80092228);
    xport_gte_write_control(5, value);
    value = r_u32(0x8009222C);
    xport_gte_write_control(6, tag);
    xport_gte_write_control(7, value);
    result = original_stack - 0x18;
    for (phase = 0; phase < 2; ++phase)
    {
        first = phase * 2;
        for (vertex = 0; vertex < 3; ++vertex)
        {
            value = (uint16)points[first + vertex][0] | ((uint32)(uint16)points[first + vertex][1] << 16);
            xport_gte_write_data(vertex * 2, value);
            xport_gte_write_data(vertex * 2 + 1, (uint32)(sint32)points[first + vertex][2]);
        }
        xport_gte_execute(0x280030);
        w_u32(packet + 8, xport_gte_read_data(12));
        w_u32(packet + 16, xport_gte_read_data(13));
        w_u32(packet + 24, xport_gte_read_data(14));
        value = (uint16)points[first + 3][0] | ((uint32)(uint16)points[first + 3][1] << 16);
        xport_gte_write_data(0, value);
        xport_gte_write_data(1, (uint32)(sint32)points[first + 3][2]);
        xport_gte_execute(0x180001);
        w_u32(packet + 32, xport_gte_read_data(14));
        xport_gte_execute(0x168002E);
        depth = (sint32)xport_gte_read_data(7);
        if (phase != 0)
            result = original_stack - 0x18;
        if (depth > 0)
        {
            if (phase == 0)
                far_plane = r_s32(0x80088140) >> 2;
            else
            {
                shift = r_u32(0x80088110);
                far_plane = r_s32(0x80088140) >> ((shift + 2) & 31);
                result = depth < far_plane;
            }
            if (depth < far_plane)
            {
                buffer = r_u32(0x800880F0);
                if (phase == 0)
                    shift = r_u32(0x80088110);
                other_shift = r_u32(0x80088164);
                tag = r_u32(packet);
                ordering = r_u32(buffer + 0x70);
                offset = (uint32)(depth >> ((shift - other_shift) & 31)) << 2;
                value = r_u32(ordering + offset);
                w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
                ordering = r_u32(buffer + 0x70);
                value = r_u32(ordering + offset);
                value = (value & 0xFF000000) | (packet & 0xFFFFFF);
                w_u32(ordering + offset, value);
                if (phase != 0)
                    result = value;
            }
        }
        packet += 40;
    }
    return result;
}

uint32 td5_80043328(uint32 slot)
{
    uint32 selected, value, other, texture, packet, buffer, ordering, tag;
    uint32 x, y, mode, width, product, offset, bar, result;
    sint32 amount;

    FUNCTION_MARKER(0x80043328, "SLUS_006.10");
    if (r_u32(0x80087724) == 0 && r_u32(0x8008771C) != 3)
        return 3;
    selected = r_u32(0x800886D0);
    if (slot != selected)
        return selected;
    value = r_u32(0x80088104);
    texture = r_u32(0x800A3B78);
    packet = 0x8009D988 + value * 40;
    SetRotMatrix((MATRIX *)psx_addr(0x80092210, 20));
    value = r_u32(0x80092224);
    other = r_u32(0x80092228);
    xport_gte_write_control(5, value);
    value = r_u32(0x8009222C);
    xport_gte_write_control(6, other);
    xport_gte_write_control(7, value);
    value = r_u32(0x800879F4);
    xport_gte_write_data(0, value);
    value = r_u32(0x800879F8);
    xport_gte_write_data(1, value);
    xport_gte_execute(0x180001);
    value = xport_gte_read_data(14);
    x = value & 0xFFFF;
    y = ((value >> 16) - 16) & 0xFFFF;
    value = r_u8(texture + 14);
    w_u8(packet + 12, value);
    value = r_u8(texture + 15);
    other = r_u8(texture + 13);
    w_u8(packet + 13, value + other - 1);
    value = r_u8(texture + 14);
    other = r_u8(texture + 12);
    w_u8(packet + 20, value + other - 1);
    value = r_u8(texture + 15);
    other = r_u8(texture + 13);
    w_u8(packet + 21, value + other - 1);
    value = r_u8(texture + 14);
    w_u8(packet + 28, value);
    value = r_u8(texture + 15);
    w_u8(packet + 29, value);
    value = r_u8(texture + 14);
    other = r_u8(texture + 12);
    w_u8(packet + 36, value + other - 1);
    value = r_u8(texture + 15);
    w_u8(packet + 37, value);
    w_u16(packet + 8, x);
    value = r_u8(texture + 13);
    w_u16(packet + 10, y - value);
    value = r_u8(texture + 12);
    w_u16(packet + 16, x + value - 1);
    value = r_u8(texture + 13);
    w_u16(packet + 18, y - value);
    w_u16(packet + 24, x);
    w_u16(packet + 26, y - 1);
    value = r_u8(texture + 12);
    w_u16(packet + 32, x + value - 1);
    buffer = r_u32(0x800880F0);
    w_u16(packet + 34, y - 1);
    ordering = r_u32(buffer + 0x70);
    tag = r_u32(packet);
    value = r_u32(ordering + 16);
    w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
    ordering = r_u32(buffer + 0x70);
    value = r_u32(ordering + 16);
    mode = r_u32(0x8008771C);
    w_u32(ordering + 16, (value & 0xFF000000) | (packet & 0xFFFFFF));
    width = mode == 3 ? 7 : 10;
    value = (uint32)r_s16(0x8009DBA0 + slot * 2);
    other = r_u8(texture + 13);
    product = other * (4096 - value);
    amount = (sint32)product / 4096;
    offset = r_u32(0x80088104) * 36;
    for (bar = 0; bar < 2; ++bar)
    {
        value = r_u8(texture + 12);
        packet = (bar == 0 ? 0x800915C8 : 0x80091580) + offset;
        w_u16(packet + 8, x + value + 4);
        value = r_u8(texture + 13);
        w_u16(packet + 10, y - value);
        value = r_u8(texture + 12);
        w_u16(packet + 16, x + value + width);
        value = r_u8(texture + 13);
        w_u16(packet + 18, y - value);
        value = r_u8(texture + 12);
        w_u16(packet + 24, x + value + 4);
        if (bar == 0)
        {
            value = r_u8(texture + 13);
            w_u16(packet + 26, y - value + (uint32)amount);
        }
        else
            w_u16(packet + 26, y);
        value = r_u8(texture + 12);
        w_u16(packet + 32, x + value + width);
        if (bar == 0)
        {
            value = r_u8(texture + 13);
            w_u16(packet + 34, y - value + (uint32)amount);
        }
        else
            w_u16(packet + 34, y);
        ordering = r_u32(buffer + 0x70);
        tag = r_u32(packet);
        value = r_u32(ordering + 16);
        w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
        ordering = r_u32(buffer + 0x70);
        value = r_u32(ordering + 16);
        result = (value & 0xFF000000) | (packet & 0xFFFFFF);
        w_u32(ordering + 16, result);
    }
    return result;
}

uint32 td5_80053158(sint32 amount, uint32 slot)
{
    uint32 result, offset, duplicate, value, mirror;
    sint32 scaled, first, second;

    FUNCTION_MARKER(0x80053158, "SLUS_006.10");
    if (r_u32(0x8008772C) != 0)
    {
        if ((sint32)slot >= 2)
            return 0x55550000;
    }
    else if (slot != r_u32(0x80088498))
        return 0x55550000;
    scaled = amount / 3;
    first = scaled < 0 ? 0 : (scaled > 4096 ? 4096 : scaled);
    second = scaled < 3072 ? 3072 : (scaled > 4096 ? 4096 : scaled);
    result = slot * 2 + 8;
    mirror = r_u32(0x80088668);
    offset = result * 92;
    w_u32(0x800919B8 + offset, (uint32)first);
    w_u32(0x800919BC + offset, (uint32)second);
    if (mirror != 0)
    {
        duplicate = (slot * 2 + 9) * 92;
        result = duplicate;
        w_u32(0x800919BC + duplicate, (uint32)second);
        w_u32(0x800919B8 + duplicate, (uint32)first);
        value = r_u32(0x800919C4 + offset);
        w_u32(0x800919C4 + duplicate, value);
        value = r_u32(0x800919C8 + offset);
        w_u32(0x800919C8 + duplicate, value);
    }
    return result;
}

uint32 td5_80044a2c(uint32 car, sint32 amount, uint32 original_stack)
{
    uint32 frame = original_stack - 0x48;
    uint32 wheel, value, type, handle, descriptor, effect;

    FUNCTION_MARKER(0x80044A2C, "SLUS_006.10");
    for (wheel = 0; wheel < 2; ++wheel)
    {
        if (amount < 15001)
            goto cleanup_gate;
        if (r_u8(car + 0x351) != 0)
            goto remove_effect;
        if ((r_u8(car + 0x34E) & 2) != 0 || ((r_u8(car + 0x354) >> wheel) & 1) != 0)
            goto cleanup_gate;
        amount -= 15000;
        if (amount >= 20481)
        {
            value = r_u32(car + wheel * 12 + 0xE4);
            w_u32(frame + 0x1C, value + 30720);
            value = r_u32(car + wheel * 12 + 0xE0);
            w_u32(frame + 0x18, value);
            value = r_u32(car + wheel * 12 + 0xE8);
            w_u32(frame + 0x20, value);
            type = r_u8(car + 0x348);
            if (type == 1 || type == 10 || type == 3 || type == 5)
                td5_8003ce3c(car, frame + 0x18, 0);
        }
        if (r_u8(car + wheel + 0x349) == 255)
        {
            value = r_u8(car + 0x34D);
            handle = td5_800443ac(wheel, value, wheel, 40, 26);
            w_u8(car + wheel + 0x349, handle);
        }
        handle = r_u8(car + wheel + 0x349);
        w_u8(0x8009D8D5 + handle * 7, (uint32)(amount / 2048));
        type = r_u8(car + 0x348);
        if (type == 2 || type - 5 < 2 || type == 4 || type == 9)
        {
            handle = r_u8(car + wheel + 0x349);
            value = r_u8(0x8009D8D5 + handle * 7);
            w_u8(0x8009D8D5 + handle * 7, value >> 1);
        }
        continue;
    cleanup_gate:
        if (r_u8(car + 0x351) == 0 && (r_u8(car + 0x34E) & 2) != 0 && ((r_u8(car + 0x354) >> wheel) & 1) == 0)
            continue;
    remove_effect:
        handle = r_u8(car + wheel + 0x349);
        if (handle != 255)
        {
            value = r_u32(0x800885A8);
            descriptor = ((handle + value * 4) & 255) * 7;
            effect = r_u8(0x8009D8D3 + descriptor);
            w_u8(0x8009D8D2 + descriptor, 0);
            value = r_u32(0x80088774);
            w_u8(value + effect * 132 + 112, 0);
            w_u8(car + wheel + 0x349, 255);
        }
    }
    return 0;
}

uint32 td5_80044cb4(uint32 car, sint32 amount, uint32 original_stack)
{
    uint32 frame = original_stack - 0x48;
    uint32 wheel, value, type, handle, descriptor, effect;

    FUNCTION_MARKER(0x80044CB4, "SLUS_006.10");
    for (wheel = 2; wheel < 4; ++wheel)
    {
        if (amount < 10001)
            goto cleanup_gate;
        if (r_u8(car + 0x351) != 0)
            goto remove_effect;
        if ((r_u8(car + 0x34E) & 1) != 0 || ((r_u8(car + 0x354) >> wheel) & 1) != 0)
            goto cleanup_gate;
        amount -= 10000;
        if (amount >= 20481)
        {
            value = r_u32(car + wheel * 12 + 0xE4);
            w_u32(frame + 0x1C, value + 30720);
            value = r_u32(car + wheel * 12 + 0xE0);
            w_u32(frame + 0x18, value);
            value = r_u32(car + wheel * 12 + 0xE8);
            w_u32(frame + 0x20, value);
            type = r_u8(car + 0x348);
            if (type == 1 || type == 10 || type == 3 || type == 5)
                td5_8003ce3c(car, frame + 0x18, 0);
        }
        if (r_u8(car + wheel + 0x349) == 255)
        {
            value = r_u8(car + 0x34D);
            handle = td5_800443ac(wheel, value, wheel, 55, 26);
            w_u8(car + wheel + 0x349, handle);
        }
        handle = r_u8(car + wheel + 0x349);
        w_u8(0x8009D8D5 + handle * 7, (uint32)(amount / 2048));
        type = r_u8(car + 0x348);
        if (type == 2 || type - 5 < 2 || type == 4 || type == 9)
        {
            handle = r_u8(car + wheel + 0x349);
            value = r_u8(0x8009D8D5 + handle * 7);
            w_u8(0x8009D8D5 + handle * 7, value >> 1);
        }
        continue;
    cleanup_gate:
        if (r_u8(car + 0x351) == 0 && (r_u8(car + 0x34E) & 1) != 0 && ((r_u8(car + 0x354) >> wheel) & 1) == 0)
            continue;
    remove_effect:
        handle = r_u8(car + wheel + 0x349);
        if (handle != 255)
        {
            value = r_u32(0x800885A8);
            descriptor = ((handle + value * 4) & 255) * 7;
            effect = r_u8(0x8009D8D3 + descriptor);
            w_u8(0x8009D8D2 + descriptor, 0);
            value = r_u32(0x80088774);
            w_u8(value + effect * 132 + 112, 0);
            w_u8(car + wheel + 0x349, 255);
        }
    }
    return 0;
}

uint32 td5_80044f3c(uint32 car, uint32 flags, uint32 original_stack, uint32 incoming_result)
{
    uint32 frame = original_stack - 0x38;
    uint32 value, slot, handle, wheel, point, type, result;
    sint32 random_value, strength;

    FUNCTION_MARKER(0x80044F3C, "SLUS_006.10");
    if ((flags & 1) == 0)
        return incoming_result;
    for (wheel = 2; wheel < 4; ++wheel)
    {
        if (r_u8(car + 0x349 + wheel) == 255)
        {
            slot = r_u8(car + 0x34D);
            value = td5_800443ac(wheel, slot, wheel, 55, 26);
            w_u8(car + 0x349 + wheel, value);
        }
    }
    random_value = randPSX();
    strength = r_s16(car + 0x314);
    if (random_value % 50 < strength / 2)
    {
        value = (uint32)randPSX();
        point = car + ((value & 1) + 2) * 12;
        value = r_u32(point + 0xE4);
        w_u32(frame + 0x1C, value + 30720);
        value = r_u32(point + 0xE0);
        w_u32(frame + 0x18, value);
        value = r_u32(point + 0xE8);
        w_u32(frame + 0x20, value);
        td5_8003ce3c(car, frame + 0x18, 0);
    }
    for (wheel = 2; wheel < 4; ++wheel)
    {
        handle = r_u8(car + 0x349 + wheel);
        strength = r_s16(car + 0x314);
        w_u8(0x8009D8D5 + handle * 7, (uint32)(strength / 4));
    }
    type = r_u8(car + 0x348);
    if (type == 2 || type - 5 < 2 || type == 4 || type == 9)
    {
        for (wheel = 2; wheel < 4; ++wheel)
        {
            handle = r_u8(car + 0x349 + wheel);
            value = r_u8(0x8009D8D5 + handle * 7);
            result = value >> 1;
            w_u8(0x8009D8D5 + handle * 7, result);
        }
        return result;
    }
    return 9;
}

uint32 td5_80025900(uint32 node)
{
    uint32 index, lane, descriptor, flags, type;

    FUNCTION_MARKER(0x80025900, "SLUS_006.10");
    index = (uint32)r_s16(node);
    lane = r_u8(node + 12);
    descriptor = r_u32(0x800884C4) + index * 24;
    flags = r_u8(descriptor + 2);
    if (((flags >> (lane & 31)) & 1) != 0)
    {
        type = r_u8(descriptor + 1);
        return (type >> 4) | 16;
    }
    type = r_u8(descriptor + 1);
    return type & 15;
}

uint32 td5_800250f0(uint32 car)
{
    VECTOR first = {0}, second = {0}, normal = {0};
    sint32 vectors[2][3];
    uint32 basis, axis, a, b, c, d, strength, product, result;

    FUNCTION_MARKER(0x800250F0, "SLUS_006.10");
    for (basis = 0; basis < 2; ++basis)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            a = r_u32(car + 0x80 + axis * 4);
            b = r_u32(car + (basis == 0 ? 0x98 : 0x8C) + axis * 4);
            c = r_u32(car + (basis == 0 ? 0x8C : 0x98) + axis * 4);
            d = r_u32(car + 0xA4 + axis * 4);
            vectors[basis][axis] = (sint32)(a + b - (c + d)) >> 8;
        }
    }
    first.vx = vectors[0][0];
    first.vy = vectors[0][1];
    first.vz = vectors[0][2];
    second.vx = vectors[1][0];
    second.vy = vectors[1][1];
    second.vz = vectors[1][2];
    VectorNormal(&first, &first);
    VectorNormal(&second, &second);
    OuterProduct12(&first, &second, &normal);
    strength = r_u32(0x800877A4);
    product = (uint32)normal.vx * strength;
    result = r_u32(car + 0x194) + (uint32)((sint32)product / 8192);
    w_u32(car + 0x194, result);
    product = (uint32)normal.vz * strength;
    result = r_u32(car + 0x19C) + (uint32)((sint32)product / 8192);
    w_u32(car + 0x19C, result);
    return result;
}

uint32 td5_80025340(uint32 car, uint32 descriptor)
{
    uint32 gear, model, active, product, result;
    sint32 limit, pedal, target, current, step, speed, coefficient, difference;

    FUNCTION_MARKER(0x80025340, "SLUS_006.10");
    gear = r_u8(car + 0x343);
    if (gear == 1)
    {
        model = r_u32(car + 0x184);
        active = r_u8(car + 0x345);
        limit = r_s16(model + 0x72);
        target = 400;
        if (active == 0)
        {
            pedal = r_s16(car + 0x316);
            if (pedal >= 0)
            {
                product = ((uint32)limit - 400) * (uint32)pedal;
                step = (sint32)product / 256;
                target = (sint32)((uint32)(step < limit ? step : limit) + 400);
            }
        }
        current = r_s32(car + 0x2E8);
        step = (sint32)((uint32)target - (uint32)current) / 16;
        if (current < target)
        {
            if (step > 400)
                step = 400;
        }
        else if (step > 200)
            step = 200;
        result = (uint32)current + (uint32)step;
        w_u32(car + 0x2E8, result);
        return result;
    }
    speed = r_s32(car + 0x2EC) >> 8;
    coefficient = r_s16(descriptor + gear * 2 + 46);
    if (speed < 0)
        speed = -speed;
    product = (uint32)speed * (uint32)coefficient;
    current = r_s32(car + 0x2E8);
    product *= 45;
    target = (sint32)((uint32)((sint32)product / 4096) + 400);
    difference = (sint32)((uint32)current - (uint32)target);
    if (difference >= 801)
        current = (sint32)((uint32)current - 200);
    else if (difference < -800)
        current = (sint32)((uint32)current + 200);
    else
    {
        step = (sint32)((uint32)target - (uint32)current) / 4;
        current = (sint32)((uint32)current + (uint32)step);
    }
    model = r_u32(car + 0x184);
    limit = r_s16(model + 0x72);
    result = limit < current;
    if (result != 0)
        current = limit;
    w_u32(car + 0x2E8, (uint32)current);
    return result;
}

uint32 td5_800257e8(uint32 car)
{
    uint32 result;

    FUNCTION_MARKER(0x800257E8, "SLUS_006.10");
    result = r_u8(car + 0x343);
    if (result == 0)
    {
        result = 0 - (uint32)r_u16(car + 0x316);
        w_u16(car + 0x316, result);
    }
    return result;
}

uint32 td5_80025600(uint32 car, uint32 descriptor)
{
    uint32 gear, value, slot, product, factor, result, words[4], index;
    sint32 pedal, current, threshold, random_value, coefficient;

    FUNCTION_MARKER(0x80025600, "SLUS_006.10");
    pedal = r_s16(car + 0x316);
    gear = r_u8(car + 0x343);
    current = r_s32(car + 0x2E8);
    if (pedal < 0)
    {
        w_u8(car + 0x343, 0);
        return (uint32)pedal;
    }
    if (gear == 0)
    {
        if (pedal <= 0)
            return 2;
        w_u8(car + 0x343, 2);
    }
    threshold = r_s16(descriptor + gear * 2 + 0x3E);
    if (threshold < current && gear < 8 && r_s32(car + 0x2EC) > 0)
    {
        value = r_u8(car + 0x343);
        slot = r_u8(car + 0x34D);
        w_u8(car + 0x343, value + 1);
        if (r_s32(0x8008772C) >= (sint32)slot)
        {
            random_value = randPSX();
            if (random_value % 100 < 80 && r_u8(car + 0x343) < 4)
                td5_80053098();
        }
        pedal = r_s16(car + 0x316);
        coefficient = r_s16(descriptor + 0x68);
        product = (uint32)coefficient * ((uint32)pedal * 26);
        value = r_u8(car + 0x343);
        factor = r_u32(0x8007DF1C + value * 4);
        product = (uint32)((sint32)product / 256) * factor;
        result = (uint32)((sint32)product / 256);
        for (index = 0; index < 4; ++index)
            words[index] = r_u32(car + 0x2C4 + index * 4);
        w_u32(car + 0x2C4, words[0] + result);
        w_u32(car + 0x2C8, words[1] + result);
        w_u32(car + 0x2CC, words[2] - result);
        w_u32(car + 0x2D0, words[3] - result);
        return result;
    }
    threshold = r_s16(descriptor + gear * 2 + 0x4E);
    result = gear < 3;
    if (current < threshold && gear >= 3)
    {
        result = r_u8(car + 0x343) - 1;
        w_u8(car + 0x343, result);
    }
    return result;
}

uint32 td5_80025810(uint32 car, uint32 descriptor)
{
    uint32 gear, table, product, result;
    sint32 pedal, speed, index, coefficient, first, second, fraction, value, limit;

    FUNCTION_MARKER(0x80025810, "SLUS_006.10");
    gear = r_u8(car + 0x343);
    pedal = r_s16(car + 0x316);
    speed = r_s32(car + 0x2E8);
    if (gear == 1)
        return 0;
    index = speed / 512;
    table = descriptor + ((uint32)index << 1);
    value = r_s16(table);
    coefficient = r_s16(descriptor + 0x68);
    product = (uint32)value * (uint32)coefficient;
    first = (sint32)product / 256;
    value = r_s16(table + 2);
    product = (uint32)value * (uint32)coefficient;
    second = (sint32)product / 256;
    fraction = (sint32)((uint32)speed - ((uint32)index << 9));
    product = ((uint32)second - (uint32)first) * (uint32)fraction;
    value = (sint32)((uint32)first + (uint32)((sint32)product / 512));
    product = (uint32)pedal * (uint32)value;
    coefficient = r_s16(descriptor + gear * 2 + 46);
    product = (uint32)((sint32)product / 256) * (uint32)coefficient;
    result = (uint32)((sint32)product / 256);
    limit = r_s16(descriptor + 0x72) - 50;
    if (limit < speed)
        result = 0;
    return result;
}

uint32 td5_80053098(void)
{
    sint32 random_value;

    FUNCTION_MARKER(0x80053098, "SLUS_006.10");
    random_value = randPSX();
    return td5_80054930(15, -1, random_value % 1024 + 1024, 0, 4096);
}

uint32 td5_80054930(sint32 sample, sint32 voice, sint32 volume, sint32 pan, sint32 pitch)
{
    uint32 product;

    FUNCTION_MARKER(0x80054930, "SLUS_006.10");
    product = (uint32)volume * r_u32(0x80087704);
    volume = (sint32)product / 4096;
    return (uint32)td5_play_sound(sample, voice, volume, pan, pitch);
}

uint32 td5_80020428(uint32 car, uint32 descriptor, uint32 acceleration_x, uint32 acceleration_z)
{
    uint32 spring, damping, response, extra, node, velocity_address, position_address;
    uint32 x, z, world_x, world_z, product, drive, velocity, resistance, tick, result;
    sint32 limit, lower_limit;

    FUNCTION_MARKER(0x80020428, "SLUS_006.10");
    response = (uint32)r_s16(descriptor + 0x62);
    spring = (uint32)r_s16(descriptor + 0x5E);
    damping = (uint32)r_s16(descriptor + 0x60);
    extra = (uint32)r_s16(descriptor + 0x66);
    limit = r_s16(descriptor + 0x64);
    lower_limit = -limit;
    for (node = 0; node < 5; ++node)
    {
        if (node < 4)
        {
            velocity_address = car + 0x2C4 + node * 4;
            position_address = car + 0x2B4 + node * 4;
            world_x = r_u32(car + 0x1C4);
            x = r_u32(car + 0x260 + node * 16);
            world_z = r_u32(car + 0x1CC);
            x -= (uint32)((sint32)world_x / 256);
            z = r_u32(car + 0x268 + node * 16);
            z -= (uint32)((sint32)world_z / 256);
        }
        else
        {
            velocity_address = car + 0x2A8;
            position_address = car + 0x2A4;
            x = r_u32(car + 0x260);
            product = r_u32(car + 0x270);
            world_x = r_u32(car + 0x1C4);
            x = (uint32)((sint32)(x + product) / 2);
            z = r_u32(car + 0x268);
            product = r_u32(car + 0x278);
            world_z = r_u32(car + 0x1CC);
            x -= (uint32)((sint32)world_x / 256);
            z = (uint32)((sint32)(z + product) / 2) - (uint32)((sint32)world_z / 256);
        }
        drive = acceleration_x * x + acceleration_z * z;
        drive = (uint32)((sint32)drive / 256);
        product = drive * response;
        velocity = r_u32(velocity_address);
        velocity += (uint32)((sint32)product / 256);
        if (node < 4)
        {
            product = r_u32(car + 0x2D4 + node * 4) * extra;
            velocity += (uint32)((sint32)product / 256);
        }
        product = r_u32(position_address) * spring;
        resistance = (uint32)((sint32)product / 256);
        product = velocity * damping;
        resistance += (uint32)((sint32)product / 256);
        velocity -= resistance;
        if (node < 4 && velocity + 15 < 31)
            velocity = 0;
        tick = r_u32(0x80088100);
        product = velocity * tick;
        w_u32(velocity_address, velocity);
        x = r_u32(position_address) + (uint32)((sint32)product / 256);
        w_u32(position_address, x);
        if (limit < (sint32)x)
        {
            w_u32(position_address, (uint32)limit);
            w_u32(velocity_address, 0);
        }
        x = r_u32(position_address);
        result = (sint32)x < lower_limit;
        if (result != 0)
        {
            w_u32(position_address, (uint32)lower_limit);
            w_u32(velocity_address, 0);
        }
    }
    return result;
}

uint32 td5_80020c88(uint32 car)
{
    uint32 mask, index, pointer, value, count, sum, product, cosine, sine, x, z;
    sint32 angle, scaled, lateral, clamped;

    FUNCTION_MARKER(0x80020C88, "SLUS_006.10");
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
        case 15:
            return 0x80020E88;
        default:
            break;
    }
    w_u32(0x800881C8, car + 0x1F8);
    w_u32(0x800881CC, car + 0x258);
    w_u32(0x800881D4, 0);
    w_u32(0x800881E8, 0);
    w_u32(0x800881D8, 0);
    do
    {
        index = r_u32(0x800881D4);
        if (((mask >> (index & 31)) & 1) == 0)
        {
            pointer = r_u32(0x800881C8);
            value = (uint32)r_s16(pointer + 2);
            count = r_u32(0x800881E8);
            sum = r_u32(0x800881D8);
            w_u32(0x800881E8, count + 1);
            w_u32(0x800881D8, sum + value);
        }
        pointer = r_u32(0x800881C8);
        ++index;
        w_u32(0x800881D4, index);
        w_u32(0x800881C8, pointer + 8);
    } while ((sint32)index < 4);
    sum = r_u32(0x800881D8);
    count = r_u32(0x800881E8);
    scaled = td5_divide_checked((sint32)sum, (sint32)count);
    pointer = r_u32(0x800881CC);
    value = (uint32)r_s16(pointer + 2);
    product = (uint32)scaled * value;
    angle = r_s16(car + 0x1D2);
    w_u32(0x800881D8, (uint32)((sint32)product / 4096));
    w_u32(0x800881D0, car + 0x194);
    cosine = (uint32)rcos(angle);
    angle = r_s16(car + 0x1D2);
    w_u32(0x800881E0, cosine);
    sine = (uint32)rsin(angle);
    pointer = r_u32(0x800881D0);
    x = r_u32(pointer);
    w_u32(0x800881E4, sine);
    cosine = r_u32(0x800881E0);
    product = (uint32)((sint32)x / 256) * cosine;
    z = r_u32(pointer + 8);
    product -= (uint32)((sint32)z / 256) * sine;
    lateral = (sint32)product / 4096;
    w_u32(0x800881DC, (uint32)lateral);
    clamped = lateral < -512 ? -512 : (lateral > 512 ? 512 : lateral);
    value = r_u32(0x800881D8);
    product = (uint32)clamped * value;
    w_u32(0x800881DC, (uint32)clamped);
    value = r_u32(car + 0x190);
    scaled = (sint32)product / 64;
    w_u32(0x800881EC, (uint32)scaled);
    value -= (uint32)scaled;
    w_u32(car + 0x190, value);
    return value;
}

static uint32 td5_22b1c_div(uint32 value, sint32 divisor)
{
    return (uint32)td5_divide_checked((sint32)value, divisor);
}

uint32 td5_80022b1c(uint32 car)
{
    uint32 model, surface, value, other, drag, tick, vx, vz, yaw_velocity;
    uint32 front_length, rear_length, inertia, grip_strength, wheelbase, mass, offset;
    uint32 heading, steering, cosine, sine, turn_cosine, turn_sine, steer_cosine, steer_sine;
    uint32 pedal, active, lateral, forward, steered_forward, grip_a, grip_b;
    uint32 torque_a, torque_b, pair_a, pair_b, bound, selected;
    uint32 denominator, coupling, momentum, term_a, term_b, term_c, term_d;
    uint32 numerator, force_a, force_b, limit_a, limit_b, norm, residual_a, residual_b;
    uint32 yaw_acceleration, acceleration_x, acceleration_z, step, previous;

    FUNCTION_MARKER(0x80022B1C, "SLUS_006.10");
    model = r_u32(car + 0x184);
    surface = td5_80025900(car + 0x70);
    w_u8(car + 0x348, surface);
    td5_800250f0(car);
    value = (uint32)r_s16(car + 0x316);
    if ((sint32)value >= 32 && r_u8(car + 0x343) >= 2)
    {
        value = (uint32)r_s16(0x8007DF7C + surface * 2);
        other = (uint32)r_s16(model + 0x6A);
    }
    else
    {
        value = (uint32)r_s16(0x8007DF7C + surface * 2);
        other = (uint32)r_s16(model + 0x6C);
    }
    drag = other + (value << 8);
    vx = r_u32(car + 0x194);
    value = td5_22b1c_div(0 - vx, 256);
    value = td5_22b1c_div(value * drag, 4096);
    tick = r_u32(0x80088100);
    value = td5_22b1c_div(value * tick, 256);
    vz = r_u32(car + 0x19C);
    w_u32(car + 0x194, vx + value);
    value = td5_22b1c_div(0 - vz, 256);
    value = td5_22b1c_div(value * drag, 4096);
    value = td5_22b1c_div(value * tick, 256);
    steering = r_u32(car + 0x2E4);
    heading = r_u32(car + 0x1BC);
    vx = r_u32(car + 0x194);
    yaw_velocity = r_u32(car + 0x18C);
    vz += value;
    w_u32(car + 0x19C, vz);
    heading = (uint32)((sint32)heading >> 8);
    steering = td5_22b1c_div(steering, 256);
    front_length = (uint32)r_s16(model + 0x28);
    pedal = (uint32)r_s16(car + 0x316);
    rear_length = (uint32)r_s16(model + 0x2A);
    inertia = r_u32(model + 0x20);
    grip_strength = (uint32)r_s16(model + 0x2C);
    cosine = (uint32)rcos((sint32)heading);
    sine = (uint32)rsin((sint32)heading);
    turn_cosine = (uint32)rcos((sint32)(heading + steering));
    turn_sine = (uint32)rsin((sint32)(heading + steering));
    steer_cosine = (uint32)rcos((sint32)steering);
    steer_sine = (uint32)rsin((sint32)steering);
    lateral = td5_22b1c_div(vx * cosine - vz * sine, 4096);
    forward = td5_22b1c_div(vx * sine + vz * cosine, 4096);
    wheelbase = front_length + rear_length;
    value = td5_22b1c_div(rear_length << 8, (sint32)wheelbase);
    mass = r_u32(model + 0x24);
    offset = r_u32(car + 0x2A4);
    grip_a = td5_22b1c_div(value * (mass - offset), (sint32)mass);
    value = td5_22b1c_div(front_length << 8, (sint32)wheelbase);
    grip_b = td5_22b1c_div(value * (mass + offset), (sint32)mass);
    if ((sint32)grip_a < 112)
        grip_a = 112;
    if ((sint32)grip_a > 160)
        grip_a = 160;
    if ((sint32)grip_b < 112)
        grip_b = 112;
    if ((sint32)grip_b > 160)
        grip_b = 160;
    value = td5_22b1c_div(vx * turn_sine + vz * turn_cosine, 4096);
    other = td5_22b1c_div(yaw_velocity * front_length * steer_sine, 2670592);
    active = r_u8(car + 0x345);
    w_u32(car + 0x2EC, forward);
    steered_forward = value - other;
    w_u32(car + 0x2F0, steered_forward);
    if (active != 0 || pedal == 0)
    {
        if (pedal == 0)
            pedal = (uint32)-32;
        td5_80025340(car, model);
        value = (uint32)r_s16(model + 0x6E);
        torque_a = td5_22b1c_div(value * pedal, 256);
        value = (uint32)r_s16(model + 0x70);
        torque_b = td5_22b1c_div(value * pedal, 256);
        bound = 0 - torque_a;
        value = td5_22b1c_div(0 - steered_forward, 2);
        if ((sint32)value < (sint32)bound)
            bound = value;
        selected = (sint32)torque_a < (sint32)bound ? bound : torque_a;
        torque_a = td5_22b1c_div(selected, 2);
        bound = 0 - torque_b;
        value = td5_22b1c_div(0 - forward, 2);
        if ((sint32)value < (sint32)bound)
            bound = value;
        selected = (sint32)torque_b < (sint32)bound ? bound : torque_b;
        torque_b = td5_22b1c_div(selected, 2);
    }
    else
    {
        td5_80025600(car, model);
        td5_80025340(car, model);
        torque_a = td5_22b1c_div(td5_80025810(car, model), 4);
        torque_b = torque_a;
        value = (uint32)r_s16(model + 0x74);
        other = r_u32(car + 0x2EC);
        if ((sint32)(value << 8) < (sint32)other)
        {
            torque_a = 0;
            torque_b = 0;
        }
    }
    pair_a = torque_a + torque_a;
    pair_b = torque_b + torque_b;
    value = td5_22b1c_div(inertia + rear_length * rear_length, 1024);
    term_a = td5_22b1c_div(value * steer_sine, 4096);
    term_a = td5_22b1c_div(term_a * steer_sine, 4096);
    value = td5_22b1c_div(wheelbase * wheelbase, 1024);
    term_b = td5_22b1c_div(value * steer_cosine, 4096);
    term_b = td5_22b1c_div(term_b * steer_cosine, 4096);
    denominator = term_a + term_b;
    coupling = td5_22b1c_div(rear_length * front_length - inertia, 1024);
    term_a = td5_22b1c_div(coupling * pair_a, 4096);
    term_a = td5_22b1c_div(term_a * steer_sine, 4096);
    term_a = td5_22b1c_div(term_a * steer_sine, 4096);
    value = td5_22b1c_div(rear_length * yaw_velocity, 652) - lateral;
    other = td5_22b1c_div(inertia, 1024);
    term_b = td5_22b1c_div(other * value, 4096);
    term_b = td5_22b1c_div(term_b * steer_sine, 4096);
    value = td5_22b1c_div(pair_a * steer_cosine, 4096) + pair_b + forward;
    term_c = td5_22b1c_div(coupling * value, 4096);
    term_c = td5_22b1c_div(term_c * steer_cosine, 4096);
    momentum = yaw_velocity * td5_22b1c_div(inertia, 652);
    value = td5_22b1c_div(momentum - front_length * lateral, 1024);
    term_d = td5_22b1c_div(value * wheelbase, 4096);
    term_d = td5_22b1c_div(term_d * steer_cosine, 4096);
    numerator = (term_a + term_b + term_c) * steer_sine + term_d * steer_cosine;
    force_a = td5_22b1c_div(numerator, (sint32)denominator);
    value = td5_22b1c_div(0 - (rear_length * lateral + momentum), 1024);
    term_a = td5_22b1c_div(value * wheelbase, 4096) * steer_cosine;
    value = td5_22b1c_div(rear_length * 2 * front_length - inertia + front_length * front_length, 1024);
    term_b = td5_22b1c_div(value * pair_a, 4096) * steer_cosine;
    value = td5_22b1c_div(inertia + rear_length * rear_length, 1024);
    term_b += value * (forward + pair_b);
    term_b = td5_22b1c_div(term_b, 4096) * steer_sine;
    force_b = td5_22b1c_div(term_a + term_b, (sint32)denominator);
    limit_b = td5_22b1c_div(grip_strength * grip_b, 256);
    w_u16(car + 0x314, 0);
    residual_a = 0;
    value = td5_22b1c_div(force_a, 16);
    other = td5_22b1c_div(pair_b, 16);
    norm = (uint32)SquareRoot0((sint32)(value * value + other * other)) << 4;
    if ((sint32)limit_b < (sint32)norm)
    {
        value = td5_22b1c_div(limit_b << 8, (sint32)norm);
        residual_a = norm - limit_b;
        force_a = td5_22b1c_div(force_a * value, 256);
    }
    limit_a = td5_22b1c_div(grip_strength * grip_a, 256);
    residual_b = 0;
    w_u32(car + 0x2F4, residual_a);
    value = td5_22b1c_div(force_b, 16);
    other = td5_22b1c_div(pair_a, 16);
    norm = (uint32)SquareRoot0((sint32)(value * value + other * other)) << 4;
    if ((sint32)limit_a < (sint32)norm)
    {
        value = td5_22b1c_div(limit_a << 8, (sint32)norm);
        residual_b = norm - limit_a;
        force_b = td5_22b1c_div(force_b * value, 256);
    }
    value = td5_22b1c_div(force_a * cosine, 4096);
    w_u32(car + 0x2F8, residual_b);
    other = td5_22b1c_div((0 - force_a) * sine, 4096);
    acceleration_x = value + td5_22b1c_div(force_b * turn_cosine, 4096);
    acceleration_z = other + td5_22b1c_div((0 - force_b) * turn_sine, 4096);
    value = td5_22b1c_div(front_length * steer_cosine, 4096);
    numerator = force_b * value - force_a * rear_length;
    yaw_acceleration = td5_22b1c_div(numerator, (sint32)td5_22b1c_div(inertia, 652));
    if ((sint32)yaw_acceleration > 1400)
        yaw_acceleration = 1400;
    if ((sint32)yaw_acceleration < -1400)
        yaw_acceleration = (uint32)-1400;
    acceleration_x += td5_22b1c_div(pair_b * sine, 4096);
    acceleration_x += td5_22b1c_div(pair_a * turn_sine, 4096);
    acceleration_z += td5_22b1c_div(pair_b * cosine, 4096);
    acceleration_z += td5_22b1c_div(pair_a * turn_cosine, 4096);
    tick = r_u32(0x80088100);
    step = td5_22b1c_div(yaw_acceleration * tick, 256);
    previous = r_u32(car + 0x18C);
    w_u32(car + 0x18C, previous + step);
    step = td5_22b1c_div(acceleration_x * tick, 256);
    previous = r_u32(car + 0x194);
    w_u32(car + 0x194, previous + step);
    step = td5_22b1c_div(acceleration_z * tick, 256);
    value = (uint32)((sint32)r_u32(car + 0x2F0) >> 8);
    previous = r_u32(car + 0x19C);
    w_u32(car + 0x19C, previous + step);
    step = td5_22b1c_div(value * tick, 256);
    value = (uint32)((sint32)r_u32(car + 0x2EC) >> 8);
    previous = r_u16(car + 0x318);
    w_u16(car + 0x318, previous + step);
    step = td5_22b1c_div(value * tick, 256);
    previous = r_u16(car + 0x31A);
    w_u16(car + 0x31A, previous + step);
    if ((sint32)residual_a > 0)
    {
        previous = r_u32(car + 0x18C);
        value = td5_22b1c_div(previous, 64);
        value = td5_22b1c_div(value * tick, 256);
        value = td5_22b1c_div(value * residual_a, 32768);
        if ((sint32)value > 512)
            value = 512;
        if ((sint32)value < -512)
            value = (uint32)-512;
        w_u32(car + 0x18C, previous - value);
    }
    return td5_80020428(car, model, acceleration_x, acceleration_z);
}

uint32 td5_8002b3e8(uint32 first_car, uint32 second_car, uint32 positions, uint32 headings, uint32 contacts)
{
    uint32 heading_a = r_u32(headings);
    uint32 heading_b = r_u32(headings + 4);
    uint32 position_ax = r_u32(positions);
    uint32 position_bz = r_u32(positions + 20);
    uint32 position_bx = r_u32(positions + 12);
    uint32 position_az = r_u32(positions + 8);
    sint32 angle = (sint32)((heading_a - heading_b) << 8) >> 16;
    uint32 dx = (uint32)((sint32)(position_bx - position_ax) >> 8);
    uint32 dz = (uint32)((sint32)(position_bz - position_az) >> 8);
    uint32 cosine = (uint32)rcos(angle);
    uint32 sine = (uint32)rsin(angle);
    uint32 cosine_a = (uint32)rcos(r_s32(headings) >> 8);
    uint32 sine_a = (uint32)rsin(r_s32(headings) >> 8);
    uint32 cosine_b = (uint32)rcos(r_s32(headings + 4) >> 8);
    uint32 sine_b = (uint32)rsin(r_s32(headings + 4) >> 8);
    uint32 model_a = r_u32(first_car + 0x180);
    uint32 width_a = (uint32)r_s16(model_a + 8);
    uint32 front_a = (uint32)r_s16(model_a + 4);
    uint32 rear_a = (uint32)r_s16(model_a + 20);
    uint32 model_b = r_u32(second_car + 0x180);
    uint32 width_b = (uint32)r_s16(model_b + 8);
    uint32 front_b = (uint32)r_s16(model_b + 4);
    uint32 rear_b = (uint32)r_s16(model_b + 20);
    uint32 aw_cos = (uint32)((sint32)(cosine * width_a) >> 12);
    uint32 aw_sin = (uint32)((sint32)((0u - sine) * width_a) >> 12);
    uint32 af_cos = (uint32)((sint32)(cosine * front_a) >> 12);
    uint32 af_sin = (uint32)((sint32)((0u - sine) * front_a) >> 12);
    uint32 ar_cos = (uint32)((sint32)(cosine * (0u - rear_a)) >> 12);
    uint32 ar_sin = (uint32)((sint32)((0u - sine) * (0u - rear_a)) >> 12);
    uint32 bw_cos = (uint32)((sint32)(cosine * width_b) >> 12);
    uint32 bw_sin = (uint32)((sint32)(sine * width_b) >> 12);
    uint32 bf_cos = (uint32)((sint32)(cosine * front_b) >> 12);
    uint32 bf_sin = (uint32)((sint32)(sine * front_b) >> 12);
    uint32 br_cos = (uint32)((sint32)(cosine * (0u - rear_b)) >> 12);
    uint32 br_sin = (uint32)((sint32)(sine * (0u - rear_b)) >> 12);
    uint32 corner_x[8];
    uint32 corner_z[8];
    uint32 translation_x = (uint32)((sint32)(dx * cosine_a - dz * sine_a) >> 12);
    uint32 translation_z = (uint32)((sint32)(dx * sine_a + dz * cosine_a) >> 12);
    uint32 width = width_a, front = front_a, rear = rear_a;
    uint32 mask = 0;
    uint32 side, point;

    FUNCTION_MARKER(0x8002B3E8, "SLUS_006.10");
    corner_x[0] = 0u - bw_cos - bf_sin;
    corner_z[0] = bf_cos - bw_sin;
    corner_x[1] = bw_cos - bf_sin;
    corner_z[1] = bw_sin + bf_cos;
    corner_x[2] = br_sin - bw_cos;
    corner_z[2] = 0u - bw_sin - br_cos;
    corner_x[3] = bw_cos + br_sin;
    corner_z[3] = bw_sin - br_cos;
    corner_x[4] = 0u - aw_cos - af_sin;
    corner_z[4] = af_cos - aw_sin;
    corner_x[5] = aw_cos - af_sin;
    corner_z[5] = aw_sin + af_cos;
    corner_x[6] = ar_sin - aw_cos;
    corner_z[6] = 0u - aw_sin - ar_cos;
    corner_x[7] = aw_cos + ar_sin;
    corner_z[7] = aw_sin - ar_cos;
    for (side = 0; side < 2; ++side)
    {
        if (side != 0)
        {
            translation_x = (uint32)((sint32)((0u - dx) * cosine_b + dz * sine_b) >> 12);
            translation_z = (uint32)((sint32)((0u - dx) * sine_b - dz * cosine_b) >> 12);
            width = width_b;
            front = front_b;
            rear = rear_b;
        }
        for (point = side * 4; point < side * 4 + 4; ++point)
        {
            uint32 x = corner_x[point] + translation_x;
            uint32 z = corner_z[point] + translation_z;
            if ((sint32)x <= (sint32)width && (sint32)x >= (sint32)(0u - width) && (sint32)z <= (sint32)front && (sint32)z >= (sint32)rear)
            {
                uint32 output = contacts + point * 8;
                mask |= 1u << point;
                w_u16(output + 4, x - translation_x);
                w_u16(output, x);
                w_u16(output + 2, z);
                w_u16(output + 6, z - translation_z);
            }
        }
    }
    return mask;
}

uint32 td5_80024938(uint32 car, uint32 original_stack)
{
    uint32 angle_x, angle_y, angle_z, position_x, position_y, position_z;
    uint32 value, second, third, mode, index, vector, output, model;
    uint32 total = 0, count = 0;
    sint32 height, suspension;

    FUNCTION_MARKER(0x80024938, "SLUS_006.10");
    angle_x = r_u32(car + 0x1B8);
    angle_y = r_u32(car + 0x1BC);
    angle_z = r_u32(car + 0x1C0);
    position_x = r_u32(car + 0x1C4);
    position_y = r_u32(car + 0x1C8);
    position_z = r_u32(car + 0x1CC);
    w_u16(car + 0x1D0, (sint32)angle_x >> 8);
    w_u16(car + 0x1D2, (sint32)angle_y >> 8);
    w_u16(car + 0x1D4, (sint32)angle_z >> 8);
    w_u32(car + 0x124, (sint32)position_x >> 8);
    w_u32(car + 0x128, (sint32)position_y >> 8);
    w_u32(car + 0x12C, (sint32)position_z >> 8);
    td5_80079548(car + 0x110, car + 0x1D0);
    td5_8001b310(car + 0x70, car + 0x1C4);
    td5_8001d8bc(car + 0x70, car + 0x1C4, car + 0x258);
    td5_80020030(car, original_stack - 0xA8);
    mode = r_u8(car + 0x354);
    switch (mode)
    {
        case 0:
        case 1:
        case 2:
        case 4:
        case 6:
        case 8:
        case 9:
            td5_8001dcf8(car + 0x1D0, car + 0xE0, car + 0x2B4);
            value = (uint32)r_s16(car + 0x1D0) << 8;
            second = (uint32)r_s16(car + 0x1D4) << 8;
            w_u32(car + 0x1B8, value);
            w_u32(car + 0x1C0, second);
            break;
        case 3:
        case 12:
            td5_8001df60(car + 0x1D0, car + 0xE0, car + 0x2B4);
            w_u32(car + 0x1C0, (uint32)r_s16(car + 0x1D4) << 8);
            break;
        case 5:
        case 10:
            td5_8001de8c(car + 0x1D0, car + 0xE0, car + 0x2B4);
            w_u32(car + 0x1B8, (uint32)r_s16(car + 0x1D0) << 8);
            break;
        default:
            break;
    }
    td5_80079548(car + 0x110, car + 0x1D0);
    model = r_u32(car + 0x180);
    height = 181 * r_s16(model + 0x82) / 256;
    value = r_u32(car + 0x110);
    second = r_u32(car + 0x114);
    xport_gte_write_control(0, value);
    xport_gte_write_control(1, second);
    value = r_u32(car + 0x118);
    second = r_u32(car + 0x11C);
    third = r_u32(car + 0x120);
    xport_gte_write_control(2, value);
    xport_gte_write_control(3, second);
    xport_gte_write_control(4, third);
    for (index = 0; index < 4; ++index)
    {
        if (((r_u8(car + 0x354) >> index) & 1) != 0)
            continue;
        model = r_u32(car + 0x180);
        suspension = r_s32(car + 0x2B4 + index * 4) / 256;
        value = r_u16(model + index * 8 + 0x42);
        vector = car + 0x1D8 + index * 8;
        output = car + 0x1F8 + index * 8;
        w_u16(vector + 2, value - (uint32)suspension - (uint32)height);
        xport_gte_write_data(0, r_u32(vector));
        xport_gte_write_data(1, r_u32(vector + 4));
        xport_gte_execute(0x486012);
        value = xport_gte_read_data(9);
        second = xport_gte_read_data(10);
        third = xport_gte_read_data(11);
        w_u16(output, value);
        w_u16(output + 2, second);
        w_u16(output + 4, third);
        value = r_u16(vector + 2);
        w_u16(vector + 2, value + (uint32)height);
        value = (uint32)r_s16(output + 2) << 8;
        second = r_u32(car + 0xE4 + index * 12);
        ++count;
        total += second - value;
    }
    if (count == 0)
        return 0;
    value = (uint32)((sint32)total / (sint32)count);
    w_u32(car + 0x1C8, value);
    return value;
}
