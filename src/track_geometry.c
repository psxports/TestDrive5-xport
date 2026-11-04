#include <string.h>

#include <stdlib.h>

#include "game.h"

TRACK_GEOMETRY_STATE track_geometry;

const TRACK_BOUNDARY_OFFSETS track_boundary_offsets[12] = {{0x00000000, 0x00000000}, {0x00000000, 0x00000000}, {0xFFFFFFFF, 0x00000000}, {0xFFFFFFFF, 0x00000000}, {0xFFFFFFFE, 0x00000000}, {0x00000000, 0xFFFFFFFF}, {0x00000000, 0xFFFFFFFF}, {0x00000000, 0xFFFFFFFE}, {0x00000000, 0x00000000}, {0x00000000, 0x00000000}, {0x00000000, 0x00000000}, {0x00000000, 0x00000000}};

typedef struct TRACK_CELL_RULE
{
    uint8 left_mask;
    uint8 right_mask;
    sint8 left_offset;
    sint8 right_offset;
} TRACK_CELL_RULE;

static const TRACK_CELL_RULE cell_rules[12] = {{0, 0, 0, 0}, {14, 11, 0, 0}, {14, 3, 0, 0}, {6, 11, -1, 0}, {6, 3, -1, 0}, {14, 9, 0, 0}, {12, 11, 0, -1}, {12, 9, 0, -1}, {14, 11, 0, 0}, {14, 11, 0, 0}, {14, 11, 0, 0}, {14, 11, 0, 0}};

typedef struct TRACK_CELL_VIEW
{
    uint32 type;
    uint32 lanes;
    uint32 left;
    uint32 right;
    VECTOR relative;
} TRACK_CELL_VIEW;

typedef struct TRACK_TRANSITION
{
    uint32 node;
    uint32 lane;
} TRACK_TRANSITION;

typedef struct TRACK_EDGE_VIEW
{
    const uint8 *points;
    const uint8 *start;
    const uint8 *end;
    sint32 start_x;
    sint32 start_z;
    sint32 end_x;
    sint32 end_z;
    sint32 cross;
} TRACK_EDGE_VIEW;

static uint32 triangle_height(sint32 v0, sint32 v1, sint32 v2, const VECTOR *pos);
static const uint8 *inc_location_count(TRACK_LOCATION *node, sint32 reload_base);
static TRACK_TRANSITION next_location(uint32 node, uint32 type, uint32 lane, sint32 dir);
static TRACK_EDGE_VIEW read_edge(const uint8 *points, uint32 start, uint32 end, const VECTOR *relative);
static TRACK_CELL_VIEW read_cell(const uint8 *desc, const VEC3 *pos, sint32 read_lanes);
static uint32 classify_surface_quad(uint32 mask, const VECTOR *pos, const uint8 *points, uint32 v0, uint32 v1, uint32 v2, uint32 v3);
static uint32 triangle_n_height(sint32 v0, sint32 v1, sint32 v2, const VECTOR *pos, SVECTOR *output);
static uint32 triangle_n(sint32 v0, sint32 v1, sint32 v2, SVECTOR *output);

TRACK_POSITION_RESULT track_place_location(TRACK_LOCATION *node, VEC3 *pos)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;
    uint16 wire_halfword_13, wire_halfword_14;

    TRACK_POSITION_RESULT result;
    sint32 idx;
    const uint8 *desc;
    uint32 type;
    uint32 lanes;
    uint32 left_idx;
    uint32 right_idx;
    sint32 left_offset;
    sint32 right_offset;
    uint32 base;
    const uint8 *points;
    const uint8 *left_points;
    const uint8 *right_points;
    sint32 a;
    sint32 b;
    sint32 c;
    sint32 d;
    uint32 delta;

    FUNCTION_MARKER(0x8001DBC0, "SLUS_006.10");
    idx = (sint16)node->node;
    XASSERT(!(idx < 0 || (uint32)idx >= track_geometry.node_count));
    desc = track_geometry.node_bytes + (uint32)idx * 24;
    node->path_count = (uint16)(idx);
    node->max_path_count = (uint16)(idx);
    type = (desc)[0];
    lanes = (desc + 3)[0];
    left_idx = (memcpy(&wire_halfword_1, desc + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    right_idx = (memcpy(&wire_halfword_2, desc + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    left_offset = cell_rules[type].left_offset;
    right_offset = cell_rules[type].right_offset;
    result.lane = node->lane;
    lanes &= 15;
    left_idx += (uint32)left_offset;
    right_idx += (uint32)right_offset;
    if (result.lane >= (sint32)lanes)
    {
        result.lane = (sint32)lanes - 1;
        node->lane = (uint8)(result.lane);
    }
    memcpy(&base, desc + 12, sizeof(base));
    points = track_geometry.vertex_bytes;
    XASSERT(!(track_geometry.vertex_count < 2 || (uint32)(left_idx + (uint32)result.lane) >= track_geometry.vertex_count - 1 || (uint32)(right_idx + (uint32)result.lane) >= track_geometry.vertex_count - 1));
    left_points = points + (left_idx + (uint32)result.lane) * 6;
    right_points = points + (right_idx + (uint32)result.lane) * 6;
    a = (sint16)(memcpy(&wire_halfword_3, left_points, sizeof(wire_halfword_3)), wire_halfword_3);
    b = (sint16)(memcpy(&wire_halfword_4, left_points + 6, sizeof(wire_halfword_4)), wire_halfword_4);
    c = (sint16)(memcpy(&wire_halfword_5, right_points, sizeof(wire_halfword_5)), wire_halfword_5);
    d = (sint16)(memcpy(&wire_halfword_6, right_points + 6, sizeof(wire_halfword_6)), wire_halfword_6);
    delta = (uint32)(a + b + c + d) << 6;
    pos->bits.x = (base << 8) + delta;
    memcpy(&base, desc + 16, sizeof(base));
    a = (sint16)(memcpy(&wire_halfword_7, left_points + 2, sizeof(wire_halfword_7)), wire_halfword_7);
    b = (sint16)(memcpy(&wire_halfword_8, left_points + 8, sizeof(wire_halfword_8)), wire_halfword_8);
    result.right_vertex_y = (sint16)(memcpy(&wire_halfword_9, right_points + 2, sizeof(wire_halfword_9)), wire_halfword_9);
    d = (sint16)(memcpy(&wire_halfword_10, right_points + 8, sizeof(wire_halfword_10)), wire_halfword_10);
    delta = (uint32)(a + b + result.right_vertex_y + d) << 6;
    pos->bits.y = (base << 8) + delta;
    memcpy(&base, desc + 20, sizeof(base));
    a = (sint16)(memcpy(&wire_halfword_11, left_points + 4, sizeof(wire_halfword_11)), wire_halfword_11);
    b = (sint16)(memcpy(&wire_halfword_12, left_points + 10, sizeof(wire_halfword_12)), wire_halfword_12);
    c = (sint16)(memcpy(&wire_halfword_13, right_points + 4, sizeof(wire_halfword_13)), wire_halfword_13);
    d = (sint16)(memcpy(&wire_halfword_14, right_points + 10, sizeof(wire_halfword_14)), wire_halfword_14);
    result.z_delta = (uint32)(a + b + c + d) << 6;
    pos->bits.z = (base << 8) + result.z_delta;
    return result;
}

static uint32 triangle_height(sint32 v0, sint32 v1, sint32 v2, const VECTOR *pos)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;
    uint16 wire_halfword_13, wire_halfword_14, wire_halfword_15;

    VECTOR left = {0};
    VECTOR right = {0};
    VECTOR n = {0};
    const uint8 *points;
    uint32 v0_offset = (uint32)(sint16)v0 * 6;
    const uint8 *a;
    const uint8 *b;
    const uint8 *c;
    sint32 val0;
    sint32 val1;
    sint32 pos_val;
    sint32 n_x;
    sint32 n_y;
    sint32 n_z;
    uint32 dot;
    sint32 quotient;

    FUNCTION_MARKER(0x8001CB58, "SLUS_006.10");
    XASSERT(!((uint32)(sint16)v0 >= track_geometry.vertex_count || (uint32)(sint16)v1 >= track_geometry.vertex_count || (uint32)(sint16)v2 >= track_geometry.vertex_count));
    points = track_geometry.vertex_bytes;
    a = points + v0_offset;
    b = points + (uint32)(sint16)v1 * 6;
    val0 = (sint16)(memcpy(&wire_halfword_1, a, sizeof(wire_halfword_1)), wire_halfword_1);
    val1 = (sint16)(memcpy(&wire_halfword_2, b, sizeof(wire_halfword_2)), wire_halfword_2);
    left.vx = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_3, a + 2, sizeof(wire_halfword_3)), wire_halfword_3);
    val1 = (sint16)(memcpy(&wire_halfword_4, b + 2, sizeof(wire_halfword_4)), wire_halfword_4);
    left.vy = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_5, a + 4, sizeof(wire_halfword_5)), wire_halfword_5);
    val1 = (sint16)(memcpy(&wire_halfword_6, b + 4, sizeof(wire_halfword_6)), wire_halfword_6);
    left.vz = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_7, a, sizeof(wire_halfword_7)), wire_halfword_7);
    c = points + (uint32)(sint16)v2 * 6;
    val1 = (sint16)(memcpy(&wire_halfword_8, c, sizeof(wire_halfword_8)), wire_halfword_8);
    right.vx = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_9, a + 2, sizeof(wire_halfword_9)), wire_halfword_9);
    val1 = (sint16)(memcpy(&wire_halfword_10, c + 2, sizeof(wire_halfword_10)), wire_halfword_10);
    right.vy = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_11, a + 4, sizeof(wire_halfword_11)), wire_halfword_11);
    val1 = (sint16)(memcpy(&wire_halfword_12, c + 4, sizeof(wire_halfword_12)), wire_halfword_12);
    right.vz = val0 - val1;
    OuterProduct0(&left, &right, &n);
    n_x = n.vx >> 4;
    n_y = n.vy >> 4;
    n_z = n.vz >> 4;
    if (n_y == 0)
        n_y = 1;
    points = track_geometry.vertex_bytes;
    pos_val = pos->vx;
    a = points + v0_offset;
    val0 = (sint16)(memcpy(&wire_halfword_13, a, sizeof(wire_halfword_13)), wire_halfword_13);
    dot = ((uint32)val0 - (uint32)pos_val) * (uint32)n_x;
    val0 = (sint16)(memcpy(&wire_halfword_14, a + 4, sizeof(wire_halfword_14)), wire_halfword_14);
    pos_val = pos->vz;
    dot += ((uint32)val0 - (uint32)pos_val) * (uint32)n_z;
    quotient = fixed_divide_checked((sint32)dot, n_y);
    val0 = (sint16)(memcpy(&wire_halfword_15, a + 2, sizeof(wire_halfword_15)), wire_halfword_15);
    return ((uint32)quotient + (uint32)val0) << 8;
}

