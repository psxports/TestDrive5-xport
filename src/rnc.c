#include <stdlib.h>

#include "rnc.h"
#include "game.h"

static void critical_leave(void)
{
    FUNCTION_MARKER(0x80068500, "SLUS_006.10");
    ExitCriticalSection();
}

static sint32 critical_enter(void)
{
    FUNCTION_MARKER(0x800684F0, "SLUS_006.10");
    return EnterCriticalSectionPSX();
}

static uint32 add_checked(uint32 left, uint32 right, uint32 pc)
{
    sint64 result = (sint64)(sint32)left + (sint64)(sint32)right;

    if (result < (-2147483647LL - 1) || result > 2147483647LL)
        xport_mips_overflow_exception(pc);
    return result;
}

static uint32 next_bit(const uint8 *src, uint32 input_limit, uint32 *bits, uint32 *cursor, uint32 inc_pc)
{
    uint32 bit;
    uint32 byte;

    *bits += *bits;
    bit = (*bits >> 8) & 1;
    *bits &= 255;
    if (*bits == 0)
    {
        XASSERT(!(*cursor >= input_limit));
        byte = src[*cursor];
        *cursor = add_checked(*cursor, 1, inc_pc);
        *bits = byte * 2 + bit;
        bit = (*bits >> 8) & 1;
    }
    return bit;
}

static void decode_method2(const uint8 *src, uint32 src_capacity, uint8 *dst, uint32 dst_capacity)
{
    uint32 unpacked;
    uint32 packed;
    uint32 byte;
    uint32 cursor;
    uint32 bits;
    uint32 bit;
    uint32 len;
    uint32 high;
    uint32 distance;
    uint32 back;
    uint32 byte0;
    uint32 byte1;
    uint32 output_offset = 0;
    uint32 input_limit;
    sint32 pairs;
    sint32 groups;
    uint32 idx;

    FUNCTION_MARKER(0x8007BDF4, "SLUS_006.10");
    XASSERT(!(src_capacity < 18));
    unpacked = src[4];
    byte = src[5];
    unpacked = (unpacked << 24) | (byte << 16);
    byte = src[6];
    unpacked |= byte << 8;
    byte = src[7];
    unpacked |= byte;
    cursor = add_checked(8, 10, 0x8007BE78);
    XASSERT(!(unpacked > dst_capacity));
    (void)add_checked(output_offset, unpacked, 0x8007BE84);
    packed = src[8];
    byte = src[9];
    packed = (packed << 24) | (byte << 16);
    byte = src[10];
    packed |= byte << 8;
    byte = src[11];
    packed |= byte;
    input_limit = add_checked(cursor, packed, 0x8007BEB8);
    XASSERT(!(input_limit > src_capacity));
    XASSERT(!(cursor >= input_limit));
    bits = src[cursor];
    cursor = add_checked(cursor, 1, 0x8007BED4);
    bits = (bits * 2 + 1) * 2;
    for (;;)
    {
        bits += bits;
        bit = (bits >> 8) & 1;
        if (bit == 0)
        {
            XASSERT(!(cursor >= input_limit));
            byte = src[cursor];
            cursor = add_checked(cursor, 1, 0x8007C244);
            XASSERT(!(output_offset >= dst_capacity));
            dst[output_offset] = byte;
            output_offset = add_checked(output_offset, 1, 0x8007C24C);
            bits += bits;
            bit = (bits >> 8) & 1;
            if (bit == 0)
                goto literal_second;
        }
        bits &= 255;
        if (bits == 0)
        {
            XASSERT(!(cursor >= input_limit));
            byte = src[cursor];
            cursor = add_checked(cursor, 1, 0x8007C200);
            bits = byte * 2 + bit;
            bit = (bits >> 8) & 1;
            if (bit == 0)
                goto literal_second;
        }
        len = 2;
        high = 0;
        bit = next_bit(src, input_limit, &bits, &cursor, 0x8007C350);
        if (bit == 0)
        {
            len = 4 + next_bit(src, input_limit, &bits, &cursor, 0x8007BF10);
            bit = next_bit(src, input_limit, &bits, &cursor, 0x8007BF2C);
            if (bit != 0)
            {
                --len;
                len = len * 2 + next_bit(src, input_limit, &bits, &cursor, 0x8007BF48);
                if (len == 9)
                {
                    for (idx = 0; idx < 4; ++idx)
                        high = high * 2 + next_bit(src, input_limit, &bits, &cursor, 0x8007BEF4);
                    groups = (sint32)high + 2;
                    do
                    {
                        XASSERT(!(cursor >= input_limit));
                        byte0 = src[cursor];
                        XASSERT(!(cursor + 1 >= input_limit));
                        byte1 = src[cursor + 1];
                        XASSERT(!(output_offset >= dst_capacity));
                        dst[output_offset] = byte0;
                        XASSERT(!(output_offset + 1 >= dst_capacity));
                        dst[output_offset + 1] = byte1;
                        XASSERT(!(cursor + 2 >= input_limit));
                        byte0 = src[cursor + 2];
                        XASSERT(!(cursor + 3 >= input_limit));
                        byte1 = src[cursor + 3];
                        XASSERT(!(output_offset + 2 >= dst_capacity));
                        dst[output_offset + 2] = byte0;
                        XASSERT(!(output_offset + 3 >= dst_capacity));
                        dst[output_offset + 3] = byte1;
                        cursor += 4;
                        --groups;
                        output_offset = add_checked(output_offset, 4, 0x8007C038);
                    } while (groups >= 0);
                    continue;
                }
            }
        }
        else
        {
            bit = next_bit(src, input_limit, &bits, &cursor, 0x8007C36C);
            if (bit == 0)
                goto copy_match;
            len = 3;
            bit = next_bit(src, input_limit, &bits, &cursor, 0x8007C388);
            if (bit != 0)
            {
                XASSERT(!(cursor >= input_limit));
                byte = src[cursor];
                cursor = add_checked(cursor, 1, 0x8007C2E0);
                len = byte + 8;
                if (byte == 0)
                {
                    bit = next_bit(src, input_limit, &bits, &cursor, 0x8007C3F4);
                    if (bit != 0)
                        continue;
                    break;
                }
            }
        }
        bit = next_bit(src, input_limit, &bits, &cursor, 0x8007BF64);
        if (bit != 0)
        {
            high = next_bit(src, input_limit, &bits, &cursor, 0x8007BF80);
            bit = next_bit(src, input_limit, &bits, &cursor, 0x8007BF9C);
            if (bit != 0)
            {
                high = high * 2 + next_bit(src, input_limit, &bits, &cursor, 0x8007C3A4);
                high |= 4;
                bit = next_bit(src, input_limit, &bits, &cursor, 0x8007C3C0);
                if (bit == 0)
                    high = high * 2 + next_bit(src, input_limit, &bits, &cursor, 0x8007BFB8);
            }
            else if (high == 0)
            {
                high = 1;
                high = high * 2 + next_bit(src, input_limit, &bits, &cursor, 0x8007BFB8);
            }
            high = (high >> 8) | ((high << 8) & 0xFF00);
        }
    copy_match:
        XASSERT(!(cursor >= input_limit));
        distance = src[cursor];
        ++cursor;
        distance |= high & 0xFF00;
        back = add_checked(output_offset, 0 - distance, 0x8007C174);
        back = add_checked(back, 0xFFFFFFFF, 0x8007C178);
        bit = len & 1;
        len >>= 1;
        if (bit != 0)
        {
            XASSERT(!(back >= output_offset));
            byte = dst[back];
            back = add_checked(back, 1, 0x8007C190);
            XASSERT(!(output_offset >= dst_capacity));
            dst[output_offset] = byte;
            output_offset = add_checked(output_offset, 1, 0x8007C198);
        }
        pairs = (sint32)len - 1;
        if (distance != 0)
        {
            do
            {
                XASSERT(!(back + 1 >= output_offset));
                byte1 = dst[back + 1];
                XASSERT(!(back >= output_offset));
                byte0 = dst[back];
                XASSERT(!(output_offset + 1 >= dst_capacity));
                dst[output_offset + 1] = byte1;
                XASSERT(!(output_offset >= dst_capacity));
                dst[output_offset] = byte0;
                back = add_checked(back, 2, 0x8007C1E4);
                --pairs;
                output_offset = add_checked(output_offset, 2, 0x8007C1F0);
            } while (pairs >= 0);
        }
        else
        {
            XASSERT(!(back >= output_offset));
            byte = dst[back];
            do
            {
                XASSERT(!(output_offset >= dst_capacity));
                dst[output_offset] = byte;
                --pairs;
                XASSERT(!(output_offset + 1 >= dst_capacity));
                dst[output_offset + 1] = byte;
                output_offset = add_checked(output_offset, 2, 0x8007C1C8);
            } while (pairs >= 0);
        }
        continue;
    literal_second:
        XASSERT(!(cursor >= input_limit));
        byte = src[cursor];
        cursor = add_checked(cursor, 1, 0x8007C220);
        XASSERT(!(output_offset >= dst_capacity));
        dst[output_offset] = byte;
        output_offset = add_checked(output_offset, 1, 0x8007C228);
    }
    XASSERT(!(output_offset != unpacked));
}

