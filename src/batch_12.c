#include "td5_functions.h"
#include <string.h>

static uint32 td5_2a244_div(uint32 value, sint32 divisor)
{
    return (uint32)td5_divide_checked((sint32)value, divisor);
}

uint32 td5_8002a244(uint32 car, uint32 other, uint32 contact, sint32 heading, uint32 fraction, uint32 original_stack)
{
    uint32 frame = original_stack - 0xA0;
    uint32 yaw_a, yaw_b, cosine, sine, vx, vz, ax, az, bx, bz;
    uint32 model, model_b, x, z, offset_x, offset_z, rear, width, front;
    uint32 distance_x, distance_z, arm_a, arm_b, shift_x, shift_z;
    uint32 inertia, coefficient_a, coefficient_b, denominator, impulse;
    uint32 result, value, second, rotation_a, rotation_b, magnitude, tick;
    uint32 factor, index, owner, slot, angles, range, half, random;
    uint32 owners[6], velocities[6], positions[6];
    int side_hit;

    FUNCTION_MARKER(0x8002A244, "SLUS_006.10");
    yaw_a = r_u32(car + 0x18C);
    yaw_b = r_u32(other + 0x18C);
    cosine = (uint32)rcos(heading);
    sine = (uint32)rsin(heading);
    vx = r_u32(car + 0x194);
    vz = r_u32(car + 0x19C);
    ax = td5_2a244_div(vx * cosine - vz * sine, 4096);
    az = td5_2a244_div(vx * sine + vz * cosine, 4096);
    vx = r_u32(other + 0x194);
    vz = r_u32(other + 0x19C);
    bx = td5_2a244_div(vx * cosine - vz * sine, 4096);
    bz = td5_2a244_div(vx * sine + vz * cosine, 4096);
    model = r_u32(car + 0x180);
    x = (uint32)r_s16(contact);
    z = (uint32)r_s16(contact + 2);
    offset_x = (uint32)r_s16(contact + 4);
    rear = (uint32)r_s16(model + 20);
    offset_z = (uint32)r_s16(contact + 6);
    width = (uint32)r_s16(model + 8);
    front = (uint32)r_s16(model + 4);
    value = (sint32)x < 0 ? 0u - x : x;
    distance_x = width - value;
    if ((sint32)distance_x < 0)
        distance_x = value - width;
    distance_z = (sint32)z > 0 ? front - z : z - rear;
    if ((sint32)distance_z < 0)
        distance_z = 0u - distance_z;
    side_hit = (sint32)distance_x < (sint32)distance_z;
    if (side_hit)
    {
        int positive = (sint32)(x - offset_x) > 0;
        shift_x = td5_2a244_div(positive ? cosine : 0u - cosine, 2);
        shift_z = td5_2a244_div(positive ? 0u - sine : sine, 2);
        arm_a = z;
        arm_b = offset_z;
    }
    else
    {
        int positive = (sint32)(z - offset_z) > 0;
        shift_x = td5_2a244_div(positive ? sine : 0u - sine, 2);
        shift_z = td5_2a244_div(positive ? cosine : 0u - cosine, 2);
        arm_a = x;
        arm_b = offset_x;
    }
    value = r_u32(car + 0x1C4);
    second = r_u32(car + 0x1CC);
    w_u32(car + 0x1C4, value - shift_x);
    w_u32(car + 0x1CC, second - shift_z);
    value = r_u32(other + 0x1C4);
    second = r_u32(other + 0x1CC);
    inertia = r_u32(0x80087830);
    w_u32(other + 0x1C4, value + shift_x);
    w_u32(other + 0x1CC, second + shift_z);
    model = r_u32(car + 0x180);
    model_b = r_u32(other + 0x180);
    coefficient_a = (uint32)r_s16(model + 0x88);
    coefficient_b = (uint32)r_s16(model_b + 0x88);
    value = (inertia + arm_b * arm_b) * coefficient_a;
    second = (inertia + arm_a * arm_a) * coefficient_b;
    denominator = td5_2a244_div(value + second, 256);
    value = td5_2a244_div(inertia, 256) * 4352;
    impulse = td5_2a244_div(value, (sint32)denominator);
    value = side_hit ? bx : bz;
    value += td5_2a244_div(yaw_b * arm_b, 652);
    value -= side_hit ? ax : az;
    value -= td5_2a244_div(yaw_a * arm_a, 652);
    impulse = td5_2a244_div(impulse * value, 4096);
    result = impulse ^ (side_hit ? offset_x - x : offset_z - z);
    if ((sint32)result < 0)
        return result;
    denominator = td5_2a244_div(inertia, 652);
    rotation_a = td5_2a244_div(impulse * arm_a * coefficient_a, (sint32)denominator);
    rotation_b = td5_2a244_div(impulse * arm_b * coefficient_b, (sint32)denominator);
    if (side_hit)
    {
        ax += impulse * coefficient_a;
        bx -= impulse * coefficient_b;
        yaw_a += rotation_a;
        yaw_b -= rotation_b;
    }
    else
    {
        az += impulse * coefficient_a;
        bz -= impulse * coefficient_b;
        yaw_a -= rotation_a;
        yaw_b += rotation_b;
    }
    magnitude = impulse * (coefficient_a + coefficient_b);
    if ((sint32)magnitude < 0)
        magnitude = 0u - magnitude;

    owners[0] = car;
    owners[1] = car;
    owners[2] = other;
    owners[3] = other;
    owners[4] = car;
    owners[5] = other;
    velocities[0] = 0x194;
    velocities[1] = 0x19C;
    velocities[2] = 0x194;
    velocities[3] = 0x19C;
    velocities[4] = 0x18C;
    velocities[5] = 0x18C;
    positions[0] = 0x1C4;
    positions[1] = 0x1CC;
    positions[2] = 0x1C4;
    positions[3] = 0x1CC;
    positions[4] = 0x1BC;
    positions[5] = 0x1BC;
    factor = 256u - fraction;
    for (index = 0; index < 6; ++index)
    {
        owner = owners[index];
        value = td5_2a244_div(factor * r_u32(owner + velocities[index]), 256);
        if (index == 0 || index == 5)
            tick = r_u32(0x80088100);
        value = td5_2a244_div(value * tick, 256);
        second = r_u32(owner + positions[index]);
        w_u32(owner + positions[index], second - value);
    }
    w_u32(car + 0x18C, yaw_a);
    w_u32(other + 0x18C, yaw_b);
    w_u32(car + 0x194, td5_2a244_div(ax * cosine + az * sine, 4096));
    w_u32(car + 0x19C, td5_2a244_div((0u - ax) * sine + az * cosine, 4096));
    w_u32(other + 0x194, td5_2a244_div(bx * cosine + bz * sine, 4096));
    w_u32(other + 0x19C, td5_2a244_div((0u - bx) * sine + bz * cosine, 4096));
    for (index = 0; index < 6; ++index)
    {
        owner = owners[index];
        value = td5_2a244_div(factor * r_u32(owner + velocities[index]), 256);
        if (index == 2)
            tick = r_u32(0x80088100);
        value = td5_2a244_div(value * tick, 256);
        second = r_u32(owner + positions[index]);
        w_u32(owner + positions[index], second + value);
    }
    if (r_u8(car + 0x34D) < 6)
        td5_80024938(car, frame);
    if (r_u8(other + 0x34D) < 6)
        td5_80024938(other, frame);
    slot = r_u8(car + 0x34D);
    if (slot == r_u32(0x800879B8))
    {
        slot = r_u8(other + 0x34D);
        if (slot < 6 && (r_u32(0x80087724) != 0 || r_u32(0x8008771C) == 3))
            td5_800437ac(slot, (sint32)magnitude);
    }
    slot = r_u8(other + 0x34D);
    if (slot == r_u32(0x800879B8))
    {
        slot = r_u8(car + 0x34D);
        if (slot >= 6)
            goto increment_gate;
        if (r_u32(0x80087724) != 0 || r_u32(0x8008771C) == 3)
            td5_800437ac(slot, (sint32)magnitude);
    }
    if (r_u8(car + 0x34D) < 6)
        goto damage;
increment_gate:
    if ((sint32)magnitude > 50000)
    {
        slot = r_u8(car + 0x34D);
        owner = 0x800977D8 + slot * 284;
        value = r_u32(owner);
        if (value - 1 < 6)
            w_u32(owner, value + 1);
    }
damage:
    for (index = 0; index < 2; ++index)
    {
        owner = index == 0 ? car : other;
        slot = r_u8(owner + 0x34D);
        if ((sint32)slot <= r_s32(0x8008772C) && (sint32)magnitude >= 12801)
        {
            int severe = (sint32)magnitude > 51200;
            td5_800415a4((sint32)slot, severe ? 1024 : 2048);
            td5_80049310(owner, severe ? 2 : 1);
            td5_play_positioned_sound(severe ? 5 : 6, -1, 4096, severe ? 1600 : 600, owner + 0x1C4, 0);
        }
    }
    result = (sint32)magnitude > 90000;
    if (result == 0)
        return result;
    result = r_u32(0x80088700);
    if (result != 0)
        return result;
    range = td5_2a244_div(magnitude, 4);
    if ((sint32)range < 32767)
        range = 32767;
    for (index = 0; index < 2; ++index)
    {
        owner = index == 0 ? car : other;
        slot = r_u8(owner + 0x34D);
        if (slot < 6)
        {
            half = td5_2a244_div(range, 2);
            random = td5_random_for_slot(slot) % range;
            value = r_u32(owner + 0x188);
            w_u32(owner + 0x188, value + random - half);
            random = td5_random_for_slot(slot) % range;
            value = r_u32(owner + 0x18C);
            w_u32(owner + 0x18C, value + random - half);
            random = td5_random_for_slot(slot) % (range * 2);
            value = r_u32(owner + 0x190);
            result = td5_2a244_div(magnitude, 6);
            w_u32(owner + 0x198, result);
            w_u32(owner + 0x190, value + random - range);
        }
        else
        {
            second = td5_2a244_div(range, 256);
            half = second / 2;
            angles = frame + 0x48 + index * 8;
            random = td5_random_for_slot(slot) % second;
            w_u16(angles, random - half);
            slot = r_u8(owner + 0x34D);
            random = td5_random_for_slot(slot) % second;
            w_u16(angles + 2, random - half);
            slot = r_u8(owner + 0x34D);
            random = td5_random_for_slot(slot) % second;
            w_u16(angles + 4, random - half);
            td5_80079548(frame + 0x20, angles);
            td5_80077970(owner + 0x154, frame + 0x20);
            td5_80077970(owner + 0x130, owner + 0x110);
            w_u8(owner + 0x351, 1);
            w_u16(owner + 0x310, 0);
            result = td5_2a244_div(magnitude, 6);
            w_u32(owner + 0x198, result);
        }
    }
    return result;
}

