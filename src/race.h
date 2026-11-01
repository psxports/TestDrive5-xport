#ifndef RACE_H
#define RACE_H

#include <stddef.h>

#include "psx.h"
#include "fixed_math.h"
#include "car.h"

typedef union RACE_RESULT_WORD
{
    uint32 val;
    uint8 bytes[4];
} RACE_RESULT_WORD;

typedef struct RACE_AI_ROUTE_SAMPLE
{
    uint8 lateral;
    uint8 heading;
    uint8 speed_limit;
} RACE_AI_ROUTE_SAMPLE;

typedef struct RACE_AI_ROUTE
{
    RACE_AI_ROUTE_SAMPLE *samples;
    uint32 count;
} RACE_AI_ROUTE;

typedef struct RACE_AI_CAR
{
    uint32 throttle_scale;
    uint32 car_slot;
    const RACE_AI_ROUTE *route;
    uint32 route_variant;
    uint32 route_offset;
    uint32 route_speed;
    uint32 lane_parameter;
    uint32 seg_u;
    uint32 seg_v;
    uint32 footprint_bounds[2];
    uint32 edge_parameters[2];
    uint32 corner_parameters[4];
    uint32 route_offsets[2];
    uint32 route_bounds[4];
    uint32 interval_upper;
    uint32 interval_lower;
    uint32 steering_error[2];
    uint32 program_speed;
    const uint32 *program;
    const uint32 *program_cursor;
    uint32 program_argument;
    uint32 program_flags;
    uint32 program_completion;
    uint32 program_aux_completion;
    uint32 program_timer;
    uint32 traffic_selection_offset;
    uint32 chase_target;
    uint32 chase_brake;
    uint32 recovery_state;
    uint32 reverse;
} RACE_AI_CAR;

extern RACE_AI_CAR race_ai_cars[12];

/* Resource layout matches the little-endian Windows/Web targets */
typedef struct RACE_TRAFFIC_RECORD
{
    sint16 node;
    uint8 flags;
    uint8 side;
} RACE_TRAFFIC_RECORD;

typedef struct RACE_TRAFFIC_LIST
{
    RACE_TRAFFIC_RECORD *records;
    uint32 count;
    uint32 next;
} RACE_TRAFFIC_LIST;

typedef struct RACE_AI_STATE
{
    uint32 trailing_gain;
    uint32 leading_gain;
    uint32 trailing_limit;
    uint32 leading_limit;
    uint32 gap_work;
    uint32 selection_side;
    RACE_AI_ROUTE routes[2];
    RACE_TRAFFIC_LIST traffic;
} RACE_AI_STATE;

extern RACE_AI_STATE race_ai;

typedef struct RACE_RESULT
{
    uint8 player_tag;
    uint8 model;
    uint16 place;
    RACE_RESULT_WORD points;
    uint32 total_time;
    uint16 speed_sum;
    uint16 top_speed;
    RACE_RESULT_WORD finish_data;
} RACE_RESULT;

typedef union RACE_RESULT_TABLE
{
    RACE_RESULT entries[6];
    uint32 words[30];
} RACE_RESULT_TABLE;

extern RACE_RESULT_TABLE race_results;
extern uint8 race_order[6];

typedef struct RACE_VIEW
{
    uint32 split_mode;
    uint32 primary_car;
    uint32 tracked_car;
    uint32 active_car;
    uint32 viewport;
    uint32 viewport_x;
    uint32 viewport_y;
    uint32 viewport_width;
    uint32 viewport_height;
} RACE_VIEW;

extern RACE_VIEW race_view;

typedef struct RACE_PLAYER_STATE
{
    uint32 finished;
    uint32 finish_frames;
} RACE_PLAYER_STATE;

typedef struct RACE_CONTROL
{
    uint32 car_count;
    uint32 stopped_players;
    uint32 frame_counter;
    RACE_PLAYER_STATE players[2];
    uint32 start_hold;
    uint32 timing_hold;
    uint32 update_cycles;
    uint32 attract_cycles;
    uint32 primary_timer_snapshot;
    uint32 stream_wait_marker;
    uint32 motion_gate_state;
    uint32 drag_aux_state;
    uint32 stop_requested;
    uint32 exit_reason;
    uint16 results_finished;
    uint32 paused;
    uint32 pause_selection;
    uint32 pause_confirmation;
    uint32 pause_request_player;
    uint32 pause_camera_mode;
} RACE_CONTROL;

extern RACE_CONTROL race_control;

uint32 race_run_single(void);
void race_run_two_player(void);

uint32 race_configure_car_count(void);
uint32 race_update_finish_wait(uint32 view);

uint32 race_rank_times(void);
uint32 race_rank_scores(void);
uint32 race_reset_results(void);
uint32 race_accumulate_results(void);

uint32 race_update_checkpoint_timer(CAR_STATE *car);

uint32 race_award_speed_points(CAR_STATE *car);

uint32 race_ai_heading_error(const RACE_AI_CAR *rec);

uint32 race_refresh_ai_route_offset(uint32 slot);

uint32 race_init_ai(void);

uint32 race_update_lap_timer(CAR_STATE *car);

uint32 race_apply_point_penalty(CAR_STATE *car, uint32 mode);

#endif
