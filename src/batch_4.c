#include "td5_functions.h"
#include <string.h>
#include <stdio.h>

void td5_800158d4(uint32 source, uint32 destination, sint32 capacity)
{
    uint32 unpacked;
    uint32 byte;

    FUNCTION_MARKER(0x800158D4, "SLUS_006.10");
    if (r_u8(source) != 'R' || r_u8(source + 1) != 'N' || r_u8(source + 2) != 'C')
    {
        td5_debug_log(0x80010018);
        return;
    }
    unpacked = r_u8(source + 7);
    byte = r_u8(source + 6);
    unpacked |= byte << 8;
    byte = r_u8(source + 5);
    unpacked |= byte << 16;
    byte = r_u8(source + 4);
    unpacked |= byte << 24;
    if (capacity < (sint32)unpacked)
    {
        td5_debug_log(0x80010024);
        return;
    }
    td5_800684f0();
    td5_8007bdf4(source, destination);
    td5_80068500();
}

uint32 td5_80068170(uint32 mode)
{
    FUNCTION_MARKER(0x80068170, "SLUS_006.10");
    return xport_bios_init_card(mode);
}

uint32 td5_80068180(void)
{
    FUNCTION_MARKER(0x80068180, "SLUS_006.10");
    return xport_bios_start_card();
}

void td5_8007a310(sint32 x, sint32 y)
{
    FUNCTION_MARKER(0x8007A310, "SLUS_006.10");
    SetGeomOffset(x, y);
}

void td5_8007a330(sint32 distance)
{
    FUNCTION_MARKER(0x8007A330, "SLUS_006.10");
    SetGeomScreen(distance);
}

static uint32 td5_rnc_add_checked(uint32 left, uint32 right, uint32 pc)
{
    sint64 result = (sint64)(sint32)left + (sint64)(sint32)right;

    if (result < (-2147483647LL - 1) || result > 2147483647LL)
        xport_mips_overflow_exception(pc);
    return (uint32)result;
}

static uint32 td5_rnc_next_bit(uint32 *bits, uint32 *cursor, uint32 increment_pc)
{
    uint32 bit;
    uint32 byte;

    *bits += *bits;
    bit = (*bits >> 8) & 1;
    *bits &= 255;
    if (*bits == 0)
    {
        byte = r_u8(*cursor);
        *cursor = td5_rnc_add_checked(*cursor, 1, increment_pc);
        *bits = byte * 2 + bit;
        bit = (*bits >> 8) & 1;
    }
    return bit;
}

void td5_8007bdf4(uint32 source, uint32 destination)
{
    uint32 unpacked;
    uint32 packed;
    uint32 byte;
    uint32 cursor;
    uint32 bits;
    uint32 bit;
    uint32 length;
    uint32 high;
    uint32 distance;
    uint32 back;
    uint32 first;
    uint32 second;
    uint32 count;
    uint32 scratch;
    sint32 pairs;
    sint32 groups;
    uint32 index;

    FUNCTION_MARKER(0x8007BDF4, "SLUS_006.10");
    unpacked = r_u8(source + 4);
    byte = r_u8(source + 5);
    unpacked = (unpacked << 24) | (byte << 16);
    byte = r_u8(source + 6);
    unpacked |= byte << 8;
    byte = r_u8(source + 7);
    unpacked |= byte;
    w_u32(0x8007C4B8, unpacked);
    cursor = td5_rnc_add_checked(source + 8, 10, 0x8007BE78);
    (void)td5_rnc_add_checked(destination, unpacked, 0x8007BE84);
    packed = r_u8(source + 8);
    byte = r_u8(source + 9);
    packed = (packed << 24) | (byte << 16);
    byte = r_u8(source + 10);
    packed |= byte << 8;
    byte = r_u8(source + 11);
    packed |= byte;
    (void)td5_rnc_add_checked(cursor, packed, 0x8007BEB8);
    w_u16(0x8007C4B6, 0);
    bits = r_u8(cursor);
    cursor = td5_rnc_add_checked(cursor, 1, 0x8007BED4);
    bits = (bits * 2 + 1) * 2;
    for (;;)
    {
        bits += bits;
        bit = (bits >> 8) & 1;
        if (bit == 0)
        {
            byte = r_u8(cursor);
            cursor = td5_rnc_add_checked(cursor, 1, 0x8007C244);
            w_u8(destination, byte);
            destination = td5_rnc_add_checked(destination, 1, 0x8007C24C);
            bits += bits;
            bit = (bits >> 8) & 1;
            if (bit == 0)
                goto literal_second;
        }
        bits &= 255;
        if (bits == 0)
        {
            byte = r_u8(cursor);
            cursor = td5_rnc_add_checked(cursor, 1, 0x8007C200);
            bits = byte * 2 + bit;
            bit = (bits >> 8) & 1;
            if (bit == 0)
                goto literal_second;
        }
        length = 2;
        high = 0;
        bit = td5_rnc_next_bit(&bits, &cursor, 0x8007C350);
        if (bit == 0)
        {
            length = 4 + td5_rnc_next_bit(&bits, &cursor, 0x8007BF10);
            bit = td5_rnc_next_bit(&bits, &cursor, 0x8007BF2C);
            if (bit != 0)
            {
                --length;
                length = length * 2 + td5_rnc_next_bit(&bits, &cursor, 0x8007BF48);
                if (length == 9)
                {
                    for (index = 0; index < 4; ++index)
                        high = high * 2 + td5_rnc_next_bit(&bits, &cursor, 0x8007BEF4);
                    groups = (sint32)high + 2;
                    do
                    {
                        first = r_u8(cursor);
                        second = r_u8(cursor + 1);
                        w_u8(destination, first);
                        w_u8(destination + 1, second);
                        first = r_u8(cursor + 2);
                        second = r_u8(cursor + 3);
                        w_u8(destination + 2, first);
                        w_u8(destination + 3, second);
                        cursor += 4;
                        --groups;
                        destination = td5_rnc_add_checked(destination, 4, 0x8007C038);
                    } while (groups >= 0);
                    continue;
                }
            }
        }
        else
        {
            bit = td5_rnc_next_bit(&bits, &cursor, 0x8007C36C);
            if (bit == 0)
                goto copy_match;
            length = 3;
            bit = td5_rnc_next_bit(&bits, &cursor, 0x8007C388);
            if (bit != 0)
            {
                byte = r_u8(cursor);
                cursor = td5_rnc_add_checked(cursor, 1, 0x8007C2E0);
                length = byte + 8;
                if (byte == 0)
                {
                    bit = td5_rnc_next_bit(&bits, &cursor, 0x8007C3F4);
                    if (bit != 0)
                        continue;
                    break;
                }
            }
        }
        bit = td5_rnc_next_bit(&bits, &cursor, 0x8007BF64);
        if (bit != 0)
        {
            high = td5_rnc_next_bit(&bits, &cursor, 0x8007BF80);
            bit = td5_rnc_next_bit(&bits, &cursor, 0x8007BF9C);
            if (bit != 0)
            {
                high = high * 2 + td5_rnc_next_bit(&bits, &cursor, 0x8007C3A4);
                high |= 4;
                bit = td5_rnc_next_bit(&bits, &cursor, 0x8007C3C0);
                if (bit == 0)
                    high = high * 2 + td5_rnc_next_bit(&bits, &cursor, 0x8007BFB8);
            }
            else if (high == 0)
            {
                high = 1;
                high = high * 2 + td5_rnc_next_bit(&bits, &cursor, 0x8007BFB8);
            }
            high = (high >> 8) | ((high << 8) & 0xFF00);
        }
copy_match:
        distance = r_u8(cursor);
        ++cursor;
        distance |= high & 0xFF00;
        back = td5_rnc_add_checked(destination, 0 - distance, 0x8007C174);
        back = td5_rnc_add_checked(back, 0xFFFFFFFF, 0x8007C178);
        bit = length & 1;
        length >>= 1;
        if (bit != 0)
        {
            byte = r_u8(back);
            back = td5_rnc_add_checked(back, 1, 0x8007C190);
            w_u8(destination, byte);
            destination = td5_rnc_add_checked(destination, 1, 0x8007C198);
        }
        pairs = (sint32)length - 1;
        if (distance != 0)
        {
            do
            {
                second = r_u8(back + 1);
                first = r_u8(back);
                w_u8(destination + 1, second);
                w_u8(destination, first);
                back = td5_rnc_add_checked(back, 2, 0x8007C1E4);
                --pairs;
                destination = td5_rnc_add_checked(destination, 2, 0x8007C1F0);
            } while (pairs >= 0);
        }
        else
        {
            byte = r_u8(back);
            do
            {
                w_u8(destination, byte);
                --pairs;
                w_u8(destination + 1, byte);
                destination = td5_rnc_add_checked(destination, 2, 0x8007C1C8);
            } while (pairs >= 0);
        }
        continue;
literal_second:
        byte = r_u8(cursor);
        cursor = td5_rnc_add_checked(cursor, 1, 0x8007C220);
        w_u8(destination, byte);
        destination = td5_rnc_add_checked(destination, 1, 0x8007C228);
    }
    count = r_u16(0x8007C4B6);
    scratch = 0x8007C4B8;
    while (count != 0)
    {
        byte = (uint32)r_s16(scratch);
        scratch = td5_rnc_add_checked(scratch, 2, 0x8007C424);
        w_u8(destination, byte);
        destination = td5_rnc_add_checked(destination, 1, 0x8007C42C);
        --count;
    }
}

