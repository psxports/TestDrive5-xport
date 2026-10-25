#include "td5_functions.h"
#include <stdio.h>

uint32 td5_80070ed0(uint32 model)
{
    VECTOR square;
    uint32 colors, first, second, third, fourth, coordinate, reference, z_reference;
    uint32 delta_x, delta_y, delta_z, bank, template_offset, vertices_offset, records_offset;
    uint32 buffer, ordering_table, shift, sum, threshold, color, packet, vertices, record;
    uint32 type, packet_bytes, tint, tag, head, head_address, rotation[3];
    sint32 center_y, center_z, center_x, model_depth, far_depth, count, polygon;
    sint32 first_clip, second_clip, depth;

    FUNCTION_MARKER(0x80070ED0, "SLUS_006.10");
    colors = r_u8(model) == 0 ? r_u32(0x80088350) : r_u32(0x80088354);
    PushMatrix();
    first = r_u32(0x800A3B90);
    second = r_u32(0x800A3B94);
    xport_gte_write_control(0, first);
    xport_gte_write_control(1, second);
    rotation[0] = r_u32(0x800A3B98);
    rotation[1] = r_u32(0x800A3B9C);
    rotation[2] = r_u32(0x800A3BA0);
    xport_gte_write_control(2, rotation[0]);
    xport_gte_write_control(3, rotation[1]);
    xport_gte_write_control(4, rotation[2]);
    coordinate = r_u32(model + 16);
    reference = r_u32(0x8008C4F8);
    delta_x = (uint32)((sint32)coordinate >> 8) - reference;
    coordinate = r_u32(model + 20);
    reference = r_u32(0x8008C4FC);
    z_reference = r_u32(0x8008C500);
    delta_y = (uint32)((sint32)coordinate >> 8) - reference;
    coordinate = r_u32(model + 24);
    center_y = r_s16(0x8008812E);
    bank = r_u32(0x80088104);
    center_z = r_s16(0x80088130);
    delta_z = (uint32)((sint32)coordinate >> 8) - z_reference;
    template_offset = r_u32(model + 52 + bank * 4);
    buffer = r_u32(0x800880F0);
    vertices_offset = r_u32(model + 44);
    records_offset = r_u32(model + 48);
    model_depth = r_s16(model + 12);
    ordering_table = r_u32(buffer + 0x70);
    center_x = r_s16(0x8008812C);
    square.vy = (sint32)(delta_y + (uint32)center_y);
    shift = r_u32(0x80088110);
    square.vz = (sint32)(delta_z + (uint32)center_z);
    packet = model + template_offset;
    far_depth = r_s32(0x80088140) >> 3;
    square.vx = (sint32)(delta_x + (uint32)center_x);
    shift -= r_u32(0x80088164);
    record = model + records_offset;
    vertices = model + vertices_offset;
    ordering_table += (uint32)(model_depth >> (shift & 31)) << 2;
    Square0(&square, &square);
    threshold = r_u32(0x80088348);
    sum = (uint32)square.vx + (uint32)square.vy + (uint32)square.vz;
    color = r_u32(colors + ((sint32)threshold < (sint32)sum ? 4092 : ((uint32)((sint32)sum >> 15) << 2)));
    while (r_u8(record + 3) != 0)
    {
        type = r_u8(record + 2);
        count = r_s16(record);
        record += 4;
        if (type > 1)
            continue;
        packet_bytes = type == 0 ? 32 : 40;
        for (polygon = 0; polygon < count; ++polygon)
        {
            first = (uint32)r_s16(record);
            second = (uint32)r_s16(record + 2);
            third = (uint32)r_s16(record + 4);
            if (type == 1)
                fourth = (uint32)r_s16(record + 6);
            first = vertices + (first << 3);
            second = vertices + (second << 3);
            third = vertices + (third << 3);
            xport_gte_write_data(0, r_u32(first));
            xport_gte_write_data(1, r_u32(first + 4));
            xport_gte_write_data(2, r_u32(second));
            xport_gte_write_data(3, r_u32(second + 4));
            xport_gte_write_data(4, r_u32(third));
            xport_gte_write_data(5, r_u32(third + 4));
            xport_gte_execute(0x280030);
            xport_gte_execute(0x1400006);
            first_clip = (sint32)xport_gte_read_data(24);
            if (type == 0)
            {
                xport_gte_execute(0x158002D);
                if (first_clip > 0)
                    goto next_flat_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_flat_polygon;
                tint = r_u32(record + 8);
                w_u32(packet + 8, xport_gte_read_data(12));
                w_u32(packet + 16, xport_gte_read_data(13));
                w_u32(packet + 24, xport_gte_read_data(14));
            }
            else
            {
                w_u32(packet + 8, xport_gte_read_data(12));
                w_u32(packet + 16, xport_gte_read_data(13));
                w_u32(packet + 24, xport_gte_read_data(14));
                fourth = vertices + (fourth << 3);
                xport_gte_write_data(0, r_u32(fourth));
                xport_gte_write_data(1, r_u32(fourth + 4));
                xport_gte_execute(0x180001);
                xport_gte_execute(0x1400006);
                second_clip = (sint32)xport_gte_read_data(24);
                xport_gte_execute(0x168002E);
                if (first_clip > 0 && second_clip < 0)
                    goto next_flat_polygon;
                depth = (sint32)xport_gte_read_data(7);
                if (depth >= far_depth)
                    goto next_flat_polygon;
                tint = r_u32(record + 8);
                w_u32(packet + 32, xport_gte_read_data(14));
            }
            w_u8(packet + 3, type == 0 ? 7 : 9);
            w_u32(packet + 4, color + tint);
            w_u8(packet + 7, type == 0 ? 36 : 44);
            tag = r_u32(packet);
            head_address = ordering_table + ((uint32)(depth >> (shift & 31)) << 2);
            head = r_u32(head_address);
            w_u32(packet, (tag & 0xFF000000) | (head & 0xFFFFFF));
            head = r_u32(head_address);
            w_u32(head_address, (head & 0xFF000000) | (packet & 0xFFFFFF));
        next_flat_polygon:
            packet += packet_bytes;
            record += 20;
        }
    }
    PopMatrix();
    return 0;
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

static void td5_interpolated_vertex(uint32 vertex, const uint32 position[3], const uint16 uv[2], const uint16 color[3], uint32 shift)
{
    w_u16(vertex, (uint16)((sint32)position[0] >> shift));
    w_u16(vertex + 2, (uint16)((sint32)position[1] >> shift));
    w_u16(vertex + 4, (uint16)((sint32)position[2] >> shift));
    w_u8(vertex + 12, (uint8)((sint16)uv[0] >> shift));
    w_u8(vertex + 13, (uint8)((sint16)uv[1] >> shift));
    w_u8(vertex + 8, (uint8)((sint16)color[0] >> shift));
    w_u8(vertex + 9, (uint8)((sint16)color[1] >> shift));
    w_u8(vertex + 10, (uint8)((sint16)color[2] >> shift));
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

uint32 td5_8007460c(uint32 packet, uint32 ordering_table, uint32 vertices, sint32 depth)
{
    uint32 shift, level, precision, required, offset, triangle_offset, bank;
    uint32 quad_packet, triangle_packet, original_tag, cursor, row, column, axis, count;
    uint32 base[3], horizontal[3], vertical[3], current[3], first, second, third;
    uint16 base_uv[2], horizontal_uv[2], vertical_uv[2], current_uv[2], clut, page;
    uint16 base_color[3], horizontal_color[3], vertical_color[3], current_color[3];
    uint32 flags[3], rejected;
    sint32 screen_x[3], screen_y[3], width, height;
    sint32 cutoff, projected_depth;

    FUNCTION_MARKER(0x8007460C, "SLUS_006.10");
    shift = r_u32(0x80088110);
    shift -= r_u32(0x80088164);
    cutoff = r_s32(0x80088134);
    if (depth >= cutoff / 4)
        return td5_subdivision_link(packet, ordering_table, depth, shift);
    cutoff = r_s32(0x80088138);
    level = depth < cutoff / 4 ? 2 : 1;
    offset = r_u32(0x80088514);
    precision = level * 2;
    required = 52 << precision;
    if (r_u32(0x800884FC) < offset + required - (12 << level))
        return td5_subdivision_link(packet, ordering_table, depth, shift);
    screen_x[0] = r_s16(packet + 8);
    screen_y[0] = r_s16(packet + 10);
    screen_x[1] = r_s16(packet + 20);
    screen_y[1] = r_s16(packet + 22);
    screen_x[2] = r_s16(packet + 32);
    screen_y[2] = r_s16(packet + 34);
    flags[0] = screen_x[0] < 0 ? 1 : 0;
    width = r_s32(0x800880F4);
    if (width < screen_x[0])
        flags[0] |= 2;
    if (screen_y[0] < 0)
        flags[0] |= 4;
    height = r_s32(0x800880F8);
    if (height < screen_y[0])
        flags[0] |= 8;
    for (axis = 1; axis < 3; ++axis)
    {
        flags[axis] = screen_x[axis] < 0 ? 1 : 0;
        if (width < screen_x[axis])
            flags[axis] |= 2;
        if (screen_y[axis] < 0)
            flags[axis] |= 4;
        if (height < screen_y[axis])
            flags[axis] |= 8;
    }
    rejected = flags[0] & flags[1] & flags[2];
    if (rejected != 0)
        return rejected;
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
        base_color[axis] = (uint16)(r_u8(packet + 4 + axis) << precision);
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
        first = r_u8(packet + 28 + axis);
        second = r_u8(packet + 4 + axis);
        vertical_color[axis] = (uint16)((first - second) << level);
    }
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
    for (axis = 0; axis < 3; ++axis)
    {
        first = r_u8(packet + 16 + axis);
        second = r_u8(packet + 4 + axis);
        horizontal_color[axis] = (uint16)((first - second) << level);
    }
    cursor = 0x800A4E58;
    count = 1 << level;
    for (axis = 0; axis < 3; ++axis)
    {
        current[axis] = base[axis];
        current_color[axis] = base_color[axis];
    }
    current_uv[0] = base_uv[0];
    current_uv[1] = base_uv[1];
    for (column = 0; column <= count; ++column)
    {
        td5_interpolated_vertex(cursor, current, current_uv, current_color, precision);
        cursor += 16;
        for (axis = 0; axis < 3; ++axis)
        {
            current[axis] += horizontal[axis];
            current_color[axis] = (uint16)(current_color[axis] + horizontal_color[axis]);
        }
        current_uv[0] = (uint16)(current_uv[0] + horizontal_uv[0]);
        current_uv[1] = (uint16)(current_uv[1] + horizontal_uv[1]);
    }
    for (row = 1; row <= count; ++row)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            base[axis] += vertical[axis];
            current[axis] = base[axis];
            base_color[axis] = (uint16)(base_color[axis] + vertical_color[axis]);
            current_color[axis] = base_color[axis];
        }
        base_uv[0] = (uint16)(base_uv[0] + vertical_uv[0]);
        base_uv[1] = (uint16)(base_uv[1] + vertical_uv[1]);
        current_uv[0] = base_uv[0];
        current_uv[1] = base_uv[1];
        td5_interpolated_vertex(cursor, current, current_uv, current_color, precision);
        cursor += 16;
        for (column = 1; column <= count - row; ++column)
        {
            for (axis = 0; axis < 3; ++axis)
            {
                current[axis] += horizontal[axis];
                current_color[axis] = (uint16)(current_color[axis] + horizontal_color[axis]);
            }
            current_uv[0] = (uint16)(current_uv[0] + horizontal_uv[0]);
            current_uv[1] = (uint16)(current_uv[1] + horizontal_uv[1]);
            td5_interpolated_vertex(cursor, current, current_uv, current_color, precision);
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

static uint32 td5_boundary_packet(uint32 output, uint32 ordering_head, const uint32 sources[4], uint16 clut, uint16 page, uint32 color, uint32 quad)
{
    uint32 vertex, value, tag = 0, head, result;

    w_u16(output + 26, clut);
    w_u16(output + 14, page);
    w_u32(output, quad == 0 ? 0x09000000 : 0x0C000000);
    for (vertex = 0; vertex < (quad == 0 ? 3u : 4u); ++vertex)
    {
        value = r_u32(sources[vertex]);
        if ((quad == 0 && vertex == 0) || (quad != 0 && vertex == 3))
            tag = r_u32(output);
        if (vertex == 0)
            value = (value & 0xFFFFFF) | color;
        w_u32(output + 4 + vertex * 12, value);
        w_u8(output + 12 + vertex * 12, r_u8(sources[vertex] + 8));
        w_u8(output + 13 + vertex * 12, r_u8(sources[vertex] + 9));
        w_u16(output + 8 + vertex * 12, r_u16(sources[vertex] + 4));
        w_u16(output + 10 + vertex * 12, r_u16(sources[vertex] + 6));
    }
    head = r_u32(ordering_head);
    w_u32(output, (tag & 0xFF000000) | (head & 0xFFFFFF));
    head = r_u32(ordering_head);
    result = (head & 0xFF000000) | (output & 0xFFFFFF);
    w_u32(ordering_head, result);
    return result;
}

uint32 td5_8007526c(uint32 packet, uint32 ordering_head, uint32 depths)
{
    static const uint32 depth_indices[4] = {1, 3, 2, 0};
    static const uint32 color_offsets[4][3] = {{4, 68, 16}, {68, 196, 92}, {132, 144, 196}, {4, 28, 132}};
    uint32 offset, limit, bank, color, base, output, source, edge, vertex, result;
    uint32 sources[4];
    uint16 clut, page;
    sint32 current_depth, next_depth;

    FUNCTION_MARKER(0x8007526C, "SLUS_006.10");
    offset = r_u32(0x80088514);
    limit = r_u32(0x800884FC);
    if (limit < offset + 160)
        return 1;
    bank = r_u32(0x80088104);
    clut = r_u16(packet + 26);
    page = r_u16(packet + 14);
    color = r_u32(packet + 4);
    w_u32(0x80088514, offset + 160);
    current_depth = r_s32(depths);
    base = r_u32(0x80088370 + bank * 4);
    color &= 0xF7FFFFFF;
    output = base + offset;
    source = output - 208;
    for (edge = 0; edge < 4; ++edge)
    {
        if (current_depth > 0)
        {
            next_depth = r_s32(depths + depth_indices[edge] * 4);
            if (next_depth > 0)
            {
                for (vertex = 0; vertex < 3; ++vertex)
                    sources[vertex] = source + color_offsets[edge][vertex];
                result = td5_boundary_packet(output, ordering_head, sources, clut, page, color, 0);
                if (edge == 3)
                    return result;
                current_depth = r_s32(depths + depth_indices[edge] * 4);
            }
            else
            {
                current_depth = next_depth;
                if (edge == 3)
                    return (uint32)current_depth;
            }
        }
        else
        {
            if (edge == 3)
                return (uint32)current_depth;
            current_depth = r_s32(depths + depth_indices[edge] * 4);
        }
        output += 40;
    }
    return (uint32)current_depth;
}

uint32 td5_80075728(uint32 packet, uint32 ordering_head, uint32 depths)
{
    static const uint32 depth_indices[4] = {1, 3, 2, 0};
    static const uint32 triangle_offsets[4][3] = {{4, 172, 16}, {172, 820, 196}, {820, 652, 808}, {652, 4, 628}};
    static const uint32 quad_offsets[4][4] = {{16, 172, 120, 68}, {196, 820, 612, 404}, {808, 652, 704, 756}, {628, 4, 212, 420}};
    uint32 offset, limit, bank, color, triangle_base, quad_base, triangle, quad;
    uint32 source, edge, vertex, result, sources[4];
    uint16 clut, page;
    sint32 current_depth, next_depth;

    FUNCTION_MARKER(0x80075728, "SLUS_006.10");
    offset = r_u32(0x80088514);
    limit = r_u32(0x800884FC);
    if (limit < offset + 368)
        return 1;
    clut = r_u16(packet + 26);
    bank = r_u32(0x80088104);
    page = r_u16(packet + 14);
    color = r_u32(packet + 4);
    triangle_base = r_u32(0x80088370 + bank * 4);
    quad_base = r_u32(0x80088370 + bank * 4);
    color &= 0xF7FFFFFF;
    w_u32(0x80088514, offset + 160);
    w_u32(0x80088514, offset + 368);
    triangle = triangle_base + offset;
    quad = quad_base + offset + 160;
    current_depth = r_s32(depths);
    source = triangle_base + offset - 832;
    for (edge = 0; edge < 4; ++edge)
    {
        if (current_depth > 0)
        {
            next_depth = r_s32(depths + depth_indices[edge] * 4);
            if (next_depth > 0)
            {
                for (vertex = 0; vertex < 3; ++vertex)
                    sources[vertex] = source + triangle_offsets[edge][vertex];
                td5_boundary_packet(triangle, ordering_head, sources, clut, page, color, 0);
                for (vertex = 0; vertex < 4; ++vertex)
                    sources[vertex] = source + quad_offsets[edge][vertex];
                result = td5_boundary_packet(quad, ordering_head, sources, clut, page, color | 0x08000000, 1);
                if (edge == 3)
                    return result;
                current_depth = r_s32(depths + depth_indices[edge] * 4);
            }
            else
            {
                current_depth = next_depth;
                if (edge == 3)
                    return (uint32)current_depth;
            }
        }
        else
        {
            if (edge == 3)
                return (uint32)current_depth;
            current_depth = r_s32(depths + depth_indices[edge] * 4);
        }
        triangle += 40;
        quad += 52;
    }
    return (uint32)current_depth;
}

uint32 td5_800760b8(uint32 packet, uint32 ordering_table, uint32 vertices, sint32 depth, uint32 original_stack)
{
    uint32 shift, level, precision, required, offset, bank, output, original_tag;
    uint32 cursor, count, axis, component, row, column, first, second, third;
    uint32 left[3], right[3], left_step[3], right_step[3], horizontal[3], current[3];
    uint16 left_uv[2], right_uv[2], left_uv_step[2], right_uv_step[2], horizontal_uv[2], current_uv[2], clut, page;
    uint16 left_color[3], right_color[3], left_color_step[3], right_color_step[3], horizontal_color[3], current_color[3];
    uint32 flag;
    sint32 screen_x[4], screen_y[4], width, height;
    uint32 depths = original_stack - 0x178 + 0x10;
    uint32 ordering_head;
    sint32 cutoff, projected_depth;

    FUNCTION_MARKER(0x800760B8, "SLUS_006.10");
    shift = r_u32(0x80088110);
    shift -= r_u32(0x80088164);
    cutoff = r_s32(0x80088134);
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
    for (axis = 0; axis < 4; ++axis)
    {
        screen_x[axis] = r_s16(packet + 8 + axis * 12);
        screen_y[axis] = r_s16(packet + 10 + axis * 12);
    }
    w_u32(depths, 0);
    if (screen_x[0] < 0)
        w_u32(depths, 1);
    width = r_s32(0x800880F4);
    if (width < screen_x[0])
        w_u32(depths, r_u32(depths) | 2);
    if (screen_y[0] < 0)
        w_u32(depths, r_u32(depths) | 4);
    height = r_s32(0x800880F8);
    if (height < screen_y[0])
        w_u32(depths, r_u32(depths) | 8);
    for (axis = 1; axis < 4; ++axis)
    {
        w_u32(depths + axis * 4, 0);
        if (screen_x[axis] < 0)
            w_u32(depths + axis * 4, 1);
        if (axis == 3)
            width = r_s32(0x800880F4);
        if (width < screen_x[axis])
            w_u32(depths + axis * 4, r_u32(depths + axis * 4) | 2);
        if (screen_y[axis] < 0)
            w_u32(depths + axis * 4, r_u32(depths + axis * 4) | 4);
        if (axis == 3)
            height = r_s32(0x800880F8);
        if (height < screen_y[axis])
            w_u32(depths + axis * 4, r_u32(depths + axis * 4) | 8);
    }
    flag = r_u32(depths);
    flag &= r_u32(depths + 4);
    flag &= r_u32(depths + 8);
    flag &= r_u32(depths + 12);
    if (flag != 0)
        return depths;
    w_u32(depths, xport_gte_read_data(16));
    w_u32(depths + 4, xport_gte_read_data(17));
    w_u32(depths + 8, xport_gte_read_data(18));
    w_u32(depths + 12, xport_gte_read_data(19));
    bank = r_u32(0x80088104);
    offset = r_u32(0x80088514);
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
        left_color[axis] = (uint16)(r_u8(packet + 4 + axis) << precision);
    for (axis = 0; axis < 3; ++axis)
        right[axis] = (uint32)r_s16(vertices + 8 + axis * 2) << precision;
    right_uv[0] = (uint16)(r_u8(packet + 24) << precision);
    right_uv[1] = (uint16)(r_u8(packet + 25) << precision);
    for (axis = 0; axis < 3; ++axis)
        right_color[axis] = (uint16)(r_u8(packet + 16 + axis) << precision);
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
        first = r_u8(packet + 28 + axis);
        second = r_u8(packet + 4 + axis);
        left_color_step[axis] = (uint16)((first - second) << level);
    }
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
    for (axis = 0; axis < 3; ++axis)
    {
        first = r_u8(packet + 40 + axis);
        second = r_u8(packet + 16 + axis);
        right_color_step[axis] = (uint16)((first - second) << level);
    }
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
            {
                right[axis] += right_step[axis];
                left_color[axis] = (uint16)(left_color[axis] + left_color_step[axis]);
                right_color[axis] = (uint16)(right_color[axis] + right_color_step[axis]);
            }
            right_uv[0] = (uint16)(right_uv[0] + right_uv_step[0]);
            right_uv[1] = (uint16)(right_uv[1] + right_uv_step[1]);
        }
        for (axis = 0; axis < 3; ++axis)
        {
            current[axis] = left[axis];
            horizontal[axis] = (uint32)((sint32)(right[axis] - left[axis]) >> level);
            current_color[axis] = left_color[axis];
            horizontal_color[axis] = (uint16)(((sint32)(sint16)right_color[axis] - (sint32)(sint16)left_color[axis]) >> level);
        }
        for (component = 0; component < 2; ++component)
        {
            current_uv[component] = left_uv[component];
            horizontal_uv[component] = (uint16)(((sint32)(sint16)right_uv[component] - (sint32)(sint16)left_uv[component]) >> level);
        }
        td5_interpolated_vertex(cursor, current, current_uv, current_color, precision);
        cursor += 16;
        for (column = 1; column <= count; ++column)
        {
            for (axis = 0; axis < 3; ++axis)
            {
                current[axis] += horizontal[axis];
                current_color[axis] = (uint16)(current_color[axis] + horizontal_color[axis]);
            }
            current_uv[0] = (uint16)(current_uv[0] + horizontal_uv[0]);
            current_uv[1] = (uint16)(current_uv[1] + horizontal_uv[1]);
            td5_interpolated_vertex(cursor, current, current_uv, current_color, precision);
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

uint32 td5_80078b7c(uint32 left, uint32 top, uint32 right, uint32 bottom)
{
    uint32 origin, size, environment;

    FUNCTION_MARKER(0x80078B7C, "SLUS_006.10");
    origin = (left & 0xFFFF) | ((top & 0xFFFF) << 16);
    size = ((right - left) & 0xFFFF) | (((bottom - top) & 0xFFFF) << 16);
    DrawSync(0);
    environment = r_u32(0x800880F0);
    w_u32(0x80097668, origin);
    w_u32(0x8009766C, size);
    w_u32(0x800976DC, origin);
    w_u32(0x800976E0, size);
    PutDrawEnv((DRAWENV *)psx_addr(environment, sizeof(DRAWENV)));
    return environment;
}

uint32 td5_8007999c(uint32 matrix, uint32 object, uint32 pivot)
{
    VECTOR relative, before, after;
    uint32 axis, coordinate, reference, relative_words[3], rotation[3];
    uint32 translation[3], result;

    FUNCTION_MARKER(0x8007999C, "SLUS_006.10");
    for (axis = 0; axis < 3; ++axis)
    {
        coordinate = r_u32(object + 16 + axis * 4);
        reference = r_u32(0x8009765C + axis * 4);
        relative_words[axis] = (uint32)((sint32)coordinate >> 8) - reference;
    }
    relative.vx = (sint32)relative_words[0];
    relative.vy = (sint32)relative_words[1];
    relative.vz = (sint32)relative_words[2];
    MulMatrix0((MATRIX *)psx_addr(0x80097648, 20), (MATRIX *)psx_addr(matrix, 20), (MATRIX *)psx_addr(0x80092210, 20));
    ApplyRotMatrixLV(&relative, (VECTOR *)psx_addr(0x80092224, 12));
    ApplyRotMatrix((SVECTOR *)psx_addr(pivot, 8), &before);
    // The original SDK boundary returns saturated IR values
    before.vx = (sint32)xport_gte_read_data(9);
    before.vy = (sint32)xport_gte_read_data(10);
    before.vz = (sint32)xport_gte_read_data(11);
    rotation[0] = r_u32(0x80092210);
    rotation[1] = r_u32(0x80092214);
    xport_gte_write_control(0, rotation[0]);
    xport_gte_write_control(1, rotation[1]);
    rotation[0] = r_u32(0x80092218);
    rotation[1] = r_u32(0x8009221C);
    rotation[2] = r_u32(0x80092220);
    xport_gte_write_control(2, rotation[0]);
    xport_gte_write_control(3, rotation[1]);
    xport_gte_write_control(4, rotation[2]);
    ApplyRotMatrix((SVECTOR *)psx_addr(pivot, 8), &after);
    after.vx = (sint32)xport_gte_read_data(9);
    after.vy = (sint32)xport_gte_read_data(10);
    after.vz = (sint32)xport_gte_read_data(11);
    translation[0] = r_u32(0x80092224);
    translation[1] = r_u32(0x80092228);
    translation[2] = r_u32(0x8009222C);
    result = translation[0] + (uint32)before.vx - (uint32)after.vx;
    translation[1] += (uint32)before.vy - (uint32)after.vy;
    translation[2] += (uint32)before.vz - (uint32)after.vz;
    w_u32(0x80092224, result);
    w_u32(0x80092228, translation[1]);
    w_u32(0x8009222C, translation[2]);
    translation[0] = r_u32(0x80092224);
    translation[1] = r_u32(0x80092228);
    xport_gte_write_control(5, translation[0]);
    translation[2] = r_u32(0x8009222C);
    xport_gte_write_control(6, translation[1]);
    xport_gte_write_control(7, translation[2]);
    return result;
}

uint32 td5_80079aec(uint32 object)
{
    VECTOR relative;
    uint32 axis, coordinate, reference, relative_words[3], translation[3];

    FUNCTION_MARKER(0x80079AEC, "SLUS_006.10");
    for (axis = 0; axis < 3; ++axis)
    {
        coordinate = r_u32(object + 16 + axis * 4);
        reference = r_u32(0x8009765C + axis * 4);
        relative_words[axis] = (uint32)((sint32)coordinate >> 8) - reference;
    }
    relative.vx = (sint32)relative_words[0];
    relative.vy = (sint32)relative_words[1];
    relative.vz = (sint32)relative_words[2];
    SetRotMatrix((MATRIX *)psx_addr(0x80097648, 20));
    ApplyRotMatrixLV(&relative, (VECTOR *)psx_addr(0x80092224, 12));
    translation[0] = r_u32(0x80092224);
    translation[1] = r_u32(0x80092228);
    xport_gte_write_control(5, translation[0]);
    translation[2] = r_u32(0x8009222C);
    xport_gte_write_control(6, translation[1]);
    xport_gte_write_control(7, translation[2]);
    return 0x80092224;
}

uint32 td5_8007a4b4(uint32 destination, uint32 left, uint32 right, sint32 inner_count, sint32 row_count, sint32 column_count, uint32 incoming_v0)
{
    uint32 row, column, inner, left_row, output_row, right_index;
    uint32 first, second, sum;

    FUNCTION_MARKER(0x8007A4B4, "SLUS_006.10");
    if (row_count <= 0)
        return incoming_v0;
    left_row = 0;
    output_row = 0;
    for (row = 0; (sint32)row < row_count; ++row)
    {
        for (column = 0; (sint32)column < column_count; ++column)
        {
            sum = 0;
            right_index = column;
            for (inner = 0; (sint32)inner < inner_count; ++inner)
            {
                first = r_u32(left + ((left_row + inner) << 2));
                second = r_u32(right + (right_index << 2));
                right_index += (uint32)column_count;
                sum += first * second;
            }
            w_u32(destination + ((output_row + column) << 2), sum);
        }
        output_row += (uint32)column_count;
        left_row += (uint32)inner_count;
    }
    return 0;
}

uint32 td5_8007a570(uint32 coefficients, uint32 points)
{
    uint32 relative[12], point, axis, row, inner, first, second, sum;
    uint32 address;
    sint32 x, y, z;

    FUNCTION_MARKER(0x8007A570, "SLUS_006.10");
    w_u32(coefficients, r_u32(points + 12));
    w_u32(coefficients + 4, r_u32(points + 16));
    w_u32(coefficients + 8, r_u32(points + 20));
    for (point = 0; point < 4; ++point)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            first = r_u32(points + point * 12 + axis * 4);
            second = r_u32(points + 12 + axis * 4);
            relative[point * 3 + axis] = (uint32)((sint32)(first - second) >> 8);
        }
    }
    for (row = 0; row < 4; ++row)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            sum = 0;
            for (inner = 0; inner < 4; ++inner)
            {
                first = r_u32(0x80086D84 + (row * 4 + inner) * 4);
                sum += first * relative[inner * 3 + axis];
            }
            w_u32(coefficients + 12 + (row * 3 + axis) * 4, sum);
        }
    }
    for (row = 0; row < 4; ++row)
    {
        address = coefficients + row * 12;
        x = r_s32(address + 12);
        y = r_s32(address + 16);
        w_u32(address + 12, (uint32)(x / 2));
        z = r_s32(address + 20);
        w_u32(address + 16, (uint32)(y / 2));
        w_u32(address + 20, (uint32)(z / 2));
    }
    return 0;
}

