#include "input.h"
#include "game.h"

typedef struct INPUT_BINDINGS
{
    uint16 fields[10][5];
} INPUT_BINDINGS;

static char *const calibration_labels[3][2][3] = {{{"CENTER NEGCON AND", "PRESS START.", presentation_pres_text.controls_empty}, {"TWIST NEGCON TO ITS", "LIMITS AND PRESS START.", presentation_pres_text.controls_empty}}, {{"CENTER MADCATZ WHEEL", "AND PRESS START.", presentation_pres_text.controls_empty}, {"TURN MADCATZ WHEEL", "TO ITS LIMITS", "AND PRESS START."}}, {{"CENTER ANALOG CONTROLLERS", "AND PRESS START.", presentation_pres_text.controls_empty}, {"MOVE ANALOG STICKS", "TO THEIR EXTREMES", "AND PRESS START."}}};

static char *const binding_labels[37] = {
    "Tri", "cro", "square", "Circle", "L1", "L2", "R1", "R2", "UP", "DOWN", "LEFT", "RIGHT", "LEFT ANALOG UP", "LEFT ANALOG DOWN", "LEFT ANALOG LEFT", "LEFT ANALOG RIGHT", "A", "B", "I", "II", "L", "R", "STEER LEFT", "STEER RIGHT", "PEDDLE LEFT", "PEDDLE RIGHT", "TWIST LEFT", "TWIST RIGHT", "RIGHT ANALOG UP", "RIGHT ANALOG DOWN", "RIGHT ANALOG LEFT", "RIGHT ANALOG RIGHT", "A", "B", "R", "GEAR STICK UP", "GEAR STICK DOWN",
};

typedef char INPUT_PACKET_SIZE_CHECK[sizeof(INPUT_PACKET) == 34 ? 1 : -1];
typedef char INPUT_BINDINGS_SIZE_CHECK[sizeof(INPUT_BINDINGS) == 100 ? 1 : -1];

const uint8 input_combined_pedal_mode = 0;

static uint8 motor_alignment[6] = {0, 1, 255, 255, 255, 255};

INPUT_OPTIONS input_options = {.profile = {0, 0}, .dead_zone = {2048, 2048}, .sensitivity = {2048, 2048}, .madcatz = {0, 0}};

INPUT_CALIBRATION input_calibration = {.center = {{128, 128}, {128, 128}}, .high = {{255, 255}, {255, 255}}, .low = {{0, 0}, {0, 0}}};

INPUT_MENU_PORT input_menu[2];

static const INPUT_BINDINGS input_bindings[4] = {{.fields = {{1, 1, 1, 1, 1}, {2, 2, 2, 2, 2}, {10, 10, 10, 10, 10}, {11, 11, 11, 11, 11}, {6, 6, 6, 6, 4}, {4, 4, 7, 7, 6}, {5, 3, 3, 3, 5}, {3, 5, 5, 0, 7}, {7, 7, 4, 4, 3}, {0, 0, 0, 5, 0}}}, {.fields = {{25, 25, 25, 25, 25}, {24, 24, 24, 24, 24}, {22, 22, 22, 22, 22}, {23, 23, 23, 23, 23}, {35, 10, 32, 35, 35}, {36, 11, 33, 36, 36}, {32, 35, 36, 11, 33}, {11, 32, 10, 32, 34}, {33, 34, 11, 34, 32}, {34, 33, 35, 33, 11}}}, {.fields = {{18, 18, 18, 18, 18}, {19, 19, 19, 19, 19}, {26, 26, 26, 26, 26}, {27, 27, 27, 27, 27}, {20, 8, 16, 8, 17}, {21, 9, 17, 9, 8}, {9, 16, 9, 17, 16}, {16, 17, 8, 10, 9}, {8, 10, 10, 11, 10}, {17, 11, 11, 16, 11}}}, {.fields = {{28, 1, 1, 28, 28}, {29, 2, 2, 29, 29}, {14, 14, 14, 14, 14}, {15, 15, 15, 15, 15}, {6, 6, 6, 6, 4}, {4, 4, 7, 7, 6}, {5, 3, 3, 1, 5}, {3, 5, 5, 0, 7}, {7, 7, 4, 3, 3}, {0, 0, 0, 2, 0}}}};