static const char *td5_car_string(uint32 address)
{
    uint32 length = 0;

    while (r_u8(address + length) != 0)
        ++length;
    return (const char *)psx_addr(address, length + 1);
}

static void td5_copy_car_metadata(uint32 table, uint32 output, uint32 slot)
{
    uint32 source = r_u32(table + 84);
    uint32 destination = output;
    uint32 index;
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;

    for (index = 0; index < 8; ++index)
    {
        a = r_u32(source);
        b = r_u32(source + 4);
        c = r_u32(source + 8);
        d = r_u32(source + 12);
        w_u32(destination, a);
        w_u32(destination + 4, b);
        w_u32(destination + 8, c);
        w_u32(destination + 12, d);
        source += 16;
        destination += 16;
    }
    a = r_u32(source);
    b = r_u32(source + 4);
    c = r_u32(source + 8);
    w_u32(destination, a);
    w_u32(destination + 4, b);
    w_u32(destination + 8, c);
    source = r_u32(table + 84) + 140;
    destination = 0x8009D488 + slot * 128;
    for (index = 0; index < 8; ++index)
    {
        a = r_u32(source);
        b = r_u32(source + 4);
        c = r_u32(source + 8);
        d = r_u32(source + 12);
        w_u32(destination, a);
        w_u32(destination + 4, b);
        w_u32(destination + 8, c);
        w_u32(destination + 12, d);
        source += 16;
        destination += 16;
    }
}

void td5_80046638(uint32 archive_error_index, uint32 original_stack, uint32 texture_carrier)
{
    uint32 work_owner = original_stack - 0x70 + 0x28;
    uint32 compressed_owner = original_stack - 0x70 + 0x2C;
    uint32 local_texture = original_stack - 0x70 + 0x18;
    uint32 saved[7];
    uint32 index;
    uint32 offset;
    uint32 metadata = 0x800A4560;
    uint32 model_offset = 0;
    uint32 record_offset = 0;
    uint32 texture_number = 0;
    uint32 slot = 0;
    sint32 count;
    uint32 master;
    sint32 primary;
    sint32 choice;
    uint32 choice_address;
    uint32 variant;
    uint32 entry;
    uint32 name;
    uint32 size;
    uint32 compressed;
    uint32 base;
    uint32 table;
    uint32 value;
    uint32 limit;
    uint32 flag;
    uint32 x;
    uint32 y;
    uint32 clut_x;
    uint32 tpage;
    uint32 clut;
    uint32 u;
    uint32 v;
    uint32 selector;
    uint32 tim;
    uint32 source;
    uint32 model;
    uint32 second_model;
    uint32 root;
    uint32 selected_texture;
    uint32 name_length;

    FUNCTION_MARKER(0x80046638, "SLUS_006.10");
    variant = r_u32(0x8008871C);
    if (variant == 0)
    {
        for (index = 0; index < 6; ++index)
        {
            offset = index * 24;
            saved[0] = r_u16(0x8008CF78 + offset);
            saved[1] = r_u16(0x8008CF7A + offset);
            saved[2] = r_u16(0x8008CF7C + offset);
            saved[3] = r_u16(0x8008CF7E + offset);
            saved[4] = r_u8(0x8008CF72 + offset);
            saved[5] = r_u8(0x8008CF74 + offset);
            saved[6] = r_u8(0x8008CF76 + offset);
            w_u16(0x8008CF80 + offset, saved[0]);
            w_u16(0x8008CF82 + offset, saved[1]);
            w_u16(0x8008CF84 + offset, saved[2]);
            w_u16(0x8008CF86 + offset, saved[3]);
            w_u8(0x8008CF73 + offset, saved[4]);
            w_u8(0x8008CF75 + offset, saved[5]);
            w_u8(0x8008CF77 + offset, saved[6]);
        }
    }
    td5_alloc_scene_memory(0x32000, work_owner);
    td5_debug_log(0x80010FB0, r_u32(0x80088498));
    for (;;)
    {
        count = r_s32(0x800875E4);
        if (count >= 7)
            count = 6;
        if ((sint32)slot >= count)
            break;
        master = r_u32(0x80088498);
        primary = slot == master;
        if (!primary && r_u32(0x8008772C) != 0)
            primary = slot == master + 1;
        if (!primary && r_u32(0x800876EC) == 0)
            primary = r_u32(0x80087738) != 0;
        choice_address = 0x8008CF72 + record_offset;
        variant = r_u32(0x8008871C);
        choice = r_s8(choice_address + variant);
        if (!primary && choice == -1)
            goto next_car;
        entry = 0x8007E414 + (uint32)choice * 4;
        name = r_u32(entry);
        size = td5_archive_aligned_size(td5_car_string(name), 1, archive_error_index, entry);
        td5_alloc_scene_memory(size, compressed_owner);
        variant = r_u32(0x8008871C);
        choice = r_s8(choice_address + variant);
        compressed = r_u32(compressed_owner);
        name = r_u32(0x8007E414 + (uint32)choice * 4);
        td5_load_archive_file(td5_car_string(name), compressed, archive_error_index);
        compressed = r_u32(compressed_owner);
        base = r_u32(work_owner);
        td5_800158d4(compressed, base, 0x32000);
        base = r_u32(work_owner);
        value = r_u32(base);
        index = 1;
        if (value != 0)
        {
            entry = base + 4;
            do
            {
                value = r_u32(entry);
                w_u32(entry, value + base);
                limit = r_u32(base);
                ++index;
                entry += 4;
            } while (index <= limit);
        }
        if (primary)
        {
            flag = r_u32(0x80087738);
            table = r_u32(work_owner);
            if (flag != 0 && r_u32(0x800876EC) == 0)
                goto dynamic_texture_logged;
            master = r_u32(0x80088498);
            if (slot == master)
            {
                texture_carrier = td5_find_texture((const char *)psx_addr(0x80087A58, 6));
                tpage = r_u16(texture_carrier + 8);
                clut = r_u16(texture_carrier + 10);
                x = (tpage << 6) & 0x3FF;
                y = ((tpage << 4) & 0x100) + ((tpage >> 2) & 0x200);
                clut_x = (clut & 63) << 4;
                archive_error_index = (clut >> 6) & 255;
                goto car_texture_ready;
            }
            x = (slot % 3) * 64 + 320;
            y = (slot / 3) * 256;
            clut_x = 512;
            archive_error_index = slot + 40;
            value = GetTPage(1, 0, (sint32)x, (sint32)y);
            w_u16(local_texture + 8, value);
            value = GetClut(512, (sint32)archive_error_index);
            w_u16(local_texture + 10, value);
            goto car_texture_ready;
dynamic_texture_logged:
            x = (slot % 3) * 64 + 320;
            y = (slot / 3) * 256;
            clut_x = 512;
            archive_error_index = slot + 40;
            value = GetTPage(1, 0, (sint32)x, (sint32)y);
            w_u16(local_texture + 8, value);
            value = GetClut(512, (sint32)archive_error_index);
            w_u16(local_texture + 10, value);
            td5_debug_log(0x80010FC4, slot, x, y, 512, archive_error_index);
car_texture_ready:
            variant = r_u32(0x8008871C);
            selector = r_u8(0x8008CF74 + record_offset + variant);
            tim = r_u32(table + selector * 4 + 16);
            td5_upload_tim(tim, (sint16)x, (sint16)y, (sint16)clut_x, (sint16)archive_error_index);
            td5_copy_car_metadata(table, metadata, slot);
            source = r_u32(table + 4);
            td5_800262e4(source, slot);
            value = r_u32(table + 8);
            source = r_u32(table + 4);
            size = value - source;
            model = td5_alloc_memory(size);
            root = r_u32(0x80088710);
            w_u32(root + model_offset + 136, model);
            source = r_u32(table + 4);
            memcpy(psx_addr(model, size), psx_addr(source, size), size);
            td5_8002e28c(model);
            flag = r_u32(0x80087738);
            if (flag != 0 && r_u32(0x800876EC) == 0)
                selected_texture = local_texture;
            else
            {
                master = r_u32(0x80088498);
                selected_texture = slot == master + 1 && flag == 0 ? local_texture : texture_carrier;
            }
            td5_80048b94(model, selected_texture, 0);
            master = r_u32(0x80088498);
            if (slot == master)
            {
                texture_carrier = td5_find_texture((const char *)psx_addr(0x80087A60, 8));
                clut = r_u16(texture_carrier + 10);
                tpage = r_u16(texture_carrier + 8);
                u = r_u8(texture_carrier + 14);
                variant = r_u32(0x8008871C);
                v = r_u8(texture_carrier + 15);
                archive_error_index = (clut >> 6) & 255;
                selector = r_u8(0x8008CF76 + record_offset + variant);
                x = ((tpage << 6) & 0x3FF) | (u >> 2);
                y = (((tpage << 4) & 0x100) + ((tpage >> 2) & 0x200)) | v;
                tim = r_u32(table + selector * 4 + 36);
                td5_upload_tim(tim, (sint16)x, (sint16)y, (sint16)((clut & 63) << 4), (sint16)archive_error_index);
                DrawSync(0);
            }
            if (r_u32(0x8008772C) == 0 || slot >= 2)
                goto free_primary;
            value = r_u32(table + 12);
            source = r_u32(table + 8);
            size = value - source;
            second_model = td5_alloc_memory(size);
            root = r_u32(0x80088710);
            w_u32(root + model_offset + 140, second_model);
            source = r_u32(table + 8);
            memcpy(psx_addr(second_model, size), psx_addr(source, size), size);
        }
        else
        {
            table = r_u32(work_owner);
            td5_copy_car_metadata(table, metadata, slot);
            source = r_u32(table + 4);
            td5_800262e4(source, slot);
            value = r_u32(table + 12);
            source = r_u32(table + 8);
            size = value - source;
            model = td5_alloc_memory(size);
            root = r_u32(0x80088710);
            w_u32(root + model_offset + 136, model);
            source = r_u32(table + 8);
            memcpy(psx_addr(model, size), psx_addr(source, size), size);
            td5_8002e28c(model);
            value = r_u32(table + 16);
            source = r_u32(table + 12);
            size = value - source;
            second_model = td5_alloc_memory(size);
            root = r_u32(0x80088710);
            w_u32(root + model_offset + 140, second_model);
            source = r_u32(table + 12);
            memcpy(psx_addr(second_model, size), psx_addr(source, size), size);
            td5_8002e28c(second_model);
        }
        for (index = 0; index < 7; ++index)
        {
            entry = 0x8007E4AC + index * 4;
            name = r_u32(entry);
            name_length = 0;
            while (r_u8(name + name_length) != 0)
                ++name_length;
            strcpy((char *)psx_addr(0x80089B58, name_length + 1), (const char *)psx_addr(name, name_length + 1));
            w_u8(0x80089B58, texture_number + 49);
            texture_carrier = td5_find_texture(td5_car_string(0x80089B58));
            tpage = r_u16(texture_carrier + 8);
            clut = r_u16(texture_carrier + 10);
            u = r_u8(texture_carrier + 14);
            v = r_u8(texture_carrier + 15);
            archive_error_index = (clut >> 6) & 255;
            tim = r_u32(table + 56);
            x = ((tpage << 6) & 0x3FF) | (u >> 2);
            y = (((tpage << 4) & 0x100) + ((tpage >> 2) & 0x200)) | v;
            td5_upload_tim(tim, (sint16)x, (sint16)y, (sint16)((clut & 63) << 4), (sint16)archive_error_index);
            root = r_u32(0x80088710);
            name = r_u32(entry);
            model = r_u32(root + model_offset + (primary ? 140 : 136));
            table += 4;
            td5_800484ac(model, name, 0x80089B58);
            if (!primary)
            {
                root = r_u32(0x80088710);
                name = r_u32(entry);
                model = r_u32(root + model_offset + 140);
                td5_800484ac(model, name, 0x80089B58);
            }
        }
        ++texture_number;
        if (primary)
        {
free_primary:
            compressed = r_u32(compressed_owner);
            td5_free_scene_memory(compressed);
        }
        else
        {
            compressed = r_u32(compressed_owner);
            td5_free_scene_memory(compressed);
            DrawSync(0);
        }
next_car:
        metadata += 140;
        model_offset += 8;
        ++slot;
        record_offset += 24;
    }
    td5_free_scene_memory(r_u32(work_owner));
}

