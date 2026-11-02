#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "hd.h"
#include "hd_track.h"
#include "hd_texture_cache.h"
#include "psx_gpu.h"
#include "vid_psx_sampling.h"
#include "game.h"

#if defined(_MSC_VER)
    #pragma function(fmod)
#endif

enum
{
    HD_VIEWS = 2,
    HD_UI_MESH_VERTICES = 1536,
    HD_BLEND_PASSES = 5
};

// measured upper sky and horizon source colors, indexed by LEVEL number minus one
static const uint8 track_fog_colors[40][3] = {
    {51, 48, 42},    // LEVEL01
    {21, 16, 12},    // LEVEL02
    {116, 71, 64},   // LEVEL03
    {41, 37, 28},    // LEVEL04
    {41, 37, 28},    // LEVEL05
    {69, 67, 65},    // LEVEL06
    {127, 124, 123}, // LEVEL07
    {80, 118, 148},  // LEVEL08
    {87, 128, 158},  // LEVEL09
    {110, 121, 112}, // LEVEL10
    {96, 100, 92},   // LEVEL11
    {136, 147, 136}, // LEVEL12
    {81, 125, 179},  // LEVEL13
    {153, 166, 170}, // LEVEL14
    {90, 104, 116},  // LEVEL15
    {131, 150, 130}, // LEVEL16
    {97, 123, 123},  // LEVEL17
    {87, 74, 20},    // LEVEL18
    {71, 35, 6},     // LEVEL19
    {142, 126, 98},  // LEVEL20
    {119, 53, 13},   // LEVEL21
    {112, 92, 64},   // LEVEL22
    {54, 55, 57},    // LEVEL23
    {15, 12, 12},    // LEVEL24
    {114, 117, 118}, // LEVEL25
    {155, 149, 126}, // LEVEL26
    {111, 132, 151}, // LEVEL27
    {89, 130, 146},  // LEVEL28
    {138, 145, 140}, // LEVEL29
    {29, 26, 17},    // LEVEL30
    {29, 26, 17},    // LEVEL31
    {114, 117, 118}, // LEVEL32
    {155, 149, 126}, // LEVEL33
    {111, 132, 151}, // LEVEL34
    {89, 130, 146},  // LEVEL35
    {138, 145, 140}, // LEVEL36
    {8, 8, 8},       // LEVEL37
    {8, 8, 8},       // LEVEL38
    {103, 129, 127}, // LEVEL39
    {103, 129, 127}, // LEVEL40
};

typedef struct HD_PARTICLE_SAMPLE
{
    VECTORF position;
    float size;
    float brightness;
} HD_PARTICLE_SAMPLE;

typedef struct HD_UI_MESH
{
    struct HD_UI_MESH *next;
    VID_GEOMETRY *geometry;
    VID_INDEX *indices;
    uint32 used;
    uint32 indexed;
} HD_UI_MESH;

typedef struct HD_PROJECTION
{
    float x, y;
    float center_x, center_y;
} HD_PROJECTION;

typedef struct HD_PROJECTION_MOTION
{
    HD_PROJECTION previous, current;
    uint64 serial, key;
    uint32 generation;
} HD_PROJECTION_MOTION;

typedef struct HD_ENV_SAMPLE
{
    float angle, u, v;
    float tint[3];
    float center[2];
} HD_ENV_SAMPLE;

typedef struct HD_ENV_MOTION
{
    HD_ENV_SAMPLE previous, current;
    const uint8 *source, *texture;
    uint64 serial;
    uint64 camera_key;
    uint32 generation, kind;
} HD_ENV_MOTION;

typedef struct HD_LIGHT_SAMPLE
{
    MATRIXF world;
    MATRIXF color;
    float ambient[3];
} HD_LIGHT_SAMPLE;

typedef struct HD_LIGHT_MOTION
{
    HD_LIGHT_SAMPLE previous, current;
    uint64 serial, camera_key;
    uint32 generation;
} HD_LIGHT_MOTION;

typedef struct HD_POLICE_SAMPLE
{
    float signal;
    float colors[3][3];
} HD_POLICE_SAMPLE;

typedef struct HD_POLICE_INPUT
{
    HD_POLICE_SAMPLE samples[2];
    VECTORF anchor;
    HD_TEXTURE texture;
    uint32 ray_flags[2];
    float depth_scale, near_depth, far_depth, half_width, half_height;
} HD_POLICE_INPUT;

typedef struct HD_POLICE_MOTION
{
    HD_POLICE_SAMPLE previous, current;
    VECTORF anchor;
    HD_TEXTURE texture;
    uint32 ray_flags[2];
    uint64 serial, camera_key;
    uint32 generation;
} HD_POLICE_MOTION;

typedef struct HD_UI_ATTACHMENT
{
    HD_POSE camera[2];
    HD_POSE object[2];
    VECTORF local;
    HD_PROJECTION projection[2];
    float source[2];
    sint32 active;
} HD_UI_ATTACHMENT;

typedef struct HD_SUN_ATTACHMENT
{
    HD_POSE camera[2], object[2];
    QUATF facing[2];
    HD_PROJECTION projection[2];
    VECTORF points[4];
    float half_width, half_height, depth_scale, zsf4, far_otz, factor;
    sint32 relative, active;
} HD_SUN_ATTACHMENT;

typedef struct HD_NEEDLE
{
    float angles[2];
    float center_x, center_y, vertical_scale;
    sint32 active;
} HD_NEEDLE;

typedef struct HD_NEEDLE_MOTION
{
    float previous, current, vertical_scale;
    sint32 rpm_limit;
    uint64 serial;
    uint32 generation;
} HD_NEEDLE_MOTION;

typedef struct HD_MAP_SAMPLE
{
    VECTORF origin, cars[14];
    float angle;
} HD_MAP_SAMPLE;

typedef struct HD_MAP_MOTION
{
    HD_MAP_SAMPLE previous, current;
    uint64 serial;
    uint32 generation, owner;
    uint32 reset_slots;
} HD_MAP_MOTION;

typedef struct HD_MAP_DRAW
{
    VECTORF origin[2], marker[2];
    float angles[2], scale[2], offset[2];
    VID_RECT clip;
    sint32 mode, active;
} HD_MAP_DRAW;

typedef struct HD_OBJECT_TEXTURE HD_OBJECT_TEXTURE;
typedef struct HD_SPRITE_TEXTURE HD_SPRITE_TEXTURE;

typedef struct HD_UI_DRAW
{
    HD_UI_MESH *mesh;
    HD_SPRITE_TEXTURE *sprite;
    uint32 first;
    uint32 count;
    uint32 view;
    uint32 blend;
    uint32 layer;
    uint32 sequence;
    HD_PARTICLE_SAMPLE particle[2];
    sint32 particle_active;
    HD_UI_VERTEX segment[2][2];
    sint32 segment_active;
    sint32 fading_out;
    HD_UI_ATTACHMENT attachment;
    HD_SUN_ATTACHMENT sun;
    HD_NEEDLE needle;
    HD_MAP_DRAW map;
} HD_UI_DRAW;

typedef struct HD_VIEW
{
    VID_RECT viewport;
    VID_RECT reference;
    float reference_aspect;
    HD_POSE headlight_car[2];
    VECTORF headlight_origin;
    sint32 headlight_active;
} HD_VIEW;

typedef struct HD_SOURCE_GROUP
{
    sint16 count;
    uint8 kind;
    uint8 active;
} HD_SOURCE_GROUP;

typedef struct HD_FLAT_FACE
{
    sint16 indices[4];
    RGBC color;
    SVECTOR normal;
} HD_FLAT_FACE;

typedef struct HD_TRIANGLE_FACE
{
    sint16 indices[4];
    RGBC colors[3];
    SVECTOR normals[3];
} HD_TRIANGLE_FACE;

typedef struct HD_QUAD_FACE
{
    sint16 indices[4];
    RGBC colors[4];
    SVECTOR normals[4];
} HD_QUAD_FACE;

typedef char HD_SOURCE_LAYOUT_CHECK[sizeof(HD_SOURCE_GROUP) == 4 && sizeof(HD_FLAT_FACE) == 20 && sizeof(HD_TRIANGLE_FACE) == 44 && sizeof(HD_QUAD_FACE) == 56 ? 1 : -1];

typedef struct HD_MODEL_RANGE
{
    uint32 first;
    uint32 count;
    uint32 blend;
    HD_TEXTURE texture;
    HD_SOURCE_TEXTURE *source_texture;
    HD_SOURCE_REGION source_region;
    uint32 source_first;
} HD_MODEL_RANGE;

typedef struct HD_MOTION
{
    HD_POSE previous;
    HD_POSE current;
    uint64 serial;
    uint64 key;
    uint32 generation;
} HD_MOTION;

typedef struct HD_WHEEL_SAMPLE
{
    VECTORF center;
    float steering;
    float spin;
} HD_WHEEL_SAMPLE;

typedef struct HD_WHEEL_MOTION
{
    HD_WHEEL_SAMPLE previous;
    HD_WHEEL_SAMPLE current;
    uint64 serial;
    uint32 generation;
} HD_WHEEL_MOTION;

typedef struct HD_PARTICLE_MOTION
{
    HD_PARTICLE_SAMPLE previous;
    HD_PARTICLE_SAMPLE current;
    uint64 serial;
    uint32 generation;
    HD_TEXTURE texture;
    RGBC color;
    float height_ratio;
    uint32 layer;
    sint32 screen;
} HD_PARTICLE_MOTION;

typedef struct HD_RAIN_MOTION
{
    HD_UI_VERTEX previous[2];
    HD_UI_VERTEX current[2];
    uint64 serial;
    uint32 generation;
} HD_RAIN_MOTION;

typedef struct HD_SNOW_MOTION
{
    HD_PARTICLE_MOTION motion;
    HD_UI_VERTEX quad[4];
    HD_TEXTURE texture;
} HD_SNOW_MOTION;

typedef struct HD_QUAD_SAMPLE
{
    VECTORF points[4];
    float brightness;
} HD_QUAD_SAMPLE;

typedef struct HD_DRAW_POSES
{
    HD_POSE camera[2];
    HD_POSE object[2];
    QUATF facing[2];
    MATRIXF source_transform;
    sint32 relative;
    sint32 view_geometry;
    sint32 billboard;
    sint32 centered_billboard;
    sint32 world_translation;
    VECTORF center;
    HD_WHEEL_SAMPLE wheel[2];
    sint32 wheel_active;
    sint32 wheel_hub;
    HD_PARTICLE_SAMPLE particle[2];
    sint32 particle_active;
    VECTORF line[2][2];
    RGBC line_colors[2];
    sint32 line_active;
} HD_DRAW_POSES;

static void object_texture_free(HD_OBJECT_TEXTURE *asset);

typedef struct HD_STATIC_POOL HD_STATIC_POOL;
typedef struct HD_STATIC_GROUP HD_STATIC_GROUP;

typedef struct HD_CACHED_GROUP
{
    VID_TEXTURE *texture;
    HD_STATIC_GROUP *static_group;
    uint32 first, count, transparent;
} HD_CACHED_GROUP;

typedef struct HD_MODEL
{
    struct HD_MODEL *next, *hash_next;
    const uint8 *source;
    uint32 source_size;
    uint32 kind;
    const sint8 *visibility;
    uint32 variant;
    VID_GEOMETRY *geometry;
    HD_OBJECT_TEXTURE *texture_asset;
    VID_INDEX *indices;
    HD_MODEL_RANGE *ranges, *sky_ranges;
    VID_INDEX *sky_indices;
    uint32 range_count, sky_range_count;
    uint32 index_count, has_transparent, has_lighting;
    uint32 batch_vertex_count;
    uint32 *batch_source_indices;
    VID_VERTEX3D *batch_vertices;
    HD_CACHED_GROUP *source_groups;
    uint32 source_group_count, uses_sources, source_ready;
    HD_STATIC_POOL *static_pool;
    HD_MOTION motion[HD_VIEWS];
    HD_MOTION facing[HD_VIEWS];
    HD_LIGHT_MOTION lighting[HD_VIEWS];
    uint32 draw_refs;
} HD_MODEL;

struct HD_TRACK_IMAGE
{
    VID_TEXTURE *texture;
    VID_MATERIAL *materials[HD_BLEND_PASSES];
    uint32 refs;
};

static HD_TRACK_IMAGE *track_image;
static sint32 loading_image_active;

typedef struct HD_MODEL_DRAW
{
    const HD_CACHED_GROUP *source_group;
    uint32 source_order;
    HD_MODEL *model;
    HD_TRACK_IMAGE *image;
    uint8 region_sampling;
    uint8 texture_transparent;
    uint8 fog_active;
    uint8 fog_color[3];
    float fog_horizon;
    MATRIXF transform;
    MATRIXF light;
    MATRIX color;
    float ambient[3];
    uint32 view;
    float focal;
    float scale_x;
    float scale_y;
    float center_x;
    float center_y;
    float far_plane;
    HD_PROJECTION projection[2];
    uint32 first;
    uint32 count;
    uint32 blend;
    sint32 dynamic;
    float environment[7][4];
    HD_ENV_SAMPLE environment_samples[2];
    float reflection_scale[2];
    sint32 reflection_centered;
    HD_DRAW_POSES poses;
    HD_QUAD_SAMPLE quad[2];
    HD_TEXTURE quad_texture;
    float quad_uv[4][2];
    sint32 quad_active;
    float depth_bias;
    float depth_offset;
    sint32 fading_out;
    VECTORF hub_normal;
    VECTORF hub_anchor;
    float hub_cue[4];
    float hub_far[3];
    sint32 hub_lit;
    HD_LIGHT_SAMPLE lighting[2];
    sint32 light_history;
    HD_POLICE_INPUT police;
} HD_MODEL_DRAW;

typedef struct HD_SCENE_BATCH_GROUP
{
    VID_MATERIAL *material;
    VID_TEXTURE *texture;
    const HD_MODEL_DRAW *reference;
    uint32 key, flags, count, capacity, first, reference_input, world_space;
    uint32 *indices;
    VEC3F origin;
    VID_GEOMETRY *geometry;
    VID_INDEX *index_buffer;
} HD_SCENE_BATCH_GROUP;

struct HD_STATIC_GROUP
{
    HD_SCENE_BATCH_GROUP batch;
    uint32 category, lighting, transparent, cursor;
    uint64 submitted[HD_VIEWS];
};

struct HD_STATIC_POOL
{
    HD_STATIC_POOL *next;
    VID_GEOMETRY *geometry;
    VID_INDEX *indices;
    HD_STATIC_GROUP *groups;
    uint32 group_count;
    VEC3F origin;
};

static HD_STATIC_POOL *static_pools;
static uint64 static_dispatch_serial;
static void static_pools_free(void);

typedef struct HD_BATCH_INPUT
{
    HD_MODEL *model;
    VID_TEXTURE *texture;
    const HD_CACHED_GROUP *source_group;
    VEC3F position;
    MATRIXF light;
    MATRIX color;
    float ambient[3];
    HD_LIGHT_SAMPLE lighting[2];
    sint32 light_history;
    uint32 view;
    uint32 draw_index;
} HD_BATCH_INPUT;

typedef struct HD_SCENE_BATCH_BANK
{
    struct HD_SCENE_BATCH_BANK *next;
    VID_GEOMETRY *geometry;
    VID_INDEX *indices;
    uint32 vertex_capacity, index_capacity, group_capacity, group_count;
    HD_SCENE_BATCH_GROUP *groups;
    HD_BATCH_INPUT *inputs;
    uint32 input_count, input_capacity;
    VEC3F origin;
} HD_SCENE_BATCH_BANK;

static HD_SCENE_BATCH_BANK *scene_batch_banks, **scene_batch_cursor;
static VID_SHADER *scene_batch_shader, *object_texture_shader, *sprite_texture_shader;
static void scene_batches_free(void);
static void source_materials_free(void);

typedef struct HD_WORLD_MESH
{
    struct HD_WORLD_MESH *next;
    HD_MODEL model;
    uint32 used;
} HD_WORLD_MESH;

typedef struct HD_LINE_STORAGE
{
    HD_MODEL model;
    VECTORF previous[2];
    VECTORF current[2];
    uint64 serial;
    uint32 generation;
    RGBC colors[2];
    uint32 blend;
} HD_LINE_STORAGE;

typedef struct HD_EFFECT_RETIREMENT
{
    HD_PARTICLE_SAMPLE particle[2];
    HD_TEXTURE texture;
    RGBC color;
    float height_ratio;
    uint32 layer;
    VECTORF line[2][2];
    RGBC line_colors[2];
    uint32 blend;
    HD_MODEL line_model;
    uint32 generation;
    sint32 kind;
} HD_EFFECT_RETIREMENT;

typedef struct HD_SKID_STORAGE
{
    HD_MODEL model[2];
    HD_QUAD_SAMPLE previous;
    HD_QUAD_SAMPLE current;
    HD_QUAD_SAMPLE retired;
    HD_TEXTURE texture;
    HD_TEXTURE retired_texture;
    uint64 serial;
    uint32 generation;
    sint32 retire_pending;
    uint32 retire_generation;
    sint32 current_retired;
} HD_SKID_STORAGE;

typedef struct HD_SHADOW_STORAGE
{
    HD_MODEL model;
    HD_QUAD_SAMPLE previous, current;
    HD_TEXTURE texture;
    uint64 serial;
    uint32 generation, kind;
} HD_SHADOW_STORAGE;

typedef struct HD_SCENE_BUFFERS
{
    HD_UI_MESH *ui_meshes;
    HD_UI_DRAW *ui_draws;
    uint32 ui_count, ui_capacity;
    HD_WORLD_MESH *world_meshes;
    HD_MODEL_DRAW *model_draws;
    uint32 model_count, model_capacity;
    HD_VIEW viewports[HD_VIEWS + 1];
    uint32 views;
    sint32 width, height, prepared;
    float clear[3];
} HD_SCENE_BUFFERS;

typedef struct HD_STATE
{
    sint32 initialized;
    sint32 collecting;
    sint32 frame_started;
    sint32 capture_sources;
    sint32 capture_native;
    sint32 prepared;
    sint32 paused;
    sint32 last_mode;
    sint32 fresh_og;
    float clear[3];
    HD_SCENE_BUFFERS frozen;
    HD_SCENE_BUFFERS completed;
    sint32 completed_valid;
    sint32 frame_ready;
    sint32 race_frame;
    sint32 race_ready;
    float render_phase;
    float presented_phase, pause_phase;
    GpuRenderList *native_lists[HD_VIEWS];
    DRAWENV native_environments[HD_VIEWS];
    uint8 native_valid[HD_VIEWS];
    uint32 views;
    uint32 view;
    uint32 race_pose_views;
    sint32 width;
    sint32 height;
    HD_VIEW viewports[HD_VIEWS + 1];
    VID_SHADER *ui_shader;
    VID_TEXTURE *vram;
    uint32 vram_version;
    sint32 vram_valid;
    VID_MATERIAL *ui_materials[HD_BLEND_PASSES];
    HD_UI_MESH *ui_meshes;
    HD_UI_MESH *ui_cursor;
    HD_UI_DRAW *ui_draws;
    uint32 ui_count;
    uint32 ui_capacity;
    uint32 ui_layer;
    sint32 ui_anchor;
    sint32 ui_clipped;
    VID_RECT ui_clip;
    float ui_offset[2];
    VID_SHADER *world_shader;
    VID_MATERIAL *world_materials[HD_BLEND_PASSES];
    VID_MATERIAL *sky_materials[HD_BLEND_PASSES];
    HD_MODEL *models;
    HD_MODEL *retired_models;
    MATRIXF camera;
    MATRIXF billboard;
    MATRIXF screen_rotation;
    VECTORF camera_position;
    MATRIXF transform;
    MATRIXF light;
    float ambient[3];
    const HD_LIGHT_SAMPLE *light_override;
    const CVECTOR *environment_tint_override;
    HD_MODEL_DRAW *model_draws;
    uint32 model_count;
    uint32 model_capacity;
    HD_WORLD_MESH *world_meshes;
    VID_MATERIAL *effect_materials[HD_BLEND_PASSES];
    VID_MATERIAL *solid_materials[HD_BLEND_PASSES];
    VID_MATERIAL *environment_materials[HD_BLEND_PASSES];
    TEXTURE_DESCRIPTOR flares[2];
    sint32 flares_valid;
    uint64 motion_serial;
    uint32 motion_generation;
    sint32 actor;
    HD_MOTION camera_motion[HD_VIEWS];
    HD_PROJECTION_MOTION projection_motion[HD_VIEWS];
    HD_ENV_MOTION environment_motion[HD_VIEWS][15];
    HD_LIGHT_MOTION car_lighting[HD_VIEWS][15];
    HD_POLICE_MOTION police_motion[HD_VIEWS][14][4];
    HD_NEEDLE_MOTION needles[HD_VIEWS][14];
    HD_MAP_MOTION maps[HD_VIEWS];
    float map_scale[2];
    VID_SHADER *map_shader;
    VID_MATERIAL *map_materials[HD_BLEND_PASSES];
    HD_MODEL police_model;
    VID_SHADER *police_shader;
    VID_MATERIAL *police_materials[HD_BLEND_PASSES];
    HD_MOTION car_motion[HD_VIEWS][14];
    HD_POSE object_pose;
    sint32 object_relative;
    HD_WHEEL_MOTION wheels[HD_VIEWS][15][4];
    HD_WHEEL_MOTION *wheel;
    sint32 wheel_hub;
    HD_MOTION preview_motion[HD_VIEWS];
    HD_PARTICLE_MOTION particles[HD_VIEWS][50];
    HD_LINE_STORAGE lines[HD_VIEWS][50];
    HD_SNOW_MOTION snow[HD_VIEWS][128];
    HD_RAIN_MOTION rain[HD_VIEWS][128];
    HD_SKID_STORAGE skids[HD_VIEWS][80];
    HD_SHADOW_STORAGE shadows[HD_VIEWS][14][2];
    VID_MATERIAL *retire_materials[HD_BLEND_PASSES];
    VID_MATERIAL *ui_retire_materials[HD_BLEND_PASSES];
    HD_EFFECT_RETIREMENT retired_effects[HD_VIEWS][50];
} HD_STATE;

static HD_MODEL *model_lookup[16384];
static sint32 original_track_suppressed;

static struct
{
    uint64 model_builds, atlas_decodes, images_created, static_batch_builds, static_batch_reuses, vram_rows, seam_components, late_texture_models;
} hd_work;

static uint16 vram_snapshot[1024 * 512];
static uint32 vram_row_versions[512];
static VID_TARGET *preparation_target;
static sint32 preparation_active, preparation_blends_ready;

static struct
{
    float radius, height, color[3];
} distance_lighting;

static uint32 model_bucket(const void *source, uint32 size, const void *visibility, uint32 variant)
{
    return (uint32)(((uintptr_t)source >> 4) ^ size ^ ((uintptr_t)visibility >> 3) ^ variant) & 16383;
}

static HD_MODEL *find_model(const MODEL_RESOURCE *resource, const sint8 *visibility, uint32 variant)
{
    HD_MODEL *model;
    for (model = model_lookup[model_bucket(resource->bytes, resource->capacity, visibility, variant)]; model != NULL; model = model->hash_next)
        if (model->source == resource->bytes && model->source_size == resource->capacity && model->visibility == visibility && model->variant == variant)
            return model;
    return NULL;
}

static void unhash_model(HD_MODEL *model)
{
    HD_MODEL **link = &model_lookup[model_bucket(model->source, model->source_size, model->visibility, model->variant)];
    while (*link != model)
        link = &(*link)->hash_next;
    *link = model->hash_next;
}

static HD_STATE hd;
sint32 game_hd_frame;

static void swap_scene_buffers(HD_SCENE_BUFFERS *other);
static void car_environment_candidate(const MODEL_RESOURCE *resource, const CAR_STATE *car);

sint32 hd_capture_active(void)
{
    return hd.initialized && hd.collecting && hd.capture_sources;
}

static const char ui_vertex_source[] = "layout(location=0) in vec4 aPos;\n"
                                       "layout(location=1) in vec2 aUV;\n"
                                       "layout(location=2) in vec4 aColor;\n"
                                       "layout(location=4) in float aAlpha;\n"
                                       "layout(location=5) in uvec4 aTex;\n"
                                       "layout(location=6) in uvec2 aRect;\n"
                                       "out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;\n"
                                       "flat out uvec4 vPage;flat out uvec2 vRect;uniform vec4 uDraw[4];\n"
                                       "void main(){vec2 point=vec2(dot(uDraw[2].xy,aPos.xy),dot(uDraw[2].zw,aPos.xy))+uDraw[3].xy;float x=point.x-uDraw[0].y+aPos.z*(uDraw[0].x-uDraw[1].x)*0.5;"
                                       "gl_Position=vec4(2.0*x/uDraw[0].x,1.0-2.0*(point.y-uDraw[0].z)/uDraw[0].w,0.0,1.0);"
                                       "vUV=aUV;vColor=vec4(uDraw[3].z>=0.0?vec3(uDraw[3].z):aColor.rgb/255.0,aAlpha*uDraw[3].w);"
                                       "vDepth=0.0;vFog=vec2(0.0);vPage=aTex;vRect=aRect;}\n";

static float hd_far_plane(void)
{
    return fmaxf((float)model_render.far_depth * 4.0f, hd.race_frame || hd.paused ? hd_track_horizon() * 1.125f : 0.0f);
}

static const char texture_fragment_source[] = "flat in uvec4 vPage;flat in uvec2 vRect;\n"
                                              "uint vidPsxWord(ivec2 p){uvec4 c=uvec4(round(texelFetch(uTex,p,0)*vec4(31.0,31.0,31.0,1.0)));"
                                              "return c.r|(c.g<<5)|(c.b<<10)|(c.a<<15);}\n" VID_PSX_SAMPLING_GLSL "void main(){uint flags=vPage.z;uint pass=uint(uParams[0].x);"
                                              "bool textured=(flags&1u)!=0u;bool semi=(flags&4u)!=0u;"
                                              "if(pass==1u&&!textured&&semi)discard;"
                                              "vec4 color=vColor;"
                                              "if(textured){ivec4 region=ivec4(int(vPage.w&255u),int(vPage.w>>8),ivec2(vRect));"
                                              "uint word;uint repeatAxes=((flags&32u)!=0u?1u:0u)|((flags&64u)!=0u?2u:0u);"
                                              "vec2 uv=vUV;bool filtered=(flags&8u)!=0u;"
                                              "vec4 texel;"
                                              "if(uParams[0].w>0.0){texel=texture(uTex,uv/vec2(region.zw));if(texel.a<=uParams[1].w&&(flags&256u)==0u)discard;color.a*=texel.a;}"
                                              "else if((flags&256u)!=0u){texel=vidPsxFilter(uv,vPage.x,vPage.y,region,repeatAxes,0u,true);}"
                                              "else{if(filtered&&!semi)texel=vidPsxFilter(uv,vPage.x,vPage.y,region,repeatAxes,0u,(flags&16u)!=0u);"
                                              "else texel=vidPsxSample(uv,vPage.x,vPage.y,region,repeatAxes,semi?pass:0u,"
                                              "filtered,(flags&16u)!=0u,word);if(texel.a==0.0)discard;}"
                                              "color.rgb=texel.rgb*((flags&2u)!=0u?vec3(1.0):vColor.rgb*(255.0/128.0));}"
                                              "color.rgb*=uParams[0].y;color.a*=uParams[0].z;outColor=vidFog(color);}\n";

static const char world_vertex_source[] = "uniform vec4 uFog[3];\n"
                                          "layout(location=0) in vec4 aPos;layout(location=1) in vec2 aUV;"
                                          "layout(location=2) in vec4 aColor;layout(location=3) in vec4 aNormal;"
                                          "layout(location=4) in vec4 aExtra;\n"
                                          "#ifndef TD5_SCENE_BATCH\nlayout(location=5) in uvec4 aTex;layout(location=6) in uvec2 aRect;\n"
                                          "#else\nuniform vec4 uParams[2];\n#define aTex uvec4(uint(aExtra.x),uint(aExtra.y),uint(aExtra.w),uint(aExtra.z))\n#define aRect uvec2(uParams[1].xy)\n#endif\n"
                                          "out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;out vec3 vViewPosition;"
                                          "flat out uvec4 vPage;flat out uvec2 vRect;uniform vec4 uDraw[32];\n"
                                          "void main(){vec4 point=vec4(aPos.xyz,1.0);"
                                          "vec3 p=vec3(dot(uDraw[0],point),dot(uDraw[1],point),dot(uDraw[2],point));"
                                          "vViewPosition=p;vec4 projection=uDraw[3];vec4 center=uDraw[4];"
                                          "float depth=(projection.w+projection.z)/(projection.w-projection.z)*p.z"
                                          "-2.0*projection.w*projection.z/(projection.w-projection.z)-uDraw[23].w*p.z;"
                                          "if(uDraw[24].w>0.0){float orderZ=max(projection.z,p.z-uDraw[24].w);"
                                          "depth=((projection.w+projection.z)/(projection.w-projection.z)"
                                          "-2.0*projection.w*projection.z/((projection.w-projection.z)*orderZ)-uDraw[23].w)*p.z;}"
                                          "gl_Position=vec4(p.x*projection.x+center.x*p.z,"
                                          "-p.y*projection.y+center.y*p.z,depth,p.z);"
                                          "vec3 color=aColor.rgb/255.0;vec2 uv=aUV*0.5;vPage=aTex;vRect=aRect;"
                                          "if(uDraw[12].x!=0.0){color=uDraw[12].yzw/255.0;vec2 atlas=uDraw[17].xy;"
                                          "if(uDraw[12].x==1.0){vec2 screen=p.xy*uDraw[18].zw/p.z+uDraw[4].zw;"
                                          "vec2 n=vec2(dot(uDraw[0].xyz,aNormal.xyz),dot(uDraw[1].xyz,aNormal.xyz));"
                                          "uv=(screen-uDraw[15].xy)*uDraw[15].zw+n*uDraw[16].xy+atlas+uDraw[16].z;"
                                          "}else{vec4 xs=vec4(aExtra.x,aExtra.z,aNormal.x,aNormal.z),zs=vec4(aExtra.y,aExtra.w,aNormal.y,aNormal.w);"
                                          "vec4 us=(xs*uDraw[13].x-zs*uDraw[13].y+uDraw[13].z)/8.0;"
                                          "vec4 vs=(xs*uDraw[13].y+zs*uDraw[13].x+uDraw[13].w)/8.0;"
                                          "float mu=min(min(us.x,us.y),min(us.z,us.w)),mv=min(min(vs.x,vs.y),min(vs.z,vs.w));"
                                          "vec2 period=uDraw[14].xy;vec2 mask=vec2(period.x>0.0?floor(mu/period.x)*period.x:0.0,period.y>0.0?floor(mv/period.y)*period.y:0.0);"
                                          "uv=vec2((aPos.x*uDraw[13].x-aPos.z*uDraw[13].y+uDraw[13].z)/8.0,"
                                          "(aPos.x*uDraw[13].y+aPos.z*uDraw[13].x+uDraw[13].w)/8.0)-mask+atlas;}"
                                          "uv=mod(uv,256.0)-atlas;vPage=uvec4(uint(uDraw[18].x),uint(uDraw[18].y),141u,uint(atlas.x)|(uint(atlas.y)<<8));"
                                          "vRect=uvec2(uDraw[17].zw);}"
                                          "else if((aNormal.w!=0.0&&aPos.w!=0.0)||uDraw[19].w!=0.0){vec3 normal=uDraw[19].w!=0.0?uDraw[19].xyz:aNormal.xyz/4096.0;"
                                          "vec3 light=clamp(vec3(dot(uDraw[5].xyz,normal),dot(uDraw[6].xyz,normal),dot(uDraw[7].xyz,normal)),vec3(0.0),vec3(32767.0/4096.0));"
                                          "vec3 illumination=clamp(uDraw[11].xyz+vec3(dot(uDraw[8].xyz,light),dot(uDraw[9].xyz,light),dot(uDraw[10].xyz,light)),vec3(0.0),vec3(32767.0/4096.0));"
                                          "if(uDraw[29].w<1.0){vec3 oldLight=clamp(vec3(dot(uDraw[23].xyz,normal),dot(uDraw[24].xyz,normal),dot(uDraw[25].xyz,normal)),vec3(0.0),vec3(32767.0/4096.0));"
                                          "vec3 oldIllumination=clamp(uDraw[29].xyz+vec3(dot(uDraw[26].xyz,oldLight),dot(uDraw[27].xyz,oldLight),dot(uDraw[28].xyz,oldLight)),vec3(0.0),vec3(32767.0/4096.0));"
                                          "illumination=mix(oldIllumination,illumination,uDraw[29].w);}color*=illumination;"
                                          "if(uDraw[19].w!=0.0){vec4 anchor=vec4(uDraw[20].xyz,1.0);float z=clamp(dot(uDraw[2],anchor)/uDraw[22].y,0.0,65535.0);"
                                          "float q=min(131071.0,uDraw[22].x*65536.0/max(z,0.00001));"
                                          "float cue=clamp((q*uDraw[20].w+uDraw[21].w)/16777216.0,0.0,1.0);"
                                          "color+=cue*clamp(uDraw[21].rgb-color,vec3(-32768.0/(16.0*255.0)),vec3(32767.0/(16.0*255.0)));}"
                                          "color=clamp(color,vec3(0.0),vec3(1.0));}"
                                          "if(uDraw[11].w>=0.0)color=vec3(uDraw[11].w);"
                                          "vPage.z|=8u;vColor=vec4(color,uDraw[5].w);vUV=uv;vDepth=p.z;vFog=vec2(0.0);"
                                          "if(uFog[0].y>uFog[0].x){float d=length(p);"
                                          "vFog=clamp(vec2(log(max(d,uFog[0].x)/uFog[0].x)/log(uFog[0].y/uFog[0].x),"
                                          "log(max(d,uFog[0].y)/uFog[0].y)/log(uFog[0].z/uFog[0].y)),0.0,1.0);vFog*=vFog;}}\n";

static const char scene_batch_fragment_source[] = "flat in uvec4 vPage;flat in uvec2 vRect;\n"
                                                  "#ifdef TD5_HEADLIGHTS\n"
                                                  "in vec3 vViewPosition;uniform vec4 uDraw[32];\n"
                                                  "float headlight(){if(uDraw[31].w==0.0)return 0.0;"
                                                  "float distance=length(vViewPosition-uDraw[30].xyz);"
                                                  "return clamp(uDraw[31].w*(1.0-distance/uDraw[30].w),0.0,1.0);}\n"
                                                  "#endif\n"
                                                  "void main(){uint flags=vPage.z;float layer=(flags&256u)!=0u||(flags&4u)==0u?0.0:(uParams[0].x==1.0?1.0:2.0);"
                                                  "vec4 texel;if((flags&1024u)!=0u){vec2 uv=vUV/vec2(vRect);uint mode=vPage.w;"
                                                  "if((flags&272u)!=0u||(flags&4u)==0u||mode==0u){texel=texture(uTex,vec3(uv,float(vPage.x)));if(mode==3u)texel+=texture(uTex,vec3(uv,float(vPage.y)));}"
                                                  "else if(uParams[0].x==1.0){texel=mode==2u?vec4(0.0):texture(uTex,vec3(uv,float(vPage.x)));}"
                                                  "else{texel=mode==1u?vec4(0.0):texture(uTex,vec3(uv,float(vPage.y)));}"
                                                  "}else texel=texture(uTex,vec3(vUV/vec2(vRect),layer));"
                                                  "if((flags&272u)!=0u)texel.a=1.0;if(texel.a<=uParams[1].w&&(flags&256u)==0u)discard;"
                                                  "vec3 modulation=(flags&2u)!=0u?vec3(1.0):vColor.rgb*(255.0/128.0);"
                                                  "float coverage=(uParams[0].x==2.0&&uParams[0].z==1.0)?1.0:max(texel.a,0.00001);"
                                                  "vec3 color=(texel.rgb/coverage)*modulation*uParams[0].y;\n"
                                                  "if((flags&8192u)!=0u)color=floor(color*31.0+0.00001)/31.0;\n"
                                                  "#ifdef TD5_HEADLIGHTS\n"
                                                  "if((flags&4096u)!=0u)color+=(texel.rgb/coverage)*uDraw[31].rgb*headlight()*((flags&16384u)!=0u?0.5:1.0);\n"
                                                  "#endif\n"
                                                  "outColor=vidFog(vec4(color,vColor.a*texel.a*uParams[0].z));}\n";

static const char police_vertex_source[] = "uniform vec4 uFog[3];\n"
                                           "layout(location=0) in vec4 aPos;"
                                           "out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;"
                                           "flat out uvec4 vPage;flat out uvec2 vRect;uniform vec4 uDraw[12];\n"
                                           "void main(){vec4 anchor=vec4(uDraw[6].xyz,1.0);"
                                           "vec3 p=vec3(dot(uDraw[0],anchor),dot(uDraw[1],anchor),dot(uDraw[2],anchor));"
                                           "int part=int(aPos.z);vUV=vec2(0.0);vColor=vec4(0.0);vDepth=p.z;vFog=vec2(0.0);"
                                           "if(uFog[0].y>uFog[0].x){float d=length(p);"
                                           "vFog=clamp(vec2(log(max(d,uFog[0].x)/uFog[0].x)/log(uFog[0].y/uFog[0].x),"
                                           "log(max(d,uFog[0].y)/uFog[0].y)/log(uFog[0].z/uFog[0].y)),0.0,1.0);vFog*=vFog;}"
                                           "vPage=uvec4(uint(uDraw[7].x),uint(uDraw[7].y),0u,uint(uDraw[7].z)|(uint(uDraw[7].w)<<8));vRect=uvec2(uDraw[8].xy);"
                                           "float raw=p.z/uDraw[8].z;float order=raw*0.25;"
                                           "if(p.z<=1.0||order<=uDraw[8].w||order>=uDraw[11].w){gl_Position=vec4(0.0,0.0,2.0,1.0);return;}"
                                           "float correction=max(0.0,(raw-400.0)*4096.0/5120.0);float size=32768.0/(raw-correction)*uDraw[6].w/4096.0;"
                                           "vec2 screen=p.xy*uDraw[5].xy/p.z+uDraw[4].zw;vec2 radial=screen-uDraw[5].zw;vec2 offset;"
                                           "if(part==0){float angle=dot(radial,radial)>0.0?atan(radial.x,radial.y):0.0;float c=cos(angle),s=sin(angle);vec2 q=aPos.xy*size;"
                                           "offset=vec2((q.x*c-q.y*s)*0.25,(q.x*s+q.y*c)*(4096.0/32767.0));"
                                           "vUV=vec2((aPos.x+1.0)*0.5,(1.0-aPos.y)*0.5)*uDraw[8].xy;"
                                           "vColor=vec4(uDraw[9].rgb/255.0,1.0);vPage.z=uint(uDraw[9].w)|8u;}"
                                           "else{vec2 direction=-radial/max(length(radial),0.000001);float angle=aPos.x*size*(6.283185307179586/4096.0);"
                                           "float c=cos(angle),s=sin(angle),sign=part==1?-1.0:1.0;"
                                           "offset=sign*size*4.0*vec2(direction.x*c-direction.y*s,direction.x*s+direction.y*c)*aPos.w;"
                                           "vec4 color=part==1?uDraw[10]:uDraw[11];vColor=vec4(color.rgb*(1.0-aPos.w)/255.0,1.0);"
                                           "vPage.z=uint(part==1?uDraw[10].w:uDraw[3].z);}"
                                           "p.xy+=offset*p.z/uDraw[5].xy;vec4 projection=uDraw[3];"
                                           "float depth=(projection.w+1.0)/(projection.w-1.0)*p.z-2.0*projection.w/(projection.w-1.0);"
                                           "gl_Position=vec4(p.x*projection.x+uDraw[4].x*p.z,-p.y*projection.y+uDraw[4].y*p.z,depth,p.z);}\n";

