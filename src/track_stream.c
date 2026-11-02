#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "track_stream.h"
#include "hd_track.h"
#include "game.h"

typedef struct TRACK_LINEAR_CACHE
{
    uint32 radius;
    uint32 pair_count;
    uint32 grp_count;
    uint32 desired_sections[TRACK_LINEAR_WINDOW_CAPACITY - 1];
    TRACK_LINEAR_ENTRY entries[2 * TRACK_LINEAR_PAIR_CAPACITY];
} TRACK_LINEAR_CACHE;

typedef struct TRACK_SECTION
{
    uint8 *buffer;
    uint32 buffer_bytes;
    sint16 section_idx;
    uint8 geometry_status;
    uint8 tex_status;
    uint8 variant_mask;
} TRACK_SECTION;

typedef struct TRACK_STREAM
{
    TRACK_SECTION sections[7];
    TRACK_SECTION *forward;
    TRACK_SECTION *reverse;
    TRACK_SECTION *section;
    uint32 tex_buffer_bytes;

    union
    {
        uint8 bytes[255 * 2048];
        uint32 alignment;
    } tex;

    uint8 *variants;
    uint32 section_count;
    uint32 prev_section;
    uint32 dir;
    uint32 queued;
    uint32 tex_dir;
    uint32 load_ticks;
    uint32 cur_node;
} TRACK_STREAM;

static TRACK_STREAM track_stream;
static uint32 stream_interval_count;
static TRACK_LINEAR_CACHE linear_cache = {.radius = 35};

TRACK_LINEAR_IMAGE track_linear_image;

static uint32 stream_count_intervals(void);
static uint32 stream_select_variant(sint32 node, sint32 lane);

static void upload_stream_image(PSX_RECT *rect, const uint8 *pixels)
{
    LoadImagePSX(rect, (uint32 *)pixels);
}

void track_stream_upload_images(void)
{
    const uint8 *data;
    TRACK_SECTION *entry;
    uint32 idx;
    uint32 val;
    uint32 width;
    uint32 height;
    const uint8 *pixels;
    const uint8 *pixel_cursor;
    uint32 pix_bytes;
    PSX_RECT rect;
    PSX_RECT pal_rect = {0, 0, 16, 1};
    uint32 size;
    uint32 result;

    FUNCTION_MARKER(0x800341FC, "SLUS_006.10");
    if (track_stream.tex_dir == 1)
    {
        entry = track_stream.forward;
        if (entry->tex_status == 2)
        {
            track_stream.tex_dir = 0;
            XASSERT(!(track_stream.tex_buffer_bytes < 0x5AC || track_stream.tex_buffer_bytes > sizeof(track_stream.tex.bytes)));
            data = track_stream.tex.bytes;
            pixel_cursor = track_stream.tex.bytes + 0x5AC;
            entry = track_stream.forward;
            entry->tex_status = 4;
            for (idx = 0; (sint32)idx < data[2]; ++idx)
            {
                val = data[idx + 0x84];
                rect.w = (sint16)((val & 15) * 2);
                val = data[idx + 0x84];
                rect.h = (sint16)((val >> 4) * 8);
                val = data[idx + 4];
                rect.x = (sint16)(val * 4);
                val = data[idx + 0x44];
                rect.y = (sint16)(val * 4);
                pixels = pixel_cursor;
                XASSERT(!((uint32)(uint16)rect.w * (uint16)rect.h * 2 > track_stream.tex_buffer_bytes - (uint32)(pixel_cursor - data)));
                upload_stream_image(&rect, pixels);
                width = (uint32)rect.w;
                height = (uint32)rect.h;
                pix_bytes = width * (height * 2);
                pixels = pixel_cursor;
                size = pix_bytes;
                pixel_cursor = pixels + size;
            }
            for (idx = 0; (sint32)idx < data[3]; ++idx)
            {
                val = data[idx + 0xC4];
                pal_rect.x = (sint16)(val * 4);
                val = data[idx + 0x104];
                pal_rect.y = (sint16)(val);
                pixels = pixel_cursor;
                XASSERT(!(32 > track_stream.tex_buffer_bytes - (uint32)(pixel_cursor - data)));
                upload_stream_image(&pal_rect, pixels);
                pixels = pixel_cursor;
                pixel_cursor = pixels + 32;
            }
        }
    }
    result = track_stream.tex_dir;
    if (result != 0xFFFFFFFF)
        return;
    entry = track_stream.reverse;
    if (entry->tex_status != 2)
        return;
    track_stream.tex_dir = 0;
    XASSERT(!(track_stream.tex_buffer_bytes < 0x5AC || track_stream.tex_buffer_bytes > sizeof(track_stream.tex.bytes)));
    data = track_stream.tex.bytes;
    pixel_cursor = track_stream.tex.bytes + 0x5AC;
    entry = track_stream.reverse;
    entry->tex_status = 4;
    for (idx = 0; (sint32)idx < data[0x144]; ++idx)
    {
        val = data[idx + 0x1C6];
        rect.w = (sint16)((val & 15) * 2);
        val = data[idx + 0x1C6];
        rect.h = (sint16)((val >> 4) * 8);
        val = data[idx + 0x146];
        rect.x = (sint16)(val * 4);
        val = data[idx + 0x186];
        rect.y = (sint16)(val * 4);
        pixels = pixel_cursor;
        XASSERT(!((uint32)(uint16)rect.w * (uint16)rect.h * 2 > track_stream.tex_buffer_bytes - (uint32)(pixel_cursor - data)));
        upload_stream_image(&rect, pixels);
        width = (uint32)rect.w;
        height = (uint32)rect.h;
        pix_bytes = width * (height * 2);
        pixels = pixel_cursor;
        size = pix_bytes;
        pixel_cursor = pixels + size;
    }
    for (idx = 0; (sint32)idx < data[0x145]; ++idx)
    {
        val = data[idx + 0x206];
        pal_rect.x = (sint16)(val * 4);
        val = data[idx + 0x246];
        pal_rect.y = (sint16)(val);
        pixels = pixel_cursor;
        XASSERT(!(32 > track_stream.tex_buffer_bytes - (uint32)(pixel_cursor - data)));
        upload_stream_image(&pal_rect, pixels);
        pixels = pixel_cursor;
        pixel_cursor = pixels + 32;
    }
}