uint32 td5_80048b94(uint32 model, uint32 texture, uint32 u_offset)
{
    uint32 groups_offset;
    uint32 second_offset;
    uint32 first_offset;
    uint32 groups;
    uint32 first;
    uint32 second;
    uint32 flag;
    uint32 type;
    sint32 count;
    sint32 index;
    uint32 vertices;
    uint32 stride;
    uint32 buffer;
    uint32 vertex;
    uint32 packet;
    uint32 value;
    uint32 coordinates[4];

    FUNCTION_MARKER(0x80048B94, "SLUS_006.10");
    groups_offset = r_u32(model + 48);
    second_offset = r_u32(model + 56);
    groups = model + groups_offset;
    first_offset = r_u32(model + 52);
    flag = r_u8(groups + 3);
    second = model + second_offset;
    first = model + first_offset;
    if (flag == 0)
        return first_offset;
    do
    {
        type = r_u8(groups + 2);
        count = r_s16(groups);
        groups += 4;
        if (type <= 1)
        {
            for (index = 0; index < count; ++index)
            {
                first += type == 0 ? 32 : 40;
                second += type == 0 ? 32 : 40;
                groups += 20;
            }
        }
        else if (type == 2 || type == 3)
        {
            vertices = type == 2 ? 3 : 4;
            stride = type == 2 ? 40 : 52;
            for (index = 0; index < count; ++index)
            {
                value = r_u16(texture + 8);
                w_u16(first + 26, value);
                w_u16(second + 26, value);
                value = r_u16(texture + 10);
                w_u16(first + 14, value);
                w_u16(second + 14, value);
                for (buffer = 0; buffer < 2; ++buffer)
                {
                    packet = buffer == 0 ? first : second;
                    for (vertex = 0; vertex < vertices; ++vertex)
                        coordinates[vertex] = r_u8(packet + 12 + vertex * 12);
                    for (vertex = 0; vertex < vertices; ++vertex)
                        w_u8(packet + 12 + vertex * 12, coordinates[vertex] + u_offset);
                }
                first += stride;
                second += stride;
                groups += type == 2 ? 44 : 56;
            }
        }
        flag = r_u8(groups + 3);
    } while (flag != 0);
    return 0;
}

uint32 td5_800262e4(uint32 source, uint32 slot)
{
    uint32 vertices;
    uint32 metadata;
    sint32 count;
    sint32 index;
    sint32 x = 0;
    sint32 y = 0;
    sint32 z = 0;
    sint32 value;
    sint32 low;
    sint32 high;
    uint32 squared;
    uint32 radius;

    FUNCTION_MARKER(0x800262E4, "SLUS_006.10");
    vertices = source + r_u32(source + 44);
    count = r_s16(source + 14);
    metadata = 0x800A4560 + slot * 140;
    for (index = 0; index < count; ++index)
    {
        value = r_s16(vertices);
        if (x < value)
            x = value;
        if (x < -value)
            x = -value;
        value = r_s16(vertices + 2);
        if (y < value)
            y = value;
        if (y < -value)
            y = -value;
        value = r_s16(vertices + 4);
        if (z < value)
            z = value;
        if (z < -value)
            z = -value;
        vertices += 8;
    }
    if ((sint32)slot < 6)
    {
        low = r_s16(metadata + 64);
        high = r_s16(metadata + 132);
        value = high - low;
        if (x < value)
            x = value;
    }
    x += 20;
    z -= 20;
    w_u16(metadata + 32, (uint32)-x);
    w_u16(metadata + 48, (uint32)-x);
    w_u16(metadata + 52, (uint32)-z);
    w_u16(metadata + 60, (uint32)-z);
    w_u16(metadata + 34, (uint32)y);
    w_u16(metadata + 36, (uint32)z);
    w_u16(metadata + 40, (uint32)x);
    w_u16(metadata + 42, (uint32)y);
    w_u16(metadata + 44, (uint32)z);
    w_u16(metadata + 50, (uint32)y);
    w_u16(metadata + 56, (uint32)x);
    w_u16(metadata + 58, (uint32)y);
    if ((sint32)slot < 6)
    {
        value = r_s16(metadata + 130) * 181;
        if (value < 0)
            value += 255;
        low = r_s16(metadata + 66);
        y = (value >> 8) - low;
    }
    squared = (uint32)x * (uint32)x;
    squared += (uint32)y * (uint32)y;
    squared += (uint32)z * (uint32)z;
    w_u16(metadata, (uint32)-x);
    w_u16(metadata + 2, (uint32)-y);
    w_u16(metadata + 4, (uint32)z);
    w_u16(metadata + 8, (uint32)x);
    w_u16(metadata + 10, (uint32)-y);
    w_u16(metadata + 12, (uint32)z);
    w_u16(metadata + 16, (uint32)-x);
    w_u16(metadata + 18, (uint32)-y);
    w_u16(metadata + 20, (uint32)-z);
    w_u16(metadata + 24, (uint32)x);
    w_u16(metadata + 26, (uint32)-y);
    w_u16(metadata + 28, (uint32)-z);
    w_u16(metadata + 134, (uint32)y);
    radius = td5_integer_sqrt(squared);
    w_u16(metadata + 128, radius);
    if ((sint32)slot < 6)
        return 0x66660000;
    w_u16(metadata + 114, 0);
    w_u16(metadata + 116, (uint32)z);
    w_u16(metadata + 122, 0);
    w_u16(metadata + 124, (uint32)z);
    w_u16(metadata + 98, 0);
    w_u16(metadata + 100, (uint32)-z);
    w_u16(metadata + 106, 0);
    w_u16(metadata + 108, (uint32)-z);
    x -= x / 5;
    w_u16(metadata + 112, (uint32)-x);
    w_u16(metadata + 120, (uint32)x);
    w_u16(metadata + 96, (uint32)-x);
    w_u16(metadata + 104, (uint32)x);
    return (uint32)x;
}