uint32 td5_8007a708(uint32 position, uint32 coefficients, uint32 amount)
{
    uint32 squared, cubed, axis, first, second, third, constant, base, value;

    FUNCTION_MARKER(0x8007A708, "SLUS_006.10");
    squared = (uint32)((sint32)(amount * amount) >> 12);
    cubed = (uint32)((sint32)(squared * amount) >> 12);
    for (axis = 0; axis < 3; ++axis)
    {
        first = r_u32(coefficients + 12 + axis * 4);
        first *= cubed;
        second = r_u32(coefficients + 24 + axis * 4);
        second *= squared;
        third = r_u32(coefficients + 36 + axis * 4);
        third *= amount;
        constant = r_u32(coefficients + 48 + axis * 4);
        base = r_u32(coefficients + axis * 4);
        value = first + second + third;
        value = (uint32)((sint32)value >> 12);
        value = ((value + constant) << 8) + base;
        w_u32(position + axis * 4, value);
    }
    return value;
}

sint32 td5_8007a83c(void)
{
    FUNCTION_MARKER(0x8007A83C, "SLUS_006.10");
    td5_800684f0();
    ResetCallback();
    InitGeom();
    td5_80068500();
    VSync(0);
    DrawSync(0);
    SetDispMask(0);
    ResetGraph(0);
    return SetGraphDebug(0);
}

