#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "hd_track.h"
#include "hd_texture_cache.h"
#include "psx_gpu.h"

enum
{
    HD_CLW_RADIUS = 6,
    HD_CIR_RADIUS = 8
};

typedef struct HD_CLW_RECORD
{
    uint8 geometry[2], textures[2];
    uint16 texture_offsets[2], last_node, sector, reserved[3];
} HD_CLW_RECORD;

typedef char HD_CLW_RECORD_SIZE[sizeof(HD_CLW_RECORD) == 18 ? 1 : -1];

typedef struct HD_TRACK_BLOCK
{
    uint8 *bytes;
    uint32 size, count, main_count;
    HD_PREPARED_MODEL **models;
    HD_TRACK_IMAGE *image;
} HD_TRACK_BLOCK;

static struct
{
    HD_TRACK_BLOCK *blocks;
    uint8 *clw, *base;
    uint32 count, clw_size, base_size, model_count;
    float horizon;
} extended;

void hd_track_reset(void)
{
    uint32 index;
    for (index = extended.count; index > 0; --index)
    {
        HD_TRACK_BLOCK *block = &extended.blocks[index - 1];
        hd_forget_span(block->bytes, block->size);
        free(block->models);
        free(block->bytes);
        hd_track_image_release(block->image);
    }
    free(extended.blocks);
    free(extended.clw);
    free(extended.base);
    memset(&extended, 0, sizeof(extended));
    hd_texture_sources_reset();
}

sint32 hd_track_ready(void)
{
    return extended.blocks != NULL;
}

float hd_track_horizon(void)
{
    uint32 node, valid = 0;
    float length = 0.0f;
    VECTORF first = {0}, previous = {0};
    if (extended.horizon > 0.0f)
        return extended.horizon;
    if (track_geometry.main_node_count == 0)
        return 24576.0f;
    for (node = 0; node < track_geometry.main_node_count; ++node)
    {
        const uint8 *record = track_geometry.node_bytes + node * 24;
        VEC3 origin;
        VEC3_16 a, b;
        uint16 left, right;
        VECTORF center, delta;
        memcpy(&left, record + 4, sizeof(left));
        memcpy(&right, record + 6, sizeof(right));
        memcpy(&origin, record + 12, sizeof(origin));
        XASSERT(left < track_geometry.vertex_count && right < track_geometry.vertex_count);
        memcpy(&a, track_geometry.vertex_bytes + left * sizeof(a), sizeof(a));
        memcpy(&b, track_geometry.vertex_bytes + right * sizeof(b), sizeof(b));
        center = (VECTORF){origin.x + (a.x + b.x) * 0.5f, origin.y + (a.y + b.y) * 0.5f, origin.z + (a.z + b.z) * 0.5f};
        if (valid == 0)
            first = center;
        else
        {
            delta = (VECTORF){center.vx - previous.vx, center.vy - previous.vy, center.vz - previous.vz};
            length += sqrtf(delta.vx * delta.vx + delta.vy * delta.vy + delta.vz * delta.vz);
        }
        previous = center;
        ++valid;
    }
    {
        VECTORF delta = {first.vx - previous.vx, first.vy - previous.vy, first.vz - previous.vz};
        length += sqrtf(delta.vx * delta.vx + delta.vy * delta.vy + delta.vz * delta.vz);
    }
    // calibrate the same fog proportions to the authored route's cell spacing
    extended.horizon = fmaxf(4096.0f, length / valid * (race_session.linear_course ? HD_CIR_RADIUS * 12 : HD_CLW_RADIUS * 20) * 0.8f);
    return extended.horizon;
}

static const uint8 *source_range(uint32 sector, uint32 sectors)
{
    uint64 offset = (uint64)sector * 2048, size = (uint64)sectors * 2048;
    XASSERT(offset <= extended.clw_size && size <= extended.clw_size - offset);
    return extended.clw + (uint32)offset;
}

static HD_CLW_RECORD clw_record(uint32 section)
{
    uint32 offset;
    HD_CLW_RECORD record;
    memcpy(&offset, scene_track_image.bytes + 12, sizeof(offset));
    XASSERT(offset <= scene_track_image.size && (uint64)(section + 1) * sizeof(record) <= scene_track_image.size - offset);
    memcpy(&record, scene_track_image.bytes + offset + section * sizeof(record), sizeof(record));
    return record;
}

