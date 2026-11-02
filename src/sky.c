#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "game.h"

static const uint32 prim_codes[2] = {0x80071E68, 0x80071EFC};

typedef char SKY_PACKET_LAYOUT_CHECK[sizeof(POLY_FT3) == 32 && sizeof(POLY_FT4) == 40 && sizeof(POINT_T) == 8 && sizeof(XY) == 4 && offsetof(POLY_FT3, p) == 8 && offsetof(POLY_FT4, p) == 8 ? 1 : -1];

typedef union SKY_VERTEX
{
    SVECTOR sdk;
    uint32 words[2];
} SKY_VERTEX;

typedef char SKY_VERTEX_SIZE_CHECK[sizeof(SKY_VERTEX) == 8 && sizeof(SVECTOR) == 8 ? 1 : -1];

static const sint8 sky_visibility[3][3][16] = {
    {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        {0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    },
};

static uint32 clip_mask(const XY *coord)
{
    sint32 x = coord->x;
    sint32 y = coord->y;
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

static uint32 draw_model(uint8 *model_data, const sint8 *visibility)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5;

    const SKY_VERTEX *vertex_input;
    DISPLAY_ORDERING ordering;
    uint32 stream_offset, buffer, packet_offset, vertex_offset;
    const uint8 *prim, *vertices, *vertex, *v3;
    uint8 *packet_cursor;
    uint32 limit, count, kind, target, val, idx;
    uint32 idx0, idx1, idx2, mask, enabled, prev;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80071E0C, "SLUS_006.10");
    stream_offset = ((const MODEL_HEADER *)model_data)->stream_offset;
    buffer = disp_state.bank;
    prim = model_data + stream_offset;
    memcpy(&packet_offset, model_data + 0x34 + buffer * 4, sizeof(packet_offset));
    vertex_offset = ((const MODEL_HEADER *)model_data)->vertex_offset;
    packet_cursor = model_data + packet_offset;
    vertices = model_data + vertex_offset;
    cur_frame = disp_state.cur;
    limit = disp_state.ordering_count;
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    limit = render_add_checked(limit, (uint32)-1, 0x80071E48);
    ordering.byte_offset += limit * 4;
    for (;;)
    {
        count = (sint16)(memcpy(&wire_halfword_1, prim, sizeof(wire_halfword_1)), wire_halfword_1);
        kind = prim[2];
        val = kind * 4;
        if (count == 0)
            return val;
        XASSERT(!(kind >= 2));
        target = prim_codes[kind];
        prim += 4;
        do
        {
            enabled = *visibility;
            count = render_add_checked(count, (uint32)-1, target == 0x80071E68 ? 0x80071E6C : 0x80071F00);
            ++visibility;
            if (enabled != 0)
            {
                idx0 = (sint16)(memcpy(&wire_halfword_2, prim, sizeof(wire_halfword_2)), wire_halfword_2);
                idx1 = (sint16)(memcpy(&wire_halfword_3, prim + 2, sizeof(wire_halfword_3)), wire_halfword_3);
                vertex = vertices + idx0 * 8;
                vertex_input = (const SKY_VERTEX *)vertex;
                xport_gte_write_data(0, vertex_input->words[0]);
                xport_gte_write_data(1, vertex_input->words[1]);
                vertex = vertices + idx1 * 8;
                idx2 = (sint16)(memcpy(&wire_halfword_4, prim + 4, sizeof(wire_halfword_4)), wire_halfword_4);
                vertex_input = (const SKY_VERTEX *)vertex;
                xport_gte_write_data(2, vertex_input->words[0]);
                xport_gte_write_data(3, vertex_input->words[1]);
                vertex = vertices + idx2 * 8;
                v3 = target == 0x80071EFC ? vertices + (uint32)(sint32)(sint16)(memcpy(&wire_halfword_5, prim + 6, sizeof(wire_halfword_5)), wire_halfword_5) * 8 : NULL;
                vertex_input = (const SKY_VERTEX *)vertex;
                xport_gte_write_data(4, vertex_input->words[0]);
                xport_gte_write_data(5, vertex_input->words[1]);
                xport_gte_execute(0x280030);
                if (target == 0x80071E68)
                {
                    POLY_FT3 *poly = (POLY_FT3 *)packet_cursor;
                    poly->p[0].xy.xy = xport_gte_read_data(12);
                    poly->p[1].xy.xy = xport_gte_read_data(13);
                    poly->p[2].xy.xy = xport_gte_read_data(14);
                    prev = ordering.tags[(sint32)ordering.byte_offset / 4];
                    ordering.tags[(sint32)ordering.byte_offset / 4] = gpu_packet_link((uintptr_t)poly);
                    poly->tag = 0x07000000 | (prev & 0xFFFFFF);
                }
                else
                {
                    POLY_FT4 *poly = (POLY_FT4 *)packet_cursor;
                    poly->p[0].xy.xy = xport_gte_read_data(12);
                    mask = clip_mask(&poly->p[0].xy);
                    poly->p[1].xy.xy = xport_gte_read_data(13);
                    val = clip_mask(&poly->p[1].xy);
                    vertex_input = (const SKY_VERTEX *)v3;
                    xport_gte_write_data(0, vertex_input->words[0]);
                    xport_gte_write_data(1, vertex_input->words[1]);
                    poly->p[2].xy.xy = xport_gte_read_data(14);
                    idx = clip_mask(&poly->p[2].xy);
                    xport_gte_execute(0x180001);
                    poly->p[3].xy.xy = xport_gte_read_data(14);
                    mask &= val & idx & clip_mask(&poly->p[3].xy);
                    if (mask == 0)
                    {
                        prev = ordering.tags[(sint32)ordering.byte_offset / 4];
                        ordering.tags[(sint32)ordering.byte_offset / 4] = gpu_packet_link((uintptr_t)poly);
                        poly->tag = 0x09000000 | (prev & 0xFFFFFF);
                    }
                }
            }
            prim += 20;
            packet_cursor += target == 0x80071E68 ? 32 : 40;
        } while (count != 0);
    }
}