static const char map_vertex_source[] = "layout(location=0) in vec4 aPos;layout(location=1) in vec2 aUV;layout(location=2) in vec4 aColor;"
                                        "out vec2 vUV;out vec4 vColor;out float vDepth;out vec2 vFog;out vec2 vMapPoint;uniform vec4 uDraw[6];\n"
                                        "vec2 projectPoint(vec2 p){vec2 d=p-uDraw[0].xy;return vec2(50.0,32.0)+"
                                        "vec2(d.x*uDraw[0].z+d.y*uDraw[0].w,-d.x*uDraw[0].w+d.y*uDraw[0].z)*uDraw[1].xy;}"
                                        "void main(){vec2 point;int mode=int(uDraw[1].z);"
                                        "if(mode==1)point=projectPoint(uDraw[3].zw)+aPos.xy;else{point=projectPoint(aPos.xy);"
                                        "if(mode==2){vec2 delta=projectPoint(aPos.zw)-point;float len=length(delta);"
                                        "if(len>0.000001)point+=vec2(delta.y,-delta.x)/len*aUV.x;}}"
                                        "point+=uDraw[2].xy;vMapPoint=point;"
                                        "float x=point.x-uDraw[4].y-(uDraw[4].x-uDraw[5].x)*0.5;"
                                        "gl_Position=vec4(2.0*x/uDraw[4].x,1.0-2.0*(point.y-uDraw[4].z)/uDraw[4].w,0.0,1.0);"
                                        "vUV=vec2(0.0);vColor=vec4(aColor.rgb/255.0,1.0);vDepth=0.0;vFog=vec2(0.0);}\n";

static const char map_fragment_source[] = "in vec2 vMapPoint;uniform vec4 uDraw[6];"
                                          "void main(){if(any(lessThan(vMapPoint,uDraw[2].zw))||any(greaterThanEqual(vMapPoint,uDraw[2].zw+uDraw[3].xy)))discard;"
                                          "if(uint(uParams[0].x)==1u&&uDraw[1].w!=0.0)discard;"
                                          "outColor=vec4(vColor.rgb*uParams[0].y,vColor.a*uParams[0].z);}\n";

static void matrix_from_native(const MATRIX *source, MATRIXF *destination)
{
    uint32 row, column;
    for (row = 0; row < 3; ++row)
    {
        for (column = 0; column < 3; ++column)
            destination->m[row][column] = source->m[row][column] / 4096.0f;
        destination->t[row] = (float)source->t[row];
    }
}

void hd_pose_from_native(HD_POSE *pose, const MATRIX *rotation, const VEC3 *position)
{
    MATRIXF matrix;
    matrix_from_native(rotation, &matrix);
    MatrixQuatf(&matrix, &pose->rotation);
    NormalizeQuatf(&pose->rotation, &pose->rotation);
    pose->position.x = position->x / 256.0f;
    pose->position.y = position->y / 256.0f;
    pose->position.z = position->z / 256.0f;
}

void hd_pose_interpolate(const HD_POSE *previous, const HD_POSE *current, float phase, HD_POSE *result)
{
    uint32 component;
    XASSERT(phase >= 0.0f && phase <= 1.0f);
    for (component = 0; component < 3; ++component)
        result->position.v[component] = previous->position.v[component] + phase * (current->position.v[component] - previous->position.v[component]);
    SlerpQuatf(&previous->rotation, &current->rotation, phase, &result->rotation);
}

static void motion_update(HD_MOTION *motion, const HD_POSE *pose, uint64 key)
{
    if (motion->serial == hd.motion_serial && motion->generation == hd.motion_generation && motion->key == key)
        return;
    if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation || motion->key != key)
        motion->previous = *pose;
    else
        motion->previous = motion->current;
    motion->current = *pose;
    motion->serial = hd.motion_serial;
    motion->generation = hd.motion_generation;
    motion->key = key;
}

static uint64 capture_camera_pose(void)
{
    HD_POSE camera;
    const CAMERA_VIEW *native_camera = &camera_views[hd.view];
    uint64 key = ((uint64)native_camera->sel_mode << 32) | native_camera->cue_idx;
    uint32 tracked = hd.views == 1 ? race_view.tracked_car : hd.view;
    key ^= ((uint64)tracked << 56) ^ ((uint64)(native_camera->attached != 0) << 55);
    if (hd.race_frame && (hd.camera_motion[hd.view].generation != hd.motion_generation || hd.camera_motion[hd.view].key != key))
        hd_camera_reset(hd.view);
    MatrixQuatf(&hd.camera, &camera.rotation);
    NormalizeQuatf(&camera.rotation, &camera.rotation);
    camera.position = (VEC3F){hd.camera_position.vx, hd.camera_position.vy, hd.camera_position.vz};
    motion_update(&hd.camera_motion[hd.view], &camera, key);
    return key;
}

void hd_distance_lighting(uint32 radius, const uint8 color[3], uint32 height)
{
    uint32 channel;
    distance_lighting.radius = (float)radius;
    distance_lighting.height = height / 256.0f;
    for (channel = 0; channel < 3; ++channel)
        distance_lighting.color[channel] = color[channel] / 128.0f;
}

void hd_capture_race_poses(void)
{
    uint32 slot;
    if (!hd_capture_active())
        return;
    XASSERT(hd.race_frame && !(hd.race_pose_views & (1u << hd.view)));
    capture_camera_pose();
    for (slot = 0; slot < 14; ++slot)
    {
        HD_POSE pose;
        hd_pose_from_native(&pose, &car_states[slot].transform, &car_states[slot].pos);
        motion_update(&hd.car_motion[hd.view][slot], &pose, 0);
    }
    {
        HD_VIEW *view = &hd.viewports[hd.view];
        uint32 tracked = hd.views == 1 ? race_view.tracked_car : hd.view;
        view->headlight_active = 0;
        if (tracked < 14 && distance_lighting.radius > 0.0f && (scene_level_image.bytes[scene_level_image.desc.header_offset + 30] & 1) != 0)
        {
            view->headlight_car[0] = hd.car_motion[hd.view][tracked].previous;
            view->headlight_car[1] = hd.car_motion[hd.view][tracked].current;
            view->headlight_origin = (VECTORF){0, (float)(sint32)model_render.reference_offset_y, (float)(sint32)model_render.reference_offset_z};
            view->headlight_active = 1;
        }
    }
    hd.race_pose_views |= 1u << hd.view;
}

sint32 hd_car_begin(uint32 slot)
{
    HD_POSE pose;
    sint32 previous = hd.actor;
    if (!hd_capture_active())
        return previous;
    XASSERT(slot < 14 && hd.view < HD_VIEWS);
    hd.actor = (sint32)slot;
    if (hd.race_frame)
        XASSERT(hd.race_pose_views & (1u << hd.view));
    else
    {
        hd_pose_from_native(&pose, &car_states[slot].transform, &car_states[slot].pos);
        motion_update(&hd.car_motion[hd.view][slot], &pose, 0);
    }
    return previous;
}

void hd_car_end(sint32 previous)
{
    hd.actor = previous;
}

static void reset_facing(uint32 view)
{
    HD_MODEL *model;
    for (model = hd.models; model != NULL; model = model->next)
        model->facing[view].generation = ~hd.motion_generation;
}

void hd_car_reset(uint32 slot)
{
    uint32 view, wheel;
    XASSERT(slot < 14);
    for (view = 0; view < HD_VIEWS; ++view)
    {
        uint32 tracked = race_session.two_players != 0 ? view : race_view.tracked_car;
        if (tracked == slot)
            hd_camera_reset(view);
        hd.car_motion[view][slot].generation = ~hd.motion_generation;
        hd.environment_motion[view][slot].generation = ~hd.motion_generation;
        hd.car_lighting[view][slot].generation = ~hd.motion_generation;
        hd.needles[view][slot].generation = ~hd.motion_generation;
        hd.maps[view].reset_slots |= 1u << slot;
        hd.shadows[view][slot][0].generation = ~hd.motion_generation;
        hd.shadows[view][slot][1].generation = ~hd.motion_generation;
        for (wheel = 0; wheel < 4; ++wheel)
        {
            hd.wheels[view][slot][wheel].generation = ~hd.motion_generation;
            hd.police_motion[view][slot][wheel].generation = ~hd.motion_generation;
        }
    }
}

void hd_camera_reset(uint32 view)
{
    uint32 slot, wheel;
    HD_MODEL *model;
    XASSERT(view < HD_VIEWS);
    reset_facing(view);
    hd.camera_motion[view].generation = ~hd.motion_generation;
    hd.projection_motion[view].generation = ~hd.motion_generation;
    hd.preview_motion[view].generation = ~hd.motion_generation;
    hd.maps[view].generation = ~hd.motion_generation;
    // a camera cut must not combine current camera and previous world poses
    for (model = hd.models; model != NULL; model = model->next)
    {
        model->motion[view].generation = ~hd.motion_generation;
        model->lighting[view].generation = ~hd.motion_generation;
    }
    for (slot = 0; slot < 15; ++slot)
    {
        hd.environment_motion[view][slot].generation = ~hd.motion_generation;
        hd.car_lighting[view][slot].generation = ~hd.motion_generation;
        for (wheel = 0; wheel < 4; ++wheel)
            hd.wheels[view][slot][wheel].generation = ~hd.motion_generation;
        if (slot < 14)
        {
            uint32 light;
            hd.car_motion[view][slot].generation = ~hd.motion_generation;
            hd.needles[view][slot].generation = ~hd.motion_generation;
            hd.shadows[view][slot][0].generation = ~hd.motion_generation;
            hd.shadows[view][slot][1].generation = ~hd.motion_generation;
            for (light = 0; light < 4; ++light)
                hd.police_motion[view][slot][light].generation = ~hd.motion_generation;
        }
    }
    for (slot = 0; slot < 50; ++slot)
    {
        hd.particles[view][slot].generation = ~hd.motion_generation;
        hd.lines[view][slot].generation = ~hd.motion_generation;
        hd.retired_effects[view][slot].kind = 0;
    }
    for (slot = 0; slot < 128; ++slot)
    {
        hd.snow[view][slot].motion.generation = ~hd.motion_generation;
        hd.rain[view][slot].generation = ~hd.motion_generation;
    }
    for (slot = 0; slot < 80; ++slot)
    {
        hd.skids[view][slot].generation = ~hd.motion_generation;
        hd.skids[view][slot].retire_pending = 0;
    }
}

void hd_wheel_begin(uint32 wheel, const SVECTOR *center, float steering, float spin, float period)
{
    HD_WHEEL_SAMPLE sample;
    HD_WHEEL_MOTION *motion;
    uint32 actor = hd.actor >= 0 ? (uint32)hd.actor : 14;
    if (!hd_capture_active())
        return;
    XASSERT(wheel < 4 && period > 0.0f);
    motion = &hd.wheels[hd.view][actor][wheel];
    sample.center = (VECTORF){(float)center->vx, (float)center->vy, (float)center->vz};
    sample.steering = steering;
    sample.spin = spin;
    if (motion->serial != hd.motion_serial || motion->generation != hd.motion_generation)
    {
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation)
            motion->previous = sample;
        else
        {
            float delta = spin - motion->current.spin;
            delta -= floorf((delta + period * 0.5f) / period) * period;
            motion->previous = motion->current;
            motion->previous.spin = spin - delta;
        }
        motion->current = sample;
        motion->serial = hd.motion_serial;
        motion->generation = hd.motion_generation;
    }
    if (actor == 14)
        motion_update(&hd.preview_motion[hd.view], &hd.object_pose, 0);
    hd.wheel = motion;
    hd.wheel_hub = 0;
}

void hd_wheel_hub(void)
{
    hd.wheel_hub = 1;
}

void hd_wheel_end(void)
{
    hd.wheel = NULL;
}

void hd_effect_reset(uint32 slot)
{
    uint32 view;
    XASSERT(slot < 50);
    for (view = 0; view < HD_VIEWS; ++view)
    {
        hd.particles[view][slot].generation = ~hd.motion_generation;
        hd.lines[view][slot].generation = ~hd.motion_generation;
        if (hd.retired_effects[view][slot].generation != hd.motion_generation)
            hd.retired_effects[view][slot].kind = 0;
    }
}

void hd_effect_retire(uint32 slot, const VEC3 *position, const VEC3 *tail)
{
    uint32 view;
    XASSERT(slot < 50);
    for (view = 0; view < HD_VIEWS; ++view)
    {
        HD_EFFECT_RETIREMENT *retired = &hd.retired_effects[view][slot];
        const HD_PARTICLE_MOTION *particle = &hd.particles[view][slot];
        const HD_LINE_STORAGE *line = &hd.lines[view][slot];
        if (particle->generation == hd.motion_generation && particle->serial != 0 && (particle->serial == hd.motion_serial || particle->serial + 1 == hd.motion_serial))
        {
            float divisor = particle->screen ? 1.0f : 256.0f;
            retired->particle[0] = retired->particle[1] = particle->current;
            retired->particle[1].position = (VECTORF){position->x / divisor, position->y / divisor, position->z / divisor};
            retired->texture = particle->texture;
            retired->color = particle->color;
            retired->height_ratio = particle->height_ratio;
            retired->layer = particle->layer;
            retired->kind = particle->screen ? 2 : 1;
            retired->generation = hd.motion_generation;
        }
        else if (line->generation == hd.motion_generation && line->serial != 0 && (line->serial == hd.motion_serial || line->serial + 1 == hd.motion_serial))
        {
            memcpy(retired->line[0], line->current, sizeof(line->current));
            retired->line[1][0] = (VECTORF){position->x / 256.0f, position->y / 256.0f, position->z / 256.0f};
            retired->line[1][1] = tail != NULL ? (VECTORF){tail->x / 256.0f, tail->y / 256.0f, tail->z / 256.0f} : line->current[0];
            memcpy(retired->line_colors, line->colors, sizeof(line->colors));
            retired->blend = line->blend;
            retired->kind = 3;
            retired->generation = hd.motion_generation;
        }
    }
}

void hd_camera_position(const VECTOR *position)
{
    hd.camera_position.vx = position->vx / 256.0f;
    hd.camera_position.vy = position->vy / 256.0f;
    hd.camera_position.vz = position->vz / 256.0f;
}

void hd_camera_angles(const SVECTOR *angles)
{
    const float radians = 6.2831853071795864769f / 4096.0f;
    float sx = sinf(angles->vx * radians), cx = cosf(angles->vx * radians);
    float sy = sinf(angles->vy * radians), cy = cosf(angles->vy * radians);
    float sz = sinf(angles->vz * radians), cz = cosf(angles->vz * radians);
    IdentityMatrixf(&hd.camera);
    hd.camera.m[0][0] = cy * cz - sx * sy * sz;
    hd.camera.m[0][1] = -cx * sz;
    hd.camera.m[0][2] = sy * cz + sx * cy * sz;
    hd.camera.m[1][0] = cy * sz + sx * sy * cz;
    hd.camera.m[1][1] = cx * cz;
    hd.camera.m[1][2] = sy * sz - sx * cy * cz;
    hd.camera.m[2][0] = -cx * sy;
    hd.camera.m[2][1] = sx;
    hd.camera.m[2][2] = cx * cy;
    IdentityMatrixf(&hd.billboard);
    hd.billboard.m[0][0] = cz;
    hd.billboard.m[0][1] = -cx * sz;
    hd.billboard.m[0][2] = sx * cz;
    hd.billboard.m[1][0] = sz;
    hd.billboard.m[1][1] = cx * cz;
    hd.billboard.m[1][2] = -sx * cz;
    hd.billboard.m[2][1] = sx;
    hd.billboard.m[2][2] = cx;
    IdentityMatrixf(&hd.screen_rotation);
    RotMatrixZf(angles->vz * radians, &hd.screen_rotation);
}

void hd_camera_aim(const VECTOR *target, sint32 roll)
{
    VECTORF forward = {target->vx / 256.0f - hd.camera_position.vx, target->vy / 256.0f - hd.camera_position.vy, target->vz / 256.0f - hd.camera_position.vz};
    float length = sqrtf(forward.vx * forward.vx + forward.vy * forward.vy + forward.vz * forward.vz);
    float horizontal;
    MATRIXF flip;
    if (length == 0.0f)
        return;
    forward.vx /= length;
    forward.vy /= length;
    forward.vz /= length;
    horizontal = sqrtf(forward.vx * forward.vx + forward.vz * forward.vz);
    IdentityMatrixf(&hd.camera);
    IdentityMatrixf(&hd.billboard);
    if (horizontal > 0.0f)
    {
        hd.camera.m[0][0] = forward.vz / horizontal;
        hd.camera.m[0][2] = -forward.vx / horizontal;
        hd.camera.m[1][0] = -forward.vx * forward.vy / horizontal;
        hd.camera.m[1][2] = -forward.vy * forward.vz / horizontal;
    }
    hd.camera.m[1][1] = horizontal;
    hd.camera.m[2][0] = forward.vx;
    hd.camera.m[2][1] = forward.vy;
    hd.camera.m[2][2] = forward.vz;
    if (horizontal == 0.0f)
        hd.camera.m[1][2] = -forward.vy;
    hd.billboard.m[1][1] = horizontal;
    hd.billboard.m[1][2] = -forward.vy;
    hd.billboard.m[2][1] = forward.vy;
    hd.billboard.m[2][2] = horizontal;
    RotMatrixZf(roll * (6.2831853071795864769f / 4096.0f), &hd.camera);
    RotMatrixZf(roll * (6.2831853071795864769f / 4096.0f), &hd.billboard);
    IdentityMatrixf(&flip);
    flip.m[0][0] = flip.m[2][2] = -1.0f;
    MulMatrix0f(&hd.billboard, &flip, &hd.billboard);
    hd.screen_rotation = flip;
    RotMatrixZf(roll * (6.2831853071795864769f / 4096.0f), &hd.screen_rotation);
}

static void release_retired_models(void)
{
    HD_MODEL **cursor = &hd.retired_models;
    if (hd.paused)
        return;
    while (*cursor != NULL)
    {
        HD_MODEL *model = *cursor;
        if (model->draw_refs != 0)
        {
            cursor = &model->next;
            continue;
        }
        *cursor = model->next;
        object_texture_free(model->texture_asset);
        vid_mesh_free(model->geometry);
        vid_index_free(model->indices);
        vid_index_free(model->sky_indices);
        free(model->batch_source_indices);
        free(model->batch_vertices);
        free(model->sky_ranges);
        free(model->ranges);
        free(model->source_groups);
        free(model);
    }
}

static void discard_draws(HD_MODEL_DRAW *draws, uint32 *count)
{
    uint32 index;
    for (index = 0; index < *count; ++index)
    {
        XASSERT(draws[index].model->draw_refs != 0);
        --draws[index].model->draw_refs;
        hd_track_image_release(draws[index].image);
    }
    *count = 0;
}

static VID_MATERIAL *create_ui_material(uint32 pass)
{
    VID_MATERIAL *material = vid_mat_create();
    VID_MATERIAL_DATA *data;
    XASSERT(material != NULL);
    data = vid_mat_data(material);
    memset(data, 0, sizeof(*data));
    data->texture = hd.vram;
    data->shader = hd.ui_shader;
    data->params[0] = pass == 0 ? 1.0f : 2.0f;
    data->params[1] = pass == 4 ? 0.25f : 1.0f;
    data->params[2] = pass == 1 ? 0.5f : 1.0f;
    data->blend = pass == 0 ? VID_BLEND_NONE : pass == 1 ? VID_BLEND_ALPHA : pass == 3 ? VID_BLEND_SUBTRACT : VID_BLEND_ADD;
    vid_mat_commit(material);
    return material;
}

typedef struct HD_TEXTURE_IMAGE
{
    struct HD_TEXTURE_IMAGE *next;
    VID_TEXTURE *texture;
    uint32 width, height, hash, refs, layers;
    VID_MATERIAL *materials[4][HD_BLEND_PASSES];
    VID_MATERIAL_DATA material_data[4][HD_BLEND_PASSES];
} HD_TEXTURE_IMAGE;

static HD_TEXTURE_IMAGE *object_images;

static void object_image_release(HD_TEXTURE_IMAGE *image)
{
    HD_TEXTURE_IMAGE **link;
    if (image == NULL || --image->refs != 0)
        return;
    for (link = &object_images; *link != image; link = &(*link)->next)
    {
    }
    *link = image->next;
    {
        uint32 state, pass;
        for (state = 0; state < 4; ++state)
            for (pass = HD_BLEND_PASSES; pass > 0; --pass)
                vid_mat_free(image->materials[state][pass - 1]);
    }
    vid_tex_free(image->texture);
    free(image);
}

// an object owns one UV layout, regardless of its polygon count
#define HD_ATLAS_GUTTER 8u
#define HD_ATLAS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HD_ATLAS_MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct HD_OBJECT_CHART
{
    uint16 page, clut, u, v, width, height, x, y;
    uint8 *used;
    uint8 coverage, semi;
} HD_OBJECT_CHART;

struct HD_OBJECT_TEXTURE
{
    HD_OBJECT_CHART *charts;
    uint32 count, width, height, version, word_count, zero_opaque, transparent, immutable;
    uint32 *word_indices;
    uint16 *words;
    VID_TEXTURE *source;
    HD_TEXTURE_IMAGE *image;
    VID_MATERIAL *materials[HD_BLEND_PASSES];
    uint32 row_versions[512];
    uint32 row_mask[16];
};

typedef struct HD_DECODE_CACHE
{
    struct HD_DECODE_CACHE *next;
    uint8 *key, *coverage;
    uint32 size, hash, transparent;
    HD_TEXTURE_IMAGE *image;
} HD_DECODE_CACHE;

static HD_DECODE_CACHE *decode_cache[1024];

static HD_DECODE_CACHE *object_decode_cache(HD_OBJECT_TEXTURE *asset)
{
    HD_DECODE_CACHE *entry;
    uint32 size = 5 * sizeof(uint32) + asset->word_count * sizeof(uint16), chart, cursor, hash = 2166136261u;
    uint32 header[5] = {asset->count, asset->width, asset->height, asset->zero_opaque, asset->word_count};
    uint8 *key;
    if (!preparation_active && !asset->immutable)
        return NULL;
    for (chart = 0; chart < asset->count; ++chart)
        size += 8 * sizeof(uint16) + 1 + 8192;
    key = (uint8 *)malloc(size);
    XASSERT(key != NULL);
    memcpy(key, header, sizeof(header));
    cursor = sizeof(header);
    memcpy(key + cursor, asset->words, asset->word_count * sizeof(uint16));
    cursor += asset->word_count * sizeof(uint16);
    for (chart = 0; chart < asset->count; ++chart)
    {
        HD_OBJECT_CHART *part = &asset->charts[chart];
        memcpy(key + cursor, &part->page, 8 * sizeof(uint16));
        cursor += 8 * sizeof(uint16);
        key[cursor++] = part->semi;
        memcpy(key + cursor, part->used, 8192);
        cursor += 8192;
    }
    XASSERT(cursor == size);
    for (cursor = 0; cursor < size; ++cursor)
        hash = (hash ^ key[cursor]) * 16777619u;
    for (entry = decode_cache[hash & 1023]; entry != NULL; entry = entry->next)
        if (entry->hash == hash && entry->size == size && memcmp(entry->key, key, size) == 0)
        {
            free(key);
            return entry;
        }
    entry = (HD_DECODE_CACHE *)calloc(1, sizeof(*entry));
    XASSERT(entry != NULL);
    entry->coverage = (uint8 *)malloc(asset->count);
    XASSERT(entry->coverage != NULL);
    entry->key = key;
    entry->size = size;
    entry->hash = hash;
    entry->next = decode_cache[hash & 1023];
    decode_cache[hash & 1023] = entry;
    return entry;
}

static void object_decode_cache_clear(void)
{
    uint32 bucket;
    for (bucket = 0; bucket < 1024; ++bucket)
        while (decode_cache[bucket] != NULL)
        {
            HD_DECODE_CACHE *entry = decode_cache[bucket];
            decode_cache[bucket] = entry->next;
            object_image_release(entry->image);
            free(entry->key);
            free(entry->coverage);
            free(entry);
        }
}

static uint32 atlas_power(uint32 size)
{
    uint32 result = 1;
    while (result < size)
        result *= 2;
    return result;
}

static HD_OBJECT_TEXTURE *object_texture_layout(VID_VERTEX16 *vertices, uint32 count)
{
    HD_OBJECT_TEXTURE *asset = (HD_OBJECT_TEXTURE *)calloc(1, sizeof(*asset));
    uint32 vertex, chart, area = 0, widest = 1, x = 0, y = 0, row = 0;
    XASSERT(asset != NULL);
    asset->charts = (HD_OBJECT_CHART *)calloc(count, sizeof(*asset->charts));
    XASSERT(asset->charts != NULL);
    for (vertex = 0; vertex < count; ++vertex)
    {
        const VID_VERTEX16 *point = &vertices[vertex];
        uint16 page = point->tex[0] & 0x19F;
        uint16 clut = ((page >> 7) & 3) < 2 ? point->tex[1] : 0;
        uint32 u = point->tex[3] & 255, v = point->tex[3] >> 8;
        for (chart = 0; chart < asset->count; ++chart)
            if (asset->charts[chart].page == page && asset->charts[chart].clut == clut)
                break;
        if (chart == asset->count)
        {
            HD_OBJECT_CHART *part = &asset->charts[asset->count++];
            part->page = page;
            part->clut = clut;
            part->u = (uint16)u;
            part->v = (uint16)v;
            part->width = point->rect[0];
            part->height = point->rect[1];
            part->used = (uint8 *)calloc(8192, 1);
            XASSERT(part->used != NULL);
        }
        {
            HD_OBJECT_CHART *part = &asset->charts[chart];
            uint32 right = HD_ATLAS_MAX((uint32)part->u + part->width, u + point->rect[0]);
            uint32 bottom = HD_ATLAS_MAX((uint32)part->v + part->height, v + point->rect[1]);
            uint32 py;
            part->u = (uint16)HD_ATLAS_MIN(part->u, u);
            part->v = (uint16)HD_ATLAS_MIN(part->v, v);
            part->width = (uint16)(right - part->u);
            part->height = (uint16)(bottom - part->v);
            XASSERT(right <= 256 && bottom <= 256);
            part->semi |= (point->tex[2] & HD_SEMITRANSPARENT) != 0;
            if (vertex != 0 && memcmp(point->tex, vertices[vertex - 1].tex, sizeof(point->tex)) == 0 && memcmp(point->rect, vertices[vertex - 1].rect, sizeof(point->rect)) == 0)
                continue;
            // the source descriptor bounds include all faces using the named texture
            for (py = v; py < bottom && py < v + point->rect[1]; ++py)
            {
                uint32 last = u + point->rect[0] - 1, first_byte = u >> 3, last_byte = last >> 3;
                uint8 *row = part->used + py * 32;
                if (first_byte == last_byte)
                    row[first_byte] |= (uint8)((255u << (u & 7)) & (255u >> (7 - (last & 7))));
                else
                {
                    row[first_byte] |= (uint8)(255u << (u & 7));
                    memset(row + first_byte + 1, 255, last_byte - first_byte - 1);
                    row[last_byte] |= (uint8)(255u >> (7 - (last & 7)));
                }
            }
        }
    }
    for (chart = 0; chart < asset->count; ++chart)
    {
        HD_OBJECT_CHART *part = &asset->charts[chart];
        area += (part->width + 2 * HD_ATLAS_GUTTER) * (part->height + 2 * HD_ATLAS_GUTTER);
        widest = HD_ATLAS_MAX(widest, part->width + 2 * HD_ATLAS_GUTTER);
    }
    asset->width = atlas_power(widest);
    while (asset->width * asset->width < area)
        asset->width *= 2;
    for (chart = 0; chart < asset->count; ++chart)
    {
        HD_OBJECT_CHART *part = &asset->charts[chart];
        uint32 width = part->width + 2 * HD_ATLAS_GUTTER, height = part->height + 2 * HD_ATLAS_GUTTER;
        if (x + width > asset->width)
        {
            y += row;
            x = row = 0;
        }
        part->x = (uint16)(x + HD_ATLAS_GUTTER);
        part->y = (uint16)(y + HD_ATLAS_GUTTER);
        row = HD_ATLAS_MAX(row, height);
        x += width;
    }
    asset->height = atlas_power(y + row);
    if (asset->count == 1)
    {
        asset->width = asset->charts[0].width;
        asset->height = asset->charts[0].height;
        asset->charts[0].x = asset->charts[0].y = 0;
    }
    XASSERT(asset->width <= 8192 && asset->height <= 8192);
    for (vertex = 0; vertex < count; ++vertex)
    {
        VID_VERTEX16 *point = &vertices[vertex];
        uint16 page = point->tex[0] & 0x19F;
        uint16 clut = ((page >> 7) & 3) < 2 ? point->tex[1] : 0;
        for (chart = 0; chart < asset->count; ++chart)
            if (asset->charts[chart].page == page && asset->charts[chart].clut == clut)
                break;
        XASSERT(chart < asset->count);
        point->uv[0] += (sint16)(2 * ((sint32)(point->tex[3] & 255) - asset->charts[chart].u + asset->charts[chart].x));
        point->uv[1] += (sint16)(2 * ((sint32)(point->tex[3] >> 8) - asset->charts[chart].v + asset->charts[chart].y));
        point->rect[0] = (uint16)asset->width;
        point->rect[1] = (uint16)asset->height;
        point->tex[3] = 0;
    }
    // retain only the packed words and palettes actually covered by this object's charts
    for (chart = 0; chart < asset->count; ++chart)
    {
        HD_OBJECT_CHART *part = &asset->charts[chart];
        uint32 depth = (part->page >> 7) & 3, shift = depth == 0 ? 2 : depth == 1 ? 1 : 0;
        asset->word_count += (((part->u + part->width - 1) >> shift) - (part->u >> shift) + 1) * part->height;
        asset->word_count += depth == 0 ? 16 : depth == 1 ? 256 : 0;
    }
    asset->word_indices = (uint32 *)malloc(asset->word_count * sizeof(uint32));
    asset->words = (uint16 *)malloc(asset->word_count * sizeof(uint16));
    XASSERT(asset->word_indices != NULL && asset->words != NULL);
    x = 0;
    for (chart = 0; chart < asset->count; ++chart)
    {
        HD_OBJECT_CHART *part = &asset->charts[chart];
        uint32 depth = (part->page >> 7) & 3, shift = depth == 0 ? 2 : depth == 1 ? 1 : 0, px, py;
        for (py = part->v; py < (uint32)part->v + part->height; ++py)
            for (px = part->u >> shift; px <= ((uint32)part->u + part->width - 1) >> shift; ++px)
                asset->word_indices[x++] = ((((part->page >> 4) & 1) * 256 + py) & 511) * 1024 + (((part->page & 15) * 64 + px) & 1023);
        for (px = 0; px < (depth == 0 ? 16u : depth == 1 ? 256u : 0u); ++px)
            asset->word_indices[x++] = ((part->clut >> 6) & 511) * 1024 + (((part->clut & 63) * 16 + px) & 1023);
    }
    XASSERT(x == asset->word_count);
    for (x = 0; x < asset->word_count; ++x)
    {
        uint32 row = asset->word_indices[x] / 1024;
        asset->row_mask[row >> 5] |= 1u << (row & 31);
    }
    return asset;
}

static void object_texture_free(HD_OBJECT_TEXTURE *asset)
{
    uint32 index;
    if (asset == NULL)
        return;
    for (index = 0; index < asset->count; ++index)
        free(asset->charts[index].used);
    object_image_release(asset->image);
    free(asset->charts);
    free(asset->word_indices);
    free(asset->words);
    free(asset);
}

static VID_MATERIAL *object_texture_material(const HD_MODEL_DRAW *draw, uint32 pass)
{
    HD_OBJECT_TEXTURE *asset = draw->model->texture_asset;
    VID_TEXTURE *source = asset->immutable ? NULL : draw->image != NULL ? draw->image->texture : hd.vram;
    const uint16 *pixels = source != NULL ? (const uint16 *)vid_tex_data(source) : NULL;
    uint32 version = draw->image != NULL ? 1 : hd.vram_version;
    uint32 index, chart;
    sint32 changed = asset->image == NULL;
    XASSERT(asset != NULL);
    if (!asset->immutable && (asset->source != source || asset->version != version || changed))
    {
        uint32 row;
        sint32 relevant = changed || asset->source != source || draw->image != NULL;
        for (row = 0; !relevant && row < 512; ++row)
            if ((asset->row_mask[row >> 5] & (1u << (row & 31))) != 0 && asset->row_versions[row] != vram_row_versions[row])
                relevant = 1;
        for (index = 0; relevant && index < asset->word_count; ++index)
        {
            uint16 word = pixels[asset->word_indices[index]];
            if (changed || asset->words[index] != word)
                changed = 1;
            asset->words[index] = word;
        }
        if (draw->image == NULL)
            memcpy(asset->row_versions, vram_row_versions, sizeof(asset->row_versions));
        asset->source = source;
        asset->version = version;
    }
    if (changed)
    {
        HD_DECODE_CACHE *cached = object_decode_cache(asset);
        if (asset->immutable && !preparation_active && hd.collecting)
            ++hd_work.late_texture_models;
        HD_TEXTURE_IMAGE *image;
        if (cached != NULL && cached->image != NULL)
        {
            image = cached->image;
            asset->transparent = cached->transparent;
            for (chart = 0; chart < asset->count; ++chart)
                asset->charts[chart].coverage = cached->coverage[chart];
        }
        else
        {
            uint32 plane = asset->width * asset->height, bytes = plane * 12, hash = 2166136261u, word_offset = 0;
            uint8 *decoded = (uint8 *)calloc(bytes, 1);
            ++hd_work.atlas_decodes;
            XASSERT(decoded != NULL);
            asset->transparent = 0;
            for (chart = 0; chart < asset->count; ++chart)
            {
                HD_OBJECT_CHART *part = &asset->charts[chart];
                uint32 size = part->width * part->height, head = 0, tail = 0, px, py;
                uint32 depth = (part->page >> 7) & 3, shift = depth == 0 ? 2 : depth == 1 ? 1 : 0;
                uint32 packed_width = (((uint32)part->u + part->width - 1) >> shift) - (part->u >> shift) + 1;
                uint32 palette_offset = word_offset + packed_width * part->height;
                uint32 gutter = asset->count == 1 ? 0 : HD_ATLAS_GUTTER;
                uint32 *nearest = (uint32 *)malloc(size * sizeof(uint32));
                uint32 *queue = (uint32 *)malloc(size * sizeof(uint32));
                XASSERT(nearest != NULL && queue != NULL);
                part->coverage = 0;
                for (py = 0; py < part->height; ++py)
                    for (px = 0; px < part->width; ++px)
                    {
                        index = py * part->width + px;
                        nearest[index] = UINT32_MAX;
                        if ((part->used[((part->v + py) * 256 + part->u + px) >> 3] & (1u << ((part->u + px) & 7))) != 0)
                        {
                            nearest[index] = index;
                            queue[tail++] = index;
                        }
                    }
                // extend each chart into unused texels before building any mip levels
                while (head < tail && tail < size)
                {
                    uint32 at = queue[head++], n;
                    px = at % part->width;
                    py = at / part->width;
                    for (n = 0; n < 4; ++n)
                    {
                        sint32 nx = (sint32)px + (n == 0 ? -1 : n == 1 ? 1 : 0);
                        sint32 ny = (sint32)py + (n == 2 ? -1 : n == 3 ? 1 : 0);
                        uint32 target;
                        if (nx < 0 || ny < 0 || nx >= part->width || ny >= part->height)
                            continue;
                        target = (uint32)ny * part->width + nx;
                        if (nearest[target] == UINT32_MAX)
                        {
                            nearest[target] = nearest[at];
                            queue[tail++] = target;
                        }
                    }
                }
                XASSERT(tail == size);
                for (py = 0; py < part->height + 2 * gutter; ++py)
                    for (px = 0; px < part->width + 2 * gutter; ++px)
                    {
                        uint32 ix = (uint32)HD_ATLAS_MAX(0, HD_ATLAS_MIN((sint32)part->width - 1, (sint32)px - (sint32)gutter));
                        uint32 iy = (uint32)HD_ATLAS_MAX(0, HD_ATLAS_MIN((sint32)part->height - 1, (sint32)py - (sint32)gutter));
                        uint32 sample = nearest[iy * part->width + ix];
                        uint32 u = part->u + sample % part->width, v = part->v + sample / part->width;
                        uint32 layer;
                        uint32 packed = asset->words[word_offset + (v - part->v) * packed_width + (u >> shift) - (part->u >> shift)];
                        uint16 word = (uint16)((packed >> 11) | (((packed >> 6) & 31) << 5) | (((packed >> 1) & 31) << 10) | ((packed & 1) << 15));
                        if (depth < 2)
                        {
                            uint32 palette = depth == 0 ? (word >> ((u & 3) * 4)) & 15 : (word >> ((u & 1) * 8)) & 255;
                            packed = asset->words[palette_offset + palette];
                            word = (uint16)((packed >> 11) | (((packed >> 6) & 31) << 5) | (((packed >> 1) & 31) << 10) | ((packed & 1) << 15));
                        }
                        if ((!asset->zero_opaque && word == 0) || (part->semi && word != 0 && (word & 32768) != 0))
                            asset->transparent = 1;
                        index = (part->y + py - gutter) * asset->width + part->x + px - gutter;
                        for (layer = 0; layer < 3; ++layer)
                        {
                            uint8 *target = decoded + (layer * plane + index) * 4;
                            uint32 axis;
                            if ((word == 0 && !asset->zero_opaque) || (layer == 1 && (word & 32768) != 0) || (layer == 2 && (word & 32768) == 0))
                                continue;
                            for (axis = 0; axis < 3; ++axis)
                            {
                                uint32 component = (word >> (axis * 5)) & 31;
                                target[axis] = (uint8)((component << 3) | (component >> 2));
                            }
                            target[3] = 255;
                            part->coverage |= (uint8)(1u << layer);
                        }
                    }
                word_offset = palette_offset + (depth == 0 ? 16 : depth == 1 ? 256 : 0);
                free(nearest);
                free(queue);
            }
            for (index = 0; index < bytes; ++index)
                hash = (hash ^ decoded[index]) * 16777619u;
            for (image = object_images; image != NULL; image = image->next)
                if (image->layers == 3 && image->width == asset->width && image->height == asset->height && image->hash == hash && memcmp(vid_tex_data(image->texture), decoded, bytes) == 0)
                    break;
            if (image == NULL)
            {
                image = (HD_TEXTURE_IMAGE *)calloc(1, sizeof(*image));
                XASSERT(image != NULL);
                ++hd_work.images_created;
                image->width = asset->width;
                image->height = asset->height;
                image->layers = 3;
                image->hash = hash;
                image->texture = vid_tex_create_array(image->width, image->height, 3);
                XASSERT(image->texture != NULL);
                vid_tex_set_mip_levels(image->texture, 3);
                memcpy(vid_tex_data(image->texture), decoded, bytes);
                vid_tex_commit(image->texture);
                image->next = object_images;
                object_images = image;
            }
            free(decoded);
            if (cached != NULL)
            {
                cached->image = image;
                ++image->refs;
                cached->transparent = asset->transparent;
                for (chart = 0; chart < asset->count; ++chart)
                    cached->coverage[chart] = asset->charts[chart].coverage;
            }
        }
        ++image->refs;
        object_image_release(asset->image);
        asset->image = image;
        if (asset->immutable)
        {
            free(asset->words);
            asset->words = NULL;
            for (chart = 0; chart < asset->count; ++chart)
            {
                free(asset->charts[chart].used);
                asset->charts[chart].used = NULL;
            }
        }
        for (index = 0; index < HD_BLEND_PASSES; ++index)
            if (asset->materials[index] != NULL)
            {
                asset->materials[index] = NULL;
            }
    }
    if (asset->materials[pass] == NULL)
    {
        uint32 state = (draw->model->kind == 12 ? 0 : 2) + (asset->transparent ? 1 : 0);
        VID_MATERIAL_DATA *data;
        if (asset->image->materials[state][pass] != NULL)
        {
            asset->materials[pass] = asset->image->materials[state][pass];
            return asset->materials[pass];
        }
        asset->materials[pass] = create_ui_material(pass);
        asset->image->materials[state][pass] = asset->materials[pass];
        data = vid_mat_data(asset->materials[pass]);
        data->texture = asset->image->texture;
        data->shader = object_texture_shader;
        data->filter = VID_FILTER_ANISOTROPIC;
        data->depth_test = 1;
        data->depth_write = pass == 0;
        data->params[7] = pass == 0 && asset->transparent ? 0.5f : 0.0f;
        data->cull = draw->model->kind == 12 ? 0 : 2;
        data->params[4] = (float)asset->width;
        data->params[5] = (float)asset->height;
        asset->image->material_data[state][pass] = *data;
        vid_mat_commit(asset->materials[pass]);
    }
    return asset->materials[pass];
}