uint32 td5_8003c69c(uint32 point, uint32 car)
{
    uint32 speed, effect, packet, record, source, destination, copy, index;
    uint32 words[4];
    sint32 random, result;

    FUNCTION_MARKER(0x8003C69C, "SLUS_006.10");
    speed = r_u32(car + 0x2EC);
    if ((sint32)speed < 0)
        speed = 0u - speed;
    if ((sint32)speed <= 65534)
        return 0;
    for (effect = 0; effect < 50; ++effect)
        if (r_u8(0x8009EFCF + effect * 68) == 0)
            break;
    if (effect == 50)
        return 1;
    for (packet = 0; packet < 20; ++packet)
        if (r_s8(0x8009D800 + packet) == 0)
            break;
    if (packet == 20)
        return 1;
    w_u8(0x8009D800 + packet, 1);
    record = 0x8009EFB0 + effect * 68;
    w_u8(record, packet);
    random = randPSX();
    source = 0x8009D860 + (uint32)(random % 4) * 20;
    for (copy = 0; copy < 2; ++copy)
    {
        destination = (copy == 0 ? 0x8009EC70 : 0x8009EE00) + packet * 20;
        for (index = 0; index < 4; ++index)
            words[index] = r_u32(source + index * 4);
        for (index = 0; index < 4; ++index)
            w_u32(destination + index * 4, words[index]);
        w_u32(destination + 16, r_u32(source + 16));
    }
    w_u8(record + 31, 0x82);
    w_u32(record + 64, 0x8003C190);
    w_u32(record + 60, 0x8003C010);
    w_u32(record + 32, r_u32(point));
    w_u32(record + 36, r_u32(point + 4) + 30720);
    w_u32(record + 40, r_u32(point + 8));
    w_u32(record + 8, r_u32(car + 0x194));
    w_u32(record + 16, r_u32(car + 0x19C));
    random = randPSX();
    w_u32(record + 12, (uint32)(random % 7000) + 4000);
    result = randPSX() % 2;
    w_u8(record + 1, (uint32)result * 15);
    return (uint32)result;
}

uint32 td5_800287b8(uint32 car, uint32 point, sint32 angle, sint32 distance)
{
    uint32 cosine, sine, separation, shift_x, shift_z, vx, vz, px, pz, yaw;
    uint32 sum, rounded_sum, normal_x, normal_z, dx, dz, arm, result, approach;
    uint32 inertia, value, denominator, impulse, new_yaw, new_normal, friction;
    uint32 volume, slot;

    FUNCTION_MARKER(0x800287B8, "SLUS_006.10");
    cosine = (uint32)rcos(angle);
    sine = (uint32)rsin(angle);
    separation = (uint32)distance - 4;
    shift_x = (uint32)td5_divide_checked((sint32)((0u - separation) * sine), 16);
    shift_z = (uint32)td5_divide_checked((sint32)(separation * cosine), 16);
    vx = r_u32(car + 0x194);
    vz = r_u32(car + 0x19C);
    px = r_u32(car + 0x1C4);
    pz = r_u32(car + 0x1CC);
    yaw = r_u32(car + 0x18C);
    w_u32(car + 0x1C4, px + shift_x);
    pz += shift_z;
    w_u32(car + 0x1CC, pz);
    sum = vx * cosine + vz * sine;
    rounded_sum = (sint32)sum < 0 ? sum + 4095 : sum;
    normal_x = (uint32)td5_divide_checked((sint32)sum, 4096);
    normal_z = (uint32)td5_divide_checked((sint32)(vz * cosine - vx * sine), 4096);
    px = r_u32(car + 0x1C4);
    value = r_u32(point);
    dx = (uint32)((sint32)(px - value) >> 8);
    value = r_u32(point + 8);
    dz = (uint32)((sint32)(pz - value) >> 8);
    arm = (uint32)td5_divide_checked((sint32)(dx * cosine + dz * sine), 4096);
    result = (uint32)td5_divide_checked((sint32)yaw, 652);
    approach = normal_z + arm * result;
    if ((sint32)approach < 0)
        return result;
    inertia = r_u32(0x8008782C);
    value = (uint32)td5_divide_checked((sint32)inertia, 256) * 4352;
    value = 0u - (uint32)td5_divide_checked((sint32)value, 4096);
    value *= approach;
    denominator = (uint32)td5_divide_checked((sint32)(inertia + arm * arm), 256);
    impulse = (uint32)td5_divide_checked((sint32)value, (sint32)denominator);
    denominator = (uint32)td5_divide_checked((sint32)inertia, 652);
    new_yaw = yaw + (uint32)td5_divide_checked((sint32)(impulse * arm), (sint32)denominator);
    new_normal = normal_z + impulse;
    if ((sint32)normal_x > 0)
    {
        value = approach * 2 + (uint32)((sint32)rounded_sum >> 18) + 2048;
        friction = (uint32)td5_divide_checked((sint32)(value * 384), 2048);
        friction = normal_x - friction;
        if ((sint32)friction < 0)
            friction = 0;
    }
    else
    {
        value = (uint32)td5_divide_checked((sint32)normal_x, 64) - 2048;
        value = approach * 2 - value;
        friction = (uint32)td5_divide_checked((sint32)(value * 384), 2048);
        friction += normal_x;
        if ((sint32)friction > 0)
            friction = 0;
    }
    w_u32(car + 0x18C, new_yaw);
    value = friction * cosine - new_normal * sine;
    w_u32(car + 0x194, (uint32)td5_divide_checked((sint32)value, 4096));
    value = new_normal * cosine + friction * sine;
    w_u32(car + 0x19C, (uint32)td5_divide_checked((sint32)value, 4096));
    if ((sint32)normal_z >= 12801)
    {
        volume = normal_z - 8192;
        if ((sint32)volume < 1024)
            volume = 1024;
        else if ((sint32)volume >= 2049)
            volume = 2048;
        td5_play_positioned_sound((sint32)normal_z > 102400 ? 5 : 7, -1, (sint32)volume, (sint32)normal_z > 102400 ? 1600 : 4096, car + 0x1C4, 0);
        td5_80049310(car, 1);
    }
    value = (sint32)normal_x < 0 ? 0u - normal_x : normal_x;
    result = (sint32)normal_z < 12801;
    if ((sint32)value >= 12801 || (sint32)normal_z >= 12801)
    {
        slot = r_u8(car + 0x34D);
        result = r_s32(0x8008772C) < (sint32)slot;
        if (result == 0)
            return td5_800415a4((sint32)slot, 2048);
    }
    return result;
}

uint32 td5_8002c074(uint32 first, uint32 second)
{
    uint32 car_a = 0x800984A0 + first * 864;
    uint32 car_b = 0x800984A0 + second * 864;
    SVECTOR direction = {0};
    sint16 components[3], relative[3];
    uint32 index, value, previous, squared = 0, result, model_a, model_b;
    sint32 radius_a, radius_b, approach;

    FUNCTION_MARKER(0x8002C074, "SLUS_006.10");
    for (index = 0; index < 3; ++index)
    {
        value = r_u32(car_b + 0x1C4 + index * 4);
        previous = r_u32(car_a + 0x1C4 + index * 4);
        components[index] = (sint16)((sint32)(value - previous) >> 8);
        squared += (uint32)((sint32)components[index] * components[index]);
    }
    direction.vx = components[0];
    direction.vy = components[1];
    direction.vz = components[2];
    value = SquareRoot0((sint32)squared);
    model_a = r_u32(car_a + 0x180);
    model_b = r_u32(car_b + 0x180);
    radius_a = r_s16(model_a + 0x80);
    radius_b = r_s16(model_b + 0x80);
    result = (sint32)value < 3 * (radius_a + radius_b) / 4;
    if (result == 0)
        return result;
    VectorNormalSS(&direction, &direction);
    components[0] = direction.vx;
    components[1] = direction.vy;
    components[2] = direction.vz;
    result = 0;
    for (index = 0; index < 3; ++index)
    {
        value = r_u32(car_b + 0x194 + index * 4);
        previous = r_u32(car_a + 0x194 + index * 4);
        relative[index] = (sint16)((sint32)(value - previous) >> 8);
        result += (uint32)((sint32)components[index] * relative[index]);
    }
    approach = (sint32)result >> 12;
    if (approach >= 0)
        return result;
    for (index = 0; index < 6; ++index)
    {
        uint32 owner = index < 3 ? car_a : car_b;
        uint32 axis = index % 3;
        value = (uint32)(sint32)components[axis] * (uint32)approach;
        previous = r_u32(owner + 0x194 + axis * 4);
        result = (uint32)((sint32)(value + (value >> 31)) >> 5);
        w_u32(owner + 0x194 + axis * 4, index < 3 ? previous + result : previous - result);
    }
    return result;
}

uint32 td5_8002d6c4(uint32 car, uint32 original_stack)
{
    uint32 frame = original_stack - 0x50;
    uint32 matrix = frame + 0x18;
    uint32 angles = frame + 0x10;
    uint32 result, value;

    FUNCTION_MARKER(0x8002D6C4, "SLUS_006.10");
    td5_80079720(car + 0x110, car + 0x1D0);
    td5_80077a04(matrix, car + 0x154);
    td5_80079720(matrix, angles);
    w_u32(car + 0x188, (uint32)r_s16(angles));
    w_u32(car + 0x18C, (uint32)r_s16(angles + 2));
    result = r_u32(0x800885E8);
    value = (uint32)r_s16(angles + 4);
    result &= 3;
    w_u32(car + 0x190, value);
    if (result != 0)
        return result;
    td5_80079548(car + 0x110, car + 0x1D0);
    td5_80077970(car + 0x130, car + 0x110);
    td5_80079548(matrix, angles);
    return td5_80077970(car + 0x154, matrix);
}

uint32 td5_80077a04(uint32 destination, uint32 source)
{
    uint32 index, value;
    uint32 result;

    FUNCTION_MARKER(0x80077A04, "SLUS_006.10");
    for (index = 0; index < 9; ++index)
    {
        value = r_u32(source + index * 4);
        result = (uint32)((sint32)(value + 0x20000) >> 18);
        w_u16(destination + index * 2, result);
    }
    return result;
}

uint32 td5_80079720(uint32 matrix, uint32 angles)
{
    sint32 first, second;
    uint32 squared, root, result;

    FUNCTION_MARKER(0x80079720, "SLUS_006.10");
    first = r_s16(matrix + 6);
    squared = (uint32)(first * first);
    second = r_s16(matrix + 8);
    squared += (uint32)(second * second);
    root = td5_integer_sqrt(squared);
    first = -r_s16(matrix + 10);
    result = (uint32)td5_8007b0c0(first, (sint32)root);
    w_u16(angles, result);
    if ((result & 0x7FF) == 0x400)
    {
        first = -r_s16(matrix + 12);
        second = r_s16(matrix);
        result = (uint32)td5_8007b0c0(first, second);
        w_u16(angles + 2, result);
        w_u16(angles + 4, 0);
    }
    else
    {
        first = r_s16(matrix + 4);
        second = r_s16(matrix + 16);
        result = (uint32)td5_8007b0c0(first, second);
        w_u16(angles + 2, result);
        first = r_s16(matrix + 6);
        second = r_s16(matrix + 8);
        result = (uint32)td5_8007b0c0(first, second);
        w_u16(angles + 4, result);
    }
    return result;
}

