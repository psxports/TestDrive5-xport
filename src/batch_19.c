#include "td5_functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint32 td5_physics_mul_div(uint32 value, uint32 factor, sint32 divisor)
{
    return (uint32)((sint32)(value * factor) / divisor);
}

/* The reviewed level overlays share this entry and vary its descriptor immediate */
void td5_call_level_initializer(uint32 target)
{
    uint32 descriptor;

    if (target == 0x800BFCB4)
    {
        td5_800bfcb4();
        return;
    }
    if (target == 0x800A50C4)
    {
        descriptor = 0x800A0000 + (uint32)(sint32)r_s16(0x800A50C8);
        w_u32(0x80088770, descriptor);
        return;
    }
    {
        char message[256];
        snprintf(message, sizeof(message), "Unbound level initializer %08X\n", target);
        xport_message_error("Test Drive 5", message);
    }
    abort();
}

sint32 td5_8001a328(uint32 archive_error_index, uint32 original_stack, uint32 original_return, uint32 saved_s0_carrier)
{
    uint32 frame = original_stack - 0x118;
    uint32 selection, size, destination, target, descriptor, header, value;
    const char *filename;

    FUNCTION_MARKER(0x8001A328, "SLUS_006.10");
    w_u32(frame + 0x110, original_return);
    w_u32(frame + 0x10C, original_stack);
    w_u32(frame + 0x108, saved_s0_carrier);
    memcpy(psx_addr(frame + 0x10, 176), psx_addr(0x800102EC, 176), 176);
    selection = r_u32(0x80087710);
    sprintf((char *)psx_addr(frame + 0xC0, 28),
        (const char *)psx_addr(0x8001039C, 21), (sint32)(selection + 1));
    filename = (const char *)psx_addr(frame + 0xC0, 28);
    size = td5_archive_aligned_size(filename, 1, archive_error_index, 0);
    w_u32(frame + 0x100, size);
    destination = r_u32(0x800132B0);
    size = r_u32(frame + 0x100);
    /* The original archive loader does not consume its capacity argument */
    (void)size;
    td5_load_archive_file((const char *)psx_addr(frame + 0xC0, 28), destination, archive_error_index);
    selection = r_u32(0x80087710);
    target = r_u32(frame + 0x10 + selection * 4);
    td5_call_level_initializer(target);
    w_u32(0x8008772C, 0);
    w_u32(0x80087730, 0);
    w_u32(0x80087734, 0);
    descriptor = r_u32(0x80088770);
    value = r_u32(descriptor + 4);
    w_u32(0x800885C4, value);
    descriptor = r_u32(0x80088770);
    header = r_u32(descriptor);
    value = (uint32)r_s16(header + 0x10);
    if (value >= 6)
        return (sint32)value;
    switch (value)
    {
    case 0:
    case 1:
        return (sint32)(0x800107A4 + value * 4);
    case 2:
        w_u32(0x80087730, 1);
        break;
    case 3:
        w_u32(0x80087730, 1);
        w_u32(0x8008772C, 1);
        break;
    case 4:
        w_u32(0x80087734, 1);
        w_u32(0x80087730, 1);
        break;
    case 5:
        w_u32(0x80087734, 1);
        w_u32(0x80087730, 1);
        w_u32(0x8008772C, 1);
        break;
    }
    return 1;
}

uint32 td5_800bfcb4(void)
{
    FUNCTION_MARKER(0x800BFCB4, "SLUS_006.10");
    w_u32(0x80088770, 0x800C187C);
    return 0x800C187C;
}

/* Native binding for the reviewed PADSEQD owner-begin callback */
uint32 td5_pad_begin_callback(uint32 owner)
{
    return psx_pad_owner_begin(owner, r_u32(0x80086B48));
}

/* Port comes from the reviewed SDK serial transaction context */
sint32 td5_8006a950(uint32 owner, uint32 port)
{
    uint32 callback, value, buffer;

    FUNCTION_MARKER(0x8006A950, "SLUS_006.10");
    callback = r_u32(0x80086B5C);
    value = xport_missing_pad_owner_callback(callback, owner);
    buffer = r_u32(owner + 0x3C);
    w_u32(0x80086BC4, value);
    w_u8(buffer, 0);
    return psx_pad_serial_transfer(owner, -2, port, 0, -2);
}

/* Adapt the SDK table callback to its selected physical port */
uint32 td5_pad_transfer_begin_callback(uint32 owner)
{
    uint32 port = r_u32(0x80086B84) != 0;

    return (uint32)td5_8006a950(owner, port);
}

/* Preserve inherited sound carriers across silent voices */
void td5_80054c58(uint32 original_stack, uint32 angle_carrier, uint32 pitch_carrier, uint32 pan_carrier)
{
    uint32 frame = original_stack - 0x48;
    uint32 voice = 0, offset = 0;
    uint32 angle = angle_carrier, pitch = pitch_carrier, pan = pan_carrier;
    uint32 position, reference, x, y, z, dx, dy, dz, value, other;
    uint32 absolute_x, absolute_z, volume, numerator, denominator, projection;
    uint32 velocity, root, rotated_x, rotated_y, rotated_z;

    FUNCTION_MARKER(0x80054C58, "SLUS_006.10");
    SetRotMatrix((MATRIX *)psx_addr(0x80097648, 20));
    do
    {
        position = r_u32(0x800919C4 + offset);
        if (position != 0)
        {
            reference = 0x8009765C;
            if (r_u32(0x8008772C) != 0)
            {
                if (position == 0x80098664)
                    reference = 0x800A2024;
                else if (position == 0x800989C4)
                    reference = 0x800A2044;
            }
            x = r_u32(position);
            other = r_u32(reference);
            dx = other - (uint32)((sint32)x >> 8);
            y = r_u32(position + 4);
            value = r_u32(reference + 4);
            z = r_u32(position + 8);
            other = r_u32(reference + 8);
            dy = value - (uint32)((sint32)y >> 8);
            dz = other - (uint32)((sint32)z >> 8);
            value = (sint32)dx < 0 ? 0 - dx : dx;
            if ((sint32)value < 512)
                dx = (sint32)dx < 0 ? 0 - 512 : 512;
            value = (sint32)dy < 0 ? 0 - dy : dy;
            if ((sint32)value < 512)
                dy = (sint32)dy < 0 ? 0 - 512 : 512;
            value = (sint32)dz < 0 ? 0 - dz : dz;
            if ((sint32)value < 512)
                dz = (sint32)dz < 0 ? 0 - 512 : 512;
            absolute_x = (sint32)dx < 0 ? 0 - dx : dx;
            absolute_z = (sint32)dz < 0 ? 0 - dz : dz;
            volume = 0;
            if (absolute_x <= 0x7FFF && absolute_z <= 0x7FFF)
            {
                value = SquareRoot0((sint32)(absolute_x * absolute_x + absolute_z * absolute_z));
                value = (uint32)cln((sint32)(value << 12));
                volume = 2708 - (uint32)((sint32)value / 16);
                if ((sint32)volume < 0)
                    volume = 0;
                if ((sint32)volume >= 1025)
                    volume = 1024;
                value = r_u32(0x800919B8 + offset);
                value *= volume;
                volume = (uint32)((sint32)value / 1024);
            }
            if (volume != 0)
            {
                w_u16(frame + 0x18, 0 - dx);
                w_u16(frame + 0x1A, 0 - dy);
                w_u16(frame + 0x1C, 0 - dz);
                xport_gte_write_data(0, r_u32(frame + 0x18));
                xport_gte_write_data(1, r_u32(frame + 0x1C));
                xport_gte_execute(0x486012);
                rotated_x = xport_gte_read_data(9);
                rotated_y = xport_gte_read_data(10);
                rotated_z = xport_gte_read_data(11);
                w_u16(frame + 0x18, rotated_x);
                w_u16(frame + 0x1A, rotated_y);
                w_u16(frame + 0x1C, rotated_z);
                x = (uint32)r_s16(frame + 0x1C);
                y = (uint32)r_s16(frame + 0x18);
                angle = (uint32)td5_8007b0c0((sint32)(x << 4), (sint32)(y << 4));
                if ((sint32)angle < 1025)
                    pan = 4096 - angle * 4;
                else if ((sint32)angle < 2049)
                    pan = (1024 - angle) << 2;
                else
                    pan = (angle - 3072) << 2;
                if (r_u32(0x800919C8 + offset) != 0)
                {
                    root = SquareRoot0((sint32)(dx * dx + dz * dz));
                    velocity = r_u32(0x800882DC);
                    value = r_u32(velocity);
                    numerator = dx * (uint32)((sint32)value >> 8);
                    value = r_u32(velocity + 8);
                    denominator = root + 1;
                    numerator += dz * (uint32)((sint32)value >> 8);
                    projection = (uint32)td5_divide_checked((sint32)numerator, (sint32)denominator);
                    velocity = r_u32(0x800919C8 + offset);
                    value = r_u32(velocity);
                    numerator = dx * (uint32)((sint32)value >> 8);
                    value = r_u32(velocity + 8);
                    numerator += dz * (uint32)((sint32)value >> 8);
                    other = (uint32)td5_divide_checked((sint32)numerator, (sint32)denominator);
                    denominator = projection + 4096;
                    numerator = other + 4096;
                    if (denominator == 0)
                        denominator = 1;
                    numerator <<= 12;
                    value = (uint32)td5_divide_checked((sint32)numerator, (sint32)denominator);
                    pitch = 128;
                    if ((sint32)value >= 128)
                    {
                        other = r_u32(0x800919BC + offset);
                        value *= other;
                        pitch = (uint32)((sint32)value / 4096);
                    }
                }
                else
                    pitch = r_u32(0x800919BC + offset);
            }
            if (r_u32(0x80088668) != 0 && r_u32(0x800919C0 + offset) != 0)
            {
                w_u32(frame + 0x10, volume);
                td5_80055184((sint32)angle, (sint32)voice, (sint32)pitch, (sint32)pan, (sint32)volume);
                offset += 92;
                ++voice;
            }
            else
                td5_8005552c((sint32)voice, (sint32)pitch, 200, (sint32)volume);
        }
        ++voice;
        offset += 92;
    } while ((sint32)voice < 24);
}

void td5_call_effect_update(uint32 callback, uint32 slot, uint32 original_stack)
{
    switch (callback)
    {
    case 0x8003BBA8:
        td5_8003bba8(callback);
        return;
    case 0x8003C190:
        td5_8003c190(slot);
        return;
    case 0x8003CD14:
        td5_8003cd14(slot);
        return;
    case 0x8003E38C:
        td5_8003e38c(slot);
        return;
    case 0x8003ECE8:
        td5_8003ece8(slot);
        return;
    case 0x8003F724:
        td5_8003f724(slot, original_stack);
        return;
    case 0x8003F8A8:
        td5_8003f8a8(slot);
        return;
    default:
        {
            char message[256];
            snprintf(message, sizeof(message), "Unresolved TD5 effect update %08x at slot %u\n", callback, slot);
            xport_message_error("Test Drive 5", message);
        }
        abort();
    }
}

