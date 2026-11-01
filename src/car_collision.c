#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"

typedef uint32 (*CAR_COLLISION_CALLBACK)(CAR_STATE *car);

typedef struct CAR_PAIR_CONTACT
{
    sint16 x;
    sint16 z;
    sint16 offset_x;
    sint16 offset_z;
} CAR_PAIR_CONTACT;

typedef struct CAR_COLLISION_BOUNDS
{
    uint32 first_node;
    uint32 last_node;
} CAR_COLLISION_BOUNDS;

typedef struct CAR_PAIR_BOX
{
    uint32 min_x;
    uint32 min_z;
    uint32 max_x;
    uint32 max_z;
    uint8 next;
} CAR_PAIR_BOX;

static CAR_PAIR_BOX pair_boxes[14];
static uint8 pair_heads[1024];

static const uint32 contact_inertia = 1500000u;
static const uint32 pair_inertia = 500000u;

typedef struct CAR_SURFACE_RESPONSE
{
    sint16 friction;
    sint16 drag;
} CAR_SURFACE_RESPONSE;

static const CAR_SURFACE_RESPONSE surface_responses[32] = {
    {256, 0}, {256, 0}, {256, 0}, {256, 0}, {256, 0}, {208, 2}, {224, 0}, {256, 0}, {256, 0}, {256, 0}, {224, 8}, {256, 0}, {256, 0}, {256, 0}, {256, 0}, {256, 0}, {224, 32}, {224, 32}, {224, 32}, {224, 32}, {224, 32}, {224, 16}, {224, 32}, {224, 32}, {224, 32}, {224, 32}, {224, 32}, {224, 32}, {224, 32}, {224, 32}, {224, 32}, {224, 32},
};

static uint32 s_table[2048];
static uint32 contact_heights[8];

static const uint32 horizontal_motion_frozen = 0;
static const uint8 ground_velocity_masks[16] = {1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0};

static uint32 find_ground_contacts(CAR_STATE *car);

static uint32 sin_q30(uint32 angle);
static uint32 cos_q30(uint32 angle);

CAR_PHYSICS_STATE car_physics = {1500, 0};

static const sint8 ground_contact_order[8] = {0, 1, 2, 3, -1, 0, 0, 0};
static const sint8 all_contact_order[8] = {0, 1, 2, 3, 4, 5, 6, 7};

static CAR_COLLISION_BOUNDS col_bounds;

static sint32 edge_angle(CAR_STATE *car, const uint8 *v0, const uint8 *v1);
static sint32 edge_distance(const VEC3 *point, const uint8 *v0, const uint8 *v1, const uint8 *seg, sint32 z1);
static sint32 road_distance(CAR_STATE *car, const uint8 *anchor, const uint8 *seg, uint32 extent, SVECTOR *n, uint32 *z_delta);
static uint32 boundary_contact(CAR_STATE *car, const uint8 *anchor, const uint8 *other, uint32 extent, SVECTOR *n);
static uint32 boundary_response(CAR_STATE *car, const uint8 *other, const uint8 *anchor, uint32 penetration);

static uint32 update_ground_contacts(CAR_STATE *car);
static uint32 mul_q30(uint32 left, uint32 right);
static uint32 pack_matrix_q30(MATRIX *dst, const uint32 src[9]);
static uint32 expand_matrix_q30(uint32 dst[9], const MATRIX *src);
static uint32 resolve_ground_impact(CAR_STATE *car, uint32 contact);
static uint32 update_rollover_state(CAR_STATE *car);
static uint32 fit_suspension_angles(SVECTOR *angles, const VEC3 *pos, const uint32 *suspension);
static uint32 fit_suspension_pitch(SVECTOR *angles, const VEC3 *pos, const uint32 *suspension);
static uint32 fit_suspension_roll(SVECTOR *angles, const VEC3 *pos, const uint32 *suspension);
static uint32 apply_contact_impact(CAR_STATE *car, const VEC3 *point, sint32 angle, sint32 distance);
static uint32 apply_contact_forces(CAR_STATE *car);
static uint32 update_airborne_motion(CAR_STATE *car);
static uint32 update_ai_motion(CAR_STATE *car);
static uint32 update_player_motion(CAR_STATE *car);
static uint32 apply_contact_gravity(CAR_STATE *car);
static uint32 update_damped_motion(CAR_STATE *car);

uint32 car_truncate_shift(uint32 val, uint32 bits)
{
    if ((sint32)val < 0)
        val += (1u << bits) - 1;
    return ((sint32)val >> bits);
}

static uint32 boundary_response(CAR_STATE *car, const uint8 *other, const uint8 *anchor, uint32 penetration)
{
    sint32 other_z;
    sint32 anchor_z;
    sint32 other_x;
    sint32 anchor_x;
    sint32 angle;
    uint32 c;
    uint32 s;
    uint32 correction;
    uint32 x_change;
    uint32 z_change;
    uint32 velocity_x;
    uint32 velocity_z;
    uint32 x;
    uint32 z;
    uint32 tangent;
    uint32 n;

    race_apply_point_penalty(car, 1);
    other_z = (sint16)xport_load_le16(other + 4);
    anchor_z = (sint16)xport_load_le16(anchor + 4);
    other_x = (sint16)xport_load_le16(other);
    anchor_x = (sint16)xport_load_le16(anchor);
    angle = fixed_angle_from_vector(other_z - anchor_z, other_x - anchor_x);
    c = (uint32)rcos(angle);
    s = (uint32)rsin(angle);
    correction = (uint32)((sint32)(penetration + 4095) >> 12) - 4;
    x_change = car_truncate_shift((0 - correction) * s, 4);
    z_change = car_truncate_shift(correction * c, 4);
    velocity_x = car->velocity.bits.x;
    velocity_z = car->velocity.bits.z;
    x = car->pos.bits.x;
    z = car->pos.bits.z;
    car->pos.bits.x = (uint32)(x + x_change);
    car->pos.bits.z = (uint32)(z + z_change);
    tangent = car_truncate_shift(velocity_x * c + velocity_z * s, 12);
    n = car_truncate_shift(velocity_z * c - velocity_x * s, 12);
    if ((sint32)n >= 0)
    {
        n = ((sint32)(0 - n) / 2);
        if ((sint32)tangent > 384)
            tangent -= 384;
        else if ((sint32)tangent < -384)
            tangent += 384;
        else
            tangent = 0;
        x = car_truncate_shift(tangent * c - n * s, 12);
        z = car_truncate_shift(n * c + tangent * s, 12);
        car->velocity.bits.x = (uint32)(x);
        car->velocity.bits.z = (uint32)(z);
        car->boundary_side = (uint8)(0);
    }
    return car_update_surface_alignment(car);
}

static uint32 boundary_contact(CAR_STATE *car, const uint8 *anchor, const uint8 *other, uint32 extent, SVECTOR *n)
{
    uint32 coord;
    uint32 origin;
    uint32 product;
    uint32 z_delta;
    uint32 penetration;

    VectorNormalSS(n, n);
    coord = car->pos.bits.x;
    origin = (sint16)xport_load_le16(anchor);
    coord = (uint32)((sint32)coord >> 8) - origin;
    product = coord * (uint32)(sint32)n->vx;
    coord = car->pos.bits.z;
    origin = (sint16)xport_load_le16(anchor + 4);
    z_delta = (uint32)((sint32)coord >> 8) - origin;
    penetration = product + z_delta * (uint32)(sint32)n->vz - extent;
    if ((sint32)penetration >= 0)
        return z_delta;
    return boundary_response(car, other, anchor, penetration);
}

static sint32 road_distance(CAR_STATE *car, const uint8 *anchor, const uint8 *seg, uint32 extent, SVECTOR *n, uint32 *z_delta)
{
    uint32 coord;
    uint32 vertex;
    uint32 origin;
    uint32 product;

    VectorNormalSS(n, n);
    coord = car->pos.bits.x;
    vertex = (sint16)xport_load_le16(anchor);
    memcpy(&origin, seg + 12, sizeof(origin));
    coord = (uint32)((sint32)coord >> 8) - vertex - origin;
    product = coord * (uint32)(sint32)n->vx;
    coord = car->pos.bits.z;
    vertex = (sint16)xport_load_le16(anchor + 4);
    memcpy(&origin, seg + 20, sizeof(origin));
    *z_delta = (uint32)((sint32)coord >> 8) - vertex - origin;
    return (product + *z_delta * (uint32)(sint32)n->vz - extent);
}

static uint32 resolve_road_boundary(uint32 slot)
{
    CAR_STATE *car;
    const uint8 *base;
    uint32 lane;
    const uint8 *seg;
    uint32 seg_offset;
    uint32 count;
    uint32 idx;
    uint32 angle;
    uint32 c;
    uint32 s;

    uint32 bound_x;
    uint32 bound_z;
    uint32 idx0;
    uint32 idx1;
    const uint8 *points;
    const uint8 *anchor;
    const uint8 *other;
    uint32 z0;
    uint32 z1;
    uint32 x0;
    uint32 x1;
    uint32 extent;
    uint32 type;
    uint32 adjust;
    uint32 z_delta;
    sint32 node;
    sint32 penetration;
    SVECTOR n;

    FUNCTION_MARKER(0x80029524, "SLUS_006.10");
    car = &car_states[slot];
    node = (sint16)car->track_location.node;
    base = track_geometry.node_bytes;
    lane = car->track_location.lane;
    XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
    seg_offset = (uint32)node * 24;
    seg = base + seg_offset;
    count = (seg + 3)[0] & 15;
    if (lane < 2)
    {
        idx = car->slot;
        angle = race_ai_heading_error(&race_ai_cars[idx]) & 0x7FF;
        if (angle >= 1024)
            angle = 2047 - angle;
        c = (uint32)rcos((sint32)angle);
        s = (uint32)rsin((sint32)angle);

        base = track_geometry.node_bytes;
        bound_x = (uint32)car_body_points[slot][1].vx;
        idx0 = xport_load_le16(base + seg_offset + 6);
        points = track_geometry.vertex_bytes;
        idx1 = xport_load_le16(base + seg_offset + 4);
        XASSERT(!((uint32)(idx0) >= track_geometry.vertex_count));
        anchor = points + idx0 * 6;
        XASSERT(!((uint32)(idx1) >= track_geometry.vertex_count));
        other = points + idx1 * 6;
        bound_z = (uint32)car_body_points[slot][1].vz;
        z0 = xport_load_le16(other + 4);
        z1 = xport_load_le16(anchor + 4);
        n.vy = 0;
        n.vx = (sint16)(z0 - z1);
        x0 = xport_load_le16(other);
        x1 = xport_load_le16(anchor);
        n.vz = (sint16)(x1 - x0);
        extent = c * bound_x + s * bound_z;
        penetration = road_distance(car, anchor, seg, extent, &n, &z_delta);
        if (penetration < 0)
            boundary_response(car, other, anchor, (uint32)penetration);
    }
    if ((sint32)lane < (sint32)(count - 2))
        return 1;
    idx = car->slot;
    base = track_geometry.node_bytes;
    seg = base + seg_offset;
    type = seg[0];
    angle = race_ai_heading_error(&race_ai_cars[idx]) & 0x7FF;
    if (angle >= 1024)
        angle = 2047 - angle;
    c = (uint32)rcos((sint32)angle);
    s = (uint32)rsin((sint32)angle);
    base = track_geometry.node_bytes;

    XASSERT(!(type >= 12));
    adjust = track_boundary_offsets[type].vertex_offset0;
    idx0 = xport_load_le16(base + seg_offset + 4);
    points = track_geometry.vertex_bytes;
    bound_x = (uint32)car_body_points[slot][1].vx;
    idx1 = xport_load_le16(base + seg_offset + 6);
    XASSERT(!((uint32)((idx0 + count + adjust)) >= track_geometry.vertex_count));
    anchor = points + (idx0 + count + adjust) * 6;
    adjust = track_boundary_offsets[type].vertex_offset1;
    z0 = xport_load_le16(anchor + 4);
    XASSERT(!((uint32)((idx1 + count + adjust)) >= track_geometry.vertex_count));
    other = points + (idx1 + count + adjust) * 6;
    bound_z = (uint32)car_body_points[slot][1].vz;
    z1 = xport_load_le16(other + 4);
    n.vy = 0;
    n.vx = (sint16)(z1 - z0);
    x0 = xport_load_le16(other);
    x1 = xport_load_le16(anchor);
    n.vz = (sint16)(x1 - x0);
    extent = c * bound_x + s * bound_z;
    penetration = road_distance(car, anchor, seg, extent, &n, &z_delta);
    if (penetration < 0)
        return boundary_response(car, other, anchor, (uint32)penetration);
    return z_delta;
}

static uint32 resolve_end_boundary(uint32 slot)
{
    CAR_STATE *car;
    uint32 idx;
    uint32 angle;
    uint32 s;
    uint32 c;

    uint32 body_x;
    uint32 body_z;
    uint32 extent;
    uint32 seg_idx;
    const uint8 *seg;
    const uint8 *base;
    uint32 count;
    uint32 vertex;
    const uint8 *low;
    const uint8 *high;
    uint32 low_z;
    uint32 high_z;
    uint32 high_x;
    uint32 low_x;
    sint32 node;
    SVECTOR n;

    FUNCTION_MARKER(0x80029EFC, "SLUS_006.10");
    car = &car_states[slot];
    idx = car->slot;
    angle = race_ai_heading_error(&race_ai_cars[idx]) & 0x7FF;
    if (angle >= 1024)
        angle = 2047 - angle;
    s = (uint32)rsin((sint32)angle);
    c = (uint32)rcos((sint32)angle);

    body_x = (uint32)car_body_points[slot][1].vx;
    body_z = (uint32)car_body_points[slot][1].vz;
    seg_idx = col_bounds.last_node;
    node = (sint16)car->track_location.node;
    extent = s * body_x + c * body_z;
    if ((uint32)node != seg_idx - 1)
        return seg_idx - 1;
    base = track_geometry.node_bytes;
    XASSERT(!((uint32)seg_idx >= track_geometry.node_count));
    seg = base + seg_idx * 24;
    count = (seg + 3)[0];
    vertex = xport_load_le16(seg + 4);
    base = track_geometry.vertex_bytes;
    XASSERT(!((uint32)((vertex + (count & 15))) >= track_geometry.vertex_count));
    high = base + (vertex + (count & 15)) * 6;
    XASSERT(!((uint32)(vertex) >= track_geometry.vertex_count));
    low = base + vertex * 6;
    low_z = xport_load_le16(low + 4);
    high_z = xport_load_le16(high + 4);
    n.vy = 0;
    n.vx = (sint16)(low_z - high_z);
    high_x = xport_load_le16(high);
    low_x = xport_load_le16(low);
    n.vz = (sint16)(high_x - low_x);
    return boundary_contact(car, high, low, extent, &n);
}

static uint32 resolve_start_boundary(uint32 slot)
{
    CAR_STATE *car;
    uint32 seg_idx;
    const uint8 *seg;
    uint32 idx;
    uint32 angle;
    uint32 s;
    uint32 c;
    const uint8 *base;

    uint32 count;
    uint32 body_x;
    uint32 body_z;
    uint32 vertex;
    uint32 extent;
    const uint8 *low;
    const uint8 *high;
    uint32 high_z;
    uint32 low_z;
    uint32 high_x;
    uint32 low_x;
    sint32 node;
    SVECTOR n;

    FUNCTION_MARKER(0x80029BB8, "SLUS_006.10");
    car = &car_states[slot];
    seg_idx = col_bounds.first_node;
    node = (sint16)car->track_location.node;
    if ((uint32)node != seg_idx + 1)
        return seg_idx + 1;
    idx = car->slot;
    angle = race_ai_heading_error(&race_ai_cars[idx]) & 0x7FF;
    if (angle >= 1024)
        angle = 2047 - angle;
    s = (uint32)rsin((sint32)angle);
    c = (uint32)rcos((sint32)angle);
    seg_idx = col_bounds.first_node;
    base = track_geometry.node_bytes;
    XASSERT(!((uint32)seg_idx >= track_geometry.node_count));
    seg = base + seg_idx * 24;

    count = (seg + 3)[0];
    body_x = (uint32)car_body_points[slot][1].vx;
    vertex = xport_load_le16(seg + 4);
    body_z = (uint32)car_body_points[slot][1].vz;
    base = track_geometry.vertex_bytes;
    extent = s * body_x + c * body_z;
    XASSERT(!((uint32)(vertex) >= track_geometry.vertex_count));
    low = base + vertex * 6;
    XASSERT(!((uint32)((vertex + (count & 15))) >= track_geometry.vertex_count));
    high = base + (vertex + (count & 15)) * 6;
    high_z = xport_load_le16(high + 4);
    low_z = xport_load_le16(low + 4);
    n.vy = 0;
    n.vx = (sint16)(high_z - low_z);
    high_x = xport_load_le16(high);
    low_x = xport_load_le16(low);
    n.vz = (sint16)(low_x - high_x);
    return boundary_contact(car, low, high, extent, &n);
}

static sint32 edge_distance(const VEC3 *point, const uint8 *v0, const uint8 *v1, const uint8 *seg, sint32 z1)
{
    uint32 wire_word_3, wire_word_4;
    VECTOR n;
    sint32 z0;
    sint32 x0;
    sint32 x1;
    uint32 coord;
    uint32 origin;
    uint32 product;

    z0 = (sint16)xport_load_le16(v0 + 4);
    n.vy = 0;
    n.vx = z1 - z0;
    x0 = (sint16)xport_load_le16(v0);
    x1 = (sint16)xport_load_le16(v1);
    n.vz = x0 - x1;
    VectorNormal(&n, &n);
    coord = ((sint32)point->bits.x >> 8);
    origin = (sint16)xport_load_le16(v0);
    coord -= origin;
    coord -= (memcpy(&wire_word_3, seg + 12, sizeof(wire_word_3)), wire_word_3);
    product = coord * (uint32)n.vx;
    coord = ((sint32)point->bits.z >> 8);
    origin = (sint16)xport_load_le16(v0 + 4);
    coord -= origin;
    coord -= (memcpy(&wire_word_4, seg + 20, sizeof(wire_word_4)), wire_word_4);
    return (product + coord * (uint32)n.vz);
}

static sint32 edge_angle(CAR_STATE *car, const uint8 *v0, const uint8 *v1)
{
    sint32 z1;
    sint32 z0;
    sint32 x1;
    sint32 x0;

    race_apply_point_penalty(car, 1);
    z1 = (sint16)xport_load_le16(v1 + 4);
    z0 = (sint16)xport_load_le16(v0 + 4);
    x1 = (sint16)xport_load_le16(v1);
    x0 = (sint16)xport_load_le16(v0);
    return fixed_angle_from_vector(z1 - z0, x1 - x0);
}

static uint32 resolve_end_contacts(uint32 slot, CAR_COLLISION_CALLBACK callback, const sint8 *list)
{
    CAR_STATE *car;
    uint32 result;
    uint32 idx;
    uint32 seg_idx;
    const uint8 *seg;
    uint32 count;
    uint32 vertex;
    const uint8 *base;
    const uint8 *v0;
    const uint8 *v1;
    const VEC3 *point;
    sint32 node;
    sint32 sel;
    sint32 z1;
    sint32 distance;
    sint32 angle;

    FUNCTION_MARKER(0x80029180, "SLUS_006.10");
    car = &car_states[slot];
    result = col_bounds.last_node;
    node = (sint16)car->track_location.node;
    result -= 1;
    if (node < (sint32)result)
        return result;
    for (idx = 0; idx < 8; ++idx)
    {
        sel = list[idx];
        if (sel < 0)
            return result;
        node = (sint16)car->track_contacts[idx].node;
        seg_idx = col_bounds.last_node;
        if ((uint32)node == seg_idx)
        {
            base = track_geometry.node_bytes;
            XASSERT(!((uint32)(uint32)node >= track_geometry.node_count));
            seg = base + (uint32)node * 24;
            count = (seg + 3)[0];
            vertex = xport_load_le16(seg + 4);
            base = track_geometry.vertex_bytes;
            XASSERT(!((uint32)((vertex + (count & 15))) >= track_geometry.vertex_count));
            v0 = base + (vertex + (count & 15)) * 6;
            XASSERT(!((uint32)(vertex) >= track_geometry.vertex_count));
            v1 = base + vertex * 6;
            z1 = (sint16)xport_load_le16(v1 + 4);
            point = &car->body_contact_pos[(uint32)sel];
            distance = edge_distance(point, v0, v1, seg, z1);
            if (distance < 0)
            {
                angle = edge_angle(car, v0, v1);
                apply_contact_impact(car, point, angle, (sint32)((uint32)distance + 4095) >> 12);
                callback(car);
            }
        }
        result = idx + 1 < 8;
    }
    return result;
}

static uint32 resolve_start_contacts(uint32 slot, CAR_COLLISION_CALLBACK callback, const sint8 *list)
{
    CAR_STATE *car;
    uint32 result;
    uint32 idx;
    const uint8 *seg;
    uint32 count;
    uint32 vertex;
    const uint8 *base;
    const uint8 *v0;
    const uint8 *v1;
    const VEC3 *point;
    uint32 boundary;
    sint32 node;
    sint32 sel;
    sint32 z1;
    sint32 distance;
    sint32 angle;

    FUNCTION_MARKER(0x80028F74, "SLUS_006.10");
    car = &car_states[slot];
    boundary = col_bounds.first_node;
    node = (sint16)car->track_location.node;
    result = (sint32)(boundary + 1) < node;
    if (result != 0)
        return result;
    for (idx = 0; idx < 8; ++idx)
    {
        sel = list[idx];
        if (sel < 0)
            return result;
        node = (sint16)car->track_contacts[idx].node;
        boundary = col_bounds.first_node;
        if ((sint32)boundary >= node)
        {
            base = track_geometry.node_bytes;
            XASSERT(!((uint32)boundary >= track_geometry.node_count));
            seg = base + boundary * 24;
            count = (seg + 3)[0];
            vertex = xport_load_le16(seg + 4);
            base = track_geometry.vertex_bytes;
            XASSERT(!((uint32)(vertex) >= track_geometry.vertex_count));
            v0 = base + vertex * 6;
            XASSERT(!((uint32)((vertex + (count & 15))) >= track_geometry.vertex_count));
            v1 = base + (vertex + (count & 15)) * 6;
            z1 = (sint16)xport_load_le16(v1 + 4);
            point = &car->body_contact_pos[(uint32)sel];
            distance = edge_distance(point, v0, v1, seg, z1);
            if (distance < 0)
            {
                angle = edge_angle(car, v0, v1);
                apply_contact_impact(car, point, angle, (sint32)((uint32)distance + 4095) >> 12);
                callback(car);
            }
        }
        result = idx + 1 < 8;
    }
    return result;
}

