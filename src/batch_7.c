#include "td5_functions.h"

uint32 td5_8004dd0c(sint32 node, uint32 slot, uint32 point)
{
    uint32 segment;
    uint32 vertices;
    uint32 type;
    uint32 count;
    uint32 vertex;
    uint32 extension;
    uint32 endpoint;
    uint32 point_z;
    uint32 point_x;
    uint32 origin_x;
    uint32 origin_z;
    uint32 world_x;
    uint32 world_z;
    uint32 x;
    uint32 z;
    uint32 target_x;
    uint32 target_z;
    uint32 dot;
    uint32 denominator;
    uint32 numerator;

    FUNCTION_MARKER(0x8004DD0C, "SLUS_006.10");
    segment = r_u32(0x800884C4) + (uint32)node * 24;
    type = r_u8(segment);
    vertices = r_u32(0x8008850C);
    count = r_u8(segment + 3);
    vertex = r_u16(segment + 6);
    extension = r_u32(0x8007E5AC + type * 8);
    point_z = r_u32(point + 8);
    endpoint = vertices + (vertex + (count & 15) + extension) * 6;
    x = (uint32)r_s16(endpoint);
    origin_x = r_u32(segment + 12);
    world_x = r_u32(0x80098664 + slot * 864);
    world_z = r_u32(0x8009866C + slot * 864);
    x = origin_x + x - (uint32)((sint32)world_x >> 8);
    target_z = (uint32)((sint32)(0 - point_z - world_z) >> 8);
    dot = x * target_z;
    point_x = r_u32(point);
    z = (uint32)r_s16(endpoint + 4);
    origin_z = r_u32(segment + 20);
    target_x = (uint32)((sint32)(point_x - world_x) >> 8);
    z = origin_z + z - (uint32)((sint32)world_z >> 8);
    dot += z * target_x;
    if ((sint32)dot <= 0)
        return 0;
    denominator = x + z;
    numerator = target_z - target_x;
    if (denominator == 0)
    {
        x = 0 - z + 1;
        denominator = x + z;
    }
    return (uint32)td5_divide_checked((sint32)numerator, (sint32)denominator);
}

uint32 td5_8004dfa4(uint32 slot, uint32 selection_vector_address)
{
    uint32 state_offset;
    uint32 selected;
    uint32 owner;
    uint32 other_offset;
    uint32 flag;
    uint32 value;
    uint32 result;
    uint32 segment;
    uint32 count;
    uint32 lane;
    sint32 node;
    sint32 other_lane;
    sint32 own_lane;
    sint32 gap;

    FUNCTION_MARKER(0x8004DFA4, "SLUS_006.10");
    state_offset = slot * 284;
    owner = 0x80097750 + state_offset;
    selected = td5_8004c0a4(owner, selection_vector_address);
    if (selected == slot)
    {
        value = r_u32(0x80097774 + state_offset);
        if ((sint32)value < 0)
        {
            result = value + 8;
            w_u32(0x80097774 + state_offset, result);
            if ((sint32)result <= 0)
                return result;
            w_u32(0x80097774 + state_offset, 0);
            value = r_u32(0x80097774 + state_offset);
        }
        result = value - 8;
        if ((sint32)value > 0)
        {
            w_u32(0x80097774 + state_offset, result);
            if ((sint32)result < 0)
                w_u32(0x80097774 + state_offset, 0);
        }
        return result;
    }
    td5_8004c868(owner, 0x80097750 + selected * 284);
    other_offset = selected * 864;
    flag = r_u32(0x800885AC);
    if (flag == 1 && r_u8(0x8009851C + other_offset) == 0)
    {
        w_u32(0x800885AC, 0);
        flag = r_u32(0x800885AC);
    }
    else
    {
        flag = r_u32(0x800885AC);
        if (flag == 0)
        {
            node = r_s16(0x80098510 + other_offset);
            segment = r_u32(0x800884C4) + (uint32)node * 24;
            count = r_u8(segment + 3);
            lane = r_u8(0x8009851C + other_offset);
            if (lane == (count & 15))
                w_u32(0x800885AC, 1);
            flag = r_u32(0x800885AC);
        }
    }
    other_lane = r_s16(0x80098512 + other_offset);
    own_lane = r_s16(0x80098512 + slot * 864);
    gap = other_lane - own_lane;
    if (gap >= 41)
        gap = 40;
    else if (gap <= 0)
        gap = 1;
    value = r_u32(0x80097774 + state_offset);
    if (flag == 0)
        value = value + 41 - (uint32)gap;
    else
        value = value - 41 + (uint32)gap;
    w_u32(0x80097774 + state_offset, value);
    return state_offset;
}

uint32 td5_8004e838(uint32 slot, uint32 vector_address)
{
    uint32 state_offset;
    uint32 car_offset;
    uint32 owner;
    uint32 value;
    uint32 route;
    uint32 primary;
    uint32 mapped;
    uint32 tag;
    uint32 offset;
    uint32 step;
    sint32 lane;
    sint32 threshold;
    sint32 selected;
    sint32 lookup;

    FUNCTION_MARKER(0x8004E838, "SLUS_006.10");
    state_offset = slot * 284;
    car_offset = slot * 864;
    owner = 0x80097750 + state_offset;
    if (r_u32(0x80097838 + state_offset) != 0)
    {
        if (td5_80050798(owner) == 0)
            return td5_8004d660(slot);
    }
    else
    {
        value = td5_8004d19c(owner);
        if (value - 801 < 0x9BF)
        {
            w_u32(0x80097838 + state_offset, 0x8007E608);
            w_u32(0x8009783C + state_offset, 0x8007E608);
        }
    }
    route = r_u32(owner);
    primary = r_u32(0x800886B8);
    lane = r_s16(0x80098512 + car_offset);
    threshold = r_s32(0x8008841C);
    selected = lane + 4;
    if (selected >= threshold)
        selected = (sint32)((uint32)lane - ((uint32)threshold - 4));
    if (route != primary)
    {
        mapped = td5_80050704(selected);
        selected = (sint32)mapped;
        if (mapped == 0xFFFFFFFF)
        {
            lane = r_s16(0x80098512 + car_offset);
            threshold = r_s32(0x8008841C);
            selected = lane + 4;
            if (selected >= threshold)
                selected = (sint32)((uint32)lane - ((uint32)threshold - 4));
        }
    }
    td5_8004dfa4(slot, vector_address - 0x90);
    lane = r_s16(0x80098512 + car_offset);
    threshold = r_s32(0x8008841C);
    route = r_u32(owner);
    lookup = lane + 4;
    if (lookup >= threshold)
        lookup = (sint32)((uint32)lane - ((uint32)threshold - 4));
    tag = r_u8(route + (uint32)lookup * 3);
    offset = r_u32(0x80097774 + state_offset);
    td5_8004de3c(selected, tag, vector_address, offset);
    td5_8004dd0c(selected, slot, vector_address);
    td5_8004d268(owner, vector_address);
    value = td5_8004e29c(slot);
    step = value != 0 ? 0x10000 : 0x20000;
    return td5_8004d330(owner, step);
}

uint32 td5_8004d19c(uint32 owner)
{
    uint32 car_offset;
    uint32 route;
    uint32 heading;
    uint32 tag;
    uint32 angle;
    uint32 difference;
    sint32 lane;

    FUNCTION_MARKER(0x8004D19C, "SLUS_006.10");
    car_offset = r_u32(owner + 0xD4) * 864;
    lane = r_s16(0x80098512 + car_offset);
    route = r_u32(owner);
    heading = r_u32(0x8009865C + car_offset);
    tag = r_u8(route + (uint32)lane * 3 + 1);
    angle = tag * 4140 / 256;
    difference = ((uint32)((sint32)heading >> 8) & 0xFFF) - 2048;
    difference = (difference - angle) & 0xFFF;
    difference = (difference + 2048) & 0xFFF;
    return (0 - difference) & 0xFFF;
}