static const uint16 button_masks[37] = {4096, 16384, 32768, 8192, 1024, 256, 2048, 512, 16, 64, 128, 32, 0, 0, 0, 0, 8192, 4096, 16384, 32768, 1024, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8192, 4096, 2048, 16, 64};

INPUT_PORT input_ports[2] = {{.config = {.button_20 = 16384, .button_40 = 32768, .button_1000 = 256, .steer_left = 128, .steer_right = 32, .pressed_2000 = 2048, .pressed_4000 = 1024, .button_6000 = 512, .camera_cycle = 4096, .camera_held = 8192, .steer_center = 120, .steer_range = 80, .steer_dead_zone = 32, .axis1_min = 32, .axis1_max = 192, .axis0_min = 32, .axis0_max = 192}}, {.config = {.button_20 = 16384, .button_40 = 32768, .button_1000 = 256, .steer_left = 128, .steer_right = 32, .pressed_2000 = 2048, .pressed_4000 = 1024, .button_6000 = 512, .camera_cycle = 4096, .camera_held = 8192, .steer_center = 120, .steer_range = 80, .steer_dead_zone = 32, .axis1_min = 32, .axis1_max = 192, .axis0_min = 32, .axis0_max = 192}}};

void input_init(void)
{
    uint32 port;

    FUNCTION_MARKER(0x8006B470, "SLUS_006.10");
    PadInitDirect(input_ports[0].packet.bytes, input_ports[1].packet.bytes);
    PadStartCom();
    for (port = 0; port < 2; ++port)
    {
        input_ports[port].motors.val[0] = 0;
        input_ports[port].motors.val[1] = 0;
        input_ports[port].motors.ready = 0;
    }
}

uint32 input_poll(void)
{
    uint32 port;
    const INPUT_PACKET *packet;
    uint32 offset;
    uint16 *held_address;
    uint16 *pressed_address;
    uint32 *prev_address;
    INPUT_MOTORS *actuator;
    uint32 type;
    uint32 held;
    uint32 prev;
    uint32 axis;
    sint32 state;

    FUNCTION_MARKER(0x8006B560, "SLUS_006.10");
    for (port = 0; port < 2; ++port)
    {
        offset = port * 4;
        packet = &input_ports[port].packet;
        held_address = &input_ports[port].held;
        pressed_address = &input_ports[port].pressed;
        prev_address = &input_ports[port].prev;
        type = packet->words[0] & 0xF0FF;
        input_ports[port].type = (type);
        if (type == 0x4000 || type == 0x5000 || type == 0x7000)
        {
            actuator = &input_ports[port].motors;
            state = PadGetState((sint32)(port * 16));
            if (state == 1)
                actuator->ready = (0);
            if (actuator->ready == 0)
            {
                PadSetAct((sint32)(port * 16), actuator->val, 2);
                if (state == 2 || (state == 6 && PadSetActAlign((sint32)(port * 16), motor_alignment) != 0))
                    actuator->ready = (1);
            }
            if (type == 0x4000)
            {
                held = ~(uint32)packet->words[1];
                prev = *prev_address;
                *pressed_address = ((~prev & held));
                *held_address = (held);
                *prev_address = (held);
            }
            else
            {
                input_ports[port].steer = (packet->words[3] & 255);
                held = ~(uint32)packet->words[1];
                prev = *prev_address;
                *pressed_address = ((~prev & held));
                *held_address = (held);
                *prev_address = (held);
                if (input_ports[port].axis1_mask != 0)
                {
                    if (((uint32)(sint16)*held_address & input_ports[port].axis0_mask) != 0)
                        input_ports[port].axis0 = (256);
                    else
                        input_ports[port].axis0 = (0);
                    if (((uint32)(sint16)*held_address & input_ports[port].axis1_mask) != 0)
                        input_ports[port].axis1 = (256);
                    else
                        input_ports[port].axis1 = (0);
                }
                else
                {
                    axis = packet->words[2] >> 8;
                    if (axis >= 128)
                    {
                        input_ports[port].axis0 = (0);
                        input_ports[port].axis1 = ((axis - 127) << 1);
                    }
                    else
                    {
                        input_ports[port].axis0 = ((128 - axis) << 1);
                        input_ports[port].axis1 = (0);
                    }
                }
            }
        }
        else if (type == 0x2000)
        {
            input_ports[port].steer = (packet->words[2] & 255);
            input_ports[port].axis0 = (packet->words[2] >> 8);
            input_ports[port].axis1 = (packet->words[3] & 255);
            held = ~(uint32)packet->words[1];
            if ((sint32)input_ports[port].axis0 >= 65)
                held |= 0x4000;
            if ((packet->words[3] >> 8) >= 65)
                held |= 0x400;
            prev = *prev_address;
            *pressed_address = ((~prev & held));
            *held_address = (held);
            *prev_address = (held);
        }
        else
        {
            *held_address = (0);
            *pressed_address = (0);
        }
    }
    return 0;
}