uint32 td5_8002d824(uint32 car, uint32 original_stack)
{
    uint32 frame = original_stack - 0x48;
    uint32 matrix = frame + 0x10;
    uint32 vx, vy, vz, px, py, pz, gravity, value, result, group, index;
    uint32 words[4];

    FUNCTION_MARKER(0x8002D824, "SLUS_006.10");
    vx = r_u32(car + 0x194);
    px = r_u32(car + 0x1C4);
    vy = r_u32(car + 0x198);
    vx -= (uint32)((sint32)vx / 256);
    w_u32(car + 0x194, vx);
    w_u32(car + 0x1C4, px + vx);
    vz = r_u32(car + 0x19C);
    gravity = r_u32(0x800877A4);
    py = r_u32(car + 0x1C8);
    vy -= (uint32)((sint32)vy / 256) + gravity;
    w_u32(car + 0x198, vy);
    w_u32(car + 0x1C8, py + vy);
    pz = r_u32(car + 0x1CC);
    vz -= (uint32)((sint32)vz / 256);
    w_u32(car + 0x19C, vz);
    w_u32(car + 0x1CC, pz + vz);
    td5_80077abc(car + 0x154, car + 0x130, matrix);
    for (group = 0; group < 2; ++group)
    {
        for (index = 0; index < 4; ++index)
            words[index] = r_u32(matrix + group * 16 + index * 4);
        for (index = 0; index < 4; ++index)
            w_u32(car + 0x130 + group * 16 + index * 4, words[index]);
    }
    w_u32(car + 0x150, r_u32(matrix + 32));
    td5_80077a04(car + 0x110, matrix);
    td5_8001b310(car + 0x70, car + 0x1C4);
    td5_8001d314(car + 0x70, car + 0x1C4, car + 0x258, (uint32 *)psx_addr(car + 0x2B0, 4));
    px = r_u32(car + 0x1C4);
    py = r_u32(car + 0x1C8);
    pz = r_u32(car + 0x1CC);
    w_u32(car + 0x124, (uint32)((sint32)px >> 8));
    w_u32(car + 0x128, (uint32)((sint32)py >> 8));
    w_u32(car + 0x12C, (uint32)((sint32)pz >> 8));
    value = td5_8002c770(car);
    if (value != 0)
        td5_8002cec4(car, value, frame);
    result = r_s16(car + 0x310) < 60;
    if (result == 0)
        return td5_80024134(car, frame - 0xC0);
    return result;
}

uint32 td5_8007a340(uint32 first, uint32 second)
{
    uint64 product;

    FUNCTION_MARKER(0x8007A340, "SLUS_006.10");
    product = (uint64)((sint64)(sint32)first * (sint32)second) + 0x40000000u;
    return (uint32)(product >> 30);
}

uint32 td5_80077abc(uint32 first, uint32 second, uint32 output)
{
    uint32 row, column, term, left, right, sum, result;

    FUNCTION_MARKER(0x80077ABC, "SLUS_006.10");
    for (row = 0; row < 3; ++row)
    {
        for (column = 0; column < 3; ++column)
        {
            sum = 0;
            for (term = 0; term < 3; ++term)
            {
                left = r_u32(first + row * 12 + term * 4);
                right = r_u32(second + term * 12 + column * 4);
                result = td5_8007a340(left, right);
                sum += result;
            }
            w_u32(output + row * 12 + column * 4, sum);
        }
    }
    return result;
}

uint32 td5_8002c770(uint32 car)
{
    uint32 model, saved_node, saved_lane, index, node, world, vector, transformed;
    uint32 current_model, suspension, value, coordinates[3], axis, ground;
    uint32 mask = 0;

    FUNCTION_MARKER(0x8002C770, "SLUS_006.10");
    model = r_u32(car + 0x180);
    saved_node = (uint32)r_s16(car + 0x70);
    saved_lane = r_u8(car + 0x7C);
    SetRotMatrix((MATRIX *)psx_addr(car + 0x110, 20));
    SetTransMatrix((MATRIX *)psx_addr(car + 0x110, 32));
    for (index = 0; index < 8; ++index)
    {
        node = car + index * 14;
        world = car + 0x80 + index * 12;
        vector = model + index * 8;
        w_u16(node, saved_node);
        w_u8(node + 12, saved_lane);
        xport_gte_write_data(0, r_u32(vector));
        xport_gte_write_data(1, r_u32(vector + 4));
        xport_gte_execute(0x480012);
        w_u32(world, xport_gte_read_data(25));
        w_u32(world + 4, xport_gte_read_data(26));
        w_u32(world + 8, xport_gte_read_data(27));
        if (index < 4)
        {
            current_model = r_u32(car + 0x180);
            suspension = (uint32)(r_s32(car + 0x2B4 + index * 4) / 256);
            value = r_u16(current_model + 0x42 + index * 8);
            vector = car + 0x1D8 + index * 8;
            w_u16(vector + 2, value - suspension);
            transformed = car + 0x260 + index * 16;
            xport_gte_write_data(0, r_u32(vector));
            xport_gte_write_data(1, r_u32(vector + 4));
            xport_gte_execute(0x480012);
            w_u32(transformed, xport_gte_read_data(25));
            w_u32(transformed + 4, xport_gte_read_data(26));
            w_u32(transformed + 8, xport_gte_read_data(27));
            for (axis = 0; axis < 3; ++axis)
            {
                value = r_u32(transformed + axis * 4);
                w_u32(car + 0xE0 + index * 12 + axis * 4, value << 8);
            }
        }
        coordinates[0] = r_u32(world);
        coordinates[1] = r_u32(world + 4);
        coordinates[2] = r_u32(world + 8);
        w_u32(world, coordinates[0] << 8);
        w_u32(world + 4, coordinates[1] << 8);
        w_u32(world + 8, coordinates[2] << 8);
        td5_8001b310(node, world);
        ground = 0x8009DBB0 + index * 4;
        td5_8001cd00(node, world, ground);
        value = r_u32(world + 4);
        ground = r_u32(ground);
        if ((sint32)(value - ground) < 0)
            mask |= 1u << index;
    }
    return mask;
}

uint32 td5_8002cec4(uint32 car, uint32 contact, uint32 original_stack)
{
    uint32 frame = original_stack - 0x120;
    uint32 average = frame + 0x20, velocity = frame + 0x30;
    uint32 cross = frame + 0x48, basis = frame + 0x58;
    uint32 rotation = frame + 0x80, combined = frame + 0xA8;
    uint32 count = 0, lift = 0, index, axis, value, other, accumulated;
    uint32 ax, ay, az, bx, bz, length, impact, restitution, angle, cosine, sine;
    uint32 x_axis, z_axis, tangent_length, limit, original_y, source, destination;
    uint32 words[4], group, result;

    FUNCTION_MARKER(0x8002CEC4, "SLUS_006.10");
    w_u32(average, 0);
    w_u32(average + 4, 0);
    w_u32(average + 8, 0);
    for (index = 0; index < 8; ++index)
    {
        if ((contact & (1u << index)) == 0)
            continue;
        for (axis = 0; axis < 3; ++axis)
        {
            value = r_u32(car + 0x80 + index * 12 + axis * 4);
            other = r_u32(car + 0x1C4 + axis * 4);
            accumulated = r_u32(average + axis * 4);
            value = (uint32)((sint32)(value - other) >> 4);
            w_u32(average + axis * 4, accumulated + value);
        }
        value = r_u32(0x8009DBB0 + index * 4);
        other = r_u32(car + 0x84 + index * 12);
        value -= other;
        if ((sint32)lift < (sint32)value)
            lift = value;
        ++count;
    }
    ax = (uint32)td5_divide_checked(r_s32(average), (sint32)count);
    ay = (uint32)td5_divide_checked(r_s32(average + 4), (sint32)count);
    az = (uint32)td5_divide_checked(r_s32(average + 8), (sint32)count);
    w_u32(average, ax);
    w_u32(average + 4, ay);
    w_u32(average + 8, az);
    td5_80077d20(car + 0x154, average, velocity);
    ax = r_u32(average);
    value = r_u32(car + 0x194);
    other = r_u32(velocity);
    az = r_u32(average + 8);
    w_u32(velocity, other + (uint32)((sint32)value >> 4) - ax);
    value = r_u32(car + 0x198);
    ay = r_u32(average + 4);
    other = r_u32(velocity + 4);
    w_u32(velocity + 4, other + (uint32)((sint32)value >> 4) - ay);
    value = r_u32(car + 0x19C);
    other = r_u32(velocity + 8);
    w_u32(velocity + 8, other + (uint32)((sint32)value >> 4) - az);
    length = td5_integer_sqrt(ax * ax + az * az);
    accumulated = 0;
    for (axis = 0; axis < 3; ++axis)
    {
        value = (uint32)r_s16(car + 0x258 + axis * 2);
        other = r_u32(velocity + axis * 4);
        accumulated += value * other;
    }
    impact = (uint32)((sint32)(0u - accumulated) >> 12);
    if ((sint32)impact < 0)
    {
        if ((sint32)impact < -4000)
        {
            td5_play_positioned_sound(4, -1, 1024, 4096, car + 0x1C4, 0);
            td5_800415a4((sint32)r_u8(car + 0x34D), 1024);
        }
        if ((sint32)length < 129)
        {
            restitution = (uint32)((sint32)((0u - impact) * 272) / 256);
            source = car + 0x154;
            destination = combined;
            for (group = 0; group < 2; ++group)
            {
                for (index = 0; index < 4; ++index)
                    words[index] = r_u32(source + group * 16 + index * 4);
                for (index = 0; index < 4; ++index)
                    w_u32(destination + group * 16 + index * 4, words[index]);
            }
            w_u32(destination + 32, r_u32(source + 32));
        }
        else
        {
            value = (uint32)((sint32)(length * length) / 256) + 200000;
            restitution = (uint32)td5_divide_checked((sint32)(impact * (uint32)-212500), (sint32)value);
            value = (uint32)((sint32)(length * restitution) / 16);
            angle = (uint32)((sint32)(value * 854) / 16);
            cosine = td5_80077898(angle);
            sine = td5_800777cc(angle);
            value = (uint32)td5_divide_checked(0x40000000, (sint32)length);
            z_axis = az * value;
            x_axis = ax * value;
            w_u32(rotation + 20, sine);
            w_u32(basis + 4, 0);
            w_u32(basis + 12, 0);
            w_u32(basis + 16, 0x40000000);
            w_u32(basis + 20, 0);
            w_u32(basis + 28, 0);
            w_u32(rotation, 0x40000000);
            w_u32(rotation + 4, 0);
            w_u32(rotation + 8, 0);
            w_u32(rotation + 12, 0);
            w_u32(rotation + 16, cosine);
            w_u32(rotation + 24, 0);
            w_u32(rotation + 28, 0u - sine);
            w_u32(rotation + 32, cosine);
            w_u32(basis, z_axis);
            w_u32(basis + 32, z_axis);
            w_u32(basis + 8, x_axis);
            w_u32(basis + 24, 0u - x_axis);
            td5_80077abc(basis, rotation, combined);
            w_u32(basis, z_axis);
            w_u32(basis + 4, 0);
            w_u32(basis + 8, 0u - x_axis);
            w_u32(basis + 12, 0);
            w_u32(basis + 16, 0x40000000);
            w_u32(basis + 20, 0);
            w_u32(basis + 24, x_axis);
            w_u32(basis + 28, 0);
            w_u32(basis + 32, z_axis);
            td5_80077abc(combined, basis, rotation);
            td5_80077abc(rotation, car + 0x154, combined);
        }
        bx = r_u32(velocity);
        bz = r_u32(velocity + 8);
        tangent_length = td5_integer_sqrt(bx * bx + bz * bz);
        if ((sint32)tangent_length > 0)
        {
            limit = (uint32)((sint32)(impact + 1024) / 32);
            original_y = r_u32(average + 4);
            ax = r_u32(average);
            az = r_u32(average + 8);
            w_u32(average, (uint32)((sint32)ax >> 1));
            w_u32(average + 4, (uint32)((sint32)original_y >> 1));
            w_u32(average + 8, (uint32)((sint32)az >> 1));
            VectorNormal((VECTOR *)psx_addr(average, 12), (VECTOR *)psx_addr(average, 12));
            w_u32(velocity + 4, 0);
            VectorNormal((VECTOR *)psx_addr(velocity, 12), (VECTOR *)psx_addr(velocity, 12));
            OuterProduct12((VECTOR *)psx_addr(velocity, 12), (VECTOR *)psx_addr(average, 12), (VECTOR *)psx_addr(cross, 12));
            if ((sint32)tangent_length < (sint32)limit)
                limit = tangent_length;
            value = (uint32)((sint32)(original_y * limit) / 16);
            angle = (uint32)((sint32)(value * 854) / 16);
            td5_80077e1c(basis, cross, angle, frame);
            td5_80077abc(combined, basis, car + 0x154);
            value = r_u32(velocity);
            value = (uint32)((sint32)(limit * value) / 256);
            other = r_u32(car + 0x194);
            w_u32(car + 0x194, other - value);
            value = r_u32(velocity + 8);
            value = (uint32)((sint32)(limit * value) / 256);
            other = r_u32(car + 0x19C);
            w_u32(car + 0x19C, other - value);
        }
        else
        {
            for (group = 0; group < 2; ++group)
            {
                for (index = 0; index < 4; ++index)
                    words[index] = r_u32(combined + group * 16 + index * 4);
                for (index = 0; index < 4; ++index)
                    w_u32(car + 0x154 + group * 16 + index * 4, words[index]);
            }
            w_u32(car + 0x174, r_u32(combined + 32));
        }
        for (axis = 0; axis < 3; ++axis)
        {
            value = (uint32)r_s16(car + 0x258 + axis * 2);
            value = (uint32)((sint32)(value * restitution) / 256);
            other = r_u32(car + 0x194 + axis * 4);
            w_u32(car + 0x194 + axis * 4, other - value);
        }
    }
    value = r_u32(car + 0x1C8);
    w_u32(car + 0x1C8, value + lift);
    td5_8002cce0(car, frame);
    ax = r_u32(car + 0x1C4);
    ay = r_u32(car + 0x1C8);
    az = r_u32(car + 0x1CC);
    w_u32(car + 0x124, (uint32)((sint32)ax >> 8));
    result = (sint32)impact < -400;
    w_u32(car + 0x128, (uint32)((sint32)ay >> 8));
    w_u32(car + 0x12C, (uint32)((sint32)az >> 8));
    if (result != 0)
    {
        result = r_u8(car + 0x34D) < 6;
        if (result != 0)
            return td5_play_positioned_sound(7, -1, 4096, 4096, car + 0x1C4, 0);
    }
    return result;
}

