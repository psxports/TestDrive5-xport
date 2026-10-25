#include "td5_functions.h"
#include <string.h>

uint32 td5_80047564(uint32 view, uint32 original_stack)
{
    uint32 selected, counter, value, count, rank;
    int ready;

    FUNCTION_MARKER(0x80047564, "SLUS_006.10");
    selected = r_u32(0x800885F8);
    counter = 0x800883E8 + selected * 4;
    value = r_u32(counter);
    if ((sint32)value < 30000)
        w_u32(counter, value + 1);
    if (r_u32(0x8008772C) != 0 && r_u32(0x8008771C) != 3)
    {
        selected = r_u32(0x800885F8);
        count = r_u32(0x800883E8 + selected * 4);
        ready = 0;
        if ((sint32)count >= 61)
        {
            count = r_u32(0x800883E8 + (selected ^ 1) * 4);
            ready = (sint32)count >= 61;
        }
    }
    else
    {
        if (r_u32(0x80087720) == 3)
        {
            if (r_u32(0x800987A0) == 0 || r_u32(0x80098B00) == 0)
            {
                selected = r_u32(0x800885F8);
                counter = 0x800883E8 + selected * 4;
                value = r_u32(counter);
                w_u32(counter, value - 1);
            }
        }
        selected = r_u32(0x800885F8);
        count = r_u32(0x800883E8 + selected * 4);
        ready = (sint32)count >= 61;
    }
    if (ready)
        w_u32(0x800885E0, 1);
    selected = r_u32(0x800885F8);
    rank = r_u8(0x800987FB + selected * 864);
    if (rank < 3 && r_u32(0x80087734) == 0 && r_u32(0x8008771C) != 3)
        td5_80047278(view, (sint32)(rank + 1), 255, 255, 0);
    selected = r_u32(0x800885F8);
    w_u32(0x800883A8 + view * 4, 0x7D000);
    value = r_u32(0x800883C0 + view * 4);
    w_u32(0x800883C0 + view * 4, value - 8);
    return td5_8001e654(0x800984A0 + selected * 864, 1, view, original_stack - 0x20);
}

uint32 td5_8003ce3c(uint32 car, uint32 position, uint32 type)
{
    uint32 effect, packet, record, destination, bank, group, index, words[4];
    uint32 cosine, sine, horizontal, forward, value, first, second, lifetime;
    sint32 random;

    FUNCTION_MARKER(0x8003CE3C, "SLUS_006.10");
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
    w_u32(record + 32, r_u32(position));
    w_u32(record + 36, r_u32(position + 4));
    w_u32(record + 40, r_u32(position + 8));
    cosine = (uint32)rcos(r_s32(car + 0x1BC) >> 8);
    sine = (uint32)rsin(r_s32(car + 0x1BC) >> 8);
    random = randPSX();
    horizontal = (uint32)(random % 24576) - 12288;
    random = randPSX();
    forward = 0u - (uint32)(random % 12288);
    value = horizontal * cosine + forward * sine;
    w_u32(record + 12, (uint32)((sint32)value / 4096));
    value = forward * cosine - horizontal * sine;
    w_u32(record + 20, (uint32)((sint32)value / 4096));
    w_u32(record + 16, 6144);
    random = randPSX();
    w_u8(record + 3, type);
    w_u8(record, (uint32)(random % 31));
    random = randPSX();
    lifetime = ((uint32)(random % 4) + 1) * 10;
    first = (uint32)td5_divide_checked(12288, (sint32)(lifetime & 0xFE));
    w_u8(record + 2, lifetime);
    lifetime = r_u8(record + 2);
    second = (uint32)td5_divide_checked(6400, (sint32)lifetime);
    w_u16(record + 4, 0x7000);
    w_u16(record + 8, 8320);
    w_u16(record + 6, 0u - first);
    w_u16(record + 10, second);
    return 8320;
}

uint32 td5_800b217c(void)
{
    uint32 value, mode, stage;

    FUNCTION_MARKER(0x800B217C, "SLUS_006.10");
    if (r_u32(0x80087720) == 2 || r_u32(0x8008771C) == 2)
        value = r_u32(0x8008772C) != 0 ? 80 : 40;
    else
        value = r_u32(0x8008772C) != 0 ? 70 : 0;
    w_u32(0x80088520, value);
    if (r_u32(0x80087728) == 2 && r_u8(0x800987FB) != 0)
        w_u32(0x80088520, 60);
    if (r_u32(0x80087738) != 0 && r_u32(0x80087728) == 2)
    {
        value = r_u32(0x80088500);
        w_u32(0x80088520, value);
    }
    mode = r_u32(0x8008771C);
    if (mode != 3)
        return 3;
    stage = r_u32(0x800885F0);
    if (stage == 0)
        w_u32(0x80088520, 90);
    if (stage == 1)
        w_u32(0x80088520, 100);
    return 100;
}

sint32 td5_800575a4(uint32 archive_error_index, uint32 original_stack)
{
    uint32 frame = original_stack - 0x70;
    uint32 flags, mode, value, other, rectangle_a, rectangle_b;
    uint32 allocation, bytes, pool, cursor, count, texture, name_length;
    sint16 width, height;

    FUNCTION_MARKER(0x800575A4, "SLUS_006.10");
    flags = r_u32(0x80088530);
    w_u16(0x800884E0, 0);
    w_u32(0x80088448, 4096);
    w_u8(0x80088060, 0);
    w_u8(0x80088061, 0);
    if (flags != 0 || r_u32(0x80087738) != 0 || r_u32(0x800875D0) != 0)
    {
        td5_800b217c();
        goto setup;
    }
    mode = r_u32(0x80087728);
    if (mode != 2 || r_u8(0x800987FB) == 0)
    {
        td5_accumulate_race_results();
        mode = r_u32(0x80087728);
    }
    if (mode == 2 && r_u8(0x800987FB) != 0)
    {
        value = r_u32(0x80088418);
        w_u32(0x80088520, value != 0 ? 60 : 50);
        goto check_time;
    }
    value = r_u32(0x80088418);
    w_u32(0x80088520, 30);
    if (value != 0)
        goto check_time;
    if (r_u32(0x80087720) != 2 && r_u32(0x8008771C) != 2)
        goto check_time;
    td5_800164e0(r_s32(0x80087710));
    mode = r_u32(0x80087728);
    switch (mode)
    {
        case 0:
        case 4:
            td5_rank_race_scores();
            break;
        case 1:
        case 2:
        case 3:
        case 5:
            td5_rank_race_times();
            break;
    }
    mode = r_u32(0x8008771C);
    if (mode == 2)
    {
        td5_rank_race_times();
        mode = r_u32(0x8008771C);
    }
    if (mode != 3)
        goto setup;
    td5_rank_race_scores();
check_time:
    if (r_u32(0x8008771C) == 3 && r_u32(0x800885F0) == 0)
        w_u32(0x80088520, 90);
setup:
    if (r_u32(0x800875D0) != 0)
    {
        w_u32(0x80088530, 0);
        w_u32(0x80087738, 0);
        w_u8(0x80088060, 1);
        w_u8(0x80088061, 1);
        w_u32(0x800875D0, 0);
    }
    td5_init_memory_pool(0x800BFD00, 0x13F700);
    td5_init_ordering_tables(0x1101);
    td5_8006c590(2);
    td5_set_projection_distance(180);
    td5_80078b6c(0, 0x10000);
    td5_set_back_color(50, 50, 50);
    td5_set_light(0, 0x80088064, 110, 110, 110);
    td5_set_light(1, 0x80088064, 0, 0, 0);
    td5_set_light(2, 0x80088064, 0, 0, 0);
    td5_set_fog(3000, 16000, 0, 0, 0);
    rectangle_a = r_u32(0x8008806C);
    rectangle_b = r_u32(0x80088070);
    w_u32(frame + 0x18, rectangle_a);
    w_u32(frame + 0x1C, rectangle_b);
    allocation = td5_alloc_memory(0x80000);
    td5_load_archive_file((const char *)psx_addr(0x80012FEC, 18), allocation, archive_error_index);
    w_u16(frame + 0x1A, 0);
    width = r_s16(frame + 0x1C);
    height = r_s16(frame + 0x1E);
    bytes = width > 0 && height > 0 ? (uint32)width * (uint32)height * 2 : 0;
    LoadImagePSX((PSX_RECT *)psx_addr(frame + 0x18, 8), (uint32 *)psx_addr(allocation, bytes));
    DrawSync(0);
    value = td5_free_memory(allocation);
    bytes = td5_archive_aligned_size((const char *)psx_addr(0x80013000, 20), 1, archive_error_index, value);
    pool = td5_alloc_memory(bytes);
    w_u32(0x800884B4, pool);
    w_u32(0x80088660, pool);
    td5_load_archive_file((const char *)psx_addr(0x80013000, 20), pool, archive_error_index);
    pool = r_u32(0x800884B4);
    value = r_u32(pool);
    count = 1;
    cursor = pool + 4;
    if (value != 0)
    {
        do
        {
            value = r_u32(cursor);
            w_u32(cursor, value + pool);
            value = r_u32(pool);
            ++count;
            cursor += 4;
        } while (value >= count);
    }
    allocation = td5_alloc_memory(0x40814);
    value = r_u32(0x80087634) + 1;
    w_u32(0x80088608, allocation);
    value = (uint32)((sint32)value % 6);
    w_u32(0x80087634, value);
    name_length = sprintfPSX(frame + 0x20, 0x80013014, value + 1);
    allocation = r_u32(0x80088608);
    td5_load_archive_file((const char *)psx_addr(frame + 0x20, name_length + 1), allocation, archive_error_index);
    pool = r_u32(0x80088660);
    td5_set_texture_table(r_u32(pool + 4));
    texture = td5_find_texture((const char *)psx_addr(0x80088074, 6));
    w_u16(texture + 8, 152);
    w_u16(texture + 10, 48);
    td5_800b25e0();
    td5_upload_presentation_image();
    value = r_u32(0x80088104);
    w_u32(0x80088104, value ^ 1);
    td5_upload_presentation_image();
    value = r_u32(0x80088104);
    w_u32(0x80088104, value ^ 1);
    SetDispMask(1);
    td5_prepare_display(2);
    value = r_u32(0x800876F8);
    other = r_u32(0x800876FC);
    w_u16(0x800976CC, value);
    w_u16(0x80097740, value);
    w_u16(0x800976CE, other);
    w_u16(0x80097742, other);
    td5_swap_display_buffer();
    return (sint32)td5_apply_display_buffer();
}