uint32 td5_8004fa10(uint32 slot)
{
    uint32 car_offset;
    uint32 state_offset;
    uint32 index;
    uint32 value;
    uint32 first;
    uint32 second;
    uint32 offset;
    uint32 route;
    uint32 tag;
    sint32 minimum;
    sint32 maximum;
    sint32 sample;
    sint32 lane;
    sint32 node;

    FUNCTION_MARKER(0x8004FA10, "SLUS_006.10");
    car_offset = slot * 864;
    state_offset = slot * 284;
    for (index = 0; index < 4; ++index)
    {
        node = r_s16(0x80098510 + car_offset);
        value = td5_8004da54(node, 0x80098520 + car_offset + index * 12);
        w_u32(0x80097778 + state_offset + index * 4, value);
    }
    minimum = 65536;
    maximum = -65536;
    for (index = 0; index < 4; ++index)
    {
        sample = r_s32(0x80097778 + state_offset + index * 4);
        if (sample < minimum)
            minimum = sample;
        if (maximum < sample)
            maximum = sample;
    }
    first = r_u32(0x8009777C + state_offset);
    second = r_u32(0x80097778 + state_offset);
    w_u32(0x80097764 + state_offset, (uint32)minimum);
    w_u32(0x80097768 + state_offset, (uint32)maximum);
    w_u32(0x8009776C + state_offset, first);
    w_u32(0x80097770 + state_offset, second);
    lane = r_s16(0x80098512 + car_offset);
    offset = r_u32(0x80097764 + state_offset);
    route = r_u32(0x800886B8);
    node = r_s16(0x80098510 + car_offset);
    tag = r_u8(route + (uint32)lane * 3);
    value = td5_8004dbe8(node, offset, tag);
    w_u32(0x80097790 + state_offset, value);
    for (index = 0; index < 3; ++index)
    {
        lane = r_s16(0x80098512 + car_offset);
        node = r_s16(0x80098510 + car_offset);
        route = r_u32(index == 0 ? 0x800886B8 : 0x800887A8);
        offset = r_u32(0x80097764 + state_offset + (index == 1 ? 0 : 4));
        tag = r_u8(route + (uint32)lane * 3);
        value = td5_8004dbe8(node, offset, tag);
        w_u32(0x80097794 + state_offset + index * 4, value);
    }
    return value;
}

uint32 td5_8004de3c(sint32 node, uint32 factor, uint32 output, uint32 offset)
{
    SVECTOR normal;
    uint32 segment;
    uint32 vertices;
    uint32 vertex;
    uint32 type;
    uint32 count;
    uint32 origin_x;
    uint32 origin_z;
    uint32 extension;
    uint32 first;
    uint32 second;
    uint32 start_x;
    uint32 start_z;
    uint32 delta_x;
    uint32 delta_z;
    uint32 value;
    uint32 result;
    sint32 x;
    sint32 z;
    sint32 step;

    FUNCTION_MARKER(0x8004DE3C, "SLUS_006.10");
    segment = r_u32(0x800884C4);
    vertices = r_u32(0x8008850C);
    segment += (uint32)node * 24;
    vertex = r_u16(segment + 4);
    type = r_u8(segment);
    count = r_u8(segment + 3);
    origin_x = r_u32(segment + 12);
    extension = r_u32(0x8007E5A8 + type * 8);
    first = vertices + vertex * 6;
    x = r_s16(first);
    second = vertices + (vertex + (count & 15) + extension) * 6;
    z = r_s16(second);
    start_x = origin_x + (uint32)x;
    delta_x = origin_x + (uint32)z - start_x;
    origin_z = r_u32(segment + 20);
    x = r_s16(first + 4);
    z = r_s16(second + 4);
    start_z = origin_z + (uint32)x;
    delta_z = origin_z + (uint32)z - start_z;
    w_u32(output, (start_x << 8) + factor * delta_x);
    w_u32(output + 8, (start_z << 8) + factor * delta_z);
    normal.vx = (sint16)delta_x;
    normal.vy = 0;
    normal.vz = (sint16)delta_z;
    VectorNormalSS(&normal, &normal);
    step = (sint32)((uint32)(sint32)normal.vx * offset) / 4096;
    value = r_u32(output);
    w_u32(output, value + ((uint32)step << 8));
    step = (sint32)((uint32)(sint32)normal.vz * offset) / 4096;
    value = r_u32(output + 8);
    result = (uint32)step << 8;
    w_u32(output + 8, value + result);
    return result;
}

uint32 td5_8004fce4(uint32 slot, uint32 offset, uint32 vector_address)
{
    uint32 car_offset;
    uint32 state_offset;
    uint32 route;
    uint32 primary;
    uint32 descriptor;
    uint32 tag;
    uint32 width;
    uint32 mapped;
    sint32 lane;
    sint32 threshold;
    sint32 lookahead;
    sint32 selected;

    FUNCTION_MARKER(0x8004FCE4, "SLUS_006.10");
    car_offset = slot * 864;
    state_offset = slot * 284;
    route = r_u32(0x80097750 + state_offset);
    primary = r_u32(0x800886B8);
    lane = r_s16(0x80098512 + car_offset);
    threshold = r_s32(0x8008841C);
    selected = lane + 4;
    if (selected >= threshold)
        selected = (sint32)((uint32)lane - ((uint32)threshold - 4));
    if (route != primary)
    {
        mapped = td5_80050704(selected);
        selected = (sint32)mapped;
        if (mapped == 0xFFFFFFFF)
        {
            lane = r_s16(0x80098512 + car_offset);
            threshold = r_s32(0x8008841C);
            selected = lane + 4;
            if (selected >= threshold)
                selected = (sint32)((uint32)lane - ((uint32)threshold - 4));
        }
    }
    lane = r_s16(0x80098512 + car_offset);
    threshold = r_s32(0x8008841C);
    route = r_u32(0x80097750 + state_offset);
    lookahead = lane + 4;
    if (lookahead >= threshold)
        lookahead = (sint32)((uint32)lane - ((uint32)threshold - 4));
    descriptor = r_u32(0x80098620 + car_offset);
    width = (uint32)r_s16(descriptor + 8);
    tag = r_u8(route + (uint32)lookahead * 3);
    td5_8004de3c(selected, tag, vector_address, offset + width);
    if ((sint32)td5_8004da54(selected, vector_address) >= 255)
        return 1;
    lane = r_s16(0x80098512 + car_offset);
    threshold = r_s32(0x8008841C);
    route = r_u32(0x80097750 + state_offset);
    lookahead = lane + 4;
    if (lookahead >= threshold)
        lookahead = (sint32)((uint32)lane - ((uint32)threshold - 4));
    descriptor = r_u32(0x80098620 + car_offset);
    width = (uint32)r_s16(descriptor);
    tag = r_u8(route + (uint32)lookahead * 3);
    td5_8004de3c(selected, tag, vector_address, offset + width);
    return (sint32)td5_8004da54(selected, vector_address) < 1 ? 2 : 0;
}

sint32 td5_8004ff8c(sint32 node, uint32 lane)
{
    uint32 segment;
    uint32 flags;
    uint32 packed;

    FUNCTION_MARKER(0x8004FF8C, "SLUS_006.10");
    segment = r_u32(0x800884C4) + (uint32)node * 24;
    flags = r_u8(segment + 2);
    if (((flags >> (lane & 31)) & 1) != 0)
    {
        packed = r_u8(segment + 1);
        return (sint32)((packed >> 4) | 16);
    }
    return r_u8(segment + 1) & 15;
}

uint32 td5_8004c9ec(uint32 slot)
{
    uint32 car_offset;
    uint32 route;
    uint32 tag;
    uint32 sum;
    uint32 first;
    uint32 second;
    sint32 lane;
    sint32 angle;
    sint32 sine;
    sint32 cosine;
    sint32 length;
    sint32 coordinate;

    FUNCTION_MARKER(0x8004C9EC, "SLUS_006.10");
    car_offset = slot * 864;
    lane = r_s16(0x80098512 + car_offset);
    route = r_u32(0x80097750 + slot * 284);
    tag = r_u8(route + (uint32)lane * 3 + 1);
    angle = (sint32)(tag * 4140) / 256;
    sine = rsin(angle);
    cosine = rcos(angle);
    sum = (uint32)sine * (uint32)sine + (uint32)cosine * (uint32)cosine;
    length = SquareRoot0((sint32)sum);
    coordinate = r_s32(0x80098634 + car_offset) >> 8;
    first = (uint32)coordinate * (uint32)sine;
    coordinate = r_s32(0x8009863C + car_offset) >> 8;
    second = (uint32)coordinate * (uint32)cosine;
    return (uint32)td5_divide_checked((sint32)(first + second), length);
}