static const VID_MATERIAL_DATA *object_texture_material_data(const HD_MODEL_DRAW *draw, uint32 pass)
{
    HD_OBJECT_TEXTURE *asset = draw->model->texture_asset;
    uint32 state;
    object_texture_material(draw, pass);
    state = (draw->model->kind == 12 ? 0 : 2) + (asset->transparent ? 1 : 0);
    return &asset->image->material_data[state][pass];
}

struct HD_SPRITE_TEXTURE
{
    struct HD_SPRITE_TEXTURE *next;
    HD_TEXTURE key;
    HD_OBJECT_TEXTURE *asset;
    HD_SOURCE_TEXTURE *source_texture;
    HD_SOURCE_REGION source_region;
    VID_MATERIAL *materials[2][HD_BLEND_PASSES];
    VID_TEXTURE *bound[2][HD_BLEND_PASSES];
};

static HD_SPRITE_TEXTURE *sprite_textures;

static HD_SPRITE_TEXTURE *sprite_texture_get(const HD_TEXTURE *texture)
{
    HD_SPRITE_TEXTURE *sprite;
    VID_VERTEX16 vertex = {0};
    HD_TEXTURE key = *texture;
    HD_SOURCE_REGION source_region = {0};
    HD_SOURCE_TEXTURE *source_texture = hd_texture_source_lookup(texture->page, texture->clut, texture->u, texture->v, texture->width, texture->height, &source_region);
    if (source_texture != NULL)
    {
        key.u = (uint16)source_region.u;
        key.v = (uint16)source_region.v;
        key.width = (uint16)source_region.width;
        key.height = (uint16)source_region.height;
    }
    key.page &= 0x19F;
    if (((key.page >> 7) & 3) >= 2)
        key.clut = 0;
    key.flags = (texture->flags & (HD_ZERO_OPAQUE | HD_SHADOW)) != 0 ? HD_ZERO_OPAQUE : 0;
    for (sprite = sprite_textures; sprite != NULL; sprite = sprite->next)
        if (memcmp(&sprite->key, &key, sizeof(key)) == 0)
            return sprite;
    sprite = (HD_SPRITE_TEXTURE *)calloc(1, sizeof(*sprite));
    XASSERT(sprite != NULL);
    sprite->key = key;
    vertex.tex[0] = texture->page;
    vertex.tex[1] = texture->clut;
    vertex.tex[2] = texture->flags;
    vertex.tex[3] = texture->u | (texture->v << 8);
    vertex.rect[0] = texture->width;
    vertex.rect[1] = texture->height;
    sprite->source_texture = source_texture;
    sprite->source_region = source_region;
    if (sprite->source_texture == NULL)
    {
        sprite->asset = object_texture_layout(&vertex, 1);
        sprite->asset->zero_opaque = (texture->flags & (HD_ZERO_OPAQUE | HD_SHADOW)) != 0;
    }
    sprite->next = sprite_textures;
    sprite_textures = sprite;
    return sprite;
}

static VID_MATERIAL *sprite_texture_material(HD_SPRITE_TEXTURE *sprite, uint32 pass, sint32 world)
{
    HD_MODEL model = {0};
    HD_MODEL_DRAW draw = {0};
    VID_MATERIAL_DATA *data;
    if (sprite->source_texture != NULL)
    {
        HD_SOURCE_GPU image = hd_texture_source_gpu(sprite->source_texture);
        if (sprite->materials[world][pass] == NULL)
        {
            sprite->materials[world][pass] = create_ui_material(pass);
            data = vid_mat_data(sprite->materials[world][pass]);
            data->texture = image.texture;
            data->shader = world ? object_texture_shader : sprite_texture_shader;
            data->filter = VID_FILTER_LINEAR;
            data->params[4] = (float)image.width;
            data->params[5] = (float)image.height;
            data->cull = 0;
            data->depth_test = world;
            data->depth_write = 0;
            if (pass == 0)
                data->blend = VID_BLEND_ALPHA;
            vid_mat_commit(sprite->materials[world][pass]);
        }
        return sprite->materials[world][pass];
    }
    model.texture_asset = sprite->asset;
    draw.model = &model;
    object_texture_material(&draw, pass);
    if (sprite->materials[world][pass] == NULL || sprite->bound[world][pass] != sprite->asset->image->texture)
    {
        vid_mat_free(sprite->materials[world][pass]);
        sprite->materials[world][pass] = create_ui_material(pass);
        data = vid_mat_data(sprite->materials[world][pass]);
        *data = *object_texture_material_data(&draw, pass);
        data->shader = world ? object_texture_shader : sprite_texture_shader;
        data->filter = VID_FILTER_LINEAR;
        data->params[7] = 0.0f;
        data->cull = 0;
        data->depth_test = world;
        data->depth_write = 0;
        if (pass == 0)
            data->blend = VID_BLEND_ALPHA;
        sprite->bound[world][pass] = data->texture;
        vid_mat_commit(sprite->materials[world][pass]);
    }
    return sprite->materials[world][pass];
}

static sint32 object_texture_has_transparency(const HD_MODEL_DRAW *draw, const HD_TEXTURE *texture)
{
    const HD_OBJECT_TEXTURE *asset = draw->model->texture_asset;
    uint32 chart, page = texture->page & 0x19F;
    uint32 clut = ((page >> 7) & 3) < 2 ? texture->clut : 0;
    for (chart = 0; chart < asset->count; ++chart)
        if (asset->charts[chart].page == page && asset->charts[chart].clut == clut)
            return (asset->charts[chart].coverage & 4) != 0;
    XASSERT(0);
    return 0;
}

static void sprite_textures_free(void)
{
    while (sprite_textures != NULL)
    {
        HD_SPRITE_TEXTURE *sprite = sprite_textures;
        uint32 pass, world;
        sprite_textures = sprite->next;
        for (world = 0; world < 2; ++world)
            for (pass = 0; pass < HD_BLEND_PASSES; ++pass)
                vid_mat_free(sprite->materials[world][pass]);
        object_texture_free(sprite->asset);
        free(sprite);
    }
}

HD_TRACK_IMAGE *hd_track_image_create(const uint16 *source)
{
    HD_TRACK_IMAGE *image = (HD_TRACK_IMAGE *)calloc(1, sizeof(*image));
    uint16 *target;
    uint32 pixel;
    XASSERT(image != NULL);
    image->refs = 1;
    image->texture = vid_tex_create_fmt(1024, 512, VID_RGBA5551);
    XASSERT(image->texture != NULL);
    vid_tex_set_mip_levels(image->texture, 3);
    target = (uint16 *)vid_tex_data(image->texture);
    for (pixel = 0; pixel < 1024 * 512; ++pixel)
    {
        uint32 word = source[pixel];
        target[pixel] = (uint16)(((word & 31) << 11) | (((word >> 5) & 31) << 6) | (((word >> 10) & 31) << 1) | (word >> 15));
    }
    vid_tex_commit(image->texture);
    return image;
}

void hd_track_image_release(HD_TRACK_IMAGE *image)
{
    if (image == NULL)
        return;
    XASSERT(image->refs != 0);
    if (--image->refs != 0)
        return;
    vid_tex_free(image->texture);
    free(image);
}

void hd_track_image_bind(HD_TRACK_IMAGE *image)
{
    track_image = image;
}

sint32 hd_init(void)
{
    uint32 pass;
    if (hd.initialized)
        return 1;
    if (xport_is_headless())
        return 0;
    hd.ui_shader = vid_shader_create(ui_vertex_source, texture_fragment_source);
    XASSERT(hd.ui_shader != NULL);
    if (hd.ui_shader == NULL)
        return 0;
    hd.vram = vid_tex_create_fmt(GPU_VRAM_WIDTH, GPU_VRAM_HEIGHT, VID_RGBA5551);
    XASSERT(hd.vram != NULL);
    vid_tex_set_mip_levels(hd.vram, 3);
    hd.presented_phase = 1.0f;
    for (pass = 0; pass < HD_BLEND_PASSES; ++pass)
        hd.ui_materials[pass] = create_ui_material(pass);
    hd.world_shader = vid_shader_create(world_vertex_source, texture_fragment_source);
    XASSERT(hd.world_shader != NULL);
    {
        char source[sizeof(world_vertex_source) + 32];
        char fragment[sizeof(scene_batch_fragment_source) + 32];
        strcpy(fragment, "#define TD5_HEADLIGHTS\n");
        strcat(fragment, scene_batch_fragment_source);
        strcpy(source, "#define TD5_SCENE_BATCH\n");
        strcat(source, world_vertex_source);
        scene_batch_shader = vid_shader_create_array(source, fragment);
        object_texture_shader = vid_shader_create_array(world_vertex_source, fragment);
        XASSERT(object_texture_shader != NULL);
        sprite_texture_shader = vid_shader_create_array(ui_vertex_source, scene_batch_fragment_source);
        XASSERT(sprite_texture_shader != NULL);
        XASSERT(scene_batch_shader != NULL);
    }
    hd.police_shader = vid_shader_create(police_vertex_source, texture_fragment_source);
    XASSERT(hd.police_shader != NULL);
    hd.map_shader = vid_shader_create(map_vertex_source, map_fragment_source);
    XASSERT(hd.map_shader != NULL);
    for (pass = 0; pass < HD_BLEND_PASSES; ++pass)
    {
        VID_MATERIAL_DATA *data;
        hd.world_materials[pass] = create_ui_material(pass);
        data = vid_mat_data(hd.world_materials[pass]);
        data->shader = hd.world_shader;
        data->depth_test = 1;
        data->depth_write = pass == 0;
        data->cull = 2;
        vid_mat_commit(hd.world_materials[pass]);
        hd.sky_materials[pass] = create_ui_material(pass);
        data = vid_mat_data(hd.sky_materials[pass]);
        data->shader = hd.world_shader;
        data->depth_test = 0;
        data->depth_write = 0;
        data->cull = 0;
        vid_mat_commit(hd.sky_materials[pass]);
        hd.effect_materials[pass] = create_ui_material(pass);
        data = vid_mat_data(hd.effect_materials[pass]);
        data->shader = hd.world_shader;
        data->depth_test = 1;
        data->depth_write = 0;
        data->cull = 0;
        vid_mat_commit(hd.effect_materials[pass]);
        hd.retire_materials[pass] = create_ui_material(pass);
        data = vid_mat_data(hd.retire_materials[pass]);
        data->shader = hd.world_shader;
        data->depth_test = 1;
        data->depth_write = 0;
        data->cull = 0;
        if (pass == 0)
            data->blend = VID_BLEND_ALPHA;
        vid_mat_commit(hd.retire_materials[pass]);
        hd.ui_retire_materials[pass] = create_ui_material(pass);
        data = vid_mat_data(hd.ui_retire_materials[pass]);
        if (pass == 0)
            data->blend = VID_BLEND_ALPHA;
        vid_mat_commit(hd.ui_retire_materials[pass]);
        hd.solid_materials[pass] = create_ui_material(pass);
        data = vid_mat_data(hd.solid_materials[pass]);
        data->shader = hd.world_shader;
        data->depth_test = 1;
        data->depth_write = pass == 0;
        data->cull = 0;
        vid_mat_commit(hd.solid_materials[pass]);
        hd.environment_materials[pass] = create_ui_material(pass);
        data = vid_mat_data(hd.environment_materials[pass]);
        data->shader = hd.world_shader;
        data->depth_test = 1;
        data->depth_write = 0;
        data->cull = 2;
        vid_mat_commit(hd.environment_materials[pass]);
        hd.police_materials[pass] = create_ui_material(pass);
        data = vid_mat_data(hd.police_materials[pass]);
        data->shader = hd.police_shader;
        data->depth_test = 1;
        data->depth_write = 0;
        data->cull = 0;
        vid_mat_commit(hd.police_materials[pass]);
        hd.map_materials[pass] = create_ui_material(pass);
        data = vid_mat_data(hd.map_materials[pass]);
        data->shader = hd.map_shader;
        vid_mat_commit(hd.map_materials[pass]);
    }
    hd.initialized = 1;
    for (pass = 0; pass < HD_VIEWS; ++pass)
    {
        hd.native_lists[pass] = gpu_render_list_create();
        XASSERT(hd.native_lists[pass] != NULL);
    }
    return 1;
}

static void upload_vram(void)
{
    uint32 version = gpu_vram_version(), row;
    uint16 *destination;
    if (hd.vram_valid && version == hd.vram_version)
        return;
    destination = (uint16 *)vid_tex_data(hd.vram);
    for (row = 0; row < 512; ++row)
    {
        uint32 x, offset = row * 1024;
        if (hd.vram_valid && memcmp(vram_snapshot + offset, VRAM + offset, 1024 * sizeof(uint16)) == 0)
            continue;
        memcpy(vram_snapshot + offset, VRAM + offset, 1024 * sizeof(uint16));
        for (x = 0; x < 1024; ++x)
        {
            uint32 word = VRAM[offset + x];
            destination[offset + x] = (uint16)(((word & 31) << 11) | (((word >> 5) & 31) << 6) | (((word >> 10) & 31) << 1) | (word >> 15));
        }
        ++vram_row_versions[row];
        ++hd_work.vram_rows;
        vid_tex_commit_rows(hd.vram, row, 1);
    }
    hd.vram_version = version;
    hd.vram_valid = 1;
}

sint32 hd_native_capture_required(void)
{
    return hd.capture_native || !game_hd_frame;
}

void hd_original_track_capture(sint32 enabled)
{
    original_track_suppressed = !enabled;
}

void hd_track_image_seal(HD_TRACK_IMAGE *image)
{
    if (image != NULL)
    {
        vid_tex_free(image->texture);
        image->texture = NULL;
    }
}

void hd_suspend(void)
{
    if (hd.collecting)
    {
        if (game_hd_frame && hd.frame_started)
            vid_cancel();
        else if (!game_hd_frame)
            gpu_frame_end();
    }
    hd.collecting = 0;
    hd.frame_started = 0;
    game_hd_frame = 0;
    ++hd.motion_generation;
    discard_draws(hd.completed.model_draws, &hd.completed.model_count);
    hd.completed_valid = 0;
    hd.race_ready = 0;
    hd.frame_ready = 0;
    gpu_set_raster(!loading_image_active);
    release_retired_models();
}

void hd_reset(void)
{
    hd_suspend();
    scene_batches_free();
    static_pools_free();
    source_materials_free();
    track_image = NULL;
    object_decode_cache_clear();
    hd_track_reset();
    hd_pause_end();
    discard_draws(hd.model_draws, &hd.model_count);
    discard_draws(hd.frozen.model_draws, &hd.frozen.model_count);
    hd_forget_span(NULL, 0);
    release_retired_models();
    hd.ui_count = 0;
    sprite_textures_free();
    hd.model_count = 0;
    hd.ui_anchor = 0;
    hd.vram_valid = 0;
}

void hd_free(void)
{
    HD_UI_MESH *mesh;
    HD_WORLD_MESH *world;
    uint32 pass;
    hd_reset();
    while ((mesh = hd.ui_meshes) != NULL)
    {
        hd.ui_meshes = mesh->next;
        vid_mesh_free(mesh->geometry);
        vid_index_free(mesh->indices);
        free(mesh);
    }
    for (pass = 0; pass < HD_BLEND_PASSES; ++pass)
    {
        vid_mat_free(hd.ui_materials[pass]);
        vid_mat_free(hd.world_materials[pass]);
        vid_mat_free(hd.sky_materials[pass]);
        vid_mat_free(hd.effect_materials[pass]);
        vid_mat_free(hd.retire_materials[pass]);
        vid_mat_free(hd.ui_retire_materials[pass]);
        vid_mat_free(hd.solid_materials[pass]);
        vid_mat_free(hd.environment_materials[pass]);
        vid_mat_free(hd.police_materials[pass]);
        vid_mat_free(hd.map_materials[pass]);
    }
    for (pass = 0; pass < HD_VIEWS; ++pass)
        gpu_render_list_free(hd.native_lists[pass]);
    while ((world = hd.world_meshes) != NULL)
    {
        hd.world_meshes = world->next;
        vid_mesh_free(world->model.geometry);
        vid_index_free(world->model.indices);
        free(world);
    }
    vid_tex_free(hd.vram);
    vid_shader_free(hd.ui_shader);
    vid_shader_free(sprite_texture_shader);
    sprite_texture_shader = NULL;
    vid_shader_free(object_texture_shader);
    object_texture_shader = NULL;
    vid_shader_free(scene_batch_shader);
    scene_batch_shader = NULL;
    vid_shader_free(hd.world_shader);
    vid_shader_free(hd.police_shader);
    vid_shader_free(hd.map_shader);
    vid_mesh_free(hd.police_model.geometry);
    vid_index_free(hd.police_model.indices);
    free(hd.ui_draws);
    free(hd.model_draws);
    while ((mesh = hd.frozen.ui_meshes) != NULL)
    {
        hd.frozen.ui_meshes = mesh->next;
        vid_mesh_free(mesh->geometry);
        vid_index_free(mesh->indices);
        free(mesh);
    }
    while ((world = hd.frozen.world_meshes) != NULL)
    {
        hd.frozen.world_meshes = world->next;
        vid_mesh_free(world->model.geometry);
        vid_index_free(world->model.indices);
        free(world);
    }
    free(hd.frozen.ui_draws);
    free(hd.frozen.model_draws);
    while ((mesh = hd.completed.ui_meshes) != NULL)
    {
        hd.completed.ui_meshes = mesh->next;
        vid_mesh_free(mesh->geometry);
        vid_index_free(mesh->indices);
        free(mesh);
    }
    while ((world = hd.completed.world_meshes) != NULL)
    {
        hd.completed.world_meshes = world->next;
        vid_mesh_free(world->model.geometry);
        vid_index_free(world->model.indices);
        free(world);
    }
    free(hd.completed.ui_draws);
    free(hd.completed.model_draws);
    for (pass = 0; pass < HD_VIEWS; ++pass)
    {
        uint32 slot;
        for (slot = 0; slot < 50; ++slot)
        {
            vid_mesh_free(hd.lines[pass][slot].model.geometry);
            vid_index_free(hd.lines[pass][slot].model.indices);
            vid_mesh_free(hd.retired_effects[pass][slot].line_model.geometry);
            vid_index_free(hd.retired_effects[pass][slot].line_model.indices);
        }
        for (slot = 0; slot < 80; ++slot)
        {
            uint32 model;
            for (model = 0; model < 2; ++model)
            {
                vid_mesh_free(hd.skids[pass][slot].model[model].geometry);
                vid_index_free(hd.skids[pass][slot].model[model].indices);
            }
        }
        for (slot = 0; slot < 14; ++slot)
        {
            uint32 part;
            for (part = 0; part < 2; ++part)
            {
                vid_mesh_free(hd.shadows[pass][slot][part].model.geometry);
                vid_index_free(hd.shadows[pass][slot][part].model.indices);
            }
        }
    }
    memset(&hd, 0, sizeof(hd));
}

static void begin_frame(uint32 views, float r, float g, float b, sint32 race)
{
    HD_UI_MESH *mesh;
    HD_WORLD_MESH *world;
    XASSERT(!hd.collecting && views >= 1 && views <= HD_VIEWS);
    loading_image_active = 0;
    if (!race || !xport_hd || hd.paused)
    {
        discard_draws(hd.completed.model_draws, &hd.completed.model_count);
        hd.completed_valid = 0;
    }
    else if (hd.race_ready)
    {
        swap_scene_buffers(&hd.completed);
        hd.completed_valid = 1;
    }
    hd.race_ready = 0;
    hd.frame_ready = 0;
    hd.race_frame = race;
    hd.render_phase = 1.0f;
    game_hd_frame = hd.initialized && xport_hd != 0;
    if (hd.last_mode != game_hd_frame)
        ++hd.motion_generation;
    ++hd.motion_serial;
    hd.actor = -1;
    hd.wheel = NULL;
    hd.fresh_og = hd.last_mode && !game_hd_frame;
    hd.last_mode = game_hd_frame;
    hd.collecting = 1;
    hd.views = views;
    hd.view = 0;
    hd.race_pose_views = 0;
    hd.ui_anchor = 0;
    hd.ui_count = 0;
    discard_draws(hd.model_draws, &hd.model_count);
    release_retired_models();
    hd.prepared = 0;
    hd.capture_native = 0;
    hd.capture_sources = game_hd_frame;
    hd.frame_started = 0;
    hd.clear[0] = r;
    hd.clear[1] = g;
    hd.clear[2] = b;
    memset(hd.native_valid, 0, sizeof(hd.native_valid));
    hd.ui_cursor = hd.ui_meshes;
    for (world = hd.world_meshes; world != NULL; world = world->next)
        world->used = 0;
    for (mesh = hd.ui_meshes; mesh != NULL; mesh = mesh->next)
    {
        mesh->used = 0;
        mesh->indexed = 0;
    }
    gpu_set_raster(!game_hd_frame);
    if (hd.initialized)
        vid_resize(&hd.width, &hd.height);
    if (game_hd_frame)
    {
        hd.frame_started = vid_begin(r, g, b);
    }
    else
        gpu_frame_begin();
}

void hd_begin_frame(uint32 views)
{
    if (hd.paused)
        begin_frame(views, hd.frozen.clear[0], hd.frozen.clear[1], hd.frozen.clear[2], 0);
    else
        begin_frame(views, 0.0f, 0.0f, 0.0f, 0);
}

void hd_begin_race_frame(uint32 views)
{
    const DRAWENV *draw = &disp_state.frames[0].draw;
    begin_frame(views, draw->r0 / 255.0f, draw->g0 / 255.0f, draw->b0 / 255.0f, 1);
}

void hd_begin_view(uint32 view, VID_RECT reference)
{
    HD_VIEW *target;
    XASSERT(hd.collecting && view < hd.views && reference.w > 0 && reference.h > 0);
    hd.view = view;
    hd.actor = -1;
    target = &hd.viewports[view];
    target->reference = reference;
    target->reference_aspect = (4.0f / 3.0f) * hd.views;
    target->viewport.x = 0;
    target->viewport.y = (sint32)((sint64)hd.height * view / hd.views);
    target->viewport.w = hd.width;
    target->viewport.h = (sint32)((sint64)hd.height * (view + 1) / hd.views) - target->viewport.y;
}

static HD_UI_DRAW *reserve_ui(uint32 count, uint32 blend)
{
    HD_UI_MESH **cursor = &hd.ui_meshes;
    HD_UI_MESH *mesh;
    HD_UI_DRAW *draw;
    while (*cursor != NULL && (*cursor)->used + count > HD_UI_MESH_VERTICES)
        cursor = &(*cursor)->next;
    mesh = *cursor;
    if (mesh == NULL)
    {
        mesh = (HD_UI_MESH *)calloc(1, sizeof(*mesh));
        XASSERT(mesh != NULL);
        mesh->geometry = vid_mesh_create_2d(HD_UI_MESH_VERTICES);
        mesh->indices = vid_index_create(HD_UI_MESH_VERTICES);
        XASSERT(mesh->geometry != NULL && mesh->indices != NULL);
        *cursor = mesh;
    }
    if (hd.ui_count == hd.ui_capacity)
    {
        uint32 capacity = hd.ui_capacity != 0 ? hd.ui_capacity * 2 : 64;
        HD_UI_DRAW *draws = (HD_UI_DRAW *)realloc(hd.ui_draws, (size_t)capacity * sizeof(*draws));
        XASSERT(draws != NULL);
        hd.ui_draws = draws;
        hd.ui_capacity = capacity;
    }
    draw = &hd.ui_draws[hd.ui_count++];
    memset(draw, 0, sizeof(*draw));
    draw->mesh = mesh;
    draw->view = hd.view;
    draw->blend = blend;
    draw->layer = hd.ui_layer;
    draw->sequence = hd.ui_count;
    draw->first = mesh->used;
    draw->count = count;
    return draw;
}

static void ui_polygon(const HD_UI_VERTEX *points, uint32 count, const HD_TEXTURE *texture, float alpha)
{
    static const uint8 triangles[6] = {0, 1, 2, 1, 3, 2};
    HD_UI_VERTEX clipped[16], scratch[16];
    sint32 fan = 0;
    HD_UI_DRAW *draw;
    VID_VERTEX2D *vertices;
    uint32 vertex, source, blend, output_count;
    if (!hd_capture_active() || !hd.collecting)
        return;
    if (hd.ui_clipped)
    {
        uint32 point, side;
        float edges[4] = {(float)hd.ui_clip.x, (float)(hd.ui_clip.x + hd.ui_clip.w), (float)hd.ui_clip.y, (float)(hd.ui_clip.y + hd.ui_clip.h)};
        for (point = 0; point < count; ++point)
        {
            uint32 perimeter = count == 4 && point >= 2 ? 5 - point : point;
            clipped[point] = points[perimeter];
            clipped[point].x += hd.ui_offset[0];
            clipped[point].y += hd.ui_offset[1];
        }
        for (side = 0; side < 4 && count != 0; ++side)
        {
            uint32 output = 0;
            HD_UI_VERTEX previous = clipped[count - 1];
            float previous_distance = (side < 2 ? previous.x : previous.y) - edges[side];
            if ((side & 1) != 0)
                previous_distance = -previous_distance;
            for (point = 0; point < count; ++point)
            {
                HD_UI_VERTEX current = clipped[point];
                float distance = (side < 2 ? current.x : current.y) - edges[side];
                if ((side & 1) != 0)
                    distance = -distance;
                if ((distance >= 0.0f) != (previous_distance >= 0.0f))
                {
                    float phase = previous_distance / (previous_distance - distance);
                    HD_UI_VERTEX *intersection = &scratch[output++];
                    uint32 component;
                    intersection->x = previous.x + phase * (current.x - previous.x);
                    intersection->y = previous.y + phase * (current.y - previous.y);
                    intersection->u = previous.u + phase * (current.u - previous.u);
                    intersection->v = previous.v + phase * (current.v - previous.v);
                    for (component = 0; component < 4; ++component)
                        intersection->color[component] = (uint8)(previous.color[component] + phase * (current.color[component] - previous.color[component]));
                }
                if (distance >= 0.0f)
                    scratch[output++] = current;
                XASSERT(output <= 16);
                previous = current;
                previous_distance = distance;
            }
            memcpy(clipped, scratch, output * sizeof(*clipped));
            count = output;
        }
        if (count < 3)
            return;
        points = clipped;
        fan = 1;
    }
    blend = (texture->flags & HD_SEMITRANSPARENT) != 0 ? 1 + ((texture->page >> 5) & 3) : 0;
    output_count = fan ? (count - 2) * 3 : count == 4 ? 6 : 3;
    draw = reserve_ui(output_count, blend);
    if ((texture->flags & HD_SCENE_TEXTURE) != 0 && (texture->flags & HD_TEXTURED) != 0 && (texture->flags & HD_ZERO_OPAQUE) == 0)
        draw->sprite = sprite_texture_get(texture);
    vertices = vid_mesh_data_2d(draw->mesh->geometry) + draw->mesh->used;
    for (vertex = 0; vertex < output_count; ++vertex)
    {
        VID_VERTEX2D *destination = &vertices[vertex];
        source = fan ? (vertex % 3 == 0 ? 0 : vertex / 3 + vertex % 3) : count == 4 ? triangles[vertex] : vertex;
        destination->pos[0] = points[source].x;
        destination->pos[1] = points[source].y;
        destination->pos[2] = (float)hd.ui_anchor;
        destination->pos[3] = 1.0f;
        destination->uv[0] = points[source].u - texture->u;
        destination->uv[1] = points[source].v - texture->v;
        destination->alpha = alpha;
        memcpy(destination->color, points[source].color, sizeof(destination->color));
        destination->tex[0] = texture->page;
        destination->tex[1] = texture->clut;
        // screen-projected scene effects share this queue with unfiltered UI
        destination->tex[2] = (texture->flags & HD_SCENE_TEXTURE) != 0 ? texture->flags | HD_FILTERED : texture->flags & ~HD_FILTERED;
        destination->tex[3] = texture->u | (texture->v << 8);
        destination->rect[0] = texture->width;
        destination->rect[1] = texture->height;
        if (draw->sprite != NULL)
        {
            if (draw->sprite->source_texture != NULL)
            {
                HD_SOURCE_GPU image = hd_texture_source_gpu(draw->sprite->source_texture);
                destination->uv[0] += texture->u - draw->sprite->source_region.u;
                destination->uv[1] += texture->v - draw->sprite->source_region.v;
                destination->tex[0] = (uint16)image.opaque_layer;
                destination->tex[1] = (uint16)image.stp_layer;
                destination->tex[2] |= 1024;
                destination->tex[3] = (uint16)image.mode;
                destination->rect[0] = (uint16)image.width;
                destination->rect[1] = (uint16)image.height;
            }
            else
            {
                const HD_OBJECT_TEXTURE *asset = draw->sprite->asset;
                destination->uv[0] += asset->charts[0].x;
                destination->uv[1] += asset->charts[0].y;
                destination->rect[0] = (uint16)asset->width;
                destination->rect[1] = (uint16)asset->height;
            }
        }
    }
    draw->mesh->used += output_count;
}

void hd_ui_motion(uint32 first, float previous_x, float previous_y, float current_x, float current_y)
{
    uint32 index;
    for (index = first; index < hd.ui_count; ++index)
    {
        HD_UI_DRAW *draw = &hd.ui_draws[index];
        draw->particle[0].position = (VECTORF){previous_x, previous_y, 0.0f};
        draw->particle[1].position = (VECTORF){current_x, current_y, 0.0f};
        draw->particle[0].size = draw->particle[1].size = 1.0f;
        draw->particle[0].brightness = draw->particle[1].brightness = -1.0f;
        draw->particle_active = 1;
    }
}

void hd_ui_region(VID_RECT clip, float offset_x, float offset_y)
{
    XASSERT(!hd.ui_clipped);
    hd.ui_clipped = 1;
    hd.ui_clip = clip;
    hd.ui_offset[0] = offset_x;
    hd.ui_offset[1] = offset_y;
}

void hd_ui_region_end(void)
{
    hd.ui_clipped = 0;
}

void hd_ui_anchor(sint32 side)
{
    XASSERT(side >= -1 && side <= 1);
    hd.ui_anchor = side;
}

void hd_ui_quad(const HD_UI_VERTEX points[4], const HD_TEXTURE *texture, float alpha)
{
    ui_polygon(points, 4, texture, alpha);
}

void hd_ui_ordered_quad(const HD_UI_VERTEX points[4], const HD_TEXTURE *texture, float alpha, uint32 layer)
{
    hd.ui_layer = layer;
    hd_ui_quad(points, texture, alpha);
    hd.ui_layer = 0;
}

void hd_ui_triangle(const HD_UI_VERTEX points[3], const HD_TEXTURE *texture, float alpha)
{
    ui_polygon(points, 3, texture, alpha);
}

void hd_ui_gouraud(const POINT_CT points[4], uint32 layer)
{
    HD_UI_VERTEX vertices[4];
    HD_TEXTURE texture = {points[1].attr, points[0].attr, 0, 0, 256, 256, HD_TEXTURED};
    uint32 point;
    if (!hd_capture_active())
        return;
    if ((points[0].rgbc.code & 1) != 0)
        texture.flags |= HD_RAW_TEXTURE;
    if ((points[0].rgbc.code & 2) != 0)
        texture.flags |= HD_SEMITRANSPARENT;
    for (point = 0; point < 4; ++point)
    {
        vertices[point].x = (float)points[point].xy.x;
        vertices[point].y = (float)points[point].xy.y;
        vertices[point].u = (float)points[point].uv.u;
        vertices[point].v = (float)points[point].uv.v;
        memcpy(vertices[point].color, &points[point].rgbc, 4);
    }
    hd.ui_layer = layer;
    hd_ui_quad(vertices, &texture, 1.0f);
    hd.ui_layer = 0;
}

void hd_ui_flat(const POINT_T points[4], const RGBC *color, uint32 layer)
{
    POINT_CT vertices[4];
    uint32 point;
    for (point = 0; point < 4; ++point)
    {
        vertices[point].xy = points[point].xy;
        vertices[point].uv = points[point].uv;
        vertices[point].attr = points[point].attr;
        vertices[point].rgbc = *color;
    }
    hd_ui_gouraud(vertices, layer);
}

static int compare_ui(const void *left, const void *right)
{
    const HD_UI_DRAW *a = (const HD_UI_DRAW *)left, *b = (const HD_UI_DRAW *)right;
    if (a->layer != b->layer)
        return a->layer > b->layer ? -1 : 1;
    if (a->view != b->view)
        return a->view < b->view ? -1 : 1;
    return a->sequence > b->sequence ? -1 : a->sequence < b->sequence;
}

void hd_ui_solid(const XY *points, const RGBC *colors, uint32 count, uint32 layer)
{
    HD_UI_VERTEX vertices[4];
    HD_TEXTURE texture = {0};
    uint32 point;
    if (!hd_capture_active())
        return;
    XASSERT(count == 3 || count == 4);
    if ((colors[0].code & 2) != 0)
        texture.flags = HD_SEMITRANSPARENT;
    for (point = 0; point < count; ++point)
    {
        vertices[point].x = (float)points[point].x;
        vertices[point].y = (float)points[point].y;
        vertices[point].u = vertices[point].v = 0.0f;
        memcpy(vertices[point].color, colors + point, 4);
    }
    hd.ui_layer = layer;
    ui_polygon(vertices, count, &texture, 1.0f);
    hd.ui_layer = 0;
}

void hd_menu_background(uint32 bank, uint32 height)
{
    uint32 strip, point;
    if (!hd_capture_active())
        return;
    for (strip = 0; strip < 2; ++strip)
    {
        HD_TEXTURE texture = {(uint16)(0x100 | (bank << 4) | (strip * 4)), 0, 0, 0, 256, 256, HD_TEXTURED | HD_RAW_TEXTURE | HD_ZERO_OPAQUE | HD_SCENE_TEXTURE};
        HD_UI_VERTEX vertices[4];
        for (point = 0; point < 4; ++point)
        {
            vertices[point].x = (float)(strip * 256 + ((point & 1) != 0 ? 256 : 0));
            vertices[point].y = (float)((point & 2) != 0 ? height : 0) - (disp_state.cur->draw.ofs[1] & 255);
            vertices[point].u = (float)((point & 1) != 0 ? 256 : 0);
            vertices[point].v = (float)((point & 2) != 0 ? height : 0);
            memset(vertices[point].color, 128, 4);
        }
        hd.ui_layer = 0xFFFFFFFFu;
        hd_ui_quad(vertices, &texture, 1.0f);
    }
    hd.ui_layer = 0;
}

sint32 hd_loading_screen(void)
{
    uint32 strip, point;
    if (!hd.initialized || !xport_hd)
        return 0;
    hd_begin_frame(1);
    hd_begin_view(0, (VID_RECT){0, 0, 320, 256});
    for (strip = 0; strip < 2; ++strip)
    {
        uint16 width = strip == 0 ? 256 : 64;
        HD_TEXTURE texture = {(uint16)(0x100 | strip * 4), 0, 0, 0, width, 256, HD_TEXTURED | HD_RAW_TEXTURE | HD_ZERO_OPAQUE | HD_FILTERED | HD_SCENE_TEXTURE};
        HD_UI_VERTEX vertices[4];
        for (point = 0; point < 4; ++point)
        {
            vertices[point].x = (float)(strip * 256 + ((point & 1) != 0 ? width : 0));
            vertices[point].y = (float)((point & 2) != 0 ? 256 : 0);
            vertices[point].u = (float)((point & 1) != 0 ? width : 0);
            vertices[point].v = (float)((point & 2) != 0 ? 256 : 0);
            memset(vertices[point].color, 128, 4);
        }
        hd_ui_quad(vertices, &texture, 1.0f);
    }
    hd_end_frame();
    hd_present_frame(0, 1);
    hd_suspend();
    // scene reset must not replace the filtered image with software scanout
    loading_image_active = 1;
    gpu_set_raster(0);
    return 1;
}

void hd_menu_view(void)
{
    const DISPLAY_FRAME *frame = disp_state.cur;
    const DISPENV *display = &disp_state.frames[disp_state.bank ^ 1].disp;
    VID_RECT reference = {display->disp.x - frame->draw.ofs[0], display->disp.y - frame->draw.ofs[1], display->disp.w, display->disp.h};
    hd_begin_view(0, reference);
    hd.native_environments[0] = frame->draw;
}

void hd_race_view(uint32 view, const DRAWENV *draw)
{
    HD_VIEW *target;
    VID_RECT reference = {draw->clip.x - draw->ofs[0], draw->clip.y - draw->ofs[1], draw->clip.w, draw->clip.h};
    hd_begin_view(view, reference);
    hd.native_environments[view] = *draw;
    target = &hd.viewports[view];
    target->reference_aspect = (4.0f / 3.0f) * reference.w / model_render.width * model_render.height / reference.h;
    target->viewport.x = (sint32)((sint64)hd.width * reference.x / model_render.width);
    target->viewport.y = (sint32)((sint64)hd.height * reference.y / model_render.height);
    target->viewport.w = (sint32)((sint64)hd.width * (reference.x + reference.w) / model_render.width) - target->viewport.x;
    target->viewport.h = (sint32)((sint64)hd.height * (reference.y + reference.h) / model_render.height) - target->viewport.y;
}

void hd_ui_color_quad(const XY points[4], const RGBC *color, uint32 layer)
{
    RGBC colors[4] = {*color, *color, *color, *color};
    hd_ui_solid(points, colors, 4, layer);
}

void hd_ui_gradient(const POINT_C points[4], uint32 layer)
{
    XY positions[4];
    RGBC colors[4];
    uint32 point;
    for (point = 0; point < 4; ++point)
    {
        positions[point] = points[point].xy;
        colors[point] = points[point].rgbc;
    }
    hd_ui_solid(positions, colors, 4, layer);
}

void hd_ui_color_triangle(const XY points[3], const RGBC *color, uint32 layer)
{
    RGBC colors[3] = {*color, *color, *color};
    hd_ui_solid(points, colors, 3, layer);
}