uint32 track_surface_height(TRACK_LOCATION *node, const VEC3 *pos, uint32 *height)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6;

    VECTOR relative = {0};
    sint32 node_idx;
    const uint8 *desc;
    uint32 type;
    uint32 left_idx;
    uint32 right_idx;
    sint32 offset;
    uint32 lane;
    uint32 lanes;
    uint32 val;
    uint32 base;
    sint32 action = 0;
    sint32 v0_idx;
    sint32 v1_idx;
    sint32 v2_idx;
    const uint8 *points;
    const uint8 *point0;
    const uint8 *point1;
    sint32 z0;
    sint32 x1;
    sint32 x0;
    sint32 z1;
    uint32 cross;
    uint32 result;

    FUNCTION_MARKER(0x8001CD00, "SLUS_006.10");
    node_idx = (sint16)node->node;
    XASSERT(!(node_idx < 0 || (uint32)node_idx >= track_geometry.node_count));
    desc = track_geometry.node_bytes + (uint32)node_idx * 24;
    type = (desc)[0];
    left_idx = (memcpy(&wire_halfword_1, desc + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    offset = cell_rules[type].left_offset;
    lane = node->lane;
    lanes = (desc + 3)[0];
    left_idx += (uint32)offset + lane;
    node->left_vertex = (uint16)(left_idx);
    right_idx = (memcpy(&wire_halfword_2, desc + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    offset = cell_rules[type].right_offset;
    right_idx += (uint32)offset + lane;
    node->right_vertex = (uint16)(right_idx);
    val = pos->bits.x;
    memcpy(&base, desc + 12, sizeof(base));
    relative.vx = (sint32)((uint32)((sint32)val >> 8) - base);
    val = pos->bits.y;
    memcpy(&base, desc + 16, sizeof(base));
    relative.vy = (sint32)((uint32)((sint32)val >> 8) - base);
    val = pos->bits.z;
    memcpy(&base, desc + 20, sizeof(base));
    relative.vz = (sint32)((uint32)((sint32)val >> 8) - base);
    lanes &= 15;
    if (lane == 0)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 3 || type == 4)
            action = 2;
        else if (type == 6 || type == 7)
            action = 1;
    }
    else if (lane == lanes - 1)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 2 || type == 4)
            action = -1;
        else if (type == 5 || type == 7)
            action = 3;
    }
    if (action == 0)
    {
        points = track_geometry.vertex_bytes;
        XASSERT(!(track_geometry.vertex_count < 2 || left_idx >= track_geometry.vertex_count || right_idx >= track_geometry.vertex_count - 1));
        point0 = points + left_idx * 6;
        point1 = points + right_idx * 6 + 6;
        z0 = (sint16)(memcpy(&wire_halfword_3, point0 + 4, sizeof(wire_halfword_3)), wire_halfword_3);
        x1 = (sint16)(memcpy(&wire_halfword_4, point1, sizeof(wire_halfword_4)), wire_halfword_4);
        x0 = (sint16)(memcpy(&wire_halfword_5, point0, sizeof(wire_halfword_5)), wire_halfword_5);
        z1 = (sint16)(memcpy(&wire_halfword_6, point1 + 4, sizeof(wire_halfword_6)), wire_halfword_6);
        cross = ((uint32)relative.vz - (uint32)z0) * (uint32)(x1 - x0);
        cross -= ((uint32)relative.vx - (uint32)x0) * (uint32)(z1 - z0);
        action = (sint32)cross > 0 ? 1 : -1;
    }
    v0_idx = (sint16)(left_idx + (action == 2 ? 1 : 0));
    v1_idx = (sint16)(right_idx + (action == 1 ? 1 : 0));
    v2_idx = (sint16)((action == 1 || action == 3 ? left_idx : right_idx) + 1);
    result = triangle_height(v0_idx, v1_idx, v2_idx, &relative);
    memcpy(&base, desc + 16, sizeof(base));
    result += base << 8;
    *height = result;
    return result;
}