uint32 td5_8004cb38(uint32 owner, uint32 original_stack)
{
    uint32 car_offset;
    uint32 segment;
    uint32 vertices;
    uint32 vertex;
    uint32 other_vertex;
    uint32 lane;
    uint32 origin_x;
    uint32 origin_z;
    uint32 type;
    uint32 first;
    uint32 other;
    uint32 next;
    uint32 local;
    uint32 coordinate;
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;
    uint32 product;
    uint32 determinant;
    uint32 numerator;
    uint32 result;
    sint32 node;
    sint32 left;
    sint32 right;
    sint32 difference;

    FUNCTION_MARKER(0x8004CB38, "SLUS_006.10");
    car_offset = r_u32(owner + 0xD4) * 864;
    node = r_s16(0x80098510 + car_offset);
    segment = r_u32(0x800884C4);
    vertices = r_u32(0x8008850C);
    segment += (uint32)node * 24;
    vertex = r_u16(segment + 4);
    lane = r_u8(0x8009851C + car_offset);
    origin_x = r_u32(segment + 12);
    other_vertex = r_u16(segment + 6);
    type = r_u8(segment);
    origin_z = r_u32(segment + 20);
    local = original_stack - 0x30;
    if (type >= 1 && type <= 7)
    {
        if ((type == 3 || type == 4 || type == 6 || type == 7) && lane == 0)
            lane = 1;
        first = vertices + vertex * 6 + lane * 6;
        other = vertices + other_vertex * 6 + lane * 6;
        if (type == 3 || type == 4)
        {
            next = first;
            first -= 6;
        }
        else
        {
            next = first + 6;
            if (type == 6 || type == 7)
                other -= 6;
        }
        left = r_s16(other);
        right = r_s16(first);
        w_u32(local, (uint32)((left - right) / 2));
        left = r_s16(other + 4);
        right = r_s16(first + 4);
        w_u32(local + 8, (uint32)((left - right) / 2));
        car_offset = r_u32(owner + 0xD4) * 864;
        coordinate = (uint32)(r_s32(0x80098664 + car_offset) >> 8);
        right = r_s16(first);
        difference = (sint32)(coordinate - (origin_x + (uint32)right));
        w_u32(local + 0x20, (uint32)(difference / 2));
        car_offset = r_u32(owner + 0xD4) * 864;
        coordinate = (uint32)(r_s32(0x8009866C + car_offset) >> 8);
        right = r_s16(first + 4);
        difference = (sint32)(coordinate - (origin_z + (uint32)right));
        w_u32(local + 0x28, (uint32)(difference / 2));
        left = r_s16(next);
        right = r_s16(first);
        w_u32(local + 0x10, (uint32)((left - right) / 2));
        left = r_s16(next + 4);
        right = r_s16(first + 4);
        w_u32(local + 0x18, (uint32)((left - right) / 2));
    }
    a = r_u32(local);
    b = r_u32(local + 0x18);
    product = a * b;
    c = r_u32(local + 0x10);
    d = r_u32(local + 8);
    result = d * c;
    determinant = product - result;
    if (determinant == 0)
        return result;
    a = r_u32(local + 0x20);
    product = a * b;
    a = r_u32(local + 0x28);
    numerator = (product - a * c) << 8;
    result = (uint32)td5_divide_checked((sint32)numerator, (sint32)determinant);
    w_u32(owner + 0xDC, result);
    a = r_u32(local + 0x28);
    b = r_u32(local);
    product = a * b;
    a = r_u32(local + 0x20);
    b = r_u32(local + 8);
    numerator = (product - a * b) << 8;
    result = (uint32)td5_divide_checked((sint32)numerator, (sint32)determinant);
    w_u32(owner + 0xE0, result);
    return result;
}

uint32 td5_800419f4(void)
{
    uint32 count;
    uint32 index;
    uint32 address;

    FUNCTION_MARKER(0x800419F4, "SLUS_006.10");
    count = r_u32(0x8007E394);
    if ((sint32)count <= 0)
        return count;
    index = 0;
    address = 0x8009130C;
    do
    {
        w_u32(address, r_u32(address) + 16);
        count = r_u32(0x8007E394);
        ++index;
        address += 208;
    } while ((sint32)index < (sint32)count);
    return 0;
}

uint32 td5_8004b0d0(void)
{
    uint32 index;
    uint32 count;
    uint32 value;
    uint32 factor;
    sint32 position;
    sint32 player_position;
    sint32 difference;
    sint32 divisor;
    sint32 quotient;

    FUNCTION_MARKER(0x8004B0D0, "SLUS_006.10");
    if (r_u32(0x8008772C) != 0)
    {
        count = r_u32(0x800875E4);
        index = 2;
        while ((sint32)count >= 7 ? (sint32)index < 6 : (sint32)index < (sint32)count)
        {
            count = r_u32(0x800875E4);
            w_u32(0x8007E66C + index * 4, 256);
            w_u32(0x80097760 + index * 284, 256);
            ++index;
        }
        return 0;
    }
    count = r_u32(0x800875E4);
    index = 1;
    while ((sint32)count >= 7 ? (sint32)index < 6 : (sint32)index < (sint32)count)
    {
        position = r_s16(0x80098514 + index * 864);
        player_position = r_s16(0x80098514);
        difference = position - player_position;
        w_u32(0x800882D0, (uint32)difference);
        if (difference < 0)
        {
            divisor = r_s32(0x80087A88);
            if (divisor < difference)
                w_u32(0x800882D0, (uint32)divisor);
            value = r_u32(0x800882D0);
            factor = r_u32(0x80087A80);
        }
        else
        {
            divisor = r_s32(0x80087A8C);
            if (divisor < difference)
                w_u32(0x800882D0, (uint32)divisor);
            value = r_u32(0x800882D0);
            factor = r_u32(0x80087A84);
        }
        quotient = td5_divide_checked((sint32)(value * factor), divisor);
        w_u32(0x800882D0, (uint32)quotient);
        value = r_u32(0x800882D0);
        count = r_u32(0x800875E4);
        w_u32(0x80097760 + index * 284, 256 - value);
        ++index;
    }
    return index << 3;
}

static uint32 td5_ai_route_offset(uint32 car_offset, uint32 state_offset, uint32 route_global)
{
    sint32 lane;
    sint32 node;
    uint32 route;
    uint32 offset;
    uint32 tag;

    lane = r_s16(0x80098512 + car_offset);
    node = r_s16(0x80098510 + car_offset);
    route = r_u32(route_global);
    offset = r_u32(0x800977B4 + state_offset);
    tag = r_u8(route + (uint32)lane * 3);
    return td5_8004dbe8(node, offset, tag);
}