void td5_call_effect_render(uint32 callback, uint32 slot, uint32 original_stack)
{
    switch (callback)
    {
    case 0x8003BD34:
        td5_8003bd34(slot);
        return;
    case 0x8003C010:
        td5_8003c010(slot);
        return;
    case 0x8003C99C:
        td5_8003c99c(slot);
        return;
    case 0x8003E1B8:
        td5_8003e1b8(slot);
        return;
    case 0x8003ED48:
        td5_8003ed48(slot);
        return;
    case 0x8003F3C8:
        td5_8003f3c8(slot, original_stack);
        return;
    default:
        {
            char message[256];
            snprintf(message, sizeof(message), "Unresolved TD5 effect render %08x at slot %u\n", callback, slot);
            xport_message_error("Test Drive 5", message);
        }
        abort();
    }
}

/* Update each active record using its constructor-selected callback */
void td5_8003bb34(uint32 original_stack)
{
    uint32 frame = original_stack - 0x20;
    uint32 slot = 0;
    uint32 offset = 0;
    uint32 callback;

    FUNCTION_MARKER(0x8003BB34, "SLUS_006.10");
    do
    {
        if ((r_u8(0x8009EFCF + offset) & 0x80) != 0)
        {
            callback = r_u32(0x8009EFF0 + offset);
            td5_call_effect_update(callback, slot, frame);
        }
        ++slot;
        offset += 68;
    } while (slot < 50);
}

/* Prepare both rotations and render each active visible record */
void td5_8003b9cc(uint32 original_stack)
{
    uint32 frame = original_stack - 0x28;
    uint32 angle = r_u32(0x80088280);
    uint32 group, word, source, destination;
    uint32 values[4];
    uint32 slot = 0;
    uint32 offset = 0;
    uint32 callback;

    FUNCTION_MARKER(0x8003B9CC, "SLUS_006.10");
    for (group = 0; group < 4; ++group)
    {
        source = 0x8009FCF8 + (group & 1) * 16;
        destination = 0x8008CF30 + group * 16;
        for (word = 0; word < 4; ++word)
            values[word] = r_u32(source + word * 4);
        for (word = 0; word < 4; ++word)
            w_u32(destination + word * 4, values[word]);
    }
    w_u32(0x80088280, angle + 128);
    angle = r_u32(0x80088284);
    destination = r_u32(0x80088280);
    w_u32(0x80088284, angle - 128);
    RotMatrixZ((sint32)destination, (MATRIX *)psx_addr(0x8008CF30, 32));
    angle = r_u32(0x80088284);
    RotMatrixZ((sint32)angle, (MATRIX *)psx_addr(0x8008CF50, 32));
    td5_8003bbb0();
    td5_8003b88c();
    SetRotMatrix((MATRIX *)psx_addr(0x8009FCF8, 20));
    do
    {
        if ((r_u8(0x8009EFCF + offset) & 0xC0) == 0xC0)
        {
            callback = r_u32(0x8009EFEC + offset);
            td5_call_effect_render(callback, slot, frame);
        }
        ++slot;
        offset += 68;
    } while (slot < 50);
}

/* Native helper for the original live ordering-table link sequence */
static void td5_texture_link(uint32 packet, uint32 frame)
{
    uint32 depth = r_u32(frame + 0x24);
    uint32 shift = r_u32(frame + 0x58);
    uint32 ordering = r_u32(frame + 0x50);
    uint32 previous = r_u32(packet);
    uint32 value, mask;

    ordering += (uint32)((sint32)depth >> (shift & 31)) * 4;
    value = r_u32(ordering);
    mask = r_u32(frame + 0xA8);
    w_u32(packet, (previous & 0xFF000000) | (value & mask));
    value = r_u32(ordering);
    mask = r_u32(frame + 0xA8);
    w_u32(ordering, (value & 0xFF000000) | (packet & mask));
}

/* Native helper retaining the original guest UV temporaries and writes */
static void td5_texture_uv(uint32 primitive, uint32 vertices, uint32 packet, uint32 frame, uint32 count, uint32 cosine, uint32 sine, uint32 shift, uint32 origin_u, uint32 origin_v, uint32 texture_u, uint32 texture_v)
{
    uint32 u[4], v[4];
    uint32 index, vertex, x, z, product, cross, minimum_u, minimum_v;
    uint32 mask_u, mask_v, value, color;

    for (index = 0; index < count; ++index)
    {
        vertex = vertices + (uint32)(sint32)r_s16(primitive + index * 2) * 8;
        x = (uint32)(sint32)r_s16(vertex);
        z = (uint32)(sint32)r_s16(vertex + 4);
        cross = z * sine;
        product = x * cosine - cross;
        if (index == 3)
        {
            w_u32(frame + 0xB0, cross);
            w_u32(frame + 0xB0, z * cosine);
            cross = r_u32(frame + 0xB0);
        }
        else
            cross = z * cosine;
        value = (uint32)((sint32)product / 4096) + origin_u;
        u[index] = (uint32)((sint32)value >> (shift & 31));
        w_u32(frame + 0x28 + index * 4, u[index]);
        product = x * sine + cross;
        value = (uint32)((sint32)product / 4096) + origin_v;
        v[index] = (uint32)((sint32)value >> (shift & 31));
        w_u32(frame + 0x38 + index * 4, v[index]);
    }
    minimum_u = u[0];
    for (index = 1; index < count; ++index)
        if ((sint32)u[index] < (sint32)minimum_u)
            minimum_u = u[index];
    mask_u = minimum_u & r_u32(frame + 0x68);
    minimum_v = count == 4 ? r_u32(frame + 0x38) : v[0];
    for (index = 1; index < count; ++index)
        if ((sint32)v[index] < (sint32)minimum_v)
            minimum_v = v[index];
    value = count == 4 ? r_u32(frame + 0x28) : u[0];
    w_u8(packet + 12, texture_u + value - mask_u);
    mask_v = minimum_v & r_u32(frame + 0x70);
    value = r_u32(frame + 0x38);
    w_u8(packet + 13, texture_v + value - mask_v);
    for (index = 1; index < count; ++index)
    {
        value = r_u32(frame + 0x28 + index * 4);
        w_u8(packet + 12 + index * 8, texture_u + value - mask_u);
        value = r_u32(frame + 0x38 + index * 4);
        if (index + 1 == count)
        {
            color = r_u32(frame + (count == 3 ? 0x78 : 0x80));
            w_u32(packet + 4, color);
        }
        w_u8(packet + 13 + index * 8, texture_v + value - mask_v);
    }
}

