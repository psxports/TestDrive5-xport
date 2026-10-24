#include "td5_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Existing shared SDK patch services */
uint32 _copy_memcard_patch(void);
uint32 _patch_card(void);
uint32 _patch_card2(void);

/* Additional original entrypoints required during integration */
void td5_80057310(void)
{
    sint32 offset = 0x2058;

    FUNCTION_MARKER(0x80057310, "SLUS_006.10");
    do
    {
        w_u8(0x800BCF08 + (uint32)offset, 0);
        offset -= 120;
    } while (offset >= 0);
}

uint32 td5_80057334(void)
{
    uint32 index;

    FUNCTION_MARKER(0x80057334, "SLUS_006.10");
    for (index = 0; index < 70; ++index)
        if (r_u8(0x800BCF08 + index * 120) == 0)
            return index;
    return 0;
}

void td5_800a99f0(void)
{
    uint32 index;

    FUNCTION_MARKER(0x800A99F0, "SLUS_006.10");
    index = r_u32(0x800B02D8) - 1;
    w_u32(0x800B02D8, index);
    w_u32(0x80088620, r_u8(0x800B0400 + index));
    td5_8001a4d8();
}

void td5_card_copy_patch(void)
{
    psx_bios_bind_card_patches(0x800681A0, 0x80068210, 0x80068224, 0, 0);
    _copy_memcard_patch();
}

void td5_card_patch_first(uint32 return_site)
{
    uint32 table, base, high, low, destination;

    w_u32(0x8008C1D8, return_site);
    EnterCriticalSectionPSX();
    table = GetC0Table();
    base = r_u32(table + 24);
    high = r_u32(base + 112);
    low = r_u32(base + 116);
    destination = ((high & 0xFFFF) << 16) + (low & 0xFFFF) + 40;
    psx_bios_bind_card_patches(0x800681A0, 0x80068210, 0x80068224, destination, 0);
    _patch_card();
}

void td5_card_patch_second(uint32 return_site)
{
    uint32 table, base;

    w_u32(0x8008C1D8, return_site);
    EnterCriticalSectionPSX();
    table = GetB0Table();
    base = r_u32(table + 364);
    psx_bios_bind_card_patches(0x800681A0, 0x80068210, 0x80068224, 0, base + 2504);
    _patch_card2();
}

void td5_call_car_collision_callback(uint32 target, uint32 car, uint32 original_stack)
{
    switch (target)
    {
        case 0x80024938:
            td5_80024938(car, original_stack);
            return;
        case 0x8002D79C:
            td5_8002d79c(car);
            return;
    }
    {
        char message[256];
        snprintf(message, sizeof(message), "Unbound TD5 car collision callback %08X\n", target);
        xport_message_error("Test Drive 5", message);
    }
    abort();
}

void td5_call_cd_request_callback(uint32 target)
{
    switch (target)
    {
        case 0x8005A840:
            td5_8005a840();
            return;
        case 0x800346DC:
            td5_800346dc();
            return;
        case 0x80034708:
            td5_80034708();
            return;
        case 0x80034734:
            td5_80034734();
            return;
    }
    {
        char message[256];
        snprintf(message, sizeof(message), "Unbound TD5 resource completion callback %08X\n", target);
        xport_message_error("Test Drive 5", message);
    }
    abort();
}

static uint32 td5_glyph_ink_height(uint32 family, uint32 glyph, uint32 pixels)
{
    uint32 slot = family * 140;
    uint32 divisor, column, row, texture, width, row_stride, rows, height;
    uint32 pixel_start, column_stride, scan, ink, index, count, value;
    sint32 remaining, scan_height;

    FUNCTION_MARKER(0x800B22A4, "SLUS_006.10");
    divisor = r_u8(0x800BBDFE + slot);
    column = (uint32)td5_divide_checked((sint32)glyph, (sint32)divisor);
    row = glyph % divisor;
    texture = r_u32(0x800BBE00 + slot);
    width = r_u8(texture + 12);
    rows = r_u8(0x800BBDFD + slot);
    row_stride = ((width + 3) & 0x1FC) >> 1;
    height = r_u8(0x800BBDFC + slot);
    pixel_start = pixels + row * (height >> 1) + column * (rows * row_stride);
    remaining = (sint32)height - 1;
    scan_height = remaining;
    if (remaining <= 0)
        return (uint32)remaining;
    column_stride = row_stride;
    do
    {
        ink = 0;
        index = 0;
        scan = 0;
        if (rows != 0)
        {
            count = r_u8(0x800BBDFD + slot);
            do
            {
                value = r_u8(pixel_start + (uint32)(scan_height / 2) + scan);
                value = (scan_height & 1) != 0 ? value & 15 : value >> 4;
                if (value != 0)
                    ++ink;
                ++index;
                scan += column_stride;
            } while ((sint32)index < (sint32)count);
        }
        if (ink == 0)
            --remaining;
        --scan_height;
    } while (scan_height > 0 && ink == 0);
    return (uint32)remaining;
}

void td5_800b2438(void)
{
    uint32 family, slot, filename, length, texture, width, height, pixels;
    uint32 page, value, glyph;
    PSX_RECT rectangle;

    FUNCTION_MARKER(0x800B2438, "SLUS_006.10");
    for (family = 0; family < 5; ++family)
    {
        slot = family * 140;
        filename = r_u32(0x800BBDF8 + slot);
        length = 0;
        while (r_u8(filename + length) != 0)
            ++length;
        texture = td5_find_texture((const char *)psx_addr(filename, length + 1));
        w_u32(0x800BBE00 + slot, texture);
        width = (r_u8(texture + 12) + 3) & 0xFFFC;
        height = r_u8(texture + 13);
        pixels = td5_alloc_memory((width >> 1) * height);
        texture = r_u32(0x800BBE00 + slot);
        value = r_u8(texture + 14);
        page = r_u16(texture + 8);
        rectangle.x = (sint16)((value >> 2) + ((page << 6) & 0x3FF));
        texture = r_u32(0x800BBE00 + slot);
        page = r_u16(texture + 8);
        value = r_u8(texture + 15);
        rectangle.y = (sint16)(value + ((page << 4) & 0x100) + ((page >> 2) & 0x200));
        rectangle.w = (sint16)(width >> 2);
        rectangle.h = (sint16)height;
        StoreImage(&rectangle, (uint32 *)psx_addr(pixels, (uint32)rectangle.w * height * 2));
        DrawSync(0);
        for (glyph = 0; glyph < 128; ++glyph)
        {
            value = td5_glyph_ink_height(family, glyph, pixels);
            value -= r_u8(0x800BBDFF + slot);
            w_u8(0x800BBE04 + slot + glyph, value);
        }
        value = r_u8(0x800BBDFC + slot);
        w_u8(0x800BBE04 + slot, value >> 1);
        td5_free_memory(pixels);
    }
}

uint32 td5_800b3394(uint32 original_stack)
{
    uint32 matrix = original_stack - 0x60 + 0x18;
    uint32 words[8];
    uint32 index, angle, result;

    FUNCTION_MARKER(0x800B3394, "SLUS_006.10");
    for (index = 0; index < 8; ++index)
        words[index] = r_u32(0x800B0B08 + index * 4);
    for (index = 0; index < 8; ++index)
        w_u32(matrix + index * 4, words[index]);
    td5_set_light(0, 0x800BC0B4, 110, 0, 0);
    td5_set_light(1, 0x800BC0BC, 110, 0, 110);
    td5_set_light(2, 0x800BC0C4, 0, 0, 110);
    td5_set_back_color(90, 50, 50);
    angle = r_u16(0x800BC0CC) + 16;
    w_u16(0x800BC0CC, angle);
    RotMatrixX((sint16)angle, (MATRIX *)psx_addr(matrix, 32));
    RotMatrixY(r_s16(0x800BC0CE), (MATRIX *)psx_addr(matrix, 32));
    angle = r_u16(0x800BC0D0) + 32;
    w_u16(0x800BC0D0, angle);
    RotMatrixZ((sint16)angle, (MATRIX *)psx_addr(matrix, 32));
    SetRotMatrix((MATRIX *)psx_addr(matrix, 20));
    for (index = 0; index < 3; ++index)
        words[index] = r_u32(matrix + index * 4);
    xport_gte_write_control(5, words[0]);
    xport_gte_write_control(6, words[1]);
    xport_gte_write_control(7, words[2]);
    td5_apply_light_matrix(matrix);
    w_u32(0x800BBD88, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0D4, 8), r_u32(0x800BBD78)));
    w_u32(0x800BBD8C, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0DC, 8), r_u32(0x800BBD7C)));
    w_u32(0x800BBD90, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0E4, 8), r_u32(0x800BBD80)));
    w_u32(0x800BBD94, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0EC, 8), r_u32(0x800BBD84)));
    td5_set_back_color(90, 90, 50);
    td5_apply_light_matrix(matrix);
    w_u32(0x800BBDD8, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0D4, 8), r_u32(0x800BBD78)));
    w_u32(0x800BBDDC, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0DC, 8), r_u32(0x800BBD7C)));
    w_u32(0x800BBDE0, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0E4, 8), r_u32(0x800BBD80)));
    w_u32(0x800BBDE4, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0EC, 8), r_u32(0x800BBD84)));
    td5_set_back_color(50, 50, 100);
    td5_apply_light_matrix(matrix);
    td5_set_light(0, 0x800BC0B4, 0, 50, 50);
    td5_set_light(1, 0x800BC0BC, 0, 0, 110);
    w_u32(0x800BBDB8, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0D4, 8), r_u32(0x800BBD78)));
    w_u32(0x800BBDBC, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0DC, 8), r_u32(0x800BBD7C)));
    w_u32(0x800BBDC0, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0E4, 8), r_u32(0x800BBD80)));
    w_u32(0x800BBDC4, gte_normal_color_col((SVECTOR *)psx_addr(0x800BC0EC, 8), r_u32(0x800BBD84)));
    td5_set_light(0, 0x800AE0C8, 110, 110, 110);
    td5_set_light(1, 0x800AE0C8, 0, 0, 0);
    result = td5_set_light(2, 0x800AE0C8, 0, 0, 0);
    td5_set_back_color(50, 50, 50);
    return result;
}