uint32 td5_8002e28c(uint32 model)
{
    uint32 groups;
    uint32 flag;
    uint32 type;
    sint32 count;
    sint32 index;
    uint32 vertex;
    uint32 vertices;
    uint32 stride;
    uint32 color;

    FUNCTION_MARKER(0x8002E28C, "SLUS_006.10");
    groups = model + r_u32(model + 48);
    flag = r_u8(groups + 3);
    while (flag != 0)
    {
        type = r_u8(groups + 2);
        count = r_s16(groups);
        groups += 4;
        if (type <= 3)
        {
            vertices = type < 2 ? 1 : (type == 2 ? 3 : 4);
            stride = type < 2 ? 20 : (type == 2 ? 44 : 56);
            for (index = 0; index < count; ++index)
            {
                for (vertex = 0; vertex < vertices; ++vertex)
                {
                    color = groups + 8 + vertex * 4;
                    w_u8(color, 128);
                    w_u8(color + 1, 128);
                    w_u8(color + 2, 128);
                }
                groups += stride;
            }
        }
        flag = r_u8(groups + 3);
    }
    return flag;
}

uint32 td5_8007a984(uint32 image, uint32 texture)
{
    uint32 page;
    uint32 u;
    uint32 x;
    uint32 y;
    uint32 signature;
    uint32 v;
    uint32 clut;
    uint32 block;
    uint32 bytes;
    PSX_RECT rectangle;

    FUNCTION_MARKER(0x8007A984, "SLUS_006.10");
    page = r_u16(texture + 8);
    u = r_u8(texture + 14);
    x = ((page & 15) << 6) + (u >> ((page & 0x180) != 0 ? 1 : 2));
    page = r_u16(texture + 8);
    signature = r_u32(image);
    v = r_u8(texture + 15);
    clut = r_u16(texture + 10);
    y = ((page & 16) << 4) + v;
    if (signature != 16)
        return 16;
    block = image + 8;
    rectangle.x = (sint16)((clut & 63) << 4);
    rectangle.y = (sint16)(clut >> 6);
    rectangle.w = (sint16)r_u16(block + 8);
    rectangle.h = (sint16)r_u16(block + 10);
    bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
    LoadImagePSX(&rectangle, (uint32 *)psx_addr(block + 12, bytes));
    block += r_u32(block) & 0xFFFFFFFC;
    if (r_u32(block) == 4)
        return 4;
    rectangle.x = (sint16)x;
    rectangle.y = (sint16)y;
    rectangle.w = (sint16)r_u16(block + 8);
    rectangle.h = (sint16)r_u16(block + 10);
    bytes = rectangle.w > 0 && rectangle.h > 0 ? (uint32)rectangle.w * (uint32)rectangle.h * 2 : 0;
    return (uint32)LoadImagePSX(&rectangle, (uint32 *)psx_addr(block + 12, bytes));
}

uint32 td5_8002ef1c(uint32 object, uint32 selection, uint32 original_stack, uint32 prior_descriptor)
{
    uint32 type;
    sint32 required;
    sint32 scene_pool = 0;
    uint32 descriptor;
    uint32 texture;
    uint32 local_texture;
    uint32 owner = original_stack - 0x50 + 0x20;
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;
    uint32 bytes;
    uint32 source;
    uint32 buffer;
    uint32 value;
    uint32 result;

    FUNCTION_MARKER(0x8002EF1C, "SLUS_006.10");
    type = r_u32(prior_descriptor + 4);
    required = type == 1 ? 0x2100 : 0x4100;
    if (r_s32(0x800A3BC0) < required)
    {
        scene_pool = 1;
        if (r_s32(0x800883B8) < required)
            return 1;
    }
    w_u8(object + 0x33E, selection);
    descriptor = r_u32(0x80088678) + selection * 16;
    type = r_u32(descriptor + 4);
    if (type == 0 || type == 1)
    {
        texture = r_u32(0x800884F0);
        a = r_u32(texture);
        b = r_u32(texture + 4);
        c = r_u32(texture + 8);
        d = r_u32(texture + 12);
        local_texture = original_stack - 0x50 + (type == 0 ? 0x10 : 0x28);
        w_u32(local_texture, a);
        w_u32(local_texture + 4, b);
        w_u32(local_texture + 8, c);
        w_u32(local_texture + 12, d);
        bytes = type == 0 ? 0x4100 : 0x2100;
        if (scene_pool)
            td5_alloc_scene_memory(bytes, owner);
        else
        {
            buffer = td5_alloc_memory(bytes);
            w_u32(owner, buffer);
        }
        source = r_u32(descriptor);
        buffer = r_u32(owner);
        td5_800158d4(source, buffer, (sint32)bytes);
        buffer = r_u32(owner);
        td5_8007a984(buffer, local_texture);
        if (type == 1)
        {
            value = r_u8(local_texture + 15);
            buffer = r_u32(owner);
            w_u8(local_texture + 15, value + 64);
            td5_8007a984(buffer, local_texture);
        }
        buffer = r_u32(owner);
        if (scene_pool)
            td5_free_scene_memory(buffer);
        else
            td5_free_memory(buffer);
        type = r_u32(descriptor + 4);
        w_u8(object + 0x33D, type);
    }
    else
        w_u8(object + 0x33D, type);
    value = r_u32(descriptor + 8);
    w_u8(object + 0x33C, value);
    value = r_u32(descriptor + 12);
    w_u16(object + 0x336, 0);
    result = 127 - value;
    w_u8(object + 0x340, result);
    return result;
}

uint32 td5_8002f158(uint32 original_stack, uint32 prior_descriptor)
{
    uint32 texture;
    uint32 level;
    uint32 state;
    uint32 entries;
    uint32 selection;
    uint32 value;

    FUNCTION_MARKER(0x8002F158, "SLUS_006.10");
    texture = td5_find_texture((const char *)psx_addr(0x8008783C, 5));
    level = r_u32(0x80088770);
    state = r_u32(0x80088488);
    entries = r_u32(level + 32);
    selection = r_u32(state + 28);
    w_u32(0x800884F0, texture);
    w_u32(0x80088678, entries);
    td5_8002ef1c(0x800984A0, selection, original_stack - 0x18, prior_descriptor);
    value = r_u8(0x800987E0);
    w_u16(0x800987D6, value);
    return value;
}

uint32 td5_800484ac(uint32 model, uint32 old_name, uint32 new_name)
{
    uint32 old_texture;
    uint32 new_texture;
    uint32 old_u;
    uint32 old_v;
    uint32 width;
    uint32 height;
    uint32 groups;
    uint32 first;
    uint32 second;
    uint32 groups_offset;
    uint32 first_offset;
    uint32 second_offset;
    uint32 page;
    uint32 flag;
    uint32 type;
    sint32 count;
    sint32 index;
    uint32 vertices;
    uint32 uv_stride;
    uint32 primitive_stride;
    uint32 record_stride;
    uint32 probe;
    uint32 probe_u;
    uint32 probe_v;
    sint32 left;
    sint32 right;
    sint32 top;
    sint32 bottom;
    sint32 x;
    sint32 y;
    uint32 vertex;
    uint32 offset;
    uint32 original_u;
    uint32 original_v;
    uint32 current;
    uint32 replacement;
    uint32 value;
    uint32 last_v = 0;

    FUNCTION_MARKER(0x800484AC, "SLUS_006.10");
    old_texture = td5_find_texture(td5_car_string(old_name));
    new_texture = td5_find_texture(td5_car_string(new_name));
    old_u = r_u8(old_texture + 14);
    width = r_u8(old_texture + 12);
    groups_offset = r_u32(model + 48);
    first_offset = r_u32(model + 52);
    second_offset = r_u32(model + 56);
    groups = model + groups_offset;
    page = r_u16(old_texture + 8);
    first = model + first_offset;
    second = model + second_offset;
    left = (sint32)((old_u >> 2) + ((page << 6) & 0x3FF));
    old_v = r_u8(old_texture + 15);
    height = r_u8(old_texture + 13);
    flag = r_u8(groups + 3);
    top = (sint32)(old_v + ((page << 4) & 0x100) + ((page >> 2) & 0x200));
    right = left + (sint32)(width >> 2);
    if (flag == 0)
        return (uint32)right;
    bottom = (sint16)((uint32)top + height);
    do
    {
        type = r_u8(groups + 2);
        count = r_s16(groups);
        groups += 4;
        if (type <= 3)
        {
            vertices = (type & 1) == 0 ? 3 : 4;
            uv_stride = type < 2 ? 8 : 12;
            primitive_stride = type == 0 ? 32 : (type == 3 ? 52 : 40);
            record_stride = type < 2 ? 20 : (type == 2 ? 44 : 56);
            probe = type == 0 ? 20 : (type == 2 ? 24 : 12);
            for (index = 0; index < count; ++index)
            {
                probe_u = r_u8(first + probe);
                page = r_u16(first + (type < 2 ? 22 : 26));
                probe_v = r_u8(first + probe + 1);
                x = (sint32)((probe_u >> 2) | ((page << 6) & 0x3FF));
                y = (sint32)(((page << 4) & 0x100) + ((page >> 2) & 0x200) + probe_v);
                if (x >= left && x < right && y >= top && y < bottom)
                {
                    current = (type & 1) != 0 ? probe_u : r_u8(first + 12);
                    original_u = r_u8(old_texture + 14);
                    replacement = r_u8(new_texture + 14);
                    original_v = r_u8(old_texture + 15);
                    value = replacement + current - original_u;
                    w_u8(first + 12, value);
                    w_u8(second + 12, value);
                    if (type == 3)
                        w_u32(0x80088580, value & 255);
                    for (vertex = 0; vertex < vertices; ++vertex)
                    {
                        offset = 12 + vertex * uv_stride;
                        if (vertex != 0)
                        {
                            current = r_u8(first + offset);
                            replacement = r_u8(new_texture + 14);
                            value = replacement + current - original_u;
                            w_u8(first + offset, value);
                            w_u8(second + offset, value);
                            if (type == 3)
                                w_u32(0x80088580, value & 255);
                        }
                        current = r_u8(first + offset + 1);
                        replacement = r_u8(new_texture + 15);
                        value = replacement + current - original_v;
                        w_u8(first + offset + 1, value);
                        w_u8(second + offset + 1, value);
                        if (type == 3)
                        {
                            last_v = value & 255;
                            if (vertex != 3)
                                w_u32(0x80088588, last_v);
                        }
                    }
                    value = r_u16(new_texture + 8);
                    w_u16(first + (type < 2 ? 22 : 26), value);
                    w_u16(second + (type < 2 ? 22 : 26), value);
                    value = r_u16(new_texture + 10);
                    if (type == 3)
                        w_u32(0x80088588, last_v);
                    w_u16(first + 14, value);
                    w_u16(second + 14, value);
                }
                first += primitive_stride;
                second += primitive_stride;
                groups += record_stride;
            }
        }
        flag = r_u8(groups + 3);
    } while (flag != 0);
    return flag;
}

