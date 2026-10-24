#include "td5_functions.h"

uint32 td5_8002c30c(uint32 original_stack)
{
    uint32 count;
    uint32 index;
    uint32 car;
    uint32 entry;
    uint32 model;
    uint32 coordinate;
    uint32 radius;
    uint32 bucket;
    uint32 next;
    uint32 car_offset;
    uint32 group;
    uint32 candidate;
    uint32 steps;
    uint32 candidate_offset;
    uint32 special;

    FUNCTION_MARKER(0x8002C30C, "SLUS_006.10");
    count = r_u32(0x800875E4);
    index = 0;
    car = 0x800984A0;
    entry = 0x800A4060;
    while ((sint32)index < (sint32)count)
    {
        model = r_u32(car + 0x180);
        coordinate = r_u32(car + 0x1C4);
        radius = (uint32)r_s16(model + 0x80);
        w_u32(entry, (uint32)((sint32)coordinate >> 8) - radius);
        coordinate = r_u32(car + 0x1CC);
        w_u32(entry + 4, (uint32)((sint32)coordinate >> 8) - radius);
        coordinate = r_u32(car + 0x1C4);
        w_u32(entry + 8, (uint32)((sint32)coordinate >> 8) + radius);
        coordinate = r_u32(car + 0x1CC);
        w_u32(entry + 12, (uint32)((sint32)coordinate >> 8) + radius);
        bucket = (uint32)(r_s16(car + 0x70) >> 2) + 1;
        next = r_u8(0x80092230 + bucket);
        w_u8(entry + 16, next);
        w_u8(0x80092230 + bucket, index);
        count = r_u32(0x800875E4);
        car += 864;
        ++index;
        entry += 20;
    }
    count = r_u32(0x800875E4);
    index = 0;
    car_offset = 0;
    if ((sint32)count > 0)
    {
        do
        {
            bucket = (uint32)(r_s16(0x80098510 + car_offset) >> 2);
            for (group = 0; group < 3; ++group)
            {
                candidate = r_u8(0x80092230 + bucket);
                steps = 0;
                while (candidate != 255)
                {
                    if ((sint32)index < (sint32)candidate && (sint32)candidate < r_s32(0x800875E4))
                    {
                        special = r_u8(0x800987F1 + car_offset);
                        if (special == 0)
                            special = r_u8(0x800987F4 + car_offset) >= 15;
                        candidate_offset = candidate * 864;
                        if (special == 0)
                            special = r_u8(0x800987F1 + candidate_offset);
                        if (special == 0)
                            special = r_u8(0x800987F4 + candidate_offset) >= 15;
                        if (special != 0)
                            td5_8002c074(index, candidate);
                        else
                            td5_8002ba04(index, candidate, original_stack - 0x50);
                    }
                    candidate = r_u8(0x800A4070 + candidate * 20);
                    ++steps;
                    if (steps >= 17)
                    {
                        td5_debug_log(0x80010AEC);
                        break;
                    }
                }
                ++bucket;
            }
            count = r_u32(0x800875E4);
            ++index;
            car_offset += 864;
        } while ((sint32)index < (sint32)count);
        count = r_u32(0x800875E4);
    }
    index = 0;
    car_offset = 0;
    entry = 0x800A4070;
    if ((sint32)count <= 0)
        return count;
    do
    {
        bucket = (uint32)(r_s16(0x80098510 + car_offset) >> 2);
        w_u8(0x80092231 + bucket, 255);
        w_u8(entry, 255);
        count = r_u32(0x800875E4);
        car_offset += 864;
        ++index;
        entry += 20;
    } while ((sint32)index < (sint32)count);
    return 0;
}

uint32 td5_80052e94(uint32 slot)
{
    uint32 enabled;
    uint32 position;
    uint32 velocity;

    FUNCTION_MARKER(0x80052E94, "SLUS_006.10");
    enabled = r_u32(0x80088668);
    position = 0x80098664 + slot * 864;
    velocity = 0x80098634 + slot * 864;
    w_u32(0x80091F84, position);
    w_u32(0x80091F88, velocity);
    if (enabled != 0)
    {
        w_u32(0x80091FE0, position);
        w_u32(0x80091FE4, velocity);
    }
    return velocity;
}