uint32 td5_8007a8bc(uint32 image)
{
    PSX_RECT rectangle;
    uint32 block, bytes;

    FUNCTION_MARKER(0x8007A8BC, "SLUS_006.10");
    if (r_u32(image) != 16)
        return 16;
    block = image + 8;
    rectangle.x = r_s16(block + 4);
    rectangle.y = r_s16(block + 6);
    rectangle.w = r_s16(block + 8);
    rectangle.h = r_s16(block + 10);
    bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(block + 12, bytes));
    block += r_u32(block) & 0xFFFFFFFC;
    if (r_u32(block) == 4)
        return 4;
    rectangle.x = r_s16(block + 4);
    rectangle.y = r_s16(block + 6);
    rectangle.w = r_s16(block + 8);
    rectangle.h = r_s16(block + 10);
    bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
    return (uint32)LoadImagePSX(&rectangle, (uint32 *)psx_addr(block + 12, bytes));
}

void td5_8007aa98(void)
{
    sint32 handle;

    FUNCTION_MARKER(0x8007AA98, "SLUS_006.10");
    td5_800684f0();
    handle = OpenEventPSX(0xF2000001, 2, 0x2000, 0);
    EnableEventPSX((uint32)handle);
    SetRCnt(0xF2000001, 0xFFFF, 0x1000);
    SetRCnt(0xF2000002, 0xFFFF, 0x2000);
    td5_80068500();
}