void hd_ui_segment(const HD_UI_VERTEX points[2], uint16 page, sint32 semi, uint32 layer)
{
    HD_UI_VERTEX vertices[4];
    HD_TEXTURE texture = {0};
    float dx = points[1].x - points[0].x, dy = points[1].y - points[0].y;
    float length = sqrtf(dx * dx + dy * dy);
    uint32 point;
    if (!hd_capture_active() || length == 0.0f)
        return;
    texture.page = page;
    if (semi)
        texture.flags = HD_SEMITRANSPARENT;
    for (point = 0; point < 4; ++point)
    {
        float side = (point & 2) != 0 ? 0.5f : -0.5f;
        vertices[point].x = points[point & 1].x - side * dy / length;
        vertices[point].y = points[point & 1].y + side * dx / length;
        vertices[point].u = vertices[point].v = 0.0f;
        memcpy(vertices[point].color, points[point & 1].color, 4);
    }
    hd.ui_layer = layer;
    hd_ui_quad(vertices, &texture, 1.0f);
    hd.ui_layer = 0;
}

void hd_ui_line(const XY points[2], const RGBC *color, uint32 layer)
{
    HD_UI_VERTEX vertices[2] = {{0}};
    uint32 point;
    for (point = 0; point < 2; ++point)
    {
        vertices[point].x = (float)points[point].x;
        vertices[point].y = (float)points[point].y;
        memcpy(vertices[point].color, color, 4);
    }
    hd_ui_segment(vertices, 0, (color->code & 2) != 0, layer);
}

float hd_project_world(const VEC3 *position, HD_UI_VERTEX *screen)
{
    VECTORF relative = {position->x / 256.0f - hd.camera_position.vx, position->y / 256.0f - hd.camera_position.vy, position->z / 256.0f - hd.camera_position.vz}, point;
    float scale = model_render.scale / 4096.0f;
    ApplyMatrixf(&hd.camera, &relative, &point);
    if (point.vz > 0.0f)
    {
        screen->x = (sint32)model_render.view_center_x + point.vx * model_render.proj_distance * model_render.x_scale / 4096.0f * scale / point.vz;
        screen->y = (sint32)model_render.view_center_y + point.vy * model_render.proj_distance * model_render.y_scale / 4096.0f * scale / point.vz;
    }
    return point.vz;
}

void hd_world_segment(const HD_UI_VERTEX points[2], float depth, uint16 page, sint32 semi)
{
    VECTORF vertices[4];
    RGBC colors[4];
    HD_TEXTURE texture = {0};
    float dx = points[1].x - points[0].x, dy = points[1].y - points[0].y;
    float length = sqrtf(dx * dx + dy * dy);
    float scale = model_render.proj_distance * model_render.scale / 4096.0f;
    uint32 point;
    if (!hd_capture_active() || length == 0.0f || depth <= 1.0f || scale == 0.0f)
        return;
    texture.page = page;
    texture.flags = semi ? HD_SEMITRANSPARENT : 0;
    for (point = 0; point < 4; ++point)
    {
        float side = (point & 2) != 0 ? 0.5f : -0.5f;
        float x = points[point & 1].x - side * dy / length;
        float y = points[point & 1].y + side * dx / length;
        vertices[point].vx = (x - (sint32)model_render.view_center_x) * depth * 4096.0f / (scale * model_render.x_scale);
        vertices[point].vy = (y - (sint32)model_render.view_center_y) * depth * 4096.0f / (scale * model_render.y_scale);
        vertices[point].vz = depth;
        memcpy(colors + point, points[point & 1].color, 4);
    }
    hd_world_quad(vertices, &texture, colors, 1);
}

static void draw_split_separator(void)
{
    HD_VIEW *view = &hd.viewports[HD_VIEWS];
    HD_UI_VERTEX vertices[4] = {{0}};
    HD_TEXTURE texture = {0};
    float left, top, right, bottom;
    uint32 point, saved_view = hd.view;
    if (hd.views != 2)
        return;
    view->reference = (VID_RECT){0, 0, (sint32)model_render.width, (sint32)model_render.height};
    view->viewport = (VID_RECT){0, 0, hd.width, hd.height};
    view->reference_aspect = hd.height > 0 ? (float)hd.width / hd.height : 4.0f / 3.0f;
    left = race_view.split_mode == 0 ? 0.0f : model_render.width / 2.0f - 1.0f;
    right = race_view.split_mode == 0 ? (float)model_render.width : left + 2.0f;
    top = race_view.split_mode == 0 ? model_render.height / 2.0f : 0.0f;
    bottom = race_view.split_mode == 0 ? top + 1.0f : (float)model_render.height;
    for (point = 0; point < 4; ++point)
    {
        vertices[point].x = (point & 1) != 0 ? right : left;
        vertices[point].y = (point & 2) != 0 ? bottom : top;
    }
    hd.view = HD_VIEWS;
    hd.ui_layer = 0;
    hd_ui_quad(vertices, &texture, 1.0f);
    hd.view = saved_view;
}

void hd_transform_model(const MATRIX *rotation, const VEC3 *position, sint32 camera_relative)
{
    MATRIXF camera, object;
    VECTORF delta, translation;
    uint32 component;
    if (!hd_capture_active())
        return;
    // sky pieces share integer authored seams after the original fixed-point shift
    hd.object_pose.position.x = camera_relative ? (float)(position->x >> 8) : position->x / 256.0f;
    hd.object_pose.position.y = camera_relative ? (float)(position->y >> 8) : position->y / 256.0f;
    hd.object_pose.position.z = camera_relative ? (float)(position->z >> 8) : position->z / 256.0f;
    hd.object_relative = camera_relative;
    camera = hd.camera;
    for (component = 0; component < 3; ++component)
        camera.t[component] = 0.0f;
    delta.vx = hd.object_pose.position.x - (camera_relative ? 0.0f : hd.camera_position.vx);
    delta.vy = hd.object_pose.position.y - (camera_relative ? 0.0f : hd.camera_position.vy);
    delta.vz = hd.object_pose.position.z - (camera_relative ? 0.0f : hd.camera_position.vz);
    ApplyMatrixf(&camera, &delta, &translation);
    if (rotation != NULL)
    {
        matrix_from_native(rotation, &object);
        MatrixQuatf(&object, &hd.object_pose.rotation);
        NormalizeQuatf(&hd.object_pose.rotation, &hd.object_pose.rotation);
        MulMatrix0f(&camera, &object, &hd.transform);
    }
    else
    {
        hd.object_pose.rotation = (QUATF){0.0f, 0.0f, 0.0f, 1.0f};
        hd.transform = camera;
    }
    hd.transform.t[0] = translation.vx;
    hd.transform.t[1] = translation.vy;
    hd.transform.t[2] = translation.vz;
}

void hd_forget_span(const void *bytes, uint32 size)
{
    HD_MODEL **cursor = &hd.models;
    uintptr_t begin = (uintptr_t)bytes;
    uint32 view, slot;
    for (view = 0; view < HD_VIEWS; ++view)
    {
        hd.car_lighting[view][14].generation = ~hd.motion_generation;
        for (slot = 0; slot < 15; ++slot)
        {
            HD_ENV_MOTION *motion = &hd.environment_motion[view][slot];
            uintptr_t source = (uintptr_t)motion->source, texture = (uintptr_t)motion->texture;
            if (bytes == NULL || (source >= begin && source - begin < size) || (texture >= begin && texture - begin < size))
                motion->generation = ~hd.motion_generation;
        }
    }
    while (*cursor != NULL)
    {
        HD_MODEL *model = *cursor;
        uintptr_t address = (uintptr_t)model->source;
        if (bytes == NULL || (address >= begin && address - begin < size))
        {
            *cursor = model->next;
            unhash_model(model);
            model->next = hd.retired_models;
            hd.retired_models = model;
        }
        else
            cursor = &model->next;
    }
    if (!hd.collecting)
        release_retired_models();
}

static void weld_sky_vertex(const MODEL_HEADER *owner, SVECTOR *vertex)
{
    sint32 source[3], best[3];
    uint32 piece, axis;
    const sint16 local[3] = {vertex->vx, vertex->vy, vertex->vz};
    for (axis = 0; axis < 3; ++axis)
        source[axis] = best[axis] = local[axis] + (owner->position.v[axis] >> 8);
    // join source seams split by independent integer quantization of sky pieces
    for (piece = 0; piece < 18; ++piece)
    {
        const MODEL_HEADER *header;
        uint32 offset, index;
        memcpy(&offset, scene_track_image.bytes + 16 + piece * 4, sizeof(offset));
        XASSERT(offset <= scene_track_image.size && sizeof(*header) <= scene_track_image.size - offset);
        header = (const MODEL_HEADER *)(scene_track_image.bytes + offset);
        XASSERT(header->vertex_offset <= scene_track_image.size - offset && (uint64)header->visible_vertices * sizeof(SVECTOR) <= scene_track_image.size - offset - header->vertex_offset);
        for (index = 0; index < header->visible_vertices; ++index)
        {
            SVECTOR candidate;
            sint32 point[3], distance = 0;
            memcpy(&candidate, scene_track_image.bytes + offset + header->vertex_offset + index * sizeof(candidate), sizeof(candidate));
            point[0] = candidate.vx + (header->position.x >> 8);
            point[1] = candidate.vy + (header->position.y >> 8);
            point[2] = candidate.vz + (header->position.z >> 8);
            for (axis = 0; axis < 3; ++axis)
            {
                sint32 delta = point[axis] - source[axis];
                if (delta < -3 || delta > 3)
                    break;
                distance += delta * delta;
            }
            if (axis == 3 && distance <= 9 && (point[0] < best[0] || (point[0] == best[0] && (point[1] < best[1] || (point[1] == best[1] && point[2] < best[2])))))
                memcpy(best, point, sizeof(best));
        }
    }
    for (axis = 0; axis < 3; ++axis)
    {
        best[axis] -= owner->position.v[axis] >> 8;
        XASSERT(best[axis] >= -32768 && best[axis] <= 32767);
    }
    vertex->vx = (sint16)best[0];
    vertex->vy = (sint16)best[1];
    vertex->vz = (sint16)best[2];
}

static HD_MODEL *create_model(const MODEL_RESOURCE *resource, const sint8 *visibility, uint32 variant)
{
    static const uint8 triangle_indices[6] = {0, 1, 2, 1, 3, 2};
    const MODEL_HEADER *header = (const MODEL_HEADER *)resource->bytes;
    uint32 offset, vertices = 0, indices = 0, faces = 0, pass;
    HD_MODEL *model = NULL;
    VID_VERTEX16 *destination = NULL;
    uint32 *index_data = NULL;
    XASSERT(resource->capacity >= sizeof(*header));
    offset = header->stream_offset;
    for (pass = 0; pass < 2; ++pass)
    {
        uint32 vertex_cursor = 0, index_cursor = 0, face_cursor = 0;
        uint32 packet_offset = header->packet_offsets[0];
        offset = header->stream_offset;
        for (;;)
        {
            HD_SOURCE_GROUP group;
            uint32 record_size, packet_size, count, point, face;
            XASSERT(offset <= resource->capacity && sizeof(group) <= resource->capacity - offset);
            memcpy(&group, resource->bytes + offset, sizeof(group));
            offset += sizeof(group);
            if (group.count == 0 || (variant != 0 && group.active == 0))
                break;
            XASSERT(group.count > 0 && group.kind < 4);
            count = (group.kind & 1) != 0 ? 4 : 3;
            record_size = group.kind < 2 ? sizeof(HD_FLAT_FACE) : group.kind == 2 ? sizeof(HD_TRIANGLE_FACE) : sizeof(HD_QUAD_FACE);
            packet_size = group.kind == 0 ? sizeof(POLY_FT3) : group.kind == 1 ? sizeof(POLY_FT4) : group.kind == 2 ? sizeof(POLY_GT3) : sizeof(POLY_GT4);
            XASSERT(offset <= resource->capacity && (uint32)group.count <= (resource->capacity - offset) / record_size);
            for (face = 0; face < (uint32)group.count; ++face)
            {
                sint32 enabled = variant == 0 || group.kind >= 2;
                if (variant == 2 && enabled)
                {
                    uint32 corner;
                    for (corner = 0; corner < count; ++corner)
                    {
                        SVECTOR normal;
                        memcpy(&normal, resource->bytes + offset + (group.kind == 2 ? 20 : 24) + corner * sizeof(normal), sizeof(normal));
                        if (normal.vy < 151)
                            enabled = 0;
                    }
                }
                if (!enabled)
                {
                    offset += record_size;
                    packet_offset += packet_size;
                    continue;
                }
                if (visibility != NULL && visibility[face_cursor++] == 0)
                {
                    offset += record_size;
                    packet_offset += packet_size;
                    continue;
                }
                if (pass == 0)
                {
                    vertices += count;
                    indices += count == 4 ? 6 : 3;
                    ++faces;
                }
                else
                {
                    const uint8 *record = resource->bytes + offset;
                    const POINT_T *flat_points = NULL;
                    const POINT_CT *gouraud_points = NULL;
                    const RGBC *flat_color = NULL;
                    uint32 flags, page, clut, blend;
                    uint16 min_u = 255, min_v = 255, max_u = 0, max_v = 0;
                    TEXTURE_REGION region = {0, 0, 256, 256};
                    sint32 filtered;
                    XASSERT(packet_offset <= resource->capacity && packet_size <= resource->capacity - packet_offset);
                    if (group.kind == 0)
                    {
                        const POLY_FT3 *template = (const POLY_FT3 *)(resource->bytes + packet_offset);
                        flat_points = template->p;
                        flat_color = &template->rgbc;
                    }
                    else if (group.kind == 1)
                    {
                        const POLY_FT4 *template = (const POLY_FT4 *)(resource->bytes + packet_offset);
                        flat_points = template->p;
                        flat_color = &template->rgbc;
                    }
                    else if (group.kind == 2)
                        gouraud_points = ((const POLY_GT3 *)(resource->bytes + packet_offset))->p;
                    else
                        gouraud_points = ((const POLY_GT4 *)(resource->bytes + packet_offset))->p;
                    page = flat_points != NULL ? flat_points[1].attr : gouraud_points[1].attr;
                    clut = flat_points != NULL ? flat_points[0].attr : gouraud_points[0].attr;
                    flags = flat_color != NULL ? flat_color->code : gouraud_points[0].rgbc.code;
                    blend = (flags & 2) != 0 ? 1 + ((page >> 5) & 3) : 0;
                    if (blend != 0)
                        model->has_transparent = 1;
                    for (point = 0; point < count; ++point)
                    {
                        uint16 u = flat_points != NULL ? flat_points[point].uv.u : gouraud_points[point].uv.u;
                        uint16 v = flat_points != NULL ? flat_points[point].uv.v : gouraud_points[point].uv.v;
                        if (u < min_u)
                            min_u = u;
                        if (v < min_v)
                            min_v = v;
                        if (u > max_u)
                            max_u = u;
                        if (v > max_v)
                            max_v = v;
                    }
                    filtered = tex_find_region((uint16)page, (uint16)clut, min_u, min_v, max_u, max_v, &region);
                    if (!filtered)
                    {
                        region = (TEXTURE_REGION){min_u, min_v, max_u - min_u + 1, max_v - min_v + 1};
                        filtered = 1;
                    }
                    HD_SOURCE_REGION source_region = {0};
                    HD_SOURCE_TEXTURE *source_texture = visibility == NULL && variant == 0 ? hd_texture_source_material((uint16)page, (uint16)clut, min_u, min_v, max_u, max_v, &source_region) : NULL;
                    HD_TEXTURE face_texture = {(uint16)page, (uint16)clut, region.u, region.v, region.width, region.height, (uint16)(HD_TEXTURED | HD_FILTERED | ((flags & 1) != 0 ? HD_RAW_TEXTURE : 0) | (blend != 0 ? HD_SEMITRANSPARENT : 0))};
                    if (visibility != NULL || model->range_count == 0 || model->ranges[model->range_count - 1].blend != blend || model->ranges[model->range_count - 1].source_texture != source_texture || memcmp(&model->ranges[model->range_count - 1].source_region, &source_region, sizeof(source_region)) != 0 || memcmp(&model->ranges[model->range_count - 1].texture, &face_texture, sizeof(face_texture)) != 0)
                    {
                        HD_MODEL_RANGE *range = &model->ranges[model->range_count++];
                        range->first = index_cursor;
                        range->count = 0;
                        range->blend = blend;
                        range->texture = face_texture;
                        range->source_texture = source_texture;
                        range->source_region = source_region;
                    }
                    model->ranges[model->range_count - 1].count += count == 4 ? 6 : 3;
                    for (point = 0; point < count; ++point)
                    {
                        VID_VERTEX16 *target = &destination[vertex_cursor + point];
                        SVECTOR position, normal = {0};
                        RGBC color;
                        sint16 index;
                        sint64 vertex_offset;
                        memcpy(&index, record + point * sizeof(index), sizeof(index));
                        vertex_offset = (sint64)header->vertex_offset + (sint64)index * sizeof(SVECTOR);
                        XASSERT(vertex_offset >= 0 && (uint64)vertex_offset <= resource->capacity && sizeof(position) <= resource->capacity - (uint32)vertex_offset);
                        memcpy(&position, resource->bytes + (uint32)vertex_offset, sizeof(position));
                        if (visibility != NULL)
                            weld_sky_vertex(header, &position);
                        if (visibility == NULL && (variant != 0 || header->renderer == 1 || header->renderer == 3 || header->renderer == 9))
                        {
                            uint32 normal_offset = group.kind < 2 ? 12 : group.kind == 2 ? 20 + point * 8 : 24 + point * 8;
                            memcpy(&normal, record + normal_offset, sizeof(normal));
                            memcpy(&color, record + 8 + (group.kind < 2 ? 0 : point * 4), sizeof(color));
                        }
                        else
                            color = flat_color != NULL ? *flat_color : gouraud_points[point].rgbc;
                        target->pos[0] = position.vx;
                        target->pos[1] = position.vy;
                        target->pos[2] = position.vz;
                        target->pos[3] = 1;
                        // keep constant source UVs inside their texel despite perspective rounding
                        target->uv[0] = (sint16)(2 * ((flat_points != NULL ? flat_points[point].uv.u : gouraud_points[point].uv.u) - region.u) + (min_u == max_u));
                        target->uv[1] = (sint16)(2 * ((flat_points != NULL ? flat_points[point].uv.v : gouraud_points[point].uv.v) - region.v) + (min_v == max_v));
                        memcpy(target->color, &color, sizeof(color));
                        target->n[0] = normal.vx;
                        target->n[1] = normal.vy;
                        target->n[2] = normal.vz;
                        target->n[3] = visibility == NULL && (header->renderer == 1 || header->renderer == 3 || header->renderer == 9);
                        model->has_lighting |= target->n[3] != 0;
                        memset(target->extra, 0, sizeof(target->extra));
                        if (variant == 2)
                        {
                            SVECTOR corners[4];
                            uint32 corner;
                            for (corner = 0; corner < 4; ++corner)
                            {
                                sint16 corner_index;
                                sint64 corner_offset;
                                memcpy(&corner_index, record + (corner < count ? corner : count - 1) * sizeof(corner_index), sizeof(corner_index));
                                corner_offset = (sint64)header->vertex_offset + (sint64)corner_index * sizeof(SVECTOR);
                                XASSERT(corner_offset >= 0 && (uint64)corner_offset <= resource->capacity && sizeof(SVECTOR) <= resource->capacity - (uint32)corner_offset);
                                memcpy(&corners[corner], resource->bytes + (uint32)corner_offset, sizeof(SVECTOR));
                            }
                            target->extra[0] = (float)corners[0].vx;
                            target->extra[1] = (float)corners[0].vz;
                            target->extra[2] = (float)corners[1].vx;
                            target->extra[3] = (float)corners[1].vz;
                            target->n[0] = corners[2].vx;
                            target->n[1] = corners[2].vz;
                            target->n[2] = corners[3].vx;
                            target->n[3] = corners[3].vz;
                        }
                        target->tex[0] = (uint16)page;
                        target->tex[1] = (uint16)clut;
                        target->tex[2] = HD_TEXTURED | (filtered ? HD_FILTERED : HD_REPEAT_U | HD_REPEAT_V) | ((flags & 1) != 0 ? HD_RAW_TEXTURE : 0) | (blend != 0 ? HD_SEMITRANSPARENT : 0);
                        target->tex[3] = region.u | (region.v << 8);
                        target->rect[0] = region.width;
                        target->rect[1] = region.height;
                    }
                    for (point = 0; point < (count == 4 ? 6u : 3u); ++point)
                        index_data[index_cursor++] = vertex_cursor + triangle_indices[point];
                    vertex_cursor += count;
                }
                offset += record_size;
                packet_offset += packet_size;
            }
        }
        if (pass == 0)
        {
            if (vertices == 0)
                return NULL;
            model = (HD_MODEL *)calloc(1, sizeof(*model));
            XASSERT(model != NULL);
            model->source = resource->bytes;
            model->source_size = resource->capacity;
            model->kind = header->renderer;
            model->visibility = visibility;
            model->variant = variant;
            model->index_count = indices;
            model->geometry = vid_mesh_create_16(vertices);
            model->indices = vid_index_create(indices);
            model->ranges = (HD_MODEL_RANGE *)malloc((size_t)faces * sizeof(*model->ranges));
            XASSERT(model->geometry != NULL && model->indices != NULL && model->ranges != NULL);
            destination = vid_mesh_data_16(model->geometry);
            index_data = vid_index_data(model->indices);
        }
    }
    if (visibility != NULL)
    {
        uint32 range, cursor = 0;
        uint32 *target;
        model->sky_ranges = (HD_MODEL_RANGE *)calloc(model->range_count, sizeof(*model->sky_ranges));
        model->sky_indices = vid_index_create(model->index_count);
        XASSERT(model->sky_ranges != NULL && model->sky_indices != NULL);
        target = vid_index_data(model->sky_indices);
        for (range = model->range_count; range > 0; --range)
        {
            const HD_MODEL_RANGE *part = &model->ranges[range - 1];
            HD_MODEL_RANGE *previous = model->sky_range_count != 0 ? &model->sky_ranges[model->sky_range_count - 1] : NULL;
            memcpy(target + cursor, index_data + part->first, part->count * sizeof(uint32));
            // retain exact sky primitive order and each transparent face's two passes
            if (part->blend == 0 && previous != NULL && previous->blend == 0)
                previous->count += part->count;
            else
            {
                HD_MODEL_RANGE *merged = &model->sky_ranges[model->sky_range_count++];
                *merged = *part;
                merged->first = cursor;
            }
            cursor += part->count;
        }
        vid_index_commit(model->sky_indices, cursor);
        vid_index_static(model->sky_indices);
    }
    if (visibility == NULL && variant == 0)
    {
        uint32 range;
        model->uses_sources = model->range_count != 0;
        for (range = 0; range < model->range_count; ++range)
        {
            HD_MODEL_RANGE *part = &model->ranges[range];
            if (part->source_texture == NULL)
                model->uses_sources = 0;
            else if (part->blend != 0)
                hd_texture_source_semitransparent(part->source_texture);
        }
        if (!model->uses_sources)
            model->texture_asset = object_texture_layout(destination, vertices);
    }
    if (visibility == NULL && variant == 0)
    {
        uint32 vertex, axis;
        const VID_VERTEX16 *source = vid_mesh_data_16(model->geometry);
        model->batch_vertex_count = vertices;
        model->batch_source_indices = (uint32 *)malloc(model->index_count * sizeof(uint32));
        model->batch_vertices = (VID_VERTEX3D *)calloc(vertices, sizeof(VID_VERTEX3D));
        XASSERT(model->batch_source_indices != NULL && model->batch_vertices != NULL);
        memcpy(model->batch_source_indices, index_data, model->index_count * sizeof(uint32));
        for (vertex = 0; vertex < vertices; ++vertex)
        {
            VID_VERTEX3D *target = &model->batch_vertices[vertex];
            for (axis = 0; axis < 4; ++axis)
            {
                target->pos[axis] = source[vertex].pos[axis];
                target->color[axis] = source[vertex].color[axis];
                target->n[axis] = source[vertex].n[axis];
                target->extra[axis] = source[vertex].extra[axis];
            }
            target->extra[3] = (float)source[vertex].tex[2];
            target->uv[0] = source[vertex].uv[0];
            target->uv[1] = source[vertex].uv[1];
        }
    }
    vid_mesh_commit(model->geometry);
    vid_index_commit(model->indices, model->index_count);
    if (!model->uses_sources)
        vid_index_static(model->indices);
    model->next = hd.models;
    hd.models = model;
    {
        uint32 bucket = model_bucket(model->source, model->source_size, model->visibility, model->variant);
        model->hash_next = model_lookup[bucket];
        model_lookup[bucket] = model;
    }
    ++hd_work.model_builds;
    return model;
}

static HD_PROJECTION interpolated_projection(const HD_PROJECTION samples[2])
{
    HD_PROJECTION result;
    result.x = samples[0].x + hd.render_phase * (samples[1].x - samples[0].x);
    result.y = samples[0].y + hd.render_phase * (samples[1].y - samples[0].y);
    result.center_x = samples[0].center_x + hd.render_phase * (samples[1].center_x - samples[0].center_x);
    result.center_y = samples[0].center_y + hd.render_phase * (samples[1].center_y - samples[0].center_y);
    return result;
}

static void capture_projection(HD_MODEL_DRAW *draw, uint64 key)
{
    HD_PROJECTION sample = {(float)model_render.proj_distance * (model_render.x_scale / 4096.0f) * (model_render.scale / 4096.0f), (float)model_render.proj_distance * (model_render.y_scale / 4096.0f) * (model_render.scale / 4096.0f), (float)(sint32)model_render.view_center_x, (float)(sint32)model_render.view_center_y};
    HD_PROJECTION_MOTION *motion = &hd.projection_motion[hd.view];
    if (motion->serial != hd.motion_serial || motion->generation != hd.motion_generation || motion->key != key)
    {
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation || motion->key != key)
            motion->previous = sample;
        else
            motion->previous = motion->current;
        motion->current = sample;
        motion->serial = hd.motion_serial;
        motion->generation = hd.motion_generation;
        motion->key = key;
    }
    draw->projection[0] = motion->previous;
    draw->projection[1] = sample;
}

static void capture_draw_poses(HD_MODEL_DRAW *draw, HD_MODEL *model, sint32 view_geometry)
{
    HD_POSE identity = {{0}};
    HD_MOTION *motion = &hd.camera_motion[hd.view];
    uint64 key;
    if (hd.race_frame)
    {
        XASSERT(hd.race_pose_views & (1u << hd.view));
        key = motion->key;
    }
    else
        key = capture_camera_pose();
    capture_projection(draw, key);
    draw->poses.camera[0] = motion->previous;
    draw->poses.camera[1] = motion->current;
    draw->poses.relative = hd.object_relative;
    draw->poses.view_geometry = view_geometry;
    draw->poses.billboard = !view_geometry && model->visibility == NULL && model->kind >= 10;
    draw->poses.source_transform = hd.transform;
    if (hd.wheel != NULL)
    {
        draw->poses.wheel[0] = hd.wheel->previous;
        draw->poses.wheel[1] = hd.wheel->current;
        draw->poses.wheel_active = 1;
        draw->poses.wheel_hub = hd.wheel_hub;
    }
    if (hd.actor >= 0)
        motion = &hd.car_motion[hd.view][hd.actor];
    else if (hd.wheel != NULL)
        motion = &hd.preview_motion[hd.view];
    else if (!view_geometry)
    {
        motion = &model->motion[hd.view];
        motion_update(motion, &hd.object_pose, 0);
    }
    else
    {
        VECTORF translation;
        identity.rotation.w = 1.0f;
        draw->poses.object[0] = draw->poses.object[1] = identity;
        draw->poses.relative = 0;
        draw->poses.source_transform = hd.camera;
        ApplyMatrixf(&hd.camera, &hd.camera_position, &translation);
        draw->poses.source_transform.t[0] = -translation.vx;
        draw->poses.source_transform.t[1] = -translation.vy;
        draw->poses.source_transform.t[2] = -translation.vz;
        return;
    }
    draw->poses.object[0] = motion->previous;
    draw->poses.object[1] = motion->current;
}

static void capture_lighting(HD_MODEL_DRAW *draw)
{
    HD_LIGHT_SAMPLE sample;
    HD_LIGHT_MOTION *motion;
    MATRIXF object, inverse;
    uint64 camera_key = hd.camera_motion[hd.view].key;
    uint32 row, column;
    if (hd.actor >= 0 || hd.wheel != NULL)
        motion = &hd.car_lighting[hd.view][hd.actor >= 0 ? hd.actor : 14];
    else
        motion = &draw->model->lighting[hd.view];
    QuatMatrixf(&draw->poses.object[1].rotation, &object);
    IdentityMatrixf(&inverse);
    for (row = 0; row < 3; ++row)
        for (column = 0; column < 3; ++column)
            inverse.m[row][column] = object.m[column][row];
    MulMatrix0f(&draw->light, &inverse, &sample.world);
    matrix_from_native(&draw->color, &sample.color);
    memcpy(sample.ambient, draw->ambient, sizeof(sample.ambient));
    if (hd.light_override != NULL)
        sample = *hd.light_override;
    if (motion->serial != hd.motion_serial || motion->generation != hd.motion_generation || motion->camera_key != camera_key)
    {
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation || motion->camera_key != camera_key)
            motion->previous = sample;
        else
            motion->previous = motion->current;
        motion->current = sample;
        motion->serial = hd.motion_serial;
        motion->generation = hd.motion_generation;
        motion->camera_key = camera_key;
    }
    draw->lighting[0] = motion->previous;
    draw->lighting[1] = sample;
    draw->light_history = 1;
}

static HD_MODEL_DRAW *reserve_model_draw(HD_MODEL *model, sint32 view_geometry)
{
    HD_MODEL_DRAW *draw;
    uint32 row;
    if (hd.model_count == hd.model_capacity)
    {
        uint32 capacity = hd.model_capacity != 0 ? hd.model_capacity * 2 : 128;
        HD_MODEL_DRAW *draws = (HD_MODEL_DRAW *)realloc(hd.model_draws, (size_t)capacity * sizeof(*draws));
        XASSERT(draws != NULL);
        hd.model_draws = draws;
        hd.model_capacity = capacity;
    }
    draw = &hd.model_draws[hd.model_count++];
    memset(draw, 0, sizeof(*draw));
    draw->image = track_image;
    draw->region_sampling = hd.race_frame && hd.actor < 0 && model->visibility == NULL;
    if (draw->image != NULL)
        ++draw->image->refs;
    draw->fog_active = hd.race_frame && model->visibility == NULL;
    if (draw->fog_active)
    {
        XASSERT(race_session.track < 40);
        memcpy(draw->fog_color, track_fog_colors[race_session.track], sizeof(draw->fog_color));
        draw->fog_horizon = hd_track_horizon();
    }
    draw->model = model;
    ++model->draw_refs;
    capture_draw_poses(draw, model, view_geometry);
    draw->transform = hd.transform;
    draw->light = hd.light;
    draw->color = model_render.color;
    draw->view = hd.view;
    draw->focal = (float)model_render.proj_distance;
    draw->scale_x = model_render.x_scale / 4096.0f * model_render.scale / 4096.0f;
    draw->scale_y = model_render.y_scale / 4096.0f * model_render.scale / 4096.0f;
    draw->center_x = (float)(sint32)model_render.view_center_x;
    draw->center_y = (float)(sint32)model_render.view_center_y;
    draw->far_plane = hd_far_plane();
    for (row = 0; row < 3; ++row)
        draw->ambient[row] = hd.ambient[row];
    if (!view_geometry && model->visibility == NULL && model->variant == 0 && (model->kind == 1 || model->kind == 3 || model->kind == 9))
        capture_lighting(draw);
    return draw;
}

void hd_police_light(uint32 index, const SVECTOR *anchor, const HD_TEXTURE *texture, const RGBC colors[3], const uint32 ray_flags[2], float signal)
{
    static const uint8 corners[6] = {0, 1, 2, 1, 3, 2};
    HD_POLICE_SAMPLE sample;
    HD_POLICE_MOTION *motion;
    HD_MODEL_DRAW *draw;
    VECTORF local = {(float)anchor->vx, (float)anchor->vy, (float)anchor->vz};
    uint64 camera_key;
    uint32 vertex, part, channel;
    if (!hd_capture_active())
        return;
    XASSERT(hd.actor >= 0 && hd.actor < 14 && index < 4);
    if (hd.police_model.geometry == NULL)
    {
        VID_VERTEX16 *vertices;
        uint32 *indices;
        hd.police_model.geometry = vid_mesh_create_16(12);
        hd.police_model.indices = vid_index_create(12);
        XASSERT(hd.police_model.geometry != NULL && hd.police_model.indices != NULL);
        vertices = vid_mesh_data_16(hd.police_model.geometry);
        indices = vid_index_data(hd.police_model.indices);
        memset(vertices, 0, 12 * sizeof(*vertices));
        for (vertex = 0; vertex < 12; ++vertex)
        {
            indices[vertex] = vertex;
            if (vertex < 6)
            {
                vertices[vertex].pos[0] = (corners[vertex] & 1) != 0 ? 1 : -1;
                vertices[vertex].pos[1] = (corners[vertex] & 2) != 0 ? -1 : 1;
            }
            else
            {
                uint32 corner = (vertex - 6) % 3;
                vertices[vertex].pos[0] = corner == 0 ? 0 : corner == 1 ? 1 : -1;
                vertices[vertex].pos[2] = vertex < 9 ? 1 : 2;
                vertices[vertex].pos[3] = corner == 0 ? 0 : 1;
            }
        }
        vid_mesh_commit(hd.police_model.geometry);
        vid_index_commit(hd.police_model.indices, 12);
        vid_index_static(hd.police_model.indices);
        hd.police_model.index_count = 12;
    }
    draw = reserve_model_draw(&hd.police_model, 0);
    camera_key = hd.camera_motion[hd.view].key;
    motion = &hd.police_motion[hd.view][hd.actor][index];
    sample.signal = signal;
    for (part = 0; part < 3; ++part)
        for (channel = 0; channel < 3; ++channel)
            sample.colors[part][channel] = ((const uint8 *)&colors[part])[channel];
    if (motion->serial != hd.motion_serial || motion->generation != hd.motion_generation || motion->camera_key != camera_key || motion->anchor.vx != local.vx || motion->anchor.vy != local.vy || motion->anchor.vz != local.vz || memcmp(&motion->texture, texture, sizeof(*texture)) != 0)
    {
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation || motion->camera_key != camera_key || motion->anchor.vx != local.vx || motion->anchor.vy != local.vy || motion->anchor.vz != local.vz || memcmp(&motion->texture, texture, sizeof(*texture)) != 0)
            motion->previous = sample;
        else
            motion->previous = motion->current;
        motion->current = sample;
        motion->serial = hd.motion_serial;
        motion->generation = hd.motion_generation;
        motion->camera_key = camera_key;
        motion->anchor = local;
        motion->texture = *texture;
        motion->ray_flags[0] = ray_flags[0];
        motion->ray_flags[1] = ray_flags[1];
    }
    draw->dynamic = 4;
    draw->blend = (texture->flags & HD_SEMITRANSPARENT) != 0 || ray_flags[0] != 0 || ray_flags[1] != 0 ? 1 + ((texture->page >> 5) & 3) : 0;
    draw->police.samples[0] = motion->previous;
    draw->police.samples[1] = sample;
    draw->police.anchor = local;
    draw->police.texture = *texture;
    draw->police.ray_flags[0] = ray_flags[0];
    draw->police.ray_flags[1] = ray_flags[1];
    draw->police.depth_scale = (float)(1u << (model_render.coord_shift & 31));
    draw->police.near_depth = (float)model_render.near_depth;
    draw->police.far_depth = (float)model_render.far_depth;
    draw->police.half_width = model_render.width * 0.5f;
    draw->police.half_height = model_render.height * 0.5f;
}

void hd_gauge_needle(const CAR_STATE *car, uint32 pass, float center_x, float center_y, float radius, float thickness, float vertical_scale, const RGBC *color, uint32 layer)
{
    HD_UI_VERTEX vertices[3] = {{0}};
    HD_TEXTURE texture = {0};
    HD_NEEDLE_MOTION *motion;
    uint32 slot, point, before, previous_layer;
    sint32 rpm_limit;
    float angle, offset;
    if (!hd_capture_active())
        return;
    slot = (uint32)(car - car_states);
    XASSERT(slot < 14 && pass < 2);
    rpm_limit = car->physics_profile->rpm_limit;
    XASSERT(rpm_limit != 0);
    angle = (float)(sint32)car->engine_speed * 3072.0f / rpm_limit * RAD12;
    motion = &hd.needles[hd.view][slot];
    if (motion->serial != hd.motion_serial || motion->generation != hd.motion_generation || motion->vertical_scale != vertical_scale || motion->rpm_limit != rpm_limit)
    {
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation || motion->vertical_scale != vertical_scale || motion->rpm_limit != rpm_limit)
            motion->previous = angle;
        else
            motion->previous = motion->current;
        motion->current = angle;
        motion->serial = hd.motion_serial;
        motion->generation = hd.motion_generation;
        motion->vertical_scale = vertical_scale;
        motion->rpm_limit = rpm_limit;
    }
    vertices[0].x = center_x - radius;
    vertices[0].y = center_y;
    vertices[1].x = vertices[2].x = center_x;
    vertices[1].y = center_y - thickness;
    vertices[2].y = center_y + thickness;
    for (point = 0; point < 3; ++point)
        memcpy(vertices[point].color, color, sizeof(*color));
    texture.flags = (color->code & 2) != 0 ? HD_SEMITRANSPARENT : 0;
    before = hd.ui_count;
    previous_layer = hd.ui_layer;
    hd.ui_layer = layer;
    hd_ui_triangle(vertices, &texture, 1.0f);
    hd.ui_layer = previous_layer;
    if (hd.ui_count != before)
    {
        HD_NEEDLE *needle = &hd.ui_draws[hd.ui_count - 1].needle;
        offset = pass * 3.14159265358979323846f;
        needle->angles[0] = motion->previous + offset;
        needle->angles[1] = motion->current + offset;
        needle->center_x = center_x;
        needle->center_y = center_y;
        needle->vertical_scale = vertical_scale;
        needle->active = 1;
    }
}

void hd_police_retire(void)
{
    uint32 index, part, channel;
    if (!hd_capture_active())
        return;
    XASSERT(hd.actor >= 0 && hd.actor < 14);
    for (index = 0; index < 4; ++index)
    {
        const HD_POLICE_MOTION *motion = &hd.police_motion[hd.view][hd.actor][index];
        SVECTOR anchor;
        HD_TEXTURE texture;
        RGBC colors[3] = {{0}};
        uint32 ray_flags[2];
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation || motion->camera_key != hd.camera_motion[hd.view].key || motion->current.signal == 0.0f)
            continue;
        anchor = (SVECTOR){(sint16)motion->anchor.vx, (sint16)motion->anchor.vy, (sint16)motion->anchor.vz, 0};
        texture = motion->texture;
        ray_flags[0] = motion->ray_flags[0];
        ray_flags[1] = motion->ray_flags[1];
        for (part = 0; part < 3; ++part)
            for (channel = 0; channel < 3; ++channel)
                ((uint8 *)&colors[part])[channel] = (uint8)motion->current.colors[part][channel];
        hd_police_light(index, &anchor, &texture, colors, ray_flags, 0.0f);
    }
}