sint32 track_render_surface_height(const TRACK_LOCATION *node, const VEC3 *pos, float *height)
{
    const uint8 *desc;
    VEC3 origin;
    VEC3_16 vertices[3];
    uint16 first, last;
    uint32 type, lanes, corner;
    sint32 left, right, action = 0, indices[3];
    float x, z, ax, ay, az, bx, by, bz, nx, ny, nz;
    XASSERT((sint16)node->node >= 0 && node->node < track_geometry.node_count);
    desc = track_geometry.node_bytes + (size_t)node->node * 24;
    type = desc[0];
    lanes = desc[3] & 15;
    if (type > 11)
        return 0;
    memcpy(&first, desc + 4, sizeof(first));
    memcpy(&last, desc + 6, sizeof(last));
    memcpy(&origin, desc + 12, sizeof(origin));
    left = first + cell_rules[type].left_offset + node->lane;
    right = last + cell_rules[type].right_offset + node->lane;
    x = pos->x / 256.0f - origin.x;
    z = pos->z / 256.0f - origin.z;
    if (node->lane == 0)
    {
        if (type == 0)
            return 0;
        if (type == 3 || type == 4)
            action = 2;
        else if (type == 6 || type == 7)
            action = 1;
    }
    else if (node->lane == lanes - 1)
    {
        if (type == 0)
            return 0;
        if (type == 2 || type == 4)
            action = -1;
        else if (type == 5 || type == 7)
            action = 3;
    }
    if (action == 0)
    {
        VEC3_16 a, b;
        XASSERT(left >= 0 && right >= 0 && (uint32)left < track_geometry.vertex_count && (uint32)(right + 1) < track_geometry.vertex_count);
        memcpy(&a, track_geometry.vertex_bytes + (size_t)left * sizeof(a), sizeof(a));
        memcpy(&b, track_geometry.vertex_bytes + (size_t)(right + 1) * sizeof(b), sizeof(b));
        action = (z - a.z) * ((float)b.x - a.x) - (x - a.x) * ((float)b.z - a.z) > 0.0f ? 1 : -1;
    }
    indices[0] = (sint16)(left + (action == 2 ? 1 : 0));
    indices[1] = (sint16)(right + (action == 1 ? 1 : 0));
    indices[2] = (sint16)((action == 1 || action == 3 ? left : right) + 1);
    for (corner = 0; corner < 3; ++corner)
    {
        XASSERT(indices[corner] >= 0 && (uint32)indices[corner] < track_geometry.vertex_count);
        memcpy(&vertices[corner], track_geometry.vertex_bytes + (size_t)indices[corner] * sizeof(vertices[corner]), sizeof(vertices[corner]));
    }
    ax = (float)vertices[0].x - vertices[1].x;
    ay = (float)vertices[0].y - vertices[1].y;
    az = (float)vertices[0].z - vertices[1].z;
    bx = (float)vertices[0].x - vertices[2].x;
    by = (float)vertices[0].y - vertices[2].y;
    bz = (float)vertices[0].z - vertices[2].z;
    nx = ay * bz - az * by;
    ny = az * bx - ax * bz;
    nz = ax * by - ay * bx;
    if (ny == 0.0f)
        ny = 16.0f;
    *height = origin.y + vertices[0].y + ((vertices[0].x - x) * nx + (vertices[0].z - z) * nz) / ny;
    return 1;
}

uint32 track_render_lane_triangles(uint32 node, uint32 lane, VEC3F output[6])
{
    if (node >= track_geometry.node_count)
        return 0;
    const uint8 *desc = track_geometry.node_bytes + (size_t)node * 24;
    uint32 type = desc[0], lanes = desc[3] & 15;
    uint16 first, last;
    VEC3 origin;
    if (type > 11 || lane >= lanes || (type == 0 && (lane == 0 || lane == lanes - 1)))
        return 0;
    memcpy(&first, desc + 4, 2);
    memcpy(&last, desc + 6, 2);
    memcpy(&origin, desc + 12, sizeof(origin));
    sint32 left = first + cell_rules[type].left_offset + (sint32)lane;
    sint32 right = last + cell_rules[type].right_offset + (sint32)lane;
    sint32 actions[2] = {1, -1};
    uint32 count = 2;
    if (lane == 0 && (type == 3 || type == 4 || type == 6 || type == 7))
    {
        actions[0] = type == 3 || type == 4 ? 2 : 1;
        count = 1;
    }
    else if (lane == lanes - 1 && (type == 2 || type == 4 || type == 5 || type == 7))
    {
        actions[0] = type == 2 || type == 4 ? -1 : 3;
        count = 1;
    }
    for (uint32 triangle = 0; triangle < count; ++triangle)
    {
        sint32 action = actions[triangle];
        sint32 indices[3] = {left + (action == 2), right + (action == 1), (action == 1 || action == 3 ? left : right) + 1};
        for (uint32 corner = 0; corner < 3; ++corner)
        {
            VEC3_16 point;
            if (indices[corner] < 0 || (uint32)indices[corner] >= track_geometry.vertex_count)
                return 0;
            memcpy(&point, track_geometry.vertex_bytes + (size_t)indices[corner] * sizeof(point), sizeof(point));
            output[triangle * 3 + corner] = (VEC3F){origin.x + (float)point.x, origin.y + (float)point.y, origin.z + (float)point.z};
        }
    }
    return count;
}

