#include <string.h>

#include <stdlib.h>
#include <math.h>

#include "psx_gpu.h"
#include "game.h"

typedef struct CAR_SHADOW_RENDER
{
    POLY_FT4 buffers[2][14 * 2];
} CAR_SHADOW_RENDER;

static CAR_SHADOW_RENDER shadow_render;

typedef struct CAR_POLICE_LIGHT_PACKETS
{
    POLY_FT4 billboards[2];
    POLY_G3 rays[2][2];
} CAR_POLICE_LIGHT_PACKETS;

static CAR_POLICE_LIGHT_PACKETS police_light_packets[4];
typedef char CAR_POLICE_LIGHT_PACKET_LAYOUT_CHECK[sizeof(POLY_G3) == 28 && sizeof(POINT_C) == 8 && offsetof(POLY_G3, p) == 4 && offsetof(POINT_C, xy) == 4 ? 1 : -1];

typedef union CAR_POLICE_LIGHT_CONFIG
{
    struct
    {
        uint32 count;
        uint8 colors[4];
        SVECTOR positions[4];
    } data;

    uint32 words[10];
} CAR_POLICE_LIGHT_CONFIG;

static CAR_POLICE_LIGHT_CONFIG police_light_config = {.words = {0x00000004u, 0x01000100u, 0x00A0FF88u, 0x0000FF60u, 0x00A0FFC4u, 0x0000FF60u, 0x00A0003Cu, 0x0000FF60u, 0x00A00078u, 0x0000FF60u}};
static uint32 police_light_phases[4];
static const char police_light_tex_name[9] = "PoliceLt";
static const uint32 police_light_srcs[5][10] = {{0x00000002u, 0x00000001u, 0x00DCFF88u, 0x0000FF88u, 0x00DC0078u, 0x0000FF88u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u}, {0x00000002u, 0x00000001u, 0x00A0FF88u, 0x0000FF60u, 0x00A00078u, 0x0000FF60u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u}, {0x00000004u, 0x00010100u, 0x00B4FF88u, 0x0000FF60u, 0x00B4FFC4u, 0x0000FF60u, 0x00B4003Cu, 0x0000FF60u, 0x00B40078u, 0x0000FF60u}, {0x00000002u, 0x00000001u, 0x00A0FF88u, 0x0000FF60u, 0x00A00078u, 0x0000FF60u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u}, {0x00000004u, 0x01000001u, 0x00A0FF88u, 0x0000FF60u, 0x00A0FFC4u, 0x0000FF60u, 0x00A0003Cu, 0x0000FF60u, 0x00A00078u, 0x0000FF60u}};
typedef char CAR_POLICE_LIGHT_CONFIG_LAYOUT_CHECK[sizeof(CAR_POLICE_LIGHT_CONFIG) == 40 && offsetof(CAR_POLICE_LIGHT_CONFIG, data.positions) == 8 ? 1 : -1];

typedef struct CAR_LIGHT_PACKET_SET
{
    POLY_FT4 billboards[14 * 4];
    POLY_GT4 projections[14 * 4];
} CAR_LIGHT_PACKET_SET;

static CAR_LIGHT_PACKET_SET light_packets[2];
static POLY_FT4 speed_smoke_packets[14 * 4];
typedef char CAR_LIGHT_PACKET_LAYOUT_CHECK[sizeof(POLY_FT4) == 40 && sizeof(POLY_GT4) == 52 && sizeof(POINT_CT) == 12 && offsetof(POLY_GT4, p) == 4 && offsetof(POINT_CT, xy) == 4 ? 1 : -1];

static uint8 rear_light_fade[14];
static const char rear_light_tex_name[7] = "Braked";
static const char headlight_tex_name[6] = "Hlite";
static const char speed_smoke_tex_name[7] = "Smoke1";
static const char environment_tex_name[] = "Tree";
static const char shadow_tex_name[] = "Shadow";

static const uint32 rear_light_geometry[2][8] = {{0xFFB00050u, 0x00000000u, 0xFFB0FFB0u, 0x00000000u, 0x00500050u, 0x00000000u, 0x0050FFB0u, 0x00000000u}, {0xFF100050u, 0x00000000u, 0xFF10FFB0u, 0x00000000u, 0x00500064u, 0x00000000u, 0x0050FF9Cu, 0x00000000u}};
static const uint32 headlight_geometry[2][8] = {{0xFF4C00B4u, 0x00000000u, 0xFF4CFF4Cu, 0x00000000u, 0x00B400B4u, 0x00000000u, 0x00B4FF4Cu, 0x00000000u}, {0xFF100050u, 0x00000000u, 0xFF10FFB0u, 0x00000000u, 0x00500064u, 0x00000000u, 0x0050FF9Cu, 0x00000000u}};
static const uint32 speed_smoke_geometry[8] = {0xFF3800C8u, 0x00000000u, 0xFF38FF38u, 0x00000000u, 0x00C800C8u, 0x00000000u, 0x00C8FF38u, 0x00000000u};

typedef char CAR_SHADOW_PACKET_LAYOUT_CHECK[sizeof(POLY_FT4) == 40 ? 1 : -1];

typedef char CAR_ENVIRONMENT_PACKET_LAYOUT_CHECK[sizeof(POLY_FT3) == 32 && sizeof(POLY_FT4) == 40 && sizeof(POINT_T) == 8 && offsetof(POLY_FT3, p) == 8 && offsetof(POLY_FT4, p) == 8 ? 1 : -1];

// Temporary heap identities until allocator migration

CAR_ENVIRONMENT_RENDER car_environment_render = {.mapping = {256, 128, 100, 230, 80, 64}, .tint = {128, 128, 128, 0}};

typedef char CAR_ENVIRONMENT_TINT_LAYOUT_CHECK[sizeof(CVECTOR) == 4 && offsetof(CVECTOR, r) == 0 && offsetof(CVECTOR, g) == 1 && offsetof(CVECTOR, b) == 2 && offsetof(CVECTOR, cd) == 3 ? 1 : -1];

typedef char CAR_MODEL_VERTEX_LAYOUT_CHECK[sizeof(SVECTOR) == 8 && offsetof(SVECTOR, vx) == 0 && offsetof(SVECTOR, vy) == 2 && offsetof(SVECTOR, vz) == 4 ? 1 : -1];

static uint32 draw_traffic_model(uint32 slot);
static uint32 draw_environment_model(const MODEL_RESOURCE *model, CAR_STATE *car);
static uint32 init_shadow_packets(void);
static void draw_contact_shadow(uint32 slot);
static void draw_body_shadow(uint32 slot);
static void draw_ground_shadow(CAR_STATE *car);
static uint32 draw_rear_lights(uint32 slot);
static uint32 draw_headlights(uint32 slot);
static uint32 draw_speed_smoke(uint32 slot);
static uint32 draw_police_lights(CAR_STATE *car);

static void draw_hd_lamp(const SVECTOR *lamp, float center_y, const uint32 geometry[8], const POLY_FT4 *material, const POINT_CT *gradient, sint32 billboard)
{
    if (gradient != NULL && geometry == headlight_geometry[1] && hd_spatial_lighting())
        return;
    VECTORF center = {(float)lamp->vx, center_y, (float)lamp->vz}, corners[4];
    HD_TEXTURE texture;
    RGBC colors[4];
    uint32 point;
    if (!hd_capture_active())
        return;
    texture = hd_quad_texture(material->p, &material->rgbc);
    if (gradient == NULL && !billboard)
        texture.flags |= HD_LAMP_GLOW;
    for (point = 0; point < 4; ++point)
    {
        SVECTOR vertex;
        memcpy(&vertex, geometry + point * 2, sizeof(vertex));
        corners[point].vx = (float)vertex.vx;
        corners[point].vy = (float)vertex.vy;
        corners[point].vz = (float)vertex.vz;
        colors[point] = gradient != NULL ? gradient[point].rgbc : material->rgbc;
    }
    if (billboard)
        hd_attached_quad(&center, corners, &texture, colors);
    else
    {
        VECTORF vertices[4];
        MATRIXF transform = hd_model_transform();
        float facing;
        for (point = 0; point < 4; ++point)
        {
            VECTORF local = {corners[point].vx + center.vx, corners[point].vy + center.vy, corners[point].vz + center.vz};
            RotTransf(&transform, &local, vertices + point);
        }
        facing = vertices[0].vx * (vertices[1].vy * vertices[2].vz - vertices[1].vz * vertices[2].vy) + vertices[0].vy * (vertices[1].vz * vertices[2].vx - vertices[1].vx * vertices[2].vz) + vertices[0].vz * (vertices[1].vx * vertices[2].vy - vertices[1].vy * vertices[2].vx);
        if (facing >= 0.0f)
        {
            if (gradient == NULL)
                hd_attached_quad(&center, corners, &texture, colors);
            else
                hd_world_quad(vertices, &texture, colors, 1);
        }
    }
}

static void draw_hd_rear_projection(uint32 slot, const SVECTOR *lamp, float ground_height, const uint32 geometry[8], const POLY_FT4 *material, const POINT_CT gradient[4])
{
    if (hd_wet_reflections_active())
        return;
    const CAR_STATE *car = &car_states[slot];
    HD_TEXTURE texture;
    VECTORF points[4];
    MATRIXF rotation = {0};
    RGBC colors[4];
    uint32 point, row, column;
    if (!hd_capture_active() || car->ground_n.vy == 0)
        return;
    texture = hd_quad_texture(material->p, &material->rgbc);
    for (row = 0; row < 3; ++row)
        for (column = 0; column < 3; ++column)
            rotation.m[row][column] = car->transform.m[row][column] / 4096.0f;
    for (point = 0; point < 4; ++point)
    {
        SVECTOR source;
        VECTORF local, world;
        memcpy(&source, geometry + point * 2, sizeof(source));
        // tuck the bright edge under the bumper and shorten the road footprint
        local = (VECTORF){lamp->vx + source.vx, 0.0f, lamp->vz + 40.0f + source.vy * 0.5f};
        ApplyMatrixf(&rotation, &local, &world);
        // place the glow on the receiving plane instead of intersecting it with a vertical billboard
        world.vy = ground_height - (world.vx * car->ground_n.vx + world.vz * car->ground_n.vz) / car->ground_n.vy;
        world.vx += car->pos.x / 256.0f;
        world.vz += car->pos.z / 256.0f;
        {
            VEC3 sample = {.x = (sint32)(world.vx * 256.0f), .y = car->pos.y, .z = (sint32)(world.vz * 256.0f)};
            float surface;
            if (track_render_surface_height(&car->track_location, &sample, &surface))
                world.vy = surface;
        }
        world.vy -= 4.0f;
        points[point] = world;
        colors[point] = gradient[point].rgbc;
    }
    hd_road_quad(points, &texture, colors);
}

static uint32 draw_racing_model(uint32 slot)
{
    MODEL_RESOURCE render_model;
    uint8 *model_data;
    CAR_STATE *car = &car_states[slot];
    uint32 val;
    uint32 camera;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 result;

    FUNCTION_MARKER(0x8002E70C, "SLUS_006.10");
    val = car->pos.bits.x;
    camera = model_render.view.sdk.t[0];
    x = (uint32)((sint32)val / 256) - camera;
    x = ((sint32)x / 16);
    val = car->pos.bits.y;
    camera = model_render.view.sdk.t[1];
    y = (uint32)((sint32)val / 256) - camera;
    y = ((sint32)y / 16);
    val = car->pos.bits.z;
    camera = model_render.view.sdk.t[2];
    z = (uint32)((sint32)val / 256) - camera;
    z = ((sint32)z / 16);
    result = SquareRoot0((sint32)(x * x + y * y + z * z));
    if ((sint32)result < 1125 || race_session.demo != 0)
    {
        model_data = car->render_models[0].bytes;
        render_model = car->render_models[0];
    }
    else
    {
        model_data = car->render_models[1].bytes;
        render_model = car->render_models[1];
    }
    x = car->pos.bits.x;
    y = car->pos.bits.y;
    z = car->pos.bits.z;
    ((MODEL_HEADER *)model_data)->position.bits.x = x;
    ((MODEL_HEADER *)model_data)->position.bits.y = y;
    ((MODEL_HEADER *)model_data)->position.bits.z = z;
    result = model_visible_matrix(model_data);
    if (result == 0)
    {
        hd_draw_car_candidate(slot, &render_model, HD_CAR_RACING);
        return result;
    }
    car_update_lighting(car);
    model_transform_matrix(&car->transform, &car->pos);
    model_draw(&render_model);
    chase_draw_health(slot);
    draw_rear_lights(slot);
    draw_headlights(slot);
    draw_speed_smoke(slot);
    wheels_draw_race(slot);
    if (car->motion_mode != 0)
        draw_ground_shadow(car);
    else
        draw_contact_shadow(slot);
    val = race_session.cop_chase;
    result = slot * 2;
    if (val == 0)
        return result;
    result = (sint16)chase_state.health[result / 2];
    if (result == 0)
        result = effect_spawn_ballistic_spr(car, &car->pos, 0);
    return result;
}

uint32 car_draw_racing_model(uint32 slot)
{
    sint32 previous = hd_car_begin(slot);
    uint32 result = draw_racing_model(slot);
    hd_car_end(previous);
    return result;
}

static uint32 draw_scene(void)
{
    MODEL_RESOURCE render_model;
    uint8 *model_data;
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    uint32 sel;
    CAR_STATE *car;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 hidden;

    uint32 slot;
    sint32 count;

    FUNCTION_MARKER(0x8002ECF4, "SLUS_006.10");
    sel = race_view.primary_car;
    car = &car_states[sel];
    {
        model_data = car->render_models[0].bytes;
        render_model = car->render_models[0];
    }
    x = car->pos.bits.x;
    y = car->pos.bits.y;
    z = car->pos.bits.z;
    ((MODEL_HEADER *)model_data)->position.bits.x = x;
    ((MODEL_HEADER *)model_data)->position.bits.y = y;
    ((MODEL_HEADER *)model_data)->position.bits.z = z;
    hidden = camera_views[0].attached;
    wheel_skid_bank = (uint32)(0);
    if (hidden == 0)
    {
        car_update_lighting(car);
        if (model_visible_matrix(model_data) != 0)
        {
            model_transform_matrix(&car->transform, &car->pos);
            level_desc = &scene_level_image.desc;
            level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
            if ((level_header[30] & 1) == 0)
                draw_environment_model(&car->render_models[0], car);
            model_draw(&render_model);
            if (race_session.cop_chase != 0 && chase_state.signal_level != 0)
                draw_police_lights(car);
            else
                hd_police_retire();
            draw_rear_lights(sel);
            draw_headlights(sel);
            draw_speed_smoke(sel);
            car_emit_rear_effect(car);
            wheels_draw_race(sel);
            if (car->motion_mode != 0)
                draw_ground_shadow(car);
            else
                draw_contact_shadow(sel);
        }
        else
            hd_draw_car_candidate(sel, &render_model, HD_CAR_PRIMARY);
    }
    car_update_skid_effects(car);
    if (race_session.time_trial == 0)
    {
        count = (sint32)race_control.car_count;
        slot = 0;
        for (;;)
        {
            if (count >= 7)
            {
                if ((sint32)slot >= 6)
                    break;
            }
            else if ((sint32)slot >= count)
                break;
            if (slot != race_view.primary_car)
                car_draw_racing_model(slot);
            count = (sint32)race_control.car_count;
            ++slot;
        }
    }
    if (race_rules.traffic == 0)
        return 0;
    slot = 6;
    count = (sint32)race_control.car_count;
    if ((sint32)slot < count)
    {
        do
        {
            draw_traffic_model(slot);
            count = (sint32)race_control.car_count;
            ++slot;
        } while ((sint32)slot < count);
    }
    return 0;
}

uint32 car_draw_scene(void)
{
    sint32 previous = hd_car_begin(race_view.primary_car);
    uint32 result = draw_scene();
    hd_car_end(previous);
    return result;
}

