#ifndef CAR_H
#define CAR_H

#include "psx.h"
#include "fixed_math.h"
#include "track_geometry.h"
#include "model_resource.h"

typedef struct CAR_LIGHTING_ZONES
{
    const uint8 *bytes;
    uint32 size;
    uint32 offset;
    uint32 count;
} CAR_LIGHTING_ZONES;

extern CAR_LIGHTING_ZONES car_lighting_zones;
void car_init_lighting_zones(void);

extern const char *const car_asset_names[45];
void car_load_race_models(void);

typedef struct CAR_ENVIRONMENT
{
    uint16 level;
    uint8 parameter;
    uint8 type;
    uint8 selection;
    uint8 phase;
    uint8 limit;
} CAR_ENVIRONMENT;

typedef struct CAR_RACE_STATISTICS
{
    uint32 speed_sum;
    uint16 top_speed;
    uint16 avg_speed;
    uint16 elapsed_ticks;
} CAR_RACE_STATISTICS;

typedef struct CAR_GEOMETRY_METRICS
{
    uint16 col_radius;
    uint16 wheel_radius;
    uint16 wheel_width;
    uint16 ground_offset;
    uint16 impact_coefficient;
} CAR_GEOMETRY_METRICS;

typedef struct CAR_PHYSICS_PROFILE
{
    sint16 torque_samples[16];
    uint32 inertia;
    uint32 mass;
    sint16 front_distance;
    sint16 rear_distance;
    sint16 grip_strength;
    sint16 gear_ratios[8];
    sint16 upshift_rpm[8];
    sint16 downshift_rpm[8];
    sint16 spring;
    sint16 damping;
    sint16 suspension_response;
    sint16 suspension_limit;
    sint16 suspension_extra;
    sint16 drive_coefficient;
    sint16 steering_response;
    sint16 motion_coefficient_108;
    sint16 front_force_weight;
    sint16 rear_force_weight;
    sint16 rpm_limit;
    sint16 motion_coefficient_116;
    sint16 drive_type;
    sint16 control_drag;
    sint16 motion_coefficient_122;
    sint16 slip_response;
} CAR_PHYSICS_PROFILE;

extern const CAR_PHYSICS_PROFILE car_traffic_profile;
extern const CAR_PHYSICS_PROFILE car_ai_profile;

enum
{
    CAR_RACE_MODEL_COUNT = 6
};

extern MODEL_RESOURCE car_model_assets[CAR_RACE_MODEL_COUNT][2];

enum
{
    CAR_TIMING_HISTORY_COUNT = 5
};

typedef struct CAR_STATE
{
    uint8 slot;
    uint8 completed_laps;
    uint16 ungrounded_updates;
    uint8 best_place;
    uint8 start_place;
    uint8 result_detail;
    uint8 place;
    uint8 chase_events;
    uint16 split_ticks[CAR_TIMING_HISTORY_COUNT];
    MODEL_RESOURCE render_models[2];
    const CAR_PHYSICS_PROFILE *physics_profile;
    TRACK_LOCATION track_contacts[8];
    TRACK_LOCATION track_location;
    uint8 gear;
    uint8 gear_count;
    uint8 automatic_transmission;
    uint8 braking;
    uint8 handbrake;
    uint8 brake_latch;
    uint32 engine_speed;
    uint32 steering_angle;
    uint32 forward_speed;
    uint32 steered_speed;
    VEC3 angular_velocity;
    VEC3 orientation;
    SVECTOR angles;
    SVECTOR suspension_local[4];
    SVECTOR suspension_rotated[4];
    SVECTOR wheel_normals[4];
    SVECTOR wheel_contact_velocity[4];
    SVECTOR ground_n;
    MATRIX transform;
    uint32 rotation_q30[9];
    uint32 rotation_step_q30[9];
    VEC3 velocity;
    VEC3 pos;
    VEC3 body_contact_pos[8];
    VEC3 wheel_contact_pos[4];
    uint32 body_spring_offset;
    uint32 body_spring_velocity;
    uint32 ground_height;
    uint32 suspension_offset[4];
    uint32 suspension_velocity[4];
    uint32 suspension_deviation[4];
    VEC3 suspension_transform[4];
    uint32 body_grip_excess;
    uint32 steered_grip_excess;
    uint32 control_drag_factor;
    uint16 wheel_slip_amount;
    uint16 motion_ticks;
    uint16 steering_ramp;
    uint16 pedal;
    uint16 wheel_rotation[2];
    uint8 wheel_air_mask;
    uint8 prev_wheel_air_mask;
    uint8 motion_mode;
    uint8 boundary_side;
    uint8 wheel_slip_flags;
    uint8 lighting_zone;
    CAR_ENVIRONMENT environment;
    CAR_RACE_STATISTICS race_stats;
    uint32 finish_time;
    uint32 points;
    uint8 surface_type;
    uint8 wheel_effect_handles[4];
    uint16 finish_speed_divisor;
    uint16 checkpoint_progress;
    uint16 checkpoint_timer;
} CAR_STATE;

extern CAR_STATE car_states[14];
extern CAR_GEOMETRY_METRICS car_geometry_metrics[14];
extern SVECTOR car_suspension_points[14][4];
extern SVECTOR car_lamp_points[14][4];
extern SVECTOR car_body_points[14][8];

uint32 car_init_all(void);

uint32 car_update_engine_speed(CAR_STATE *car, const CAR_PHYSICS_PROFILE *desc);

uint32 car_update_automatic_gears(CAR_STATE *car, const CAR_PHYSICS_PROFILE *desc);

uint32 car_apply_reverse_pedal(CAR_STATE *car);

uint32 car_compute_drive_force(CAR_STATE *car, const CAR_PHYSICS_PROFILE *desc);

uint32 car_update_player_input(uint32 slot, uint32 pad);

uint32 car_update_free_engine_speed(CAR_STATE *car);

uint32 car_load_environment(CAR_STATE *car_state, uint32 selection);

void car_update_lighting(CAR_STATE *car);

uint32 car_emit_rear_effect(CAR_STATE *car);

uint32 car_update_skid_effects(CAR_STATE *car);

uint32 car_place_on_track(uint32 slot, uint32 node, uint32 side, uint32 raised);

sint32 car_heading_from_components(sint32 x, sint32 z);

#endif