sint32 td5_8007ab9c(sint32 value)
{
    uint32 angle = 512, step = 256, counter = 0;

    FUNCTION_MARKER(0x8007AB9C, "SLUS_006.10");
    do
    {
        if (rsin((sint32)angle >> 1) < value)
            angle += step;
        else
            angle -= step;
        // Preserve the original decrement and signed wraparound exit
        counter -= 1;
        step = (uint32)((sint32)step >> 1);
    } while ((sint32)counter < 9);
    return (sint32)angle >> 1;
}

sint32 td5_8007ac18(sint32 first, sint32 second)
{
    uint32 numerator, base, subtract, angle = 512, step = 256, counter = 0;
    sint32 denominator, ratio;

    FUNCTION_MARKER(0x8007AC18, "SLUS_006.10");
    if (second == 0 && first == 0)
        return 0;
    if (first >= 0)
    {
        if (second > 0)
        {
            if (first < second)
            {
                numerator = ((uint32)first << 10) + (uint32)(second / 2);
                denominator = second;
                base = 0;
                subtract = 0;
            }
            else
            {
                numerator = ((uint32)second << 10) + (uint32)(first / 2);
                denominator = first;
                base = 1024;
                subtract = 1;
            }
        }
        else if (second < (sint32)(0 - (uint32)first))
        {
            numerator = (uint32)(second / 2) - ((uint32)first << 10);
            denominator = second;
            base = 2048;
            subtract = 1;
        }
        else
        {
            numerator = (uint32)(first / 2) - ((uint32)second << 10);
            denominator = first;
            base = 1024;
            subtract = 0;
        }
    }
    else if (second > 0)
    {
        if ((sint32)(0 - (uint32)first) < second)
        {
            numerator = (uint32)(second / 2) - ((uint32)first << 10);
            denominator = second;
            base = 4096;
            subtract = 1;
        }
        else
        {
            numerator = (uint32)(first / 2) - ((uint32)second << 10);
            denominator = first;
            base = 3072;
            subtract = 0;
        }
    }
    else if (second < first)
    {
        numerator = ((uint32)first << 10) + (uint32)(second / 2);
        denominator = second;
        base = 2048;
        subtract = 0;
    }
    else
    {
        numerator = ((uint32)second << 10) + (uint32)(first / 2);
        denominator = first;
        base = 3072;
        subtract = 1;
    }
    ratio = td5_divide_checked((sint32)numerator, denominator);
    do
    {
        if (rsin((sint32)angle >> 1) < ratio)
            angle += step;
        else
            angle -= step;
        counter -= 1;
        step = (uint32)((sint32)step >> 1);
    } while ((sint32)counter < 9);
    angle = (uint32)((sint32)angle >> 1);
    return (sint32)(subtract ? base - angle : base + angle);
}