/* Native UV inputs retain the original inherited-register path */
uint32 td5_8002fe8c(uint32 object, uint32 car, uint32 original_stack, uint32 angle, uint32 uv_shift, uint32 origin_v, uint32 origin_u)
{
    uint32 frame = original_stack - 0xE0;
    uint32 maximum = r_u8(car + 0x340);
    uint32 state = r_u8(car + 0x33F);
    uint32 texture = r_u32(0x800884F0);
    uint32 value, other, mode, descriptor, cosine, sine, stream, vertices;
    uint32 texture_u, texture_v, ordering, far_shift, tag, packet, primitive;
    uint32 kind, index, vertex[3], point, product, screen_x, screen_y, count;
    uint32 x, y, z, relative_x, relative_y, relative_z;
    sint32 level, amount;
    VECTOR input, output;

    FUNCTION_MARKER(0x8002FE8C, "SLUS_006.10");
    ++maximum;
    if (state == 1)
    {
        level = r_s16(car + 0x336);
        if (level < (sint32)maximum)
        {
            value = (uint32)level + (maximum >> 4);
            w_u16(car + 0x336, value);
            level = (sint16)value;
        }
        if (level >= (sint32)maximum)
        {
            w_u8(car + 0x33F, 0);
            w_u16(car + 0x336, maximum);
        }
    }
    else if (state == 2)
    {
        level = r_s16(car + 0x336);
        if (level > 0)
        {
            value = (uint32)level - 16;
            w_u16(car + 0x336, value);
            level = (sint16)value;
        }
        if (level <= 0)
        {
            w_u8(car + 0x33F, 1);
            value = r_u8(car + 0x34F);
            descriptor = r_u32(0x80088488);
            w_u16(car + 0x336, 0);
            value = r_u32(descriptor + value * 32 + 28);
            td5_8002ef1c(car, value, frame, car);
        }
    }
    w_u8(0x80087838, r_u16(car + 0x336));
    w_u8(0x80087839, r_u16(car + 0x336));
    w_u8(0x8008783A, r_u16(car + 0x336));
    mode = r_u8(car + 0x33D);
    if (mode == 0)
    {
        value = (uint32)(r_s32(car + 0x1C4) / 256);
        other = r_u32(0x8009765C);
        value -= other;
        relative_x = (uint32)((sint32)value / 4);
        w_u32(frame + 0x10, relative_x);
        value = (uint32)(r_s32(car + 0x1C8) / 256);
        other = r_u32(0x80097660);
        value -= other;
        relative_y = (uint32)((sint32)value / 4);
        w_u32(frame + 0x14, relative_y);
        value = (uint32)(r_s32(car + 0x1CC) / 256);
        other = r_u32(0x80097664);
        value -= other;
        relative_z = (uint32)((sint32)value / 4);
        w_u32(frame + 0x18, relative_z);
        input.vx = (sint32)r_u32(frame + 0x10);
        input.vy = (sint32)r_u32(frame + 0x14);
        input.vz = (sint32)r_u32(frame + 0x18);
        ApplyMatrixLV((MATRIX *)psx_addr(0x80097648, 20), &input, &output);
        w_u32(frame + 0x10, (uint32)output.vx);
        w_u32(frame + 0x14, (uint32)output.vy);
        w_u32(frame + 0x18, (uint32)output.vz);
        z = r_u32(frame + 0x18);
        screen_x = 0;
        screen_y = 0;
        if (z != 0)
        {
            x = r_u32(frame + 0x10);
            value = r_u32(0x80088144);
            screen_x = (uint32)td5_divide_checked((sint32)(x * value), (sint32)z);
            y = r_u32(frame + 0x14);
            screen_y = (uint32)td5_divide_checked((sint32)(y * value), (sint32)z);
        }
        value = r_u32(0x800880F4);
        other = r_u32(0x800880F8);
        w_u16(0x8007E09C, screen_x + (uint32)((sint32)value / 2));
        w_u16(0x8007E09E, screen_y + (uint32)((sint32)other / 2));
        td5_800797fc(car + 0x110, object);
        value = td5_integer_sqrt(relative_x * relative_x + relative_y * relative_y + relative_z * relative_z);
        return td5_8002f384(object, 0x8007E09C, value * 4);
    }
    if (mode == 1)
    {
        angle = (uint32)(sint32)r_s16(car + 0x1D2);
        sine = (uint32)rsin((sint32)angle);
        cosine = (uint32)rcos((sint32)angle);
        value = (uint32)(r_s32(car + 0x194) / 256);
        other = r_u32(car + 0x19C);
        product = sine * value + cosine * (uint32)((sint32)other / 256);
        value = (uint32)((sint32)product / 16);
        angle = 0;
        value = r_u32(0x800881F0) + (uint32)((sint32)value / 32);
        uv_shift = 3;
        w_u32(0x800881F0, value);
        origin_u = 512;
        origin_v = (uint32)((sint32)value / 256);
        w_u32(frame + 0x68, 0xFFFFFF80);
        w_u32(frame + 0x70, 0xFFFFFFC0);
    }
    else if (mode == 2)
    {
        value = r_u32(car + 0x1BC);
        other = r_u32(object + 0x10);
        uv_shift = 3;
        angle = (uint32)((sint32)(0 - value) >> 8);
        value = r_u32(object + 0x18);
        origin_u = (uint32)((sint32)other / 8192);
        origin_v = (uint32)((sint32)value / 8192);
        w_u32(frame + 0x68, 0xFFFFFFC0);
        w_u32(frame + 0x70, 0xFFFFFFC0);
    }
    cosine = (uint32)rcos((sint32)angle);
    sine = (uint32)rsin((sint32)angle);
    value = r_u32(object + 0x30);
    texture_u = r_u8(texture + 14);
    texture_v = r_u8(texture + 15);
    stream = object + value;
    value = r_u32(0x80087838);
    other = r_u32(0x80088110);
    w_u32(frame + 0x78, value | 0x26000000);
    w_u32(frame + 0x80, value | 0x2E000000);
    value = r_u32(object + 0x2C);
    far_shift = r_u32(0x80088164);
    amount = r_s16(object + 12);
    ordering = r_u32(0x800880F0);
    vertices = object + value;
    other -= far_shift;
    value = (uint32)(amount >> (other & 31)) * 4;
    w_u32(frame + 0x58, other);
    ordering = r_u32(ordering + 0x70);
    other = r_u32(0x80088140);
    tag = r_u8(stream + 3);
    w_u32(frame + 0x50, ordering + value);
    w_u32(frame + 0x60, (uint32)((sint32)other >> ((far_shift + 2) & 31)));
    if (tag == 0)
        return value;
    w_u32(frame + 0xA8, 0xFFFFFF);
    do
    {
        kind = r_u8(stream + 2);
        amount = r_s16(stream);
        stream += 4;
        if (kind == 0 || kind == 1)
            stream += (uint32)amount * 20;
        else if (kind == 2 || kind == 3)
        {
            primitive = stream;
            packet = r_u32((kind == 2 ? 0x800881F4 : 0x800881FC) + r_u32(0x80088104) * 4);
            count = kind == 2 ? 3 : 4;
            for (index = 0; (sint32)index < amount; ++index)
            {
                if (r_s16(primitive + (kind == 2 ? 22 : 26)) >= 151
                    && r_s16(primitive + (kind == 2 ? 30 : 34)) >= 151
                    && r_s16(primitive + (kind == 2 ? 38 : 42)) >= 151
                    && (kind == 2 || r_s16(primitive + 50) >= 151))
                {
                    vertex[0] = vertices + (uint32)(sint32)r_s16(primitive) * 8;
                    vertex[1] = vertices + (uint32)(sint32)r_s16(primitive + 2) * 8;
                    vertex[2] = vertices + (uint32)(sint32)r_s16(primitive + 4) * 8;
                    xport_gte_write_data(0, r_u32(vertex[0]));
                    xport_gte_write_data(1, r_u32(vertex[0] + 4));
                    xport_gte_write_data(2, r_u32(vertex[1]));
                    xport_gte_write_data(3, r_u32(vertex[1] + 4));
                    xport_gte_write_data(4, r_u32(vertex[2]));
                    xport_gte_write_data(5, r_u32(vertex[2] + 4));
                    xport_gte_execute(0x280030);
                    xport_gte_execute(0x1400006);
                    w_u32(frame + 0x20, xport_gte_read_data(24));
                    if (kind == 3)
                    {
                        w_u32(packet + 8, xport_gte_read_data(12));
                        w_u32(packet + 16, xport_gte_read_data(13));
                        w_u32(packet + 24, xport_gte_read_data(14));
                        point = vertices + (uint32)(sint32)r_s16(primitive + 6) * 8;
                        xport_gte_write_data(0, r_u32(point));
                        xport_gte_write_data(1, r_u32(point + 4));
                        xport_gte_execute(0x180001);
                        xport_gte_execute(0x1400006);
                        w_u32(frame + 0x48, xport_gte_read_data(24));
                    }
                    xport_gte_execute(kind == 2 ? 0x158002D : 0x168002E);
                    if (r_s32(frame + 0x20) <= 0 || (kind == 3 && r_s32(frame + 0x48) >= 0))
                    {
                        w_u32(frame + 0x24, xport_gte_read_data(7));
                        value = r_u32(frame + 0x24);
                        other = r_u32(frame + 0x60);
                        if ((sint32)value < (sint32)other)
                        {
                            if (kind == 2)
                            {
                                w_u32(packet + 8, xport_gte_read_data(12));
                                w_u32(packet + 16, xport_gte_read_data(13));
                                w_u32(packet + 24, xport_gte_read_data(14));
                            }
                            else
                                w_u32(packet + 32, xport_gte_read_data(14));
                            td5_texture_link(packet, frame);
                            td5_texture_uv(primitive, vertices, packet, frame, count, cosine, sine, uv_shift, origin_u, origin_v, texture_u, texture_v);
                            packet += kind == 2 ? 32 : 40;
                        }
                    }
                }
                primitive += kind == 2 ? 44 : 56;
            }
            stream += (uint32)amount * (kind == 2 ? 44 : 56);
        }
        value = r_u8(stream + 3);
    } while (value != 0);
    return value;
}