static void td5_menu_wheel_uv(uint32 packet)
{
    w_u8(packet + 12, r_u8(0x8008F5AC));
    w_u8(packet + 13, r_u8(0x8008F5AD));
    w_u8(packet + 20, r_u8(0x8008F5AC) + 31);
    w_u8(packet + 21, r_u8(0x8008F5AD));
    w_u8(packet + 28, r_u8(0x8008F5AC));
    w_u8(packet + 29, r_u8(0x8008F5AD) + 31);
    w_u8(packet + 36, r_u8(0x8008F5AC) + 31);
    w_u8(packet + 37, r_u8(0x8008F5AD) + 31);
    w_u16(packet + 22, r_u16(0x8008F5A8));
    w_u16(packet + 14, r_u16(0x8008F5AA));
}

static void td5_menu_link_wheel(uint32 packet, uint32 bucket)
{
    uint32 display, head, tag, previous;

    display = r_u32(0x800880F0);
    head = r_u32(display + 0x70) + bucket * 4;
    tag = r_u32(packet);
    previous = r_u32(head);
    w_u32(packet, (tag & 0xFF000000) | (previous & 0xFFFFFF));
    head = r_u32(display + 0x70) + bucket * 4;
    previous = r_u32(head);
    w_u32(head, (previous & 0xFF000000) | (packet & 0xFFFFFF));
}

static sint32 td5_menu_project_wheel(uint32 packet, uint32 frame, uint32 area_offset, uint32 depth_offset)
{
    uint32 word, flags;

    for (word = 0; word < 6; ++word)
        xport_gte_write_data(word, r_u32(frame + 0x60 + word * 4));
    xport_gte_execute(0x280030);
    w_u32(packet + 8, xport_gte_read_data(12));
    w_u32(packet + 16, xport_gte_read_data(13));
    w_u32(packet + 24, xport_gte_read_data(14));
    flags = xport_gte_read_control(31);
    w_u32(frame + 0xA0, flags);
    if ((flags & 0x1C66000) != 0)
        return 0;
    xport_gte_execute(0x1400006);
    w_u32(frame + area_offset, xport_gte_read_data(24));
    xport_gte_write_data(0, r_u32(frame + 0x78));
    xport_gte_write_data(1, r_u32(frame + 0x7C));
    xport_gte_execute(0x180001);
    w_u32(packet + 32, xport_gte_read_data(14));
    xport_gte_execute(0x168002E);
    w_u32(frame + depth_offset, xport_gte_read_data(7));
    return 1;
}

uint32 td5_800b5970(uint32 original_stack)
{
    uint32 frame = original_stack - 0xE0;
    uint32 model, points, angle, product, index, wheel, half, translation;
    uint32 value, x, z, coordinate, vertex, packet, shift, depth, bucket;
    sint32 radius, side, area, limit;

    FUNCTION_MARKER(0x800B5970, "SLUS_006.10");
    model = r_u32(0x800BC898);
    angle = r_u32(0x800BC874);
    model = r_u32(model + 0x54);
    radius = r_s16(model + 0x82);
    side = r_s16(model + 0x84);
    points = model + 0x40;
    product = (uint32)rcos((sint32)(angle << 2)) * (uint32)radius;
    angle = r_u32(0x800BC874);
    w_u32(frame + 0x10, (uint32)((sint32)product / 4096));
    product = (uint32)rsin((sint32)(angle << 2)) * (uint32)radius;
    angle = r_u32(0x800BC878);
    w_u32(frame + 0x18, (uint32)((sint32)product / 4096));
    product = (uint32)rcos((sint32)(angle << 2)) * (uint32)radius;
    angle = r_u32(0x800BC878);
    w_u32(frame + 0x14, (uint32)((sint32)product / 4096));
    product = (uint32)rsin((sint32)(angle << 2)) * (uint32)radius;
    angle = r_u32(0x800BC874);
    value = r_u32(0x800BC878);
    w_u32(frame + 0x1C, (uint32)((sint32)product / 4096));
    w_u32(0x800BC874, angle - 32);
    w_u32(0x800BC878, value - 32);
    for (index = 0; index < 4; ++index)
    {
        xport_gte_write_data(0, r_u32(points + index * 8));
        xport_gte_write_data(1, r_u32(points + index * 8 + 4));
        xport_gte_execute(0x480012);
        w_u32(frame + 0x20 + index * 16, xport_gte_read_data(25));
        w_u32(frame + 0x24 + index * 16, xport_gte_read_data(26));
        w_u32(frame + 0x28 + index * 16, xport_gte_read_data(27));
    }
    for (wheel = 0; wheel < 4; ++wheel)
    {
        half = wheel >> 1;
        if (wheel == 2)
        {
            w_u16(frame + 0x80, rcos(0));
            w_u16(frame + 0x82, 0);
            w_u16(frame + 0x84, rsin(0));
            w_u16(frame + 0x86, 0);
            w_u16(frame + 0x88, 4096);
            w_u16(frame + 0x8A, 0);
            w_u16(frame + 0x8C, 0 - (uint32)rsin(0));
            w_u16(frame + 0x8E, 0);
            w_u16(frame + 0x90, rcos(0));
            MulMatrix2((MATRIX *)psx_addr(0x80092210, 20), (MATRIX *)psx_addr(frame + 0x80, 20));
            SetRotMatrix((MATRIX *)psx_addr(frame + 0x80, 20));
        }
        translation = frame + 0x20 + ((wheel + 2) & 3) * 16;
        xport_gte_write_control(5, r_u32(translation));
        xport_gte_write_control(6, r_u32(translation + 4));
        xport_gte_write_control(7, r_u32(translation + 8));
        td5_80033454(0, wheel);
        coordinate = frame + 0x10 + half * 4;
        value = (wheel & 1) != 0 ? (uint32)side : 0 - (uint32)side;
        w_u16(frame + 0x60, value);
        z = r_u32(coordinate + 8);
        w_u16(frame + 0x62, z);
        x = r_u32(coordinate);
        w_u16(frame + 0x68, value);
        w_u16(frame + 0x64, x);
        x = r_u32(coordinate);
        w_u16(frame + 0x6A, (wheel & 1) != 0 ? 0 - x : x);
        z = r_u32(coordinate + 8);
        w_u16(frame + 0x70, value);
        w_u16(frame + 0x6C, (wheel & 1) != 0 ? z : 0 - z);
        x = r_u32(coordinate);
        w_u16(frame + 0x72, (wheel & 1) != 0 ? x : 0 - x);
        z = r_u32(coordinate + 8);
        w_u16(frame + 0x78, value);
        w_u16(frame + 0x74, (wheel & 1) != 0 ? 0 - z : z);
        z = r_u32(coordinate + 8);
        w_u16(frame + 0x7A, 0 - z);
        x = r_u32(coordinate);
        w_u16(frame + 0x7C, 0 - x);
        packet = 0x8009DD00 + r_u32(0x80088104) * 1920 + wheel * 80;
        td5_menu_wheel_uv(packet);
        if (td5_menu_project_wheel(packet, frame, 0xA4, 0xA8) != 0)
        {
            area = r_s32(frame + 0xA4);
            depth = r_u32(frame + 0xA8);
            if (area < 0)
            {
                shift = r_u32(0x80088110);
                value = r_u32(0x80088164);
                bucket = (uint32)((sint32)(depth - 8) >> ((shift - value) & 31));
                if ((sint32)bucket < 0)
                    bucket = 0;
                td5_menu_link_wheel(packet, bucket);
                xport_gte_write_data(6, r_u32(0x800BC2A4));
                vertex = 0x800BC2A8 + wheel * 8;
                xport_gte_write_data(0, r_u32(vertex));
                xport_gte_write_data(1, r_u32(vertex + 4));
                xport_gte_execute(0xE80413);
                w_u32(packet + 4, xport_gte_read_data(22));
            }
            else
            {
                limit = (sint32)(r_u32(0x8008857C) - 1);
                shift = r_u32(0x80088110);
                value = r_u32(0x80088164);
                bucket = (uint32)((sint32)(depth + 8) >> ((shift - value) & 31));
                if ((sint32)bucket >= limit)
                    bucket = (uint32)limit;
                td5_menu_link_wheel(packet, bucket);
                w_u32(packet + 4, 0x2C000000);
            }
        }
        value = r_u32(0x80088104);
        packet = 0x8009DD28 + value * 1920 + wheel * 80;
        x = r_u16(frame + 0x68);
        z = r_u16(frame + 0x70);
        coordinate = r_u16(frame + 0x78);
        vertex = r_u16(frame + 0x60);
        w_u16(frame + 0x68, 0 - x);
        w_u16(frame + 0x70, 0 - z);
        w_u16(frame + 0x78, 0 - coordinate);
        w_u16(frame + 0x60, 0 - vertex);
        td5_menu_wheel_uv(packet);
        if (td5_menu_project_wheel(packet, frame, 0xAC, 0xB0) != 0 && r_s32(frame + 0xAC) > 0)
        {
            depth = r_u32(frame + 0xB0);
            limit = (sint32)(r_u32(0x8008857C) - 1);
            shift = r_u32(0x80088110);
            value = r_u32(0x80088164);
            bucket = (uint32)((sint32)(depth + 8) >> ((shift - value) & 31));
            if ((sint32)bucket >= limit)
                bucket = (uint32)limit;
            td5_menu_link_wheel(packet, bucket);
        }
    }
    return 0;
}

const char *td5_menu_string(uint32 address)
{
    uint32 length = 0;

    while (r_u8(address + length) != 0)
        ++length;
    return (const char *)psx_addr(address, length + 1);
}

static uint32 td5_menu_result_prefix(uint32 pointer_slot)
{
    uint32 destination = r_u32(pointer_slot);
    uint32 index = 0, value;

    do
    {
        value = r_u8(0x800B0AA4 + index);
        w_u8(destination + index++, value);
    } while (value != 0);
    return destination + 2;
}