uint32 td5_80077d20(uint32 matrix, uint32 vector, uint32 output)
{
    uint32 row, term, left, right, sum, result;

    FUNCTION_MARKER(0x80077D20, "SLUS_006.10");
    for (row = 0; row < 3; ++row)
    {
        sum = 0;
        for (term = 0; term < 3; ++term)
        {
            left = r_u32(matrix + row * 12 + term * 4);
            right = r_u32(vector + term * 4);
            result = td5_8007a340(left, right);
            sum += result;
        }
        w_u32(output + row * 4, sum);
    }
    return result;
}

uint32 td5_80077898(uint32 angle)
{
    uint32 index, first, second, fraction, result;

    FUNCTION_MARKER(0x80077898, "SLUS_006.10");
    index = ((uint32)((sint32)angle >> 18) + 1024) & 4095;
    first = index < 2048 ? r_u32(0x8009B3F8 + index * 4) : 0u - r_u32(0x800993F8 + index * 4);
    index = ((uint32)((sint32)angle >> 18) + 1025) & 4095;
    second = index < 2048 ? r_u32(0x8009B3F8 + index * 4) : 0u - r_u32(0x800993F8 + index * 4);
    fraction = (angle & 0x3FFFF) << 12;
    result = td5_8007a340(second, fraction);
    return result + td5_8007a340(first, 0x40000000u - fraction);
}

uint32 td5_800777cc(uint32 angle)
{
    uint32 index, first, second, fraction, result;

    FUNCTION_MARKER(0x800777CC, "SLUS_006.10");
    index = (uint32)((sint32)angle >> 18) & 4095;
    first = index < 2048 ? r_u32(0x8009B3F8 + index * 4) : 0u - r_u32(0x800993F8 + index * 4);
    index = ((uint32)((sint32)angle >> 18) + 1) & 4095;
    second = index < 2048 ? r_u32(0x8009B3F8 + index * 4) : 0u - r_u32(0x800993F8 + index * 4);
    fraction = (angle & 0x3FFFF) << 12;
    result = td5_8007a340(second, fraction);
    return result + td5_8007a340(first, 0x40000000u - fraction);
}

uint32 td5_80077e1c(uint32 output, uint32 axis, uint32 angle, uint32 original_stack)
{
    uint32 frame = original_stack - 0xD8;
    uint32 basis = frame + 0x10, rotation = frame + 0x38, combined = frame + 0x60;
    uint32 index, first, second, fraction, value, cosine, sine, x, y, z;
    uint32 yy, zz, radial, length, scale, coupled_scale, radial_scale, q;
    uint32 b00, b02, b10, b11, b12, b20, b21, b22;

    FUNCTION_MARKER(0x80077E1C, "SLUS_006.10");
    index = ((uint32)((sint32)angle >> 18) + 1024) & 4095;
    first = index < 2048 ? r_u32(0x8009B3F8 + index * 4) : 0u - r_u32(0x800993F8 + index * 4);
    index = ((uint32)((sint32)angle >> 18) + 1025) & 4095;
    second = index < 2048 ? r_u32(0x8009B3F8 + index * 4) : 0u - r_u32(0x800993F8 + index * 4);
    fraction = (angle & 0x3FFFF) << 12;
    value = td5_8007a340(second, fraction);
    cosine = value + td5_8007a340(first, 0x40000000u - fraction);
    w_u32(frame + 0x88, cosine);
    index = (uint32)((sint32)angle >> 18) & 4095;
    first = index < 2048 ? r_u32(0x8009B3F8 + index * 4) : 0u - r_u32(0x800993F8 + index * 4);
    index = ((uint32)((sint32)angle >> 18) + 1) & 4095;
    second = index < 2048 ? r_u32(0x8009B3F8 + index * 4) : 0u - r_u32(0x800993F8 + index * 4);
    value = td5_8007a340(second, fraction);
    w_u32(frame + 0xA8, 0x40000000);
    sine = value + td5_8007a340(first, 0x40000000u - fraction);
    y = r_u32(axis + 4);
    yy = y * y;
    z = r_u32(axis + 8);
    zz = z * z;
    x = r_u32(axis);
    radial = td5_integer_sqrt(yy + zz);
    q = r_u32(frame + 0xA8);
    if ((sint32)radial <= 0)
    {
        if ((sint32)x < 0)
            sine = 0u - sine;
        w_u32(output, q);
        w_u32(output + 4, 0);
        w_u32(output + 8, 0);
        w_u32(output + 12, 0);
        cosine = r_u32(frame + 0x88);
        value = 0u - sine;
        w_u32(output + 20, value);
        w_u32(output + 24, 0);
        w_u32(output + 28, sine);
        w_u32(output + 16, cosine);
        w_u32(output + 32, cosine);
        return value;
    }
    length = td5_integer_sqrt(x * x + yy + zz);
    q = r_u32(frame + 0xA8);
    if ((sint32)length <= 0)
    {
        for (index = 0; index < 9; ++index)
            w_u32(output + index * 4, index == 0 || index == 4 || index == 8 ? q : 0);
        return length;
    }
    scale = (uint32)td5_divide_checked((sint32)q, (sint32)length);
    coupled_scale = (uint32)td5_divide_checked((sint32)q, (sint32)(radial * length));
    radial_scale = (uint32)td5_divide_checked((sint32)q, (sint32)radial);
    b00 = radial * scale;
    b10 = (0u - x) * y * coupled_scale;
    b02 = x * scale;
    b11 = z * radial_scale;
    b12 = y * scale;
    b20 = (0u - x) * z * coupled_scale;
    b21 = (0u - y) * radial_scale;
    b22 = z * scale;
    cosine = r_u32(frame + 0x88);
    w_u32(rotation, cosine);
    w_u32(rotation + 16, cosine);
    w_u32(basis + 8, b02);
    w_u32(basis + 16, b11);
    w_u32(basis + 4, 0);
    w_u32(rotation + 4, sine);
    w_u32(rotation + 8, 0);
    w_u32(rotation + 20, 0);
    w_u32(rotation + 24, 0);
    w_u32(rotation + 28, 0);
    w_u32(rotation + 32, q);
    w_u32(basis, b00);
    w_u32(basis + 12, b10);
    w_u32(rotation + 12, 0u - sine);
    w_u32(basis + 20, b12);
    w_u32(basis + 28, b21);
    w_u32(basis + 32, b22);
    w_u32(basis + 24, b20);
    td5_80077abc(basis, rotation, combined);
    w_u32(basis + 16, b11);
    w_u32(basis + 24, b02);
    w_u32(basis, b00);
    w_u32(basis + 4, b10);
    w_u32(basis + 8, b20);
    w_u32(basis + 12, 0);
    w_u32(basis + 20, b21);
    w_u32(basis + 32, b22);
    w_u32(basis + 28, b12);
    return td5_80077abc(combined, basis, output);
}