uint32 td5_80020e9c(uint32 car, uint32 original_stack, uint32 physics_force_carrier)
{
    uint32 frame = original_stack - 0x128;
    uint32 descriptor, value, other, drag, tick, velocity_x, velocity_z;
    uint32 steering, heading, pedal, drive, sine_steer, distance;
    uint32 weight, balance, denominator, coefficient, surface, offset;
    uint32 rpm, gear, maximum, synthetic, forward, target, step;
    uint32 limit_speed, brake, front_force, rear_force, lateral_force, steered_force;
    uint32 inertia, front_distance, rear_distance, lateral, yaw;
    uint32 first, second, third, moment, numerator, capacity, slip, root;
    uint32 residual, acceleration_x, acceleration_z, rotation, rotation_step, first_wheel;

    FUNCTION_MARKER(0x80020E9C, "SLUS_006.10");
    descriptor = r_u32(car + 0x184);
    value = td5_80025900(car + 0x70);
    w_u32(frame + 0x10, value);
    value = td5_80025900(car);
    w_u32(frame + 0x14, value);
    value = td5_80025900(car + 0xE);
    w_u32(frame + 0x18, value);
    value = td5_80025900(car + 0x1C);
    w_u32(frame + 0x1C, value);
    value = td5_80025900(car + 0x2A);
    surface = r_u32(frame + 0x10);
    w_u32(frame + 0x20, value);
    w_u8(car + 0x348, surface);
    td5_800250f0(car);
    pedal = (uint32)r_s16(car + 0x316);
    surface = r_u32(frame + 0x10);
    if ((sint32)pedal < 32 || r_u8(car + 0x343) < 2)
    {
        value = (uint32)r_s16(0x8007DF7C + surface * 2);
        other = (uint32)r_s16(descriptor + 0x6C);
    }
    else
    {
        value = (uint32)r_s16(0x8007DF7C + surface * 2);
        other = (uint32)r_s16(descriptor + 0x6A);
    }
    drag = other + (value << 8);
    value = r_u32(car + 0x194);
    other = td5_physics_mul_div((uint32)((sint32)(0 - value) / 256), drag, 4096);
    tick = r_u32(0x80088100);
    other = td5_physics_mul_div(other, tick, 256);
    velocity_z = r_u32(car + 0x19C);
    w_u32(car + 0x194, value + other);
    other = td5_physics_mul_div((uint32)((sint32)(0 - velocity_z) / 256), drag, 4096);
    other = td5_physics_mul_div(other, tick, 256);
    steering = r_u32(car + 0x2E4);
    heading = r_u32(car + 0x1BC);
    velocity_x = r_u32(car + 0x194);
    yaw = r_u32(car + 0x18C);
    velocity_z += other;
    w_u32(car + 0x19C, velocity_z);
    heading = (uint32)((sint32)heading >> 8);
    w_u32(frame + 0x80, yaw);
    front_distance = (uint32)r_s16(descriptor + 0x28);
    pedal = (uint32)r_s16(car + 0x316);
    w_u32(frame + 0x30, front_distance);
    rear_distance = (uint32)r_s16(descriptor + 0x2A);
    w_u32(frame + 0x28, rear_distance);
    inertia = r_u32(descriptor + 0x20);
    w_u32(frame + 0x88, inertia);
    value = (uint32)r_s16(descriptor + 0x2C);
    w_u32(frame + 0xD0, value);
    drive = (uint32)r_s16(descriptor + 0x76);
    value = (uint32)r_s16(descriptor + 0x7C);
    steering = (uint32)((sint32)steering / 256);
    w_u32(frame + 0xE8, value);
    value = (uint32)rcos((sint32)heading);
    w_u32(frame + 0x68, value);
    value = (uint32)rsin((sint32)heading);
    heading += steering;
    w_u32(frame + 0x70, value);
    value = (uint32)rcos((sint32)heading);
    w_u32(frame + 0x58, value);
    value = (uint32)rsin((sint32)heading);
    w_u32(frame + 0x60, value);
    value = (uint32)rcos((sint32)steering);
    w_u32(frame + 0x78, value);
    sine_steer = (uint32)rsin((sint32)steering);
    first = velocity_x * r_u32(frame + 0x68);
    second = velocity_z * r_u32(frame + 0x70);
    front_distance = r_u32(frame + 0x30);
    rear_distance = r_u32(frame + 0x28);
    distance = front_distance + rear_distance;
    lateral = (uint32)((sint32)(first - second) / 4096);
    first = velocity_x * r_u32(frame + 0x70);
    second = velocity_z * r_u32(frame + 0x68);
    w_u32(frame + 0x90, lateral);
    forward = (uint32)((sint32)(first + second) / 4096);
    rear_distance = r_u32(frame + 0x28);
    value = (uint32)td5_divide_checked((sint32)(rear_distance << 8), (sint32)distance);
    weight = r_u32(descriptor + 0x24);
    balance = r_u32(car + 0x2A4);
    value *= weight - balance;
    denominator = weight << 1;
    coefficient = (uint32)td5_divide_checked((sint32)value, (sint32)denominator);
    surface = r_u32(frame + 0x14);
    first = coefficient * (uint32)r_s16(0x8007DF3C + surface * 2);
    w_u32(frame + 0x98, forward);
    surface = r_u32(frame + 0x18);
    second = coefficient * (uint32)r_s16(0x8007DF3C + surface * 2);
    w_u32(frame + 0xA8, (uint32)((sint32)first / 256));
    front_distance = r_u32(frame + 0x30);
    value = (uint32)td5_divide_checked((sint32)(front_distance << 8), (sint32)distance);
    value *= weight + balance;
    coefficient = (uint32)td5_divide_checked((sint32)value, (sint32)denominator);
    surface = r_u32(frame + 0x1C);
    first = coefficient * (uint32)r_s16(0x8007DF3C + surface * 2);
    w_u32(frame + 0xB8, (uint32)((sint32)second / 256));
    surface = r_u32(frame + 0x20);
    second = coefficient * (uint32)r_s16(0x8007DF3C + surface * 2);
    w_u32(frame + 0xA0, (uint32)((sint32)first / 256));
    value = r_u32(frame + 0xA8);
    w_u32(frame + 0xB0, (uint32)((sint32)second / 256));
    for (offset = 0; offset < 4; ++offset)
    {
        uint32 grip_address = frame + (offset == 0 ? 0xA8 : offset == 1 ? 0xB8 : offset == 2 ? 0xA0 : 0xB0);
        if (offset != 0)
            value = r_u32(grip_address);
        if ((sint32)value < 56)
        {
            w_u32(grip_address, 56);
            value = r_u32(grip_address);
        }
        else if (offset == 0)
            value = r_u32(grip_address);
        if ((sint32)value >= 81)
            w_u32(grip_address, 80);
    }
    if (r_u8(car + 0x346) != 0)
    {
        value = (uint32)r_s16(descriptor + 0x7A);
        first = r_u32(frame + 0xA0);
        first = td5_physics_mul_div(first, value, 256);
        second = r_u32(frame + 0xB0);
        second = td5_physics_mul_div(second, value, 256);
        w_u32(frame + 0xA0, first);
        w_u32(frame + 0xB0, second);
    }
    first = velocity_x * r_u32(frame + 0x70);
    second = velocity_z * r_u32(frame + 0x68);
    forward = (uint32)((sint32)(first + second) / 4096);
    first = velocity_x * r_u32(frame + 0x60);
    second = velocity_z * r_u32(frame + 0x58);
    w_u32(frame + 0xC0, forward);
    first = (uint32)((sint32)(first + second) / 4096);
    yaw = r_u32(frame + 0x80);
    value = r_u32(frame + 0x30);
    second = yaw * value * sine_steer;
    brake = r_u8(car + 0x34E);
    first -= (uint32)((sint32)second / 2670592);
    w_u32(frame + 0xC8, first);
    limit_speed = 0;
    if (brake != 0)
    {
        if (drive == 1 || drive == 2 || drive == 3)
        {
            if (drive == 1)
            {
                rpm = r_u32(car + 0x2E8);
                gear = r_u8(car + 0x343);
                value = r_u32(frame + 0xC8);
                w_u32(car + 0x2F0, value);
                maximum = (uint32)r_s16(descriptor + 0x72);
            }
            else if (drive == 2)
            {
                gear = r_u8(car + 0x343);
                rpm = r_u32(car + 0x2E8);
                maximum = (uint32)r_s16(descriptor + 0x72);
            }
            else
            {
                first = r_u32(frame + 0xC8);
                second = r_u32(frame + 0xC0);
                rpm = r_u32(car + 0x2E8);
                gear = r_u8(car + 0x343);
                maximum = (uint32)r_s16(descriptor + 0x72);
                forward = (uint32)((sint32)(first + second) / 2);
            }
            synthetic = 0;
            if (gear != 1)
            {
                value = (rpm - 400) << 12;
                other = (uint32)r_s16(descriptor + gear * 2 + 0x2E);
                value = (uint32)((sint32)value / 45);
                synthetic = (uint32)td5_divide_checked((sint32)value, (sint32)other) << 8;
            }
            step = 200;
            if (r_s16(car + 0x316) > 0 && gear == 2)
            {
                if (drive != 3)
                    forward = r_u32(frame + (drive == 1 ? 0xC0 : 0xC8));
                value = (uint32)((sint32)forward / 64);
                target = maximum - 1800;
                if ((sint32)value >= 0)
                    target += value;
            }
            else
            {
                if (r_u8(car + 0x345) != 0)
                    step *= 2;
                step *= 2;
                target = 0;
            }
            if ((sint32)target < (sint32)rpm)
            {
                rpm -= step;
                if ((sint32)rpm < (sint32)target)
                    rpm = target;
            }
            else if ((sint32)rpm < (sint32)(target - step * 4))
                rpm += step;
            else
                rpm += (uint32)((sint32)(target - rpm) / 4);
            w_u32(car + 0x2E8, rpm);
            if (drive == 2 || drive == 3)
                w_u32(car + 0x2F0, synthetic);
            if (drive == 2)
                synthetic = r_u32(frame + 0xC0);
            w_u32(car + 0x2EC, synthetic);
        }
        if (r_u8(car + 0x345) != 0)
        {
            value = (uint32)r_s16(descriptor + 0x6E);
            value = td5_physics_mul_div(value, pedal, 256);
            other = r_u32(frame + 0xC8);
            first = 0 - value;
            second = 0 - other;
            if ((sint32)second < (sint32)first)
                first = second;
            if ((sint32)value < (sint32)first)
                value = first;
            physics_force_carrier = (uint32)((sint32)value / 2);
            w_u32(frame + 0x48, physics_force_carrier);
            w_u32(frame + 0x40, 0);
            w_u32(frame + 0x38, 0);
        }
        else
            goto engine_force;
    }
    else
    {
        value = r_u32(frame + 0xC8);
        brake = r_u8(car + 0x345);
        w_u32(car + 0x2F0, value);
        value = r_u32(frame + 0xC0);
        w_u32(car + 0x2EC, value);
        if (brake == 0 && pedal != 0)
        {
            if (r_u8(car + 0x350) != 0)
                td5_80025600(car, descriptor);
            else
                td5_800257e8(car);
            td5_80025340(car, descriptor);
            limit_speed = 1;
            goto engine_force;
        }
        if (pedal == 0)
            pedal = (uint32)-32;
        td5_80025340(car, descriptor);
        first = (uint32)r_s16(descriptor + 0x6E);
        first = td5_physics_mul_div(first, pedal, 256);
        second = (uint32)r_s16(descriptor + 0x70);
        second = td5_physics_mul_div(second, pedal, 256);
        if (r_u8(car + 0x346) != 0)
        {
            physics_force_carrier = 0;
            other = r_u32(frame + 0xC0);
            value = (uint32)((sint32)(0 - second) / 2);
            other = 0 - other;
            if ((sint32)other < (sint32)value)
                value = other;
            w_u32(frame + 0x48, 0);
        }
        else
        {
            other = r_u32(frame + 0xC8);
            value = 0 - first;
            other = (uint32)((sint32)(0 - other) / 2);
            if ((sint32)other < (sint32)value)
                value = other;
            if ((sint32)first < (sint32)value)
                first = value;
            other = r_u32(frame + 0xC0);
            physics_force_carrier = (uint32)((sint32)first / 2);
            value = 0 - second;
            other = (uint32)((sint32)(0 - other) / 2);
            w_u32(frame + 0x48, physics_force_carrier);
            if ((sint32)other < (sint32)value)
                value = other;
        }
        if ((sint32)second < (sint32)value)
            second = value;
        w_u32(frame + 0x38, (uint32)((sint32)second / 2));
        value = r_u32(frame + 0x38);
        w_u32(frame + 0x40, value);
    }
    goto weighted_force;

engine_force:
    if (drive == 1)
    {
        value = td5_80025810(car, descriptor);
        w_u32(frame + 0x38, (uint32)((sint32)value / 2));
        other = r_u32(frame + 0x38);
        physics_force_carrier = 0;
        w_u32(frame + 0x48, 0);
        w_u32(frame + 0x40, other);
    }
    else if (drive == 2)
    {
        w_u32(frame + 0x40, 0);
        w_u32(frame + 0x38, 0);
        value = td5_80025810(car, descriptor);
        physics_force_carrier = (uint32)((sint32)value / 2);
        w_u32(frame + 0x48, physics_force_carrier);
    }
    else if (drive == 3)
    {
        value = td5_80025810(car, descriptor);
        physics_force_carrier = (uint32)((sint32)value / 4);
        w_u32(frame + 0x48, physics_force_carrier);
        w_u32(frame + 0x40, physics_force_carrier);
        w_u32(frame + 0x38, physics_force_carrier);
    }
    if (limit_speed != 0)
    {
        value = (uint32)r_s16(descriptor + 0x74);
        other = r_u32(car + 0x2EC);
        if ((sint32)(value << 8) < (sint32)other)
        {
            physics_force_carrier = 0;
            w_u32(frame + 0x48, 0);
            w_u32(frame + 0x40, 0);
            w_u32(frame + 0x38, 0);
        }
    }

weighted_force:
    surface = r_u32(frame + 0x14);
    value = (uint32)r_s16(0x8007DF3C + surface * 2);
    first = td5_physics_mul_div(physics_force_carrier, value, 256);
    first_wheel = first;
    surface = r_u32(frame + 0x18);
    other = r_u32(frame + 0x48);
    value = (uint32)r_s16(0x8007DF3C + surface * 2);
    second = td5_physics_mul_div(other, value, 256);
    surface = r_u32(frame + 0x1C);
    other = r_u32(frame + 0x38);
    value = (uint32)r_s16(0x8007DF3C + surface * 2);
    third = td5_physics_mul_div(other, value, 256);
    w_u32(frame + 0x48, second);
    surface = r_u32(frame + 0x20);
    other = r_u32(frame + 0x40);
    value = (uint32)r_s16(0x8007DF3C + surface * 2);
    value = td5_physics_mul_div(other, value, 256);
    w_u32(frame + 0x38, third);
    rear_distance = r_u32(frame + 0x28);
    other = r_u32(frame + 0x28);
    second = r_u32(frame + 0x48);
    third = r_u32(frame + 0x38);
    front_force = first + second;
    inertia = r_u32(frame + 0x88);
    w_u32(frame + 0x40, value);
    rear_force = third + value;
    w_u32(frame + 0x50, rear_force);
    value = inertia + rear_distance * other;
    value = (uint32)((sint32)value / 1024);
    value = td5_physics_mul_div(value, sine_steer, 4096);
    value = td5_physics_mul_div(value, sine_steer, 4096);
    other = (uint32)((sint32)(distance * distance) / 1024);
    coefficient = r_u32(frame + 0x78);
    other = td5_physics_mul_div(other, coefficient, 4096);
    other = td5_physics_mul_div(other, coefficient, 4096);
    rear_distance = r_u32(frame + 0x28);
    front_distance = r_u32(frame + 0x30);
    inertia = r_u32(frame + 0x88);
    denominator = value + other;
    balance = rear_distance * front_distance - inertia;
    balance = (uint32)((sint32)balance / 1024);
    value = td5_physics_mul_div(balance, front_force, 4096);
    value = td5_physics_mul_div(value, sine_steer, 4096);
    first = td5_physics_mul_div(value, sine_steer, 4096);
    inertia = r_u32(frame + 0x88);
    rear_distance = r_u32(frame + 0x28);
    yaw = r_u32(frame + 0x80);
    lateral = r_u32(frame + 0x90);
    value = (uint32)((sint32)(rear_distance * yaw) / 652) - lateral;
    other = (uint32)((sint32)inertia / 1024);
    value = td5_physics_mul_div(other, value, 4096);
    second = td5_physics_mul_div(value, sine_steer, 4096);
    coefficient = r_u32(frame + 0x78);
    value = td5_physics_mul_div(front_force, coefficient, 4096);
    other = r_u32(frame + 0x50);
    forward = r_u32(frame + 0x98);
    value += other + forward;
    value = td5_physics_mul_div(balance, value, 4096);
    coefficient = r_u32(frame + 0x78);
    third = td5_physics_mul_div(value, coefficient, 4096);
    inertia = r_u32(frame + 0x88);
    yaw = r_u32(frame + 0x80);
    moment = yaw * (uint32)((sint32)inertia / 652);
    front_distance = r_u32(frame + 0x30);
    lateral = r_u32(frame + 0x90);
    numerator = (first + second + third) * sine_steer;
    value = (uint32)((sint32)(moment - front_distance * lateral) / 1024);
    value = td5_physics_mul_div(value, distance, 4096);
    coefficient = r_u32(frame + 0x78);
    value = td5_physics_mul_div(value, coefficient, 4096);
    rear_distance = r_u32(frame + 0x28);
    lateral = r_u32(frame + 0x90);
    coefficient = r_u32(frame + 0x78);
    numerator += value * coefficient;
    other = 0 - (rear_distance * lateral + moment);
    lateral_force = (uint32)td5_divide_checked((sint32)numerator, (sint32)denominator);
    value = (uint32)((sint32)other / 1024);
    value = td5_physics_mul_div(value, distance, 4096);
    front_distance = r_u32(frame + 0x30);
    rear_distance = r_u32(frame + 0x28);
    other = r_u32(frame + 0x30);
    inertia = r_u32(frame + 0x88);
    balance = rear_distance * (front_distance << 1) - inertia + other * front_distance;
    coefficient = r_u32(frame + 0x78);
    first = value * coefficient;
    value = (uint32)((sint32)balance / 1024);
    value = td5_physics_mul_div(value, front_force, 4096);
    rear_distance = r_u32(frame + 0x28);
    other = r_u32(frame + 0x28);
    inertia = r_u32(frame + 0x88);
    coefficient = r_u32(frame + 0x78);
    second = value * coefficient;
    value = (uint32)((sint32)(inertia + rear_distance * other) / 1024);
    forward = r_u32(frame + 0x98);
    rear_force = r_u32(frame + 0x50);
    second += value * (forward + rear_force);
    value = (uint32)((sint32)second / 4096);
    second = value * sine_steer;
    first += second;
    value = r_u32(frame + 0xA0);
    other = r_u32(frame + 0xB0);
    coefficient = r_u32(frame + 0xD0);
    capacity = td5_physics_mul_div(coefficient, value + other, 256);
    steered_force = (uint32)td5_divide_checked((sint32)first, (sint32)denominator);
    brake = r_u8(car + 0x34E);
    w_u32(frame + 0xD8, 0);
    w_u16(car + 0x314, 0);
    if ((brake & 1) != 0)
    {
        forward = r_u32(frame + 0xC0);
        value = r_u32(car + 0x2EC);
        if ((sint32)forward < 0 ? (sint32)value >= 0 : (sint32)(value - forward) >= 0)
        {
            other = r_u32(frame + 0xC0);
            value = r_u32(car + 0x2EC);
            if ((sint32)other >= 0)
                value -= other;
        }
        else
        {
            if ((sint32)forward < 0)
                forward = r_u32(frame + 0xC0);
            value = r_u32(car + 0x2EC);
            value = (sint32)forward < 0 ? 0 - value : forward - value;
        }
        slip = (uint32)((sint32)value / 256);
        if ((sint32)slip < 65)
            w_u8(car + 0x34E, 0);
        value = r_u32(frame + 0xE8);
        value *= slip;
        w_u16(car + 0x314, slip);
        slip = (uint32)((sint32)value / 256);
        coefficient = (uint32)((sint32)lateral_force / 256);
        value = td5_physics_mul_div(coefficient, lateral_force, 256) + slip * slip;
        root = (uint32)SquareRoot0((sint32)value);
        if ((sint32)coefficient < 0)
            coefficient = 0 - coefficient;
        root += 1;
        capacity = (uint32)td5_divide_checked((sint32)(capacity * coefficient), (sint32)root);
        value = r_u32(frame + 0x50);
        value = (uint32)((sint32)value / 2) * slip;
        value = (uint32)td5_divide_checked((sint32)value, (sint32)root);
        w_u32(frame + 0x50, value);
    }
    first = (uint32)((sint32)lateral_force / 16);
    value = r_u32(frame + 0x50);
    second = (uint32)((sint32)value / 16);
    root = (uint32)SquareRoot0((sint32)(first * first + second * second)) << 4;
    if ((sint32)capacity < (sint32)root)
    {
        value = (uint32)td5_divide_checked((sint32)(capacity << 8), (sint32)root);
        value *= lateral_force;
        w_u32(frame + 0xD8, root - capacity);
        lateral_force = (uint32)((sint32)value / 256);
    }
    first = r_u32(frame + 0xA8);
    second = r_u32(frame + 0xB8);
    coefficient = r_u32(frame + 0xD0);
    residual = r_u32(frame + 0xD8);
    w_u32(car + 0x2F4, residual);
    value = coefficient * (first + second);
    w_u32(frame + 0xE0, 0);
    brake = r_u8(car + 0x34E);
    capacity = (uint32)((sint32)value / 256);
    if ((brake & 2) != 0)
    {
        forward = r_u32(frame + 0xC8);
        value = r_u32(car + 0x2F0);
        if ((sint32)forward < 0 ? (sint32)value >= 0 : (sint32)(value - forward) >= 0)
        {
            other = r_u32(frame + 0xC8);
            value = r_u32(car + 0x2F0);
            if ((sint32)other >= 0)
                value -= other;
        }
        else
        {
            if ((sint32)forward < 0)
                forward = r_u32(frame + 0xC8);
            value = r_u32(car + 0x2F0);
            value = (sint32)forward < 0 ? 0 - value : forward - value;
        }
        slip = (uint32)((sint32)value / 256);
        if ((sint32)slip < 65)
            w_u8(car + 0x34E, 0);
        value = r_u32(frame + 0xE8);
        value *= slip;
        w_u16(car + 0x314, slip);
        slip = (uint32)((sint32)value / 256);
        coefficient = (uint32)((sint32)steered_force / 256);
        value = td5_physics_mul_div(coefficient, steered_force, 256) + slip * slip;
        root = (uint32)SquareRoot0((sint32)value);
        if ((sint32)coefficient < 0)
            coefficient = 0 - coefficient;
        root += 1;
        capacity = (uint32)td5_divide_checked((sint32)(capacity * coefficient), (sint32)root);
        value = (uint32)((sint32)front_force / 2) * slip;
        front_force = (uint32)td5_divide_checked((sint32)value, (sint32)root);
    }
    first = (uint32)((sint32)steered_force / 16);
    second = (uint32)((sint32)front_force / 16);
    root = (uint32)SquareRoot0((sint32)(first * first + second * second)) << 4;
    if ((sint32)capacity < (sint32)root)
    {
        value = (uint32)td5_divide_checked((sint32)(capacity << 8), (sint32)root);
        value *= steered_force;
        w_u32(frame + 0xE0, root - capacity);
        steered_force = (uint32)((sint32)value / 256);
    }
    value = r_u32(frame + 0x68);
    residual = r_u32(frame + 0xE0);
    first = lateral_force * value;
    w_u32(car + 0x2F8, residual);
    first = (uint32)((sint32)first / 4096);
    value = r_u32(frame + 0x70);
    second = td5_physics_mul_div(0 - lateral_force, value, 4096);
    value = r_u32(frame + 0x58);
    third = td5_physics_mul_div(steered_force, value, 4096);
    value = r_u32(frame + 0x60);
    other = td5_physics_mul_div(0 - steered_force, value, 4096);
    front_distance = r_u32(frame + 0x30);
    coefficient = r_u32(frame + 0x78);
    value = td5_physics_mul_div(front_distance, coefficient, 4096);
    numerator = steered_force * value;
    rear_distance = r_u32(frame + 0x28);
    value = r_u32(frame + 0x48);
    coefficient = r_u32(frame + 0x40);
    value += coefficient;
    coefficient = r_u32(frame + 0x38);
    numerator -= lateral_force * rear_distance;
    value = value - first_wheel - coefficient;
    numerator += value * 500;
    inertia = r_u32(frame + 0x88);
    value = (uint32)((sint32)inertia / 652);
    rotation = (uint32)td5_divide_checked((sint32)numerator, (sint32)value);
    if ((sint32)rotation > 1400)
        rotation = 1400;
    if ((sint32)rotation < -1400)
        rotation = (uint32)-1400;
    value = r_u32(frame + 0x50);
    coefficient = r_u32(frame + 0x70);
    acceleration_x = first + third + td5_physics_mul_div(value, coefficient, 4096);
    coefficient = r_u32(frame + 0x60);
    acceleration_x += td5_physics_mul_div(front_force, coefficient, 4096);
    value = r_u32(frame + 0x50);
    coefficient = r_u32(frame + 0x68);
    acceleration_z = second + other + td5_physics_mul_div(value, coefficient, 4096);
    coefficient = r_u32(frame + 0x58);
    acceleration_z += td5_physics_mul_div(front_force, coefficient, 4096);
    tick = r_u32(0x80088100);
    rotation_step = td5_physics_mul_div(rotation, tick, 256);
    value = r_u32(car + 0x18C);
    w_u32(car + 0x18C, value + rotation_step);
    value = r_u32(car + 0x194);
    w_u32(car + 0x194, value + td5_physics_mul_div(acceleration_x, tick, 256));
    value = r_u32(car + 0x2F0);
    rotation_step = td5_physics_mul_div((uint32)((sint32)value >> 8), tick, 256);
    value = r_u32(car + 0x19C);
    w_u32(car + 0x19C, value + td5_physics_mul_div(acceleration_z, tick, 256));
    value = r_u16(car + 0x318);
    brake = r_u8(car + 0x346);
    w_u16(car + 0x318, value + rotation_step);
    if (brake == 0)
    {
        value = r_u32(car + 0x2EC);
        rotation_step = td5_physics_mul_div((uint32)((sint32)value >> 8), tick, 256);
        value = r_u16(car + 0x31A);
        w_u16(car + 0x31A, value + rotation_step);
    }
    residual = r_u32(frame + 0xD8);
    if ((sint32)residual > 0)
    {
        rotation = r_u32(car + 0x18C);
        tick = r_u32(0x80088100);
        value = td5_physics_mul_div((uint32)((sint32)rotation / 64), tick, 256);
        residual = r_u32(frame + 0xD8);
        value = td5_physics_mul_div(value, residual, 32768);
        if ((sint32)value > 512)
            value = 512;
        if ((sint32)value < -512)
            value = (uint32)-512;
        w_u32(car + 0x18C, rotation - value);
    }
    if (r_u8(car + 0x343) == 2)
    {
        value = (r_u32(car + 0x2E8) - 400) << 12;
        other = (uint32)r_s16(descriptor + 0x32);
        value = (uint32)((sint32)value / 45);
        value = (uint32)td5_divide_checked((sint32)value, (sint32)other);
        other = r_u32(frame + 0xC0);
        if ((sint32)((value << 8) - other) > 76800 && r_s16(car + 0x316) >= 128)
            w_u8(car + 0x34E, r_u16(descriptor + 0x76));
    }
    if (r_u32(car + 0x194) + 63 < 127 && r_u32(car + 0x19C) + 63 < 127 && r_u32(car + 0x18C) + 31 < 63)
    {
        w_u32(car + 0x194, 0);
        w_u32(car + 0x19C, 0);
        w_u32(car + 0x18C, 0);
    }
    td5_80020428(car, descriptor, acceleration_x, acceleration_z);
    return td5_80020c88(car);
}

