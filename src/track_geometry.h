#ifndef TRACK_GEOMETRY_H
#define TRACK_GEOMETRY_H

#include <stddef.h>

#include "psx.h"
#include "fixed_math.h"

typedef struct TRACK_LOCATION
{
    uint16 node;
    uint16 n_node;
    uint16 path_count;
    uint16 max_path_count;
    uint16 left_vertex;
    uint16 right_vertex;
    uint8 lane;
} TRACK_LOCATION;

typedef struct TRACK_BOUNDARY_OFFSETS
{
    uint32 vertex_offset0;
    uint32 vertex_offset1;
} TRACK_BOUNDARY_OFFSETS;

extern const TRACK_BOUNDARY_OFFSETS track_boundary_offsets[12];

typedef struct TRACK_BRANCH_MAP
{
    uint16 branch_first;
    uint16 branch_last;
    uint16 main_first;
} TRACK_BRANCH_MAP;

typedef struct TRACK_GEOMETRY_STATE
{
    uint32 main_node_count;
    uint32 node_count;
    uint32 vertex_count;
    uint8 *node_bytes;
    uint8 *vertex_bytes;
    TRACK_BRANCH_MAP *branches;
    uint32 branch_count;
} TRACK_GEOMETRY_STATE;

extern TRACK_GEOMETRY_STATE track_geometry;

void track_geometry_init(void);
uint32 track_map_branch_node(sint32 node);
uint32 track_wrap_location(TRACK_LOCATION *location);
uint32 track_normalize_location(TRACK_LOCATION *location);

typedef struct TRACK_POSITION_RESULT
{
    uint32 z_delta;
    sint32 lane;
    sint32 right_vertex_y;
} TRACK_POSITION_RESULT;

TRACK_POSITION_RESULT track_place_location(TRACK_LOCATION *node, VEC3 *pos);
uint32 track_surface_height(TRACK_LOCATION *node, const VEC3 *pos, uint32 *height);
sint32 track_render_surface_height(const TRACK_LOCATION *node, const VEC3 *pos, float *height);
uint32 track_update_location(TRACK_LOCATION *node, const VEC3 *pos);
uint32 track_surface_n_height(TRACK_LOCATION *node, const VEC3 *pos, SVECTOR *n, uint32 *height);
uint32 track_surface_n(TRACK_LOCATION *node, const VEC3 *pos, SVECTOR *output);

uint32 track_lateral_fraction(sint32 node, const VEC3 *pos);

uint32 track_surface_material(const TRACK_LOCATION *node);

sint32 track_lane_material(sint32 node, uint32 lane);

uint32 track_lateral_distance(sint32 node, uint32 val, uint32 kind);

#endif