static intptr_t resolve_road_contacts(uint32 slot, CAR_COLLISION_CALLBACK callback, const sint8 *list)
{
    uint32 wire_word_5, wire_word_6;
    CAR_STATE *car;
    uint32 idx;
    const TRACK_LOCATION *contact;
    const uint8 *seg;
    uint32 count;
    uint32 lane;
    uint32 vertex;
    uint32 other_vertex;
    const uint8 *base;
    const uint8 *v0;
    const uint8 *v1;
    const VEC3 *point;
    uint32 type;
    uint32 offset;
    uint32 other_offset;
    uint32 side;
    sint32 sel;
    sint32 node;
    sint32 z1;
    sint32 distance;
    sint32 angle;

    FUNCTION_MARKER(0x80028BD0, "SLUS_006.10");
    car = &car_states[slot];
    car_states[slot].boundary_side = (uint8)(0);
    for (idx = 0; idx < 8; ++idx)
    {
        sel = list[idx];
        if (sel < 0)
            return (intptr_t)(list + idx);
        contact = &car->track_contacts[(uint32)sel];
        node = (sint16)contact->node;
        base = track_geometry.node_bytes;
        XASSERT(!((uint32)(uint32)node >= track_geometry.node_count));
        seg = base + (uint32)node * 24;
        count = (seg + 3)[0];
        lane = contact->lane;
        count &= 15;
        point = &car->body_contact_pos[(uint32)sel];
        if (lane == 0)
        {
            other_vertex = xport_load_le16(seg + 6);
            base = track_geometry.vertex_bytes;
            vertex = xport_load_le16(seg + 4);
            XASSERT(!((uint32)(other_vertex) >= track_geometry.vertex_count));
            v0 = base + other_vertex * 6;
            XASSERT(!((uint32)(vertex) >= track_geometry.vertex_count));
            v1 = base + vertex * 6;
            z1 = (sint16)xport_load_le16(v1 + 4);
            side = 1;
        }
        else
        {
            if ((sint32)lane < (sint32)(count - 1))
                continue;
            type = seg[0];
            vertex = xport_load_le16(seg + 4);
            XASSERT(!(type >= 12));
            offset = track_boundary_offsets[type].vertex_offset0;
            base = track_geometry.vertex_bytes;
            other_offset = track_boundary_offsets[type].vertex_offset1;
            XASSERT(!((uint32)((vertex + count + offset)) >= track_geometry.vertex_count));
            v0 = base + (vertex + count + offset) * 6;
            other_vertex = xport_load_le16(seg + 6);
            /* The first vertex Z precedes the second vertex address calculation */
            distance = (sint16)xport_load_le16(v0 + 4);
            XASSERT(!((uint32)((other_vertex + count + other_offset)) >= track_geometry.vertex_count));
            v1 = base + (other_vertex + count + other_offset) * 6;
            z1 = (sint16)xport_load_le16(v1 + 4);
            side = 2;
        }
        if (side == 1)
            distance = edge_distance(point, v0, v1, seg, z1);
        else
        {
            /* Reuse the already loaded first vertex Z without an extra guest read */
            VECTOR n;
            uint32 coord;
            uint32 origin;
            uint32 product;
            sint32 x0;
            sint32 x1;

            n.vy = 0;
            n.vx = z1 - distance;
            x0 = (sint16)xport_load_le16(v0);
            x1 = (sint16)xport_load_le16(v1);
            n.vz = x0 - x1;
            VectorNormal(&n, &n);
            coord = ((sint32)point->bits.x >> 8);
            origin = (sint16)xport_load_le16(v0);
            coord -= origin;
            coord -= (memcpy(&wire_word_5, seg + 12, sizeof(wire_word_5)), wire_word_5);
            product = coord * (uint32)n.vx;
            coord = ((sint32)point->bits.z >> 8);
            origin = (sint16)xport_load_le16(v0 + 4);
            coord -= origin;
            coord -= (memcpy(&wire_word_6, seg + 20, sizeof(wire_word_6)), wire_word_6);
            distance = (product + coord * (uint32)n.vz);
        }
        if (distance < 0)
        {
            angle = edge_angle(car, v0, v1);
            effect_spawn_col_line(point, car);
            apply_contact_impact(car, point, angle, (sint32)((uint32)distance + 4095) >> 12);
            callback(car);
            car_states[slot].boundary_side = (uint8)(side);
        }
    }
    return 0;
}

static uint32 update_col_contacts(CAR_STATE *car)
{
    uint32 angle_x, angle_y, angle_z, pos_x, pos_y, pos_z;
    uint32 val, val0, val1, mode, idx;
    SVECTOR *output;
    SVECTOR *vector;
    uint32 total = 0, count = 0;
    sint32 height, suspension;

    FUNCTION_MARKER(0x80024938, "SLUS_006.10");
    angle_x = car->orientation.bits.x;
    angle_y = car->orientation.bits.y;
    angle_z = car->orientation.bits.z;
    pos_x = car->pos.bits.x;
    pos_y = car->pos.bits.y;
    pos_z = car->pos.bits.z;
    car->angles.vx = (sint16)(uint16)((sint32)angle_x >> 8);
    car->angles.vy = (sint16)(uint16)((sint32)angle_y >> 8);
    car->angles.vz = (sint16)(uint16)((sint32)angle_z >> 8);
    car->transform.t[0] = (sint32)(uint32)((sint32)pos_x >> 8);
    car->transform.t[1] = (sint32)(uint32)((sint32)pos_y >> 8);
    car->transform.t[2] = (sint32)(uint32)((sint32)pos_z >> 8);
    model_build_rotation(&car->transform, &car->angles);
    track_update_location(&car->track_location, &car->pos);
    track_surface_n(&car->track_location, &car->pos, &car->ground_n);
    update_ground_contacts(car);
    mode = car->wheel_air_mask;
    switch (mode)
    {
        case 0:
        case 1:
        case 2:
        case 4:
        case 6:
        case 8:
        case 9:
            fit_suspension_angles(&car->angles, car->wheel_contact_pos, car->suspension_offset);
            val = (uint32)car->angles.vx << 8;
            val0 = (uint32)car->angles.vz << 8;
            car->orientation.bits.x = (uint32)(val);
            car->orientation.bits.z = (uint32)(val0);
            break;
        case 3:
        case 12:
            fit_suspension_roll(&car->angles, car->wheel_contact_pos, car->suspension_offset);
            car->orientation.bits.z = (uint32)((uint32)car->angles.vz << 8);
            break;
        case 5:
        case 10:
            fit_suspension_pitch(&car->angles, car->wheel_contact_pos, car->suspension_offset);
            car->orientation.bits.x = (uint32)((uint32)car->angles.vx << 8);
            break;
        default:
            break;
    }
    model_build_rotation(&car->transform, &car->angles);

    height = 181 * (sint16)car_geometry_metrics[car - car_states].wheel_radius / 256;
    val = ((uint32)(uint16)car->transform.m[0][0] | ((uint32)(uint16)car->transform.m[0][1] << 16));
    val0 = ((uint32)(uint16)car->transform.m[0][2] | ((uint32)(uint16)car->transform.m[1][0] << 16));
    xport_gte_write_control(0, val);
    xport_gte_write_control(1, val0);
    val = ((uint32)(uint16)car->transform.m[1][1] | ((uint32)(uint16)car->transform.m[1][2] << 16));
    val0 = ((uint32)(uint16)car->transform.m[2][0] | ((uint32)(uint16)car->transform.m[2][1] << 16));
    val1 = (uint16)car->transform.m[2][2];
    xport_gte_write_control(2, val);
    xport_gte_write_control(3, val0);
    xport_gte_write_control(4, val1);
    for (idx = 0; idx < 4; ++idx)
    {
        if (((car->wheel_air_mask >> idx) & 1) != 0)
            continue;

        suspension = (sint32)car->suspension_offset[idx] / 256;
        val = (uint16)car_suspension_points[car - car_states][idx].vy;
        vector = &car->suspension_local[idx];
        output = &car->suspension_rotated[idx];
        vector->vy = (sint16)(uint16)(val - (uint32)suspension - (uint32)height);
        xport_gte_write_data(0, ((uint32)(uint16)vector->vx | ((uint32)(uint16)vector->vy << 16)));
        xport_gte_write_data(1, ((uint32)(uint16)vector->vz | ((uint32)(uint16)vector->pad << 16)));
        xport_gte_execute(0x486012);
        val = xport_gte_read_data(9);
        val0 = xport_gte_read_data(10);
        val1 = xport_gte_read_data(11);
        output->vx = (sint16)(uint16)(val);
        output->vy = (sint16)(uint16)(val0);
        output->vz = (sint16)(uint16)(val1);
        val = (uint16)vector->vy;
        vector->vy = (sint16)(uint16)(val + (uint32)height);
        val = (uint32)output->vy << 8;
        val0 = car->wheel_contact_pos[idx].bits.y;
        ++count;
        total += val0 - val;
    }
    if (count == 0)
        return 0;
    val = ((sint32)total / (sint32)count);
    car->pos.bits.y = (uint32)(val);
    return val;
}

static uint32 update_col_surface(CAR_STATE *car)
{
    uint32 x;
    uint32 y;
    uint32 z;

    FUNCTION_MARKER(0x8002D79C, "SLUS_006.10");
    track_update_location(&car->track_location, &car->pos);
    track_surface_n_height(&car->track_location, &car->pos, &car->ground_n, &car->ground_height);
    x = car->pos.bits.x;
    y = car->pos.bits.y;
    z = car->pos.bits.z;
    car->transform.t[0] = (sint32)(uint32)((uint32)((sint32)x >> 8));
    car->transform.t[1] = (sint32)(uint32)((uint32)((sint32)y >> 8));
    car->transform.t[2] = (sint32)(uint32)((uint32)((sint32)z >> 8));
    return find_ground_contacts(car);
}

static uint32 update_ground_contacts(CAR_STATE *car)
{
    sint32 model_height;
    sint32 saved_node;
    uint32 saved_lane;
    sint32 height;
    uint32 old_mask;
    uint32 idx;
    VEC3 *world;
    VEC3 *body_world;
    TRACK_LOCATION *node;
    SVECTOR *vector;
    VEC3 prev_pos;
    uint32 val;

    sint32 suspension;
    VEC3 *t;
    VEC3 coord;
    uint32 ground;
    uint32 gravity;
    uint32 *deviation;
    uint32 mask = 0;
    sint32 step;

    FUNCTION_MARKER(0x80020030, "SLUS_006.10");

    model_height = (sint16)car_geometry_metrics[car - car_states].wheel_radius;
    saved_node = (sint16)car->track_location.node;
    saved_lane = car->track_location.lane;
    height = model_height * 181 / 256;
    SetRotMatrix(&car->transform);
    SetTransMatrix(&car->transform);
    old_mask = car->wheel_air_mask;
    car->prev_wheel_air_mask = (uint8)(old_mask);
    for (idx = 0; idx < 4; ++idx)
    {
        world = &car->wheel_contact_pos[idx];
        prev_pos.bits.x = world->bits.x;
        prev_pos.bits.y = world->bits.y;
        prev_pos.bits.z = world->bits.z;
        node = &car->track_contacts[4 + idx];
        node->node = (uint16)(saved_node);
        node->lane = (uint8)(saved_lane);

        suspension = (sint32)car->suspension_offset[idx] / 256;
        val = (uint16)car_suspension_points[car - car_states][idx].vy - (uint32)suspension - (uint32)height;
        vector = &car->suspension_local[idx];
        vector->vy = (sint16)(uint16)(val);
        xport_gte_write_data(0, ((uint32)(uint16)vector->vx | ((uint32)(uint16)vector->vy << 16)));
        xport_gte_write_data(1, ((uint32)(uint16)vector->vz | ((uint32)(uint16)vector->pad << 16)));
        xport_gte_execute(0x480012);
        world->bits.x = (uint32)(xport_gte_read_data(25));
        world->bits.y = (uint32)(xport_gte_read_data(26));
        world->bits.z = (uint32)(xport_gte_read_data(27));
        val = (uint16)vector->vy;
        vector->vy = (sint16)(uint16)(val + (uint32)height);
        xport_gte_write_data(0, ((uint32)(uint16)vector->vx | ((uint32)(uint16)vector->vy << 16)));
        xport_gte_write_data(1, ((uint32)(uint16)vector->vz | ((uint32)(uint16)vector->pad << 16)));
        xport_gte_execute(0x480012);
        t = &car->suspension_transform[idx];
        t->bits.x = (uint32)(xport_gte_read_data(25));
        t->bits.y = (uint32)(xport_gte_read_data(26));
        t->bits.z = (uint32)(xport_gte_read_data(27));
        coord.bits.x = world->bits.x;
        coord.bits.y = world->bits.y;
        coord.bits.z = world->bits.z;
        world->bits.x = (uint32)(coord.bits.x << 8);
        world->bits.y = (uint32)(coord.bits.y << 8);
        world->bits.z = (uint32)(coord.bits.z << 8);
        track_update_location(node, world);
        track_surface_n_height(node, world, &car->wheel_normals[idx], &ground);
        val = world->bits.y;
        gravity = car_physics.gravity;
        deviation = &car->suspension_deviation[idx];
        *deviation = (val - ground + gravity);
        val = world->bits.x;
        step = (sint32)(val - prev_pos.bits.x) / 256;
        car->wheel_contact_velocity[idx].vx = (sint16)(uint16)(step);
        val = world->bits.y;
        step = (sint32)(val - prev_pos.bits.y) / 256;
        car->wheel_contact_velocity[idx].vy = (sint16)(uint16)(step);
        val = world->bits.z;
        step = (sint32)(val - prev_pos.bits.z) / 256;
        car->wheel_contact_velocity[idx].vz = (sint16)(uint16)(step);
        val = *deviation;
        if ((sint32)val < 0)
            val = 0 - val;
        if ((sint32)val < 512)
            *deviation = (0);
        if ((sint32)*deviation >= 2049)
        {
            mask |= 1 << idx;
            *deviation = (12000);
        }
        else
            world->bits.y = (uint32)(ground);
    }
    car->wheel_air_mask = (uint8)(mask);
    for (idx = 0; idx < 4; ++idx)
    {
        node = &car->track_contacts[idx];
        node->node = (uint16)(saved_node);
        node->lane = (uint8)(saved_lane);

        body_world = &car->body_contact_pos[idx];
        xport_gte_write_data(0, ((uint32)(uint16)car_body_points[car - car_states][idx].vx | ((uint32)(uint16)car_body_points[car - car_states][idx].vy << 16)));
        xport_gte_write_data(1, ((uint32)(uint16)car_body_points[car - car_states][idx].vz | ((uint32)(uint16)car_body_points[car - car_states][idx].pad << 16)));
        xport_gte_execute(0x480012);
        body_world->bits.x = (uint32)(xport_gte_read_data(25));
        body_world->bits.y = (uint32)(xport_gte_read_data(26));
        body_world->bits.z = (uint32)(xport_gte_read_data(27));
        coord.bits.x = body_world->bits.x;
        coord.bits.y = body_world->bits.y;
        coord.bits.z = body_world->bits.z;
        body_world->bits.x = (uint32)(coord.bits.x << 8);
        body_world->bits.y = (uint32)(coord.bits.y << 8);
        body_world->bits.z = (uint32)(coord.bits.z << 8);
        track_update_location(node, body_world);
        track_surface_height(node, body_world, &body_world->bits.y);
    }
    return 0;
}

uint32 car_update_grounding(CAR_STATE *car)
{
    uint32 gravity;
    uint32 time;
    uint32 saved_x;
    uint32 saved_z;
    sint32 gravity_step;
    sint32 step;
    sint32 next_step;
    uint32 val;
    uint32 flag;
    uint32 snapshot[6];
    uint32 idx;
    uint32 mask;
    uint32 prev_mask;
    sint32 factor;
    sint32 delta;
    sint32 velocity;
    sint32 limit_val;

    sint32 height;
    uint32 rotation[5];
    SVECTOR *vector;
    SVECTOR *output;
    sint32 suspension;
    uint32 y;
    VEC3 rotated;
    uint32 total;
    sint32 count;
    sint32 avg;

    FUNCTION_MARKER(0x80024220, "SLUS_006.10");
    gravity = car_physics.gravity;
    time = disp_state.time_step;
    val = car->pos.bits.y;
    car->ground_height = (uint32)(val);
    saved_x = (uint16)car->angles.vx | ((uint32)(uint16)car->angles.vy << 16);
    saved_z = (uint16)car->angles.vz | ((uint32)(uint16)car->angles.pad << 16);
    gravity_step = (sint32)(gravity * time) / 256;
    step = (sint32)(car->angular_velocity.bits.x * time) / 256;
    val = car->velocity.bits.y - (uint32)gravity_step;
    car->velocity.bits.y = (uint32)(val);
    next_step = (sint32)(car->angular_velocity.bits.y * time) / 256;
    val = car->orientation.bits.x + (uint32)step;
    car->orientation.bits.x = (uint32)(val);
    step = (sint32)(car->angular_velocity.bits.z * time) / 256;
    val = car->orientation.bits.y + (uint32)next_step;
    car->orientation.bits.y = (uint32)(val);
    next_step = (sint32)(car->velocity.bits.y * time) / 256;
    val = car->orientation.bits.z + (uint32)step;
    car->orientation.bits.z = (uint32)(val);
    val = car->pos.bits.y;
    flag = horizontal_motion_frozen;
    car->pos.bits.y = (uint32)(val + (uint32)next_step);
    if (flag == 0)
    {
        step = (sint32)(car->velocity.bits.x * time) / 256;
        next_step = (sint32)(car->velocity.bits.z * time) / 256;
        val = car->pos.bits.x + (uint32)step;
        car->pos.bits.x = (uint32)(val);
        val = car->pos.bits.z + (uint32)next_step;
        car->pos.bits.z = (uint32)(val);
    }
    for (idx = 0; idx < 6; ++idx)
        snapshot[idx] = idx < 3 ? car->orientation.u[idx] : car->pos.u[idx - 3];
    car->angles.vx = (sint16)(uint16)((sint32)snapshot[0] >> 8);
    car->angles.vy = (sint16)(uint16)((sint32)snapshot[1] >> 8);
    car->angles.vz = (sint16)(uint16)((sint32)snapshot[2] >> 8);
    car->transform.t[0] = (sint32)(uint32)((sint32)snapshot[3] >> 8);
    car->transform.t[1] = (sint32)(uint32)((sint32)snapshot[4] >> 8);
    car->transform.t[2] = (sint32)(uint32)((sint32)snapshot[5] >> 8);
    model_build_rotation(&car->transform, &car->angles);
    track_update_location(&car->track_location, &car->pos);
    track_surface_n(&car->track_location, &car->pos, &car->ground_n);
    update_ground_contacts(car);
    mask = car->wheel_air_mask;
    switch (mask)
    {
        case 0:
        case 1:
        case 2:
        case 4:
        case 6:
        case 8:
        case 9:
            fit_suspension_angles(&car->angles, car->wheel_contact_pos, car->suspension_offset);
            mask = car->wheel_air_mask;
            prev_mask = car->prev_wheel_air_mask;
            if (mask == prev_mask)
            {
                factor = fixed_divide_checked(65536, (sint32)disp_state.time_step);
                delta = ((car->angles.vx - (sint16)saved_x + 2048) & 4095) - 2048;
                velocity = ((uint32)delta * (uint32)factor);
                car->angular_velocity.bits.x = (uint32)(delta);
                val = (uint32)car->angles.vz;
                car->angular_velocity.bits.x = (uint32)(velocity);
                delta = (((sint32)val - (sint16)saved_z + 2048) & 4095) - 2048;
                limit_val = car->angular_velocity.x;
                car->angular_velocity.bits.z = (uint32)((uint32)delta * (uint32)factor);
                if (limit_val > 6000)
                    car->angular_velocity.bits.x = (uint32)(6000);
                if (car->angular_velocity.x < -6000)
                    car->angular_velocity.bits.x = (uint32)(-6000);
                if (car->angular_velocity.z > 6000)
                    car->angular_velocity.bits.z = (uint32)(6000);
                if (car->angular_velocity.z < -6000)
                    car->angular_velocity.bits.z = (uint32)(-6000);
            }
            val = (uint32)car->angles.vx << 8;
            y = (uint32)car->angles.vz << 8;
            car->orientation.bits.x = (uint32)(val);
            car->orientation.bits.z = (uint32)(y);
            break;
        case 3:
        case 12:
            fit_suspension_roll(&car->angles, car->wheel_contact_pos, car->suspension_offset);
            mask = car->wheel_air_mask;
            prev_mask = car->prev_wheel_air_mask;
            if (mask == prev_mask)
            {
                factor = fixed_divide_checked(65536, (sint32)disp_state.time_step);
                delta = ((car->angles.vz - (sint16)saved_z + 2048) & 4095) - 2048;
                velocity = ((uint32)delta * (uint32)factor);
                car->angular_velocity.bits.z = (uint32)(delta);
                car->angular_velocity.bits.z = (uint32)(velocity);
                if (velocity > 6000)
                    car->angular_velocity.bits.z = (uint32)(6000);
                if (car->angular_velocity.z < -6000)
                    car->angular_velocity.bits.z = (uint32)(-6000);
            }
            car->orientation.bits.z = (uint32)((uint32)car->angles.vz << 8);
            break;
        case 5:
        case 10:
            fit_suspension_pitch(&car->angles, car->wheel_contact_pos, car->suspension_offset);
            mask = car->wheel_air_mask;
            prev_mask = car->prev_wheel_air_mask;
            if (mask == prev_mask)
            {
                factor = fixed_divide_checked(65536, (sint32)disp_state.time_step);
                delta = ((car->angles.vx - (sint16)saved_x + 2048) & 4095) - 2048;
                velocity = ((uint32)delta * (uint32)factor);
                car->angular_velocity.bits.x = (uint32)(delta);
                car->angular_velocity.bits.x = (uint32)(velocity);
                if (velocity > 6000)
                    car->angular_velocity.bits.x = (uint32)(6000);
                if (car->angular_velocity.x < -6000)
                    car->angular_velocity.bits.x = (uint32)(-6000);
            }
            car->orientation.bits.x = (uint32)((uint32)car->angles.vx << 8);
            break;
        default:
            break;
    }
    model_build_rotation(&car->transform, &car->angles);

    height = (sint16)car_geometry_metrics[car - car_states].wheel_radius * 181 / 256;
    rotation[0] = ((uint32)(uint16)car->transform.m[0][0] | ((uint32)(uint16)car->transform.m[0][1] << 16));
    rotation[1] = ((uint32)(uint16)car->transform.m[0][2] | ((uint32)(uint16)car->transform.m[1][0] << 16));
    xport_gte_write_control(0, rotation[0]);
    xport_gte_write_control(1, rotation[1]);
    rotation[2] = ((uint32)(uint16)car->transform.m[1][1] | ((uint32)(uint16)car->transform.m[1][2] << 16));
    rotation[3] = ((uint32)(uint16)car->transform.m[2][0] | ((uint32)(uint16)car->transform.m[2][1] << 16));
    rotation[4] = (uint16)car->transform.m[2][2];
    xport_gte_write_control(2, rotation[2]);
    xport_gte_write_control(3, rotation[3]);
    xport_gte_write_control(4, rotation[4]);
    total = 0;
    count = 0;
    for (idx = 0; idx < 4; ++idx)
    {
        mask = car->wheel_air_mask;
        if (((mask >> idx) & 1) == 0)
        {
            vector = &car->suspension_local[idx];
            output = &car->suspension_rotated[idx];

            suspension = (sint32)car->suspension_offset[idx] / 256;
            y = (uint16)car_suspension_points[car - car_states][idx].vy - (uint32)suspension - (uint32)height;
            vector->vy = (sint16)(uint16)(y);
            xport_gte_write_data(0, ((uint32)(uint16)vector->vx | ((uint32)(uint16)vector->vy << 16)));
            xport_gte_write_data(1, ((uint32)(uint16)vector->vz | ((uint32)(uint16)vector->pad << 16)));
            xport_gte_execute(0x486012);
            rotated.bits.x = xport_gte_read_data(9);
            rotated.bits.y = xport_gte_read_data(10);
            rotated.bits.z = xport_gte_read_data(11);
            output->vx = (sint16)(uint16)(rotated.bits.x);
            output->vy = (sint16)(uint16)(rotated.bits.y);
            output->vz = (sint16)(uint16)(rotated.bits.z);
            y = (uint16)vector->vy;
            vector->vy = (sint16)(uint16)(y + (uint32)height);
            y = (uint32)output->vy << 8;
            val = car->wheel_contact_pos[idx].bits.y;
            ++count;
            total += val - y;
        }
    }
    if (count > 0)
    {
        avg = fixed_divide_checked((sint32)total, count);
        mask = car->wheel_air_mask;
        car->pos.bits.y = (uint32)(avg);
        if (ground_velocity_masks[mask] != 0)
        {
            prev_mask = car->prev_wheel_air_mask;
            if ((prev_mask & (mask ^ 15)) == 0)
            {
                val = car->ground_height;
                gravity = car_physics.gravity;
                time = disp_state.time_step;
                val = ((uint32)avg - val - gravity) << 8;
                velocity = fixed_divide_checked((sint32)val, (sint32)time);
                car->velocity.bits.y = (uint32)(velocity);
            }
        }
    }
    else
        car->ungrounded_updates = (uint16)(car->ungrounded_updates + 1);
    return apply_contact_forces(car);
}