sint32 td5_80057b30(uint32 original_stack, uint32 invalid_header_index)
{
    uint32 frame = original_stack - 0x30;
    uint32 value, first, second, tick, buttons, image, buffer;
    sint32 offset, random;

    FUNCTION_MARKER(0x80057B30, "SLUS_006.10");
    td5_80013cd8();
    w_u32(0x800875D8, 0);
    w_u16(0x80088794, 0);
    for (offset = 8280; offset >= 0; offset -= 120)
        w_u8(0x800BCF08 + (uint32)offset, 0);
    random = randPSX();
    td5_play_xa(random % 8);
    tick = (uint32)VSync(-1);
    if (r_u32(0x80087738) == 0)
        td5_800b7a8c();
    while (r_s16(0x80088794) == 0)
    {
        first = r_u32(0x8008807C);
        second = r_u32(0x80088080);
        w_u32(frame + 0x18, first);
        w_u32(frame + 0x1C, second);
        td5_poll_pads();
        td5_swap_display_buffer();
        td5_clear_ordering_table();
        td5_set_view_position(0x8007FBC0);
        td5_set_view_angles(0x80088084);
        td5_800b2638(frame);
        td5_80058d0c(frame, invalid_header_index);
        value = td5_800b73ec();
        value = (uint32)td5_read_timer(1, (sint32)value);
        w_u32(0x800875FC, value);
        td5_800557f0(frame);
        value = (uint32)DrawSync(0);
        value = (uint32)td5_read_timer(1, (sint32)value);
        buttons = r_u16(0x800885D0);
        w_u32(0x80087600, value);
        /* The button hook 13CF8 has no observable body */
        (void)buttons;
        do
        {
            value = (uint32)VSync(-1) - tick;
        } while ((sint32)value <= 0);
        tick = (uint32)VSync(-1);
        td5_reset_timer(1);
        td5_apply_display_buffer();
        if (r_u32(0x80088520) == 11)
            td5_800b4674();
        else
        {
            image = r_u32(0x80088608);
            buffer = r_u32(0x80088104);
            w_u16(frame + 0x26, 256);
            w_u16(frame + 0x24, 512);
            w_u16(frame + 0x20, 0);
            w_u16(frame + 0x22, buffer << 8);
            LoadImagePSX((PSX_RECT *)psx_addr(frame + 0x20, 8), (uint32 *)psx_addr(image + 20, 512 * 256 * 2));
        }
        value = r_u32(0x800875D8);
        w_u32(0x800875D8, value + 1);
        td5_draw_ordering_table();
    }
    td5_800557c0();
    return td5_8005490c(13, 20, r_s32(0x80087700), 0, 5120);
}

uint32 td5_800b7a8c(void)
{
    uint32 index, value, mission, entry, count, pointer;

    FUNCTION_MARKER(0x800B7A8C, "SLUS_006.10");
    for (index = 0; index < 5; ++index)
        w_u32(0x800BC3A0 - index * 4, 0);
    value = r_u32(0x80087720);
    w_u32(0x800BF0F8, 0);
    if (value == 0)
    {
        value = r_u32(0x80087730);
        if (value == 0)
        {
            value = r_u32(0x8008763C);
            if (value == 0)
                return value;
            value = r_u32(0x800883F8);
            if (value == 0)
                return value;
            value = r_u32(0x80088578);
            if (value == 0)
                return value;
        }
    }
    value = r_u32(0x8008772C);
    if (value != 0)
        return value;
    value = r_u32(0x80087720);
    if (value != 0)
        return value;
    value = r_u32(0x80088530);
    if (value != 0)
        return value;
    if (r_u8(0x800987FB) != 0)
        return 36;
    mission = r_u32(0x80087710);
    if (mission == 36)
    {
        if (r_s8(0x8008CF72) == 30 && r_u8(0x8009DB44) == 0)
        {
            count = r_u32(0x800BF0F8);
            w_u8(0x8009DB44, 1);
            w_u32(0x800BC390 + count * 4, 0x800B0E28);
            w_u32(0x800BF0F8, count + 1);
        }
        mission = r_u32(0x80087710);
    }
    entry = (uint32)r_s16(0x800BC3A4 + mission * 2);
    if (mission < 24)
    {
        if (r_u8(0x8009DB71 + entry) == 2)
            return 2;
        w_u8(0x8009DB71 + entry, 2);
        count = r_u32(0x800BF0F8);
        pointer = r_u32(0x8007DC5C + entry * 4);
    }
    else
    {
        if (entry == 0)
            return 2;
        if (r_u8(0x8009DB45 + entry) == 2)
            return 2;
        w_u8(0x8009DB45 + entry, 2);
        count = r_u32(0x800BF0F8);
        pointer = r_u32(0x8007EAA0 + entry * 68);
    }
    value = count * 4;
    w_u32(0x800BC390 + value, pointer);
    w_u32(0x800BF0F8, count + 1);
    return value;
}

void td5_80058d0c(uint32 original_stack, uint32 invalid_header_index)
{
    uint32 buttons, state, value, other, text, length;
    sint32 offset, choice;

    FUNCTION_MARKER(0x80058D0C, "SLUS_006.10");
    buttons = r_u16(0x800884E8);
    /* The original 4AC1C button hook is empty */
    (void)buttons;
    state = r_u32(0x80088520);
    if (state >= 102)
        return;
    // TODO: Restore the in-game storage menu after xport_save/load integration
    if (!td5_memory_card_enabled && (state == 20 || state == 21))
    {
        w_u32(0x80087694, 0);
        w_u32(0x800875D0, 0);
        td5_800b217c();
        return;
    }
    switch (state)
    {
        case 0:
        case 10:
        case 20:
        case 30:
        case 40:
        case 50:
        case 60:
        case 70:
        case 80:
        case 90:
        case 100:
            for (offset = 8280; offset >= 0; offset -= 120)
                w_u8(0x800BCF08 + (uint32)offset, 0);
            if (state == 10)
                td5_800b3f24(1);
            else if (state == 20)
            {
                td5_800b7294(0x800BC400);
                w_u32(0x80088520, 21);
                td5_800b7f0c(1);
                return;
            }
            else if (state == 30)
                td5_800b1358();
            w_u32(0x80088520, state + 1);
            return;
        case 1:
            td5_80058298();
            td5_80057f44();
            return;
        case 11:
            td5_800b633c(1, original_stack - 0x18);
            return;
        case 21:
            td5_800b80c8(1, original_stack - 0x18, invalid_header_index);
            return;
        case 31:
            td5_800b13ac();
            return;
        case 41:
            td5_800585e4();
            td5_80057f44();
            w_u32(0x80088500, 40);
            return;
        case 51:
            td5_800b32e4(2);
            text = r_u32(0x800BC11C);
            length = 0;
            while (r_u8(text + length) != 0)
                ++length;
            td5_800b2790(131, 256, 100, (const char *)psx_addr(text, length + 1));
            text = r_u32(0x800BC120);
            length = 0;
            while (r_u8(text + length) != 0)
                ++length;
            td5_800b2790(131, 256, 120, (const char *)psx_addr(text, length + 1));
            buttons = r_u16(0x800884E8);
            if ((buttons & 0x4000) != 0)
                w_u32(0x80088520, 60);
            return;
        case 61:
            td5_80058b88();
            td5_80057f44();
            w_u32(0x80088500, 60);
            return;
        case 71:
            td5_80058474();
            td5_80057f44();
            return;
        case 81:
            buttons = r_u16(0x800884E8);
            w_u16(0x8008C508, 2);
            w_u16(0x8008C50A, 8);
            w_u16(0x8008C50C, 4);
            w_u16(0x8008C50E, 0xFFFF);
            w_u16(0x8008C510, 0xFFFF);
            w_u16(0x8008C512, 0xFFFF);
            w_u16(0x800884EC, 72);
            if ((buttons & 0x4000) != 0)
            {
                td5_800530f0();
                choice = r_s16(0x800884E0);
                if (choice == 0)
                {
                    value = r_u32(0x800885F0);
                    w_u16(0x80088794, 1);
                    w_u32(0x8008858C, 5);
                    w_u32(0x800885F0, value + 1);
                }
                else if (choice == 1)
                {
                    if (r_u32(0x80088530) == 0 && r_u32(0x80087738) == 0)
                        w_u32(0x80088520, 30);
                }
                else if (choice == 2)
                {
                    w_u16(0x80088794, 1);
                    w_u32(0x8008858C, 0);
                    w_u32(0x80088620, 80);
                }
            }
            td5_80057f44();
            return;
        case 91:
            buttons = r_u16(0x800884E8);
            w_u16(0x8008C508, 9);
            w_u16(0x8008C50A, 4);
            w_u16(0x8008C50C, 0xFFFF);
            w_u16(0x8008C50E, 0xFFFF);
            w_u16(0x8008C510, 0xFFFF);
            w_u16(0x8008C512, 0xFFFF);
            w_u16(0x800884EC, 72);
            if ((buttons & 0x4000) != 0)
            {
                td5_800530f0();
                choice = r_s16(0x800884E0);
                if (choice == 0)
                {
                    value = r_u32(0x80088530);
                    w_u16(0x80088794, 1);
                    w_u32(0x8008858C, 5);
                    if (value == 0 && r_u32(0x80087738) == 0)
                    {
                        value = r_u8(0x8008CF8A);
                        other = r_u32(0x800885F0);
                        text = (uint32)r_s8(0x8008CF72);
                        w_u8(0x8008CF72, value);
                        w_u8(0x8008CF8A, text);
                        w_u32(0x800885F0, other ^ 1);
                    }
                }
                else if (choice == 1)
                {
                    w_u16(0x80088794, 1);
                    w_u32(0x8008858C, 0);
                    w_u32(0x80088620, 80);
                }
            }
            td5_80057f44();
            return;
        case 101:
            td5_80058a08();
            td5_80057f44();
            return;
        default:
            return;
    }
}