static uint32 td5_menu_store_text(uint32 destination, const char *text)
{
    uint32 index = 0;

    do
    {
        w_u8(destination + index, (uint8)text[index]);
    } while (text[index++] != 0);
    return index - 1;
}

uint32 td5_menu_format_number(uint32 destination, uint32 format, uint32 value)
{
    char text[64];

    sprintf(text, td5_menu_string(format), (sint32)value);
    return td5_menu_store_text(destination, text);
}

uint32 td5_menu_format_time(uint32 destination, uint32 format, uint32 time, sint32 seconds_unit)
{
    sint32 minutes, seconds, fraction;
    uint32 remainder;
    char text[64];

    minutes = (sint32)time / (seconds_unit * 60);
    remainder = time - (uint32)minutes * (uint32)(seconds_unit * 60);
    seconds = (sint32)remainder / seconds_unit;
    fraction = (sint32)(remainder - (uint32)seconds * (uint32)seconds_unit);
    sprintf(text, td5_menu_string(format), minutes, seconds, fraction);
    return td5_menu_store_text(destination, text);
}

uint32 td5_menu_unsigned_time(uint32 ticks, uint32 divisor)
{
    if (divisor == 0)
        xport_mips_break(7);
    return ticks / divisor;
}

uint32 td5_menu_result_speed(uint32 fixed_speed, uint32 multiplier, uint32 shift)
{
    uint32 high = (uint32)(((sint64)(sint32)fixed_speed * (sint64)(sint32)multiplier) >> 32);
    uint32 quotient = (uint32)((sint32)(high + fixed_speed) >> shift);

    return quotient - (uint32)((sint32)fixed_speed >> 31);
}

uint32 td5_800b187c(uint32 selection)
{
    uint32 car, record, value, destination, selected, divisor, time, index, rate;

    FUNCTION_MARKER(0x800B187C, "SLUS_006.10");
    value = r_u32(0x8008871C);
    w_u32(0x800BF3A4, selection);
    selected = (uint32)r_s8(0x8008CF72 + selection * 24 + value);
    w_u32(0x800BBCB4, r_u32(0x8007EAA0 + selected * 68));
    value = selection == 0 ? r_u32(0x8008773C) : 0x800B0A88;
    w_u32(0x800BBCB8, value);
    value = r_u32(0x8008771C);
    if (value - 2 < 2 && (sint32)selection < 2)
        w_u32(0x800BBCB8, selection == 0 ? 0x800B0A8C : 0x800B0A98);
    destination = td5_menu_result_prefix(0x800BBCBC);
    record = selection * 20;
    value = (uint32)r_s16(0x8009DA22 + record);
    value = r_u32(0x8007E0F0 + value * 4);
    value = (uint32)sprintf((char *)psx_addr(destination, strlen(td5_menu_string(value)) + 1), td5_menu_string(0x800B0AA8), td5_menu_string(value));
    destination = td5_menu_result_prefix(0x800BBD0C);
    value = (uint32)r_s16(0x8009DA22 + record);
    value = r_u32(0x8007E0F0 + value * 4);
    value = (uint32)sprintf((char *)psx_addr(destination, strlen(td5_menu_string(value)) + 1), td5_menu_string(0x800B0AA8), td5_menu_string(value));
    destination = td5_menu_result_prefix(0x800BBCC0);
    td5_menu_format_number(destination, 0x800B0AAC, r_u32(0x8009DA24 + record));
    value = r_u32(0x8009DA28 + record) * 100;
    divisor = (uint32)(r_s32(0x800880FC) / 2);
    time = td5_menu_unsigned_time(value, divisor);
    destination = td5_menu_result_prefix(0x800BBCC4);
    td5_menu_format_time(destination, 0x800B0AB0, time, 100);
    destination = td5_menu_result_prefix(0x800BBCC8);
    car = selection * 864;
    value = r_u32(0x800876C4);
    selected = (uint32)(sint32)r_s16(0x800987AA + car) << 8;
    if (value == 0)
        td5_menu_format_number(destination, 0x800B0AC0, td5_menu_result_speed(selected, 0xD161543F, 10));
    else
        td5_menu_format_number(destination, 0x800B0AC8, td5_menu_result_speed(selected, 0xA8791709, 9));
    destination = td5_menu_result_prefix(0x800BBCCC);
    value = r_u32(0x800876C4);
    selected = (uint32)(sint32)r_s16(0x800987A8 + car) << 8;
    if (value == 0)
        td5_menu_format_number(destination, 0x800B0AC0, td5_menu_result_speed(selected, 0xD161543F, 10));
    else
        td5_menu_format_number(destination, 0x800B0AC8, td5_menu_result_speed(selected, 0xA8791709, 9));
    destination = td5_menu_result_prefix(0x800BBCD0);
    value = r_u8(0x800987FB + car);
    value = r_u32(0x8007E0F0 + value * 4);
    sprintf((char *)psx_addr(destination, strlen(td5_menu_string(value)) + 1), td5_menu_string(0x800B0AA8), td5_menu_string(value));
    destination = td5_menu_result_prefix(0x800BBCD4);
    value = r_u8(0x800987F8 + car);
    value = r_u32(0x8007E0F0 + value * 4);
    sprintf((char *)psx_addr(destination, strlen(td5_menu_string(value)) + 1), td5_menu_string(0x800B0AA8), td5_menu_string(value));
    destination = td5_menu_result_prefix(0x800BBCD8);
    value = r_u8(0x800987F9 + car);
    value = r_u32(0x8007E0F0 + value * 4);
    sprintf((char *)psx_addr(destination, strlen(td5_menu_string(value)) + 1), td5_menu_string(0x800B0AA8), td5_menu_string(value));
    destination = td5_menu_result_prefix(0x800BBCDC);
    td5_menu_format_number(destination, 0x800B0AAC, r_u8(0x800987FA + car));
    value = r_u32(0x800987A0 + car) * 100;
    divisor = (uint32)(r_s32(0x800880FC) / 2);
    time = td5_menu_unsigned_time(value, divisor);
    destination = td5_menu_result_prefix(0x800BBCE0);
    td5_menu_format_time(destination, 0x800B0AB0, time, 100);
    for (index = 0; index < 5; ++index)
    {
        value = r_u16(0x800987C6 + car + index * 2) * 100;
        divisor = (uint32)(r_s32(0x800880FC) / 2);
        time = (uint32)td5_divide_checked((sint32)value, (sint32)divisor);
        destination = td5_menu_result_prefix(0x800BBCE4 + index * 4);
        td5_menu_format_time(destination, 0x800B0AB0, time, 100);
        if (index == 0)
        {
            destination = td5_menu_result_prefix(0x800BBCFC);
            td5_menu_format_time(destination, 0x800B0AB0, time, 100);
        }
    }
    destination = td5_menu_result_prefix(0x800BBCF8);
    td5_menu_format_number(destination, 0x800B0AAC, r_u32(0x80098740 + car));
    rate = r_u32(0x800880FC);
    value = r_u32(0x800987A0 + car);
    divisor = (uint32)((sint32)rate / 2);
    time = td5_menu_unsigned_time(value, divisor);
    destination = td5_menu_result_prefix(0x800BBD00);
    td5_menu_format_time(destination, 0x800B0AD0, time, 1000);
    destination = td5_menu_result_prefix(0x800BBD04);
    td5_menu_format_number(destination, 0x800B0AAC, r_u8(0x8009DA30 + record));
    destination = td5_menu_result_prefix(0x800BBD08);
    return td5_menu_format_number(destination, 0x800B0AAC, r_u32(0x8009DA24 + record));
}

static uint32 td5_menu_copy_pair(uint32 destination, uint32 source)
{
    uint32 first, second;

    (void)r_u32(source);
    first = r_u32(source);
    (void)r_u32(source + 4);
    second = r_u32(source + 4);
    w_u32(destination, first);
    w_u32(destination, first);
    w_u32(destination + 4, second);
    w_u32(destination + 4, second);
    return second;
}

static void td5_reset_menu_history(uint32 index)
{
    FUNCTION_MARKER(0x800A98B4, "SLUS_006.10");
    w_u32(0x800B02D8, 0);
    while ((sint32)index < 16)
        w_u8(0x800B0400 + index++, 255);
}

static void td5_update_menu_item_flags(void)
{
    uint32 first, second, third, fourth, fifth, sixth, seventh, enabled;

    FUNCTION_MARKER(0x800ADE68, "SLUS_006.10");
    first = r_u16(0x800AEFAC);
    second = r_u16(0x800AEE2C);
    third = r_u16(0x800AF0CC);
    fourth = r_u16(0x800AF0E4);
    fifth = r_u16(0x800AF0FC);
    sixth = r_u16(0x800AF114);
    seventh = r_u16(0x800AEC64);
    enabled = r_u8(0x8009DB44);
    first |= 0x8000;
    second |= 0x8000;
    third |= 0x8000;
    fourth |= 0x8000;
    fifth |= 0x4000;
    sixth |= 0x4000;
    seventh |= 0x4000;
    w_u16(0x800AEFAC, first);
    w_u16(0x800AEE2C, second);
    w_u16(0x800AF0CC, third);
    w_u16(0x800AF0E4, fourth);
    w_u16(0x800AF0FC, fifth);
    w_u16(0x800AF114, sixth);
    w_u16(0x800AEC64, seventh);
    if (enabled != 0)
        w_u16(0x800AEC64, seventh & 0x3FFF);
    if (r_u8(0x8009DB3F) != 0)
    {
        w_u16(0x800AF0CC, third & 0x3FFF);
        w_u16(0x800AF0FC, (fifth & 0x3FFF) | 0x8000);
    }
    if (r_u8(0x8009DB40) != 0)
    {
        w_u16(0x800AF0E4, fourth & 0x3FFF);
        w_u16(0x800AF114, (sixth & 0x3FFF) | 0x8000);
    }
    if (r_u8(0x8009DB41) != 0)
        w_u16(0x800AF0FC, r_u16(0x800AF0FC) & 0x3FFF);
    if (r_u8(0x8009DB42) != 0)
        w_u16(0x800AF114, r_u16(0x800AF114) & 0x3FFF);
    if (r_u8(0x8009DB43) != 0)
    {
        w_u16(0x800AEFAC, first & 0x3FFF);
        w_u16(0x800AEE2C, second & 0x3FFF);
    }
}