uint32 track_update_location(TRACK_LOCATION *node, const VEC3 *pos)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;

    TRACK_LOCATION *destination = node;
    TRACK_LOCATION candidate = *node;
    uint32 cur;
    const uint8 *base;
    uint32 lane;
    uint32 mask;
    const uint8 *points;
    uint32 left;
    uint32 right;
    uint32 classification;
    uint32 count;
    uint32 cross;
    sint32 prev_z;
    sint32 prev_x;
    sint32 next_x;
    sint32 next_z;
    TRACK_CELL_VIEW cell;
    TRACK_TRANSITION next;
    TRACK_EDGE_VIEW edge;
    TRACK_EDGE_VIEW adjacent;

    FUNCTION_MARKER(0x8001B310, "SLUS_006.10");
    node = &candidate;
    cur = (sint16)node->node;
    base = track_geometry.node_bytes;
    lane = node->lane;
    XASSERT(!(cur >= track_geometry.node_count));
    cell = read_cell(base + cur * 24, pos, 1);
    mask = 15;
    if (lane == 0)
        mask = cell_rules[cell.type].left_mask & 15;
    if (lane == cell.lanes - 1)
        mask &= cell_rules[cell.type].right_mask;
    points = track_geometry.vertex_bytes;
    right = cell.right + lane;
    left = cell.left + lane;
    classification = classify_surface_quad(mask, &cell.relative, points, left, left + 1, right + 1, right);
    if (classification == 1 || classification == 4)
    {
        mask = 15;
        if (classification == 1)
        {
            if ((sint32)(lane - 1) <= 0)
                mask = cell_rules[cell.type].left_mask;
        }
        else if ((sint32)(lane + 1) >= (sint32)(cell.lanes - 1))
            mask = cell_rules[cell.type].right_mask;
        if ((mask & 2) != 0)
        {
            points = track_geometry.vertex_bytes;
            right = cell.right + lane;
            edge = read_edge(points, right + (classification == 1 ? 0 : 2), right + (classification == 1 ? (uint32)-1 : 1), &cell.relative);
            if (edge.cross > 0)
            {
                next = next_location(cur, cell.type, lane, 1);
                lane = next.lane;
                inc_location_count(node, 0);
                node->node = (uint16)(next.node);
                node->lane = (uint8)(lane + (classification == 1 ? (uint32)-1 : 1));
                goto complete;
            }
        }
        if ((mask & 8) != 0)
        {
            points = track_geometry.vertex_bytes;
            left = cell.left + lane;
            edge = read_edge(points, left + (classification == 1 ? (uint32)-1 : 1), left + (classification == 1 ? 0 : 2), &cell.relative);
            if (edge.cross > 0)
            {
                next = next_location(cur, cell.type, lane, -1);
                lane = next.lane;
                count = node->path_count;
                node->node = (uint16)(next.node);
                node->path_count = (uint16)(count - 1);
            }
        }
        node->lane = (uint8)(lane + (classification == 1 ? (uint32)-1 : 1));
        goto complete;
    }
    if (classification != 2 && classification != 3 && classification != 6 && classification != 8 && classification != 9 && classification != 12)
        goto complete;
    next = next_location(cur, cell.type, lane, classification < 8 ? 1 : -1);
    lane = next.lane;
    if (classification < 8)
        base = inc_location_count(node, 1);
    else
    {
        count = node->path_count;
        node->path_count = (uint16)(count - 1);
        base = track_geometry.node_bytes;
    }
    cell = read_cell(base + next.node * 24, pos, classification == 3 || classification == 9 ? 0 : 1);
    if ((sint32)lane < 0 && (classification == 6 || classification == 12 || ((classification == 2 || classification == 8) && (cell_rules[cell.type].left_mask & 4) != 0)) && (cell.left + lane + 1 >= track_geometry.vertex_count || cell.right + lane + 1 >= track_geometry.vertex_count))
    {
        candidate = *destination;
        goto complete;
    }
    mask = 15;
    if (classification == 2 || classification == 8)
    {
        if ((sint32)lane <= 0)
            mask = cell_rules[cell.type].left_mask;
        if ((sint32)lane >= (sint32)(cell.lanes - 1))
            mask &= cell_rules[cell.type].right_mask;
        if ((mask & 1) != 0)
        {
            points = track_geometry.vertex_bytes;
            edge = read_edge(points, cell.right + lane, cell.left + lane, &cell.relative);
            if (edge.cross > 0)
            {
                node->lane = (uint8)(lane - 1);
                node->node = (uint16)(next.node);
                goto complete;
            }
        }
        if ((mask & 4) != 0)
        {
            points = track_geometry.vertex_bytes;
            edge = read_edge(points, cell.left + lane + 1, cell.right + lane + 1, &cell.relative);
            if (edge.cross > 0)
            {
                node->lane = (uint8)(lane + 1);
                node->node = (uint16)(next.node);
                goto complete;
            }
        }
        node->lane = (uint8)(lane);
        node->node = (uint16)(next.node);
        goto complete;
    }
    if (classification == 3 || classification == 9)
    {
        if ((sint32)lane <= 0)
            mask = cell_rules[cell.type].left_mask;
        if ((mask & 1) != 0)
        {
            points = track_geometry.vertex_bytes;
            edge = read_edge(points, cell.right + lane, cell.left + lane, &cell.relative);
            if (edge.cross > 0)
            {
                mask = 15;
                if ((sint32)(lane - 1) <= 0)
                    mask = cell_rules[cell.type].left_mask;
                if ((mask & (classification == 3 ? 8 : 2)) != 0)
                {
                    if (classification == 3)
                    {
                        XASSERT(!(edge.end == edge.points));
                        prev_z = (sint16)(memcpy(&wire_halfword_1, edge.end - 2, sizeof(wire_halfword_1)), wire_halfword_1);
                        prev_x = (sint16)(memcpy(&wire_halfword_2, edge.end - 6, sizeof(wire_halfword_2)), wire_halfword_2);
                        cross = ((uint32)cell.relative.vz - (uint32)prev_z) * (uint32)(edge.end_x - prev_x);
                        cross -= ((uint32)cell.relative.vx - (uint32)prev_x) * (uint32)(edge.end_z - prev_z);
                    }
                    else
                    {
                        XASSERT(!(edge.start == edge.points));
                        next_x = (sint16)(memcpy(&wire_halfword_3, edge.start - 6, sizeof(wire_halfword_3)), wire_halfword_3);
                        next_z = (sint16)(memcpy(&wire_halfword_4, edge.start - 2, sizeof(wire_halfword_4)), wire_halfword_4);
                        cross = ((uint32)cell.relative.vz - (uint32)edge.start_z) * (uint32)(next_x - edge.start_x);
                        cross -= ((uint32)cell.relative.vx - (uint32)edge.start_x) * (uint32)(next_z - edge.start_z);
                    }
                    if ((sint32)cross > 0)
                    {
                        node->lane = (uint8)(node->lane - 1);
                        goto complete;
                    }
                }
                if (classification == 3)
                {
                    node->node = (uint16)(next.node);
                    node->lane = (uint8)(lane - 1);
                }
                else
                {
                    node->lane = (uint8)(lane - 1);
                    node->node = (uint16)(next.node);
                }
                goto complete;
            }
        }
        if (classification == 3)
        {
            node->node = (uint16)(next.node);
            node->lane = (uint8)(lane);
        }
        else
        {
            node->lane = (uint8)(lane);
            node->node = (uint16)(next.node);
        }
        goto complete;
    }
    if ((sint32)lane >= (sint32)(cell.lanes - 1))
        mask = cell_rules[cell.type].right_mask;
    if ((mask & 4) != 0)
    {
        points = track_geometry.vertex_bytes;
        edge = read_edge(points, cell.left + lane + 1, cell.right + lane + 1, &cell.relative);
        if (edge.cross > 0)
        {
            mask = 15;
            if ((sint32)(lane + 1) >= (sint32)(cell.lanes - 1))
                mask = cell_rules[cell.type].right_mask;
            if ((mask & (classification == 6 ? 8 : 2)) != 0)
            {
                if (classification == 6)
                    adjacent = read_edge(edge.points, cell.left + lane + 1, cell.left + lane + 2, &cell.relative);
                else
                    adjacent = read_edge(edge.points, cell.right + lane + 2, cell.right + lane + 1, &cell.relative);
                if (adjacent.cross > 0)
                {
                    node->lane = (uint8)(node->lane + 1);
                    goto complete;
                }
            }
            node->lane = (uint8)(lane + 1);
            node->node = (uint16)(next.node);
            goto complete;
        }
    }
    node->lane = (uint8)(lane);
    node->node = (uint16)(next.node);
    goto complete;

complete:
    // Keep the current cell when the transition has no corresponding lane
    if (candidate.lane < (track_geometry.node_bytes[(uint32)candidate.node * 24 + 3] & 15))
        *destination = candidate;
    return classification;
}

static const uint8 *inc_location_count(TRACK_LOCATION *node, sint32 reload_base)
{
    uint32 count;
    uint32 maximum;
    sint32 signed_maximum;
    const uint8 *base = NULL;

    count = node->path_count + 1;
    maximum = node->max_path_count;
    signed_maximum = (sint16)node->max_path_count;
    node->path_count = (uint16)(count);
    if (signed_maximum < (sint16)count)
        maximum = count;
    if (reload_base != 0)
        base = track_geometry.node_bytes;
    node->max_path_count = (uint16)(maximum);
    return base;
}

static TRACK_TRANSITION next_location(uint32 node, uint32 type, uint32 lane, sint32 dir)
{
    uint16 wire_halfword_1, wire_halfword_2;

    TRACK_TRANSITION next;
    const uint8 *base;
    const uint8 *cur;
    uint32 target_lanes;
    uint32 cur_lanes;
    uint32 target_header;
    uint32 cur_header;
    uint32 link = dir > 0 ? 8 : 10;

    next.node = node + (uint32)dir;
    next.lane = lane;
    base = track_geometry.node_bytes;
    XASSERT(!(node >= track_geometry.node_count));
    cur = base + node * 24;
    if (type == (dir > 0 ? 10 : 9))
    {
        next.node = (uint32)(sint16)(memcpy(&wire_halfword_1, cur + link, sizeof(wire_halfword_1)), wire_halfword_1);
        XASSERT(!(next.node >= track_geometry.node_count));
        target_lanes = (base + next.node * 24 + 3)[0];
        cur_lanes = (cur + 3)[0];
        next.lane += (target_lanes & 15) - (cur_lanes & 15);
    }
    else if (type == (dir > 0 ? 8 : 11))
    {
        XASSERT(!((dir > 0 && node + 1 >= track_geometry.node_count) || (dir <= 0 && node == 0)));
        target_lanes = (cur + (dir > 0 ? 27 : -21))[0] & 15;
        if ((sint32)lane >= (sint32)target_lanes)
        {
            next.node = (uint32)(sint16)(memcpy(&wire_halfword_2, cur + link, sizeof(wire_halfword_2)), wire_halfword_2);
            XASSERT(!(next.node >= track_geometry.node_count));
            cur_lanes = (cur + 3)[0];
            target_lanes = (base + next.node * 24 + 3)[0];
            next.lane -= (cur_lanes & 15) - (target_lanes & 15);
        }
    }
    else
    {
        XASSERT(!(next.node >= track_geometry.node_count));
        memcpy(&target_header, base + next.node * 24, sizeof(target_header));
        memcpy(&cur_header, cur, sizeof(cur_header));
        next.lane -= (target_header >> 28) - (cur_header >> 28);
    }
    return next;
}