uint32 td5_80064110(sint32 mode)
{
    FUNCTION_MARKER(0x80064110, "SLUS_006.10");
    return (uint32)SetVideoMode(mode);
}

void td5_80065990(void)
{
    FUNCTION_MARKER(0x80065990, "SLUS_006.10");
    SpuQuit();
}

void td5_80068190(void)
{
    FUNCTION_MARKER(0x80068190, "SLUS_006.10");
    xport_bios_stop_card();
}

uint32 td5_80068144(void)
{
    FUNCTION_MARKER(0x80068144, "SLUS_006.10");
    psx_bios_bind_card_end_patch(GetC0Table(), 0x800683E0);
    return (uint32)MemCardEndPSX();
}

static uint32 td5_sky_add_checked(uint32 left, uint32 right, uint32 pc)
{
    sint64 result = (sint64)(sint32)left + (sint64)(sint32)right;

    if (result < (-2147483647 - (sint64)1) || result > 2147483647)
        xport_mips_overflow_exception(pc);
    return (uint32)result;
}

static uint32 td5_sky_clip_mask(uint32 packet_coordinate)
{
    sint32 x = r_s16(packet_coordinate);
    sint32 y = r_s16(packet_coordinate + 2);
    uint32 mask = 15;

    if (x >= 0)
    {
        mask &= 14;
        if (x < 512)
            mask &= 11;
    }
    if (y >= 0)
    {
        mask &= 13;
        if (y < 240)
            mask &= 7;
    }
    return mask;
}

