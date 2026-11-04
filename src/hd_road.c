#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "hd_road.h"

enum
{
    ROAD_BUCKETS = 8192
};

typedef struct ROAD_TRIANGLE
{
    VEC3F p[3];
    float denominator;
} ROAD_TRIANGLE;

typedef struct ROAD_LINK
{
    sint32 x, z, next;
    uint32 triangle;
} ROAD_LINK;

static struct
{
    ROAD_TRIANGLE *triangles;
    ROAD_LINK *links;
    uint32 triangle_count, triangle_capacity, link_count, link_capacity;
    sint32 buckets[ROAD_BUCKETS];
} road;

static uint32 bucket(sint32 x, sint32 z)
{
    return ((uint32)x * 73856093u ^ (uint32)z * 19349663u) & (ROAD_BUCKETS - 1);
}

void hd_road_end(void)
{
    free(road.triangles);
    free(road.links);
    memset(&road, 0, sizeof(road));
}

static void add_triangle(const VEC3F *p)
{
    float denominator = (p[1].z - p[2].z) * (p[0].x - p[2].x) + (p[2].x - p[1].x) * (p[0].z - p[2].z);
    if (fabsf(denominator) < 0.001f)
        return;
    sint32 x0 = (sint32)floorf(fminf(p[0].x, fminf(p[1].x, p[2].x)) / 1024.0f);
    sint32 x1 = (sint32)floorf(fmaxf(p[0].x, fmaxf(p[1].x, p[2].x)) / 1024.0f);
    sint32 z0 = (sint32)floorf(fminf(p[0].z, fminf(p[1].z, p[2].z)) / 1024.0f);
    sint32 z1 = (sint32)floorf(fmaxf(p[0].z, fmaxf(p[1].z, p[2].z)) / 1024.0f);
    // ambiguous or excessive cells retain authored materials
    if ((sint64)(x1 - x0 + 1) * (z1 - z0 + 1) > 64)
        return;
    if (road.triangle_count == road.triangle_capacity)
    {
        road.triangle_capacity = road.triangle_capacity != 0 ? road.triangle_capacity * 2 : 1024;
        road.triangles = realloc(road.triangles, road.triangle_capacity * sizeof(*road.triangles));
        XASSERT(road.triangles != NULL);
    }
    ROAD_TRIANGLE *triangle = &road.triangles[road.triangle_count];
    memcpy(triangle->p, p, sizeof(triangle->p));
    triangle->denominator = denominator;
    for (sint32 z = z0; z <= z1; ++z)
        for (sint32 x = x0; x <= x1; ++x)
        {
            if (road.link_count == road.link_capacity)
            {
                road.link_capacity = road.link_capacity != 0 ? road.link_capacity * 2 : 2048;
                road.links = realloc(road.links, road.link_capacity * sizeof(*road.links));
                XASSERT(road.links != NULL);
            }
            uint32 hash = bucket(x, z);
            road.links[road.link_count] = (ROAD_LINK){x, z, road.buckets[hash], road.triangle_count};
            road.buckets[hash] = (sint32)road.link_count++;
        }
    ++road.triangle_count;
}

void hd_road_begin(void)
{
    hd_road_end();
    memset(road.buckets, 0xFF, sizeof(road.buckets));
    for (uint32 node = 0; node < track_geometry.node_count; ++node)
    {
        uint32 lanes = track_geometry.node_bytes[(size_t)node * 24 + 3] & 15;
        for (uint32 lane = 0; lane < lanes; ++lane)
        {
            // primary wet asphalt uses type5 drag; flagged shoulders have ids16..31
            uint32 surface = (uint32)track_lane_material((sint32)node, lane);
            if (surface != 5 && car_surface_slows(surface))
                continue;
            VEC3F points[6];
            uint32 count = track_render_lane_triangles(node, lane, points);
            for (uint32 triangle = 0; triangle < count; ++triangle)
                add_triangle(points + triangle * 3);
        }
    }
}