static sint32 reference_contains(const HD_VIEW *view, float x, float y)
{
    float aspect, half_width, center;
    if (view->viewport.w <= 0 || view->viewport.h <= 0)
        return 0;
    aspect = (float)view->viewport.w / view->viewport.h;
    half_width = view->reference.w * 0.5f * aspect / view->reference_aspect;
    center = view->reference.x + view->reference.w * 0.5f;
    return x >= center - half_width && x < center + half_width && y >= view->reference.y && y < view->reference.y + view->reference.h;
}

static void capture_facing(HD_MODEL_DRAW *draw)
{
    HD_POSE facing = {0};
    HD_MOTION *motion = &draw->model->facing[hd.view];
    MatrixQuatf(&draw->transform, &facing.rotation);
    NormalizeQuatf(&facing.rotation, &facing.rotation);
    motion_update(motion, &facing, hd.camera_motion[hd.view].key);
    draw->poses.facing[0] = motion->previous.rotation;
    draw->poses.facing[1] = motion->current.rotation;
}

static void draw_sun(HD_MODEL_DRAW *draw, sint32 emit_flares)
{
    const VID_VERTEX16 *source = vid_mesh_data_16(draw->model->geometry);
    MATRIXF radial, rotation;
    HD_SUN_ATTACHMENT attachment = {0};
    float angle = atan2f(draw->transform.t[1], draw->transform.t[0]);
    uint32 point, flare;
    XASSERT(draw->model->index_count >= 6);
    IdentityMatrixf(&radial);
    RotMatrixZf(angle, &radial);
    MulMatrix0f(&hd.billboard, &radial, &rotation);
    memcpy(draw->transform.m, rotation.m, sizeof(rotation.m));
    draw->dynamic = 2;
    draw->first = 0;
    draw->count = 6;
    draw->blend = draw->model->ranges[0].blend;
    capture_facing(draw);
    if (!emit_flares || !hd.flares_valid)
        return;
    memcpy(attachment.camera, draw->poses.camera, sizeof(attachment.camera));
    memcpy(attachment.object, draw->poses.object, sizeof(attachment.object));
    memcpy(attachment.facing, draw->poses.facing, sizeof(attachment.facing));
    memcpy(attachment.projection, draw->projection, sizeof(attachment.projection));
    attachment.relative = draw->poses.relative;
    attachment.half_width = model_render.width * 0.5f;
    attachment.half_height = model_render.height * 0.5f;
    attachment.depth_scale = (float)(1u << (model_render.coord_shift & 31));
    attachment.zsf4 = (float)(sint16)xport_gte_read_control(30);
    attachment.far_otz = (float)(model_render.far_depth >> 2);
    attachment.active = 1;
    for (point = 0; point < 4; ++point)
        attachment.points[point] = (VECTORF){(float)source[point].pos[0], (float)source[point].pos[1], (float)source[point].pos[2]};
    for (flare = 0; flare < 2; ++flare)
    {
        const TEXTURE_DESCRIPTOR *source_texture = &hd.flares[flare];
        HD_TEXTURE texture = {source_texture->page, source_texture->clut, source_texture->u, source_texture->v, source_texture->width, source_texture->height, HD_TEXTURED | HD_SEMITRANSPARENT | HD_FILTERED | HD_SCENE_TEXTURE | HD_LENS_FLARE};
        HD_UI_VERTEX vertices[4];
        uint32 before = hd.ui_count;
        attachment.factor = flare == 0 ? 192.0f / 256.0f : 96.0f / 256.0f;
        for (point = 0; point < 4; ++point)
        {
            vertices[point].x = attachment.half_width + ((point & 1) != 0 ? texture.width * 0.5f - 1.0f : -texture.width * 0.5f);
            vertices[point].y = attachment.half_height + ((point & 2) != 0 ? texture.height * 0.5f - 1.0f : -texture.height * 0.5f);
            vertices[point].u = texture.u + ((point & 1) != 0 ? texture.width - 1.0f : 0.0f);
            vertices[point].v = texture.v + ((point & 2) != 0 ? texture.height - 1.0f : 0.0f);
            memset(vertices[point].color, 128, 4);
        }
        hd.ui_layer = 1;
        hd_ui_quad(vertices, &texture, 1.0f);
        if (hd.ui_count != before)
            hd.ui_draws[hd.ui_count - 1].sun = attachment;
    }
    hd.ui_layer = 0;
}

void hd_flare_textures(const uint8 *const textures[2])
{
    memcpy(&hd.flares[0], textures[0], sizeof(hd.flares[0]));
    memcpy(&hd.flares[1], textures[1], sizeof(hd.flares[1]));
    hd.flares_valid = 1;
}

typedef struct HD_SOURCE_MATERIAL
{
    struct HD_SOURCE_MATERIAL *next;
    VID_TEXTURE *texture;
    VID_MATERIAL *material, *batch_material;
    VID_MATERIAL_DATA data;
    uint32 pass, cull, transparent;
} HD_SOURCE_MATERIAL;

static HD_SOURCE_MATERIAL *source_materials;

static HD_SOURCE_MATERIAL *source_material(VID_TEXTURE *texture, uint32 pass, uint32 cull, uint32 transparent)
{
    HD_SOURCE_MATERIAL *entry;
    VID_MATERIAL_DATA *data;
    for (entry = source_materials; entry != NULL; entry = entry->next)
        if (entry->texture == texture && entry->pass == pass && entry->cull == cull && entry->transparent == transparent)
            return entry;
    entry = (HD_SOURCE_MATERIAL *)calloc(1, sizeof(*entry));
    XASSERT(entry != NULL);
    entry->texture = texture;
    entry->pass = pass;
    entry->cull = cull;
    entry->transparent = transparent;
    entry->material = create_ui_material(pass);
    data = vid_mat_data(entry->material);
    data->texture = texture;
    data->shader = object_texture_shader;
    data->filter = VID_FILTER_ANISOTROPIC;
    data->cull = cull;
    data->depth_test = 1;
    data->depth_write = pass == 0;
    data->params[7] = pass == 0 && transparent ? 0.5f : 0.0f;
    {
        uint32 width, height;
        hd_texture_source_bank_size(texture, &width, &height);
        data->params[4] = (float)width;
        data->params[5] = (float)height;
    }
    entry->data = *data;
    vid_mat_commit(entry->material);
    entry->batch_material = vid_mat_create();
    XASSERT(entry->batch_material != NULL);
    data = vid_mat_data(entry->batch_material);
    *data = entry->data;
    data->shader = scene_batch_shader;
    vid_mat_commit(entry->batch_material);
    entry->next = source_materials;
    source_materials = entry;
    return entry;
}

static void warm_source_bank(VID_TEXTURE *texture, void *geometry)
{
    vid_pass(source_material(texture, 0, 0, 0)->material);
    vid_submit((VID_GEOMETRY *)geometry, NULL, 0, 3);
}

static void source_materials_free(void)
{
    while (source_materials != NULL)
    {
        HD_SOURCE_MATERIAL *entry = source_materials;
        source_materials = entry->next;
        vid_mat_free(entry->material);
        vid_mat_free(entry->batch_material);
        free(entry);
    }
}

static void prepare_source_model(HD_MODEL *model)
{
    uint32 range, group, cursor = 0;
    uint32 *ordered;
    VID_VERTEX16 *vertices;
    if (!model->uses_sources || model->source_ready)
        return;
    vertices = vid_mesh_data_16(model->geometry);
    ordered = (uint32 *)malloc(model->index_count * sizeof(*ordered));
    model->source_groups = (HD_CACHED_GROUP *)calloc(model->range_count, sizeof(*model->source_groups));
    XASSERT(vertices != NULL && ordered != NULL && model->source_groups != NULL);
    for (range = 0; range < model->range_count; ++range)
    {
        HD_MODEL_RANGE *part = &model->ranges[range];
        HD_SOURCE_GPU image = hd_texture_source_gpu(part->source_texture);
        uint32 index;
        for (group = 0; group < model->source_group_count; ++group)
            if (model->source_groups[group].texture == image.texture)
                break;
        if (group == model->source_group_count)
            model->source_groups[model->source_group_count++].texture = image.texture;
        model->source_groups[group].count += part->count;
        model->source_groups[group].transparent |= (image.classes & 1) != 0 || (part->blend != 0 && (image.classes & 4) != 0);
        for (index = part->first; index < part->first + part->count; ++index)
        {
            uint32 vertex = model->batch_source_indices[index];
            VID_VERTEX16 *point = &vertices[vertex];
            VID_VERTEX3D *batch = &model->batch_vertices[vertex];
            if ((point->tex[2] & 1024) != 0)
                continue;
            point->uv[0] += (sint16)(2 * ((sint32)(point->tex[3] & 255) - part->source_region.u));
            point->uv[1] += (sint16)(2 * ((sint32)(point->tex[3] >> 8) - part->source_region.v));
            point->tex[0] = (uint16)image.opaque_layer;
            point->tex[1] = (uint16)image.stp_layer;
            point->tex[2] |= 1024;
            if (model->kind == 0 || model->kind == 2 || model->kind == 4 || model->kind == 6 || model->kind == 10 || model->kind == 11 || model->kind == 13)
                point->tex[2] |= HD_DISTANCE_LIT;
            if (((const MODEL_HEADER *)model->source)->format[0] != 0)
                point->tex[2] |= HD_HALF_DISTANCE_LIGHT;
            point->tex[3] = (uint16)image.mode;
            point->rect[0] = (uint16)image.width;
            point->rect[1] = (uint16)image.height;
            batch->uv[0] = point->uv[0];
            batch->uv[1] = point->uv[1];
            batch->extra[0] = (float)image.opaque_layer;
            batch->extra[1] = (float)image.stp_layer;
            batch->extra[2] = (float)image.mode;
            batch->extra[3] = (float)point->tex[2];
        }
    }
    for (group = 0; group < model->source_group_count; ++group)
    {
        HD_CACHED_GROUP *target = &model->source_groups[group];
        target->first = cursor;
        for (range = 0; range < model->range_count; ++range)
        {
            HD_MODEL_RANGE *part = &model->ranges[range];
            if (hd_texture_source_gpu(part->source_texture).texture != target->texture)
                continue;
            part->source_first = cursor;
            memcpy(ordered + cursor, model->batch_source_indices + part->first, part->count * sizeof(uint32));
            cursor += part->count;
        }
    }
    XASSERT(cursor == model->index_count);
    memcpy(model->batch_source_indices, ordered, cursor * sizeof(uint32));
    memcpy(vid_index_data(model->indices), ordered, cursor * sizeof(uint32));
    free(ordered);
    vid_mesh_commit(model->geometry);
    vid_index_commit(model->indices, cursor);
    vid_index_static(model->indices);
    model->source_ready = 1;
}

static void static_pools_free(void)
{
    while (static_pools != NULL)
    {
        HD_STATIC_POOL *pool = static_pools;
        static_pools = pool->next;
        vid_mesh_free(pool->geometry);
        vid_index_free(pool->indices);
        free(pool->groups);
        free(pool);
    }
}

uint32 hd_prepared_draw_slots(const HD_PREPARED_MODEL *model)
{
    return model == NULL ? 0 : model->uses_sources ? model->source_group_count + (model->has_transparent != 0) : 1;
}

void hd_prepare_track_geometry(HD_PREPARED_MODEL *const *models, uint32 count, uint32 main_count)
{
    HD_STATIC_POOL *pool;
    VID_VERTEX3D *vertices;
    uint32 *indices, *offsets;
    uint32 item, total_vertices = 0, total_indices = 0, maximum_groups = 0, group, cursor = 0;
    for (item = 0; item < count; ++item)
    {
        HD_MODEL *model = models[item];
        if (model == NULL || model->kind >= 10 || !model->source_ready || model->static_pool != NULL)
            continue;
        total_vertices += model->batch_vertex_count;
        total_indices += model->index_count;
        maximum_groups += model->source_group_count;
    }
    if (total_vertices == 0)
        return;
    pool = (HD_STATIC_POOL *)calloc(1, sizeof(*pool));
    offsets = (uint32 *)calloc(count, sizeof(*offsets));
    XASSERT(pool != NULL && offsets != NULL);
    pool->groups = (HD_STATIC_GROUP *)calloc(maximum_groups, sizeof(*pool->groups));
    pool->geometry = vid_mesh_create_3d(total_vertices);
    pool->indices = vid_index_create(total_indices);
    XASSERT(pool->groups != NULL && pool->geometry != NULL && pool->indices != NULL);
    vertices = vid_mesh_data_3d(pool->geometry);
    indices = vid_index_data(pool->indices);
    for (item = 0; item < count; ++item)
    {
        HD_MODEL *model = models[item];
        const MODEL_HEADER *header;
        uint32 vertex, source;
        float position[3];
        if (model == NULL || model->kind >= 10 || !model->source_ready || model->static_pool != NULL)
            continue;
        header = (const MODEL_HEADER *)model->source;
        for (vertex = 0; vertex < 3; ++vertex)
            position[vertex] = (float)(header->position.v[vertex] >> 8);
        if (cursor == 0)
            pool->origin = static_pools != NULL ? static_pools->origin : (VEC3F){floorf(position[0] / 1024) * 1024, floorf(position[1] / 1024) * 1024, floorf(position[2] / 1024) * 1024};
        offsets[item] = cursor;
        memcpy(vertices + cursor, model->batch_vertices, model->batch_vertex_count * sizeof(*vertices));
        for (vertex = 0; vertex < model->batch_vertex_count; ++vertex)
        {
            vertices[cursor + vertex].pos[0] += position[0] - pool->origin.x;
            vertices[cursor + vertex].pos[1] += position[1] - pool->origin.y;
            vertices[cursor + vertex].pos[2] += position[2] - pool->origin.z;
        }
        cursor += model->batch_vertex_count;
        for (source = 0; source < model->source_group_count; ++source)
        {
            HD_CACHED_GROUP *part = &model->source_groups[source];
            HD_STATIC_GROUP *target;
            for (group = 0; group < pool->group_count; ++group)
                if (pool->groups[group].batch.texture == part->texture && pool->groups[group].transparent == part->transparent && pool->groups[group].category == (item >= main_count) && pool->groups[group].lighting == model->has_lighting)
                    break;
            if (group == pool->group_count)
            {
                target = &pool->groups[pool->group_count++];
                target->batch.texture = part->texture;
                target->batch.material = source_material(part->texture, 0, 2, part->transparent)->batch_material;
                target->batch.geometry = pool->geometry;
                target->batch.index_buffer = pool->indices;
                target->batch.world_space = 1;
                target->batch.origin = pool->origin;
                target->category = item >= main_count;
                target->lighting = model->has_lighting;
                target->transparent = part->transparent;
            }
            target = &pool->groups[group];
            target->batch.count += part->count;
            part->static_group = target;
        }
        model->static_pool = pool;
    }
    XASSERT(cursor == total_vertices);
    cursor = 0;
    for (group = 0; group < pool->group_count; ++group)
    {
        pool->groups[group].batch.first = cursor;
        pool->groups[group].cursor = cursor;
        cursor += pool->groups[group].batch.count;
    }
    XASSERT(cursor == total_indices);
    for (item = 0; item < count; ++item)
    {
        HD_MODEL *model = models[item];
        uint32 range;
        if (model == NULL || model->static_pool != pool)
            continue;
        for (range = 0; range < model->range_count; ++range)
        {
            HD_MODEL_RANGE *part = &model->ranges[range];
            HD_SOURCE_GPU image = hd_texture_source_gpu(part->source_texture);
            HD_STATIC_GROUP *target = NULL;
            uint32 source, index;
            for (source = 0; source < model->source_group_count; ++source)
                if (model->source_groups[source].texture == image.texture)
                    target = model->source_groups[source].static_group;
            XASSERT(target != NULL);
            for (index = 0; index < part->count; ++index)
                indices[target->cursor + index] = offsets[item] + model->batch_source_indices[part->source_first + index];
            part->source_first = target->cursor;
            target->cursor += part->count;
        }
        vid_mesh_free(model->geometry);
        vid_index_free(model->indices);
        model->geometry = NULL;
        model->indices = NULL;
        free(model->batch_vertices);
        free(model->batch_source_indices);
        model->batch_vertices = NULL;
        model->batch_source_indices = NULL;
    }
    for (group = 0; group < pool->group_count; ++group)
        XASSERT(pool->groups[group].cursor == pool->groups[group].batch.first + pool->groups[group].batch.count);
    free(offsets);
    vid_mesh_commit(pool->geometry);
    vid_index_commit(pool->indices, total_indices);
    vid_mesh_static(pool->geometry);
    vid_index_static(pool->indices);
    if (preparation_active)
        for (group = 0; group < pool->group_count; ++group)
        {
            HD_SCENE_BATCH_GROUP *batch = &pool->groups[group].batch;
            vid_pass(batch->material);
            vid_submit(pool->geometry, pool->indices, batch->first, batch->count);
        }
    pool->next = static_pools;
    static_pools = pool;
}

static void draw_model_snapshot(HD_MODEL *model)
{
    uint32 group, count = model->uses_sources ? model->source_group_count + (model->has_transparent != 0) : 1;
    XASSERT(!model->uses_sources || model->source_ready);
    for (group = 0; group < count; ++group)
    {
        HD_MODEL_DRAW *draw = reserve_model_draw(model, 0);
        if (model->uses_sources)
        {
            draw->source_order = group < model->source_group_count ? 1 : 2;
            if (draw->source_order == 1)
            {
                draw->source_group = &model->source_groups[group];
                draw->texture_transparent = (uint8)draw->source_group->transparent;
            }
        }
        if (model->visibility == NULL && model->kind >= 10)
            memcpy(draw->transform.m, hd.billboard.m, sizeof(draw->transform.m));
        if (model->visibility == NULL && model->kind == 12)
            draw_sun(draw, group == 0);
        else if (model->visibility == NULL && model->kind >= 10)
            capture_facing(draw);
    }
}

static const MODEL_RESOURCE *high_detail_car_resource(const MODEL_RESOURCE *resource)
{
    if (hd.actor >= 0 && hd.actor < 14)
    {
        const MODEL_RESOURCE *models = car_states[hd.actor].render_models;
        if (resource->bytes == models[1].bytes && models[0].bytes != NULL)
            return &models[0];
    }
    return resource;
}

static void draw_model(const MODEL_RESOURCE *resource, const sint8 *visibility)
{
    HD_MODEL *model;
    if (!hd_capture_active() || !hd.collecting)
        return;
    if (visibility == NULL)
        resource = high_detail_car_resource(resource);
    model = find_model(resource, visibility, 0);
    if (model == NULL)
        model = create_model(resource, visibility, 0);
    if (model == NULL)
        return;
    draw_model_snapshot(model);
}

void hd_draw_model(const MODEL_RESOURCE *resource)
{
    if (original_track_suppressed)
        return;
    draw_model(resource, NULL);
}

void hd_draw_sky(const MODEL_RESOURCE *resource, const sint8 *visibility)
{
    draw_model(resource, visibility);
}

static sint32 sphere_outside(float x, float z, float a, float b, float radius)
{
    return a * x + b * z > radius * sqrtf(a * a + b * b);
}

static void static_candidate(const MODEL_RESOURCE *resource, const sint8 *sky_visibility, HD_MODEL *prepared, sint32 cull)
{
    const MODEL_HEADER *header;
    const HD_VIEW *view;
    VECTORF relative, center;
    float aspect, x_scale, y_scale, center_x, center_y, radius;
    MATRIXF saved_transform;
    HD_POSE saved_pose;
    sint32 saved_relative;
    if (!hd_capture_active() || original_track_suppressed)
        return;
    XASSERT(resource->capacity >= sizeof(*header));
    header = (const MODEL_HEADER *)resource->bytes;
    view = &hd.viewports[hd.view];
    if (view->viewport.w <= 0 || view->viewport.h <= 0)
        return;
    if (cull)
    {
        relative.vx = header->position.x / 256.0f + header->origin.x;
        relative.vy = header->position.y / 256.0f + header->origin.y;
        relative.vz = header->position.z / 256.0f + header->origin.z;
        if (sky_visibility == NULL)
        {
            relative.vx -= hd.camera_position.vx;
            relative.vy -= hd.camera_position.vy;
            relative.vz -= hd.camera_position.vz;
        }
        ApplyMatrixf(&hd.camera, &relative, &center);
        radius = (float)header->radius;
        if (center.vz + radius <= 1.0f || center.vz - radius >= hd_far_plane())
            return;
        aspect = (float)view->viewport.w / view->viewport.h;
        x_scale = 2.0f * model_render.proj_distance * model_render.x_scale / 4096.0f * model_render.scale / 4096.0f / view->reference.w * view->reference_aspect / aspect;
        y_scale = 2.0f * model_render.proj_distance * model_render.y_scale / 4096.0f * model_render.scale / 4096.0f / view->reference.h;
        center_x = 2.0f * ((sint32)model_render.view_center_x - view->reference.x) / view->reference.w - 1.0f;
        center_y = 1.0f - 2.0f * ((sint32)model_render.view_center_y - view->reference.y) / view->reference.h;
        if (sphere_outside(center.vx, center.vz, x_scale, center_x - 1.0f, radius) || sphere_outside(center.vx, center.vz, -x_scale, -center_x - 1.0f, radius) || sphere_outside(center.vy, center.vz, -y_scale, center_y - 1.0f, radius) || sphere_outside(center.vy, center.vz, y_scale, -center_y - 1.0f, radius))
            return;
    }
    saved_transform = hd.transform;
    saved_pose = hd.object_pose;
    saved_relative = hd.object_relative;
    hd_transform_model(NULL, &header->position, sky_visibility != NULL);
    if (prepared != NULL)
        draw_model_snapshot(prepared);
    else
        draw_model(resource, sky_visibility);
    hd.transform = saved_transform;
    hd.object_pose = saved_pose;
    hd.object_relative = saved_relative;
}

void hd_draw_static_candidate(const MODEL_RESOURCE *resource, const sint8 *sky_visibility)
{
    static_candidate(resource, sky_visibility, NULL, 1);
}

void hd_draw_prepared_static(HD_PREPARED_MODEL *model, sint32 cull)
{
    MODEL_RESOURCE resource;
    if (model == NULL)
        return;
    resource = (MODEL_RESOURCE){(uint8 *)model->source, model->source_size};
    static_candidate(&resource, NULL, model, cull || model->kind >= 10);
}

typedef struct HD_LIGHT_ZONE
{
    sint16 first, last;
    SVECTOR direction;
    uint8 color[3], unused0, ambient[3], unused1;
    sint16 lamp_origin;
    uint16 height;
    uint8 kind, step, selector, attenuation;
    uint32 environment;
} HD_LIGHT_ZONE;

typedef struct HD_LIGHT_NODE
{
    uint8 unused[3], lane;
    uint16 first, last;
    uint32 unused_word;
    VEC3 origin;
} HD_LIGHT_NODE;

typedef char HD_LIGHT_SOURCE_LAYOUT_CHECK[sizeof(HD_LIGHT_ZONE) == 32 && sizeof(HD_LIGHT_NODE) == 24 ? 1 : -1];

static HD_LIGHT_ZONE lighting_zone(uint32 index)
{
    HD_LIGHT_ZONE zone;
    uint64 offset = (uint64)car_lighting_zones.offset + (uint64)index * sizeof(zone);
    XASSERT(index < car_lighting_zones.count && offset <= car_lighting_zones.size && sizeof(zone) <= car_lighting_zones.size - offset);
    memcpy(&zone, car_lighting_zones.bytes + (size_t)offset, sizeof(zone));
    return zone;
}

static HD_LIGHT_NODE lighting_node(sint32 index, sint32 wrap)
{
    HD_LIGHT_NODE node;
    if (wrap && (index < 0 || (uint32)index >= track_geometry.node_count))
    {
        XASSERT(track_geometry.main_node_count != 0);
        index %= (sint32)track_geometry.main_node_count;
        if (index < 0)
            index += (sint32)track_geometry.main_node_count;
    }
    XASSERT(index >= 0 && (uint32)index < track_geometry.node_count);
    memcpy(&node, track_geometry.node_bytes + (size_t)index * sizeof(node), sizeof(node));
    return node;
}

static VEC3_16 lighting_vertex(uint32 index)
{
    VEC3_16 vertex;
    XASSERT(index < track_geometry.vertex_count);
    memcpy(&vertex, track_geometry.vertex_bytes + (size_t)index * sizeof(vertex), sizeof(vertex));
    return vertex;
}

static float lighting_delta(float value)
{
    return value - floorf((value + 32768.0f) / 65536.0f) * 65536.0f;
}

static void lighting_direction(HD_LIGHT_SAMPLE *field, uint32 index, const SVECTOR *direction)
{
    field->world.m[index][0] = direction->vx / 4096.0f;
    field->world.m[index][1] = direction->vy / 4096.0f;
    field->world.m[index][2] = direction->vz / 4096.0f;
}

static void lighting_boundary(const CAR_STATE *car, uint32 index, const HD_LIGHT_ZONE *zone, sint32 end, float width, HD_LIGHT_SAMPLE *field)
{
    HD_LIGHT_NODE node = lighting_node(end ? zone->last + 1 : zone->first, 0);
    HD_LIGHT_ZONE neighbor = lighting_zone(end ? (index + 1) % car_lighting_zones.count : (index + car_lighting_zones.count - 1) % car_lighting_zones.count);
    VEC3_16 first = lighting_vertex(node.first), last;
    float nx, nz, length, dx, dz, weight;
    uint32 channel;
    XASSERT(node.last != 0 && width > 0.0f);
    last = lighting_vertex(node.last - 1);
    nx = lighting_delta((float)first.z - last.z);
    nz = lighting_delta((float)last.x - first.x);
    length = sqrtf(nx * nx + nz * nz);
    if (length > 0.0f)
    {
        nx /= length;
        nz /= length;
    }
    dx = lighting_delta(car->pos.x / 256.0f - last.x - node.origin.x);
    dz = lighting_delta(car->pos.z / 256.0f - last.z - node.origin.z);
    weight = fminf(1.0f, fmaxf(0.0f, (end ? 1.0f : -1.0f) * (nx * dx + nz * dz) / width));
    for (channel = 0; channel < 3; ++channel)
    {
        field->ambient[channel] = (zone->ambient[channel] * weight + neighbor.ambient[channel] * (1.0f - weight)) / 128.0f;
        field->color.m[channel][0] = neighbor.color[channel] * (1.0f - weight) / 128.0f;
    }
    lighting_direction(field, 0, &neighbor.direction);
    lighting_direction(field, 1, &zone->direction);
    lighting_direction(field, 2, &zone->direction);
}

static VECTORF lighting_point(const HD_LIGHT_NODE *node, uint32 index)
{
    VEC3_16 vertex = lighting_vertex(index);
    return (VECTORF){(float)node->origin.x + vertex.x, (float)node->origin.y + vertex.y, (float)node->origin.z + vertex.z};
}

static sint32 car_lighting_field(const CAR_STATE *car, HD_LIGHT_SAMPLE *field)
{
    uint32 index = car->lighting_zone, light, channel;
    sint32 at = (sint16)car->track_location.node, rounded, span;
    HD_LIGHT_ZONE zone = lighting_zone(index);
    memset(field, 0, sizeof(*field));
    while (at < zone.first)
    {
        XASSERT(index != 0);
        zone = lighting_zone(--index);
    }
    while (at > zone.last)
        zone = lighting_zone(++index);
    if (zone.kind > 3)
        return 0;
    for (channel = 0; channel < 3; ++channel)
        field->ambient[channel] = zone.ambient[channel] / 128.0f;
    if (zone.kind == 0)
    {
        for (light = 0; light < 3; ++light)
            lighting_direction(field, light, &zone.direction);
        for (channel = 0; channel < 3; ++channel)
            field->color.m[channel][0] = zone.color[channel] / 128.0f;
        return 1;
    }
    span = zone.last - zone.first + 1;
    if (zone.kind == 3)
    {
        lighting_boundary(car, index, &zone, at - zone.first >= span / 2, span * 512.0f, field);
        return 1;
    }
    if (at - zone.first < 3 || zone.last - at < 3)
    {
        lighting_boundary(car, index, &zone, at - zone.first >= 3, 2048.0f, field);
        return 1;
    }
    XASSERT(zone.step != 0);
    rounded = ((at - zone.lamp_origin + zone.step / 2) / zone.step) * zone.step + zone.lamp_origin;
    for (light = 0; light < (zone.kind == 1 ? 2u : 3u); ++light)
    {
        HD_LIGHT_NODE node = lighting_node(rounded + (zone.kind == 1 ? 0 : ((sint32)light - 1) * zone.step), 1);
        VECTORF position = {0}, direction;
        float square, length, strength;
        if (zone.kind == 1)
            position = lighting_point(&node, node.first + (light == 0 ? 0 : node.lane & 15));
        else if (zone.selector < 3)
        {
            if (zone.selector == 1)
            {
                XASSERT(node.last != 0);
                position = lighting_point(&node, node.last - 1);
            }
            else
            {
                position = lighting_point(&node, node.first);
                if (zone.selector == 2)
                {
                    VECTORF other;
                    XASSERT(node.last != 0);
                    other = lighting_point(&node, node.last - 1);
                    position.vx = (position.vx + other.vx) * 0.5f;
                    position.vy = (position.vy + other.vy) * 0.5f;
                    position.vz = (position.vz + other.vz) * 0.5f;
                }
            }
        }
        direction = (VECTORF){lighting_delta(position.vx - car->pos.x / 256.0f), lighting_delta(position.vy - car->pos.y / 256.0f + zone.height), lighting_delta(position.vz - car->pos.z / 256.0f)};
        square = direction.vx * direction.vx + direction.vy * direction.vy + direction.vz * direction.vz;
        length = sqrtf(square);
        strength = fmaxf(0.0f, 1.0f - (float)fmod(square * zone.attenuation, 4294967296.0) / 67108864.0f);
        if (length > 0.0f)
        {
            field->world.m[light][0] = direction.vx / length;
            field->world.m[light][1] = direction.vy / length;
            field->world.m[light][2] = direction.vz / length;
        }
        for (channel = 0; channel < 3; ++channel)
            field->color.m[channel][light] = zone.color[channel] * strength / 128.0f;
    }
    if (zone.kind == 1)
        lighting_direction(field, 2, &zone.direction);
    return 1;
}

void hd_draw_car_candidate(uint32 slot, const MODEL_RESOURCE *resource, HD_CAR_FAMILY family)
{
    const CAR_STATE *car;
    const MODEL_HEADER *header;
    const HD_VIEW *view;
    MATRIXF saved_transform, saved_light, rotation;
    HD_POSE saved_pose;
    HD_LIGHT_SAMPLE field;
    const HD_LIGHT_SAMPLE *saved_override;
    VECTORF local, center;
    float saved_ambient[3], radius, aspect, scale_x, scale_y, center_x, center_y;
    sint32 saved_relative;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 14 && hd.actor == (sint32)slot && resource->capacity >= sizeof(*header) && family <= HD_CAR_SPLIT);
    car = &car_states[slot];
    resource = high_detail_car_resource(resource);
    header = (const MODEL_HEADER *)resource->bytes;
    view = &hd.viewports[hd.view];
    if (view->viewport.w <= 0 || view->viewport.h <= 0)
        return;
    saved_transform = hd.transform;
    saved_light = hd.light;
    saved_pose = hd.object_pose;
    saved_relative = hd.object_relative;
    saved_override = hd.light_override;
    memcpy(saved_ambient, hd.ambient, sizeof(saved_ambient));
    hd_transform_model(&car->transform, &car->pos, 0);
    local = (VECTORF){(float)header->origin.x, (float)header->origin.y, (float)header->origin.z};
    RotTransf(&hd.transform, &local, &center);
    radius = (float)header->radius;
    aspect = (float)view->viewport.w / view->viewport.h;
    scale_x = 2.0f * model_render.proj_distance * model_render.x_scale / 4096.0f * model_render.scale / 4096.0f / view->reference.w * view->reference_aspect / aspect;
    scale_y = 2.0f * model_render.proj_distance * model_render.y_scale / 4096.0f * model_render.scale / 4096.0f / view->reference.h;
    center_x = 2.0f * ((sint32)model_render.view_center_x - view->reference.x) / view->reference.w - 1.0f;
    center_y = 1.0f - 2.0f * ((sint32)model_render.view_center_y - view->reference.y) / view->reference.h;
    if (center.vz + radius > 1.0f && center.vz - radius < hd_far_plane() && !sphere_outside(center.vx, center.vz, scale_x, center_x - 1.0f, radius) && !sphere_outside(center.vx, center.vz, -scale_x, -center_x - 1.0f, radius) && !sphere_outside(center.vy, center.vz, -scale_y, center_y - 1.0f, radius) && !sphere_outside(center.vy, center.vz, scale_y, -center_y - 1.0f, radius))
    {
        float police_signal = 0.0f;
        if ((family == HD_CAR_RACING || family == HD_CAR_TRAFFIC) && car_lighting_field(car, &field))
        {
            matrix_from_native(&car->transform, &rotation);
            MulMatrix0f(&field.world, &rotation, &hd.light);
            memcpy(hd.ambient, field.ambient, sizeof(hd.ambient));
            hd.light_override = &field;
        }
        if (family == HD_CAR_PRIMARY && (scene_level_image.bytes[scene_level_image.desc.header_offset + 30] & 1) == 0)
            car_environment_candidate(resource, car);
        draw_model(resource, NULL);
        if (family == HD_CAR_RACING || (family == HD_CAR_SPLIT && race_session.multiplayer_mode == 3 && slot != chase_state.pursuer_slot))
            chase_draw_hd_health(slot);
        if ((family == HD_CAR_PRIMARY && race_session.cop_chase != 0) || (family == HD_CAR_SPLIT && race_session.multiplayer_mode == 3 && slot == chase_state.pursuer_slot))
            police_signal = (float)(sint16)chase_state.signal_level;
        if (family == HD_CAR_TRAFFIC && slot == 9 && race_ai_cars[9].chase_target != 0xFFFFFFFF)
            police_signal = 4096.0f;
        car_draw_hd_candidate_lamps(slot, police_signal);
        if (family != HD_CAR_TRAFFIC)
            wheels_draw_hd_race(slot);
        car_draw_hd_shadow(slot, car->motion_mode != 0 ? 2 : family == HD_CAR_TRAFFIC ? 1 : 0);
    }
    hd.transform = saved_transform;
    hd.light = saved_light;
    hd.object_pose = saved_pose;
    hd.object_relative = saved_relative;
    hd.light_override = saved_override;
    memcpy(hd.ambient, saved_ambient, sizeof(hd.ambient));
}

static HD_MODEL_DRAW *environment_draw(const MODEL_RESOURCE *resource, uint32 variant)
{
    HD_MODEL *model = find_model(resource, NULL, variant);
    const CVECTOR *tint = hd.environment_tint_override != NULL ? hd.environment_tint_override : &car_environment_render.tint;
    HD_MODEL_DRAW *draw;
    TEXTURE_DESCRIPTOR texture;
    if (!hd_capture_active() || !hd.collecting)
        return NULL;
    if (model == NULL)
        model = create_model(resource, NULL, variant);
    if (model == NULL)
        return NULL;
    memcpy(&texture, car_environment_render.tex, sizeof(texture));
    draw = reserve_model_draw(model, 0);
    draw->dynamic = 3;
    draw->first = 0;
    draw->count = model->index_count;
    draw->blend = 1 + ((texture.page >> 5) & 3);
    draw->environment[0][0] = variant == 1 ? 1.0f : 2.0f;
    draw->environment[0][1] = (float)tint->r;
    draw->environment[0][2] = (float)tint->g;
    draw->environment[0][3] = (float)tint->b;
    draw->environment[5][0] = (float)texture.u;
    draw->environment[5][1] = (float)texture.v;
    draw->environment[5][2] = texture.width != 0 ? (float)texture.width : 256.0f;
    draw->environment[5][3] = texture.height != 0 ? (float)texture.height : 256.0f;
    draw->environment[6][0] = (float)texture.page;
    draw->environment[6][1] = (float)texture.clut;
    draw->environment[6][2] = draw->focal * draw->scale_x;
    draw->environment[6][3] = draw->focal * draw->scale_y;
    return draw;
}

static float wrapped_previous(float previous, float current, float period)
{
    float delta = current - previous;
    if (period > 0.0f)
        delta -= floorf((delta + period * 0.5f) / period) * period;
    return current - delta;
}

void hd_minimap_begin(const CAR_STATE *car, float scale_x, float scale_z)
{
    HD_MAP_SAMPLE sample;
    HD_MAP_MOTION *motion;
    uint32 slot, owner;
    if (!hd_capture_active())
        return;
    owner = (uint32)(car - car_states);
    XASSERT(owner < 14 && scale_x != 0.0f && scale_z != 0.0f);
    motion = &hd.maps[hd.view];
    for (slot = 0; slot < 14; ++slot)
        sample.cars[slot] = (VECTORF){car_states[slot].pos.x / 256.0f, 0.0f, car_states[slot].pos.z / 256.0f};
    sample.origin = sample.cars[0];
    sample.angle = (2048.0f - car->orientation.y / 256.0f) * RAD12;
    if (motion->serial != hd.motion_serial || motion->generation != hd.motion_generation || motion->owner != owner)
    {
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation || motion->owner != owner)
            motion->previous = sample;
        else
        {
            motion->previous = motion->current;
            motion->previous.angle = wrapped_previous(motion->previous.angle, sample.angle, 2.0f * 3.14159265358979323846f);
        }
        motion->current = sample;
        motion->serial = hd.motion_serial;
        motion->generation = hd.motion_generation;
        motion->owner = owner;
        for (slot = 0; slot < 14; ++slot)
            if (motion->reset_slots & (1u << slot))
                motion->previous.cars[slot] = sample.cars[slot];
        if (motion->reset_slots & 1u)
            motion->previous.origin = sample.origin;
        if (motion->reset_slots & (1u << owner))
            motion->previous.angle = sample.angle;
        motion->reset_slots = 0;
    }
    hd.map_scale[0] = scale_x;
    hd.map_scale[1] = scale_z;
}

static void minimap_geometry(const VECTORF *points, const RGBC *color, uint32 mode, uint32 slot)
{
    static const uint8 order[6] = {0, 1, 2, 1, 3, 2};
    const HD_MAP_MOTION *motion = &hd.maps[hd.view];
    HD_UI_DRAW *draw;
    VID_VERTEX2D *vertices;
    uint32 vertex, blend;
    if (!hd_capture_active())
        return;
    XASSERT(hd.ui_clipped && hd.ui_anchor == -1 && motion->serial == hd.motion_serial && slot < 14);
    blend = (color->code & 2) != 0 ? 1 : 0;
    draw = reserve_ui(6, blend);
    draw->map.origin[0] = motion->previous.origin;
    draw->map.origin[1] = motion->current.origin;
    draw->map.angles[0] = motion->previous.angle;
    draw->map.angles[1] = motion->current.angle;
    draw->map.marker[0] = motion->previous.cars[slot];
    draw->map.marker[1] = motion->current.cars[slot];
    memcpy(draw->map.scale, hd.map_scale, sizeof(draw->map.scale));
    memcpy(draw->map.offset, hd.ui_offset, sizeof(draw->map.offset));
    draw->map.clip = hd.ui_clip;
    draw->map.mode = (sint32)mode;
    draw->map.active = 1;
    vertices = vid_mesh_data_2d(draw->mesh->geometry) + draw->mesh->used;
    memset(vertices, 0, 6 * sizeof(*vertices));
    for (vertex = 0; vertex < 6; ++vertex)
    {
        uint32 corner = order[vertex];
        if (mode == 1)
        {
            vertices[vertex].pos[0] = (corner & 1) != 0 ? 2.0f : -1.0f;
            vertices[vertex].pos[1] = (corner & 2) != 0 ? 1.0f : -1.0f;
        }
        else
        {
            uint32 point = mode == 2 ? corner & 1 : corner;
            vertices[vertex].pos[0] = points[point].vx;
            vertices[vertex].pos[1] = points[point].vz;
            if (mode == 2)
            {
                vertices[vertex].pos[2] = points[point ^ 1].vx;
                vertices[vertex].pos[3] = points[point ^ 1].vz;
                vertices[vertex].uv[0] = ((corner & 2) != 0 ? 0.5f : -0.5f) * (point == 0 ? 1.0f : -1.0f);
            }
        }
        vertices[vertex].alpha = 1.0f;
        memcpy(vertices[vertex].color, color, sizeof(*color));
    }
    draw->mesh->used += 6;
}