static void stream_forward_ready(void)
{
    TRACK_SECTION *entry;

    FUNCTION_MARKER(0x800346DC, "SLUS_006.10");
    entry = track_stream.forward;
    entry->tex_status = 2;
}

static void stream_reverse_ready(void)
{
    TRACK_SECTION *entry;

    FUNCTION_MARKER(0x80034708, "SLUS_006.10");
    entry = track_stream.reverse;
    entry->tex_status = 2;
}

static void stream_section_ready(void)
{
    TRACK_SECTION *entry;

    FUNCTION_MARKER(0x80034734, "SLUS_006.10");
    entry = track_stream.section;
    entry->geometry_status = 2;
}

static uint32 stream_queue_tex(uint32 rec)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5;

    const uint8 *data;
    TRACK_SECTION *entry;
    uint32 sector;
    uint32 size;
    uint32 result;
    uint8 *buffer;

    FUNCTION_MARKER(0x80034804, "SLUS_006.10");
    data = track_stream.variants + rec * 18;
    (void)(memcpy(&wire_halfword_1, data + 10, sizeof(wire_halfword_1)), wire_halfword_1);
    entry = track_stream.sections + (uint32)((sint32)rec % 7);
    if (entry->buffer != 0)
        scene_pool_free(entry->buffer);
    track_stream.dir = 0;
    if ((sint32)track_stream.prev_section < (sint32)rec)
        track_stream.dir = 1;
    if ((sint32)rec < (sint32)track_stream.prev_section)
        track_stream.dir = 0xFFFFFFFF;
    track_stream.prev_section = rec;
    entry->tex_status = 4;
    sector = (memcpy(&wire_halfword_2, data + 10, sizeof(wire_halfword_2)), wire_halfword_2);
    sector += (memcpy(&wire_halfword_3, data + 4, sizeof(wire_halfword_3)), wire_halfword_3);
    size = data[2];
    if (size != 0 && track_stream.dir == 1)
    {
        track_stream.tex_dir = track_stream.dir;
        entry->tex_status = 1;
        track_stream.forward = entry;
        track_stream.tex_buffer_bytes = size << 11;
        buffer = track_stream.tex.bytes;
        stream_queue_read(1, buffer, size, stream_forward_ready, sector, NULL);
        track_stream.queued = 1;
    }
    sector = (memcpy(&wire_halfword_4, data + 10, sizeof(wire_halfword_4)), wire_halfword_4);
    sector += (memcpy(&wire_halfword_5, data + 6, sizeof(wire_halfword_5)), wire_halfword_5);
    size = data[3];
    result = size;
    if (size != 0)
    {
        result = track_stream.dir;
        if (result == 0xFFFFFFFF)
        {
            track_stream.tex_dir = track_stream.dir;
            entry->tex_status = 1;
            track_stream.reverse = entry;
            track_stream.tex_buffer_bytes = size << 11;
            buffer = track_stream.tex.bytes;
            stream_queue_read(1, buffer, size, stream_reverse_ready, sector, NULL);
            result = 1;
            track_stream.queued = 1;
        }
    }
    return result;
}

static uint32 stream_queue_section(uint32 rec, uint32 node, sint32 lane)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3;

    uint32 variant;
    const uint8 *data;
    TRACK_SECTION *entry;
    uint32 sector;
    uint32 size;
    uint8 *buffer;

    FUNCTION_MARKER(0x80034AE8, "SLUS_006.10");
    variant = stream_select_variant((sint32)node, lane);
    data = track_stream.variants + rec * 18;
    entry = track_stream.sections + (uint32)((sint32)rec % 7);
    if (data[1] == 0)
        variant = 3;
    if (variant == 2)
    {
        sector = (memcpy(&wire_halfword_1, data + 10, sizeof(wire_halfword_1)), wire_halfword_1);
        sector += data[0];
        size = data[1];
        entry->variant_mask = 2;
    }
    else if (variant == 1)
    {
        sector = (memcpy(&wire_halfword_2, data + 10, sizeof(wire_halfword_2)), wire_halfword_2);
        size = data[0];
        entry->variant_mask = 1;
    }
    else
    {
        /* The audited selector returns only 1, 2 or 3 */
        sector = (memcpy(&wire_halfword_3, data + 10, sizeof(wire_halfword_3)), wire_halfword_3);
        size = data[0];
        size += data[1];
        entry->variant_mask = 3;
    }
    scene_pool_alloc(size << 11, &entry->buffer);
    entry->buffer_bytes = size << 11;
    buffer = entry->buffer;
    entry->section_idx = (sint16)rec;
    entry->geometry_status = 1;
    track_stream.section = entry;
    stream_queue_read(0, buffer, size, stream_section_ready, sector, &entry->buffer);
    track_stream.queued = 1;
    return 1;
}

