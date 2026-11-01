#ifndef CAMERA_H
#define CAMERA_H

#include "car.h"
#include "psx.h"

typedef struct CAMERA_CUE
{
    uint16 kind;
    uint16 node_start;
    uint16 node_end;
    uint16 node;
    uint16 vertex;
    uint16 height;
    uint16 spline_variant;
    uint16 spline_step;
} CAMERA_CUE;

extern CAMERA_CUE *camera_cues;
extern uint32 camera_cue_count;

typedef union CAMERA_ATTACH_OFFSET
{
    SVECTOR sdk;
    uint32 words[2];
} CAMERA_ATTACH_OFFSET;

typedef struct CAMERA_VIEW
{
    VECTOR eye;
    VECTOR displacement;
    CAMERA_ATTACH_OFFSET attach_offset;
    uint32 spline_coefficients[15];
    uint32 prev_mode;
    uint32 follow_distance;
    uint32 lateral_offset;
    uint32 offset_aux;
    uint32 target_height;
    uint32 target_distance;
    uint32 target_aim_height;
    uint32 aim_height;
    uint32 reverse_heading;
    uint32 follow_height;
    uint32 look_back_angle;
    uint32 attached;
    uint32 damping;
    uint32 yaw;
    uint32 sel_mode;
    uint32 roll;
    uint32 cue_idx;
    uint32 trackside_x;
    uint32 trackside_height;
    uint32 trackside_z;
    uint32 cue_mode;
    uint32 spline_phase;
    uint32 spline_step;
    uint32 spline_variant;
    uint32 target_distance_measured;
    uint32 target_scale;
    uint32 proj_scale;
    uint32 zoom_gain;
    uint32 track_node;
    uint32 cue_vertex;
    uint32 trackside_vertex;
} CAMERA_VIEW;

extern CAMERA_VIEW camera_views[2];

uint32 camera_follow(CAR_STATE *car, uint32 update, uint32 view);

uint32 camera_init_cues(void);
uint32 camera_reset_follow(const SVECTOR *angles, sint32 mode, uint32 view);
uint32 camera_update(CAR_STATE *car, uint32 view);

#endif