static uint32 draw_traffic_body(uint32 slot)
{
    MODEL_RESOURCE render_model;
    uint8 *model_data;
    CAR_STATE *car = &car_states[slot];
    uint32 val;
    uint32 camera;
    uint32 x;
    uint32 y;
    uint32 z;
    uint32 result;

    FUNCTION_MARKER(0x8002E938, "SLUS_006.10");
    val = car->pos.bits.x;
    camera = model_render.view.sdk.t[0];
    x = (uint32)((sint32)val / 256) - camera;
    x = ((sint32)x / 16);
    val = car->pos.bits.y;
    camera = model_render.view.sdk.t[1];
    y = (uint32)((sint32)val / 256) - camera;
    y = ((sint32)y / 16);
    val = car->pos.bits.z;
    camera = model_render.view.sdk.t[2];
    z = (uint32)((sint32)val / 256) - camera;
    z = ((sint32)z / 16);
    result = SquareRoot0((sint32)(x * x + y * y + z * z));
    if ((sint32)result < 375 || race_session.demo != 0)
    {
        model_data = car->render_models[0].bytes;
        render_model = car->render_models[0];
    }
    else
    {
        model_data = car->render_models[1].bytes;
        render_model = car->render_models[1];
    }
    x = car->pos.bits.x;
    y = car->pos.bits.y;
    z = car->pos.bits.z;
    ((MODEL_HEADER *)model_data)->position.bits.x = x;
    ((MODEL_HEADER *)model_data)->position.bits.y = y;
    ((MODEL_HEADER *)model_data)->position.bits.z = z;
    result = model_visible_matrix(model_data);
    if (result == 0)
    {
        hd_draw_car_candidate(slot, &render_model, HD_CAR_TRAFFIC);
        return result;
    }
    car_update_lighting(car);
    model_apply_light_matrix(&car->transform);
    model_transform_matrix(&car->transform, &car->pos);
    model_draw(&render_model);
    draw_rear_lights(slot);
    draw_headlights(slot);
    if (slot == 9 && race_ai_cars[9].chase_target != 0xFFFFFFFF)
    {
        chase_state.signal_level = 4096;
        draw_police_lights(car);
    }
    else if (slot == 9)
        hd_police_retire();
    if (car->motion_mode != 0)
        draw_ground_shadow(car);
    else
        draw_body_shadow(slot);
    result = race_ai_cars[slot].recovery_state;
    if (result != 0)
        result = effect_spawn_ballistic_spr(car, &car->pos, 0);
    return result;
}

static uint32 draw_traffic_model(uint32 slot)
{
    sint32 previous = hd_car_begin(slot);
    uint32 result = draw_traffic_body(slot);
    hd_car_end(previous);
    return result;
}

static uint32 draw_split_model(uint32 slot, uint32 alternate)
{
    MODEL_RESOURCE render_model;
    uint8 *model_data;
    CAR_STATE *car = &car_states[slot];
    uint32 x, y, z, val, mode;

    FUNCTION_MARKER(0x8002E568, "SLUS_006.10");
    {
        model_data = car_states[slot].render_models[alternate == 0 ? 0 : 1].bytes;
        render_model = car_states[slot].render_models[alternate == 0 ? 0 : 1];
    }
    x = car->pos.bits.x;
    y = car->pos.bits.y;
    z = car->pos.bits.z;
    ((MODEL_HEADER *)model_data)->position.bits.x = x;
    ((MODEL_HEADER *)model_data)->position.bits.y = y;
    ((MODEL_HEADER *)model_data)->position.bits.z = z;
    val = camera_views[slot].attached;
    wheel_skid_bank = (uint32)(slot);
    if (val == 0 || race_view.viewport != slot)
    {
        car_update_lighting(car);
        val = model_visible_matrix(model_data);
        if (val != 0)
        {
            model_transform_matrix(&car->transform, &car->pos);
            model_draw(&render_model);
            mode = race_session.multiplayer_mode;
            if (mode == 3)
            {
                if (slot != chase_state.pursuer_slot)
                    chase_draw_health(slot);
                if (race_session.multiplayer_mode == mode && chase_state.signal_level != 0 && slot == chase_state.pursuer_slot)
                    draw_police_lights(car);
                else
                    hd_police_retire();
            }
            draw_rear_lights(slot);
            draw_headlights(slot);
            draw_speed_smoke(slot);
            wheels_draw_race(slot);
            if (car->motion_mode == 0)
                draw_contact_shadow(slot);
            else
                draw_ground_shadow(car);
        }
        else
            hd_draw_car_candidate(slot, &render_model, HD_CAR_SPLIT);
    }
    else
        hd_draw_car_probe(slot, &render_model);
    return car_update_skid_effects(car);
}

uint32 car_draw_split_model(uint32 slot, uint32 alternate)
{
    sint32 previous = hd_car_begin(slot);
    uint32 result = draw_split_model(slot, alternate);
    hd_car_end(previous);
    return result;
}

static uint32 init_environment(void)
{
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    const uint8 *tex;

    const uint8 *state;
    uint32 entries_offset;
    uint32 entries;
    uint32 selection;
    uint32 val;

    FUNCTION_MARKER(0x8002F158, "SLUS_006.10");
    tex = tex_find(environment_tex_name);
    level_desc = &scene_level_image.desc;
    state = car_lighting_zones.bytes + car_lighting_zones.offset;
    entries = level_desc->environment_offset;
    memcpy(&selection, state + 28, sizeof(selection));
    car_environment_render.tex = (tex);
    entries_offset = entries;
    XASSERT(!(entries_offset > scene_level_image.size || scene_level_image.size - entries_offset < 16));
    car_environment_render.entries = scene_level_image.bytes + entries_offset;
    car_environment_render.entries_capacity = scene_level_image.size - entries_offset;
    car_load_environment(&car_states[0], selection);
    val = car_states[0].environment.limit;
    car_states[0].environment.level = (uint16)(val);
    return val;
}

uint32 car_init_scene_rendering(void)
{
    uint16 wire_halfword_1;

    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    sint32 count;
    uint32 slot = 0;
    const uint8 *root;
    CAR_STATE *car;
    const MODEL_RESOURCE *src;
    uint32 model0;
    uint32 model1;
    uint32 offset;
    uint32 file_offset;

    uint32 flag;

    FUNCTION_MARKER(0x8002EB88, "SLUS_006.10");
    XASSERT(!(race_control.car_count > 14));
    count = (sint32)race_control.car_count;
    while (count >= 7 ? slot < 6 : (sint32)slot < count)
    {
        XASSERT(!(scene_track_image.bytes == NULL || scene_track_image.size < 144 + slot * 8));
        root = scene_track_image.bytes + slot * 8;
        memcpy(&model0, root + 136, sizeof(model0));
        car = &car_states[slot];
        if (car_model_assets[slot][0].bytes != NULL)
        {
            car_states[slot].render_models[0].bytes = car_model_assets[slot][0].bytes;
            car_states[slot].render_models[0].capacity = car_model_assets[slot][0].capacity;
        }
        else
        {
            file_offset = model0;
            XASSERT(!(file_offset > scene_track_image.size || scene_track_image.size - file_offset < 60));
            car_states[slot].render_models[0].bytes = scene_track_image.bytes + file_offset;
            car_states[slot].render_models[0].capacity = scene_track_image.size - file_offset;
        }
        memcpy(&model1, root + 140, sizeof(model1));
        count = (sint32)race_control.car_count;
        if (car_model_assets[slot][1].bytes != NULL)
        {
            car_states[slot].render_models[1].bytes = car_model_assets[slot][1].bytes;
            car_states[slot].render_models[1].capacity = car_model_assets[slot][1].capacity;
        }
        else
        {
            file_offset = model1;
            XASSERT(!(file_offset > scene_track_image.size || scene_track_image.size - file_offset < 60));
            car_states[slot].render_models[1].bytes = scene_track_image.bytes + file_offset;
            car_states[slot].render_models[1].capacity = scene_track_image.size - file_offset;
        }
        ++slot;
    }
    slot = 6;
    count = (sint32)race_control.car_count;
    if ((sint32)slot < count)
    {
        offset = 0x1440;
        do
        {
            XASSERT(!(scene_track_image.bytes == NULL || scene_track_image.size < 144 + slot * 8));
            root = scene_track_image.bytes + slot * 8;
            memcpy(&model0, root + 136, sizeof(model0));
            file_offset = model0;
            XASSERT(!(file_offset > scene_track_image.size || scene_track_image.size - file_offset < 60));
            car_states[slot].render_models[0].bytes = scene_track_image.bytes + file_offset;
            car_states[slot].render_models[0].capacity = scene_track_image.size - file_offset;
            memcpy(&model1, root + 140, sizeof(model1));
            file_offset = model1;
            XASSERT(!(file_offset > scene_track_image.size || scene_track_image.size - file_offset < 60));
            car_states[slot].render_models[1].bytes = scene_track_image.bytes + file_offset;
            car_states[slot].render_models[1].capacity = scene_track_image.size - file_offset;
            src = &car_states[offset / 864u].render_models[0];
            {
                uint32 vertex_offset;
                sint32 vertex_count;
                const SVECTOR *vertices = NULL;
                XASSERT(!(src->capacity < 48));
                vertex_offset = ((const MODEL_HEADER *)src->bytes)->vertex_offset;
                vertex_count = (sint16)(memcpy(&wire_halfword_1, src->bytes + 14, sizeof(wire_halfword_1)), wire_halfword_1);
                if (vertex_count > 0)
                {
                    uint32 vertex_bytes = (uint32)(vertex_count - 1) * sizeof(SVECTOR) + 3 * sizeof(sint16);
                    XASSERT(!(vertex_offset > src->capacity || vertex_bytes > src->capacity - vertex_offset));
                    vertices = (const SVECTOR *)(src->bytes + vertex_offset);
                }
                car_fit_model_bounds(vertices, vertex_count, slot);
            }
            count = (sint32)race_control.car_count;
            ++slot;
            offset += 864;
        } while ((sint32)slot < count);
    }
    init_shadow_packets();
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    flag = level_header[30] & 1;
    if (flag != 0)
        return flag;
    init_environment();
    return car_init_environment_packets();
}

uint32 car_fit_model_bounds(const SVECTOR *vertices, sint32 count, uint32 slot)
{
    sint32 idx;
    sint32 x = 0;
    sint32 y = 0;
    sint32 z = 0;
    sint32 val;
    sint32 low;
    sint32 high;
    uint32 sq;
    uint32 radius;

    FUNCTION_MARKER(0x800262E4, "SLUS_006.10");
    for (idx = 0; idx < count; ++idx)
    {
        val = vertices[idx].vx;
        if (x < val)
            x = val;
        if (x < -val)
            x = -val;
        val = vertices[idx].vy;
        if (y < val)
            y = val;
        if (y < -val)
            y = -val;
        val = vertices[idx].vz;
        if (z < val)
            z = val;
        if (z < -val)
            z = -val;
    }
    if ((sint32)slot < 6)
    {
        low = car_suspension_points[slot][0].vx;
        high = (sint16)car_geometry_metrics[slot].wheel_width;
        val = high - low;
        if (x < val)
            x = val;
    }
    x += 20;
    z -= 20;
    car_body_points[slot][4].vx = (sint16)(uint16)((uint32)-x);
    car_body_points[slot][6].vx = (sint16)(uint16)((uint32)-x);
    car_body_points[slot][6].vz = (sint16)(uint16)((uint32)-z);
    car_body_points[slot][7].vz = (sint16)(uint16)((uint32)-z);
    car_body_points[slot][4].vy = (sint16)(uint16)((uint32)y);
    car_body_points[slot][4].vz = (sint16)(uint16)((uint32)z);
    car_body_points[slot][5].vx = (sint16)(uint16)((uint32)x);
    car_body_points[slot][5].vy = (sint16)(uint16)((uint32)y);
    car_body_points[slot][5].vz = (sint16)(uint16)((uint32)z);
    car_body_points[slot][6].vy = (sint16)(uint16)((uint32)y);
    car_body_points[slot][7].vx = (sint16)(uint16)((uint32)x);
    car_body_points[slot][7].vy = (sint16)(uint16)((uint32)y);
    if ((sint32)slot < 6)
    {
        val = (sint16)car_geometry_metrics[slot].wheel_radius * 181;
        if (val < 0)
            val += 255;
        low = car_suspension_points[slot][0].vy;
        y = (val >> 8) - low;
    }
    sq = (uint32)x * (uint32)x;
    sq += (uint32)y * (uint32)y;
    sq += (uint32)z * (uint32)z;
    car_body_points[slot][0].vx = (sint16)(uint16)((uint32)-x);
    car_body_points[slot][0].vy = (sint16)(uint16)((uint32)-y);
    car_body_points[slot][0].vz = (sint16)(uint16)((uint32)z);
    car_body_points[slot][1].vx = (sint16)(uint16)((uint32)x);
    car_body_points[slot][1].vy = (sint16)(uint16)((uint32)-y);
    car_body_points[slot][1].vz = (sint16)(uint16)((uint32)z);
    car_body_points[slot][2].vx = (sint16)(uint16)((uint32)-x);
    car_body_points[slot][2].vy = (sint16)(uint16)((uint32)-y);
    car_body_points[slot][2].vz = (sint16)(uint16)((uint32)-z);
    car_body_points[slot][3].vx = (sint16)(uint16)((uint32)x);
    car_body_points[slot][3].vy = (sint16)(uint16)((uint32)-y);
    car_body_points[slot][3].vz = (sint16)(uint16)((uint32)-z);
    car_geometry_metrics[slot].ground_offset = (uint16)((uint32)y);
    radius = fixed_sqrt(sq);
    car_geometry_metrics[slot].col_radius = (uint16)(radius);
    if ((sint32)slot < 6)
        return 0x66660000;
    car_lamp_points[slot][2].vy = (sint16)(uint16)(0);
    car_lamp_points[slot][2].vz = (sint16)(uint16)((uint32)z);
    car_lamp_points[slot][3].vy = (sint16)(uint16)(0);
    car_lamp_points[slot][3].vz = (sint16)(uint16)((uint32)z);
    car_lamp_points[slot][0].vy = (sint16)(uint16)(0);
    car_lamp_points[slot][0].vz = (sint16)(uint16)((uint32)-z);
    car_lamp_points[slot][1].vy = (sint16)(uint16)(0);
    car_lamp_points[slot][1].vz = (sint16)(uint16)((uint32)-z);
    x -= x / 5;
    car_lamp_points[slot][2].vx = (sint16)(uint16)((uint32)-x);
    car_lamp_points[slot][3].vx = (sint16)(uint16)((uint32)x);
    car_lamp_points[slot][0].vx = (sint16)(uint16)((uint32)-x);
    car_lamp_points[slot][1].vx = (sint16)(uint16)((uint32)x);
    return x;
}

uint32 car_upload_environment_tex(const uint8 *image, uint32 capacity, const uint8 tex[16])
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;

    uint32 wire_word_7, wire_word_8;
    uint32 page;
    uint32 u;
    uint32 x;
    uint32 y;
    uint32 signature;
    uint32 v;
    uint32 clut;
    const uint8 *block;
    uint32 block_offset;
    uint32 bytes;
    PSX_RECT rect;

    FUNCTION_MARKER(0x8007A984, "SLUS_006.10");
    page = ((const TEXTURE_DESCRIPTOR *)tex)->page;
    u = tex[14];
    x = ((page & 15) << 6) + (u >> ((page & 0x180) != 0 ? 1 : 2));
    page = ((const TEXTURE_DESCRIPTOR *)tex)->page;
    XASSERT(!(capacity < 4));
    memcpy(&signature, image, sizeof(signature));
    v = tex[15];
    clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
    y = ((page & 16) << 4) + v;
    if (signature != 16)
        return 16;
    block_offset = 8;
    XASSERT(!(capacity < 20));
    block = image + block_offset;
    rect.x = (sint16)((clut & 63) << 4);
    rect.y = (sint16)(clut >> 6);
    rect.w = (sint16)(memcpy(&wire_halfword_1, block + 8, sizeof(wire_halfword_1)), wire_halfword_1);
    rect.h = (sint16)(memcpy(&wire_halfword_2, block + 10, sizeof(wire_halfword_2)), wire_halfword_2);
    bytes = rect.w > 0 && rect.h > 0 ? (uint32)rect.w * (uint32)rect.h * 2 : 0;
    XASSERT(!(bytes > capacity - block_offset - 12));
    LoadImagePSX(&rect, (uint32 *)(block + 12));
    block_offset += (memcpy(&wire_word_7, block, sizeof(wire_word_7)), wire_word_7) & 0xFFFFFFFC;
    XASSERT(!(block_offset > capacity || capacity - block_offset < 4));
    block = image + block_offset;
    if ((memcpy(&wire_word_8, block, sizeof(wire_word_8)), wire_word_8) == 4)
        return 4;
    XASSERT(!(capacity - block_offset < 12));
    rect.x = (sint16)x;
    rect.y = (sint16)y;
    rect.w = (sint16)(memcpy(&wire_halfword_3, block + 8, sizeof(wire_halfword_3)), wire_halfword_3);
    rect.h = (sint16)(memcpy(&wire_halfword_4, block + 10, sizeof(wire_halfword_4)), wire_halfword_4);
    bytes = rect.w > 0 && rect.h > 0 ? (uint32)rect.w * (uint32)rect.h * 2 : 0;
    XASSERT(!(bytes > capacity - block_offset - 12));
    return (uint32)LoadImagePSX(&rect, (uint32 *)(block + 12));
}