void td5_800bfccc(void)
{
    FUNCTION_MARKER(0x800BFCCC, "SLUS_006.10");
}

void td5_800bfcd4(void)
{
    FUNCTION_MARKER(0x800BFCD4, "SLUS_006.10");
}

void td5_800bfcdc(void)
{
    FUNCTION_MARKER(0x800BFCDC, "SLUS_006.10");
}

uint32 td5_800b1358(void)
{
    uint32 mode;

    FUNCTION_MARKER(0x800B1358, "SLUS_006.10");
    w_u32(0x800BF3AC, 0);
    w_u32(0x800BCF00, 0);
    w_u32(0x800BF104, 0);
    td5_800b187c(0);
    mode = r_u32(0x80087720);
    if (mode == 1)
        w_u32(0x800875E4, mode);
    return 1;
}

static void td5_carousel_draw_guest_text(uint32 mode, uint32 x, sint32 y, uint32 text)
{
    uint32 length = 0;

    while (r_u8(text + length) != 0)
        ++length;
    td5_800b2790(mode, x, y, (const char *)psx_addr(text, length + 1));
}

uint32 td5_800b13ac(void)
{
    uint32 maximum, current, rotation, target, selected, x, mode, list, count, pointer, end, text;
    uint32 buttons, choice, row = 82;

    FUNCTION_MARKER(0x800B13AC, "SLUS_006.10");
    maximum = r_u32(0x800875E4);
    if ((sint32)maximum >= 7)
        maximum = 6;
    maximum -= 1;
    buttons = r_u16(0x800884E8);
    if (buttons & 0x4008)
    {
        td5_800b217c();
        td5_800530f0();
    }
    if (r_u16(0x800884E8) & 0x80)
    {
        current = r_u32(0x800BCF00);
        if ((sint32)current > 0 && r_u32(0x800BF3AC) == 0)
        {
            w_u32(0x800BF104, 416);
            w_u32(0x800BCF00, current - 1);
            td5_80053030();
        }
    }
    if (r_u16(0x800884E8) & 0x20)
    {
        current = r_u32(0x800BCF00);
        if ((sint32)current < (sint32)maximum && r_u32(0x800BF3AC) == 0)
        {
            w_u32(0x800BF104, 0 - 416);
            w_u32(0x800BCF00, current + 1);
            td5_80053030();
        }
    }
    target = r_u32(0x800BF104);
    rotation = r_u32(0x800BF3AC);
    if (target == rotation)
    {
        selected = r_u32(0x800BF3A4);
        current = r_u32(0x800BCF00);
        if (selected != current)
        {
            td5_800b187c(current);
            rotation = r_u32(0x800BF3AC);
            w_u32(0x800BF104, 0);
            w_u32(0x800BF3AC, 0 - rotation);
        }
    }
    rotation = r_u32(0x800BF3AC);
    target = r_u32(0x800BF104);
    if ((sint32)target < (sint32)rotation)
    {
        w_u32(0x800BF3AC, rotation - 32);
        rotation = r_u32(0x800BF3AC);
    }
    if ((sint32)rotation < (sint32)target)
        w_u32(0x800BF3AC, rotation + 32);
    td5_800b32e4(0);
    td5_800b2f8c(128, 104, 206, (const char *)psx_addr(0x800B0A38, 6));
    td5_800b32e4(2);
    td5_carousel_draw_guest_text(0, 117, 198, 0x800B0A40);
    if (r_u32(0x800BF3AC) == 0)
    {
        current = r_u32(0x800BCF00);
        if (current != 0)
        {
            td5_800b2f8c(128, 64, 128, (const char *)psx_addr(0x800B0A48, 9));
            current = r_u32(0x800BCF00);
        }
        if (current != maximum)
            td5_800b2f8c(128, 448, 128, (const char *)psx_addr(0x800B0A54, 9));
    }
    td5_800b32e4(2);
    td5_carousel_draw_guest_text(131, 256, 22, 0x800B0A60);
    rotation = r_u32(0x800BF3AC);
    x = rotation + 128;
    w_u32(0x800BF0F0, rotation + 308);
    w_u32(0x800BF420, x);
    td5_800b32e4(3);
    text = r_u32(0x800BBCB4);
    td5_carousel_draw_guest_text(1, x, 48, text);
    if (r_u32(0x8008771C) == 2 && (r_u32(0x800875D8) & 8) == 0)
    {
        text = r_u32(0x800BBCB8);
        td5_carousel_draw_guest_text(1, x, 57, text);
    }
    td5_800b32e4(0);
    mode = r_u32(0x80087720);
    list = 0x800BBD3C;
    if (mode == 2)
    {
        choice = r_u32(0x80087728);
        if (choice < 6)
        {
            switch (choice)
            {
                case 0:
                    list = 0x800BBD1C;
                    break;
                case 1:
                case 2:
                case 3:
                    list = 0x800BBD2C;
                    break;
                case 4:
                    list = 0x800BBD10;
                    break;
                case 5:
                    list = 0x800BBD48;
                    break;
            }
        }
    }
    mode = r_u32(0x8008771C);
    if (mode == 2)
        list = 0x800BBD58;
    if (r_u32(0x80087724) != 0)
        list = 0x800BBD74;
    if (mode == 3)
        list = 0x800BBD6C;
    if (r_u32(0x80087734) != 0)
        list = 0x800BBD64;
    count = r_u8(list) + 1;
    if (r_u32(0x80087728) == 5 || r_u32(0x80087730) != 0 || r_u32(0x8008772C) != 0)
    {
        if (r_u32(0x8008771C) - 3 >= 2)
            count = count - 5 + r_u32(0x800876F4);
    }
    pointer = list + 1;
    end = list + count;
    if ((sint32)count > 1)
    {
        do
        {
            choice = r_u8(pointer);
            text = r_u32(0x800BBCB4 + choice * 4);
            ++pointer;
            td5_carousel_draw_guest_text(1, x, (sint32)row, text);
            row += 9;
        } while ((sint32)pointer < (sint32)end);
    }
    return 0;
}

uint32 td5_800b3bec(void)
{
    uint32 slot, allocation;

    FUNCTION_MARKER(0x800B3BEC, "SLUS_006.10");
    td5_debug_log(0x800B0C74);
    slot = r_u32(0x800BF10C);
    w_u8(0x800BC928 + slot * 12, 0);
    td5_8005a9f0(0);
    allocation = r_u32(0x800BF418);
    return td5_free_memory(allocation);
}