static void prepare_sources(void)
{
    char filename[80];
    uint32 read;
    if (extended.clw != NULL)
        return;
    sprintf(filename, "%sLEVEL%d.CLW", scene_level_image.asset_prefix, race_session.track + 1);
    extended.clw_size = archive_file_size(filename);
    extended.clw = (uint8 *)malloc(extended.clw_size);
    XASSERT(extended.clw != NULL);
    read = archive_read_file(filename, extended.clw, extended.clw_size);
    XASSERT(read == extended.clw_size);
    sprintf(filename, "%sVRAM.OUT", scene_level_image.asset_prefix);
    extended.base_size = archive_file_size(filename);
    extended.base = (uint8 *)malloc(extended.base_size);
    XASSERT(extended.base != NULL && extended.base_size >= 512 * 512 * 2);
    read = archive_read_file(filename, extended.base, extended.base_size);
    XASSERT(read == extended.base_size);
    (void)read;
}

static void apply_section(uint16 *pixels, uint32 section)
{
    uint32 row;
    HD_CLW_RECORD record = clw_record(section);
    const uint8 *data;
    uint32 cursor = 0x5AC, part, size = record.textures[0] * 2048;
    if (size == 0)
        return;
    data = source_range(record.sector + record.texture_offsets[0], record.textures[0]);
    XASSERT(size >= cursor);
    for (part = 0; part < (uint32)data[2] + data[3]; ++part)
    {
        uint32 x, y, width, height;
        if (part < data[2])
        {
            uint32 shape = data[0x84 + part];
            x = data[4 + part] * 4;
            y = data[0x44 + part] * 4;
            width = (shape & 15) * 2;
            height = (shape >> 4) * 8;
        }
        else
        {
            uint32 palette = part - data[2];
            x = data[0xC4 + palette] * 4;
            y = data[0x104 + palette];
            width = 16;
            height = 1;
        }
        XASSERT(x + width <= 1024 && y + height <= 512 && cursor <= size && width * height * 2 <= size - cursor);
        if (part < data[2] && width != 0 && height != 0)
            hd_texture_source_upload(x, y, width, height, data + cursor, size - cursor);
        for (row = 0; row < height; ++row)
            memcpy(pixels + (y + row) * 1024 + x, data + cursor + row * width * 2, width * 2);
        cursor += width * height * 2;
    }
}

static void prepare_block(HD_TRACK_BLOCK *block, uint32 section, uint16 *pixels)
{
    uint32 index, table, secondary = 0;
    if (race_session.linear_course)
    {
        uint32 offset, allocation;
        memcpy(&offset, track_linear_image.bytes + 4 + section * 4, sizeof(offset));
        XASSERT(offset <= track_linear_image.size && track_linear_image.size - offset >= 18);
        allocation = rnc_allocation_size(track_linear_image.bytes + offset);
        XASSERT(allocation > 1024);
        block->bytes = (uint8 *)malloc(allocation);
        XASSERT(block->bytes != NULL);
        rnc_decode(track_linear_image.bytes + offset, track_linear_image.size - offset, block->bytes, allocation);
        block->size = allocation - 1024;
        XASSERT(block->size >= 4);
        memcpy(&block->count, block->bytes, sizeof(block->count));
        block->main_count = block->count;
        table = 4;
    }
    else
    {
        HD_CLW_RECORD record = clw_record(section);
        block->size = ((uint32)record.geometry[0] + record.geometry[1]) * 2048;
        XASSERT(block->size >= 0x2A0);
        block->bytes = (uint8 *)malloc(block->size);
        XASSERT(block->bytes != NULL);
        memcpy(block->bytes, source_range(record.sector, record.geometry[0] + record.geometry[1]), block->size);
        apply_section(pixels, section);
        block->main_count = block->bytes[0];
        block->count = block->main_count + block->bytes[1];
        secondary = record.geometry[0] * 2048;
        table = 0x2A0;
    }
    XASSERT(table <= block->size && block->count <= (block->size - table) / 4);
    block->models = (HD_PREPARED_MODEL **)calloc(block->count != 0 ? block->count : 1, sizeof(*block->models));
    XASSERT(block->models != NULL);
    for (index = 0; index < block->count; ++index)
    {
        uint32 offset;
        MODEL_RESOURCE model;
        memcpy(&offset, block->bytes + table + index * 4, sizeof(offset));
        if (!race_session.linear_course && index >= block->main_count)
            offset += secondary;
        XASSERT(offset <= block->size && sizeof(MODEL_HEADER) <= block->size - offset);
        model = (MODEL_RESOURCE){block->bytes + offset, block->size - offset};
        block->models[index] = hd_prepare_track_model(&model, pixels);
        ++extended.model_count;
    }
}