static sint32 road_point(VEC3F p, sint32 check_height)
{
    if (road.triangle_count == 0)
        return 0;
    sint32 x = (sint32)floorf(p.x / 1024.0f), z = (sint32)floorf(p.z / 1024.0f);
    for (sint32 index = road.buckets[bucket(x, z)]; index >= 0; index = road.links[index].next)
    {
        const ROAD_LINK *link = &road.links[index];
        if (link->x != x || link->z != z)
            continue;
        const ROAD_TRIANGLE *t = &road.triangles[link->triangle];
        float a = ((t->p[1].z - t->p[2].z) * (p.x - t->p[2].x) + (t->p[2].x - t->p[1].x) * (p.z - t->p[2].z)) / t->denominator;
        float b = ((t->p[2].z - t->p[0].z) * (p.x - t->p[2].x) + (t->p[0].x - t->p[2].x) * (p.z - t->p[2].z)) / t->denominator;
        float c = 1.0f - a - b;
        if (a >= -0.001f && b >= -0.001f && c >= -0.001f && (!check_height || fabsf(p.y - (a * t->p[0].y + b * t->p[1].y + c * t->p[2].y)) <= 2.0f))
            return 1;
    }
    return 0;
}

sint32 hd_road_point(VEC3F p)
{
    return road_point(p, 1);
}

typedef struct ROAD_POINT
{
    double x, z;
} ROAD_POINT;

typedef struct ROAD_POLYGON
{
    ROAD_POINT p[8];
    uint32 count;
} ROAD_POLYGON;

static double side(ROAD_POINT a, ROAD_POINT b, ROAD_POINT p)
{
    return (b.x - a.x) * (p.z - a.z) - (b.z - a.z) * (p.x - a.x);
}

static double polygon_area(const ROAD_POLYGON *polygon)
{
    double area = 0;
    for (uint32 i = 2; i < polygon->count; ++i)
        area += side(polygon->p[0], polygon->p[i - 1], polygon->p[i]);
    return fabs(area) * 0.5;
}

static sint32 append_point(ROAD_POLYGON *polygon, ROAD_POINT point)
{
    if (polygon->count != 0 && fabs(polygon->p[polygon->count - 1].x - point.x) <= 1e-9 && fabs(polygon->p[polygon->count - 1].z - point.z) <= 1e-9)
        return 1;
    if (polygon->count == 8)
        return fabs(polygon->p[0].x - point.x) <= 1e-9 && fabs(polygon->p[0].z - point.z) <= 1e-9;
    polygon->p[polygon->count++] = point;
    return 1;
}

static sint32 clip(const ROAD_POLYGON *source, ROAD_POLYGON *output, ROAD_POINT a, ROAD_POINT b, double sign)
{
    output->count = 0;
    if (source->count == 0)
        return 1;
    ROAD_POINT previous = source->p[source->count - 1];
    double previous_side = sign * side(a, b, previous);
    for (uint32 i = 0; i < source->count; ++i)
    {
        ROAD_POINT current = source->p[i];
        double current_side = sign * side(a, b, current);
        if ((previous_side >= 0) != (current_side >= 0))
        {
            double fraction = previous_side / (previous_side - current_side);
            if (!append_point(output, (ROAD_POINT){previous.x + (current.x - previous.x) * fraction, previous.z + (current.z - previous.z) * fraction}))
                return 0;
        }
        if (current_side >= 0)
        {
            if (!append_point(output, current))
                return 0;
        }
        previous = current;
        previous_side = current_side;
    }
    if (output->count > 1 && fabs(output->p[0].x - output->p[output->count - 1].x) <= 1e-9 && fabs(output->p[0].z - output->p[output->count - 1].z) <= 1e-9)
        --output->count;
    return 1;
}