void car_reset_grounding(CAR_STATE *car)
{
    uint32 idx;
    sint32 orientation;
    sint32 x;
    sint32 y;
    sint32 z;
    sint32 offset;

    FUNCTION_MARKER(0x80024134, "SLUS_006.10");
    car->pos.bits.y = (uint32)(0xC0000000);
    car->gear = (uint8)(2);
    car->wheel_slip_flags = (uint8)(0);
    car->motion_mode = (uint8)(0);
    for (idx = 0x188; idx <= 0x19C; idx += 4)
    {
        if (idx <= 0x190)
            car->angular_velocity.u[(idx - 0x188) / 4] = 0;
        else
            car->velocity.u[(idx - 0x194) / 4] = 0;
    }
    car->motion_ticks = (uint16)(0);
    car->wheel_air_mask = (uint8)(0);
    car->engine_speed = 400;
    for (idx = 0; idx < 16; idx += 4)
    {
        car->suspension_velocity[idx / 4] = (uint32)(0);
        car->suspension_offset[idx / 4] = (uint32)(0);
    }
    orientation = car->orientation.y;
    x = car->pos.x;
    y = car->pos.y;
    z = car->pos.z;
    car->angles.vx = (sint16)(uint16)(0);
    car->orientation.bits.x = (uint32)(0);
    car->angles.vz = (sint16)(uint16)(0);
    car->orientation.bits.z = (uint32)(0);
    car->transform.t[1] = (sint32)(uint32)(y >> 8);
    car->angles.vy = (sint16)(uint16)(orientation >> 8);
    car->transform.t[0] = (sint32)(uint32)(x >> 8);
    car->transform.t[2] = (sint32)(uint32)(z >> 8);
    car_update_grounding(car);
    for (offset = 12; offset >= 0; offset -= 4)
        car->suspension_deviation[(uint32)offset / 4] = (uint32)(0);
    car->angular_velocity.bits.x = (uint32)(0);
    car->angular_velocity.bits.z = (uint32)(0);
    car->velocity.bits.y = (uint32)(0);
    return;
}

static uint32 mul_matrix_q30(const uint32 m0[9], const uint32 m1[9], uint32 output[9])
{
    uint32 row, column, term, left, right, sum, result;

    FUNCTION_MARKER(0x80077ABC, "SLUS_006.10");
    for (row = 0; row < 3; ++row)
    {
        for (column = 0; column < 3; ++column)
        {
            sum = 0;
            for (term = 0; term < 3; ++term)
            {
                left = m0[row * 3 + term];
                right = m1[term * 3 + column];
                result = mul_q30(left, right);
                sum += result;
            }
            output[row * 3 + column] = sum;
        }
    }
    return result;
}

static uint32 mul_q30(uint32 left, uint32 right)
{
    uint64 product;

    FUNCTION_MARKER(0x8007A340, "SLUS_006.10");
    product = (uint64)((sint64)(sint32)left * (sint32)right) + 0x40000000u;
    return (product >> 30);
}

static void step_matrix_motion(CAR_STATE *car)
{
    uint32 matrix[9];
    uint32 vx, vy, vz, px, py, pz, gravity, val, result, grp, idx;
    uint32 words[4];

    FUNCTION_MARKER(0x8002D824, "SLUS_006.10");
    vx = car->velocity.bits.x;
    px = car->pos.bits.x;
    vy = car->velocity.bits.y;
    vx -= (uint32)((sint32)vx / 256);
    car->velocity.bits.x = (uint32)(vx);
    car->pos.bits.x = (uint32)(px + vx);
    vz = car->velocity.bits.z;
    gravity = car_physics.gravity;
    py = car->pos.bits.y;
    vy -= (uint32)((sint32)vy / 256) + gravity;
    car->velocity.bits.y = (uint32)(vy);
    car->pos.bits.y = (uint32)(py + vy);
    pz = car->pos.bits.z;
    vz -= (uint32)((sint32)vz / 256);
    car->velocity.bits.z = (uint32)(vz);
    car->pos.bits.z = (uint32)(pz + vz);
    mul_matrix_q30(car->rotation_step_q30, car->rotation_q30, matrix);
    for (grp = 0; grp < 2; ++grp)
    {
        for (idx = 0; idx < 4; ++idx)
            words[idx] = matrix[grp * 4 + idx];
        for (idx = 0; idx < 4; ++idx)
            car->rotation_q30[grp * 4 + idx] = (uint32)(words[idx]);
    }
    car->rotation_q30[8] = (uint32)(matrix[8]);
    pack_matrix_q30(&car->transform, matrix);
    track_update_location(&car->track_location, &car->pos);
    track_surface_n_height(&car->track_location, &car->pos, &car->ground_n, &car->ground_height);
    px = car->pos.bits.x;
    py = car->pos.bits.y;
    pz = car->pos.bits.z;
    car->transform.t[0] = (sint32)(uint32)((uint32)((sint32)px >> 8));
    car->transform.t[1] = (sint32)(uint32)((uint32)((sint32)py >> 8));
    car->transform.t[2] = (sint32)(uint32)((uint32)((sint32)pz >> 8));
    val = find_ground_contacts(car);
    if (val != 0)
        resolve_ground_impact(car, val);
    result = (sint16)car->motion_ticks < 60;
    if (result == 0)
    {
        car_reset_grounding(car);
        return;
    }
    return;
}

static uint32 matrix_to_angles(const MATRIX *matrix, SVECTOR *angles)
{
    sint32 val0, val1;
    uint32 sq, root, result;

    FUNCTION_MARKER(0x80079720, "SLUS_006.10");
    val0 = matrix->m[1][0];
    sq = (val0 * val0);
    val1 = matrix->m[1][1];
    sq += (uint32)(val1 * val1);
    root = fixed_sqrt(sq);
    val0 = -matrix->m[1][2];
    result = (uint32)fixed_angle_from_vector(val0, (sint32)root);
    angles->vx = (sint16)result;
    if ((result & 0x7FF) == 0x400)
    {
        val0 = -matrix->m[2][0];
        val1 = matrix->m[0][0];
        result = (uint32)fixed_angle_from_vector(val0, val1);
        angles->vy = (sint16)result;
        angles->vz = 0;
    }
    else
    {
        val0 = matrix->m[0][2];
        val1 = matrix->m[2][2];
        result = (uint32)fixed_angle_from_vector(val0, val1);
        angles->vy = (sint16)result;
        val0 = matrix->m[1][0];
        val1 = matrix->m[1][1];
        result = (uint32)fixed_angle_from_vector(val0, val1);
        angles->vz = (sint16)result;
    }
    return result;
}

static uint32 pack_matrix_q30(MATRIX *dst, const uint32 src[9])
{
    uint32 idx, val;
    uint32 result;

    FUNCTION_MARKER(0x80077A04, "SLUS_006.10");
    for (idx = 0; idx < 9; ++idx)
    {
        val = src[idx];
        result = ((sint32)(val + 0x20000) >> 18);
        dst->m[idx / 3][idx % 3] = (sint16)result;
    }
    return result;
}

static uint32 sync_rotation(CAR_STATE *car)
{
    MATRIX matrix;
    SVECTOR angles;
    uint32 result, val;

    FUNCTION_MARKER(0x8002D6C4, "SLUS_006.10");
    matrix_to_angles(&car->transform, &car->angles);
    pack_matrix_q30(&matrix, car->rotation_step_q30);
    matrix_to_angles(&matrix, &angles);
    car->angular_velocity.bits.x = (uint32)((uint32)angles.vx);
    car->angular_velocity.bits.y = (uint32)((uint32)angles.vy);
    result = race_control.update_cycles;
    val = (uint32)angles.vz;
    result &= 3;
    car->angular_velocity.bits.z = (uint32)(val);
    if (result != 0)
        return result;
    model_build_rotation(&car->transform, &car->angles);
    expand_matrix_q30(car->rotation_q30, &car->transform);
    model_build_rotation(&matrix, &angles);
    return expand_matrix_q30(car->rotation_step_q30, &matrix);
}

static uint32 expand_matrix_q30(uint32 dst[9], const MATRIX *src)
{
    uint32 idx;
    uint32 val;

    FUNCTION_MARKER(0x80077970, "SLUS_006.10");
    for (idx = 0; idx < 9; ++idx)
    {
        val = (uint32)src->m[idx / 3][idx % 3] << 18;
        dst[idx] = val;
    }
    return val;
}

static void check_ground_recovery(CAR_STATE *car)
{
    sint32 heading, n_x, n_z, local_x, local_y, local_z, error_x;
    uint32 c, s, val, pitch, roll, root, dx, dz, vx, vz, result;

    FUNCTION_MARKER(0x8002CCE0, "SLUS_006.10");
    result = (sint16)car->motion_ticks < 3;
    if (result != 0)
        return;
    heading = car->angles.vy;
    c = (uint32)rcos(heading);
    s = (uint32)rsin(heading);
    n_x = car->ground_n.vx;
    n_z = car->ground_n.vz;
    val = (uint32)n_x * c - (uint32)n_z * s;
    local_x = (sint16)((sint32)val / 4096);
    local_y = (sint16)(uint16)car->ground_n.vy;
    n_x = car->ground_n.vx;
    n_z = car->ground_n.vz;
    val = (uint32)n_x * s + (uint32)n_z * c;
    local_z = (sint16)((sint32)val / 4096);
    pitch = (uint32)fixed_angle_from_vector(-local_z, -local_y);
    root = fixed_sqrt((uint32)(local_y * local_y) + (uint32)(local_z * local_z));
    roll = (uint32)fixed_angle_from_vector(local_x, (sint32)root);
    n_x = car->angles.vx;
    n_z = car->angles.vz;
    dx = pitch - (uint32)n_x + 512;
    dz = roll - (uint32)n_z + 512;
    error_x = (sint32)(dx & 1023) - 512;
    if (error_x < 0)
        error_x = -error_x;
    result = (dz & 1023) - 512;
    if (error_x >= 24)
        return;
    if ((sint32)result < 0)
        result = 0u - result;
    result = result < 24;
    if (result == 0)
        return;
    vx = car->angular_velocity.bits.x;
    vz = car->angular_velocity.bits.z;
    result = ((vx + 2048) & 4095) - 2017 < 63;
    if (result == 0)
        return;
    result = ((vz + 2048) & 4095) - 2017 < 63;
    if (result == 0)
        return;
    val = ((dx & 0xC00) >> 10) | ((dz & 0xC00) >> 8);
    if (val == 0 || val == 10)
    {
        car_reset_grounding(car);
        return;
    }
    return;
}

static uint32 build_axis_rotation_q30(uint32 output[9], const VECTOR *axis, uint32 angle)
{
    uint32 basis[9], rotation[9], combined[9];
    uint32 idx, sample0, sample1, fraction, val, c, s, x, y, z;
    uint32 yy, zz, radial, len, scale, coupled_scale, radial_scale, q;
    uint32 b00, b02, b10, b11, b12, b20, b21, b22;

    FUNCTION_MARKER(0x80077E1C, "SLUS_006.10");
    idx = ((uint32)((sint32)angle >> 18) + 1024) & 4095;
    sample0 = idx < 2048 ? s_table[idx] : 0u - s_table[idx - 2048];
    idx = ((uint32)((sint32)angle >> 18) + 1025) & 4095;
    sample1 = idx < 2048 ? s_table[idx] : 0u - s_table[idx - 2048];
    fraction = (angle & 0x3FFFF) << 12;
    val = mul_q30(sample1, fraction);
    c = val + mul_q30(sample0, 0x40000000u - fraction);
    idx = (uint32)((sint32)angle >> 18) & 4095;
    sample0 = idx < 2048 ? s_table[idx] : 0u - s_table[idx - 2048];
    idx = ((uint32)((sint32)angle >> 18) + 1) & 4095;
    sample1 = idx < 2048 ? s_table[idx] : 0u - s_table[idx - 2048];
    val = mul_q30(sample1, fraction);
    q = 0x40000000;
    s = val + mul_q30(sample0, 0x40000000u - fraction);
    y = (uint32)axis->vy;
    yy = y * y;
    z = (uint32)axis->vz;
    zz = z * z;
    x = (uint32)axis->vx;
    radial = fixed_sqrt(yy + zz);
    if ((sint32)radial <= 0)
    {
        if ((sint32)x < 0)
            s = 0u - s;
        output[0] = q;
        output[1] = 0;
        output[2] = 0;
        output[3] = 0;
        val = 0u - s;
        output[5] = val;
        output[6] = 0;
        output[7] = s;
        output[4] = c;
        output[8] = c;
        return val;
    }
    len = fixed_sqrt(x * x + yy + zz);
    if ((sint32)len <= 0)
    {
        for (idx = 0; idx < 9; ++idx)
            output[idx] = idx == 0 || idx == 4 || idx == 8 ? q : 0;
        return len;
    }
    scale = (uint32)fixed_divide_checked((sint32)q, (sint32)len);
    coupled_scale = (uint32)fixed_divide_checked((sint32)q, (sint32)(radial * len));
    radial_scale = (uint32)fixed_divide_checked((sint32)q, (sint32)radial);
    b00 = radial * scale;
    b10 = (0u - x) * y * coupled_scale;
    b02 = x * scale;
    b11 = z * radial_scale;
    b12 = y * scale;
    b20 = (0u - x) * z * coupled_scale;
    b21 = (0u - y) * radial_scale;
    b22 = z * scale;
    rotation[0] = c;
    rotation[4] = c;
    basis[2] = b02;
    basis[4] = b11;
    basis[1] = 0;
    rotation[1] = s;
    rotation[2] = 0;
    rotation[5] = 0;
    rotation[6] = 0;
    rotation[7] = 0;
    rotation[8] = q;
    basis[0] = b00;
    basis[3] = b10;
    rotation[3] = 0u - s;
    basis[5] = b12;
    basis[7] = b21;
    basis[8] = b22;
    basis[6] = b20;
    mul_matrix_q30(basis, rotation, combined);
    basis[4] = b11;
    basis[6] = b02;
    basis[0] = b00;
    basis[1] = b10;
    basis[2] = b20;
    basis[3] = 0;
    basis[5] = b21;
    basis[8] = b22;
    basis[7] = b12;
    return mul_matrix_q30(combined, basis, output);
}

static uint32 rotate_vector_q30(const uint32 matrix[9], const VECTOR *vector, VECTOR *output)
{
    uint32 row, term, left, right, sum, result;
    const sint32 *vector_components[3] = {&vector->vx, &vector->vy, &vector->vz};
    sint32 *output_components[3] = {&output->vx, &output->vy, &output->vz};

    FUNCTION_MARKER(0x80077D20, "SLUS_006.10");
    for (row = 0; row < 3; ++row)
    {
        sum = 0;
        for (term = 0; term < 3; ++term)
        {
            left = matrix[row * 3 + term];
            right = *vector_components[term];
            result = mul_q30(left, right);
            sum += result;
        }
        *output_components[row] = sum;
    }
    return result;
}

static uint32 resolve_ground_impact(CAR_STATE *car, uint32 contact)
{
    VECTOR avg, velocity, cross;
    const sint16 *n_components[3] = {&car->ground_n.vx, &car->ground_n.vy, &car->ground_n.vz};
    sint32 *avg_components[3] = {&avg.vx, &avg.vy, &avg.vz};
    const sint32 *velocity_components[3] = {&velocity.vx, &velocity.vy, &velocity.vz};
    uint32 basis[9], rotation[9], combined[9];
    uint32 count = 0, lift = 0, idx, axis, val, other, accumulated;
    uint32 ax, ay, az, bx, bz, len, impact, restitution, angle, c, s;
    uint32 x_axis, z_axis, tangent_len, limit, original_y;
    const uint32 *src;
    uint32 *dst;
    uint32 words[4], grp, result;

    FUNCTION_MARKER(0x8002CEC4, "SLUS_006.10");
    avg.vx = (sint32)(0);
    avg.vy = (sint32)(0);
    avg.vz = (sint32)(0);
    for (idx = 0; idx < 8; ++idx)
    {
        if ((contact & (1u << idx)) == 0)
            continue;
        for (axis = 0; axis < 3; ++axis)
        {
            val = car->body_contact_pos[idx].u[axis];
            other = car->pos.u[axis];
            accumulated = *avg_components[axis];
            val = ((sint32)(val - other) >> 4);
            *avg_components[axis] = (accumulated + val);
        }
        val = contact_heights[idx];
        other = car->body_contact_pos[idx].bits.y;
        val -= other;
        if ((sint32)lift < (sint32)val)
            lift = val;
        ++count;
    }
    ax = (uint32)fixed_divide_checked(avg.vx, (sint32)count);
    ay = (uint32)fixed_divide_checked(avg.vy, (sint32)count);
    az = (uint32)fixed_divide_checked(avg.vz, (sint32)count);
    avg.vx = (sint32)(ax);
    avg.vy = (sint32)(ay);
    avg.vz = (sint32)(az);
    rotate_vector_q30(car->rotation_step_q30, &avg, &velocity);
    ax = (uint32)avg.vx;
    val = car->velocity.bits.x;
    other = (uint32)velocity.vx;
    az = (uint32)avg.vz;
    velocity.vx = (sint32)(other + (uint32)((sint32)val >> 4) - ax);
    val = car->velocity.bits.y;
    ay = (uint32)avg.vy;
    other = (uint32)velocity.vy;
    velocity.vy = (sint32)(other + (uint32)((sint32)val >> 4) - ay);
    val = car->velocity.bits.z;
    other = (uint32)velocity.vz;
    velocity.vz = (sint32)(other + (uint32)((sint32)val >> 4) - az);
    len = fixed_sqrt(ax * ax + az * az);
    accumulated = 0;
    for (axis = 0; axis < 3; ++axis)
    {
        val = *n_components[axis];
        other = *velocity_components[axis];
        accumulated += val * other;
    }
    impact = ((sint32)(0u - accumulated) >> 12);
    if ((sint32)impact < 0)
    {
        if ((sint32)impact < -4000)
        {
            audio_play_positioned(4, -1, 1024, 4096, &car->pos, 0);
            input_trigger_rumble((sint32)car->slot, 1024);
        }
        if ((sint32)len < 129)
        {
            restitution = ((sint32)((0u - impact) * 272) / 256);
            src = car->rotation_step_q30;
            dst = combined;
            for (grp = 0; grp < 2; ++grp)
            {
                for (idx = 0; idx < 4; ++idx)
                    words[idx] = src[grp * 4 + idx];
                for (idx = 0; idx < 4; ++idx)
                    dst[grp * 4 + idx] = words[idx];
            }
            dst[8] = src[8];
        }
        else
        {
            val = (uint32)((sint32)(len * len) / 256) + 200000;
            restitution = (uint32)fixed_divide_checked((sint32)(impact * (uint32)-212500), (sint32)val);
            val = ((sint32)(len * restitution) / 16);
            angle = ((sint32)(val * 854) / 16);
            c = cos_q30(angle);
            s = sin_q30(angle);
            val = (uint32)fixed_divide_checked(0x40000000, (sint32)len);
            z_axis = az * val;
            x_axis = ax * val;
            rotation[5] = s;
            basis[1] = 0;
            basis[3] = 0;
            basis[4] = 0x40000000;
            basis[5] = 0;
            basis[7] = 0;
            rotation[0] = 0x40000000;
            rotation[1] = 0;
            rotation[2] = 0;
            rotation[3] = 0;
            rotation[4] = c;
            rotation[6] = 0;
            rotation[7] = 0u - s;
            rotation[8] = c;
            basis[0] = z_axis;
            basis[8] = z_axis;
            basis[2] = x_axis;
            basis[6] = 0u - x_axis;
            mul_matrix_q30(basis, rotation, combined);
            basis[0] = z_axis;
            basis[1] = 0;
            basis[2] = 0u - x_axis;
            basis[3] = 0;
            basis[4] = 0x40000000;
            basis[5] = 0;
            basis[6] = x_axis;
            basis[7] = 0;
            basis[8] = z_axis;
            mul_matrix_q30(combined, basis, rotation);
            mul_matrix_q30(rotation, car->rotation_step_q30, combined);
        }
        bx = (uint32)velocity.vx;
        bz = (uint32)velocity.vz;
        tangent_len = fixed_sqrt(bx * bx + bz * bz);
        if ((sint32)tangent_len > 0)
        {
            limit = ((sint32)(impact + 1024) / 32);
            original_y = (uint32)avg.vy;
            ax = (uint32)avg.vx;
            az = (uint32)avg.vz;
            avg.vx = (sint32)((uint32)((sint32)ax >> 1));
            avg.vy = (sint32)((uint32)((sint32)original_y >> 1));
            avg.vz = (sint32)((uint32)((sint32)az >> 1));
            VectorNormal(&avg, &avg);
            velocity.vy = (sint32)(0);
            VectorNormal(&velocity, &velocity);
            OuterProduct12(&velocity, &avg, &cross);
            if ((sint32)tangent_len < (sint32)limit)
                limit = tangent_len;
            val = ((sint32)(original_y * limit) / 16);
            angle = ((sint32)(val * 854) / 16);
            build_axis_rotation_q30(basis, &cross, angle);
            mul_matrix_q30(combined, basis, car->rotation_step_q30);
            val = (uint32)velocity.vx;
            val = ((sint32)(limit * val) / 256);
            other = car->velocity.bits.x;
            car->velocity.bits.x = (uint32)(other - val);
            val = (uint32)velocity.vz;
            val = ((sint32)(limit * val) / 256);
            other = car->velocity.bits.z;
            car->velocity.bits.z = (uint32)(other - val);
        }
        else
        {
            for (grp = 0; grp < 2; ++grp)
            {
                for (idx = 0; idx < 4; ++idx)
                    words[idx] = combined[grp * 4 + idx];
                for (idx = 0; idx < 4; ++idx)
                    car->rotation_step_q30[grp * 4 + idx] = (uint32)(words[idx]);
            }
            car->rotation_step_q30[8] = (uint32)(combined[8]);
        }
        for (axis = 0; axis < 3; ++axis)
        {
            val = *n_components[axis];
            val = ((sint32)(val * restitution) / 256);
            other = car->velocity.u[axis];
            car->velocity.u[axis] = (uint32)(other - val);
        }
    }
    val = car->pos.bits.y;
    car->pos.bits.y = (uint32)(val + lift);
    check_ground_recovery(car);
    ax = car->pos.bits.x;
    ay = car->pos.bits.y;
    az = car->pos.bits.z;
    car->transform.t[0] = (sint32)(uint32)((uint32)((sint32)ax >> 8));
    result = (sint32)impact < -400;
    car->transform.t[1] = (sint32)(uint32)((uint32)((sint32)ay >> 8));
    car->transform.t[2] = (sint32)(uint32)((uint32)((sint32)az >> 8));
    if (result != 0)
    {
        result = car->slot < 6;
        if (result != 0)
            return audio_play_positioned(7, -1, 4096, 4096, &car->pos, 0);
    }
    return result;
}