uint32 car_init_environment_packets(void)
{
    const uint8 *tex;
    uint32 idx;
    uint32 buffer;
    POLY_FT3 *poly3;
    POLY_FT4 *poly4;
    uint32 page;
    uint32 clut;
    uint32 code;

    FUNCTION_MARKER(0x8002F208, "SLUS_006.10");
    tex = tex_find(environment_tex_name);
    car_environment_render.tex = (tex);
    {
        sint32 operation_ok = !(!gpu_register_packet_range(car_environment_render.buffers, (uint32)sizeof(car_environment_render.buffers)));
        XASSERT(operation_ok);
    }
    for (idx = 0; idx < CAR_ENVIRONMENT_PACKET_COUNT; ++idx)
    {
        for (buffer = 0; buffer < 2; ++buffer)
        {
            poly3 = car_environment_render.buffers[buffer].triangles + idx;
            setPolyFT3(poly3);
            page = ((const TEXTURE_DESCRIPTOR *)tex)->page;
            code = poly3->rgbc.code;
            poly3->p[1].attr = (uint16)(page);
            clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
            poly3->rgbc.code = (uint8)((code | 2) & 0xFE);
            poly3->rgbc.r = (uint8)(128);
            poly3->rgbc.g = (uint8)(128);
            poly3->rgbc.b = (uint8)(128);
            poly3->p[0].attr = (uint16)(clut);
        }
    }
    for (idx = 0; idx < CAR_ENVIRONMENT_PACKET_COUNT; ++idx)
    {
        for (buffer = 0; buffer < 2; ++buffer)
        {
            poly4 = car_environment_render.buffers[buffer].quads + idx;
            setPolyFT4(poly4);
            page = ((const TEXTURE_DESCRIPTOR *)tex)->page;
            code = poly4->rgbc.code;
            poly4->p[1].attr = (uint16)(page);
            clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
            poly4->rgbc.code = (uint8)((code | 2) & 0xFE);
            poly4->rgbc.r = (uint8)(128);
            poly4->rgbc.g = (uint8)(128);
            poly4->rgbc.b = (uint8)(128);
            poly4->p[0].attr = (uint16)(clut);
        }
    }
    return 0;
}

uint32 car_draw_environment_mapping(const MODEL_RESOURCE *model, const CAR_ENVIRONMENT_MAPPING *mapping, uint32 distance)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;

    DISPLAY_ORDERING bucket, ordering;
    static const uint32 n_commands[4] = {0x486012, 0x48E012, 0x496012, 0x486012};
    uint32 val, product, scale_x, scale_y, n_scale;
    uint32 shift, other_shift, tex_u, tex_v;
    const uint8 *tex;
    const uint8 *stream;
    uint32 groups_offset;
    uint32 vertex_offset, model_offset;
    uint32 result, kind;
    const uint8 *vertex_addresses[3];
    POLY_FT3 *poly3;
    POLY_FT4 *poly4;
    POINT_T *packet_points;
    RGBC *packet_color;
    uint32 *packet_tag;
    uint32 vertex, word_idx, corner, tag;
    const uint8 *address;
    const uint8 *normals;
    uint32 n_product, y_product, coord_u, coord_v;
    sint32 proj, multiplier, quotient_x, quotient_y;
    sint32 center_x, center_y, tex_bias, far_plane, area0, area1, depth;
    sint32 count, idx, pixel_x, pixel_y;
    sint16 n_x, n_y;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8002F384, "SLUS_006.10");
    val = mapping->scale_x;
    product = val * distance;
    proj = (sint32)model_render.proj_distance;
    quotient_x = fixed_divide_checked((sint32)product, proj);
    val = mapping->scale_y;
    quotient_y = fixed_divide_checked((sint32)(val * distance), proj);
    multiplier = (sint32)model_render.scale;
    scale_x = (uint32)fixed_divide_checked((sint32)((uint32)quotient_x << 12), multiplier);
    scale_y = (uint32)fixed_divide_checked((sint32)((uint32)quotient_y << 12), multiplier);
    tex_bias = mapping->tex_bias;
    center_x = mapping->center_x;
    center_y = mapping->center_y;
    shift = model_render.ot_shift;
    XASSERT(!(model->capacity < 60));
    groups_offset = ((const MODEL_HEADER *)model->bytes)->stream_offset;
    XASSERT(!(groups_offset > model->capacity || model->capacity - groups_offset < 4));
    stream = model->bytes + groups_offset;
    val = mapping->n_scale;
    tex = car_environment_render.tex;
    far_plane = (sint32)model_render.far_depth;
    other_shift = model_render.coord_shift;
    tex_u = tex[14];
    tex_v = tex[15];
    vertex_offset = ((const MODEL_HEADER *)model->bytes)->vertex_offset;
    shift = (shift - other_shift) & 31;
    cur_frame = disp_state.cur;
    quotient_x = (sint16)(memcpy(&wire_halfword_1, model->bytes + 12, sizeof(wire_halfword_1)), wire_halfword_1);
    far_plane >>= (other_shift + 2) & 31;
    ordering.tags = cur_frame->ordering;
    ordering.byte_offset = 0;
    kind = stream[3];
    result = (uint32)(quotient_x >> shift) << 2;
    ordering.byte_offset += result;
    n_scale = (uint32)fixed_divide_checked((sint32)(val << 12), multiplier);
    if (kind == 0)
        return result;
    do
    {
        kind = stream[2];
        count = (sint16)(memcpy(&wire_halfword_2, stream, sizeof(wire_halfword_2)), wire_halfword_2);
        stream += 4;
        if (kind <= 1)
            stream += (uint32)count * 20;
        else if (kind == 2 || kind == 3)
        {
            val = disp_state.bank;
            if (kind == 2)
            {
                poly3 = car_environment_render.buffers[val].triangles;
                poly4 = NULL;
                packet_tag = &poly3->tag;
                packet_color = &poly3->rgbc;
                packet_points = poly3->p;
            }
            else
            {
                poly4 = car_environment_render.buffers[val].quads;
                poly3 = NULL;
                packet_tag = &poly4->tag;
                packet_color = &poly4->rgbc;
                packet_points = poly4->p;
            }
            for (idx = 0; idx < count; ++idx)
            {
                for (vertex = 0; vertex < 3; ++vertex)
                {
                    val = (sint16)(memcpy(&wire_halfword_3, stream + vertex * 2, sizeof(wire_halfword_3)), wire_halfword_3);
                    model_offset = vertex_offset + (val << 3);
                    XASSERT(!(model_offset > model->capacity || model->capacity - model_offset < 8));
                    vertex_addresses[vertex] = model->bytes + model_offset;
                }
                for (vertex = 0; vertex < 3; ++vertex)
                {
                    memcpy(&val, vertex_addresses[vertex], sizeof(val));
                    xport_gte_write_data(vertex * 2, val);
                    memcpy(&val, vertex_addresses[vertex] + 4, sizeof(val));
                    xport_gte_write_data(vertex * 2 + 1, val);
                }
                xport_gte_execute(0x280030);
                xport_gte_execute(0x1400006);
                area0 = (sint32)xport_gte_read_data(24);
                area1 = -1;
                if (kind == 3)
                {
                    packet_points[0].xy.xy = (uint32)(xport_gte_read_data(12));
                    packet_points[1].xy.xy = (uint32)(xport_gte_read_data(13));
                    packet_points[2].xy.xy = (uint32)(xport_gte_read_data(14));
                    val = (sint16)(memcpy(&wire_halfword_4, stream + 6, sizeof(wire_halfword_4)), wire_halfword_4);
                    model_offset = vertex_offset + (val << 3);
                    XASSERT(!(model_offset > model->capacity || model->capacity - model_offset < 8));
                    address = model->bytes + model_offset;
                    memcpy(&val, address, sizeof(val));
                    xport_gte_write_data(0, val);
                    memcpy(&val, address + 4, sizeof(val));
                    xport_gte_write_data(1, val);
                    xport_gte_execute(0x180001);
                    xport_gte_execute(0x1400006);
                    area1 = (sint32)xport_gte_read_data(24);
                    xport_gte_execute(0x168002E);
                }
                else
                    xport_gte_execute(0x158002D);
                if (area0 <= 0 || (kind == 3 && area1 >= 0))
                {
                    depth = (sint32)xport_gte_read_data(7);
                    if (depth < far_plane)
                    {
                        if (kind == 3)
                            packet_points[3].xy.xy = (uint32)(xport_gte_read_data(14));
                        else
                        {
                            packet_points[0].xy.xy = (uint32)(xport_gte_read_data(12));
                            packet_points[1].xy.xy = (uint32)(xport_gte_read_data(13));
                            packet_points[2].xy.xy = (uint32)(xport_gte_read_data(14));
                        }
                        bucket = ordering;
                        bucket.byte_offset += ((uint32)(depth >> shift) << 2);
                        tag = *packet_tag;
                        val = bucket.tags[(sint32)bucket.byte_offset / 4];
                        *packet_tag = ((tag & 0xFF000000) | (val & 0xFFFFFF));
                        val = bucket.tags[(sint32)bucket.byte_offset / 4];
                        bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet_tag);
                        normals = stream + (kind == 2 ? 20 : 24);
                        for (word_idx = 0; word_idx < 6; ++word_idx)
                        {
                            memcpy(&val, normals + word_idx * 4, sizeof(val));
                            xport_gte_write_data(word_idx, val);
                        }
                        for (corner = 0; corner < (kind == 2 ? 3u : 4u); ++corner)
                        {
                            if (corner == 3)
                            {
                                memcpy(&val, stream + 48, sizeof(val));
                                xport_gte_write_data(0, val);
                                memcpy(&val, stream + 52, sizeof(val));
                                xport_gte_write_data(1, val);
                            }
                            xport_gte_execute(n_commands[corner]);
                            n_x = (sint16)xport_gte_read_data(9);
                            n_y = (sint16)xport_gte_read_data(10);
                            xport_gte_read_data(11);
                            pixel_x = packet_points[corner].xy.x;
                            product = ((uint32)pixel_x - (uint32)center_x) * scale_x;
                            n_product = (uint32)(sint32)n_x * n_scale;
                            coord_u = ((sint32)product / 4096);
                            coord_u += (uint32)((sint32)n_product / 4096) + tex_u + (uint32)tex_bias;
                            if (corner < 2)
                            {
                                pixel_y = packet_points[corner].xy.y;
                                y_product = ((uint32)pixel_y - (uint32)center_y) * scale_y;
                                packet_points[corner].uv.u = (uint8)(coord_u);
                            }
                            else
                            {
                                packet_points[corner].uv.u = (uint8)(coord_u);
                                pixel_y = packet_points[corner].xy.y;
                                y_product = ((uint32)pixel_y - (uint32)center_y) * scale_y;
                            }
                            n_product = (uint32)(sint32)n_y * n_scale;
                            coord_v = ((sint32)y_product / 4096);
                            coord_v += (uint32)((sint32)n_product / 4096) + tex_v + (uint32)tex_bias;
                            packet_points[corner].uv.v = (uint8)(coord_v);
                        }
                        val = (uint32)car_environment_render.tint.r | ((uint32)car_environment_render.tint.g << 8) | ((uint32)car_environment_render.tint.b << 16) | ((uint32)car_environment_render.tint.cd << 24);
                        packet_color->rgbc = (uint32)(val | (kind == 2 ? 0x26000000 : 0x2E000000));
                    }
                }
                if (kind == 2)
                {
                    ++poly3;
                    packet_tag = &poly3->tag;
                    packet_color = &poly3->rgbc;
                    packet_points = poly3->p;
                }
                else
                {
                    ++poly4;
                    packet_tag = &poly4->tag;
                    packet_color = &poly4->rgbc;
                    packet_points = poly4->p;
                }
                stream += kind == 2 ? 44 : 56;
            }
        }
        result = stream[3];
    } while (result != 0);
    return result;
}

typedef struct CAR_ENVIRONMENT_WORK
{
    VEC3 pos;
    uint32 area0;
    uint32 depth;
    uint32 u[4];
    uint32 v[4];
    uint32 area1;
    uint32 ot_shift;
    uint32 far_depth;
    uint32 u_mask;
    uint32 v_mask;
    uint32 triangle_color;
    uint32 quad_color;
    uint32 link_mask;
    uint32 cross_product;
} CAR_ENVIRONMENT_WORK;

static void environment_link_packet(uint32 *packet_tag, const CAR_ENVIRONMENT_WORK *work, DISPLAY_ORDERING ordering)
{
    uint32 depth = work->depth;
    uint32 shift = work->ot_shift;
    uint32 prev = *packet_tag;
    uint32 val, mask;

    ordering.byte_offset += (uint32)((sint32)depth >> (shift & 31)) * 4;
    val = ordering.tags[(sint32)ordering.byte_offset / 4];
    mask = work->link_mask;
    *packet_tag = ((prev & 0xFF000000) | (val & mask));
    val = ordering.tags[(sint32)ordering.byte_offset / 4];
    mask = work->link_mask;
    ordering.tags[(sint32)ordering.byte_offset / 4] = (val & 0xFF000000) | (gpu_packet_link((uintptr_t)packet_tag) & mask);
}

static void environment_map_uv(const uint8 *prim, const MODEL_RESOURCE *model, uint32 vertex_offset, POINT_T *packet_points, RGBC *packet_color, CAR_ENVIRONMENT_WORK *work, uint32 count, uint32 c, uint32 s, uint32 shift, uint32 origin_u, uint32 origin_v, uint32 tex_u, uint32 tex_v)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3;

    uint32 u[4], v[4];
    uint32 idx, x, z, product, cross, minimum_u, minimum_v;
    const uint8 *vertex;
    uint32 mask_u, mask_v, val, color;
    uint32 model_offset;

    for (idx = 0; idx < count; ++idx)
    {
        model_offset = (vertex_offset + (uint32)(sint32)(sint16)(memcpy(&wire_halfword_1, prim + idx * 2, sizeof(wire_halfword_1)), wire_halfword_1) * 8);
        XASSERT(!(model_offset > model->capacity || model->capacity - model_offset < 6));
        vertex = model->bytes + model_offset;
        x = (sint16)(memcpy(&wire_halfword_2, vertex, sizeof(wire_halfword_2)), wire_halfword_2);
        z = (sint16)(memcpy(&wire_halfword_3, vertex + 4, sizeof(wire_halfword_3)), wire_halfword_3);
        cross = z * s;
        product = x * c - cross;
        if (idx == 3)
        {
            work->cross_product = (uint32)(cross);
            work->cross_product = (uint32)(z * c);
            cross = work->cross_product;
        }
        else
            cross = z * c;
        val = (uint32)((sint32)product / 4096) + origin_u;
        u[idx] = ((sint32)val >> (shift & 31));
        work->u[idx] = (uint32)(u[idx]);
        product = x * s + cross;
        val = (uint32)((sint32)product / 4096) + origin_v;
        v[idx] = ((sint32)val >> (shift & 31));
        work->v[idx] = (uint32)(v[idx]);
    }
    minimum_u = u[0];
    for (idx = 1; idx < count; ++idx)
        if ((sint32)u[idx] < (sint32)minimum_u)
            minimum_u = u[idx];
    mask_u = minimum_u & work->u_mask;
    minimum_v = count == 4 ? work->v[0] : v[0];
    for (idx = 1; idx < count; ++idx)
        if ((sint32)v[idx] < (sint32)minimum_v)
            minimum_v = v[idx];
    val = count == 4 ? work->u[0] : u[0];
    packet_points[0].uv.u = (uint8)(tex_u + val - mask_u);
    mask_v = minimum_v & work->v_mask;
    val = work->v[0];
    packet_points[0].uv.v = (uint8)(tex_v + val - mask_v);
    for (idx = 1; idx < count; ++idx)
    {
        val = work->u[idx];
        packet_points[idx].uv.u = (uint8)(tex_u + val - mask_u);
        val = work->v[idx];
        if (idx + 1 == count)
        {
            color = (count == 3 ? work->triangle_color : work->quad_color);
            packet_color->rgbc = color;
        }
        packet_points[idx].uv.v = (uint8)(tex_v + val - mask_v);
    }
}