uint32 td5_80058298(void)
{
    uint32 value, mode, pointer;
    sint32 choice;

    FUNCTION_MARKER(0x80058298, "SLUS_006.10");
    w_u16(0x8008C50A, 1);
    w_u16(0x8008C50C, 8);
    w_u16(0x8008C50E, 6);
    w_u16(0x8008C510, 4);
    w_u16(0x8008C512, 0xFFFF);
    w_u16(0x8008C508, 0);
    w_u16(0x800884EC, 62);
    td5_800b7cb8();
    value = r_u16(0x800884E8) & 0x4000;
    if (value == 0)
        return value;
    td5_800530f0();
    choice = r_s16(0x800884E0);
    switch ((uint32)choice)
    {
        case 0:
            w_u16(0x80088794, 1);
            w_u32(0x8008858C, 4);
            return 4;
        case 1:
            pointer = r_u32(0x80088770);
            pointer = r_u32(pointer);
            value = r_u8(pointer + 30);
            if ((value & 8) == 0)
                return 1;
            w_u16(0x80088794, 1);
            w_u32(0x8008858C, 6);
            return 6;
        case 2:
            value = r_u32(0x80088530);
            if (value != 0)
                return value;
            value = r_u32(0x80087738);
            if (value == 0)
                w_u32(0x80088520, 30);
            return 30;
        case 3:
            w_u32(0x80088520, 10);
            return 10;
        case 4:
            mode = r_u32(0x80087720);
            w_u16(0x80088794, 1);
            w_u32(0x80088620, 80);
            value = 4;
            if (mode != 4)
            {
                mode = td5_80016c2c();
                value = 1;
                if (mode == 1)
                {
                    value = r_u32(0x80088530);
                    if (value == 0)
                    {
                        mode = r_u32(0x80087660);
                        value = mode == 1 ? 120 : 190;
                        w_u32(0x80088620, value);
                    }
                }
            }
            w_u32(0x8008858C, 0);
            return value;
        default:
            return (uint32)choice << 2;
    }
}

uint32 td5_800b7cb8(void)
{
    uint32 count, text, length, index, y;

    FUNCTION_MARKER(0x800B7CB8, "SLUS_006.10");
    count = r_u32(0x800BF0F8);
    if (count == 0)
        return count;
    td5_800b32e4(3);
    text = r_u32(0x800BC124);
    length = 0;
    while (r_u8(text + length) != 0)
        ++length;
    td5_800b2790(129, 256, 170, (const char *)psx_addr(text, length + 1));
    td5_800b32e4(0);
    count = r_u32(0x800BF0F8);
    if ((sint32)count <= 0)
        return count;
    index = 0;
    y = 180;
    do
    {
        text = r_u32(0x800BC390 + index * 4);
        length = 0;
        while (r_u8(text + length) != 0)
            ++length;
        td5_800b2790(129, 256, (sint32)y, (const char *)psx_addr(text, length + 1));
        count = r_u32(0x800BF0F8);
        ++index;
        y += 10;
    } while ((sint32)index < (sint32)count);
    return 0;
}

uint32 td5_80057f44(void)
{
    uint32 buttons, value, previous, index, entry, text, length, palette;
    sint32 choice, item, y;

    FUNCTION_MARKER(0x80057F44, "SLUS_006.10");
    buttons = r_u16(0x800884E8);
    if ((buttons & 0x10) != 0)
    {
        choice = r_s16(0x800884E0);
        if (choice > 0 && r_s16(0x8008C506 + (uint32)choice * 2) != -1)
        {
            td5_80053030();
            value = r_u16(0x800884E0) - 1;
            item = r_s16(0x8008C508 + (uint32)(sint32)(sint16)value * 2);
            w_u16(0x800884E0, value);
            if (item == 8 && (r_u32(0x80088530) != 0 || r_u32(0x80087738) != 0 || r_u8(0x80088061) != 0))
            {
                value = r_u16(0x800884E0);
                w_u16(0x800884E0, value - 1);
            }
            choice = r_s16(0x800884E0);
            item = r_s16(0x8008C508 + (uint32)choice * 2);
            if (item == 1 && r_u8(0x80088060) != 0)
                w_u16(0x800884E0, (uint32)choice - 1);
        }
    }
    buttons = r_u16(0x800884E8);
    if ((buttons & 0x40) != 0)
    {
        choice = r_s16(0x800884E0);
        if (choice < 5 && r_s16(0x8008C50A + (uint32)choice * 2) != -1)
        {
            td5_80053030();
            previous = r_u16(0x800884E0);
            value = previous + 1;
            item = r_s16(0x8008C508 + (uint32)(sint32)(sint16)value * 2);
            w_u16(0x800884E0, value);
            if (item == 1 && r_u8(0x80088060) != 0)
                w_u16(0x800884E0, previous + 2);
            choice = r_s16(0x800884E0);
            item = r_s16(0x8008C508 + (uint32)choice * 2);
            if (item == 8 && (r_u32(0x80088530) != 0 || r_u32(0x80087738) != 0 || r_u8(0x80088061) != 0))
            {
                value = r_u16(0x800884E0);
                w_u16(0x800884E0, value + 1);
            }
        }
    }
    td5_800b32e4(0);
    td5_800b2f8c(128, 104, 206, (const char *)psx_addr(0x8008808C, 6));
    td5_800b32e4(2);
    td5_800b2790(0, 117, 198, (const char *)psx_addr(0x80088094, 7));
    for (index = 0; index < 6; ++index)
    {
        entry = 0x8008C508 + index * 2;
        item = r_s16(entry);
        if (item == -1)
            continue;
        choice = r_s16(0x800884E0);
        palette = choice == (sint32)index ? 3 : 2;
        td5_800b32e4(palette);
        item = r_s16(entry);
        if (item == 8 && (r_u32(0x80088530) != 0 || r_u32(0x80087738) != 0 || r_u8(0x80088061) != 0))
        {
            td5_800b32e4(7);
            item = r_s16(entry);
        }
        if (item == 1 && r_u8(0x80088060) != 0)
            td5_800b32e4(7);
        item = r_s16(entry);
        y = r_s16(0x800884EC);
        text = r_u32(0x800BC0F4 + (uint32)item * 4);
        length = 0;
        while (r_u8(text + length) != 0)
            ++length;
        td5_800b2790(128, 256, (sint32)(index * 20) + y, (const char *)psx_addr(text, length + 1));
    }
    return 0;
}