void rnc_decode(const uint8 *src, uint32 src_capacity, uint8 *dst, sint32 capacity)
{
    uint32 unpacked;
    uint32 byte;

    FUNCTION_MARKER(0x800158D4, "SLUS_006.10");
    XASSERT(!(src_capacity < 1));
    XASSERT(!(src[0] != 'R'));
    XASSERT(!(src_capacity < 2));
    XASSERT(!(src[1] != 'N'));
    XASSERT(!(src_capacity < 3));
    XASSERT(!(src[2] != 'C'));
    XASSERT(!(src_capacity < 8));
    unpacked = src[7];
    byte = src[6];
    unpacked |= byte << 8;
    byte = src[5];
    unpacked |= byte << 16;
    byte = src[4];
    unpacked |= byte << 24;
    XASSERT(!(capacity < (sint32)unpacked));
    XASSERT(!(src[3] != 2 || capacity < 0 || unpacked > (uint32)capacity));
    critical_enter();
    decode_method2(src, src_capacity, dst, (uint32)capacity);
    critical_leave();
}

uint32 rnc_allocation_size(const uint8 *src)
{
    uint32 val, next;

    FUNCTION_MARKER(0x80015A3C, "SLUS_006.10");
    val = src[0];
    XASSERT(!(val != 'R'));
    val = src[1];
    XASSERT(!(val != 'N'));
    val = src[2];
    XASSERT(!(val != 'C'));
    val = src[7];
    next = src[6];
    val |= next << 8;
    next = src[5];
    val |= next << 16;
    next = src[4];
    val |= next << 24;
    XASSERT(!(val > 0x7FFFFBFF));
    return val + 1024;
}