uint32 td5_8002cce0(uint32 car, uint32 original_stack)
{
    sint32 heading, normal_x, normal_z, local_x, local_y, local_z, error_x;
    uint32 cosine, sine, value, pitch, roll, root, dx, dz, vx, vz, result;

    FUNCTION_MARKER(0x8002CCE0, "SLUS_006.10");
    result = r_s16(car + 0x310) < 3;
    if (result != 0)
        return result;
    heading = r_s16(car + 0x1D2);
    cosine = (uint32)rcos(heading);
    sine = (uint32)rsin(heading);
    normal_x = r_s16(car + 0x258);
    normal_z = r_s16(car + 0x25C);
    value = (uint32)normal_x * cosine - (uint32)normal_z * sine;
    local_x = (sint16)((sint32)value / 4096);
    local_y = (sint16)r_u16(car + 0x25A);
    normal_x = r_s16(car + 0x258);
    normal_z = r_s16(car + 0x25C);
    value = (uint32)normal_x * sine + (uint32)normal_z * cosine;
    local_z = (sint16)((sint32)value / 4096);
    pitch = (uint32)td5_8007b0c0(-local_z, -local_y);
    root = td5_integer_sqrt((uint32)(local_y * local_y) + (uint32)(local_z * local_z));
    roll = (uint32)td5_8007b0c0(local_x, (sint32)root);
    normal_x = r_s16(car + 0x1D0);
    normal_z = r_s16(car + 0x1D4);
    dx = pitch - (uint32)normal_x + 512;
    dz = roll - (uint32)normal_z + 512;
    error_x = (sint32)(dx & 1023) - 512;
    if (error_x < 0)
        error_x = -error_x;
    result = (dz & 1023) - 512;
    if (error_x >= 24)
        return result;
    if ((sint32)result < 0)
        result = 0u - result;
    result = result < 24;
    if (result == 0)
        return result;
    vx = r_u32(car + 0x188);
    vz = r_u32(car + 0x190);
    result = ((vx + 2048) & 4095) - 2017 < 63;
    if (result == 0)
        return result;
    result = ((vz + 2048) & 4095) - 2017 < 63;
    if (result == 0)
        return result;
    value = ((dx & 0xC00) >> 10) | ((dz & 0xC00) >> 8);
    if (value == 0 || value == 10)
        return td5_80024134(car, original_stack - 0xE8);
    return 10;
}

uint32 td5_80020a6c(uint32 car)
{
    uint32 model, cosine, sine, value, previous, roll, first, second, tick, result;
    sint32 heading, vx, vz, lateral, pitch;

    FUNCTION_MARKER(0x80020A6C, "SLUS_006.10");
    model = r_u32(car + 0x184);
    td5_8002527c(car);
    td5_80020428(car, model, 0, 0);
    heading = r_s16(car + 0x1D2);
    w_u8(car + 0x34E, 0);
    w_u32(car + 0x2F4, 0);
    w_u32(car + 0x2F8, 0);
    cosine = (uint32)rcos(-heading);
    sine = (uint32)rsin(-heading);
    vx = (sint16)(r_s32(car + 0x194) >> 8);
    (void)r_s32(car + 0x198);
    vz = (sint16)(r_s32(car + 0x19C) >> 8);
    value = (uint32)(-vx) * sine + (uint32)vz * cosine;
    lateral = (sint16)((sint32)value / 4096);
    pitch = ((r_s16(car + 0x1D0) + 2048) & 4095) - 2048;
    if ((lateral > 32 && pitch < 128) || (lateral < -32 && pitch >= -127))
    {
        value = r_u32(car + 0x188);
        w_u32(car + 0x188, value + (uint32)(lateral / 4));
    }
    previous = r_u32(car + 0x188);
    roll = r_u32(car + 0x190);
    w_u32(car + 0x188, previous - (uint32)((sint32)previous / 16));
    value = r_u32(car + 0x2F0);
    tick = r_u32(0x80088100);
    first = (uint32)((sint32)value >> 8) * tick;
    w_u32(car + 0x190, roll - (uint32)((sint32)roll / 16));
    value = r_u32(car + 0x2EC);
    second = (uint32)((sint32)value >> 8) * tick;
    value = r_u16(car + 0x318);
    first = (uint32)((sint32)first / 256);
    w_u16(car + 0x318, value + first);
    value = r_u16(car + 0x31A);
    result = value + (uint32)((sint32)second / 256);
    w_u16(car + 0x31A, result);
    return result;
}

uint32 td5_8004d244(sint32 angle)
{
    uint32 value;

    FUNCTION_MARKER(0x8004D244, "SLUS_006.10");
    value = (uint32)angle + (angle < 0 ? 6144u : 2048u);
    return (0u - (value & 4095)) & 4095;
}

uint32 td5_80050798(uint32 owner)
{
    uint32 timer, slot, car, program, flags, value, angle, cursor, next, argument, command;
    uint32 route, node, sample, steering, heading, threshold, speed, middle, lane;

    FUNCTION_MARKER(0x80050798, "SLUS_006.10");
    timer = r_u32(owner + 0x114);
    slot = r_u32(owner + 0xD4);
    timer -= 1;
    car = 0x800984A0 + slot * 864;
    w_u32(owner + 0x114, timer);
    if ((sint32)timer < 0)
    {
        program = r_u32(owner + 0xE8);
        w_u32(owner + 0x114, 150);
        switch (program)
        {
            case 0x8007E614:
                program = 0x8007E62C;
                break;
            case 0x8007E62C:
                program = 0x8007E614;
                break;
            case 0x8007E640:
                program = 0x8007E658;
                break;
            case 0x8007E658:
                program = 0x8007E640;
                break;
            default:
                goto steering_flags;
        }
        w_u32(owner + 0xE8, program);
        w_u32(owner + 0xEC, program);
    }
steering_flags:
    flags = r_u32(owner + 0xF4);
    if ((flags & 4) != 0)
    {
        angle = td5_8004d19c(owner);
        if ((sint32)angle >= 513)
        {
            value = r_u32(car + 0x2E4) + 0x4000;
            w_u32(car + 0x2E4, value);
            if ((sint32)value > 102400)
                w_u32(car + 0x2E4, 102400);
            goto velocity_flags;
        }
        if (r_s32(owner + 0x6C) < 0)
        {
            w_u32(car + 0x2E4, 0);
            flags = r_u32(owner + 0xF4);
            w_u32(owner + 0xF4, flags ^ 4);
        }
        goto follow_course;
    }
    if ((flags & 8) != 0)
    {
        angle = td5_8004d19c(owner);
        if ((sint32)angle < 3584)
        {
            value = r_u32(car + 0x2E4) - 0x4000;
            w_u32(car + 0x2E4, value);
            if ((sint32)value < -102400)
                w_u32(car + 0x2E4, (uint32)-102400);
            goto velocity_flags;
        }
        if (r_s32(owner + 0x6C) < 0)
        {
            w_u32(car + 0x2E4, 0);
            flags = r_u32(owner + 0xF4);
            w_u32(owner + 0xF4, flags ^ 8);
        }
        goto follow_course;
    }
    goto velocity_flags;
follow_course:
    node = (uint32)r_s16(car + 0x72);
    route = r_u32(owner);
    sample = r_u8(route + node * 3 + 1);
    steering = r_u32(car + 0x2E4);
    heading = r_u32(car + 0x1BC);
    value = (uint32)((sint32)(heading + steering) >> 8) & 4095;
    value = (value - sample * 4140 / 256 + 2048) & 4095;
    value = td5_8004d244((sint32)value);
    w_u32(owner + 0x58, value);
    w_u32(owner + 0x5C, 4095 - value);
    td5_8004d330(owner, 0x4000);
velocity_flags:
    flags = r_u32(owner + 0xF4);
    if ((flags & 0x10) != 0)
    {
        value = r_u32(car + 0x2EC) + 255;
        if (value < 511)
        {
            w_u32(owner + 0xF4, flags ^ 0x10);
            w_u8(car + 0x345, 0);
            w_u16(car + 0x316, 0);
        }
        else
        {
            w_u8(car + 0x345, 1);
            w_u16(car + 0x316, 0xFF00);
        }
        return 0;
    }
    if ((flags & 2) != 0)
    {
        w_u8(car + 0x345, 0);
        value = r_u32(owner + 0x6C);
        if ((sint32)value > 0)
        {
            w_u16(car + 0x316, 255);
            value = r_u32(owner + 0x6C) * 262920;
            threshold = (uint32)((sint32)value / 256);
            speed = r_u32(car + 0x2EC);
            if ((sint32)speed < (sint32)threshold)
                w_u16(car + 0x316, 0);
        }
        else
        {
            w_u16(car + 0x316, 0xFF00);
            value = r_u32(owner + 0x6C) * 262920;
            threshold = (uint32)((sint32)value / 256);
            speed = r_u32(car + 0x2EC);
            if ((sint32)threshold > (sint32)speed)
                w_u16(car + 0x316, 0);
        }
    }
    cursor = r_u32(owner + 0xEC);
    command = r_u32(cursor);
    switch (command)
    {
        case 0:
            angle = td5_8004d19c(owner);
            if ((sint32)angle >= 64 && (sint32)td5_8004d19c(owner) < 4033)
                return 0;
            if (r_s32(owner + 0x6C) < 0)
                w_u32(car + 0x2E4, 0);
            w_u32(owner + 0xE8, 0);
            w_u32(owner + 0xF4, 0);
            w_u32(owner + 0xF8, 0);
            w_u32(owner + 0x10C, 0);
            w_u32(car + 0x2E4, 0);
            w_u32(car + 0x18C, 0);
            return 1;
        case 1:
            cursor = r_u32(owner + 0xEC);
            flags = r_u32(owner + 0xF4);
            next = r_u32(owner + 0xEC);
            argument = r_u32(cursor + 4);
            w_u32(owner + 0xF4, flags | 1);
            w_u32(owner + 0xEC, next + 8);
            w_u32(owner + 0xF0, argument);
            return 0;
        case 2:
            flags = r_u32(owner + 0xF4);
            cursor = r_u32(owner + 0xEC);
            w_u32(owner + 0xF4, flags | 2);
            next = r_u32(owner + 0xEC);
            argument = r_u32(cursor + 4);
            w_u32(owner + 0xEC, next + 8);
            w_u32(owner + 0x6C, argument);
            return 0;
        case 3:
            cursor = r_u32(owner + 0xEC);
            flags = r_u32(owner + 0xF4);
            next = r_u32(owner + 0xEC);
            argument = r_u32(cursor + 4);
            w_u32(owner + 0xEC, next + 8);
            w_u32(owner + 0xF4, flags | argument);
            return 0;
        case 4:
            cursor = r_u32(owner + 0xEC);
            argument = r_u32(cursor + 4);
            next = r_u32(owner + 0xEC);
            flags = r_u32(owner + 0xF4);
            w_u32(owner + 0xEC, next + 8);
            w_u32(owner + 0xF4, flags & ~argument);
            return 0;
        case 5:
        case 6:
        case 8:
        case 10:
        case 11:
            flags = r_u32(owner + 0xF4);
            cursor = r_u32(owner + 0xEC);
            switch (command)
            {
                case 5:
                    flags |= 4;
                    break;
                case 6:
                    flags |= 8;
                    break;
                case 8:
                    flags |= 0x10;
                    break;
                case 10:
                    flags |= 0x40;
                    break;
                default:
                    flags |= 0x80;
                    break;
            }
            w_u32(owner + 0xF4, flags);
            w_u32(owner + 0xEC, cursor + 4);
            return 0;
        case 7:
            w_u8(car + 0x345, 1);
            w_u16(car + 0x316, 0xFF00);
            cursor = r_u32(owner + 0xEC);
            w_u32(owner + 0xEC, cursor + 4);
            return 0;
        case 9:
            angle = td5_8004d19c(owner);
            w_u32(owner + 0x114, 250);
            if (angle - 2305 < 1535)
            {
                slot = r_u32(owner + 0xD4);
                node = (uint32)r_s16(0x80098510 + slot * 864);
                route = r_u32(0x800884C4);
                middle = (r_u8(route + node * 24 + 3) & 15) >> 1;
                lane = r_u8(car + 0x7C);
                if (middle >= lane)
                {
                    program = 0x8007E614;
                    goto select_program;
                }
            }
            if ((sint32)angle >= 1792)
            {
                slot = r_u32(owner + 0xD4);
                node = (uint32)r_s16(0x80098510 + slot * 864);
                route = r_u32(0x800884C4);
                middle = (r_u8(route + node * 24 + 3) & 15) >> 1;
                lane = r_u8(car + 0x7C);
                if (middle < lane)
                {
                    program = 0x8007E62C;
                    goto select_program;
                }
            }
            program = 0x8007E658;
            if (angle - 256 < 1537)
            {
                slot = r_u32(owner + 0xD4);
                node = (uint32)r_s16(0x80098510 + slot * 864);
                route = r_u32(0x800884C4);
                middle = (r_u8(route + node * 24 + 3) & 15) >> 1;
                lane = r_u8(car + 0x7C);
                if (lane >= middle)
                    program = 0x8007E640;
            }
        select_program:
            w_u32(owner + 0xE8, program);
            w_u32(owner + 0xEC, program);
            return 0;
        default:
            return 0;
    }
}