uint32 td5_800b2f8c(uint32 mode, uint32 x, sint32 y, const char *name)
{
    uint32 texture, counter, base, packet, vertex, component, value, other;
    uint32 display, head, header, link, count, result;

    FUNCTION_MARKER(0x800B2F8C, "SLUS_006.10");
    texture = td5_find_texture(name);
    counter = r_u32(0x800BF398);
    base = r_u32(0x800BF108);
    packet = base + counter * 52;
    for (vertex = 0; vertex < 4; ++vertex)
    {
        for (component = 0; component < 3; ++component)
        {
            value = r_u8(0x800BCCB0 + vertex * 4 + component);
            if (vertex == 3 && component == 2)
            {
                w_u8(packet + 3, 12);
                w_u8(packet + 7, 0x3E);
            }
            w_u8(packet + 4 + vertex * 12 + component, value);
        }
    }
    value = r_u16(texture + 8);
    w_u16(packet + 26, value);
    value = r_u16(texture + 10);
    w_u16(packet + 14, value);
    if ((mode & 0x80) == 0)
    {
        w_u16(packet + 8, x);
        w_u16(packet + 10, (uint32)y);
        value = r_u8(texture + 12);
        w_u16(packet + 22, (uint32)y);
        w_u16(packet + 32, x);
        w_u16(packet + 20, x + value + 0xFFFF);
        value = r_u8(texture + 13);
        w_u16(packet + 34, (uint32)y + value + 0xFFFF);
        value = r_u8(texture + 12);
        w_u16(packet + 44, x + value + 0xFFFF);
        value = r_u8(texture + 13);
        w_u16(packet + 46, (uint32)y + value + 0xFFFF);
    }
    else
    {
        value = r_u8(texture + 12);
        w_u16(packet + 8, x - (value >> 1));
        value = r_u8(texture + 13);
        w_u16(packet + 10, (uint32)y - (value >> 1));
        value = r_u8(texture + 12);
        w_u16(packet + 20, x + (value >> 1) - 1);
        value = r_u8(texture + 13);
        w_u16(packet + 22, (uint32)y - (value >> 1));
        value = r_u8(texture + 12);
        w_u16(packet + 32, x - (value >> 1));
        value = r_u8(texture + 13);
        w_u16(packet + 34, (uint32)y + (value >> 1) - 1);
        value = r_u8(texture + 12);
        w_u16(packet + 44, x + (value >> 1) - 1);
        value = r_u8(texture + 13);
        w_u16(packet + 46, (uint32)y + (value >> 1) - 1);
    }
    value = r_u8(texture + 14);
    w_u8(packet + 12, value);
    value = r_u8(texture + 15);
    w_u8(packet + 13, value);
    value = r_u8(texture + 14);
    other = r_u8(texture + 12);
    w_u8(packet + 24, other + value + 255);
    value = r_u8(texture + 15);
    w_u8(packet + 25, value);
    value = r_u8(texture + 14);
    w_u8(packet + 36, value);
    value = r_u8(texture + 15);
    other = r_u8(texture + 13);
    w_u8(packet + 37, other + value + 255);
    value = r_u8(texture + 14);
    other = r_u8(texture + 12);
    w_u8(packet + 48, other + value + 255);
    value = r_u8(texture + 15);
    other = r_u8(texture + 13);
    w_u8(packet + 49, other + value + 255);
    display = r_u32(0x800880F0);
    head = r_u32(display + 0x70);
    header = r_u32(packet);
    link = r_u32(head);
    w_u32(packet, (header & 0xFF000000) | (link & 0xFFFFFF));
    head = r_u32(display + 0x70);
    counter = r_u32(0x800BF398);
    count = r_u32(0x800BF410);
    link = r_u32(head);
    result = counter + 1;
    w_u32(0x800BF398, result);
    w_u32(0x800BF410, count + 1);
    w_u32(head, (link & 0xFF000000) | (packet & 0xFFFFFF));
    return result;
}

uint32 td5_8001fa7c(uint32 car, uint32 view)
{
    uint32 offset = view * 4, previous, selected, table, cursor, index;
    uint32 first, second, record, point, vertices, value, other, kind;
    sint32 count, node;

    FUNCTION_MARKER(0x8001FA7C, "SLUS_006.10");
    count = r_s32(0x80088788);
    node = r_s16(car + 0x70);
    previous = r_u32(0x80087798 + offset);
    if (count > 0)
    {
        cursor = r_u32(0x800886EC);
        for (index = 0; (sint32)index < count; ++index)
        {
            first = r_u16(cursor + 2);
            if (node >= (sint32)first)
            {
                second = r_u16(cursor + 4);
                if ((sint32)second >= node)
                {
                    w_u32(0x80087798 + offset, index);
                    break;
                }
            }
            cursor += 16;
        }
    }
    selected = r_u32(0x80087798 + offset);
    if (previous == selected)
        return selected * 16;
    table = r_u32(0x800886EC);
    cursor = table + selected * 16;
    point = r_u16(cursor + 8);
    second = r_u16(cursor + 6);
    value = r_u32(0x800884C4);
    record = value + second * 24;
    w_u32(0x80088470 + offset, point);
    w_u32(0x80088720 + offset, second);
    first = r_u16(record + 4);
    vertices = r_u32(0x8008850C);
    value = (uint32)r_s16(vertices + (first + point) * 6);
    other = r_u32(record + 12);
    w_u32(0x80088388 + offset, (value + other) << 8);
    first = r_u16(record + 4);
    value = (uint32)r_s16(vertices + (first + point) * 6 + 4);
    other = r_u32(record + 20);
    w_u32(0x80088398 + offset, (value + other) << 8);
    selected = r_u32(0x80087798 + offset);
    value = r_u16(table + selected * 16 + 10);
    w_u32(0x80088440 + offset, 0);
    w_u32(0x80088390 + offset, value);
    selected = r_u32(0x80087798 + offset);
    kind = r_u16(table + selected * 16);
    w_u32(0x80088658 + offset, kind);
    switch (kind)
    {
        case 0:
            w_u32(0x800883C0 + offset, 0);
            w_u32(0x80087788 + offset, 20000);
            return 20000;
        case 1:
        case 2:
            w_u32(0x80088160, 4096);
            w_u32(0x800883C0 + offset, 0);
            w_u16(0x800887FC + view * 8, kind == 1 ? 512 : 0xFE00);
            w_u16(0x800887FE + view * 8, 196);
            w_u16(0x80088800 + view * 8, 0xFC00);
            return 0xFFFFFC00;
        case 3:
            w_u32(0x80088160, 4096);
            w_u32(0x800883C0 + offset, 0);
            return offset;
        case 4:
        case 5:
        case 9:
        case 10:
            selected = r_u32(0x80087798 + offset);
            table = r_u32(0x800886EC);
            value = r_u16(table + selected * 16 + 10);
            w_u32(0x80088160, 4096);
            w_u32(0x80088390 + offset, value);
            return 4096;
        case 6:
            w_u32(0x800883C0 + offset, 0);
            w_u32(0x80087788 + offset, 20000);
            w_u32(0x80088748 + offset, 0);
            selected = r_u32(0x80087798 + offset);
            table = r_u32(0x800886EC);
            value = r_u16(table + selected * 16 + 14);
            w_u32(0x80088560 + offset, value);
            selected = r_u32(0x80087798 + offset);
            value = r_u16(table + selected * 16 + 12);
            w_u32(0x80088778 + offset, value);
            return value;
        case 7:
            w_u32(0x80088160, 4096);
            w_u32(0x800883C0 + offset, 0);
            w_u16(0x800887FC + view * 8, 0);
            w_u16(0x800887FE + view * 8, 0xFF38);
            w_u16(0x80088800 + view * 8, 0);
            w_u32(0x80088440 + offset, 1);
            return 1;
        case 8:
            w_u32(0x80088160, 4096);
            w_u16(0x800887FE + view * 8, 0xFF38);
            w_u16(0x800887FC + view * 8, 0);
            w_u16(0x80088800 + view * 8, 0);
            w_u32(0x80088440 + offset, 1);
            return 1;
        default:
            return kind << 2;
    }
}

uint32 td5_8001fe94(uint32 car, uint32 view, uint32 original_stack)
{
    uint32 offset = view * 4, mode, value, previous;

    FUNCTION_MARKER(0x8001FE94, "SLUS_006.10");
    mode = r_u32(0x80088658 + offset);
    switch (mode)
    {
        case 0:
            td5_8001efac(car, view, original_stack - 0x18);
            break;
        case 1:
        case 2:
            td5_8001f700(car, view, original_stack - 0x18);
            break;
        case 3:
            td5_8001f5c4(car, view, original_stack - 0x18);
            break;
        case 4:
            value = r_u32(0x800883C0 + offset);
            w_u32(0x800883C0 + offset, value + 8);
            td5_8001e654(car, 1, view, original_stack - 0x18);
            break;
        case 5:
            value = r_u32(0x800883C0 + offset);
            w_u32(0x800883C0 + offset, value - 8);
            td5_8001e654(car, 1, view, original_stack - 0x18);
            break;
        case 6:
            value = r_u32(0x80088778 + offset);
            td5_8001f1e4(car, view, value, original_stack - 0x18);
            break;
        case 7:
            td5_8001ea78(car, view, original_stack - 0x18);
            break;
        case 8:
            w_u32(0x800883C0 + offset, 2048);
            td5_8001ea78(car, view, original_stack - 0x18);
            break;
        case 9:
            w_u32(0x800883C0 + offset, 2048);
            td5_8001e654(car, 1, view, original_stack - 0x18);
            break;
        case 10:
            w_u32(0x800883C0 + offset, 0);
            td5_8001e654(car, 1, view, original_stack - 0x18);
            break;
    }
    previous = r_u32(0x800884A0 + offset);
    value = r_u32(0x80088160);
    if (previous != value)
    {
        td5_8007880c();
        value = r_u32(0x80088160);
    }
    w_u32(0x800884A0 + offset, value);
    return value;
}