static uint32 draw_environment_model(const MODEL_RESOURCE *model, CAR_STATE *car)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10;

    uint32 wire_word_17, wire_word_18, wire_word_19, wire_word_20, wire_word_21, wire_word_22, wire_word_23, wire_word_24;
    VEC3 model_pos;
    DISPLAY_ORDERING ordering;
    CAR_ENVIRONMENT_WORK work;
    uint32 maximum = car->environment.limit;
    uint32 state = car->environment.phase;
    const uint8 *tex = car_environment_render.tex;
    uint32 val, other, mode, c, s;
    uint32 angle = 0;
    uint32 uv_shift = 3, origin_v = 0, origin_u = 0;
    const uint8 *desc;
    uint32 lighting_offset;
    uint32 groups_offset, vertex_offset, model_offset;
    const uint8 *stream;
    const uint8 *prim;
    uint32 tex_u, tex_v, far_shift, tag;
    POLY_FT3 *poly3;
    POLY_FT4 *poly4;
    POINT_T *packet_points;
    RGBC *packet_color;
    uint32 *packet_tag;
    uint32 kind, idx, product, scr_x, scr_y, count;
    const uint8 *vertex[3];
    const uint8 *point;
    uint32 x, y, z, relative_x, relative_y, relative_z;
    sint32 level, amount;
    VECTOR input, output;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x8002FE8C, "SLUS_006.10");
    ++maximum;
    if (state == 1)
    {
        level = (sint16)car->environment.level;
        if (level < (sint32)maximum)
        {
            val = (uint32)level + (maximum >> 4);
            car->environment.level = (uint16)(val);
            level = (sint16)val;
        }
        if (level >= (sint32)maximum)
        {
            car->environment.phase = (uint8)(0);
            car->environment.level = (uint16)(maximum);
        }
    }
    else if (state == 2)
    {
        level = (sint16)car->environment.level;
        if (level > 0)
        {
            val = (uint32)level - 16;
            car->environment.level = (uint16)(val);
            level = (sint16)val;
        }
        if (level <= 0)
        {
            car->environment.phase = (uint8)(1);
            val = car->lighting_zone;
            desc = car_lighting_zones.bytes + car_lighting_zones.offset;
            car->environment.level = (uint16)(0);
            lighting_offset = car_lighting_zones.offset + val * 32;
            XASSERT(!(lighting_offset > car_lighting_zones.size || car_lighting_zones.size - lighting_offset < 32));
            memcpy(&val, desc + val * 32 + 28, sizeof(val));
            car_load_environment(car, val);
        }
    }
    car_environment_render.tint.r = (uint8)(car->environment.level);
    car_environment_render.tint.g = (uint8)(car->environment.level);
    car_environment_render.tint.b = (uint8)(car->environment.level);
    mode = car->environment.type;
    if (mode >= 3)
    {
        work.u_mask = (uint32)(0);
        work.v_mask = (uint32)(0);
    }
    XASSERT(!(model->capacity < 60));
    if (mode == 0)
    {
        val = (uint32)(car->pos.x / 256);
        other = model_render.view.sdk.t[0];
        val -= other;
        relative_x = ((sint32)val / 4);
        work.pos.bits.x = (uint32)(relative_x);
        val = (uint32)(car->pos.y / 256);
        other = model_render.view.sdk.t[1];
        val -= other;
        relative_y = ((sint32)val / 4);
        work.pos.bits.y = (uint32)(relative_y);
        val = (uint32)(car->pos.z / 256);
        other = model_render.view.sdk.t[2];
        val -= other;
        relative_z = ((sint32)val / 4);
        work.pos.bits.z = (uint32)(relative_z);
        input.vx = (sint32)work.pos.bits.x;
        input.vy = (sint32)work.pos.bits.y;
        input.vz = (sint32)work.pos.bits.z;
        ApplyMatrixLV(&model_render.view.sdk, &input, &output);
        work.pos.bits.x = (uint32)((uint32)output.vx);
        work.pos.bits.y = (uint32)((uint32)output.vy);
        work.pos.bits.z = (uint32)((uint32)output.vz);
        z = work.pos.bits.z;
        scr_x = 0;
        scr_y = 0;
        if (z != 0)
        {
            x = work.pos.bits.x;
            val = model_render.proj_distance;
            scr_x = (uint32)fixed_divide_checked((sint32)(x * val), (sint32)z);
            y = work.pos.bits.y;
            scr_y = (uint32)fixed_divide_checked((sint32)(y * val), (sint32)z);
        }
        val = model_render.width;
        other = model_render.height;
        car_environment_render.mapping.center_x = (sint16)(uint16)(scr_x + (uint32)((sint32)val / 2));
        car_environment_render.mapping.center_y = (sint16)(uint16)(scr_y + (uint32)((sint32)other / 2));
        model_pos.bits.x = ((const MODEL_HEADER *)model->bytes)->position.bits.x;
        model_pos.bits.y = ((const MODEL_HEADER *)model->bytes)->position.bits.y;
        model_pos.bits.z = ((const MODEL_HEADER *)model->bytes)->position.bits.z;
        model_transform_matrix(&car->transform, &model_pos);
        val = fixed_sqrt(relative_x * relative_x + relative_y * relative_y + relative_z * relative_z);
        hd_environment_reflection(model, &car_environment_render.mapping, 1);
        return car_draw_environment_mapping(model, &car_environment_render.mapping, val * 4);
    }
    if (mode == 1)
    {
        angle = (sint32)car->angles.vy;
        s = (uint32)rsin((sint32)angle);
        c = (uint32)rcos((sint32)angle);
        val = (uint32)(car->velocity.x / 256);
        other = car->velocity.bits.z;
        product = s * val + c * (uint32)((sint32)other / 256);
        val = ((sint32)product / 16);
        angle = 0;
        val = car_environment_render.reflection_phase + (uint32)((sint32)val / 32);
        uv_shift = 3;
        car_environment_render.reflection_phase = (val);
        origin_u = 512;
        origin_v = ((sint32)val / 256);
        work.u_mask = (uint32)(0xFFFFFF80);
        work.v_mask = (uint32)(0xFFFFFFC0);
    }
    else if (mode == 2)
    {
        val = car->orientation.bits.y;
        other = ((const MODEL_HEADER *)model->bytes)->position.bits.x;
        uv_shift = 3;
        angle = ((sint32)(0 - val) >> 8);
        val = ((const MODEL_HEADER *)model->bytes)->position.bits.z;
        origin_u = ((sint32)other / 8192);
        origin_v = ((sint32)val / 8192);
        work.u_mask = (uint32)(0xFFFFFFC0);
        work.v_mask = (uint32)(0xFFFFFFC0);
    }
    hd_environment_surface(model, mode == 2 ? -car->orientation.y / 256.0f * RAD12 : 0.0f, mode == 2 ? ((const MODEL_HEADER *)model->bytes)->position.x / 8192.0f : (float)(sint32)origin_u, mode == 1 ? (sint32)car_environment_render.reflection_phase / 256.0f : mode == 2 ? ((const MODEL_HEADER *)model->bytes)->position.z / 8192.0f : (float)(sint32)origin_v, mode == 1 ? 128 : mode == 2 ? 64 : 0, mode == 1 || mode == 2 ? 64 : 0);
    c = (uint32)rcos((sint32)angle);
    s = (uint32)rsin((sint32)angle);
    val = ((const MODEL_HEADER *)model->bytes)->stream_offset;
    groups_offset = val;
    tex_u = tex[14];
    tex_v = tex[15];
    XASSERT(!(groups_offset > model->capacity || model->capacity - groups_offset < 4));
    stream = model->bytes + groups_offset;
    val = (uint32)car_environment_render.tint.r | ((uint32)car_environment_render.tint.g << 8) | ((uint32)car_environment_render.tint.b << 16) | ((uint32)car_environment_render.tint.cd << 24);
    other = model_render.ot_shift;
    work.triangle_color = (uint32)(val | 0x26000000);
    work.quad_color = (uint32)(val | 0x2E000000);
    val = ((const MODEL_HEADER *)model->bytes)->vertex_offset;
    far_shift = model_render.coord_shift;
    amount = (sint16)(memcpy(&wire_halfword_1, model->bytes + 12, sizeof(wire_halfword_1)), wire_halfword_1);
    cur_frame = disp_state.cur;
    vertex_offset = val;
    other -= far_shift;
    val = (uint32)(amount >> (other & 31)) * 4;
    work.ot_shift = (uint32)(other);
    ordering.tags = cur_frame->ordering;
    other = model_render.far_depth;
    tag = stream[3];
    ordering.byte_offset = val;
    work.far_depth = (uint32)((uint32)((sint32)other >> ((far_shift + 2) & 31)));
    if (tag == 0)
        return val;
    work.link_mask = (uint32)(0xFFFFFF);
    do
    {
        kind = stream[2];
        amount = (sint16)(memcpy(&wire_halfword_2, stream, sizeof(wire_halfword_2)), wire_halfword_2);
        stream += 4;
        if (kind == 0 || kind == 1)
            stream += (uint32)amount * 20;
        else if (kind == 2 || kind == 3)
        {
            prim = stream;
            if (kind == 2)
            {
                poly3 = car_environment_render.buffers[disp_state.bank].triangles;
                poly4 = NULL;
                packet_tag = &poly3->tag;
                packet_color = &poly3->rgbc;
                packet_points = poly3->p;
            }
            else
            {
                poly4 = car_environment_render.buffers[disp_state.bank].quads;
                poly3 = NULL;
                packet_tag = &poly4->tag;
                packet_color = &poly4->rgbc;
                packet_points = poly4->p;
            }
            count = kind == 2 ? 3 : 4;
            for (idx = 0; (sint32)idx < amount; ++idx)
            {
                if ((sint16)(memcpy(&wire_halfword_3, prim + (kind == 2 ? 22 : 26), sizeof(wire_halfword_3)), wire_halfword_3) >= 151 && (sint16)(memcpy(&wire_halfword_4, prim + (kind == 2 ? 30 : 34), sizeof(wire_halfword_4)), wire_halfword_4) >= 151 && (sint16)(memcpy(&wire_halfword_5, prim + (kind == 2 ? 38 : 42), sizeof(wire_halfword_5)), wire_halfword_5) >= 151 && (kind == 2 || (sint16)(memcpy(&wire_halfword_6, prim + 50, sizeof(wire_halfword_6)), wire_halfword_6) >= 151))
                {
                    model_offset = (vertex_offset + (uint32)(sint32)(sint16)(memcpy(&wire_halfword_7, prim, sizeof(wire_halfword_7)), wire_halfword_7) * 8);
                    XASSERT(!(model_offset > model->capacity || model->capacity - model_offset < 8));
                    vertex[0] = model->bytes + model_offset;
                    model_offset = (vertex_offset + (uint32)(sint32)(sint16)(memcpy(&wire_halfword_8, prim + 2, sizeof(wire_halfword_8)), wire_halfword_8) * 8);
                    XASSERT(!(model_offset > model->capacity || model->capacity - model_offset < 8));
                    vertex[1] = model->bytes + model_offset;
                    model_offset = (vertex_offset + (uint32)(sint32)(sint16)(memcpy(&wire_halfword_9, prim + 4, sizeof(wire_halfword_9)), wire_halfword_9) * 8);
                    XASSERT(!(model_offset > model->capacity || model->capacity - model_offset < 8));
                    vertex[2] = model->bytes + model_offset;
                    xport_gte_write_data(0, (memcpy(&wire_word_17, vertex[0], sizeof(wire_word_17)), wire_word_17));
                    xport_gte_write_data(1, (memcpy(&wire_word_18, vertex[0] + 4, sizeof(wire_word_18)), wire_word_18));
                    xport_gte_write_data(2, (memcpy(&wire_word_19, vertex[1], sizeof(wire_word_19)), wire_word_19));
                    xport_gte_write_data(3, (memcpy(&wire_word_20, vertex[1] + 4, sizeof(wire_word_20)), wire_word_20));
                    xport_gte_write_data(4, (memcpy(&wire_word_21, vertex[2], sizeof(wire_word_21)), wire_word_21));
                    xport_gte_write_data(5, (memcpy(&wire_word_22, vertex[2] + 4, sizeof(wire_word_22)), wire_word_22));
                    xport_gte_execute(0x280030);
                    xport_gte_execute(0x1400006);
                    work.area0 = (uint32)(xport_gte_read_data(24));
                    if (kind == 3)
                    {
                        packet_points[0].xy.xy = (uint32)(xport_gte_read_data(12));
                        packet_points[1].xy.xy = (uint32)(xport_gte_read_data(13));
                        packet_points[2].xy.xy = (uint32)(xport_gte_read_data(14));
                        model_offset = (vertex_offset + (uint32)(sint32)(sint16)(memcpy(&wire_halfword_10, prim + 6, sizeof(wire_halfword_10)), wire_halfword_10) * 8);
                        XASSERT(!(model_offset > model->capacity || model->capacity - model_offset < 8));
                        point = model->bytes + model_offset;
                        xport_gte_write_data(0, (memcpy(&wire_word_23, point, sizeof(wire_word_23)), wire_word_23));
                        xport_gte_write_data(1, (memcpy(&wire_word_24, point + 4, sizeof(wire_word_24)), wire_word_24));
                        xport_gte_execute(0x180001);
                        xport_gte_execute(0x1400006);
                        work.area1 = (uint32)(xport_gte_read_data(24));
                    }
                    xport_gte_execute(kind == 2 ? 0x158002D : 0x168002E);
                    if ((sint32)work.area0 <= 0 || (kind == 3 && (sint32)work.area1 >= 0))
                    {
                        work.depth = (uint32)(xport_gte_read_data(7));
                        val = work.depth;
                        other = work.far_depth;
                        if ((sint32)val < (sint32)other)
                        {
                            if (kind == 2)
                            {
                                packet_points[0].xy.xy = (uint32)(xport_gte_read_data(12));
                                packet_points[1].xy.xy = (uint32)(xport_gte_read_data(13));
                                packet_points[2].xy.xy = (uint32)(xport_gte_read_data(14));
                            }
                            else
                                packet_points[3].xy.xy = (uint32)(xport_gte_read_data(14));
                            environment_link_packet(packet_tag, &work, ordering);
                            environment_map_uv(prim, model, vertex_offset, packet_points, packet_color, &work, count, c, s, uv_shift, origin_u, origin_v, tex_u, tex_v);
                            if (kind == 2)
                            {
                                ++poly3;
                                packet_tag = &poly3->tag;
                                packet_color = &poly3->rgbc;
                                packet_points = poly3->p;
                            }
                            else
                            {
                                ++poly4;
                                packet_tag = &poly4->tag;
                                packet_color = &poly4->rgbc;
                                packet_points = poly4->p;
                            }
                        }
                    }
                }
                prim += kind == 2 ? 44 : 56;
            }
            stream += (uint32)amount * (kind == 2 ? 44 : 56);
        }
        val = stream[3];
    } while (val != 0);
    return val;
}

static uint32 init_shadow_packets(void)
{
    const uint8 *tex;
    uint32 initial_u;
    uint32 v;
    uint32 slot;
    uint32 buffer;
    uint32 half;
    uint32 u;
    POLY_FT4 *poly;
    uint32 page;
    uint32 clut;
    uint32 code;

    FUNCTION_MARKER(0x80032090, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&shadow_render, (uint32)sizeof(shadow_render)));
        XASSERT(operation_ok);
    }
    tex = tex_find(shadow_tex_name);
    initial_u = tex[14];
    v = tex[15];
    for (slot = 0; slot < 14; ++slot)
    {
        for (buffer = 0; buffer < 2; ++buffer)
        {
            u = initial_u;
            for (half = 0; half < 2; ++half)
            {
                poly = shadow_render.buffers[buffer] + slot * 2 + half;
                setPolyFT4(poly);
                page = ((const TEXTURE_DESCRIPTOR *)tex)->page;
                poly->p[1].attr = (uint16)(page | 0x40);
                clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
                code = poly->rgbc.code;
                poly->p[0].uv.u = (uint8)(u);
                poly->p[1].uv.u = (uint8)(u);
                poly->rgbc.r = (uint8)(128);
                poly->rgbc.g = (uint8)(128);
                poly->rgbc.b = (uint8)(128);
                poly->p[0].uv.v = (uint8)(v);
                poly->p[1].uv.v = (uint8)(v + 31);
                poly->p[2].uv.u = (uint8)(u + 31);
                poly->p[2].uv.v = (uint8)(v);
                poly->p[3].uv.u = (uint8)(u + 31);
                poly->p[3].uv.v = (uint8)(v + 31);
                poly->rgbc.code = (uint8)((code & 0xFE) | 2);
                poly->p[0].attr = (uint16)(clut);
                u += 32;
            }
        }
    }
    return 0;
}