uint32 car_load_s_table(void)
{
    uint8 wire[8192];
    uint32 size, idx;

    FUNCTION_MARKER(0x80077720, "SLUS_006.10");
    size = archive_read_file("PRES\\SINE.DAT", wire, 8192);
    XASSERT(!(size != 8192));
    for (idx = 0; idx < 2048; ++idx)
        memcpy(&s_table[idx], wire + idx * 4, sizeof(s_table[idx]));
    return size;
}

static uint32 sin_q30(uint32 angle)
{
    uint32 idx, sample0, sample1, fraction, result;

    FUNCTION_MARKER(0x800777CC, "SLUS_006.10");
    idx = (uint32)((sint32)angle >> 18) & 4095;
    sample0 = idx < 2048 ? s_table[idx] : 0u - s_table[idx - 2048];
    idx = ((uint32)((sint32)angle >> 18) + 1) & 4095;
    sample1 = idx < 2048 ? s_table[idx] : 0u - s_table[idx - 2048];
    fraction = (angle & 0x3FFFF) << 12;
    result = mul_q30(sample1, fraction);
    return result + mul_q30(sample0, 0x40000000u - fraction);
}

static uint32 cos_q30(uint32 angle)
{
    uint32 idx, sample0, sample1, fraction, result;

    FUNCTION_MARKER(0x80077898, "SLUS_006.10");
    idx = ((uint32)((sint32)angle >> 18) + 1024) & 4095;
    sample0 = idx < 2048 ? s_table[idx] : 0u - s_table[idx - 2048];
    idx = ((uint32)((sint32)angle >> 18) + 1025) & 4095;
    sample1 = idx < 2048 ? s_table[idx] : 0u - s_table[idx - 2048];
    fraction = (angle & 0x3FFFF) << 12;
    result = mul_q30(sample1, fraction);
    return result + mul_q30(sample0, 0x40000000u - fraction);
}

static uint32 find_ground_contacts(CAR_STATE *car)
{
    uint32 saved_node, saved_lane, idx;
    TRACK_LOCATION *node;
    SVECTOR *vector;
    VEC3 *world;
    VEC3 *t;
    uint32 suspension, val, axis, ground;
    VEC3 coord;
    uint32 mask = 0;

    FUNCTION_MARKER(0x8002C770, "SLUS_006.10");

    saved_node = (sint16)car->track_location.node;
    saved_lane = car->track_location.lane;
    SetRotMatrix(&car->transform);
    SetTransMatrix(&car->transform);
    for (idx = 0; idx < 8; ++idx)
    {
        node = &car->track_contacts[idx];
        world = &car->body_contact_pos[idx];

        node->node = (uint16)(saved_node);
        node->lane = (uint8)(saved_lane);
        xport_gte_write_data(0, ((uint32)(uint16)car_body_points[car - car_states][idx].vx | ((uint32)(uint16)car_body_points[car - car_states][idx].vy << 16)));
        xport_gte_write_data(1, ((uint32)(uint16)car_body_points[car - car_states][idx].vz | ((uint32)(uint16)car_body_points[car - car_states][idx].pad << 16)));
        xport_gte_execute(0x480012);
        world->bits.x = (uint32)(xport_gte_read_data(25));
        world->bits.y = (uint32)(xport_gte_read_data(26));
        world->bits.z = (uint32)(xport_gte_read_data(27));
        if (idx < 4)
        {
            suspension = ((sint32)car->suspension_offset[idx] / 256);
            val = (uint16)car_suspension_points[car - car_states][idx].vy;
            vector = &car->suspension_local[idx];
            vector->vy = (sint16)(uint16)(val - suspension);
            t = &car->suspension_transform[idx];
            xport_gte_write_data(0, ((uint32)(uint16)vector->vx | ((uint32)(uint16)vector->vy << 16)));
            xport_gte_write_data(1, ((uint32)(uint16)vector->vz | ((uint32)(uint16)vector->pad << 16)));
            xport_gte_execute(0x480012);
            t->bits.x = (uint32)(xport_gte_read_data(25));
            t->bits.y = (uint32)(xport_gte_read_data(26));
            t->bits.z = (uint32)(xport_gte_read_data(27));
            for (axis = 0; axis < 3; ++axis)
            {
                val = t->u[axis];
                car->wheel_contact_pos[idx].u[axis] = (uint32)(val << 8);
            }
        }
        coord.bits.x = world->bits.x;
        coord.bits.y = world->bits.y;
        coord.bits.z = world->bits.z;
        world->bits.x = (uint32)(coord.bits.x << 8);
        world->bits.y = (uint32)(coord.bits.y << 8);
        world->bits.z = (uint32)(coord.bits.z << 8);
        track_update_location(node, world);
        track_surface_height(node, world, &contact_heights[idx]);
        val = world->bits.y;
        ground = contact_heights[idx];
        if ((sint32)(val - ground) < 0)
            mask |= 1u << idx;
    }
    return mask;
}

intptr_t car_update_motion(uint32 slot)
{
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    CAR_STATE *car;
    uint32 val;
    uint32 desc;
    const CAR_PHYSICS_PROFILE *profile;
    uint32 limit;
    uint32 cur;
    uint32 count;
    uint32 idx;
    uint32 state;
    uint32 sum;
    uint32 timer;
    uint32 mag;
    CAR_COLLISION_CALLBACK callback;
    const sint8 *list;
    VEC3 heading;
    sint32 threshold;
    sint32 speed;
    sint32 peak;
    sint32 avg;
    sint32 rpm;

    FUNCTION_MARKER(0x80024C14, "SLUS_006.10");
    car = &car_states[slot];
    timer = car_states[slot].motion_ticks;
    car_states[slot].boundary_side = (uint8)(0);
    state = race_session.drag_race;
    car_states[slot].motion_ticks = (uint16)(timer + 1);
    if (state != 0)
    {
        level_desc = &scene_level_image.desc;
        desc = level_desc->checkpoints_offset;
        cur = car->completed_laps;
        XASSERT(!(desc >= scene_level_image.size));
        limit = scene_level_image.bytes[desc];
        if (cur >= limit)
        {
            car_states[slot].pedal = (uint16)(0xFF00);
            car->steering_angle = 0;
            car->braking = (uint8)(1);
        }
    }
    profile = car->physics_profile;
    threshold = -profile->suspension_limit;
    count = 0;
    for (idx = 0; idx < 4; ++idx)
        if ((sint32)car->suspension_offset[idx] <= threshold)
            ++count;
    if (count != 0)
    {
        audio_play_positioned(4, -1, 4096, 4096, &car_states[slot].pos, 0);
        input_trigger_rumble(car->slot, 2048);
    }
    if (car_states[slot].finish_time == 0)
    {
        val = car->forward_speed;
        mag = (sint32)val < 0 ? 0 - val : val;
        sum = car_states[slot].race_stats.speed_sum;
        peak = (sint16)car_states[slot].race_stats.top_speed;
        speed = (sint32)mag / 256;
        car_states[slot].race_stats.speed_sum = (uint32)(sum + (uint32)speed);
        if (speed < peak)
            speed = peak;
        car_states[slot].race_stats.top_speed = (uint16)((uint32)speed);
    }
    if (race_session.cup == 4)
        race_award_speed_points(car);
    if (race_control.timing_hold == 0 && car_states[slot].finish_time == 0)
    {
        timer = car_states[slot].race_stats.elapsed_ticks;
        if (timer != 0xFFFF)
            car_states[slot].race_stats.elapsed_ticks = (uint16)(timer + 1);
        timer = car_states[slot].race_stats.elapsed_ticks;
        sum = car_states[slot].race_stats.speed_sum;
        avg = fixed_divide_checked((sint32)sum, (sint32)timer);
        val = car->forward_speed;
        peak = (sint16)car_states[slot].race_stats.top_speed;
        mag = (sint32)val < 0 ? 0 - val : val;
        car_states[slot].race_stats.avg_speed = (uint16)((uint32)avg);
        speed = (sint32)mag / 256;
        if (speed < peak)
            speed = peak;
        car_states[slot].race_stats.top_speed = (uint16)((uint32)speed);
    }
    if (session_flow.attract_active == 0)
    {
        race_update_checkpoint_timer(car);
        race_update_lap_timer(car);
    }
    if (car_states[slot].motion_mode == 0)
        update_rollover_state(car);
    state = car_states[slot].motion_mode;
    if (state == 0)
    {
        if (race_control.start_hold == 0)
        {
            cur = car->best_place;
            limit = car->place;
            if (limit < cur)
                cur = limit;
            threshold = (sint16)car->ungrounded_updates;
            car->best_place = (uint8)cur;
            if (threshold >= 3 && car->wheel_air_mask == 15)
                update_airborne_motion(car);
            else if ((sint32)race_session.two_players < (sint32)slot)
                update_ai_motion(car);
            else
                update_player_motion(car);
        }
        else
        {
            val = car->place;
            car->start_place = (uint8)val;
            car_update_free_engine_speed(car);
            cur = car->slot;
            threshold = (sint32)race_session.two_players;
            if (threshold < (sint32)cur || session_flow.attract_active != 0)
            {
                profile = car->physics_profile;
                rpm = profile->rpm_limit * 2 / 3;
                state = session_flow.attract_active;
                car->engine_speed = (uint32)rpm;
                if (state != 0 && car->slot == 0)
                {
                    profile = car->physics_profile;
                    car->engine_speed = (uint32)(profile->rpm_limit - 1);
                }
            }
            profile = car->physics_profile;
            car->wheel_slip_flags = (uint8)(0);
            rpm = profile->rpm_limit * 3 / 4;
            if (rpm < (sint32)car->engine_speed)
                car->wheel_slip_flags = (uint8)((uint16)profile->drive_type);
        }
        car_update_grounding(car);
        callback = update_col_contacts;
        list = ground_contact_order;
    }
    else if (state == 1)
    {
        sync_rotation(car);
        heading.bits.x = (uint32)car_states[slot].angles.vx << 8;
        heading.bits.y = (uint32)car_states[slot].angles.vy << 8;
        heading.bits.z = (uint32)car_states[slot].angles.vz << 8;
        car_states[slot].orientation.bits.x = (uint32)(heading.bits.x);
        car_states[slot].orientation.bits.y = (uint32)(heading.bits.y);
        car_states[slot].orientation.bits.z = (uint32)(heading.bits.z);
        car_update_free_engine_speed(car);
        step_matrix_motion(car);
        callback = update_col_surface;
        list = all_contact_order;
    }
    else
        return 1;
    resolve_end_contacts(slot, callback, list);
    resolve_start_contacts(slot, callback, list);
    return resolve_road_contacts(slot, callback, list);
}

intptr_t car_update_boundary_motion(uint32 slot)
{
    CAR_STATE *car;
    uint32 timer;
    uint32 state;
    sint32 x;
    sint32 y;
    sint32 z;

    FUNCTION_MARKER(0x80027FC4, "SLUS_006.10");
    car = &car_states[slot];
    timer = car_states[slot].motion_ticks;
    state = car_states[slot].motion_mode;
    car_states[slot].boundary_side = (uint8)(0);
    car_states[slot].motion_ticks = (uint16)(timer + 1);
    if (state == 0)
    {
        if (race_control.motion_gate_state == 0xFFFFFFFF)
            update_damped_motion(car);
        car_update_surface_alignment(car);
        resolve_end_boundary(slot);
        resolve_start_boundary(slot);
        return resolve_road_boundary(slot);
    }
    if (state != 1)
        return 1;
    sync_rotation(car);
    x = car_states[slot].angles.vx;
    y = car_states[slot].angles.vy;
    z = car_states[slot].angles.vz;
    car_states[slot].orientation.bits.x = (uint32)((uint32)x << 8);
    car_states[slot].orientation.bits.y = (uint32)((uint32)y << 8);
    car_states[slot].orientation.bits.z = (uint32)((uint32)z << 8);
    step_matrix_motion(car);
    resolve_end_contacts(slot, update_col_surface, all_contact_order);
    resolve_start_contacts(slot, update_col_surface, all_contact_order);
    return resolve_road_contacts(slot, update_col_surface, all_contact_order);
}

static uint32 update_rollover_state(CAR_STATE *car)
{
    uint32 pitch_bits;
    uint32 roll_bits;
    uint32 rate;
    uint32 velocity;
    uint32 coord;
    MATRIX matrix;
    MATRIX transpose;
    SVECTOR angles;
    uint32 values[9];
    uint32 idx;
    sint32 pitch;
    sint32 roll;
    sint32 step;

    FUNCTION_MARKER(0x80023E9C, "SLUS_006.10");
    pitch = car->angles.vx;
    roll = car->angles.vz;
    pitch_bits = ((uint32)pitch + 2048) & 0xFFF;
    pitch = (sint32)pitch_bits - 2048;
    rate = race_rules.collisions_disabled;
    roll_bits = ((uint32)roll + 2048) & 0xFFF;
    roll = (sint32)roll_bits - 2048;
    if (rate != 0)
    {
        if (pitch < -639)
            car->angular_velocity.bits.x = (uint32)(car->angular_velocity.bits.x + 512);
        if (pitch >= 640)
            car->angular_velocity.bits.x = (uint32)(car->angular_velocity.bits.x - 512);
        if (roll < -699)
            car->angular_velocity.bits.z = (uint32)(car->angular_velocity.bits.z + 512);
        if (roll >= 700)
            car->angular_velocity.bits.z = (uint32)(car->angular_velocity.bits.z - 512);
        if (pitch < -853)
        {
            car->angular_velocity.bits.x = (uint32)(0);
            car->angles.vx = (sint16)(uint16)((uint32)-853);
            car->orientation.bits.x = (uint32)(0xFFFCAB00);
        }
        if (pitch >= 854)
        {
            car->angular_velocity.bits.x = (uint32)(0);
            car->angles.vx = (sint16)(uint16)(853);
            car->orientation.bits.x = (uint32)(0x35500);
        }
        if (roll < -932)
        {
            car->angular_velocity.bits.z = (uint32)(0);
            car->angles.vz = (sint16)(uint16)((uint32)-932);
            car->orientation.bits.z = (uint32)(0xFFFC5C00);
        }
        if (roll < 933)
            return 1;
        car->angular_velocity.bits.z = (uint32)(0);
        car->angles.vz = (sint16)(uint16)(932);
        car->orientation.bits.z = (uint32)(0x3A400);
        return 932;
    }
    if (pitch_bits - 1195 < 0x6AB && roll_bits - 1116 < 0x749)
        return 1;
    velocity = car->angular_velocity.bits.x;
    rate = disp_state.time_step;
    step = (sint32)(velocity * rate) / 256;
    coord = car->orientation.bits.x - (uint32)step;
    angles.vx = (sint16)(uint16)((sint32)coord >> 8);
    velocity = car->angular_velocity.bits.y;
    step = (sint32)(velocity * rate) / 256;
    coord = car->orientation.bits.y - (uint32)step;
    angles.vy = (sint16)(uint16)((sint32)coord >> 8);
    velocity = car->angular_velocity.bits.z;
    step = (sint32)(velocity * rate) / 256;
    coord = car->orientation.bits.z - (uint32)step;
    angles.vz = (sint16)(uint16)((sint32)coord >> 8);
    model_build_rotation(&matrix, &angles);
    values[0] = (uint16)matrix.m[0][0];
    values[1] = (uint16)matrix.m[1][0];
    values[2] = (uint16)matrix.m[2][0];
    values[3] = (uint16)matrix.m[0][1];
    values[4] = (uint16)matrix.m[1][1];
    values[5] = (uint16)matrix.m[2][1];
    values[6] = (uint16)matrix.m[0][2];
    values[7] = (uint16)matrix.m[1][2];
    values[8] = (uint16)matrix.m[2][2];
    memset(&transpose, 0, sizeof(transpose));
    for (idx = 0; idx < 9; ++idx)
        transpose.m[idx / 3][idx % 3] = (sint16)(uint16)values[idx];
    MulMatrix2(&car->transform, &transpose);
    expand_matrix_q30(car->rotation_step_q30, &transpose);
    expand_matrix_q30(car->rotation_q30, &car->transform);
    car->motion_mode = (uint8)(1);
    car->motion_ticks = (uint16)(0);
    return 1;
}

static uint32 fit_suspension_angles(SVECTOR *angles, const VEC3 *pos, const uint32 *suspension)
{
    uint32 step;
    uint32 positive;
    uint32 negative;
    uint32 x[4];
    uint32 z[4];
    uint32 y[4];
    uint32 spring[4];
    sint32 dx;
    sint32 dz;
    sint32 height;
    uint32 distance;
    uint32 result = 0;

    FUNCTION_MARKER(0x8001DCF8, "SLUS_006.10");
    for (step = 0; step < 2; ++step)
    {
        positive = step == 0 ? 24 : 12;
        negative = step == 0 ? 12 : 24;
        x[0] = pos[0].bits.x;
        x[1] = pos[positive / 12].bits.x;
        x[2] = pos[negative / 12].bits.x;
        x[3] = pos[3].bits.x;
        dx = (sint32)(x[0] + x[1] - (x[2] + x[3])) >> 8;
        z[1] = pos[positive / 12].bits.z;
        spring[0] = suspension[0];
        z[0] = pos[0].bits.z;
        z[2] = pos[negative / 12].bits.z;
        z[3] = pos[3].bits.z;
        y[1] = pos[positive / 12].bits.y;
        y[0] = pos[0].bits.y;
        dz = (sint32)(z[0] + z[1] - (z[2] + z[3])) >> 8;
        spring[1] = suspension[(positive / 3) / 4];
        y[2] = pos[negative / 12].bits.y;
        spring[2] = suspension[(negative / 3) / 4];
        y[3] = pos[3].bits.y;
        spring[3] = suspension[3];
        height = (sint32)(y[0] + spring[0] + y[1] + spring[1] - (y[2] + spring[2] + y[3] + spring[3])) >> 8;
        distance = SquareRoot0((sint32)((uint32)dx * (uint32)dx + (uint32)dz * (uint32)dz));
        result = (uint32)fixed_angle_from_vector((sint32)(0 - (uint32)height), (sint32)distance);
        if (step == 0)
            angles->vz = (sint16)(uint16)result;
        else
            angles->vx = (sint16)(uint16)result;
    }
    return result;
}

static uint32 fit_suspension_pitch(SVECTOR *angles, const VEC3 *pos, const uint32 *suspension)
{
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;
    uint32 y[4];
    uint32 spring[4];
    sint32 x;
    sint32 z;
    sint32 height;
    uint32 distance;
    uint32 result;

    FUNCTION_MARKER(0x8001DE8C, "SLUS_006.10");
    a = pos[0].bits.x;
    b = pos[1].bits.x;
    c = pos[2].bits.x;
    d = pos[3].bits.x;
    x = (sint32)(a + b - (c + d)) >> 8;
    a = pos[0].bits.z;
    b = pos[1].bits.z;
    c = pos[2].bits.z;
    d = pos[3].bits.z;
    z = (sint32)(a + b - (c + d)) >> 8;
    y[0] = pos[0].bits.y;
    spring[0] = suspension[0];
    y[1] = pos[1].bits.y;
    y[2] = pos[2].bits.y;
    spring[2] = suspension[2];
    y[3] = pos[3].bits.y;
    spring[1] = suspension[1];
    spring[3] = suspension[3];
    height = (sint32)(y[0] + spring[0] + y[1] + spring[1] - (y[2] + spring[2] + y[3] + spring[3])) >> 8;
    distance = SquareRoot0((sint32)((uint32)x * (uint32)x + (uint32)z * (uint32)z));
    result = (uint32)fixed_angle_from_vector((sint32)(0 - (uint32)height), (sint32)distance);
    angles->vx = (sint16)(uint16)result;
    return result;
}

static uint32 fit_suspension_roll(SVECTOR *angles, const VEC3 *pos, const uint32 *suspension)
{
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;
    uint32 y[4];
    uint32 spring[4];
    sint32 x;
    sint32 z;
    sint32 height;
    uint32 distance;
    uint32 result;

    FUNCTION_MARKER(0x8001DF60, "SLUS_006.10");
    a = pos[0].bits.x;
    b = pos[2].bits.x;
    c = pos[1].bits.x;
    d = pos[3].bits.x;
    x = (sint32)(a + b - (c + d)) >> 8;
    a = pos[0].bits.z;
    b = pos[2].bits.z;
    c = pos[1].bits.z;
    d = pos[3].bits.z;
    z = (sint32)(a + b - (c + d)) >> 8;
    y[0] = pos[0].bits.y;
    spring[0] = suspension[0];
    y[2] = pos[2].bits.y;
    y[1] = pos[1].bits.y;
    spring[1] = suspension[1];
    y[3] = pos[3].bits.y;
    spring[2] = suspension[2];
    spring[3] = suspension[3];
    height = (sint32)(y[0] + spring[0] + y[2] + spring[2] - (y[1] + spring[1] + y[3] + spring[3])) >> 8;
    distance = SquareRoot0((sint32)((uint32)x * (uint32)x + (uint32)z * (uint32)z));
    result = (uint32)fixed_angle_from_vector((sint32)(0 - (uint32)height), (sint32)distance);
    angles->vz = (sint16)(uint16)result;
    return result;
}