uint32 td5_8001efac(uint32 car, uint32 view, uint32 original_stack)
{
    uint32 frame = original_stack - 0x40;
    uint32 offset = view * 4, camera = 0x8009B3E0 + view * 12;
    uint32 record, pool, index, first, x, value, other, vertices, square;
    uint32 distance, coefficient, product;
    sint32 roll, amount;

    FUNCTION_MARKER(0x8001EFAC, "SLUS_006.10");
    record = r_u32(0x80088720 + offset);
    pool = r_u32(0x800884C4);
    index = r_u32(0x800886F0 + offset);
    record = pool + record * 24;
    first = r_u16(record + 4);
    x = r_u32(0x80088388 + offset);
    vertices = r_u32(0x8008850C);
    w_u32(camera, x);
    value = (uint32)r_s16(vertices + (first + index) * 6 + 2);
    other = r_u32(record + 16);
    x = r_u32(0x80088390 + offset);
    w_u32(camera + 4, (other + value + x) << 8);
    value = r_u32(0x80088398 + offset);
    w_u32(camera + 8, value);
    value = r_u32(car + 0x1C4);
    w_u32(frame + 0x10, value);
    value = r_u32(car + 0x1C8);
    w_u32(frame + 0x14, value);
    value = r_u32(car + 0x1CC);
    w_u32(frame + 0x18, value);
    td5_set_view_position(camera);
    roll = r_s32(0x80087790 + offset);
    td5_80079094(frame + 0x10, roll);
    value = r_u32(camera);
    other = r_u32(frame + 0x10);
    value = (uint32)((sint32)(value - other) >> 10);
    square = value * value;
    value = r_u32(camera + 4);
    other = r_u32(frame + 0x14);
    value = (uint32)((sint32)(value - other) >> 10);
    square += value * value;
    other = r_u32(frame + 0x18);
    value = r_u32(camera + 8);
    value = (uint32)((sint32)(value - other) >> 10);
    square += value * value;
    distance = (uint32)SquareRoot0((sint32)square) << 2;
    coefficient = r_u32(0x80087788 + offset);
    product = distance * coefficient;
    w_u32(0x80088640 + offset, distance);
    amount = (sint32)product / 4096;
    w_u32(0x80088648 + offset, (uint32)amount);
    if (amount < 4096)
        amount = 4096;
    else if (amount >= 20001)
        amount = 20000;
    w_u32(0x80088648 + offset, (uint32)amount);
    value = r_u32(0x80088648 + offset);
    w_u32(0x80088160, value);
    return value;
}

void td5_800a7b84(uint32 archive_error_index, uint32 original_stack, uint32 invalid_header_index)
{
    uint32 frame = original_stack - 0x78;
    uint32 value, other, allocation, bytes, pool, cursor, count, length;
    sint16 width, height;
    sint32 random;

    FUNCTION_MARKER(0x800A7B84, "SLUS_006.10");
    td5_debug_log(0x800A534C);
    td5_init_memory_pool(0x800BFD00, 0x13F700);
    td5_init_ordering_tables(0x1101);
    td5_8006c590(2);
    td5_set_projection_distance(180);
    td5_80078b6c(0, 0x10000);
    td5_set_back_color(50, 50, 50);
    td5_set_light(0, 0x800AE0C8, 110, 110, 110);
    td5_set_light(1, 0x800AE0C8, 0, 0, 0);
    td5_set_light(2, 0x800AE0C8, 0, 0, 0);
    td5_set_fog(3000, 16000, 0, 0, 0);
    value = r_u32(0x800A535C);
    other = r_u32(0x800A5360);
    w_u32(frame + 0x18, value);
    w_u32(frame + 0x1C, other);
    allocation = td5_alloc_memory(0x80000);
    td5_load_archive_file((const char *)psx_addr(0x800A5364, 17), allocation, archive_error_index);
    w_u16(frame + 0x1A, 0);
    width = r_s16(frame + 0x1C);
    height = r_s16(frame + 0x1E);
    bytes = width > 0 && height > 0 ? (uint32)width * (uint32)height * 2 : 0;
    LoadImagePSX((PSX_RECT *)psx_addr(frame + 0x18, 8), (uint32 *)psx_addr(allocation, bytes));
    w_u16(frame + 0x1A, 256);
    width = r_s16(frame + 0x1C);
    height = r_s16(frame + 0x1E);
    bytes = width > 0 && height > 0 ? (uint32)width * (uint32)height * 2 : 0;
    LoadImagePSX((PSX_RECT *)psx_addr(frame + 0x18, 8), (uint32 *)psx_addr(allocation + 0x40000, bytes));
    DrawSync(0);
    value = td5_free_memory(allocation);
    bytes = td5_archive_aligned_size((const char *)psx_addr(0x800A5378, 17), 1, archive_error_index, value);
    pool = td5_alloc_memory(bytes);
    w_u32(0x800884B4, pool);
    w_u32(0x80088660, pool);
    td5_load_archive_file((const char *)psx_addr(0x800A5378, 17), pool, archive_error_index);
    pool = r_u32(0x800884B4);
    value = r_u32(pool);
    count = 1;
    cursor = pool + 4;
    if (value != 0)
    {
        do
        {
            value = r_u32(cursor);
            w_u32(cursor, value + pool);
            value = r_u32(pool);
            ++count;
            cursor += 4;
        } while (value >= count);
    }
    pool = r_u32(0x80088660);
    td5_set_texture_table(r_u32(pool + 4));
    allocation = td5_alloc_memory(0x40814);
    value = r_u32(0x80087634) + 1;
    w_u32(0x80088608, allocation);
    value = (uint32)((sint32)value % 6);
    w_u32(0x80087634, value);
    length = sprintfPSX(frame + 0x20, 0x800A538C, value + 1);
    allocation = r_u32(0x80088608);
    td5_load_archive_file((const char *)psx_addr(frame + 0x20, length + 1), allocation, archive_error_index);
    if (r_u32(0x80087654) == 0)
    {
        td5_800557c0();
        td5_stop_sound_voice(20);
    }
    else
    {
        td5_wait_loading_screen();
        td5_fade_audio();
    }
    random = randPSX();
    td5_play_xa(random % 9);
    td5_800b25e0();
    w_u16(frame + 0x64, 512);
    w_u16(frame + 0x66, 512);
    w_u16(frame + 0x60, 0);
    w_u16(frame + 0x62, 0);
    ClearImage((PSX_RECT *)psx_addr(frame + 0x60, 8), 0, 0, 0);
    SetDispMask(1);
    td5_prepare_display(2);
    value = r_u32(0x800876F8);
    other = r_u32(0x800876FC);
    w_u32(0x800AEA2C, 0);
    w_u16(0x800976CC, value);
    w_u16(0x80097740, value);
    w_u16(0x800976CE, other);
    w_u16(0x80097742, other);
    td5_swap_display_buffer();
    td5_apply_display_buffer();
    td5_debug_log(0x800A539C);
    td5_800a79b0(frame, invalid_header_index, archive_error_index);
    td5_debug_log(0x800A53A8);
}