uint32 td5_8004ffe4(uint32 original_stack, uint32 physics_force_carrier)
{
    uint32 index;
    uint32 count;
    uint32 car_offset;
    uint32 state_offset;
    uint32 value;
    uint32 route;
    uint32 descriptor;
    uint32 tag;
    uint32 saved;
    uint32 copy;
    uint32 height_address;
    sint32 node;
    sint32 lane;
    sint32 threshold;
    sint32 edge;

    FUNCTION_MARKER(0x8004FFE4, "SLUS_006.10");
    height_address = original_stack - 0x118;
    td5_8004b0d0();
    count = r_u32(0x800875E4);
    index = 0;
    if ((sint32)count > 0)
    {
        physics_force_carrier = 0x80097788;
        do
        {
            car_offset = index * 864;
            state_offset = index * 284;
            node = r_s16(0x80098510 + car_offset);
            threshold = r_s32(0x8008841C);
            if (threshold < node)
            {
                route = r_u32(0x800887A8);
                w_u32(0x8009775C + state_offset, 1);
                w_u32(0x80097750 + state_offset, route);
            }
            else if (td5_80050704(r_s16(0x80098512 + car_offset)) == 0xFFFFFFFF)
                w_u32(0x8009775C + state_offset, 0);
            else
            {
                route = r_u32(0x800886B8);
                w_u32(0x8009775C + state_offset, 0);
                w_u32(0x80097750 + state_offset, route);
            }
            value = td5_8004c9ec(index);
            w_u32(0x800977B0 + state_offset, value);
            value = td5_8004da54(r_s16(0x80098510 + car_offset), 0x80098664 + car_offset);
            w_u32(0x800977B4 + state_offset, value);
            td5_8004cb38(0x80097750 + state_offset, original_stack - 0x40);
            td5_8004fa10(index);
            value = td5_8004fce4(index, r_u32(0x80097774 + state_offset), original_stack - 0x68);
            if (value == 1)
                edge = 1;
            else
            {
                value = td5_8004fce4(index, r_u32(0x80097774 + state_offset), original_stack - 0x68);
                edge = value == 2 ? 2 : 0;
            }
            if (edge != 0)
            {
                lane = r_s16(0x80098512 + car_offset);
                route = r_u32(0x80097750 + state_offset);
                node = r_s16(0x80098510 + car_offset);
                tag = r_u8(route + (uint32)lane * 3);
                value = td5_8004dbe8(node, edge == 1 ? 256 : 0, tag);
                descriptor = r_u32(0x80098620 + car_offset);
                if (edge == 1)
                    value += (uint32)r_s16(descriptor) - 32;
                else
                    value += (uint32)r_s16(descriptor + 8) + 32;
                w_u32(0x80097774 + state_offset, value);
            }
            node = r_s16(0x80098510 + car_offset);
            tag = r_u8(0x8009851C + car_offset);
            if (td5_8004ff8c(node, tag) >= 16)
                w_u32(0x80097774 + state_offset, 0);
            tag = r_u8(0x800987F3 + car_offset);
            if (tag == 1 || tag == 2)
                w_u32(0x80097774 + state_offset, 0);
            route = r_u32(0x80097750 + state_offset);
            saved = r_u32(0x800886B8);
            if (route == saved)
            {
                if ((sint32)index >= 6 && (index != 9 || r_u32(0x800981C8) == 0xFFFFFFFF))
                {
                    value = td5_ai_route_offset(car_offset, state_offset, 0x800886B8);
                    w_u32(0x80097774 + state_offset, value);
                    w_u32(0x80097788 + state_offset, value);
                }
                else
                    w_u32(0x80097788 + state_offset, r_u32(0x80097774 + state_offset));
                value = td5_ai_route_offset(car_offset, state_offset, 0x800887A8);
                copy = r_u32(0x80097790 + state_offset);
                w_u32(0x8009778C + state_offset, value);
                w_u32(0x800977A4 + state_offset, copy);
                copy = r_u32(0x80097794 + state_offset);
                w_u32(0x800977A0 + state_offset, copy);
            }
            else
            {
                if ((sint32)index >= 6 && (index != 9 || r_u32(0x800981C8) == 0xFFFFFFFF))
                {
                    value = td5_ai_route_offset(car_offset, state_offset, 0x800887A8);
                    w_u32(0x80097774 + state_offset, value);
                }
                else
                    value = r_u32(0x80097774 + state_offset);
                w_u32(0x8009778C + state_offset, value);
                value = td5_ai_route_offset(car_offset, state_offset, 0x800886B8);
                w_u32(0x80097788 + state_offset, value);
                copy = r_u32(0x80097798 + state_offset);
                w_u32(0x800977A4 + state_offset, copy);
                copy = r_u32(0x8009779C + state_offset);
                w_u32(0x800977A0 + state_offset, copy);
            }
            physics_force_carrier += 284;
            count = r_u32(0x800875E4);
            ++index;
        } while ((sint32)index < (sint32)count);
    }
    if ((r_u32(0x800875E8) & 1) != 0)
    {
        td5_8004e838(0, original_stack - 0x80);
        td5_80024c14(0, height_address, physics_force_carrier);
    }
    if (r_u32(0x8008772C) != 0 && (r_u32(0x800875E8) & 2) != 0)
    {
        td5_8004e838(1, original_stack - 0x80);
        td5_80024c14(1, height_address, physics_force_carrier);
    }
    if (r_u32(0x80087630) == 0)
    {
        index = r_u32(0x8008772C);
        count = r_u32(0x800875E4);
        ++index;
        while ((sint32)count >= 7 ? (sint32)index < 6 : (sint32)index < (sint32)count)
        {
            if (r_u32(0x80087628) == 0 && (r_u32(0x80087724) == 0 || r_s16(0x8009DBA0 + index * 2) != 0))
                td5_8004e838(index, original_stack - 0x80);
            td5_80024c14(index, height_address, physics_force_carrier);
            count = r_u32(0x800875E4);
            ++index;
        }
        (void)r_u32(0x80087630);
    }
    count = r_u32(0x800875E4);
    if ((sint32)count < 7)
        return 1;
    index = 6;
    while ((sint32)count >= 13 ? (sint32)index < 12 : (sint32)index < (sint32)count)
    {
        if (index == 9 && r_u32(0x800981C8) != 0xFFFFFFFF)
        {
            td5_8004e838(9, original_stack - 0x80);
            td5_80024c14(9, height_address, physics_force_carrier);
        }
        else
        {
            td5_8004f03c(index, original_stack - 0x40);
            td5_80027fc4(index, original_stack - 0x40);
        }
        count = r_u32(0x800875E4);
        ++index;
    }
    if (r_u32(0x80088578) == 1)
        return td5_8004e5b0(original_stack - 0x120);
    return 1;
}

uint32 td5_8002d79c(uint32 car)
{
    uint32 x;
    uint32 y;
    uint32 z;

    FUNCTION_MARKER(0x8002D79C, "SLUS_006.10");
    td5_8001b310(car + 0x70, car + 0x1C4);
    td5_8001d314(car + 0x70, car + 0x1C4, car + 0x258, (uint32 *)psx_addr(car + 0x2B0, 4));
    x = r_u32(car + 0x1C4);
    y = r_u32(car + 0x1C8);
    z = r_u32(car + 0x1CC);
    w_u32(car + 0x124, (uint32)((sint32)x >> 8));
    w_u32(car + 0x128, (uint32)((sint32)y >> 8));
    w_u32(car + 0x12C, (uint32)((sint32)z >> 8));
    return td5_8002c770(car);
}

static sint32 td5_car_edge_distance(uint32 point, uint32 first, uint32 second, uint32 segment, sint32 second_z)
{
    VECTOR normal;
    sint32 first_z;
    sint32 first_x;
    sint32 second_x;
    uint32 coordinate;
    uint32 origin;
    uint32 product;

    first_z = r_s16(first + 4);
    normal.vy = 0;
    normal.vx = second_z - first_z;
    first_x = r_s16(first);
    second_x = r_s16(second);
    normal.vz = first_x - second_x;
    VectorNormal(&normal, &normal);
    coordinate = (uint32)(r_s32(point) >> 8);
    origin = (uint32)r_s16(first);
    coordinate -= origin;
    coordinate -= r_u32(segment + 12);
    product = coordinate * (uint32)normal.vx;
    coordinate = (uint32)(r_s32(point + 8) >> 8);
    origin = (uint32)r_s16(first + 4);
    coordinate -= origin;
    coordinate -= r_u32(segment + 20);
    return (sint32)(product + coordinate * (uint32)normal.vz);
}

static sint32 td5_car_edge_angle(uint32 car, uint32 first, uint32 second)
{
    sint32 second_z;
    sint32 first_z;
    sint32 second_x;
    sint32 first_x;

    td5_80049310(car, 1);
    second_z = r_s16(second + 4);
    first_z = r_s16(first + 4);
    second_x = r_s16(second);
    first_x = r_s16(first);
    return td5_8007b0c0(second_z - first_z, second_x - first_x);
}

uint32 td5_80029180(uint32 slot, uint32 callback, uint32 list, uint32 original_stack)
{
    uint32 car;
    uint32 result;
    uint32 index;
    uint32 segment;
    uint32 count;
    uint32 vertex;
    uint32 base;
    uint32 first;
    uint32 second;
    uint32 point;
    sint32 node;
    sint32 selected;
    sint32 second_z;
    sint32 distance;
    sint32 angle;

    FUNCTION_MARKER(0x80029180, "SLUS_006.10");
    car = 0x800984A0 + slot * 864;
    result = r_u32(0x80088738);
    node = r_s16(car + 0x70);
    result -= 1;
    if (node < (sint32)result)
        return result;
    for (index = 0; index < 8; ++index)
    {
        selected = r_s8(list + index);
        if (selected < 0)
            return result;
        node = r_s16(car + index * 14);
        segment = r_u32(0x80088738);
        if ((uint32)node == segment)
        {
            base = r_u32(0x800884C4);
            segment = base + (uint32)node * 24;
            count = r_u8(segment + 3);
            vertex = r_u16(segment + 4);
            base = r_u32(0x8008850C);
            first = base + (vertex + (count & 15)) * 6;
            second = base + vertex * 6;
            second_z = r_s16(second + 4);
            point = car + (uint32)selected * 12 + 0x80;
            distance = td5_car_edge_distance(point, first, second, segment, second_z);
            if (distance < 0)
            {
                angle = td5_car_edge_angle(car, first, second);
                td5_800287b8(car, point, angle, (sint32)((uint32)distance + 4095) >> 12);
                td5_call_car_collision_callback(callback, car, original_stack - 0x48);
            }
        }
        result = index + 1 < 8;
    }
    return result;
}

uint32 td5_80028f74(uint32 slot, uint32 callback, uint32 list, uint32 original_stack)
{
    uint32 car;
    uint32 result;
    uint32 index;
    uint32 segment;
    uint32 count;
    uint32 vertex;
    uint32 base;
    uint32 first;
    uint32 second;
    uint32 point;
    uint32 boundary;
    sint32 node;
    sint32 selected;
    sint32 second_z;
    sint32 distance;
    sint32 angle;

    FUNCTION_MARKER(0x80028F74, "SLUS_006.10");
    car = 0x800984A0 + slot * 864;
    boundary = r_u32(0x80088508);
    node = r_s16(car + 0x70);
    result = (sint32)(boundary + 1) < node;
    if (result != 0)
        return result;
    for (index = 0; index < 8; ++index)
    {
        selected = r_s8(list + index);
        if (selected < 0)
            return result;
        node = r_s16(car + index * 14);
        boundary = r_u32(0x80088508);
        if ((sint32)boundary >= node)
        {
            base = r_u32(0x800884C4);
            segment = base + boundary * 24;
            count = r_u8(segment + 3);
            vertex = r_u16(segment + 4);
            base = r_u32(0x8008850C);
            first = base + vertex * 6;
            second = base + (vertex + (count & 15)) * 6;
            second_z = r_s16(second + 4);
            point = car + (uint32)selected * 12 + 0x80;
            distance = td5_car_edge_distance(point, first, second, segment, second_z);
            if (distance < 0)
            {
                angle = td5_car_edge_angle(car, first, second);
                td5_800287b8(car, point, angle, (sint32)((uint32)distance + 4095) >> 12);
                td5_call_car_collision_callback(callback, car, original_stack - 0x48);
            }
        }
        result = index + 1 < 8;
    }
    return result;
}