uint32 td5_8003d708(uint32 car, uint32 point, uint32 type)
{
    uint32 effect, packet, record, destination, bank, group, index, words[4];
    uint32 cosine, sine, value, first, second, lifetime;
    sint32 random;

    FUNCTION_MARKER(0x8003D708, "SLUS_006.10");
    for (effect = 0; effect < 50; ++effect)
        if (r_u8(0x8009EFCF + effect * 68) == 0)
            break;
    if (effect == 50)
        return 1;
    for (packet = 0; packet < 20; ++packet)
        if (r_s8(0x8009EC00 + packet) == 0)
            break;
    if (packet == 20)
        return 1;
    w_u8(0x8009EC00 + packet, 1);
    record = 0x8009EFB0 + effect * 68;
    w_u8(record + 1, packet);
    for (bank = 0; bank < 2; ++bank)
    {
        destination = (bank == 0 ? 0x8008C518 : 0x8008C838) + packet * 40;
        for (group = 0; group < 2; ++group)
        {
            for (index = 0; index < 4; ++index)
                words[index] = r_u32(0x8009EC48 + group * 16 + index * 4);
            for (index = 0; index < 4; ++index)
                w_u32(destination + group * 16 + index * 4, words[index]);
        }
        first = r_u32(0x8009EC68);
        second = r_u32(0x8009EC6C);
        w_u32(destination + 32, first);
        w_u32(destination + 36, second);
    }
    w_u8(record + 31, 0x80);
    w_u32(record + 64, 0x8003CD14);
    w_u32(record + 60, 0x8003C99C);
    w_u32(record + 32, r_u32(point));
    w_u32(record + 36, r_u32(point + 4));
    w_u32(record + 40, r_u32(point + 8));
    cosine = (uint32)rcos((r_s32(car + 0x1BC) >> 8) + 2048);
    sine = (uint32)rsin((r_s32(car + 0x1BC) >> 8) + 2048);
    value = r_u32(car + 0x194);
    w_u32(record + 12, value + (cosine + sine) * 3);
    value = r_u32(car + 0x19C);
    w_u32(record + 16, 512);
    w_u32(record + 20, value + (cosine - sine) * 3);
    random = randPSX();
    w_u8(record + 3, type);
    w_u8(record, (uint32)(random % 31));
    random = randPSX();
    lifetime = ((uint32)(random % 4) + 1) * 10;
    first = (uint32)td5_divide_checked(8192, (sint32)(lifetime & 0xFE));
    w_u8(record + 2, lifetime);
    lifetime = r_u8(record + 2);
    second = (uint32)td5_divide_checked(6400, (sint32)lifetime);
    w_u16(record + 4, 12288);
    w_u16(record + 8, 3520);
    w_u16(record + 6, 0u - first);
    w_u16(record + 10, second);
    return 3520;
}

uint32 td5_8003cd14(uint32 index)
{
    uint32 record = 0x8009EFB0 + index * 68;
    uint32 x, z, value, rate, axis, size_a, step_a, size_b, step_b, frame, life;

    FUNCTION_MARKER(0x8003CD14, "SLUS_006.10");
    x = r_u32(record + 12);
    z = r_u32(record + 20);
    w_u32(record + 12, x - (uint32)((sint32)x / 4));
    w_u32(record + 20, z - (uint32)((sint32)z / 4));
    for (axis = 0; axis < 3; ++axis)
    {
        value = r_u32(record + 32 + axis * 4);
        rate = r_u32(record + 12 + axis * 4);
        w_u32(record + 32 + axis * 4, value + rate);
    }
    size_a = r_u16(record + 4);
    step_a = r_u16(record + 6);
    size_b = r_u16(record + 8);
    step_b = r_u16(record + 10);
    frame = r_u8(record);
    life = r_u8(record + 2);
    w_u8(record, (frame + 1) & 31);
    value = life + 255;
    w_u16(record + 4, size_a + step_a);
    w_u16(record + 8, size_b + step_b);
    w_u8(record + 2, value);
    if (life == 0)
    {
        index = r_u8(record + 1);
        value = (uint32)-3;
        w_u8(0x8009EC00 + index, value);
        w_u8(record + 31, 0);
    }
    return value;
}