void td5_800a90dc(void)
{
    uint32 channel, word_offset, half_offset, pair_offset, mode, family;
    uint32 value, current, low, high, zoom, product, control, field, index;
    sint32 before, after, distance;

    FUNCTION_MARKER(0x800A90DC, "SLUS_006.10");
    for (channel = 0; channel < 2; ++channel)
    {
        word_offset = channel * 4;
        half_offset = channel * 2;
        pair_offset = channel * 8;
        mode = r_u32(0x80088450 + word_offset);
        family = 0x800AE100;
        if (mode == 0x2000 || mode == 0x5000 || mode == 0x7000)
        {
            if (mode == 0x2000)
            {
                value = r_u32(0x80087684 + word_offset);
                family = value != 0 ? 0x800AE164 : 0x800AE1C8;
            }
            else
                family = 0x800AE22C;
            value = r_u32(0x8007DC24 + pair_offset);
            w_u16(0x80087808 + half_offset, value);
            current = r_u32(0x8007DC24 + pair_offset);
            low = r_u32(0x8007DC44 + pair_offset);
            high = r_u32(0x8007DC34 + pair_offset);
            before = (sint32)(current - low);
            after = (sint32)(high - current);
            distance = after < before ? after : before;
            zoom = r_u32(0x8008767C + word_offset);
            product = (uint32)distance * (0x1200u - zoom);
            control = r_u32(0x80087674 + word_offset);
            w_u16(0x8008780C + half_offset, (uint32)((sint32)product / 4608));
            w_u16(0x80087810 + half_offset, (uint32)((sint32)control / 128));
            if (mode == 0x2000)
            {
                value = r_u32(0x8007DC24 + pair_offset);
                w_u16(0x80087808 + half_offset, value);
                w_u16(0x80087820 + half_offset, 128);
                w_u16(0x80087818 + half_offset, 128);
            }
        }
        for (field = 0; field < 10; ++field)
        {
            index = r_u32(0x8008766C + word_offset);
            value = r_u16(family + field * 10 + index * 2);
            switch (field)
            {
                case 0:
                    mode = r_u32(0x80088450 + word_offset);
                    value = r_u16(0x800AE9A0 + value * 2);
                    if (mode == 0x7000 || mode == 0x5000)
                        w_u32(0x800887B0 + word_offset, value);
                    else
                    {
                        w_u16(0x800877E0 + half_offset, value);
                        w_u32(0x800887B0 + word_offset, 0);
                    }
                    break;
                case 1:
                    mode = r_u32(0x80088450 + word_offset);
                    value = r_u16(0x800AE9A0 + value * 2);
                    if (mode == 0x7000 || mode == 0x5000)
                        w_u32(0x80088380 + word_offset, value);
                    else
                    {
                        w_u16(0x800877E4 + half_offset, value);
                        w_u32(0x80088380 + word_offset, 0);
                    }
                    break;
                case 4:
                    value = r_u16(0x800AE9A0 + value * 2);
                    w_u16(0x800877F4 + half_offset, value);
                    break;
                case 5:
                    value = r_u16(0x800AE9A0 + value * 2);
                    w_u16(0x800877F8 + half_offset, value);
                    break;
                case 6:
                    value = r_u16(0x800AE9A0 + value * 2);
                    w_u16(0x800877E8 + half_offset, value);
                    break;
                case 7:
                    value = r_u16(0x800AE9A0 + value * 2);
                    w_u16(0x80087804 + half_offset, value);
                    break;
                case 8:
                    value = r_u16(0x800AE9A0 + value * 2);
                    w_u16(0x800877FC + half_offset, value);
                    break;
                case 9:
                    value = r_u16(0x800AE9A0 + value * 2);
                    w_u16(0x80087800 + half_offset, value);
                    break;
            }
        }
    }
}

void td5_800adb84(void)
{
    uint32 index;

    FUNCTION_MARKER(0x800ADB84, "SLUS_006.10");
    memset(psx_addr(0x8009DB38, 101), 0, 101);
    for (index = 0; index < 6; ++index)
        w_u8(0x8009DB56 + index, 1);
    w_u8(0x8009DB65, 1);
    w_u8(0x8009DB88, 2);
    w_u8(0x8009DB81, 2);
    w_u8(0x8009DB7F, 2);
    w_u8(0x8009DB82, 2);
    w_u8(0x8009DB8B, 2);
    w_u8(0x8009DB8E, 2);
    w_u8(0x8009DB8D, 2);
    w_u8(0x8009DB8C, 2);
    w_u8(0x8009DB92, 2);
    w_u8(0x8009DB95, 2);
    w_u8(0x8009DB94, 2);
    w_u8(0x8009DB93, 2);
    for (index = 0; index < 6; ++index)
        w_u8(0x8009DB72 + index, 1);
    w_u8(0x8009DB7E, 1);
    w_u8(0x8009DB80, 1);
}

void td5_8007a8b4(void)
{
    FUNCTION_MARKER(0x8007A8B4, "SLUS_006.10");
}

void td5_80013c9c(void)
{
    FUNCTION_MARKER(0x80013C9C, "SLUS_006.10");
    for (;;)
        td5_8007a8b4();
}

void td5_80013cf8(void)
{
    FUNCTION_MARKER(0x80013CF8, "SLUS_006.10");
}

uint32 td5_80015a3c(uint32 source)
{
    uint32 value, next;

    FUNCTION_MARKER(0x80015A3C, "SLUS_006.10");
    value = r_u8(source);
    if (value != 'R')
    {
        td5_debug_log(0x80010018);
        return 'R';
    }
    value = r_u8(source + 1);
    if (value != 'N')
    {
        td5_debug_log(0x80010018);
        return 'N';
    }
    value = r_u8(source + 2);
    if (value != 'C')
    {
        td5_debug_log(0x80010018);
        return 'C';
    }
    value = r_u8(source + 7);
    next = r_u8(source + 6);
    value |= next << 8;
    next = r_u8(source + 5);
    value |= next << 16;
    next = r_u8(source + 4);
    value |= next << 24;
    return value + 1024;
}

sint32 td5_80015b64(uint32 original_stack, uint32 inherited_s7)
{
    TD5_STACK_EXCHANGE_RESULT stack;

    FUNCTION_MARKER(0x80015B64, "SLUS_006.10");
    stack = td5_80068510(0x1F800400, original_stack - 0x18);
    w_u32(0x80088714, stack.previous_stack);
    w_u32(0x8008858C, 3);
    td5_8004a3a0(stack.replacement_stack, original_stack - 0x18, inherited_s7);
    stack = td5_80068510(r_u32(0x80088714), stack.replacement_stack);
    td5_8006b9e0(0, 0, 0);
    return (sint32)td5_8006b9e0(1, 0, 0);
}

uint32 td5_80015d08(void)
{
    uint32 index;

    FUNCTION_MARKER(0x80015D08, "SLUS_006.10");
    for (index = 0; index < 40000; ++index)
    {
        SetDispMask(0);
        SetDispMask(1);
    }
    for (index = 0; index < 40000; ++index)
        SetDispMask(1);
    return 1;
}

uint32 td5_800164e0(sint32 record)
{
    uint32 offset = (uint32)record * 120;
    uint32 value, previous, index;

    FUNCTION_MARKER(0x800164E0, "SLUS_006.10");
    if (record < 40)
    {
        value = r_u32(0x800987A0);
        previous = r_u32(0x8007C538 + offset);
        if (value < previous)
        {
            value = r_u32(0x800987A0);
            w_u32(0x8007C538 + offset, value);
            w_u8(0x8007C53C + offset, 0);
            w_u8(0x8007C53D + offset, 0);
            value = r_u8(0x8008CF72);
            w_u8(0x8007C546 + offset, value);
            value = r_u16(0x800987AA);
            w_u16(0x8007C548 + offset, value);
            value = r_u16(0x800987A8);
            w_u16(0x8007C54A + offset, value);
            w_u8(0x8007C547 + offset, 0);
        }
        value = r_u32(0x80087730);
        if (value == 0)
            return value;
        value = r_u32(0x80087734);
        if (value != 0)
            return value;
        value = r_u16(0x800987C6);
        w_u32(0x8007C538 + offset, value);
        for (index = 1; index < 5; ++index)
        {
            value = r_u16(0x800987C6 + index * 2);
            if (value != 0)
            {
                value = r_u16(0x800987C6 + index * 2);
                previous = r_u32(0x8007C538 + offset);
                if (value < previous)
                {
                    value = r_u16(0x800987C6 + index * 2);
                    w_u32(0x8007C538 + offset, value);
                }
            }
        }
        return 5;
    }
    value = r_u32(record == 44 ? 0x8009DA24 : 0x8009DA28);
    w_u32(0x8007C538 + offset, value);
    w_u8(0x8007C53C + offset, 0);
    w_u8(0x8007C53D + offset, 0);
    value = r_u8(0x8009DA21);
    w_u8(0x8007C546 + offset, value);
    value = r_u16(0x8009DA2C);
    w_u16(0x8007C548 + offset, value);
    value = r_u16(0x8009DA2E);
    w_u16(0x8007C54A + offset, value);
    value = r_u8(0x8009DA24);
    w_u8(0x8007C547 + offset, value);
    return offset;
}

uint64 td5_80016a28(void)
{
    uint32 index, source;

    FUNCTION_MARKER(0x80016A28, "SLUS_006.10");
    for (index = 0; index < 47; ++index)
    {
        source = r_u32(0x8008773C);
        memcpy(psx_addr(0x8007C53D + index * 120, 8), psx_addr(source, 8), 8);
        w_u8(0x8007C53C + index * 120, 1);
    }
    return 47;
}

uint64 td5_80016af4(void)
{
    uint32 index, offset;

    FUNCTION_MARKER(0x80016AF4, "SLUS_006.10");
    for (index = 0; index < 47; ++index)
    {
        offset = index * 120;
        w_u8(0x8007C4D8 + offset, 0);
        w_u8(0x8007C4EC + offset, 0);
        w_u8(0x8007C500 + offset, 0);
        w_u8(0x8007C514 + offset, 0);
        w_u8(0x8007C528 + offset, 0);
        w_u8(0x8007C53C + offset, 0);
    }
    return 47;
}