void td5_800a79b0(uint32 original_stack, uint32 invalid_header_index, uint32 archive_error_index)
{
    uint32 frame = original_stack - 0x28;
    uint32 history_index, value, last_frame;

    FUNCTION_MARKER(0x800A79B0, "SLUS_006.10");
    history_index = td5_menu_copy_pair(frame + 0x10, 0x800A533C);
    td5_80013cd8();
    td5_reset_menu_history(history_index);
    td5_80057310();
    w_u32(0x800AE0C4, 0);
    w_u32(0x800875D0, 0);
    w_u32(0x80087648, 0);
    last_frame = VSync(-1);
    while (r_u32(0x800AE0C4) == 0)
    {
        td5_menu_copy_pair(frame + 0x18, 0x800A5344);
        td5_update_menu_item_flags();
        td5_poll_pads();
        td5_swap_display_buffer();
        td5_clear_ordering_table();
        td5_set_view_position(0x800AE0D0);
        td5_set_view_angles(0x800AE0E8);
        td5_800b2638(frame);
        td5_800abebc(frame, invalid_header_index, archive_error_index);
        td5_800b73ec();
        value = (uint32)td5_800557f0(frame);
        value = (uint32)td5_read_timer(1, (sint32)value);
        w_u32(0x800875FC, value);
        value = (uint32)DrawSync(0);
        value = (uint32)td5_read_timer(1, (sint32)value);
        w_u32(0x80087600, value);
        while ((sint32)((uint32)VSync(-1) - last_frame) <= 0)
        {
        }
        last_frame = VSync(-1);
        td5_reset_timer(1);
        w_u32(0x800B02D4, r_u32(0x800B02D4) + 1);
        td5_apply_display_buffer();
        if (r_u32(0x80088620) == 21)
            td5_800b4674();
        else
            td5_upload_presentation_image();
        td5_draw_ordering_table();
    }
}

static void td5_menu_idle_tick(void)
{
    FUNCTION_MARKER(0x800A994C, "SLUS_006.10");
    w_u32(0x80087648, r_u32(0x80087648) + 1);
    if (r_s16(0x800885D0) != 0)
        w_u32(0x80087648, 0);
    if (r_s16(0x800885D2) != 0)
        w_u32(0x80087648, 0);
    if (r_u32(0x80087714) != 0)
        w_u32(0x80087648, r_u32(0x80087648) + 120);
    if (r_s32(0x80087648) >= 7200)
    {
        w_u32(0x80087648, 0);
        w_u32(0x80088620, 140);
    }
}

static void td5_menu_update_selection(uint32 address, sint32 count)
{
    uint32 selected;

    selected = (uint32)td5_move_menu_selection(r_s32(address), count);
    w_u32(address, selected);
    td5_800574f0((sint32)selected, count);
}

uint32 td5_menu_copy_string(uint32 destination, uint32 source)
{
    const char *text = td5_menu_string(source);
    size_t length = strlen(text);

    strcpy((char *)psx_addr(destination, length + 1), text);
    return destination;
}

static void td5_menu_variant_label(void)
{
    uint32 destination, format;

    destination = td5_menu_copy_string(r_u32(0x800BCF84), 0x800A5E70);
    format = r_u32(0x80087718) != 0 ? 0x800A5EEC : 0x800A5EE4;
    td5_menu_copy_string(destination + 3, format);
}

static void td5_menu_hide_options(void)
{
    uint32 index;

    for (index = 0; index < 7; ++index)
        w_u8(0x800BD340 + index * 120, 0);
}

/* Pending callee ABI reviews are recorded separately from body presence */
void td5_800abebc(uint32 original_stack, uint32 invalid_header_index, uint32 archive_error_index)
{
    uint32 frame = original_stack - 0x20;
    uint32 state, value, other, selected, destination, index, record, source;
    sint32 sine;

    FUNCTION_MARKER(0x800ABEBC, "SLUS_006.10");
    state = r_u32(0x80088620);
    switch (state)
    {
        case 0:
            w_u32(0x80088620, 1);
            return;
        case 10:
            td5_800aa198(frame);
            td5_800b7294(0x800AF724);
            w_u32(0x80088620, 11);
            td5_800a9568();
            return;
        case 11:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_800aa45c(frame);
            return;
        case 220:
            td5_800aa79c();
            td5_800b7294(0x800AF82C);
            w_u32(0x80088620, 221);
            td5_800a9568();
            return;
        case 221:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_800aa920(frame);
            return;
        case 20:
            td5_800b3f24(0);
            w_u32(0x80088620, 21);
            return;
        case 21:
            td5_menu_idle_tick();
            td5_800b633c(0, frame);
            return;
        case 80:
            td5_800b7294(0x800AEBCC);
            w_u32(0x80088620, 81);
            td5_800a9568();
            w_u32(0x800BC174, 0);
            w_u32(0x80087660, 0);
            w_u32(0x80087640, 0);
            w_u32(0x800B02D0, 0);
            td5_menu_hide_options();
            return;
        case 81:
            SetDispMask(1);
            index = r_u8(0x800884F4);
            value = r_u32(0x800B0280);
            selected = r_u8(0x80088454);
            record = r_u32(0x800AE9EC + index * 4);
            other = r_u32(0x800B027C);
            w_u32(0x800B0280, value + 48);
            w_u32(0x800B027C, other + 32);
            w_u32(0x800BD2CC, record);
            value = r_u16(0x800BD000);
            other = r_u16(0x800AEC04);
            if (selected == 255)
            {
                w_u16(0x800BD000, value | 0x8000);
                w_u16(0x800AEC04, other | 0x8000);
            }
            else
            {
                w_u16(0x800BD000, value & 0x3FFF);
                w_u16(0x800AEC04, other & 0x3FFF);
            }
            sine = rsin(r_s32(0x800B027C));
            value = r_u32(0x800B0280);
            w_u16(0x800BD25A, (uint32)(sine / 128) + 100);
            sine = rsin((sint32)value);
            value = r_u32(0x800B027C);
            w_u16(0x800BD258, (uint32)(sine / 128) + 400);
            sine = rsin((sint32)value);
            value = r_u32(0x800B0280);
            w_u16(0x800BD2D2, (uint32)(sine / 128) + 108);
            sine = rsin((sint32)value);
            w_u16(0x800BD2D0, (uint32)(sine / 128) + 400);
            td5_800a9568();
            td5_menu_idle_tick();
            value = r_u32(0x80087654);
            td5_menu_hide_options();
            if (value == 0 && r_u32(0x800AEA2C) == 0)
            {
                td5_800adc90(invalid_header_index);
                return;
            }
            selected = (uint32)td5_move_menu_selection(r_s32(0x8008769C), 7);
            value = r_u16(0x800884E8);
            w_u32(0x8008769C, selected);
            if ((value & 0x4008) == 0)
                return;
            if (selected == 6)
            {
                td5_800530f0();
                w_u32(0x8008858C, 10);
                w_u32(0x800AE0C4, 1);
                return;
            }
            record = r_u32(0x800BF3A8) + selected * 24;
            value = r_u32(record + 16);
            if (value != 0)
            {
                td5_800a98f0(value);
                td5_800530f0();
            }
            return;
        case 70:
            td5_800b7294(0x800AEF2C);
            w_u32(0x80088620, 71);
            td5_800a9568();
            return;
        case 71:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_menu_update_selection(0x800876A0, 6);
            value = r_u16(0x800884E8);
            w_u32(0x800875D0, 0);
            if ((value & 0x4008) != 0 && r_u32(0x800876A0) == 3)
                w_u32(0x800875D0, 1);
            return;
        case 150:
            td5_800b7294(0x800AF094);
            td5_menu_variant_label();
            w_u32(0x80088620, 151);
            td5_800a9568();
            return;
        case 151:
            td5_menu_variant_label();
            if ((r_u16(0x800884E8) & 0xA0) != 0 && r_u32(0x800876BC) == 1)
            {
                w_u32(0x80087718, r_u32(0x80087718) ^ 1);
                td5_80053064();
            }
            td5_800a9568();
            td5_menu_idle_tick();
            td5_menu_update_selection(0x800876BC, 6);
            return;
        case 40:
            td5_800b7294(0x800AF1FC);
            w_u32(0x80088620, 41);
            td5_800a9568();
            return;
        case 41:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_menu_update_selection(0x800876B0, 5);
            return;
        case 90:
            td5_800b7294(0x800AF34C);
            w_u32(0x80088620, 91);
            td5_800a9568();
            return;
        case 91:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_menu_update_selection(0x800876B8, 3);
            td5_800aaf68();
            return;
        case 100:
            w_u32(0x800B02D0, 0);
            td5_800b7294(0x800AF43C);
            w_u32(0x80088620, 101);
            td5_800a9568();
            return;
        case 101:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_800574f0(r_s32(0x800B02D0), 4);
            td5_800aad1c();
            return;
        case 110:
            td5_800b7294(0x800AF574);
            w_u32(0x80088620, 111);
            td5_800a9568();
            return;
        case 111:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_menu_update_selection(0x800876B4, 11);
            td5_800ab3b0();
            return;
        case 120:
            td5_800b7294(0x800AF934);
            td5_800a63e4();
            w_u32(0x80088620, 121);
            return;
        case 121:
            td5_menu_idle_tick();
            td5_800a6594(frame);
            if (r_u32(0x80087640) == 0)
                td5_800574f0(0, 1);
            else if ((r_u16(0x800884E8) & 0x4008) != 0)
                w_u32(0x80088620, 80);
            return;
        case 130:
            td5_800b7294(0x800BC400);
            w_u32(0x80088620, 131);
            td5_800b7f0c(0);
            return;
        case 131:
            td5_menu_idle_tick();
            td5_800b80c8(0, frame, invalid_header_index);
            return;
        case 140:
            w_u32(0x8008858C, 9);
            for (index = 0; index < 8; ++index)
                w_u32(0x800876B8 - index * 4, 0);
            w_u32(0x800AE0C4, 1);
            w_u32(0x80087650, 1);
            td5_800557c0();
            value = r_u32(0x80087700);
            w_u32(frame + 0x10, 5120);
            td5_8005490c(13, 20, (sint32)value, 0, 5120);
            return;
        case 160:
            td5_800b7294(0x800AE370);
            w_u32(0x80088620, 161);
            td5_800a9568();
            td5_800a8a1c();
            return;
        case 161:
            td5_800a8a48(0, archive_error_index);
            td5_800a8a48(1, archive_error_index);
            td5_800a9568();
            td5_menu_idle_tick();
            return;
        case 1:
            td5_menu_idle_tick();
            return;
        case 170:
            value = r_u32(0x80088420);
            w_u32(0x800887B8, 0xFFFFFFFF);
            if (value == 0)
                td5_800b7294(0x800B00B4);
            else
            {
                td5_800b7294(0x800AFF1C);
                td5_800ad5e0(frame);
            }
            value = r_u32(0x80087728);
            other = r_u32(0x8007DD00 + value * 4);
            w_u32(0x80088620, 171);
            w_u32(0x800BD074, other);
            return;
        case 171:
            value = r_u16(0x800884E8);
            w_u32(0x800887B8, 0xFFFFFFFF);
            if ((value & 0x4008) == 0)
                return;
            value = td5_80016c2c();
            if (value != 1 || r_u32(0x80088530) != 0)
                w_u32(0x80088620, 80);
            else if (r_u32(0x80087660) == 1)
                w_u32(0x80088620, 120);
            else
                w_u32(0x80088620, 190);
            return;
        case 240:
            value = r_u32(0x80088420);
            w_u32(0x800887B8, 0xFFFFFFFF);
            if (value == 0)
                td5_800b7294(0x800AFE74);
            else
            {
                td5_800b7294(0x800AFDB4);
                destination = td5_menu_copy_string(r_u32(0x800BCF0C), 0x800A5EF4);
                selected = r_u8(0x800883F0);
                value = r_u8(0x8009DA20 + selected * 20);
                td5_menu_format_number(destination + 1, 0x800A5EB4, value);
            }
            w_u32(0x80088620, 241);
            return;
        case 241:
            value = r_u16(0x800884E8);
            w_u32(0x800887B8, 0xFFFFFFFF);
            if ((value & 0x4008) != 0)
                w_u32(0x80088620, 80);
            return;
        case 190:
            td5_800b7294(0x800AF9C4);
            w_u32(0x80088620, 191);
            td5_800a9568();
            if (r_u32(0x80087640) != 0)
            {
                w_u8(0x800BE0D8, 0);
                w_u8(0x800BE150, 0);
            }
            return;
        case 191:
            td5_800a9568();
            td5_800a70b4();
            return;
        case 200:
            w_u32(0x800876C0, 0);
            td5_800b7294(0x800AEDF4);
            w_u32(0x80088620, 201);
            td5_800a9568();
            return;
        case 201:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_menu_update_selection(0x800876C0, 4);
            return;
        case 180:
            td5_800b7294(0x800AEA34);
            w_u32(0x80088620, 181);
            td5_800a9568();
            return;
        case 181:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_menu_update_selection(0x800876B4, 16);
            for (index = 0; index < 16; ++index)
            {
                source = 0x8007DC14 + index;
                value = r_u8(source);
                destination = r_u32(0x800BCF0C + index * 120);
                w_u8(destination, value + 48);
                if (r_u32(0x800876B4) == index && (r_u16(0x800884E8) & 0x4008) != 0)
                    w_u8(source, r_u8(source) ^ 1);
            }
            return;
        case 210:
            w_u32(0x800B02D0, 2);
            w_u32(0x800876A8, 0);
            td5_800b7294(0x800AEA4C);
            w_u32(0x80088620, 211);
            td5_800a9568();
            td5_800aa198(frame);
            td5_800b36e8();
            w_u32(0x800B04D8, r_u32(0x800BC164));
            return;
        case 211:
            td5_800a9568();
            td5_menu_idle_tick();
            td5_800aba04(frame);
            td5_menu_update_selection(0x800B02D0, 3);
            return;
        case 60:
            td5_800b7294(0x800AF8EC);
            td5_800a73cc(frame, archive_error_index);
            w_u32(0x80088620, 61);
            return;
        case 61:
            td5_800a7884(frame, archive_error_index);
            return;
        default:
            return;
    }
}

