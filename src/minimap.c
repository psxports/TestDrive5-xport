#include <string.h>

#include <stdlib.h>

#include "game.h"
#include "psx_gpu.h"

typedef struct MINIMAP_POINT
{
    uint32 x;
    uint32 y;
} MINIMAP_POINT;

typedef struct MINIMAP_FIXED_PACKETS
{
    LINE_F2 border[4][2];
    POLY_F4 background[2];
    DR_ENV draw_environment[2];
    LINE_F2 boundary[2][2];
    POLY_F4 cars[2][12];
    POLY_F4 road[2][48];
} MINIMAP_FIXED_PACKETS;

static const CVECTOR car_colors[12] = {
    {255, 255, 255, 0}, {255, 64, 64, 0}, {64, 255, 64, 0}, {64, 128, 255, 0}, {255, 255, 64, 0}, {255, 64, 255, 0}, {128, 128, 128, 0}, {128, 128, 128, 0}, {128, 128, 128, 0}, {128, 128, 128, 0}, {128, 128, 128, 0}, {128, 128, 128, 0},
};

static MINIMAP_FIXED_PACKETS fixed_packets;

typedef char MINIMAP_FIXED_PACKET_LAYOUT_CHECK[sizeof(LINE_F2) == 16 && sizeof(POLY_F4) == 24 && sizeof(MINIMAP_FIXED_PACKETS) == 3248 ? 1 : -1];

typedef struct MINIMAP_ROUTE
{
    uint16 first_node;
    uint16 last_node;
    uint16 alternate_first;
} MINIMAP_ROUTE;

static MINIMAP_ROUTE *minimap_routes;
static uint32 primary_route_count;
static uint32 route_count;

typedef char MINIMAP_ROAD_PACKET_LAYOUT_CHECK[sizeof(POLY_F4) == 24 ? 1 : -1];

static MINIMAP_POINT project_minimap_point(const uint8 *seg, const uint8 *vertex, uint32 origin_x, uint32 origin_z, uint32 c, uint32 s, uint32 scale_x, uint32 scale_z)
{
    uint32 wire_word_1, wire_word_2;
    MINIMAP_POINT point;
    uint32 x;
    uint32 z;
    uint32 val;
    uint32 component;

    component = (sint16)xport_load_le16(vertex);
    x = (memcpy(&wire_word_1, seg + 12, sizeof(wire_word_1)), wire_word_1) + component + origin_x;
    component = (sint16)xport_load_le16(vertex + 4);
    z = (memcpy(&wire_word_2, seg + 20, sizeof(wire_word_2)), wire_word_2) + component + origin_z;
    val = x * c + z * s;
    val = (uint32)((sint32)val / 4096) * scale_x;
    point.x = (uint32)((sint32)val / 65536) + 50;
    val = (0 - x) * s + z * c;
    val = (uint32)((sint32)val / 4096) * scale_z;
    point.y = (uint32)((sint32)val / 65536) + 32;
    return point;
}

