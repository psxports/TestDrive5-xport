#ifndef MODEL_RENDER_H
#define MODEL_RENDER_H

#include <stddef.h>

#include "psx.h"
#include "fixed_math.h"
#include "display.h"
#include "model_resource.h"

typedef union MODEL_MATRIX
{
    MATRIX sdk;
    uint32 words[8];
} MODEL_MATRIX;

typedef struct MODEL_RENDER_STATE
{
    MODEL_MATRIX matrix;
    MODEL_MATRIX view;
    MODEL_MATRIX view_base;
    MODEL_MATRIX billboard;
    MODEL_MATRIX scr;
    MODEL_MATRIX listener_view[2];
    uint32 depth;
    uint32 width;
    uint32 height;
    uint32 ot_shift;
    uint32 near_depth;
    uint32 far_depth;
    uint32 proj_distance;
    uint32 x_scale;
    uint32 y_scale;
    uint32 scale;
    uint32 lod_half_distance;
    uint32 lod_quarter_distance;
    uint32 view_center_x;
    uint32 view_center_y;
    uint32 reference_offset_y;
    uint32 reference_offset_z;
    uint32 coord_shift;
    VEC3 distance_reference;
    VEC3_16 clip_x;
    VEC3_16 clip_y;
    MATRIX light;
    MATRIX color;
} MODEL_RENDER_STATE;

extern MODEL_RENDER_STATE model_render;

void model_set_view_center(sint32 x, sint32 y);
sint32 model_update_lod_distances(sint32 pos, sint32 near_distance, sint32 far_distance);
void model_set_reference_offset(sint32 lower, sint32 upper);
uint32 model_update_distance_reference(const MATRIX *rotation, const VEC3 *pos);
sint32 model_set_view_pos(const VECTOR *pos);
sint32 model_set_view_angles(const SVECTOR *angles);
uint32 model_build_rotation(MATRIX *matrix, const SVECTOR *angles);
uint32 model_aim_view(const VECTOR *pos, sint32 roll);

void model_set_ot_shift(uint32 shift);
void model_set_depth_range(uint32 near_depth, uint32 far_depth);
uint32 model_set_proj_distance(sint32 distance);
uint32 model_update_clip_planes(void);
void model_set_back_color(sint32 r, sint32 g, sint32 b);
sint16 *model_set_light(uint32 idx, const VEC3_16 *dir, uint32 r, uint32 g, uint32 b);
uint32 model_set_fog(sint32 near_depth, sint32 far_depth, sint32 r, sint32 g, sint32 b);
void model_apply_light_matrix(const MATRIX *matrix);

const sint16 *model_transform_matrix(const MATRIX *matrix, const VEC3 *pos);

sint32 *model_transform_active_pos(const VEC3 *pos);
sint32 *model_transform_pos(const VEC3 *pos);
uint32 model_visible_matrix(const uint8 *model_header);
uint32 model_visible(const uint8 *model_header);
uint32 model_visible_origin(const uint8 *model_header);
intptr model_draw(const MODEL_RESOURCE *object);
intptr model_draw_lod(const MODEL_RESOURCE *object);

uint32 render_add_checked(uint32 left, uint32 right, uint32 pc);

uint8 *model_packet_pool_init(uint32 bytes);
uint32 model_packet_pool_reset(void);

const uint8 *model_init_flare_tex(void);

uint32 model_init_lighting(uint32 radius, uint8 *dst, const uint8 *color, uint32 height);

typedef struct MODEL_PRIMITIVE_BUFFERS
{
    const uint8 *groups;
    uint8 *packets[2];
    uint32 packet_offset;
} MODEL_PRIMITIVE_BUFFERS;

uint32 model_reset_prim_colors(uint8 *groups);

typedef struct MODEL_TEXTURE_ATTRIBUTES
{
    uint16 page;
    uint16 clut;
} MODEL_TEXTURE_ATTRIBUTES;

typedef struct MODEL_TEXTURE_SOURCE
{
    const MODEL_TEXTURE_ATTRIBUTES *attributes;
    const uint8 *resource;
} MODEL_TEXTURE_SOURCE;

uint32 model_set_gouraud_tex(const MODEL_PRIMITIVE_BUFFERS *streams, const MODEL_TEXTURE_SOURCE *tex, uint32 u_offset);

uint32 model_replace_tex_region(const MODEL_RESOURCE *model, const char *old_name, const char *new_name);

#endif