uint32 td5_80025960(uint32 original_stack)
{
    uint32 variant;
    uint32 slot;
    uint32 offset;
    uint32 car;
    uint32 metadata;
    uint32 source;
    uint32 a;
    uint32 b;
    uint32 index;
    uint32 gears;
    uint32 field;
    uint32 product;
    uint32 factor;
    uint32 modifier_address;
    sint32 value;
    sint32 second;
    sint32 third;
    sint32 modifier;
    sint32 scaled;
    sint32 quotient;
    sint32 multiplayer;
    uint32 height_address = original_stack - 0x58 - 0x30 - 0x98 + 0x20;

    FUNCTION_MARKER(0x80025960, "SLUS_006.10");
    variant = r_u32(0x8008871C) << 3;
    for (slot = 0; slot < 2; ++slot)
    {
        offset = variant + slot * 24;
        value = r_s16(0x8008CF7C + offset);
        w_u32(0x800877C0 + slot * 4, (uint32)(value / 16));
        value = r_s16(0x8008CF78 + offset);
        w_u32(0x800877C8 + slot * 4, (uint32)(value / 16));
        value = r_s16(0x8008CF7A + offset);
        w_u32(0x800877D0 + slot * 4, (uint32)(value / 16));
        value = r_s16(0x8008CF7E + offset);
        w_u32(0x800877D8 + slot * 4, (uint32)(value / 16));
    }
    for (slot = 0; slot < 14; ++slot)
    {
        car = 0x800984A0 + slot * 864;
        memset(psx_addr(car, 864), 0, 864);
        w_u8(car + 0x34D, slot);
        td5_80048318(car);
        a = r_u8(0x8008CF71 + slot * 24);
        w_u8(car + 0x350, a ^ 1);
        if (slot == 1 && r_u32(0x8008772C) == 0)
            w_u8(car + 0x350, 1);
        if (slot >= 2)
            w_u8(car + 0x350, 1);
        if (r_u32(0x80087650) != 0)
            w_u8(car + 0x350, 1);
        w_u32(car + 0x1BC, 944640);
        w_u8(car + 0x343, 2);
        w_u32(car + 0x180, 0x800A4560 + slot * 140);
        metadata = 0x8009D488 + slot * 128;
        w_u32(car + 0x184, metadata);
        value = r_s16(metadata + 120);
        w_u8(car + 0x358, 255);
        w_u32(car + 0x2FC, (uint32)value);
        for (index = 0; index < 4; ++index)
        {
            w_u8(car + 0x349 + index, 255);
            source = r_u32(car + 0x180) + index * 8;
            a = r_u32(source + 64);
            b = r_u32(source + 68);
            w_u32(car + 0x1D8 + index * 8, a);
            w_u32(car + 0x1DC + index * 8, b);
        }
        if (slot < 6)
        {
            w_u8(0x800883F0 + slot, slot);
            td5_80024220(car, height_address);
            metadata = r_u32(car + 0x184) + 4;
            index = 2;
            for (;;)
            {
                value = r_s16(metadata + 62);
                if (value >= 9999)
                    break;
                ++index;
                metadata += 2;
                if (index >= 8)
                    break;
            }
            w_u8(car + 0x344, index + 1);
        }
        else
        {
            metadata = r_u32(car + 0x180);
            w_u16(metadata + 136, 32);
            td5_80027d58(car);
        }
    }
    if (r_u32(0x800877AC) != 0)
    {
        w_u32(0x800877A4, 2048);
        for (slot = 0; slot < 6; ++slot)
        {
            metadata = r_u32(0x80098624 + slot * 864);
            value = r_s16(metadata + 104) * 650;
            second = r_s16(metadata + 44);
            third = r_s16(metadata + 120);
            w_u16(metadata + 104, (uint32)(value / 256));
            value = second * 380;
            w_u16(metadata + 120, (uint32)third << 2);
            second = r_s16(metadata + 110);
            w_u16(metadata + 44, (uint32)(value / 256));
            value = second * 450;
            second = r_s16(metadata + 112);
            w_u16(metadata + 110, (uint32)(value / 256));
            value = second * 400;
            w_u16(metadata + 112, (uint32)(value / 256));
        }
    }
    else if (r_u32(0x800877A8) != 0)
        w_u32(0x800877A4, 1500);
    else
    {
        w_u32(0x800877A4, 1900);
        for (slot = 0; slot < 6; ++slot)
        {
            metadata = r_u32(0x80098624 + slot * 864);
            value = r_s16(metadata + 104) * 360;
            second = r_s16(metadata + 44);
            third = r_s16(metadata + 120);
            w_u16(metadata + 104, (uint32)(value / 256));
            value = second * 300;
            w_u16(metadata + 120, (uint32)third << 1);
            w_u16(metadata + 44, (uint32)(value / 256));
        }
    }
    multiplayer = r_s32(0x8008772C);
    if (multiplayer < 0)
        return (uint32)multiplayer;
    slot = 0;
    for (;;)
    {
        car = 0x800984A0 + slot * 864;
        metadata = r_u32(car + 0x184);
        modifier = r_s32(0x800877D8 + slot * 4);
        value = r_s16(metadata + 110);
        product = (uint32)value * (uint32)modifier;
        scaled = (sint32)product / 256;
        a = r_u16(metadata + 112);
        w_u16(metadata + 110, (uint32)value + (uint32)scaled);
        w_u16(metadata + 112, a - (uint32)scaled);
        gears = r_u8(car + 0x344);
        field = metadata + 4;
        for (index = 2; index < gears; ++index)
        {
            modifier = r_s32(0x800877C8 + slot * 4);
            value = r_s16(field + 46);
            product = (uint32)value * (uint32)modifier;
            scaled = (sint32)product / (modifier > 0 ? 512 : 768);
            factor = gears - (index + 1);
            product = (uint32)scaled * factor;
            quotient = td5_divide_checked((sint32)product, (sint32)(gears - 3));
            w_u16(field + 46, (uint32)value + (uint32)quotient);
            field += 2;
        }
        modifier_address = 0x800877D0 + slot * 4;
        modifier = r_s32(modifier_address);
        value = r_s16(metadata + 116);
        product = (uint32)value * ((uint32)modifier + 2048);
        scaled = (sint32)product / 2048;
        field = metadata + gears * 2;
        w_u16(metadata + 116, (uint32)scaled);
        value = r_s16(field + 44);
        modifier = r_s32(modifier_address);
        product = (uint32)value << 11;
        quotient = td5_divide_checked((sint32)product, (sint32)((uint32)modifier + 2048));
        w_u16(field + 44, (uint32)quotient);
        value = r_s16(field + 42);
        modifier = r_s32(modifier_address);
        product = (uint32)value << 12;
        quotient = td5_divide_checked((sint32)product, (sint32)((uint32)modifier + 4096));
        w_u16(field + 42, (uint32)quotient);
        modifier = r_s32(modifier_address);
        value = r_s16(metadata + 106);
        product = (uint32)value * (512 - (uint32)modifier);
        scaled = (sint32)product / 512;
        w_u16(metadata + 106, (uint32)scaled);
        modifier_address = 0x800877C0 + slot * 4;
        modifier = r_s32(modifier_address);
        value = r_s16(metadata + 44);
        product = (uint32)value * (uint32)modifier;
        if (modifier < 0)
        {
            scaled = (sint32)product / 768;
            w_u16(metadata + 44, (uint32)value + (uint32)scaled);
        }
        else
        {
            scaled = (sint32)product / 512;
            w_u16(metadata + 44, (uint32)value + (uint32)scaled);
            a = r_u32(car + 0x2FC);
            b = r_u32(modifier_address);
            product = a * b;
            scaled = (sint32)product / 512;
            w_u32(car + 0x2FC, a + (uint32)scaled);
        }
        multiplayer = r_s32(0x8008772C);
        ++slot;
        if (multiplayer < (sint32)slot)
            return 1;
    }
}