uint32 td5_80028bd0(uint32 slot, uint32 callback, uint32 list, uint32 original_stack)
{
    uint32 car;
    uint32 index;
    uint32 contact;
    uint32 segment;
    uint32 count;
    uint32 lane;
    uint32 vertex;
    uint32 other_vertex;
    uint32 base;
    uint32 first;
    uint32 second;
    uint32 point;
    uint32 type;
    uint32 offset;
    uint32 other_offset;
    uint32 side;
    sint32 selected;
    sint32 node;
    sint32 second_z;
    sint32 distance;
    sint32 angle;

    FUNCTION_MARKER(0x80028BD0, "SLUS_006.10");
    car = 0x800984A0 + slot * 864;
    w_u8(car + 0x353, 0);
    for (index = 0; index < 8; ++index)
    {
        selected = r_s8(list + index);
        if (selected < 0)
            return list + index;
        contact = car + (uint32)selected * 14;
        node = r_s16(contact);
        base = r_u32(0x800884C4);
        segment = base + (uint32)node * 24;
        count = r_u8(segment + 3);
        lane = r_u8(contact + 12);
        count &= 15;
        point = car + (uint32)selected * 12 + 0x80;
        if (lane == 0)
        {
            other_vertex = r_u16(segment + 6);
            base = r_u32(0x8008850C);
            vertex = r_u16(segment + 4);
            first = base + other_vertex * 6;
            second = base + vertex * 6;
            second_z = r_s16(second + 4);
            side = 1;
        }
        else
        {
            if ((sint32)lane < (sint32)(count - 1))
                continue;
            type = r_u8(segment);
            vertex = r_u16(segment + 4);
            offset = r_u32(0x8007E03C + type * 8);
            base = r_u32(0x8008850C);
            other_offset = r_u32(0x8007E040 + type * 8);
            first = base + (vertex + count + offset) * 6;
            other_vertex = r_u16(segment + 6);
            /* The first vertex Z precedes the second vertex address calculation */
            distance = r_s16(first + 4);
            second = base + (other_vertex + count + other_offset) * 6;
            second_z = r_s16(second + 4);
            side = 2;
        }
        if (side == 1)
            distance = td5_car_edge_distance(point, first, second, segment, second_z);
        else
        {
            /* Reuse the already loaded first vertex Z without an extra guest read */
            VECTOR normal;
            uint32 coordinate;
            uint32 origin;
            uint32 product;
            sint32 first_x;
            sint32 second_x;

            normal.vy = 0;
            normal.vx = second_z - distance;
            first_x = r_s16(first);
            second_x = r_s16(second);
            normal.vz = first_x - second_x;
            VectorNormal(&normal, &normal);
            coordinate = (uint32)(r_s32(point) >> 8);
            origin = (uint32)r_s16(first);
            coordinate -= origin;
            coordinate -= r_u32(segment + 12);
            product = coordinate * (uint32)normal.vx;
            coordinate = (uint32)(r_s32(point + 8) >> 8);
            origin = (uint32)r_s16(first + 4);
            coordinate -= origin;
            coordinate -= r_u32(segment + 20);
            distance = (sint32)(product + coordinate * (uint32)normal.vz);
        }
        if (distance < 0)
        {
            angle = td5_car_edge_angle(car, first, second);
            td5_8003c69c(point, car);
            td5_800287b8(car, point, angle, (sint32)((uint32)distance + 4095) >> 12);
            td5_call_car_collision_callback(callback, car, original_stack - 0x50);
            w_u8(car + 0x353, side);
        }
    }
    return 0;
}

uint32 td5_80077970(uint32 destination, uint32 source)
{
    uint32 index;
    uint32 value;

    FUNCTION_MARKER(0x80077970, "SLUS_006.10");
    for (index = 0; index < 9; ++index)
    {
        value = (uint32)r_s16(source + index * 2) << 18;
        w_u32(destination + index * 4, value);
    }
    return value;
}

uint32 td5_80023e9c(uint32 car, uint32 original_stack)
{
    uint32 pitch_bits;
    uint32 roll_bits;
    uint32 rate;
    uint32 velocity;
    uint32 coordinate;
    uint32 matrix;
    uint32 transpose;
    uint32 angles;
    uint32 values[9];
    uint32 index;
    sint32 pitch;
    sint32 roll;
    sint32 step;

    FUNCTION_MARKER(0x80023E9C, "SLUS_006.10");
    pitch = r_s16(car + 0x1D0);
    roll = r_s16(car + 0x1D4);
    pitch_bits = ((uint32)pitch + 2048) & 0xFFF;
    pitch = (sint32)pitch_bits - 2048;
    rate = r_u32(0x80088700);
    roll_bits = ((uint32)roll + 2048) & 0xFFF;
    roll = (sint32)roll_bits - 2048;
    if (rate != 0)
    {
        if (pitch < -639)
            w_u32(car + 0x188, r_u32(car + 0x188) + 512);
        if (pitch >= 640)
            w_u32(car + 0x188, r_u32(car + 0x188) - 512);
        if (roll < -699)
            w_u32(car + 0x190, r_u32(car + 0x190) + 512);
        if (roll >= 700)
            w_u32(car + 0x190, r_u32(car + 0x190) - 512);
        if (pitch < -853)
        {
            w_u32(car + 0x188, 0);
            w_u16(car + 0x1D0, (uint32)-853);
            w_u32(car + 0x1B8, 0xFFFCAB00);
        }
        if (pitch >= 854)
        {
            w_u32(car + 0x188, 0);
            w_u16(car + 0x1D0, 853);
            w_u32(car + 0x1B8, 0x35500);
        }
        if (roll < -932)
        {
            w_u32(car + 0x190, 0);
            w_u16(car + 0x1D4, (uint32)-932);
            w_u32(car + 0x1C0, 0xFFFC5C00);
        }
        if (roll < 933)
            return 1;
        w_u32(car + 0x190, 0);
        w_u16(car + 0x1D4, 932);
        w_u32(car + 0x1C0, 0x3A400);
        return 932;
    }
    if (pitch_bits - 1195 < 0x6AB && roll_bits - 1116 < 0x749)
        return 1;
    matrix = original_stack - 0x58;
    transpose = original_stack - 0x38;
    angles = original_stack - 0x18;
    velocity = r_u32(car + 0x188);
    rate = r_u32(0x80088100);
    step = (sint32)(velocity * rate) / 256;
    coordinate = r_u32(car + 0x1B8) - (uint32)step;
    w_u16(angles, (uint32)((sint32)coordinate >> 8));
    velocity = r_u32(car + 0x18C);
    step = (sint32)(velocity * rate) / 256;
    coordinate = r_u32(car + 0x1BC) - (uint32)step;
    w_u16(angles + 2, (uint32)((sint32)coordinate >> 8));
    velocity = r_u32(car + 0x190);
    step = (sint32)(velocity * rate) / 256;
    coordinate = r_u32(car + 0x1C0) - (uint32)step;
    w_u16(angles + 4, (uint32)((sint32)coordinate >> 8));
    td5_80079548(matrix, angles);
    values[0] = r_u16(matrix);
    values[1] = r_u16(matrix + 6);
    values[2] = r_u16(matrix + 12);
    values[3] = r_u16(matrix + 2);
    values[4] = r_u16(matrix + 8);
    values[5] = r_u16(matrix + 14);
    values[6] = r_u16(matrix + 4);
    values[7] = r_u16(matrix + 10);
    values[8] = r_u16(matrix + 16);
    for (index = 0; index < 9; ++index)
        w_u16(transpose + index * 2, values[index]);
    MulMatrix2((MATRIX *)psx_addr(car + 0x110, 20), (MATRIX *)psx_addr(transpose, 20));
    td5_80077970(car + 0x154, transpose);
    td5_80077970(car + 0x130, car + 0x110);
    w_u8(car + 0x351, 1);
    w_u16(car + 0x310, 0);
    return 1;
}

uint32 td5_8002527c(uint32 car)
{
    uint32 descriptor;
    uint32 braking;
    uint32 result;
    sint32 maximum;
    sint32 throttle;
    sint32 target;
    sint32 scaled;
    sint32 current;
    sint32 step;

    FUNCTION_MARKER(0x8002527C, "SLUS_006.10");
    descriptor = r_u32(car + 0x184);
    braking = r_u8(car + 0x345);
    maximum = r_s16(descriptor + 0x72);
    target = 400;
    if (braking == 0)
    {
        throttle = r_s16(car + 0x316);
        if (throttle >= 0)
        {
            scaled = (sint32)((uint32)(maximum - 400) * (uint32)throttle) / 256;
            target = (scaled < maximum ? scaled : maximum) + 400;
        }
    }
    current = r_s32(car + 0x2E8);
    step = (sint32)((uint32)target - (uint32)current) / 16;
    if (current < target)
    {
        if (step >= 401)
            step = 400;
    }
    else if (step >= 201)
        step = 200;
    result = (uint32)current + (uint32)step;
    w_u32(car + 0x2E8, result);
    return result;
}