void hd_minimap_quad(const VECTORF points[4], const RGBC *color)
{
    minimap_geometry(points, color, 0, 0);
}

void hd_minimap_line(const VECTORF *start, const VECTORF *end, const RGBC *color)
{
    VECTORF points[2] = {*start, *end};
    minimap_geometry(points, color, 2, 0);
}

void hd_minimap_car(uint32 slot, const RGBC *color)
{
    minimap_geometry(NULL, color, 1, slot);
}

static void capture_environment(HD_MODEL_DRAW *draw, HD_ENV_SAMPLE sample, uint32 kind)
{
    uint32 actor = hd.actor >= 0 ? (uint32)hd.actor : 14;
    HD_ENV_MOTION *motion = &hd.environment_motion[hd.view][actor];
    uint64 camera_key = hd.camera_motion[hd.view].key;
    float period_u = draw->environment[2][0] * 8.0f, period_v = draw->environment[2][1] * 8.0f;
    uint32 channel;
    sample.angle -= floorf((sample.angle + 3.14159265358979323846f) / (2.0f * 3.14159265358979323846f)) * (2.0f * 3.14159265358979323846f);
    if (period_u > 0.0f)
        sample.u -= floorf(sample.u / period_u) * period_u;
    if (period_v > 0.0f)
        sample.v -= floorf(sample.v / period_v) * period_v;
    for (channel = 0; channel < 3; ++channel)
        sample.tint[channel] = draw->environment[0][channel + 1];
    if (motion->serial != hd.motion_serial || motion->generation != hd.motion_generation || motion->kind != kind || motion->source != draw->model->source || motion->texture != car_environment_render.tex || motion->camera_key != camera_key)
    {
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation || motion->kind != kind || motion->source != draw->model->source || motion->texture != car_environment_render.tex || motion->camera_key != camera_key)
            motion->previous = sample;
        else
        {
            motion->previous = motion->current;
            motion->previous.angle = wrapped_previous(motion->previous.angle, sample.angle, 2.0f * 3.14159265358979323846f);
            motion->previous.u = wrapped_previous(motion->previous.u, sample.u, period_u);
            motion->previous.v = wrapped_previous(motion->previous.v, sample.v, period_v);
        }
        motion->current = sample;
        motion->serial = hd.motion_serial;
        motion->generation = hd.motion_generation;
        motion->kind = kind;
        motion->source = draw->model->source;
        motion->texture = car_environment_render.tex;
        motion->camera_key = camera_key;
    }
    draw->environment_samples[0] = motion->previous;
    draw->environment_samples[1] = motion->current;
}

void hd_environment_reflection(const MODEL_RESOURCE *resource, const CAR_ENVIRONMENT_MAPPING *mapping, sint32 center_on_model)
{
    HD_MODEL_DRAW *draw = environment_draw(resource, 1);
    HD_ENV_SAMPLE sample = {0};
    float normal_scale;
    if (draw == NULL)
        return;
    normal_scale = mapping->n_scale / (4096.0f * (float)(1u << model_render.coord_shift));
    draw->environment[4][0] = model_render.x_scale / 4096.0f * normal_scale;
    draw->environment[4][1] = model_render.y_scale / 4096.0f * normal_scale;
    draw->environment[4][2] = (float)mapping->tex_bias;
    draw->reflection_scale[0] = mapping->scale_x * (model_render.x_scale / 16777216.0f);
    draw->reflection_scale[1] = mapping->scale_y * (model_render.y_scale / 16777216.0f);
    draw->reflection_centered = center_on_model;
    sample.center[0] = (float)mapping->center_x;
    sample.center[1] = (float)mapping->center_y;
    capture_environment(draw, sample, 0);
}

void hd_environment_surface(const MODEL_RESOURCE *resource, float angle, float origin_u, float origin_v, uint32 period_u, uint32 period_v)
{
    HD_MODEL_DRAW *draw = environment_draw(resource, 2);
    HD_ENV_SAMPLE sample = {0};
    if (draw == NULL)
        return;
    draw->environment[2][0] = (float)period_u;
    draw->environment[2][1] = (float)period_v;
    sample.angle = angle;
    sample.u = origin_u;
    sample.v = origin_v;
    capture_environment(draw, sample, period_u == 128 ? 1 : period_u == 64 ? 2 : 3);
}

static void car_environment_candidate(const MODEL_RESOURCE *resource, const CAR_STATE *car)
{
    const CVECTOR *previous = hd.environment_tint_override;
    CVECTOR tint = {(uint8)car->environment.level, (uint8)car->environment.level, (uint8)car->environment.level, 0};
    hd.environment_tint_override = &tint;
    if (car->environment.type == 0)
        hd_environment_reflection(resource, &car_environment_render.mapping, 1);
    else
        hd_environment_surface(resource, car->environment.type == 2 ? -car->orientation.y / 256.0f * RAD12 : 0.0f, car->environment.type == 1 ? 512.0f : car->environment.type == 2 ? car->pos.x / 8192.0f : 0.0f, car->environment.type == 1 ? (sint32)car_environment_render.reflection_phase / 256.0f : car->environment.type == 2 ? car->pos.z / 8192.0f : 0.0f, car->environment.type == 1 ? 128 : car->environment.type == 2 ? 64 : 0, car->environment.type == 1 || car->environment.type == 2 ? 64 : 0);
    hd.environment_tint_override = previous;
}

void hd_set_back_color(sint32 r, sint32 g, sint32 b)
{
    hd.ambient[0] = r / 128.0f;
    hd.ambient[1] = g / 128.0f;
    hd.ambient[2] = b / 128.0f;
}

void hd_world_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4], sint32 view_space)
{
    static const uint8 order[6] = {0, 1, 2, 1, 3, 2};
    HD_WORLD_MESH **cursor = &hd.world_meshes, *mesh;
    HD_MODEL_DRAW *draw;
    VID_VERTEX2D *vertices;
    uint32 vertex, source;
    sint32 actor;
    if (!hd_capture_active() || !hd.collecting)
        return;
    while (*cursor != NULL && (*cursor)->used + 6 > HD_UI_MESH_VERTICES)
        cursor = &(*cursor)->next;
    mesh = *cursor;
    if (mesh == NULL)
    {
        uint32 *indices;
        mesh = (HD_WORLD_MESH *)calloc(1, sizeof(*mesh));
        XASSERT(mesh != NULL);
        mesh->model.geometry = vid_mesh_create_2d(HD_UI_MESH_VERTICES);
        mesh->model.indices = vid_index_create(HD_UI_MESH_VERTICES);
        XASSERT(mesh->model.geometry != NULL && mesh->model.indices != NULL);
        indices = vid_index_data(mesh->model.indices);
        for (vertex = 0; vertex < HD_UI_MESH_VERTICES; ++vertex)
            indices[vertex] = vertex;
        vid_index_commit(mesh->model.indices, HD_UI_MESH_VERTICES);
        vid_index_static(mesh->model.indices);
        *cursor = mesh;
    }
    vertices = vid_mesh_data_2d(mesh->model.geometry) + mesh->used;
    for (vertex = 0; vertex < 6; ++vertex)
    {
        VECTORF position;
        source = order[vertex];
        if (view_space)
            position = points[source];
        else
        {
            VECTORF relative = {points[source].vx - hd.camera_position.vx, points[source].vy - hd.camera_position.vy, points[source].vz - hd.camera_position.vz};
            ApplyMatrixf(&hd.camera, &relative, &position);
        }
        vertices[vertex].pos[0] = position.vx;
        vertices[vertex].pos[1] = position.vy;
        vertices[vertex].pos[2] = position.vz;
        vertices[vertex].pos[3] = 0.0f;
        vertices[vertex].uv[0] = (source & 1) != 0 ? texture->width * 2.0f : 0.0f;
        vertices[vertex].uv[1] = (source & 2) != 0 ? texture->height * 2.0f : 0.0f;
        vertices[vertex].alpha = 1.0f;
        memcpy(vertices[vertex].color, colors + source, 4);
        vertices[vertex].tex[0] = texture->page;
        vertices[vertex].tex[1] = texture->clut;
        vertices[vertex].tex[2] = texture->flags;
        vertices[vertex].tex[3] = texture->u | (texture->v << 8);
        vertices[vertex].rect[0] = texture->width;
        vertices[vertex].rect[1] = texture->height;
    }
    actor = hd.actor;
    if (!view_space)
        hd.actor = -1;
    draw = reserve_model_draw(&mesh->model, 1);
    hd.actor = actor;
    if (!view_space && actor >= 0)
    {
        const HD_MOTION *motion = &hd.car_motion[hd.view][actor];
        draw->poses.object[0] = motion->previous;
        draw->poses.object[1] = motion->current;
        draw->poses.world_translation = 1;
    }
    IdentityMatrixf(&draw->transform);
    draw->first = mesh->used;
    draw->depth_offset = (texture->flags & HD_LAMP_GLOW) != 0 ? 160.0f : 0.0f;
    draw->count = 6;
    draw->blend = (texture->flags & HD_SEMITRANSPARENT) != 0 ? 1 + ((texture->page >> 5) & 3) : 0;
    draw->dynamic = 1;
    mesh->used += 6;
}

HD_TEXTURE hd_quad_texture(const POINT_T points[4], const RGBC *color)
{
    HD_TEXTURE texture = {points[1].attr, points[0].attr, points[0].uv.u, points[0].uv.v, (uint16)((uint8)(points[1].uv.u - points[0].uv.u) + 1), (uint16)((uint8)(points[2].uv.v - points[0].uv.v) + 1), HD_TEXTURED | HD_FILTERED};
    if ((color->code & 1) != 0)
        texture.flags |= HD_RAW_TEXTURE;
    if ((color->code & 2) != 0)
        texture.flags |= HD_SEMITRANSPARENT;
    return texture;
}

MATRIXF hd_model_transform(void)
{
    return hd.transform;
}

uint32 hd_ui_mark(void)
{
    return hd.ui_count;
}

void hd_ui_follow_car(uint32 first, uint32 slot, const SVECTOR *local)
{
    HD_UI_ATTACHMENT attachment;
    const HD_MOTION *camera, *object;
    VECTORF point;
    uint32 index;
    if (!hd_capture_active())
        return;
    XASSERT(first <= hd.ui_count && slot < 14);
    attachment.local = (VECTORF){(float)local->vx, (float)local->vy, (float)local->vz};
    RotTransf(&hd.transform, &attachment.local, &point);
    if (point.vz <= 1.0f)
        return;
    camera = &hd.camera_motion[hd.view];
    object = &hd.car_motion[hd.view][slot];
    XASSERT(camera->serial == hd.motion_serial && object->serial == hd.motion_serial);
    attachment.camera[0] = camera->previous;
    attachment.camera[1] = camera->current;
    attachment.object[0] = object->previous;
    attachment.object[1] = object->current;
    attachment.projection[0] = hd.projection_motion[hd.view].previous;
    attachment.projection[1] = hd.projection_motion[hd.view].current;
    attachment.source[0] = attachment.projection[1].center_x + point.vx * attachment.projection[1].x / point.vz;
    attachment.source[1] = attachment.projection[1].center_y + point.vy * attachment.projection[1].y / point.vz;
    attachment.active = 1;
    for (index = first; index < hd.ui_count; ++index)
        hd.ui_draws[index].attachment = attachment;
}

void hd_local_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4])
{
    VECTORF transformed[4];
    uint32 point;
    if (!hd_capture_active())
        return;
    for (point = 0; point < 4; ++point)
        RotTransf(&hd.transform, points + point, transformed + point);
    hd_world_quad(transformed, texture, colors, 1);
}

void hd_solid_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4], sint32 view_space)
{
    uint32 before = hd.model_count;
    hd_world_quad(points, texture, colors, view_space);
    if (hd.model_count != before)
        hd.model_draws[hd.model_count - 1].dynamic = 2;
}

void hd_road_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4])
{
    uint32 before = hd.model_count;
    hd_world_quad(points, texture, colors, 0);
    if (hd.model_count != before)
    {
        HD_MODEL_DRAW *draw = &hd.model_draws[hd.model_count - 1];
        draw->depth_bias = 0.0001f;
        // retain the original lamp projection's sixteen-OTZ foreground offset
        draw->depth_offset = 64.0f * (float)(1u << (model_render.coord_shift & 31));
    }
}

void hd_lit_wheel_quad(const VECTORF points[4], const HD_TEXTURE *texture, const RGBC colors[4], const SVECTOR *normal)
{
    uint32 before = hd.model_count;
    HD_MODEL_DRAW *draw;
    uint32 row;
    hd_solid_quad(points, texture, colors, 1);
    if (before == hd.model_count)
        return;
    draw = &hd.model_draws[hd.model_count - 1];
    draw->hub_lit = 1;
    draw->hub_normal = (VECTORF){normal->vx / 4096.0f, normal->vy / 4096.0f, normal->vz / 4096.0f};
    draw->hub_anchor = points[3];
    draw->hub_cue[0] = (float)(sint16)xport_gte_read_control(27);
    draw->hub_cue[1] = (float)(sint32)xport_gte_read_control(28);
    draw->hub_cue[2] = (float)model_render.proj_distance;
    draw->hub_cue[3] = (float)(1u << (model_render.coord_shift & 31));
    for (row = 0; row < 3; ++row)
        draw->hub_far[row] = (sint32)xport_gte_read_control(21 + row) / (16.0f * 255.0f);
    capture_lighting(draw);
}

static void draw_world_billboard(const VECTORF *position, float size, const HD_TEXTURE *texture, const RGBC *color)
{
    VECTORF points[4], center, relative = {position->vx - hd.camera_position.vx, position->vy - hd.camera_position.vy, position->vz - hd.camera_position.vz};
    RGBC colors[4] = {*color, *color, *color, *color};
    uint32 point;
    sint32 actor;
    if (!hd_capture_active())
        return;
    ApplyMatrixf(&hd.camera, &relative, &center);
    for (point = 0; point < 4; ++point)
    {
        VECTORF corner = {(point & 1) != 0 ? -size : size, (point & 2) != 0 ? size : -size, 0.0f};
        ApplyMatrixf(&hd.screen_rotation, &corner, &points[point]);
        points[point].vx += center.vx;
        points[point].vy += center.vy;
        points[point].vz += center.vz;
    }
    actor = hd.actor;
    hd.actor = -1;
    hd_world_quad(points, texture, colors, 1);
    hd.actor = actor;
    hd.model_draws[hd.model_count - 1].poses.centered_billboard = 1;
    hd.model_draws[hd.model_count - 1].poses.center = (VECTORF){position->vx, position->vy, position->vz};
}

void hd_world_billboard(const VEC3 *position, float size, const HD_TEXTURE *texture, const RGBC *color)
{
    VECTORF world = {position->x / 256.0f, position->y / 256.0f, position->z / 256.0f};
    draw_world_billboard(&world, size, texture, color);
}

static HD_PARTICLE_MOTION *sample_particle_motion(HD_PARTICLE_MOTION *motion, const HD_PARTICLE_SAMPLE *source)
{
    HD_PARTICLE_SAMPLE sample = *source;
    if (motion->serial != hd.motion_serial || motion->generation != hd.motion_generation)
    {
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation)
            motion->previous = sample;
        else
            motion->previous = motion->current;
        motion->current = sample;
        motion->serial = hd.motion_serial;
        motion->generation = hd.motion_generation;
    }
    return motion;
}

static HD_PARTICLE_MOTION *sample_particle(uint32 slot, const HD_PARTICLE_SAMPLE *source)
{
    XASSERT(slot < 50);
    return sample_particle_motion(&hd.particles[hd.view][slot], source);
}

void hd_particle_billboard(uint32 slot, const VEC3 *position, float size, const HD_TEXTURE *texture, const RGBC *color)
{
    HD_PARTICLE_MOTION *motion;
    HD_PARTICLE_SAMPLE sample;
    HD_DRAW_POSES *poses;
    if (!hd_capture_active() || size <= 0.0f)
        return;
    sample.position = (VECTORF){position->x / 256.0f, position->y / 256.0f, position->z / 256.0f};
    sample.size = size;
    sample.brightness = color->r / 255.0f;
    motion = sample_particle(slot, &sample);
    motion->texture = *texture;
    motion->color = *color;
    motion->screen = 0;
    hd_world_billboard(position, size, texture, color);
    poses = &hd.model_draws[hd.model_count - 1].poses;
    poses->particle[0] = motion->previous;
    poses->particle[1] = motion->current;
    poses->particle_active = 1;
}

static void draw_screen_particle(const HD_PARTICLE_SAMPLE *previous, const HD_PARTICLE_SAMPLE *current, float height_ratio, const HD_TEXTURE *texture, const RGBC *color, uint32 layer, sint32 retiring)
{
    HD_UI_VERTEX vertices[4];
    HD_TEXTURE filtered = *texture;
    uint32 point, before;
    filtered.flags |= HD_SCENE_TEXTURE;
    for (point = 0; point < 4; ++point)
    {
        vertices[point].x = current->position.vx + ((point & 1) != 0 ? current->size : 0.0f);
        vertices[point].y = current->position.vy + ((point & 2) != 0 ? current->size * height_ratio : 0.0f);
        vertices[point].u = texture->u + ((point & 1) != 0 ? texture->width - 1.0f : 0.0f);
        vertices[point].v = texture->v + ((point & 2) != 0 ? texture->height - 1.0f : 0.0f);
        memcpy(vertices[point].color, color, 4);
    }
    before = hd.ui_count;
    hd_ui_ordered_quad(vertices, &filtered, 1.0f, layer);
    if (hd.ui_count > before)
    {
        HD_UI_DRAW *draw = &hd.ui_draws[hd.ui_count - 1];
        draw->particle[0] = *previous;
        draw->particle[1] = *current;
        draw->particle_active = 1;
        draw->fading_out = retiring;
    }
}

void hd_screen_particle(uint32 slot, const VEC3 *position, float width, float height, const HD_TEXTURE *texture, const RGBC *color, uint32 layer)
{
    HD_PARTICLE_SAMPLE sample;
    HD_PARTICLE_MOTION *motion;
    if (!hd_capture_active() || width <= 0.0f || height <= 0.0f)
        return;
    sample.position = (VECTORF){(float)position->x, (float)position->y, 0.0f};
    sample.size = width;
    sample.brightness = color->r / 255.0f;
    motion = sample_particle(slot, &sample);
    motion->texture = *texture;
    motion->color = *color;
    motion->height_ratio = height / width;
    motion->layer = layer;
    motion->screen = 1;
    draw_screen_particle(&motion->previous, &motion->current, motion->height_ratio, texture, color, layer, 0);
}

void hd_weather_reset(uint32 slot)
{
    uint32 view;
    XASSERT(slot < 128);
    for (view = 0; view < HD_VIEWS; ++view)
    {
        hd.snow[view][slot].motion.generation = ~hd.motion_generation;
        hd.rain[view][slot].generation = ~hd.motion_generation;
    }
}

void hd_weather_snow(uint32 slot, const HD_UI_VERTEX points[4], const HD_TEXTURE *texture)
{
    HD_PARTICLE_SAMPLE sample;
    HD_PARTICLE_MOTION *motion;
    uint32 before;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 128);
    sample.position = (VECTORF){points[0].x, points[0].y, 0.0f};
    sample.size = points[1].x - points[0].x;
    sample.brightness = points[0].color[0] / 255.0f;
    motion = sample_particle_motion(&hd.snow[hd.view][slot].motion, &sample);
    memcpy(hd.snow[hd.view][slot].quad, points, sizeof(hd.snow[hd.view][slot].quad));
    hd.snow[hd.view][slot].texture = *texture;
    hd.snow[hd.view][slot].texture.flags |= HD_SCENE_TEXTURE;
    before = hd.ui_count;
    hd_ui_quad(points, &hd.snow[hd.view][slot].texture, 1.0f);
    if (hd.ui_count > before)
    {
        HD_UI_DRAW *draw = &hd.ui_draws[hd.ui_count - 1];
        draw->particle[0] = motion->previous;
        draw->particle[1] = motion->current;
        draw->particle_active = 1;
    }
}

void hd_weather_rain(uint32 slot, const HD_UI_VERTEX *head, const RGBC *tail_color)
{
    HD_RAIN_MOTION *motion;
    HD_UI_VERTEX current[2];
    uint32 before;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 128);
    motion = &hd.rain[hd.view][slot];
    current[0] = *head;
    if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation)
        current[1] = *head;
    else
        current[1] = motion->current[0];
    memcpy(current[1].color, tail_color, sizeof(*tail_color));
    if (motion->serial != hd.motion_serial || motion->generation != hd.motion_generation)
    {
        if (motion->serial == 0 || motion->serial + 1 != hd.motion_serial || motion->generation != hd.motion_generation)
            memcpy(motion->previous, current, sizeof(current));
        else
            memcpy(motion->previous, motion->current, sizeof(current));
        memcpy(motion->current, current, sizeof(current));
        motion->serial = hd.motion_serial;
        motion->generation = hd.motion_generation;
    }
    before = hd.ui_count;
    hd_ui_segment(current, 32, 1, 0);
    if (hd.ui_count > before)
    {
        HD_UI_DRAW *draw = &hd.ui_draws[hd.ui_count - 1];
        memcpy(draw->segment[0], motion->previous, sizeof(motion->previous));
        memcpy(draw->segment[1], motion->current, sizeof(motion->current));
        draw->segment_active = 1;
    }
}

void hd_weather_retire(uint32 slot, uint32 mode)
{
    uint32 before;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 128);
    before = hd.ui_count;
    if (mode == 1)
    {
        const HD_SNOW_MOTION *snow = &hd.snow[hd.view][slot];
        const HD_PARTICLE_MOTION *motion = &snow->motion;
        if (motion->generation != hd.motion_generation || motion->serial == 0 || (motion->serial != hd.motion_serial && motion->serial + 1 != hd.motion_serial))
            return;
        hd_ui_quad(snow->quad, &snow->texture, 1.0f);
        if (hd.ui_count > before)
        {
            HD_UI_DRAW *draw = &hd.ui_draws[hd.ui_count - 1];
            draw->particle[0] = draw->particle[1] = motion->current;
            draw->particle_active = 1;
            draw->fading_out = 1;
        }
    }
    else if (mode == 0)
    {
        const HD_RAIN_MOTION *motion = &hd.rain[hd.view][slot];
        if (motion->generation != hd.motion_generation || motion->serial == 0 || (motion->serial != hd.motion_serial && motion->serial + 1 != hd.motion_serial))
            return;
        hd_ui_segment(motion->current, 32, 1, 0);
        if (hd.ui_count > before)
        {
            HD_UI_DRAW *draw = &hd.ui_draws[hd.ui_count - 1];
            memcpy(draw->segment[0], motion->current, sizeof(motion->current));
            memcpy(draw->segment[1], motion->current, sizeof(motion->current));
            draw->segment_active = 1;
            draw->fading_out = 1;
        }
    }
}

static void init_effect_quad(HD_MODEL *model)
{
    if (model->geometry == NULL)
    {
        uint32 *indices, index;
        model->geometry = vid_mesh_create_2d(6);
        model->indices = vid_index_create(6);
        XASSERT(model->geometry != NULL && model->indices != NULL);
        indices = vid_index_data(model->indices);
        for (index = 0; index < 6; ++index)
            indices[index] = index;
        vid_index_commit(model->indices, 6);
        vid_index_static(model->indices);
        model->index_count = 6;
    }
}

static void quad_draw(HD_MODEL *model, const HD_QUAD_SAMPLE *previous, const HD_QUAD_SAMPLE *current, const HD_TEXTURE *texture, sint32 retiring)
{
    HD_MODEL_DRAW *draw;
    sint32 actor = hd.actor;
    init_effect_quad(model);
    hd.actor = -1;
    draw = reserve_model_draw(model, 1);
    hd.actor = actor;
    draw->dynamic = 1;
    draw->count = 6;
    draw->blend = (texture->flags & HD_SEMITRANSPARENT) != 0 ? 1 + ((texture->page >> 5) & 3) : 0;
    draw->quad[0] = *previous;
    draw->quad[1] = *current;
    draw->quad_texture = *texture;
    {
        uint32 corner;
        for (corner = 0; corner < 4; ++corner)
        {
            draw->quad_uv[corner][0] = (corner & 1) != 0 ? texture->width * 2.0f : 0.0f;
            draw->quad_uv[corner][1] = (corner & 2) != 0 ? texture->height * 2.0f : 0.0f;
        }
    }
    draw->quad_active = 1;
    draw->fading_out = retiring;
}

void hd_shadow_quad(uint32 slot, uint32 part, uint32 kind, const VECTORF points[4], const HD_TEXTURE *texture, const RGBC *color, const POINT_T uv[4])
{
    HD_SHADOW_STORAGE *storage;
    HD_QUAD_SAMPLE sample;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 14 && part < 2 && color->r == color->g && color->g == color->b);
    storage = &hd.shadows[hd.view][slot][part];
    memcpy(sample.points, points, sizeof(sample.points));
    sample.brightness = color->r / 255.0f;
    if (storage->serial != hd.motion_serial || storage->generation != hd.motion_generation || storage->kind != kind || memcmp(&storage->texture, texture, sizeof(*texture)) != 0)
    {
        if (storage->serial == 0 || storage->serial + 1 != hd.motion_serial || storage->generation != hd.motion_generation || storage->kind != kind || memcmp(&storage->texture, texture, sizeof(*texture)) != 0)
            storage->previous = sample;
        else
            storage->previous = storage->current;
        storage->current = sample;
        storage->texture = *texture;
        storage->serial = hd.motion_serial;
        storage->generation = hd.motion_generation;
        storage->kind = kind;
    }
    quad_draw(&storage->model, &storage->previous, &storage->current, texture, 0);
    {
        HD_MODEL_DRAW *draw = &hd.model_draws[hd.model_count - 1];
        uint32 corner;
        draw->depth_bias = 0.0001f;
        for (corner = 0; corner < 4; ++corner)
        {
            draw->quad_uv[corner][0] = (uint8)(uv[corner].uv.u - texture->u) * 2.0f + 1.0f;
            draw->quad_uv[corner][1] = (uint8)(uv[corner].uv.v - texture->v) * 2.0f + 1.0f;
        }
    }
}

void hd_skid_reset(uint32 slot)
{
    uint32 view;
    XASSERT(slot < 80);
    for (view = 0; view < HD_VIEWS; ++view)
    {
        HD_SKID_STORAGE *storage = &hd.skids[view][slot];
        if (storage->retire_generation != hd.motion_generation)
            storage->retire_pending = 0;
        if (storage->generation == hd.motion_generation && storage->serial != 0 && !storage->current_retired && (storage->serial == hd.motion_serial || storage->serial + 1 == hd.motion_serial))
        {
            storage->retired = storage->current;
            storage->retired_texture = storage->texture;
            storage->retire_pending = 1;
            storage->retire_generation = hd.motion_generation;
        }
        storage->generation = ~hd.motion_generation;
    }
}

static void skid_quad_draw(HD_MODEL *model, const HD_QUAD_SAMPLE *previous, const HD_QUAD_SAMPLE *current, const HD_TEXTURE *texture, sint32 retiring)
{
    quad_draw(model, previous, current, texture, retiring);
    hd.model_draws[hd.model_count - 1].depth_bias = 0.0002f;
}

void hd_skids_begin(void)
{
    uint32 slot;
    if (!hd_capture_active())
        return;
    for (slot = 0; slot < 80; ++slot)
    {
        HD_SKID_STORAGE *storage = &hd.skids[hd.view][slot];
        if (!storage->retire_pending)
            continue;
        if (storage->retire_generation == hd.motion_generation)
            skid_quad_draw(&storage->model[1], &storage->retired, &storage->retired, &storage->retired_texture, 1);
        storage->retire_pending = 0;
    }
}

void hd_skid_quad(uint32 slot, const VECTORF points[4], const HD_TEXTURE *texture, const RGBC *color, sint32 retiring)
{
    HD_QUAD_SAMPLE sample;
    HD_SKID_STORAGE *storage;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 80);
    storage = &hd.skids[hd.view][slot];
    memcpy(sample.points, points, sizeof(sample.points));
    {
        VECTORF a = {points[1].vx - points[0].vx, points[1].vy - points[0].vy, points[1].vz - points[0].vz};
        VECTORF b = {points[2].vx - points[0].vx, points[2].vy - points[0].vy, points[2].vz - points[0].vz};
        VECTORF normal = {a.vy * b.vz - a.vz * b.vy, a.vz * b.vx - a.vx * b.vz, a.vx * b.vy - a.vy * b.vx};
        float length = sqrtf(normal.vx * normal.vx + normal.vy * normal.vy + normal.vz * normal.vz);
        uint32 corner;
        if (length > 0.00001f)
        {
            float lift = (normal.vy > 0.0f ? -0.5f : 0.5f) / length;
            for (corner = 0; corner < 4; ++corner)
            {
                sample.points[corner].vx += normal.vx * lift;
                sample.points[corner].vy += normal.vy * lift;
                sample.points[corner].vz += normal.vz * lift;
            }
        }
    }
    sample.brightness = color->r / 255.0f;
    if (storage->serial != hd.motion_serial || storage->generation != hd.motion_generation)
    {
        if (storage->serial == 0 || storage->serial + 1 != hd.motion_serial || storage->generation != hd.motion_generation)
            storage->previous = sample;
        else
            storage->previous = storage->current;
        storage->current = sample;
        storage->texture = *texture;
        storage->serial = hd.motion_serial;
        storage->generation = hd.motion_generation;
    }
    storage->current_retired = retiring;
    skid_quad_draw(&storage->model[0], &storage->previous, &storage->current, texture, retiring);
}

void hd_effect_line(uint32 slot, const VEC3 points[2], const RGBC colors[2], uint16 page, sint32 semi)
{
    HD_LINE_STORAGE *line;
    HD_MODEL_DRAW *draw;
    VECTORF current[2];
    uint32 point;
    sint32 actor;
    if (!hd_capture_active())
        return;
    XASSERT(slot < 50);
    line = &hd.lines[hd.view][slot];
    init_effect_quad(&line->model);
    for (point = 0; point < 2; ++point)
        current[point] = (VECTORF){points[point].x / 256.0f, points[point].y / 256.0f, points[point].z / 256.0f};
    if (line->serial != hd.motion_serial || line->generation != hd.motion_generation)
    {
        if (line->serial == 0 || line->serial + 1 != hd.motion_serial || line->generation != hd.motion_generation)
            memcpy(line->previous, current, sizeof(current));
        else
            memcpy(line->previous, line->current, sizeof(current));
        memcpy(line->current, current, sizeof(current));
        line->serial = hd.motion_serial;
        line->generation = hd.motion_generation;
    }
    actor = hd.actor;
    hd.actor = -1;
    draw = reserve_model_draw(&line->model, 1);
    hd.actor = actor;
    draw->dynamic = 1;
    draw->first = 0;
    draw->count = 6;
    draw->blend = semi ? 1 + ((page >> 5) & 3) : 0;
    line->blend = draw->blend;
    memcpy(line->colors, colors, sizeof(line->colors));
    draw->poses.line_active = 1;
    memcpy(draw->poses.line[0], line->previous, sizeof(line->previous));
    memcpy(draw->poses.line[1], line->current, sizeof(line->current));
    memcpy(draw->poses.line_colors, colors, sizeof(draw->poses.line_colors));
}

void hd_effects_begin(void)
{
    uint32 slot;
    if (!hd_capture_active())
        return;
    for (slot = 0; slot < 50; ++slot)
    {
        HD_EFFECT_RETIREMENT *retired = &hd.retired_effects[hd.view][slot];
        HD_MODEL_DRAW *draw;
        if (retired->kind == 0)
            continue;
        if (retired->generation != hd.motion_generation)
        {
            retired->kind = 0;
            continue;
        }
        if (retired->kind == 1)
        {
            draw_world_billboard(&retired->particle[1].position, retired->particle[1].size, &retired->texture, &retired->color);
            draw = &hd.model_draws[hd.model_count - 1];
            memcpy(draw->poses.particle, retired->particle, sizeof(retired->particle));
            draw->poses.particle_active = 1;
            draw->fading_out = 1;
        }
        else if (retired->kind == 2)
            draw_screen_particle(&retired->particle[0], &retired->particle[1], retired->height_ratio, &retired->texture, &retired->color, retired->layer, 1);
        else
        {
            sint32 actor = hd.actor;
            init_effect_quad(&retired->line_model);
            hd.actor = -1;
            draw = reserve_model_draw(&retired->line_model, 1);
            hd.actor = actor;
            draw->dynamic = 1;
            draw->count = 6;
            draw->blend = retired->blend;
            memcpy(draw->poses.line, retired->line, sizeof(retired->line));
            memcpy(draw->poses.line_colors, retired->line_colors, sizeof(retired->line_colors));
            draw->poses.line_active = 1;
            draw->fading_out = 1;
        }
        retired->kind = 0;
    }
}

void hd_attached_quad(const VECTORF *center, const VECTORF corners[4], const HD_TEXTURE *texture, const RGBC colors[4])
{
    VECTORF position, vertices[4];
    uint32 point;
    if (!hd_capture_active())
        return;
    RotTransf(&hd.transform, center, &position);
    for (point = 0; point < 4; ++point)
    {
        ApplyMatrixf(&hd.screen_rotation, corners + point, vertices + point);
        vertices[point].vx += position.vx;
        vertices[point].vy += position.vy;
        vertices[point].vz += position.vz;
    }
    hd_world_quad(vertices, texture, colors, 1);
    hd.model_draws[hd.model_count - 1].poses.centered_billboard = 1;
    hd.model_draws[hd.model_count - 1].poses.center = *center;
}

void hd_set_lighting(const MATRIX *rotation)
{
    MATRIXF source, object;
    matrix_from_native(&model_render.light, &source);
    matrix_from_native(rotation, &object);
    MulMatrix0f(&source, &object, &hd.light);
}

static void inverse_source_transform(const MATRIXF *source, MATRIXF *inverse)
{
    VECTORF rows[3], cofactors[3], translation;
    float determinant;
    uint32 row, column;
    for (row = 0; row < 3; ++row)
        rows[row] = (VECTORF){source->m[row][0], source->m[row][1], source->m[row][2]};
    OuterProductf(&rows[1], &rows[2], &cofactors[0]);
    OuterProductf(&rows[2], &rows[0], &cofactors[1]);
    OuterProductf(&rows[0], &rows[1], &cofactors[2]);
    determinant = rows[0].vx * cofactors[0].vx + rows[0].vy * cofactors[0].vy + rows[0].vz * cofactors[0].vz;
    XASSERT(fabsf(determinant) > 0.000001f);
    for (row = 0; row < 3; ++row)
        for (column = 0; column < 3; ++column)
            inverse->m[row][column] = ((const float *)&cofactors[column])[row] / determinant;
    translation = (VECTORF){source->t[0], source->t[1], source->t[2]};
    ApplyMatrixf(inverse, &translation, &translation);
    inverse->t[0] = -translation.vx;
    inverse->t[1] = -translation.vy;
    inverse->t[2] = -translation.vz;
}

static MATRIXF wheel_transform(const HD_WHEEL_SAMPLE *sample, sint32 hub)
{
    MATRIXF result;
    IdentityMatrixf(&result);
    RotMatrixYf(sample->steering, &result);
    if (hub)
    {
        MATRIXF spin, combined;
        IdentityMatrixf(&spin);
        RotMatrixXf(-sample->spin, &spin);
        MulMatrix0f(&result, &spin, &combined);
        result = combined;
    }
    result.t[0] = sample->center.vx;
    result.t[1] = sample->center.vy;
    result.t[2] = sample->center.vz;
    return result;
}

static MATRIXF interpolated_transform(const HD_MODEL_DRAW *draw)
{
    HD_POSE camera, object;
    MATRIXF view, model, result;
    VECTORF delta, translated;
    if (hd.render_phase >= 1.0f && !hd.race_frame)
        return draw->transform;
    hd_pose_interpolate(&draw->poses.camera[0], &draw->poses.camera[1], hd.render_phase, &camera);
    hd_pose_interpolate(&draw->poses.object[0], &draw->poses.object[1], hd.render_phase, &object);
    if (draw->poses.world_translation)
    {
        object.position.x -= draw->poses.object[1].position.x;
        object.position.y -= draw->poses.object[1].position.y;
        object.position.z -= draw->poses.object[1].position.z;
        object.rotation = (QUATF){0.0f, 0.0f, 0.0f, 1.0f};
    }
    QuatMatrixf(&camera.rotation, &view);
    QuatMatrixf(&object.rotation, &model);
    MulMatrix0f(&view, &model, &result);
    delta = (VECTORF){object.position.x, object.position.y, object.position.z};
    if (!draw->poses.relative)
    {
        delta.vx -= camera.position.x;
        delta.vy -= camera.position.y;
        delta.vz -= camera.position.z;
    }
    ApplyMatrixf(&view, &delta, &translated);
    result.t[0] = translated.vx;
    result.t[1] = translated.vy;
    result.t[2] = translated.vz;
    if (draw->poses.wheel_active)
    {
        const HD_WHEEL_SAMPLE *previous = &draw->poses.wheel[0], *current = &draw->poses.wheel[1];
        HD_WHEEL_SAMPLE sample;
        MATRIXF local, source_local, source, inverse, desired, combined;
        sample.center.vx = previous->center.vx + hd.render_phase * (current->center.vx - previous->center.vx);
        sample.center.vy = previous->center.vy + hd.render_phase * (current->center.vy - previous->center.vy);
        sample.center.vz = previous->center.vz + hd.render_phase * (current->center.vz - previous->center.vz);
        sample.steering = previous->steering + hd.render_phase * (current->steering - previous->steering);
        sample.spin = previous->spin + hd.render_phase * (current->spin - previous->spin);
        local = wheel_transform(&sample, draw->poses.wheel_hub);
        source_local = wheel_transform(current, draw->poses.wheel_hub);
        CompMatrixf(&result, &local, &desired);
        CompMatrixf(&draw->poses.source_transform, &source_local, &source);
        inverse_source_transform(&source, &inverse);
        CompMatrixf(&desired, &inverse, &combined);
        return combined;
    }
    if (draw->poses.billboard)
    {
        QUATF facing;
        MATRIXF rotation;
        SlerpQuatf(&draw->poses.facing[0], &draw->poses.facing[1], hd.render_phase, &facing);
        QuatMatrixf(&facing, &rotation);
        memcpy(result.m, rotation.m, sizeof(result.m));
    }
    if (draw->poses.centered_billboard)
    {
        VECTORF previous_center, center;
        VECTORF source_center = draw->poses.center;
        MATRIXF offset;
        float scale = 1.0f;
        if (draw->poses.particle_active)
        {
            const HD_PARTICLE_SAMPLE *previous = &draw->poses.particle[0], *current = &draw->poses.particle[1];
            source_center.vx = previous->position.vx + hd.render_phase * (current->position.vx - previous->position.vx);
            source_center.vy = previous->position.vy + hd.render_phase * (current->position.vy - previous->position.vy);
            source_center.vz = previous->position.vz + hd.render_phase * (current->position.vz - previous->position.vz);
            scale = (previous->size + hd.render_phase * (current->size - previous->size)) / current->size;
        }
        RotTransf(&draw->poses.source_transform, &draw->poses.center, &previous_center);
        RotTransf(&result, &source_center, &center);
        IdentityMatrixf(&offset);
        offset.m[0][0] = offset.m[1][1] = offset.m[2][2] = scale;
        offset.t[0] = center.vx - scale * previous_center.vx;
        offset.t[1] = center.vy - scale * previous_center.vy;
        offset.t[2] = center.vz - scale * previous_center.vz;
        return offset;
    }
    if (draw->poses.view_geometry)
    {
        MATRIXF inverse, combined;
        inverse_source_transform(&draw->poses.source_transform, &inverse);
        CompMatrixf(&result, &inverse, &combined);
        return combined;
    }
    return result;
}