static TRACK_EDGE_VIEW read_edge(const uint8 *points, uint32 start, uint32 end, const VECTOR *relative)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;

    TRACK_EDGE_VIEW edge;
    uint32 cross;

    XASSERT(!(start >= track_geometry.vertex_count || end >= track_geometry.vertex_count));
    edge.points = points;
    edge.start = points + start * 6;
    edge.end = points + end * 6;
    edge.start_z = (sint16)(memcpy(&wire_halfword_1, edge.start + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    edge.end_x = (sint16)(memcpy(&wire_halfword_2, edge.end, sizeof(wire_halfword_2)), wire_halfword_2);
    edge.start_x = (sint16)(memcpy(&wire_halfword_3, edge.start, sizeof(wire_halfword_3)), wire_halfword_3);
    edge.end_z = (sint16)(memcpy(&wire_halfword_4, edge.end + 4, sizeof(wire_halfword_4)), wire_halfword_4);
    cross = ((uint32)relative->vz - (uint32)edge.start_z) * (uint32)(edge.end_x - edge.start_x);
    cross -= ((uint32)relative->vx - (uint32)edge.start_x) * (uint32)(edge.end_z - edge.start_z);
    edge.cross = (sint32)cross;
    return edge;
}

static TRACK_CELL_VIEW read_cell(const uint8 *desc, const VEC3 *pos, sint32 read_lanes)
{
    uint16 wire_halfword_1, wire_halfword_2;

    TRACK_CELL_VIEW cell = {0};
    uint32 val;
    uint32 base;
    sint32 left_offset;
    sint32 right_offset;

    cell.type = (desc)[0];
    if (read_lanes != 0)
        cell.lanes = (desc + 3)[0] & 15;
    cell.left = (memcpy(&wire_halfword_1, desc + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    cell.right = (memcpy(&wire_halfword_2, desc + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    val = pos->bits.x;
    memcpy(&base, desc + 12, sizeof(base));
    left_offset = cell_rules[cell.type].left_offset;
    right_offset = cell_rules[cell.type].right_offset;
    cell.relative.vx = (sint32)((uint32)((sint32)val >> 8) - base);
    val = pos->bits.y;
    memcpy(&base, desc + 16, sizeof(base));
    cell.relative.vy = (sint32)((uint32)((sint32)val >> 8) - base);
    val = pos->bits.z;
    memcpy(&base, desc + 20, sizeof(base));
    cell.left += (uint32)left_offset;
    cell.right += (uint32)right_offset;
    cell.relative.vz = (sint32)((uint32)((sint32)val >> 8) - base);
    return cell;
}

static uint32 classify_surface_quad(uint32 mask, const VECTOR *pos, const uint8 *points, uint32 v0, uint32 v1, uint32 v2, uint32 v3)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;

    uint32 starts[4];
    uint32 ends[4];
    uint32 idx;
    uint32 bit;
    sint32 start_z;
    sint32 end_x;
    sint32 start_x;
    sint32 end_z;
    uint32 z;
    uint32 x;
    uint32 cross;
    uint32 result = 0;

    FUNCTION_MARKER(0x8001B1B8, "SLUS_006.10");
    starts[0] = v3;
    starts[1] = v2;
    starts[2] = v1;
    starts[3] = v0;
    ends[0] = v0;
    ends[1] = v3;
    ends[2] = v2;
    ends[3] = v1;
    for (idx = 0; idx < 4; ++idx)
    {
        bit = 1 << idx;
        if ((mask & bit) != 0)
        {
            XASSERT(!(starts[idx] >= track_geometry.vertex_count || ends[idx] >= track_geometry.vertex_count));
            start_z = (sint16)(memcpy(&wire_halfword_1, points + starts[idx] * 6 + 4, sizeof(wire_halfword_1)), wire_halfword_1);
            z = (uint32)pos->vz;
            end_x = (sint16)(memcpy(&wire_halfword_2, points + ends[idx] * 6, sizeof(wire_halfword_2)), wire_halfword_2);
            start_x = (sint16)(memcpy(&wire_halfword_3, points + starts[idx] * 6, sizeof(wire_halfword_3)), wire_halfword_3);
            cross = (z - (uint32)start_z) * (uint32)(end_x - start_x);
            x = (uint32)pos->vx;
            end_z = (sint16)(memcpy(&wire_halfword_4, points + ends[idx] * 6 + 4, sizeof(wire_halfword_4)), wire_halfword_4);
            cross -= (x - (uint32)start_x) * (uint32)(end_z - start_z);
            if ((sint32)cross > 0)
                result |= bit;
        }
    }
    return result;
}

static uint32 triangle_n_height(sint32 v0, sint32 v1, sint32 v2, const VECTOR *pos, SVECTOR *output)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;
    uint16 wire_halfword_13, wire_halfword_14, wire_halfword_15;

    VECTOR left = {0};
    VECTOR right = {0};
    VECTOR n = {0};
    const uint8 *points;
    uint32 v0_offset = (uint32)(sint16)v0 * 6;
    const uint8 *a;
    const uint8 *b;
    const uint8 *c;
    sint32 val0;
    sint32 val1;
    sint32 pos_val;
    sint32 n_x;
    sint32 n_z;
    sint32 n_y;
    uint32 dot;
    sint32 quotient;

    FUNCTION_MARKER(0x8001D144, "SLUS_006.10");
    XASSERT(!((uint32)(sint16)v0 >= track_geometry.vertex_count || (uint32)(sint16)v1 >= track_geometry.vertex_count || (uint32)(sint16)v2 >= track_geometry.vertex_count));
    points = track_geometry.vertex_bytes;
    a = points + v0_offset;
    b = points + (uint32)(sint16)v1 * 6;
    val0 = (sint16)(memcpy(&wire_halfword_1, a, sizeof(wire_halfword_1)), wire_halfword_1);
    val1 = (sint16)(memcpy(&wire_halfword_2, b, sizeof(wire_halfword_2)), wire_halfword_2);
    left.vx = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_3, a + 2, sizeof(wire_halfword_3)), wire_halfword_3);
    val1 = (sint16)(memcpy(&wire_halfword_4, b + 2, sizeof(wire_halfword_4)), wire_halfword_4);
    left.vy = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_5, a + 4, sizeof(wire_halfword_5)), wire_halfword_5);
    val1 = (sint16)(memcpy(&wire_halfword_6, b + 4, sizeof(wire_halfword_6)), wire_halfword_6);
    left.vz = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_7, a, sizeof(wire_halfword_7)), wire_halfword_7);
    c = points + (uint32)(sint16)v2 * 6;
    val1 = (sint16)(memcpy(&wire_halfword_8, c, sizeof(wire_halfword_8)), wire_halfword_8);
    right.vx = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_9, a + 2, sizeof(wire_halfword_9)), wire_halfword_9);
    val1 = (sint16)(memcpy(&wire_halfword_10, c + 2, sizeof(wire_halfword_10)), wire_halfword_10);
    right.vy = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_11, a + 4, sizeof(wire_halfword_11)), wire_halfword_11);
    val1 = (sint16)(memcpy(&wire_halfword_12, c + 4, sizeof(wire_halfword_12)), wire_halfword_12);
    right.vz = val0 - val1;
    OuterProduct0(&left, &right, &n);
    n.vx >>= 12;
    n.vy >>= 12;
    n.vz >>= 12;
    VectorNormalS(&n, output);
    if (output->vy == 0)
        output->vy = 1;
    points = track_geometry.vertex_bytes;
    pos_val = pos->vx;
    a = points + v0_offset;
    val0 = (sint16)(memcpy(&wire_halfword_13, a, sizeof(wire_halfword_13)), wire_halfword_13);
    n_x = output->vx;
    dot = ((uint32)val0 - (uint32)pos_val) * (uint32)n_x;
    val0 = (sint16)(memcpy(&wire_halfword_14, a + 4, sizeof(wire_halfword_14)), wire_halfword_14);
    pos_val = pos->vz;
    n_z = output->vz;
    dot += ((uint32)val0 - (uint32)pos_val) * (uint32)n_z;
    n_y = output->vy;
    quotient = fixed_divide_checked((sint32)dot, n_y);
    val0 = (sint16)(memcpy(&wire_halfword_15, a + 2, sizeof(wire_halfword_15)), wire_halfword_15);
    return ((uint32)quotient + (uint32)val0) << 8;
}