uint32 td5_8003c99c(uint32 index)
{
    uint32 record = 0x8009EFB0 + index * 68;
    uint32 size, frame, buffer, slot, negative, packet, texture;
    uint32 x, y, z, vertex, value, other, result, shift, depth, ordering;

    FUNCTION_MARKER(0x8003C99C, "SLUS_006.10");
    size = r_u16(record + 8);
    size = (size >> 4) / 3;
    frame = r_u8(record);
    buffer = r_u32(0x80088104);
    slot = r_u8(record + 1);
    frame >>= 2;
    negative = 0u - size;
    w_u16(0x8007E29C, size);
    w_u16(0x8007E29E, negative);
    w_u16(0x8007E2A4, negative);
    w_u16(0x8007E2A6, negative);
    w_u16(0x8007E2B4, negative);
    w_u16(0x8007E2AC, size);
    w_u16(0x8007E2AE, size);
    w_u16(0x8007E2B6, size);
    packet = 0x8008C518 + buffer * 800 + slot * 40;
    x = r_u32(record + 44);
    y = r_u32(record + 48);
    z = r_u32(record + 52);
    xport_gte_write_control(5, x);
    xport_gte_write_control(6, y);
    xport_gte_write_control(7, z);
    for (vertex = 0; vertex < 3; ++vertex)
    {
        xport_gte_write_data(vertex * 2, r_u32(0x8007E29C + vertex * 8));
        xport_gte_write_data(vertex * 2 + 1, r_u32(0x8007E2A0 + vertex * 8));
    }
    xport_gte_execute(0x280030);
    texture = 0x8009D7B0 + frame * 10;
    w_u8(packet + 12, r_u8(texture));
    w_u8(packet + 13, r_u8(texture + 1));
    value = r_u8(texture);
    other = r_u8(texture + 2);
    w_u8(packet + 20, value + other);
    w_u8(packet + 21, r_u8(texture + 1));
    w_u8(packet + 28, r_u8(texture));
    value = r_u8(texture + 1);
    other = r_u8(texture + 3);
    w_u8(packet + 29, value + other);
    value = r_u8(texture);
    other = r_u8(texture + 2);
    w_u8(packet + 36, value + other);
    value = r_u8(texture + 1);
    other = r_u8(texture + 3);
    w_u8(packet + 37, value + other);
    w_u8(packet + 4, r_u16(record + 4) >> 8);
    w_u8(packet + 5, r_u16(record + 4) >> 8);
    w_u8(packet + 6, r_u16(record + 4) >> 8);
    w_u32(packet + 8, xport_gte_read_data(12));
    w_u32(packet + 16, xport_gte_read_data(13));
    w_u32(packet + 24, xport_gte_read_data(14));
    result = xport_gte_read_control(31) & 0x1C66000;
    if (result != 0)
        return result;
    xport_gte_write_data(0, r_u32(0x8007E2B4));
    xport_gte_write_data(1, r_u32(0x8007E2B8));
    xport_gte_execute(0x180001);
    w_u16(packet + 22, r_u16(texture + 4));
    value = r_u8(record + 3);
    w_u16(packet + 14, r_u16(0x800A1F70 + value * 2));
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
        return result;
    ordering = r_u32(0x800880F0);
    value = r_u32(ordering + 0x70);
    other = r_u32(packet);
    value = r_u32(value + depth * 4);
    w_u32(packet, (other & 0xFF000000) | (value & 0xFFFFFF));
    value = r_u32(ordering + 0x70) + depth * 4;
    other = r_u32(value);
    result = (other & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(value, result);
    return result;
}

uint32 td5_8006dbb4(uint32 object, uint32 original_stack)
{
    MATRIX matrix = {0};
    uint32 rotation[5], addresses[3];
    uint32 first, second, third, angle, cosine, sine, buffer, primitive, packet, vertices;
    uint32 index, vertex, depth, result, ordering, shift, pointer, value, other, tag;
    uint32 width, height, used, capacity, flare, brightness, color, factor, texture;
    uint32 offset_x, offset_y, center_x, center_y, product_y, half_height;
    sint32 average_x, average_y, sum_x, sum_y;

    FUNCTION_MARKER(0x8006DBB4, "SLUS_006.10");
    first = r_u32(0x80092228);
    second = r_u32(0x80092224);
    angle = (uint32)td5_8007b0c0((sint32)first, (sint32)second);
    cosine = (uint32)rcos((sint32)angle);
    sine = (uint32)rsin((sint32)angle);
    matrix.m[0][0] = (sint16)cosine;
    matrix.m[0][1] = (sint16)(0u - sine);
    matrix.m[1][0] = (sint16)sine;
    matrix.m[1][1] = (sint16)cosine;
    matrix.m[2][2] = 4096;
    MulMatrix2((MATRIX *)psx_addr(0x800A3B90, 20), &matrix);
    memcpy(rotation, &matrix, 20);
    for (index = 0; index < 5; ++index)
        xport_gte_write_control(index, rotation[index]);
    buffer = r_u32(0x80088104);
    primitive = r_u32(object + 0x30);
    packet = r_u32(object + 0x34 + buffer * 4);
    vertices = r_u32(object + 0x2C);
    primitive += object;
    packet += object;
    vertices += object;
    for (vertex = 0; vertex < 3; ++vertex)
        addresses[vertex] = vertices + (uint32)r_s16(primitive + 4 + vertex * 2) * 8;
    for (vertex = 0; vertex < 3; ++vertex)
    {
        xport_gte_write_data(vertex * 2, r_u32(addresses[vertex]));
        xport_gte_write_data(vertex * 2 + 1, r_u32(addresses[vertex] + 4));
    }
    xport_gte_execute(0x280030);
    w_u32(packet + 8, xport_gte_read_data(12));
    w_u32(packet + 16, xport_gte_read_data(13));
    w_u32(packet + 24, xport_gte_read_data(14));
    pointer = vertices + (uint32)r_s16(primitive + 10) * 8;
    xport_gte_write_data(0, r_u32(pointer));
    xport_gte_write_data(1, r_u32(pointer + 4));
    xport_gte_execute(0x180001);
    xport_gte_execute(0x168002E);
    result = original_stack - 0x20;
    depth = xport_gte_read_data(7);
    if ((sint32)depth <= 0)
        goto restore_rotation;
    value = (uint32)(r_s32(0x80088140) >> 2);
    result = packet + 32;
    if ((sint32)depth >= (sint32)value)
        goto restore_rotation;
    w_u32(packet + 32, xport_gte_read_data(14));
    ordering = r_u32(0x800880F0);
    first = r_u32(0x80088110);
    second = r_u32(0x80088164);
    other = r_u32(packet);
    pointer = r_u32(ordering + 0x70);
    shift = first - second;
    value = (uint32)((sint32)depth >> (shift & 31)) * 4;
    tag = r_u32(pointer + value);
    w_u32(packet, (other & 0xFF000000) | (tag & 0xFFFFFF));
    pointer = r_u32(ordering + 0x70) + value;
    other = r_u32(pointer);
    result = (other & 0xFF000000) | (packet & 0xFFFFFF);
    w_u32(pointer, result);
    if ((sint32)depth >= 2048)
        goto restore_rotation;
    sum_x = r_s16(packet + 8);
    sum_x += r_s16(packet + 16);
    sum_x += r_s16(packet + 24);
    sum_x += r_s16(packet + 32);
    average_x = sum_x / 4;
    sum_y = r_s16(packet + 10);
    sum_y += r_s16(packet + 18);
    sum_y += r_s16(packet + 26);
    sum_y += r_s16(packet + 34);
    result = (uint32)(sum_y < 0 ? sum_y + 3 : sum_y);
    average_y = sum_y / 4;
    if (average_x < 0)
        goto restore_rotation;
    width = r_u32(0x800880F4);
    result = average_x < (sint32)width;
    if (result == 0 || average_y < 0)
        goto restore_rotation;
    height = r_u32(0x800880F8);
    result = average_y < (sint32)height;
    if (result == 0)
        goto restore_rotation;
    used = r_u32(0x80088514);
    capacity = r_u32(0x800884FC);
    value = used + 80;
    result = width >> 31;
    if (capacity < value)
        goto restore_rotation;
    offset_x = (uint32)average_x - (uint32)((sint32)width / 2);
    offset_y = (uint32)average_y - (uint32)((sint32)height / 2);
    buffer = r_u32(0x80088104);
    flare = r_u32(0x80088370 + buffer * 4);
    w_u32(0x80088514, value);
    flare += used;
    SquareRoot0((sint32)(offset_x * offset_x + offset_y * offset_y));
    brightness = (uint32)((sint32)(2048u - depth) / 16);
    color = ((brightness << 16) | (brightness << 8) | brightness) & 0xFFFFFF;
    color |= 0x2E000000;
    for (index = 0; index < 2; ++index)
    {
        factor = (uint32)r_s16(0x80088118 + index * 2);
        first = offset_x * factor;
        width = r_u32(0x800880F4);
        texture = r_u32(0x80088758 + index * 4);
        center_x = (uint32)((sint32)width / 2) + (uint32)((sint32)first / 256);
        product_y = offset_y * factor;
        height = r_u32(0x800880F8);
        half_height = (uint32)((sint32)height / 2);
        w_u32(flare, 0x09000000);
        w_u16(flare + 22, r_u16(texture + 8));
        value = r_u16(texture + 10);
        w_u32(flare + 4, color);
        w_u16(flare + 14, value);
        w_u8(flare + 12, r_u8(texture + 14));
        w_u8(flare + 13, r_u8(texture + 15));
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(flare + 20, other + value + 255);
        w_u8(flare + 21, r_u8(texture + 15));
        w_u8(flare + 28, r_u8(texture + 14));
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(flare + 29, other + value + 255);
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(flare + 36, other + value + 255);
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(flare + 37, other + value + 255);
        w_u16(flare + 8, center_x - (r_u8(texture + 12) >> 1));
        value = r_u8(texture + 13);
        center_y = half_height + (uint32)((sint32)product_y / 256);
        w_u16(flare + 10, center_y - (value >> 1));
        w_u16(flare + 16, center_x + (r_u8(texture + 12) >> 1) - 1);
        w_u16(flare + 18, center_y - (r_u8(texture + 13) >> 1));
        w_u16(flare + 24, center_x - (r_u8(texture + 12) >> 1));
        w_u16(flare + 26, center_y + (r_u8(texture + 13) >> 1) - 1);
        w_u16(flare + 32, center_x + (r_u8(texture + 12) >> 1) - 1);
        value = r_u8(texture + 13);
        ordering = r_u32(0x800880F0);
        w_u16(flare + 34, center_y + (value >> 1) - 1);
        pointer = r_u32(ordering + 0x70);
        other = r_u32(flare);
        tag = r_u32(pointer + 4);
        w_u32(flare, (other & 0xFF000000) | (tag & 0xFFFFFF));
        pointer = r_u32(ordering + 0x70);
        tag = r_u32(pointer + 4);
        w_u32(pointer + 4, (tag & 0xFF000000) | (flare & 0xFFFFFF));
        result = index + 1 < 2;
        flare += 40;
    }
restore_rotation:
    first = r_u32(0x80097648);
    second = r_u32(0x8009764C);
    xport_gte_write_control(0, first);
    xport_gte_write_control(1, second);
    first = r_u32(0x80097650);
    second = r_u32(0x80097654);
    third = r_u32(0x80097658);
    xport_gte_write_control(2, first);
    xport_gte_write_control(3, second);
    xport_gte_write_control(4, third);
    return result;
}

uint32 td5_800326d0(uint32 slot, uint32 original_stack)
{
    SVECTOR points[6] = {0};
    uint32 buffer, model, packet, quad, vertex, point, value, first, second;
    uint32 depth, order_shift, limit, ordering, pointer, offset, tag, result;
    sint32 width, height, length;

    FUNCTION_MARKER(0x800326D0, "SLUS_006.10");
    buffer = r_u32(0x80088104);
    model = r_u32(0x80098620 + slot * 864);
    packet = r_u32(0x80088208 + buffer * 4);
    width = r_s16(model + 8);
    height = r_s16(model + 10);
    length = r_s16(model + 12);
    packet += slot * 80;
    for (point = 0; point < 6; ++point)
    {
        points[point].vx = (sint16)((point & 1) != 0 ? width : -width);
        points[point].vy = (sint16)height;
        points[point].vz = (sint16)(point < 2 ? length : point < 4 ? 0 : -length);
    }
    for (quad = 0; quad < 2; ++quad)
    {
        for (vertex = 0; vertex < 3; ++vertex)
        {
            point = quad * 2 + vertex;
            value = (uint32)(uint16)points[point].vx | ((uint32)(uint16)points[point].vy << 16);
            xport_gte_write_data(vertex * 2, value);
            xport_gte_write_data(vertex * 2 + 1, (uint32)(uint16)points[point].vz);
        }
        xport_gte_execute(0x280030);
        w_u32(packet + 8, xport_gte_read_data(12));
        w_u32(packet + 16, xport_gte_read_data(13));
        w_u32(packet + 24, xport_gte_read_data(14));
        point = quad * 2 + 3;
        value = (uint32)(uint16)points[point].vx | ((uint32)(uint16)points[point].vy << 16);
        xport_gte_write_data(0, value);
        xport_gte_write_data(1, (uint32)(uint16)points[point].vz);
        xport_gte_execute(0x180001);
        w_u32(packet + 32, xport_gte_read_data(14));
        xport_gte_execute(0x168002E);
        result = original_stack - 8;
        depth = xport_gte_read_data(7);
        if ((sint32)depth > 0)
        {
            order_shift = r_u32(0x80088110);
            limit = r_u32(0x80088140);
            limit = (uint32)((sint32)limit >> ((order_shift + 2) & 31));
            result = (sint32)depth < (sint32)limit;
            if (result != 0)
            {
                ordering = r_u32(0x800880F0);
                second = r_u32(0x80088164);
                first = r_u32(packet);
                pointer = r_u32(ordering + 0x70);
                offset = (uint32)((sint32)depth >> ((order_shift - second) & 31)) * 4;
                tag = r_u32(pointer + offset + 40);
                w_u32(packet, (first & 0xFF000000) | (tag & 0xFFFFFF));
                pointer = r_u32(ordering + 0x70) + offset;
                tag = r_u32(pointer + 40);
                result = (tag & 0xFF000000) | (packet & 0xFFFFFF);
                w_u32(pointer + 40, result);
            }
        }
        packet += 40;
    }
    return result;
}

uint32 td5_8003d288(uint32 car, uint32 point, uint32 style)
{
    uint32 effect, packet, record, destination, bank, group, index, words[4];
    uint32 cosine, sine, horizontal, forward, value, first, second, lifetime;
    sint32 random;

    FUNCTION_MARKER(0x8003D288, "SLUS_006.10");
    random = randPSX();
    if (random % 10 < 6)
        return 1;
    for (effect = 0; effect < 50; ++effect)
        if (r_u8(0x8009EFCF + effect * 68) == 0)
            break;
    if (effect == 50)
        return 1;
    for (packet = 0; packet < 20; ++packet)
        if (r_s8(0x8009EC00 + packet) == 0)
            break;
    if (packet == 20)
        return 1;
    w_u8(0x8009EC00 + packet, 1);
    record = 0x8009EFB0 + effect * 68;
    w_u8(record + 1, packet);
    for (bank = 0; bank < 2; ++bank)
    {
        destination = (bank == 0 ? 0x8008C518 : 0x8008C838) + packet * 40;
        for (group = 0; group < 2; ++group)
        {
            for (index = 0; index < 4; ++index)
                words[index] = r_u32(0x8009EC48 + group * 16 + index * 4);
            for (index = 0; index < 4; ++index)
                w_u32(destination + group * 16 + index * 4, words[index]);
        }
        first = r_u32(0x8009EC68);
        second = r_u32(0x8009EC6C);
        w_u32(destination + 32, first);
        w_u32(destination + 36, second);
    }
    w_u8(record + 31, 0x80);
    w_u32(record + 64, 0x8003CD14);
    w_u32(record + 60, 0x8003C99C);
    w_u32(record + 32, r_u32(point));
    w_u32(record + 36, r_u32(point + 4));
    w_u32(record + 40, r_u32(point + 8));
    cosine = (uint32)rcos(r_s32(car + 0x1BC) >> 8);
    sine = (uint32)rsin(r_s32(car + 0x1BC) >> 8);
    random = randPSX();
    horizontal = (uint32)(random % 24576) - 12288;
    random = randPSX();
    forward = (uint32)(random % 12288);
    value = horizontal * cosine + forward * sine;
    w_u32(record + 12, (uint32)((sint32)value / 4096));
    value = forward * cosine - horizontal * sine;
    w_u32(record + 20, (uint32)((sint32)value / 4096));
    w_u32(record + 16, 8192);
    random = randPSX();
    w_u8(record + 3, style);
    w_u8(record, (uint32)(random % 31));
    random = randPSX();
    lifetime = ((uint32)(random % 4) + 1) * 15;
    first = (uint32)td5_divide_checked(12288, (sint32)(lifetime & 255));
    w_u8(record + 2, lifetime);
    lifetime = r_u8(record + 2);
    second = (uint32)td5_divide_checked(6400, (sint32)lifetime);
    w_u16(record + 4, 0x9000);
    w_u16(record + 8, 8320);
    w_u16(record + 6, 0u - first);
    w_u16(record + 10, second);
    return 8320;
}

uint32 td5_80032984(uint32 car, uint32 original_stack)
{
    sint16 points[6][3] = {0};
    VECTOR relative = {0}, transformed = {0};
    sint32 heading, maximum_x = 0, maximum_z = 0, local_x, local_z, divisor;
    uint32 cosine, sine, center_x, center_z, x, z, buffer, slot, normal_y;
    uint32 packet, result, vertex, point, quad, axis, value, first, second;
    uint32 xc, zs, negative_xs, zc, negative_xc, xs, nx, nz;
    uint32 depth, order_shift, limit, ordering, pointer, offset, tag;

    FUNCTION_MARKER(0x80032984, "SLUS_006.10");
    heading = r_s16(car + 0x1D2);
    cosine = (uint32)rcos(heading);
    sine = (uint32)rsin(heading);
    center_x = r_u32(car + 0x1C4);
    center_z = r_u32(car + 0x1CC);
    for (vertex = 0; vertex < 8; ++vertex)
    {
        x = r_u32(car + 0x80 + vertex * 12);
        x = (uint32)((sint32)(x - center_x) >> 8);
        z = r_u32(car + 0x88 + vertex * 12);
        z = (uint32)((sint32)(z - center_z) >> 8);
        local_x = (sint32)(x * cosine - z * sine) >> 12;
        if (local_x < 0)
            local_x = (sint32)(0u - (uint32)local_x);
        if (local_x > maximum_x)
            maximum_x = local_x;
        local_z = (sint32)(x * sine + z * cosine) >> 12;
        if (local_z < 0)
            local_z = (sint32)(0u - (uint32)local_z);
        if (local_z > maximum_z)
            maximum_z = local_z;
    }
    buffer = r_u32(0x80088104);
    slot = r_u8(car + 0x34D);
    normal_y = (uint32)r_s16(car + 0x25A);
    packet = r_u32(0x80088208 + buffer * 4);
    result = slot * 80;
    packet += result;
    if (normal_y == 0)
        return result;
    xc = (uint32)maximum_x * cosine;
    zs = (uint32)maximum_z * sine;
    negative_xs = (0u - (uint32)maximum_x) * sine;
    zc = (uint32)maximum_z * cosine;
    negative_xc = (0u - (uint32)maximum_x) * cosine;
    xs = (uint32)maximum_x * sine;
    points[0][0] = (sint16)((sint32)(xc + zs) >> 12);
    points[0][2] = (sint16)((sint32)(negative_xs + zc) >> 12);
    points[1][0] = (sint16)((sint32)(negative_xc + zs) >> 12);
    points[1][2] = (sint16)((sint32)(xs + zc) >> 12);
    points[4][0] = (sint16)((sint32)(xc - zs) >> 12);
    points[4][2] = (sint16)((sint32)(negative_xs - zc) >> 12);
    points[5][0] = (sint16)((sint32)(negative_xc - zs) >> 12);
    points[5][2] = (sint16)((sint32)(xs - zc) >> 12);
    for (vertex = 0; vertex < 4; ++vertex)
    {
        point = vertex < 2 ? vertex : vertex + 2;
        nx = (uint32)r_s16(car + 0x258);
        nz = (uint32)r_s16(car + 0x25C);
        divisor = r_s16(car + 0x25A);
        value = (uint32)(sint32)points[point][0] * nx;
        value += (uint32)(sint32)points[point][2] * nz;
        points[point][1] = (sint16)td5_divide_checked((sint32)(0u - value), divisor);
    }
    for (axis = 0; axis < 3; ++axis)
    {
        points[2][axis] = (sint16)(((sint32)points[0][axis] + points[4][axis]) / 2);
        points[3][axis] = (sint16)(((sint32)points[1][axis] + points[5][axis]) / 2);
    }
    value = (uint32)(r_s32(car + 0x1C4) >> 8);
    first = r_u32(0x8009765C);
    relative.vx = (sint32)(value - first);
    value = (uint32)(r_s32(car + 0x2B0) >> 8);
    first = r_u32(0x80097660);
    relative.vy = (sint32)(value - first);
    value = (uint32)(r_s32(car + 0x1CC) >> 8);
    first = r_u32(0x80097664);
    relative.vz = (sint32)(value - first);
    SetRotMatrix((MATRIX *)psx_addr(0x80097648, 20));
    ApplyMatrixLV((MATRIX *)psx_addr(0x80097648, 20), &relative, &transformed);
    w_u32(0x80092224, (uint32)transformed.vx);
    w_u32(0x80092228, (uint32)transformed.vy);
    w_u32(0x8009222C, (uint32)transformed.vz);
    first = r_u32(0x80092224);
    second = r_u32(0x80092228);
    xport_gte_write_control(5, first);
    value = r_u32(0x8009222C);
    xport_gte_write_control(6, second);
    xport_gte_write_control(7, value);
    for (quad = 0; quad < 2; ++quad)
    {
        for (vertex = 0; vertex < 3; ++vertex)
        {
            point = quad * 2 + vertex;
            value = (uint32)(uint16)points[point][0] | ((uint32)(uint16)points[point][1] << 16);
            xport_gte_write_data(vertex * 2, value);
            xport_gte_write_data(vertex * 2 + 1, (uint32)(sint32)points[point][2]);
        }
        xport_gte_execute(0x280030);
        w_u32(packet + 8, xport_gte_read_data(12));
        w_u32(packet + 16, xport_gte_read_data(13));
        w_u32(packet + 24, xport_gte_read_data(14));
        point = quad * 2 + 3;
        value = (uint32)(uint16)points[point][0] | ((uint32)(uint16)points[point][1] << 16);
        xport_gte_write_data(0, value);
        xport_gte_write_data(1, (uint32)(sint32)points[point][2]);
        xport_gte_execute(0x180001);
        w_u32(packet + 32, xport_gte_read_data(14));
        xport_gte_execute(0x168002E);
        result = original_stack - 0x18;
        depth = xport_gte_read_data(7);
        if ((sint32)depth > 0)
        {
            order_shift = r_u32(0x80088110);
            limit = r_u32(0x80088140);
            limit = (uint32)((sint32)limit >> ((order_shift + 2) & 31));
            result = (sint32)depth < (sint32)limit;
            if (result != 0)
            {
                ordering = r_u32(0x800880F0);
                second = r_u32(0x80088164);
                first = r_u32(packet);
                pointer = r_u32(ordering + 0x70);
                offset = (uint32)((sint32)depth >> ((order_shift - second) & 31)) * 4;
                tag = r_u32(pointer + offset);
                w_u32(packet, (first & 0xFF000000) | (tag & 0xFFFFFF));
                pointer = r_u32(ordering + 0x70) + offset;
                tag = r_u32(pointer);
                result = (tag & 0xFF000000) | (packet & 0xFFFFFF);
                w_u32(pointer, result);
            }
        }
        packet += 40;
    }
    return result;
}

uint32 td5_800443ac(uint32 index, uint32 slot, uint32 wheel, uint32 rate, uint32 strength)
{
    uint32 handle, record, selected, pool, source, value;

    FUNCTION_MARKER(0x800443AC, "SLUS_006.10");
    handle = index + r_u32(0x800885A8) * 4;
    record = 0x8009D8D0 + handle * 7;
    w_u8(record + 1, wheel);
    w_u8(record, slot);
    w_u8(record + 2, 1);
    value = td5_800449a0();
    w_u8(record + 3, value);
    selected = r_u8(record + 3);
    w_u8(record + 4, rate);
    w_u8(record + 6, strength);
    w_u8(record + 5, strength);
    pool = r_u32(0x80088774);
    source = 0x80098580 + wheel * 12 + slot * 864;
    value = r_u32(source);
    w_u32(pool + selected * 132 + 0x74, value);
    selected = r_u8(record + 3);
    value = r_u32(source + 4);
    w_u32(pool + selected * 132 + 0x78, value);
    selected = r_u8(record + 3);
    value = r_u32(source + 8);
    w_u32(pool + selected * 132 + 0x7C, value);
    selected = r_u8(record + 3);
    w_u8(pool + selected * 132 + 0x70, 1);
    selected = r_u8(record + 3);
    pool = r_u32(0x80088774);
    w_u16(pool + selected * 132 + 0x72, 0);
    selected = r_u8(record + 3);
    w_u8(pool + selected * 132 + 0x71, strength);
    value = r_u32(0x800885A8);
    return (handle - value * 4) & 255;
}