static uint32 apply_ground_roll_feedback(CAR_STATE *car)
{
    uint32 mask, idx, val, count, sum, product, c, s, x, z;
    const SVECTOR *pointer;
    sint32 angle, scaled, lateral, clamped;
    uint32 contact_idx, n_force, lateral_force;
    const SVECTOR *surface_n;
    const SVECTOR *contact_cursor;
    uint32 heading_c, heading_s, contact_count, roll_correction;
    const uint32 *src_velocity;
    const uint32 *velocity;

    FUNCTION_MARKER(0x80020C88, "SLUS_006.10");
    mask = car->wheel_air_mask;
    switch (mask)
    {
        case 0:
        case 1:
        case 2:
        case 4:
        case 6:
        case 8:
        case 9:
        case 15:
            return 0x80020E88;
        default:
            break;
    }
    contact_cursor = car->suspension_rotated;
    surface_n = &car->ground_n;
    contact_idx = (0);
    contact_count = (0);
    n_force = (0);
    do
    {
        idx = contact_idx;
        if (((mask >> (idx & 31)) & 1) == 0)
        {
            pointer = contact_cursor;
            val = (uint32)pointer->vy;
            count = contact_count;
            sum = n_force;
            contact_count = (count + 1);
            n_force = (sum + val);
        }
        pointer = contact_cursor;
        ++idx;
        contact_idx = (idx);
        contact_cursor = pointer + 1;
    } while ((sint32)idx < 4);
    sum = n_force;
    count = contact_count;
    scaled = fixed_divide_checked((sint32)sum, (sint32)count);
    pointer = surface_n;
    val = (uint32)pointer->vy;
    product = (uint32)scaled * val;
    angle = car->angles.vy;
    n_force = (((sint32)product / 4096));
    src_velocity = car->velocity.u;
    c = (uint32)rcos(angle);
    angle = car->angles.vy;
    heading_c = (c);
    s = (uint32)rsin(angle);
    velocity = src_velocity;
    x = velocity[0];
    heading_s = (s);
    c = heading_c;
    product = (uint32)((sint32)x / 256) * c;
    z = velocity[2];
    product -= (uint32)((sint32)z / 256) * s;
    lateral = (sint32)product / 4096;
    lateral_force = (lateral);
    clamped = lateral < -512 ? -512 : (lateral > 512 ? 512 : lateral);
    val = n_force;
    product = (uint32)clamped * val;
    lateral_force = (clamped);
    val = car->angular_velocity.bits.z;
    scaled = (sint32)product / 64;
    roll_correction = (scaled);
    val -= (uint32)scaled;
    car->angular_velocity.bits.z = (uint32)(val);
    return val;
}

static uint32 integrate_suspension(CAR_STATE *car, const CAR_PHYSICS_PROFILE *desc, uint32 acceleration_x, uint32 acceleration_z)
{
    uint32 spring, damping, response, extra, node;
    uint32 *velocity_address, *pos_address;
    uint32 x, z, world_x, world_z, product, drive, velocity, resistance, tick, result;
    sint32 limit, lower_limit;

    FUNCTION_MARKER(0x80020428, "SLUS_006.10");
    response = (uint32)desc->suspension_response;
    spring = (uint32)desc->spring;
    damping = (uint32)desc->damping;
    extra = (uint32)desc->suspension_extra;
    limit = desc->suspension_limit;
    lower_limit = -limit;
    for (node = 0; node < 5; ++node)
    {
        if (node < 4)
        {
            velocity_address = &car->suspension_velocity[node];
            pos_address = &car->suspension_offset[node];
            world_x = car->pos.bits.x;
            x = car->suspension_transform[node].bits.x;
            world_z = car->pos.bits.z;
            x -= (uint32)((sint32)world_x / 256);
            z = car->suspension_transform[node].bits.z;
            z -= (uint32)((sint32)world_z / 256);
        }
        else
        {
            velocity_address = &car->body_spring_velocity;
            pos_address = &car->body_spring_offset;
            x = car->suspension_transform[0].bits.x;
            product = car->suspension_transform[1].bits.x;
            world_x = car->pos.bits.x;
            x = ((sint32)(x + product) / 2);
            z = car->suspension_transform[0].bits.z;
            product = car->suspension_transform[1].bits.z;
            world_z = car->pos.bits.z;
            x -= (uint32)((sint32)world_x / 256);
            z = (uint32)((sint32)(z + product) / 2) - (uint32)((sint32)world_z / 256);
        }
        drive = acceleration_x * x + acceleration_z * z;
        drive = ((sint32)drive / 256);
        product = drive * response;
        velocity = *velocity_address;
        velocity += (uint32)((sint32)product / 256);
        if (node < 4)
        {
            product = car->suspension_deviation[node] * extra;
            velocity += (uint32)((sint32)product / 256);
        }
        product = *pos_address * spring;
        resistance = ((sint32)product / 256);
        product = velocity * damping;
        resistance += (uint32)((sint32)product / 256);
        velocity -= resistance;
        if (node < 4 && velocity + 15 < 31)
            velocity = 0;
        tick = disp_state.time_step;
        product = velocity * tick;
        *velocity_address = (velocity);
        x = *pos_address + (uint32)((sint32)product / 256);
        *pos_address = (x);
        if (limit < (sint32)x)
        {
            *pos_address = (limit);
            *velocity_address = (0);
        }
        x = *pos_address;
        result = (sint32)x < lower_limit;
        if (result != 0)
        {
            *pos_address = (lower_limit);
            *velocity_address = (0);
        }
    }
    return result;
}

static uint32 apply_contact_impact(CAR_STATE *car, const VEC3 *point, sint32 angle, sint32 distance)
{
    uint32 c, s, separation, shift_x, shift_z, vx, vz, px, pz, yaw;
    uint32 sum, rounded_sum, n_x, n_z, dx, dz, arm, result, approach;
    uint32 inertia, val, denominator, impulse, new_yaw, new_n, friction;
    uint32 volume, slot;

    FUNCTION_MARKER(0x800287B8, "SLUS_006.10");
    c = (uint32)rcos(angle);
    s = (uint32)rsin(angle);
    separation = (uint32)distance - 4;
    shift_x = (uint32)fixed_divide_checked((sint32)((0u - separation) * s), 16);
    shift_z = (uint32)fixed_divide_checked((sint32)(separation * c), 16);
    vx = car->velocity.bits.x;
    vz = car->velocity.bits.z;
    px = car->pos.bits.x;
    pz = car->pos.bits.z;
    yaw = car->angular_velocity.bits.y;
    car->pos.bits.x = (uint32)(px + shift_x);
    pz += shift_z;
    car->pos.bits.z = (uint32)(pz);
    sum = vx * c + vz * s;
    rounded_sum = (sint32)sum < 0 ? sum + 4095 : sum;
    n_x = (uint32)fixed_divide_checked((sint32)sum, 4096);
    n_z = (uint32)fixed_divide_checked((sint32)(vz * c - vx * s), 4096);
    px = car->pos.bits.x;
    val = point->bits.x;
    dx = ((sint32)(px - val) >> 8);
    val = point->bits.z;
    dz = ((sint32)(pz - val) >> 8);
    arm = (uint32)fixed_divide_checked((sint32)(dx * c + dz * s), 4096);
    result = (uint32)fixed_divide_checked((sint32)yaw, 652);
    approach = n_z + arm * result;
    if ((sint32)approach < 0)
        return result;
    inertia = contact_inertia;
    val = (uint32)fixed_divide_checked((sint32)inertia, 256) * 4352;
    val = 0u - (uint32)fixed_divide_checked((sint32)val, 4096);
    val *= approach;
    denominator = (uint32)fixed_divide_checked((sint32)(inertia + arm * arm), 256);
    impulse = (uint32)fixed_divide_checked((sint32)val, (sint32)denominator);
    denominator = (uint32)fixed_divide_checked((sint32)inertia, 652);
    new_yaw = yaw + (uint32)fixed_divide_checked((sint32)(impulse * arm), (sint32)denominator);
    new_n = n_z + impulse;
    if ((sint32)n_x > 0)
    {
        val = approach * 2 + (uint32)((sint32)rounded_sum >> 18) + 2048;
        friction = (uint32)fixed_divide_checked((sint32)(val * 384), 2048);
        friction = n_x - friction;
        if ((sint32)friction < 0)
            friction = 0;
    }
    else
    {
        val = (uint32)fixed_divide_checked((sint32)n_x, 64) - 2048;
        val = approach * 2 - val;
        friction = (uint32)fixed_divide_checked((sint32)(val * 384), 2048);
        friction += n_x;
        if ((sint32)friction > 0)
            friction = 0;
    }
    car->angular_velocity.bits.y = (uint32)(new_yaw);
    val = friction * c - new_n * s;
    car->velocity.bits.x = (uint32)((uint32)fixed_divide_checked((sint32)val, 4096));
    val = new_n * c + friction * s;
    car->velocity.bits.z = (uint32)((uint32)fixed_divide_checked((sint32)val, 4096));
    if ((sint32)n_z >= 12801)
    {
        volume = n_z - 8192;
        if ((sint32)volume < 1024)
            volume = 1024;
        else if ((sint32)volume >= 2049)
            volume = 2048;
        audio_play_positioned((sint32)n_z > 102400 ? 5 : 7, -1, (sint32)volume, (sint32)n_z > 102400 ? 1600 : 4096, &car->pos, 0);
        race_apply_point_penalty(car, 1);
    }
    val = (sint32)n_x < 0 ? 0u - n_x : n_x;
    result = (sint32)n_z < 12801;
    if ((sint32)val >= 12801 || (sint32)n_z >= 12801)
    {
        slot = car->slot;
        result = (sint32)race_session.two_players < (sint32)slot;
        if (result == 0)
            return input_trigger_rumble((sint32)slot, 2048);
    }
    return result;
}

static uint32 apply_contact_forces(CAR_STATE *car)
{
    uint32 wire_word_8;
    MATRIX transpose = {0};
    uint32 mask;
    uint32 prev_mask;
    uint32 idx;
    const SVECTOR *vector;
    sint32 local_x;
    sint32 local_z;
    VEC3 rotated;
    uint32 gravity;
    sint32 force;
    sint32 a;
    sint32 b;
    uint32 product;
    uint32 dot;
    uint32 gravity_x = 0;
    uint32 gravity_z = 0;
    uint32 impact_x = 0;
    uint32 impact_y = 0;
    uint32 impact_z = 0;
    sint32 contacts = 0;
    sint32 impacts = 0;
    sint32 time;
    sint32 divisor;
    sint32 correction_x;
    sint32 correction_z;
    uint32 velocity_x;
    uint32 velocity_y;
    uint32 velocity_z;
    uint32 result;

    FUNCTION_MARKER(0x80023998, "SLUS_006.10");
    mask = car->wheel_air_mask;
    prev_mask = car->prev_wheel_air_mask;
    if (mask == 15)
        return 15;
    TransposeMatrix(&car->transform, &transpose);
    for (idx = 0; idx < 4; ++idx)
        xport_gte_write_control(idx, (memcpy(&wire_word_8, (const uint8 *)&transpose + idx * 4, sizeof(wire_word_8)), wire_word_8));
    xport_gte_write_control(4, xport_load_le16((const uint8 *)&transpose + 16));
    for (idx = 0; idx < 4; ++idx)
    {
        if (((mask >> idx) & 1) == 0)
        {
            local_x = car->suspension_local[idx].vx;
            local_z = car->suspension_local[idx].vz;
            vector = &car->wheel_normals[idx];
            xport_gte_write_data(0, ((uint32)(uint16)vector->vx | ((uint32)(uint16)vector->vy << 16)));
            xport_gte_write_data(1, ((uint32)(uint16)vector->vz | ((uint32)(uint16)vector->pad << 16)));
            xport_gte_execute(0x486012);
            rotated.bits.x = xport_gte_read_data(9);
            rotated.bits.y = xport_gte_read_data(10);
            rotated.bits.z = xport_gte_read_data(11);
            gravity = car_physics.gravity;
            force = (sint32)((uint32)(sint32)(sint16)rotated.bits.y * gravity) / 4096;
            gravity_x += (uint32)force * (uint32)local_z;
            ++contacts;
            gravity_z -= (uint32)force * (uint32)local_x;
            if (((prev_mask >> idx) & 1) != 0)
            {
                a = car->wheel_contact_velocity[idx].vx;
                b = vector->vx;
                dot = (uint32)a * (uint32)b;
                a = car->wheel_contact_velocity[idx].vy;
                b = vector->vy;
                product = (uint32)a * (uint32)b;
                a = car->wheel_contact_velocity[idx].vz;
                b = vector->vz;
                dot += product + (uint32)a * (uint32)b;
                force = (sint32)dot / 4096;
                ++impacts;
                impact_y += (uint32)(force / 2);
                impact_x -= ((uint32)force * (uint32)local_z) << 8;
                impact_z += ((uint32)force * (uint32)local_x) << 8;
            }
        }
    }
    gravity_x = (uint32)fixed_divide_checked((sint32)gravity_x, contacts);
    gravity_z = (uint32)fixed_divide_checked((sint32)gravity_z, contacts);
    if (impacts > 0)
    {
        impact_x = (uint32)fixed_divide_checked((sint32)impact_x, impacts);
        impact_y = (uint32)fixed_divide_checked((sint32)impact_y, impacts);
        impact_z = (uint32)fixed_divide_checked((sint32)impact_z, impacts);
    }
    time = (sint32)disp_state.time_step;
    divisor = fixed_divide_checked(307200, time);
    correction_x = fixed_divide_checked((sint32)(gravity_x + impact_x), divisor);
    divisor = fixed_divide_checked(140800, time);
    correction_z = fixed_divide_checked((sint32)(gravity_z + impact_z), divisor);
    gravity = car_physics.gravity;
    velocity_z = car->angular_velocity.bits.z;
    velocity_x = car->angular_velocity.bits.x;
    velocity_y = car->velocity.bits.y;
    car->angular_velocity.bits.x = (uint32)(velocity_x + (uint32)correction_x);
    result = velocity_y + impact_y + gravity;
    car->velocity.bits.y = (uint32)(result);
    car->angular_velocity.bits.z = (uint32)(velocity_z + (uint32)correction_z);
    if (impacts <= 0)
        return result;
    if (prev_mask >= 16)
        return prev_mask << 2;
    switch (prev_mask)
    {
        case 0:
        case 1:
        case 2:
        case 4:
        case 6:
        case 8:
        case 9:
            if (car->angular_velocity.x > 4000)
                car->angular_velocity.bits.x = (uint32)(4000);
            if (car->angular_velocity.x < -4000)
                car->angular_velocity.bits.x = (uint32)(-4000);
            /* Continue into the second rotation clamp */
        case 5:
        case 10:
            if (car->angular_velocity.z > 4000)
                car->angular_velocity.bits.z = (uint32)(4000);
            if (car->angular_velocity.z < -4000)
                car->angular_velocity.bits.z = (uint32)(-4000);
            return -4000;
        case 3:
        case 12:
            if (car->angular_velocity.x > 4000)
                car->angular_velocity.bits.x = (uint32)(4000);
            if (car->angular_velocity.x < -4000)
                car->angular_velocity.bits.x = (uint32)(-4000);
            return -4000;
        default:
            return 0x80023E68;
    }
}

typedef struct CAR_PLAYER_MOTION_WORK
{
    uint32 surface[5];
    uint32 wheel_force[3];
    uint32 rear_distance;
    uint32 front_distance;
    uint32 rear_force;
    uint32 wheel_c;
    uint32 wheel_s;
    uint32 body_c;
    uint32 body_s;
    uint32 steering_c;
    uint32 yaw_velocity;
    uint32 inertia;
    uint32 lateral_speed;
    uint32 forward_speed;
    uint32 wheel_grip[4];
    uint32 body_forward_speed;
    uint32 wheel_forward_speed;
    uint32 grip_coefficient;
    uint32 front_slip_residual;
    uint32 rear_slip_residual;
    uint32 slip_response;
} CAR_PLAYER_MOTION_WORK;

static uint32 motion_divide(uint32 val, sint32 divisor)
{
    return (uint32)fixed_divide_checked((sint32)val, divisor);
}

static uint32 motion_mul_div(uint32 val, uint32 factor, sint32 divisor)
{
    return ((sint32)(val * factor) / divisor);
}

static uint32 update_airborne_motion(CAR_STATE *car)
{
    uint32 c, s, val, prev, roll, wheel_delta0, wheel_delta1, tick, result;
    const CAR_PHYSICS_PROFILE *model;
    sint32 heading, vx, vz, lateral, pitch;

    FUNCTION_MARKER(0x80020A6C, "SLUS_006.10");
    model = car->physics_profile;
    car_update_free_engine_speed(car);
    integrate_suspension(car, model, 0, 0);
    heading = car->angles.vy;
    car->wheel_slip_flags = (uint8)(0);
    car->body_grip_excess = 0;
    car->steered_grip_excess = 0;
    c = (uint32)rcos(-heading);
    s = (uint32)rsin(-heading);
    vx = (sint16)(car->velocity.x >> 8);
    (void)car->velocity.y;
    vz = (sint16)(car->velocity.z >> 8);
    val = (uint32)(-vx) * s + (uint32)vz * c;
    lateral = (sint16)((sint32)val / 4096);
    pitch = ((car->angles.vx + 2048) & 4095) - 2048;
    if ((lateral > 32 && pitch < 128) || (lateral < -32 && pitch >= -127))
    {
        val = car->angular_velocity.bits.x;
        car->angular_velocity.bits.x = (uint32)(val + (uint32)(lateral / 4));
    }
    prev = car->angular_velocity.bits.x;
    roll = car->angular_velocity.bits.z;
    car->angular_velocity.bits.x = (uint32)(prev - (uint32)((sint32)prev / 16));
    val = car->steered_speed;
    tick = disp_state.time_step;
    wheel_delta0 = (uint32)((sint32)val >> 8) * tick;
    car->angular_velocity.bits.z = (uint32)(roll - (uint32)((sint32)roll / 16));
    val = car->forward_speed;
    wheel_delta1 = (uint32)((sint32)val >> 8) * tick;
    val = car->wheel_rotation[0];
    wheel_delta0 = ((sint32)wheel_delta0 / 256);
    car->wheel_rotation[0] = (uint16)(val + wheel_delta0);
    val = car->wheel_rotation[1];
    result = val + (uint32)((sint32)wheel_delta1 / 256);
    car->wheel_rotation[1] = (uint16)(result);
    return result;
}

static uint32 update_ai_motion(CAR_STATE *car)
{
    uint32 surface, val, other, drag, tick, vx, vz, yaw_velocity;
    const CAR_PHYSICS_PROFILE *model;
    uint32 front_len, rear_len, inertia, grip_strength, wheelbase, mass, offset;
    uint32 heading, steering, c, s, turn_c, turn_s, steer_c, steer_s;
    uint32 pedal, active, lateral, forward, steered_forward, grip_a, grip_b;
    uint32 torque_a, torque_b, pair_a, pair_b, bound, sel;
    uint32 denominator, coupling, momentum, term_a, term_b, term_c, term_d;
    uint32 numerator, force_a, force_b, limit_a, limit_b, norm, residual_a, residual_b;
    uint32 yaw_acceleration, acceleration_x, acceleration_z, step, prev;

    FUNCTION_MARKER(0x80022B1C, "SLUS_006.10");
    model = car->physics_profile;
    surface = track_surface_material(&car->track_location);
    car->surface_type = (uint8)(surface);
    apply_contact_gravity(car);
    val = (sint16)car->pedal;
    if ((sint32)val >= 32 && car->gear >= 2)
    {
        val = (uint32)surface_responses[surface].drag;
        other = (uint32)model->steering_response;
    }
    else
    {
        val = (uint32)surface_responses[surface].drag;
        other = (uint32)model->motion_coefficient_108;
    }
    drag = other + (val << 8);
    vx = car->velocity.bits.x;
    val = motion_divide(0 - vx, 256);
    val = motion_divide(val * drag, 4096);
    tick = disp_state.time_step;
    val = motion_divide(val * tick, 256);
    vz = car->velocity.bits.z;
    car->velocity.bits.x = (uint32)(vx + val);
    val = motion_divide(0 - vz, 256);
    val = motion_divide(val * drag, 4096);
    val = motion_divide(val * tick, 256);
    steering = car->steering_angle;
    heading = car->orientation.bits.y;
    vx = car->velocity.bits.x;
    yaw_velocity = car->angular_velocity.bits.y;
    vz += val;
    car->velocity.bits.z = (uint32)(vz);
    heading = ((sint32)heading >> 8);
    steering = motion_divide(steering, 256);
    front_len = (uint32)model->front_distance;
    pedal = (sint16)car->pedal;
    rear_len = (uint32)model->rear_distance;
    inertia = model->inertia;
    grip_strength = (uint32)model->grip_strength;
    c = (uint32)rcos((sint32)heading);
    s = (uint32)rsin((sint32)heading);
    turn_c = (uint32)rcos((sint32)(heading + steering));
    turn_s = (uint32)rsin((sint32)(heading + steering));
    steer_c = (uint32)rcos((sint32)steering);
    steer_s = (uint32)rsin((sint32)steering);
    lateral = motion_divide(vx * c - vz * s, 4096);
    forward = motion_divide(vx * s + vz * c, 4096);
    wheelbase = front_len + rear_len;
    val = motion_divide(rear_len << 8, (sint32)wheelbase);
    mass = model->mass;
    offset = car->body_spring_offset;
    grip_a = motion_divide(val * (mass - offset), (sint32)mass);
    val = motion_divide(front_len << 8, (sint32)wheelbase);
    grip_b = motion_divide(val * (mass + offset), (sint32)mass);
    if ((sint32)grip_a < 112)
        grip_a = 112;
    if ((sint32)grip_a > 160)
        grip_a = 160;
    if ((sint32)grip_b < 112)
        grip_b = 112;
    if ((sint32)grip_b > 160)
        grip_b = 160;
    val = motion_divide(vx * turn_s + vz * turn_c, 4096);
    other = motion_divide(yaw_velocity * front_len * steer_s, 2670592);
    active = car->braking;
    car->forward_speed = forward;
    steered_forward = val - other;
    car->steered_speed = steered_forward;
    if (active != 0 || pedal == 0)
    {
        if (pedal == 0)
            pedal = -32;
        car_update_engine_speed(car, model);
        val = (uint32)model->front_force_weight;
        torque_a = motion_divide(val * pedal, 256);
        val = (uint32)model->rear_force_weight;
        torque_b = motion_divide(val * pedal, 256);
        bound = 0 - torque_a;
        val = motion_divide(0 - steered_forward, 2);
        if ((sint32)val < (sint32)bound)
            bound = val;
        sel = (sint32)torque_a < (sint32)bound ? bound : torque_a;
        torque_a = motion_divide(sel, 2);
        bound = 0 - torque_b;
        val = motion_divide(0 - forward, 2);
        if ((sint32)val < (sint32)bound)
            bound = val;
        sel = (sint32)torque_b < (sint32)bound ? bound : torque_b;
        torque_b = motion_divide(sel, 2);
    }
    else
    {
        car_update_automatic_gears(car, model);
        car_update_engine_speed(car, model);
        torque_a = motion_divide(car_compute_drive_force(car, model), 4);
        torque_b = torque_a;
        val = (uint32)model->motion_coefficient_116;
        other = car->forward_speed;
        if ((sint32)(val << 8) < (sint32)other)
        {
            torque_a = 0;
            torque_b = 0;
        }
    }
    pair_a = torque_a + torque_a;
    pair_b = torque_b + torque_b;
    val = motion_divide(inertia + rear_len * rear_len, 1024);
    term_a = motion_divide(val * steer_s, 4096);
    term_a = motion_divide(term_a * steer_s, 4096);
    val = motion_divide(wheelbase * wheelbase, 1024);
    term_b = motion_divide(val * steer_c, 4096);
    term_b = motion_divide(term_b * steer_c, 4096);
    denominator = term_a + term_b;
    coupling = motion_divide(rear_len * front_len - inertia, 1024);
    term_a = motion_divide(coupling * pair_a, 4096);
    term_a = motion_divide(term_a * steer_s, 4096);
    term_a = motion_divide(term_a * steer_s, 4096);
    val = motion_divide(rear_len * yaw_velocity, 652) - lateral;
    other = motion_divide(inertia, 1024);
    term_b = motion_divide(other * val, 4096);
    term_b = motion_divide(term_b * steer_s, 4096);
    val = motion_divide(pair_a * steer_c, 4096) + pair_b + forward;
    term_c = motion_divide(coupling * val, 4096);
    term_c = motion_divide(term_c * steer_c, 4096);
    momentum = yaw_velocity * motion_divide(inertia, 652);
    val = motion_divide(momentum - front_len * lateral, 1024);
    term_d = motion_divide(val * wheelbase, 4096);
    term_d = motion_divide(term_d * steer_c, 4096);
    numerator = (term_a + term_b + term_c) * steer_s + term_d * steer_c;
    force_a = motion_divide(numerator, (sint32)denominator);
    val = motion_divide(0 - (rear_len * lateral + momentum), 1024);
    term_a = motion_divide(val * wheelbase, 4096) * steer_c;
    val = motion_divide(rear_len * 2 * front_len - inertia + front_len * front_len, 1024);
    term_b = motion_divide(val * pair_a, 4096) * steer_c;
    val = motion_divide(inertia + rear_len * rear_len, 1024);
    term_b += val * (forward + pair_b);
    term_b = motion_divide(term_b, 4096) * steer_s;
    force_b = motion_divide(term_a + term_b, (sint32)denominator);
    limit_b = motion_divide(grip_strength * grip_b, 256);
    car->wheel_slip_amount = (uint16)(0);
    residual_a = 0;
    val = motion_divide(force_a, 16);
    other = motion_divide(pair_b, 16);
    norm = (uint32)SquareRoot0((sint32)(val * val + other * other)) << 4;
    if ((sint32)limit_b < (sint32)norm)
    {
        val = motion_divide(limit_b << 8, (sint32)norm);
        residual_a = norm - limit_b;
        force_a = motion_divide(force_a * val, 256);
    }
    limit_a = motion_divide(grip_strength * grip_a, 256);
    residual_b = 0;
    car->body_grip_excess = residual_a;
    val = motion_divide(force_b, 16);
    other = motion_divide(pair_a, 16);
    norm = (uint32)SquareRoot0((sint32)(val * val + other * other)) << 4;
    if ((sint32)limit_a < (sint32)norm)
    {
        val = motion_divide(limit_a << 8, (sint32)norm);
        residual_b = norm - limit_a;
        force_b = motion_divide(force_b * val, 256);
    }
    val = motion_divide(force_a * c, 4096);
    car->steered_grip_excess = residual_b;
    other = motion_divide((0 - force_a) * s, 4096);
    acceleration_x = val + motion_divide(force_b * turn_c, 4096);
    acceleration_z = other + motion_divide((0 - force_b) * turn_s, 4096);
    val = motion_divide(front_len * steer_c, 4096);
    numerator = force_b * val - force_a * rear_len;
    yaw_acceleration = motion_divide(numerator, (sint32)motion_divide(inertia, 652));
    if ((sint32)yaw_acceleration > 1400)
        yaw_acceleration = 1400;
    if ((sint32)yaw_acceleration < -1400)
        yaw_acceleration = -1400;
    acceleration_x += motion_divide(pair_b * s, 4096);
    acceleration_x += motion_divide(pair_a * turn_s, 4096);
    acceleration_z += motion_divide(pair_b * c, 4096);
    acceleration_z += motion_divide(pair_a * turn_c, 4096);
    tick = disp_state.time_step;
    step = motion_divide(yaw_acceleration * tick, 256);
    prev = car->angular_velocity.bits.y;
    car->angular_velocity.bits.y = (uint32)(prev + step);
    step = motion_divide(acceleration_x * tick, 256);
    prev = car->velocity.bits.x;
    car->velocity.bits.x = (uint32)(prev + step);
    step = motion_divide(acceleration_z * tick, 256);
    val = ((sint32)car->steered_speed >> 8);
    prev = car->velocity.bits.z;
    car->velocity.bits.z = (uint32)(prev + step);
    step = motion_divide(val * tick, 256);
    val = ((sint32)car->forward_speed >> 8);
    prev = car->wheel_rotation[0];
    car->wheel_rotation[0] = (uint16)(prev + step);
    step = motion_divide(val * tick, 256);
    prev = car->wheel_rotation[1];
    car->wheel_rotation[1] = (uint16)(prev + step);
    if ((sint32)residual_a > 0)
    {
        prev = car->angular_velocity.bits.y;
        val = motion_divide(prev, 64);
        val = motion_divide(val * tick, 256);
        val = motion_divide(val * residual_a, 32768);
        if ((sint32)val > 512)
            val = 512;
        if ((sint32)val < -512)
            val = -512;
        car->angular_velocity.bits.y = (uint32)(prev - val);
    }
    return integrate_suspension(car, model, acceleration_x, acceleration_z);
}