uint32 input_set_motors(uint32 slot, uint32 motor0, uint32 motor1)
{
    INPUT_MOTORS *entry = &input_ports[slot].motors;

    FUNCTION_MARKER(0x8006B9E0, "SLUS_006.10");
    if (motor0 != 0xFFFFFFFF)
        entry->val[0] = (motor0);
    if (motor1 != 0xFFFFFFFF)
        entry->val[1] = (motor1);
    return 0xFFFFFFFF;
}

uint32 input_trigger_rumble(sint32 slot, uint32 amount)
{
    uint32 offset;

    FUNCTION_MARKER(0x800415A4, "SLUS_006.10");
    offset = (uint32)slot << 2;
    if (slot < 2)
    {
        input_ports[(uint32)slot].small_pulse = (amount);
        input_ports[(uint32)slot].large_pulse = (amount);
    }
    return offset;
}

uint32 input_update_rumble(uint32 slot, uint32 car_idx)
{
    CAR_STATE *car;
    uint32 offset;
    uint32 val;
    uint32 motor0;
    uint32 motor1;
    sint32 amplitude;
    sint32 divisor;
    sint32 quotient;

    FUNCTION_MARKER(0x800415D0, "SLUS_006.10");
    if (game_options.vibration == 0 || race_session.demo != 0)
        return input_set_motors(slot, 0, 0);
    val = car_states[car_idx].engine_speed;
    car = &car_states[car_idx];
    amplitude = 80 - (sint32)val / 128;
    if (amplitude < 0)
        amplitude = 0;
    else if (amplitude >= 256)
        amplitude = 255;
    /* Keep event pulses without the original continuous motor baseline */
    amplitude = 0;
    offset = car_idx * 4;
    divisor = (sint32)input_ports[car_idx].large_pulse;
    if (divisor != 0)
    {
        quotient = fixed_divide_checked((sint32)car->forward_speed, divisor);
        if (amplitude < quotient)
            amplitude = quotient;
    }
    if (amplitude < 0)
        motor1 = 0;
    else if (amplitude >= 256)
        motor1 = 255;
    else
        motor1 = amplitude;
    val = input_ports[car_idx].large_pulse;
    if ((sint32)val < 8193 && val != 0)
        input_ports[car_idx].large_pulse = (val + 1024);
    else
        input_ports[car_idx].large_pulse = (0);
    motor0 = 0;
    divisor = (sint32)input_ports[car_idx].small_pulse;
    if (divisor != 0)
    {
        quotient = fixed_divide_checked((sint32)car->forward_speed, divisor);
        if (quotient < 0)
            motor0 = 0;
        else if (quotient >= 256)
            motor0 = 255;
        else
            motor0 = quotient;
        motor0 |= 1;
    }
    val = input_ports[car_idx].small_pulse;
    if ((sint32)val < 8193 && val != 0)
        input_ports[car_idx].small_pulse = (val + 1024);
    else
        input_ports[car_idx].small_pulse = (0);
    return input_set_motors(slot, motor0, motor1);
}