void td5_800a8a1c(void)
{
    FUNCTION_MARKER(0x800A8A1C, "SLUS_006.10");
    w_u32(0x800B03F8, 0xFFFFFFFF);
    w_u32(0x800B03FC, 0xFFFFFFFF);
    w_u32(0x800B0420, 0);
    w_u32(0x800B0424, 0);
}

static void td5_menu_frame_header(uint32 packet)
{
    w_u8(packet + 3, 5);
    w_u8(packet + 7, 40);
    w_u8(packet + 4, 0);
    w_u8(packet + 5, 0);
    w_u8(packet + 6, 16);
}

void td5_800a9568(void)
{
    uint32 packet, next_packet, display, head, tag, previous, buffer, value;

    FUNCTION_MARKER(0x800A9568, "SLUS_006.10");
    buffer = r_u32(0x80088104);
    packet = 0x800B0510 + buffer * 24;
    td5_menu_frame_header(packet);
    display = r_u32(0x800880F0);
    w_u16(packet + 8, 32);
    w_u16(packet + 10, 0);
    w_u16(packet + 12, 91);
    w_u16(packet + 14, 0);
    w_u16(packet + 16, 32);
    w_u16(packet + 18, 200);
    w_u16(packet + 20, 91);
    w_u16(packet + 22, 200);
    head = r_u32(display + 0x70);
    tag = r_u32(packet);
    previous = r_u32(head + 0x800);
    w_u32(packet, (tag & 0xFF000000) | (previous & 0xFFFFFF));
    head = r_u32(display + 0x70);
    buffer = r_u32(0x80088104);
    previous = r_u32(head + 0x800);
    next_packet = 0x800B0540 + buffer * 24;
    w_u32(head + 0x800, (previous & 0xFF000000) | (packet & 0xFFFFFF));
    packet = next_packet;
    td5_menu_frame_header(packet);
    w_u16(packet + 8, 0);
    w_u16(packet + 10, 27);
    w_u16(packet + 12, 450);
    w_u16(packet + 14, 27);
    w_u16(packet + 16, 0);
    w_u16(packet + 18, 32);
    value = r_u16(packet + 12);
    display = r_u32(0x800880F0);
    w_u16(packet + 20, 450);
    w_u16(packet + 22, 32);
    w_u16(packet + 12, value + 5);
    head = r_u32(display + 0x70);
    tag = r_u32(packet);
    previous = r_u32(head + 0x800);
    w_u32(packet, (tag & 0xFF000000) | (previous & 0xFFFFFF));
    head = r_u32(display + 0x70);
    buffer = r_u32(0x80088104);
    previous = r_u32(head + 0x800);
    next_packet = 0x800B0570 + buffer * 24;
    w_u32(head + 0x800, (previous & 0xFF000000) | (packet & 0xFFFFFF));
    packet = next_packet;
    w_u16(packet + 12, 442);
    value = r_u16(packet + 12);
    td5_menu_frame_header(packet);
    display = r_u32(0x800880F0);
    w_u16(packet + 10, 35);
    w_u16(packet + 14, 35);
    w_u16(packet + 8, 0);
    w_u16(packet + 16, 0);
    w_u16(packet + 18, 40);
    w_u16(packet + 20, 442);
    w_u16(packet + 22, 40);
    w_u16(packet + 12, value + 5);
    head = r_u32(display + 0x70);
    tag = r_u32(packet);
    previous = r_u32(head + 0x800);
    w_u32(packet, (tag & 0xFF000000) | (previous & 0xFFFFFF));
    head = r_u32(display + 0x70);
    previous = r_u32(head + 0x800);
    w_u32(head + 0x800, (previous & 0xFF000000) | (packet & 0xFFFFFF));
    buffer = r_u32(0x80088104);
    packet = 0x800B05A0 + buffer * 24;
    w_u16(packet + 12, 434);
    value = r_u16(packet + 12);
    td5_menu_frame_header(packet);
    display = r_u32(0x800880F0);
    w_u16(packet + 10, 43);
    w_u16(packet + 14, 43);
    w_u16(packet + 8, 0);
    w_u16(packet + 16, 0);
    w_u16(packet + 18, 47);
    w_u16(packet + 20, 434);
    w_u16(packet + 22, 47);
    w_u16(packet + 12, value + 5);
    head = r_u32(display + 0x70);
    tag = r_u32(packet);
    previous = r_u32(head + 0x800);
    buffer = r_u32(0x80088104);
    w_u32(packet, (tag & 0xFF000000) | (previous & 0xFFFFFF));
    head = r_u32(display + 0x70);
    previous = r_u32(head + 0x800);
    next_packet = 0x800B05D0 + buffer * 24;
    w_u32(head + 0x800, (previous & 0xFF000000) | (packet & 0xFFFFFF));
    packet = next_packet;
    td5_menu_frame_header(packet);
    w_u16(packet + 8, 91);
    w_u16(packet + 10, 227);
    w_u16(packet + 12, 513);
    w_u16(packet + 14, 227);
    value = r_u16(packet + 12);
    display = r_u32(0x800880F0);
    w_u16(packet + 16, 91);
    w_u16(packet + 18, 232);
    w_u16(packet + 20, 513);
    w_u16(packet + 22, 232);
    w_u16(packet + 12, value + 5);
    head = r_u32(display + 0x70);
    tag = r_u32(packet);
    previous = r_u32(head + 0x800);
    w_u32(packet, (tag & 0xFF000000) | (previous & 0xFFFFFF));
    head = r_u32(display + 0x70);
    previous = r_u32(head + 0x800);
    w_u32(head + 0x800, (previous & 0xFF000000) | (packet & 0xFFFFFF));
}