uint32 td5_800b3f24(uint32 mode)
{
    uint32 game_mode, value, index, offset, allocation, count, first, second, third, fourth, flag;

    FUNCTION_MARKER(0x800B3F24, "SLUS_006.10");
    game_mode = r_u32(0x8008771C);
    w_u8(0x800BD070, 0);
    w_u8(0x800BD0E8, 0);
    w_u32(0x800BC880, 0);
    w_u32(0x800887B8, 0xFFFFFFFF);
    w_u32(0x800BC178, 0);
    if (game_mode != 0)
    {
        value = r_u32(0x800BC174);
        if (value == 0)
            w_u32(0x800BC174, 1);
        else
        {
            w_u32(0x800BC174, 2);
            w_u32(0x800BC178, 1);
        }
    }
    if (r_u32(0x80087720) == 3)
    {
        value = r_u32(0x800BC174);
        w_u32(0x800BC174, value == 0 ? 1 : 2);
    }
    w_u32(0x800BC8A4, mode);
    td5_800b36e8();
    if (r_s32(0x800BC174) < 2)
    {
        td5_800557c0();
        value = r_u32(0x80087700);
        td5_8005490c(13, 20, (sint32)value, 0, 0x1400);
    }
    w_u32(0x800BC158, 0);
    w_u32(0x800BC8A0, 0);
    w_u32(0x800BC15C, 0x1000);
    w_u32(0x800BC160, 0);
    w_u32(0x800BF418, 0);
    w_u32(0x800BC16C, 0);
    if (mode == 0)
    {
        w_u16(0x800BC884, 0);
        w_u16(0x800BC88C, 0);
        w_u16(0x800BC886, 0);
        w_u16(0x800BC88E, 0);
        w_u16(0x800BC888, 0);
        w_u16(0x800BC890, 0);
        w_u16(0x800BC88A, 0);
        w_u16(0x800BC892, 0);
        w_u32(0x800BC154, 0);
    }
    else
    {
        first = r_u16(0x8008CF78);
        second = r_u16(0x8008CF7A);
        third = r_u16(0x8008CF7C);
        fourth = r_u16(0x8008CF7E);
        flag = r_u8(0x8008CF71);
        w_u16(0x800BC884, first);
        w_u16(0x800BC88C, first);
        w_u16(0x800BC886, second);
        w_u16(0x800BC88E, second);
        w_u16(0x800BC888, third);
        w_u16(0x800BC890, third);
        w_u16(0x800BC88A, fourth);
        w_u16(0x800BC892, fourth);
        w_u32(0x800BC154, flag);
    }
    if (r_u32(0x80087734) != 0)
        w_u32(0x800BC154, 1);
    td5_8002f208();
    td5_800b7294(0x800BC17C);
    w_u8(0x800BCF08, 0);
    if (r_u32(0x80087728) == 3 && mode != 0)
    {
        w_u8(0x800BD250, 0);
        w_u8(0x800BD2C8, 0);
    }
    w_u32(0x800BCCA0, 1);
    index = 0;
    offset = 0;
    do
    {
        td5_debug_log(0x800B0C9C, index);
        w_u8(0x800BCB40 + offset, 0xFF);
        allocation = td5_alloc_memory(0x32000);
        count = r_u32(0x800BCCA0);
        w_u32(0x800BCB44 + offset, allocation);
        ++index;
        offset += 8;
    } while ((sint32)index < (sint32)count);
    td5_debug_log(0x800B0CA0);
    for (index = 0; index < 44; ++index)
    {
        offset = index * 12;
        w_u8(0x800BC928 + offset, 0);
        w_u16(0x800BC92A + offset, 0);
        w_u8(0x800BC92C + offset, 0xFF);
    }
    return td5_800b5800();
}

uint32 td5_800b5800(void)
{
    uint32 texture, index, destination, value, bank, packet;

    FUNCTION_MARKER(0x800B5800, "SLUS_006.10");
    texture = td5_find_texture((const char *)psx_addr(0x800B0D88, 8));
    for (index = 0; index < 4; ++index)
    {
        destination = 0x8008F5A8 + index * 6;
        w_u16(destination, r_u16(texture + 8));
        w_u16(destination + 2, r_u16(texture + 10));
        value = r_u8(texture + 14);
        if (index & 1)
            value += 32;
        w_u8(destination + 4, value);
        value = r_u8(texture + 15);
        if (index >= 2)
            value += 32;
        w_u8(destination + 5, value);
    }
    for (index = 0; index < 48; ++index)
    {
        for (bank = 0; bank < 2; ++bank)
        {
            packet = 0x8009DD00 + index * 40 + bank * 0x780;
            w_u8(packet + 3, 9);
            w_u8(packet + 7, 44);
        }
    }
    return 0;
}

uint32 td5_800b4674(void)
{
    PSX_RECT rectangle;
    uint32 state, pixels, bank, bytes;

    FUNCTION_MARKER(0x800B4674, "SLUS_006.10");
    state = r_u32(0x800BC8A0);
    pixels = r_u32(0x80088608);
    rectangle.h = state != 0 ? 152 : 256;
    bank = r_u32(0x80088104);
    rectangle.w = 512;
    rectangle.x = 0;
    rectangle.y = (sint16)(bank << 8);
    bytes = (uint32)rectangle.w * (uint32)rectangle.h * 2;
    return (uint32)LoadImagePSX(&rectangle, (uint32 *)psx_addr(pixels + 20, bytes));
}

uint32 td5_800b36e8(void)
{
    uint32 list = 0x8007DAE0, category = 1, limit = 44;
    uint32 index, previous, count, cursor, selected, group, result;
    sint32 value;

    FUNCTION_MARKER(0x800B36E8, "SLUS_006.10");
    if (r_u32(0x80087720) == 2)
    {
        selected = r_u32(0x80087728);
        if (selected == 3)
        {
            if (r_u32(0x800BC8A4) == 0)
            {
                for (index = 0; index < 13; ++index)
                    w_u16(0x800A50B0 - index * 2, 0xFFFF);
                index = 0;
                while (index < 10)
                {
                    value = randPSX() % 16;
                    value = r_s16(0x8007DAE0 + (uint32)value * 2);
                    for (previous = 0; previous < index; ++previous)
                    {
                        if (r_s16(0x800A5098 + previous * 2) == value)
                            break;
                    }
                    if (previous != index)
                        continue;
                    w_u16(0x800A5098 + index * 2, (uint32)value);
                    ++index;
                }
            }
            list = 0x800A5098;
            category = 0;
        }
        else if (selected == 5)
        {
            selected = r_u32(0x80087718);
            limit = 8;
            list = selected != 0 ? 0x8007DB28 : 0x8007DB04;
            category = 0;
        }
        else
            category = 2;
    }
    if (r_u32(0x80087724) != 0)
    {
        list = 0x8007DB4C;
        category = 0;
        limit = 44;
    }
    if (r_u32(0x8008772C) != 0)
        category = 3;
    if (r_u32(0x800BC174) == 1 && r_u32(0x8008771C) == 3)
    {
        list = 0x8007DB4C;
        category = 0;
        limit = 44;
    }
    if (r_u32(0x80087734) != 0)
        category = 0;
    w_u32(0x800BF114, 0);
    for (index = 0; index < limit; ++index)
    {
        value = r_s16(list + index * 2);
        if (value == -2)
            continue;
        if (value == -1)
            break;
        count = r_u32(0x800BF114);
        w_u16(0x800BF3B8 + count * 2, (uint32)value);
        w_u32(0x800BF114, count + 1);
    }
    if (category == 1 || category == 3)
    {
        for (group = 2; group != 0; --group)
        {
            for (index = 0; index < 44; ++index)
            {
                if (r_u8(0x8009DB45 + index) != group)
                    continue;
                if (category == 3 && (r_u8(0x800BC128 + index) & 2) == 0)
                    continue;
                count = r_u32(0x800BF114);
                value = (sint32)(group == 1 ? index | 0x100 : index);
                w_u16(0x800BF3B8 + count * 2, (uint32)value);
                w_u32(0x800BF114, count + 1);
            }
        }
    }
    else if (category == 2)
    {
        cursor = 0;
        for (index = 0; index < 44; ++index)
        {
            if (r_u8(0x8009DB45 + index) != 2)
                continue;
            if (r_u8(0x800BC26C + cursor) != index)
                continue;
            count = r_u32(0x800BF114);
            ++cursor;
            w_u16(0x800BF3B8 + count * 2, index);
            w_u32(0x800BF114, count + 1);
        }
    }
    count = r_u32(0x800BF114);
    w_u32(0x800BC164, 0);
    if ((sint32)count <= 0)
        return count;
    selected = r_u32(0x8008768C);
    for (index = 0; (sint32)index < (sint32)count; ++index)
    {
        result = r_u8(0x800BF3B8 + index * 2);
        if (selected == result)
        {
            w_u32(0x800BC164, index);
            return result;
        }
    }
    return 0;
}