uint32 track_stream_init(void)
{
    uint16 wire_halfword_1, wire_halfword_2;

    char filename[64];
    uint32 val;
    uint32 level;
    const char *prefix;
    uint8 *root;
    uint32 idx;
    uint8 *dst;
    const uint8 *src;
    const uint8 *rec;
    TRACK_SECTION empty = {0};
    uint32 variants_bytes;

    FUNCTION_MARKER(0x80034D90, "SLUS_006.10");
    val = race_session.linear_course;
    if (val != 0)
        return val;
    track_stream.load_ticks = 0;
    track_stream.queued = 0;
    track_stream.prev_section = 0xFFFFFFFF;
    track_stream.dir = 0;
    track_stream.tex_dir = 0;
    stream_count_intervals();
    level = race_session.track + 1;
    prefix = scene_level_image.asset_prefix;
    sprintf(filename, "%sLEVEL%d.CLW", prefix, (sint32)level);
    archive_bind_stream(0, filename);
    archive_bind_stream(1, filename);
    for (idx = 0; idx < 7; ++idx)
        track_stream.sections[idx] = empty;
    root = scene_track_image.bytes;
    XASSERT(!(root == NULL || scene_track_image.size < 16));
    memcpy(&val, root + 12, sizeof(val));
    XASSERT(!(val >= scene_track_image.size));
    track_stream.variants = root + val;
    variants_bytes = scene_track_image.size - val;
    for (idx = 0; idx < 7; ++idx)
        track_stream.sections[idx].buffer = 0;
    idx = 0;
    for (;;)
    {
        XASSERT(!(idx > (variants_bytes - 1) / 18));
        rec = track_stream.variants + idx * 18;
        if (rec[0] == 0)
            break;
        XASSERT(!(variants_bytes - idx * 18 < 18));
        dst = track_stream.variants + idx * 18;
        src = track_stream.variants + idx * 18;
        val = (memcpy(&wire_halfword_1, src + 8, sizeof(wire_halfword_1)), wire_halfword_1);
        (wire_halfword_2 = (uint16)((uint16)(val + 19)), memcpy(dst + 14, &wire_halfword_2, sizeof(wire_halfword_2)));
        ++idx;
    }
    track_stream.section_count = idx;
    for (idx = 0; idx < 7; ++idx)
    {
        track_stream.sections[idx].geometry_status = 0;
        track_stream.sections[idx].tex_status = 0;
    }
    return idx;
}

static uint64 stream_draw_objects(void)
{
    VEC3 model_pos;
    MODEL_RESOURCE render_model;
    uint32 row, count, idx, variant;
    const uint8 *table;
    uint32 section_idx, table_offset, object_offset;
    const uint8 *object_bytes;
    const uint8 *variant_table;
    const uint8 *geometry;
    TRACK_SECTION *rec;
    uint32 x, y, z, val;

    FUNCTION_MARKER(0x80035024, "SLUS_006.10");
    model_update_distance_reference(&car_states[0].transform, &car_states[0].pos);
    x = model_render.view.words[0];
    y = model_render.view.words[1];
    xport_gte_write_control(0, x);
    xport_gte_write_control(1, y);
    x = model_render.view.words[2];
    y = model_render.view.words[3];
    z = model_render.view.words[4];
    xport_gte_write_control(2, x);
    xport_gte_write_control(3, y);
    xport_gte_write_control(4, z);
    for (row = 0; row < 7; ++row)
    {
        rec = track_stream.sections + row;
        table = rec->buffer;
        XASSERT(!(rec->geometry_status == 2 && (table == NULL || rec->buffer_bytes < 2)));
        count = rec->geometry_status == 2 ? table[0] : 0;
        section_idx = (uint32)rec->section_idx;
        variant_table = track_stream.variants;
        variant = 0;
        if (rec->geometry_status == 2)
        {
            XASSERT(!(rec->buffer_bytes < 2));
            geometry = table;
            if ((rec->variant_mask & 1) != 0)
            {
                for (idx = 0; idx < count; ++idx)
                {
                    table_offset = idx * 4 + 0x2A0;
                    XASSERT(!(table_offset > rec->buffer_bytes || rec->buffer_bytes - table_offset < 4));
                    memcpy(&val, geometry + table_offset, sizeof(val));
                    object_offset = val;
                    XASSERT(!(object_offset > rec->buffer_bytes || rec->buffer_bytes - object_offset < 40));
                    object_bytes = geometry + object_offset;
                    if (model_visible(object_bytes) != 0)
                    {
                        model_pos.bits.x = ((const MODEL_HEADER *)object_bytes)->position.bits.x;
                        model_pos.bits.y = ((const MODEL_HEADER *)object_bytes)->position.bits.y;
                        model_pos.bits.z = ((const MODEL_HEADER *)object_bytes)->position.bits.z;
                        model_transform_active_pos(&model_pos);
                        render_model.bytes = (uint8 *)object_bytes;
                        render_model.capacity = rec->buffer_bytes - object_offset;
                        model_draw_lod(&render_model);
                    }
                    else if (hd_capture_active())
                    {
                        render_model.bytes = (uint8 *)object_bytes;
                        render_model.capacity = rec->buffer_bytes - object_offset;
                        hd_draw_static_candidate(&render_model, NULL);
                    }
                }
            }
            if (geometry[1] != 0 && (rec->variant_mask & 2) != 0)
            {
                if (rec->variant_mask == 2)
                    variant = 0;
                else
                {
                    XASSERT(!(section_idx >= track_stream.section_count));
                    variant = (uint32)variant_table[section_idx * 18] << 11;
                }
                val = geometry[1];
                count = geometry[0];
                count += val;
                idx = geometry[0];
                for (; idx < count; ++idx)
                {
                    table_offset = idx * 4 + 0x2A0;
                    XASSERT(!(table_offset > rec->buffer_bytes || rec->buffer_bytes - table_offset < 4));
                    memcpy(&val, geometry + table_offset, sizeof(val));
                    object_offset = val;
                    XASSERT(!(object_offset > rec->buffer_bytes || variant > rec->buffer_bytes - object_offset));
                    object_offset += variant;
                    XASSERT(!(object_offset > rec->buffer_bytes || rec->buffer_bytes - object_offset < 40));
                    object_bytes = geometry + object_offset;
                    if (model_visible(object_bytes) != 0)
                    {
                        model_pos.bits.x = ((const MODEL_HEADER *)object_bytes)->position.bits.x;
                        model_pos.bits.y = ((const MODEL_HEADER *)object_bytes)->position.bits.y;
                        model_pos.bits.z = ((const MODEL_HEADER *)object_bytes)->position.bits.z;
                        model_transform_active_pos(&model_pos);
                        render_model.bytes = (uint8 *)object_bytes;
                        render_model.capacity = rec->buffer_bytes - object_offset;
                        model_draw_lod(&render_model);
                    }
                    else if (hd_capture_active())
                    {
                        render_model.bytes = (uint8 *)object_bytes;
                        render_model.capacity = rec->buffer_bytes - object_offset;
                        hd_draw_static_candidate(&render_model, NULL);
                    }
                }
            }
        }
    }
    return 7;
}