uint32 td5_80047f48(uint32 car)
{
    uint32 result;
    uint32 state;
    uint32 lap;
    uint32 timer;
    uint32 destination;
    uint32 index;
    uint32 first;
    uint32 second;
    uint32 descriptor;
    uint32 node;
    uint32 bonus;
    uint32 incremented;
    uint32 slot;
    uint32 old_mask;
    sint32 speed;
    sint32 divisor;
    sint32 quotient;
    sint32 correction;

    FUNCTION_MARKER(0x80047F48, "SLUS_006.10");
    result = r_u32(0x80087650);
    if (result != 0)
        return result;
    result = r_u32(0x80087738);
    if (result != 0)
        return result;
    if (r_u32(0x80087730) != 0 || r_u32(0x8008772C) != 0)
        if (r_u32(0x80087734) == 0)
            return td5_80047bf0(car);
    result = r_u32(0x800875E8);
    if (result != 0)
    {
        result = r_u32(0x8008772C);
        if (result == 0)
        {
            result = r_u32(0x80087734);
            if (result == 0)
                return result;
        }
    }
    lap = r_u8(car + 0x356);
    timer = r_u16(car + 0x324);
    w_u16(car + 0x326 + lap * 2, timer);
    lap = r_u8(car + 0x356);
    index = 0;
    while (index < lap)
    {
        destination = car + lap * 2 + 0x326;
        first = r_u16(destination);
        second = r_u16(car + 0x326 + index * 2);
        w_u16(destination, first - second);
        lap = r_u8(car + 0x356);
        ++index;
    }
    descriptor = r_u32(0x80088770);
    lap = r_u8(car + 0x356);
    descriptor = r_u32(descriptor + 0x2C);
    node = (uint32)r_s16(car + 0x72);
    descriptor += lap * 4;
    result = r_u16(descriptor + 4);
    if (result != node)
        return result;
    lap = r_u8(car + 0x356);
    bonus = r_u16(descriptor + 6);
    timer = r_u16(car + 0x31C);
    incremented = lap + 1;
    w_u8(car + 0x356, incremented);
    descriptor = r_u32(0x80088770);
    w_u16(car + 0x31C, timer + bonus);
    descriptor = r_u32(descriptor + 0x2C);
    result = r_u8(descriptor);
    if ((incremented & 255) == result)
    {
        if (r_u32(car + 0x300) == 0)
        {
            speed = r_s32(car + 0x2EC);
            timer = r_u16(car + 0x324);
            w_u32(car + 0x300, timer);
            speed /= 256;
            divisor = speed > 0 ? speed : 1;
            slot = r_u8(car + 0x34D);
            w_u16(car + 0x30C, (uint32)divisor);
            first = r_u32(0x8009782C + slot * 284) * 1500;
            quotient = td5_divide_checked((sint32)first, (sint16)divisor);
            state = r_u32(0x80087734);
            w_u16(car + 0x30E, (uint32)quotient);
            if (state != 0)
            {
                timer = r_u32(car + 0x300);
                correction = (sint16)quotient * 1000 / 256;
                w_u32(car + 0x300, timer * 1000 - (uint32)correction);
            }
        }
        slot = r_u8(car + 0x34D);
        result = r_u32(0x800885F8);
        if (slot == result)
        {
            if (r_u32(0x80087738) == 0)
            {
                w_u32(0x80088610 + slot * 4, 0);
                td5_8001e034(0x80098670 + slot * 864, 0, slot);
            }
            old_mask = r_u32(0x800875E8);
            w_u32(0x800884D0 + slot * 4, 1);
            w_u32(0x800883E8 + slot * 4, 0);
            w_u32(0x800875E8, old_mask | ((uint32)1 << (slot & 31)));
        }
        slot = r_u8(car + 0x34D);
        for (index = 0; index < 6; ++index)
        {
            result = r_u8(0x800883F0 + index);
            if (result == slot)
                return result;
        }
        return 0;
    }
    slot = r_u8(car + 0x34D);
    if (slot == r_u32(0x800885F8))
    {
        w_u16(0x800878C8, 60);
        slot = r_u8(car + 0x34D);
    }
    result = r_u8(0x800883F0);
    if (slot == result)
    {
        result = r_u32(0x80087728);
        if (result == 0)
        {
            result = r_u32(car + 0x2A0) + 1;
            w_u32(car + 0x2A0, result);
        }
    }
    return result;
}

uint32 td5_800415a4(sint32 slot, uint32 amount)
{
    uint32 offset;

    FUNCTION_MARKER(0x800415A4, "SLUS_006.10");
    offset = (uint32)slot << 2;
    if (slot < 2)
    {
        w_u32(0x800885B0 + offset, amount);
        w_u32(0x80088480 + offset, amount);
    }
    return offset;
}

void td5_800484a4(void)
{
    FUNCTION_MARKER(0x800484A4, "SLUS_006.10");
}

uint32 td5_80047970(uint32 car)
{
    uint32 slot;
    uint32 result;
    uint32 state;
    uint32 packed;
    uint32 old_mask;
    sint32 low;
    sint32 high;
    sint32 speed;
    sint32 divisor;
    sint32 numerator;

    FUNCTION_MARKER(0x80047970, "SLUS_006.10");
    slot = r_u8(car + 0x34D);
    result = r_u32(0x80088498);
    if (slot != result)
        return result;
    result = r_u32(0x80088618);
    if (result != 0)
        return result;
    result = r_u32(0x80087650);
    if (result != 0)
        return result;
    result = r_u32(0x80087738);
    if (result != 0)
        return result;
    packed = r_u16(car + 0x31C);
    state = r_u32(car + 0x300);
    low = packed & 255;
    high = packed >> 8;
    w_u32(0x8008869C, packed);
    if (state != 0)
        return packed;
    if (r_u32(0x8008763C) != 0)
    {
        low -= 2;
        if (low < 0)
        {
            state = r_u32(0x800880FC);
            --high;
            low = (sint32)((uint32)(low - 1) + state);
            if (high < 0)
            {
                speed = r_s32(car + 0x2EC);
                state = r_u16(car + 0x324);
                low = 0;
                high = 0;
                w_u32(car + 0x300, state);
                speed /= 256;
                divisor = speed > 0 ? speed : 1;
                slot = r_u8(car + 0x34D);
                w_u16(car + 0x30C, (uint32)divisor);
                numerator = (sint32)(r_u32(0x8009782C + slot * 284) * 1500);
                result = (uint32)td5_divide_checked(numerator, (sint16)divisor);
                slot = r_u8(car + 0x34D);
                state = r_u32(0x800885F8);
                w_u16(car + 0x30E, result);
                if (slot == state)
                {
                    state = r_u32(0x80087738);
                    w_u32(0x80088530, 2);
                    if (state == 0)
                    {
                        w_u32(0x80088610 + slot * 4, 0);
                        td5_8001e034(0x80098670 + slot * 864, 0, slot);
                    }
                    old_mask = r_u32(0x800875E8);
                    w_u32(0x800884D0 + slot * 4, 1);
                    w_u32(0x800883E8 + slot * 4, 0);
                    w_u32(0x800875E8, old_mask | ((uint32)1 << (slot & 31)));
                }
            }
        }
    }
    result = ((uint32)high & 255) << 8;
    packed = ((uint32)low & 255) | result;
    w_u16(car + 0x31C, packed);
    w_u32(0x8008869C, packed);
    if (packed < 0xA00)
    {
        state = (uint32)r_s16(0x800878C8);
        result = 2;
        if ((sint32)state < 2)
            w_u16(0x800878C8, 2);
    }
    return result;
}