uint32 td5_800b3c50(void)
{
    uint32 slot, entry, source, size, buffer, index, cursor, count, model, texture, result;

    FUNCTION_MARKER(0x800B3C50, "SLUS_006.10");
    slot = r_u32(0x800BF10C);
    entry = 0x800BC928 + slot * 12;
    if (r_u8(entry) != 2)
        return 2;
    source = r_u32(0x800BF418);
    size = td5_80015a3c(source);
    buffer = td5_800b61bc(slot);
    source = r_u32(0x800BF418);
    w_u32(entry + 8, buffer);
    td5_800158d4(source, buffer, (sint32)size);
    td5_free_memory(r_u32(0x800BF418));
    buffer = r_u32(entry + 8);
    w_u32(0x800BF418, 0);
    count = r_u32(buffer);
    if (count != 0)
    {
        index = 1;
        cursor = buffer + 4;
        do
        {
            w_u32(cursor, r_u32(cursor) + buffer);
            count = r_u32(buffer);
            ++index;
            cursor += 4;
        } while (count >= index);
    }
    buffer = r_u32(entry + 8);
    w_u8(entry, 3);
    model = r_u32(buffer + 4);
    w_u32(0x800BC898, buffer);
    w_u32(0x800BC894, model);
    td5_8002e28c(model);
    td5_800b7d6c(r_u32(0x800BC894));
    td5_800b4cbc();
    texture = td5_find_texture((const char *)psx_addr(0x800B0C80, 8));
    buffer = r_u32(0x800BC898);
    model = r_u32(buffer + 4);
    td5_80048b94(model, texture, 0);
    result = td5_800b5944();
    td5_debug_log(0x800B0C88);
    return result;
}

uint32 td5_800b5944(void)
{
    uint32 buffer, data;

    FUNCTION_MARKER(0x800B5944, "SLUS_006.10");
    buffer = r_u32(0x800BC898);
    data = r_u32(buffer + 0x54);
    return td5_80033030(0, data);
}

uint32 td5_800b61bc(uint32 slot)
{
    uint32 count, index = 0, offset = 0, entry, age, oldest = 0, chosen = 0, bank;

    FUNCTION_MARKER(0x800B61BC, "SLUS_006.10");
    count = r_u32(0x800BCCA0);
    if ((sint32)count > 0)
    {
        do
        {
            if (r_s8(0x800BCB40 + offset) == -1)
            {
                w_u8(0x800BC92C + slot * 12, index);
                w_u8(0x800BCB40 + offset, slot);
                return r_u32(0x800BCB44 + offset);
            }
            count = r_u32(0x800BCCA0);
            offset += 8;
            ++index;
        } while ((sint32)index < (sint32)count);
    }
    for (index = 0; index < 44; ++index)
    {
        entry = 0x800BC928 + index * 12;
        if (r_s8(entry + 4) == -1)
            continue;
        age = r_u16(entry + 2);
        if ((sint32)oldest < (sint32)age)
        {
            oldest = age;
            chosen = index;
        }
    }
    td5_debug_log(0x800B0DD4, chosen);
    entry = 0x800BC928 + chosen * 12;
    bank = (uint32)r_s8(entry + 4);
    w_u8(0x800BC92C + slot * 12, bank);
    offset = bank << 3;
    w_u8(0x800BCB40 + offset, slot);
    w_u8(entry + 4, 0xFF);
    w_u8(entry, 0);
    return r_u32(0x800BCB44 + offset);
}

uint32 td5_800b4cbc(void)
{
    uint32 texture, clut, page, index, buffer, image, u, v, image_x, image_y;

    FUNCTION_MARKER(0x800B4CBC, "SLUS_006.10");
    texture = td5_find_texture((const char *)psx_addr(0x800B0C80, 8));
    clut = r_u16(texture + 10);
    page = r_u16(texture + 8);
    index = r_u32(0x800BCB38);
    buffer = r_u32(0x800BC898);
    image = r_u32(buffer + (index << 2) + 16);
    image_x = (page << 6) & 0x3C0;
    image_y = ((page << 4) & 0x100) | ((page >> 2) & 0x200);
    td5_upload_tim(image, (sint16)image_x, (sint16)image_y, (sint16)((clut & 63) << 4), (sint16)((clut >> 6) & 255));
    texture = td5_find_texture((const char *)psx_addr(0x800B0D88, 8));
    page = r_u16(texture + 8);
    u = r_u8(texture + 14);
    clut = r_u16(texture + 10);
    v = r_u8(texture + 15);
    index = r_u32(0x800BCB3C);
    buffer = r_u32(0x800BC898);
    image = r_u32(buffer + (index << 2) + 36);
    image_x = ((page << 6) & 0x3FF) | (u >> 2);
    image_y = (((page << 4) & 0x100) + ((page >> 2) & 0x200)) | v;
    return (uint32)td5_upload_tim(image, (sint16)image_x, (sint16)image_y, (sint16)((clut & 63) << 4), (sint16)((clut >> 6) & 255));
}

uint32 td5_800b7d6c(uint32 model)
{
    uint32 records, first, second, offset, texture, type, clut, value;
    sint32 count, index;

    FUNCTION_MARKER(0x800B7D6C, "SLUS_006.10");
    records = r_u32(model + 48);
    offset = r_u32(model + 52);
    first = model + offset;
    offset = r_u32(model + 56);
    records += model;
    second = model + offset;
    texture = td5_find_texture((const char *)psx_addr(0x800B0E3C, 8));
    if (r_u8(records + 3) == 0)
        return texture;
    do
    {
        type = r_u8(records + 2);
        count = r_s16(records);
        records += 4;
        for (index = 0; type <= 3 && index < count; ++index)
        {
            if (type == 0)
            {
                clut = r_u16(texture + 10);
                w_u16(first + 14, clut);
                w_u16(second + 14, clut);
                first += 32;
                second += 32;
                records += 20;
            }
            else if (type == 1)
            {
                clut = r_u16(texture + 10);
                w_u16(first + 14, clut);
                w_u16(second + 14, clut);
                value = r_u8(first + 7);
                w_u8(first + 7, value | 1);
                value = r_u8(second + 7);
                w_u8(second + 7, value & 0xFE);
                first += 40;
                second += 40;
                records += 20;
            }
            else if (type == 2)
            {
                first += 40;
                second += 40;
                records += 44;
            }
            else if (type == 3)
            {
                first += 52;
                second += 52;
                records += 56;
            }
        }
    } while (r_u8(records + 3) != 0);
    return 0;
}

uint32 td5_800b42f0(void)
{
    uint32 selected, flag, offset;

    FUNCTION_MARKER(0x800B42F0, "SLUS_006.10");
    selected = r_u32(0x800BC174) - 1;
    if ((sint32)selected < 0)
        selected = 0;
    else if ((sint32)selected >= 2)
        selected = 1;
    flag = r_u32(0x800BC154);
    offset = selected * 24;
    w_u32(0x8008768C, selected);
    w_u8(0x8008CF71 + offset, flag);
    w_u8(0x8008CF72 + offset, r_u32(0x800BC89C));
    w_u8(0x8008CF74 + offset, r_u32(0x800BCB38));
    w_u8(0x8008CF76 + offset, r_u32(0x800BCB3C));
    w_u16(0x8008CF78 + offset, r_u16(0x800BC884));
    w_u16(0x8008CF7A + offset, r_u16(0x800BC886));
    w_u16(0x8008CF7C + offset, r_u16(0x800BC888));
    w_u16(0x8008CF7E + offset, r_u16(0x800BC88A));
    return offset;
}