uint32 minimap_init(void)
{
    DRAWENV environment;
    uint32 result;
    uint32 buffer;
    uint32 idx;
    LINE_F2 *line;
    POLY_F4 *poly;
    LINE_F2 *boundary_line;
    MINIMAP_ROUTE *routes;
    POLY_F4 *road_poly;
    uint32 code;
    uint32 val;
    uint32 other;
    uint32 link;
    uint32 count;
    const uint8 *base;
    const uint8 *rec;
    uint32 start = 0;
    uint32 total = 0;
    uint32 original_total;
    uint32 dst;
    const uint8 *src_link;

    FUNCTION_MARKER(0x80052348, "SLUS_006.10");
    result = race_session.linear_course;
    if (result != 0)
        return result;
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&fixed_packets, (uint32)sizeof(fixed_packets)));
        XASSERT(operation_ok);
    }
    tex_find("Scandot1");
    for (buffer = 0; buffer < 2; ++buffer)
    {
        road_poly = fixed_packets.road[buffer];
        for (idx = 0; idx < 48; ++idx)
        {
            setlen(road_poly, 5);
            road_poly->rgbc.code = (uint8)(0x2A);
            road_poly->rgbc.r = (uint8)(88);
            road_poly->rgbc.g = (uint8)(88);
            road_poly->rgbc.b = (uint8)(88);
            ++road_poly;
        }
        line = &fixed_packets.border[0][buffer];
        setLineF2(line);
        line->rgbc.r = (uint8)(128);
        line->rgbc.g = (uint8)(128);
        line->rgbc.b = (uint8)(0);
        line->p[0].x = (sint16)(uint16)(29);
        line->p[0].y = (sint16)(uint16)(159);
        line->p[1].x = (sint16)(uint16)(131);
        line->p[1].y = (sint16)(uint16)(159);
        line = &fixed_packets.border[1][buffer];
        setLineF2(line);
        line->rgbc.r = (uint8)(128);
        line->rgbc.g = (uint8)(128);
        line->rgbc.b = (uint8)(0);
        line->p[0].x = (sint16)(uint16)(131);
        line->p[0].y = (sint16)(uint16)(159);
        line->p[1].x = (sint16)(uint16)(131);
        line->p[1].y = (sint16)(uint16)(225);
        line = &fixed_packets.border[2][buffer];
        setLineF2(line);
        line->rgbc.r = (uint8)(128);
        line->rgbc.g = (uint8)(128);
        line->rgbc.b = (uint8)(0);
        line->p[0].x = (sint16)(uint16)(29);
        line->p[0].y = (sint16)(uint16)(225);
        line->p[1].x = (sint16)(uint16)(131);
        line->p[1].y = (sint16)(uint16)(225);
        line = &fixed_packets.border[3][buffer];
        setLineF2(line);
        line->rgbc.r = (uint8)(128);
        line->rgbc.g = (uint8)(128);
        line->rgbc.b = (uint8)(0);
        line->p[0].x = (sint16)(uint16)(29);
        line->p[0].y = (sint16)(uint16)(159);
        line->p[1].x = (sint16)(uint16)(29);
        line->p[1].y = (sint16)(uint16)(225);
        poly = &fixed_packets.background[buffer];
        poly->rgbc.code = (uint8)(0x2A);
        setlen(poly, 5);
        poly->rgbc.r = (uint8)(0);
        poly->rgbc.g = (uint8)(0);
        poly->rgbc.b = (uint8)(0);
        poly->p[0].x = (sint16)(uint16)(0);
        poly->p[0].y = (sint16)(uint16)(0);
        poly->p[1].x = (sint16)(uint16)(100);
        poly->p[1].y = (sint16)(uint16)(0);
        poly->p[2].x = (sint16)(uint16)(0);
        poly->p[2].y = (sint16)(uint16)(64);
        poly->p[3].x = (sint16)(uint16)(100);
        poly->p[3].y = (sint16)(uint16)(64);
        SetDefDrawEnv(&environment, 30, 168 + buffer * 256, 100, 64);
        environment.tpage = 0;
        SetDrawEnv(&fixed_packets.draw_environment[buffer], &environment);
        boundary_line = &fixed_packets.boundary[buffer][0];
        setlen(boundary_line, 3);
        boundary_line->rgbc.code = (uint8)(0x42);
        boundary_line->rgbc.r = (uint8)(112);
        boundary_line->rgbc.g = (uint8)(112);
        boundary_line->rgbc.b = (uint8)(112);
        ++boundary_line;
        setlen(boundary_line, 3);
        boundary_line->rgbc.code = (uint8)(0x42);
        boundary_line->rgbc.r = (uint8)(112);
        boundary_line->rgbc.g = (uint8)(112);
        boundary_line->rgbc.b = (uint8)(112);
        for (idx = 0; idx < 12; ++idx)
        {
            poly = &fixed_packets.cars[buffer][idx];
            setPolyF4(poly);
            poly->rgbc.r = (uint8)(car_colors[idx].r);
            poly->rgbc.g = (uint8)(car_colors[idx].g);
            code = poly->rgbc.code;
            val = car_colors[idx].b;
            poly->rgbc.code = (uint8)(code | 2);
            poly->rgbc.b = (uint8)(val);
        }
    }
    count = track_geometry.node_count;
    routes = (MINIMAP_ROUTE *)malloc(((size_t)count + 1) * 2 * sizeof(*routes));
    XASSERT(!(routes == NULL));
    free(minimap_routes);
    minimap_routes = routes;
    if ((sint32)count > 0)
    {
        base = track_geometry.node_bytes;
        rec = base;
        for (idx = 0; (sint32)idx < (sint32)count; ++idx)
        {
            val = rec[0];
            if (val == 8)
            {
                minimap_routes[total].first_node = (uint16)(start);
                start = idx + 1;
                minimap_routes[total].last_node = (uint16)(idx);
                minimap_routes[total].alternate_first = (uint16)(0xFFFF);
                ++total;
            }
            else if (val == 11)
            {
                src_link = base + start * 24 - 16;
                minimap_routes[total].first_node = (uint16)(start);
                start = idx;
                minimap_routes[total].last_node = (uint16)(idx - 1);
                val = xport_load_le16(src_link);
                minimap_routes[total].alternate_first = (uint16)(val);
                ++total;
            }
            rec += 24;
        }
    }
    val = track_geometry.main_node_count;
    result = total * 6;
    ++total;
    minimap_routes[result / 6].first_node = (uint16)(start);
    minimap_routes[result / 6].alternate_first = (uint16)(0xFFFF);
    primary_route_count = (uint32)(total);
    minimap_routes[result / 6].last_node = (uint16)(val - 2);
    if ((sint32)total > 0)
    {
        original_total = total;
        dst = total * 6;
        for (idx = 0; (sint32)idx < (sint32)original_total; ++idx)
        {
            link = (sint16)(minimap_routes[idx].alternate_first);
            if (link != 0xFFFFFFFF)
            {
                minimap_routes[dst / 6].first_node = (uint16)(link);
                val = minimap_routes[idx].last_node;
                other = minimap_routes[idx].first_node;
                link = minimap_routes[idx].alternate_first;
                minimap_routes[dst / 6].last_node = (uint16)(link + val - other);
                val = minimap_routes[idx].first_node;
                ++total;
                minimap_routes[dst / 6].alternate_first = (uint16)(val);
                dst += 6;
            }
            result = (sint32)(idx + 1) < (sint32)original_total;
        }
    }
    route_count = (uint32)(total);
    return result;
}