void car_draw_hd_shadow(uint32 slot, uint32 kind)
{
}

void car_draw_hd_candidate_lamps(uint32 slot, float police_signal)
{
    const CAR_STATE *car;
    const uint8 *header;
    uint32 active = 0, flags, type, stage, index, point;
    sint32 intensity = -64, color = 32, fade, height_checked = 0, height_valid = 0;
    float height = 0.0f;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 14);
    car = &car_states[slot];
    header = scene_level_image.bytes + scene_level_image.desc.header_offset;
    flags = header[30];
    if (slot == 0 && (input_ports[0].controls & 0x1000) == 0)
        active = car->braking != 0;
    if (slot == 1 && race_session.two_players != 0 && (input_ports[1].controls & 0x1000) == 0 && car->braking != 0)
        active = 1;
    if ((flags & 1) != 0)
    {
        color = 96;
        intensity = 80;
    }
    else if ((flags & 2) != 0)
    {
        color = 48;
        intensity = 32;
    }
    if (active)
    {
        color += 48;
        intensity += 48;
    }
    fade = intensity - rear_light_fade[slot];
    if (fade < 0)
        fade = 0;
    if (fade > 128)
        fade = 128;
    for (type = 0; type < 2; ++type)
    {
        if ((type == 0 && !active && (flags & 3) == 0) || (type == 1 && (flags & 1) == 0))
            continue;
        for (stage = 0; stage < 2; ++stage)
        {
            if (type == 0 && stage == 1 && fade == 0)
                continue;
            if (stage == 1 && !height_checked)
            {
                height_valid = track_render_surface_height(&car->track_location, &car->pos, &height);
                height_checked = 1;
            }
            if (stage == 1 && !height_valid)
                continue;
            for (index = 0; index < 2; ++index)
            {
                uint32 offset = slot * 4 + index * 2 + disp_state.bank;
                const POLY_FT4 *source = &light_packets[type].billboards[offset];
                const SVECTOR *lamp = &car_lamp_points[slot][index + type * 2];
                POLY_FT4 material = {0};
                POINT_CT gradient[4] = {{0}};
                material.rgbc = source->rgbc;
                if (type == 0)
                    material.rgbc.r = material.rgbc.g = material.rgbc.b = (uint8)color;
                for (point = 0; point < 4; ++point)
                {
                    material.p[point].uv = source->p[point].uv;
                    material.p[point].attr = source->p[point].attr;
                    gradient[point].rgbc = light_packets[type].projections[offset].p[point].rgbc;
                    if (type == 0)
                        gradient[point].rgbc.r = gradient[point].rgbc.g = gradient[point].rgbc.b = point < 2 ? 0 : (uint8)fade;
                }
                if (type == 0 && stage == 1)
                    draw_hd_rear_projection(slot, lamp, height, rear_light_geometry[stage], &material, gradient);
                else
                    draw_hd_lamp(lamp, stage == 0 ? (float)lamp->vy : height - car->pos.y / 256.0f + car->suspension_local[(index ^ 1) + 2].vy - lamp->vy + 120.0f, type == 0 ? rear_light_geometry[stage] : headlight_geometry[stage], &material, stage == 0 ? NULL : gradient, type == 0);
            }
        }
    }
    if (police_signal != 0.0f)
    {
        XASSERT(police_light_config.data.count <= 4);
        for (index = 0; index < police_light_config.data.count; ++index)
        {
            const CAR_POLICE_LIGHT_PACKETS *packets = &police_light_packets[index];
            const POLY_FT4 *billboard = &packets->billboards[disp_state.bank];
            HD_TEXTURE texture = hd_quad_texture(billboard->p, &billboard->rgbc);
            RGBC colors[3] = {billboard->rgbc, packets->rays[1][disp_state.bank].p[0].rgbc, packets->rays[0][disp_state.bank].p[0].rgbc};
            uint32 ray_flags[2] = {(colors[1].code & 2) != 0 ? HD_SEMITRANSPARENT : 0, (colors[2].code & 2) != 0 ? HD_SEMITRANSPARENT : 0};
            uint32 phase = (uint8)police_light_phases[index], tint = police_light_config.data.colors[index];
            uint32 product = (uint32)rsin(-(sint32)phase * 4) * 255u, brightness, rgb[3], part, channel;
            if ((sint32)product < 0)
                product += 4095;
            brightness = product >> 12;
            if (tint < 4)
            {
                rgb[0] = tint == 0 || tint == 3 ? brightness : 0;
                rgb[1] = tint == 2 ? brightness : tint == 3 ? (brightness & 255) >> 1 : 0;
                rgb[2] = tint == 1 ? brightness : 0;
                for (part = 0; part < 3; ++part)
                    for (channel = 0; channel < 3; ++channel)
                        ((uint8 *)&colors[part])[channel] = (uint8)rgb[channel];
            }
            hd_police_light(index, &police_light_config.data.positions[index], &texture, colors, ray_flags, police_signal);
        }
    }
    else
        hd_police_retire();
}

static void draw_contact_shadow(uint32 slot)
{
    DISPLAY_ORDERING ordering;
    CAR_STATE *car = &car_states[slot];
    uint32 val, cur_buffer, vertex, axis, phase, base_vertex;
    POLY_FT4 *poly;
    uint32 tag, offset, shift, other_shift;
    VEC3 world[4], center;
    sint32 depth, far_plane;
    VEC3_16 points[6];
    VECTOR relative = {0}, t = {0};

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x800321D0, "SLUS_006.10");
    car_draw_hd_shadow(slot, 0);
    cur_buffer = disp_state.bank;
    world[0].x = car->body_contact_pos[0].x >> 8;
    poly = shadow_render.buffers[cur_buffer] + slot * 2;
    for (axis = 1; axis < 3; ++axis)
        world[0].v[axis] = car->body_contact_pos[0].v[axis] >> 8;
    for (vertex = 1; vertex < 4; ++vertex)
        for (axis = 0; axis < 3; ++axis)
            world[vertex].v[axis] = car->body_contact_pos[vertex].v[axis] >> 8;
    for (axis = 0; axis < 3; ++axis)
    {
        val = (uint32)world[0].v[axis] + (uint32)world[1].v[axis];
        val += (uint32)world[2].v[axis] + (uint32)world[3].v[axis];
        center.v[axis] = (sint32)val >> 2;
        points[0].v[axis] = (sint16)((uint32)world[0].v[axis] - (uint32)center.v[axis]);
        points[1].v[axis] = (sint16)((uint32)world[1].v[axis] - (uint32)center.v[axis]);
        points[4].v[axis] = (sint16)((uint32)world[2].v[axis] - (uint32)center.v[axis]);
        points[5].v[axis] = (sint16)((uint32)world[3].v[axis] - (uint32)center.v[axis]);
        points[2].v[axis] = (sint16)(((sint32)points[0].v[axis] + points[4].v[axis]) / 2);
        points[3].v[axis] = (sint16)(((sint32)points[1].v[axis] + points[5].v[axis]) / 2);
    }
    val = model_render.view.sdk.t[0];
    relative.vx = (sint32)((uint32)center.x - val);
    val = model_render.view.sdk.t[1];
    relative.vy = (sint32)((uint32)center.y - val);
    val = model_render.view.sdk.t[2];
    relative.vz = (sint32)((uint32)center.z - val);
    SetRotMatrix(&model_render.view.sdk);
    ApplyMatrixLV(&model_render.view.sdk, &relative, &t);
    model_render.matrix.sdk.t[0] = t.vx;
    model_render.matrix.sdk.t[1] = t.vy;
    model_render.matrix.sdk.t[2] = t.vz;
    val = model_render.matrix.sdk.t[0];
    tag = model_render.matrix.sdk.t[1];
    xport_gte_write_control(5, val);
    val = model_render.matrix.sdk.t[2];
    xport_gte_write_control(6, tag);
    xport_gte_write_control(7, val);
    for (phase = 0; phase < 2; ++phase)
    {
        base_vertex = phase * 2;
        for (vertex = 0; vertex < 3; ++vertex)
        {
            val = (uint16)points[base_vertex + vertex].x | ((uint32)(uint16)points[base_vertex + vertex].y << 16);
            xport_gte_write_data(vertex * 2, val);
            xport_gte_write_data(vertex * 2 + 1, (uint32)(sint32)points[base_vertex + vertex].z);
        }
        xport_gte_execute(0x280030);
        poly->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
        poly->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
        poly->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
        val = (uint16)points[base_vertex + 3].x | ((uint32)(uint16)points[base_vertex + 3].y << 16);
        xport_gte_write_data(0, val);
        xport_gte_write_data(1, (uint32)(sint32)points[base_vertex + 3].z);
        xport_gte_execute(0x180001);
        poly->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
        xport_gte_execute(0x168002E);
        depth = (sint32)xport_gte_read_data(7);
        if (depth > 0)
        {
            if (phase == 0)
                far_plane = (sint32)model_render.far_depth >> 2;
            else
            {
                shift = model_render.ot_shift;
                far_plane = (sint32)model_render.far_depth >> ((shift + 2) & 31);
            }
            if (depth < far_plane)
            {
                cur_frame = disp_state.cur;
                if (phase == 0)
                    shift = model_render.ot_shift;
                other_shift = model_render.coord_shift;
                tag = poly->tag;
                ordering.tags = cur_frame->ordering;
                ordering.byte_offset = 0;
                offset = (uint32)(depth >> ((shift - other_shift) & 31)) << 2;
                val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
                poly->tag = (uint32)((tag & 0xFF000000) | (val & 0xFFFFFF));
                ordering.tags = cur_frame->ordering;
                ordering.byte_offset = 0;
                val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
                val = (val & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
                ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4] = val;
            }
        }
        ++poly;
    }
    return;
}

static void draw_body_shadow(uint32 slot)
{
    DISPLAY_ORDERING pointer;
    SVECTOR points[6] = {0};
    uint32 buffer, quad, vertex, point, val, saved_tag, coord_shift;
    POLY_FT4 *poly;
    uint32 depth, order_shift, limit, offset, tag, result;
    sint32 width, height, len;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x800326D0, "SLUS_006.10");
    car_draw_hd_shadow(slot, 1);
    buffer = disp_state.bank;

    poly = shadow_render.buffers[buffer];
    width = car_body_points[slot][1].vx;
    height = car_body_points[slot][1].vy;
    len = car_body_points[slot][1].vz;
    poly += slot * 2;
    for (point = 0; point < 6; ++point)
    {
        points[point].vx = (sint16)((point & 1) != 0 ? width : -width);
        points[point].vy = (sint16)height;
        points[point].vz = (sint16)(point < 2 ? len : point < 4 ? 0 : -len);
    }
    for (quad = 0; quad < 2; ++quad)
    {
        for (vertex = 0; vertex < 3; ++vertex)
        {
            point = quad * 2 + vertex;
            val = (uint32)(uint16)points[point].vx | ((uint32)(uint16)points[point].vy << 16);
            xport_gte_write_data(vertex * 2, val);
            xport_gte_write_data(vertex * 2 + 1, (uint32)(uint16)points[point].vz);
        }
        xport_gte_execute(0x280030);
        poly->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
        poly->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
        poly->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
        point = quad * 2 + 3;
        val = (uint32)(uint16)points[point].vx | ((uint32)(uint16)points[point].vy << 16);
        xport_gte_write_data(0, val);
        xport_gte_write_data(1, (uint32)(uint16)points[point].vz);
        xport_gte_execute(0x180001);
        poly->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
        xport_gte_execute(0x168002E);
        depth = xport_gte_read_data(7);
        if ((sint32)depth > 0)
        {
            order_shift = model_render.ot_shift;
            limit = model_render.far_depth;
            limit = ((sint32)limit >> ((order_shift + 2) & 31));
            result = (sint32)depth < (sint32)limit;
            if (result != 0)
            {
                cur_frame = disp_state.cur;
                coord_shift = model_render.coord_shift;
                saved_tag = poly->tag;
                pointer.tags = cur_frame->ordering;
                pointer.byte_offset = 0;
                offset = (uint32)((sint32)depth >> ((order_shift - coord_shift) & 31)) * 4;
                tag = pointer.tags[(sint32)(pointer.byte_offset + (uint32)(offset + 40)) / 4];
                poly->tag = (uint32)((saved_tag & 0xFF000000) | (tag & 0xFFFFFF));
                pointer.tags = cur_frame->ordering;
                pointer.byte_offset = (uint32)(offset);
                tag = pointer.tags[(sint32)(pointer.byte_offset + (uint32)(40)) / 4];
                result = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
                pointer.tags[(sint32)(pointer.byte_offset + (uint32)(40)) / 4] = result;
            }
        }
        ++poly;
    }
    return;
}