uint32 td5_80048318(uint32 car)
{
    uint32 level;
    uint32 slot;
    uint32 table;
    uint32 cursor;
    sint32 mode;
    uint32 multiplier;
    uint32 value;
    sint32 count;
    sint32 index;

    FUNCTION_MARKER(0x80048318, "SLUS_006.10");
    level = r_u32(0x80088770);
    slot = r_u8(car + 0x34D);
    table = r_u32(level + 44);
    if (slot == 0)
    {
        mode = r_s32(0x80088680);
        if (mode == 0 || mode == 1)
        {
            multiplier = mode == 0 ? 12 : 11;
            value = r_u16(table + 2);
            count = (sint32)r_u8(table) - 1;
            w_u16(table + 2, value * multiplier / 10);
            if (count > 0)
            {
                index = 0;
                cursor = table;
                do
                {
                    value = r_u16(cursor + 6);
                    w_u16(cursor + 6, value * multiplier / 10);
                    count = (sint32)r_u8(table) - 1;
                    ++index;
                    cursor += 4;
                } while (index < count);
            }
        }
    }
    value = r_u16(table + 2);
    w_u8(car + 0x356, 0);
    w_u32(car + 0x300, 0);
    w_u16(car + 0x324, 0);
    w_u16(car + 0x31C, value);
    w_u32(0x800884D0, 0);
    w_u32(0x800884D4, 0);
    return value;
}

uint32 td5_80027d58(uint32 car)
{
    uint32 matrix = car + 0x110;
    uint32 normal = car + 0x258;
    uint32 source;
    uint32 destination;
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;
    uint32 metadata;
    sint32 delta;
    uint32 index;
    sint32 angle;
    sint32 cosine;
    sint32 sine;
    sint32 nx;
    sint32 nz;
    sint16 rotated_x;
    sint16 rotated_z;
    uint16 rotated_y;
    uint32 product;
    sint32 adjustment;
    sint32 result;
    uint32 radius;
    uint32 value;

    FUNCTION_MARKER(0x80027D58, "SLUS_006.10");
    a = r_u32(car + 0x1BC);
    b = r_u32(car + 0x1C4);
    c = r_u32(car + 0x1C8);
    d = r_u32(car + 0x1CC);
    w_u16(car + 0x1D2, (uint32)((sint32)a >> 8));
    w_u32(car + 0x124, (uint32)((sint32)b >> 8));
    w_u32(car + 0x128, (uint32)((sint32)c >> 8));
    w_u32(car + 0x12C, (uint32)((sint32)d >> 8));
    td5_80079548(matrix, car + 0x1D0);
    td5_8001b310(car + 0x70, car + 0x1C4);
    td5_8001d314(car + 0x70, car + 0x1C4, normal, (uint32 *)psx_addr(car + 0x1C8, 4));
    metadata = r_u32(car + 0x180);
    delta = r_s16(metadata + 134);
    value = r_u32(car + 0x1C8);
    source = r_u32(car + 0x180);
    w_u32(car + 0x1C8, value + ((uint32)delta << 8));
    SetRotMatrix((MATRIX *)psx_addr(matrix, 20));
    SetTransMatrix((MATRIX *)psx_addr(matrix, 32));
    destination = car + 0x80;
    for (index = 0; index < 4; ++index)
    {
        value = r_u32(source);
        xport_gte_write_data(0, value);
        value = r_u32(source + 4);
        xport_gte_write_data(1, value);
        xport_gte_execute(0x480012);
        w_u32(destination, xport_gte_read_data(25));
        w_u32(destination + 4, xport_gte_read_data(26));
        w_u32(destination + 8, xport_gte_read_data(27));
        a = r_u32(destination);
        b = r_u32(destination + 4);
        c = r_u32(destination + 8);
        source += 8;
        w_u32(destination, a << 8);
        w_u32(destination + 4, b << 8);
        w_u32(destination + 8, c << 8);
        destination += 12;
    }
    angle = r_s16(car + 0x1D2);
    cosine = rcos(angle);
    sine = rsin(angle);
    nx = r_s16(normal);
    nz = r_s16(normal + 4);
    product = (uint32)nx * (uint32)cosine - (uint32)nz * (uint32)sine;
    rotated_x = (sint16)((sint32)product / 4096);
    rotated_y = r_u16(normal + 2);
    nx = r_s16(normal);
    nz = r_s16(normal + 4);
    product = (uint32)nx * (uint32)sine + (uint32)nz * (uint32)cosine;
    rotated_z = (sint16)((sint32)product / 4096);
    result = td5_8007b0c0(-(sint32)rotated_z, -(sint32)(sint16)rotated_y);
    adjustment = r_s32(car + 0x2B8) / 256;
    value = (uint32)result - (uint32)adjustment;
    w_u16(car + 0x1D0, value);
    product = (uint32)(sint32)(sint16)rotated_y * (uint32)(sint32)(sint16)rotated_y;
    product += (uint32)(sint32)rotated_z * (uint32)(sint32)rotated_z;
    radius = (uint32)SquareRoot0((sint32)product);
    result = td5_8007b0c0(rotated_x, (sint32)radius);
    adjustment = r_s32(car + 0x2B4) / 256;
    value = (uint32)result + (uint32)adjustment;
    w_u16(car + 0x1D4, value);
    return value;
}

uint32 td5_80032090(void)
{
    uint32 texture;
    uint32 initial_u;
    uint32 v;
    uint32 slot;
    uint32 buffer;
    uint32 half;
    uint32 u;
    uint32 packet;
    uint32 page;
    uint32 clut;
    uint32 code;
    uint32 allocation;

    FUNCTION_MARKER(0x80032090, "SLUS_006.10");
    allocation = td5_alloc_memory(1120);
    w_u32(0x80088208, allocation);
    allocation = td5_alloc_memory(1120);
    w_u32(0x8008820C, allocation);
    texture = td5_find_texture((const char *)psx_addr(0x80087844, 7));
    initial_u = r_u8(texture + 14);
    v = r_u8(texture + 15);
    for (slot = 0; slot < 14; ++slot)
    {
        for (buffer = 0; buffer < 2; ++buffer)
        {
            u = initial_u;
            for (half = 0; half < 2; ++half)
            {
                packet = r_u32(0x80088208 + buffer * 4) + (slot * 2 + half) * 40;
                w_u8(packet + 3, 9);
                w_u8(packet + 7, 0x2C);
                page = r_u16(texture + 8);
                w_u16(packet + 22, page | 0x40);
                clut = r_u16(texture + 10);
                code = r_u8(packet + 7);
                w_u8(packet + 12, u);
                w_u8(packet + 20, u);
                w_u8(packet + 4, 128);
                w_u8(packet + 5, 128);
                w_u8(packet + 6, 128);
                w_u8(packet + 13, v);
                w_u8(packet + 21, v + 31);
                w_u8(packet + 28, u + 31);
                w_u8(packet + 29, v);
                w_u8(packet + 36, u + 31);
                w_u8(packet + 37, v + 31);
                w_u8(packet + 7, (code & 0xFE) | 2);
                w_u16(packet + 14, clut);
                u += 32;
            }
        }
    }
    return 0;
}

uint32 td5_8002f208(void)
{
    uint32 texture;
    uint32 allocation;
    uint32 index;
    uint32 buffer;
    uint32 packet;
    uint32 page;
    uint32 clut;
    uint32 code;

    FUNCTION_MARKER(0x8002F208, "SLUS_006.10");
    texture = td5_find_texture((const char *)psx_addr(0x8008783C, 5));
    w_u32(0x800884F0, texture);
    allocation = td5_alloc_memory(6400);
    w_u32(0x800881F4, allocation);
    allocation = td5_alloc_memory(6400);
    w_u32(0x800881F8, allocation);
    for (index = 0; index < 200; ++index)
    {
        for (buffer = 0; buffer < 2; ++buffer)
        {
            packet = r_u32(0x800881F4 + buffer * 4) + index * 32;
            w_u8(packet + 3, 7);
            w_u8(packet + 7, 0x24);
            page = r_u16(texture + 8);
            code = r_u8(packet + 7);
            w_u16(packet + 22, page);
            clut = r_u16(texture + 10);
            w_u8(packet + 7, (code | 2) & 0xFE);
            w_u8(packet + 4, 128);
            w_u8(packet + 5, 128);
            w_u8(packet + 6, 128);
            w_u16(packet + 14, clut);
        }
    }
    allocation = td5_alloc_memory(8000);
    w_u32(0x800881FC, allocation);
    allocation = td5_alloc_memory(8000);
    w_u32(0x80088200, allocation);
    for (index = 0; index < 200; ++index)
    {
        for (buffer = 0; buffer < 2; ++buffer)
        {
            packet = r_u32(0x800881FC + buffer * 4) + index * 40;
            w_u8(packet + 3, 9);
            w_u8(packet + 7, 0x2C);
            page = r_u16(texture + 8);
            code = r_u8(packet + 7);
            w_u16(packet + 22, page);
            clut = r_u16(texture + 10);
            w_u8(packet + 7, (code | 2) & 0xFE);
            w_u8(packet + 4, 128);
            w_u8(packet + 5, 128);
            w_u8(packet + 6, 128);
            w_u16(packet + 14, clut);
        }
    }
    return 0;
}