void hd_track_prepare(void)
{
    uint32 section, maximum = 0, window, index;
    uint16 *pixels = NULL;
    if (extended.blocks != NULL)
        return;
    if (race_session.linear_course)
    {
        XASSERT(track_linear_image.bytes != NULL && track_linear_image.size >= 4);
        memcpy(&extended.count, track_linear_image.bytes, sizeof(extended.count));
        XASSERT(extended.count <= (track_linear_image.size - 4) / 4);
    }
    else
    {
        uint32 row;
        extended.count = track_stream_section_count();
        prepare_sources();
        pixels = (uint16 *)calloc(1024 * 512, sizeof(*pixels));
        XASSERT(pixels != NULL);
        for (row = 0; row < 512; ++row)
            memcpy(pixels + row * 1024 + 512, extended.base + row * 512 * 2, 512 * 2);
    }
    XASSERT(extended.count != 0);
    hd_texture_sources_begin(pixels != NULL ? pixels : VRAM);
    extended.blocks = (HD_TRACK_BLOCK *)calloc(extended.count, sizeof(*extended.blocks));
    XASSERT(extended.blocks != NULL);
    for (section = 0; section < extended.count; ++section)
        prepare_block(&extended.blocks[section], section, pixels);
    {
        uint32 total = 0, at = 0;
        HD_PREPARED_MODEL **models;
        uint32 *sections;
        for (section = 0; section < extended.count; ++section)
            total += extended.blocks[section].count;
        models = (HD_PREPARED_MODEL **)malloc(total * sizeof(*models));
        sections = (uint32 *)malloc(total * sizeof(*sections));
        XASSERT(models != NULL && sections != NULL);
        for (section = 0; section < extended.count; ++section)
            for (index = 0; index < extended.blocks[section].count; ++index)
            {
                models[at] = extended.blocks[section].models[index];
                sections[at++] = section;
            }
        hd_weld_course(models, sections, total, extended.count);
        free(sections);
        free(models);
    }
    free(pixels);
    hd_texture_sources_seal();
    free(extended.base);
    free(extended.clw);
    extended.base = extended.clw = NULL;
    window = race_session.linear_course ? HD_CIR_RADIUS * 2 + 1 : HD_CLW_RADIUS * 2 + 1;
    for (section = 0; section < extended.count; ++section)
    {
        uint32 count = 0;
        for (index = 0; index < window; ++index)
            count += extended.blocks[(section + index) % extended.count].count;
        if (count > maximum)
            maximum = count;
    }
    hd_prepare_draw_capacity(maximum * 2 + 512);
    hd_track_horizon();
}

void hd_track_prepare_gpu(void)
{
    uint32 section, maximum = 0, index;
    uint32 window = race_session.linear_course ? HD_CIR_RADIUS * 2 + 1 : HD_CLW_RADIUS * 2 + 1;
    for (section = 0; section < extended.count; ++section)
    {
        HD_TRACK_BLOCK *block = &extended.blocks[section];
        hd_prepare_track_geometry(block->models, block->count, block->main_count, section);
    }
    for (section = 0; section < extended.count; ++section)
    {
        uint32 slots = 0;
        for (index = 0; index < window; ++index)
        {
            HD_TRACK_BLOCK *block = &extended.blocks[(section + index) % extended.count];
            uint32 model;
            for (model = 0; model < block->count; ++model)
                slots += hd_prepared_draw_slots(block->models[model]);
        }
        if (slots > maximum)
            maximum = slots;
    }
    hd_prepare_draw_capacity(maximum * 2 + 512);
}

static void draw_block(HD_TRACK_BLOCK *block, uint32 variant)
{
    uint32 index, first = !race_session.linear_course && variant == 2 ? block->main_count : 0;
    uint32 count = !race_session.linear_course && variant == 1 ? block->main_count : block->count;
    hd_track_image_bind(block->image);
    for (index = first; index < count; ++index)
        hd_draw_prepared_static(block->models[index], 0);
    hd_track_image_bind(NULL);
}

void hd_track_draw(uint32 slot)
{
    sint32 section, center, lower, upper;
    uint32 count = extended.count;
    if (!hd_capture_active() || extended.blocks == NULL)
        return;
    if (race_session.linear_course)
    {
        center = car_states[slot].track_location.n_node / 12;
        lower = center - HD_CIR_RADIUS;
        upper = center + HD_CIR_RADIUS;
    }
    else
    {
        center = (car_states[slot].track_location.n_node + 10) / 20;
        lower = center - HD_CLW_RADIUS;
        upper = center + HD_CLW_RADIUS;
    }
    for (section = lower; section <= upper; ++section)
    {
        uint32 selected, variant;
        if (!race_session.linear_course && (section < 0 || section >= (sint32)count))
            continue;
        selected = (uint32)((section % (sint32)count + (sint32)count) % (sint32)count);
        variant = race_session.linear_course ? 0 : track_stream_variant(selected * 20, car_states[slot].track_location.node);
        draw_block(&extended.blocks[selected], variant);
    }
}