uint32 td5_80024c14(uint32 slot, uint32 height_address, uint32 physics_force_carrier)
{
    uint32 car;
    uint32 value;
    uint32 descriptor;
    uint32 limit;
    uint32 current;
    uint32 count;
    uint32 index;
    uint32 state;
    uint32 sum;
    uint32 timer;
    uint32 magnitude;
    uint32 callback;
    uint32 list;
    uint32 heading[3];
    sint32 threshold;
    sint32 speed;
    sint32 peak;
    sint32 average;
    sint32 rpm;

    FUNCTION_MARKER(0x80024C14, "SLUS_006.10");
    car = 0x800984A0 + slot * 864;
    timer = r_u16(car + 0x310);
    w_u8(car + 0x353, 0);
    state = r_u32(0x80087734);
    w_u16(car + 0x310, timer + 1);
    if (state != 0)
    {
        descriptor = r_u32(0x80088770);
        descriptor = r_u32(descriptor + 0x2C);
        current = r_u8(car + 0x356);
        limit = r_u8(descriptor);
        if (current >= limit)
        {
            w_u16(car + 0x316, 0xFF00);
            w_u32(car + 0x2E4, 0);
            w_u8(car + 0x345, 1);
        }
    }
    descriptor = r_u32(car + 0x184);
    threshold = -r_s16(descriptor + 100);
    count = 0;
    for (index = 0; index < 4; ++index)
        if (r_s32(car + 0x2B4 + index * 4) <= threshold)
            ++count;
    if (count != 0)
    {
        td5_play_positioned_sound(4, -1, 4096, 4096, car + 0x1C4, 0);
        td5_800415a4(r_u8(car + 0x34D), 2048);
    }
    if (r_u32(car + 0x300) == 0)
    {
        value = r_u32(car + 0x2EC);
        magnitude = (sint32)value < 0 ? 0 - value : value;
        sum = r_u32(car + 0x304);
        peak = r_s16(car + 0x308);
        speed = (sint32)magnitude / 256;
        w_u32(car + 0x304, sum + (uint32)speed);
        if (speed < peak)
            speed = peak;
        w_u16(car + 0x308, (uint32)speed);
    }
    if (r_u32(0x80087728) == 4)
        td5_80049258(car);
    if (r_u32(0x80088618) == 0 && r_u32(car + 0x300) == 0)
    {
        timer = r_u16(car + 0x324);
        if (timer != 0xFFFF)
            w_u16(car + 0x324, timer + 1);
        timer = r_u16(car + 0x324);
        sum = r_u32(car + 0x304);
        average = td5_divide_checked((sint32)sum, (sint32)timer);
        value = r_u32(car + 0x2EC);
        peak = r_s16(car + 0x308);
        magnitude = (sint32)value < 0 ? 0 - value : value;
        w_u16(car + 0x30A, (uint32)average);
        speed = (sint32)magnitude / 256;
        if (speed < peak)
            speed = peak;
        w_u16(car + 0x308, (uint32)speed);
    }
    if (r_u32(0x80087650) == 0)
    {
        td5_80047970(car);
        td5_80047f48(car);
    }
    td5_800484a4();
    if (r_u8(car + 0x351) == 0)
        td5_80023e9c(car, height_address + 0xA8);
    state = r_u8(car + 0x351);
    if (state == 0)
    {
        if (r_u32(0x8008873C) == 0)
        {
            current = r_u8(car + 0x358);
            limit = r_u8(car + 0x35B);
            if (limit < current)
                current = limit;
            threshold = r_s16(car + 0x338);
            w_u8(car + 0x358, current);
            if (threshold >= 3 && r_u8(car + 0x354) == 15)
                td5_80020a6c(car);
            else if (r_s32(0x8008772C) < (sint32)slot)
                td5_80022b1c(car);
            else
                td5_80020e9c(car, height_address + 0xA8, physics_force_carrier);
        }
        else
        {
            value = r_u8(car + 0x35B);
            w_u8(car + 0x359, value);
            td5_8002527c(car);
            current = r_u8(car + 0x34D);
            threshold = r_s32(0x8008772C);
            if (threshold < (sint32)current || r_u32(0x80087650) != 0)
            {
                descriptor = r_u32(car + 0x184);
                rpm = r_s16(descriptor + 0x72) * 2 / 3;
                state = r_u32(0x80087650);
                w_u32(car + 0x2E8, (uint32)rpm);
                if (state != 0 && r_u8(car + 0x34D) == 0)
                {
                    descriptor = r_u32(car + 0x184);
                    w_u32(car + 0x2E8, (uint32)(r_s16(descriptor + 0x72) - 1));
                }
            }
            descriptor = r_u32(car + 0x184);
            w_u8(car + 0x34E, 0);
            rpm = r_s16(descriptor + 0x72) * 3 / 4;
            if (rpm < r_s32(car + 0x2E8))
                w_u8(car + 0x34E, r_u16(descriptor + 0x76));
        }
        td5_80024220(car, height_address);
        callback = 0x80024938;
        list = 0x800877B0;
    }
    else if (state == 1)
    {
        td5_8002d6c4(car, height_address + 0xA8);
        heading[0] = (uint32)r_s16(car + 0x1D0) << 8;
        heading[1] = (uint32)r_s16(car + 0x1D2) << 8;
        heading[2] = (uint32)r_s16(car + 0x1D4) << 8;
        w_u32(car + 0x1B8, heading[0]);
        w_u32(car + 0x1BC, heading[1]);
        w_u32(car + 0x1C0, heading[2]);
        td5_8002527c(car);
        td5_8002d824(car, height_address + 0xA8);
        callback = 0x8002D79C;
        list = 0x800877B8;
    }
    else
        return 1;
    td5_80029180(slot, callback, list, height_address + 0xA8);
    td5_80028f74(slot, callback, list, height_address + 0xA8);
    return td5_80028bd0(slot, callback, list, height_address + 0xA8);
}

uint32 td5_80052f58(uint32 position, uint32 velocity)
{
    FUNCTION_MARKER(0x80052F58, "SLUS_006.10");
    return (uint32)td5_play_positioned_sound(12, 12, 4096, 4096, position, velocity);
}

uint32 td5_80052f94(void)
{
    sint32 value;

    FUNCTION_MARKER(0x80052F94, "SLUS_006.10");
    value = randPSX();
    return (uint32)td5_play_sound(12, 12, value % 1024 + 1024, 0, 4096);
}

uint32 td5_80052fec(uint32 position, uint32 velocity)
{
    FUNCTION_MARKER(0x80052FEC, "SLUS_006.10");
    return (uint32)td5_play_positioned_sound(9, 14, 4096, 4096, position, velocity);
}

uint32 td5_800439b0(void)
{
    uint32 value;

    FUNCTION_MARKER(0x800439B0, "SLUS_006.10");
    value = r_u16(0x800885C8) + 1024;
    w_u16(0x800885C8, value);
    return value;
}

static sint32 td5_control_drag(uint32 car, sint32 speed)
{
    uint32 factor;
    sint32 first;
    sint32 second;

    factor = r_u32(car + 0x2FC);
    first = (sint32)((uint32)speed * factor) / 2048;
    second = (sint32)(1500 * factor) / 256;
    if (first < second)
        second = first;
    return (sint32)((uint32)second * r_u32(0x80088100)) / 256;
}