uint32 td5_8002eb88(uint32 original_stack, uint32 prior_descriptor)
{
    sint32 count;
    uint32 slot = 0;
    uint32 root;
    uint32 car;
    uint32 source;
    uint32 model;
    uint32 second;
    uint32 cursor;
    uint32 offset;
    uint32 level;
    uint32 flag;

    FUNCTION_MARKER(0x8002EB88, "SLUS_006.10");
    count = r_s32(0x800875E4);
    while (count >= 7 ? slot < 6 : (sint32)slot < count)
    {
        root = r_u32(0x80088710) + slot * 8;
        model = r_u32(root + 136);
        car = 0x800984A0 + slot * 864;
        w_u32(car + 0x178, model);
        second = r_u32(root + 140);
        count = r_s32(0x800875E4);
        w_u32(car + 0x17C, second);
        ++slot;
    }
    slot = 6;
    count = r_s32(0x800875E4);
    if ((sint32)slot < count)
    {
        cursor = 0x800998E0;
        offset = 0x1440;
        do
        {
            root = r_u32(0x80088710) + slot * 8;
            model = r_u32(root + 136);
            w_u32(cursor + 0x178, model);
            second = r_u32(root + 140);
            w_u32(cursor + 0x17C, second);
            source = r_u32(0x80098618 + offset);
            td5_800262e4(source, slot);
            count = r_s32(0x800875E4);
            ++slot;
            cursor += 864;
            offset += 864;
        } while ((sint32)slot < count);
        prior_descriptor = cursor;
    }
    td5_80032090();
    level = r_u32(0x80088770);
    level = r_u32(level);
    flag = r_u8(level + 30) & 1;
    if (flag != 0)
        return flag;
    td5_8002f158(original_stack - 0x20, prior_descriptor);
    return td5_8002f208();
}

uint32 td5_8003320c(void)
{
    uint32 standard;
    uint32 race;
    uint32 texture;
    uint32 record;
    uint32 index;
    uint32 buffer;
    uint32 packet;
    uint32 value;

    FUNCTION_MARKER(0x8003320C, "SLUS_006.10");
    standard = td5_find_texture((const char *)psx_addr(0x8008784C, 8));
    race = td5_find_texture((const char *)psx_addr(0x80087854, 8));
    for (index = 0; index < 8; ++index)
    {
        texture = index < 4 ? standard : race;
        record = 0x8008F5A8 + index * 6;
        value = r_u16(texture + 8);
        w_u16(record, value);
        value = r_u16(texture + 10);
        w_u16(record + 2, value);
        value = r_u8(texture + 14);
        w_u8(record + 4, value + ((index & 1) != 0 ? 32 : 0));
        value = r_u8(texture + 15);
        w_u8(record + 5, value + ((index & 2) != 0 ? 32 : 0));
    }
    for (index = 0; index < 48; ++index)
    {
        for (buffer = 0; buffer < 2; ++buffer)
        {
            packet = 0x8009DD00 + index * 40 + buffer * 0x780;
            w_u8(packet + 3, 9);
            w_u8(packet + 7, 0x2C);
            w_u8(packet + 4, 32);
            w_u8(packet + 5, 32);
            w_u8(packet + 6, 32);
        }
    }
    return 0;
}

uint32 td5_80033030(uint32 slot, uint32 data)
{
    uint32 buffer;
    uint32 group;
    uint32 index;
    uint32 packet;
    uint32 vertex;
    uint32 angle;
    sint32 radius;
    sint32 width;
    sint32 trig;
    uint32 product;

    FUNCTION_MARKER(0x80033030, "SLUS_006.10");
    for (buffer = 0; buffer < 2; ++buffer)
    {
        for (group = 0; group < 4; ++group)
        {
            for (index = 0; index < 8; ++index)
            {
                packet = 0x8008D1A8 + slot * 768 + buffer * 0x1200 + group * 192 + index * 24;
                w_u8(packet + 3, 5);
                w_u8(packet + 7, 0x28);
                w_u8(packet + 4, 0);
                w_u8(packet + 5, 0);
                w_u8(packet + 6, 0);
            }
        }
    }
    radius = r_s16(data + 130) * 195 / 256;
    width = r_s16(data + 132) + 4;
    vertex = 0x80091610 + slot * 144;
    for (index = 0; index < 18; index += 2)
    {
        w_u16(vertex, (uint32)-width);
        angle = index << 8;
        trig = rsin((sint32)angle);
        product = (uint32)trig * (uint32)radius;
        w_u16(vertex + 2, (uint32)((sint32)product / 4096));
        trig = rcos((sint32)angle);
        product = (uint32)trig * (uint32)radius;
        w_u16(vertex + 4, (uint32)((sint32)product / 4096));
        w_u16(vertex + 8, (uint32)width);
        trig = rsin((sint32)angle);
        product = (uint32)trig * (uint32)radius;
        w_u16(vertex + 10, (uint32)((sint32)product / 4096));
        trig = rcos((sint32)angle);
        product = (uint32)trig * (uint32)radius;
        w_u16(vertex + 12, (uint32)((sint32)product / 4096));
        vertex += 16;
    }
    return 0;
}

uint32 td5_8007b7b8(const char *filename)
{
    sint32 count;
    uint32 index = 0;
    uint32 entry = 0x80092648;
    uint32 result;

    FUNCTION_MARKER(0x8007B7B8, "SLUS_006.10");
    count = r_s32(0x8008816C);
    result = (uint32)count;
    if (count > 0)
    {
        for (;;)
        {
            if (strcmp(filename, td5_car_string(entry)) == 0)
                return index;
            count = r_s32(0x8008816C);
            ++index;
            if ((sint32)index >= count)
                break;
            entry += 40;
        }
        result = 0;
    }
    td5_debug_log(0x80013C74, filename);
    return result;
}

uint32 td5_8005a860(uint32 slot, const char *filename)
{
    uint32 index;
    uint32 offset;
    uint32 value;

    FUNCTION_MARKER(0x8005A860, "SLUS_006.10");
    index = td5_8007b7b8(filename);
    offset = slot * 24;
    w_u16(0x800A4E28 + offset, 1);
    w_u16(0x800A4E2A + offset, 0);
    value = r_u32(0x8009266C + index * 40);
    w_u32(0x800A4E2C + offset, value);
    w_u32(0x800A4E30 + offset, 0);
    value = r_u32(0x80092668 + index * 40);
    w_u32(0x800A4E34 + offset, value);
    w_u32(0x800A4E38 + offset, 0);
    w_u32(0x800A4E3C + offset, 0x8005A840);
    return offset;
}

void td5_8005a840(void)
{
    FUNCTION_MARKER(0x8005A840, "SLUS_006.10");
}

uint32 td5_80033f18(void)
{
    uint32 level;
    uint32 entry;
    uint32 count;

    FUNCTION_MARKER(0x80033F18, "SLUS_006.10");
    level = r_u32(0x80088770);
    entry = r_u32(level + 28);
    w_u32(0x800886D4, 0);
    while (r_u16(entry) != 0xFFFF)
    {
        count = r_u32(0x800886D4);
        w_u32(0x800886D4, count + 1);
        entry += 6;
    }
    return 0xFFFF;
}

uint32 td5_80034d90(void)
{
    char filename[64];
    uint32 value;
    uint32 level;
    uint32 prefix;
    uint32 root;
    uint32 index;
    uint32 destination;
    uint32 source;
    uint32 record;

    FUNCTION_MARKER(0x80034D90, "SLUS_006.10");
    value = r_u32(0x80087730);
    if (value != 0)
        return value;
    w_u32(0x80088674, 0);
    w_u32(0x80087868, 0);
    w_u32(0x80087860, 0xFFFFFFFF);
    w_u32(0x80087864, 0);
    w_u32(0x8008786C, 0);
    td5_80033f18();
    level = r_u32(0x80087710) + 1;
    prefix = r_u32(0x800885C4);
    sprintf(filename, (const char *)psx_addr(0x80010B08, 14), td5_car_string(prefix), (sint32)level);
    td5_8005a860(0, filename);
    td5_8005a860(1, filename);
    memset(psx_addr(0x8009DBD0, 252), 0, 252);
    root = r_u32(0x80088710);
    value = r_u32(root + 12);
    w_u32(0x800884C0, value);
    for (index = 0; index < 7; ++index)
        w_u32(0x8009DBD0 + index * 36, 0);
    index = 0;
    for (;;)
    {
        record = r_u32(0x800884C0) + index * 18;
        if (r_u8(record) == 0)
            break;
        destination = r_u32(0x800884C0) + index * 18;
        source = r_u32(0x800884C0) + index * 18;
        value = r_u16(source + 8);
        w_u16(destination + 14, value + 19);
        ++index;
    }
    w_u32(0x800885F4, index);
    for (index = 0; index < 7; ++index)
    {
        w_u8(0x8009DBF0 + index * 36, 0);
        w_u8(0x8009DBF1 + index * 36, 0);
    }
    return index;
}

