#ifndef CAR_RENDER_H
#define CAR_RENDER_H

#include "psx.h"
#include "car.h"

typedef struct CAR_ENVIRONMENT_MAPPING
{
    sint16 center_x;
    sint16 center_y;
    uint32 scale_x;
    uint32 scale_y;
    uint32 n_scale;
    sint16 tex_bias;
} CAR_ENVIRONMENT_MAPPING;

enum
{
    CAR_ENVIRONMENT_PACKET_COUNT = 200
};

typedef struct CAR_ENVIRONMENT_PACKET_BUFFER
{
    POLY_FT3 triangles[CAR_ENVIRONMENT_PACKET_COUNT];
    POLY_FT4 quads[CAR_ENVIRONMENT_PACKET_COUNT];
} CAR_ENVIRONMENT_PACKET_BUFFER;

typedef struct CAR_ENVIRONMENT_RENDER
{
    uint32 reflection_phase;
    CAR_ENVIRONMENT_PACKET_BUFFER buffers[2];
    const uint8 *tex;
    const uint8 *entries;
    uint32 entries_capacity;
    CAR_ENVIRONMENT_MAPPING mapping;
    CVECTOR tint;
} CAR_ENVIRONMENT_RENDER;

extern CAR_ENVIRONMENT_RENDER car_environment_render;

uint32 car_draw_racing_model(uint32 slot);

uint32 car_draw_scene(void);

uint32 car_draw_split_model(uint32 slot, uint32 alternate);

uint32 car_init_scene_rendering(void);

uint32 car_fit_model_bounds(const SVECTOR *vertices, sint32 count, uint32 slot);

uint32 car_upload_environment_tex(const uint8 *image, uint32 capacity, const uint8 tex[16]);

uint32 car_init_environment_packets(void);

uint32 car_draw_environment_mapping(const MODEL_RESOURCE *model, const CAR_ENVIRONMENT_MAPPING *mapping, uint32 distance);

void car_init_rear_light_packets(void);
void car_init_headlight_packets(void);
void car_init_speed_smoke_packets(void);

uint32 car_init_police_light_packets(void);
uint32 car_update_police_light_phases(void);
void car_select_police_lights(sint32 selection);

#endif