uint32 td5_800269dc(uint32 slot, uint32 controller)
{
    uint32 car;
    uint32 controls;
    uint32 replay_mode;
    uint32 state;
    uint32 descriptor;
    uint32 gear;
    uint32 maximum_gear;
    uint32 mask;
    uint32 result;
    sint32 speed;
    sint32 yaw;
    sint32 direction;
    sint32 divisor;
    sint32 steering_rate;
    sint32 steering_limit;
    sint32 angle;
    sint32 heading;
    sint32 scale;
    sint32 left_limit;
    sint32 right_limit;
    sint32 steering;
    sint32 ramp;
    sint32 amount;
    sint32 drag;
    sint32 throttle;
    sint32 value;

    FUNCTION_MARKER(0x800269DC, "SLUS_006.10");
    controls = r_u16(0x800885DC + controller * 2);
    replay_mode = r_u32(0x80088418);
    car = 0x800984A0 + slot * 864;
    (void)r_u8(car + 0x34D);
    result = (uint32)td5_8004adf8((uint32)(sint32)(sint16)controls);
    if (replay_mode != 0)
        controls = result;
    if (slot == 1)
    {
        (void)r_u8(car + 0x34D);
        controls = (uint32)td5_8004adf8((uint32)(sint32)(sint16)controls);
    }
    speed = r_s32(car + 0x2EC);
    yaw = r_s32(car + 0x2F4);
    state = r_u8(car + 0x34E);
    speed >>= 8;
    if (speed < 0)
        speed = -speed;
    if (state == 0)
    {
        yaw >>= 8;
        divisor = (sint32)((uint32)yaw * (uint32)yaw + 64);
        divisor = (sint32)((uint32)td5_divide_checked((sint32)((uint32)speed << 6), divisor) + 64);
    }
    else
        divisor = 64;
    steering_rate = td5_divide_checked(0xC0000, divisor);
    direction = r_s32(car + 0x194);
    heading = r_s32(car + 0x19C);
    state = r_u8(car + 0x34E);
    divisor = state == 0 ? speed + 128 : 128;
    steering_limit = td5_divide_checked(0xC00000, divisor);
    angle = td5_8007b0c0(direction >> 8, heading >> 8);
    heading = r_s32(car + 0x1BC);
    scale = r_s32(car + 0x2F8);
    angle = (sint32)(((uint32)angle - (uint32)(heading >> 8) + 2048) & 0xFFF) - 2048;
    if (scale < 2048 && (r_u32(car + 0x344) & 0xFFFF00) == 0)
        angle = (sint32)((uint32)angle * (uint32)scale) / 2048;
    left_limit = (sint32)((uint32)angle << 8);
    if (left_limit < steering_limit)
        left_limit = steering_limit;
    if (left_limit > 0x18000)
        left_limit = 0x18000;
    right_limit = (sint32)((uint32)angle << 8);
    value = (sint32)(0 - (uint32)steering_limit);
    if (value < right_limit)
        right_limit = value;
    if (right_limit < -0x18000)
        right_limit = -0x18000;
    steering_rate = (sint32)((uint32)steering_rate * r_u32(0x80088100)) / 256;
    if ((controls & 0x8000) != 0)
    {
        throttle = (sint32)((controls & 0xF8) >> 3) - 16;
        if (throttle > 0)
            steering = (sint32)((uint32)throttle * (uint32)left_limit) / 15;
        else
            steering = (sint32)((uint32)(-throttle) * (uint32)right_limit) / 15;
        w_u32(car + 0x2E4, (uint32)steering);
        value = r_s32(car + 0x2F4);
        if (value > 0)
        {
            drag = td5_control_drag(car, value);
            if (throttle < 16)
            {
                value = r_s32(car + 0x18C);
                if (value > 0)
                {
                    amount = (sint32)((uint32)drag * (uint32)throttle) / 15;
                    w_u32(car + 0x18C, (uint32)value + (uint32)amount);
                }
            }
            if (throttle >= 17)
            {
                value = r_s32(car + 0x18C);
                if (value < 0)
                {
                    amount = (sint32)((uint32)drag * (uint32)throttle) / 15;
                    w_u32(car + 0x18C, (uint32)value + (uint32)amount);
                }
            }
        }
        if ((controls & 0x1000) != 0)
            goto handbrake;
        w_u8(car + 0x346, 0);
        if ((controls & 0x100) != 0)
            goto brake;
        if (r_u32(0x800977D0) != 0)
            td5_8004e46c();
        else
        {
            w_u8(car + 0x347, 1);
            w_u8(car + 0x345, 0);
            if (r_u32(0x800977D0) != 0)
                w_u16(car + 0x316, r_u16(0x80087824 + controller * 2));
            else
            {
                value = r_s16(0x80087824 + controller * 2);
                amount = (sint32)((controls & 0xE00) >> 9) * value / 7;
                w_u16(car + 0x316, (uint32)amount);
            }
        }
        goto gears;
    }
    if ((controls & 0x10) != 0)
    {
        ramp = r_s16(car + 0x312);
        if (ramp < 256)
            w_u16(car + 0x312, (uint32)(ramp + 64));
        steering = r_s32(car + 0x2E4);
        amount = (sint32)((uint32)steering_rate << 1);
        if (steering <= 0)
            amount = (sint32)((uint32)steering_rate * (uint32)r_s16(car + 0x312)) / 256;
        w_u32(car + 0x2E4, (uint32)steering - (uint32)amount);
        if (r_s32(car + 0x2E4) < right_limit)
            w_u32(car + 0x2E4, (uint32)right_limit);
    }
    if ((controls & 8) != 0)
    {
        ramp = r_s16(car + 0x312);
        if (ramp < 256)
            w_u16(car + 0x312, (uint32)(ramp + 64));
        steering = r_s32(car + 0x2E4);
        amount = (sint32)((uint32)steering_rate << 1);
        if (steering >= 0)
            amount = (sint32)((uint32)steering_rate * (uint32)r_s16(car + 0x312)) / 256;
        w_u32(car + 0x2E4, (uint32)steering + (uint32)amount);
        if (left_limit < r_s32(car + 0x2E4))
            w_u32(car + 0x2E4, (uint32)left_limit);
    }
    if ((controls & 0x18) == 0)
    {
        ramp = r_s16(car + 0x312);
        if (ramp > 0)
            w_u16(car + 0x312, (uint32)(ramp - 64));
        steering = r_s32(car + 0x2E4);
        amount = (sint32)((uint32)steering_rate << 2);
        if ((sint32)(0 - (uint32)amount) < steering)
        {
            if (steering < amount)
                w_u32(car + 0x2E4, 0);
            else
                w_u32(car + 0x2E4, (uint32)steering - (uint32)amount);
        }
        else
            w_u32(car + 0x2E4, (uint32)steering + (uint32)amount);
    }
    value = r_s32(car + 0x2F8);
    if (value > 0)
    {
        drag = td5_control_drag(car, value);
        if ((controls & 0x10) != 0)
        {
            value = r_s32(car + 0x18C);
            if (value > 0)
                w_u32(car + 0x18C, (uint32)value - (uint32)drag);
        }
        if ((controls & 8) != 0)
        {
            value = r_s32(car + 0x18C);
            if (value < 0)
                w_u32(car + 0x18C, (uint32)value + (uint32)drag);
        }
    }
    if ((controls & 0x1000) != 0)
    {
handbrake:
        w_u16(car + 0x316, 0xFF00);
        w_u8(car + 0x346, 1);
        w_u8(car + 0x345, 1);
        goto gears;
    }
    w_u8(car + 0x346, 0);
    if ((controls & 0x40) != 0)
    {
brake:
        if (r_u8(car + 0x350) != 0 && r_u8(car + 0x347) == 1 && speed < 10 && r_u8(car + 0x34E) == 0)
            w_u8(car + 0x347, 0);
        state = r_u8(car + 0x347);
        w_u16(car + 0x316, 0xFF00);
        w_u8(car + 0x345, state);
    }
    else if (r_u32(0x800977D0) != 0)
        td5_8004e46c();
    else
    {
        w_u8(car + 0x347, 1);
        w_u8(car + 0x345, 0);
        if ((controls & 0x20) != 0)
            w_u16(car + 0x316, r_u16(0x80087824 + controller * 2));
        else
            w_u16(car + 0x316, 0);
    }
gears:
    if (r_u8(car + 0x350) == 0)
    {
        mask = controls & 0x6000;
        if (mask == 0x2000)
        {
            maximum_gear = r_u8(car + 0x344);
            gear = r_u8(car + 0x343);
            if ((sint32)gear < (sint32)maximum_gear - 1)
            {
                descriptor = r_u32(car + 0x184);
                w_u8(car + 0x343, gear + 1);
                td5_800254c0(car, descriptor);
                if (r_u8(car + 0x343) == 2)
                {
                    descriptor = r_u32(car + 0x184);
                    value = r_s16(descriptor + 0x72) * 3 / 4;
                    if (value < r_s32(car + 0x2E8))
                        w_u8(car + 0x34E, r_u16(descriptor + 0x76));
                }
            }
        }
        if ((controls & 0x6000) == 0x4000)
        {
            gear = r_u8(car + 0x343);
            if (gear != 0)
            {
                state = r_u8(car + 0x351);
                if (state == 0 && gear >= 3)
                {
                    descriptor = r_u32(car + 0x184);
                    value = r_s32(car + 0x2EC);
                    amount = r_s16(descriptor + gear * 2 + 0x2C);
                    value >>= 8;
                    if (value < 0)
                        value = -value;
                    value = (sint32)((uint32)value * (uint32)amount * 45) / 4096;
                    amount = r_s16(descriptor + 0x72);
                    if ((sint32)((uint32)value + 400) < amount)
                    {
                        w_u8(car + 0x343, gear - 1);
                        goto final_action;
                    }
                }
                else
                {
                    gear = r_u8(car + 0x343);
                    if (gear != 1)
                    {
                        w_u8(car + 0x343, gear - 1);
                        goto final_action;
                    }
                    descriptor = r_u32(car + 0x184);
                    value = (sint16)r_u16(descriptor + 0x72);
                    amount = r_s32(car + 0x2E8);
                    if (amount < value / 2)
                    {
                        w_u8(car + 0x343, gear - 1);
                        goto final_action;
                    }
                }
                td5_80052fec(car + 0x1C4, car + 0x194);
            }
        }
    }
final_action:
    if (r_u8(0x8007EA01) != 0)
    {
        if (r_u8(car + 0x345) != 0)
            return 0x6000;
        result = controller * 2;
        if ((controls & 0x6000) == 0x6000)
        {
            result = (uint32)r_s16(0x80087824 + controller * 2) << 1;
            w_u16(car + 0x316, result);
        }
        return result;
    }
    if ((controls & 0x6000) != 0x6000)
        return 0x6000;
    if (r_u32(0x80087724) != 0)
    {
        state = r_u8(car + 0x34D);
        if (state == r_u32(0x800879B8))
            return td5_800439b0();
    }
    if (r_u32(0x80087738) != 0 || r_u32(0x80087650) != 0)
        return td5_80052f58(car + 0x1C4, car + 0x194);
    return td5_80052f94();
}