static void td5_menu_collect_unlocked(uint32 list, uint32 limit, uint32 category, uint32 flags)
{
    uint32 index, count, item;
    sint32 value;

    for (index = 0; index < limit; ++index)
    {
        value = r_s16(list + index * 2);
        if (value == -1)
            return;
        if (r_u8(0x8009DB71 + (uint32)value) == category)
        {
            count = r_u32(0x800B03F4);
            item = ((uint32)value - 1) | flags;
            w_u32(0x800B0328 + count * 4, item);
            w_u32(0x800B03F4, count + 1);
        }
    }
}

void td5_800aa198(uint32 original_stack)
{
    uint32 frame = original_stack - 0x10;
    uint32 first = 0x8007DBCC;
    uint32 second = 0x8007DBAC;
    uint32 mode, index, count, selected, value;

    FUNCTION_MARKER(0x800AA198, "SLUS_006.10");
    mode = r_u32(0x80087720);
    w_u16(frame, 0xFFFF);
    w_u32(0x800B0694, 0 - 64);
    w_u32(0x800B0314, 448);
    w_u32(0x800876A8, 0);
    w_u32(0x800B03F4, 0);
    if (mode == 1 || mode == 4)
        second = frame;
    if (r_u32(0x80087720) == 5)
    {
        first = frame;
        second = 0x8007DBBC;
    }
    for (index = 0; index < 46; ++index)
        w_u32(0x800B03DC - index * 4, 0);
    td5_menu_collect_unlocked(first, 24, 2, 0);
    td5_menu_collect_unlocked(second, 7, 2, 0);
    td5_menu_collect_unlocked(first, 24, 1, 0x100);
    td5_menu_collect_unlocked(second, 7, 1, 0x100);
    count = r_u32(0x800B03F4);
    if ((sint32)count > 0)
    {
        selected = r_u32(0x80087690);
        for (index = 0; (sint32)index < (sint32)count; ++index)
        {
            value = r_u8(0x800B0328 + index * 4);
            if (value == selected)
                w_u32(0x800876A8, index);
        }
    }
}

void td5_800aa79c(void)
{
    uint32 owner, source, destination, buffer, offset, word, index, count;
    uint32 words[4];
    sint32 value;

    FUNCTION_MARKER(0x800AA79C, "SLUS_006.10");
    owner = r_u32(0x80088660);
    source = r_u32(owner + 0x64);
    w_u32(0x800876A8, 0);
    w_u32(0x800B03F4, 0);
    w_u32(0x800B02D0, 0);
    w_u32(0x800B0694, 0 - 64);
    w_u32(0x800B0314, 448);
    w_u32(0x800B0318, source);
    for (buffer = 1; buffer < 4; ++buffer)
    {
        destination = td5_alloc_memory(0x7FFF);
        owner = r_u32(0x80088660);
        w_u32(0x800B0318 + buffer * 4, destination);
        source = r_u32(owner + 0x64);
        for (offset = 0; offset < 0x7FF0; offset += 16)
        {
            for (word = 0; word < 4; ++word)
                words[word] = r_u32(source + offset + word * 4);
            for (word = 0; word < 4; ++word)
                w_u32(destination + offset + word * 4, words[word]);
        }
        words[0] = r_u32(source + 0x7FF0);
        words[1] = r_u32(source + 0x7FF4);
        words[2] = r_u32(source + 0x7FF8);
        value = r_s16(source + 0x7FFC);
        w_u32(destination + 0x7FF0, words[0]);
        w_u32(destination + 0x7FF4, words[1]);
        w_u32(destination + 0x7FF8, words[2]);
        w_u16(destination + 0x7FFC, (uint32)value);
        value = r_s8(source + 0x7FFE);
        w_u8(destination + 0x7FFE, (uint32)value);
    }
    for (index = 0; index < 46; ++index)
        w_u32(0x800B03DC - index * 4, 0);
    for (index = 0; index < 6; ++index)
    {
        value = r_s16(0x8007DBBC + index * 2);
        if (r_u8(0x8009DB71 + (uint32)value) == 2)
        {
            count = r_u32(0x800B03F4);
            w_u32(0x800B0328 + count * 4, (uint32)value - 1);
            w_u32(0x800B03F4, count + 1);
        }
    }
}

void td5_800aad1c(void)
{
    uint32 selected, buttons, old_x, old_y, current_x;
    sint32 x, y;

    FUNCTION_MARKER(0x800AAD1C, "SLUS_006.10");
    if ((r_u16(0x800884E8) & 0x10) != 0 && r_u32(0x800B02D0) != 0)
    {
        w_u32(0x800B02D0, 0);
        td5_80053030();
    }
    if ((r_u16(0x800884E8) & 0x40) != 0 && r_u32(0x800B02D0) != 1)
    {
        w_u32(0x800B02D0, 1);
        td5_80053030();
    }
    if ((r_u16(0x800884E8) & 0x80) != 0 && r_u32(0x800B02D0) != 2)
    {
        w_u32(0x800B02D0, 2);
        td5_80053030();
    }
    if ((r_u16(0x800884E8) & 0x20) != 0 && r_u32(0x800B02D0) != 3)
    {
        w_u32(0x800B02D0, 3);
        td5_80053030();
    }
    buttons = r_u16(0x800884E8);
    selected = r_u32(0x800B02D0);
    w_u32(0x800887B8, selected);
    if ((buttons & 0x4008) == 0)
        return;
    old_x = r_u32(0x800876F8);
    old_y = r_u32(0x800876FC);
    switch (selected)
    {
        case 0:
            w_u32(0x800876FC, old_y - 1);
            break;
        case 1:
            w_u32(0x800876FC, old_y + 1);
            break;
        case 2:
            w_u32(0x800876F8, old_x - 1);
            break;
        case 3:
            w_u32(0x800876F8, old_x + 1);
            break;
    }
    x = r_s32(0x800876F8);
    if (x < 0)
        x = 0;
    else if (x > 32)
        x = 32;
    y = r_s32(0x800876FC);
    w_u32(0x800876F8, (uint32)x);
    if (y < 0)
        y = 0;
    else if (y > 32)
        y = 32;
    current_x = r_u32(0x800876F8);
    w_u32(0x800876FC, (uint32)y);
    if (old_x != current_x || old_y != (uint32)y)
        td5_800530f0();
    current_x = r_u32(0x800876F8);
    selected = r_u32(0x800876FC);
    w_u16(0x800976CC, current_x);
    w_u16(0x80097740, current_x);
    w_u16(0x800976CE, selected);
    w_u16(0x80097742, selected);
}

uint32 td5_800a9b44(uint32 selection, uint32 original_stack)
{
    uint32 frame = original_stack - 0x40;
    uint32 matrix = frame + 0x10;
    uint32 offset = selection * 4;
    uint32 first_target, second_target, current, value, delta, owner, object;
    uint32 words[3], group, index, count;
    sint32 changed;

    FUNCTION_MARKER(0x800A9B44, "SLUS_006.10");
    for (group = 0; group < 8; group += 3)
    {
        count = group == 6 ? 2 : 3;
        for (index = 0; index < count; ++index)
            words[index] = r_u32(0x800A5E44 + (group + index) * 4);
        for (index = 0; index < count; ++index)
            w_u32(matrix + (group + index) * 4, words[index]);
    }
    first_target = (uint32)(sint32)r_s16(0x800B015C + offset);
    value = (uint32)(sint32)r_s16(0x800B0220);
    changed = value != first_target;
    if (!changed)
    {
        value = (uint32)(sint32)r_s16(0x800B0222);
        second_target = (uint32)(sint32)r_s16(0x800B015E + offset);
        changed = value != second_target;
    }
    if (changed)
    {
        second_target = r_u16(0x800B015E + offset);
        current = r_u32(0x800B0218);
        value = (uint32)(sint32)r_s16(0x800B015C + offset);
        w_u16(0x800B0220, first_target);
        current &= 0xFFF00;
        w_u32(0x800B0218, current);
        delta = value - (uint32)((sint32)current >> 8);
        w_u16(0x800B0222, second_target);
        w_u32(0x800B0224, delta);
        if ((sint32)delta < -2048)
            w_u32(0x800B0224, delta + 4096);
        delta = r_u32(0x800B0224);
        if ((sint32)delta > 2048)
            w_u32(0x800B0224, delta - 4096);
        delta = r_u32(0x800B0224) << 8;
        current = r_u32(0x800B021C);
        value = (uint32)(sint32)r_s16(0x800B015E + offset);
        w_u32(0x800B0224, (uint32)((sint32)delta / 64));
        current &= 0xFFF00;
        w_u32(0x800B021C, current);
        delta = value - (uint32)((sint32)current >> 8);
        w_u32(0x800B0228, delta);
        if ((sint32)delta < -2048)
            w_u32(0x800B0228, delta + 4096);
        delta = r_u32(0x800B0228);
        if ((sint32)delta > 2048)
            w_u32(0x800B0228, delta - 4096);
        delta = r_u32(0x800B0228) << 8;
        w_u32(0x800B0228, (uint32)((sint32)delta / 64));
    }
    current = r_u32(0x800B0218);
    value = (uint32)(sint32)r_s16(0x800B0220);
    if ((uint32)((sint32)current >> 8) != value)
        w_u32(0x800B0218, current + r_u32(0x800B0224));
    value = r_u32(0x800B0218);
    current = r_u32(0x800B021C);
    second_target = (uint32)(sint32)r_s16(0x800B0222);
    w_u32(0x800B0218, value & 0xFFFFF);
    if ((uint32)((sint32)current >> 8) != second_target)
        w_u32(0x800B021C, current + r_u32(0x800B0228));
    value = r_u32(0x800B021C);
    w_u32(0x800B021C, value & 0xFFFFF);
    td5_set_view_center(256, 118);
    owner = r_u32(0x80088660);
    value = r_u32(0x800B0218);
    object = r_u32(owner + 0x64);
    w_u32(object + 16, 0);
    w_u32(object + 20, 0);
    w_u32(object + 24, 0x9C400);
    RotMatrixX((sint32)value >> 8, (MATRIX *)psx_addr(matrix, 32));
    value = r_u32(0x800B021C);
    RotMatrixY((sint32)value >> 8, (MATRIX *)psx_addr(matrix, 32));
    td5_apply_light_matrix(matrix);
    td5_800797fc(matrix, object);
    td5_8006e180(object, frame);
    td5_800b32e4(3);
    return td5_800b2790(128, 256, 114, td5_menu_string(0x800A5E64));
}

