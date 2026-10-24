#include "td5_functions.h"

uint32 td5_800797fc(uint32 matrix, uint32 object)
{
    VECTOR input;
    VECTOR output;
    MATRIX *view;
    uint32 value;
    uint32 origin;
    uint32 column;
    uint32 first;
    uint32 second;
    uint32 third;

    FUNCTION_MARKER(0x800797FC, "SLUS_006.10");
    value = r_u32(object + 0x10);
    origin = r_u32(0x8009765C);
    input.vx = (sint32)((uint32)((sint32)value >> 8) - origin);
    value = r_u32(object + 0x14);
    origin = r_u32(0x80097660);
    input.vy = (sint32)((uint32)((sint32)value >> 8) - origin);
    value = r_u32(object + 0x18);
    origin = r_u32(0x80097664);
    input.vz = (sint32)((uint32)((sint32)value >> 8) - origin);
    view = (MATRIX *)psx_addr(0x80097648, 20);
    SetRotMatrix(view);
    ApplyMatrixLV(view, &input, &output);
    w_u32(0x80092224, (uint32)output.vx);
    w_u32(0x80092228, (uint32)output.vy);
    w_u32(0x8009222C, (uint32)output.vz);
    for (column = 0; column < 3; ++column)
    {
        first = r_u16(matrix + column * 2);
        second = r_u16(matrix + column * 2 + 6);
        third = r_u16(matrix + column * 2 + 12);
        xport_gte_write_data(9, first);
        xport_gte_write_data(10, second);
        xport_gte_write_data(11, third);
        xport_gte_execute(0x49E012);
        first = xport_gte_read_data(9);
        second = xport_gte_read_data(10);
        third = xport_gte_read_data(11);
        w_u16(0x80092210 + column * 2, first);
        w_u16(0x80092216 + column * 2, second);
        w_u16(0x8009221C + column * 2, third);
    }
    first = r_u32(0x80092210);
    second = r_u32(0x80092214);
    xport_gte_write_control(0, first);
    xport_gte_write_control(1, second);
    first = r_u32(0x80092218);
    second = r_u32(0x8009221C);
    third = r_u32(0x80092220);
    xport_gte_write_control(2, first);
    xport_gte_write_control(3, second);
    xport_gte_write_control(4, third);
    first = r_u32(0x80092224);
    second = r_u32(0x80092228);
    xport_gte_write_control(5, first);
    third = r_u32(0x8009222C);
    xport_gte_write_control(6, second);
    xport_gte_write_control(7, third);
    return matrix + 2;
}

void td5_800724ac(uint32 packet, uint32 ordering, uint32 vertices, sint32 depth)
{
    uint32 shift, other_shift, level, scale, allocation, limit, reserved;
    uint32 triangle_offset, buffer_index, first_buffer, second_buffer;
    uint32 quad_output, triangle_output, header, color, texture_22, texture_14;
    uint32 edge[3], vertical[3], across[3], current[3];
    uint32 edge_uv[2], vertical_uv[2], across_uv[2], current_uv[2];
    sint32 screen[3][2];
    uint32 clip[3];
    uint32 side, component, divisions, row, column, row_width;
    uint32 cursor, row_base, previous_row = 0, point, old_left, old_right, new_left;
    uint32 value, other, tag, bucket;
    sint32 threshold, width, height, projected_depth;

    FUNCTION_MARKER(0x800724AC, "SLUS_006.10");
    shift = r_u32(0x80088110);
    other_shift = r_u32(0x80088164);
    threshold = r_s32(0x80088134);
    shift = (shift - other_shift) & 31;
    if (depth >= threshold / 4)
        goto link_original;
    threshold = r_s32(0x80088138);
    level = depth < threshold / 4 ? 2 : 1;
    scale = level * 2;
    allocation = r_u32(0x80088514);
    limit = r_u32(0x800884FC);
    reserved = allocation + (40 << scale) - (8 << level);
    if (limit < reserved)
        goto link_original;
    for (side = 0; side < 3; ++side)
    {
        screen[side][0] = r_s16(packet + 8 + side * 8);
        screen[side][1] = r_s16(packet + 10 + side * 8);
    }
    width = r_s32(0x800880F4);
    height = r_s32(0x800880F8);
    for (side = 0; side < 3; ++side)
    {
        clip[side] = screen[side][0] < 0 ? 1 : 0;
        if (width < screen[side][0])
            clip[side] |= 2;
        if (screen[side][1] < 0)
            clip[side] |= 4;
        if (height < screen[side][1])
            clip[side] |= 8;
    }
    if ((clip[0] & clip[1] & clip[2]) != 0)
        return;
    allocation = r_u32(0x80088514);
    triangle_offset = allocation + (40 << scale) - (40 << level);
    buffer_index = r_u32(0x80088104);
    w_u32(0x80088514, triangle_offset);
    w_u32(0x80088514, triangle_offset + (32 << level));
    color = r_u32(packet + 4);
    first_buffer = r_u32(0x80088370 + buffer_index * 4);
    second_buffer = r_u32(0x80088370 + buffer_index * 4);
    header = r_u32(packet);
    texture_22 = r_u16(packet + 22);
    value = (uint32)r_s16(vertices);
    texture_14 = r_u16(packet + 14);
    edge[0] = value << scale;
    for (component = 1; component < 3; ++component)
        edge[component] = (uint32)r_s16(vertices + component * 2) << scale;
    for (component = 0; component < 2; ++component)
        edge_uv[component] = (uint16)(r_u8(packet + 12 + component) << scale);
    for (component = 0; component < 3; ++component)
    {
        value = (uint32)r_s16(vertices + 16 + component * 2);
        other = (uint32)r_s16(vertices + component * 2);
        vertical[component] = (value - other) << level;
    }
    for (component = 0; component < 2; ++component)
    {
        value = r_u8(packet + 28 + component);
        other = r_u8(packet + 12 + component);
        vertical_uv[component] = (uint16)((value - other) << level);
    }
    for (component = 0; component < 3; ++component)
    {
        value = (uint32)r_s16(vertices + 8 + component * 2);
        other = (uint32)r_s16(vertices + component * 2);
        across[component] = (value - other) << level;
    }
    for (component = 0; component < 2; ++component)
    {
        value = r_u8(packet + 20 + component);
        other = r_u8(packet + 12 + component);
        across_uv[component] = (uint16)((value - other) << level);
    }
    quad_output = first_buffer + allocation;
    triangle_output = second_buffer + triangle_offset;
    divisions = 1 << level;
    cursor = 0x800A4E58;
    for (row = 0; row <= divisions; ++row)
    {
        if (row != 0)
        {
            for (component = 0; component < 3; ++component)
                edge[component] += vertical[component];
            for (component = 0; component < 2; ++component)
                edge_uv[component] = (uint16)(edge_uv[component] + vertical_uv[component]);
        }
        for (component = 0; component < 3; ++component)
            current[component] = edge[component];
        for (component = 0; component < 2; ++component)
            current_uv[component] = edge_uv[component];
        row_base = cursor;
        row_width = divisions - row + 1;
        for (column = 0; column < row_width; ++column)
        {
            if (column != 0)
            {
                for (component = 0; component < 3; ++component)
                    current[component] += across[component];
                for (component = 0; component < 2; ++component)
                    current_uv[component] = (uint16)(current_uv[component] + across_uv[component]);
            }
            point = cursor;
            for (component = 0; component < 3; ++component)
                w_u16(point + component * 2, (sint32)current[component] >> scale);
            for (component = 0; component < 2; ++component)
                w_u8(point + 12 + component, (sint32)(sint16)current_uv[component] >> scale);
            cursor += 16;
            if (row == 0 || column == 0)
                continue;
            old_left = previous_row + (column - 1) * 16;
            old_right = old_left + 16;
            new_left = point - 16;
            w_u16(quad_output + 22, texture_22);
            w_u16(quad_output + 14, texture_14);
            w_u32(quad_output + 4, color);
            value = r_u8(old_left + 12);
            w_u8(quad_output + 12, value);
            value = r_u8(old_left + 13);
            w_u8(quad_output + 13, value);
            value = r_u8(old_right + 12);
            w_u8(quad_output + 20, value);
            value = r_u8(old_right + 13);
            w_u8(quad_output + 21, value);
            value = r_u8(new_left + 12);
            w_u8(quad_output + 28, value);
            value = r_u8(new_left + 13);
            w_u8(quad_output + 29, value);
            value = r_u8(point + 12);
            w_u8(quad_output + 36, value);
            value = r_u8(point + 13);
            w_u8(quad_output + 37, value);
            value = r_u32(old_left);
            xport_gte_write_data(0, value);
            value = r_u32(old_left + 4);
            xport_gte_write_data(1, value);
            value = r_u32(old_right);
            xport_gte_write_data(2, value);
            value = r_u32(old_right + 4);
            xport_gte_write_data(3, value);
            value = r_u32(new_left);
            xport_gte_write_data(4, value);
            value = r_u32(new_left + 4);
            xport_gte_write_data(5, value);
            xport_gte_execute(0x280030);
            w_u32(quad_output + 8, xport_gte_read_data(12));
            w_u32(quad_output + 16, xport_gte_read_data(13));
            w_u32(quad_output + 24, xport_gte_read_data(14));
            value = r_u32(point);
            xport_gte_write_data(0, value);
            value = r_u32(point + 4);
            xport_gte_write_data(1, value);
            xport_gte_execute(0x180001);
            xport_gte_execute(0x168002E);
            projected_depth = (sint32)xport_gte_read_data(7);
            if (projected_depth > 0)
            {
                w_u32(quad_output + 32, xport_gte_read_data(14));
                w_u8(quad_output + 3, 9);
                w_u8(quad_output + 7, 0x2C);
                bucket = ordering + ((uint32)projected_depth << 2);
                tag = r_u32(quad_output);
                value = r_u32(bucket);
                w_u32(quad_output, (tag & 0xFF000000) | (value & 0xFFFFFF));
                value = r_u32(bucket);
                w_u32(bucket, (value & 0xFF000000) | (quad_output & 0xFFFFFF));
            }
            quad_output += 40;
        }
        if (row != 0)
        {
            old_left = previous_row + (row_width - 1) * 16;
            old_right = old_left + 16;
            point = row_base + (row_width - 1) * 16;
            w_u16(triangle_output + 22, texture_22);
            w_u16(triangle_output + 14, texture_14);
            w_u32(triangle_output, header);
            w_u32(triangle_output + 4, color);
            value = r_u8(old_left + 12);
            w_u8(triangle_output + 12, value);
            value = r_u8(old_left + 13);
            w_u8(triangle_output + 13, value);
            value = r_u8(old_right + 12);
            w_u8(triangle_output + 20, value);
            value = r_u8(old_right + 13);
            w_u8(triangle_output + 21, value);
            value = r_u8(point + 12);
            w_u8(triangle_output + 28, value);
            value = r_u8(point + 13);
            w_u8(triangle_output + 29, value);
            value = r_u32(old_left);
            xport_gte_write_data(0, value);
            value = r_u32(old_left + 4);
            xport_gte_write_data(1, value);
            value = r_u32(old_right);
            xport_gte_write_data(2, value);
            value = r_u32(old_right + 4);
            xport_gte_write_data(3, value);
            value = r_u32(point);
            xport_gte_write_data(4, value);
            value = r_u32(point + 4);
            xport_gte_write_data(5, value);
            xport_gte_execute(0x280030);
            xport_gte_execute(0x158002D);
            projected_depth = (sint32)xport_gte_read_data(7);
            if (projected_depth > 0)
            {
                w_u32(triangle_output + 8, xport_gte_read_data(12));
                w_u32(triangle_output + 16, xport_gte_read_data(13));
                w_u32(triangle_output + 24, xport_gte_read_data(14));
                bucket = ordering + ((uint32)(projected_depth >> shift) << 2);
                tag = r_u32(triangle_output);
                value = r_u32(bucket);
                w_u32(triangle_output, (tag & 0xFF000000) | (value & 0xFFFFFF));
                value = r_u32(bucket);
                w_u32(bucket, (value & 0xFF000000) | (triangle_output & 0xFFFFFF));
            }
            triangle_output += 32;
        }
        previous_row = row_base;
    }
    return;

link_original:
    bucket = ordering + ((uint32)(depth >> shift) << 2);
    tag = r_u32(packet);
    value = r_u32(bucket);
    w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
    value = r_u32(bucket);
    w_u32(bucket, (value & 0xFF000000) | (packet & 0xFFFFFF));
}