static uint32 update_player_motion(CAR_STATE *car)
{
    CAR_PLAYER_MOTION_WORK work;
    uint32 wheel_force0 = 0;
    uint32 val, other, drag, tick, velocity_x, velocity_z;
    const CAR_PHYSICS_PROFILE *desc;
    uint32 steering, heading, pedal, drive, s_steer, distance;
    uint32 weight, balance, denominator, coefficient, surface, offset;
    uint32 rpm, gear, maximum, synthetic, forward, target, step;
    uint32 limit_speed, brake, front_force, rear_force, lateral_force, steered_force;
    uint32 inertia, front_distance, rear_distance, lateral, yaw;
    uint32 val0, val1, val2, moment, numerator, capacity, slip, root;
    uint32 residual, acceleration_x, acceleration_z, rotation, rotation_step, scaled_wheel_force0;

    FUNCTION_MARKER(0x80020E9C, "SLUS_006.10");
    desc = car->physics_profile;
    val = track_surface_material(&car->track_location);
    work.surface[0] = (uint32)(val);
    val = track_surface_material(&car->track_contacts[0]);
    work.surface[1] = (uint32)(val);
    val = track_surface_material(&car->track_contacts[1]);
    work.surface[2] = (uint32)(val);
    val = track_surface_material(&car->track_contacts[2]);
    work.surface[3] = (uint32)(val);
    val = track_surface_material(&car->track_contacts[3]);
    surface = work.surface[0];
    work.surface[4] = (uint32)(val);
    car->surface_type = (uint8)(surface);
    apply_contact_gravity(car);
    pedal = (sint16)car->pedal;
    surface = work.surface[0];
    if ((sint32)pedal < 32 || car->gear < 2)
    {
        val = (uint32)surface_responses[surface].drag;
        other = (uint32)desc->motion_coefficient_108;
    }
    else
    {
        val = (uint32)surface_responses[surface].drag;
        other = (uint32)desc->steering_response;
    }
    drag = other + (val << 8);
    val = car->velocity.bits.x;
    other = motion_mul_div((uint32)((sint32)(0 - val) / 256), drag, 4096);
    tick = disp_state.time_step;
    other = motion_mul_div(other, tick, 256);
    velocity_z = car->velocity.bits.z;
    car->velocity.bits.x = (uint32)(val + other);
    other = motion_mul_div((uint32)((sint32)(0 - velocity_z) / 256), drag, 4096);
    other = motion_mul_div(other, tick, 256);
    steering = car->steering_angle;
    heading = car->orientation.bits.y;
    velocity_x = car->velocity.bits.x;
    yaw = car->angular_velocity.bits.y;
    velocity_z += other;
    car->velocity.bits.z = (uint32)(velocity_z);
    heading = ((sint32)heading >> 8);
    work.yaw_velocity = (uint32)(yaw);
    front_distance = (uint32)desc->front_distance;
    pedal = (sint16)car->pedal;
    work.front_distance = (uint32)(front_distance);
    rear_distance = (uint32)desc->rear_distance;
    work.rear_distance = (uint32)(rear_distance);
    inertia = desc->inertia;
    work.inertia = (uint32)(inertia);
    val = (uint32)desc->grip_strength;
    work.grip_coefficient = (uint32)(val);
    drive = (uint32)desc->drive_type;
    val = (uint32)desc->slip_response;
    steering = ((sint32)steering / 256);
    work.slip_response = (uint32)(val);
    val = (uint32)rcos((sint32)heading);
    work.body_c = (uint32)(val);
    val = (uint32)rsin((sint32)heading);
    heading += steering;
    work.body_s = (uint32)(val);
    val = (uint32)rcos((sint32)heading);
    work.wheel_c = (uint32)(val);
    val = (uint32)rsin((sint32)heading);
    work.wheel_s = (uint32)(val);
    val = (uint32)rcos((sint32)steering);
    work.steering_c = (uint32)(val);
    s_steer = (uint32)rsin((sint32)steering);
    val0 = velocity_x * work.body_c;
    val1 = velocity_z * work.body_s;
    front_distance = work.front_distance;
    rear_distance = work.rear_distance;
    distance = front_distance + rear_distance;
    lateral = ((sint32)(val0 - val1) / 4096);
    val0 = velocity_x * work.body_s;
    val1 = velocity_z * work.body_c;
    work.lateral_speed = (uint32)(lateral);
    forward = ((sint32)(val0 + val1) / 4096);
    rear_distance = work.rear_distance;
    val = (uint32)fixed_divide_checked((sint32)(rear_distance << 8), (sint32)distance);
    weight = desc->mass;
    balance = car->body_spring_offset;
    val *= weight - balance;
    denominator = weight << 1;
    coefficient = (uint32)fixed_divide_checked((sint32)val, (sint32)denominator);
    surface = work.surface[1];
    val0 = coefficient * (uint32)surface_responses[surface].friction;
    work.forward_speed = (uint32)(forward);
    surface = work.surface[2];
    val1 = coefficient * (uint32)surface_responses[surface].friction;
    work.wheel_grip[0] = (uint32)((uint32)((sint32)val0 / 256));
    front_distance = work.front_distance;
    val = (uint32)fixed_divide_checked((sint32)(front_distance << 8), (sint32)distance);
    val *= weight + balance;
    coefficient = (uint32)fixed_divide_checked((sint32)val, (sint32)denominator);
    surface = work.surface[3];
    val0 = coefficient * (uint32)surface_responses[surface].friction;
    work.wheel_grip[1] = (uint32)((uint32)((sint32)val1 / 256));
    surface = work.surface[4];
    val1 = coefficient * (uint32)surface_responses[surface].friction;
    work.wheel_grip[2] = (uint32)((uint32)((sint32)val0 / 256));
    val = work.wheel_grip[0];
    work.wheel_grip[3] = (uint32)((uint32)((sint32)val1 / 256));
    for (offset = 0; offset < 4; ++offset)
    {
        uint32 *grip_address = &work.wheel_grip[offset];
        if (offset != 0)
            val = *grip_address;
        if ((sint32)val < 56)
        {
            *grip_address = (56);
            val = *grip_address;
        }
        else if (offset == 0)
            val = *grip_address;
        if ((sint32)val >= 81)
            *grip_address = (80);
    }
    if (car->handbrake != 0)
    {
        val = (uint32)desc->motion_coefficient_122;
        val0 = work.wheel_grip[2];
        val0 = motion_mul_div(val0, val, 256);
        val1 = work.wheel_grip[3];
        val1 = motion_mul_div(val1, val, 256);
        work.wheel_grip[2] = (uint32)(val0);
        work.wheel_grip[3] = (uint32)(val1);
    }
    val0 = velocity_x * work.body_s;
    val1 = velocity_z * work.body_c;
    forward = ((sint32)(val0 + val1) / 4096);
    val0 = velocity_x * work.wheel_s;
    val1 = velocity_z * work.wheel_c;
    work.body_forward_speed = (uint32)(forward);
    val0 = ((sint32)(val0 + val1) / 4096);
    yaw = work.yaw_velocity;
    val = work.front_distance;
    val1 = yaw * val * s_steer;
    brake = car->wheel_slip_flags;
    val0 -= (uint32)((sint32)val1 / 2670592);
    work.wheel_forward_speed = (uint32)(val0);
    if (drive != 1 && drive != 2 && drive != 3)
    {
        work.wheel_force[1] = (uint32)(0);
        work.wheel_force[2] = (uint32)(0);
        work.wheel_force[0] = (uint32)(0);
    }
    limit_speed = 0;
    if (brake != 0)
    {
        if (drive == 1 || drive == 2 || drive == 3)
        {
            if (drive == 1)
            {
                rpm = car->engine_speed;
                gear = car->gear;
                val = work.wheel_forward_speed;
                car->steered_speed = val;
                maximum = (uint32)desc->rpm_limit;
            }
            else if (drive == 2)
            {
                gear = car->gear;
                rpm = car->engine_speed;
                maximum = (uint32)desc->rpm_limit;
            }
            else
            {
                val0 = work.wheel_forward_speed;
                val1 = work.body_forward_speed;
                rpm = car->engine_speed;
                gear = car->gear;
                maximum = (uint32)desc->rpm_limit;
                forward = ((sint32)(val0 + val1) / 2);
            }
            synthetic = 0;
            if (gear != 1)
            {
                val = (rpm - 400) << 12;
                other = (uint32) * (gear == 8 ? &desc->upshift_rpm[0] : &desc->gear_ratios[gear]);
                val = ((sint32)val / 45);
                synthetic = (uint32)fixed_divide_checked((sint32)val, (sint32)other) << 8;
            }
            step = 200;
            if ((sint16)car->pedal > 0 && gear == 2)
            {
                if (drive != 3)
                    forward = (drive == 1 ? work.body_forward_speed : work.wheel_forward_speed);
                val = ((sint32)forward / 64);
                target = maximum - 1800;
                if ((sint32)val >= 0)
                    target += val;
            }
            else
            {
                if (car->braking != 0)
                    step *= 2;
                step *= 2;
                target = 0;
            }
            if ((sint32)target < (sint32)rpm)
            {
                rpm -= step;
                if ((sint32)rpm < (sint32)target)
                    rpm = target;
            }
            else if ((sint32)rpm < (sint32)(target - step * 4))
                rpm += step;
            else
                rpm += (uint32)((sint32)(target - rpm) / 4);
            car->engine_speed = rpm;
            if (drive == 2 || drive == 3)
                car->steered_speed = synthetic;
            if (drive == 2)
                synthetic = work.body_forward_speed;
            car->forward_speed = synthetic;
        }
        if (car->braking != 0)
        {
            val = (uint32)desc->front_force_weight;
            val = motion_mul_div(val, pedal, 256);
            other = work.wheel_forward_speed;
            val0 = 0 - val;
            val1 = 0 - other;
            if ((sint32)val1 < (sint32)val0)
                val0 = val1;
            if ((sint32)val < (sint32)val0)
                val = val0;
            wheel_force0 = ((sint32)val / 2);
            work.wheel_force[0] = (uint32)(wheel_force0);
            work.wheel_force[2] = (uint32)(0);
            work.wheel_force[1] = (uint32)(0);
        }
        else
            goto engine_force;
    }
    else
    {
        val = work.wheel_forward_speed;
        brake = car->braking;
        car->steered_speed = val;
        val = work.body_forward_speed;
        car->forward_speed = val;
        if (brake == 0 && pedal != 0)
        {
            if (car->automatic_transmission != 0)
                car_update_automatic_gears(car, desc);
            else
                car_apply_reverse_pedal(car);
            car_update_engine_speed(car, desc);
            limit_speed = 1;
            goto engine_force;
        }
        if (pedal == 0)
            pedal = -32;
        car_update_engine_speed(car, desc);
        val0 = (uint32)desc->front_force_weight;
        val0 = motion_mul_div(val0, pedal, 256);
        val1 = (uint32)desc->rear_force_weight;
        val1 = motion_mul_div(val1, pedal, 256);
        if (car->handbrake != 0)
        {
            wheel_force0 = 0;
            other = work.body_forward_speed;
            val = ((sint32)(0 - val1) / 2);
            other = 0 - other;
            if ((sint32)other < (sint32)val)
                val = other;
            work.wheel_force[0] = (uint32)(0);
        }
        else
        {
            other = work.wheel_forward_speed;
            val = 0 - val0;
            other = ((sint32)(0 - other) / 2);
            if ((sint32)other < (sint32)val)
                val = other;
            if ((sint32)val0 < (sint32)val)
                val0 = val;
            other = work.body_forward_speed;
            wheel_force0 = ((sint32)val0 / 2);
            val = 0 - val1;
            other = ((sint32)(0 - other) / 2);
            work.wheel_force[0] = (uint32)(wheel_force0);
            if ((sint32)other < (sint32)val)
                val = other;
        }
        if ((sint32)val1 < (sint32)val)
            val1 = val;
        work.wheel_force[1] = (uint32)((uint32)((sint32)val1 / 2));
        val = work.wheel_force[1];
        work.wheel_force[2] = (uint32)(val);
    }
    goto weighted_force;

engine_force:
    if (drive == 1)
    {
        val = car_compute_drive_force(car, desc);
        work.wheel_force[1] = (uint32)((uint32)((sint32)val / 2));
        other = work.wheel_force[1];
        wheel_force0 = 0;
        work.wheel_force[0] = (uint32)(0);
        work.wheel_force[2] = (uint32)(other);
    }
    else if (drive == 2)
    {
        work.wheel_force[2] = (uint32)(0);
        work.wheel_force[1] = (uint32)(0);
        val = car_compute_drive_force(car, desc);
        wheel_force0 = ((sint32)val / 2);
        work.wheel_force[0] = (uint32)(wheel_force0);
    }
    else if (drive == 3)
    {
        val = car_compute_drive_force(car, desc);
        wheel_force0 = ((sint32)val / 4);
        work.wheel_force[0] = (uint32)(wheel_force0);
        work.wheel_force[2] = (uint32)(wheel_force0);
        work.wheel_force[1] = (uint32)(wheel_force0);
    }
    if (limit_speed != 0)
    {
        val = (uint32)desc->motion_coefficient_116;
        other = car->forward_speed;
        if ((sint32)(val << 8) < (sint32)other)
        {
            wheel_force0 = 0;
            work.wheel_force[0] = (uint32)(0);
            work.wheel_force[2] = (uint32)(0);
            work.wheel_force[1] = (uint32)(0);
        }
    }

weighted_force:
    surface = work.surface[1];
    val = (uint32)surface_responses[surface].friction;
    val0 = motion_mul_div(wheel_force0, val, 256);
    scaled_wheel_force0 = val0;
    surface = work.surface[2];
    other = work.wheel_force[0];
    val = (uint32)surface_responses[surface].friction;
    val1 = motion_mul_div(other, val, 256);
    surface = work.surface[3];
    other = work.wheel_force[1];
    val = (uint32)surface_responses[surface].friction;
    val2 = motion_mul_div(other, val, 256);
    work.wheel_force[0] = (uint32)(val1);
    surface = work.surface[4];
    other = work.wheel_force[2];
    val = (uint32)surface_responses[surface].friction;
    val = motion_mul_div(other, val, 256);
    work.wheel_force[1] = (uint32)(val2);
    rear_distance = work.rear_distance;
    other = work.rear_distance;
    val1 = work.wheel_force[0];
    val2 = work.wheel_force[1];
    front_force = val0 + val1;
    inertia = work.inertia;
    work.wheel_force[2] = (uint32)(val);
    rear_force = val2 + val;
    work.rear_force = (uint32)(rear_force);
    val = inertia + rear_distance * other;
    val = ((sint32)val / 1024);
    val = motion_mul_div(val, s_steer, 4096);
    val = motion_mul_div(val, s_steer, 4096);
    other = ((sint32)(distance * distance) / 1024);
    coefficient = work.steering_c;
    other = motion_mul_div(other, coefficient, 4096);
    other = motion_mul_div(other, coefficient, 4096);
    rear_distance = work.rear_distance;
    front_distance = work.front_distance;
    inertia = work.inertia;
    denominator = val + other;
    balance = rear_distance * front_distance - inertia;
    balance = ((sint32)balance / 1024);
    val = motion_mul_div(balance, front_force, 4096);
    val = motion_mul_div(val, s_steer, 4096);
    val0 = motion_mul_div(val, s_steer, 4096);
    inertia = work.inertia;
    rear_distance = work.rear_distance;
    yaw = work.yaw_velocity;
    lateral = work.lateral_speed;
    val = (uint32)((sint32)(rear_distance * yaw) / 652) - lateral;
    other = ((sint32)inertia / 1024);
    val = motion_mul_div(other, val, 4096);
    val1 = motion_mul_div(val, s_steer, 4096);
    coefficient = work.steering_c;
    val = motion_mul_div(front_force, coefficient, 4096);
    other = work.rear_force;
    forward = work.forward_speed;
    val += other + forward;
    val = motion_mul_div(balance, val, 4096);
    coefficient = work.steering_c;
    val2 = motion_mul_div(val, coefficient, 4096);
    inertia = work.inertia;
    yaw = work.yaw_velocity;
    moment = yaw * (uint32)((sint32)inertia / 652);
    front_distance = work.front_distance;
    lateral = work.lateral_speed;
    numerator = (val0 + val1 + val2) * s_steer;
    val = ((sint32)(moment - front_distance * lateral) / 1024);
    val = motion_mul_div(val, distance, 4096);
    coefficient = work.steering_c;
    val = motion_mul_div(val, coefficient, 4096);
    rear_distance = work.rear_distance;
    lateral = work.lateral_speed;
    coefficient = work.steering_c;
    numerator += val * coefficient;
    other = 0 - (rear_distance * lateral + moment);
    lateral_force = (uint32)fixed_divide_checked((sint32)numerator, (sint32)denominator);
    val = ((sint32)other / 1024);
    val = motion_mul_div(val, distance, 4096);
    front_distance = work.front_distance;
    rear_distance = work.rear_distance;
    other = work.front_distance;
    inertia = work.inertia;
    balance = rear_distance * (front_distance << 1) - inertia + other * front_distance;
    coefficient = work.steering_c;
    val0 = val * coefficient;
    val = ((sint32)balance / 1024);
    val = motion_mul_div(val, front_force, 4096);
    rear_distance = work.rear_distance;
    other = work.rear_distance;
    inertia = work.inertia;
    coefficient = work.steering_c;
    val1 = val * coefficient;
    val = ((sint32)(inertia + rear_distance * other) / 1024);
    forward = work.forward_speed;
    rear_force = work.rear_force;
    val1 += val * (forward + rear_force);
    val = ((sint32)val1 / 4096);
    val1 = val * s_steer;
    val0 += val1;
    val = work.wheel_grip[2];
    other = work.wheel_grip[3];
    coefficient = work.grip_coefficient;
    capacity = motion_mul_div(coefficient, val + other, 256);
    steered_force = (uint32)fixed_divide_checked((sint32)val0, (sint32)denominator);
    brake = car->wheel_slip_flags;
    work.front_slip_residual = (uint32)(0);
    car->wheel_slip_amount = (uint16)(0);
    if ((brake & 1) != 0)
    {
        forward = work.body_forward_speed;
        val = car->forward_speed;
        if ((sint32)forward < 0 ? (sint32)val >= 0 : (sint32)(val - forward) >= 0)
        {
            other = work.body_forward_speed;
            val = car->forward_speed;
            if ((sint32)other >= 0)
                val -= other;
        }
        else
        {
            if ((sint32)forward < 0)
                forward = work.body_forward_speed;
            val = car->forward_speed;
            val = (sint32)forward < 0 ? 0 - val : forward - val;
        }
        slip = ((sint32)val / 256);
        if ((sint32)slip < 65)
            car->wheel_slip_flags = (uint8)(0);
        val = work.slip_response;
        val *= slip;
        car->wheel_slip_amount = (uint16)(slip);
        slip = ((sint32)val / 256);
        coefficient = ((sint32)lateral_force / 256);
        val = motion_mul_div(coefficient, lateral_force, 256) + slip * slip;
        root = (uint32)SquareRoot0((sint32)val);
        if ((sint32)coefficient < 0)
            coefficient = 0 - coefficient;
        root += 1;
        capacity = (uint32)fixed_divide_checked((sint32)(capacity * coefficient), (sint32)root);
        val = work.rear_force;
        val = (uint32)((sint32)val / 2) * slip;
        val = (uint32)fixed_divide_checked((sint32)val, (sint32)root);
        work.rear_force = (uint32)(val);
    }
    val0 = ((sint32)lateral_force / 16);
    val = work.rear_force;
    val1 = ((sint32)val / 16);
    root = (uint32)SquareRoot0((sint32)(val0 * val0 + val1 * val1)) << 4;
    if ((sint32)capacity < (sint32)root)
    {
        val = (uint32)fixed_divide_checked((sint32)(capacity << 8), (sint32)root);
        val *= lateral_force;
        work.front_slip_residual = (uint32)(root - capacity);
        lateral_force = ((sint32)val / 256);
    }
    val0 = work.wheel_grip[0];
    val1 = work.wheel_grip[1];
    coefficient = work.grip_coefficient;
    residual = work.front_slip_residual;
    car->body_grip_excess = residual;
    val = coefficient * (val0 + val1);
    work.rear_slip_residual = (uint32)(0);
    brake = car->wheel_slip_flags;
    capacity = ((sint32)val / 256);
    if ((brake & 2) != 0)
    {
        forward = work.wheel_forward_speed;
        val = car->steered_speed;
        if ((sint32)forward < 0 ? (sint32)val >= 0 : (sint32)(val - forward) >= 0)
        {
            other = work.wheel_forward_speed;
            val = car->steered_speed;
            if ((sint32)other >= 0)
                val -= other;
        }
        else
        {
            if ((sint32)forward < 0)
                forward = work.wheel_forward_speed;
            val = car->steered_speed;
            val = (sint32)forward < 0 ? 0 - val : forward - val;
        }
        slip = ((sint32)val / 256);
        if ((sint32)slip < 65)
            car->wheel_slip_flags = (uint8)(0);
        val = work.slip_response;
        val *= slip;
        car->wheel_slip_amount = (uint16)(slip);
        slip = ((sint32)val / 256);
        coefficient = ((sint32)steered_force / 256);
        val = motion_mul_div(coefficient, steered_force, 256) + slip * slip;
        root = (uint32)SquareRoot0((sint32)val);
        if ((sint32)coefficient < 0)
            coefficient = 0 - coefficient;
        root += 1;
        capacity = (uint32)fixed_divide_checked((sint32)(capacity * coefficient), (sint32)root);
        val = (uint32)((sint32)front_force / 2) * slip;
        front_force = (uint32)fixed_divide_checked((sint32)val, (sint32)root);
    }
    val0 = ((sint32)steered_force / 16);
    val1 = ((sint32)front_force / 16);
    root = (uint32)SquareRoot0((sint32)(val0 * val0 + val1 * val1)) << 4;
    if ((sint32)capacity < (sint32)root)
    {
        val = (uint32)fixed_divide_checked((sint32)(capacity << 8), (sint32)root);
        val *= steered_force;
        work.rear_slip_residual = (uint32)(root - capacity);
        steered_force = ((sint32)val / 256);
    }
    val = work.body_c;
    residual = work.rear_slip_residual;
    val0 = lateral_force * val;
    car->steered_grip_excess = residual;
    val0 = ((sint32)val0 / 4096);
    val = work.body_s;
    val1 = motion_mul_div(0 - lateral_force, val, 4096);
    val = work.wheel_c;
    val2 = motion_mul_div(steered_force, val, 4096);
    val = work.wheel_s;
    other = motion_mul_div(0 - steered_force, val, 4096);
    front_distance = work.front_distance;
    coefficient = work.steering_c;
    val = motion_mul_div(front_distance, coefficient, 4096);
    numerator = steered_force * val;
    rear_distance = work.rear_distance;
    val = work.wheel_force[0];
    coefficient = work.wheel_force[2];
    val += coefficient;
    coefficient = work.wheel_force[1];
    numerator -= lateral_force * rear_distance;
    val = val - scaled_wheel_force0 - coefficient;
    numerator += val * 500;
    inertia = work.inertia;
    val = ((sint32)inertia / 652);
    rotation = (uint32)fixed_divide_checked((sint32)numerator, (sint32)val);
    if ((sint32)rotation > 1400)
        rotation = 1400;
    if ((sint32)rotation < -1400)
        rotation = -1400;
    val = work.rear_force;
    coefficient = work.body_s;
    acceleration_x = val0 + val2 + motion_mul_div(val, coefficient, 4096);
    coefficient = work.wheel_s;
    acceleration_x += motion_mul_div(front_force, coefficient, 4096);
    val = work.rear_force;
    coefficient = work.body_c;
    acceleration_z = val1 + other + motion_mul_div(val, coefficient, 4096);
    coefficient = work.wheel_c;
    acceleration_z += motion_mul_div(front_force, coefficient, 4096);
    tick = disp_state.time_step;
    rotation_step = motion_mul_div(rotation, tick, 256);
    val = car->angular_velocity.bits.y;
    car->angular_velocity.bits.y = (uint32)(val + rotation_step);
    val = car->velocity.bits.x;
    car->velocity.bits.x = (uint32)(val + motion_mul_div(acceleration_x, tick, 256));
    val = car->steered_speed;
    rotation_step = motion_mul_div((uint32)((sint32)val >> 8), tick, 256);
    val = car->velocity.bits.z;
    car->velocity.bits.z = (uint32)(val + motion_mul_div(acceleration_z, tick, 256));
    val = car->wheel_rotation[0];
    brake = car->handbrake;
    car->wheel_rotation[0] = (uint16)(val + rotation_step);
    if (brake == 0)
    {
        val = car->forward_speed;
        rotation_step = motion_mul_div((uint32)((sint32)val >> 8), tick, 256);
        val = car->wheel_rotation[1];
        car->wheel_rotation[1] = (uint16)(val + rotation_step);
    }
    residual = work.front_slip_residual;
    if ((sint32)residual > 0)
    {
        rotation = car->angular_velocity.bits.y;
        tick = disp_state.time_step;
        val = motion_mul_div((uint32)((sint32)rotation / 64), tick, 256);
        residual = work.front_slip_residual;
        val = motion_mul_div(val, residual, 32768);
        if ((sint32)val > 512)
            val = 512;
        if ((sint32)val < -512)
            val = -512;
        car->angular_velocity.bits.y = (uint32)(rotation - val);
    }
    if (car->gear == 2)
    {
        val = (car->engine_speed - 400) << 12;
        other = (uint32)desc->gear_ratios[2];
        val = ((sint32)val / 45);
        val = (uint32)fixed_divide_checked((sint32)val, (sint32)other);
        other = work.body_forward_speed;
        if ((sint32)((val << 8) - other) > 76800 && (sint16)car->pedal >= 128)
            car->wheel_slip_flags = (uint8)((uint16)desc->drive_type);
    }
    if (car->velocity.bits.x + 63 < 127 && car->velocity.bits.z + 63 < 127 && car->angular_velocity.bits.y + 31 < 63)
    {
        car->velocity.bits.x = (uint32)(0);
        car->velocity.bits.z = (uint32)(0);
        car->angular_velocity.bits.y = (uint32)(0);
    }
    integrate_suspension(car, desc, acceleration_x, acceleration_z);
    return apply_ground_roll_feedback(car);
}

