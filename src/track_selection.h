#ifndef TRACK_SELECTION_H
#define TRACK_SELECTION_H

#include "psx.h"
#include "fixed_math.h"
#include "model_resource.h"

typedef struct TRACK_PREVIEW_BUFFER
{
    uint8 *bytes;
    uint32 capacity;
} TRACK_PREVIEW_BUFFER;

typedef struct TRACK_PREVIEW_STATE
{
    uint32 angle_x;
    uint32 angle_y;
    uint16 target_x;
    uint16 target_y;
    uint32 step_x;
    uint32 step_y;
} TRACK_PREVIEW_STATE;

typedef struct TRACK_SELECTION_STATE
{
    uint32 entries[46];
    uint32 count;
    uint32 left_x;
    uint32 right_x;
    uint32 quick_model_idx;
    TRACK_PREVIEW_BUFFER preview_buffers[4];
    VEC3_16 banner_angles;
    TRACK_PREVIEW_STATE preview;
    TRACK_PREVIEW_STATE previews[4];
} TRACK_SELECTION_STATE;

extern TRACK_SELECTION_STATE track_selection;

void track_selection_build_list(void);
void track_selection_prepare_previews(void);
void track_selection_update(void);
void track_selection_update_multiplayer(void);
void track_selection_update_quick(void);

#endif