void td5_800aa45c(uint32 original_stack)
{
    uint32 frame = original_stack - 0x30;
    uint32 buttons, selected, last, value, record, count, history, state;
    sint32 left, right;

    FUNCTION_MARKER(0x800AA45C, "SLUS_006.10");
    buttons = r_u16(0x800884E8);
    w_u32(0x800887B8, 0xFFFFFFFF);
    if ((buttons & 0x80) != 0)
    {
        value = r_u32(0x800876A8);
        if ((sint32)value > 0)
        {
            w_u32(0x800876A8, value - 1);
            td5_80053064();
        }
    }
    if ((r_u16(0x800884E8) & 0x20) != 0)
    {
        count = r_u32(0x800B03F4);
        value = r_u32(0x800876A8);
        if ((sint32)value < (sint32)(count - 1))
        {
            w_u32(0x800876A8, value + 1);
            td5_80053064();
        }
    }
    selected = r_u32(0x800876A8);
    left = (sint32)selected > 0 ? 64 : -64;
    last = r_u32(0x800B03F4) - 1;
    right = (sint32)selected < (sint32)last ? 448 : 576;
    value = r_u32(0x800B0694);
    if ((sint32)value < left)
    {
        w_u32(0x800B0694, value + 16);
        value = r_u32(0x800B0694);
    }
    if (left < (sint32)value)
        w_u32(0x800B0694, value - 16);
    value = r_u32(0x800B0314);
    if ((sint32)value < right)
    {
        w_u32(0x800B0314, value + 16);
        value = r_u32(0x800B0314);
    }
    if (right < (sint32)value)
        w_u32(0x800B0314, value - 16);
    w_u8(0x800BD070, 1);
    w_u8(0x800BD0E8, 1);
    if (selected == 0)
        w_u8(0x800BD070, 0);
    if (selected == last)
        w_u8(0x800BD0E8, 0);
    value = r_u8(0x800B0328 + selected * 4);
    td5_800a9b44(value, frame);
    if ((r_u16(0x800884E8) & 0x4008) != 0)
    {
        selected = r_u32(0x800876A8);
        record = r_u32(0x800B0328 + selected * 4);
        if ((record & 0x100) != 0)
            td5_80053124();
        else
        {
            history = r_u32(0x800B02D8);
            state = r_u32(0x80088620);
            w_u32(0x8008770C, record & 255);
            w_u32(0x80087690, record & 255);
            w_u8(0x800B0400 + history, state - 1);
            history = r_u32(0x800B02D8);
            w_u32(0x80088620, 20);
            w_u32(0x800B02D8, history + 1);
            td5_8001a4d8();
        }
    }
    if ((r_u16(0x800884E8) & 0x1000) != 0)
    {
        td5_800a99f0();
        td5_800530f0();
    }
    selected = r_u32(0x800876A8);
    record = r_u32(0x800B0328 + selected * 4);
    if ((record & 0x100) != 0)
    {
        td5_800b32e4(3);
        td5_800b2790(128, 256, 180, td5_menu_string(0x800A5E68));
    }
    td5_800b32e4(2);
    selected = r_u32(0x800876A8);
    value = r_u8(0x800B0328 + selected * 4);
    value = r_u32(0x8007DC60 + value * 4);
    td5_800b2790(128, 256, 170, td5_menu_string(value));
}

uint32 td5_800a9e54(uint32 slot, uint32 selection, uint32 original_stack)
{
    uint32 frame = original_stack - 0x50;
    uint32 matrix = frame + 0x10;
    uint32 offset = selection * 4, slot_word = slot * 4, slot_half = slot * 2;
    uint32 first_target, second_target, value, current, delta, object;
    uint32 words[3], group, index, count;
    sint32 changed;

    FUNCTION_MARKER(0x800A9E54, "SLUS_006.10");
    for (group = 0; group < 8; group += 3)
    {
        count = group == 6 ? 2 : 3;
        for (index = 0; index < count; ++index)
            words[index] = r_u32(0x800A5E44 + (group + index) * 4);
        for (index = 0; index < count; ++index)
            w_u32(matrix + (group + index) * 4, words[index]);
    }
    first_target = (uint32)(sint32)r_s16(0x800B015C + offset);
    value = (uint32)(sint32)r_s16(0x800B024C + slot_half);
    changed = value != first_target;
    if (!changed)
    {
        value = (uint32)(sint32)r_s16(0x800B0254 + slot_half);
        second_target = (uint32)(sint32)r_s16(0x800B015E + offset);
        changed = value != second_target;
    }
    if (changed)
    {
        w_u16(0x800B024C + slot_half, first_target);
        second_target = r_u16(0x800B015E + offset);
        w_u16(0x800B0254 + slot_half, second_target);
        current = r_u32(0x800B022C + slot_word) & 0xFFF00;
        w_u32(0x800B022C + slot_word, current);
        value = (uint32)(sint32)r_s16(0x800B015C + offset);
        delta = value - (uint32)((sint32)current >> 8);
        w_u32(0x800B025C + slot_word, delta);
        if ((sint32)delta < -2048)
            w_u32(0x800B025C + slot_word, delta + 4096);
        delta = r_u32(0x800B025C + slot_word);
        if ((sint32)delta > 2048)
            w_u32(0x800B025C + slot_word, delta - 4096);
        delta = r_u32(0x800B025C + slot_word) << 8;
        w_u32(0x800B025C + slot_word, (uint32)((sint32)delta / 64));
        current = r_u32(0x800B023C + slot_word) & 0xFFF00;
        w_u32(0x800B023C + slot_word, current);
        value = (uint32)(sint32)r_s16(0x800B015E + offset);
        delta = value - (uint32)((sint32)current >> 8);
        w_u32(0x800B026C + slot_word, delta);
        if ((sint32)delta < -2048)
            w_u32(0x800B026C + slot_word, delta + 4096);
        delta = r_u32(0x800B026C + slot_word);
        if ((sint32)delta > 2048)
            w_u32(0x800B026C + slot_word, delta - 4096);
        delta = r_u32(0x800B026C + slot_word) << 8;
        w_u32(0x800B026C + slot_word, (uint32)((sint32)delta / 64));
    }
    current = r_u32(0x800B022C + slot_word);
    value = (uint32)(sint32)r_s16(0x800B024C + slot_half);
    if ((uint32)((sint32)current >> 8) != value)
        w_u32(0x800B022C + slot_word, current + r_u32(0x800B025C + slot_word));
    value = r_u32(0x800B022C + slot_word);
    w_u32(0x800B022C + slot_word, value & 0xFFFFF);
    current = r_u32(0x800B023C + slot_word);
    value = (uint32)(sint32)r_s16(0x800B0254 + slot_half);
    if ((uint32)((sint32)current >> 8) != value)
        w_u32(0x800B023C + slot_word, current + r_u32(0x800B026C + slot_word));
    value = r_u32(0x800B023C + slot_word);
    w_u32(0x800B023C + slot_word, value & 0xFFFFF);
    td5_set_view_center(196, (sint32)(slot * 40 + 72));
    object = r_u32(0x800B0318 + slot_word);
    w_u32(object + 16, 0);
    w_u32(object + 20, 0);
    w_u32(object + 24, 0x213400);
    value = r_u32(0x800B022C + slot_word);
    RotMatrixX((sint32)value >> 8, (MATRIX *)psx_addr(matrix, 32));
    value = r_u32(0x800B023C + slot_word);
    RotMatrixY((sint32)value >> 8, (MATRIX *)psx_addr(matrix, 32));
    td5_apply_light_matrix(matrix);
    td5_800797fc(matrix, object);
    td5_8006e180(object, frame);
    td5_800b32e4(3);
    return td5_800b2790(128, 196, (sint32)(slot * 40 + 62), td5_menu_string(0x800A5D3C));
}