uint32 car_update_surface_alignment(CAR_STATE *car)
{
    MATRIX *matrix = &car->transform;
    SVECTOR *n = &car->ground_n;

    VEC3 *dst;
    uint32 a;
    uint32 b;
    uint32 coord;
    uint32 d;

    sint32 delta;
    uint32 idx;
    sint32 angle;
    sint32 c;
    sint32 s;
    sint32 nx;
    sint32 nz;
    sint16 rotated_x;
    sint16 rotated_z;
    uint16 rotated_y;
    uint32 product;
    sint32 adjust;
    sint32 result;
    uint32 radius;
    uint32 val;

    FUNCTION_MARKER(0x80027D58, "SLUS_006.10");
    a = car->orientation.bits.y;
    b = car->pos.bits.x;
    coord = car->pos.bits.y;
    d = car->pos.bits.z;
    car->angles.vy = (sint16)(uint16)((uint32)((sint32)a >> 8));
    car->transform.t[0] = (sint32)(uint32)((uint32)((sint32)b >> 8));
    car->transform.t[1] = (sint32)(uint32)((uint32)((sint32)coord >> 8));
    car->transform.t[2] = (sint32)(uint32)((uint32)((sint32)d >> 8));
    model_build_rotation(matrix, &car->angles);
    track_update_location(&car->track_location, &car->pos);
    track_surface_n_height(&car->track_location, &car->pos, n, &car->pos.bits.y);

    delta = (sint16)car_geometry_metrics[car - car_states].ground_offset;
    val = car->pos.bits.y;

    car->pos.bits.y = (uint32)(val + ((uint32)delta << 8));
    SetRotMatrix(matrix);
    SetTransMatrix(matrix);
    dst = car->body_contact_pos;
    for (idx = 0; idx < 4; ++idx)
    {
        val = ((uint32)(uint16)car_body_points[car - car_states][idx].vx | ((uint32)(uint16)car_body_points[car - car_states][idx].vy << 16));
        xport_gte_write_data(0, val);
        val = ((uint32)(uint16)car_body_points[car - car_states][idx].vz | ((uint32)(uint16)car_body_points[car - car_states][idx].pad << 16));
        xport_gte_write_data(1, val);
        xport_gte_execute(0x480012);
        dst->bits.x = (uint32)(xport_gte_read_data(25));
        dst->bits.y = (uint32)(xport_gte_read_data(26));
        dst->bits.z = (uint32)(xport_gte_read_data(27));
        a = dst->bits.x;
        b = dst->bits.y;
        coord = dst->bits.z;
        dst->bits.x = (uint32)(a << 8);
        dst->bits.y = (uint32)(b << 8);
        dst->bits.z = (uint32)(coord << 8);
        ++dst;
    }
    angle = car->angles.vy;
    c = rcos(angle);
    s = rsin(angle);
    nx = n->vx;
    nz = n->vz;
    product = (uint32)nx * (uint32)c - (uint32)nz * (uint32)s;
    rotated_x = ((sint32)product / 4096);
    rotated_y = (uint16)n->vy;
    nx = n->vx;
    nz = n->vz;
    product = (uint32)nx * (uint32)s + (uint32)nz * (uint32)c;
    rotated_z = ((sint32)product / 4096);
    result = fixed_angle_from_vector(-(sint32)rotated_z, -(sint32)(sint16)rotated_y);
    adjust = (sint32)car->suspension_offset[1] / 256;
    val = (uint32)result - (uint32)adjust;
    car->angles.vx = (sint16)(uint16)(val);
    product = (uint32)(sint32)(sint16)rotated_y * (uint32)(sint32)(sint16)rotated_y;
    product += (uint32)(sint32)rotated_z * (uint32)(sint32)rotated_z;
    radius = (uint32)SquareRoot0((sint32)product);
    result = fixed_angle_from_vector(rotated_x, (sint32)radius);
    adjust = (sint32)car->suspension_offset[0] / 256;
    val = (uint32)result + (uint32)adjust;
    car->angles.vz = (sint16)(uint16)(val);
    return val;
}

static uint32 pair_divide(uint32 val, sint32 divisor)
{
    return (uint32)fixed_divide_checked((sint32)val, divisor);
}

static uint32 resolve_pair_impact(CAR_STATE *car, CAR_STATE *other, const CAR_PAIR_CONTACT *contact, sint32 heading, uint32 fraction)
{
    MATRIX rotation;
    SVECTOR angle_storage[2];
    SVECTOR *angles;
    uint32 yaw_a, yaw_b, c, s, vx, vz, ax, az, bx, bz;
    uint32 x, z, offset_x, offset_z, rear, width, front;
    uint32 distance_x, distance_z, arm_a, arm_b, shift_x, shift_z;
    uint32 inertia, coefficient_a, coefficient_b, denominator, impulse;
    uint32 result, val, val0, rotation_a, rotation_b, mag, tick;
    uint32 factor, idx, slot, range, half, random;
    CAR_STATE *owner;
    CAR_STATE *owners[6];
    int side_hit;

    FUNCTION_MARKER(0x8002A244, "SLUS_006.10");
    yaw_a = car->angular_velocity.bits.y;
    yaw_b = other->angular_velocity.bits.y;
    c = (uint32)rcos(heading);
    s = (uint32)rsin(heading);
    vx = car->velocity.bits.x;
    vz = car->velocity.bits.z;
    ax = pair_divide(vx * c - vz * s, 4096);
    az = pair_divide(vx * s + vz * c, 4096);
    vx = other->velocity.bits.x;
    vz = other->velocity.bits.z;
    bx = pair_divide(vx * c - vz * s, 4096);
    bz = pair_divide(vx * s + vz * c, 4096);

    x = (uint32)contact->x;
    z = (uint32)contact->z;
    offset_x = (uint32)contact->offset_x;
    rear = (uint32)car_body_points[car - car_states][2].vz;
    offset_z = (uint32)contact->offset_z;
    width = (uint32)car_body_points[car - car_states][1].vx;
    front = (uint32)car_body_points[car - car_states][0].vz;
    val = (sint32)x < 0 ? 0u - x : x;
    distance_x = width - val;
    if ((sint32)distance_x < 0)
        distance_x = val - width;
    distance_z = (sint32)z > 0 ? front - z : z - rear;
    if ((sint32)distance_z < 0)
        distance_z = 0u - distance_z;
    side_hit = (sint32)distance_x < (sint32)distance_z;
    if (side_hit)
    {
        int positive = (sint32)(x - offset_x) > 0;
        shift_x = pair_divide(positive ? c : 0u - c, 2);
        shift_z = pair_divide(positive ? 0u - s : s, 2);
        arm_a = z;
        arm_b = offset_z;
    }
    else
    {
        int positive = (sint32)(z - offset_z) > 0;
        shift_x = pair_divide(positive ? s : 0u - s, 2);
        shift_z = pair_divide(positive ? c : 0u - c, 2);
        arm_a = x;
        arm_b = offset_x;
    }
    val = car->pos.bits.x;
    val0 = car->pos.bits.z;
    car->pos.bits.x = (uint32)(val - shift_x);
    car->pos.bits.z = (uint32)(val0 - shift_z);
    val = other->pos.bits.x;
    val0 = other->pos.bits.z;
    inertia = pair_inertia;
    other->pos.bits.x = (uint32)(val + shift_x);
    other->pos.bits.z = (uint32)(val0 + shift_z);

    coefficient_a = (sint16)car_geometry_metrics[car - car_states].impact_coefficient;
    coefficient_b = (sint16)car_geometry_metrics[other - car_states].impact_coefficient;
    val = (inertia + arm_b * arm_b) * coefficient_a;
    val0 = (inertia + arm_a * arm_a) * coefficient_b;
    denominator = pair_divide(val + val0, 256);
    val = pair_divide(inertia, 256) * 4352;
    impulse = pair_divide(val, (sint32)denominator);
    val = side_hit ? bx : bz;
    val += pair_divide(yaw_b * arm_b, 652);
    val -= side_hit ? ax : az;
    val -= pair_divide(yaw_a * arm_a, 652);
    impulse = pair_divide(impulse * val, 4096);
    result = impulse ^ (side_hit ? offset_x - x : offset_z - z);
    if ((sint32)result < 0)
        return result;
    denominator = pair_divide(inertia, 652);
    rotation_a = pair_divide(impulse * arm_a * coefficient_a, (sint32)denominator);
    rotation_b = pair_divide(impulse * arm_b * coefficient_b, (sint32)denominator);
    if (side_hit)
    {
        ax += impulse * coefficient_a;
        bx -= impulse * coefficient_b;
        yaw_a += rotation_a;
        yaw_b -= rotation_b;
    }
    else
    {
        az += impulse * coefficient_a;
        bz -= impulse * coefficient_b;
        yaw_a -= rotation_a;
        yaw_b += rotation_b;
    }
    mag = impulse * (coefficient_a + coefficient_b);
    if ((sint32)mag < 0)
        mag = 0u - mag;

    owners[0] = car;
    owners[1] = car;
    owners[2] = other;
    owners[3] = other;
    owners[4] = car;
    owners[5] = other;
    factor = 256u - fraction;
    for (idx = 0; idx < 6; ++idx)
    {
        owner = owners[idx];
        val = pair_divide(factor * (idx >= 4 ? owner->angular_velocity.bits.y : owner->velocity.u[(idx & 1) * 2]), 256);
        if (idx == 0 || idx == 5)
            tick = disp_state.time_step;
        val = pair_divide(val * tick, 256);
        val0 = idx >= 4 ? owner->orientation.bits.y : owner->pos.u[(idx & 1) * 2];
        if (idx >= 4)
            owner->orientation.bits.y = (uint32)(val0 - val);
        else
            owner->pos.u[(idx & 1) * 2] = val0 - val;
    }
    car->angular_velocity.bits.y = (uint32)(yaw_a);
    other->angular_velocity.bits.y = (uint32)(yaw_b);
    car->velocity.bits.x = (uint32)(pair_divide(ax * c + az * s, 4096));
    car->velocity.bits.z = (uint32)(pair_divide((0u - ax) * s + az * c, 4096));
    other->velocity.bits.x = (uint32)(pair_divide(bx * c + bz * s, 4096));
    other->velocity.bits.z = (uint32)(pair_divide((0u - bx) * s + bz * c, 4096));
    for (idx = 0; idx < 6; ++idx)
    {
        owner = owners[idx];
        val = pair_divide(factor * (idx >= 4 ? owner->angular_velocity.bits.y : owner->velocity.u[(idx & 1) * 2]), 256);
        if (idx == 2)
            tick = disp_state.time_step;
        val = pair_divide(val * tick, 256);
        val0 = idx >= 4 ? owner->orientation.bits.y : owner->pos.u[(idx & 1) * 2];
        if (idx >= 4)
            owner->orientation.bits.y = (uint32)(val0 + val);
        else
            owner->pos.u[(idx & 1) * 2] = val0 + val;
    }
    if (car->slot < 6)
        update_col_contacts(car);
    if (other->slot < 6)
        update_col_contacts(other);
    slot = car->slot;
    if (slot == chase_state.pursuer_slot)
    {
        slot = other->slot;
        if (slot < 6 && (race_session.cop_chase != 0 || race_session.multiplayer_mode == 3))
            chase_apply_contact(slot, (sint32)mag);
    }
    slot = other->slot;
    if (slot == chase_state.pursuer_slot)
    {
        slot = car->slot;
        if (slot >= 6)
            goto increment_gate;
        if (race_session.cop_chase != 0 || race_session.multiplayer_mode == 3)
            chase_apply_contact(slot, (sint32)mag);
    }
    if (car->slot < 6)
        goto damage;
increment_gate:
    if ((sint32)mag > 50000)
    {
        slot = car->slot;
        val = race_ai_cars[slot].recovery_state;
        if (val - 1 < 6)
            race_ai_cars[slot].recovery_state = val + 1;
    }
damage:
    for (idx = 0; idx < 2; ++idx)
    {
        owner = idx == 0 ? car : other;
        slot = owner->slot;
        if ((sint32)slot <= (sint32)race_session.two_players && (sint32)mag >= 12801)
        {
            int severe = (sint32)mag > 51200;
            input_trigger_rumble((sint32)slot, severe ? 1024 : 2048);
            race_apply_point_penalty(owner, severe ? 2 : 1);
            audio_play_positioned(severe ? 5 : 6, -1, 4096, severe ? 1600 : 600, &owner->pos, 0);
        }
    }
    result = (sint32)mag > 90000;
    if (result == 0)
        return result;
    result = race_rules.collisions_disabled;
    if (result != 0)
        return result;
    range = pair_divide(mag, 4);
    if ((sint32)range < 32767)
        range = 32767;
    for (idx = 0; idx < 2; ++idx)
    {
        owner = idx == 0 ? car : other;
        slot = owner->slot;
        if (slot < 6)
        {
            half = pair_divide(range, 2);
            random = car_random_next(slot) % range;
            val = owner->angular_velocity.bits.x;
            owner->angular_velocity.bits.x = (uint32)(val + random - half);
            random = car_random_next(slot) % range;
            val = owner->angular_velocity.bits.y;
            owner->angular_velocity.bits.y = (uint32)(val + random - half);
            random = car_random_next(slot) % (range * 2);
            val = owner->angular_velocity.bits.z;
            result = pair_divide(mag, 6);
            owner->velocity.bits.y = (uint32)(result);
            owner->angular_velocity.bits.z = (uint32)(val + random - range);
        }
        else
        {
            val0 = pair_divide(range, 256);
            half = val0 / 2;
            angles = &angle_storage[idx];
            random = car_random_next(slot) % val0;
            angles->vx = (sint16)(uint16)(random - half);
            slot = owner->slot;
            random = car_random_next(slot) % val0;
            angles->vy = (sint16)(uint16)(random - half);
            slot = owner->slot;
            random = car_random_next(slot) % val0;
            angles->vz = (sint16)(uint16)(random - half);
            model_build_rotation(&rotation, angles);
            expand_matrix_q30(owner->rotation_step_q30, &rotation);
            expand_matrix_q30(owner->rotation_q30, &owner->transform);
            owner->motion_mode = (uint8)(1);
            owner->motion_ticks = (uint16)(0);
            result = pair_divide(mag, 6);
            owner->velocity.bits.y = (uint32)(result);
        }
    }
    return result;
}

