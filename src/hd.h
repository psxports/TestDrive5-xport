#ifndef TD5_HD_H
#define TD5_HD_H

#include "psx.h"
#include "vid.h"
#include "fixed_math.h"
#include "model_resource.h"

typedef struct HD_TRACK_IMAGE HD_TRACK_IMAGE;
typedef struct HD_MODEL HD_PREPARED_MODEL;
void hd_prepare_race_resources(void);
void hd_load_race_car(uint32 slot, const uint8 *bytes, uint32 size, sint32 primary);
void hd_prepare_sky_model(const MODEL_RESOURCE *resource, const sint8 *visibility);
HD_PREPARED_MODEL *hd_prepare_track_model(const MODEL_RESOURCE *resource, const uint16 *pixels);
void hd_prepare_track_texture(HD_PREPARED_MODEL *model);
void hd_weld_course(HD_PREPARED_MODEL *const *models, const uint32 *sections, uint32 count, uint32 section_count);
HD_PREPARED_MODEL *hd_prepare_model(const MODEL_RESOURCE *resource, HD_TRACK_IMAGE *image, uint32 variant);
void hd_draw_prepared_static(HD_PREPARED_MODEL *model, sint32 cull);
void hd_track_image_seal(HD_TRACK_IMAGE *image);
sint32 hd_native_capture_required(void);
void hd_original_track_capture(sint32 enabled);
void hd_prepare_draw_capacity(uint32 count);
uint32 hd_prepared_draw_slots(const HD_PREPARED_MODEL *model);
void hd_prepare_track_geometry(HD_PREPARED_MODEL *const *models, uint32 count, uint32 main_count);
HD_TRACK_IMAGE *hd_track_image_create(const uint16 *source);
void hd_track_image_release(HD_TRACK_IMAGE *image);
void hd_track_image_bind(HD_TRACK_IMAGE *image);

typedef struct HD_TEXTURE
{
    uint16 page;
    uint16 clut;
    uint16 u;
    uint16 v;
    uint16 width;
    uint16 height;
    uint16 flags;
} HD_TEXTURE;

void hd_race_wheel_texture(uint32 slot, uint32 frame, HD_TEXTURE *texture);

enum
{
    HD_TEXTURED = 1,
    HD_RAW_TEXTURE = 2,
    HD_SEMITRANSPARENT = 4,
    HD_FILTERED = 8,
    HD_ZERO_OPAQUE = 16,
    HD_REPEAT_U = 32,
    HD_REPEAT_V = 64,
    HD_FILTER_REGION = 128,
    HD_SHADOW = 256,
    HD_SCENE_TEXTURE = 512,
    HD_LAMP_GLOW = 2048,
    HD_DISTANCE_LIT = 4096,
    HD_LENS_FLARE = 8192,
    HD_HALF_DISTANCE_LIGHT = 16384,
    HD_PRIVATE_CAR_HUB = 32768
};

typedef struct HD_UI_VERTEX
{
    float x, y;
    float u, v;
    uint8 color[4];
} HD_UI_VERTEX;

typedef struct HD_POSE
{
    VEC3F position;
    QUATF rotation;
} HD_POSE;

extern sint32 game_hd_frame;
sint32 hd_capture_active(void);