/* Render the two primitive layouts used by the original sky resources */
uint32 td5_80071e0c(uint32 object, uint32 visibility)
{
    uint32 stream_offset, buffer, packet_offset, vertex_offset, primitive, vertices;
    uint32 packet, ordering, limit, count, kind, target, value, index, vertex, fourth;
    uint32 first_index, second_index, third_index, mask, enabled, previous;

    FUNCTION_MARKER(0x80071E0C, "SLUS_006.10");
    stream_offset = r_u32(object + 0x30);
    buffer = r_u32(0x80088104);
    primitive = object + stream_offset;
    packet_offset = r_u32(object + 0x34 + buffer * 4);
    vertex_offset = r_u32(object + 0x2C);
    packet = object + packet_offset;
    vertices = object + vertex_offset;
    ordering = r_u32(0x800880F0);
    limit = r_u32(0x8008857C);
    ordering = r_u32(ordering + 0x70);
    limit = td5_sky_add_checked(limit, (uint32)-1, 0x80071E48);
    ordering += limit * 4;
    for (;;)
    {
        count = (uint32)r_s16(primitive);
        kind = r_u8(primitive + 2);
        value = kind * 4;
        if (count == 0)
            return value;
        target = r_u32(0x80071E04 + value);
        primitive += 4;
        if (target != 0x80071E68 && target != 0x80071EFC)
        {
            {
                char message[256];
                snprintf(message, sizeof(message), "Unresolved TD5 sky dispatch %08x\n", target);
                xport_message_error("Test Drive 5", message);
            }
            abort();
        }
        do
        {
            enabled = (uint32)r_s8(visibility);
            count = td5_sky_add_checked(count, (uint32)-1,
                target == 0x80071E68 ? 0x80071E6C : 0x80071F00);
            visibility = td5_sky_add_checked(visibility, 1,
                target == 0x80071E68 ? 0x80071E74 : 0x80071F08);
            if (enabled != 0)
            {
                first_index = (uint32)(sint32)r_s16(primitive);
                second_index = (uint32)(sint32)r_s16(primitive + 2);
                vertex = vertices + first_index * 8;
                xport_gte_write_data(0, r_u32(vertex));
                xport_gte_write_data(1, r_u32(vertex + 4));
                vertex = vertices + second_index * 8;
                third_index = (uint32)(sint32)r_s16(primitive + 4);
                xport_gte_write_data(2, r_u32(vertex));
                xport_gte_write_data(3, r_u32(vertex + 4));
                vertex = vertices + third_index * 8;
                if (target == 0x80071EFC)
                    fourth = vertices + (uint32)(sint32)r_s16(primitive + 6) * 8;
                xport_gte_write_data(4, r_u32(vertex));
                xport_gte_write_data(5, r_u32(vertex + 4));
                xport_gte_execute(0x280030);
                if (target == 0x80071E68)
                {
                    w_u32(packet + 8, xport_gte_read_data(12));
                    w_u32(packet + 16, xport_gte_read_data(13));
                    w_u32(packet + 24, xport_gte_read_data(14));
                    previous = r_u32(ordering);
                    w_u32(ordering, packet & 0xFFFFFF);
                    w_u32(packet, 0x07000000 | (previous & 0xFFFFFF));
                }
                else
                {
                    w_u32(packet + 8, xport_gte_read_data(12));
                    mask = td5_sky_clip_mask(packet + 8);
                    w_u32(packet + 16, xport_gte_read_data(13));
                    value = td5_sky_clip_mask(packet + 16);
                    xport_gte_write_data(0, r_u32(fourth));
                    xport_gte_write_data(1, r_u32(fourth + 4));
                    w_u32(packet + 24, xport_gte_read_data(14));
                    index = td5_sky_clip_mask(packet + 24);
                    xport_gte_execute(0x180001);
                    w_u32(packet + 32, xport_gte_read_data(14));
                    mask &= value & index & td5_sky_clip_mask(packet + 32);
                    if (mask == 0)
                    {
                        previous = r_u32(ordering);
                        w_u32(ordering, packet & 0xFFFFFF);
                        w_u32(packet, 0x09000000 | (previous & 0xFFFFFF));
                    }
                }
            }
            primitive += 20;
            packet += target == 0x80071E68 ? 32 : 40;
        } while (count != 0);
    }
}

/* Bind reviewed entrypoints without treating an unknown target as success */
static sint32 td5_dispatch_known_model(uint32 target, uint32 object, uint32 original_stack, uint32 return_site, uint32 *result)
{
    uint64 wrapper_result;
    uint32 continuation;

    switch (target)
    {
    case 0x80071544:
        *result = td5_80071544(object);
        break;
    case 0x8007196C:
        *result = td5_8007196c(object);
        break;
    case 0x80076F44:
        *result = td5_80076f44(object, original_stack);
        break;
    case 0x800714A0:
        wrapper_result = td5_800714a0(object, original_stack, return_site);
        continuation = (uint32)(wrapper_result >> 32);
        if (continuation != return_site)
        {
            {
                char message[256];
                snprintf(message, sizeof(message), "Unresolved TD5 renderer continuation %08x from %08x\n",
                continuation, return_site);
                xport_message_error("Test Drive 5", message);
            }
            abort();
        }
        *result = (uint32)wrapper_result;
        break;
    case 0x8006DBB4:
        *result = td5_8006dbb4(object, original_stack);
        break;
    case 0x80070020:
        *result = td5_80070020(object);
        break;
    case 0x80070704:
        *result = td5_80070704(object, original_stack);
        break;
    case 0x80070ED0:
        *result = td5_80070ed0(object);
        break;
    default:
        return 0;
    }
    return 1;
}

uint32 td5_call_model_renderer(uint32 target, uint32 object, uint32 original_stack, uint32 published_type)
{
    uint32 result;

    if (td5_dispatch_known_model(target, object, original_stack, 0x8006E1B0, &result))
        return result;
    {
        char message[256];
        snprintf(message, sizeof(message), "Unresolved TD5 model dispatch %08x, object %08x, type %08x\n",
        target, object, published_type);
        xport_message_error("Test Drive 5", message);
    }
    abort();
}

uint32 td5_call_distance_renderer(uint32 target, uint32 object, uint32 original_stack, uint32 x_squared, uint32 y_squared, uint32 radius_squared)
{
    uint32 result;

    if (td5_dispatch_known_model(target, object, original_stack, 0x80071480, &result))
        return result;
    {
        char message[256];
        snprintf(message, sizeof(message), "Unresolved TD5 distance dispatch %08x, object %08x, operands %08x/%08x/%08x\n",
        target, object, x_squared, y_squared, radius_squared);
        xport_message_error("Test Drive 5", message);
    }
    abort();
}

