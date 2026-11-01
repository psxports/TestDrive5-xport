#ifndef CAR_SELECTION_H
#define CAR_SELECTION_H

#include "psx.h"
#include "car.h"

typedef struct CAR_SELECTION_TUNING_PACKETS
{
    POLY_G4 gradients[8][2];
    LINE_F2 ticks[68][2];
    LINE_F2 markers[20][2];
} CAR_SELECTION_TUNING_PACKETS;

typedef struct CAR_SELECTION_BUFFER
{
    uint8 *bytes;
    uint32 capacity;
} CAR_SELECTION_BUFFER;

typedef struct CAR_SELECTION_MODEL
{
    uint8 state;
    uint16 age;
    uint8 bank;
    uint8 *buffer;
} CAR_SELECTION_MODEL;

typedef struct CAR_SELECTION_BANK
{
    uint8 model;
    uint8 *buffer;
} CAR_SELECTION_BANK;

typedef struct CAR_SELECTION_STATE
{
    uint32 stage;
    uint32 input_port;
    uint32 phase;
    uint32 mode;
    uint32 manual_transmission;
    uint32 intro_progress;
    uint32 transition_depth;
    uint32 vertical_offset;
    uint32 list_idx;
    uint32 list_count;
    uint16 entries[44];
    uint32 retained_transition_code;
    uint32 refresh_frames;
    uint32 banner_angle;
    uint32 banner_wiggle;
    uint32 sel_entry;
    uint32 primary_color;
    uint32 secondary_color;
    uint32 tuning_row;
    uint16 model_angle;
    uint32 label_phase;
    uint32 wheel_angles[2];
    CAR_SELECTION_MODEL models[44];
    CAR_SELECTION_BANK banks[1];
    uint32 bank_count;
    uint32 loading_model;
    uint32 active_model_offset;
    CAR_SELECTION_BUFFER active_buffer;
    uint16 tuning[4];
    uint16 disp_tuning[4];
    CAR_SELECTION_TUNING_PACKETS tuning_packets;
} CAR_SELECTION_STATE;

extern CAR_SELECTION_STATE car_selection;
extern const uint16 car_selection_standard_models[14];
extern const uint16 car_selection_beast_models[14];

uint32 car_selection_build_list(void);

uint32 car_selection_begin(uint32 mode);
void car_selection_update(uint32 mode);

void car_selection_cancel_load(void);

#endif