uint32 track_stream_draw(void)
{
    uint16 wire_halfword_1, wire_halfword_2;

    VEC3 model_pos;
    MODEL_RESOURCE render_model;
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    sint32 near_distance;
    sint32 far_distance;
    sint32 r;
    sint32 g;
    sint32 b;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 grp;
    const uint8 *geometry;
    TRACK_SECTION *desc;
    const uint8 *data;
    uint32 count;
    const uint8 *variant;
    uint32 variant_offset;
    uint32 idx;
    uint32 count_part;
    uint32 table_offset, object_offset;
    const uint8 *object_bytes;
    uint32 flags;
    sint32 variant_idx;

    FUNCTION_MARKER(0x80035390, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    near_distance = (sint16)(memcpy(&wire_halfword_1, level_header + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    far_distance = (sint16)(memcpy(&wire_halfword_2, level_header + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    r = level_header[0];
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    g = level_header[1];
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    b = level_header[2];
    model_set_fog(near_distance, far_distance, r, g, b);
    model_set_back_color(32, 32, 32);
    SetFarColor(32, 32, 32);
    SetFogNearFar(15000, 60000, 160);
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    if (hd_track_ready() && hd_capture_active())
    {
        if (!hd_native_capture_required())
        {
            hd_track_draw(0);
            return 7;
        }
        hd_original_track_capture(0);
    }
    if ((level_header[30] & 1) != 0)
    {
        uint64 result = stream_draw_objects();
        hd_original_track_capture(1);
        hd_track_draw(0);
        return (uint32)result;
    }
    x = model_render.view.words[0];
    y = model_render.view.words[1];
    xport_gte_write_control(0, x);
    xport_gte_write_control(1, y);
    x = model_render.view.words[2];
    y = model_render.view.words[3];
    z = model_render.view.words[4];
    xport_gte_write_control(2, x);
    xport_gte_write_control(3, y);
    xport_gte_write_control(4, z);
    for (grp = 0; grp < 7; ++grp)
    {
        desc = track_stream.sections + grp;
        data = desc->buffer;
        XASSERT(!(desc->geometry_status == 2 && (data == NULL || desc->buffer_bytes < 2)));
        count = desc->geometry_status == 2 ? data[0] : 0;
        variant_idx = desc->section_idx;
        variant = track_stream.variants;
        if (desc->geometry_status != 2)
            continue;
        XASSERT(!(desc->buffer_bytes < 2));
        geometry = data;
        if ((desc->variant_mask & 1) != 0)
        {
            for (idx = 0; idx < count; ++idx)
            {
                table_offset = idx * 4 + 672;
                XASSERT(!(table_offset > desc->buffer_bytes || desc->buffer_bytes - table_offset < 4));
                memcpy(&object_offset, geometry + table_offset, sizeof(object_offset));
                XASSERT(!(object_offset > desc->buffer_bytes || desc->buffer_bytes - object_offset < 40));
                object_bytes = geometry + object_offset;
                if (model_visible(object_bytes) != 0)
                {
                    model_pos.bits.x = ((const MODEL_HEADER *)object_bytes)->position.bits.x;
                    model_pos.bits.y = ((const MODEL_HEADER *)object_bytes)->position.bits.y;
                    model_pos.bits.z = ((const MODEL_HEADER *)object_bytes)->position.bits.z;
                    model_transform_active_pos(&model_pos);
                    render_model.bytes = (uint8 *)object_bytes;
                    render_model.capacity = desc->buffer_bytes - object_offset;
                    model_draw(&render_model);
                }
                else if (hd_capture_active())
                {
                    render_model.bytes = (uint8 *)object_bytes;
                    render_model.capacity = desc->buffer_bytes - object_offset;
                    hd_draw_static_candidate(&render_model, NULL);
                }
            }
        }
        if (geometry[1] == 0)
            continue;
        if ((desc->variant_mask & 2) == 0)
            continue;
        flags = desc->variant_mask;
        if (flags == 2)
            variant_offset = 0;
        else
        {
            XASSERT(!(variant_idx < 0 || (uint32)variant_idx >= track_stream.section_count));
            variant_offset = (uint32)variant[(uint32)variant_idx * 18] << 11;
        }
        count = geometry[1];
        count_part = geometry[0];
        count += count_part;
        idx = geometry[0];
        for (; idx < count; ++idx)
        {
            table_offset = idx * 4 + 672;
            XASSERT(!(table_offset > desc->buffer_bytes || desc->buffer_bytes - table_offset < 4));
            memcpy(&object_offset, geometry + table_offset, sizeof(object_offset));
            XASSERT(!(object_offset > desc->buffer_bytes || variant_offset > desc->buffer_bytes - object_offset));
            object_offset += variant_offset;
            XASSERT(!(object_offset > desc->buffer_bytes || desc->buffer_bytes - object_offset < 40));
            object_bytes = geometry + object_offset;
            if (model_visible(object_bytes) != 0)
            {
                model_pos.bits.x = ((const MODEL_HEADER *)object_bytes)->position.bits.x;
                model_pos.bits.y = ((const MODEL_HEADER *)object_bytes)->position.bits.y;
                model_pos.bits.z = ((const MODEL_HEADER *)object_bytes)->position.bits.z;
                model_transform_active_pos(&model_pos);
                render_model.bytes = (uint8 *)object_bytes;
                render_model.capacity = desc->buffer_bytes - object_offset;
                model_draw(&render_model);
            }
            else if (hd_capture_active())
            {
                render_model.bytes = (uint8 *)object_bytes;
                render_model.capacity = desc->buffer_bytes - object_offset;
                hd_draw_static_candidate(&render_model, NULL);
            }
        }
    }
    hd_original_track_capture(1);
    hd_track_draw(0);
    return 7;
}

uint32 track_stream_update(sint32 node, sint32 lane)
{
    uint32 result;
    uint32 aligned;
    uint32 idx;
    TRACK_SECTION *entry;
    uint32 state;
    sint32 lower;
    sint32 upper;
    sint32 last;
    sint32 rec;

    FUNCTION_MARKER(0x800357E4, "SLUS_006.10");
    result = asset_stream.retries;
    if (result != 0)
        return result;
    result = asset_stream.active;
    if (result != 0)
        return result;
    track_stream.queued = 0;
    aligned = (uint32)node + 10 - (uint32)(node % 20);
    track_stream.cur_node = aligned;
    lower = (sint32)(aligned - 60) / 20;
    if (lower < 0)
        lower = 0;
    upper = (sint32)(aligned + 60) / 20;
    last = (sint32)(track_stream.section_count - 1);
    if (upper < last)
        last = upper;
    for (idx = 0; idx < 7; ++idx)
    {
        entry = track_stream.sections + idx;
        state = entry->geometry_status;
        if (state == 3)
        {
            entry->geometry_status = 0;
            entry->tex_status = 0;
        }
        if (state == 2)
        {
            rec = entry->section_idx;
            if (rec < lower || last < rec)
                entry->geometry_status = 3;
        }
    }
    for (idx = lower;; ++idx)
    {
        if (last < (sint32)idx)
            return 1;
        entry = track_stream.sections + (uint32)((sint32)idx % 7);
        if (entry->tex_status == 0 && track_stream.tex_dir == 0)
            stream_queue_tex(idx);
        result = track_stream.queued;
        if (result != 0)
            return result;
        if (entry->tex_status == 4 && entry->geometry_status == 0 && track_stream.tex_dir == 0)
            stream_queue_section(idx, idx * 20, lane);
        result = track_stream.queued;
        if (result != 0)
            return result;
    }
}

static uint32 stream_is_loaded(void)
{
    uint32 val;
    uint32 idx;
    sint32 lower;
    sint32 upper;
    sint32 last;

    FUNCTION_MARKER(0x80035BD4, "SLUS_006.10");
    val = track_stream.load_ticks;
    track_stream.load_ticks = val + 1;
    if ((sint32)track_stream.load_ticks >= 1200)
        return 1;
    lower = (sint32)(track_stream.cur_node - 60);
    upper = (sint32)(track_stream.cur_node + 60);
    lower /= 20;
    if (lower < 0)
        lower = 0;
    upper /= 20;
    last = (sint32)(track_stream.section_count - 1);
    if (upper < last)
        last = upper;
    for (idx = lower; (sint32)idx <= last; ++idx)
    {
        val = ((sint32)idx % 7);
        if (track_stream.sections[val].geometry_status != 2)
            return 0;
    }
    return 1;
}

uint32 track_stream_release_pending(void)
{
    uint32 idx;
    TRACK_SECTION *entry;

    FUNCTION_MARKER(0x80035F2C, "SLUS_006.10");
    track_stream.tex_dir = 0;
    track_stream.queued = 0;
    for (idx = 0; idx < 7; ++idx)
    {
        entry = track_stream.sections + idx;
        if (entry->geometry_status == 1 || entry->geometry_status == 3)
        {
            entry->geometry_status = 0;
            if (entry->buffer != 0)
            {
                scene_pool_free(entry->buffer);
                entry->buffer = 0;
            }
        }
        if (entry->tex_status == 1 || entry->tex_status == 3)
        {
            entry->tex_status = 0;
        }
    }
    return idx;
}

static uint32 stream_count_intervals(void)
{
    uint16 wire_halfword_1;

    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    const uint8 *entry;
    uint32 count;

    FUNCTION_MARKER(0x80033F18, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    entry = scene_level_image.bytes + (level_desc->stream_intervals_offset);
    stream_interval_count = 0;
    while ((memcpy(&wire_halfword_1, entry, sizeof(wire_halfword_1)), wire_halfword_1) != 0xFFFF)
    {
        count = stream_interval_count;
        stream_interval_count = count + 1;
        entry += 6;
    }
    return 0xFFFF;
}

static uint32 stream_select_variant(sint32 node, sint32 lane)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3;

    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    const uint8 *entry;
    uint32 idx;

    FUNCTION_MARKER(0x80033FA4, "SLUS_006.10");
    (void)track_geometry.branch_count;
    level_desc = &scene_level_image.desc;
    entry = scene_level_image.bytes + (level_desc->stream_intervals_offset);
    for (idx = 0; (sint32)idx < (sint32)stream_interval_count; ++idx)
    {
        if (node >= (memcpy(&wire_halfword_1, entry, sizeof(wire_halfword_1)), wire_halfword_1) && node <= (memcpy(&wire_halfword_2, entry + 2, sizeof(wire_halfword_2)), wire_halfword_2))
        {
            if ((memcpy(&wire_halfword_3, entry + 4, sizeof(wire_halfword_3)), wire_halfword_3) == 0)
                return 3;
            if (lane < (sint32)track_geometry.main_node_count)
                return 1;
            return 2;
        }
        entry += 6;
    }
    return 3;
}

uint32 track_stream_draw_linear(uint32 slot)
{
    uint32 wire_word_6;
    VEC3 model_pos;
    MODEL_RESOURCE render_model;
    TRACK_LINEAR_ENTRY *entry;
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 mode, rotation0, rotation1, rotation2, rotation3, rotation4;
    uint32 state, idx, object_idx;
    const uint8 *resource;
    uint32 offset, table_offset;
    const uint8 *payload;
    const uint8 *object_bytes;
    sint32 count, object_count;

    FUNCTION_MARKER(0x80049BC4, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    mode = level_header[30] & 1;
    if (mode != 0)
    {
        model_update_distance_reference(&car_states[0].transform, &car_states[0].pos);
    }
    if (hd_track_ready() && hd_capture_active())
    {
        if (!hd_native_capture_required())
        {
            hd_track_draw(slot);
            return 0;
        }
        hd_original_track_capture(0);
    }
    rotation0 = model_render.view.words[0];
    rotation1 = model_render.view.words[1];
    xport_gte_write_control(0, rotation0);
    xport_gte_write_control(1, rotation1);
    rotation2 = model_render.view.words[2];
    rotation3 = model_render.view.words[3];
    rotation4 = model_render.view.words[4];
    xport_gte_write_control(2, rotation2);
    xport_gte_write_control(3, rotation3);
    xport_gte_write_control(4, rotation4);
    count = (sint32)linear_cache.pair_count;
    if (count <= 0)
    {
        hd_original_track_capture(1);
        return mode != 0 ? slot * 2 : (uint32)count;
    }
    entry = linear_cache.entries + slot;
    idx = 0;
    do
    {
        resource = entry->buffer;
        state = entry->state;
        XASSERT(!(state == 2 && (resource == NULL || entry->buffer_bytes < 4)));
        object_count = state == 2 ? (sint32)(memcpy(&wire_word_6, resource, sizeof(wire_word_6)), wire_word_6) : 0;
        if (state == 2 && object_count > 0)
        {
            payload = resource;
            object_idx = 0;
            do
            {
                table_offset = 4 + object_idx * 4;
                XASSERT(!(table_offset > entry->buffer_bytes || entry->buffer_bytes - table_offset < 4));
                memcpy(&offset, payload + table_offset, sizeof(offset));
                XASSERT(!(offset > entry->buffer_bytes || entry->buffer_bytes - offset < 40));
                object_bytes = payload + offset;
                if (model_visible(object_bytes) != 0)
                {
                    model_pos.bits.x = ((const MODEL_HEADER *)object_bytes)->position.bits.x;
                    model_pos.bits.y = ((const MODEL_HEADER *)object_bytes)->position.bits.y;
                    model_pos.bits.z = ((const MODEL_HEADER *)object_bytes)->position.bits.z;
                    model_transform_active_pos(&model_pos);
                    render_model.bytes = (uint8 *)object_bytes;
                    render_model.capacity = entry->buffer_bytes - offset;
                    if (mode != 0)
                        model_draw_lod(&render_model);
                    else
                        model_draw(&render_model);
                }
                else if (hd_capture_active())
                {
                    render_model.bytes = (uint8 *)object_bytes;
                    render_model.capacity = entry->buffer_bytes - offset;
                    hd_draw_static_candidate(&render_model, NULL);
                }
                ++object_idx;
            } while ((sint32)object_idx < object_count);
        }
        count = (sint32)linear_cache.pair_count;
        ++idx;
        if ((sint32)idx < count)
            entry += 2;
    } while ((sint32)idx < count);
    hd_original_track_capture(1);
    hd_track_draw(slot);
    return 0;
}

uint32 track_stream_init_linear(void)
{
    char filename[72];
    uint32 result;
    uint32 flag;
    const char *prefix;
    uint32 level;
    uint32 size;
    uint32 idx;
    uint8 *entry;
    uint8 *image;
    uint32 val;
    uint32 count;
    uint32 offset;
    uint32 numerator;
    sint32 formatted;

    FUNCTION_MARKER(0x80049364, "SLUS_006.10");
    hd_forget_span(track_linear_image.bytes, track_linear_image.size);
    free(track_linear_image.bytes);
    track_linear_image = (TRACK_LINEAR_IMAGE){0};
    result = race_session.linear_course;
    if (result == 0)
        return result;
    flag = race_session.two_players;
    linear_cache.radius = 47;
    if (flag != 0)
        linear_cache.radius = 35;
    ;
    prefix = scene_level_image.asset_prefix;
    level = race_session.track;
    race_session.linear_course = (1);
    formatted = sprintf(filename, "%sCIR%02d.DAT", prefix, (sint32)(level + 1));
    size = archive_aligned_size(filename);
    track_linear_image.bytes = (uint8 *)malloc(size);
    XASSERT(!(track_linear_image.bytes == NULL));
    track_linear_image.size = archive_read_file(filename, track_linear_image.bytes, size);
    XASSERT(!(track_linear_image.size < 4 || track_linear_image.size > size));
    image = track_linear_image.bytes;
    memcpy(&count, image, sizeof(count));
    XASSERT(!(count > (track_linear_image.size - 4) / 4));
    for (idx = 0; idx < count; ++idx)
    {
        entry = image + 4 + idx * 4;
        memcpy(&val, entry, sizeof(val));
        XASSERT(!(val < 4 + count * 4 || val > track_linear_image.size || track_linear_image.size - val < 18));
    }
    size = linear_cache.pair_count * 2 * sizeof(linear_cache.entries[0]);
    memset(linear_cache.entries, 0, size);
    numerator = track_geometry.main_node_count + 11;
    count = linear_cache.radius + 1;
    val = ((sint32)numerator / 12);
    linear_cache.grp_count = val;
    result = (uint32)((sint32)count / 6) + 1;
    linear_cache.pair_count = result;
    if ((sint32)result > 0)
    {
        idx = 0;
        offset = 0;
        for (;;)
        {
            linear_cache.entries[offset].state = 0;
            linear_cache.entries[offset + 1].state = 0;
            count = linear_cache.pair_count;
            linear_cache.entries[offset].buffer = 0;
            linear_cache.entries[offset + 1].buffer = 0;
            linear_cache.entries[offset].buffer_bytes = 0;
            linear_cache.entries[offset + 1].buffer_bytes = 0;
            linear_cache.entries[offset].section = 0xFFFF;
            linear_cache.entries[offset + 1].section = 0xFFFF;
            ++idx;
            result = (sint32)idx < (sint32)count;
            offset += 2;
            if (result == 0)
                break;
        }
    }
    return result;
}

uint32 track_stream_update_linear(uint32 slot, sint32 node)
{
    uint32 wire_word_10, wire_word_12;
    uint32 src_offset;
    TRACK_LINEAR_ENTRY *entry;
    const uint8 *src_table;
    TRACK_LINEAR_ENTRY *candidate_entry;
    sint32 track, groups, quotient, cur, end, start, count, candidate;
    uint32 centered, radius, lower, upper, list_count = 0, idx;
    uint32 state, matches, first_free, offset, src, len;
    uint8 *dst;
    uint32 result;

    FUNCTION_MARKER(0x80049638, "SLUS_006.10");
    track = (sint32)track_geometry.main_node_count;
    groups = (sint32)((uint32)track + 11) / 12;
    quotient = fixed_divide_checked(node, track);
    cur = ((uint32)node - (uint32)quotient * (uint32)track);
    if (node < 0)
        cur = ((uint32)cur + (uint32)track);
    centered = (uint32)(cur / 12) * 12 + 6;
    radius = linear_cache.radius;
    track = (sint32)track_geometry.main_node_count;
    lower = centered - radius;
    upper = centered + radius;
    quotient = fixed_divide_checked((sint32)upper, track);
    cur = (upper - (uint32)quotient * (uint32)track);
    if ((sint32)lower < 0)
        lower += (uint32)track;
    cur /= 12;
    quotient = fixed_divide_checked(cur, groups);
    end = ((uint32)cur - (uint32)quotient * (uint32)groups);
    start = (sint32)lower / 12;
    cur = start;
    while (cur != end)
    {
        if (list_count < TRACK_LINEAR_WINDOW_CAPACITY - 1)
            linear_cache.desired_sections[list_count] = (uint32)cur;
        else
            wheel_skid_controls[0].section_word = (uint32)cur;
        cur = ((uint32)cur + 1);
        quotient = fixed_divide_checked(cur, groups);
        cur = ((uint32)cur - (uint32)quotient * (uint32)groups);
        ++list_count;
    }
    count = (sint32)linear_cache.pair_count;
    idx = 0;
    if (count > 0)
    {
        entry = linear_cache.entries + slot;
        do
        {
            state = entry->state;
            if (state == 3)
                entry->state = (0);
            if (state == 2)
            {
                matches = 0;
                if ((sint32)list_count > 0)
                {
                    candidate = (sint16)entry->section;
                    offset = 0;
                    do
                    {
                        if (candidate == (sint32)(offset < TRACK_LINEAR_WINDOW_CAPACITY - 1 ? linear_cache.desired_sections[offset] : wheel_skid_controls[0].section_word))
                            ++matches;
                        ++offset;
                    } while ((sint32)offset < (sint32)list_count);
                }
                if (matches == 0)
                    entry->state = (3);
            }
            count = (sint32)linear_cache.pair_count;
            ++idx;
            if ((sint32)idx < count)
                entry += 2;
        } while ((sint32)idx < count);
    }
    result = slot * 2;
    cur = start;
    while (cur != end)
    {
        count = (sint32)linear_cache.pair_count;
        first_free = 0xFFFFFFFF;
        matches = 0;
        idx = 0;
        if (count > 0)
        {
            candidate_entry = linear_cache.entries + slot;
            do
            {
                state = candidate_entry->state;
                if (state == 0 && first_free == 0xFFFFFFFF)
                    first_free = idx;
                candidate = (sint16)candidate_entry->section;
                if (candidate == cur && candidate_entry->state == 2)
                    ++matches;
                ++idx;
                if ((sint32)idx < count)
                    candidate_entry += 2;
            } while ((sint32)idx < count);
        }
        if (matches != 0)
            first_free = 0xFFFFFFFF;
        result = first_free * 2;
        if (first_free != 0xFFFFFFFF)
        {
            entry = linear_cache.entries + slot + first_free * 2;
            result = entry->state;
            if (result == 0)
            {
                dst = entry->buffer;
                if (dst != 0)
                    scene_pool_free(dst);
                {
                    sint32 operation_ok = !(track_linear_image.bytes == NULL || track_linear_image.size < 4 || cur < 0 || (uint32)cur >= (memcpy(&wire_word_10, track_linear_image.bytes, sizeof(wire_word_10)), wire_word_10));
                    XASSERT(operation_ok);
                }
                src_table = track_linear_image.bytes + (uint32)cur * 4;
                memcpy(&src, src_table + 4, sizeof(src));
                src_offset = src;
                XASSERT(!(src_offset > track_linear_image.size || track_linear_image.size - src_offset < 8));
                len = rnc_allocation_size(track_linear_image.bytes + src_offset);
                (void)(len);
                scene_pool_alloc(len, &entry->buffer);
                entry->buffer_bytes = len - 1024;
                entry->state = (1);
                {
                    sint32 operation_ok = !(track_linear_image.bytes == NULL || track_linear_image.size < 4 || cur < 0 || (uint32)cur >= (memcpy(&wire_word_12, track_linear_image.bytes, sizeof(wire_word_12)), wire_word_12));
                    XASSERT(operation_ok);
                }
                src_table = track_linear_image.bytes + (uint32)cur * 4;
                dst = entry->buffer;
                memcpy(&src, src_table + 4, sizeof(src));
                src_offset = src;
                XASSERT(!(src_offset > track_linear_image.size));
                rnc_decode(track_linear_image.bytes + src_offset, track_linear_image.size - src_offset, dst, (sint32)len);
                entry->state = (2);
                entry->section = (uint16)((uint32)cur);
                entry->center = (uint16)(centered);
                result = 2;
            }
        }
        cur = ((uint32)cur + 1);
        quotient = fixed_divide_checked(cur, groups);
        cur = ((uint32)cur - (uint32)quotient * (uint32)groups);
    }
    return result;
}

uint32 track_stream_wait_for_load(void)
{
    uint32 idx;
    uint32 val;
    uint32 result;
    sint32 node;
    sint32 lane;

    FUNCTION_MARKER(0x80013D20, "SLUS_006.10");
    race_view.primary_car = (uint32)(0);
    val = race_view.primary_car;
    race_view.tracked_car = (uint32)(val);
    race_control.stream_wait_marker = 1;
    for (idx = 0; (sint32)idx < (sint32)race_control.car_count; ++idx)
        track_wrap_location(&car_states[(idx * 864) / 864u].track_location);
    for (;;)
    {
        result = stream_is_loaded();
        if (result != 0)
            return result;
        for (idx = 0; (sint32)idx < (sint32)race_control.car_count; ++idx)
            track_wrap_location(&car_states[(idx * 864) / 864u].track_location);
        val = race_view.tracked_car;
        node = (sint16)car_states[(val * 864) / 864u].track_location.n_node;
        val = race_view.tracked_car;
        lane = (sint16)car_states[(val * 864) / 864u].track_location.node;
        track_stream_update(node, lane);
        DrawSync(0);
        stream_update();
        track_stream_upload_images();
    }
}

uint32 track_stream_section_count(void)
{
    return track_stream.section_count;
}

uint32 track_stream_variant(sint32 node, sint32 lane)
{
    uint32 variant = stream_select_variant(node, lane);
    if (track_stream.variants[(uint32)(node / 20) * 18 + 1] == 0)
        variant = 3;
    return variant;
}

sint32 track_stream_hd_resident(uint32 slot, uint32 section, uint32 variant)
{
    uint32 index;
    if (race_session.linear_course)
    {
        for (index = 0; index < linear_cache.pair_count; ++index)
        {
            const TRACK_LINEAR_ENTRY *entry = &linear_cache.entries[index * 2 + slot];
            if (entry->state == 2 && entry->section == section)
                return 1;
        }
    }
    else
        for (index = 0; index < 7; ++index)
        {
            const TRACK_SECTION *entry = &track_stream.sections[index];
            if (entry->geometry_status == 2 && entry->section_idx == section && (entry->variant_mask & variant) == variant)
                return 1;
        }
    return 0;
}