static uint32 find_pair_contacts(CAR_STATE *car0, CAR_STATE *car1, const VEC3 *pos, const uint32 headings[2], CAR_PAIR_CONTACT contacts[8])
{
    uint32 heading_a = headings[0];
    uint32 heading_b = headings[1];
    uint32 pos_ax = pos[0].bits.x;
    uint32 pos_bz = pos[1].bits.z;
    uint32 pos_bx = pos[1].bits.x;
    uint32 pos_az = pos[0].bits.z;
    sint32 angle = (sint32)((heading_a - heading_b) << 8) >> 16;
    uint32 dx = ((sint32)(pos_bx - pos_ax) >> 8);
    uint32 dz = ((sint32)(pos_bz - pos_az) >> 8);
    uint32 c = (uint32)rcos(angle);
    uint32 s = (uint32)rsin(angle);
    uint32 c_a = (uint32)rcos((sint32)headings[0] >> 8);
    uint32 s_a = (uint32)rsin((sint32)headings[0] >> 8);
    uint32 c_b = (uint32)rcos((sint32)headings[1] >> 8);
    uint32 s_b = (uint32)rsin((sint32)headings[1] >> 8);

    uint32 width_a = (uint32)car_body_points[car0 - car_states][1].vx;
    uint32 front_a = (uint32)car_body_points[car0 - car_states][0].vz;
    uint32 rear_a = (uint32)car_body_points[car0 - car_states][2].vz;

    uint32 width_b = (uint32)car_body_points[car1 - car_states][1].vx;
    uint32 front_b = (uint32)car_body_points[car1 - car_states][0].vz;
    uint32 rear_b = (uint32)car_body_points[car1 - car_states][2].vz;
    uint32 aw_cos = ((sint32)(c * width_a) >> 12);
    uint32 aw_sin = ((sint32)((0u - s) * width_a) >> 12);
    uint32 af_cos = ((sint32)(c * front_a) >> 12);
    uint32 af_sin = ((sint32)((0u - s) * front_a) >> 12);
    uint32 ar_cos = ((sint32)(c * (0u - rear_a)) >> 12);
    uint32 ar_sin = ((sint32)((0u - s) * (0u - rear_a)) >> 12);
    uint32 bw_cos = ((sint32)(c * width_b) >> 12);
    uint32 bw_sin = ((sint32)(s * width_b) >> 12);
    uint32 bf_cos = ((sint32)(c * front_b) >> 12);
    uint32 bf_sin = ((sint32)(s * front_b) >> 12);
    uint32 br_cos = ((sint32)(c * (0u - rear_b)) >> 12);
    uint32 br_sin = ((sint32)(s * (0u - rear_b)) >> 12);
    VEC2_XZ corner[8];
    uint32 translation_x = ((sint32)(dx * c_a - dz * s_a) >> 12);
    uint32 translation_z = ((sint32)(dx * s_a + dz * c_a) >> 12);
    uint32 width = width_a, front = front_a, rear = rear_a;
    uint32 mask = 0;
    uint32 side, point;

    FUNCTION_MARKER(0x8002B3E8, "SLUS_006.10");
    corner[0].bits.x = 0u - bw_cos - bf_sin;
    corner[0].bits.z = bf_cos - bw_sin;
    corner[1].bits.x = bw_cos - bf_sin;
    corner[1].bits.z = bw_sin + bf_cos;
    corner[2].bits.x = br_sin - bw_cos;
    corner[2].bits.z = 0u - bw_sin - br_cos;
    corner[3].bits.x = bw_cos + br_sin;
    corner[3].bits.z = bw_sin - br_cos;
    corner[4].bits.x = 0u - aw_cos - af_sin;
    corner[4].bits.z = af_cos - aw_sin;
    corner[5].bits.x = aw_cos - af_sin;
    corner[5].bits.z = aw_sin + af_cos;
    corner[6].bits.x = ar_sin - aw_cos;
    corner[6].bits.z = 0u - aw_sin - ar_cos;
    corner[7].bits.x = aw_cos + ar_sin;
    corner[7].bits.z = aw_sin - ar_cos;
    for (side = 0; side < 2; ++side)
    {
        if (side != 0)
        {
            translation_x = ((sint32)((0u - dx) * c_b + dz * s_b) >> 12);
            translation_z = ((sint32)((0u - dx) * s_b - dz * c_b) >> 12);
            width = width_b;
            front = front_b;
            rear = rear_b;
        }
        for (point = side * 4; point < side * 4 + 4; ++point)
        {
            uint32 x = corner[point].bits.x + translation_x;
            uint32 z = corner[point].bits.z + translation_z;
            if ((sint32)x <= (sint32)width && (sint32)x >= (sint32)(0u - width) && (sint32)z <= (sint32)front && (sint32)z >= (sint32)rear)
            {
                CAR_PAIR_CONTACT *output = &contacts[point];
                mask |= 1u << point;
                output->offset_x = (sint16)(uint16)(x - translation_x);
                output->x = (sint16)(uint16)x;
                output->z = (sint16)(uint16)z;
                output->offset_z = (sint16)(uint16)(z - translation_z);
            }
        }
    }
    return mask;
}

static uint32 resolve_pair_contacts(uint32 slot0, uint32 slot1)
{
    const CAR_PAIR_BOX *box0;
    const CAR_PAIR_BOX *box1;
    uint32 val;
    uint32 other_val;
    CAR_STATE *car0;
    CAR_STATE *car1;
    VEC3 pos[2];
    uint32 headings[2];
    CAR_PAIR_CONTACT contacts[8];
    uint32 *pose_components[6] = {&pos[0].bits.x, &pos[0].bits.z, &headings[0], &pos[1].bits.x, &pos[1].bits.z, &headings[1]};
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 mask;
    uint32 result;
    uint32 time_step;
    uint32 deltas[6];
    uint32 trial[6];
    uint32 fraction;
    uint32 step;
    uint32 i;
    uint32 idx;
    uint32 bit;
    uint32 corner;
    CAR_STATE *car;
    CAR_STATE *other;
    uint32 heading;

    FUNCTION_MARKER(0x8002BA04, "SLUS_006.10");
    box0 = &pair_boxes[slot0];
    box1 = &pair_boxes[slot1];
    val = box0->min_x;
    other_val = box1->max_x;
    if ((sint32)val >= (sint32)other_val)
        return 0;
    other_val = box0->max_x;
    val = box1->min_x;
    if ((sint32)val >= (sint32)other_val)
        return 0;
    val = box0->min_z;
    other_val = box1->max_z;
    if ((sint32)val >= (sint32)other_val)
        return 0;
    other_val = box0->max_z;
    val = box1->min_z;
    if ((sint32)val >= (sint32)other_val)
        return slot0 * 8;
    car0 = &car_states[slot0];
    car1 = &car_states[slot1];
    x = car0->pos.bits.x;
    y = car0->pos.bits.y;
    z = car0->pos.bits.z;
    pos[0].bits.x = x;
    pos[0].bits.y = y;
    pos[0].bits.z = z;
    x = car1->pos.bits.x;
    y = car1->pos.bits.y;
    z = car1->pos.bits.z;
    pos[1].bits.x = x;
    pos[1].bits.y = y;
    pos[1].bits.z = z;
    val = car0->orientation.bits.y;
    headings[0] = val;
    val = car1->orientation.bits.y;
    headings[1] = val;
    mask = find_pair_contacts(car0, car1, pos, headings, contacts);
    if (mask == 0)
        return 0;
    fraction = 128;
    step = 128;
    val = car0->velocity.bits.x;
    time_step = disp_state.time_step;
    deltas[0] = car_truncate_shift((uint32)((sint32)val / 2) * time_step, 8);
    val = car0->velocity.bits.z;
    deltas[1] = car_truncate_shift((uint32)((sint32)val / 2) * time_step, 8);
    val = car1->velocity.bits.x;
    deltas[3] = car_truncate_shift((uint32)((sint32)val / 2) * time_step, 8);
    val = car1->velocity.bits.z;
    deltas[4] = car_truncate_shift((uint32)((sint32)val / 2) * time_step, 8);
    val = car0->angular_velocity.bits.y;
    deltas[2] = car_truncate_shift((uint32)((sint32)val / 2) * time_step, 8);
    val = car1->angular_velocity.bits.y;
    deltas[5] = car_truncate_shift((uint32)((sint32)val / 2) * time_step, 8);
    for (idx = 0; idx < 6; ++idx)
        trial[idx] = *pose_components[idx];
    for (idx = 0; idx < 6; ++idx)
        *pose_components[idx] = trial[idx] - deltas[idx];
    for (i = 0; i < 7; ++i)
    {
        step /= 2;
        for (idx = 0; idx < 6; ++idx)
        {
            val = deltas[idx];
            deltas[idx] = ((sint32)val / 2);
        }
        result = find_pair_contacts(car0, car1, pos, headings, contacts);
        for (idx = 0; idx < 6; ++idx)
        {
            trial[idx] = *pose_components[idx];
        }
        if (result != 0)
        {
            fraction -= step;
            mask = result;
        }
        else
            fraction += step;
        for (idx = 0; idx < 6; ++idx)
            *pose_components[idx] = result != 0 ? trial[idx] - deltas[idx] : trial[idx] + deltas[idx];
    }
    fraction -= 16;
    if (mask != 0 && mask <= 128 && (mask & (mask - 1)) == 0)
    {
        for (idx = 0; (1u << idx) != mask; ++idx)
            ;
        car = idx < 4 ? car0 : car1;
        other = idx < 4 ? car1 : car0;
        corner = 0x80 + (idx & 3) * 12;
        effect_try_col_line(other, &other->body_contact_pos[(corner - 0x80) / 12]);
        heading = headings[idx < 4 ? 0 : 1];
        return resolve_pair_impact(car, other, &contacts[idx], (sint32)heading >> 8, fraction);
    }
    bit = 1;
    for (idx = 0; idx < 8; ++idx)
    {
        if ((mask & bit) != 0)
        {
            car = idx < 4 ? car0 : car1;
            other = idx < 4 ? car1 : car0;
            heading = headings[idx < 4 ? 0 : 1];
            resolve_pair_impact(car, other, &contacts[idx], (sint32)heading >> 8, fraction);
        }
        bit <<= 1;
    }
    return 0;
}

static uint32 resolve_sphere_impact(uint32 slot0, uint32 slot1)
{
    CAR_STATE *car0 = &car_states[slot0];
    CAR_STATE *car1 = &car_states[slot1];
    SVECTOR dir = {0};
    VEC3_16 components, relative;
    uint32 idx, val, prev, sq = 0, result;
    sint32 radius0, radius1, approach;

    FUNCTION_MARKER(0x8002C074, "SLUS_006.10");
    for (idx = 0; idx < 3; ++idx)
    {
        val = car1->pos.u[idx];
        prev = car0->pos.u[idx];
        components.v[idx] = (sint16)((sint32)(val - prev) >> 8);
        sq += (uint32)((sint32)components.v[idx] * components.v[idx]);
    }
    dir.vx = components.x;
    dir.vy = components.y;
    dir.vz = components.z;
    val = SquareRoot0((sint32)sq);

    radius0 = (sint16)car_geometry_metrics[slot0].col_radius;
    radius1 = (sint16)car_geometry_metrics[slot1].col_radius;
    result = (sint32)val < 3 * (radius0 + radius1) / 4;
    if (result == 0)
        return result;
    VectorNormalSS(&dir, &dir);
    components.x = dir.vx;
    components.y = dir.vy;
    components.z = dir.vz;
    result = 0;
    for (idx = 0; idx < 3; ++idx)
    {
        val = car1->velocity.u[idx];
        prev = car0->velocity.u[idx];
        relative.v[idx] = (sint16)((sint32)(val - prev) >> 8);
        result += (uint32)((sint32)components.v[idx] * relative.v[idx]);
    }
    approach = (sint32)result >> 12;
    if (approach >= 0)
        return result;
    for (idx = 0; idx < 6; ++idx)
    {
        CAR_STATE *owner = idx < 3 ? car0 : car1;
        uint32 axis = idx % 3;
        val = (uint32)(sint32)components.v[axis] * (uint32)approach;
        prev = owner->velocity.u[axis];
        result = ((sint32)(val + (val >> 31)) >> 5);
        owner->velocity.u[axis] = (uint32)(idx < 3 ? prev + result : prev - result);
    }
    return result;
}

uint32 car_update_pair_cols(void)
{
    uint32 count;
    uint32 idx;
    CAR_STATE *car;
    CAR_PAIR_BOX *entry;

    uint32 coord;
    uint32 radius;
    uint32 bucket;
    uint32 next;
    uint32 car_offset;
    uint32 grp;
    uint32 candidate;
    uint32 steps;
    uint32 candidate_offset;
    uint32 special;

    FUNCTION_MARKER(0x8002C30C, "SLUS_006.10");
    count = race_control.car_count;
    idx = 0;
    car = &car_states[0];
    entry = pair_boxes;
    while ((sint32)idx < (sint32)count)
    {
        coord = car->pos.bits.x;
        radius = (sint16)car_geometry_metrics[idx].col_radius;
        entry->min_x = (uint32)((sint32)coord >> 8) - radius;
        coord = car->pos.bits.z;
        entry->min_z = (uint32)((sint32)coord >> 8) - radius;
        coord = car->pos.bits.x;
        entry->max_x = (uint32)((sint32)coord >> 8) + radius;
        coord = car->pos.bits.z;
        entry->max_z = (uint32)((sint32)coord >> 8) + radius;
        bucket = (uint32)((sint16)car->track_location.node >> 2) + 1;
        XASSERT(!(bucket >= 1024));
        next = pair_heads[bucket];
        entry->next = (uint8)next;
        pair_heads[bucket] = (uint8)idx;
        count = race_control.car_count;
        ++car;
        ++idx;
        ++entry;
    }
    count = race_control.car_count;
    idx = 0;
    car_offset = 0;
    if ((sint32)count > 0)
    {
        do
        {
            bucket = ((sint16)car_states[(car_offset) / 864u].track_location.node >> 2);
            for (grp = 0; grp < 3; ++grp)
            {
                XASSERT(!(bucket >= 1024));
                candidate = pair_heads[bucket];
                steps = 0;
                while (candidate != 255)
                {
                    if ((sint32)idx < (sint32)candidate && (sint32)candidate < (sint32)race_control.car_count)
                    {
                        special = car_states[car_offset / 864u].motion_mode;
                        if (special == 0)
                            special = car_states[car_offset / 864u].wheel_air_mask >= 15;
                        candidate_offset = candidate * 864;
                        if (special == 0)
                            special = car_states[candidate_offset / 864u].motion_mode;
                        if (special == 0)
                            special = car_states[candidate_offset / 864u].wheel_air_mask >= 15;
                        if (special != 0)
                            resolve_sphere_impact(idx, candidate);
                        else
                            resolve_pair_contacts(idx, candidate);
                    }
                    XASSERT(!(candidate >= 14));
                    candidate = pair_boxes[candidate].next;
                    ++steps;
                    if (steps >= 17)
                    {
                        ;
                        break;
                    }
                }
                ++bucket;
            }
            count = race_control.car_count;
            ++idx;
            car_offset += 864;
        } while ((sint32)idx < (sint32)count);
        count = race_control.car_count;
    }
    idx = 0;
    car_offset = 0;
    entry = pair_boxes;
    if ((sint32)count <= 0)
        return count;
    do
    {
        bucket = ((sint16)car_states[(car_offset) / 864u].track_location.node >> 2);
        XASSERT(!(bucket + 1u >= 1024));
        pair_heads[bucket + 1u] = 255;
        entry->next = 255;
        count = race_control.car_count;
        car_offset += 864;
        ++idx;
        ++entry;
    } while ((sint32)idx < (sint32)count);
    return 0;
}

uint32 car_init_pair_cols(void)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 idx;

    uint32 val;
    uint32 result;

    FUNCTION_MARKER(0x80028464, "SLUS_006.10");
    for (idx = 0; idx < 1024; ++idx)
        pair_heads[1023 - idx] = 255;
    for (idx = 0; idx < 14; ++idx)
    {
        pair_boxes[idx].next = 255;
    }
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    val = (sint16)xport_load_le16(level_header + 8);
    result = (sint16)xport_load_le16(level_header + 10);
    col_bounds.first_node = val;
    col_bounds.last_node = result;
    return result;
}

static uint32 apply_contact_gravity(CAR_STATE *car)
{
    VECTOR dir0 = {0}, dir1 = {0}, n = {0};
    VEC3 vectors[2];
    uint32 basis, axis, a, b, c, d, strength, product, result;

    FUNCTION_MARKER(0x800250F0, "SLUS_006.10");
    for (basis = 0; basis < 2; ++basis)
    {
        for (axis = 0; axis < 3; ++axis)
        {
            a = car->body_contact_pos[0].u[axis];
            b = car->body_contact_pos[basis == 0 ? 2 : 1].u[axis];
            c = car->body_contact_pos[basis == 0 ? 1 : 2].u[axis];
            d = car->body_contact_pos[3].u[axis];
            vectors[basis].v[axis] = (sint32)(a + b - (c + d)) >> 8;
        }
    }
    dir0.vx = vectors[0].x;
    dir0.vy = vectors[0].y;
    dir0.vz = vectors[0].z;
    dir1.vx = vectors[1].x;
    dir1.vy = vectors[1].y;
    dir1.vz = vectors[1].z;
    VectorNormal(&dir0, &dir0);
    VectorNormal(&dir1, &dir1);
    OuterProduct12(&dir0, &dir1, &n);
    strength = car_physics.gravity;
    product = (uint32)n.vx * strength;
    result = car->velocity.bits.x + (uint32)((sint32)product / 8192);
    car->velocity.bits.x = (uint32)(result);
    product = (uint32)n.vz * strength;
    result = car->velocity.bits.z + (uint32)((sint32)product / 8192);
    car->velocity.bits.z = (uint32)(result);
    return result;
}

static uint32 update_damped_motion(CAR_STATE *car)
{
    uint32 x;
    uint32 z;
    uint32 damped;
    uint32 velocity_x;
    uint32 velocity_z;
    uint32 yaw_rate;
    uint32 heading;
    uint32 steering;
    uint32 c;
    uint32 s;
    uint32 turned_c;
    uint32 turned_s;
    uint32 steering_c;
    uint32 steering_s;
    uint32 lateral;
    uint32 longitudinal;
    uint32 wheel;
    uint32 denominator;
    uint32 side_force;
    uint32 yaw_force;
    uint32 rolling;
    uint32 val0;
    uint32 val1;
    uint32 val2;
    uint32 front;
    uint32 rear;
    uint32 yaw_change;
    uint32 x_change;
    uint32 z_change;
    uint32 pos;
    uint32 rate;
    uint32 new_yaw;
    uint32 new_x;
    uint32 new_z;
    uint32 checked_pos;
    uint32 time_step;
    uint32 angle;

    FUNCTION_MARKER(0x8002765C, "SLUS_006.10");
    x = car->velocity.bits.x;
    damped = car_truncate_shift(0 - x, 8);
    z = car->velocity.bits.z;
    car->velocity.bits.x = (uint32)(x + damped);
    damped = car_truncate_shift(0 - z, 8);
    steering = car->steering_angle;
    heading = car->orientation.bits.y;
    velocity_x = car->velocity.bits.x;
    yaw_rate = car->angular_velocity.bits.y;
    velocity_z = z + damped;
    car->velocity.bits.z = (uint32)(velocity_z);
    heading = ((sint32)heading >> 8);
    steering = car_truncate_shift(steering, 8);
    c = (uint32)rcos((sint32)heading);
    s = (uint32)rsin((sint32)heading);
    angle = heading + steering;
    turned_c = (uint32)rcos((sint32)angle);
    turned_s = (uint32)rsin((sint32)angle);
    steering_c = (uint32)rcos((sint32)steering);
    steering_s = (uint32)rsin((sint32)steering);
    wheel = (uint32)(sint16)car->pedal << 2;
    lateral = car_truncate_shift(velocity_x * c - velocity_z * s, 12);
    longitudinal = car_truncate_shift(velocity_x * s + velocity_z * c, 12);
    val0 = car_truncate_shift(steering_s * 332, 12);
    val0 = car_truncate_shift(val0 * steering_s, 12);
    val1 = car_truncate_shift(steering_c * 625, 12);
    val1 = car_truncate_shift(val1 * steering_c, 12);
    denominator = val0 + val1;
    side_force = lateral * 400;
    yaw_force = yaw_rate * 276;
    val0 = car_truncate_shift(0 - (side_force + yaw_force), 10);
    val0 = car_truncate_shift(val0 * 800, 12) * steering_c;
    rolling = longitudinal + wheel;
    val1 = car_truncate_shift(rolling * 332, 12) * steering_s;
    val2 = ((uint32)((sint32)(yaw_rate * 400) / 652) - lateral) * 175;
    front = (uint32)fixed_divide_checked((sint32)(val0 + val1), (sint32)denominator);
    val0 = car_truncate_shift(val2, 12);
    val0 = car_truncate_shift(val0 * steering_s, 12);
    val1 = car_truncate_shift(0 - rolling * 19, 12);
    val1 = car_truncate_shift(val1 * steering_c, 12);
    val0 = (val0 + val1) * steering_s;
    val1 = car_truncate_shift(yaw_force - side_force, 10);
    val1 = car_truncate_shift(val1 * 800, 12);
    val1 = car_truncate_shift(val1 * steering_c, 12) * steering_c;
    rear = (uint32)fixed_divide_checked((sint32)(val0 + val1), (sint32)denominator);
    if ((sint32)front > 2048)
        front = 2048;
    else if ((sint32)front < -2048)
        front = -2048;
    if ((sint32)rear > 2048)
        rear = 2048;
    else if ((sint32)rear < -2048)
        rear = -2048;
    val0 = car_truncate_shift(rear * c, 12);
    val1 = car_truncate_shift((0 - rear) * s, 12);
    val2 = car_truncate_shift(front * turned_c, 12);
    x_change = val0 + val2;
    val2 = car_truncate_shift((0 - front) * turned_s, 12);
    z_change = val1 + val2;
    val0 = front * car_truncate_shift(steering_c * 400, 12) - rear * 400;
    yaw_change = ((sint32)val0 / 276);
    x_change += car_truncate_shift(wheel * s, 12);
    z_change += car_truncate_shift(wheel * c, 12);
    pos = car->suspension_offset[0];
    rate = car->suspension_velocity[0];
    new_yaw = car->angular_velocity.bits.y;
    new_x = car->velocity.bits.x;
    new_z = car->velocity.bits.z;
    pos += rate;
    car->angular_velocity.bits.y = (uint32)(new_yaw + yaw_change);
    car->velocity.bits.x = (uint32)(new_x + x_change);
    car->velocity.bits.z = (uint32)(new_z + z_change);
    car->suspension_offset[0] = (uint32)(pos);
    rate = car->suspension_velocity[0];
    val0 = (uint32)((sint32)(rear + front) / 2) - car_truncate_shift(pos, 3);
    checked_pos = car->suspension_offset[0];
    car->suspension_velocity[0] = (uint32)(rate + val0 - car_truncate_shift(rate, 3));
    if ((sint32)checked_pos > 8192)
    {
        car->suspension_offset[0] = (uint32)(8192);
        car->suspension_velocity[0] = (uint32)(0);
    }
    if ((sint32)car->suspension_offset[0] < -8192)
    {
        car->suspension_offset[0] = (uint32)((uint32)-8192);
        car->suspension_velocity[0] = (uint32)(0);
    }
    pos = car->suspension_offset[1];
    rate = car->suspension_velocity[1];
    pos += rate;
    car->suspension_offset[1] = (uint32)(pos);
    rate = car->suspension_velocity[1];
    val0 = (uint32)((sint32)wheel >> 1) - car_truncate_shift(pos, 3);
    checked_pos = car->suspension_offset[1];
    car->suspension_velocity[1] = (uint32)(rate + val0 - car_truncate_shift(rate, 3));
    if ((sint32)checked_pos > 16384)
    {
        car->suspension_offset[1] = (uint32)(16384);
        car->suspension_velocity[1] = (uint32)(0);
    }
    if ((sint32)car->suspension_offset[1] < -16384)
    {
        car->suspension_offset[1] = (uint32)((uint32)-16384);
        car->suspension_velocity[1] = (uint32)(0);
    }
    new_yaw = car->angular_velocity.bits.y;
    time_step = disp_state.time_step;
    val0 = car_truncate_shift(new_yaw * time_step, 8);
    new_x = car->velocity.bits.x;
    val1 = car_truncate_shift(new_x * time_step, 8);
    angle = car->orientation.bits.y;
    car->orientation.bits.y = (uint32)(angle + val0);
    new_z = car->velocity.bits.z;
    val2 = car_truncate_shift(new_z * time_step, 8);
    pos = car->pos.bits.x;
    car->pos.bits.x = (uint32)(pos + val1);
    pos = car->pos.bits.z;
    car->pos.bits.z = (uint32)(pos + val2);
    car->forward_speed = longitudinal;
    return longitudinal;
}