uint32 minimap_draw(CAR_STATE *car)
{
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 result;
    uint32 angle;
    uint32 c;
    uint32 s;
    uint32 scale_x;
    uint32 scale_z;
    uint32 origin_x;
    uint32 origin_z;
    uint32 val;
    uint32 component;
    uint32 buffer_idx;
    POLY_F4 *poly;
    POLY_F4 *car_poly;
    uint32 mode;
    uint32 car_offset;
    uint32 car_idx;
    uint32 x;
    uint32 z;

    uint32 data;
    LINE_F2 *boundary_line;
    uint32 idx;
    uint32 limit;
    uint32 row;
    uint32 row_offset;
    uint32 mapped_row;
    POLY_F4 *alternate_poly;
    uint32 special;
    uint32 special_offset;
    const uint8 *seg_base;
    const uint8 *seg;
    const uint8 *vertex_base;
    const uint8 *vertex;
    uint32 vertex_offset;
    uint32 type;
    uint32 lane;
    uint32 mapped_start;
    uint32 mapped_end;
    uint32 i;
    sint32 node;
    sint32 len;
    sint32 half_len;
    sint32 difference;
    sint32 quotient;
    sint32 end_node;
    sint32 alternate;
    sint32 row_start;
    MINIMAP_POINT v0;
    MINIMAP_POINT v1;
    MINIMAP_POINT v2;
    MINIMAP_POINT v3;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80050DE8, "SLUS_006.10");
    result = race_session.linear_course;
    if (result != 0)
        return result;
    angle = 2048 - (uint32)(car->orientation.y >> 8);
    c = (uint32)rcos((sint32)angle);
    s = (uint32)rsin((sint32)angle);
    val = model_render.x_scale;
    origin_x = car_states[0].pos.bits.x;
    origin_z = car_states[0].pos.bits.z;
    origin_x = ((sint32)(0 - origin_x) >> 8);
    origin_z = ((sint32)(0 - origin_z) >> 8);
    scale_x = ((sint32)(val * 56) / 4096);
    val = model_render.y_scale;
    scale_z = ((sint32)(val * 56) / 4096);
    buffer_idx = disp_state.bank;
    node = (sint16)car_states[0].track_location.n_node;
    val = race_control.car_count;
    car_poly = &fixed_packets.cars[buffer_idx][0];
    if ((sint32)val > 0)
    {
        mode = race_session.event_mode;
        cur_frame = disp_state.cur;
        len = (sint32)track_geometry.main_node_count;
        half_len = len / 2;
        car_offset = 0;
        car_idx = 0;
        do
        {
            if (mode != 1 || car_idx - 1 >= 5)
            {
                val = race_session.linear_course;
                component = (sint16)car_states[(car_offset) / 864u].track_location.n_node;
                difference = ((uint32)node - component);
                if (val != 0)
                {
                    val = (uint32)difference + (uint32)half_len;
                    quotient = fixed_divide_checked((sint32)val, len);
                    val -= (uint32)quotient * (uint32)len;
                    difference = (val - (uint32)half_len);
                }
                if ((uint32)difference + 72 < 145)
                {
                    x = (uint32)(car_states[(car_offset) / 864u].pos.x >> 8) + origin_x;
                    z = (uint32)(car_states[(car_offset) / 864u].pos.z >> 8) + origin_z;
                    component = (0 - x) * s + z * c;
                    val = x * c + z * s;
                    val = (uint32)((sint32)val / 4096) * scale_x;
                    x = ((sint32)val / 65536);
                    val = component;
                    val = (uint32)((sint32)val / 4096) * scale_z;
                    z = ((sint32)val / 65536);
                    car_poly->p[0].x = (sint16)(uint16)(x + 49);
                    car_poly->p[0].y = (sint16)(uint16)(z + 31);
                    car_poly->p[1].x = (sint16)(uint16)(x + 52);
                    car_poly->p[1].y = (sint16)(uint16)(z + 31);
                    car_poly->p[2].x = (sint16)(uint16)(x + 49);
                    car_poly->p[2].y = (sint16)(uint16)(z + 33);
                    car_poly->p[3].x = (sint16)(uint16)(x + 52);
                    car_poly->p[3].y = (sint16)(uint16)(z + 33);
                    disp_link_front_packet(cur_frame, &car_poly->tag);
                }
            }
            car_offset += 864;
            val = race_control.car_count;
            ++car_idx;
            ++car_poly;
        } while ((sint32)car_idx < (sint32)val);
    }
    if (node >= (sint32)track_geometry.main_node_count)
    {
        idx = primary_route_count;
        limit = route_count;
        if ((sint32)idx < (sint32)limit)
        {
            row = idx * 6;
            do
            {
                end_node = (sint16)(minimap_routes[row / 6].last_node);
                ++idx;
                if (end_node >= node)
                {
                    row_start = (sint16)(minimap_routes[row / 6].first_node);
                    alternate = (sint16)(minimap_routes[row / 6].alternate_first);
                    node = ((uint32)node - (uint32)row_start + (uint32)alternate);
                    break;
                }
                row += 6;
            } while ((sint32)idx < (sint32)limit);
        }
    }
    mode = race_session.linear_course;
    node = ((uint32)(node / 12) * 12 - 72);
    if (node < 0)
    {
        if (mode != 0)
            node = (sint32)((uint32)node + track_geometry.main_node_count);
        else
            node = 0;
    }
    level_desc = &scene_level_image.desc;
    data = level_desc->checkpoints_offset;
    special = 9999;
    idx = 0;
    XASSERT(!(data >= scene_level_image.size));
    if (scene_level_image.bytes[data] != 0)
    {
        do
        {
            data = level_desc->checkpoints_offset;
            XASSERT(!(scene_level_image.size < 6 || data > scene_level_image.size - 6 || idx > (scene_level_image.size - 6 - data) / 4));
            val = xport_load_le16(scene_level_image.bytes + data + idx * 4 + 4);
            if ((sint32)val >= node && (sint32)val < (sint32)((uint32)node + 144))
            {
                special = val;
                break;
            }
            level_desc = &scene_level_image.desc;
            data = level_desc->checkpoints_offset;
            XASSERT(!(data >= scene_level_image.size));
            val = scene_level_image.bytes[data];
            ++idx;
        } while ((sint32)idx < (sint32)val);
    }
    buffer_idx = disp_state.bank;
    poly = fixed_packets.road[buffer_idx];
    alternate_poly = poly + 24;
    row_offset = 0;
    mapped_row = 0;
    i = 0;
    do
    {
        if ((sint16)(minimap_routes[row_offset / 6].last_node) < node)
        {
            row = mapped_row * 6;
            do
            {
                row += 6;
                end_node = (sint16)(minimap_routes[row / 6].last_node);
                row_offset += 6;
                ++mapped_row;
            } while (end_node < node);
        }
        end_node = (sint16)(minimap_routes[row_offset / 6].last_node);
        val = (uint32)node + 5;
        if ((sint32)val >= end_node)
        {
            if (race_session.linear_course != 0)
            {
                len = (sint32)track_geometry.main_node_count;
                end_node = (sint32)val < len ? end_node : 0;
            }
        }
        else
            end_node = val;
        if (end_node >= (sint32)special)
        {
            end_node = special;
            seg_base = track_geometry.node_bytes;
            special_offset = special * 24;
            seg = seg_base + special_offset;
            idx = xport_load_le16(seg + 6);
            vertex_base = track_geometry.vertex_bytes;
            vertex = vertex_base + idx * 6;
            v0 = project_minimap_point(seg, vertex, origin_x, origin_z, c, s, scale_x, scale_z);
            type = seg[0];
            lane = seg[3];
            vertex_offset = track_boundary_offsets[type].vertex_offset1 + (lane & 15);
            v1 = project_minimap_point(seg, vertex + (sint32)(vertex_offset * 6), origin_x, origin_z, c, s, scale_x, scale_z);
            buffer_idx = disp_state.bank;
            cur_frame = disp_state.cur;
            special_offset = buffer_idx * 32;
            boundary_line = &fixed_packets.boundary[special_offset / 32][0];
            boundary_line->p[0].x = (sint16)(uint16)(v0.x);
            boundary_line->p[0].y = (sint16)(uint16)(v0.y);
            boundary_line->p[1].x = (sint16)(uint16)(v1.x);
            boundary_line->p[1].y = (sint16)(uint16)(v1.y);
            disp_link_front_packet(cur_frame, &boundary_line->tag);
            alternate = (sint16)(minimap_routes[row_offset / 6].alternate_first);
            if (alternate != -1)
            {
                row_start = (sint16)(minimap_routes[row_offset / 6].first_node);
                mapped_end = ((uint32)end_node + (uint32)alternate - (uint32)row_start) * 24;
                seg = seg_base + mapped_end;
                idx = xport_load_le16(seg + 6);
                vertex = vertex_base + idx * 6;
                v0 = project_minimap_point(seg, vertex, origin_x, origin_z, c, s, scale_x, scale_z);
                type = seg[0];
                lane = seg[3];
                vertex_offset = track_boundary_offsets[type].vertex_offset1 + (lane & 15);
                seg_base = track_geometry.node_bytes;
                seg = seg_base + mapped_end;
                v1 = project_minimap_point(seg, vertex + (sint32)(vertex_offset * 6), origin_x, origin_z, c, s, scale_x, scale_z);
                boundary_line = &fixed_packets.boundary[special_offset / 32][1];
                boundary_line->p[0].x = (sint16)(uint16)(v0.x);
                boundary_line->p[0].y = (sint16)(uint16)(v0.y);
                boundary_line->p[1].x = (sint16)(uint16)(v1.x);
                boundary_line->p[1].y = (sint16)(uint16)(v1.y);
                disp_link_front_packet(cur_frame, &boundary_line->tag);
            }
            special = 9999;
        }
        seg_base = track_geometry.node_bytes;
        seg = seg_base + (uint32)node * 24;
        idx = xport_load_le16(seg + 4);
        vertex_base = track_geometry.vertex_bytes;
        vertex = vertex_base + idx * 6;
        v0 = project_minimap_point(seg, vertex, origin_x, origin_z, c, s, scale_x, scale_z);
        type = seg[0];
        lane = seg[3];
        vertex_offset = track_boundary_offsets[type].vertex_offset0 + (lane & 15);
        v1 = project_minimap_point(seg, vertex + (sint32)(vertex_offset * 6), origin_x, origin_z, c, s, scale_x, scale_z);
        mapped_end = (uint32)end_node * 24;
        seg = seg_base + mapped_end;
        idx = xport_load_le16(seg + 6);
        vertex = vertex_base + idx * 6;
        v2 = project_minimap_point(seg, vertex, origin_x, origin_z, c, s, scale_x, scale_z);
        type = seg[0];
        lane = seg[3];
        vertex_offset = track_boundary_offsets[type].vertex_offset1 + (lane & 15);
        seg_base = track_geometry.node_bytes;
        seg = seg_base + mapped_end;
        v3 = project_minimap_point(seg, vertex + (sint32)(vertex_offset * 6), origin_x, origin_z, c, s, scale_x, scale_z);
        cur_frame = disp_state.cur;
        poly->p[0].x = (sint16)(uint16)(v0.x);
        poly->p[0].y = (sint16)(uint16)(v0.y);
        poly->p[1].x = (sint16)(uint16)(v1.x);
        poly->p[1].y = (sint16)(uint16)(v1.y);
        poly->p[2].x = (sint16)(uint16)(v2.x);
        poly->p[2].y = (sint16)(uint16)(v2.y);
        poly->p[3].x = (sint16)(uint16)(v3.x);
        poly->p[3].y = (sint16)(uint16)(v3.y);
        disp_link_front_packet(cur_frame, &poly->tag);
        alternate = (sint16)(minimap_routes[row_offset / 6].alternate_first);
        if (alternate != -1)
        {
            row_start = (sint16)(minimap_routes[row_offset / 6].first_node);
            mapped_start = ((uint32)node + (uint32)alternate - (uint32)row_start) * 24;
            seg = seg_base + mapped_start;
            idx = xport_load_le16(seg + 4);
            vertex_base = track_geometry.vertex_bytes;
            vertex = vertex_base + idx * 6;
            v0 = project_minimap_point(seg, vertex, origin_x, origin_z, c, s, scale_x, scale_z);
            type = seg[0];
            lane = seg[3];
            vertex_offset = track_boundary_offsets[type].vertex_offset0 + (lane & 15);
            mapped_end = ((uint32)end_node + (uint32)alternate - (uint32)row_start) * 24;
            v1 = project_minimap_point(seg, vertex + (sint32)(vertex_offset * 6), origin_x, origin_z, c, s, scale_x, scale_z);
            seg_base = track_geometry.node_bytes;
            seg = seg_base + mapped_end;
            idx = xport_load_le16(seg + 6);
            vertex = vertex_base + idx * 6;
            v2 = project_minimap_point(seg, vertex, origin_x, origin_z, c, s, scale_x, scale_z);
            type = seg[0];
            lane = seg[3];
            vertex_offset = track_boundary_offsets[type].vertex_offset1 + (lane & 15);
            v3 = project_minimap_point(seg, vertex + (sint32)(vertex_offset * 6), origin_x, origin_z, c, s, scale_x, scale_z);
            cur_frame = disp_state.cur;
            alternate_poly->p[0].x = (sint16)(uint16)(v0.x);
            alternate_poly->p[0].y = (sint16)(uint16)(v0.y);
            alternate_poly->p[1].x = (sint16)(uint16)(v1.x);
            alternate_poly->p[1].y = (sint16)(uint16)(v1.y);
            alternate_poly->p[2].x = (sint16)(uint16)(v2.x);
            alternate_poly->p[2].y = (sint16)(uint16)(v2.y);
            alternate_poly->p[3].x = (sint16)(uint16)(v3.x);
            alternate_poly->p[3].y = (sint16)(uint16)(v3.y);
            disp_link_front_packet(cur_frame, &alternate_poly->tag);
        }
        val = track_geometry.main_node_count;
        node = ((uint32)end_node + 1);
        if (node >= (sint32)(val - 2))
        {
            if (race_session.linear_course == 0)
                break;
            node = 0;
        }
        ++i;
        ++alternate_poly;
        ++poly;
    } while (i < 24);
    buffer_idx = disp_state.bank;
    cur_frame = disp_state.cur;
    disp_link_front_packet(cur_frame, &fixed_packets.background[buffer_idx].tag);
    disp_link_front_packet(cur_frame, &fixed_packets.draw_environment[buffer_idx].tag);
    disp_link_front_packet(cur_frame, &fixed_packets.border[0][buffer_idx].tag);
    disp_link_front_packet(cur_frame, &fixed_packets.border[1][buffer_idx].tag);
    disp_link_front_packet(cur_frame, &fixed_packets.border[2][buffer_idx].tag);
    return disp_link_front_packet(cur_frame, &fixed_packets.border[3][buffer_idx].tag);
}