uint32 track_surface_n_height(TRACK_LOCATION *node, const VEC3 *pos, SVECTOR *n, uint32 *height)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6;

    VECTOR relative = {0};
    sint32 node_idx;
    const uint8 *desc;
    uint32 type;
    uint32 left_idx;
    uint32 right_idx;
    sint32 offset;
    uint32 lane;
    uint32 lanes;
    uint32 val;
    uint32 base;
    sint32 action = 0;
    sint32 v0_idx;
    sint32 v1_idx;
    sint32 v2_idx;
    const uint8 *points;
    const uint8 *point0;
    const uint8 *point1;
    sint32 z0;
    sint32 x1;
    sint32 x0;
    sint32 z1;
    uint32 cross;
    uint32 result;

    FUNCTION_MARKER(0x8001D314, "SLUS_006.10");
    node_idx = (sint16)node->node;
    XASSERT(!(node_idx < 0 || (uint32)node_idx >= track_geometry.node_count));
    desc = track_geometry.node_bytes + (uint32)node_idx * 24;
    type = (desc)[0];
    left_idx = (memcpy(&wire_halfword_1, desc + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    offset = cell_rules[type].left_offset;
    lane = node->lane;
    lanes = (desc + 3)[0];
    left_idx += (uint32)offset + lane;
    node->left_vertex = (uint16)(left_idx);
    right_idx = (memcpy(&wire_halfword_2, desc + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    offset = cell_rules[type].right_offset;
    right_idx += (uint32)offset + lane;
    node->right_vertex = (uint16)(right_idx);
    val = pos->bits.x;
    memcpy(&base, desc + 12, sizeof(base));
    relative.vx = (sint32)((uint32)((sint32)val >> 8) - base);
    val = pos->bits.y;
    memcpy(&base, desc + 16, sizeof(base));
    relative.vy = (sint32)((uint32)((sint32)val >> 8) - base);
    val = pos->bits.z;
    memcpy(&base, desc + 20, sizeof(base));
    relative.vz = (sint32)((uint32)((sint32)val >> 8) - base);
    lanes &= 15;
    if (lane == 0)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 3 || type == 4)
            action = 2;
        else if (type == 6 || type == 7)
            action = 1;
    }
    else if (lane == lanes - 1)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 2 || type == 4)
            action = -1;
        else if (type == 5 || type == 7)
            action = 3;
    }
    if (action == 0)
    {
        points = track_geometry.vertex_bytes;
        XASSERT(!(track_geometry.vertex_count < 2 || left_idx >= track_geometry.vertex_count || right_idx >= track_geometry.vertex_count - 1));
        point0 = points + left_idx * 6;
        point1 = points + right_idx * 6 + 6;
        z0 = (sint16)(memcpy(&wire_halfword_3, point0 + 4, sizeof(wire_halfword_3)), wire_halfword_3);
        x1 = (sint16)(memcpy(&wire_halfword_4, point1, sizeof(wire_halfword_4)), wire_halfword_4);
        x0 = (sint16)(memcpy(&wire_halfword_5, point0, sizeof(wire_halfword_5)), wire_halfword_5);
        z1 = (sint16)(memcpy(&wire_halfword_6, point1 + 4, sizeof(wire_halfword_6)), wire_halfword_6);
        cross = ((uint32)relative.vz - (uint32)z0) * (uint32)(x1 - x0);
        cross -= ((uint32)relative.vx - (uint32)x0) * (uint32)(z1 - z0);
        action = (sint32)cross > 0 ? 1 : -1;
    }
    v0_idx = (sint16)(left_idx + (action == 2 ? 1 : 0));
    v1_idx = (sint16)(right_idx + (action == 1 ? 1 : 0));
    v2_idx = (sint16)((action == 1 || action == 3 ? left_idx : right_idx) + 1);
    result = triangle_n_height(v0_idx, v1_idx, v2_idx, &relative, n);
    memcpy(&base, desc + 16, sizeof(base));
    result += base << 8;
    *height = result;
    return result;
}

static uint32 triangle_n(sint32 v0, sint32 v1, sint32 v2, SVECTOR *output)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;

    VECTOR left = {0};
    VECTOR right = {0};
    VECTOR n = {0};
    const uint8 *points;
    const uint8 *a;
    const uint8 *b;
    const uint8 *c;
    sint32 val0;
    sint32 val1;

    FUNCTION_MARKER(0x8001D780, "SLUS_006.10");
    XASSERT(!((uint32)(sint16)v0 >= track_geometry.vertex_count || (uint32)(sint16)v1 >= track_geometry.vertex_count || (uint32)(sint16)v2 >= track_geometry.vertex_count));
    points = track_geometry.vertex_bytes;
    a = points + (uint32)(sint16)v0 * 6;
    b = points + (uint32)(sint16)v1 * 6;
    val0 = (sint16)(memcpy(&wire_halfword_1, a, sizeof(wire_halfword_1)), wire_halfword_1);
    val1 = (sint16)(memcpy(&wire_halfword_2, b, sizeof(wire_halfword_2)), wire_halfword_2);
    left.vx = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_3, a + 2, sizeof(wire_halfword_3)), wire_halfword_3);
    val1 = (sint16)(memcpy(&wire_halfword_4, b + 2, sizeof(wire_halfword_4)), wire_halfword_4);
    left.vy = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_5, a + 4, sizeof(wire_halfword_5)), wire_halfword_5);
    val1 = (sint16)(memcpy(&wire_halfword_6, b + 4, sizeof(wire_halfword_6)), wire_halfword_6);
    left.vz = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_7, a, sizeof(wire_halfword_7)), wire_halfword_7);
    c = points + (uint32)(sint16)v2 * 6;
    val1 = (sint16)(memcpy(&wire_halfword_8, c, sizeof(wire_halfword_8)), wire_halfword_8);
    right.vx = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_9, a + 2, sizeof(wire_halfword_9)), wire_halfword_9);
    val1 = (sint16)(memcpy(&wire_halfword_10, c + 2, sizeof(wire_halfword_10)), wire_halfword_10);
    right.vy = val0 - val1;
    val0 = (sint16)(memcpy(&wire_halfword_11, a + 4, sizeof(wire_halfword_11)), wire_halfword_11);
    val1 = (sint16)(memcpy(&wire_halfword_12, c + 4, sizeof(wire_halfword_12)), wire_halfword_12);
    right.vz = val0 - val1;
    OuterProduct0(&left, &right, &n);
    n.vx >>= 12;
    n.vy >>= 12;
    n.vz >>= 12;
    VectorNormalS(&n, output);
    if (output->vy == 0)
        output->vy = 1;
    return 1;
}