uint32 td5_80016c2c(void)
{
    uint32 index, offset, current, previous;

    FUNCTION_MARKER(0x80016C2C, "SLUS_006.10");
    if (r_u32(0x80087720) == 0 && r_u32(0x80087730) == 0 && (r_u32(0x8008763C) == 0 || r_u32(0x800883F8) == 0 || r_u32(0x80088578) == 0))
        return 0;
    for (index = 0; index < 47; ++index)
    {
        if (index == 40)
            continue;
        offset = index * 120;
        current = r_u32(0x8007C538 + offset);
        previous = r_u32(0x8007C524 + offset);
        if (index != 44)
        {
            if (previous >= current && r_u32(0x8007C538 + offset) != 0)
                return 1;
        }
        else
        {
            if (current >= previous && r_u32(0x8007C538 + offset) != 0)
                return 1;
            current = r_u8(0x8007C547 + offset);
            previous = r_u8(0x8007C533 + offset);
            if (current >= previous && r_u8(0x8007C547 + offset) != 0)
                return 1;
        }
    }
    return 0;
}

uint32 td5_8001a1bc(uint32 buffer, uint32 archive_error_index)
{
    FUNCTION_MARKER(0x8001A1BC, "SLUS_006.10");
    td5_show_legal_image(buffer, 0x80010278, 4, archive_error_index);
    td5_show_legal_image(buffer, 0x8001028C, 4, archive_error_index);
    td5_show_legal_image(buffer, 0x800102A0, 4, archive_error_index);
    td5_show_legal_image(buffer, 0x800102B4, 4, archive_error_index);
    td5_show_legal_image(buffer, 0x800102C8, 4, archive_error_index);
    SetDispMask(0);
    return 3;
}

void td5_8001a26c(void)
{
    FUNCTION_MARKER(0x8001A26C, "SLUS_006.10");
}

sint32 td5_8001a28c(void)
{
    FUNCTION_MARKER(0x8001A28C, "SLUS_006.10");
    /* The native movie boundary does not execute an external player image */
    td5_8001a26c();
    td5_play_movies(r_u8(r_u32(0x80087618)));
    stopIntr();
    ResetCallback();
    CdInit();
    return td5_800557c0();
}

sint32 td5_8007b9b8(uint32 filename, uint32 destination, sint32 bytes)
{
    sint32 sectors;

    FUNCTION_MARKER(0x8007B9B8, "SLUS_006.10");
    sectors = bytes / 2048;
    td5_read_data_prefix(filename, destination, (uint32)sectors * 2048);
    return 0;
}

sint32 td5_8001ae4c(void)
{
    uint32 value;

    FUNCTION_MARKER(0x8001AE4C, "SLUS_006.10");
    /* Movie playback no longer loads or executes the PS1 player */
    DrawSync(0);
    td5_80055efc();
    StopRCnt(0xF2000000, 0x80086B0C, 0x80086B14);
    StopRCnt(0xF2000001, 0x80086B0C, 0x80086B14);
    StopRCnt(0xF2000002, 0x80086B0C, 0x80086B14);
    StopRCnt(0xF2000003, 0x80086B0C, 0x80086B14);
    td5_80065990();
    ResetGraph(3);
    stopIntr();
    EnterCriticalSectionPSX();
    value = r_u32(0x80087618);
    w_u8(value, 1);
    td5_play_movies(r_u8(r_u32(0x80087618)));
    ResetCallback();
    CdInit();
    ResetGraph(3);
    SetGraphDebug(0);
    DrawSync(0);
    SetDispMask(0);
    td5_init_sound();
    td5_80055b3c();
    return (sint32)td5_load_menu_sounds();
}

uint32 td5_8001ea78(uint32 car, uint32 view, uint32 original_stack)
{
    uint32 angles = original_stack - 0x30 + 0x10;
    uint32 camera = 0x8009B3E0 + view * 12;
    uint32 value, rotation, coordinate;

    FUNCTION_MARKER(0x8001EA78, "SLUS_006.10");
    value = r_u16(car + 0x1D0);
    w_u16(angles, value + 0x800);
    value = r_u32(0x800883C0 + view * 4);
    rotation = r_u16(car + 0x1D2);
    w_u16(angles + 2, value + 0x800 - rotation);
    value = r_u16(car + 0x1D4);
    w_u16(angles + 4, 0 - value);
    td5_set_view_angles(angles);
    ApplyMatrix((MATRIX *)psx_addr(0x8009D928, 20), (SVECTOR *)psx_addr(0x800887FC + view * 8, 8), (VECTOR *)psx_addr(camera, 12));
    value = r_u32(camera);
    coordinate = r_u32(car + 0x1C4);
    w_u32(camera, (value << 8) + coordinate);
    value = r_u32(camera + 4);
    coordinate = r_u32(car + 0x1C8);
    w_u32(camera + 4, (value << 8) + coordinate);
    value = r_u32(camera + 8);
    coordinate = r_u32(car + 0x1CC);
    w_u32(camera + 8, (value << 8) + coordinate);
    td5_set_view_position(camera);
    td5_set_view_angles(angles);
    value = (uint32)r_s16(car + 0x70);
    w_u32(0x80088720 + view * 4, value);
    return value;
}

uint32 td5_8001f1e4(uint32 car, uint32 view, uint32 offset, uint32 original_stack)
{
    uint32 frame = original_stack - 0xD0;
    uint32 stride = view * 4, camera = 0x8009B3E0 + view * 12;
    uint32 i, a, b, c, d, value, other, step, counter;
    uint32 records, vertices, table, output, record, index, first;
    uint32 coefficients = 0x8008CCB0 + view * 60;
    uint32 square, distance, product;
    sint32 amount;

    FUNCTION_MARKER(0x8001F1E4, "SLUS_006.10");
    for (i = 0; i < 80; i += 16)
    {
        a = r_u32(0x80010940 + i);
        b = r_u32(0x80010944 + i);
        c = r_u32(0x80010948 + i);
        d = r_u32(0x8001094C + i);
        w_u32(frame + 0x60 + i, a);
        w_u32(frame + 0x64 + i, b);
        w_u32(frame + 0x68 + i, c);
        w_u32(frame + 0x6C + i, d);
    }
    step = r_u32(0x80088560 + stride);
    counter = r_u32(0x80088748 + stride);
    value = (sint32)counter < (sint32)(4095 - step) ? counter + step : 4095;
    w_u32(0x80088748 + stride, value);
    records = r_u32(0x800884C4);
    vertices = r_u32(0x8008850C);
    table = frame + 0x60 + (offset << 4);
    for (i = 0; i < 4; ++i)
    {
        value = (uint32)r_s16(table);
        other = r_u32(0x80088720 + stride);
        a = (uint32)r_s16(table + 2);
        index = r_u32(0x80088470 + stride) + a;
        record = records + (other + value) * 24;
        first = r_u16(record + 4);
        value = (uint32)r_s16(vertices + (first + index) * 6);
        other = r_u32(record + 12);
        output = frame + 0x30 + i * 12;
        w_u32(output, (value + other) << 8);
        first = r_u16(record + 4);
        value = (uint32)r_s16(vertices + (first + index) * 6 + 4);
        other = r_u32(record + 20);
        w_u32(output + 8, (value + other) << 8);
        first = r_u16(record + 4);
        table += 4;
        value = (uint32)r_s16(vertices + (first + index) * 6 + 2);
        other = r_u32(record + 16);
        a = r_u32(0x80088390 + stride);
        w_u32(output + 4, (value + other + a) << 8);
    }
    td5_8007a570(coefficients, frame + 0x30);
    value = r_u32(0x80088748 + stride);
    td5_8007a708(camera, coefficients, value);
    value = r_u32(car + 0x1C4);
    w_u32(frame + 0x10, value);
    value = r_u32(car + 0x1C8);
    w_u32(frame + 0x14, value);
    value = r_u32(car + 0x1CC);
    w_u32(frame + 0x18, value);
    td5_set_view_position(camera);
    amount = r_s32(0x80087790 + stride);
    td5_80079094(frame + 0x10, amount);
    value = r_u32(camera);
    other = r_u32(frame + 0x10);
    value = (uint32)((sint32)(value - other) >> 10);
    square = value * value;
    value = r_u32(camera + 4);
    other = r_u32(frame + 0x14);
    value = (uint32)((sint32)(value - other) >> 10);
    square += value * value;
    other = r_u32(frame + 0x18);
    value = r_u32(camera + 8);
    value = (uint32)((sint32)(value - other) >> 10);
    square += value * value;
    distance = (uint32)SquareRoot0((sint32)square) << 2;
    value = r_u32(0x80087788 + stride);
    product = distance * value;
    w_u32(0x80088640 + stride, distance);
    amount = (sint32)product / 4096;
    w_u32(0x80088648 + stride, (uint32)amount);
    if (amount < 4096)
        amount = 4096;
    else if (amount >= 20001)
        amount = 20000;
    w_u32(0x80088648 + stride, (uint32)amount);
    value = r_u32(0x80088648 + stride);
    w_u32(0x80088160, value);
    return value;
}

