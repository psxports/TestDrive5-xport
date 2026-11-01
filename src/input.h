#ifndef INPUT_H
#define INPUT_H

#include "psx.h"

extern const uint8 input_combined_pedal_mode;

typedef union INPUT_PACKET
{
    uint8 bytes[34];
    uint16 words[17];
} INPUT_PACKET;

typedef struct INPUT_MOTORS
{
    uint8 val[2];
    uint8 ready;
} INPUT_MOTORS;

typedef struct INPUT_CONFIG
{
    uint16 button_20;
    uint16 button_40;
    uint16 button_1000;
    uint16 steer_left;
    uint16 steer_right;
    uint16 pressed_2000;
    uint16 pressed_4000;
    uint16 button_6000;
    uint16 camera_cycle;
    uint16 camera_held;
    uint16 steer_center;
    uint16 steer_range;
    uint16 steer_dead_zone;
    uint16 axis1_min;
    uint16 axis1_max;
    uint16 axis0_min;
    uint16 axis0_max;
} INPUT_CONFIG;

typedef struct INPUT_PORT
{
    INPUT_PACKET packet;
    INPUT_MOTORS motors;
    uint16 held;
    uint16 pressed;
    uint16 controls;
    uint32 prev;
    uint32 type;
    uint32 steer;
    uint32 axis0;
    uint32 axis1;
    uint32 axis0_mask;
    uint32 axis1_mask;
    uint32 large_pulse;
    uint32 small_pulse;
    INPUT_CONFIG config;
} INPUT_PORT;

typedef struct INPUT_OPTIONS
{
    uint32 profile[2];
    uint32 dead_zone[2];
    uint32 sensitivity[2];
    uint32 madcatz[2];
} INPUT_OPTIONS;

typedef struct INPUT_CALIBRATION
{
    uint32 center[2][2];
    uint32 high[2][2];
    uint32 low[2][2];
} INPUT_CALIBRATION;

typedef struct INPUT_MENU_PORT
{
    uint32 selection;
    uint32 phase;
    uint32 center;
    uint32 high;
    uint32 low;
    uint32 prev_type;
    uint32 analog_mode_available;
} INPUT_MENU_PORT;

enum
{
    INPUT_BINDING_DIGITAL,
    INPUT_BINDING_MADCATZ,
    INPUT_BINDING_NEGCON,
    INPUT_BINDING_ANALOG
};

extern INPUT_OPTIONS input_options;
extern INPUT_CALIBRATION input_calibration;
extern INPUT_MENU_PORT input_menu[2];
extern INPUT_PORT input_ports[2];

void input_init(void);
uint32 input_poll(void);

uint32 input_set_motors(uint32 slot, uint32 small, uint32 large);
uint32 input_trigger_rumble(sint32 slot, uint32 amount);
uint32 input_update_rumble(uint32 slot, uint32 car_idx);
uint32 input_pack_controls(uint32 slot);
void input_configure_controls(void);

void input_menu_update(uint32 slot);

void input_reset_menu_devices(void);

#endif