static void draw_ground_shadow(CAR_STATE *car)
{
    DISPLAY_ORDERING pointer;
    VEC3_16 points[6] = {0};
    VECTOR relative = {0}, t = {0};
    sint32 heading, maximum_x = 0, maximum_z = 0, local_x, local_z, divisor;
    uint32 c, s, center_x, center_z, x, z, buffer, slot, n_y;
    uint32 result, vertex, point, quad, axis, val, val0, val1;
    POLY_FT4 *poly;
    uint32 xc, zs, negative_xs, zc, negative_xc, xs, nx, nz;
    uint32 depth, order_shift, limit, offset, tag;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80032984, "SLUS_006.10");
    car_draw_hd_shadow(car->slot, 2);
    heading = car->angles.vy;
    c = (uint32)rcos(heading);
    s = (uint32)rsin(heading);
    center_x = car->pos.bits.x;
    center_z = car->pos.bits.z;
    for (vertex = 0; vertex < 8; ++vertex)
    {
        x = car->body_contact_pos[vertex].bits.x;
        x = ((sint32)(x - center_x) >> 8);
        z = car->body_contact_pos[vertex].bits.z;
        z = ((sint32)(z - center_z) >> 8);
        local_x = (sint32)(x * c - z * s) >> 12;
        if (local_x < 0)
            local_x = (0u - (uint32)local_x);
        if (local_x > maximum_x)
            maximum_x = local_x;
        local_z = (sint32)(x * s + z * c) >> 12;
        if (local_z < 0)
            local_z = (0u - (uint32)local_z);
        if (local_z > maximum_z)
            maximum_z = local_z;
    }
    buffer = disp_state.bank;
    slot = car->slot;
    n_y = (uint32)car->ground_n.vy;
    poly = shadow_render.buffers[buffer];
    result = slot * 80;
    poly += result / 40;
    if (n_y == 0)
        return;
    xc = (uint32)maximum_x * c;
    zs = (uint32)maximum_z * s;
    negative_xs = (0u - (uint32)maximum_x) * s;
    zc = (uint32)maximum_z * c;
    negative_xc = (0u - (uint32)maximum_x) * c;
    xs = (uint32)maximum_x * s;
    points[0].x = (sint16)((sint32)(xc + zs) >> 12);
    points[0].z = (sint16)((sint32)(negative_xs + zc) >> 12);
    points[1].x = (sint16)((sint32)(negative_xc + zs) >> 12);
    points[1].z = (sint16)((sint32)(xs + zc) >> 12);
    points[4].x = (sint16)((sint32)(xc - zs) >> 12);
    points[4].z = (sint16)((sint32)(negative_xs - zc) >> 12);
    points[5].x = (sint16)((sint32)(negative_xc - zs) >> 12);
    points[5].z = (sint16)((sint32)(xs - zc) >> 12);
    for (vertex = 0; vertex < 4; ++vertex)
    {
        point = vertex < 2 ? vertex : vertex + 2;
        nx = (uint32)car->ground_n.vx;
        nz = (uint32)car->ground_n.vz;
        divisor = car->ground_n.vy;
        val = (uint32)(sint32)points[point].x * nx;
        val += (uint32)(sint32)points[point].z * nz;
        points[point].y = (sint16)fixed_divide_checked((sint32)(0u - val), divisor);
    }
    for (axis = 0; axis < 3; ++axis)
    {
        points[2].v[axis] = (sint16)(((sint32)points[0].v[axis] + points[4].v[axis]) / 2);
        points[3].v[axis] = (sint16)(((sint32)points[1].v[axis] + points[5].v[axis]) / 2);
    }
    val = (uint32)(car->pos.x >> 8);
    val0 = model_render.view.sdk.t[0];
    relative.vx = (sint32)(val - val0);
    val = ((sint32)car->ground_height >> 8);
    val0 = model_render.view.sdk.t[1];
    relative.vy = (sint32)(val - val0);
    val = (uint32)(car->pos.z >> 8);
    val0 = model_render.view.sdk.t[2];
    relative.vz = (sint32)(val - val0);
    SetRotMatrix(&model_render.view.sdk);
    ApplyMatrixLV(&model_render.view.sdk, &relative, &t);
    model_render.matrix.sdk.t[0] = t.vx;
    model_render.matrix.sdk.t[1] = t.vy;
    model_render.matrix.sdk.t[2] = t.vz;
    val0 = model_render.matrix.sdk.t[0];
    val1 = model_render.matrix.sdk.t[1];
    xport_gte_write_control(5, val0);
    val = model_render.matrix.sdk.t[2];
    xport_gte_write_control(6, val1);
    xport_gte_write_control(7, val);
    for (quad = 0; quad < 2; ++quad)
    {
        for (vertex = 0; vertex < 3; ++vertex)
        {
            point = quad * 2 + vertex;
            val = (uint32)(uint16)points[point].x | ((uint32)(uint16)points[point].y << 16);
            xport_gte_write_data(vertex * 2, val);
            xport_gte_write_data(vertex * 2 + 1, (uint32)(sint32)points[point].z);
        }
        xport_gte_execute(0x280030);
        poly->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
        poly->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
        poly->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
        point = quad * 2 + 3;
        val = (uint32)(uint16)points[point].x | ((uint32)(uint16)points[point].y << 16);
        xport_gte_write_data(0, val);
        xport_gte_write_data(1, (uint32)(sint32)points[point].z);
        xport_gte_execute(0x180001);
        poly->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
        xport_gte_execute(0x168002E);
        depth = xport_gte_read_data(7);
        if ((sint32)depth > 0)
        {
            order_shift = model_render.ot_shift;
            limit = model_render.far_depth;
            limit = ((sint32)limit >> ((order_shift + 2) & 31));
            result = (sint32)depth < (sint32)limit;
            if (result != 0)
            {
                cur_frame = disp_state.cur;
                val1 = model_render.coord_shift;
                val0 = poly->tag;
                pointer.tags = cur_frame->ordering;
                pointer.byte_offset = 0;
                offset = (uint32)((sint32)depth >> ((order_shift - val1) & 31)) * 4;
                tag = pointer.tags[(sint32)(pointer.byte_offset + (uint32)(offset)) / 4];
                poly->tag = (uint32)((val0 & 0xFF000000) | (tag & 0xFFFFFF));
                pointer.tags = cur_frame->ordering;
                pointer.byte_offset = (uint32)(offset);
                tag = pointer.tags[(sint32)pointer.byte_offset / 4];
                result = (tag & 0xFF000000) | gpu_packet_link((uintptr_t)poly);
                pointer.tags[(sint32)pointer.byte_offset / 4] = result;
            }
        }
        ++poly;
    }
    return;
}

void car_init_rear_light_packets(void)
{
    const uint8 *tex;
    uint32 result;
    uint32 idx;
    POLY_FT4 *flat_packet;
    POLY_GT4 *gouraud_packet;
    uint32 val;
    uint32 other;
    uint32 code;
    uint32 clut;
    sint32 count;

    FUNCTION_MARKER(0x8003FDF4, "SLUS_006.10");
    tex = tex_find(rear_light_tex_name);
    XASSERT(!(race_control.car_count > 14));
    {
        sint32 operation_ok = !(!gpu_register_packet_range(light_packets, (uint32)sizeof(light_packets)));
        XASSERT(operation_ok);
    }
    count = (sint32)(race_control.car_count << 2);
    idx = 0;
    if (count > 0)
    {
        for (;;)
        {
            flat_packet = light_packets[0].billboards + idx;
            rear_light_fade[(uint32)((sint32)idx / 4)] = 0;
            setlen(flat_packet, 9);
            flat_packet->rgbc.code = (uint8)(0x2C);
            flat_packet->p[0].uv.u = (uint8)(tex[14]);
            flat_packet->p[0].uv.v = (uint8)(tex[15]);
            val = tex[14];
            other = tex[12];
            flat_packet->p[1].uv.u = (uint8)(other + val - 1);
            flat_packet->p[1].uv.v = (uint8)(tex[15]);
            flat_packet->p[2].uv.u = (uint8)(tex[14]);
            val = tex[15];
            other = tex[13];
            flat_packet->p[2].uv.v = (uint8)(other + val - 1);
            val = tex[14];
            other = tex[12];
            flat_packet->p[3].uv.u = (uint8)(other + val - 1);
            val = tex[15];
            other = tex[13];
            flat_packet->p[3].uv.v = (uint8)(other + val - 1);
            val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
            code = flat_packet->rgbc.code;
            flat_packet->p[1].attr = (uint16)(val);
            clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
            flat_packet->rgbc.code = (uint8)((code & 0xFE) | 2);
            count = (sint32)(race_control.car_count << 2);
            ++idx;
            flat_packet->p[0].attr = (uint16)(clut);
            if ((sint32)idx >= count)
                break;
        }
    }
    count = (sint32)(race_control.car_count << 2);
    idx = 0;
    if (count > 0)
    {
        for (;;)
        {
            gouraud_packet = light_packets[0].projections + idx;
            setlen(gouraud_packet, 12);
            gouraud_packet->p[0].rgbc.code = (uint8)(0x3C);
            gouraud_packet->p[0].uv.u = (uint8)(tex[14]);
            gouraud_packet->p[0].uv.v = (uint8)(tex[15]);
            val = tex[14];
            other = tex[12];
            gouraud_packet->p[1].uv.u = (uint8)(other + val - 1);
            gouraud_packet->p[1].uv.v = (uint8)(tex[15]);
            gouraud_packet->p[2].uv.u = (uint8)(tex[14]);
            val = tex[15];
            other = tex[13];
            gouraud_packet->p[2].uv.v = (uint8)(other + val - 1);
            val = tex[14];
            other = tex[12];
            gouraud_packet->p[3].uv.u = (uint8)(other + val - 1);
            val = tex[15];
            other = tex[13];
            gouraud_packet->p[3].uv.v = (uint8)(other + val - 1);
            val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
            code = gouraud_packet->p[0].rgbc.code;
            gouraud_packet->p[1].attr = (uint16)(val);
            clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
            gouraud_packet->p[0].rgbc.code = (uint8)((code & 0xFE) | 2);
            count = (sint32)(race_control.car_count << 2);
            ++idx;
            result = (sint32)idx < count;
            gouraud_packet->p[0].attr = (uint16)(clut);
            if (result == 0)
                break;
        }
    }
    return;
}

static uint32 draw_rear_lights(uint32 slot)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    DISPLAY_ORDERING bucket, ordering;
    CAR_STATE *car = &car_states[slot];
    uint32 height;
    uint32 active = 0, color = 32, fade, flags, val;
    uint32 stage, idx, word, words[2];
    POLY_FT4 *flat_packet;
    POLY_GT4 *gouraud_packet;
    uint32 *packet_tag;
    RGBC *packet_color;
    uint32 high, low, shift;
    VEC3 translation;
    uint32 vertex, stride;
    const uint32 *geometry;
    uint32 tag, offset;
    sint32 intensity = -64, depth;

    DISPLAY_FRAME *cur_frame;
    const SVECTOR *lamp;

    FUNCTION_MARKER(0x80040060, "SLUS_006.10");
    if (slot == 0 && (input_ports[0].controls & 0x1000) == 0)
        active = car->braking != 0;
    if (slot == 1 && race_session.two_players != 0 && (input_ports[1].controls & 0x1000) == 0 && car->braking != 0)
        active = 1;
    if (active == 0)
    {
        level_desc = &scene_level_image.desc;
        level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
        if ((level_header[30] & 3) == 0)
            return 0;
    }
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    flags = level_header[30];
    if ((flags & 1) != 0)
    {
        color = 96;
        intensity = 80;
    }
    else if ((flags & 2) != 0)
    {
        color = 48;
        intensity = 32;
    }
    if (active != 0)
    {
        color += 48;
        intensity += 48;
    }
    if ((car->wheel_air_mask & 15) != 0)
    {
        val = rear_light_fade[slot];
        if (val < 128)
            rear_light_fade[slot] = (uint8)(val + 8);
    }
    else
    {
        val = rear_light_fade[slot];
        if (val != 0)
            rear_light_fade[slot] = (uint8)(val >> 1);
    }
    val = rear_light_fade[slot];
    fade = (uint32)intensity - val;
    if ((sint16)fade < 0)
        fade = 0;
    else if ((sint16)fade > 128)
        fade = 128;
    track_surface_height(&car->track_location, &car->pos, &height);
    for (stage = 0; stage < 2; ++stage)
    {
        if (stage == 1 && (fade & 0xFFFF) == 0)
            break;
        stride = stage == 0 ? 8 : 12;
        geometry = rear_light_geometry[stage];
        for (idx = 0; idx < 2; ++idx)
        {
            val = disp_state.bank;
            offset = slot * 4 + idx * 2 + val;
            if (stage == 0)
            {
                flat_packet = light_packets[0].billboards + offset;
                gouraud_packet = NULL;
                packet_tag = &flat_packet->tag;
                packet_color = &flat_packet->rgbc;
            }
            else
            {
                gouraud_packet = light_packets[0].projections + offset;
                flat_packet = NULL;
                packet_tag = &gouraud_packet->tag;
                packet_color = &gouraud_packet->p[0].rgbc;
            }
            packet_color->r = (uint8)(stage == 0 ? color : 0);
            packet_color->g = (uint8)(stage == 0 ? color : 0);
            packet_color->b = (uint8)(stage == 0 ? color : 0);
            if (stage != 0)
            {
                gouraud_packet->p[1].rgbc.r = (uint8)(0);
                gouraud_packet->p[1].rgbc.g = (uint8)(0);
                gouraud_packet->p[1].rgbc.b = (uint8)(0);
                gouraud_packet->p[2].rgbc.r = (uint8)(fade);
                gouraud_packet->p[2].rgbc.g = (uint8)(fade);
                gouraud_packet->p[2].rgbc.b = (uint8)(fade);
                gouraud_packet->p[3].rgbc.r = (uint8)(fade);
                gouraud_packet->p[3].rgbc.g = (uint8)(fade);
                gouraud_packet->p[3].rgbc.b = (uint8)(fade);
            }
            PushMatrix();
            lamp = &car_lamp_points[slot][idx];
            if (stage == 1)
                draw_hd_rear_projection(slot, lamp, (sint32)height / 256.0f, geometry, light_packets[0].billboards + offset, gouraud_packet->p);
            else
                draw_hd_lamp(lamp, (float)lamp->vy, geometry, light_packets[0].billboards + offset, NULL, 1);
            if (stage == 0)
            {
                val = ((uint32)(uint16)lamp->vx | ((uint32)(uint16)lamp->vy << 16));
                xport_gte_write_data(0, val);
                val = ((uint32)(uint16)lamp->vz | ((uint32)(uint16)lamp->pad << 16));
                xport_gte_write_data(1, val);
            }
            else
            {
                for (word = 0; word < 2; ++word)
                {
                    high = word == 0 ? ((uint32)(uint16)lamp->vx | ((uint32)(uint16)lamp->vy << 16)) : ((uint32)(uint16)lamp->vz | ((uint32)(uint16)lamp->pad << 16));
                    low = word == 0 ? ((uint32)(uint16)lamp->vx | ((uint32)(uint16)lamp->vy << 16)) : ((uint32)(uint16)lamp->vz | ((uint32)(uint16)lamp->pad << 16));
                    shift = 0;
                    words[word] = shift == 0 ? low : (high << (32 - shift)) | (low >> shift);
                }
                val = height;
                low = car->pos.bits.y;
                high = (uint16)lamp->vy;
                low = ((sint32)(val - low) >> 8);
                val = (uint16)car->suspension_local[(idx ^ 1) + 2].vy;
                val = low + val - (high - 120);
                words[0] = (words[0] & 0xFFFF) | (val << 16);
                xport_gte_write_data(0, words[0]);
                xport_gte_write_data(1, words[1]);
            }
            xport_gte_execute(0x480012);
            translation.bits.x = xport_gte_read_data(25);
            translation.bits.y = xport_gte_read_data(26);
            translation.bits.z = xport_gte_read_data(27);
            SetRotMatrix(&model_render.scr.sdk);
            for (word = 0; word < 3; ++word)
                xport_gte_write_control(5 + word, translation.u[word]);
            for (vertex = 0; vertex < 3; ++vertex)
            {
                val = geometry[vertex * 2];
                xport_gte_write_data(vertex * 2, val);
                val = geometry[vertex * 2 + 1];
                xport_gte_write_data(vertex * 2 + 1, val);
            }
            xport_gte_execute(0x280030);
            if (stage == 0)
                flat_packet->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
            else
                gouraud_packet->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
            if (stage == 0)
                flat_packet->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
            else
                gouraud_packet->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
            if (stage == 0)
                flat_packet->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
            else
                gouraud_packet->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
            flags = xport_gte_read_control(31);
            if ((flags & 0x1C66000) == 0)
            {
                val = geometry[6];
                xport_gte_write_data(0, val);
                val = geometry[7];
                xport_gte_write_data(1, val);
                xport_gte_execute(0x180001);
                if (stage == 0)
                    flat_packet->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
                else
                    gouraud_packet->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
                xport_gte_execute(0x168002E);
                depth = (sint32)(xport_gte_read_data(7) - 16);
                if (depth < 0)
                    depth = 0;
                cur_frame = disp_state.cur;
                shift = model_render.ot_shift;
                shift = (shift - model_render.coord_shift) & 31;
                tag = *packet_tag;
                ordering.tags = cur_frame->ordering;
                ordering.byte_offset = 0;
                offset = (uint32)(depth >> shift) << 2;
                val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
                *packet_tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
                ordering.tags = cur_frame->ordering;
                ordering.byte_offset = 0;
                bucket = ordering;
                bucket.byte_offset += offset;
                val = bucket.tags[(sint32)bucket.byte_offset / 4];
                bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet_tag);
            }
            PopMatrix();
        }
    }
    return 0;
}