uint32 input_pack_controls(uint32 slot)
{
    uint32 half_offset;
    uint32 word_offset;
    uint16 *output;
    uint16 *input;
    uint32 mode;
    uint32 packed;
    uint32 mask;
    uint32 edges;
    uint32 result;
    sint32 center;
    sint32 raw;
    sint32 dead_zone;
    sint32 delta;
    sint32 limit;
    sint32 numerator;
    sint32 divisor;
    sint32 lower;
    sint32 upper;

    FUNCTION_MARKER(0x80026590, "SLUS_006.10");
    half_offset = slot * 2;
    word_offset = slot * 4;
    output = &input_ports[slot].controls;
    input = &input_ports[slot].held;
    mode = input_ports[slot].type;
    if (mode == 0x2000 || mode == 0x5000 || mode == 0x7000)
    {
        center = (sint16)input_ports[slot].config.steer_center;
        raw = (sint32)input_ports[slot].steer;
        dead_zone = (sint16)input_ports[slot].config.steer_dead_zone;
        *output = (0x8000);
        delta = ((uint32)center - (uint32)raw);
        if (delta < -dead_zone)
        {
            limit = -(sint16)input_ports[slot].config.steer_range;
            delta = ((uint32)delta + (uint32)dead_zone);
            if (delta < limit)
                delta = limit;
        }
        else if (dead_zone < delta)
        {
            delta = ((uint32)delta - (uint32)dead_zone);
            limit = (sint16)input_ports[slot].config.steer_range;
            if (limit < delta)
                delta = limit;
        }
        else
            delta = 0;
        divisor = (sint16)input_ports[slot].config.steer_range;
        numerator = (((uint32)delta << 4) - (uint32)delta);
        packed = ((uint32)fixed_divide_checked(numerator, divisor) + 16) << 3;
        packed |= *output;
        *output = (packed);
        upper = (sint16)input_ports[slot].config.axis1_max;
        lower = (sint16)input_ports[slot].config.axis1_min;
        raw = (sint32)input_ports[slot].axis1;
        if ((upper - lower) / 2 < raw)
            *output = (packed | 0x100);
        else
        {
            raw = (sint32)input_ports[slot].axis0;
            lower = (sint16)input_ports[slot].config.axis0_min;
            if (raw < lower)
                raw = lower;
            upper = (sint16)input_ports[slot].config.axis0_max;
            if (upper < raw)
                raw = upper;
            delta = raw - lower;
            numerator = (((uint32)delta << 3) - (uint32)delta);
            divisor = upper - lower;
            result = (uint32)fixed_divide_checked(numerator, divisor);
            *output = (packed | (result << 9));
        }
    }
    else if (mode == 0x4000)
    {
        *output = (0);
        mask = *input;
        mask &= input_ports[slot].config.steer_left;
        if (mask != 0)
            *output = (8);
        mask = *input;
        mask &= input_ports[slot].config.steer_right;
        if (mask != 0)
            *output = (*output | 0x10);
        mask = *input;
        mask &= input_ports[slot].config.button_20;
        if (mask != 0)
            *output = (*output | 0x20);
        mask = *input;
        mask &= input_ports[slot].config.button_40;
        if (mask != 0)
            *output = (*output | 0x40);
    }
    mask = *input;
    mask &= input_ports[slot].config.button_1000;
    if (mask != 0)
        *output = (*output | 0x1000);
    edges = input_ports[slot].pressed;
    mask = input_ports[slot].config.pressed_2000;
    if ((edges & mask) != 0)
        result = *output | 0x2000;
    else
    {
        mask = input_ports[slot].config.pressed_4000;
        if ((edges & mask) != 0)
            result = *output | 0x4000;
        else
        {
            if (input_combined_pedal_mode != 0 || race_session.cop_chase != 0)
            {
                result = *input;
                result &= input_ports[slot].config.button_6000;
            }
            else
                result = edges & input_ports[slot].config.button_6000;
            if (result == 0)
                return 0;
            result = *output | 0x6000;
        }
    }
    *output = (result);
    return result;
}

