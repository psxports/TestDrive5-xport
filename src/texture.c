#include <string.h>

#include "texture.h"
#include "game.h"

typedef struct TEXTURE_TABLE
{
    uint8 *entries;
    uint32 capacity;
} TEXTURE_TABLE;

static TEXTURE_TABLE texture_table;

void tex_set_table(uint8 *table, uint32 capacity)
{
    FUNCTION_MARKER(0x8006C8E0, "SLUS_006.10");
    texture_table.entries = table;
    texture_table.capacity = capacity;
}

uint8 *tex_find(const char *name)
{
    uint8 *entry;
    uint32 offset = 0;

    FUNCTION_MARKER(0x8006C8EC, "SLUS_006.10");
    for (;;)
    {
        if (texture_table.entries == NULL || texture_table.capacity < 16 || offset > texture_table.capacity - 16)
            break;
        entry = texture_table.entries + offset;
        if (strncmp((const char *)entry, name, 8) == 0)
            return entry;
        if (entry[0] == 0)
        {
            if (offset < 16)
                break;
            return entry - 16;
        }
        offset += 16;
    }
    XASSERT(0);
    return NULL;
}

sint32 tex_upload_tim(const uint8 *image, uint32 capacity, sint16 image_x, sint16 image_y, sint16 pal_x, sint16 pal_y)
{
    uint32 wire_word_1, wire_word_2, wire_word_3;
    PSX_RECT rect;
    uint32 block;
    uint32 bytes;
    uint32 advance;

    FUNCTION_MARKER(0x80048DB8, "SLUS_006.10");
    XASSERT(!(capacity < 4));
    if ((memcpy(&wire_word_1, image, sizeof(wire_word_1)), wire_word_1) != 16)
        return 16;
    block = 8;
    XASSERT(!(capacity - 4 < 16));
    rect.x = pal_x;
    rect.y = pal_y;
    rect.w = (sint16)xport_load_le16(image + block + 8);
    rect.h = (sint16)xport_load_le16(image + block + 10);
    bytes = rect.w > 0 && rect.h > 0 ? (uint32)rect.w * (uint32)rect.h * 2 : 0;
    XASSERT(!(bytes > capacity - block - 12));
    LoadImagePSX(&rect, (uint32 *)(image + block + 12));
    advance = (memcpy(&wire_word_2, image + block, sizeof(wire_word_2)), wire_word_2) & 0xFFFFFFFC;
    XASSERT(!(advance > capacity - block));
    block += advance;
    XASSERT(!(capacity - block < 4));
    if ((memcpy(&wire_word_3, image + block, sizeof(wire_word_3)), wire_word_3) != 4)
    {
        XASSERT(!(capacity - block < 12));
        rect.x = image_x;
        rect.y = image_y;
        rect.w = (sint16)xport_load_le16(image + block + 8);
        rect.h = (sint16)xport_load_le16(image + block + 10);
        bytes = rect.w > 0 && rect.h > 0 ? (uint32)rect.w * (uint32)rect.h * 2 : 0;
        XASSERT(!(bytes > capacity - block - 12));
        LoadImagePSX(&rect, (uint32 *)(image + block + 12));
    }
    return DrawSync(0);
}