void car_init_headlight_packets(void)
{
    const uint8 *tex;
    uint32 result;
    uint32 idx;
    POLY_FT4 *flat_packet;
    POLY_GT4 *gouraud_packet;
    uint32 val;
    uint32 other;
    uint32 code;
    uint32 clut;
    sint32 count;

    FUNCTION_MARKER(0x8004070C, "SLUS_006.10");
    tex = tex_find(headlight_tex_name);
    XASSERT(!(race_control.car_count > 14));
    {
        sint32 operation_ok = !(!gpu_register_packet_range(light_packets, (uint32)sizeof(light_packets)));
        XASSERT(operation_ok);
    }
    count = (sint32)(race_control.car_count << 2);
    idx = 0;
    if (count > 0)
    {
        for (;;)
        {
            flat_packet = light_packets[1].billboards + idx;
            flat_packet->rgbc.r = (uint8)(64);
            flat_packet->rgbc.g = (uint8)(64);
            flat_packet->rgbc.b = (uint8)(64);
            setlen(flat_packet, 9);
            flat_packet->rgbc.code = (uint8)(0x2C);
            flat_packet->p[0].uv.u = (uint8)(tex[14]);
            flat_packet->p[0].uv.v = (uint8)(tex[15]);
            val = tex[14];
            other = tex[12];
            flat_packet->p[1].uv.u = (uint8)(other + val - 1);
            flat_packet->p[1].uv.v = (uint8)(tex[15]);
            flat_packet->p[2].uv.u = (uint8)(tex[14]);
            val = tex[15];
            other = tex[13];
            flat_packet->p[2].uv.v = (uint8)(other + val - 1);
            val = tex[14];
            other = tex[12];
            flat_packet->p[3].uv.u = (uint8)(other + val - 1);
            val = tex[15];
            other = tex[13];
            flat_packet->p[3].uv.v = (uint8)(other + val - 1);
            val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
            code = flat_packet->rgbc.code;
            flat_packet->p[1].attr = (uint16)(val);
            clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
            flat_packet->rgbc.code = (uint8)((code & 0xFE) | 2);
            count = (sint32)(race_control.car_count << 2);
            ++idx;
            flat_packet->p[0].attr = (uint16)(clut);
            if ((sint32)idx >= count)
                break;
        }
    }
    count = (sint32)(race_control.car_count << 2);
    idx = 0;
    if (count > 0)
    {
        for (;;)
        {
            gouraud_packet = light_packets[1].projections + idx;
            setlen(gouraud_packet, 12);
            gouraud_packet->p[0].rgbc.r = (uint8)(0);
            gouraud_packet->p[0].rgbc.g = (uint8)(0);
            gouraud_packet->p[0].rgbc.b = (uint8)(0);
            gouraud_packet->p[1].rgbc.r = (uint8)(0);
            gouraud_packet->p[1].rgbc.g = (uint8)(0);
            gouraud_packet->p[1].rgbc.b = (uint8)(0);
            gouraud_packet->p[2].rgbc.r = (uint8)(32);
            gouraud_packet->p[2].rgbc.g = (uint8)(32);
            gouraud_packet->p[2].rgbc.b = (uint8)(32);
            gouraud_packet->p[3].rgbc.r = (uint8)(32);
            gouraud_packet->p[3].rgbc.g = (uint8)(32);
            gouraud_packet->p[3].rgbc.b = (uint8)(32);
            gouraud_packet->p[0].rgbc.code = (uint8)(0x3C);
            gouraud_packet->p[0].uv.u = (uint8)(tex[14]);
            gouraud_packet->p[0].uv.v = (uint8)(tex[15]);
            val = tex[14];
            other = tex[12];
            gouraud_packet->p[1].uv.u = (uint8)(other + val - 1);
            gouraud_packet->p[1].uv.v = (uint8)(tex[15]);
            gouraud_packet->p[2].uv.u = (uint8)(tex[14]);
            val = tex[15];
            other = tex[13];
            gouraud_packet->p[2].uv.v = (uint8)(other + val - 1);
            val = tex[14];
            other = tex[12];
            gouraud_packet->p[3].uv.u = (uint8)(other + val - 1);
            val = tex[15];
            other = tex[13];
            gouraud_packet->p[3].uv.v = (uint8)(other + val - 1);
            val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
            code = gouraud_packet->p[0].rgbc.code;
            gouraud_packet->p[1].attr = (uint16)(val);
            clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
            gouraud_packet->p[0].rgbc.code = (uint8)((code & 0xFE) | 2);
            count = (sint32)(race_control.car_count << 2);
            ++idx;
            result = (sint32)idx < count;
            gouraud_packet->p[0].attr = (uint16)(clut);
            if (result == 0)
                break;
        }
    }
    return;
}

static uint32 draw_headlights(uint32 slot)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    DISPLAY_ORDERING bucket, ordering;
    CAR_STATE *car = &car_states[slot];
    uint32 height;
    uint32 stage, idx, word, words[2];
    POLY_FT4 *flat_packet;
    POLY_GT4 *gouraud_packet;
    uint32 *packet_tag;
    RGBC *packet_color;
    uint32 high, low, shift;
    VEC3 translation;
    uint32 vertex, stride;
    const uint32 *geometry;
    uint32 val, flags, tag, offset;
    sint32 area, depth;

    DISPLAY_FRAME *cur_frame;
    const SVECTOR *lamp;

    FUNCTION_MARKER(0x8004099C, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    if ((level_header[30] & 1) == 0)
        return slot * 864;
    track_surface_height(&car->track_location, &car->pos, &height);
    for (stage = 0; stage < 2; ++stage)
    {
        stride = stage == 0 ? 8 : 12;
        geometry = headlight_geometry[stage];
        for (idx = 0; idx < 2; ++idx)
        {
            val = disp_state.bank;
            offset = slot * 4 + idx * 2 + val;
            if (stage == 0)
            {
                flat_packet = light_packets[1].billboards + offset;
                gouraud_packet = NULL;
                packet_tag = &flat_packet->tag;
                packet_color = &flat_packet->rgbc;
            }
            else
            {
                gouraud_packet = light_packets[1].projections + offset;
                flat_packet = NULL;
                packet_tag = &gouraud_packet->tag;
                packet_color = &gouraud_packet->p[0].rgbc;
            }
            PushMatrix();
            lamp = &car_lamp_points[slot][idx + 2];
            draw_hd_lamp(lamp, stage == 0 ? (float)lamp->vy : ((sint32)height / 256.0f - car->pos.y / 256.0f) + car->suspension_local[(idx ^ 1) + 2].vy - lamp->vy + 120.0f, geometry, light_packets[1].billboards + offset, stage == 0 ? NULL : gouraud_packet->p, 0);
            if (stage == 0)
            {
                val = ((uint32)(uint16)lamp->vx | ((uint32)(uint16)lamp->vy << 16));
                xport_gte_write_data(0, val);
                val = ((uint32)(uint16)lamp->vz | ((uint32)(uint16)lamp->pad << 16));
                xport_gte_write_data(1, val);
            }
            else
            {
                for (word = 0; word < 2; ++word)
                {
                    high = word == 0 ? ((uint32)(uint16)lamp->vx | ((uint32)(uint16)lamp->vy << 16)) : ((uint32)(uint16)lamp->vz | ((uint32)(uint16)lamp->pad << 16));
                    low = word == 0 ? ((uint32)(uint16)lamp->vx | ((uint32)(uint16)lamp->vy << 16)) : ((uint32)(uint16)lamp->vz | ((uint32)(uint16)lamp->pad << 16));
                    shift = 0;
                    words[word] = shift == 0 ? low : (high << (32 - shift)) | (low >> shift);
                }
                val = height;
                low = car->pos.bits.y;
                high = (uint16)lamp->vy;
                low = ((sint32)(val - low) >> 8);
                val = (uint16)car->suspension_local[(idx ^ 1) + 2].vy;
                val = low + val - (high - 120);
                words[0] = (words[0] & 0xFFFF) | (val << 16);
                xport_gte_write_data(0, words[0]);
                xport_gte_write_data(1, words[1]);
            }
            xport_gte_execute(0x480012);
            translation.bits.x = xport_gte_read_data(25);
            translation.bits.y = xport_gte_read_data(26);
            translation.bits.z = xport_gte_read_data(27);
            for (word = 0; word < 3; ++word)
                xport_gte_write_control(5 + word, translation.u[word]);
            for (vertex = 0; vertex < 3; ++vertex)
            {
                val = geometry[vertex * 2];
                xport_gte_write_data(vertex * 2, val);
                val = geometry[vertex * 2 + 1];
                xport_gte_write_data(vertex * 2 + 1, val);
            }
            xport_gte_execute(0x280030);
            if (stage == 0)
                flat_packet->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
            else
                gouraud_packet->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
            if (stage == 0)
                flat_packet->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
            else
                gouraud_packet->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
            if (stage == 0)
                flat_packet->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
            else
                gouraud_packet->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
            flags = xport_gte_read_control(31);
            if ((flags & 0x1C66000) == 0)
            {
                xport_gte_execute(0x1400006);
                area = (sint32)xport_gte_read_data(24);
                if (area >= 0)
                {
                    val = geometry[6];
                    xport_gte_write_data(0, val);
                    val = geometry[7];
                    xport_gte_write_data(1, val);
                    xport_gte_execute(0x180001);
                    if (stage == 0)
                        flat_packet->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
                    else
                        gouraud_packet->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
                    xport_gte_execute(0x168002E);
                    depth = (sint32)(xport_gte_read_data(7) - 16);
                    if (depth < 0)
                        depth = 0;
                    cur_frame = disp_state.cur;
                    shift = model_render.ot_shift;
                    shift = (shift - model_render.coord_shift) & 31;
                    tag = *packet_tag;
                    ordering.tags = cur_frame->ordering;
                    ordering.byte_offset = 0;
                    offset = (uint32)(depth >> shift) << 2;
                    val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
                    *packet_tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
                    ordering.tags = cur_frame->ordering;
                    ordering.byte_offset = 0;
                    bucket = ordering;
                    bucket.byte_offset += offset;
                    val = bucket.tags[(sint32)bucket.byte_offset / 4];
                    bucket.tags[(sint32)bucket.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet_tag);
                }
            }
            PopMatrix();
        }
    }
    return 0;
}

void car_init_speed_smoke_packets(void)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    const uint8 *tex;
    uint32 result;
    uint32 idx = 0;
    POLY_FT4 *flat_packet;
    uint32 val;
    uint32 other;
    uint32 code;
    uint32 clut;
    sint32 count;

    FUNCTION_MARKER(0x80040FB0, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    result = level_header[30] & 2;
    if (result == 0)
        return;
    tex = tex_find(speed_smoke_tex_name);
    XASSERT(!(race_control.car_count > 14));
    {
        sint32 operation_ok = !(!gpu_register_packet_range(speed_smoke_packets, (uint32)sizeof(speed_smoke_packets)));
        XASSERT(operation_ok);
    }
    count = (sint32)(race_control.car_count << 2);
    if (count <= 0)
        return;
    for (;;)
    {
        flat_packet = speed_smoke_packets + idx;
        setlen(flat_packet, 9);
        flat_packet->rgbc.code = (uint8)(0x2C);
        flat_packet->p[0].uv.u = (uint8)(tex[14]);
        flat_packet->p[0].uv.v = (uint8)(tex[15]);
        val = tex[14];
        other = tex[12];
        flat_packet->p[1].uv.u = (uint8)(other + val - 1);
        flat_packet->p[1].uv.v = (uint8)(tex[15]);
        flat_packet->p[2].uv.u = (uint8)(tex[14]);
        val = tex[15];
        other = tex[13];
        flat_packet->p[2].uv.v = (uint8)(other + val - 1);
        val = tex[14];
        other = tex[12];
        flat_packet->p[3].uv.u = (uint8)(other + val - 1);
        val = tex[15];
        other = tex[13];
        flat_packet->p[3].uv.v = (uint8)(other + val - 1);
        val = ((const TEXTURE_DESCRIPTOR *)tex)->page;
        code = flat_packet->rgbc.code;
        flat_packet->p[1].attr = (uint16)(val);
        clut = ((const TEXTURE_DESCRIPTOR *)tex)->clut;
        flat_packet->rgbc.code = (uint8)((code & 0xFE) | 2);
        count = (sint32)(race_control.car_count << 2);
        ++idx;
        result = (sint32)idx < count;
        flat_packet->p[0].attr = (uint16)(clut);
        if (result == 0)
            return;
    }
}

static uint32 draw_speed_smoke(uint32 slot)
{
    uint8 *level_header;
    const SCENE_LEVEL_DESCRIPTOR *level_desc;
    DISPLAY_ORDERING ordering;
    uint32 color, animation, idx;
    CAR_STATE *car;
    POLY_FT4 *flat_packet;
    uint32 val, other, shift, high, low, words[2], word;
    VEC3 translation;
    uint32 vertex, flags, tag, offset;
    sint32 intensity, random_val, depth;

    const EFFECT_TEXTURE *tex;
    DISPLAY_FRAME *cur_frame;
    const SVECTOR *body_point;

    FUNCTION_MARKER(0x80041114, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    level_header = (uint8 *)(scene_level_image.bytes + (uint32)(level_desc->header_offset));
    if ((level_header[30] & 2) == 0)
        return 0;
    car = &car_states[slot];
    intensity = (sint32)car->forward_speed / 4096;
    if ((sint16)intensity < 0)
        intensity = 0;
    else if ((sint16)intensity > 24)
        intensity = 24;
    color = intensity;
    if ((color << 16) == 0)
        return 0;
    random_val = randPSX();
    val = disp_state.frame_count;
    animation = (uint32)((sint32)(val + (uint32)(random_val % 3)) >> 1) & 3;
    for (idx = 0; idx < 2; ++idx)
    {
        val = disp_state.bank;
        offset = slot * 4 + idx * 2 + val;
        flat_packet = speed_smoke_packets + offset;
        flat_packet->rgbc.r = (uint8)(color);
        flat_packet->rgbc.g = (uint8)(color);
        flat_packet->rgbc.b = (uint8)(color);
        PushMatrix();
        body_point = &car_body_points[slot][idx + 2];
        if (hd_capture_active())
        {
            VECTORF center = {body_point->vx + (idx == 0 ? 72.0f : -72.0f), body_point->vy + 256.0f, body_point->vz - 64.0f};
            VECTORF corners[4];
            const EFFECT_TEXTURE *source = &effect_tex[animation];
            HD_TEXTURE texture = {source->tpage, source->clut, source->u, source->v, source->du, source->dv, HD_TEXTURED | HD_FILTERED};
            RGBC colors[4] = {flat_packet->rgbc, flat_packet->rgbc, flat_packet->rgbc, flat_packet->rgbc};
            uint32 corner;
            if ((flat_packet->rgbc.code & 1) != 0)
                texture.flags |= HD_RAW_TEXTURE;
            if ((flat_packet->rgbc.code & 2) != 0)
                texture.flags |= HD_SEMITRANSPARENT;
            for (corner = 0; corner < 4; ++corner)
            {
                SVECTOR source_vertex;
                memcpy(&source_vertex, speed_smoke_geometry + corner * 2, sizeof(source_vertex));
                corners[corner].vx = (float)source_vertex.vx;
                corners[corner].vy = (float)source_vertex.vy;
                corners[corner].vz = (float)source_vertex.vz;
            }
            hd_attached_quad(&center, corners, &texture, colors);
        }
        for (word = 0; word < 2; ++word)
        {
            high = word == 0 ? ((uint32)(uint16)(*body_point).vx | ((uint32)(uint16)(*body_point).vy << 16)) : ((uint32)(uint16)(*body_point).vz | ((uint32)(uint16)(*body_point).pad << 16));
            low = word == 0 ? ((uint32)(uint16)(*body_point).vx | ((uint32)(uint16)(*body_point).vy << 16)) : ((uint32)(uint16)(*body_point).vz | ((uint32)(uint16)(*body_point).pad << 16));
            shift = 0;
            words[word] = shift == 0 ? low : (high << (32 - shift)) | (low >> shift);
        }
        val = ((words[0] & 0xFFFF) + (idx == 0 ? 72 : (uint32)-72)) & 0xFFFF;
        words[0] = val | ((words[0] + 0x1000000) & 0xFFFF0000);
        words[1] = (words[1] & 0xFFFF0000) | ((words[1] - 64) & 0xFFFF);
        xport_gte_write_data(0, words[0]);
        xport_gte_write_data(1, words[1]);
        xport_gte_execute(0x480012);
        translation.bits.x = xport_gte_read_data(25);
        translation.bits.y = xport_gte_read_data(26);
        translation.bits.z = xport_gte_read_data(27);
        SetRotMatrix(&model_render.scr.sdk);
        for (word = 0; word < 3; ++word)
            xport_gte_write_control(5 + word, translation.u[word]);
        for (vertex = 0; vertex < 3; ++vertex)
        {
            val = speed_smoke_geometry[vertex * 2];
            xport_gte_write_data(vertex * 2, val);
            val = speed_smoke_geometry[vertex * 2 + 1];
            xport_gte_write_data(vertex * 2 + 1, val);
        }
        xport_gte_execute(0x280030);
        flat_packet->p[0].xy.xy = (uint32)(xport_gte_read_data(12));
        flat_packet->p[1].xy.xy = (uint32)(xport_gte_read_data(13));
        flat_packet->p[2].xy.xy = (uint32)(xport_gte_read_data(14));
        flags = xport_gte_read_control(31);
        if ((flags & 0x1C66000) == 0)
        {
            val = speed_smoke_geometry[6];
            xport_gte_write_data(0, val);
            val = speed_smoke_geometry[7];
            xport_gte_write_data(1, val);
            xport_gte_execute(0x180001);
            tex = &effect_tex[animation];
            val = tex->u;
            flat_packet->p[0].uv.u = (uint8)(val);
            val = tex->v;
            flat_packet->p[0].uv.v = (uint8)(val);
            val = tex->u;
            other = tex->du;
            flat_packet->p[1].uv.u = (uint8)(other + val + 255);
            val = tex->v;
            flat_packet->p[1].uv.v = (uint8)(val);
            val = tex->u;
            flat_packet->p[2].uv.u = (uint8)(val);
            val = tex->v;
            other = tex->dv;
            flat_packet->p[2].uv.v = (uint8)(other + val + 255);
            val = tex->u;
            other = tex->du;
            flat_packet->p[3].uv.u = (uint8)(other + val + 255);
            val = tex->v;
            other = tex->dv;
            flat_packet->p[3].uv.v = (uint8)(other + val + 255);
            val = tex->tpage;
            flat_packet->p[1].attr = (uint16)(val);
            val = tex->clut;
            flat_packet->p[0].attr = (uint16)(val);
            flat_packet->p[3].xy.xy = (uint32)(xport_gte_read_data(14));
            xport_gte_execute(0x168002E);
            depth = (sint32)xport_gte_read_data(7);
            cur_frame = disp_state.cur;
            shift = model_render.ot_shift;
            shift = (shift - model_render.coord_shift) & 31;
            tag = flat_packet->tag;
            ordering.tags = cur_frame->ordering;
            ordering.byte_offset = 0;
            offset = (uint32)(depth >> shift) << 2;
            val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
            flat_packet->tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
            ordering.tags = cur_frame->ordering;
            ordering.byte_offset = 0;
            val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4];
            ordering.tags[(sint32)(ordering.byte_offset + (uint32)(offset)) / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)flat_packet);
        }
        PopMatrix();
    }
    return 0;
}