uint32 td5_8001f5c4(uint32 car, uint32 view, uint32 original_stack)
{
    uint32 frame = original_stack - 0x38;
    uint32 offset = view * 4, camera = 0x8009B3E0 + view * 12;
    uint32 record, pool, index, first, vertices, value, other, height;
    sint32 roll;

    FUNCTION_MARKER(0x8001F5C4, "SLUS_006.10");
    record = r_u32(0x80088720 + offset);
    pool = r_u32(0x800884C4);
    index = r_u32(0x800886F0 + offset);
    record = pool + record * 24;
    first = r_u16(record + 4);
    value = r_u32(0x80088388 + offset);
    vertices = r_u32(0x8008850C);
    w_u32(camera, value);
    value = (uint32)r_s16(vertices + first * 6 + index * 6 + 2);
    other = r_u32(record + 16);
    height = r_u32(0x80088390 + offset);
    w_u32(camera + 4, (other + value + height) << 8);
    value = r_u32(0x80088398 + offset);
    w_u32(camera + 8, value);
    value = r_u32(car + 0x1C4);
    w_u32(frame + 0x10, value);
    value = r_u32(car + 0x1C8);
    height = r_u32(0x800881B8 + offset);
    w_u32(frame + 0x14, value + height);
    value = r_u32(car + 0x1CC);
    w_u32(frame + 0x18, value);
    td5_set_view_position(camera);
    roll = r_s32(0x80087790 + offset);
    return td5_80079094(frame + 0x10, roll);
}

uint32 td5_8001f700(uint32 car, uint32 view, uint32 original_stack)
{
    uint32 angles = original_stack - 0x40 + 0x20;
    uint32 camera = 0x8009B3E0 + view * 12;
    uint32 value, coordinate;

    FUNCTION_MARKER(0x8001F700, "SLUS_006.10");
    value = r_u16(car + 0x1D0);
    w_u16(angles, value + 0x800);
    value = r_u16(car + 0x1D2);
    w_u16(angles + 2, 0x800 - value);
    value = r_u16(car + 0x1D4);
    w_u16(angles + 4, 0 - value);
    td5_set_view_angles(angles);
    ApplyMatrix((MATRIX *)psx_addr(car + 0x110, 20), (SVECTOR *)psx_addr(0x800887FC + view * 8, 8), (VECTOR *)psx_addr(camera, 12));
    value = r_u32(camera);
    coordinate = r_u32(car + 0x1C4);
    w_u32(camera, (value << 8) + coordinate);
    value = r_u32(camera + 4);
    coordinate = r_u32(car + 0x1C8);
    w_u32(camera + 4, (value << 8) + coordinate);
    value = r_u32(camera + 8);
    coordinate = r_u32(car + 0x1CC);
    w_u32(camera + 8, (value << 8) + coordinate);
    td5_set_view_position(camera);
    td5_set_view_angles(angles);
    value = (uint32)r_s16(car + 0x70);
    w_u32(0x80088720 + view * 4, value);
    return value;
}

uint32 td5_800254c0(uint32 car, uint32 descriptor)
{
    uint32 rank, level, factor, value, coefficient, a, b, c, d;
    sint32 random, amount;

    FUNCTION_MARKER(0x800254C0, "SLUS_006.10");
    rank = r_u8(car + 0x34D);
    level = r_u32(0x8008772C);
    if ((sint32)level >= (sint32)rank)
    {
        random = randPSX();
        if (random % 100 < 80 && r_u8(car + 0x343) < 4)
            td5_80053098();
    }
    factor = (uint32)r_s16(car + 0x316);
    value = (uint32)r_s16(descriptor + 0x68);
    value *= factor * 26;
    amount = (sint32)value / 256;
    coefficient = r_u8(car + 0x343);
    coefficient = r_u32(0x8007DF1C + coefficient * 4);
    value = (uint32)amount * coefficient;
    amount = (sint32)value / 256;
    a = r_u32(car + 0x2C4);
    b = r_u32(car + 0x2C8);
    c = r_u32(car + 0x2CC);
    d = r_u32(car + 0x2D0);
    w_u32(car + 0x2C4, a + (uint32)amount);
    w_u32(car + 0x2C8, b + (uint32)amount);
    w_u32(car + 0x2CC, c - (uint32)amount);
    w_u32(car + 0x2D0, d - (uint32)amount);
    return (uint32)amount;
}

uint32 td5_8002740c(void)
{
    sint32 first, second, limit, difference;
    uint32 value;

    FUNCTION_MARKER(0x8002740C, "SLUS_006.10");
    first = r_s16(0x80098514);
    second = r_s16(0x80098874);
    if (first < second)
    {
        limit = r_s32(0x80087828);
        difference = (second - first) * 2;
        if (difference < limit)
            limit = difference;
        w_u16(0x80087824, (uint32)limit + 256);
        value = 256 - (uint32)limit;
    }
    else
    {
        limit = r_s32(0x80087828);
        difference = (first - second) * 2;
        if (difference < limit)
            limit = difference;
        w_u16(0x80087824, 256 - (uint32)limit);
        value = (uint32)limit + 256;
    }
    w_u16(0x80087826, value);
    return value;
}

uint32 td5_80027498(uint32 car)
{
    uint32 input, output = car + 0x80;
    uint32 i, value, x, y, z;

    FUNCTION_MARKER(0x80027498, "SLUS_006.10");
    input = r_u32(car + 0x180);
    SetRotMatrix((MATRIX *)psx_addr(car + 0x110, 20));
    SetTransMatrix((MATRIX *)psx_addr(car + 0x110, 32));
    for (i = 0; i < 4; ++i)
    {
        value = r_u32(input);
        xport_gte_write_data(0, value);
        value = r_u32(input + 4);
        xport_gte_write_data(1, value);
        xport_gte_execute(0x480012);
        value = xport_gte_read_data(25);
        w_u32(output, value);
        value = xport_gte_read_data(26);
        w_u32(output + 4, value);
        value = xport_gte_read_data(27);
        w_u32(output + 8, value);
        x = r_u32(output);
        y = r_u32(output + 4);
        z = r_u32(output + 8);
        input += 8;
        w_u32(output, x << 8);
        w_u32(output + 4, y << 8);
        w_u32(output + 8, z << 8);
        output += 12;
    }
    return 0;
}

uint32 td5_800280fc(uint32 car)
{
    uint32 magnitude, slot, value, product, result, i;
    sint32 velocity, random;

    FUNCTION_MARKER(0x800280FC, "SLUS_006.10");
    velocity = r_s32(car + 0x2EC);
    slot = r_u8(car + 0x34D);
    velocity >>= 8;
    magnitude = velocity < 0 ? 0 - (uint32)velocity : (uint32)velocity;
    random = (sint32)td5_random_for_slot(slot);
    for (i = 0; i < 4; ++i)
    {
        product = magnitude * (uint32)(random % 16 - 8);
        value = r_u32(car + 0x2C4 + i * 4);
        if (i < 3)
            slot = r_u8(car + 0x34D);
        result = value + product;
        w_u32(car + 0x2C4 + i * 4, result);
        if (i < 3)
            random = (sint32)td5_random_for_slot(slot);
    }
    return result;
}

uint32 td5_80028230(uint32 car)
{
    uint32 magnitude, slot, value, product, result, i;
    sint32 velocity, random;

    FUNCTION_MARKER(0x80028230, "SLUS_006.10");
    velocity = r_s32(car + 0x2EC);
    slot = r_u8(car + 0x34D);
    velocity >>= 9;
    magnitude = velocity < 0 ? 0 - (uint32)velocity : (uint32)velocity;
    random = (sint32)td5_random_for_slot(slot);
    for (i = 0; i < 4; ++i)
    {
        product = magnitude * (uint32)(random % 16 - 8);
        value = r_u32(car + 0x2C4 + i * 4);
        if (i < 3)
            slot = r_u8(car + 0x34D);
        result = value + product;
        w_u32(car + 0x2C4 + i * 4, result);
        if (i < 3)
            random = (sint32)td5_random_for_slot(slot);
    }
    return result;
}

void td5_80028398(void)
{
    FUNCTION_MARKER(0x80028398, "SLUS_006.10");
}

uint32 td5_800283a0(uint32 car)
{
    uint32 magnitude, slot, value, result, product;
    sint32 velocity, random;

    FUNCTION_MARKER(0x800283A0, "SLUS_006.10");
    velocity = r_s32(car + 0x2EC);
    slot = r_u8(car + 0x34D);
    velocity >>= 7;
    magnitude = velocity < 0 ? 0 - (uint32)velocity : (uint32)velocity;
    random = (sint32)td5_random_for_slot(slot);
    product = magnitude * (uint32)(random % 16 - 8);
    value = r_u32(car + 0x2C4);
    slot = r_u8(car + 0x34D);
    result = value + product;
    w_u32(car + 0x2C4, result);
    w_u32(car + 0x2C8, result);
    random = (sint32)td5_random_for_slot(slot);
    product = magnitude * (uint32)(random % 16 - 8);
    value = r_u32(car + 0x2CC);
    result = value + product;
    w_u32(car + 0x2CC, result);
    w_u32(car + 0x2D0, result);
    return result;
}