uint32 td5_800b46d8(uint32 original_stack)
{
    uint32 frame = original_stack - 0x58;
    uint32 height, first, second, third, value, reference, object, selected, state, flag, text, length;
    sint32 dx, dy, dz;

    FUNCTION_MARKER(0x800B46D8, "SLUS_006.10");
    height = r_u32(0x800BC160);
    first = r_u32(0x800B0CAC);
    second = r_u32(0x800B0CB0);
    third = r_u32(0x800B0CB4);
    w_u32(frame + 0x10, first);
    w_u32(frame + 0x14, second);
    w_u32(frame + 0x18, third);
    first = r_u32(0x800B0CB8);
    second = r_u32(0x800B0CBC);
    third = r_u32(0x800B0CC0);
    w_u32(frame + 0x1C, first);
    w_u32(frame + 0x20, second);
    w_u32(frame + 0x24, third);
    first = r_u32(0x800B0CC4);
    second = r_u32(0x800B0CC8);
    w_u32(frame + 0x28, first);
    w_u32(frame + 0x2C, second);
    td5_set_view_center(256, (sint32)(height + 60));
    td5_find_texture((const char *)psx_addr(0x800B0CCC, 8));
    value = r_u32(0x800BC15C);
    if ((sint32)value < 3841)
    {
        selected = r_u8(0x800BC89C);
        if (r_u8(0x800BC928 + selected * 12) == 3)
        {
            object = r_u32(0x800BC894);
            value *= 40000;
            w_u32(object + 16, 0);
            w_u32(object + 20, 0);
            value = (uint32)((sint32)value / 4096);
            w_u32(object + 24, (value + 1600) << 8);
            value = r_u16(0x800BC86A) + 16;
            w_u16(0x800BC86A, value);
            RotMatrixY((sint16)value, (MATRIX *)psx_addr(frame + 0x10, 32));
            RotMatrixX(-240, (MATRIX *)psx_addr(frame + 0x10, 32));
            td5_apply_light_matrix(frame + 0x10);
            td5_800797fc(frame + 0x10, object);
            value = r_u32(object + 16);
            reference = r_u32(0x8009765C);
            value = (uint32)((sint32)value / 256) - reference;
            dx = (sint32)value / 4;
            w_u32(frame + 0x30, (uint32)dx);
            value = r_u32(object + 20);
            reference = r_u32(0x80097660);
            value = (uint32)((sint32)value / 256) - reference;
            dy = (sint32)value / 4;
            w_u32(frame + 0x34, (uint32)dy);
            value = r_u32(object + 24);
            reference = r_u32(0x80097664);
            value = (uint32)((sint32)value / 256) - reference;
            dz = (sint32)value / 4;
            w_u32(frame + 0x38, (uint32)dz);
            SetRotMatrix((MATRIX *)psx_addr(0x80097648, 20));
            ApplyRotMatrixLVPSX(frame + 0x30, frame + 0x30);
            height = r_u32(0x800BC160);
            w_u16(0x8007E09C, 256);
            w_u8(0x80087838, 64);
            w_u8(0x80087839, 64);
            w_u8(0x8008783A, 64);
            w_u16(0x8007E09E, height + 70);
            td5_800797fc(frame + 0x10, object);
            value = (uint32)dx * (uint32)dx + (uint32)dy * (uint32)dy + (uint32)dz * (uint32)dz;
            value = td5_integer_sqrt(value);
            td5_8002f384(object, 0x8007E09C, value << 2);
            td5_8006e180(object, frame);
            td5_800b5970(frame);
        }
    }
    state = r_u32(0x800BC8A0);
    if ((sint32)state < 12)
        w_u32(0x800BC870, state);
    if (r_u32(0x8008772C) != 0)
    {
        td5_800b32e4(2);
        state = r_u32(0x800BC174);
        if (state == 1)
        {
            td5_carousel_draw_guest_text(131, 256, 32, 0x800B0CD4);
            state = r_u32(0x800BC174);
        }
        if (state == 2)
            td5_carousel_draw_guest_text(131, 256, 32, 0x800B0CE0);
    }
    if (r_u32(0x800BC870) < 3)
    {
        td5_800b32e4(2);
        if (r_u32(0x80087720) == 3 && r_u32(0x800BC174) == 2)
            td5_carousel_draw_guest_text(131, 256, 20, 0x800B0CEC);
        else
        {
            td5_carousel_draw_guest_text(131, 256, 20, 0x800B0D08);
            if (r_u32(0x80087720) - 4 >= 2)
            {
                selected = r_u8(0x800BC89C);
                if ((r_u8(0x800BC128 + selected) & 1) != 0 && r_u8(0x800BC928 + selected * 12) == 3 && r_u32(0x800BC8A0) == 2)
                    td5_carousel_draw_guest_text(129, 256, 135, 0x800B0D20);
            }
        }
    }
    if (r_u32(0x800BC870) - 6 < 3)
    {
        td5_800b32e4(2);
        td5_carousel_draw_guest_text(131, 256, 20, 0x800B0D40);
        if (r_u32(0x800BC870) == 7)
            w_u8(0x800BD070, 1);
        flag = r_u32(0x800BC154);
        w_u8(0x800BD0E8, 1);
        value = r_u32(0x800BC880);
        if (flag == 0)
        {
            w_u16(0x800BD07A, 100);
            w_u16(0x800BD0F2, 100);
            first = 184 - value;
            second = value + 328;
        }
        else
        {
            w_u16(0x800BD07A, 120);
            w_u16(0x800BD0F2, 120);
            first = 202 - value;
            second = value + 310;
        }
        w_u16(0x800BD078, first);
        w_u16(0x800BD0F0, second);
        td5_800b32e4(r_u32(0x800BC154) == 0 ? 1 : 2);
        td5_carousel_draw_guest_text(130, 256, 90, 0x800B0D5C);
        td5_800b32e4(r_u32(0x800BC154) != 0 ? 1 : 2);
        td5_carousel_draw_guest_text(130, 256, 110, 0x800B0D68);
    }
    if (r_u32(0x800BC870) - 9 >= 3)
        return 0;
    td5_800b32e4(2);
    text = 0x800B0D70;
    length = 0;
    while (r_u8(text + length) != 0)
        ++length;
    return td5_800b2790(131, 256, 20, (const char *)psx_addr(text, length + 1));
}

uint32 td5_800b44f8(uint32 original_stack)
{
    uint32 frame = original_stack - 0x130;
    uint32 selected, column, index, y, label, value, label_bytes, value_bytes, bytes, x;

    FUNCTION_MARKER(0x800B44F8, "SLUS_006.10");
    index = r_u32(0x800BC164);
    selected = r_u8(0x800BF3B8 + (index << 1));
    td5_800b32e4(0);
    for (column = 0; column < 2; ++column)
    {
        y = (r_u32(0x8008810C) << 3) + 160;
        w_u32(0x800BF0F0, column == 0 ? 264 : 482);
        w_u32(0x800BF420, 0);
        x = column == 0 ? 30 : 280;
        for (index = 0; index < 8; ++index)
        {
            label = r_u32(0x8007EA60 + column * 32 + index * 4);
            value = r_u32(0x8007EAA4 + selected * 68 + column * 32 + index * 4);
            label_bytes = 0;
            while (r_u8(label + label_bytes) != 0)
                ++label_bytes;
            value_bytes = 0;
            while (r_u8(value + value_bytes) != 0)
                ++value_bytes;
            bytes = label_bytes + value_bytes + 1;
            sprintf((char *)psx_addr(frame + 0x10, bytes), (const char *)psx_addr(0x800B0CA4, 5), (const char *)psx_addr(label, label_bytes + 1), (const char *)psx_addr(value, value_bytes + 1));
            td5_800b2790(1, x, (sint32)y, (const char *)psx_addr(frame + 0x10, bytes));
            y += 8;
        }
    }
    return 0;
}

uint32 td5_800b7f0c(uint32 mode)
{
    uint32 result;

    FUNCTION_MARKER(0x800B7F0C, "SLUS_006.10");
    w_u32(0x800B02D0, 0);
    w_u32(0x800BF110, 0);
    w_u32(0x800BC3F8, 0);
    w_u32(0x800BF3A0, mode);
    w_u32(0x800BC3FC, 0);
    w_u16(0x800BC408, r_u16(0x800BC408) & 0xFFF);
    w_u16(0x800BCF10, r_u16(0x800BCF10) & 0xFFF);
    w_u16(0x800BC420, r_u16(0x800BC420) & 0xFFF);
    w_u16(0x800BCF88, r_u16(0x800BCF88) & 0xFFF);
    w_u16(0x800BC438, r_u16(0x800BC438) & 0xFFF);
    w_u16(0x800BD000, r_u16(0x800BD000) & 0xFFF);
    if (r_u32(0x800875D0) != 0)
    {
        w_u32(0x800BCF84, 0x800B1040);
        w_u16(0x800BC438, r_u16(0x800BC438) | 0x4000);
        w_u16(0x800BD000, r_u16(0x800BD000) | 0x4000);
    }
    if (r_u32(0x800BF3A0) == 1)
    {
        w_u16(0x800BC408, r_u16(0x800BC408) | 0x4000);
        w_u16(0x800BCF10, r_u16(0x800BCF10) | 0x4000);
        w_u32(0x800B02D0, 1);
        return 1;
    }
    w_u16(0x800BC408, r_u16(0x800BC408) & 0xFFF);
    result = r_u16(0x800BCF10);
    w_u16(0x800BCF10, result & 0xFFF);
    return result;
}