static uint32 flare_vertical_offset(uint32 product)
{
    sint64 scaled = (sint64)(sint32)product * (sint32)0x80010003;
    uint32 combined = (uint32)(scaled >> 32) + product;

    return (uint32)((sint32)combined >> 14) - (uint32)((sint32)product >> 31);
}

static uint32 init_police_light_triangle(POLY_G3 *packet)
{
    FUNCTION_MARKER(0x80063440, "SLUS_006.10");
    setPolyG3(packet);
    return 0x30;
}

uint32 car_init_police_light_packets(void)
{
    const uint8 *tex;
    uint32 grp;
    uint32 idx;
    POLY_FT4 *billboard;
    POLY_G3 *ray;
    uint32 val;
    uint32 other;

    FUNCTION_MARKER(0x800417E0, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(police_light_packets, (uint32)sizeof(police_light_packets)));
        XASSERT(operation_ok);
    }
    tex = tex_find(police_light_tex_name);
    for (grp = 0; grp < 4; ++grp)
    {
        for (idx = 0; idx < 2; ++idx)
        {
            billboard = &police_light_packets[grp].billboards[idx];
            setlen(billboard, 9);
            billboard->rgbc.code = (uint8)(0x2C);
            billboard->p[0].uv.u = (uint8)(tex[14]);
            billboard->p[0].uv.v = (uint8)(tex[15]);
            val = tex[14];
            other = tex[12];
            billboard->p[1].uv.u = (uint8)(other + val - 1);
            billboard->p[1].uv.v = (uint8)(tex[15]);
            billboard->p[2].uv.u = (uint8)(tex[14]);
            val = tex[15];
            other = tex[13];
            billboard->p[2].uv.v = (uint8)(other + val - 1);
            val = tex[14];
            other = tex[12];
            billboard->p[3].uv.u = (uint8)(other + val - 1);
            val = tex[15];
            other = tex[13];
            billboard->p[3].uv.v = (uint8)(other + val - 1);
            billboard->p[1].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->page);
            billboard->p[0].attr = (uint16)(((const TEXTURE_DESCRIPTOR *)tex)->clut);
            val = billboard->rgbc.code;
            billboard->rgbc.code = (uint8)((val & 0xFE) | 2);
            ray = &police_light_packets[grp].rays[0][idx];
            init_police_light_triangle(ray);
            val = ray->p[0].rgbc.code;
            ray->p[0].rgbc.code = (uint8)((val & 0xFE) | 2);
            ray = &police_light_packets[grp].rays[1][idx];
            init_police_light_triangle(ray);
            val = ray->p[0].rgbc.code;
            ray->p[0].rgbc.code = (uint8)((val & 0xFE) | 2);
        }
    }
    police_light_phases[1] = 128;
    police_light_phases[2] = 64;
    police_light_phases[0] = 0;
    police_light_phases[3] = 192;
    return 192;
}

uint32 car_update_police_light_phases(void)
{
    uint32 count;
    uint32 idx;
    uint32 *phase_cursor;

    FUNCTION_MARKER(0x800419F4, "SLUS_006.10");
    count = police_light_config.data.count;
    if ((sint32)count <= 0)
        return count;
    idx = 0;
    phase_cursor = police_light_phases;
    do
    {
        *phase_cursor = *phase_cursor + 16;
        count = police_light_config.data.count;
        ++idx;
        phase_cursor += 1;
    } while ((sint32)idx < (sint32)count);
    return 0;
}

static void draw_hd_police_light(uint32 index)
{
    const CAR_POLICE_LIGHT_PACKETS *packets = &police_light_packets[index];
    const POLY_FT4 *billboard = &packets->billboards[disp_state.bank];
    HD_TEXTURE texture;
    RGBC colors[3];
    uint32 ray_flags[2];
    if (!hd_capture_active())
        return;
    texture = hd_quad_texture(billboard->p, &billboard->rgbc);
    colors[0] = billboard->rgbc;
    colors[1] = packets->rays[1][disp_state.bank].p[0].rgbc;
    colors[2] = packets->rays[0][disp_state.bank].p[0].rgbc;
    ray_flags[0] = (colors[1].code & 2) != 0 ? HD_SEMITRANSPARENT : 0;
    ray_flags[1] = (colors[2].code & 2) != 0 ? HD_SEMITRANSPARENT : 0;
    hd_police_light(index, &police_light_config.data.positions[index], &texture, colors, ray_flags, (float)(sint16)chase_state.signal_level);
}

static uint32 draw_police_lights(CAR_STATE *car)
{
    DISPLAY_ORDERING ordering;
    uint32 idx = 0, slot, grp, component, phase, color;
    uint32 *packet_tag;
    RGBC *packet_color;
    uint32 brightness, rgb[3], scr, depth, raw_depth, flags, x, y, val, other;
    uint32 size, negative_size, angle, c, s, product, width, height;
    uint32 dx, dy, len, packet_bank, shift, tag, link;
    sint32 correction, count;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x80041A44, "SLUS_006.10");

    SetRotMatrix(&model_render.matrix.sdk);
    val = model_render.matrix.sdk.t[0];
    other = model_render.matrix.sdk.t[1];
    xport_gte_write_control(5, val);
    product = model_render.matrix.sdk.t[2];
    xport_gte_write_control(6, other);
    xport_gte_write_control(7, product);
    count = (sint32)police_light_config.data.count;
    if (count <= 0)
        return count;
    do
    {
        slot = disp_state.bank;
        phase = (uint8)police_light_phases[idx];
        for (grp = 0; grp < 2; ++grp)
            for (component = 0; component < 6; ++component)
            {
                if (grp != 0 || component != 0)
                    slot = disp_state.bank;
                packet_color = &police_light_packets[idx].rays[grp][slot].p[1 + component / 3].rgbc;
                if (component % 3 == 0)
                    packet_color->r = 0;
                else if (component % 3 == 1)
                    packet_color->g = 0;
                else
                    packet_color->b = 0;
            }
        product = (uint32)rsin((sint32)(0 - phase * 4)) * 255;
        if ((sint32)product < 0)
            product += 4095;
        brightness = product >> 12;
        color = police_light_config.data.colors[idx];
        if (color < 4)
        {
            rgb[0] = color == 0 || color == 3 ? brightness : 0;
            rgb[1] = color == 2 ? brightness : color == 3 ? (brightness & 255) >> 1 : 0;
            rgb[2] = color == 1 ? brightness : 0;
            for (grp = 0; grp < 3; ++grp)
                for (component = 0; component < 3; ++component)
                {
                    slot = disp_state.bank;
                    packet_color = grp == 0 ? &police_light_packets[idx].billboards[slot].rgbc : &police_light_packets[idx].rays[grp - 1][slot].p[0].rgbc;
                    if (component == 0)
                        packet_color->r = (uint8)rgb[component];
                    else if (component == 1)
                        packet_color->g = (uint8)rgb[component];
                    else
                        packet_color->b = (uint8)rgb[component];
                }
        }
        draw_hd_police_light(idx);
        xport_gte_write_data(0, police_light_config.words[2 + idx * 2]);
        xport_gte_write_data(1, police_light_config.words[3 + idx * 2]);
        xport_gte_execute(0x180001);
        scr = xport_gte_read_data(14);
        depth = ((sint32)xport_gte_read_data(19) >> 2);
        raw_depth = xport_gte_read_data(19);
        flags = xport_gte_read_control(31);
        if ((flags & 0x1C66000) == 0 && (sint32)model_render.near_depth < (sint32)depth && (sint32)depth < (sint32)model_render.far_depth)
        {
            correction = (sint32)((raw_depth - 400) << 12) / 5120;
            val = raw_depth - (correction < 0 ? 0 : (uint32)correction);
            size = (uint32)fixed_divide_checked(32768, (sint32)val);
            size *= (uint32)(sint16)chase_state.signal_level;
            size = ((sint32)size / 4096);
            x = scr & 65535;
            y = scr >> 16;
            width = model_render.width;
            height = model_render.height;
            dx = (uint32)(sint32)(sint16)x - (uint32)((sint32)width / 2);
            dy = (uint32)(sint32)(sint16)y - (uint32)((sint32)height / 2);
            angle = (uint32)fixed_angle_from_vector((sint32)dx, (sint32)dy) & 4095;
            negative_size = 0 - size;
            for (component = 0; component < 4; ++component)
            {
                c = (uint32)rcos((sint32)angle);
                s = (uint32)rsin((sint32)angle);
                if (component == 0)
                    product = negative_size * c + size * (0 - s);
                else if (component == 1)
                    product = (c - s) * size;
                else if (component == 2)
                    product = (c - s) * negative_size;
                else
                    product = size * c + negative_size * (0 - s);
                slot = disp_state.bank;
                police_light_packets[idx].billboards[slot].p[component].xy.x = (sint16)(uint16)(x + (uint32)((sint32)product / 16384));
                s = (uint32)rsin((sint32)angle);
                c = (uint32)rcos((sint32)angle);
                if (component == 0)
                    product = negative_size * s + size * c;
                else if (component == 1)
                    product = (c + s) * size;
                else if (component == 2)
                    product = (c + s) * negative_size;
                else
                    product = size * s + negative_size * c;
                slot = disp_state.bank;
                if (component == 3)
                    width = model_render.width;
                police_light_packets[idx].billboards[slot].p[component].xy.y = (sint16)(uint16)(y + flare_vertical_offset(product));
            }
            dx = (uint32)((sint32)width / 2) - (uint32)(sint32)(sint16)x;
            height = model_render.height;
            dy = (uint32)((sint32)height / 2) - (uint32)(sint32)(sint16)y;
            len = (uint32)SquareRoot0((sint32)(dx * dx + dy * dy));
            if (len == 0)
                len = 1;
            slot = disp_state.bank;
            packet_bank = slot;
            police_light_packets[idx].rays[0][packet_bank].p[0].xy.x = (sint16)(uint16)x;
            police_light_packets[idx].rays[0][packet_bank].p[0].xy.y = (sint16)(uint16)y;
            for (component = 0; component < 4; ++component)
            {
                angle = component < 2 ? size : negative_size;
                if ((component & 1) == 0)
                {
                    c = (uint32)rcos((sint32)angle);
                    s = (uint32)rsin((sint32)angle);
                    product = dx * c + dy * (0 - s);
                }
                else
                {
                    s = (uint32)rsin((sint32)angle);
                    c = (uint32)rcos((sint32)angle);
                    product = dx * s + dy * c;
                }
                slot = disp_state.bank;
                if (component == 3)
                {
                    packet_bank = slot;
                }
                val = size * ((uint32)((sint32)product / 4096) * 4);
                val = (uint32)fixed_divide_checked((sint32)val, (sint32)len);
                if ((component & 1) == 0)
                    police_light_packets[idx].rays[0][slot].p[1 + component / 2].xy.x = (sint16)(uint16)(x + val);
                else
                    police_light_packets[idx].rays[0][slot].p[1 + component / 2].xy.y = (sint16)(uint16)(y + val);
            }
            police_light_packets[idx].rays[1][packet_bank].p[0].xy.x = (sint16)(uint16)x;
            police_light_packets[idx].rays[1][packet_bank].p[0].xy.y = (sint16)(uint16)y;
            val = (uint16)police_light_packets[idx].rays[0][packet_bank].p[1].xy.x;
            police_light_packets[idx].rays[1][packet_bank].p[1].xy.x = (sint16)(uint16)(x + x - val);
            val = (uint16)police_light_packets[idx].rays[0][packet_bank].p[1].xy.y;
            police_light_packets[idx].rays[1][packet_bank].p[1].xy.y = (sint16)(uint16)(y + y - val);
            val = (uint16)police_light_packets[idx].rays[0][packet_bank].p[2].xy.x;
            police_light_packets[idx].rays[1][packet_bank].p[2].xy.x = (sint16)(uint16)(x + x - val);
            val = (uint16)police_light_packets[idx].rays[0][packet_bank].p[2].xy.y;
            cur_frame = disp_state.cur;
            shift = model_render.ot_shift;
            police_light_packets[idx].rays[1][packet_bank].p[2].xy.y = (sint16)(uint16)(y + y - val);
            packet_tag = &police_light_packets[idx].rays[0][packet_bank].tag;
            tag = *packet_tag;
            ordering.tags = cur_frame->ordering;
            ordering.byte_offset = 0;
            link = (uint32)((sint32)depth >> ((shift - 1) & 31)) * 4;
            val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(link)) / 4];
            *packet_tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
            ordering.tags = cur_frame->ordering;
            ordering.byte_offset = (uint32)(link);
            val = ordering.tags[(sint32)ordering.byte_offset / 4];
            ordering.tags[(sint32)ordering.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet_tag);
            for (grp = 1; grp < 3; ++grp)
            {
                ordering.tags = cur_frame->ordering;
                ordering.byte_offset = 0;
                packet_tag = grp == 1 ? &police_light_packets[idx].rays[1][packet_bank].tag : &police_light_packets[idx].billboards[slot].tag;
                tag = *packet_tag;
                val = ordering.tags[(sint32)(ordering.byte_offset + (uint32)(link)) / 4];
                *packet_tag = (tag & 0xFF000000) | (val & 0xFFFFFF);
                ordering.tags = cur_frame->ordering;
                ordering.byte_offset = (uint32)(link);
                val = ordering.tags[(sint32)ordering.byte_offset / 4];
                ordering.tags[(sint32)ordering.byte_offset / 4] = (val & 0xFF000000) | gpu_packet_link((uintptr_t)packet_tag);
            }
        }
        count = (sint32)police_light_config.data.count;
        ++idx;
    } while ((sint32)idx < count);
    return 0;
}

void car_select_police_lights(sint32 selection)
{
    const uint32 *src;
    uint32 *dst;
    uint32 words[4];
    uint32 grp;
    uint32 word;

    FUNCTION_MARKER(0x800B789C, "SLUS_006.10");
    if (race_session.multiplayer_mode == 3 || race_session.cop_chase != 0 || race_session.event_mode == 4)
    {
        switch (selection)
        {
            case 25:
                src = police_light_srcs[1];
                break;
            case 33:
                src = police_light_srcs[2];
                break;
            case 34:
                src = police_light_srcs[3];
                break;
            case 35:
                src = police_light_srcs[4];
                break;
            default:
                return;
        }
    }
    else
        src = police_light_srcs[0];
    dst = police_light_config.words;
    for (grp = 0; grp < 2; ++grp)
    {
        for (word = 0; word < 4; ++word)
            words[word] = src[word];
        for (word = 0; word < 4; ++word)
            dst[word] = words[word];
        src += 4;
        dst += 4;
    }
    words[0] = src[0];
    words[1] = src[1];
    dst[0] = words[0];
    dst[1] = words[1];
}