static sint32 prepare_line_geometry(const HD_MODEL_DRAW *draw)
{
    static const uint8 order[6] = {0, 1, 2, 1, 3, 2};
    const HD_VIEW *view = &hd.viewports[draw->view];
    HD_PROJECTION projection = interpolated_projection(draw->projection);
    HD_POSE camera;
    MATRIXF rotation;
    VECTORF points[2];
    RGBC colors[2] = {draw->poses.line_colors[0], draw->poses.line_colors[1]};
    VID_VERTEX2D *vertices;
    float projected[2][2], aspect, x_scale, y_scale, center_x, center_y, dx, dy, length, half_width;
    uint32 point, vertex;
    if (view->viewport.w <= 0 || view->viewport.h <= 0)
        return 0;
    hd_pose_interpolate(&draw->poses.camera[0], &draw->poses.camera[1], hd.render_phase, &camera);
    QuatMatrixf(&camera.rotation, &rotation);
    for (point = 0; point < 2; ++point)
    {
        const VECTORF *previous = &draw->poses.line[0][point], *current = &draw->poses.line[1][point];
        VECTORF relative = {previous->vx + hd.render_phase * (current->vx - previous->vx) - camera.position.x, previous->vy + hd.render_phase * (current->vy - previous->vy) - camera.position.y, previous->vz + hd.render_phase * (current->vz - previous->vz) - camera.position.z};
        ApplyMatrixf(&rotation, &relative, &points[point]);
    }
    if (points[0].vz <= 1.001f && points[1].vz <= 1.001f)
        return 0;
    for (point = 0; point < 2; ++point)
        if (points[point].vz <= 1.001f)
        {
            uint32 other = point ^ 1, component;
            float fraction = (1.001f - points[point].vz) / (points[other].vz - points[point].vz);
            points[point].vx += fraction * (points[other].vx - points[point].vx);
            points[point].vy += fraction * (points[other].vy - points[point].vy);
            points[point].vz = 1.001f;
            for (component = 0; component < 3; ++component)
                ((uint8 *)&colors[point])[component] = (uint8)(((uint8 *)&colors[point])[component] + fraction * (((uint8 *)&colors[other])[component] - ((uint8 *)&colors[point])[component]));
        }
    aspect = (float)view->viewport.w / view->viewport.h;
    x_scale = 2.0f * projection.x / view->reference.w * view->reference_aspect / aspect;
    y_scale = 2.0f * projection.y / view->reference.h;
    center_x = 2.0f * (projection.center_x - view->reference.x) / view->reference.w - 1.0f;
    center_y = 1.0f - 2.0f * (projection.center_y - view->reference.y) / view->reference.h;
    if (x_scale == 0.0f || y_scale == 0.0f)
        return 0;
    for (point = 0; point < 2; ++point)
    {
        projected[point][0] = points[point].vx * x_scale / points[point].vz + center_x;
        projected[point][1] = -points[point].vy * y_scale / points[point].vz + center_y;
    }
    dx = (projected[1][0] - projected[0][0]) * view->viewport.w;
    dy = (projected[1][1] - projected[0][1]) * view->viewport.h;
    length = sqrtf(dx * dx + dy * dy);
    if (length <= 0.000001f)
        return 0;
    half_width = 0.5f * view->viewport.h / view->reference.h;
    dx = dx / length * 2.0f * half_width / view->viewport.h;
    dy = -dy / length * 2.0f * half_width / view->viewport.w;
    vertices = vid_mesh_data_2d(draw->model->geometry);
    XASSERT(vertices != NULL);
    memset(vertices, 0, 6 * sizeof(*vertices));
    for (vertex = 0; vertex < 6; ++vertex)
    {
        uint32 corner = order[vertex];
        float side = (corner & 2) != 0 ? 1.0f : -1.0f;
        point = corner & 1;
        vertices[vertex].pos[0] = (projected[point][0] + side * dy - center_x) * points[point].vz / x_scale;
        vertices[vertex].pos[1] = -(projected[point][1] + side * dx - center_y) * points[point].vz / y_scale;
        vertices[vertex].pos[2] = points[point].vz;
        memcpy(vertices[vertex].color, &colors[point], sizeof(RGBC));
        vertices[vertex].tex[2] = draw->blend != 0 ? HD_SEMITRANSPARENT : 0;
    }
    vid_mesh_commit(draw->model->geometry);
    return 1;
}

static void prepare_quad_geometry(const HD_MODEL_DRAW *draw)
{
    static const uint8 order[6] = {0, 1, 2, 1, 3, 2};
    HD_POSE camera;
    MATRIXF rotation;
    VID_VERTEX2D *vertices = vid_mesh_data_2d(draw->model->geometry);
    const HD_TEXTURE *texture = &draw->quad_texture;
    HD_SPRITE_TEXTURE *sprite = (texture->flags & HD_TEXTURED) != 0 ? sprite_texture_get(texture) : NULL;
    uint32 vertex;
    XASSERT(vertices != NULL);
    hd_pose_interpolate(&draw->poses.camera[0], &draw->poses.camera[1], hd.render_phase, &camera);
    QuatMatrixf(&camera.rotation, &rotation);
    memset(vertices, 0, 6 * sizeof(*vertices));
    for (vertex = 0; vertex < 6; ++vertex)
    {
        uint32 corner = order[vertex];
        const VECTORF *previous = &draw->quad[0].points[corner], *current = &draw->quad[1].points[corner];
        VECTORF point, relative = {previous->vx + hd.render_phase * (current->vx - previous->vx) - camera.position.x, previous->vy + hd.render_phase * (current->vy - previous->vy) - camera.position.y, previous->vz + hd.render_phase * (current->vz - previous->vz) - camera.position.z};
        ApplyMatrixf(&rotation, &relative, &point);
        vertices[vertex].pos[0] = point.vx;
        vertices[vertex].pos[1] = point.vy;
        vertices[vertex].pos[2] = point.vz;
        vertices[vertex].uv[0] = draw->quad_uv[corner][0];
        vertices[vertex].uv[1] = draw->quad_uv[corner][1];
        memset(vertices[vertex].color, 128, 3);
        vertices[vertex].tex[0] = texture->page;
        vertices[vertex].tex[1] = texture->clut;
        vertices[vertex].tex[2] = texture->flags;
        vertices[vertex].tex[3] = texture->u | (texture->v << 8);
        vertices[vertex].rect[0] = texture->width;
        vertices[vertex].rect[1] = texture->height;
        if (sprite != NULL)
        {
            if (sprite->source_texture != NULL)
            {
                HD_SOURCE_GPU image = hd_texture_source_gpu(sprite->source_texture);
                vertices[vertex].uv[0] += (texture->u - sprite->source_region.u) * 2.0f;
                vertices[vertex].uv[1] += (texture->v - sprite->source_region.v) * 2.0f;
                vertices[vertex].tex[0] = (uint16)image.opaque_layer;
                vertices[vertex].tex[1] = (uint16)image.stp_layer;
                vertices[vertex].tex[2] |= 1024;
                vertices[vertex].tex[3] = (uint16)image.mode;
                vertices[vertex].rect[0] = (uint16)image.width;
                vertices[vertex].rect[1] = (uint16)image.height;
            }
            else
            {
                vertices[vertex].uv[0] += sprite->asset->charts[0].x * 2.0f;
                vertices[vertex].uv[1] += sprite->asset->charts[0].y * 2.0f;
                vertices[vertex].rect[0] = (uint16)sprite->asset->width;
                vertices[vertex].rect[1] = (uint16)sprite->asset->height;
            }
        }
    }
    vid_mesh_commit(draw->model->geometry);
}

static void environment_uniforms(const HD_MODEL_DRAW *draw, const MATRIXF *transform, const HD_PROJECTION *projection, float uniforms[30][4])
{
    const HD_ENV_SAMPLE *previous = &draw->environment_samples[0], *current = &draw->environment_samples[1];
    uint32 channel;
    if (draw->dynamic != 3)
        return;
    for (channel = 0; channel < 3; ++channel)
        uniforms[12][channel + 1] = previous->tint[channel] + hd.render_phase * (current->tint[channel] - previous->tint[channel]);
    if (draw->environment[0][0] == 1.0f)
    {
        float distance = sqrtf(transform->t[0] * transform->t[0] + transform->t[1] * transform->t[1] + transform->t[2] * transform->t[2]);
        XASSERT(projection->x != 0.0f && projection->y != 0.0f);
        uniforms[15][0] = previous->center[0] + hd.render_phase * (current->center[0] - previous->center[0]);
        uniforms[15][1] = previous->center[1] + hd.render_phase * (current->center[1] - previous->center[1]);
        if (draw->reflection_centered && transform->t[2] != 0.0f)
        {
            uniforms[15][0] = projection->center_x + transform->t[0] * projection->x / transform->t[2];
            uniforms[15][1] = projection->center_y + transform->t[1] * projection->y / transform->t[2];
        }
        uniforms[15][2] = draw->reflection_scale[0] * distance / projection->x;
        uniforms[15][3] = draw->reflection_scale[1] * distance / projection->y;
        uniforms[18][2] = projection->x;
        uniforms[18][3] = projection->y;
    }
    else
    {
        float angle = previous->angle + hd.render_phase * (current->angle - previous->angle);
        uniforms[13][0] = cosf(angle);
        uniforms[13][1] = sinf(angle);
        uniforms[13][2] = previous->u + hd.render_phase * (current->u - previous->u);
        uniforms[13][3] = previous->v + hd.render_phase * (current->v - previous->v);
    }
}

static void submit_police_light(const HD_MODEL_DRAW *draw, uint32 phase)
{
    const HD_POLICE_INPUT *input = &draw->police;
    const HD_VIEW *view = &hd.viewports[draw->view];
    HD_PROJECTION projection = interpolated_projection(draw->projection);
    MATRIXF transform;
    float uniforms[12][4] = {{0}};
    float aspect = view->viewport.h > 0 ? (float)view->viewport.w / view->viewport.h : 4.0f / 3.0f;
    uint32 row, column, part;
    if (phase == 0)
        return;
    transform = interpolated_transform(draw);
    for (row = 0; row < 3; ++row)
    {
        for (column = 0; column < 3; ++column)
            uniforms[row][column] = transform.m[row][column];
        uniforms[row][3] = transform.t[row];
    }
    uniforms[3][0] = 2.0f * projection.x / view->reference.w * view->reference_aspect / aspect;
    uniforms[3][1] = 2.0f * projection.y / view->reference.h;
    uniforms[3][2] = (float)input->ray_flags[1];
    uniforms[3][3] = draw->far_plane;
    uniforms[4][0] = 2.0f * (projection.center_x - view->reference.x) / view->reference.w - 1.0f;
    uniforms[4][1] = 1.0f - 2.0f * (projection.center_y - view->reference.y) / view->reference.h;
    uniforms[4][2] = projection.center_x;
    uniforms[4][3] = projection.center_y;
    uniforms[5][0] = projection.x;
    uniforms[5][1] = projection.y;
    uniforms[5][2] = input->half_width;
    uniforms[5][3] = input->half_height;
    uniforms[6][0] = input->anchor.vx;
    uniforms[6][1] = input->anchor.vy;
    uniforms[6][2] = input->anchor.vz;
    uniforms[6][3] = input->samples[0].signal + hd.render_phase * (input->samples[1].signal - input->samples[0].signal);
    uniforms[7][0] = (float)input->texture.page;
    uniforms[7][1] = (float)input->texture.clut;
    uniforms[7][2] = (float)input->texture.u;
    uniforms[7][3] = (float)input->texture.v;
    uniforms[8][0] = (float)input->texture.width;
    uniforms[8][1] = (float)input->texture.height;
    uniforms[8][2] = input->depth_scale;
    uniforms[8][3] = input->near_depth;
    for (part = 0; part < 3; ++part)
        for (column = 0; column < 3; ++column)
            uniforms[9 + part][column] = input->samples[0].colors[part][column] + hd.render_phase * (input->samples[1].colors[part][column] - input->samples[0].colors[part][column]);
    uniforms[9][3] = (float)input->texture.flags;
    uniforms[10][3] = (float)input->ray_flags[0];
    uniforms[11][3] = input->far_depth;
    vid_viewport(view->viewport);
    vid_uniforms(&uniforms[0][0], 12);
    vid_pass(hd.police_materials[0]);
    vid_submit(draw->model->geometry, draw->model->indices, 0, 12);
    if (draw->blend != 0)
    {
        vid_pass(hd.police_materials[draw->blend]);
        vid_submit(draw->model->geometry, draw->model->indices, 0, 12);
    }
}

static void submit_model(const HD_MODEL_DRAW *draw, uint32 phase, const HD_SCENE_BATCH_GROUP *batch)
{
    if (batch == NULL && draw->model->static_pool != NULL)
    {
        uint32 range;
        if (phase != 2 || draw->source_order != 2)
            return;
        for (range = 0; range < draw->model->range_count; ++range)
        {
            const HD_MODEL_RANGE *part = &draw->model->ranges[range];
            HD_SOURCE_GPU image = hd_texture_source_gpu(part->source_texture);
            if (part->blend != 0 && (image.classes & 4) != 0)
            {
                HD_STATIC_POOL *pool = draw->model->static_pool;
                HD_SCENE_BATCH_GROUP selected = {0};
                selected.geometry = pool->geometry;
                selected.index_buffer = pool->indices;
                selected.origin = pool->origin;
                selected.world_space = 1;
                selected.first = part->source_first;
                selected.count = part->count;
                selected.material = source_material(image.texture, part->blend, 2, 1)->batch_material;
                submit_model(draw, phase, &selected);
            }
        }
        return;
    }
    float uniforms[32][4] = {{0}};
    MATRIXF transform, light = draw->light, previous_light;
    const HD_VIEW *view = &hd.viewports[draw->view];
    HD_PROJECTION projection = interpolated_projection(draw->projection);
    uint32 row, column, range;
    float aspect = view->viewport.h > 0 ? (float)view->viewport.w / view->viewport.h : 4.0f / 3.0f;
    if (draw->fog_active)
    {
        VID_FOG fog = {draw->fog_horizon / 3.0f, draw->fog_horizon * (5.0f / 6.0f), draw->fog_horizon, {draw->fog_color[0] / 255.0f, draw->fog_color[1] / 255.0f, draw->fog_color[2] / 255.0f}, {fabsf(draw->projection[1].x), fabsf(draw->projection[1].y)}};
        vid_fog(&fog);
    }
    else
        vid_fog(NULL);
    if (draw->dynamic == 4)
    {
        if (phase == 2)
            submit_police_light(draw, 1);
        return;
    }
    if (draw->quad_active)
    {
        if (phase != 2)
            return;
        prepare_quad_geometry(draw);
        IdentityMatrixf(&transform);
    }
    else if (draw->poses.line_active)
    {
        if (phase != 2 || !prepare_line_geometry(draw))
            return;
        IdentityMatrixf(&transform);
    }
    else if (batch == NULL)
        transform = interpolated_transform(draw);
    else
        IdentityMatrixf(&transform);
    if (batch != NULL && batch->world_space)
    {
        HD_POSE camera;
        VECTORF delta, translated;
        hd_pose_interpolate(&draw->poses.camera[0], &draw->poses.camera[1], hd.render_phase, &camera);
        QuatMatrixf(&camera.rotation, &transform);
        delta = (VECTORF){batch->origin.x - camera.position.x, batch->origin.y - camera.position.y, batch->origin.z - camera.position.z};
        ApplyMatrixf(&transform, &delta, &translated);
        transform.t[0] = translated.vx;
        transform.t[1] = translated.vy;
        transform.t[2] = translated.vz;
    }
    if (draw->light_history)
    {
        HD_POSE object;
        MATRIXF sampled;
        hd_pose_interpolate(&draw->poses.object[0], &draw->poses.object[1], hd.render_phase, &object);
        QuatMatrixf(&object.rotation, &sampled);
        MulMatrix0f(&draw->lighting[0].world, &sampled, &previous_light);
        MulMatrix0f(&draw->lighting[1].world, &sampled, &light);
    }
    if (draw->dynamic != 0 && draw->dynamic != 2 && phase != 2)
        return;
    for (row = 0; row < 3; ++row)
    {
        for (column = 0; column < 3; ++column)
        {
            uniforms[row][column] = transform.m[row][column];
            uniforms[5 + row][column] = light.m[row][column];
            uniforms[8 + row][column] = draw->light_history ? draw->lighting[1].color.m[row][column] : draw->color.m[row][column] / 4096.0f;
        }
        uniforms[row][3] = transform.t[row];
        uniforms[11][row] = draw->light_history ? draw->lighting[1].ambient[row] : draw->ambient[row];
    }
    uniforms[29][3] = 1.0f;
    if (draw->light_history)
    {
        for (row = 0; row < 3; ++row)
        {
            for (column = 0; column < 3; ++column)
            {
                uniforms[23 + row][column] = previous_light.m[row][column];
                uniforms[26 + row][column] = draw->lighting[0].color.m[row][column];
            }
            uniforms[29][row] = draw->lighting[0].ambient[row];
        }
        uniforms[29][3] = hd.render_phase;
    }
    uniforms[3][0] = 2.0f * projection.x / view->reference.w * (view->reference_aspect / aspect);
    uniforms[3][1] = 2.0f * projection.y / view->reference.h;
    uniforms[3][2] = 1.0f;
    uniforms[3][3] = draw->far_plane;
    uniforms[4][0] = 2.0f * (projection.center_x - view->reference.x) / view->reference.w - 1.0f;
    uniforms[4][1] = 1.0f - 2.0f * (projection.center_y - view->reference.y) / view->reference.h;
    uniforms[4][2] = projection.center_x;
    uniforms[4][3] = projection.center_y;
    memcpy(uniforms + 12, draw->environment, sizeof(draw->environment));
    environment_uniforms(draw, &transform, &projection, uniforms);
    uniforms[11][3] = -1.0f;
    uniforms[5][3] = draw->fading_out ? 1.0f - hd.render_phase : 1.0f;
    if (draw->quad_active)
        uniforms[11][3] = draw->quad[0].brightness + hd.render_phase * (draw->quad[1].brightness - draw->quad[0].brightness);
    if (draw->poses.particle_active)
        uniforms[11][3] = draw->poses.particle[0].brightness + hd.render_phase * (draw->poses.particle[1].brightness - draw->poses.particle[0].brightness);
    if (draw->hub_lit)
    {
        uniforms[19][0] = draw->hub_normal.vx;
        uniforms[19][1] = draw->hub_normal.vy;
        uniforms[19][2] = draw->hub_normal.vz;
        uniforms[19][3] = 1.0f;
        uniforms[20][0] = draw->hub_anchor.vx;
        uniforms[20][1] = draw->hub_anchor.vy;
        uniforms[20][2] = draw->hub_anchor.vz;
        uniforms[20][3] = draw->hub_cue[0];
        for (row = 0; row < 3; ++row)
            uniforms[21][row] = draw->hub_far[row];
        uniforms[21][3] = draw->hub_cue[1];
        uniforms[22][0] = draw->hub_cue[2];
        uniforms[22][1] = draw->hub_cue[3];
    }
    vid_viewport(view->viewport);
    uniforms[23][3] = draw->depth_bias;
    uniforms[24][3] = draw->depth_offset;
    if (view->headlight_active && (draw->model->static_pool != NULL || (draw->model->uses_sources && draw->model->kind >= 10 && draw->model->kind != 12)))
    {
        HD_POSE car_pose, camera;
        MATRIXF car_rotation, camera_rotation;
        VECTORF anchor, relative, position;
        hd_pose_interpolate(&view->headlight_car[0], &view->headlight_car[1], hd.render_phase, &car_pose);
        hd_pose_interpolate(&draw->poses.camera[0], &draw->poses.camera[1], hd.render_phase, &camera);
        QuatMatrixf(&car_pose.rotation, &car_rotation);
        QuatMatrixf(&camera.rotation, &camera_rotation);
        ApplyMatrixf(&car_rotation, &view->headlight_origin, &anchor);
        relative = (VECTORF){anchor.vx + car_pose.position.x - camera.position.x, anchor.vy + car_pose.position.y - camera.position.y, anchor.vz + car_pose.position.z - camera.position.z};
        ApplyMatrixf(&camera_rotation, &relative, &position);
        uniforms[30][0] = position.vx;
        uniforms[30][1] = position.vy;
        uniforms[30][2] = position.vz;
        uniforms[30][3] = distance_lighting.radius;
        memcpy(uniforms[31], distance_lighting.color, sizeof(distance_lighting.color));
        uniforms[31][3] = distance_lighting.height;
    }
    vid_uniforms(&uniforms[0][0], 32);
    if (batch != NULL)
    {
        vid_pass(batch->material);
        vid_submit(batch->geometry, batch->index_buffer, batch->first, batch->count);
        return;
    }
    if (draw->model->visibility != NULL)
    {
        for (range = 0; range < draw->model->sky_range_count; ++range)
        {
            const HD_MODEL_RANGE *part = &draw->model->sky_ranges[range];
            vid_pass(hd.sky_materials[0]);
            vid_submit(draw->model->geometry, draw->model->sky_indices, part->first, part->count);
            if (part->blend != 0)
            {
                vid_pass(hd.sky_materials[part->blend]);
                vid_submit(draw->model->geometry, draw->model->sky_indices, part->first, part->count);
            }
        }
        return;
    }
    if (draw->quad_active && (draw->quad_texture.flags & HD_TEXTURED) != 0)
    {
        HD_SPRITE_TEXTURE *sprite = sprite_texture_get(&draw->quad_texture);
        if ((draw->quad_texture.flags & HD_SHADOW) != 0)
        {
            vid_pass(sprite_texture_material(sprite, 3, 1));
            vid_submit(draw->model->geometry, draw->model->indices, draw->first, draw->count);
        }
        else
        {
            vid_pass(sprite_texture_material(sprite, 0, 1));
            vid_submit(draw->model->geometry, draw->model->indices, draw->first, draw->count);
            if (draw->blend != 0)
            {
                vid_pass(sprite_texture_material(sprite, draw->blend, 1));
                vid_submit(draw->model->geometry, draw->model->indices, draw->first, draw->count);
            }
        }
        return;
    }
    if (draw->model->uses_sources)
    {
        uint32 cull = draw->model->kind == 12 ? 0 : 2;
        if (phase < 2)
        {
            const HD_CACHED_GROUP *group = draw->source_group;
            if (draw->source_order != 1 || draw->texture_transparent != (phase == 1))
                return;
            vid_pass(source_material(group->texture, 0, cull, group->transparent)->material);
            vid_submit(draw->model->geometry, draw->model->indices, group->first, group->count);
        }
        else if (draw->source_order == 2)
            for (range = 0; range < draw->model->range_count; ++range)
            {
                const HD_MODEL_RANGE *part = &draw->model->ranges[range];
                HD_SOURCE_GPU image = hd_texture_source_gpu(part->source_texture);
                if (part->blend != 0 && (image.classes & 4) != 0)
                {
                    vid_pass(source_material(image.texture, part->blend, cull, 1)->material);
                    vid_submit(draw->model->geometry, draw->model->indices, part->source_first, part->count);
                }
            }
        return;
    }
    if (draw->dynamic == 2 && draw->model->texture_asset != NULL)
    {
        uint32 pass = phase == 2 ? draw->blend : 0;
        if ((phase < 2 && draw->texture_transparent == (phase == 1)) || (phase == 2 && pass != 0))
        {
            vid_pass(object_texture_material(draw, pass));
            vid_submit(draw->model->geometry, draw->model->indices, draw->first, draw->count);
        }
        return;
    }
    if (draw->dynamic)
    {
        if (phase == 1)
            return;
        VID_MATERIAL **materials = draw->fading_out ? hd.retire_materials : draw->dynamic == 2 ? hd.solid_materials : draw->dynamic == 3 ? hd.environment_materials : hd.effect_materials;
        if (phase == 0 || draw->dynamic != 2)
        {
            vid_pass(materials[0]);
            vid_submit(draw->model->geometry, draw->model->indices, draw->first, draw->count);
        }
        if (draw->blend != 0 && phase == 2)
        {
            vid_pass(materials[draw->blend]);
            vid_submit(draw->model->geometry, draw->model->indices, draw->first, draw->count);
        }
        return;
    }
    if (draw->model->texture_asset != NULL)
    {
        if (phase < 2)
        {
            if (draw->model->static_pool != NULL || draw->source_order == 2 || draw->texture_transparent != (phase == 1))
                return;
            vid_pass(object_texture_material(draw, 0));
            vid_submit(draw->model->geometry, draw->model->indices, 0, draw->model->index_count);
        }
        else
        {
            for (range = 0; range < draw->model->range_count; ++range)
            {
                const HD_MODEL_RANGE *part = &draw->model->ranges[range];
                uint32 count = part->count;
                if (part->blend == 0 || !object_texture_has_transparency(draw, &part->texture))
                    continue;
                while (range + 1 < draw->model->range_count && draw->model->ranges[range + 1].blend == part->blend)
                    count += draw->model->ranges[++range].count;
                vid_pass(object_texture_material(draw, part->blend));
                vid_submit(draw->model->geometry, draw->model->indices, part->first, count);
            }
        }
        return;
    }
}

static void scene_batches_free(void)
{
    while (scene_batch_banks != NULL)
    {
        HD_SCENE_BATCH_BANK *bank = scene_batch_banks;
        uint32 group;
        scene_batch_banks = bank->next;
        for (group = 0; group < bank->group_capacity; ++group)
        {
            vid_mat_free(bank->groups[group].material);
            free(bank->groups[group].indices);
        }
        vid_mesh_free(bank->geometry);
        vid_index_free(bank->indices);
        free(bank->groups);
        free(bank->inputs);
        free(bank);
    }
    scene_batch_cursor = NULL;
}

static sint32 scene_batch_eligible(const HD_MODEL_DRAW *draw, uint32 phase)
{
    const HD_DRAW_POSES *poses = &draw->poses;
    uint32 sample;
    if (draw->source_order == 2 || draw->texture_transparent != (phase == 1) || !draw->region_sampling || draw->dynamic || draw->model->batch_vertices == NULL || poses->relative || poses->view_geometry || draw->depth_bias != 0.0f || draw->depth_offset != 0.0f || draw->fading_out)
        return 0;
    if (memcmp(&poses->object[0].position, &poses->object[1].position, sizeof(VEC3F)) != 0)
        return 0;
    for (sample = 0; sample < 2; ++sample)
    {
        const QUATF *q = &poses->object[sample].rotation;
        if (q->x != 0.0f || q->y != 0.0f || q->z != 0.0f || q->w != 1.0f)
            return 0;
    }
    return 1;
}

static sint32 scene_batch_compatible(const HD_MODEL_DRAW *a, const HD_MODEL_DRAW *b)
{
    uint32 sample;
    if (a->view != b->view || a->far_plane != b->far_plane || a->fog_active != b->fog_active || a->fog_horizon != b->fog_horizon || memcmp(a->fog_color, b->fog_color, 3) != 0 || a->light_history != b->light_history || memcmp(a->projection, b->projection, sizeof(a->projection)) != 0 || memcmp(a->poses.camera, b->poses.camera, sizeof(a->poses.camera)) != 0)
        return 0;
    if (!a->model->has_lighting && !b->model->has_lighting)
        return 1;
    if (!a->light_history)
        return memcmp(a->light.m, b->light.m, sizeof(a->light.m)) == 0 && memcmp(a->color.m, b->color.m, sizeof(a->color.m)) == 0 && memcmp(a->ambient, b->ambient, sizeof(a->ambient)) == 0;
    for (sample = 0; sample < 2; ++sample)
        if (memcmp(a->lighting[sample].world.m, b->lighting[sample].world.m, sizeof(a->light.m)) != 0 || memcmp(a->lighting[sample].color.m, b->lighting[sample].color.m, sizeof(a->light.m)) != 0 || memcmp(a->lighting[sample].ambient, b->lighting[sample].ambient, sizeof(a->ambient)) != 0)
            return 0;
    return 1;
}

static uint32 scene_batch_capacity(uint32 count)
{
    uint32 capacity = 256;
    while (capacity < count)
    {
        XASSERT(capacity <= 0x3FFFFFFFu);
        capacity *= 2;
    }
    return capacity;
}

static uint32 source_draw_count(const HD_MODEL_DRAW *draw)
{
    return draw->source_group != NULL ? draw->source_group->count : draw->model->index_count;
}

static const VID_MATERIAL_DATA *source_draw_material(const HD_MODEL_DRAW *draw)
{
    if (draw->source_group != NULL)
        return &source_material(draw->source_group->texture, 0, draw->model->kind == 12 ? 0 : 2, draw->source_group->transparent)->data;
    return object_texture_material_data(draw, 0);
}

static void submit_scene_batches(uint32 phase, sint32 billboard)
{
    HD_SCENE_BATCH_BANK *bank;
    uint32 draw_index, total_vertices = 0, total_indices = 0, used_vertices = 0, used_indices = 0, group, input_index = 0;
    VID_VERTEX3D *vertices;
    uint32 *indices;
    for (draw_index = 0; draw_index < hd.model_count; ++draw_index)
        if (scene_batch_eligible(&hd.model_draws[draw_index], phase) && (hd.model_draws[draw_index].model->kind >= 10) == billboard)
        {
            total_vertices += hd.model_draws[draw_index].model->batch_vertex_count;
            total_indices += source_draw_count(&hd.model_draws[draw_index]);
        }
    if (total_vertices == 0)
        return;
    XASSERT(scene_batch_cursor != NULL);
    if (*scene_batch_cursor == NULL)
    {
        *scene_batch_cursor = (HD_SCENE_BATCH_BANK *)calloc(1, sizeof(HD_SCENE_BATCH_BANK));
        XASSERT(*scene_batch_cursor != NULL);
    }
    bank = *scene_batch_cursor;
    scene_batch_cursor = &bank->next;
    if (bank->vertex_capacity < total_vertices)
    {
        vid_mesh_free(bank->geometry);
        bank->vertex_capacity = scene_batch_capacity(total_vertices);
        bank->geometry = vid_mesh_create_3d(bank->vertex_capacity);
        XASSERT(bank->geometry != NULL);
    }
    if (bank->index_capacity < total_indices)
    {
        vid_index_free(bank->indices);
        bank->index_capacity = scene_batch_capacity(total_indices);
        bank->indices = vid_index_create(bank->index_capacity);
        XASSERT(bank->indices != NULL);
    }
    {
        uint32 count = 0;
        sint32 reuse = !billboard && bank->input_count != 0;
        for (draw_index = 0; draw_index < hd.model_count; ++draw_index)
        {
            const HD_MODEL_DRAW *draw = &hd.model_draws[draw_index];
            HD_BATCH_INPUT input;
            memset(&input, 0, sizeof(input));
            if (!scene_batch_eligible(draw, phase) || (draw->model->kind >= 10) != billboard)
                continue;
            if (count == bank->input_capacity)
            {
                uint32 capacity = bank->input_capacity != 0 ? bank->input_capacity * 2 : 256;
                bank->inputs = (HD_BATCH_INPUT *)realloc(bank->inputs, capacity * sizeof(*bank->inputs));
                XASSERT(bank->inputs != NULL);
                bank->input_capacity = capacity;
            }
            input.model = draw->model;
            input.source_group = draw->source_group;
            input.texture = source_draw_material(draw)->texture;
            input.position = draw->poses.object[1].position;
            input.view = draw->view;
            if (draw->model->has_lighting)
            {
                input.light_history = draw->light_history;
                if (draw->light_history)
                {
                    uint32 sample;
                    for (sample = 0; sample < 2; ++sample)
                    {
                        memcpy(input.lighting[sample].world.m, draw->lighting[sample].world.m, sizeof(input.lighting[sample].world.m));
                        memcpy(input.lighting[sample].color.m, draw->lighting[sample].color.m, sizeof(input.lighting[sample].color.m));
                        memcpy(input.lighting[sample].ambient, draw->lighting[sample].ambient, sizeof(input.lighting[sample].ambient));
                    }
                }
                else
                {
                    memcpy(input.light.m, draw->light.m, sizeof(input.light.m));
                    memcpy(input.color.m, draw->color.m, sizeof(input.color.m));
                    memcpy(input.ambient, draw->ambient, sizeof(input.ambient));
                }
            }
            input.draw_index = draw_index;
            if (count >= bank->input_count || memcmp(&bank->inputs[count], &input, offsetof(HD_BATCH_INPUT, draw_index)) != 0)
                reuse = 0;
            bank->inputs[count++] = input;
        }
        if (count != bank->input_count)
            reuse = 0;
        bank->input_count = count;
        if (reuse)
        {
            ++hd_work.static_batch_reuses;
            goto dispatch;
        }
        if (!billboard)
            ++hd_work.static_batch_builds;
        bank->origin = bank->inputs[0].position;
        bank->origin.x = floorf(bank->origin.x / 1024.0f) * 1024.0f;
        bank->origin.y = floorf(bank->origin.y / 1024.0f) * 1024.0f;
        bank->origin.z = floorf(bank->origin.z / 1024.0f) * 1024.0f;
        bank->group_count = 0;
    }
    vertices = vid_mesh_data_3d(bank->geometry);
    for (draw_index = 0; draw_index < hd.model_count; ++draw_index)
    {
        const HD_MODEL_DRAW *draw = &hd.model_draws[draw_index];
        const HD_MODEL *model = draw->model;
        VEC3F origin = bank->origin;
        MATRIXF transform;
        uint32 vertex;
        if (!scene_batch_eligible(draw, phase) || (model->kind >= 10) != billboard)
            continue;
        if (billboard)
            transform = interpolated_transform(draw);
        else
        {
            IdentityMatrixf(&transform);
            transform.t[0] = draw->poses.object[1].position.x - bank->origin.x;
            transform.t[1] = draw->poses.object[1].position.y - bank->origin.y;
            transform.t[2] = draw->poses.object[1].position.z - bank->origin.z;
        }
        memcpy(vertices + used_vertices, model->batch_vertices, model->batch_vertex_count * sizeof(VID_VERTEX3D));
        for (vertex = 0; vertex < model->batch_vertex_count; ++vertex)
        {
            VID_VERTEX3D *target = &vertices[used_vertices + vertex];
            VECTORF local = {target->pos[0], target->pos[1], target->pos[2]}, view;
            RotTransf(&transform, &local, &view);
            target->pos[0] = view.vx;
            target->pos[1] = view.vy;
            target->pos[2] = view.vz;
        }

        {
            const VID_MATERIAL_DATA *source = source_draw_material(draw);
            HD_SCENE_BATCH_GROUP *target;
            uint32 index;
            XASSERT(source != NULL);
            for (group = 0; group < bank->group_count; ++group)
                if (bank->groups[group].texture == source->texture && scene_batch_compatible(bank->groups[group].reference, draw))
                    break;
            if (group == bank->group_count)
            {
                VID_MATERIAL_DATA *data;
                if (bank->group_count == bank->group_capacity)
                {
                    uint32 capacity = bank->group_capacity != 0 ? bank->group_capacity * 2 : 32;
                    HD_SCENE_BATCH_GROUP *groups = (HD_SCENE_BATCH_GROUP *)realloc(bank->groups, capacity * sizeof(*groups));
                    XASSERT(groups != NULL);
                    memset(groups + bank->group_capacity, 0, (capacity - bank->group_capacity) * sizeof(*groups));
                    bank->groups = groups;
                    bank->group_capacity = capacity;
                }
                target = &bank->groups[bank->group_count++];
                target->count = 0;
                target->reference = draw;
                target->reference_input = input_index;
                target->world_space = !billboard;
                target->origin = origin;
                target->texture = source->texture;
                target->flags = 0;
                if (target->material == NULL)
                    target->material = vid_mat_create();
                XASSERT(target->material != NULL);
                data = vid_mat_data(target->material);
                XASSERT(data != NULL);
                *data = *source;
                data->shader = scene_batch_shader;
                data->params[4] = source->params[4];
                data->params[5] = source->params[5];
                data->params[6] = 0;
                vid_mat_commit(target->material);
            }
            target = &bank->groups[group];
            if (target->capacity < target->count + source_draw_count(draw))
            {
                target->capacity = scene_batch_capacity(target->count + source_draw_count(draw));
                target->indices = (uint32 *)realloc(target->indices, target->capacity * sizeof(uint32));
                XASSERT(target->indices != NULL);
            }
            for (index = 0; index < source_draw_count(draw); ++index)
                target->indices[target->count++] = used_vertices + model->batch_source_indices[(draw->source_group != NULL ? draw->source_group->first : 0) + index];
        }
        used_vertices += model->batch_vertex_count;
        ++input_index;
    }
    indices = vid_index_data(bank->indices);
    for (group = 0; group < bank->group_count; ++group)
    {
        HD_SCENE_BATCH_GROUP *batch = &bank->groups[group];
        batch->first = used_indices;
        memcpy(indices + used_indices, batch->indices, batch->count * sizeof(uint32));
        used_indices += batch->count;
    }
    XASSERT(used_vertices == total_vertices && used_indices == total_indices);
    vid_mesh_commit(bank->geometry);
    vid_index_commit(bank->indices, used_indices);
dispatch:
    for (group = 0; group < bank->group_count; ++group)
    {
        HD_SCENE_BATCH_GROUP *batch = &bank->groups[group];
        batch->reference = &hd.model_draws[bank->inputs[batch->reference_input].draw_index];
        batch->geometry = bank->geometry;
        batch->index_buffer = bank->indices;
        submit_model(batch->reference, phase, batch);
    }
}

static void submit_world(void)
{
    uint32 phase, index;
    for (index = 0; index < hd.model_count; ++index)
    {
        HD_MODEL_DRAW *draw = &hd.model_draws[index];
        if (draw->model->texture_asset != NULL)
        {
            object_texture_material(draw, 0);
            draw->texture_transparent = (uint8)draw->model->texture_asset->transparent;
        }
    }
    for (index = hd.model_count; index > 0; --index)
        if (hd.model_draws[index - 1].model->visibility != NULL)
            submit_model(&hd.model_draws[index - 1], 0, NULL);
    ++static_dispatch_serial;
    for (phase = 0; phase < 3; ++phase)
    {
        if (phase < 2)
        {
            for (index = 0; index < hd.model_count; ++index)
            {
                const HD_MODEL_DRAW *draw = &hd.model_draws[index];
                HD_STATIC_GROUP *group;
                if (draw->source_group == NULL || draw->model->static_pool == NULL || draw->texture_transparent != (phase == 1))
                    continue;
                group = draw->source_group->static_group;
                XASSERT(group != NULL);
                if (group->submitted[draw->view] == static_dispatch_serial)
                    continue;
                group->submitted[draw->view] = static_dispatch_serial;
                submit_model(draw, phase, &group->batch);
            }
            submit_scene_batches(phase, 0);
            submit_scene_batches(phase, 1);
        }
        for (index = 0; index < hd.model_count; ++index)
        {
            const HD_MODEL_DRAW *draw = &hd.model_draws[index];
            if (draw->model->visibility != NULL || (phase < 2 && (draw->model->static_pool != NULL || scene_batch_eligible(draw, phase))))
                continue;
            if (phase < 2 || draw->dynamic != 0 || draw->model->has_transparent)
                submit_model(draw, phase, NULL);
        }
    }
    vid_fog(NULL);
}