void td5_800aa920(uint32 original_stack)
{
    uint32 frame = original_stack - 0x38;
    uint32 buttons, value, count, selected, slot, index, history, state, name;
    uint32 left_position, right_position, rotation, record;
    sint32 wanted, left, right;

    FUNCTION_MARKER(0x800AA920, "SLUS_006.10");
    buttons = r_u16(0x800884E8);
    w_u32(0x800887B8, 0xFFFFFFFF);
    if ((buttons & 0x10) != 0 && r_s32(0x800B02D0) > 0)
    {
        td5_80053030();
        w_u32(0x800B02D0, r_u32(0x800B02D0) - 1);
    }
    if ((r_u16(0x800884E8) & 0x40) != 0 && r_s32(0x800B02D0) < 3)
    {
        td5_80053030();
        w_u32(0x800B02D0, r_u32(0x800B02D0) + 1);
    }
    count = r_u32(0x800B03F4);
    index = 0;
    if ((sint32)count > 0)
    {
        slot = r_u32(0x800B02D0);
        wanted = r_s16(0x8007DBFC + slot * 2);
        for (; (sint32)index < (sint32)count; ++index)
            if (r_u32(0x800B0328 + index * 4) == (uint32)wanted)
                break;
    }
    buttons = r_u16(0x800884E8);
    w_u32(0x800876A8, index);
    if ((buttons & 0x80) != 0 && (sint32)index > 0)
        w_u32(0x800876A8, index - 1);
    if ((r_u16(0x800884E8) & 0x20) != 0)
    {
        count = r_u32(0x800B03F4);
        selected = r_u32(0x800876A8);
        if ((sint32)selected < (sint32)(count - 1))
        {
            w_u32(0x800876A8, selected + 1);
            selected = r_u32(0x800876A8);
        }
    }
    else
        selected = r_u32(0x800876A8);
    left = (sint32)selected > 0 ? 64 : -64;
    count = r_u32(0x800B03F4);
    right = (sint32)selected < (sint32)(count - 1) ? 448 : 576;
    value = r_u32(0x800B0694);
    if ((sint32)value < left)
    {
        w_u32(0x800B0694, value + 16);
        value = r_u32(0x800B0694);
    }
    if (left < (sint32)value)
        w_u32(0x800B0694, value - 16);
    value = r_u32(0x800B0314);
    if ((sint32)value < right)
    {
        w_u32(0x800B0314, value + 16);
        value = r_u32(0x800B0314);
    }
    if (right < (sint32)value)
        w_u32(0x800B0314, value - 16);
    left_position = r_u32(0x800B0694);
    rotation = r_u16(0x800B02C2);
    right_position = r_u32(0x800B0314);
    buttons = r_u16(0x800884E8);
    w_u16(frame + 0x12, 128);
    w_u16(frame + 0x14, 2500);
    w_u16(0x800B02C0, 0);
    w_u16(0x800B02C4, 1024);
    w_u16(frame + 0x10, left_position);
    w_u16(0x800B02C2, rotation + 32);
    w_u16(frame + 0x10, right_position);
    if ((buttons & 0x4008) != 0)
    {
        wanted = r_s16(0x8007DBFC);
        state = r_u32(0x80088620);
        history = r_u32(0x800B02D8);
        w_u32(0x8008770C, (uint32)wanted);
        w_u8(0x800B0400 + history, state - 1);
        history = r_u32(0x800B02D8);
        w_u32(0x80088620, 20);
        w_u32(0x800B02D8, history + 1);
        td5_8001a4d8();
    }
    if ((r_u16(0x800884E8) & 0x1000) != 0)
        td5_800a99f0();
    for (slot = 0; slot < 4; ++slot)
    {
        value = 2;
        if (r_u32(0x800B02D0) == slot)
        {
            selected = r_u32(0x800876A8);
            record = r_u32(0x800B0328 + selected * 4);
            value = 1;
            w_u16(0x8007DBFC + slot * 2, record);
        }
        td5_800b32e4(value);
        wanted = r_s16(0x8007DBFC + slot * 2);
        name = r_u32(0x8007DC60 + (uint32)wanted * 4);
        td5_800b2790(0, 232, (sint32)(64 + slot * 40), td5_menu_string(name));
        wanted = r_s16(0x8007DBFC + slot * 2);
        td5_800a9e54(slot, (uint32)wanted, frame);
    }
}

void td5_800aaf68(void)
{
    uint32 destination, format, row_offset, y;
    uint32 packet, display, head, tag, previous, amount, left, right;
    sint32 width;

    FUNCTION_MARKER(0x800AAF68, "SLUS_006.10");
    if ((r_u16(0x800884E8) & 0x80) != 0 && r_u32(0x800876B8) == 1 && r_s32(0x80087704) > 0)
    {
        td5_80053064();
        w_u32(0x80087704, r_u32(0x80087704) - 512);
    }
    if ((r_u16(0x800884E8) & 0x20) != 0 && r_u32(0x800876B8) == 1 && r_s32(0x80087704) < 4096)
    {
        td5_80053064();
        w_u32(0x80087704, r_u32(0x80087704) + 512);
    }
    if ((r_u16(0x800884E8) & 0x80) != 0 && r_u32(0x800876B8) == 0 && r_s32(0x80087700) > 0)
    {
        td5_80053064();
        w_u32(0x80087700, r_u32(0x80087700) - 512);
        td5_update_cd_volume();
    }
    if ((r_u16(0x800884E8) & 0x20) != 0 && r_u32(0x800876B8) == 0 && r_s32(0x80087700) < 4096)
    {
        td5_80053064();
        w_u32(0x80087700, r_u32(0x80087700) + 512);
        td5_update_cd_volume();
    }
    if (r_u32(0x800876B8) == 2 && (r_u16(0x800884E8) & 0xA0) != 0)
    {
        td5_80053030();
        w_u32(0x80087708, r_u32(0x80087708) ^ 1);
    }
    destination = td5_menu_copy_string(r_u32(0x800BCFFC), 0x800A5E70);
    format = r_u32(0x80087708) != 0 ? 0x800A5E78 : 0x800A5E74;
    td5_menu_copy_string(destination + 3, format);
    y = 80;
    for (row_offset = 0; row_offset < 144; row_offset += 72)
    {
        packet = 0x800B0600 + r_u32(0x80088104) * 36 + row_offset;
        w_u8(packet + 3, 8);
        w_u8(packet + 7, 0x38);
        w_u8(packet + 4, 0);
        w_u8(packet + 5, 255);
        w_u8(packet + 6, 0);
        w_u8(packet + 12, 255);
        w_u8(packet + 13, 0);
        w_u8(packet + 14, 0);
        w_u8(packet + 20, 0);
        w_u8(packet + 21, 255);
        w_u8(packet + 22, 0);
        w_u8(packet + 28, 255);
        w_u8(packet + 29, 0);
        w_u8(packet + 30, 0);
        display = r_u32(0x800880F0);
        w_u16(packet + 8, 96);
        w_u16(packet + 10, y);
        w_u16(packet + 16, 224);
        w_u16(packet + 18, y);
        w_u16(packet + 24, 96);
        w_u16(packet + 26, y + 8);
        w_u16(packet + 32, 224);
        w_u16(packet + 34, y + 8);
        head = r_u32(display + 0x70);
        tag = r_u32(packet);
        previous = r_u32(head + 8);
        w_u32(packet, (tag & 0xFF000000) | (previous & 0xFFFFFF));
        head = r_u32(display + 0x70);
        previous = r_u32(head + 8);
        w_u32(head + 8, (previous & 0xFF000000) | (packet & 0xFFFFFF));
        amount = 4096 - r_u32(row_offset != 0 ? 0x80087704 : 0x80087700);
        packet = 0x800B0448 + r_u32(0x80088104) * 36 + row_offset;
        w_u8(packet + 3, 8);
        w_u8(packet + 7, 0x38);
        w_u8(packet + 4, 15);
        w_u8(packet + 5, 0);
        w_u8(packet + 6, 0);
        w_u8(packet + 12, 15);
        w_u8(packet + 13, 0);
        w_u8(packet + 14, 0);
        w_u8(packet + 20, 15);
        w_u8(packet + 21, 0);
        w_u8(packet + 22, 0);
        w_u8(packet + 28, 15);
        w_u8(packet + 29, 0);
        w_u8(packet + 30, 0);
        width = (sint32)amount / 32;
        left = 224 - (uint32)width;
        right = left + (uint32)width;
        w_u16(packet + 8, left);
        w_u16(packet + 24, left);
        w_u16(packet + 10, y);
        w_u16(packet + 16, right);
        w_u16(packet + 18, y);
        w_u16(packet + 26, y + 8);
        display = r_u32(0x800880F0);
        w_u16(packet + 32, right);
        w_u16(packet + 34, y + 8);
        head = r_u32(display + 0x70);
        tag = r_u32(packet);
        previous = r_u32(head);
        w_u32(packet, (tag & 0xFF000000) | (previous & 0xFFFFFF));
        head = r_u32(display + 0x70);
        previous = r_u32(head);
        y = 130;
        w_u32(head, (previous & 0xFF000000) | (packet & 0xFFFFFF));
    }
}

void td5_800a6318(void)
{
    uint32 index, offset, value, previous, skip, count;

    FUNCTION_MARKER(0x800A6318, "SLUS_006.10");
    w_u8(0x800B0414, 0);
    for (index = 0, offset = 0; index < 47; ++index, offset += 120)
    {
        if (index == 40)
        {
            value = r_u8(0x8007D807);
            previous = r_u8(0x8007D7F3);
            skip = value < previous;
        }
        else if (index == 44)
        {
            value = r_u32(0x8007D9D8);
            previous = r_u32(0x8007D9C4);
            skip = value < previous;
        }
        else
        {
            value = r_u32(0x8007C538 + offset);
            previous = r_u32(0x8007C524 + offset);
            skip = previous < value;
        }
        if (skip == 0 && value != 0)
        {
            count = r_u8(0x800B0414);
            w_u8(0x800B02E0 + count, index);
            count = r_u8(0x800B0414);
            w_u8(0x800B0414, count + 1);
        }
    }
}

void td5_800a63e4(void)
{
    uint32 index, count, item;
    sint32 entry;

    FUNCTION_MARKER(0x800A63E4, "SLUS_006.10");
    w_u8(0x800B0414, 0);
    count = 0;
    for (index = 0; index < 24; ++index)
    {
        entry = r_s16(0x8007DBCC + index * 2);
        if (r_u8(0x8009DB71 + (uint32)entry) == 2)
        {
            w_u8(0x800B02E0 + count, (uint32)entry - 1);
            item = r_u8(0x800B0414);
            ++count;
            w_u8(0x800B0414, item + 1);
        }
    }
    for (index = 0; index < 7; ++index)
    {
        entry = r_s16(0x8007DBAC + index * 2);
        if (r_u8(0x8009DB71 + (uint32)entry) == 2)
        {
            w_u8(0x800B02E0 + count, (uint32)entry - 1);
            item = r_u8(0x800B0414);
            ++count;
            w_u8(0x800B0414, item + 1);
        }
    }
    w_u8(0x800B02E0 + count, 29);
    item = r_u8(0x800B0414);
    ++count;
    w_u8(0x800B0414, item + 1);
    for (index = 40; index < 46; ++index, ++count)
    {
        w_u8(0x800B02E0 + count, index);
        item = r_u8(0x800B0414);
        w_u8(0x800B0414, item + 1);
    }
    if (r_u32(0x80087640) != 0)
    {
        td5_80016af4();
        td5_800a6318();
        td5_80016a28();
        td5_80016ef8();
        w_u8(0x800BCFF8, 0);
        w_u8(0x800BD070, 0);
    }
    else
    {
        td5_80016af4();
        w_u8(0x800BCFF8, 1);
        w_u8(0x800BD070, 1);
    }
    w_u32(0x800B0698, 0);
}