sint32 hd_init(void);
void hd_free(void);
void hd_reset(void);
void hd_begin_frame(uint32 views);
void hd_begin_race_frame(uint32 views);
void hd_capture_race_poses(void);
void hd_distance_lighting(uint32 radius, const uint8 color[3], uint32 height);
void hd_begin_view(uint32 view, VID_RECT reference);
void hd_end_frame(void);
void hd_present_frame(uint64 processed_field, uint32 fields);
void hd_reseed_race(void);
void hd_suspend(void);
void hd_capture_pause(sint32 requested);
void hd_native_begin(uint32 view, const DRAWENV *environment);
void hd_native_end(uint32 view);
void hd_pause_begin(void);
void hd_pause_end(void);
void hd_ui_quad(const HD_UI_VERTEX points[4], const HD_TEXTURE *texture, float alpha);
void hd_ui_ordered_quad(const HD_UI_VERTEX points[4], const HD_TEXTURE *texture, float alpha, uint32 layer);
void hd_ui_triangle(const HD_UI_VERTEX points[3], const HD_TEXTURE *texture, float alpha);
void hd_ui_gouraud(const POINT_CT points[4], uint32 layer);
void hd_ui_flat(const POINT_T points[4], const RGBC *color, uint32 layer);
void hd_ui_solid(const XY *points, const RGBC *colors, uint32 count, uint32 layer);
void hd_ui_color_quad(const XY points[4], const RGBC *color, uint32 layer);
void hd_ui_color_triangle(const XY points[3], const RGBC *color, uint32 layer);
void hd_ui_line(const XY points[2], const RGBC *color, uint32 layer);
void hd_ui_segment(const HD_UI_VERTEX points[2], uint16 page, sint32 semi, uint32 layer);
float hd_project_world(const VEC3 *position, HD_UI_VERTEX *screen);
void hd_world_segment(const HD_UI_VERTEX points[2], float depth, uint16 page, sint32 semi);
void hd_ui_region(VID_RECT clip, float offset_x, float offset_y);
void hd_ui_region_end(void);
void hd_ui_anchor(sint32 side);
uint32 hd_ui_mark(void);
void hd_ui_motion(uint32 first, float previous_x, float previous_y, float current_x, float current_y);
void hd_ui_follow_car(uint32 first, uint32 slot, const SVECTOR *local);
struct CAR_STATE;
void hd_gauge_needle(const struct CAR_STATE *car, uint32 pass, float center_x, float center_y, float radius, float thickness, float vertical_scale, const RGBC *color, uint32 layer);
void hd_minimap_begin(const struct CAR_STATE *car, float scale_x, float scale_z);
void hd_minimap_quad(const VECTORF points[4], const RGBC *color);
void hd_minimap_line(const VECTORF *start, const VECTORF *end, const RGBC *color);
void hd_minimap_car(uint32 slot, const RGBC *color);
void hd_police_light(uint32 index, const SVECTOR *anchor, const HD_TEXTURE *texture, const RGBC colors[3], const uint32 ray_flags[2], float signal);
void hd_police_retire(void);
void hd_ui_gradient(const POINT_C points[4], uint32 layer);
void hd_menu_view(void);
void hd_race_view(uint32 view, const DRAWENV *draw);
void hd_world_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4], sint32 view_space);
void hd_road_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4]);
void hd_world_billboard(const VEC3 *position, float size, const HD_TEXTURE *texture, const RGBC *color);
void hd_attached_quad(const VECTORF *center, const VECTORF corners[4], const HD_TEXTURE *texture, const RGBC colors[4]);
HD_TEXTURE hd_quad_texture(const POINT_T points[4], const RGBC *color);
void hd_local_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4]);
MATRIXF hd_model_transform(void);
void hd_solid_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4], sint32 view_space);
void hd_lit_wheel_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4], const SVECTOR *normal);
void hd_menu_background(uint32 bank, uint32 height);
sint32 hd_loading_screen(void);
void hd_transform_model(const MATRIX *rotation, const VEC3 *position, sint32 camera_relative);
void hd_draw_model(const MODEL_RESOURCE *model);
void hd_draw_sky(const MODEL_RESOURCE *model, const sint8 *visibility);
void hd_draw_static_candidate(const MODEL_RESOURCE *model, const sint8 *sky_visibility);

typedef enum HD_CAR_FAMILY
{
    HD_CAR_RACING,
    HD_CAR_PRIMARY,
    HD_CAR_TRAFFIC,
    HD_CAR_SPLIT
} HD_CAR_FAMILY;

void hd_draw_car_candidate(uint32 slot, const MODEL_RESOURCE *model, HD_CAR_FAMILY family);
void hd_shadow_quad(uint32 slot, uint32 part, uint32 kind, const VECTORF points[4], const HD_TEXTURE *texture, const RGBC *color, const POINT_T uv[4]);
void hd_forget_span(const void *bytes, uint32 size);
void hd_set_back_color(sint32 r, sint32 g, sint32 b);
void hd_set_lighting(const MATRIX *rotation);
void hd_flare_textures(const uint8 *const textures[2]);
struct CAR_ENVIRONMENT_MAPPING;
void hd_environment_reflection(const MODEL_RESOURCE *model, const struct CAR_ENVIRONMENT_MAPPING *mapping, sint32 center_on_model);
void hd_environment_surface(const MODEL_RESOURCE *model, float angle, float origin_u, float origin_v, uint32 period_u, uint32 period_v);
void hd_camera_position(const VECTOR *position);
void hd_camera_angles(const SVECTOR *angles);
void hd_camera_aim(const VECTOR *target, sint32 roll);
void hd_pose_from_native(HD_POSE *pose, const MATRIX *rotation, const VEC3 *position);
void hd_pose_interpolate(const HD_POSE *previous, const HD_POSE *current, float phase, HD_POSE *result);
sint32 hd_car_begin(uint32 slot);
void hd_car_end(sint32 previous);
void hd_car_reset(uint32 slot);
void hd_camera_reset(uint32 view);
void hd_wheel_begin(uint32 wheel, const SVECTOR *center, float steering, float spin, float period);
void hd_wheel_hub(void);
void hd_wheel_end(void);
void hd_effect_reset(uint32 slot);
void hd_particle_billboard(uint32 slot, const VEC3 *position, float size, const HD_TEXTURE *texture, const RGBC *color);
void hd_screen_particle(uint32 slot, const VEC3 *position, float width, float height, const HD_TEXTURE *texture, const RGBC *color, uint32 layer);
void hd_effect_line(uint32 slot, const VEC3 points[2], const RGBC colors[2], uint16 page, sint32 semi);
void hd_weather_reset(uint32 slot);
void hd_weather_snow(uint32 slot, const HD_UI_VERTEX points[4], const HD_TEXTURE *texture);
void hd_weather_rain(uint32 slot, const HD_UI_VERTEX *head, const RGBC *tail_color);
void hd_weather_retire(uint32 slot, uint32 mode);
void hd_skid_reset(uint32 slot);
void hd_skids_begin(void);
void hd_skid_quad(uint32 slot, const VECTORF points[4], const HD_TEXTURE *texture, const RGBC *color, sint32 retiring);
void hd_effect_retire(uint32 slot, const VEC3 *position, const VEC3 *tail);
void hd_effects_begin(void);

#endif