uint32 td5_80072e84(uint32 packet, uint32 ordering, uint32 depths)
{
    static const uint32 triangle_uv[4][3] = {
        {12, 60, 20}, {60, 156, 76}, {108, 116, 156}, {12, 28, 108}
    };
    static const uint32 neighbor[4] = {1, 3, 2, 0};
    uint32 allocation, limit, reserved, buffer_index, buffer, color;
    uint32 texture_22, texture_14, triangle, source, group, corner;
    uint32 address, destination, value, tag = 0, result;
    sint32 current;

    FUNCTION_MARKER(0x80072E84, "SLUS_006.10");
    allocation = r_u32(0x80088514);
    limit = r_u32(0x800884FC);
    reserved = allocation + 128;
    result = limit < reserved;
    if (result != 0)
        return result;
    buffer_index = r_u32(0x80088104);
    color = r_u32(packet + 4);
    texture_22 = r_u16(packet + 22);
    texture_14 = r_u16(packet + 14);
    w_u32(0x80088514, reserved);
    buffer = r_u32(0x80088370 + buffer_index * 4);
    color &= 0xF7FFFFFF;
    current = r_s32(depths);
    triangle = buffer + allocation;
    source = buffer + allocation - 160;
    for (group = 0; group < 4; ++group, triangle += 32)
    {
        if (current <= 0)
        {
            if (group == 3)
                return (uint32)current;
            current = r_s32(depths + neighbor[group] * 4);
            continue;
        }
        current = r_s32(depths + neighbor[group] * 4);
        if (current <= 0)
        {
            if (group == 3)
                return (uint32)current;
            continue;
        }
        w_u32(triangle, 0x07000000);
        w_u16(triangle + 22, texture_22);
        w_u16(triangle + 14, texture_14);
        w_u32(triangle + 4, color);
        for (corner = 0; corner < 3; ++corner)
        {
            address = source + triangle_uv[group][corner];
            destination = triangle + corner * 8;
            value = r_u8(address);
            if (corner == 0)
                tag = r_u32(triangle);
            w_u8(destination + 12, value);
            value = r_u8(address + 1);
            w_u8(destination + 13, value);
            value = r_u16(address - 4);
            w_u16(destination + 8, value);
            value = r_u16(address - 2);
            w_u16(destination + 10, value);
        }
        value = r_u32(ordering);
        w_u32(triangle, (tag & 0xFF000000) | (value & 0xFFFFFF));
        value = r_u32(ordering);
        result = (value & 0xFF000000) | (triangle & 0xFFFFFF);
        w_u32(ordering, result);
        if (group == 3)
            return result;
        current = r_s32(depths + neighbor[group] * 4);
    }
    return result;
}

uint32 td5_800732a0(uint32 packet, uint32 ordering, uint32 depths)
{
    static const uint32 triangle_uv[4][3] = {
        {12, 140, 20}, {140, 636, 156}, {636, 508, 628}, {508, 12, 492}
    };
    static const uint32 quad_uv[4][4] = {
        {20, 140, 100, 60}, {156, 636, 476, 316},
        {628, 508, 548, 588}, {492, 12, 172, 332}
    };
    static const uint32 neighbor[4] = {1, 3, 2, 0};
    uint32 allocation, limit, reserved, buffer_index, first_buffer, second_buffer;
    uint32 triangle, quad, source, texture_22, texture_14, color, quad_color;
    uint32 group, corner, address, destination, value, tag = 0, result;
    sint32 current;

    FUNCTION_MARKER(0x800732A0, "SLUS_006.10");
    allocation = r_u32(0x80088514);
    limit = r_u32(0x800884FC);
    reserved = allocation + 288;
    result = limit < reserved;
    if (result != 0)
        return result;
    color = r_u32(packet + 4);
    buffer_index = r_u32(0x80088104);
    texture_22 = r_u16(packet + 22);
    texture_14 = r_u16(packet + 14);
    first_buffer = r_u32(0x80088370 + buffer_index * 4);
    second_buffer = r_u32(0x80088370 + buffer_index * 4);
    color &= 0xF7FFFFFF;
    w_u32(0x80088514, allocation + 128);
    w_u32(0x80088514, reserved);
    triangle = first_buffer + allocation;
    quad = second_buffer + allocation + 128;
    current = r_s32(depths);
    quad_color = color | 0x08000000;
    source = first_buffer + allocation - 640;
    for (group = 0; group < 4; ++group, triangle += 32, quad += 40)
    {
        if (current <= 0)
        {
            if (group == 3)
                return (uint32)current;
            current = r_s32(depths + neighbor[group] * 4);
            continue;
        }
        current = r_s32(depths + neighbor[group] * 4);
        if (current <= 0)
        {
            if (group == 3)
                return (uint32)current;
            continue;
        }
        w_u32(triangle, 0x07000000);
        w_u16(triangle + 22, texture_22);
        w_u16(triangle + 14, texture_14);
        w_u32(triangle + 4, color);
        for (corner = 0; corner < 3; ++corner)
        {
            address = source + triangle_uv[group][corner];
            destination = triangle + corner * 8;
            value = r_u8(address);
            if (corner == 0)
                tag = r_u32(triangle);
            w_u8(destination + 12, value);
            value = r_u8(address + 1);
            w_u8(destination + 13, value);
            value = r_u16(address - 4);
            w_u16(destination + 8, value);
            value = r_u16(address - 2);
            w_u16(destination + 10, value);
        }
        value = r_u32(ordering);
        w_u32(triangle, (tag & 0xFF000000) | (value & 0xFFFFFF));
        value = r_u32(ordering);
        w_u32(ordering, (value & 0xFF000000) | (triangle & 0xFFFFFF));
        w_u16(quad + 22, texture_22);
        w_u16(quad + 14, texture_14);
        w_u32(quad, 0x09000000);
        w_u32(quad + 4, quad_color);
        for (corner = 0; corner < 4; ++corner)
        {
            address = source + quad_uv[group][corner];
            destination = quad + corner * 8;
            value = r_u8(address);
            w_u8(destination + 12, value);
            value = r_u8(address + 1);
            w_u8(destination + 13, value);
            value = r_u16(address - 4);
            w_u16(destination + 8, value);
            value = r_u16(address - 2);
            w_u16(destination + 10, value);
        }
        tag = r_u32(quad);
        value = r_u32(ordering);
        w_u32(quad, (tag & 0xFF000000) | (value & 0xFFFFFF));
        value = r_u32(ordering);
        result = (value & 0xFF000000) | (quad & 0xFFFFFF);
        w_u32(ordering, result);
        if (group == 3)
            return result;
        current = r_s32(depths + neighbor[group] * 4);
    }
    return result;
}