static sint32 covered_face(const VEC3F *points, uint32 count)
{
    static const uint32 indices[6] = {0, 1, 2, 1, 3, 2};
    ROAD_POLYGON remaining[2][32] = {0};
    uint32 bank = 0, polygons = count == 4 ? 2 : 1;
    float min_x = points[0].x, max_x = min_x, min_y = points[0].y, max_y = min_y, min_z = points[0].z, max_z = min_z;
    double area = 0;
    for (uint32 i = 0; i < count; ++i)
    {
        min_x = fminf(min_x, points[i].x);
        max_x = fmaxf(max_x, points[i].x);
        min_y = fminf(min_y, points[i].y);
        max_y = fmaxf(max_y, points[i].y);
        min_z = fminf(min_z, points[i].z);
        max_z = fmaxf(max_z, points[i].z);
    }
    for (uint32 i = 0; i < polygons; ++i)
    {
        remaining[0][i].count = 3;
        for (uint32 j = 0; j < 3; ++j)
        {
            const VEC3F *p = &points[indices[i * 3 + j]];
            remaining[0][i].p[j] = (ROAD_POINT){(double)p->x - points[0].x, (double)p->z - points[0].z};
        }
        area += polygon_area(&remaining[0][i]);
    }
    if (area <= 0.0)
        return 0;
    sint32 x0 = (sint32)floorf(min_x / 1024.0f), x1 = (sint32)floorf(max_x / 1024.0f);
    sint32 z0 = (sint32)floorf(min_z / 1024.0f), z1 = (sint32)floorf(max_z / 1024.0f);
    if ((sint64)(x1 - x0 + 1) * (z1 - z0 + 1) > 64)
        return 0;
    for (sint32 z = z0; z <= z1; ++z)
        for (sint32 x = x0; x <= x1; ++x)
            for (sint32 item = road.buckets[bucket(x, z)]; item >= 0; item = road.links[item].next)
            {
                const ROAD_LINK *link = &road.links[item];
                if (link->x != x || link->z != z)
                    continue;
                const ROAD_TRIANGLE *t = &road.triangles[link->triangle];
                float tx0 = fminf(t->p[0].x, fminf(t->p[1].x, t->p[2].x)), tx1 = fmaxf(t->p[0].x, fmaxf(t->p[1].x, t->p[2].x));
                float tz0 = fminf(t->p[0].z, fminf(t->p[1].z, t->p[2].z)), tz1 = fmaxf(t->p[0].z, fmaxf(t->p[1].z, t->p[2].z));
                sint32 first_x = (sint32)floorf(tx0 / 1024.0f), first_z = (sint32)floorf(tz0 / 1024.0f);
                // visit each triangle at the first shared cell without scratch allocation
                if (x != (first_x > x0 ? first_x : x0) || z != (first_z > z0 ? first_z : z0))
                    continue;
                if (tx1 < min_x || tx0 > max_x || tz1 < min_z || tz0 > max_z || fmaxf(t->p[0].y, fmaxf(t->p[1].y, t->p[2].y)) < min_y - 2.0f || fminf(t->p[0].y, fminf(t->p[1].y, t->p[2].y)) > max_y + 2.0f)
                    continue;
                ROAD_POINT triangle[3];
                for (uint32 i = 0; i < 3; ++i)
                    triangle[i] = (ROAD_POINT){(double)t->p[i].x - points[0].x, (double)t->p[i].z - points[0].z};
                double sign = side(triangle[0], triangle[1], triangle[2]) >= 0.0 ? 1.0 : -1.0;
                uint32 next_count = 0;
                double uncovered = 0;
                for (uint32 i = 0; i < polygons; ++i)
                {
                    ROAD_POLYGON inside = remaining[bank][i], part;
                    for (uint32 edge = 0; edge < 3; ++edge)
                    {
                        if (!clip(&inside, &part, triangle[edge], triangle[(edge + 1) % 3], sign))
                            return 0;
                        inside = part;
                    }
                    if (polygon_area(&inside) <= area * 1e-12)
                    {
                        if (next_count == 32)
                            return 0;
                        remaining[bank ^ 1][next_count++] = remaining[bank][i];
                        uncovered += polygon_area(&remaining[bank][i]);
                        continue;
                    }
                    inside = remaining[bank][i];
                    for (uint32 edge = 0; edge < 3 && inside.count != 0; ++edge)
                    {
                        if (!clip(&inside, &part, triangle[edge], triangle[(edge + 1) % 3], -sign))
                            return 0;
                        double part_area = polygon_area(&part);
                        if (part_area > area * 1e-12)
                        {
                            if (next_count == 32)
                                return 0;
                            remaining[bank ^ 1][next_count++] = part;
                            uncovered += part_area;
                        }
                        if (!clip(&inside, &part, triangle[edge], triangle[(edge + 1) % 3], sign))
                            return 0;
                        inside = part;
                    }
                }
                // subtraction measures the union, so overlaps cannot conceal a shoulder gap
                if (uncovered <= area * 0.001)
                    return 1;
                polygons = next_count;
                bank ^= 1;
            }
    return 0;
}

sint32 hd_road_face(const VEC3F *points, uint32 count)
{
    VEC3F center = {0};
    XASSERT(points != NULL && (count == 3 || count == 4));
    for (uint32 i = 0; i < count; ++i)
    {
        if (!hd_road_point(points[i]))
            return 0;
        center.x += points[i].x / count;
        center.y += points[i].y / count;
        center.z += points[i].z / count;
    }
    if (hd_road_point(center))
        return 1;
    return road_point(center, 0) && covered_face(points, count);
}

sint32 hd_road_active(void)
{
    return road.triangle_count != 0;
}