/* Select the original near or far renderer using wrapped distance products */
uint32 td5_80071378(uint32 object, uint32 original_stack)
{
    uint32 frame = original_stack - 0x28;
    uint32 position, offset, camera, x, y, z, radius, distance, target, table;
    uint32 x_squared, y_squared, z_squared, radius_squared, index;

    FUNCTION_MARKER(0x80071378, "SLUS_006.10");
    position = r_u32(object + 0x10);
    offset = r_u32(object + 0x1C);
    camera = r_u32(0x8008C4F8);
    x = (uint32)((sint32)((uint32)((sint32)position >> 8) + offset - camera) >> 4);
    x_squared = x * x;
    w_u32(frame + 0x10, x);
    position = r_u32(object + 0x14);
    offset = r_u32(object + 0x20);
    camera = r_u32(0x8008C4FC);
    y = (uint32)((sint32)((uint32)((sint32)position >> 8) + offset - camera) >> 4);
    y_squared = y * y;
    w_u32(frame + 0x14, y);
    position = r_u32(object + 0x18);
    offset = r_u32(object + 0x24);
    camera = r_u32(0x8008C500);
    z = (uint32)((sint32)((uint32)((sint32)position >> 8) + offset - camera) >> 4);
    z_squared = z * z;
    radius = r_u32(0x80088344);
    w_u32(frame + 0x18, z);
    offset = r_u16(object + 8);
    radius = (uint32)((sint32)(offset << 16) >> 20) + (uint32)((sint32)radius >> 4);
    radius_squared = radius * radius;
    distance = x_squared + y_squared + z_squared;
    table = (sint32)distance < (sint32)radius_squared ? 0x80086D4C : 0x80086D14;
    index = (uint32)(sint32)r_s16(object + 0xA);
    target = r_u32(table + index * 4);
    return td5_call_distance_renderer(target, object, frame, x_squared, y_squared, radius_squared);
}

/* Publish the model and select its original format renderer */
uint32 td5_8006e180(uint32 object, uint32 original_stack)
{
    uint32 frame = original_stack - 0x18;
    uint32 index, published_type, target;

    FUNCTION_MARKER(0x8006E180, "SLUS_006.10");
    index = (uint32)(sint32)r_s16(object + 0xA);
    published_type = (uint32)(sint32)r_s16(object + 0xA);
    target = r_u32(0x80086CDC + index * 4);
    w_u32(0x80088378, object);
    w_u32(0x80088708, published_type);
    return td5_call_model_renderer(target, object, frame, published_type);
}

/* Render the four primitive layouts in the original model format */
uint32 td5_80071544(uint32 object)
{
    uint32 stream_offset, buffer, shift, lod, primitive, packet, vertices, offset;
    uint32 ordering, ordering_root, limit, result, count, kind, target, previous;
    uint32 first, second, third, fourth, next_first, next_second, next_third;
    uint32 first_index, second_index, third_index, fourth_index, packet_tag, depth;
    uint32 triangle, colored, ordering_slot, packet_step, stream_step, decrement_pc;
    sint32 area, second_area;
    sint64 shift_difference;

    FUNCTION_MARKER(0x80071544, "SLUS_006.10");
    stream_offset = r_u32(object + 0x30);
    buffer = r_u32(0x80088104);
    shift = r_u32(0x80088110);
    lod = r_u32(0x80088164);
    primitive = object + stream_offset;
    offset = r_u32(object + 0x34 + buffer * 4);
    vertices = r_u32(object + 0x2C);
    packet = object + offset;
    vertices += object;
    offset = (uint32)(sint32)r_s16(object + 0xC);
    ordering_root = r_u32(0x800880F0);
    shift_difference = (sint64)(sint32)shift - (sint64)(sint32)lod;
    if (shift_difference < (-2147483647 - (sint64)1) || shift_difference > 2147483647)
        xport_mips_overflow_exception(0x80071588);
    shift = (uint32)shift_difference;
    offset = (uint32)((sint32)offset >> (shift & 31));
    ordering = r_u32(ordering_root + 0x70);
    ordering += offset * 4;
    result = r_u32(0x80088140) >> (lod & 31);
    limit = (uint32)((sint32)result >> 2);
    for (;;)
    {
        count = (uint32)(sint32)r_s16(primitive);
        kind = r_u8(primitive + 2);
        if (count == 0)
            return result;
        target = r_u32(0x80071490 + kind * 4);
        result = target;
        primitive += 4;
        if (target != 0x800715B8 && target != 0x80071670 &&
            target != 0x80071770 && target != 0x80071828)
        {
            {
                char message[256];
                snprintf(message, sizeof(message), "Unresolved TD5 primitive dispatch %08x\n", target);
                xport_message_error("Test Drive 5", message);
            }
            abort();
        }
        triangle = target == 0x800715B8 || target == 0x80071770;
        colored = target == 0x80071770 || target == 0x80071828;
        stream_step = triangle ? (colored ? 44 : 20) : (colored ? 56 : 20);
        packet_step = triangle ? (colored ? 40 : 32) : (colored ? 52 : 40);
        packet_tag = triangle ? (colored ? 0x09000000 : 0x07000000) :
            (colored ? 0x0C000000 : 0x09000000);
        decrement_pc = triangle ? (colored ? 0x800717C8 : 0x80071610) :
            (colored ? 0x80071898 : 0x800716E0);
        if (!triangle)
        {
            first_index = (uint32)(sint32)r_s16(primitive);
            second_index = (uint32)(sint32)r_s16(primitive + 2);
            third_index = (uint32)(sint32)r_s16(primitive + 4);
            first = vertices + first_index * 8;
            second = vertices + second_index * 8;
            third = vertices + third_index * 8;
        }
        do
        {
            if (triangle)
            {
                first_index = (uint32)(sint32)r_s16(primitive);
                second_index = (uint32)(sint32)r_s16(primitive + 2);
                third_index = (uint32)(sint32)r_s16(primitive + 4);
                first = vertices + first_index * 8;
                second = vertices + second_index * 8;
                third = vertices + third_index * 8;
            }
            xport_gte_write_data(0, r_u32(first));
            xport_gte_write_data(1, r_u32(first + 4));
            xport_gte_write_data(2, r_u32(second));
            xport_gte_write_data(3, r_u32(second + 4));
            xport_gte_write_data(4, r_u32(third));
            xport_gte_write_data(5, r_u32(third + 4));
            if (!triangle)
            {
                fourth_index = (uint32)(sint32)r_s16(primitive + 6);
                primitive += stream_step;
            }
            xport_gte_execute(0x280030);
            if (!triangle)
            {
                first_index = (uint32)(sint32)r_s16(primitive);
                second_index = (uint32)(sint32)r_s16(primitive + 2);
                third_index = (uint32)(sint32)r_s16(primitive + 4);
                next_first = vertices + first_index * 8;
            }
            xport_gte_execute(0x1400006);
            if (!triangle)
                fourth = vertices + fourth_index * 8;
            area = (sint32)xport_gte_read_data(24);
            if (triangle)
            {
                count = td5_sky_add_checked(count, (uint32)-1, decrement_pc);
                xport_gte_execute(0x158002D);
                if (area <= 0)
                {
                    depth = xport_gte_read_data(7);
                    result = packet_tag;
                    if ((sint32)depth > 0)
                    {
                        w_u32(packet + 8, xport_gte_read_data(12));
                        if ((sint32)depth < (sint32)limit)
                        {
                            w_u32(packet + (colored ? 20 : 16), xport_gte_read_data(13));
                            w_u32(packet + (colored ? 32 : 24), xport_gte_read_data(14));
                            ordering_slot = ordering + (uint32)((sint32)depth >> (shift & 31)) * 4;
                            previous = r_u32(ordering_slot);
                            w_u32(ordering_slot, packet & 0xFFFFFF);
                            result |= previous & 0xFFFFFF;
                            w_u32(packet, result);
                        }
                    }
                }
                primitive += stream_step;
            }
            else
            {
                w_u32(packet + 8, xport_gte_read_data(12));
                count = td5_sky_add_checked(count, (uint32)-1, decrement_pc);
                w_u32(packet + (colored ? 20 : 16), xport_gte_read_data(13));
                w_u32(packet + (colored ? 32 : 24), xport_gte_read_data(14));
                xport_gte_write_data(0, r_u32(fourth));
                xport_gte_write_data(1, r_u32(fourth + 4));
                result = packet_tag;
                xport_gte_execute(0x180001);
                next_second = vertices + second_index * 8;
                next_third = vertices + third_index * 8;
                if (area > 0)
                {
                    xport_gte_execute(0x1400006);
                    second_area = (sint32)xport_gte_read_data(24);
                }
                else
                    second_area = 0;
                xport_gte_execute(0x168002E);
                if (area <= 0 || second_area >= 0)
                {
                    depth = xport_gte_read_data(7);
                    w_u32(packet + (colored ? 44 : 32), xport_gte_read_data(14));
                    if ((sint32)depth > 0 && (sint32)depth < (sint32)limit)
                    {
                        ordering_slot = ordering + (uint32)((sint32)depth >> (shift & 31)) * 4;
                        previous = r_u32(ordering_slot);
                        w_u32(ordering_slot, packet & 0xFFFFFF);
                        result |= previous & 0xFFFFFF;
                        w_u32(packet, result);
                    }
                }
                first = next_first;
                second = next_second;
                third = next_third;
            }
            packet += packet_step;
        } while (count != 0);
    }
}

/* Preserve the guest rotation words and return continuation */
uint64 td5_800714a0(uint32 object, uint32 original_stack, uint32 return_address)
{
    uint32 saved_rotation = original_stack - 0x18;
    uint32 first, second, third, result, continuation;

    FUNCTION_MARKER(0x800714A0, "SLUS_006.10");
    w_u32(original_stack - 4, return_address);
    first = xport_gte_read_control(0);
    second = xport_gte_read_control(1);
    w_u32(saved_rotation, first);
    w_u32(saved_rotation + 4, second);
    first = xport_gte_read_control(2);
    second = xport_gte_read_control(3);
    third = xport_gte_read_control(4);
    w_u32(saved_rotation + 8, first);
    w_u32(saved_rotation + 12, second);
    w_u32(saved_rotation + 16, third);
    first = r_u32(0x800A3B90);
    second = r_u32(0x800A3B94);
    xport_gte_write_control(0, first);
    xport_gte_write_control(1, second);
    first = r_u32(0x800A3B98);
    second = r_u32(0x800A3B9C);
    third = r_u32(0x800A3BA0);
    xport_gte_write_control(2, first);
    xport_gte_write_control(3, second);
    xport_gte_write_control(4, third);
    result = td5_80071544(object);
    first = r_u32(saved_rotation);
    second = r_u32(saved_rotation + 4);
    xport_gte_write_control(0, first);
    xport_gte_write_control(1, second);
    first = r_u32(saved_rotation + 8);
    second = r_u32(saved_rotation + 12);
    third = r_u32(saved_rotation + 16);
    xport_gte_write_control(2, first);
    xport_gte_write_control(3, second);
    xport_gte_write_control(4, third);
    continuation = r_u32(original_stack - 4);
    return ((uint64)continuation << 32) | result;
}