void td5_80073ad0(uint32 packet, uint32 ordering, uint32 vertices, sint32 depth, uint32 original_stack)
{
    uint32 frame = original_stack - 0x210;
    uint32 shift, other_shift, level, scale, reserve, allocation, limit;
    uint32 buffer, output, header, color, texture_22, texture_14;
    uint32 edge[2][3], edge_step[2][3], edge_uv[2][2], edge_uv_step[2][2];
    uint32 current[3], across[3], current_uv[2], across_uv[2];
    sint32 screen[4][2];
    uint32 clip[4];
    uint32 side, component, row, column, divisions;
    uint32 row_base, previous_row, point, old_left, old_right, new_left;
    uint32 value, other, tag, bucket;
    sint32 width, height, threshold, projected_depth;

    FUNCTION_MARKER(0x80073AD0, "SLUS_006.10");
    shift = r_u32(0x80088110);
    other_shift = r_u32(0x80088164);
    threshold = r_s32(0x80088134);
    shift = (shift - other_shift) & 31;
    if (depth >= threshold / 4)
        goto link_original;
    threshold = r_s32(0x80088138);
    level = depth < threshold / 4 ? 2 : 1;
    scale = level * 2;
    allocation = r_u32(0x80088514);
    limit = r_u32(0x800884FC);
    reserve = 40 << scale;
    if (limit < allocation + reserve)
        goto link_original;
    for (side = 0; side < 4; ++side)
    {
        screen[side][0] = r_s16(packet + 8 + side * 8);
        screen[side][1] = r_s16(packet + 10 + side * 8);
    }
    width = r_s32(0x800880F4);
    height = r_s32(0x800880F8);
    for (side = 0; side < 4; ++side)
    {
        if (side == 3)
            width = r_s32(0x800880F4);
        clip[side] = screen[side][0] < 0 ? 1 : 0;
        if (width < screen[side][0])
            clip[side] |= 2;
        if (screen[side][1] < 0)
            clip[side] |= 4;
        if (side == 3)
            height = r_s32(0x800880F8);
        if (height < screen[side][1])
            clip[side] |= 8;
    }
    if ((clip[0] & clip[1] & clip[2] & clip[3]) != 0)
        return;
    for (component = 0; component < 4; ++component)
        w_u32(frame + 0x180 + component * 4, xport_gte_read_data(16 + component));
    buffer = r_u32(0x80088104);
    allocation = r_u32(0x80088514);
    w_u32(0x80088514, allocation + reserve);
    color = r_u32(packet + 4);
    buffer = r_u32(0x80088370 + buffer * 4);
    header = r_u32(packet);
    texture_22 = r_u16(packet + 22);
    value = (uint32)r_s16(vertices);
    texture_14 = r_u16(packet + 14);
    edge[0][0] = value << scale;
    for (component = 1; component < 3; ++component)
        edge[0][component] = (uint32)r_s16(vertices + component * 2) << scale;
    for (component = 0; component < 2; ++component)
        edge_uv[0][component] = (uint16)(r_u8(packet + 12 + component) << scale);
    for (component = 0; component < 3; ++component)
        edge[1][component] = (uint32)r_s16(vertices + 8 + component * 2) << scale;
    for (component = 0; component < 2; ++component)
        edge_uv[1][component] = (uint16)(r_u8(packet + 20 + component) << scale);
    for (side = 0; side < 2; ++side)
    {
        for (component = 0; component < 3; ++component)
        {
            value = (uint32)r_s16(vertices + 16 + side * 8 + component * 2);
            other = (uint32)r_s16(vertices + side * 8 + component * 2);
            edge_step[side][component] = (value - other) << level;
        }
        for (component = 0; component < 2; ++component)
        {
            value = r_u8(packet + 28 + side * 8 + component);
            other = r_u8(packet + 12 + side * 8 + component);
            edge_uv_step[side][component] = (uint16)((value - other) << level);
        }
    }
    output = buffer + allocation;
    divisions = 1 << level;
    for (row = 0; row <= divisions; ++row)
    {
        if (row != 0)
        {
            for (side = 0; side < 2; ++side)
            {
                for (component = 0; component < 3; ++component)
                    edge[side][component] += edge_step[side][component];
                for (component = 0; component < 2; ++component)
                    edge_uv[side][component] = (uint16)(edge_uv[side][component] + edge_uv_step[side][component]);
            }
        }
        for (component = 0; component < 3; ++component)
        {
            current[component] = edge[0][component];
            across[component] = (uint32)((sint32)(edge[1][component] - edge[0][component]) >> level);
        }
        for (component = 0; component < 2; ++component)
        {
            current_uv[component] = edge_uv[0][component];
            value = (uint32)(sint32)(sint16)edge_uv[1][component];
            other = (uint32)(sint32)(sint16)edge_uv[0][component];
            across_uv[component] = (uint16)((sint32)(value - other) >> level);
        }
        row_base = frame + 0x10 + (row & 1) * 80;
        previous_row = frame + 0x10 + ((row - 1) & 1) * 80;
        for (column = 0; column <= divisions; ++column)
        {
            if (column != 0)
            {
                for (component = 0; component < 3; ++component)
                    current[component] += across[component];
                for (component = 0; component < 2; ++component)
                    current_uv[component] = (uint16)(current_uv[component] + across_uv[component]);
            }
            point = row_base + column * 16;
            for (component = 0; component < 3; ++component)
                w_u16(point + component * 2, (sint32)current[component] >> scale);
            for (component = 0; component < 2; ++component)
                w_u8(point + 12 + component, (sint32)(sint16)current_uv[component] >> scale);
            if (row == 0 || column == 0)
                continue;
            old_left = previous_row + (column - 1) * 16;
            old_right = old_left + 16;
            new_left = point - 16;
            w_u16(output + 22, texture_22);
            w_u16(output + 14, texture_14);
            w_u32(output, header);
            w_u32(output + 4, color);
            value = r_u8(old_left + 12);
            w_u8(output + 12, value);
            value = r_u8(old_left + 13);
            w_u8(output + 13, value);
            value = r_u8(old_right + 12);
            w_u8(output + 20, value);
            value = r_u8(old_right + 13);
            w_u8(output + 21, value);
            value = r_u8(new_left + 12);
            w_u8(output + 28, value);
            value = r_u8(new_left + 13);
            w_u8(output + 29, value);
            value = r_u8(point + 12);
            w_u8(output + 36, value);
            value = r_u8(point + 13);
            w_u8(output + 37, value);
            value = r_u32(old_left);
            xport_gte_write_data(0, value);
            value = r_u32(old_left + 4);
            xport_gte_write_data(1, value);
            value = r_u32(old_right);
            xport_gte_write_data(2, value);
            value = r_u32(old_right + 4);
            xport_gte_write_data(3, value);
            value = r_u32(new_left);
            xport_gte_write_data(4, value);
            value = r_u32(new_left + 4);
            xport_gte_write_data(5, value);
            xport_gte_execute(0x280030);
            w_u32(output + 8, xport_gte_read_data(12));
            w_u32(output + 16, xport_gte_read_data(13));
            w_u32(output + 24, xport_gte_read_data(14));
            value = r_u32(point);
            xport_gte_write_data(0, value);
            value = r_u32(point + 4);
            xport_gte_write_data(1, value);
            xport_gte_execute(0x180001);
            xport_gte_execute(0x168002E);
            w_u32(frame + 0x190, xport_gte_read_data(7));
            projected_depth = r_s32(frame + 0x190);
            if (projected_depth > 0)
            {
                w_u32(output + 32, xport_gte_read_data(14));
                bucket = ordering + ((uint32)(projected_depth >> shift) << 2);
                tag = r_u32(output);
                value = r_u32(bucket);
                w_u32(output, (tag & 0xFF000000) | (value & 0xFFFFFF));
                value = r_u32(bucket);
                w_u32(bucket, (value & 0xFF000000) | (output & 0xFFFFFF));
            }
            output += 40;
        }
    }
    bucket = ordering + ((uint32)((sint32)((uint32)depth + 256) >> shift) << 2);
    if (level == 1)
        td5_80072e84(packet, bucket, frame + 0x180);
    else
        td5_800732a0(packet, bucket, frame + 0x180);
    return;

link_original:
    bucket = ordering + ((uint32)(depth >> shift) << 2);
    tag = r_u32(packet);
    value = r_u32(bucket);
    w_u32(packet, (tag & 0xFF000000) | (value & 0xFFFFFF));
    value = r_u32(bucket);
    w_u32(bucket, (value & 0xFF000000) | (packet & 0xFFFFFF));
}