static void prepare_scene(void)
{
    HD_UI_MESH *mesh;
    HD_WORLD_MESH *world;
    uint32 index;
    if (hd.prepared)
        return;
    draw_split_separator();
    for (world = hd.world_meshes; world != NULL; world = world->next)
        if (world->used != 0)
            vid_mesh_commit(world->model.geometry);
    qsortPSX(hd.ui_draws, hd.ui_count, sizeof(*hd.ui_draws), compare_ui);
    for (index = 0; index < hd.ui_count; ++index)
    {
        HD_UI_DRAW *draw = &hd.ui_draws[index];
        uint32 *indices = vid_index_data(draw->mesh->indices);
        uint32 vertex, first = draw->first;
        draw->first = draw->mesh->indexed;
        for (vertex = 0; vertex < draw->count; ++vertex)
            indices[draw->mesh->indexed++] = first + vertex;
    }
    for (mesh = hd.ui_meshes; mesh != NULL; mesh = mesh->next)
        if (mesh->used != 0)
        {
            vid_mesh_commit(mesh->geometry);
            vid_index_commit(mesh->indices, mesh->indexed);
        }
    hd.prepared = 1;
}

static void segment_layout(const HD_UI_DRAW *draw, float layout[16])
{
    HD_UI_VERTEX target[2];
    const HD_UI_VERTEX *current = draw->segment[1];
    float ux = current[1].x - current[0].x, uy = current[1].y - current[0].y;
    float source_length = sqrtf(ux * ux + uy * uy), vx, vy, target_length, ratio;
    uint32 point;
    if (source_length <= 0.000001f)
        return;
    for (point = 0; point < 2; ++point)
    {
        target[point].x = draw->segment[0][point].x + hd.render_phase * (current[point].x - draw->segment[0][point].x);
        target[point].y = draw->segment[0][point].y + hd.render_phase * (current[point].y - draw->segment[0][point].y);
    }
    ux /= source_length;
    uy /= source_length;
    vx = target[1].x - target[0].x;
    vy = target[1].y - target[0].y;
    target_length = sqrtf(vx * vx + vy * vy);
    ratio = target_length / source_length;
    if (target_length > 0.000001f)
    {
        vx /= target_length;
        vy /= target_length;
    }
    else
    {
        vx = ux;
        vy = uy;
    }
    layout[8] = ratio * vx * ux + vy * uy;
    layout[9] = ratio * vx * uy - vy * ux;
    layout[10] = ratio * vy * ux - vx * uy;
    layout[11] = ratio * vy * uy + vx * ux;
    layout[12] = target[0].x - layout[8] * current[0].x - layout[9] * current[0].y;
    layout[13] = target[0].y - layout[10] * current[0].x - layout[11] * current[0].y;
}

static void needle_layout(const HD_NEEDLE *needle, float layout[16])
{
    float angle = needle->angles[0] + hd.render_phase * (needle->angles[1] - needle->angles[0]);
    float cosine = cosf(angle), sine = sinf(angle);
    layout[8] = cosine;
    layout[9] = -sine;
    layout[10] = needle->vertical_scale * sine;
    layout[11] = needle->vertical_scale * cosine;
    layout[12] = needle->center_x - layout[8] * needle->center_x - layout[9] * needle->center_y;
    layout[13] = needle->center_y - layout[10] * needle->center_x - layout[11] * needle->center_y;
}

static sint32 sun_layout(const HD_UI_DRAW *draw, float layout[16])
{
    const HD_SUN_ATTACHMENT *sun = &draw->sun;
    HD_POSE camera, object;
    HD_PROJECTION projection;
    QUATF facing;
    MATRIXF view, rotation;
    VECTORF relative, center;
    float x = 0.0f, y = 0.0f, depth = 0.0f;
    uint32 point;
    if (!sun->active)
        return 1;
    hd_pose_interpolate(&sun->camera[0], &sun->camera[1], hd.render_phase, &camera);
    hd_pose_interpolate(&sun->object[0], &sun->object[1], hd.render_phase, &object);
    projection = interpolated_projection(sun->projection);
    QuatMatrixf(&camera.rotation, &view);
    relative = (VECTORF){object.position.x, object.position.y, object.position.z};
    if (!sun->relative)
    {
        relative.vx -= camera.position.x;
        relative.vy -= camera.position.y;
        relative.vz -= camera.position.z;
    }
    ApplyMatrixf(&view, &relative, &center);
    SlerpQuatf(&sun->facing[0], &sun->facing[1], hd.render_phase, &facing);
    QuatMatrixf(&facing, &rotation);
    rotation.t[0] = center.vx;
    rotation.t[1] = center.vy;
    rotation.t[2] = center.vz;
    for (point = 0; point < 4; ++point)
    {
        VECTORF position;
        RotTransf(&rotation, &sun->points[point], &position);
        if (position.vz <= 1.0f)
            return 0;
        x += projection.center_x + position.vx * projection.x / position.vz;
        y += projection.center_y + position.vy * projection.y / position.vz;
        depth += fminf(65535.0f, floorf(position.vz / sun->depth_scale));
    }
    x *= 0.25f;
    y *= 0.25f;
    depth = floorf(depth * sun->zsf4 / 4096.0f);
    if (depth <= 0.0f || depth >= fminf(2048.0f, sun->far_otz) || !reference_contains(&hd.viewports[draw->view], x, y))
        return 0;
    layout[12] += (x - sun->half_width) * sun->factor;
    layout[13] += (y - sun->half_height) * sun->factor;
    layout[14] = floorf((2048.0f - depth) / 16.0f) / 255.0f;
    return 1;
}

static sint32 attachment_layout(const HD_UI_ATTACHMENT *attachment, float layout[16])
{
    HD_POSE camera, object;
    MATRIXF view, model;
    VECTORF local, relative, point;
    HD_PROJECTION projection;
    if (!attachment->active)
        return 1;
    hd_pose_interpolate(&attachment->camera[0], &attachment->camera[1], hd.render_phase, &camera);
    hd_pose_interpolate(&attachment->object[0], &attachment->object[1], hd.render_phase, &object);
    QuatMatrixf(&object.rotation, &model);
    QuatMatrixf(&camera.rotation, &view);
    ApplyMatrixf(&model, &attachment->local, &local);
    relative = (VECTORF){local.vx + object.position.x - camera.position.x, local.vy + object.position.y - camera.position.y, local.vz + object.position.z - camera.position.z};
    ApplyMatrixf(&view, &relative, &point);
    if (point.vz <= 1.0f)
        return 0;
    projection = interpolated_projection(attachment->projection);
    layout[12] += projection.center_x + point.vx * projection.x / point.vz - attachment->source[0];
    layout[13] += projection.center_y + point.vy * projection.y / point.vz - attachment->source[1];
    return 1;
}

static void submit_minimap(const HD_UI_DRAW *draw)
{
    const HD_MAP_DRAW *map = &draw->map;
    const HD_VIEW *view = &hd.viewports[draw->view];
    float aspect = view->viewport.h > 0 ? (float)view->viewport.w / view->viewport.h : view->reference_aspect;
    float angle = map->angles[0] + hd.render_phase * (map->angles[1] - map->angles[0]);
    float uniforms[6][4] = {{0}};
    uniforms[0][0] = map->origin[0].vx + hd.render_phase * (map->origin[1].vx - map->origin[0].vx);
    uniforms[0][1] = map->origin[0].vz + hd.render_phase * (map->origin[1].vz - map->origin[0].vz);
    uniforms[0][2] = cosf(angle);
    uniforms[0][3] = sinf(angle);
    uniforms[1][0] = map->scale[0];
    uniforms[1][1] = map->scale[1];
    uniforms[1][2] = (float)map->mode;
    uniforms[1][3] = draw->blend != 0 ? 1.0f : 0.0f;
    uniforms[2][0] = map->offset[0];
    uniforms[2][1] = map->offset[1];
    uniforms[2][2] = (float)map->clip.x;
    uniforms[2][3] = (float)map->clip.y;
    uniforms[3][0] = (float)map->clip.w;
    uniforms[3][1] = (float)map->clip.h;
    uniforms[3][2] = map->marker[0].vx + hd.render_phase * (map->marker[1].vx - map->marker[0].vx);
    uniforms[3][3] = map->marker[0].vz + hd.render_phase * (map->marker[1].vz - map->marker[0].vz);
    uniforms[4][0] = view->reference.w * aspect / view->reference_aspect;
    uniforms[4][1] = view->reference.x + view->reference.w * 0.5f;
    uniforms[4][2] = (float)view->reference.y;
    uniforms[4][3] = (float)view->reference.h;
    uniforms[5][0] = (float)view->reference.w;
    vid_viewport(view->viewport);
    vid_uniforms(&uniforms[0][0], 6);
    vid_pass(hd.map_materials[0]);
    vid_submit(draw->mesh->geometry, draw->mesh->indices, draw->first, draw->count);
    if (draw->blend != 0)
    {
        vid_pass(hd.map_materials[draw->blend]);
        vid_submit(draw->mesh->geometry, draw->mesh->indices, draw->first, draw->count);
    }
}

static void submit_scene(void)
{
    uint32 index;
    for (index = 0; index < hd.ui_count; ++index)
    {
        HD_UI_DRAW *draw = &hd.ui_draws[index];
        uint32 count = draw->count;
        if (draw->layer != 0xFFFFFFFFu && (index == 0 || hd.ui_draws[index - 1].layer == 0xFFFFFFFFu))
        {
            submit_world();
        }
        if (draw->map.active)
        {
            submit_minimap(draw);
            continue;
        }
        while (!draw->particle_active && !draw->segment_active && !draw->fading_out && !draw->attachment.active && !draw->sun.active && !draw->needle.active && draw->blend == 0 && index + 1 < hd.ui_count)
        {
            HD_UI_DRAW *next = &hd.ui_draws[index + 1];
            if (next->particle_active || next->segment_active || next->fading_out || next->attachment.active || next->sun.active || next->needle.active || next->map.active || next->sprite != draw->sprite || next->mesh != draw->mesh || next->view != draw->view || next->layer != draw->layer || next->blend != draw->blend || next->first != draw->first + count)
                break;
            count += next->count;
            ++index;
        }
        vid_viewport(hd.viewports[draw->view].viewport);
        {
            const HD_VIEW *view = &hd.viewports[draw->view];
            float aspect = view->viewport.h > 0 ? (float)view->viewport.w / view->viewport.h : view->reference_aspect;
            float layout[16] = {view->reference.w * aspect / view->reference_aspect, view->reference.x + view->reference.w * 0.5f, (float)view->reference.y, (float)view->reference.h, (float)view->reference.w, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f};
            if (draw->particle_active)
            {
                const HD_PARTICLE_SAMPLE *previous = &draw->particle[0], *current = &draw->particle[1];
                float scale = (previous->size + hd.render_phase * (current->size - previous->size)) / current->size;
                layout[8] = layout[11] = scale;
                layout[12] = previous->position.vx + hd.render_phase * (current->position.vx - previous->position.vx) - (current->brightness < 0.0f ? 0.0f : scale * current->position.vx);
                layout[13] = previous->position.vy + hd.render_phase * (current->position.vy - previous->position.vy) - (current->brightness < 0.0f ? 0.0f : scale * current->position.vy);
                layout[14] = previous->brightness + hd.render_phase * (current->brightness - previous->brightness);
            }
            if (draw->segment_active)
                segment_layout(draw, layout);
            if (draw->needle.active)
                needle_layout(&draw->needle, layout);
            if (draw->fading_out)
                layout[15] = 1.0f - hd.render_phase;
            if (!attachment_layout(&draw->attachment, layout))
                continue;
            if (!sun_layout(draw, layout))
                continue;
            vid_uniforms(layout, 4);
        }
        vid_pass(draw->sprite != NULL ? sprite_texture_material(draw->sprite, 0, 0) : draw->fading_out ? hd.ui_retire_materials[0] : hd.ui_materials[0]);
        vid_submit(draw->mesh->geometry, draw->mesh->indices, draw->first, count);
        if (draw->blend != 0)
        {
            vid_pass(draw->sprite != NULL ? sprite_texture_material(draw->sprite, draw->blend, 0) : draw->fading_out ? hd.ui_retire_materials[draw->blend] : hd.ui_materials[draw->blend]);
            vid_submit(draw->mesh->geometry, draw->mesh->indices, draw->first, count);
        }
    }
    if (hd.ui_count == 0 || hd.ui_draws[hd.ui_count - 1].layer == 0xFFFFFFFFu)
        submit_world();
}

static void swap_scene_buffers(HD_SCENE_BUFFERS *other)
{
    HD_SCENE_BUFFERS current = {0};
    current.ui_meshes = hd.ui_meshes;
    current.ui_draws = hd.ui_draws;
    current.ui_count = hd.ui_count;
    current.ui_capacity = hd.ui_capacity;
    current.world_meshes = hd.world_meshes;
    current.model_draws = hd.model_draws;
    current.model_count = hd.model_count;
    current.model_capacity = hd.model_capacity;
    current.views = hd.views;
    current.width = hd.width;
    current.height = hd.height;
    current.prepared = hd.prepared;
    memcpy(current.viewports, hd.viewports, sizeof(current.viewports));
    memcpy(current.clear, hd.clear, sizeof(current.clear));
    hd.ui_meshes = other->ui_meshes;
    hd.ui_draws = other->ui_draws;
    hd.ui_count = other->ui_count;
    hd.ui_capacity = other->ui_capacity;
    hd.world_meshes = other->world_meshes;
    hd.model_draws = other->model_draws;
    hd.model_count = other->model_count;
    hd.model_capacity = other->model_capacity;
    hd.views = other->views;
    hd.width = other->width;
    hd.height = other->height;
    hd.prepared = other->prepared;
    memcpy(hd.viewports, other->viewports, sizeof(hd.viewports));
    memcpy(hd.clear, other->clear, sizeof(hd.clear));
    *other = current;
}

static void submit_frozen_scene(void)
{
    sint32 width = hd.width, height = hd.height;
    float menu_phase = hd.render_phase;
    uint32 view;
    hd.render_phase = hd.pause_phase;
    swap_scene_buffers(&hd.frozen);
    hd.width = width;
    hd.height = height;
    for (view = 0; view < hd.views; ++view)
    {
        HD_VIEW *target = &hd.viewports[view];
        const VID_RECT *reference = &target->reference;
        target->viewport.x = (sint32)((sint64)width * reference->x / model_render.width);
        target->viewport.y = (sint32)((sint64)height * reference->y / model_render.height);
        target->viewport.w = (sint32)((sint64)width * (reference->x + reference->w) / model_render.width) - target->viewport.x;
        target->viewport.h = (sint32)((sint64)height * (reference->y + reference->h) / model_render.height) - target->viewport.y;
    }
    hd.viewports[HD_VIEWS].viewport = (VID_RECT){0, 0, width, height};
    hd.viewports[HD_VIEWS].reference_aspect = height > 0 ? (float)width / height : 4.0f / 3.0f;
    submit_scene();
    swap_scene_buffers(&hd.frozen);
    hd.render_phase = menu_phase;
}

void hd_reseed_race(void)
{
    ++hd.motion_generation;
    hd.race_ready = 0;
}

void hd_present_frame(uint64 processed_field, uint32 fields)
{
    uint64 field;
    uint64 elapsed;
    uint32 view;
    float fraction;
    sint32 width, height, presented;
    XASSERT(!hd.collecting);
    if (!game_hd_frame)
    {
        gpu_present();
        return;
    }
    if (!hd.frame_ready || !hd.prepared)
        return;
    XASSERT(fields != 0);
    fraction = xport_frame_fract(&field);
    elapsed = field >= processed_field ? field - processed_field : 0;
    vid_resize(&width, &height);
    hd.width = width;
    hd.height = height;
    for (view = 0; view < hd.views; ++view)
    {
        HD_VIEW *target = &hd.viewports[view];
        const VID_RECT *reference = &target->reference;
        target->viewport.x = (sint32)((sint64)width * reference->x / model_render.width);
        target->viewport.y = (sint32)((sint64)height * reference->y / model_render.height);
        target->viewport.w = (sint32)((sint64)width * (reference->x + reference->w) / model_render.width) - target->viewport.x;
        target->viewport.h = (sint32)((sint64)height * (reference->y + reference->h) / model_render.height) - target->viewport.y;
    }
    hd.viewports[HD_VIEWS].viewport = (VID_RECT){0, 0, width, height};
    hd.viewports[HD_VIEWS].reference_aspect = height > 0 ? (float)width / height : 4.0f / 3.0f;
    hd.render_phase = ((float)elapsed + fraction) / fields;
    if (hd.render_phase > 1.0f)
        hd.render_phase = 1.0f;
    if (vid_begin(hd.clear[0], hd.clear[1], hd.clear[2]))
    {
        upload_vram();
        scene_batch_cursor = &scene_batch_banks;
        if (hd.paused)
            submit_frozen_scene();
        submit_scene();
        presented = vid_end();
        XASSERT(presented);
        if (presented && !hd.paused && hd.race_frame)
            hd.presented_phase = hd.render_phase;
    }
}

void hd_capture_pause(sint32 requested)
{
    if (!hd.initialized || !hd.collecting || !requested)
        return;
    hd.capture_sources = 1;
    hd.capture_native = 1;
}

void hd_native_begin(uint32 view, const DRAWENV *environment)
{
    sint32 recorded;
    if (!hd.capture_native)
        return;
    XASSERT(view < hd.views);
    hd.native_environments[view] = *environment;
    recorded = gpu_render_list_begin(hd.native_lists[view], environment);
    XASSERT(recorded);
}

void hd_native_end(uint32 view)
{
    sint32 recorded;
    if (!hd.capture_native)
        return;
    XASSERT(view < hd.views);
    recorded = gpu_render_list_end(hd.native_lists[view]);
    XASSERT(recorded);
    hd.native_valid[view] = recorded != 0;
}

void hd_pause_begin(void)
{
    uint32 view;
    if (!hd.initialized)
        return;
    hd.pause_phase = game_hd_frame ? hd.presented_phase : 1.0f;
    if (hd.collecting)
        hd_end_frame();
    XASSERT(!hd.collecting && !hd.paused && hd.capture_sources && hd.prepared);
    if (game_hd_frame)
    {
        uint32 draw_bank, capture_bank;
        gpu_frame_begin();
        for (view = 0; view < hd.views; ++view)
        {
            sint32 replayed;
            XASSERT(hd.native_valid[view]);
            replayed = gpu_render_list_replay(hd.native_lists[view]);
            XASSERT(replayed);
        }
        draw_bank = ((uint32)hd.native_environments[0].ofs[1] >> 8) & 1;
        capture_bank = disp_state.bank ^ 1;
        if (draw_bank != capture_bank)
        {
            PSX_RECT source = {0, (sint16)(draw_bank * 256), (sint16)model_render.width, 256};
            MoveImage(&source, 0, (sint32)(capture_bank * 256));
        }
        gpu_frame_end();
    }
    swap_scene_buffers(&hd.frozen);
    discard_draws(hd.model_draws, &hd.model_count);
    hd.ui_count = 0;
    hd.prepared = 0;
    hd.frame_ready = 0;
    hd.race_ready = 0;
    hd.race_frame = 0;
    hd.paused = 1;
}

void hd_pause_end(void)
{
    if (!hd.paused)
        return;
    XASSERT(!hd.collecting);
    swap_scene_buffers(&hd.frozen);
    hd.paused = 0;
    ++hd.motion_generation;
}

void hd_end_frame(void)
{
    if (!hd.collecting)
        return;
    if (game_hd_frame && hd.race_frame)
        XASSERT(hd.race_pose_views == (1u << hd.views) - 1u);
    if (hd.capture_sources)
        prepare_scene();
    if (!game_hd_frame)
    {
        if (hd.fresh_og)
        {
            DISPENV display;
            sint32 read = gpu_get_display_env(&display);
            uint32 draw_bank = ((uint32)hd.native_environments[0].ofs[1] >> 8) & 1;
            uint32 display_bank;
            XASSERT(read);
            display_bank = ((uint32)display.disp.y >> 8) & 1;
            if (draw_bank != display_bank)
            {
                PSX_RECT source = {0, (sint16)(draw_bank * 256), (sint16)model_render.width, 256};
                MoveImage(&source, 0, (sint32)(display_bank * 256));
            }
            hd.fresh_og = 0;
        }
    }
    // publish completed source commands without presenting a catch-up step
    if (game_hd_frame && hd.frame_started)
        vid_cancel();
    else if (!game_hd_frame)
        gpu_frame_end();
    hd.collecting = 0;
    hd.frame_started = 0;
    hd.frame_ready = 1;
    hd.race_ready = game_hd_frame && hd.race_frame && hd.prepared && !hd.paused;
    if (!hd.capture_sources)
        release_retired_models();
}

typedef struct HD_SEAM_POINT
{
    sint32 position[3], lower[3], upper[3];
    uint32 section, parent, next;
} HD_SEAM_POINT;

static uint32 seam_bucket(const sint32 position[3], uint32 mask)
{
    return ((uint32)position[0] * 73856093u ^ (uint32)position[1] * 19349663u ^ (uint32)position[2] * 83492791u) & mask;
}

static uint32 seam_root(HD_SEAM_POINT *points, uint32 index)
{
    uint32 root = index;
    while (points[root].parent != root)
        root = points[root].parent;
    while (index != root)
    {
        uint32 next = points[index].parent;
        points[index].parent = root;
        index = next;
    }
    return root;
}

void hd_weld_course(HD_PREPARED_MODEL *const *models, const uint32 *sections, uint32 count, uint32 section_count)
{
    HD_SEAM_POINT *points;
    uint32 *buckets, capacity = 1, total = 0, used = 0, item, stage;
    for (item = 0; item < count; ++item)
        if (models[item] != NULL && models[item]->kind < 10)
            total += models[item]->batch_vertex_count;
    if (total == 0)
        return;
    while (capacity < total)
        capacity *= 2;
    buckets = (uint32 *)calloc(capacity, sizeof(*buckets));
    points = (HD_SEAM_POINT *)calloc(total + 1, sizeof(*points));
    XASSERT(points != NULL && buckets != NULL);
    for (stage = 0; stage < 2; ++stage)
        for (item = 0; item < count; ++item)
        {
            HD_MODEL *model = models[item];
            const MODEL_HEADER *header;
            VID_VERTEX16 *vertices;
            uint32 vertex, axis;
            sint32 origin[3];
            if (model == NULL || model->kind >= 10)
                continue;
            header = (const MODEL_HEADER *)model->source;
            for (axis = 0; axis < 3; ++axis)
                origin[axis] = header->position.v[axis] >> 8;
            vertices = vid_mesh_data_16(model->geometry);
            XASSERT(vertices != NULL);
            for (vertex = 0; vertex < model->batch_vertex_count; ++vertex)
            {
                sint32 position[3];
                uint32 bucket, index;
                for (axis = 0; axis < 3; ++axis)
                    position[axis] = origin[axis] + vertices[vertex].pos[axis];
                bucket = seam_bucket(position, capacity - 1);
                for (index = buckets[bucket]; index != 0; index = points[index].next)
                    if (points[index].section == sections[item] && memcmp(position, points[index].position, sizeof(position)) == 0)
                        break;
                if (stage == 0 && index == 0)
                {
                    sint32 dx, dy, dz;
                    index = ++used;
                    XASSERT(used <= total);
                    memcpy(points[index].position, position, sizeof(position));
                    memcpy(points[index].lower, position, sizeof(position));
                    memcpy(points[index].upper, position, sizeof(position));
                    points[index].parent = index;
                    points[index].section = sections[item];
                    points[index].next = buckets[bucket];
                    buckets[bucket] = index;
                    // join only neighboring authored sections within one coordinate unit
                    for (dx = -1; dx <= 1; ++dx)
                        for (dy = -1; dy <= 1; ++dy)
                            for (dz = -1; dz <= 1; ++dz)
                            {
                                sint32 candidate[3] = {position[0] + dx, position[1] + dy, position[2] + dz};
                                uint32 other;
                                for (other = buckets[seam_bucket(candidate, capacity - 1)]; other != 0; other = points[other].next)
                                {
                                    uint32 a, b, distance = points[other].section > sections[item] ? points[other].section - sections[item] : sections[item] - points[other].section;
                                    sint32 lower[3], upper[3];
                                    if ((distance != 1 && distance != section_count - 1) || memcmp(candidate, points[other].position, sizeof(candidate)) != 0)
                                        continue;
                                    a = seam_root(points, index);
                                    b = seam_root(points, other);
                                    if (a == b)
                                        continue;
                                    for (axis = 0; axis < 3; ++axis)
                                    {
                                        lower[axis] = points[a].lower[axis] < points[b].lower[axis] ? points[a].lower[axis] : points[b].lower[axis];
                                        upper[axis] = points[a].upper[axis] > points[b].upper[axis] ? points[a].upper[axis] : points[b].upper[axis];
                                        if (upper[axis] - lower[axis] > 1)
                                            break;
                                    }
                                    if (axis != 3)
                                        continue;
                                    points[b].parent = a;
                                    memcpy(points[a].lower, lower, sizeof(lower));
                                    memcpy(points[a].upper, upper, sizeof(upper));
                                }
                            }
                }
                else if (stage == 1)
                {
                    uint32 root;
                    XASSERT(index != 0);
                    root = seam_root(points, index);
                    for (axis = 0; axis < 3; ++axis)
                    {
                        sint32 local = points[root].lower[axis] - origin[axis];
                        XASSERT(local >= -32768 && local <= 32767);
                        if (vertices[vertex].pos[axis] != local)
                            ++hd_work.seam_components;
                        vertices[vertex].pos[axis] = (sint16)local;
                        model->batch_vertices[vertex].pos[axis] = (float)local;
                    }
                }
            }
            if (stage == 1)
                vid_mesh_commit(model->geometry);
        }
    free(points);
    free(buckets);
}

HD_PREPARED_MODEL *hd_prepare_track_model(const MODEL_RESOURCE *resource, const uint16 *pixels)
{
    HD_MODEL *model = find_model(resource, NULL, 0);
    if (model == NULL)
        model = create_model(resource, NULL, 0);
    if (model != NULL && model->texture_asset != NULL && !model->texture_asset->immutable)
    {
        HD_OBJECT_TEXTURE *asset = model->texture_asset;
        uint32 word;
        if (pixels == NULL)
            pixels = VRAM;
        for (word = 0; word < asset->word_count; ++word)
        {
            uint32 value = pixels[asset->word_indices[word]];
            asset->words[word] = (uint16)(((value & 31) << 11) | (((value >> 5) & 31) << 6) | (((value >> 10) & 31) << 1) | (value >> 15));
        }
        free(asset->word_indices);
        asset->word_indices = NULL;
        asset->immutable = 1;
    }
    return model;
}

void hd_prepare_track_texture(HD_PREPARED_MODEL *model)
{
    HD_MODEL_DRAW draw = {0};
    uint32 pass;
    if (model == NULL)
        return;
    draw.model = model;
    for (pass = 0; model->texture_asset != NULL && pass < HD_BLEND_PASSES; ++pass)
        object_texture_material(&draw, pass);
    if (preparation_active)
    {
        vid_pass(model->texture_asset != NULL ? object_texture_material(&draw, 0) : hd.environment_materials[0]);
        vid_submit(model->geometry, model->indices, 0, model->index_count);
    }
}

HD_PREPARED_MODEL *hd_prepare_model(const MODEL_RESOURCE *resource, HD_TRACK_IMAGE *image, uint32 variant)
{
    HD_MODEL *model = find_model(resource, NULL, variant);
    HD_MODEL_DRAW draw = {0};
    uint32 pass;
    if (model == NULL)
        model = create_model(resource, NULL, variant);
    if (model == NULL)
        return NULL;
    if (model->uses_sources)
        return model;
    draw.model = model;
    draw.image = image;
    if (model->texture_asset != NULL)
    {
        HD_OBJECT_TEXTURE *asset = model->texture_asset;
        object_texture_material(&draw, 0);
        if (image != NULL && !asset->immutable)
        {
            uint32 chart;
            asset->immutable = 1;
            asset->source = NULL;
            free(asset->words);
            free(asset->word_indices);
            asset->words = NULL;
            asset->word_indices = NULL;
            asset->word_count = 0;
            for (chart = 0; chart < asset->count; ++chart)
            {
                free(asset->charts[chart].used);
                asset->charts[chart].used = NULL;
            }
        }
    }
    if (model->texture_asset != NULL)
        for (pass = 0; pass < HD_BLEND_PASSES; ++pass)
            object_texture_material(&draw, pass);
    if (preparation_active)
        for (pass = 0; pass < (preparation_blends_ready ? 1u : (uint32)HD_BLEND_PASSES); ++pass)
        {
            vid_pass(model->texture_asset != NULL ? object_texture_material(&draw, pass) : hd.environment_materials[pass]);
            vid_submit(model->geometry, model->indices, 0, model->index_count);
        }
    if (model->texture_asset != NULL)
        preparation_blends_ready = 1;
    return model;
}

void hd_prepare_sky_model(const MODEL_RESOURCE *resource, const sint8 *visibility)
{
    HD_MODEL *model = find_model(resource, visibility, 0);
    if (model == NULL)
        model = create_model(resource, visibility, 0);
    if (model != NULL && preparation_active)
    {
        vid_pass(hd.sky_materials[0]);
        vid_submit(model->geometry, model->indices, 0, model->index_count);
    }
}

void hd_prepare_draw_capacity(uint32 count)
{
    uint32 index;
    for (index = 0; index < 3; ++index)
    {
        HD_MODEL_DRAW **draws = index == 0 ? &hd.model_draws : index == 1 ? &hd.completed.model_draws : &hd.frozen.model_draws;
        uint32 *capacity = index == 0 ? &hd.model_capacity : index == 1 ? &hd.completed.model_capacity : &hd.frozen.model_capacity;
        if (*capacity < count)
        {
            *draws = (HD_MODEL_DRAW *)realloc(*draws, count * sizeof(HD_MODEL_DRAW));
            XASSERT(*draws != NULL);
            *capacity = count;
        }
    }
}

void hd_prepare_race_resources(void)
{
    float uniforms[30][4] = {{0}};
    uint32 car, lod, variant;
    VID_SAMPLE *completion;
    if (!hd.initialized)
        return;
    XASSERT(!hd.collecting);
    upload_vram();
    preparation_blends_ready = 0;
    preparation_target = vid_target_create();
    XASSERT(preparation_target != NULL);
    preparation_active = vid_begin(0, 0, 0);
    if (preparation_active)
    {
        sint32 selected = vid_target_select_offscreen(preparation_target);
        XASSERT(selected);
        vid_viewport((VID_RECT){0, 0, 1, 1});
        uniforms[0][0] = uniforms[1][1] = uniforms[2][2] = 1;
        uniforms[2][3] = 10000000.0f;
        uniforms[3][0] = uniforms[3][1] = uniforms[3][2] = 1;
        uniforms[3][3] = 1000000.0f;
        uniforms[5][3] = uniforms[29][3] = 1;
        uniforms[11][3] = -1;
        vid_uniforms(&uniforms[0][0], 30);
        vid_fog(NULL);
    }
    hd_track_prepare();
    hd_texture_sources_native(VRAM);
    {
        uint32 table, offset;
        memcpy(&table, scene_track_image.bytes + 8, sizeof(table));
        for (offset = table; offset + sizeof(TEXTURE_DESCRIPTOR) <= scene_track_image.size; offset += sizeof(TEXTURE_DESCRIPTOR))
        {
            const TEXTURE_DESCRIPTOR *texture = (const TEXTURE_DESCRIPTOR *)(scene_track_image.bytes + offset);
            HD_SOURCE_TEXTURE *source;
            if (texture->name[0] == 0)
                break;
            source = hd_texture_source_named(texture->page, texture->clut, texture->u, texture->v, texture->width != 0 ? texture->width : 256, texture->height != 0 ? texture->height : 256, VRAM);
            hd_texture_source_semitransparent(source);
        }
    }
    for (car = 0; car < race_control.car_count; ++car)
        for (lod = 0; lod < 2; ++lod)
        {
            MODEL_RESOURCE *resource = &car_states[car].render_models[lod];
            if (resource->bytes == NULL || resource->capacity < sizeof(MODEL_HEADER) || memcmp(resource->bytes, "\0PSMODL", 8) != 0)
                continue;
            for (variant = 0; variant < 3; ++variant)
                if (find_model(resource, NULL, variant) == NULL)
                    create_model(resource, NULL, variant);
        }
    hd_texture_sources_seal();
    hd_texture_sources_build_gpu();
    {
        HD_MODEL *model;
        for (model = hd.models; model != NULL; model = model->next)
        {
            uint32 group, pass;
            if (!model->uses_sources)
            {
                if (model->texture_asset != NULL)
                {
                    MODEL_RESOURCE resource = {(uint8 *)model->source, model->source_size};
                    hd_prepare_model(&resource, NULL, model->variant);
                }
                continue;
            }
            prepare_source_model(model);
            for (group = 0; group < model->source_group_count; ++group)
            {
                HD_CACHED_GROUP *source = &model->source_groups[group];
                for (pass = 0; pass < HD_BLEND_PASSES; ++pass)
                    source_material(source->texture, pass, model->kind == 12 ? 0 : 2, source->transparent);
            }
        }
    }
    hd_track_prepare_gpu();
    if (preparation_active)
    {
        HD_MODEL *model;
        for (model = hd.models; model != NULL; model = model->next)
            if (model->uses_sources && model->static_pool == NULL)
            {
                uint32 group;
                for (group = 0; group < model->source_group_count; ++group)
                {
                    HD_CACHED_GROUP *source = &model->source_groups[group];
                    vid_pass(source_material(source->texture, 0, model->kind == 12 ? 0 : 2, source->transparent)->material);
                    vid_submit(model->geometry, model->indices, source->first, source->count);
                }
            }
    }
    sky_prepare_hd();
    for (car = 0; car < race_control.car_count; ++car)
        for (lod = 0; lod < 2; ++lod)
        {
            MODEL_RESOURCE *resource = &car_states[car].render_models[lod];
            if (resource->bytes == NULL || resource->capacity < sizeof(MODEL_HEADER) || memcmp(resource->bytes, "\0PSMODL", 8) != 0)
                continue;
            for (variant = 0; variant < 3; ++variant)
                hd_prepare_model(resource, NULL, variant);
        }
    if (preparation_active)
    {
        HD_MODEL *model;
        VID_GEOMETRY *warm_geometry = vid_mesh_create_3d(3);
        VID_GEOMETRY *sprite_geometry = vid_mesh_create_2d(3);
        VID_MATERIAL *warm_material = vid_mat_create();
        uint32 table, offset, view, slot, part;
        VID_FOG fog = {hd_track_horizon() / 3.0f, hd_track_horizon() * (5.0f / 6.0f), hd_track_horizon(), {track_fog_colors[race_session.track][0] / 255.0f, track_fog_colors[race_session.track][1] / 255.0f, track_fog_colors[race_session.track][2] / 255.0f}, {180, 180}};
        vid_fog(&fog);
        XASSERT(warm_geometry != NULL && sprite_geometry != NULL && warm_material != NULL);
        memset(vid_mesh_data_2d(sprite_geometry), 0, 3 * sizeof(VID_VERTEX2D));
        vid_mesh_commit(sprite_geometry);
        hd_texture_sources_visit_banks(warm_source_bank, sprite_geometry);
        memset(vid_mesh_data_3d(warm_geometry), 0, 3 * sizeof(VID_VERTEX3D));
        vid_mesh_commit(warm_geometry);
        for (model = hd.models; model != NULL; model = model->next)
            if (model->texture_asset != NULL)
            {
                HD_OBJECT_TEXTURE *asset = model->texture_asset;
                VID_MATERIAL_DATA *data = vid_mat_data(warm_material);
                memset(data, 0, sizeof(*data));
                data->shader = scene_batch_shader;
                data->texture = asset->image->texture;
                data->filter = VID_FILTER_ANISOTROPIC;
                data->depth_test = data->depth_write = 1;
                data->params[0] = data->params[1] = data->params[2] = 1;
                data->params[4] = (float)asset->width;
                data->params[5] = (float)asset->height;
                vid_mat_commit(warm_material);
                vid_pass(warm_material);
                vid_submit(warm_geometry, NULL, 0, 3);
                break;
            }
        memcpy(&table, scene_track_image.bytes + 8, sizeof(table));
        for (offset = table; offset <= scene_track_image.size && sizeof(TEXTURE_DESCRIPTOR) <= scene_track_image.size - offset; offset += sizeof(TEXTURE_DESCRIPTOR))
        {
            TEXTURE_DESCRIPTOR descriptor;
            uint32 name;
            memcpy(&descriptor, scene_track_image.bytes + offset, sizeof(descriptor));
            if (descriptor.name[0] == 0)
                break;
            for (name = 0; name < 2; ++name)
            {
                HD_TEXTURE texture = {descriptor.page, descriptor.clut, descriptor.u, descriptor.v, descriptor.width != 0 ? descriptor.width : 256, descriptor.height != 0 ? descriptor.height : 256, HD_TEXTURED};
                HD_SPRITE_TEXTURE *sprite;
                uint32 pass, world;
                if (name != 0)
                    texture.flags |= HD_ZERO_OPAQUE;
                sprite = sprite_texture_get(&texture);
                for (world = 0; world < 2; ++world)
                    for (pass = 0; pass < HD_BLEND_PASSES; ++pass)
                    {
                        vid_uniforms(&uniforms[0][0], world ? 30 : 4);
                        vid_pass(sprite_texture_material(sprite, pass, (sint32)world));
                        vid_submit(sprite_geometry, NULL, 0, 3);
                    }
            }
        }

        for (view = 0; view < HD_VIEWS; ++view)
        {
            for (slot = 0; slot < 80; ++slot)
                for (part = 0; part < 2; ++part)
                {
                    HD_MODEL *quad = &hd.skids[view][slot].model[part];
                    init_effect_quad(quad);
                    vid_pass(hd.effect_materials[0]);
                    vid_submit(quad->geometry, quad->indices, 0, 6);
                }
            for (slot = 0; slot < 14; ++slot)
                for (part = 0; part < 2; ++part)
                {
                    HD_MODEL *quad = &hd.shadows[view][slot][part].model;
                    init_effect_quad(quad);
                    vid_pass(hd.effect_materials[3]);
                    vid_submit(quad->geometry, quad->indices, 0, 6);
                }
        }
        vid_mesh_free(sprite_geometry);
        vid_mesh_free(warm_geometry);
        vid_mat_free(warm_material);
    }
    if (preparation_active)
    {
        sint32 complete;
        completion = vid_sample_create();
        XASSERT(completion != NULL);
        complete = vid_sample(completion, 0, 0);
        XASSERT(complete);
        complete = vid_end();
        XASSERT(complete);
        vid_sample_free(completion);
    }
    preparation_active = 0;
    vid_target_free(preparation_target);
    preparation_target = NULL;
}