uint32 td5_8004e5b0(uint32 height_address)
{
    uint32 active;
    uint32 selected;
    uint32 timer;
    uint32 value;
    uint32 first;
    uint32 expected;
    uint32 mode;
    sint32 lane;
    sint32 target_lane;

    FUNCTION_MARKER(0x8004E5B0, "SLUS_006.10");
    td5_8004d918();
    active = r_u32(0x800981D4);
    selected = r_u32(0x800981C8);
    if (active == 0 && selected == 0xFFFFFFFF)
    {
        timer = r_u32(0x8009823C);
        if (timer != 0)
            return 0xFFFFFFFF;
        lane = r_s16(0x80098512);
        target_lane = r_s16(0x8009A372);
        if (target_lane != lane - 2)
            return 0x10000;
        if (r_s32(0x8009878C) <= 87608)
            return 0;
        value = r_u32(0x800977B0);
        if ((sint32)value <= 0)
            return value;
        first = r_u32(0x8009775C);
        expected = r_u32(0x80098158);
        if (first != expected)
            return expected;
        w_u32(0x800981C8, 0);
        td5_8004d660(9);
        value = r_u32(0x80097750);
        w_u32(0x8009814C, value);
        td5_80024134(0x8009A300, height_address);
        mode = r_u32(0x80087724);
        if (mode != 0)
            return mode;
        td5_80052e94(9);
        return td5_set_engine_parameter(4096);
    }
    if (selected == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    timer = r_u32(0x8009823C);
    if (timer != 0)
        return timer;
    selected = r_u32(0x800981C8);
    lane = r_s16(0x80098512 + selected * 864);
    target_lane = r_s16(0x8009A372);
    if (lane - target_lane >= 65 && r_u8(0x8009A651) == 0)
    {
        mode = r_u32(0x80087724);
        w_u32(0x800981C8, 0xFFFFFFFF);
        w_u32(0x80098248, 0);
        w_u32(0x800981D4, 0);
        w_u32(0x8009823C, 300);
        if (mode == 0)
            td5_set_engine_parameter(0);
        return td5_80024134(0x8009A300, height_address);
    }
    selected = r_u32(0x800981C8);
    target_lane = r_s16(0x8009A372);
    lane = r_s16(0x80098512 + selected * 864);
    if (target_lane - lane < 2)
        return selected * 9;
    first = r_u32(0x8009775C + selected * 284);
    expected = r_u32(0x80098158);
    if (first != expected)
        return expected;
    if (r_u8(0x8009A651) != 0)
        return 1;
    mode = r_u32(0x80087724);
    w_u32(0x800977D0 + selected * 284, 1);
    if (mode != 0)
        return 1;
    return td5_set_engine_parameter(0);
}

uint32 td5_8004d918(void)
{
    uint32 selected;
    uint32 previous;
    uint32 route;

    FUNCTION_MARKER(0x8004D918, "SLUS_006.10");
    selected = r_u32(0x800981C8);
    if (selected == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    previous = r_u32(0x8009814C);
    route = r_u32(0x80097750 + selected * 284);
    if (previous == route)
        return route;
    w_u32(0x8009814C, route);
    return td5_8004d660(9);
}

uint32 td5_80049310(uint32 car, uint32 mode)
{
    uint32 value;

    FUNCTION_MARKER(0x80049310, "SLUS_006.10");
    value = r_u32(car + 0x300);
    if (value != 0)
        return 4;
    if (r_u32(0x80087728) != 4)
        return 4;
    value = r_u32(car + 0x2A0);
    if ((sint32)value > 0)
        w_u32(car + 0x2A0, value - mode);
    value = r_u32(car + 0x2A0);
    if ((sint32)value < 0)
        w_u32(car + 0x2A0, 0);
    return value;
}

static uint32 td5_car_truncate(uint32 value, uint32 bits)
{
    if ((sint32)value < 0)
        value += (1u << bits) - 1;
    return (uint32)((sint32)value >> bits);
}

static uint32 td5_car_boundary_response(uint32 car, uint32 other, uint32 anchor, uint32 penetration)
{
    sint32 other_z;
    sint32 anchor_z;
    sint32 other_x;
    sint32 anchor_x;
    sint32 angle;
    uint32 cosine;
    uint32 sine;
    uint32 correction;
    uint32 x_change;
    uint32 z_change;
    uint32 velocity_x;
    uint32 velocity_z;
    uint32 x;
    uint32 z;
    uint32 tangent;
    uint32 normal;

    td5_80049310(car, 1);
    other_z = r_s16(other + 4);
    anchor_z = r_s16(anchor + 4);
    other_x = r_s16(other);
    anchor_x = r_s16(anchor);
    angle = td5_8007b0c0(other_z - anchor_z, other_x - anchor_x);
    cosine = (uint32)rcos(angle);
    sine = (uint32)rsin(angle);
    correction = (uint32)((sint32)(penetration + 4095) >> 12) - 4;
    x_change = td5_car_truncate((0 - correction) * sine, 4);
    z_change = td5_car_truncate(correction * cosine, 4);
    velocity_x = r_u32(car + 0x194);
    velocity_z = r_u32(car + 0x19C);
    x = r_u32(car + 0x1C4);
    z = r_u32(car + 0x1CC);
    w_u32(car + 0x1C4, x + x_change);
    w_u32(car + 0x1CC, z + z_change);
    tangent = td5_car_truncate(velocity_x * cosine + velocity_z * sine, 12);
    normal = td5_car_truncate(velocity_z * cosine - velocity_x * sine, 12);
    if ((sint32)normal >= 0)
    {
        normal = (uint32)((sint32)(0 - normal) / 2);
        if ((sint32)tangent > 384)
            tangent -= 384;
        else if ((sint32)tangent < -384)
            tangent += 384;
        else
            tangent = 0;
        x = td5_car_truncate(tangent * cosine - normal * sine, 12);
        z = td5_car_truncate(normal * cosine + tangent * sine, 12);
        w_u32(car + 0x194, x);
        w_u32(car + 0x19C, z);
        w_u8(car + 0x353, 0);
    }
    return td5_80027d58(car);
}

static uint32 td5_car_boundary_contact(uint32 car, uint32 anchor, uint32 other, uint32 extent, SVECTOR *normal)
{
    uint32 coordinate;
    uint32 origin;
    uint32 product;
    uint32 z_delta;
    uint32 penetration;

    VectorNormalSS(normal, normal);
    coordinate = r_u32(car + 0x1C4);
    origin = (uint32)r_s16(anchor);
    coordinate = (uint32)((sint32)coordinate >> 8) - origin;
    product = coordinate * (uint32)(sint32)normal->vx;
    coordinate = r_u32(car + 0x1CC);
    origin = (uint32)r_s16(anchor + 4);
    z_delta = (uint32)((sint32)coordinate >> 8) - origin;
    penetration = product + z_delta * (uint32)(sint32)normal->vz - extent;
    if ((sint32)penetration >= 0)
        return z_delta;
    return td5_car_boundary_response(car, other, anchor, penetration);
}

static sint32 td5_car_road_distance(uint32 car, uint32 anchor, uint32 segment, uint32 extent, SVECTOR *normal, uint32 *z_delta)
{
    uint32 coordinate;
    uint32 vertex;
    uint32 origin;
    uint32 product;

    VectorNormalSS(normal, normal);
    coordinate = r_u32(car + 0x1C4);
    vertex = (uint32)r_s16(anchor);
    origin = r_u32(segment + 12);
    coordinate = (uint32)((sint32)coordinate >> 8) - vertex - origin;
    product = coordinate * (uint32)(sint32)normal->vx;
    coordinate = r_u32(car + 0x1CC);
    vertex = (uint32)r_s16(anchor + 4);
    origin = r_u32(segment + 20);
    *z_delta = (uint32)((sint32)coordinate >> 8) - vertex - origin;
    return (sint32)(product + *z_delta * (uint32)(sint32)normal->vz - extent);
}

uint32 td5_8002ba04(uint32 first, uint32 second, uint32 original_stack)
{
    static const uint32 position_offsets[6] = {0x30, 0x38, 0x50, 0x3C, 0x44, 0x54};
    static const uint32 delta_offsets[6] = {0x18, 0x20, 0x48, 0x24, 0x2C, 0x4C};
    uint32 first_box;
    uint32 second_box;
    uint32 value;
    uint32 other_value;
    uint32 first_car;
    uint32 second_car;
    uint32 frame;
    uint32 positions;
    uint32 headings;
    uint32 contacts;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 mask;
    uint32 result;
    uint32 time_step;
    uint32 deltas[6];
    uint32 trial[6];
    uint32 fraction;
    uint32 step;
    uint32 iteration;
    uint32 index;
    uint32 bit;
    uint32 corner;
    uint32 car;
    uint32 other;
    uint32 heading;

    FUNCTION_MARKER(0x8002BA04, "SLUS_006.10");
    first_box = 0x800A4060 + first * 20;
    second_box = 0x800A4060 + second * 20;
    value = r_u32(first_box);
    other_value = r_u32(second_box + 8);
    if ((sint32)value >= (sint32)other_value)
        return 0;
    other_value = r_u32(first_box + 8);
    value = r_u32(second_box);
    if ((sint32)value >= (sint32)other_value)
        return 0;
    value = r_u32(first_box + 4);
    other_value = r_u32(second_box + 12);
    if ((sint32)value >= (sint32)other_value)
        return 0;
    other_value = r_u32(first_box + 12);
    value = r_u32(second_box + 4);
    if ((sint32)value >= (sint32)other_value)
        return first * 8;
    first_car = 0x800984A0 + first * 864;
    second_car = 0x800984A0 + second * 864;
    frame = original_stack - 0xC0;
    positions = frame + 0x30;
    headings = frame + 0x50;
    contacts = frame + 0x58;
    x = r_u32(first_car + 0x1C4);
    y = r_u32(first_car + 0x1C8);
    z = r_u32(first_car + 0x1CC);
    w_u32(positions, x);
    w_u32(positions + 4, y);
    w_u32(positions + 8, z);
    x = r_u32(second_car + 0x1C4);
    y = r_u32(second_car + 0x1C8);
    z = r_u32(second_car + 0x1CC);
    w_u32(positions + 12, x);
    w_u32(positions + 16, y);
    w_u32(positions + 20, z);
    value = r_u32(first_car + 0x1BC);
    w_u32(headings, value);
    value = r_u32(second_car + 0x1BC);
    w_u32(headings + 4, value);
    w_u32(frame + 0x10, contacts);
    mask = td5_8002b3e8(first_car, second_car, positions, headings, contacts);
    if (mask == 0)
        return 0;
    fraction = 128;
    step = 128;
    value = r_u32(first_car + 0x194);
    time_step = r_u32(0x80088100);
    deltas[0] = td5_car_truncate((uint32)((sint32)value / 2) * time_step, 8);
    w_u32(frame + 0x18, deltas[0]);
    value = r_u32(first_car + 0x19C);
    deltas[1] = td5_car_truncate((uint32)((sint32)value / 2) * time_step, 8);
    w_u32(frame + 0x20, deltas[1]);
    value = r_u32(second_car + 0x194);
    deltas[3] = td5_car_truncate((uint32)((sint32)value / 2) * time_step, 8);
    w_u32(frame + 0x24, deltas[3]);
    value = r_u32(second_car + 0x19C);
    deltas[4] = td5_car_truncate((uint32)((sint32)value / 2) * time_step, 8);
    w_u32(frame + 0x2C, deltas[4]);
    value = r_u32(first_car + 0x18C);
    deltas[2] = td5_car_truncate((uint32)((sint32)value / 2) * time_step, 8);
    w_u32(frame + 0x48, deltas[2]);
    value = r_u32(second_car + 0x18C);
    deltas[5] = td5_car_truncate((uint32)((sint32)value / 2) * time_step, 8);
    for (index = 0; index < 6; ++index)
        trial[index] = r_u32(frame + position_offsets[index]);
    w_u32(frame + 0x4C, deltas[5]);
    for (index = 0; index < 6; ++index)
        w_u32(frame + position_offsets[index], trial[index] - deltas[index]);
    for (iteration = 0; iteration < 7; ++iteration)
    {
        step /= 2;
        for (index = 0; index < 6; ++index)
        {
            value = r_u32(frame + delta_offsets[index]);
            deltas[index] = (uint32)((sint32)value / 2);
            w_u32(frame + delta_offsets[index], deltas[index]);
        }
        w_u32(frame + 0x10, contacts);
        result = td5_8002b3e8(first_car, second_car, positions, headings, contacts);
        for (index = 0; index < 6; ++index)
        {
            trial[index] = r_u32(frame + position_offsets[index]);
            deltas[index] = r_u32(frame + delta_offsets[index]);
        }
        if (result != 0)
        {
            fraction -= step;
            mask = result;
        }
        else
            fraction += step;
        for (index = 0; index < 6; ++index)
            w_u32(frame + position_offsets[index], result != 0 ? trial[index] - deltas[index] : trial[index] + deltas[index]);
    }
    fraction -= 16;
    if (mask != 0 && mask <= 128 && (mask & (mask - 1)) == 0)
    {
        for (index = 0; (1u << index) != mask; ++index)
            ;
        car = index < 4 ? first_car : second_car;
        other = index < 4 ? second_car : first_car;
        corner = 0x80 + (index & 3) * 12;
        td5_8003f33c(other, other + corner);
        heading = r_u32(headings + (index < 4 ? 0 : 4));
        w_u32(frame + 0x10, fraction);
        return td5_8002a244(car, other, contacts + index * 8, (sint32)heading >> 8, fraction, frame);
    }
    bit = 1;
    for (index = 0; index < 8; ++index)
    {
        if ((mask & bit) != 0)
        {
            car = index < 4 ? first_car : second_car;
            other = index < 4 ? second_car : first_car;
            heading = r_u32(headings + (index < 4 ? 0 : 4));
            w_u32(frame + 0x10, fraction);
            td5_8002a244(car, other, contacts + index * 8, (sint32)heading >> 8, fraction, frame);
        }
        bit <<= 1;
    }
    return 0;
}

void td5_80055184(sint32 angle, sint32 voice, sint32 pitch, sint32 pan, sint32 volume)
{
    uint32 half;
    uint32 distance;
    uint32 product;
    uint32 first_volume;
    uint32 second_volume;
    sint32 side;

    FUNCTION_MARKER(0x80055184, "SLUS_006.10");
    if (angle < 2048)
    {
        td5_8005552c(voice, pitch, pan, volume);
        td5_8005552c((sint32)((uint32)voice + 1), pitch, pan, 0);
        return;
    }
    side = pan < 0 ? -4096 : 4096;
    half = (uint32)(volume / 2);
    distance = (uint32)angle - 3072;
    if ((sint32)distance < 0)
        distance = 3072 - (uint32)angle;
    product = td5_car_truncate(distance * half, 10);
    first_volume = half + product;
    half = (uint32)(volume / 2);
    distance = (uint32)angle - 3072;
    if ((sint32)distance < 0)
        distance = 3072 - (uint32)angle;
    product = td5_car_truncate(distance * half, 10);
    second_volume = half - product;
    td5_8005552c(voice, pitch, side, (sint32)first_volume);
    td5_8005552c((sint32)((uint32)voice + 1), pitch, -side, (sint32)(0 - second_volume));
}

uint32 td5_80053348(uint32 original_stack)
{
    uint32 frame;
    uint32 distances;
    uint32 index;
    uint32 count;
    uint32 collected;
    uint32 car_offset;
    uint32 entry;
    uint32 coordinate;
    uint32 view;
    uint32 x;
    uint32 z;
    uint32 value;
    uint32 pass;
    uint32 current;
    uint32 next;
    uint32 first;
    uint32 second;
    uint32 limit;
    uint32 fixed_limit;
    uint32 selected;
    uint32 focus;
    uint32 channel;
    uint32 mirror;
    uint32 pitch;
    uint32 volume;
    uint32 enabled;
    uint32 override;
    uint32 restore;
    uint32 old_speed;
    sint32 random;
    sint32 throttle;

    FUNCTION_MARKER(0x80053348, "SLUS_006.10");
    frame = original_stack - 0xB8;
    distances = frame + 0x20;
    value = r_u8(0x80087BDA);
    if (value < 255)
        w_u8(0x80087BDA, value + 1);
    count = r_u32(0x800875E4);
    collected = 0;
    index = 0;
    car_offset = 0;
    if ((sint32)count > 0)
    {
        do
        {
            focus = r_u32(0x80088498);
            if (index != focus)
            {
                entry = distances + collected * 4;
                w_u32(entry + 0x38, index);
                coordinate = (uint32)(r_s32(0x80098664 + car_offset) >> 8);
                view = r_u32(0x8009765C);
                x = view - coordinate;
                if ((sint32)x < 0)
                    x = coordinate - view;
                coordinate = (uint32)(r_s32(0x8009866C + car_offset) >> 8);
                view = r_u32(0x80097664);
                z = view - coordinate;
                if ((sint32)z < 0)
                    z = coordinate - view;
                value = 0x10000;
                if ((sint32)x <= 32767 && (sint32)z <= 32767)
                    value = (uint32)SquareRoot0((sint32)(x * x + z * z));
                w_u32(entry, value);
                ++collected;
            }
            count = r_u32(0x800875E4);
            ++index;
            car_offset += 864;
        } while ((sint32)index < (sint32)count);
        count = r_u32(0x800875E4);
    }
    fixed_limit = count - 2;
    for (pass = 0; pass < 6; ++pass)
    {
        if ((sint32)fixed_limit > 1)
        {
            limit = r_u32(0x800875E4) - 2;
            entry = distances + 4;
            index = 1;
            do
            {
                current = r_u32(entry);
                next = r_u32(entry + 4);
                if ((sint32)next < (sint32)current)
                {
                    first = r_u32(entry + 0x3C);
                    second = r_u32(entry + 0x38);
                    w_u32(entry + 0x38, first);
                    w_u32(entry + 0x3C, second);
                    next = r_u32(entry + 4);
                    current = r_u32(entry);
                    w_u32(entry, next);
                    w_u32(entry + 4, current);
                }
                entry += 4;
                ++index;
            } while ((sint32)index < (sint32)limit);
        }
    }
    if (r_u32(0x8008873C) != 0 && r_s32(0x800875E4) >= 6)
    {
        random = randPSX();
        if (random % 1000 < 100)
        {
            random = randPSX();
            w_u32(frame + 0x14, 0);
            w_u32(frame + 0x18, 1);
            value = 0x80098664 + (uint32)(random % 6) * 864;
            w_u32(frame + 0x10, value);
            td5_play_positioned_sound(14, 18, 2048, 4096, value, 0);
        }
    }
    focus = r_u32(0x80088498);
    car_offset = focus * 864;
    if (r_s32(0x80098788 + car_offset) < 1000 && r_s16(0x800987B6 + car_offset) == 256 && r_u8(0x80087BDA) >= 41)
    {
        value = 0x80098664 + car_offset;
        w_u32(frame + 0x10, value);
        w_u32(frame + 0x14, 0);
        w_u32(frame + 0x18, 1);
        td5_play_positioned_sound(14, 18, 2048, 4096, value, 0);
        w_u8(0x80087BDA, 0);
    }
    focus = r_u32(0x80088498);
    car_offset = focus * 864;
    throttle = r_s16(0x800987B6 + car_offset);
    value = r_u32(0x80098788 + car_offset);
    value = (value - 4000 - (uint32)((256 - throttle) / 4)) << 12;
    pitch = (uint32)((sint32)value / 5000) + 4096;
    w_u32(0x800919BC, pitch);
    value = r_u32(0x80098788 + car_offset);
    throttle = r_s16(0x800987B6 + car_offset);
    first = (uint32)(256 - throttle) * 2;
    second = td5_car_truncate(value, 2);
    volume = (sint32)(second + 3072) < 4097 ? second - (first - 3072) : 4096 - first;
    enabled = r_u32(0x80088668);
    w_u32(0x800919B8, volume);
    if (enabled != 0)
    {
        pitch = r_u32(0x800919BC);
        w_u32(0x80091A14, (uint32)-4096);
        w_u32(0x80091A18, pitch);
    }
    count = r_u32(0x800875E4);
    index = 1;
    while ((sint32)count >= 5 ? (sint32)index < 4 : (sint32)index < (sint32)count)
    {
        selected = r_u32(frame + 0x54 + index * 4);
        channel = index * 184;
        mirror = (index * 2 + 1) * 92;
        car_offset = selected * 864;
        if ((sint32)selected < 6)
        {
            override = r_u32(0x8008873C);
            if (override != 0)
            {
                old_speed = r_u32(0x80098788 + car_offset);
                w_u32(0x80098788 + car_offset, 2000);
            }
            value = r_u32(0x80098788 + car_offset);
            value = (value - 4000) << 12;
            pitch = (uint32)((sint32)value / 5000) + 4096;
            w_u32(0x800919BC + channel, pitch);
            value = r_u32(0x80098788 + car_offset);
            volume = td5_car_truncate(value, 2) + 3072;
            if ((sint32)volume >= 4097)
                volume = 4096;
            enabled = r_u32(0x80088668);
            w_u32(0x800919B8 + channel, volume);
            if (enabled != 0)
            {
                pitch = r_u32(0x800919BC + channel);
                w_u32(0x800919B8 + mirror, (uint32)-4096);
                w_u32(0x800919C4 + mirror, 0x80098664 + car_offset);
                w_u32(0x800919C8 + mirror, 0x80098634 + car_offset);
                w_u32(0x800919BC + mirror, pitch);
            }
            restore = r_u32(0x8008873C);
            w_u32(0x800919C4 + channel, 0x80098664 + car_offset);
            w_u32(0x800919C8 + channel, 0x80098634 + car_offset);
            if (override != 0 && restore != 0)
                w_u32(0x80098788 + car_offset, old_speed);
        }
        else
        {
            enabled = r_u32(0x80088668);
            w_u32(0x800919BC + channel, 4096);
            w_u32(0x800919B8 + channel, 4096);
            if (enabled != 0)
            {
                pitch = r_u32(0x800919BC + channel);
                w_u32(0x800919B8 + mirror, (uint32)-4096);
                w_u32(0x800919C4 + mirror, 0x80098664 + car_offset);
                w_u32(0x800919C8 + mirror, 0x80098634 + car_offset);
                w_u32(0x800919BC + mirror, pitch);
            }
            w_u32(0x800919C4 + channel, 0x80098664 + car_offset);
            w_u32(0x800919C8 + channel, 0x80098634 + car_offset);
        }
        count = r_u32(0x800875E4);
        ++index;
    }
    return index * 4;
}

uint32 td5_80029524(uint32 slot)
{
    uint32 car;
    uint32 base;
    uint32 lane;
    uint32 segment;
    uint32 segment_offset;
    uint32 count;
    uint32 index;
    uint32 angle;
    uint32 cosine;
    uint32 sine;
    uint32 model;
    uint32 bound_x;
    uint32 bound_z;
    uint32 first;
    uint32 second;
    uint32 points;
    uint32 anchor;
    uint32 other;
    uint32 first_z;
    uint32 second_z;
    uint32 first_x;
    uint32 second_x;
    uint32 extent;
    uint32 type;
    uint32 adjustment;
    uint32 z_delta;
    sint32 node;
    sint32 penetration;
    SVECTOR normal;

    FUNCTION_MARKER(0x80029524, "SLUS_006.10");
    car = 0x800984A0 + slot * 864;
    node = r_s16(car + 0x70);
    base = r_u32(0x800884C4);
    lane = r_u8(car + 0x7C);
    segment_offset = (uint32)node * 24;
    segment = base + segment_offset;
    count = r_u8(segment + 3) & 15;
    if (lane < 2)
    {
        index = r_u8(car + 0x34D);
        angle = td5_8004d19c(0x80097750 + index * 284) & 0x7FF;
        if (angle >= 1024)
            angle = 2047 - angle;
        cosine = (uint32)rcos((sint32)angle);
        sine = (uint32)rsin((sint32)angle);
        model = r_u32(car + 0x180);
        base = r_u32(0x800884C4);
        bound_x = (uint32)r_s16(model + 8);
        first = r_u16(base + segment_offset + 6);
        points = r_u32(0x8008850C);
        second = r_u16(base + segment_offset + 4);
        anchor = points + first * 6;
        other = points + second * 6;
        bound_z = (uint32)r_s16(model + 12);
        first_z = r_u16(other + 4);
        second_z = r_u16(anchor + 4);
        normal.vy = 0;
        normal.vx = (sint16)(first_z - second_z);
        first_x = r_u16(other);
        second_x = r_u16(anchor);
        normal.vz = (sint16)(second_x - first_x);
        extent = cosine * bound_x + sine * bound_z;
        penetration = td5_car_road_distance(car, anchor, segment, extent, &normal, &z_delta);
        if (penetration < 0)
            td5_car_boundary_response(car, other, anchor, (uint32)penetration);
    }
    if ((sint32)lane < (sint32)(count - 2))
        return 1;
    index = r_u8(car + 0x34D);
    base = r_u32(0x800884C4);
    segment = base + segment_offset;
    type = r_u8(segment);
    angle = td5_8004d19c(0x80097750 + index * 284) & 0x7FF;
    if (angle >= 1024)
        angle = 2047 - angle;
    cosine = (uint32)rcos((sint32)angle);
    sine = (uint32)rsin((sint32)angle);
    base = r_u32(0x800884C4);
    model = r_u32(car + 0x180);
    adjustment = r_u32(0x8007E03C + type * 8);
    first = r_u16(base + segment_offset + 4);
    points = r_u32(0x8008850C);
    bound_x = (uint32)r_s16(model + 8);
    second = r_u16(base + segment_offset + 6);
    anchor = points + (first + count + adjustment) * 6;
    adjustment = r_u32(0x8007E040 + type * 8);
    first_z = r_u16(anchor + 4);
    other = points + (second + count + adjustment) * 6;
    bound_z = (uint32)r_s16(model + 12);
    second_z = r_u16(other + 4);
    normal.vy = 0;
    normal.vx = (sint16)(second_z - first_z);
    first_x = r_u16(other);
    second_x = r_u16(anchor);
    normal.vz = (sint16)(second_x - first_x);
    extent = cosine * bound_x + sine * bound_z;
    penetration = td5_car_road_distance(car, anchor, segment, extent, &normal, &z_delta);
    if (penetration < 0)
        return td5_car_boundary_response(car, other, anchor, (uint32)penetration);
    return z_delta;
}

uint32 td5_80029efc(uint32 slot)
{
    uint32 car;
    uint32 index;
    uint32 angle;
    uint32 sine;
    uint32 cosine;
    uint32 model;
    uint32 first;
    uint32 second;
    uint32 extent;
    uint32 segment;
    uint32 base;
    uint32 count;
    uint32 vertex;
    uint32 low;
    uint32 high;
    uint32 first_z;
    uint32 second_z;
    uint32 first_x;
    uint32 second_x;
    sint32 node;
    SVECTOR normal;

    FUNCTION_MARKER(0x80029EFC, "SLUS_006.10");
    car = 0x800984A0 + slot * 864;
    index = r_u8(car + 0x34D);
    angle = td5_8004d19c(0x80097750 + index * 284) & 0x7FF;
    if (angle >= 1024)
        angle = 2047 - angle;
    sine = (uint32)rsin((sint32)angle);
    cosine = (uint32)rcos((sint32)angle);
    model = r_u32(car + 0x180);
    first = (uint32)r_s16(model + 8);
    second = (uint32)r_s16(model + 12);
    segment = r_u32(0x80088738);
    node = r_s16(car + 0x70);
    extent = sine * first + cosine * second;
    if ((uint32)node != segment - 1)
        return segment - 1;
    base = r_u32(0x800884C4);
    segment = base + segment * 24;
    count = r_u8(segment + 3);
    vertex = r_u16(segment + 4);
    base = r_u32(0x8008850C);
    high = base + (vertex + (count & 15)) * 6;
    low = base + vertex * 6;
    first_z = r_u16(low + 4);
    second_z = r_u16(high + 4);
    normal.vy = 0;
    normal.vx = (sint16)(first_z - second_z);
    first_x = r_u16(high);
    second_x = r_u16(low);
    normal.vz = (sint16)(first_x - second_x);
    return td5_car_boundary_contact(car, high, low, extent, &normal);
}

uint32 td5_80029bb8(uint32 slot)
{
    uint32 car;
    uint32 segment;
    uint32 index;
    uint32 angle;
    uint32 sine;
    uint32 cosine;
    uint32 base;
    uint32 model;
    uint32 count;
    uint32 first;
    uint32 second;
    uint32 vertex;
    uint32 extent;
    uint32 low;
    uint32 high;
    uint32 first_z;
    uint32 second_z;
    uint32 first_x;
    uint32 second_x;
    sint32 node;
    SVECTOR normal;

    FUNCTION_MARKER(0x80029BB8, "SLUS_006.10");
    car = 0x800984A0 + slot * 864;
    segment = r_u32(0x80088508);
    node = r_s16(car + 0x70);
    if ((uint32)node != segment + 1)
        return segment + 1;
    index = r_u8(car + 0x34D);
    angle = td5_8004d19c(0x80097750 + index * 284) & 0x7FF;
    if (angle >= 1024)
        angle = 2047 - angle;
    sine = (uint32)rsin((sint32)angle);
    cosine = (uint32)rcos((sint32)angle);
    segment = r_u32(0x80088508);
    base = r_u32(0x800884C4);
    segment = base + segment * 24;
    model = r_u32(car + 0x180);
    count = r_u8(segment + 3);
    first = (uint32)r_s16(model + 8);
    vertex = r_u16(segment + 4);
    second = (uint32)r_s16(model + 12);
    base = r_u32(0x8008850C);
    extent = sine * first + cosine * second;
    low = base + vertex * 6;
    high = base + (vertex + (count & 15)) * 6;
    first_z = r_u16(high + 4);
    second_z = r_u16(low + 4);
    normal.vy = 0;
    normal.vx = (sint16)(first_z - second_z);
    first_x = r_u16(high);
    second_x = r_u16(low);
    normal.vz = (sint16)(second_x - first_x);
    return td5_car_boundary_contact(car, low, high, extent, &normal);
}

uint32 td5_8002765c(uint32 car)
{
    uint32 x;
    uint32 z;
    uint32 damped;
    uint32 velocity_x;
    uint32 velocity_z;
    uint32 yaw_rate;
    uint32 heading;
    uint32 steering;
    uint32 cosine;
    uint32 sine;
    uint32 turned_cosine;
    uint32 turned_sine;
    uint32 steering_cosine;
    uint32 steering_sine;
    uint32 lateral;
    uint32 longitudinal;
    uint32 wheel;
    uint32 denominator;
    uint32 side_force;
    uint32 yaw_force;
    uint32 rolling;
    uint32 first;
    uint32 second;
    uint32 third;
    uint32 front;
    uint32 rear;
    uint32 yaw_change;
    uint32 x_change;
    uint32 z_change;
    uint32 position;
    uint32 rate;
    uint32 new_yaw;
    uint32 new_x;
    uint32 new_z;
    uint32 checked_position;
    uint32 time_step;
    uint32 angle;

    FUNCTION_MARKER(0x8002765C, "SLUS_006.10");
    x = r_u32(car + 0x194);
    damped = td5_car_truncate(0 - x, 8);
    z = r_u32(car + 0x19C);
    w_u32(car + 0x194, x + damped);
    damped = td5_car_truncate(0 - z, 8);
    steering = r_u32(car + 0x2E4);
    heading = r_u32(car + 0x1BC);
    velocity_x = r_u32(car + 0x194);
    yaw_rate = r_u32(car + 0x18C);
    velocity_z = z + damped;
    w_u32(car + 0x19C, velocity_z);
    heading = (uint32)((sint32)heading >> 8);
    steering = td5_car_truncate(steering, 8);
    cosine = (uint32)rcos((sint32)heading);
    sine = (uint32)rsin((sint32)heading);
    angle = heading + steering;
    turned_cosine = (uint32)rcos((sint32)angle);
    turned_sine = (uint32)rsin((sint32)angle);
    steering_cosine = (uint32)rcos((sint32)steering);
    steering_sine = (uint32)rsin((sint32)steering);
    wheel = (uint32)r_s16(car + 0x316) << 2;
    lateral = td5_car_truncate(velocity_x * cosine - velocity_z * sine, 12);
    longitudinal = td5_car_truncate(velocity_x * sine + velocity_z * cosine, 12);
    first = td5_car_truncate(steering_sine * 332, 12);
    first = td5_car_truncate(first * steering_sine, 12);
    second = td5_car_truncate(steering_cosine * 625, 12);
    second = td5_car_truncate(second * steering_cosine, 12);
    denominator = first + second;
    side_force = lateral * 400;
    yaw_force = yaw_rate * 276;
    first = td5_car_truncate(0 - (side_force + yaw_force), 10);
    first = td5_car_truncate(first * 800, 12) * steering_cosine;
    rolling = longitudinal + wheel;
    second = td5_car_truncate(rolling * 332, 12) * steering_sine;
    third = ((uint32)((sint32)(yaw_rate * 400) / 652) - lateral) * 175;
    front = (uint32)td5_divide_checked((sint32)(first + second), (sint32)denominator);
    first = td5_car_truncate(third, 12);
    first = td5_car_truncate(first * steering_sine, 12);
    second = td5_car_truncate(0 - rolling * 19, 12);
    second = td5_car_truncate(second * steering_cosine, 12);
    first = (first + second) * steering_sine;
    second = td5_car_truncate(yaw_force - side_force, 10);
    second = td5_car_truncate(second * 800, 12);
    second = td5_car_truncate(second * steering_cosine, 12) * steering_cosine;
    rear = (uint32)td5_divide_checked((sint32)(first + second), (sint32)denominator);
    if ((sint32)front > 2048)
        front = 2048;
    else if ((sint32)front < -2048)
        front = (uint32)-2048;
    if ((sint32)rear > 2048)
        rear = 2048;
    else if ((sint32)rear < -2048)
        rear = (uint32)-2048;
    first = td5_car_truncate(rear * cosine, 12);
    second = td5_car_truncate((0 - rear) * sine, 12);
    third = td5_car_truncate(front * turned_cosine, 12);
    x_change = first + third;
    third = td5_car_truncate((0 - front) * turned_sine, 12);
    z_change = second + third;
    first = front * td5_car_truncate(steering_cosine * 400, 12) - rear * 400;
    yaw_change = (uint32)((sint32)first / 276);
    x_change += td5_car_truncate(wheel * sine, 12);
    z_change += td5_car_truncate(wheel * cosine, 12);
    position = r_u32(car + 0x2B4);
    rate = r_u32(car + 0x2C4);
    new_yaw = r_u32(car + 0x18C);
    new_x = r_u32(car + 0x194);
    new_z = r_u32(car + 0x19C);
    position += rate;
    w_u32(car + 0x18C, new_yaw + yaw_change);
    w_u32(car + 0x194, new_x + x_change);
    w_u32(car + 0x19C, new_z + z_change);
    w_u32(car + 0x2B4, position);
    rate = r_u32(car + 0x2C4);
    first = (uint32)((sint32)(rear + front) / 2) - td5_car_truncate(position, 3);
    checked_position = r_u32(car + 0x2B4);
    w_u32(car + 0x2C4, rate + first - td5_car_truncate(rate, 3));
    if ((sint32)checked_position > 8192)
    {
        w_u32(car + 0x2B4, 8192);
        w_u32(car + 0x2C4, 0);
    }
    if (r_s32(car + 0x2B4) < -8192)
    {
        w_u32(car + 0x2B4, (uint32)-8192);
        w_u32(car + 0x2C4, 0);
    }
    position = r_u32(car + 0x2B8);
    rate = r_u32(car + 0x2C8);
    position += rate;
    w_u32(car + 0x2B8, position);
    rate = r_u32(car + 0x2C8);
    first = (uint32)((sint32)wheel >> 1) - td5_car_truncate(position, 3);
    checked_position = r_u32(car + 0x2B8);
    w_u32(car + 0x2C8, rate + first - td5_car_truncate(rate, 3));
    if ((sint32)checked_position > 16384)
    {
        w_u32(car + 0x2B8, 16384);
        w_u32(car + 0x2C8, 0);
    }
    if (r_s32(car + 0x2B8) < -16384)
    {
        w_u32(car + 0x2B8, (uint32)-16384);
        w_u32(car + 0x2C8, 0);
    }
    new_yaw = r_u32(car + 0x18C);
    time_step = r_u32(0x80088100);
    first = td5_car_truncate(new_yaw * time_step, 8);
    new_x = r_u32(car + 0x194);
    second = td5_car_truncate(new_x * time_step, 8);
    angle = r_u32(car + 0x1BC);
    w_u32(car + 0x1BC, angle + first);
    new_z = r_u32(car + 0x19C);
    third = td5_car_truncate(new_z * time_step, 8);
    position = r_u32(car + 0x1C4);
    w_u32(car + 0x1C4, position + second);
    position = r_u32(car + 0x1CC);
    w_u32(car + 0x1CC, position + third);
    w_u32(car + 0x2EC, longitudinal);
    return longitudinal;
}

uint32 td5_80027fc4(uint32 slot, uint32 original_stack)
{
    uint32 car;
    uint32 timer;
    uint32 state;
    sint32 x;
    sint32 y;
    sint32 z;

    FUNCTION_MARKER(0x80027FC4, "SLUS_006.10");
    car = 0x800984A0 + slot * 864;
    timer = r_u16(car + 0x310);
    state = r_u8(car + 0x351);
    w_u8(car + 0x353, 0);
    w_u16(car + 0x310, timer + 1);
    if (state == 0)
    {
        if (r_u32(0x80088360) == 0xFFFFFFFF)
            td5_8002765c(car);
        td5_80027d58(car);
        td5_80029efc(slot);
        td5_80029bb8(slot);
        return td5_80029524(slot);
    }
    if (state != 1)
        return 1;
    td5_8002d6c4(car, original_stack - 0x20);
    x = r_s16(car + 0x1D0);
    y = r_s16(car + 0x1D2);
    z = r_s16(car + 0x1D4);
    w_u32(car + 0x1B8, (uint32)x << 8);
    w_u32(car + 0x1BC, (uint32)y << 8);
    w_u32(car + 0x1C0, (uint32)z << 8);
    td5_8002d824(car, original_stack - 0x20);
    td5_80029180(slot, 0x8002D79C, 0x800877B8, original_stack - 0x20);
    td5_80028f74(slot, 0x8002D79C, 0x800877B8, original_stack - 0x20);
    return td5_80028bd0(slot, 0x8002D79C, 0x800877B8, original_stack - 0x20);
}

uint32 td5_8001b090(uint32 car)
{
    uint32 cursor;
    uint32 count;
    uint32 index;
    uint32 lower;
    uint32 upper;
    uint32 value;
    sint32 node;

    FUNCTION_MARKER(0x8001B090, "SLUS_006.10");
    cursor = r_u32(0x80088584);
    node = r_s16(car + 0x70);
    count = r_u32(cursor + 4);
    if ((uint32)node <= count)
    {
        w_u16(car + 0x72, (uint32)node);
        w_u16(car + 0x74, (uint32)node);
        return 0;
    }
    count = r_u32(cursor + 0x14);
    if (count == 0)
        return 0;
    index = 0;
    do
    {
        lower = r_u16(cursor + 0x18);
        if (node >= (sint32)lower)
        {
            upper = r_u16(cursor + 0x1A);
            if ((sint32)upper >= node)
            {
                value = r_u16(cursor + 0x1C);
                value += (uint32)node - lower;
                w_u16(car + 0x72, value);
                w_u16(car + 0x74, value);
                return value;
            }
        }
        ++index;
        cursor += 6;
    } while (index < count);
    return 0;
}

uint32 td5_8004ead8(uint32 height_address)
{
    uint32 count;
    uint32 cursor;
    uint32 index;
    uint32 selected;
    uint32 value;
    uint32 state_offset;
    uint32 car_offset;
    uint32 flags;
    uint32 base;
    uint32 sides;
    uint32 side;
    uint32 mapped;
    sint32 node;
    sint32 next_node;
    sint32 player_node;
    sint32 car_node;
    sint32 gap;
    sint32 best_gap;

    FUNCTION_MARKER(0x8004EAD8, "SLUS_006.10");
    count = r_u32(0x800875E4);
    if ((sint32)count < 7)
        return 1;
    cursor = r_u32(0x800883E0);
    node = r_s16(cursor);
    if (node != -1)
    {
        player_node = r_s16(0x80098512);
        do
        {
            node = r_s16(cursor);
            if (node >= player_node && node - player_node >= 40)
                break;
            next_node = r_s16(cursor + 4);
            cursor += 4;
            w_u32(0x800883E0, cursor);
        } while (next_node != -1);
    }
    count = r_u32(0x800875E4);
    best_gap = 0;
    index = 6;
    while ((sint32)count >= 13 ? (sint32)index < 12 : (sint32)index < (sint32)count)
    {
        player_node = r_s16(0x80098512);
        car_node = r_s16(0x80098512 + index * 864);
        gap = player_node - car_node;
        if (gap > best_gap)
        {
            selected = index;
            best_gap = gap;
        }
        count = r_u32(0x800875E4);
        ++index;
    }
    if (best_gap < 41)
        return 1;
    cursor = r_u32(0x800883E0);
    node = r_s16(cursor);
    if (node == -1)
        return 9;
    if (selected == 9)
    {
        value = r_u32(0x800981C8);
        if (value != 0xFFFFFFFF)
            return value;
    }
    state_offset = selected * 284;
    car_offset = selected * 864;
    flags = r_u8(cursor + 2);
    w_u32(0x800977B8 + state_offset, selected * 4 + 16);
    w_u32(0x8009784C + state_offset, flags & 1);
    node = r_s16(cursor);
    base = r_u32(0x800884C4);
    sides = r_u8(base + (uint32)node * 24 + 3) & 15;
    side = r_u8(cursor + 3);
    if (side < sides)
    {
        w_u32(0x8009775C + state_offset, 0);
        node = r_s16(cursor);
        flags = r_u8(cursor + 2);
        side = r_u8(cursor + 3);
        td5_8004d984(selected, (uint32)node, side, flags & 1, height_address);
        td5_8001affc(0x800984A0 + car_offset);
    }
    else
    {
        w_u32(0x8009775C + state_offset, 1);
        node = r_s16(cursor);
        mapped = td5_80050704(node);
        cursor = r_u32(0x800883E0);
        node = r_s16(cursor);
        base = r_u32(0x800884C4);
        flags = r_u8(cursor + 2);
        sides = r_u8(base + (uint32)node * 24 + 3);
        side = r_u8(cursor + 3);
        td5_8004d984(selected, mapped, side - (sides & 15), flags & 1, height_address);
        td5_8001b090(0x800984A0 + car_offset);
    }
    cursor = r_u32(0x800883E0);
    w_u32(0x800883E0, cursor + 4);
    w_u32(0x800977D8 + state_offset, 0);
    w_u32(0x80097840 + state_offset, 0);
    w_u32(0x800977CC + state_offset, 0xFFFFFFFF);
    w_u32(0x8009878C + car_offset, 0);
    w_u32(0x80098784 + car_offset, 0);
    w_u8(0x800987F1 + car_offset, 0);
    w_u32(0x80098658 + car_offset, 0);
    w_u32(0x80098660 + car_offset, 0);
    w_u32(0x80098628 + car_offset, 0);
    w_u32(0x8009862C + car_offset, 0);
    w_u32(0x80098630 + car_offset, 0);
    w_u32(0x80098634 + car_offset, 0);
    w_u32(0x80098638 + car_offset, 0);
    w_u32(0x8009863C + car_offset, 0);
    w_u16(0x800987B0 + car_offset, 0);
    return car_offset;
}

uint32 td5_8004bea8(uint32 owner)
{
    uint32 mode;
    uint32 own;
    uint32 selected;
    uint32 index;
    uint32 first;
    uint32 second;
    sint32 own_node;
    sint32 candidate_node;
    sint32 gap;
    sint32 best_gap;

    FUNCTION_MARKER(0x8004BEA8, "SLUS_006.10");
    mode = r_u32(owner + 0xFC);
    own = r_u32(owner + 0xD4);
    selected = own;
    best_gap = 192000;
    for (index = 0; index < 12; ++index)
    {
        if (index == own)
            continue;
        if (mode == 0)
        {
            own_node = r_s16(0x80098510 + own * 864);
            candidate_node = r_s16(0x80098510 + index * 864);
            if (candidate_node < own_node)
                continue;
            first = r_u8(0x8009851C + own * 864);
            second = r_u8(0x8009851C + index * 864);
            if (first != second)
                continue;
            gap = candidate_node - own_node;
        }
        else
        {
            first = r_u8(0x8009851C + index * 864);
            second = r_u8(0x8009851C + own * 864);
            if (first != second)
                continue;
            candidate_node = r_s16(0x80098510 + index * 864);
            own_node = r_s16(0x80098510 + own * 864);
            if (own_node < candidate_node)
                continue;
            gap = own_node - candidate_node;
        }
        first = r_u32(0x8009775C + own * 284);
        second = r_u32(0x8009775C + index * 284);
        if (first == second && gap < best_gap)
        {
            best_gap = gap;
            selected = index;
        }
    }
    if (best_gap == 0 || best_gap >= 33)
        return r_u32(owner + 0xD4);
    return selected;
}

static void td5_ai_follow_segment(uint32 car_offset, uint32 reverse, uint32 node_address)
{
    uint32 mapped;
    uint32 base;
    uint32 current;
    uint32 adjacent;
    uint32 first;
    uint32 second;
    uint32 lane_index;
    uint32 type;
    sint32 lane;
    sint32 node;
    sint32 threshold;
    sint32 direction;

    direction = reverse != 0 ? -1 : 1;
    lane = r_s16(0x80098512 + car_offset);
    mapped = (uint32)td5_80050704(lane + direction);
    if (reverse == 0 && mapped != 0xFFFFFFFF)
    {
        node = r_s16(0x80098510 + car_offset);
        threshold = r_s32(0x8008841C);
        if (node < threshold && threshold < (sint32)mapped)
        {
            lane = r_s16(0x80098512 + car_offset);
            base = r_u32(0x800884C4);
            first = r_u8(base + (uint32)lane * 24 + 27) & 15;
            lane_index = r_u8(0x8009851C + car_offset);
            if (lane_index >= first)
            {
                lane_index -= first;
                w_u16(node_address + 2, (uint32)lane + 1);
                goto selected_segment;
            }
        }
    }
    else if (reverse != 0 && mapped != 0xFFFFFFFF)
    {
        lane = r_s16(0x80098512 + car_offset);
        base = r_u32(0x800884C4);
        first = r_u8(base + (uint32)lane * 24 - 21) & 15;
        lane_index = r_u8(0x8009851C + car_offset);
        if (lane_index >= first)
        {
            node = r_s16(0x80098510 + car_offset);
            second = r_u8(base + (uint32)node * 24 + 3);
            w_u16(node_address + 2, (uint32)lane - 1);
            lane_index += first - (second & 15);
            goto selected_segment;
        }
    }
    if (mapped == 0xFFFFFFFF)
    {
        node = r_s16(0x80098510 + car_offset);
        threshold = r_s32(0x8008841C);
        if (threshold < node)
        {
            lane = r_s16(0x80098512 + car_offset);
            lane_index = r_u8(0x8009851C + car_offset);
            base = r_u32(0x800884C4);
            first = r_u8(base + (uint32)lane * 24 + (reverse != 0 ? (uint32)-21 : 27));
            second = r_u8(base + (uint32)node * 24 + 3);
            mapped = (uint32)lane + (uint32)direction;
            w_u16(node_address + 2, mapped);
            lane_index += (first & 15) - (second & 15);
            goto selected_segment;
        }
    }
    node = r_s16(0x80098510 + car_offset);
    base = r_u32(0x800884C4);
    mapped = (uint32)node + (uint32)direction;
    current = base + (uint32)node * 24;
    first = r_u32(current);
    second = r_u32(base + mapped * 24);
    if ((first & 0x0F000000) == (second & 0x0F000000))
        lane_index = r_u8(0x8009851C + car_offset);
    else
    {
        type = r_u8(current);
        lane_index = r_u8(0x8009851C + car_offset);
        if (reverse != 0 ? (type >= 3 && type < 5) : (type >= 6 && type < 8))
            --lane_index;
        base = r_u32(0x800884C4);
        adjacent = base + mapped * 24;
        type = r_u8(adjacent);
        if (reverse != 0 ? (type >= 6 && type < 8) : (type >= 3 && type < 5))
            ++lane_index;
    }
    lane = r_u16(0x80098512 + car_offset);
    w_u16(node_address + 2, (uint32)lane + (uint32)direction);
selected_segment:
    w_u8(node_address + 12, lane_index);
    w_u16(node_address, mapped);
}

uint32 td5_8004f03c(uint32 slot, uint32 original_stack)
{
    uint32 state_offset;
    uint32 car_offset;
    uint32 owner;
    uint32 mode;
    uint32 angle;
    uint32 value;
    uint32 valid;
    uint32 node_address;
    uint32 point;
    uint32 selected;
    uint32 other_offset;
    uint32 result;
    sint32 node;
    sint32 threshold;
    sint32 speed;
    sint32 other_speed;
    sint32 other_node;
    sint32 gap;
    sint32 time;
    sint32 quotient;

    FUNCTION_MARKER(0x8004F03C, "SLUS_006.10");
    state_offset = slot * 284;
    car_offset = slot * 864;
    owner = 0x80097750 + state_offset;
    td5_8004ead8(original_stack - 0x170);
    angle = td5_8004d19c(owner);
    mode = r_u32(0x8009784C + state_offset);
    if (mode == 0)
    {
        if (angle - 1025 < 2047)
        {
            value = r_u32(0x8009866C) & 7;
            w_u32(0x800977D8 + state_offset, value);
            if (value == 0)
                w_u32(0x800977D8 + state_offset, 1);
            if (slot == 9)
            {
                if (r_u32(0x80087724) == 0)
                    td5_set_engine_parameter(0);
                w_u32(0x800981C8, 0xFFFFFFFF);
            }
        }
    }
    else if (((angle + 2048) & 0xFFF) - 1025 < 2047)
    {
        if (r_u32(0x800977D8 + state_offset) == 0)
            w_u32(0x800977D8 + state_offset, 1);
        if (slot == 9 && r_u32(0x80087724) == 0)
            td5_set_engine_parameter(0);
    }
    node = r_s16(0x80098510 + car_offset);
    valid = 0;
    if (node >= 3)
    {
        threshold = r_s32(0x8008841C);
        valid = node < (sint32)((uint32)threshold - 8);
    }
    if (valid == 0)
        valid = r_u32(0x8009775C + state_offset) != 0;
    if (valid == 0 || r_u32(0x80097840 + state_offset) != 0 || r_u32(0x800977D8 + state_offset) != 0)
    {
        w_u8(0x800987E5 + car_offset, 1);
        w_u16(0x800987B6 + car_offset, 0);
        return car_offset;
    }
    w_u8(0x800987E5 + car_offset, 0);
    value = r_u32(0x80087624);
    w_u16(0x800987B6 + car_offset, 60);
    if (value != 0)
        w_u16(0x800987B6 + car_offset, 0);
    mode = r_u32(0x8009784C + state_offset);
    node_address = original_stack - 0x48;
    point = original_stack - 0x58;
    td5_ai_follow_segment(car_offset, mode, node_address);
    td5_8001dbc0(node_address, point);
    td5_8004d268(owner, point);
    td5_8004d330(owner, 0x8000);
    mode = r_u32(owner + 0xFC);
    selected = td5_8004bea8(owner);
    if (mode != 0)
    {
        value = r_u32(owner + 0xD4);
        if (selected == value)
            return selected;
    }
    else if (selected == slot)
        return selected;
    other_offset = selected * 284;
    speed = r_s32(owner + 0x60);
    other_speed = r_s32(0x800977B0 + other_offset);
    if (mode != 0 ? speed >= other_speed : other_speed >= speed)
        return slot * 7;
    node = r_s16(0x80098510 + car_offset);
    other_node = r_s16(0x80098510 + selected * 864);
    gap = node - other_node;
    if (mode != 0)
    {
        if (gap >= 25)
            return gap < 4;
        valid = gap < 4;
    }
    else
    {
        if (gap < -24)
            return gap < -3;
        valid = gap >= -3;
    }
    if (valid != 0 && r_s32(0x8009878C + car_offset) > 0)
    {
        w_u8(0x800987E5 + car_offset, 1);
        w_u16(0x800987B6 + car_offset, 0xFF00);
    }
    time = (sint32)td5_8004c868(owner, 0x80097750 + other_offset);
    speed = r_s32(0x8009878C + car_offset);
    quotient = speed / 1024;
    result = quotient < (sint32)((uint32)time - 8);
    if (result != 0 || time < 0)
        return result;
    if (speed <= 0)
        return 1;
    w_u8(0x800987E5 + car_offset, 1);
    w_u16(0x800987B6 + car_offset, 0xFF00);
    return 0xFFFFFF00;
}

uint32 td5_8004c868(uint32 owner, uint32 other)
{
    uint32 other_slot;
    uint32 own_slot;
    uint32 other_offset;
    uint32 own_offset;
    uint32 other_position;
    uint32 own_position;
    uint32 distance;
    uint32 scaled;
    uint32 numerator;
    uint32 own_speed;
    uint32 other_speed;
    sint32 other_node;
    sint32 own_node;
    sint32 quotient;

    FUNCTION_MARKER(0x8004C868, "SLUS_006.10");
    other_slot = r_u32(other + 0xD4);
    other_offset = r_u32(other + 0xDC);
    own_speed = r_u32(owner + 0x60);
    own_slot = r_u32(owner + 0xD4);
    other_speed = r_u32(other + 0x60);
    other_node = r_s16(0x80098510 + other_slot * 864);
    own_node = r_s16(0x80098510 + own_slot * 864);
    own_offset = r_u32(owner + 0xDC);
    other_position = ((uint32)other_node << 8) + other_offset;
    own_position = ((uint32)own_node << 8) + own_offset;
    if (own_speed == other_speed || own_speed == 0)
        return 192000;
    distance = other_position - own_position;
    scaled = distance * 1500;
    numerator = other_speed * scaled;
    quotient = td5_divide_checked((sint32)numerator, (sint32)(own_speed - other_speed));
    numerator = scaled + (uint32)quotient;
    quotient = td5_divide_checked((sint32)numerator, (sint32)own_speed);
    if (quotient < 0)
        quotient = (sint32)((uint32)quotient + 255);
    return (uint32)(quotient >> 8);
}

uint32 td5_8004e29c(uint32 slot)
{
    uint32 selected;
    uint32 result;
    uint32 car_offset;
    uint32 state_offset;
    uint32 route;
    uint32 tag;
    uint32 throttle;
    sint32 lane;
    sint32 speed;
    sint32 threshold;

    FUNCTION_MARKER(0x8004E29C, "SLUS_006.10");
    if (slot == 9)
    {
        selected = r_u32(0x800981C8);
        if (selected != 0xFFFFFFFF)
        {
            result = r_u32(0x800977D0 + selected * 284);
            if (result != 0)
                return result;
        }
    }
    car_offset = slot * 864;
    state_offset = slot * 284;
    lane = r_s16(0x80098512 + car_offset);
    route = r_u32(0x80097750 + state_offset);
    tag = r_u8(route + (uint32)lane * 3 + 2);
    throttle = r_u32(0x80097760 + state_offset);
    if (tag != 0)
    {
        speed = r_s32(0x800977B0 + state_offset);
        threshold = (sint32)((tag << 10) / 255);
        w_u16(0x800987B6 + car_offset, speed < threshold ? throttle : 0);
        w_u8(0x800987E5 + car_offset, 0);
        w_u8(0x800987E7 + car_offset, 0);
        return 0;
    }
    speed = r_s32(0x800977B0 + state_offset);
    if (speed >= 129 && r_s32(0x80098794 + car_offset) <= 65535)
    {
        w_u16(0x800987B6 + car_offset, 0xFF00);
        w_u8(0x800987E5 + car_offset, 1);
        w_u8(0x800987E7 + car_offset, 1);
        return 1;
    }
    w_u16(0x800987B6 + car_offset, throttle);
    w_u8(0x800987E5 + car_offset, 0);
    w_u8(0x800987E7 + car_offset, 0);
    return 0;
}

uint32 td5_8004d330(uint32 owner, uint32 step)
{
    uint32 car;
    uint32 square;
    uint32 value;
    uint32 delta;
    uint32 limit;
    sint32 yaw;
    sint32 speed;
    sint32 rate;
    sint32 first;
    sint32 second;
    sint32 angle;
    sint32 counter;
    sint32 product;
    uint32 subtract;

    FUNCTION_MARKER(0x8004D330, "SLUS_006.10");
    car = 0x800984A0 + r_u32(owner + 0xD4) * 864;
    yaw = r_s32(car + 0x2F8) >> 8;
    square = (uint32)yaw * (uint32)yaw;
    speed = r_s32(car + 0x2EC) >> 8;
    if (speed < 0)
        speed = -speed;
    first = td5_divide_checked((sint32)((uint32)speed << 10), (sint32)(square + 1024));
    second = td5_divide_checked((sint32)((uint32)speed << 16), (sint32)(square + 65536));
    rate = td5_divide_checked(0xC0000, (sint32)((uint32)first + 64));
    first = r_s32(owner + 0x58);
    second = (sint32)((uint32)second + 256);
    angle = r_s32(owner + 0x5C);
    limit = (uint32)td5_divide_checked(0x1800000, second);
    subtract = first >= 2048;
    if (subtract == 0)
        angle = first;
    if (angle >= 1025)
    {
        value = r_u32(car + 0x2E4);
        value = subtract != 0 ? value - 0x4000 : value + 0x4000;
        w_u32(car + 0x2E4, value);
    }
    else if (step != 0)
    {
        delta = step;
        if (angle < 256)
        {
            product = (sint32)(step * (uint32)rsin(angle));
            if (product < 0)
                product = (sint32)((uint32)product + 4095);
            delta = (uint32)(product >> 12);
        }
        value = r_u32(car + 0x2E4);
        value = subtract != 0 ? value - delta : value + delta;
        w_u32(car + 0x2E4, value);
    }
    else
    {
        counter = r_s16(car + 0x312);
        if (counter < 256)
            w_u16(car + 0x312, (uint32)counter + 64);
        counter = r_s16(car + 0x312);
        product = (sint32)((uint32)rate * (uint32)counter);
        if (product < 0)
            product = (sint32)((uint32)product + 255);
        value = r_u32(car + 0x2E4);
        delta = (uint32)(product >> 8);
        value = subtract != 0 ? value - delta : value + delta;
        w_u32(car + 0x2E4, value);
        if (subtract != 0)
        {
            limit = 0 - limit;
            if ((sint32)value < (sint32)limit)
                w_u32(car + 0x2E4, limit);
        }
        else if ((sint32)limit < (sint32)value)
            w_u32(car + 0x2E4, limit);
    }
    value = r_u32(car + 0x2E4);
    if ((sint32)value > 98304)
    {
        w_u32(car + 0x2E4, 98304);
        return 1;
    }
    if ((sint32)value < -98304)
    {
        w_u32(car + 0x2E4, (uint32)-98304);
        return 1;
    }
    return 0;
}

static uint32 td5_ai_candidate_interval(uint32 owner, uint32 index, uint32 vector_address, sint32 *gap, uint32 *last_offset)
{
    uint32 state_offset;
    uint32 car_offset;
    uint32 route;
    uint32 candidate_route;
    uint32 primary;
    uint32 value;
    uint32 first;
    uint32 second;
    uint32 own;
    uint32 model;
    uint32 edge;
    uint32 tag;
    sint32 lane;
    sint32 node;
    sint32 bound;
    sint32 candidate;
    sint32 own_lane;

    state_offset = index * 284;
    car_offset = index * 864;
    route = r_u32(owner);
    candidate_route = r_u32(0x80097750 + state_offset);
    if (route != candidate_route)
    {
        primary = r_u32(0x800886B8);
        if (route == primary)
        {
            value = r_u32(owner + 0x3C);
            route = r_u32(0x800887A8);
            w_u32(owner + 0x24, value);
            w_u32(owner, route);
        }
        else
        {
            value = r_u32(owner + 0x38);
            w_u32(owner, primary);
            w_u32(owner + 0x24, value);
        }
    }
    route = r_u32(owner);
    candidate_route = r_u32(0x800887A8);
    if (route == candidate_route)
    {
        first = r_u32(owner + 0x48);
        second = r_u32(owner + 0x4C);
    }
    else
    {
        first = r_u32(owner + 0x40);
        second = r_u32(owner + 0x44);
    }
    w_u32(owner + 0x54, first);
    w_u32(owner + 0x50, second);
    value = r_u32(owner + 0x24);
    edge = td5_8004fce4(index, value, vector_address);
    if (edge == 1 || edge == 2)
    {
        lane = r_s16(0x80098512 + car_offset);
        route = r_u32(0x80097750 + state_offset);
        node = r_s16(0x80098510 + car_offset);
        tag = r_u8(route + (uint32)lane * 3);
        value = td5_8004dbe8(node, edge == 1 ? 256 : 0, tag);
        own = r_u32(owner + 0xD4);
        model = r_u32(0x80098620 + own * 864);
        bound = r_s16(model + (edge == 1 ? 0 : 8));
        *last_offset = value + (uint32)bound - 32;
    }
    else
        *last_offset = r_u32(owner + 0x24);
    own = r_u32(owner + 0xD4);
    car_offset = own * 864;
    model = r_u32(0x80098620 + car_offset);
    bound = r_s16(model);
    candidate = r_s32(0x800977A0 + state_offset);
    if (candidate < (sint32)((uint32)bound + *last_offset - 32))
        return 0;
    bound = r_s16(model + 8);
    candidate = r_s32(0x800977A4 + state_offset);
    if ((sint32)((uint32)bound + *last_offset + 32) < candidate)
        return 0;
    lane = r_s16(0x80098512 + index * 864);
    own_lane = r_s16(0x80098512 + car_offset);
    *gap = lane - own_lane;
    return 1;
}

static uint32 td5_ai_finish_selection(uint32 selected, uint32 own, uint32 last_offset)
{
    uint32 first;
    uint32 second;
    uint32 model;
    uint32 midpoint;
    uint32 lower;
    uint32 upper;
    uint32 lower_distance;
    uint32 upper_distance;

    first = r_u32(0x800977A0 + selected * 284);
    second = r_u32(0x800977A4 + selected * 284);
    model = r_u32(0x80098620 + own * 864);
    lower = (uint32)r_s16(model);
    midpoint = (uint32)((sint32)(first + second) / 2);
    lower += last_offset - 32;
    lower_distance = lower - midpoint;
    if ((sint32)lower_distance < 0)
        lower_distance = midpoint - lower;
    upper = (uint32)r_s16(model + 8);
    upper += last_offset + 32;
    upper_distance = upper - midpoint;
    if ((sint32)upper_distance < 0)
        upper_distance = midpoint - upper;
    w_u32(0x800885AC, (sint32)lower_distance < (sint32)upper_distance ? 0 : 1);
    return selected;
}

uint32 td5_8004c0a4(uint32 owner, uint32 vector_address)
{
    uint32 count;
    uint32 selected;
    uint32 index;
    uint32 own;
    uint32 first;
    uint32 second;
    uint32 state_offset;
    uint32 last_offset;
    sint32 best_gap;
    sint32 gap;
    sint32 lane;
    sint32 candidate_lane;
    sint32 speed;
    sint32 candidate_speed;

    FUNCTION_MARKER(0x8004C0A4, "SLUS_006.10");
    count = r_u32(0x800875E4);
    selected = r_u32(owner + 0xD4);
    best_gap = 192000;
    index = 6;
    if ((sint32)count >= 6 && (sint32)index < (sint32)count)
    {
        do
        {
            state_offset = index * 284;
            first = r_u32(owner + 12);
            second = r_u32(0x8009775C + state_offset);
            if (first == second)
            {
                own = r_u32(owner + 0xD4);
                lane = r_s16(0x80098512 + own * 864);
                candidate_lane = r_s16(0x80098512 + index * 864);
                if (candidate_lane >= lane)
                {
                    if (td5_ai_candidate_interval(owner, index, vector_address, &gap, &last_offset) != 0 && gap < best_gap)
                    {
                        best_gap = gap;
                        selected = index;
                    }
                }
            }
            count = r_u32(0x800875E4);
            ++index;
        } while ((sint32)index < (sint32)count);
    }
    if ((uint32)(best_gap - 1) < 39)
    {
        own = r_u32(owner + 0xD4);
        if (selected != own)
            return td5_ai_finish_selection(selected, own, last_offset);
    }
    selected = r_u32(owner + 0xD4);
    best_gap = 192000;
    index = 0;
    for (;;)
    {
        count = r_u32(0x800875E4);
        if ((sint32)count >= 7)
            count = 6;
        if ((sint32)index >= (sint32)count)
            break;
        own = r_u32(owner + 0xD4);
        if (index != own)
        {
            state_offset = index * 284;
            first = r_u32(owner + 12);
            second = r_u32(0x8009775C + state_offset);
            if (first == second)
            {
                lane = r_s16(0x80098512 + own * 864);
                candidate_lane = r_s16(0x80098512 + index * 864);
                if (candidate_lane >= lane)
                {
                    candidate_speed = r_s32(0x800977B0 + state_offset);
                    speed = r_s32(owner + 0x60);
                    if (speed >= (sint32)((uint32)candidate_speed - 16))
                    {
                        if (td5_ai_candidate_interval(owner, index, vector_address, &gap, &last_offset) != 0 && gap < best_gap)
                        {
                            best_gap = gap;
                            selected = index;
                        }
                    }
                }
            }
        }
        ++index;
    }
    if ((uint32)best_gap < 41)
    {
        own = r_u32(owner + 0xD4);
        if (selected != own)
            return td5_ai_finish_selection(selected, own, last_offset);
    }
    return r_u32(owner + 0xD4);
}

uint32 td5_8004d268(uint32 owner, uint32 point)
{
    uint32 slot;
    uint32 car;
    uint32 coordinate;
    uint32 position;
    uint32 angle;
    uint32 steering;
    uint32 result;
    sint32 x;
    sint32 z;
    sint32 difference;
    sint32 magnitude;

    FUNCTION_MARKER(0x8004D268, "SLUS_006.10");
    slot = r_u32(owner + 0xD4);
    coordinate = r_u32(point);
    car = 0x800984A0 + slot * 864;
    position = r_u32(car + 0x1C4);
    x = (sint32)(coordinate - position) >> 8;
    coordinate = r_u32(point + 8);
    position = r_u32(car + 0x1CC);
    z = (sint32)(coordinate - position) >> 8;
    angle = (uint32)td5_8004cf8c(x, z);
    position = r_u32(car + 0x1BC);
    steering = r_u32(car + 0x2E4);
    difference = (sint32)(((uint32)((sint32)(position + steering) >> 8) & 0xFFF) - (angle & 0xFFF));
    magnitude = difference < 0 ? -difference : difference;
    result = 4095 - (uint32)magnitude;
    if (difference < 0)
    {
        result = (uint32)magnitude;
        difference += 4095;
    }
    w_u32(owner + 0x58, result);
    w_u32(owner + 0x5C, (uint32)difference);
    return result;
}