uint32 td5_80076f44(uint32 object, uint32 original_stack)
{
    uint32 frame = original_stack - 0xE0;
    uint32 radius;
    uint32 value;
    uint32 shift;
    uint32 packet;
    uint32 stream;
    uint32 vertex_base;
    uint32 buffer;
    uint32 ordering;
    uint32 result;
    uint32 kind;
    uint32 copy_vectors;
    uint32 vectors;
    uint32 vertex_count;
    uint32 vertex_index;
    uint32 source;
    uint32 destination;
    uint32 word_index;
    uint32 high;
    uint32 low;
    uint32 mask;
    uint32 address;
    uint32 words[2];
    sint32 count;
    sint32 index;
    sint32 area;
    sint32 depth;
    sint32 far_plane;
    sint32 bias;

    FUNCTION_MARKER(0x80076F44, "SLUS_006.10");
    radius = (uint32)r_s16(object + 8);
    value = r_u32(0x800883D4);
    far_plane = r_s32(0x80088134);
    if ((sint32)(value - radius) >= far_plane)
        return td5_80071544(object);
    value = r_u32(0x80088104);
    packet = object + r_u32(object + 0x34 + value * 4);
    stream = object + r_u32(object + 0x30);
    shift = r_u32(0x80088164);
    vertex_base = object + r_u32(object + 0x2C);
    value = r_u32(0x80088110);
    buffer = r_u32(0x800880F0);
    bias = r_s16(object + 12);
    shift = (value - shift) & 31;
    ordering = r_u32(buffer + 0x70);
    value = r_u8(stream + 3);
    result = (uint32)(bias >> shift) << 2;
    ordering += result;
    if (value == 0)
        return result;
    w_u32(frame + 0x90, frame + 0x10);
    do
    {
        kind = r_u8(stream + 2);
        count = r_s16(stream);
        stream += 4;
        if (kind <= 3 && count > 0)
        {
            if (kind == 0)
                copy_vectors = frame + 0x10;
            else if (kind == 1)
                copy_vectors = frame + 0x30;
            else if (kind == 2)
                copy_vectors = frame + 0x58;
            else
                copy_vectors = frame + 0x70;
            vertex_count = (kind & 1) != 0 ? 4 : 3;
            for (index = 0; index < count; ++index)
            {
                for (vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
                {
                    value = (uint32)r_s16(stream + vertex_index * 2);
                    source = vertex_base + (value << 3);
                    // Complete LWL/LWR pairs retain unaligned vertex words
                    for (word_index = 0; word_index < 2; ++word_index)
                    {
                        address = source + word_index * 4;
                        high = r_u32((address + 3) & ~3);
                        low = r_u32(address & ~3);
                        shift = (address & 3) * 8;
                        words[word_index] = shift == 0 ? low : (high << (32 - shift)) | (low >> shift);
                    }
                    // Each original SWL precedes its matching SWR
                    for (word_index = 0; word_index < 2; ++word_index)
                    {
                        destination = copy_vectors + vertex_index * 8 + word_index * 4;
                        address = destination + 3;
                        shift = (3 - (address & 3)) * 8;
                        mask = 0xFFFFFFFF >> shift;
                        value = r_u32(address & ~3);
                        w_u32(address & ~3, (value & ~mask) | (words[word_index] >> shift));
                        shift = (destination & 3) * 8;
                        mask = 0xFFFFFFFF << shift;
                        value = r_u32(destination & ~3);
                        w_u32(destination & ~3, (value & ~mask) | (words[word_index] << shift));
                    }
                }
                vectors = kind == 0 ? r_u32(frame + 0x90) : copy_vectors;
                for (word_index = 0; word_index < 6; ++word_index)
                {
                    value = r_u32(vectors + word_index * 4);
                    xport_gte_write_data(word_index, value);
                }
                xport_gte_execute(0x280030);
                xport_gte_execute(0x1400006);
                w_u32(frame + 0x28, xport_gte_read_data(24));
                if ((kind & 1) != 0)
                {
                    w_u32(packet + 8, xport_gte_read_data(12));
                    w_u32(packet + (kind == 1 ? 16 : 20), xport_gte_read_data(13));
                    w_u32(packet + (kind == 1 ? 24 : 32), xport_gte_read_data(14));
                    value = r_u32(copy_vectors + 24);
                    xport_gte_write_data(0, value);
                    value = r_u32(copy_vectors + 28);
                    xport_gte_write_data(1, value);
                    xport_gte_execute(0x180001);
                    xport_gte_execute(0x1400006);
                    w_u32(frame + 0x50, xport_gte_read_data(24));
                }
                if (kind == 2)
                    xport_gte_execute(0x158002D);
                else if (kind == 3)
                    xport_gte_execute(0x168002E);
                area = r_s32(frame + 0x28);
                if (area <= 0 || ((kind & 1) != 0 && r_s32(frame + 0x50) >= 0))
                {
                    if (kind == 0)
                        xport_gte_execute(0x158002D);
                    else if (kind == 1)
                        xport_gte_execute(0x168002E);
                    if ((kind & 1) != 0)
                        w_u32(packet + (kind == 1 ? 32 : 44), xport_gte_read_data(14));
                    else
                    {
                        w_u32(packet + 8, xport_gte_read_data(12));
                        w_u32(packet + (kind == 0 ? 16 : 20), xport_gte_read_data(13));
                        w_u32(packet + (kind == 0 ? 24 : 32), xport_gte_read_data(14));
                    }
                    w_u32(frame + 0x2C, xport_gte_read_data(7));
                    depth = r_s32(frame + 0x2C);
                    if (depth >= 0)
                    {
                        shift = r_u32(0x80088164);
                        far_plane = r_s32(0x80088140);
                        if (depth < (far_plane >> ((shift + 2) & 31)))
                        {
                            if (kind == 0)
                            {
                                vectors = r_u32(frame + 0x90);
                                td5_800724ac(packet, ordering, vectors, depth);
                            }
                            else if (kind == 1)
                                td5_80073ad0(packet, ordering, vectors, depth, frame);
                            else if (kind == 2)
                                td5_8007460c(packet, ordering, vectors, depth);
                            else
                                td5_800760b8(packet, ordering, vectors, depth, frame);
                        }
                    }
                }
                packet += kind == 0 ? 32 : kind == 3 ? 52 : 40;
                stream += kind <= 1 ? 20 : kind == 2 ? 44 : 56;
            }
        }
        result = r_u8(stream + 3);
    } while (result != 0);
    return result;
}

uint32 td5_80079094(uint32 position, sint32 roll)
{
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 value;
    uint32 origin;
    uint32 absolute;
    uint32 half;
    uint32 normalized_x;
    uint32 normalized_y;
    uint32 normalized_z;
    uint32 first;
    uint32 second;
    uint32 third;
    uint32 fourth;
    uint32 cosine;
    uint32 sine;
    sint32 length;
    sint32 horizontal;
    MATRIX rotation = {0};
    MATRIX flip = {0};
    MATRIX *matrix;

    FUNCTION_MARKER(0x80079094, "SLUS_006.10");
    value = r_u32(position);
    origin = r_u32(0x8009D93C);
    x = (uint32)((sint32)value >> 8) - origin;
    value = r_u32(position + 4);
    origin = r_u32(0x8009D940);
    y = (uint32)((sint32)value >> 8) - origin;
    value = r_u32(position + 8);
    origin = r_u32(0x8009D944);
    z = (uint32)((sint32)value >> 8) - origin;
    for (;;)
    {
        absolute = (sint32)x < 0 ? 0 - x : x;
        if ((sint32)absolute < 16385)
        {
            absolute = (sint32)y < 0 ? 0 - y : y;
            if ((sint32)absolute < 16385)
            {
                absolute = (sint32)z < 0 ? 0 - z : z;
                if ((sint32)absolute < 16385)
                    break;
            }
        }
        x = (uint32)((sint32)x / 2);
        y = (uint32)((sint32)y / 2);
        z = (uint32)((sint32)z / 2);
    }
    length = (sint32)td5_integer_sqrt(x * x + y * y + z * z);
    value = x << 12;
    if (length <= 0)
        return value;
    half = (uint32)(length / 2);
    normalized_x = (uint32)td5_divide_checked((sint32)(value + half), length);
    normalized_z = (uint32)td5_divide_checked((sint32)((z << 12) + half), length);
    value = normalized_x * normalized_x + normalized_z * normalized_z;
    normalized_y = (uint32)td5_divide_checked((sint32)((y << 12) + half), length);
    horizontal = (sint32)td5_integer_sqrt(value);
    if (horizontal > 0)
    {
        half = (uint32)(horizontal / 2);
        first = (uint32)td5_divide_checked((sint32)((normalized_z << 12) + half), horizontal);
        value = 0 - ((normalized_x << 12) + half);
        second = (uint32)td5_divide_checked((sint32)value, horizontal);
        value = 0 - (normalized_x * normalized_y + half);
        third = (uint32)td5_divide_checked((sint32)value, horizontal);
        value = 0 - (normalized_y * normalized_z + half);
        fourth = (uint32)td5_divide_checked((sint32)value, horizontal);
        w_u16(0x8009D92A, 0);
        w_u16(0x8009D930, horizontal);
        w_u16(0x8009D934, normalized_x);
        w_u16(0x8009D936, normalized_y);
        w_u16(0x8009D938, normalized_z);
        w_u16(0x800A3B92, 0);
        w_u16(0x800A3B94, 0);
        w_u16(0x800A3B96, 0);
        w_u16(0x800A3B98, horizontal);
        w_u16(0x800A3B9C, 0);
        w_u16(0x800A3B9E, normalized_y);
        w_u16(0x800A3BA0, horizontal);
        w_u16(0x800A3B90, 4096);
        w_u16(0x800A3B9A, 0 - normalized_y);
        w_u16(0x8009D928, first);
        w_u16(0x8009D92C, second);
        w_u16(0x8009D92E, third);
        w_u16(0x8009D932, fourth);
    }
    else
    {
        w_u16(0x8009D928, 4096);
        w_u16(0x8009D92A, 0);
        w_u16(0x8009D92C, 0);
        w_u16(0x8009D92E, 0);
        w_u16(0x8009D930, 0);
        w_u16(0x8009D932, 0 - normalized_y);
        w_u16(0x8009D934, 0);
        w_u16(0x8009D936, normalized_y);
        w_u16(0x8009D938, 0);
    }
    cosine = (uint32)rcos(roll);
    sine = (uint32)rsin(roll);
    rotation.m[0][0] = (sint16)cosine;
    rotation.m[1][1] = (sint16)cosine;
    rotation.m[0][1] = (sint16)(0 - sine);
    rotation.m[0][2] = 0;
    rotation.m[1][0] = (sint16)sine;
    rotation.m[1][2] = 0;
    rotation.m[2][0] = 0;
    rotation.m[2][1] = 0;
    rotation.m[2][2] = 4096;
    matrix = (MATRIX *)psx_addr(0x8009D928, 20);
    MulMatrix2(&rotation, matrix);
    flip.m[0][0] = -4096;
    flip.m[0][1] = 0;
    flip.m[0][2] = 0;
    flip.m[1][0] = 0;
    flip.m[1][1] = 4096;
    flip.m[1][2] = 0;
    flip.m[2][0] = 0;
    flip.m[2][1] = 0;
    flip.m[2][2] = -4096;
    matrix = (MATRIX *)psx_addr(0x800A3B90, 20);
    MulMatrix2(&rotation, matrix);
    MulMatrix0(matrix, &flip, matrix);
    MulMatrix0(&rotation, &flip, (MATRIX *)psx_addr(0x8009FCF8, 20));
    return (uint32)td5_scale_view_matrices();
}

uint32 td5_8001e22c(uint32 car, sint32 angle, uint32 view, uint32 original_stack)
{
    uint32 frame = original_stack - 0xB8;
    uint32 offset = view * 4;
    uint32 cosine;
    uint32 sine;
    uint32 value;
    uint32 first;
    uint32 second;
    uint32 third;
    uint32 product;
    uint32 rate;
    uint32 previous;
    sint32 scaled;
    sint32 direction;

    FUNCTION_MARKER(0x8001E22C, "SLUS_006.10");
    cosine = (uint32)rcos(angle);
    sine = (uint32)rsin(angle) << 5;
    value = r_u32(car + 0x1C4);
    w_u32(frame + 0x10, value + sine);
    value = r_u32(car + 0x1CC);
    cosine <<= 5;
    w_u32(frame + 0x18, value + cosine);
    value = r_u32(car + 0x1C4);
    w_u32(frame + 0x1C, value - cosine);
    value = r_u32(car + 0x1CC);
    w_u32(frame + 0x24, value + sine);
    value = r_u32(car + 0x1C4);
    w_u32(frame + 0x28, value + cosine);
    value = r_u32(car + 0x1CC);
    w_u32(frame + 0x30, value - sine);
    value = r_u16(car + 0x70);
    w_u16(frame + 0x54, value);
    w_u16(frame + 0x46, value);
    w_u16(frame + 0x38, value);
    value = r_u8(car + 0x7C);
    w_u8(frame + 0x60, value);
    w_u8(frame + 0x52, value);
    w_u8(frame + 0x44, value);
    // The original height inputs remain in guest stack memory
    td5_8001b310(frame + 0x38, frame + 0x10);
    td5_8001cd00(frame + 0x38, frame + 0x10, frame + 0x14);
    td5_8001b310(frame + 0x46, frame + 0x1C);
    td5_8001cd00(frame + 0x46, frame + 0x1C, frame + 0x20);
    td5_8001b310(frame + 0x54, frame + 0x28);
    td5_8001cd00(frame + 0x54, frame + 0x28, frame + 0x2C);
    second = r_u32(frame + 0x20);
    third = r_u32(frame + 0x2C);
    first = r_u32(frame + 0x14);
    value = first - (uint32)((sint32)(second + third) / 2);
    direction = td5_8007b0c0((sint32)(0 - (uint32)((sint32)value >> 8)), 512);
    second = r_u32(frame + 0x20);
    third = r_u32(frame + 0x2C);
    w_u16(frame + 0x68, direction);
    w_u16(frame + 0x6A, angle);
    value = second - third;
    direction = td5_8007b0c0((sint32)(0 - (uint32)((sint32)value >> 8)), 1024);
    w_u16(frame + 0x6C, direction);
    td5_80079548(frame + 0x70, frame + 0x68);
    value = r_u32(0x80088598 + offset);
    first = r_u32(0x800883C0 + offset);
    second = (uint32)r_s16(car + 0x1D2);
    direction = (sint32)(first + (uint32)((sint32)value >> 8) - second);
    sine = (uint32)rsin(direction);
    value = r_u32(0x800881A8 + offset);
    product = (uint32)((sint32)value / 256) * sine;
    w_u16(frame + 0x90, (sint32)product / 4096);
    value = r_u32(0x800881A0 + offset);
    w_u16(frame + 0x92, (sint32)value / 256);
    value = r_u32(0x80088598 + offset);
    first = r_u32(0x800883C0 + offset);
    second = (uint32)r_s16(car + 0x1D2);
    direction = (sint32)(first + (uint32)((sint32)value >> 8) - second);
    cosine = (uint32)rcos(direction);
    value = r_u32(0x800881A8 + offset);
    product = (uint32)((sint32)(0 - value) / 256) * cosine;
    w_u16(frame + 0x94, (sint32)product / 4096);
    first = r_u32(frame + 0x70);
    second = r_u32(frame + 0x74);
    xport_gte_write_control(0, first);
    xport_gte_write_control(1, second);
    first = r_u32(frame + 0x78);
    second = r_u32(frame + 0x7C);
    third = r_u32(frame + 0x80);
    xport_gte_write_control(2, first);
    xport_gte_write_control(3, second);
    xport_gte_write_control(4, third);
    value = r_u32(frame + 0x90);
    xport_gte_write_data(0, value);
    value = r_u32(frame + 0x94);
    xport_gte_write_data(1, value);
    xport_gte_execute(0x486012);
    first = xport_gte_read_data(9);
    second = xport_gte_read_data(10);
    third = xport_gte_read_data(11);
    w_u16(frame + 0x90, first);
    w_u16(frame + 0x92, second);
    w_u16(frame + 0x94, third);
    value = (uint32)r_s16(frame + 0x92);
    previous = r_u32(0x800883A8 + offset);
    value = (value << 8) - previous;
    rate = r_u32(0x80088100);
    product = (uint32)((sint32)value / 8) * rate;
    w_u32(0x800883A8 + offset, previous + (uint32)((sint32)product / 256));
    first = (uint32)r_s16(frame + 0x90);
    second = (uint32)r_s16(frame + 0x94);
    value = td5_integer_sqrt(first * first + second * second);
    previous = r_u32(0x80088188 + offset);
    value = (value << 8) - previous;
    rate = r_u32(0x80088100);
    product = (uint32)((sint32)value / 8) * rate;
    w_u32(0x80088188 + offset, previous + (uint32)((sint32)product / 256));
    value = r_u32(0x800881B0 + offset);
    previous = r_u32(0x800881B8 + offset);
    scaled = (sint32)(value - previous) / 8;
    product = (uint32)scaled * rate;
    value = previous + (uint32)((sint32)product / 256);
    w_u32(0x800881B8 + offset, value);
    return value;
}

uint32 td5_8001e654(uint32 car, uint32 update, uint32 view, uint32 original_stack)
{
    uint32 offset = view * 4;
    uint32 displacement = 0x800887DC + view * 16;
    uint32 eye = 0x8009B3E0 + view * 12;
    uint32 target = original_stack - 0x30;
    uint32 value;
    uint32 other;
    uint32 product;
    uint32 base;
    uint32 heading;
    uint32 phase;
    sint32 counter;
    sint32 angle;
    sint32 cosine;
    sint32 sine;
    sint32 scaled;
    sint32 error;
    sint32 speed;
    sint32 limit;

    FUNCTION_MARKER(0x8001E654, "SLUS_006.10");
    if (r_u8(car + 0x351) != 0)
    {
        counter = r_s32(0x80088458 + offset);
        if (counter >= 6)
            w_u32(0x80088458 + offset, (uint32)counter - 1);
    }
    else
    {
        counter = r_s32(0x80088458 + offset);
        if (counter < r_s32(0x80087784))
        {
            w_u32(0x80088458 + offset, (uint32)counter + 1);
            counter = r_s32(0x80088458 + offset);
        }
        if (r_s32(0x80087784) < counter)
            w_u32(0x80088458 + offset, (uint32)counter - 1);
    }
    w_u32(0x800881C0 + offset, 0);
    phase = r_u32(0x80088598 + offset);
    base = r_u32(0x800883C0 + offset);
    angle = (sint32)(base + (uint32)((sint32)(0 - phase) >> 8));
    cosine = rcos(angle);
    sine = rsin(angle);
    scaled = r_s32(0x80088188 + offset) / 16;
    product = (uint32)scaled * (uint32)sine;
    value = r_u32(0x800883A8 + offset);
    w_u32(displacement, (uint32)((sint32)product / 256));
    w_u32(displacement + 4, value);
    value = r_u32(0x80088188 + offset);
    scaled = (sint32)(0 - value) / 16;
    product = (uint32)scaled * (uint32)cosine;
    w_u32(displacement + 8, (uint32)((sint32)product / 256));
    value = r_u32(0x80088190 + offset);
    product = (uint32)((sint32)value >> 8) * (uint32)cosine;
    value = r_u32(car + 0x1C4);
    other = r_u32(displacement);
    w_u32(eye, value + other + (uint32)((sint32)product / 256));
    value = r_u32(car + 0x1C8);
    other = r_u32(displacement + 4);
    w_u32(eye + 4, value + other);
    value = r_u32(0x80088190 + offset);
    product = (uint32)((sint32)value >> 8) * (uint32)sine;
    value = r_u32(car + 0x1CC);
    other = r_u32(displacement + 8);
    w_u32(eye + 8, value + other + (uint32)((sint32)product / 256));
    value = r_u32(0x800881C0 + offset);
    phase = r_u32(0x80088598 + offset);
    angle = (sint32)((value << 11) + (uint32)((sint32)phase >> 8));
    td5_8001e22c(car, angle, view, original_stack - 0x40);
    if (update != 0)
    {
        value = r_u32(0x800881C0 + offset);
        heading = (uint32)r_s16(car + 0x1D2);
        phase = r_u32(0x80088598 + offset);
        value = (((value << 11) + heading) << 8) - phase;
        value &= 0xFFFFF;
        if (value > 0x7FFFF)
            value -= 0x100000;
        error = (sint32)value;
        counter = r_s32(0x80088458 + offset);
        speed = error >> 11;
        if (speed < 0)
            speed = -speed;
        limit = speed + 5;
        if (limit < 15)
            limit = 15;
        if (counter < limit)
        {
            counter = r_s32(0x80088458 + offset);
            product = value * (uint32)counter;
        }
        else
            product = value * (uint32)limit;
        other = r_u32(0x80088100);
        product = (uint32)((sint32)product >> 8) * other;
        value = r_u32(0x80088598 + offset);
        w_u32(0x80088598 + offset, value + (uint32)((sint32)product / 256));
    }
    value = r_u32(car + 0x1C4);
    w_u32(target, value);
    value = r_u32(car + 0x1C8);
    other = r_u32(0x800881B8 + offset);
    w_u32(target + 4, value + other);
    value = r_u32(car + 0x1CC);
    w_u32(target + 8, value);
    td5_set_view_position(eye);
    angle = r_s32(0x80087790 + offset);
    td5_80079094(target, angle);
    value = (uint32)r_s16(car + 0x70);
    w_u32(0x80088720 + offset, value);
    return value;
}

uint32 td5_8002e70c(uint32 slot, uint32 original_stack)
{
    uint32 car = 0x800984A0 + slot * 864;
    uint32 object;
    uint32 value;
    uint32 camera;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 result;

    FUNCTION_MARKER(0x8002E70C, "SLUS_006.10");
    value = r_u32(car + 0x1C4);
    camera = r_u32(0x8009765C);
    x = (uint32)((sint32)value / 256) - camera;
    x = (uint32)((sint32)x / 16);
    value = r_u32(car + 0x1C8);
    camera = r_u32(0x80097660);
    y = (uint32)((sint32)value / 256) - camera;
    y = (uint32)((sint32)y / 16);
    value = r_u32(car + 0x1CC);
    camera = r_u32(0x80097664);
    z = (uint32)((sint32)value / 256) - camera;
    z = (uint32)((sint32)z / 16);
    result = SquareRoot0((sint32)(x * x + y * y + z * z));
    if ((sint32)result < 1125 || r_u32(0x80087738) != 0)
        object = r_u32(car + 0x178);
    else
        object = r_u32(car + 0x17C);
    x = r_u32(car + 0x1C4);
    y = r_u32(car + 0x1C8);
    z = r_u32(car + 0x1CC);
    w_u32(object + 0x10, x);
    w_u32(object + 0x14, y);
    w_u32(object + 0x18, z);
    result = td5_80079c90(object);
    if (result == 0)
        return result;
    td5_800314b8(car, original_stack - 0x30);
    td5_800797fc(car + 0x110, object);
    td5_8006e180(object, original_stack - 0x30);
    td5_80043328(slot);
    td5_80040060(slot, original_stack - 0x30);
    td5_8004099c(slot, original_stack - 0x30);
    td5_80041114(slot);
    td5_80033610(slot);
    td5_80040e94();
    if (r_u8(car + 0x351) != 0)
        td5_80032984(car, original_stack - 0x30);
    else
        td5_800321d0(slot, original_stack - 0x30);
    value = r_u32(0x80087724);
    result = slot * 2;
    if (value == 0)
        return result;
    result = (uint32)r_s16(0x8009DBA0 + result);
    if (result == 0)
        result = td5_8003d288(car, car + 0x1C4, 0);
    return result;
}

void td5_800314b8(uint32 car, uint32 original_stack)
{
    uint32 frame = original_stack - 0x80;
    uint32 descriptor;
    uint32 kind;
    uint32 red;
    uint32 green;
    uint32 blue;
    uint32 step;
    uint32 segments;
    uint32 segment;
    uint32 current_segment;
    uint32 vertices;
    uint32 vertex;
    uint32 other_vertex;
    uint32 vertex_index;
    uint32 far_index;
    uint32 selector;
    uint32 height;
    uint32 vector;
    uint32 point;
    uint32 value;
    uint32 position;
    uint32 component;
    uint32 light;
    uint32 norm;
    sint32 node;
    sint32 origin;
    sint32 rounded;
    sint32 span;
    sint32 scaled;
    sint32 strength;
    sint32 channel;
    SVECTOR normal;

    FUNCTION_MARKER(0x800314B8, "SLUS_006.10");
    value = r_u8(car + 0x34F);
    descriptor = r_u32(0x80088488) + value * 32;
    node = r_s16(car + 0x70);
    while (node < r_s16(descriptor))
    {
        descriptor -= 32;
        value = r_u8(car + 0x34F);
        w_u8(car + 0x34F, value - 1);
    }
    while (r_s16(descriptor + 2) < node)
    {
        value = r_u8(car + 0x34F);
        descriptor += 32;
        w_u8(car + 0x34F, value + 1);
    }
    value = r_u8(car + 0x33E);
    if (value != r_u32(descriptor + 28))
        w_u8(car + 0x33F, 2);
    kind = r_u8(descriptor + 24);
    if (kind == 0)
    {
        red = r_u8(descriptor + 16);
        green = r_u8(descriptor + 17);
        blue = r_u8(descriptor + 18);
        td5_set_back_color(red, green, blue);
        red = r_u8(descriptor + 12);
        green = r_u8(descriptor + 13);
        blue = r_u8(descriptor + 14);
        td5_set_light(0, descriptor + 4, red, green, blue);
        td5_set_light(1, descriptor + 4, 0, 0, 0);
        td5_set_light(2, descriptor + 4, 0, 0, 0);
        td5_apply_light_matrix(car + 0x110);
        return;
    }
    if (kind == 3)
    {
        span = r_s16(descriptor + 2);
        origin = r_s16(descriptor);
        span = span - origin + 1;
        if (node - origin < span / 2)
            td5_80030d68(car, descriptor, (sint32)((uint32)span << 9));
        else
            td5_8003110c(car, descriptor, (sint32)((uint32)span << 9));
        return;
    }
    if (kind != 1 && kind != 2)
        return;
    if (node - r_s16(descriptor) < 3)
    {
        td5_80030d68(car, descriptor, 2048);
        return;
    }
    if (r_s16(descriptor + 2) - node < 3)
    {
        td5_8003110c(car, descriptor, 2048);
        return;
    }
    origin = r_s16(descriptor + 20);
    step = r_u8(descriptor + 25);
    rounded = td5_divide_checked(node - origin + (sint32)(step >> 1), step);
    rounded = (sint32)((uint32)rounded * step + (uint32)origin);
    if (kind == 1)
    {
        red = r_u8(descriptor + 12);
        green = r_u8(descriptor + 13);
        segments = r_u32(0x800884C4);
        segment = segments + (uint32)rounded * 24;
        far_index = r_u8(segment + 3) & 15;
        vertex_index = r_u16(segment + 4);
        blue = r_u8(descriptor + 14);
        vertices = r_u32(0x8008850C);
        far_index += vertex_index;
        vertex = vertices + vertex_index * 6;
        height = 0;
        vector = frame + 0x18;
        for (light = 0; light < 2; ++light)
        {
            if (light != 0)
                vertex = r_u32(0x8008850C) + far_index * 6;
            for (component = 0; component < 3; ++component)
            {
                value = r_u32(segment + 12 + component * 4);
                value += r_u16(vertex + component * 2);
                position = r_u32(car + 0x1C4 + component * 4);
                if (light == 0 && component == 0)
                    height = r_u16(descriptor + 22);
                value -= (uint32)((sint32)position / 256);
                if (component == 1)
                    value += height;
                w_u16(vector + component * 2, value);
            }
            normal.vx = r_s16(vector);
            normal.vy = r_s16(vector + 2);
            normal.vz = r_s16(vector + 4);
            norm = VectorNormalSS(&normal, &normal);
            w_u16(vector, normal.vx);
            w_u16(vector + 2, normal.vy);
            w_u16(vector + 4, normal.vz);
            scaled = (sint32)(norm * r_u8(descriptor + 27));
            scaled /= 16384;
            strength = scaled < 4097 ? (sint32)(4096 - (uint32)scaled) : 0;
            channel = (sint32)(red * (uint32)strength) / 4096;
            value = (uint32)((sint32)(green * (uint32)strength) / 4096);
            position = (uint32)((sint32)(blue * (uint32)strength) / 4096);
            td5_set_light(light, vector, channel, value, position);
        }
        red = r_u8(descriptor + 16);
        green = r_u8(descriptor + 17);
        blue = r_u8(descriptor + 18);
        td5_set_back_color(red, green, blue);
        td5_set_light(2, descriptor + 4, 0, 0, 0);
        td5_apply_light_matrix(car + 0x110);
        return;
    }
    segments = r_u32(0x800884C4);
    red = r_u8(descriptor + 12);
    selector = r_u8(descriptor + 26);
    green = r_u8(descriptor + 13);
    current_segment = segments + (uint32)rounded * 24;
    w_u32(frame + 0x50, green);
    blue = r_u8(descriptor + 14);
    height = r_u16(descriptor + 22);
    if (selector < 3)
    {
        vertices = 0;
        for (light = 0; light < 3; ++light)
        {
            if (light == 0)
                segment = segments + ((uint32)rounded - step) * 24;
            else if (light == 1)
                segment = current_segment;
            else
                segment = segments + ((uint32)rounded + r_u8(descriptor + 25)) * 24;
            if (selector == 2)
            {
                far_index = r_u16(segment + 6) - 1;
                vertex_index = r_u16(segment + 4);
            }
            else if (selector == 1)
                vertex_index = r_u16(segment + 6) - 1;
            else
                vertex_index = r_u16(segment + 4);
            if (light == 0)
                vertices = r_u32(0x8008850C);
            vertex = vertices + vertex_index * 6;
            if (selector == 2)
                other_vertex = vertices + far_index * 6;
            point = frame + 0x20 + light * 12;
            for (component = 0; component < 3; ++component)
            {
                scaled = r_s16(vertex + component * 2);
                if (selector == 2)
                    scaled += r_s16(other_vertex + component * 2);
                value = r_u32(segment + 12 + component * 4);
                if (selector == 2)
                    scaled /= 2;
                w_u32(point + component * 4, (uint32)scaled + value);
            }
        }
    }
    // Unsupported selectors retain the original guest stack point data
    vector = frame + 0x48;
    for (light = 0; light < 3; ++light)
    {
        point = frame + 0x20 + light * 12;
        for (component = 0; component < 3; ++component)
        {
            position = r_u32(car + 0x1C4 + component * 4);
            value = r_u32(point + component * 4);
            value -= (uint32)((sint32)position / 256);
            if (component == 1)
                value += height;
            w_u16(vector + component * 2, value);
        }
        normal.vx = r_s16(vector);
        normal.vy = r_s16(vector + 2);
        normal.vz = r_s16(vector + 4);
        norm = VectorNormalSS(&normal, &normal);
        w_u16(vector, normal.vx);
        w_u16(vector + 2, normal.vy);
        w_u16(vector + 4, normal.vz);
        scaled = (sint32)(norm * r_u8(descriptor + 27));
        scaled /= 16384;
        strength = scaled < 4097 ? (sint32)(4096 - (uint32)scaled) : 0;
        channel = (sint32)(red * (uint32)strength) / 4096;
        green = r_u32(frame + 0x50);
        value = (uint32)((sint32)(green * (uint32)strength) / 4096);
        position = (uint32)((sint32)(blue * (uint32)strength) / 4096);
        td5_set_light(light, vector, channel, value, position);
    }
    red = r_u8(descriptor + 16);
    green = r_u8(descriptor + 17);
    blue = r_u8(descriptor + 18);
    td5_set_back_color(red, green, blue);
    td5_apply_light_matrix(car + 0x110);
}

void td5_80040e94(void)
{
    FUNCTION_MARKER(0x80040E94, "SLUS_006.10");
}

uint32 td5_80045388(uint32 car, uint32 original_stack)
{
    uint32 value;
    uint32 incoming_result = 1;
    uint32 model;
    uint32 flags;
    uint32 slot;
    sint32 amount;
    sint32 other;
    sint32 type;

    FUNCTION_MARKER(0x80045388, "SLUS_006.10");
    value = r_u32(0x8008873C);
    if (value != 0)
        return value;
    amount = r_s32(car + 0x2F8);
    other = r_s32(car + 0x2F4);
    if (amount < other)
        amount = other;
    if (r_u8(car + 0x34E) != 0)
        amount = 0x7FFF;
    if (r_u8(car + 0x354) == 15)
        amount = 0;
    slot = r_u8(car + 0x34D);
    td5_80053158(amount, slot);
    amount = r_s32(car + 0x2F8);
    td5_80044a2c(car, amount, original_stack - 0x18);
    amount = r_s32(car + 0x2F4);
    td5_80044cb4(car, amount, original_stack - 0x18);
    model = r_u32(car + 0x184);
    type = r_s16(model + 0x76);
    if (type == 2)
    {
        flags = r_u8(car + 0x34E);
        return td5_8004515c(car, flags, original_stack - 0x18, 1);
    }
    if (type < 3)
    {
        if (type != 1)
            return 1;
    }
    else
    {
        if (type != 3)
            return 3;
        flags = r_u8(car + 0x34E);
        incoming_result = td5_8004515c(car, flags, original_stack - 0x18, 3);
    }
    flags = r_u8(car + 0x34E);
    return td5_80044f3c(car, flags, original_stack - 0x18, incoming_result);
}

uint32 td5_8002ecf4(uint32 original_stack, uint32 texture_v_carrier, uint32 texture_u_carrier)
{
    uint32 selected;
    uint32 car;
    uint32 object;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 hidden;
    uint32 level;
    uint32 header;
    uint32 slot;
    sint32 count;

    FUNCTION_MARKER(0x8002ECF4, "SLUS_006.10");
    selected = r_u32(0x80088498);
    car = 0x800984A0 + selected * 864;
    object = r_u32(car + 0x178);
    x = r_u32(car + 0x1C4);
    y = r_u32(car + 0x1C8);
    z = r_u32(car + 0x1CC);
    w_u32(object + 0x10, x);
    w_u32(object + 0x14, y);
    w_u32(object + 0x18, z);
    hidden = r_u32(0x80088440);
    w_u32(0x800885A8, 0);
    if (hidden == 0)
    {
        td5_800314b8(car, original_stack - 0x20);
        if (td5_80079c90(object) != 0)
        {
            td5_800797fc(car + 0x110, object);
            level = r_u32(0x80088770);
            header = r_u32(level);
            if ((r_u8(header + 30) & 1) == 0)
                td5_8002fe8c(object, car, original_stack - 0x20, car, selected, texture_v_carrier, texture_u_carrier);
            td5_8006e180(object, original_stack - 0x20);
            if (r_u32(0x80087724) != 0 && r_u32(0x800885C8) != 0)
                td5_80041a44(car);
            td5_80040060(selected, original_stack - 0x20);
            td5_8004099c(selected, original_stack - 0x20);
            td5_80041114(selected);
            td5_80040ea4(car, original_stack - 0x20);
            td5_80033610(selected);
            td5_80040e94();
            if (r_u8(car + 0x351) != 0)
                td5_80032984(car, original_stack - 0x20);
            else
                td5_800321d0(selected, original_stack - 0x20);
        }
    }
    td5_80045388(car, original_stack - 0x20);
    if (r_u32(0x80087630) == 0)
    {
        count = r_s32(0x800875E4);
        slot = 0;
        for (;;)
        {
            if (count >= 7)
            {
                if ((sint32)slot >= 6)
                    break;
            }
            else if ((sint32)slot >= count)
                break;
            if (slot != r_u32(0x80088498))
                td5_8002e70c(slot, original_stack - 0x20);
            count = r_s32(0x800875E4);
            ++slot;
        }
    }
    if (r_u32(0x800883F8) == 0)
        return 0;
    slot = 6;
    count = r_s32(0x800875E4);
    if ((sint32)slot < count)
    {
        do
        {
            td5_8002e938(slot, original_stack - 0x20);
            count = r_s32(0x800875E4);
            ++slot;
        } while ((sint32)slot < count);
    }
    return 0;
}

uint32 td5_80079b84(uint32 object)
{
    VECTOR input;
    VECTOR output;
    uint32 value;
    uint32 origin;
    uint32 x;
    uint32 y;
    uint32 z;

    FUNCTION_MARKER(0x80079B84, "SLUS_006.10");
    value = r_u32(object + 0x10);
    origin = r_u32(0x8009765C);
    input.vx = (sint32)((uint32)((sint32)value >> 8) - origin);
    value = r_u32(object + 0x14);
    origin = r_u32(0x80097660);
    input.vy = (sint32)((uint32)((sint32)value >> 8) - origin);
    value = r_u32(object + 0x18);
    origin = r_u32(0x80097664);
    input.vz = (sint32)((uint32)((sint32)value >> 8) - origin);
    ApplyRotMatrixLV(&input, &output);
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

uint32 td5_80079e34(uint32 object)
{
    VECTOR input;
    VECTOR output;
    uint32 value;
    uint32 offset;
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

    FUNCTION_MARKER(0x80079E34, "SLUS_006.10");
    value = r_u32(object + 0x10);
    offset = r_u32(object + 0x1C);
    origin = r_u32(0x8009765C);
    radius = r_s16(object + 8);
    input.vx = (sint32)((uint32)((sint32)value >> 8) + offset - origin);
    value = r_u32(object + 0x14);
    offset = r_u32(object + 0x20);
    origin = r_u32(0x80097660);
    input.vy = (sint32)((uint32)((sint32)value >> 8) + offset - origin);
    value = r_u32(object + 0x18);
    offset = r_u32(object + 0x24);
    origin = r_u32(0x80097664);
    shift = r_u32(0x80088164);
    input.vz = (sint32)((uint32)((sint32)value >> 8) + offset - origin);
    radius >>= shift & 31;
    ApplyRotMatrixLV(&input, &output);
    depth = (uint32)output.vz;
    near_plane = r_s32(0x8008813C);
    shift = r_u32(0x80088164);
    if ((near_plane >> (shift & 31)) >= (sint32)(depth + (uint32)radius))
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

uint32 td5_80035390(uint32 original_stack)
{
    uint32 level;
    uint32 header;
    sint32 near_distance;
    sint32 far_distance;
    sint32 red;
    sint32 green;
    sint32 blue;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 group;
    uint32 descriptor;
    uint32 data;
    uint32 count;
    uint32 variant;
    uint32 variant_offset;
    uint32 index;
    uint32 first;
    uint32 object;
    uint32 flags;
    sint32 variant_index;

    FUNCTION_MARKER(0x80035390, "SLUS_006.10");
    level = r_u32(0x80088770);
    header = r_u32(level);
    near_distance = r_s16(header + 4);
    level = r_u32(0x80088770);
    header = r_u32(level);
    far_distance = r_s16(header + 6);
    level = r_u32(0x80088770);
    header = r_u32(level);
    red = r_u8(header);
    level = r_u32(0x80088770);
    header = r_u32(level);
    green = r_u8(header + 1);
    level = r_u32(0x80088770);
    header = r_u32(level);
    blue = r_u8(header + 2);
    td5_set_fog(near_distance, far_distance, red, green, blue);
    td5_set_back_color(32, 32, 32);
    SetFarColor(32, 32, 32);
    SetFogNearFar(15000, 60000, 160);
    level = r_u32(0x80088770);
    header = r_u32(level);
    if ((r_u8(header + 30) & 1) != 0)
        return td5_80035024(original_stack - 0x48);
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
    for (group = 0; group < 7; ++group)
    {
        descriptor = 0x8009DBD0 + group * 36;
        data = r_u32(descriptor);
        count = r_u8(data);
        variant_index = r_s16(descriptor + 30);
        variant = r_u32(0x800884C0) + (uint32)variant_index * 18;
        if (r_u8(descriptor + 32) != 2)
            continue;
        if ((r_u8(descriptor + 34) & 1) != 0)
        {
            for (index = 0; index < count; ++index)
            {
                object = data + r_u32(data + index * 4 + 672);
                if (td5_80079e34(object) != 0)
                {
                    td5_80079b84(object);
                    td5_8006e180(object, original_stack - 0x48);
                }
            }
        }
        if (r_u8(data + 1) == 0)
            continue;
        if ((r_u8(descriptor + 34) & 2) == 0)
            continue;
        flags = r_u8(descriptor + 34);
        if (flags == 2)
            variant_offset = 0;
        else
            variant_offset = (uint32)r_u8(variant) << 11;
        count = r_u8(data + 1);
        first = r_u8(data);
        count += first;
        index = r_u8(data);
        for (; index < count; ++index)
        {
            object = data + r_u32(data + index * 4 + 672) + variant_offset;
            if (td5_80079e34(object) != 0)
            {
                td5_80079b84(object);
                td5_8006e180(object, original_stack - 0x48);
            }
        }
    }
    return 7;
}

uint32 td5_80079c90(uint32 object)
{
    VECTOR input;
    VECTOR output;
    MATRIX *matrix;
    uint32 value;
    uint32 offset;
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

    FUNCTION_MARKER(0x80079C90, "SLUS_006.10");
    value = r_u32(object + 0x10);
    offset = r_u32(object + 0x1C);
    origin = r_u32(0x8009765C);
    radius = r_s16(object + 8);
    input.vx = (sint32)((uint32)((sint32)value >> 8) + offset - origin);
    value = r_u32(object + 0x14);
    offset = r_u32(object + 0x20);
    origin = r_u32(0x80097660);
    input.vy = (sint32)((uint32)((sint32)value >> 8) + offset - origin);
    value = r_u32(object + 0x18);
    offset = r_u32(object + 0x24);
    origin = r_u32(0x80097664);
    shift = r_u32(0x80088164);
    input.vz = (sint32)((uint32)((sint32)value >> 8) + offset - origin);
    radius >>= shift & 31;
    matrix = (MATRIX *)psx_addr(0x80097648, 20);
    SetRotMatrix(matrix);
    ApplyMatrixLV(matrix, &input, &output);
    depth = (uint32)output.vz;
    near_plane = r_s32(0x8008813C);
    shift = r_u32(0x80088164);
    if ((near_plane >> (shift & 31)) >= (sint32)(depth + (uint32)radius))
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


uint32 td5_8002e938(uint32 slot, uint32 original_stack)
{
    uint32 car = 0x800984A0 + slot * 864;
    uint32 object;
    uint32 value;
    uint32 camera;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 result;

    FUNCTION_MARKER(0x8002E938, "SLUS_006.10");
    value = r_u32(car + 0x1C4);
    camera = r_u32(0x8009765C);
    x = (uint32)((sint32)value / 256) - camera;
    x = (uint32)((sint32)x / 16);
    value = r_u32(car + 0x1C8);
    camera = r_u32(0x80097660);
    y = (uint32)((sint32)value / 256) - camera;
    y = (uint32)((sint32)y / 16);
    value = r_u32(car + 0x1CC);
    camera = r_u32(0x80097664);
    z = (uint32)((sint32)value / 256) - camera;
    z = (uint32)((sint32)z / 16);
    result = SquareRoot0((sint32)(x * x + y * y + z * z));
    if ((sint32)result < 375 || r_u32(0x80087738) != 0)
        object = r_u32(car + 0x178);
    else
        object = r_u32(car + 0x17C);
    x = r_u32(car + 0x1C4);
    y = r_u32(car + 0x1C8);
    z = r_u32(car + 0x1CC);
    w_u32(object + 0x10, x);
    w_u32(object + 0x14, y);
    w_u32(object + 0x18, z);
    result = td5_80079c90(object);
    if (result == 0)
        return result;
    td5_800314b8(car, original_stack - 0x38);
    td5_apply_light_matrix(car + 0x110);
    td5_800797fc(car + 0x110, object);
    td5_8006e180(object, original_stack - 0x38);
    td5_80040060(slot, original_stack - 0x38);
    td5_8004099c(slot, original_stack - 0x38);
    if (slot == 9 && r_u32(0x800981C8) != 0xFFFFFFFF)
    {
        w_u32(0x800885C8, 4096);
        td5_80041a44(car);
    }
    if (r_u8(car + 0x351) != 0)
        td5_80032984(car, original_stack - 0x38);
    else
        td5_800326d0(slot, original_stack - 0x38);
    result = r_u32(0x800977D8 + slot * 284);
    if (result != 0)
        result = td5_8003d288(car, car + 0x1C4, 0);
    return result;
}