/* Apply the four lighting layouts after the primitive-table prefix */
uint32 td5_8007196c(uint32 object)
{
    uint32 stream_offset, buffer, shift, lod, primitive, packet, vertices, offset;
    uint32 ordering, ordering_root, limit, result, count, kind, target, previous;
    uint32 first, second, third, fourth, next_first, next_second, next_third;
    uint32 first_index, second_index, third_index, fourth_index, packet_tag, depth;
    uint32 triangle, colored, ordering_slot, packet_step, stream_step, decrement_pc;
    sint32 area, second_area;
    sint64 shift_difference;

    FUNCTION_MARKER(0x8007196C, "SLUS_006.10");
    stream_offset = r_u32(object + 0x30);
    buffer = r_u32(0x80088104);
    shift = r_u32(0x80088110);
    lod = r_u32(0x80088164);
    primitive = object + stream_offset;
    offset = r_u32(object + 0x34 + buffer * 4);
    vertices = r_u32(object + 0x2C);
    packet = object + offset;
    vertices += object;
    offset = (uint32)(sint32)r_s16(object + 0xC);
    ordering_root = r_u32(0x800880F0);
    shift_difference = (sint64)(sint32)shift - (sint64)(sint32)lod;
    if (shift_difference < (-2147483647 - (sint64)1) || shift_difference > 2147483647)
        xport_mips_overflow_exception(0x800719B0);
    shift = (uint32)shift_difference;
    offset = (uint32)((sint32)offset >> (shift & 31));
    ordering = r_u32(ordering_root + 0x70);
    ordering += offset * 4;
    result = r_u32(0x80088140) >> (lod & 31);
    limit = (uint32)((sint32)result >> 2);
    for (;;)
    {
        count = (uint32)(sint32)r_s16(primitive);
        kind = r_u8(primitive + 2);
        if (count == 0)
            return result;
        target = r_u32(0x8007195C + kind * 4);
        result = target;
        primitive += 4;
        if (target != 0x800719E0 && target != 0x80071AA8 &&
            target != 0x80071BB8 && target != 0x80071C98)
        {
            {
                char message[256];
                snprintf(message, sizeof(message), "Unresolved TD5 lighting dispatch %08x\n", target);
                xport_message_error("Test Drive 5", message);
            }
            abort();
        }
        triangle = target == 0x800719E0 || target == 0x80071BB8;
        colored = target == 0x80071BB8 || target == 0x80071C98;
        stream_step = triangle ? (colored ? 44 : 20) : (colored ? 56 : 20);
        packet_step = triangle ? (colored ? 40 : 32) : (colored ? 52 : 40);
        packet_tag = triangle ? (colored ? 0x09000000 : 0x07000000) :
            (colored ? 0x0C000000 : 0x09000000);
        decrement_pc = triangle ? (colored ? 0x80071C10 : 0x80071A38) :
            (colored ? 0x80071D08 : 0x80071B18);
        if (!triangle)
        {
            first_index = (uint32)(sint32)r_s16(primitive);
            second_index = (uint32)(sint32)r_s16(primitive + 2);
            third_index = (uint32)(sint32)r_s16(primitive + 4);
            first = vertices + first_index * 8;
            second = vertices + second_index * 8;
            third = vertices + third_index * 8;
        }
        do
        {
            if (triangle)
            {
                first_index = (uint32)(sint32)r_s16(primitive);
                second_index = (uint32)(sint32)r_s16(primitive + 2);
                third_index = (uint32)(sint32)r_s16(primitive + 4);
                first = vertices + first_index * 8;
                second = vertices + second_index * 8;
                third = vertices + third_index * 8;
            }
            xport_gte_write_data(0, r_u32(first));
            xport_gte_write_data(1, r_u32(first + 4));
            xport_gte_write_data(2, r_u32(second));
            xport_gte_write_data(3, r_u32(second + 4));
            xport_gte_write_data(4, r_u32(third));
            xport_gte_write_data(5, r_u32(third + 4));
            if (!triangle)
            {
                fourth_index = (uint32)(sint32)r_s16(primitive + 6);
                primitive += stream_step;
            }
            xport_gte_execute(0x280030);
            if (!triangle)
            {
                first_index = (uint32)(sint32)r_s16(primitive);
                second_index = (uint32)(sint32)r_s16(primitive + 2);
                third_index = (uint32)(sint32)r_s16(primitive + 4);
                next_first = vertices + first_index * 8;
            }
            xport_gte_execute(0x1400006);
            if (!triangle)
                fourth = vertices + fourth_index * 8;
            area = (sint32)xport_gte_read_data(24);
            if (triangle)
            {
                count = td5_sky_add_checked(count, (uint32)-1, decrement_pc);
                xport_gte_execute(0x158002D);
                if (area <= 0)
                {
                    depth = xport_gte_read_data(7);
                    result = packet_tag;
                    w_u32(packet + 8, xport_gte_read_data(12));
                    if ((sint32)depth < (sint32)limit)
                    {
                        w_u32(packet + (colored ? 20 : 16), xport_gte_read_data(13));
                        w_u32(packet + (colored ? 32 : 24), xport_gte_read_data(14));
                        ordering_slot = ordering + (uint32)((sint32)depth >> (shift & 31)) * 4;
                        previous = r_u32(ordering_slot);
                        xport_gte_write_data(6, r_u32(primitive + 8));
                        if (colored)
                        {
                            xport_gte_write_data(0, r_u32(primitive + 20));
                            xport_gte_write_data(1, r_u32(primitive + 24));
                            xport_gte_write_data(2, r_u32(primitive + 28));
                            xport_gte_write_data(3, r_u32(primitive + 32));
                            xport_gte_write_data(4, r_u32(primitive + 36));
                            xport_gte_write_data(5, r_u32(primitive + 40));
                        }
                        else
                        {
                            xport_gte_write_data(0, r_u32(primitive + 12));
                            xport_gte_write_data(1, r_u32(primitive + 16));
                        }
                        w_u32(ordering_slot, packet & 0xFFFFFF);
                        xport_gte_execute(colored ? 0x118043F : 0x108041B);
                        result |= previous & 0xFFFFFF;
                        w_u32(packet, result);
                        if (colored)
                        {
                            w_u32(packet + 4, xport_gte_read_data(20));
                            w_u32(packet + 16, xport_gte_read_data(21));
                            w_u32(packet + 28, xport_gte_read_data(22));
                        }
                        else
                            w_u32(packet + 4, xport_gte_read_data(22));
                    }
                }
                primitive += stream_step;
            }
            else
            {
                w_u32(packet + 8, xport_gte_read_data(12));
                count = td5_sky_add_checked(count, (uint32)-1, decrement_pc);
                w_u32(packet + (colored ? 20 : 16), xport_gte_read_data(13));
                w_u32(packet + (colored ? 32 : 24), xport_gte_read_data(14));
                xport_gte_write_data(0, r_u32(fourth));
                xport_gte_write_data(1, r_u32(fourth + 4));
                result = packet_tag;
                xport_gte_execute(0x180001);
                next_second = vertices + second_index * 8;
                next_third = vertices + third_index * 8;
                if (area > 0)
                {
                    xport_gte_execute(0x1400006);
                    second_area = (sint32)xport_gte_read_data(24);
                }
                else
                    second_area = 0;
                xport_gte_execute(0x168002E);
                if (area <= 0 || second_area >= 0)
                {
                    depth = xport_gte_read_data(7);
                    w_u32(packet + (colored ? 44 : 32), xport_gte_read_data(14));
                    if (colored)
                        xport_gte_write_data(6, r_u32(primitive - 48));
                    if ((sint32)depth < (sint32)limit)
                    {
                        if (colored)
                        {
                            xport_gte_write_data(0, r_u32(primitive - 32));
                            xport_gte_write_data(1, r_u32(primitive - 28));
                            xport_gte_write_data(2, r_u32(primitive - 24));
                            xport_gte_write_data(3, r_u32(primitive - 20));
                            xport_gte_write_data(4, r_u32(primitive - 16));
                            xport_gte_write_data(5, r_u32(primitive - 12));
                            ordering_slot = ordering + (uint32)((sint32)depth >> (shift & 31)) * 4;
                            xport_gte_execute(0x118043F);
                            previous = r_u32(ordering_slot);
                            w_u32(ordering_slot, packet & 0xFFFFFF);
                            w_u32(packet + 4, xport_gte_read_data(20));
                            w_u32(packet + 16, xport_gte_read_data(21));
                            w_u32(packet + 28, xport_gte_read_data(22));
                            xport_gte_write_data(0, r_u32(primitive - 8));
                            xport_gte_write_data(1, r_u32(primitive - 4));
                            result |= previous & 0xFFFFFF;
                            w_u32(packet, result);
                            xport_gte_execute(0x108041B);
                            w_u32(packet + 40, xport_gte_read_data(22));
                        }
                        else
                        {
                            ordering_slot = ordering + (uint32)((sint32)depth >> (shift & 31)) * 4;
                            previous = r_u32(ordering_slot);
                            xport_gte_write_data(6, r_u32(primitive - 12));
                            xport_gte_write_data(0, r_u32(primitive - 8));
                            xport_gte_write_data(1, r_u32(primitive - 4));
                            w_u32(ordering_slot, packet & 0xFFFFFF);
                            xport_gte_execute(0x108041B);
                            result |= previous & 0xFFFFFF;
                            w_u32(packet, result);
                            w_u32(packet + 4, xport_gte_read_data(22));
                        }
                    }
                }
                first = next_first;
                second = next_second;
                third = next_third;
            }
            packet += packet_step;
        } while (count != 0);
    }
}


/* Dispatch the four callbacks used by the original action table */
uint32 td5_80028364(uint32 car, uint32 selector, uint32 original_stack)
{
    uint32 frame = original_stack - 0x18;
    uint32 selector_bytes = selector << 2;
    uint32 callback;

    FUNCTION_MARKER(0x80028364, "SLUS_006.10");
    callback = r_u32(0x8007DFBC + selector_bytes);
    switch (callback)
    {
    case 0x800280FC:
        return td5_800280fc(car);
    case 0x80028230:
        return td5_80028230(car);
    case 0x80028398:
        td5_80028398();
        return callback;
    case 0x800283A0:
        return td5_800283a0(car);
    default:
        {
            char message[256];
            snprintf(message, sizeof(message), "Unbound TD5 car action %08X, selector %08X, frame %08X\n",
            callback, selector, frame);
            xport_message_error("Test Drive 5", message);
        }
        abort();
    }
}