uint32 td5_80036778(void)
{
    uint32 buffer;
    uint32 index;
    uint32 texture;
    uint32 packet;
    uint32 value;
    uint32 other;
    uint32 code;
    uint32 clut;
    uint32 name;
    uint32 left;
    uint32 right;

    FUNCTION_MARKER(0x80036778, "SLUS_006.10");
    w_u32(0x80088264, 0);
    w_u32(0x80088268, 0);
    for (buffer = 0; buffer < 2; ++buffer)
    {
        td5_80052348();
        texture = td5_find_texture((const char *)psx_addr(0x800878CC, 7));
        packet = 0x80088DD8 + buffer * 40;
        w_u32(0x80088230, texture);
        w_u8(packet + 3, 9);
        w_u8(packet + 7, 0x2C);
        w_u8(packet + 12, r_u8(texture + 14));
        w_u8(packet + 13, r_u8(texture + 15));
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 20, other + value - 1);
        w_u8(packet + 21, r_u8(texture + 15));
        w_u8(packet + 28, r_u8(texture + 14));
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 29, other + value - 1);
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 36, other + value - 1);
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 37, other + value - 1);
        value = r_u16(texture + 8);
        code = r_u8(packet + 7);
        w_u16(packet + 22, value);
        clut = r_u16(texture + 10);
        w_u8(packet + 7, code | 3);
        w_u16(packet + 14, clut);
        for (index = 0; index < 2; ++index)
        {
            packet = 0x80088950 + buffer * 20 + index * 40;
            w_u8(packet + 7, 0x20);
            code = r_u8(packet + 7);
            w_u8(packet + 3, 4);
            w_u8(packet + 4, 255);
            w_u8(packet + 5, 255);
            w_u8(packet + 6, 0);
            w_u8(packet + 7, code & 0xFD);
            packet = 0x800889A0 + buffer * 20 + index * 40;
            w_u8(packet + 7, 0x20);
            code = r_u8(packet + 7);
            w_u8(packet + 3, 4);
            w_u8(packet + 4, 1);
            w_u8(packet + 5, 1);
            w_u8(packet + 6, 1);
            w_u8(packet + 7, code & 0xFD);
        }
        for (index = 0; index < 3; ++index)
        {
            packet = 0x800889F0 + buffer * 40 + index * 80;
            texture = td5_find_texture((const char *)psx_addr(0x800878D4, 8));
            w_u32(0x80088228, texture);
            w_u8(packet + 3, 9);
            w_u8(packet + 7, 0x2C);
            w_u8(packet + 12, r_u8(texture + 14));
            w_u8(packet + 13, r_u8(texture + 15));
            w_u8(packet + 20, r_u8(texture + 14) + 18);
            w_u8(packet + 21, r_u8(texture + 15));
            w_u8(packet + 28, r_u8(texture + 14));
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            w_u8(packet + 29, other + value - 1);
            w_u8(packet + 36, r_u8(texture + 14) + 18);
            value = r_u8(texture + 15);
            other = r_u8(texture + 13);
            left = 19 * (2 - index) + 350;
            right = 19 * (2 - index) + 368;
            w_u16(packet + 8, left);
            w_u16(packet + 10, 200);
            w_u16(packet + 16, right);
            w_u16(packet + 18, 200);
            w_u16(packet + 24, left);
            w_u8(packet + 37, other + value - 1);
            value = r_u8(texture + 13);
            w_u16(packet + 32, right);
            w_u16(packet + 26, value + 199);
            w_u16(packet + 34, r_u8(texture + 13) + 199);
            w_u16(packet + 22, r_u16(texture + 8));
            w_u16(packet + 14, r_u16(texture + 10));
            code = r_u8(packet + 7);
            w_u8(packet + 7, code | 3);
        }
        name = r_u32(0x800876C4) != 0 ? 0x800878E0 : 0x800878DC;
        texture = td5_find_texture((const char *)psx_addr(name, 4));
        w_u32(0x800885C0, texture);
        packet = 0x80088AE0 + buffer * 40;
        w_u8(packet + 3, 9);
        w_u8(packet + 7, 0x2C);
        w_u8(packet + 12, r_u8(texture + 14));
        w_u8(packet + 13, r_u8(texture + 15));
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 20, other + value - 1);
        w_u8(packet + 21, r_u8(texture + 15));
        w_u8(packet + 28, r_u8(texture + 14));
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 29, other + value - 1);
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 36, other + value - 1);
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u16(packet + 8, 409);
        w_u16(packet + 10, 212);
        w_u8(packet + 37, other + value - 1);
        value = r_u8(texture + 12);
        w_u16(packet + 18, 212);
        w_u16(packet + 24, 409);
        w_u16(packet + 16, value + 408);
        w_u16(packet + 26, r_u8(texture + 13) + 211);
        w_u16(packet + 32, r_u8(texture + 12) + 408);
        w_u16(packet + 34, r_u8(texture + 13) + 211);
        w_u16(packet + 22, r_u16(texture + 8));
        code = r_u8(packet + 7);
        clut = r_u16(texture + 10);
        w_u8(packet + 7, code | 3);
        w_u16(packet + 14, clut);
        texture = td5_find_texture((const char *)psx_addr(0x800878E4, 6));
        packet = 0x80088B30 + buffer * 40;
        w_u32(0x8008822C, texture);
        w_u8(packet + 3, 9);
        w_u8(packet + 7, 0x2C);
        w_u8(packet + 12, r_u8(texture + 14));
        w_u8(packet + 13, r_u8(texture + 15));
        w_u8(packet + 20, r_u8(texture + 14) + 19);
        w_u8(packet + 21, r_u8(texture + 15));
        w_u8(packet + 28, r_u8(texture + 14));
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 29, other + value - 1);
        w_u8(packet + 36, r_u8(texture + 14) + 19);
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u16(packet + 8, 409);
        w_u16(packet + 10, 190);
        w_u16(packet + 16, 428);
        w_u16(packet + 18, 190);
        w_u16(packet + 24, 409);
        w_u8(packet + 37, other + value - 1);
        value = r_u8(texture + 13);
        w_u16(packet + 32, 428);
        w_u16(packet + 26, value + 189);
        w_u16(packet + 34, r_u8(texture + 13) + 189);
        w_u16(packet + 22, r_u16(texture + 8));
        w_u16(packet + 14, r_u16(texture + 10));
        code = r_u8(packet + 7);
        w_u8(packet + 7, code | 3);
        name = r_u32(0x8008772C) != 0 ? 0x800878F4 : 0x800878EC;
        texture = td5_find_texture((const char *)psx_addr(name, 8));
        w_u32(0x80088254, texture);
        packet = 0x80088810 + buffer * 40;
        w_u8(packet + 3, 9);
        w_u8(packet + 7, 0x2C);
        w_u8(packet + 12, r_u8(texture + 14));
        w_u8(packet + 13, r_u8(texture + 15));
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 20, other + value - 1);
        w_u8(packet + 21, r_u8(texture + 15));
        w_u8(packet + 28, r_u8(texture + 14));
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u8(packet + 29, other + value - 1);
        value = r_u8(texture + 14);
        other = r_u8(texture + 12);
        w_u8(packet + 36, other + value - 1);
        value = r_u8(texture + 15);
        other = r_u8(texture + 13);
        w_u16(packet + 8, 380);
        w_u16(packet + 10, 150);
        w_u8(packet + 37, other + value - 1);
        value = r_u8(texture + 12);
        w_u16(packet + 18, 150);
        w_u16(packet + 24, 380);
        w_u16(packet + 16, value + 379);
        w_u16(packet + 26, r_u8(texture + 13) + 149);
        w_u16(packet + 32, r_u8(texture + 12) + 379);
        w_u16(packet + 34, r_u8(texture + 13) + 149);
        w_u16(packet + 22, r_u16(texture + 8));
        code = r_u8(packet + 7);
        clut = r_u16(texture + 10);
        w_u8(packet + 7, code | 3);
        w_u16(packet + 14, clut);
        texture = td5_find_texture((const char *)psx_addr(0x800878FC, 8));
        w_u32(0x80088224, texture);
        for (index = 0; index < 3; ++index)
        {
            packet = 0x80088860 + buffer * 120 + index * 40;
            w_u8(packet + 3, 9);
            w_u8(packet + 7, 0x2C);
            w_u8(packet + 12, r_u8(r_u32(0x80088224) + 14));
            w_u8(packet + 13, r_u8(r_u32(0x80088224) + 15));
            w_u8(packet + 20, r_u8(r_u32(0x80088224) + 14) + 15);
            w_u8(packet + 21, r_u8(r_u32(0x80088224) + 15));
            w_u8(packet + 28, r_u8(r_u32(0x80088224) + 14));
            w_u8(packet + 29, r_u8(r_u32(0x80088224) + 15) + 23);
            w_u8(packet + 36, r_u8(r_u32(0x80088224) + 14) + 15);
            value = r_u8(r_u32(0x80088224) + 15);
            w_u16(packet + 8, 232 + index * 16);
            w_u16(packet + 10, 24);
            w_u16(packet + 16, 247 + index * 16);
            w_u16(packet + 18, 24);
            w_u16(packet + 24, 232 + index * 16);
            w_u16(packet + 26, 47);
            w_u16(packet + 32, 247 + index * 16);
            w_u8(packet + 37, value + 23);
            w_u16(packet + 34, 47);
            texture = r_u32(0x80088224);
            code = r_u8(packet + 7);
            value = r_u16(texture + 8);
            w_u16(packet + 22, value);
            clut = r_u16(texture + 10);
            w_u8(packet + 7, (code | 1) & 0xFD);
            w_u16(packet + 14, clut);
        }
    }
    return 0;
}