void input_configure_controls(void)
{
    uint32 channel, word_offset, half_offset, pair_offset, mode;
    const uint16(*family)[5];
    uint32 val, cur, low, high, zoom, product, control, field, idx;
    sint32 before, after, distance;

    FUNCTION_MARKER(0x800A90DC, "SLUS_006.10");
    for (channel = 0; channel < 2; ++channel)
    {
        word_offset = channel * 4;
        half_offset = channel * 2;
        pair_offset = channel * 8;
        mode = input_ports[channel].type;
        family = input_bindings[INPUT_BINDING_DIGITAL].fields;
        if (mode == 0x2000 || mode == 0x5000 || mode == 0x7000)
        {
            if (mode == 0x2000)
            {
                val = input_options.madcatz[channel];
                family = input_bindings[val != 0 ? INPUT_BINDING_MADCATZ : INPUT_BINDING_NEGCON].fields;
            }
            else
                family = input_bindings[INPUT_BINDING_ANALOG].fields;
            val = input_calibration.center[channel][0];
            input_ports[channel].config.steer_center = (val);
            cur = input_calibration.center[channel][0];
            low = input_calibration.low[channel][0];
            high = input_calibration.high[channel][0];
            before = (cur - low);
            after = (high - cur);
            distance = after < before ? after : before;
            zoom = input_options.sensitivity[channel];
            product = (uint32)distance * (0x1200u - zoom);
            control = input_options.dead_zone[channel];
            input_ports[channel].config.steer_range = ((uint32)((sint32)product / 4608));
            input_ports[channel].config.steer_dead_zone = ((uint32)((sint32)control / 128));
            if (mode == 0x2000)
            {
                val = input_calibration.center[channel][0];
                input_ports[channel].config.steer_center = (val);
                input_ports[channel].config.axis0_max = (128);
                input_ports[channel].config.axis1_max = (128);
            }
        }
        for (field = 0; field < 10; ++field)
        {
            idx = input_options.profile[channel];
            val = family[field][idx];
            switch (field)
            {
                case 0:
                    mode = input_ports[channel].type;
                    val = button_masks[val];
                    if (mode == 0x7000 || mode == 0x5000)
                        input_ports[channel].axis0_mask = (val);
                    else
                    {
                        input_ports[channel].config.button_20 = (val);
                        input_ports[channel].axis0_mask = (0);
                    }
                    break;
                case 1:
                    mode = input_ports[channel].type;
                    val = button_masks[val];
                    if (mode == 0x7000 || mode == 0x5000)
                        input_ports[channel].axis1_mask = (val);
                    else
                    {
                        input_ports[channel].config.button_40 = (val);
                        input_ports[channel].axis1_mask = (0);
                    }
                    break;
                case 4:
                    val = button_masks[val];
                    input_ports[channel].config.pressed_2000 = (val);
                    break;
                case 5:
                    val = button_masks[val];
                    input_ports[channel].config.pressed_4000 = (val);
                    break;
                case 6:
                    val = button_masks[val];
                    input_ports[channel].config.button_1000 = (val);
                    break;
                case 7:
                    val = button_masks[val];
                    input_ports[channel].config.camera_held = (val);
                    break;
                case 8:
                    val = button_masks[val];
                    input_ports[channel].config.button_6000 = (val);
                    break;
                case 9:
                    val = button_masks[val];
                    input_ports[channel].config.camera_cycle = (val);
                    break;
            }
        }
    }
}

static void menu_draw_calibration(uint32 slot)
{
    uint32 base = slot * 29;
    char *const *src;
    uint32 idx, state, prev, type, flag;

    FUNCTION_MARKER(0x800A8BA8, "SLUS_006.10");
    presentation_items[base + 5].active = (0);
    for (idx = 0; idx < 4; ++idx)
        presentation_items[base + idx].active = (0);
    state = input_menu[slot].phase;
    presentation_items[base + 4].active = (0);
    prev = state - 1;
    for (idx = 0; idx < 10; ++idx)
    {
        presentation_items[base + idx + 6].active = (0);
        presentation_items[base + idx + 16].active = (0);
    }
    type = input_ports[slot].type;
    presentation_items[base + 26].active = (1);
    presentation_items[base + 27].active = (1);
    presentation_items[base + 28].active = (1);
    if (type == 0x4000)
        input_menu[slot].phase = (0);
    else if (type == 0x2000)
    {
        flag = input_options.madcatz[slot];
        src = calibration_labels[flag == 0 ? 0 : 1][prev];
        for (idx = 0; idx < 3; ++idx)
        {
            presentation_items[base + 26 + idx].content.text = src[idx];
        }
    }
    else if (type == 0x5000 || type == 0x7000)
    {
        src = calibration_labels[2][prev];
        for (idx = 0; idx < 3; ++idx)
        {
            presentation_items[base + 26 + idx].content.text = src[idx];
        }
    }
}