uint32 track_surface_n(TRACK_LOCATION *node, const VEC3 *pos, SVECTOR *output)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6;

    sint32 node_idx;
    const uint8 *desc;
    uint32 type;
    uint32 left_idx;
    uint32 right_idx;
    sint32 offset;
    uint32 lane;
    uint32 lanes;
    uint32 val;
    uint32 base;
    uint32 relative_x;
    uint32 relative_y;
    uint32 relative_z;
    sint32 action = 0;
    const uint8 *points;
    const uint8 *point0;
    const uint8 *point1;
    sint32 z0;
    sint32 x1;
    sint32 x0;
    sint32 z1;
    uint32 cross;

    FUNCTION_MARKER(0x8001D8BC, "SLUS_006.10");
    node_idx = (sint16)node->node;
    XASSERT(!(node_idx < 0 || (uint32)node_idx >= track_geometry.node_count));
    desc = track_geometry.node_bytes + (uint32)node_idx * 24;
    type = (desc)[0];
    left_idx = (memcpy(&wire_halfword_1, desc + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    offset = cell_rules[type].left_offset;
    lane = node->lane;
    lanes = (desc + 3)[0];
    left_idx += (uint32)offset + lane;
    node->left_vertex = (uint16)(left_idx);
    right_idx = (memcpy(&wire_halfword_2, desc + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    offset = cell_rules[type].right_offset;
    right_idx += (uint32)offset + lane;
    node->right_vertex = (uint16)(right_idx);
    val = pos->bits.x;
    memcpy(&base, desc + 12, sizeof(base));
    relative_x = (uint32)((sint32)val >> 8) - base;
    val = pos->bits.y;
    memcpy(&base, desc + 16, sizeof(base));
    relative_y = (uint32)((sint32)val >> 8) - base;
    val = pos->bits.z;
    memcpy(&base, desc + 20, sizeof(base));
    relative_z = (uint32)((sint32)val >> 8) - base;
    lanes &= 15;
    if (lane == 0)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 3 || type == 4)
            return triangle_n((sint16)(left_idx + 1), (sint16)right_idx, (sint16)(right_idx + 1), output);
        if (type == 6 || type == 7)
            action = 1;
    }
    else if (lane == lanes - 1)
    {
        if (type - 1 >= 11)
            return (type - 1) << 2;
        if (type == 2 || type == 4)
            action = -1;
        else if (type == 5 || type == 7)
            return triangle_n((sint16)left_idx, (sint16)right_idx, (sint16)(left_idx + 1), output);
    }
    if (action == 0)
    {
        points = track_geometry.vertex_bytes;
        XASSERT(!(track_geometry.vertex_count < 2 || left_idx >= track_geometry.vertex_count || right_idx >= track_geometry.vertex_count - 1));
        point0 = points + left_idx * 6;
        point1 = points + right_idx * 6 + 6;
        z0 = (sint16)(memcpy(&wire_halfword_3, point0 + 4, sizeof(wire_halfword_3)), wire_halfword_3);
        x1 = (sint16)(memcpy(&wire_halfword_4, point1, sizeof(wire_halfword_4)), wire_halfword_4);
        x0 = (sint16)(memcpy(&wire_halfword_5, point0, sizeof(wire_halfword_5)), wire_halfword_5);
        z1 = (sint16)(memcpy(&wire_halfword_6, point1 + 4, sizeof(wire_halfword_6)), wire_halfword_6);
        cross = (relative_z - (uint32)z0) * (uint32)(x1 - x0);
        cross -= (relative_x - (uint32)x0) * (uint32)(z1 - z0);
        action = (sint32)cross > 0 ? 1 : -1;
    }
    (void)relative_y;
    if (action > 0)
        return triangle_n((sint16)left_idx, (sint16)(right_idx + 1), (sint16)(left_idx + 1), output);
    return triangle_n((sint16)left_idx, (sint16)right_idx, (sint16)(right_idx + 1), output);
}

void track_geometry_init(void)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5;

    const SCENE_TRACK_IMAGE *image = &scene_track_image;
    const uint8 *wire;
    TRACK_BRANCH_MAP *branches = NULL;
    uint32 geometry_offset, branch_count, branch_idx;
    uint32 node_offset;
    uint32 vertex_offset;
    uint32 count;
    uint32 extra;
    uint32 last;

    FUNCTION_MARKER(0x8001B128, "SLUS_006.10");
    XASSERT(!(image->bytes == NULL || image->size < 8));
    memcpy(&geometry_offset, image->bytes + 4, sizeof(geometry_offset));
    XASSERT(!(image->size < 24 || geometry_offset > image->size - 24));
    wire = image->bytes + geometry_offset;
    memcpy(&node_offset, wire, sizeof(node_offset));
    memcpy(&vertex_offset, wire + 8, sizeof(vertex_offset));
    memcpy(&count, wire + 4, sizeof(count));
    memcpy(&extra, wire + 16, sizeof(extra));
    memcpy(&last, wire + 12, sizeof(last));
    XASSERT(!(count == 0 || count > extra || node_offset > image->size - geometry_offset || vertex_offset > image->size - geometry_offset || extra > (image->size - geometry_offset - node_offset) / 24 || last > (image->size - geometry_offset - vertex_offset) / 6));
    memcpy(&branch_count, wire + 20, sizeof(branch_count));
    XASSERT(!(node_offset < 24 || node_offset > image->size - geometry_offset || branch_count > (node_offset - 24) / 6));
    if (branch_count != 0)
    {
        branches = (TRACK_BRANCH_MAP *)malloc((size_t)branch_count * sizeof(*branches));
        XASSERT(!(branches == NULL));
        for (branch_idx = 0; branch_idx < branch_count; ++branch_idx)
        {
            wire = image->bytes + geometry_offset + 24 + branch_idx * 6;
            branches[branch_idx].branch_first = (memcpy(&wire_halfword_1, wire, sizeof(wire_halfword_1)), wire_halfword_1);
            branches[branch_idx].branch_last = (memcpy(&wire_halfword_2, wire + 2, sizeof(wire_halfword_2)), wire_halfword_2);
            branches[branch_idx].main_first = (memcpy(&wire_halfword_3, wire + 4, sizeof(wire_halfword_3)), wire_halfword_3);
        }
    }
    free(track_geometry.branches);
    track_geometry.branches = branches;
    track_geometry.branch_count = branch_count;
    track_geometry.vertex_count = last;
    track_geometry.node_bytes = image->bytes + geometry_offset + node_offset;
    track_geometry.vertex_bytes = image->bytes + geometry_offset + vertex_offset;
    track_geometry.main_node_count = count;
    track_geometry.node_count = extra;
    track_geometry.node_bytes[0] = 9;
    (wire_halfword_4 = (uint16)((uint16)(count - 1)), memcpy(track_geometry.node_bytes + 10, &wire_halfword_4, sizeof(wire_halfword_4)));
    (wire_halfword_5 = (uint16)(0), memcpy(track_geometry.node_bytes + track_geometry.main_node_count * 24 - 16, &wire_halfword_5, sizeof(wire_halfword_5)));
    track_geometry.node_bytes[track_geometry.main_node_count * 24 - 24] = 10;
}

uint32 track_map_branch_node(sint32 node)
{
    uint32 count;
    uint32 idx = 0;
    const TRACK_BRANCH_MAP *entry;
    uint32 start;
    uint32 end;
    uint32 src;

    FUNCTION_MARKER(0x80050704, "SLUS_006.10");
    count = track_geometry.branch_count;
    if (count == 0)
        return 0xFFFFFFFF;
    for (;;)
    {
        entry = track_geometry.branches + idx;
        start = entry->main_first;
        if (node >= (sint32)start)
        {
            end = entry->branch_last;
            src = entry->branch_first;
            if ((sint32)(start + end - src - 1) >= node)
                return (uint32)node - start + src;
        }
        count = track_geometry.branch_count;
        ++idx;
        if (idx >= count)
            return 0xFFFFFFFF;
    }
}