uint32 sky_draw(sint32 idx)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5;

    VEC3 model_pos;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint8 *owner;
    uint8 *model_data;
    const uint8 *range;
    uint32 lower;
    uint32 selection;
    uint32 grp;
    uint32 item;
    uint32 model_offset;
    const sint8 *table;
    sint32 range_start;
    sint32 node;

    FUNCTION_MARKER(0x8003A1B4, "SLUS_006.10");
    if (idx < 0)
        selection = 1;
    else
    {
        level_desc = &scene_level_image.desc;
        range = scene_level_image.bytes + (level_desc->sky_ranges_offset);
        range_start = (sint16)(memcpy(&wire_halfword_1, range, sizeof(wire_halfword_1)), wire_halfword_1);
        node = (sint32)camera_views[(uint32)idx].track_node;
        lower = (memcpy(&wire_halfword_2, range, sizeof(wire_halfword_2)), wire_halfword_2);
        selection = 0;
        while (range_start >= 0)
        {
            if ((sint16)(memcpy(&wire_halfword_3, range + 2, sizeof(wire_halfword_3)), wire_halfword_3) < node)
                break;
            if ((sint16)lower < node)
            {
                memcpy(&selection, range + 4, sizeof(selection));
                break;
            }
            range += 8;
            range_start = (sint16)(memcpy(&wire_halfword_4, range, sizeof(wire_halfword_4)), wire_halfword_4);
            lower = (memcpy(&wire_halfword_5, range, sizeof(wire_halfword_5)), wire_halfword_5);
        }
    }
    for (grp = 0; grp < 3; ++grp)
    {
        table = sky_visibility[grp][selection];
        for (item = 0; item < 6; ++item)
        {
            owner = scene_track_image.bytes;
            XASSERT(!(owner == NULL || scene_track_image.size < 88));
            memcpy(&model_offset, owner + 0x10 + grp * 24 + item * 4, sizeof(model_offset));
            XASSERT(!(model_offset > scene_track_image.size || scene_track_image.size - model_offset < 60));
            model_data = owner + model_offset;
            if (model_visible_origin(model_data) != 0)
            {
                model_pos.bits.x = ((const MODEL_HEADER *)model_data)->position.bits.x;
                model_pos.bits.y = ((const MODEL_HEADER *)model_data)->position.bits.y;
                model_pos.bits.z = ((const MODEL_HEADER *)model_data)->position.bits.z;
                model_transform_pos(&model_pos);
                if (hd_capture_active())
                {
                    MODEL_RESOURCE resource = {model_data, scene_track_image.size - model_offset};
                    hd_draw_sky(&resource, table);
                }
                draw_model(model_data, table);
            }
            else if (hd_capture_active())
            {
                MODEL_RESOURCE resource = {model_data, scene_track_image.size - model_offset};
                hd_draw_static_candidate(&resource, table);
            }
        }
    }
    return 0;
}

void sky_prepare_hd(void)
{
    uint32 group, selection, item;
    for (group = 0; group < 3; ++group)
        for (selection = 0; selection < 3; ++selection)
            for (item = 0; item < 6; ++item)
            {
                uint32 offset;
                MODEL_RESOURCE resource;
                memcpy(&offset, scene_track_image.bytes + 16 + group * 24 + item * 4, sizeof(offset));
                XASSERT(offset <= scene_track_image.size && sizeof(MODEL_HEADER) <= scene_track_image.size - offset);
                resource = (MODEL_RESOURCE){scene_track_image.bytes + offset, scene_track_image.size - offset};
                hd_prepare_sky_model(&resource, sky_visibility[group][selection]);
            }
}