static void menu_edit_calibration(uint32 slot)
{
    uint32 *state_address = &input_menu[slot].phase;
    uint32 *center_address = &input_menu[slot].center;
    uint32 *high_address = &input_menu[slot].high;
    uint32 *low_address = &input_menu[slot].low;
    uint32 val, center, high, low;

    FUNCTION_MARKER(0x800A8EDC, "SLUS_006.10");
    if (*state_address == 1)
    {
        if ((input_ports[slot].pressed & 8) != 0)
        {
            audio_play_menu_move();
            val = input_ports[slot].steer;
            *center_address = (val);
            val = *state_address;
            *state_address = (val + 1);
            center = *center_address;
            if ((sint32)center < 48)
                center = 48;
            else if ((sint32)center >= 209)
                center = 208;
            *center_address = (center);
            high = *center_address;
            low = *center_address;
            *high_address = (high + 32);
            *low_address = (low - 32);
        }
    }
    else
    {
        val = input_ports[slot].steer;
        high = *high_address;
        if ((sint32)high < (sint32)val)
        {
            *high_address = (val);
            val = input_ports[slot].steer;
        }
        low = *low_address;
        if ((sint32)val < (sint32)low)
            *low_address = (val);
        if ((input_ports[slot].pressed & 8) != 0)
        {
            audio_play_menu_move();
            center = *center_address;
            high = *high_address;
            low = *low_address;
            input_calibration.center[slot][0] = (center);
            input_calibration.high[slot][0] = (high);
            input_calibration.low[slot][0] = (low);
            *state_address = (0);
        }
    }
}

static void menu_draw_cfg(uint32 slot, uint32 cfg)
{
    const uint8 *found_tex;
    uint32 base = slot * 29;
    const uint16(*mapping)[5] = input_bindings[INPUT_BINDING_DIGITAL].fields;
    PRESENTATION_ITEM *entry;
    char *dst;
    char *pointer;
    uint32 val, type, idx, flag;
    sint32 parameter;

    FUNCTION_MARKER(0x800A7EF8, "SLUS_006.10");
    if (input_menu[slot].phase != 0)
    {
        menu_draw_calibration(slot);
        return;
    }
    dst = presentation_items[base].content.text;
    presentation_format_number(dst, "CONFIGURATION %d", cfg + 1);
    parameter = (sint32)input_options.dead_zone[slot];
    dst = presentation_items[base + 1].content.text;
    presentation_format_number(dst, "DEADZONE %d", (uint32)(parameter / 512) + 1);
    parameter = (sint32)input_options.sensitivity[slot];
    dst = presentation_items[base + 2].content.text;
    presentation_format_number(dst, "SENSITIVITY %d", (uint32)(parameter / 512) + 1);
    val = input_options.madcatz[slot];
    dst = presentation_items[base + 4].content.text;
    presentation_copy_text(dst, val != 0 ? "MADCATZ" : "NEGCON");
    type = input_ports[slot].type;
    if (type == 0x4000)
        mapping = input_bindings[INPUT_BINDING_DIGITAL].fields;
    else if (type == 0x2000)
    {
        val = input_options.madcatz[slot];
        mapping = input_bindings[val != 0 ? INPUT_BINDING_MADCATZ : INPUT_BINDING_NEGCON].fields;
    }
    else if (type == 0x5000 || type == 0x7000)
        mapping = input_bindings[INPUT_BINDING_ANALOG].fields;
    presentation_items[base + 26].active = (0);
    presentation_items[base + 27].active = (0);
    type = input_ports[slot].type;
    presentation_items[base + 28].active = (0);
    if (type != 255)
    {
        presentation_items[base + 5].active = (0);
        for (idx = 0; idx < 5; ++idx)
            presentation_items[base + idx].active = (1);
        for (idx = 0; idx < 10; ++idx)
        {
            presentation_items[base + idx + 6].active = (1);
            entry = presentation_items + base + idx + 16;
            entry->active = (1);
            val = mapping[0][cfg];
            pointer = ((uint32)(val) < 37 ? binding_labels[val] : "");
            entry->content.text = pointer;
            val = mapping[0][cfg];
            if (val < 4)
            {
                entry->kind = (0x8000);
                val = mapping[0][cfg];
                pointer = ((uint32)(val) < 37 ? binding_labels[val] : "");
                ++mapping;
                found_tex = tex_find(pointer);
                entry->content.tex = found_tex;
            }
            else
            {
                entry->kind = (1);
                ++mapping;
            }
        }
    }
    else
    {
        presentation_items[base + 5].active = (1);
        for (idx = 0; idx < 5; ++idx)
            presentation_items[base + idx].active = (0);
        for (idx = 0; idx < 10; ++idx)
        {
            presentation_items[base + idx + 6].active = (0);
            presentation_items[base + idx + 16].active = (0);
        }
    }
    type = input_ports[slot].type;
    if (type == 0x4000)
    {
        for (idx = 1; idx < 5; ++idx)
            presentation_items[base + idx].active = (0);
    }
    else if (type == 0x5000 || type == 0x7000)
    {
        flag = input_menu[slot].analog_mode_available;
        presentation_items[base + 3].active = (0);
        if (flag == 0)
            presentation_items[base + 3].active = (1);
        presentation_items[base + 4].active = (0);
    }
}