uint32 track_wrap_location(TRACK_LOCATION *location)
{
    sint32 node;
    sint32 count;
    sint32 quotient;
    uint32 remainder;

    FUNCTION_MARKER(0x8001AFFC, "SLUS_006.10");
    node = (sint16)location->path_count;
    count = (sint32)track_geometry.main_node_count;
    quotient = fixed_divide_checked(node, count);
    remainder = (uint32)node - (uint32)quotient * (uint32)count;
    if (node < 0 && remainder != 0)
        remainder += (uint32)count;
    location->n_node = (uint16)(remainder);
    return count;
}

uint32 track_normalize_location(TRACK_LOCATION *location)
{
    const TRACK_BRANCH_MAP *cursor;
    uint32 count;
    uint32 idx;
    uint32 lower;
    uint32 upper;
    uint32 val;
    sint32 node;

    FUNCTION_MARKER(0x8001B090, "SLUS_006.10");
    cursor = track_geometry.branches;
    node = (sint16)location->node;
    count = track_geometry.main_node_count;
    if ((uint32)node <= count)
    {
        location->n_node = (uint16)((uint32)node);
        location->path_count = (uint16)((uint32)node);
        return 0;
    }
    count = track_geometry.branch_count;
    if (count == 0)
        return 0;
    idx = 0;
    do
    {
        lower = cursor->branch_first;
        if (node >= (sint32)lower)
        {
            upper = cursor->branch_last;
            if ((sint32)upper >= node)
            {
                val = cursor->main_first;
                val += (uint32)node - lower;
                location->n_node = (uint16)(val);
                location->path_count = (uint16)(val);
                return val;
            }
        }
        ++idx;
        ++cursor;
    } while (idx < count);
    return 0;
}

uint32 track_lateral_fraction(sint32 node, const VEC3 *pos)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5;

    const uint8 *desc;
    uint32 idx;
    const uint8 *points;
    uint32 type;
    uint32 lanes;
    uint32 extra;
    const uint8 *point0;
    const uint8 *point1;
    sint32 x0;
    sint32 x1;
    sint32 z0;
    sint32 z1;
    sint32 dx;
    sint32 dz;
    uint32 base_x;
    uint32 base_z;
    sint32 x;
    sint32 z;
    sint32 distance;
    uint32 product;
    sint32 quotient;

    FUNCTION_MARKER(0x8004DA54, "SLUS_006.10");
    XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
    desc = track_geometry.node_bytes + (uint32)node * 24;
    idx = (memcpy(&wire_halfword_1, desc + 6, sizeof(wire_halfword_1)), wire_halfword_1);
    points = track_geometry.vertex_bytes;
    type = (desc)[0];
    lanes = (desc + 3)[0];
    extra = track_boundary_offsets[type].vertex_offset1;
    XASSERT(!(idx >= track_geometry.vertex_count));
    point0 = points + idx * 6;
    idx += lanes & 15;
    x0 = (sint16)(memcpy(&wire_halfword_2, point0, sizeof(wire_halfword_2)), wire_halfword_2);
    idx += extra;
    XASSERT(!(idx >= track_geometry.vertex_count));
    point1 = points + idx * 6;
    x1 = (sint16)(memcpy(&wire_halfword_3, point1, sizeof(wire_halfword_3)), wire_halfword_3);
    dx = x1 - x0;
    z1 = (sint16)(memcpy(&wire_halfword_4, point1 + 4, sizeof(wire_halfword_4)), wire_halfword_4);
    z0 = (sint16)(memcpy(&wire_halfword_5, point0 + 4, sizeof(wire_halfword_5)), wire_halfword_5);
    dz = z1 - z0;
    memcpy(&base_x, desc + 12, sizeof(base_x));
    x = pos->x;
    memcpy(&base_z, desc + 20, sizeof(base_z));
    z = pos->z;
    x = ((uint32)(x >> 8) - (base_x + (uint32)x0));
    z = ((uint32)(z >> 8) - (base_z + (uint32)z0));
    product = (uint32)dx * (uint32)dx + (uint32)dz * (uint32)dz;
    distance = (sint32)SquareRoot0((sint32)product);
    if (distance == 0)
        return 0;
    product = (uint32)x * (uint32)dx + (uint32)z * (uint32)dz;
    quotient = fixed_divide_checked((sint32)product, distance);
    return (uint32)fixed_divide_checked((sint32)((uint32)quotient << 8), distance);
}

uint32 track_surface_material(const TRACK_LOCATION *node)
{
    uint32 idx, lane, flags, type;
    const uint8 *desc;

    FUNCTION_MARKER(0x80025900, "SLUS_006.10");
    idx = (sint16)node->node;
    lane = node->lane;
    XASSERT(!(idx >= track_geometry.node_count));
    desc = track_geometry.node_bytes + idx * 24;
    flags = (desc + 2)[0];
    if (((flags >> (lane & 31)) & 1) != 0)
    {
        type = (desc + 1)[0];
        return (type >> 4) | 16;
    }
    type = (desc + 1)[0];
    return type & 15;
}

uint32 track_lateral_distance(sint32 node, uint32 val, uint32 kind)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5;

    const uint8 *desc;
    uint32 idx0;
    const uint8 *points;
    uint32 type;
    uint32 lanes;
    uint32 extra;
    uint32 idx1;
    const uint8 *point0;
    const uint8 *point1;
    sint32 a;
    sint32 b;
    uint32 difference = val - kind;
    sint32 x;
    sint32 z;
    uint32 result;

    FUNCTION_MARKER(0x8004DBE8, "SLUS_006.10");
    XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
    desc = track_geometry.node_bytes + (uint32)node * 24;
    idx0 = (memcpy(&wire_halfword_1, desc + 6, sizeof(wire_halfword_1)), wire_halfword_1);
    points = track_geometry.vertex_bytes;
    type = (desc)[0];
    lanes = (desc + 3)[0];
    extra = track_boundary_offsets[type].vertex_offset1;
    idx1 = idx0 + (lanes & 15) + extra;
    XASSERT(!(idx0 >= track_geometry.vertex_count || idx1 >= track_geometry.vertex_count));
    point0 = points + idx0 * 6;
    point1 = points + idx1 * 6;
    a = (sint16)(memcpy(&wire_halfword_2, point1, sizeof(wire_halfword_2)), wire_halfword_2);
    b = (sint16)(memcpy(&wire_halfword_3, point0, sizeof(wire_halfword_3)), wire_halfword_3);
    x = (sint32)((uint32)(a - b) * difference) / 256;
    a = (sint16)(memcpy(&wire_halfword_4, point1 + 4, sizeof(wire_halfword_4)), wire_halfword_4);
    b = (sint16)(memcpy(&wire_halfword_5, point0 + 4, sizeof(wire_halfword_5)), wire_halfword_5);
    z = (sint32)((uint32)(a - b) * difference) / 256;
    result = fixed_sqrt((uint32)x * (uint32)x + (uint32)z * (uint32)z);
    return (sint32)val < (sint32)kind ? 0 - result : result;
}

sint32 track_lane_material(sint32 node, uint32 lane)
{
    const uint8 *seg;
    uint32 flags;
    uint32 packed;

    FUNCTION_MARKER(0x8004FF8C, "SLUS_006.10");
    XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
    seg = track_geometry.node_bytes + (uint32)node * 24;
    flags = seg[2];
    if (((flags >> (lane & 31)) & 1) != 0)
    {
        packed = seg[1];
        return ((packed >> 4) | 16);
    }
    return seg[1] & 15;
}