static void menu_edit_cfg(uint32 slot)
{
    uint32 base = slot * 29;
    uint32 idx, type, limit, val, selection;
    uint32 *field;
    uint16 *pad = &input_ports[slot].pressed;
    uint32 *selection_address = &input_menu[slot].selection;

    FUNCTION_MARKER(0x800A8588, "SLUS_006.10");
    if (input_menu[slot].phase != 0)
    {
        menu_edit_calibration(slot);
        return;
    }
    for (idx = 0; idx < 5; ++idx)
        presentation_items[base + idx].pal = (0);
    limit = 0;
    type = input_ports[slot].type;
    if (type == 0x2000)
        limit = 4;
    else if (type == 0x5000 || type == 0x7000)
        limit = input_menu[slot].analog_mode_available != 0 ? 2 : 3;
    if ((*pad & 0x10) != 0 && (sint32)*selection_address > 0)
    {
        audio_play_menu_move();
        *selection_address = (*selection_address - 1);
    }
    if ((*pad & 0x40) != 0 && (sint32)*selection_address < (sint32)limit)
    {
        audio_play_menu_move();
        *selection_address = (*selection_address + 1);
    }
    selection = *selection_address;
    presentation_items[base + selection].pal = (1);
    selection = *selection_address;
    switch (selection)
    {
        case 0:
            field = &input_options.profile[slot];
            if ((*pad & 0x80) != 0 && (sint32)*field > 0)
            {
                audio_play_menu_move();
                *field = (*field - 1);
            }
            if ((*pad & 0x20) != 0 && (sint32)*field < 4)
            {
                audio_play_menu_move();
                *field = (*field + 1);
            }
            break;
        case 1:
        case 2:
            field = selection == 1 ? &input_options.dead_zone[slot] : &input_options.sensitivity[slot];
            if ((*pad & 0x80) != 0 && (sint32)*field >= 512)
            {
                audio_play_menu_move();
                *field = (*field - 512);
            }
            if ((*pad & 0x20) != 0 && (sint32)*field < 4096)
            {
                audio_play_menu_move();
                *field = (*field + 512);
            }
            break;
        case 3:
            if ((*pad & 0x4008) != 0)
                input_menu[slot].phase = (1);
            break;
        case 4:
            if ((*pad & 0x4008) != 0)
            {
                field = &input_options.madcatz[slot];
                val = *field;
                *field = (val ^ 1);
            }
            break;
        default:
            break;
    }
}

void input_menu_update(uint32 slot)
{
    uint32 type, prev, ticks, available, cfg;
    sint32 state;

    FUNCTION_MARKER(0x800A8A48, "SLUS_006.10");
    type = input_ports[slot].type;
    prev = input_menu[slot].prev_type;
    presentation_menu.sel = (0xFFFFFFFF);
    if (type != prev)
    {
        input_menu[slot].analog_mode_available = (0);
        type = input_ports[slot].type;
        if (type == 0x5000 || type == 0x7000)
        {
            ticks = 0;
            for (;;)
            {
                state = PadGetState((sint32)(slot << 4));
                if (state == 6 || ticks >= 31)
                    break;
                VSync(0);
                ++ticks;
            }
            available = 0;
            if (ticks < 30)
                available = (uint32)PadInfoMode((sint32)(slot << 4), 2, 0) != 0;
            input_menu[slot].analog_mode_available = (available);
        }
    }
    type = input_ports[slot].type;
    input_menu[slot].prev_type = (type);
    cfg = input_options.profile[slot];
    menu_draw_cfg(slot, cfg);
    (void)input_options.profile[slot];
    menu_edit_cfg(slot);
    if ((input_ports[slot].pressed & 0x1000) != 0 && slot == 0)
        session_pop_menu();
}

void input_reset_menu_devices(void)
{
    FUNCTION_MARKER(0x800A8A1C, "SLUS_006.10");
    input_menu[0].prev_type = (0xFFFFFFFF);
    input_menu[1].prev_type = (0xFFFFFFFF);
    input_menu[0].analog_mode_available = (0);
    input_menu[1].analog_mode_available = (0);
}
