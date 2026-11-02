#include <stdlib.h>
#include <string.h>

#include "car.h"
#include "psx_gpu.h"
#include "game.h"

typedef struct CAR_LIGHTING_WORK
{
    VEC3 points[3];
    uint32 g;

    union
    {
        SVECTOR sdk;
        sint16 component[4];
    } dir;
} CAR_LIGHTING_WORK;

CAR_LIGHTING_ZONES car_lighting_zones;

static void blend_lighting_start(CAR_STATE *car, const uint8 *desc, sint32 width);
static void blend_lighting_end(CAR_STATE *car, const uint8 *desc, sint32 width);

const char *const car_asset_names[45] = {
    "CARS\\VIP.RNC", "CARS\\VET.RNC", "CARS\\VAN.RNC", "CARS\\TVR.RNC", "CARS\\XKR.RNC", "CARS\\FRD.RNC", "CARS\\97C.RNC", "CARS\\SKY.RNC", "CARS\\NIS.RNC", "CARS\\COB.RNC", "CARS\\GTO.RNC", "CARS\\MUS.RNC", "CARS\\CAM.RNC", "CARS\\69V.RNC", "CARS\\CHV.RNC", "CARS\\CUD.RNC", "CARS\\CRG.RNC", "CARS\\ATP.RNC", "CARS\\SS1.RNC", "CARS\\128.RNC", "CARS\\GTR.RNC", "CARS\\JAG.RNC", "CARS\\CAT.RNC", "CARS\\PIT.RNC", "CARS\\C21.RNC", "CARS\\COP.RNC", "CARS\\DAY.RNC", "CARS\\HOT.RNC", "CARS\\FHM.RNC", "CARS\\SP1.RNC", "CARS\\SP2.RNC", "CARS\\SP3.RNC", "CARS\\SP4.RNC", "CARS\\SP5.RNC", "CARS\\SP6.RNC", "CARS\\SP7.RNC", "CARS\\SP8.RNC", "CARS\\SP9.RNC", "1bon", "1boot", "1bump", "1roof", "1side", "1tail", "1win",
};

static char model_tex_name[6];
static uint32 race_model_workspace[0x32000 / 4];

enum
{
    CAR_MODEL_STORAGE_BYTES = CAR_RACE_MODEL_COUNT * (0x32000 + 8)
};

static uint32 race_model_storage[CAR_MODEL_STORAGE_BYTES / 4];

MODEL_RESOURCE car_model_assets[CAR_RACE_MODEL_COUNT][2];

CAR_STATE car_states[14];
CAR_GEOMETRY_METRICS car_geometry_metrics[14];
SVECTOR car_suspension_points[14][4];
SVECTOR car_lamp_points[14][4];
SVECTOR car_body_points[14][8];

static CAR_PHYSICS_PROFILE profiles[6];

const CAR_PHYSICS_PROFILE car_traffic_profile = {
    .torque_samples = {128, 192, 232, 248, 252, 254, 255, 256, 256, 256, 256, 256, 256, 0, -256, -256},
    .inertia = 120000,
    .mass = 12000,
    .front_distance = 400,
    .rear_distance = 400,
    .grip_strength = 3200,
    .gear_ratios = {2092, 0, 2092, 1398, 1021, 786, 581, 394},
    .upshift_rpm = {9999, 9999, 5400, 5400, 5400, 5400, 5400, 9999},
    .downshift_rpm = {9999, 9999, 1000, 3000, 3300, 3600, 3500, 3300},
    .spring = 50,
    .damping = 40,
    .suspension_response = 30,
    .suspension_limit = 12288,
    .suspension_extra = 32,
    .drive_coefficient = 86,
    .steering_response = 300,
    .motion_coefficient_108 = 3000,
    .front_force_weight = 700,
    .rear_force_weight = 700,
    .rpm_limit = 6200,
    .motion_coefficient_116 = 1000,
    .drive_type = 1,
    .control_drag = 96,
    .motion_coefficient_122 = 192,
    .slip_response = 48,
};

const CAR_PHYSICS_PROFILE car_ai_profile = {
    .torque_samples = {160, 192, 216, 224, 228, 232, 236, 240, 244, 248, 252, 256, 256, 256, 256, 256},
    .inertia = 180000,
    .mass = 24000,
    .front_distance = 400,
    .rear_distance = 400,
    .grip_strength = 3500,
    .gear_ratios = {2800, 0, 2500, 1850, 1370, 1010, 750, 550},
    .upshift_rpm = {9999, 0, 6500, 6500, 6500, 6500, 6500, 9999},
    .downshift_rpm = {9999, 0, 0, 4300, 4300, 4300, 4300, 4300},
    .spring = 50,
    .damping = 40,
    .suspension_response = 30,
    .suspension_limit = 8192,
    .suspension_extra = 24,
    .drive_coefficient = 100,
    .steering_response = 300,
    .motion_coefficient_108 = 3000,
    .front_force_weight = 600,
    .rear_force_weight = 600,
    .rpm_limit = 7400,
    .motion_coefficient_116 = 1066,
    .drive_type = 3,
    .control_drag = 80,
    .motion_coefficient_122 = 160,
    .slip_response = 160,
};

static const uint32 gear_impulse_factors[8] = {0, 0, 256, 192, 128, 64, 32, 16};

typedef struct CAR_TUNING_MODIFIERS
{
    uint32 grip_adjust;
    uint32 ratio_adjust;
    uint32 response_adjust;
    uint32 balance_adjust;
} CAR_TUNING_MODIFIERS;

static uint32 init_race_timing(CAR_STATE *car)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5;

    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    uint32 slot;
    uint32 address;
    uint8 *table;
    uint8 *cursor;
    sint32 mode;
    uint32 multiplier;
    uint32 val;
    sint32 count;
    sint32 idx;

    FUNCTION_MARKER(0x80048318, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    slot = car->slot;
    address = level_desc->checkpoints_offset;
    XASSERT(!(address > scene_level_image.size || scene_level_image.size - (address) < 4));
    table = (uint8 *)(scene_level_image.bytes + (address));
    XASSERT(!(table[0] == 0 || table[0] > CAR_TIMING_HISTORY_COUNT || table[0] > (scene_level_image.size - address - 4) / 4));
    if (slot == 0)
    {
        mode = (sint32)race_rules.difficulty;
        if (mode == 0 || mode == 1)
        {
            multiplier = mode == 0 ? 12 : 11;
            val = (memcpy(&wire_halfword_1, table + 2, sizeof(wire_halfword_1)), wire_halfword_1);
            count = (sint32)table[0] - 1;
            (wire_halfword_2 = (uint16)((uint16)(val * multiplier / 10)), memcpy(table + 2, &wire_halfword_2, sizeof(wire_halfword_2)));
            if (count > 0)
            {
                idx = 0;
                cursor = table;
                do
                {
                    XASSERT(!(scene_level_image.size < 8 || (size_t)(cursor - scene_level_image.bytes) > scene_level_image.size - 8));
                    val = (memcpy(&wire_halfword_3, cursor + 6, sizeof(wire_halfword_3)), wire_halfword_3);
                    (wire_halfword_4 = (uint16)((uint16)(val * multiplier / 10)), memcpy(cursor + 6, &wire_halfword_4, sizeof(wire_halfword_4)));
                    count = (sint32)table[0] - 1;
                    ++idx;
                    cursor += 4;
                } while (idx < count);
            }
        }
    }
    val = (memcpy(&wire_halfword_5, table + 2, sizeof(wire_halfword_5)), wire_halfword_5);
    car->completed_laps = (uint8)(0);
    car->finish_time = (uint32)(0);
    car->race_stats.elapsed_ticks = (uint16)(0);
    car->checkpoint_timer = (uint16)(val);
    race_control.players[0].finished = (uint32)(0);
    race_control.players[1].finished = (uint32)(0);
    return val;
}

uint32 car_init_all(void)
{
    CAR_TUNING_MODIFIERS tuning[2];
    uint32 variant;
    uint32 slot;
    CAR_STATE *car;
    CAR_PHYSICS_PROFILE *metadata;
    uint32 a;
    uint32 b;
    uint32 idx;
    uint32 gears;
    uint32 gear_idx;
    uint32 product;
    uint32 factor;
    uint32 *modifier_address;
    sint32 val;
    sint32 metadata_val;
    sint32 drag;
    sint32 modifier;
    sint32 scaled;
    sint32 quotient;
    sint32 multiplayer;

    FUNCTION_MARKER(0x80025960, "SLUS_006.10");
    variant = session_options.model_variant << 3;
    for (slot = 0; slot < 2; ++slot)
    {
        val = (sint16)session_car_choices.cars[slot].tuning[variant >> 3][2];
        tuning[slot].grip_adjust = (uint32)(val / 16);
        val = (sint16)session_car_choices.cars[slot].tuning[variant >> 3][0];
        tuning[slot].ratio_adjust = (uint32)(val / 16);
        val = (sint16)session_car_choices.cars[slot].tuning[variant >> 3][1];
        tuning[slot].response_adjust = (uint32)(val / 16);
        val = (sint16)session_car_choices.cars[slot].tuning[variant >> 3][3];
        tuning[slot].balance_adjust = (uint32)(val / 16);
    }
    for (slot = 0; slot < 14; ++slot)
    {
        car = &car_states[slot];
        memset(&car_states[slot], 0, sizeof(car_states[slot]));
        car_random_states[slot] = 0;
        car_states[slot].slot = (uint8)slot;
        init_race_timing(car);
        if (slot < 2)
        {
            a = session_car_choices.cars[slot].manual_transmission;
            car_states[slot].automatic_transmission = (uint8)(a ^ 1);
        }
        if (slot == 1 && race_session.two_players == 0)
            car_states[slot].automatic_transmission = (uint8)(1);
        if (slot >= 2)
            car_states[slot].automatic_transmission = (uint8)(1);
        if (session_flow.attract_active != 0)
            car_states[slot].automatic_transmission = (uint8)(1);
        car->orientation.bits.y = (uint32)(944640);
        car_states[slot].gear = (uint8)(2);
        metadata = slot < 6 ? &profiles[slot] : NULL;
        car_states[slot].physics_profile = metadata;
        if (slot < 2)
            val = metadata->control_drag;
        car->best_place = (uint8)255;
        if (slot < 2)
            car_states[slot].control_drag_factor = (uint32)val;
        for (idx = 0; idx < 4; ++idx)
        {
            car_states[slot].wheel_effect_handles[idx] = (uint8)(255);

            car_states[slot].suspension_local[idx] = car_suspension_points[slot][idx];
        }
        if (slot < 6)
        {
            race_order[slot] = (uint8)(slot);
            car_update_grounding(car);
            metadata = (CAR_PHYSICS_PROFILE *)car->physics_profile;
            idx = 2;
            for (;;)
            {
                val = metadata->upshift_rpm[idx];
                if (val >= 9999)
                    break;
                ++idx;
                if (idx >= 8)
                    break;
            }
            car_states[slot].gear_count = (uint8)(idx + 1);
        }
        else
        {
            car_geometry_metrics[slot].impact_coefficient = (uint16)(32);
            car_update_surface_alignment(car);
        }
    }
    if (car_physics.enhanced_mode != 0)
    {
        car_physics.gravity = 2048;
        for (slot = 0; slot < 6; ++slot)
        {
            metadata = (CAR_PHYSICS_PROFILE *)car_states[slot].physics_profile;
            val = metadata->drive_coefficient * 650;
            metadata_val = metadata->grip_strength;
            drag = metadata->control_drag;
            metadata->drive_coefficient = (sint16)(uint16)((uint32)(val / 256));
            val = metadata_val * 380;
            metadata->control_drag = (sint16)(uint16)((uint32)drag << 2);
            metadata_val = metadata->front_force_weight;
            metadata->grip_strength = (sint16)(uint16)((uint32)(val / 256));
            val = metadata_val * 450;
            metadata_val = metadata->rear_force_weight;
            metadata->front_force_weight = (sint16)(uint16)((uint32)(val / 256));
            val = metadata_val * 400;
            metadata->rear_force_weight = (sint16)(uint16)((uint32)(val / 256));
        }
    }
    else if (race_rules.simulation != 0)
        car_physics.gravity = 1500;
    else
    {
        car_physics.gravity = 1900;
        for (slot = 0; slot < 6; ++slot)
        {
            metadata = (CAR_PHYSICS_PROFILE *)car_states[slot].physics_profile;
            val = metadata->drive_coefficient * 360;
            metadata_val = metadata->grip_strength;
            drag = metadata->control_drag;
            metadata->drive_coefficient = (sint16)(uint16)((uint32)(val / 256));
            val = metadata_val * 300;
            metadata->control_drag = (sint16)(uint16)((uint32)drag << 1);
            metadata->grip_strength = (sint16)(uint16)((uint32)(val / 256));
        }
    }
    multiplayer = (sint32)race_session.two_players;
    if (multiplayer < 0)
        return multiplayer;
    slot = 0;
    for (;;)
    {
        car = &car_states[slot];
        metadata = (CAR_PHYSICS_PROFILE *)car->physics_profile;
        modifier = (sint32)tuning[slot].balance_adjust;
        val = metadata->front_force_weight;
        product = (uint32)val * (uint32)modifier;
        scaled = (sint32)product / 256;
        a = (uint16)metadata->rear_force_weight;
        metadata->front_force_weight = (sint16)(uint16)((uint32)val + (uint32)scaled);
        metadata->rear_force_weight = (sint16)(uint16)(a - (uint32)scaled);
        gears = car_states[slot].gear_count;
        gear_idx = 2;
        for (idx = 2; idx < gears; ++idx)
        {
            modifier = (sint32)tuning[slot].ratio_adjust;
            val = *(gear_idx == 8 ? &metadata->upshift_rpm[0] : &metadata->gear_ratios[gear_idx]);
            product = (uint32)val * (uint32)modifier;
            scaled = (sint32)product / (modifier > 0 ? 512 : 768);
            factor = gears - (idx + 1);
            product = (uint32)scaled * factor;
            quotient = fixed_divide_checked((sint32)product, (sint32)(gears - 3));
            *(gear_idx == 8 ? &metadata->upshift_rpm[0] : &metadata->gear_ratios[gear_idx]) = (sint16)(uint16)((uint32)val + (uint32)quotient);
            ++gear_idx;
        }
        modifier_address = &tuning[slot].response_adjust;
        modifier = *modifier_address;
        val = metadata->motion_coefficient_116;
        product = (uint32)val * ((uint32)modifier + 2048);
        scaled = (sint32)product / 2048;
        gear_idx = gears;
        metadata->motion_coefficient_116 = (sint16)(uint16)((uint32)scaled);
        val = *(gear_idx == 9 ? &metadata->upshift_rpm[0] : &metadata->gear_ratios[gear_idx - 1]);
        modifier = *modifier_address;
        product = (uint32)val << 11;
        quotient = fixed_divide_checked((sint32)product, (sint32)((uint32)modifier + 2048));
        *(gear_idx == 9 ? &metadata->upshift_rpm[0] : &metadata->gear_ratios[gear_idx - 1]) = (sint16)(uint16)((uint32)quotient);
        val = metadata->gear_ratios[gear_idx - 2];
        modifier = *modifier_address;
        product = (uint32)val << 12;
        quotient = fixed_divide_checked((sint32)product, (sint32)((uint32)modifier + 4096));
        metadata->gear_ratios[gear_idx - 2] = (sint16)(uint16)((uint32)quotient);
        modifier = *modifier_address;
        val = metadata->steering_response;
        product = (uint32)val * (512 - (uint32)modifier);
        scaled = (sint32)product / 512;
        metadata->steering_response = (sint16)(uint16)((uint32)scaled);
        modifier_address = &tuning[slot].grip_adjust;
        modifier = *modifier_address;
        val = metadata->grip_strength;
        product = (uint32)val * (uint32)modifier;
        if (modifier < 0)
        {
            scaled = (sint32)product / 768;
            metadata->grip_strength = (sint16)(uint16)((uint32)val + (uint32)scaled);
        }
        else
        {
            scaled = (sint32)product / 512;
            metadata->grip_strength = (sint16)(uint16)((uint32)val + (uint32)scaled);
            a = car_states[slot].control_drag_factor;
            b = *modifier_address;
            product = a * b;
            scaled = (sint32)product / 512;
            car_states[slot].control_drag_factor = a + (uint32)scaled;
        }
        multiplayer = (sint32)race_session.two_players;
        ++slot;
        if (multiplayer < (sint32)slot)
            return 1;
    }
}

uint32 car_update_engine_speed(CAR_STATE *car, const CAR_PHYSICS_PROFILE *desc)
{
    uint32 gear, active, product, result;
    const CAR_PHYSICS_PROFILE *model;
    sint32 limit, pedal, target, cur, step, speed, coefficient, difference;

    FUNCTION_MARKER(0x80025340, "SLUS_006.10");
    gear = car->gear;
    if (gear == 1)
    {
        model = car->physics_profile;
        active = car->braking;
        limit = model->rpm_limit;
        target = 400;
        if (active == 0)
        {
            pedal = (sint16)car->pedal;
            if (pedal >= 0)
            {
                product = ((uint32)limit - 400) * (uint32)pedal;
                step = (sint32)product / 256;
                target = ((uint32)(step < limit ? step : limit) + 400);
            }
        }
        cur = (sint32)car->engine_speed;
        step = (sint32)((uint32)target - (uint32)cur) / 16;
        if (cur < target)
        {
            if (step > 400)
                step = 400;
        }
        else if (step > 200)
            step = 200;
        result = (uint32)cur + (uint32)step;
        car->engine_speed = result;
        return result;
    }
    speed = (sint32)car->forward_speed >> 8;
    coefficient = *(gear == 8 ? &desc->upshift_rpm[0] : &desc->gear_ratios[gear]);
    if (speed < 0)
        speed = -speed;
    product = (uint32)speed * (uint32)coefficient;
    cur = (sint32)car->engine_speed;
    product *= 45;
    target = ((uint32)((sint32)product / 4096) + 400);
    difference = ((uint32)cur - (uint32)target);
    if (difference >= 801)
        cur = ((uint32)cur - 200);
    else if (difference < -800)
        cur = ((uint32)cur + 200);
    else
    {
        step = (sint32)((uint32)target - (uint32)cur) / 4;
        cur = ((uint32)cur + (uint32)step);
    }
    model = car->physics_profile;
    limit = model->rpm_limit;
    result = limit < cur;
    if (result != 0)
        cur = limit;
    car->engine_speed = (uint32)cur;
    return result;
}

uint32 car_update_automatic_gears(CAR_STATE *car, const CAR_PHYSICS_PROFILE *desc)
{
    uint32 gear, val, slot, product, factor, result, words[4], idx;
    sint32 pedal, cur, threshold, random_val, coefficient;

    FUNCTION_MARKER(0x80025600, "SLUS_006.10");
    pedal = (sint16)car->pedal;
    gear = car->gear;
    cur = (sint32)car->engine_speed;
    if (pedal < 0)
    {
        car->gear = (uint8)(0);
        return pedal;
    }
    if (gear == 0)
    {
        if (pedal <= 0)
            return 2;
        car->gear = (uint8)(2);
    }
    threshold = *(gear == 8 ? &desc->downshift_rpm[0] : &desc->upshift_rpm[gear]);
    if (threshold < cur && gear < 8 && (sint32)car->forward_speed > 0)
    {
        val = car->gear;
        slot = car->slot;
        car->gear = (uint8)(val + 1);
        if ((sint32)race_session.two_players >= (sint32)slot)
        {
            random_val = randPSX();
            if (random_val % 100 < 80 && car->gear < 4)
                audio_play_gear_shift();
        }
        pedal = (sint16)car->pedal;
        coefficient = desc->drive_coefficient;
        product = (uint32)coefficient * ((uint32)pedal * 26);
        val = car->gear;
        factor = (val == 8 ? 0x01000100u : gear_impulse_factors[val]);
        product = (uint32)((sint32)product / 256) * factor;
        result = ((sint32)product / 256);
        for (idx = 0; idx < 4; ++idx)
            words[idx] = car->suspension_velocity[idx];
        car->suspension_velocity[0] = (uint32)(words[0] + result);
        car->suspension_velocity[1] = (uint32)(words[1] + result);
        car->suspension_velocity[2] = (uint32)(words[2] - result);
        car->suspension_velocity[3] = (uint32)(words[3] - result);
        return result;
    }
    threshold = *(gear == 8 ? &desc->spring : &desc->downshift_rpm[gear]);
    result = gear < 3;
    if (cur < threshold && gear >= 3)
    {
        result = car->gear - 1;
        car->gear = (uint8)(result);
    }
    return result;
}

uint32 car_apply_reverse_pedal(CAR_STATE *car)
{
    uint32 result;

    FUNCTION_MARKER(0x800257E8, "SLUS_006.10");
    result = car->gear;
    if (result == 0)
    {
        result = 0 - (uint32)car->pedal;
        car->pedal = (uint16)(result);
    }
    return result;
}

uint32 car_compute_drive_force(CAR_STATE *car, const CAR_PHYSICS_PROFILE *desc)
{
    uint32 gear, product, result;
    const sint16 *table;
    sint32 pedal, speed, idx, coefficient, sample0, sample1, fraction, val, limit;

    FUNCTION_MARKER(0x80025810, "SLUS_006.10");
    gear = car->gear;
    pedal = (sint16)car->pedal;
    speed = (sint32)car->engine_speed;
    if (gear == 1)
        return 0;
    limit = desc->rpm_limit - 50;
    if (limit < speed)
        return 0;
    idx = speed / 512;
    if (idx < 0 || idx >= 15)
        return 0;
    table = desc->torque_samples + idx;
    val = *table;
    coefficient = desc->drive_coefficient;
    product = (uint32)val * (uint32)coefficient;
    sample0 = (sint32)product / 256;
    val = table[1];
    product = (uint32)val * (uint32)coefficient;
    sample1 = (sint32)product / 256;
    fraction = ((uint32)speed - ((uint32)idx << 9));
    product = ((uint32)sample1 - (uint32)sample0) * (uint32)fraction;
    val = ((uint32)sample0 + (uint32)((sint32)product / 512));
    product = (uint32)pedal * (uint32)val;
    coefficient = *(gear == 8 ? &desc->upshift_rpm[0] : &desc->gear_ratios[gear]);
    product = (uint32)((sint32)product / 256) * (uint32)coefficient;
    result = ((sint32)product / 256);
    return result;
}

static uint32 apply_gear_impulse(CAR_STATE *car, const CAR_PHYSICS_PROFILE *desc)
{
    uint32 rank, level, factor, val, coefficient, a, b, c, d;
    sint32 random, amount;

    FUNCTION_MARKER(0x800254C0, "SLUS_006.10");
    rank = car->slot;
    level = race_session.two_players;
    if ((sint32)level >= (sint32)rank)
    {
        random = randPSX();
        if (random % 100 < 80 && car->gear < 4)
            audio_play_gear_shift();
    }
    factor = (sint16)car->pedal;
    val = (uint32)desc->drive_coefficient;
    val *= factor * 26;
    amount = (sint32)val / 256;
    coefficient = car->gear;
    coefficient = (coefficient == 8 ? 0x01000100u : gear_impulse_factors[coefficient]);
    val = (uint32)amount * coefficient;
    amount = (sint32)val / 256;
    a = car->suspension_velocity[0];
    b = car->suspension_velocity[1];
    c = car->suspension_velocity[2];
    d = car->suspension_velocity[3];
    car->suspension_velocity[0] = (uint32)(a + (uint32)amount);
    car->suspension_velocity[1] = (uint32)(b + (uint32)amount);
    car->suspension_velocity[2] = (uint32)(c - (uint32)amount);
    car->suspension_velocity[3] = (uint32)(d - (uint32)amount);
    return amount;
}

static sint32 car_control_drag(const CAR_STATE *car, sint32 speed)
{
    uint32 factor;
    sint32 speed_drag;
    sint32 limited_drag;

    factor = car->control_drag_factor;
    speed_drag = (sint32)((uint32)speed * factor) / 2048;
    limited_drag = (sint32)(1500 * factor) / 256;
    if (speed_drag < limited_drag)
        limited_drag = speed_drag;
    return (sint32)((uint32)limited_drag * disp_state.time_step) / 256;
}

uint32 car_update_player_input(uint32 slot, uint32 pad)
{
    CAR_STATE *car;
    uint32 controls;
    uint32 mode_snapshot;
    uint32 state;
    const CAR_PHYSICS_PROFILE *desc;
    uint32 gear;
    uint32 maximum_gear;
    uint32 mask;
    uint32 result;
    sint32 speed;
    sint32 yaw;
    sint32 dir;
    sint32 divisor;
    sint32 steering_rate;
    sint32 steering_limit;
    sint32 angle;
    sint32 heading;
    sint32 scale;
    sint32 left_limit;
    sint32 right_limit;
    sint32 steering;
    sint32 ramp;
    sint32 amount;
    sint32 drag;
    sint32 throttle;
    sint32 val;

    FUNCTION_MARKER(0x800269DC, "SLUS_006.10");
    controls = input_ports[pad].controls;
    mode_snapshot = replay_mode;
    car = &car_states[slot];
    (void)car_states[slot].slot;
    result = (uint32)replay_exchange_input((uint32)(sint32)(sint16)controls);
    if (mode_snapshot != 0)
        controls = result;
    if (slot == 1)
    {
        (void)car_states[slot].slot;
        controls = (uint32)replay_exchange_input((uint32)(sint32)(sint16)controls);
    }
    speed = (sint32)car->forward_speed;
    yaw = (sint32)car_states[slot].body_grip_excess;
    state = car_states[slot].wheel_slip_flags;
    speed >>= 8;
    if (speed < 0)
        speed = -speed;
    if (state == 0)
    {
        yaw >>= 8;
        divisor = ((uint32)yaw * (uint32)yaw + 64);
        divisor = ((uint32)fixed_divide_checked((sint32)((uint32)speed << 6), divisor) + 64);
    }
    else
        divisor = 64;
    steering_rate = fixed_divide_checked(0xC0000, divisor);
    dir = car_states[slot].velocity.x;
    heading = car_states[slot].velocity.z;
    state = car_states[slot].wheel_slip_flags;
    divisor = state == 0 ? speed + 128 : 128;
    steering_limit = fixed_divide_checked(0xC00000, divisor);
    angle = fixed_angle_from_vector(dir >> 8, heading >> 8);
    heading = car_states[slot].orientation.y;
    scale = (sint32)car_states[slot].steered_grip_excess;
    angle = (sint32)(((uint32)angle - (uint32)(heading >> 8) + 2048) & 0xFFF) - 2048;
    if (scale < 2048 && (car_states[slot].braking | car_states[slot].handbrake) == 0)
        angle = (sint32)((uint32)angle * (uint32)scale) / 2048;
    left_limit = ((uint32)angle << 8);
    if (left_limit < steering_limit)
        left_limit = steering_limit;
    if (left_limit > 0x18000)
        left_limit = 0x18000;
    right_limit = ((uint32)angle << 8);
    val = (0 - (uint32)steering_limit);
    if (val < right_limit)
        right_limit = val;
    if (right_limit < -0x18000)
        right_limit = -0x18000;
    steering_rate = (sint32)((uint32)steering_rate * disp_state.time_step) / 256;
    if ((controls & 0x8000) != 0)
    {
        throttle = (sint32)((controls & 0xF8) >> 3) - 16;
        if (throttle > 0)
            steering = (sint32)((uint32)throttle * (uint32)left_limit) / 15;
        else
            steering = (sint32)((uint32)(-throttle) * (uint32)right_limit) / 15;
        car_states[slot].steering_angle = (uint32)steering;
        val = (sint32)car_states[slot].body_grip_excess;
        if (val > 0)
        {
            drag = car_control_drag(&car_states[slot], val);
            if (throttle < 16)
            {
                val = car_states[slot].angular_velocity.y;
                if (val > 0)
                {
                    amount = (sint32)((uint32)drag * (uint32)throttle) / 15;
                    car_states[slot].angular_velocity.bits.y = (uint32)((uint32)val + (uint32)amount);
                }
            }
            if (throttle >= 17)
            {
                val = car_states[slot].angular_velocity.y;
                if (val < 0)
                {
                    amount = (sint32)((uint32)drag * (uint32)throttle) / 15;
                    car_states[slot].angular_velocity.bits.y = (uint32)((uint32)val + (uint32)amount);
                }
            }
        }
        if ((controls & 0x1000) != 0)
            goto handbrake;
        car_states[slot].handbrake = (uint8)(0);
        if ((controls & 0x100) != 0)
            goto brake;
        if (race_ai_cars[0].chase_brake != 0)
            chase_update_braking();
        else
        {
            car_states[slot].brake_latch = (uint8)(1);
            car_states[slot].braking = (uint8)(0);
            if (race_ai_cars[0].chase_brake != 0)
                car_states[slot].pedal = (uint16)(race_rules.catchup_scale[pad]);
            else
            {
                val = (sint16)race_rules.catchup_scale[pad];
                amount = (sint32)((controls & 0xE00) >> 9) * val / 7;
                car_states[slot].pedal = (uint16)((uint32)amount);
            }
        }
        goto gears;
    }
    if ((controls & 0x10) != 0)
    {
        ramp = (sint16)car_states[slot].steering_ramp;
        if (ramp < 256)
            car_states[slot].steering_ramp = (uint16)((uint32)(ramp + 64));
        steering = (sint32)car_states[slot].steering_angle;
        amount = ((uint32)steering_rate << 1);
        if (steering <= 0)
            amount = (sint32)((uint32)steering_rate * (uint32)(sint16)car_states[slot].steering_ramp) / 256;
        car_states[slot].steering_angle = (uint32)steering - (uint32)amount;
        if ((sint32)car_states[slot].steering_angle < right_limit)
            car_states[slot].steering_angle = (uint32)right_limit;
    }
    if ((controls & 8) != 0)
    {
        ramp = (sint16)car_states[slot].steering_ramp;
        if (ramp < 256)
            car_states[slot].steering_ramp = (uint16)((uint32)(ramp + 64));
        steering = (sint32)car_states[slot].steering_angle;
        amount = ((uint32)steering_rate << 1);
        if (steering >= 0)
            amount = (sint32)((uint32)steering_rate * (uint32)(sint16)car_states[slot].steering_ramp) / 256;
        car_states[slot].steering_angle = (uint32)steering + (uint32)amount;
        if (left_limit < (sint32)car_states[slot].steering_angle)
            car_states[slot].steering_angle = (uint32)left_limit;
    }
    if ((controls & 0x18) == 0)
    {
        ramp = (sint16)car_states[slot].steering_ramp;
        if (ramp > 0)
            car_states[slot].steering_ramp = (uint16)((uint32)(ramp - 64));
        steering = (sint32)car_states[slot].steering_angle;
        amount = ((uint32)steering_rate << 2);
        if ((sint32)(0 - (uint32)amount) < steering)
        {
            if (steering < amount)
                car_states[slot].steering_angle = 0;
            else
                car_states[slot].steering_angle = (uint32)steering - (uint32)amount;
        }
        else
            car_states[slot].steering_angle = (uint32)steering + (uint32)amount;
    }
    val = (sint32)car_states[slot].steered_grip_excess;
    if (val > 0)
    {
        drag = car_control_drag(&car_states[slot], val);
        if ((controls & 0x10) != 0)
        {
            val = car_states[slot].angular_velocity.y;
            if (val > 0)
                car_states[slot].angular_velocity.bits.y = (uint32)((uint32)val - (uint32)drag);
        }
        if ((controls & 8) != 0)
        {
            val = car_states[slot].angular_velocity.y;
            if (val < 0)
                car_states[slot].angular_velocity.bits.y = (uint32)((uint32)val + (uint32)drag);
        }
    }
    if ((controls & 0x1000) != 0)
    {
    handbrake:
        car_states[slot].pedal = (uint16)(0xFF00);
        car_states[slot].handbrake = (uint8)(1);
        car_states[slot].braking = (uint8)(1);
        goto gears;
    }
    car_states[slot].handbrake = (uint8)(0);
    if ((controls & 0x40) != 0)
    {
    brake:
        if (car_states[slot].automatic_transmission != 0 && car_states[slot].brake_latch == 1 && speed < 10 && car_states[slot].wheel_slip_flags == 0)
            car_states[slot].brake_latch = (uint8)(0);
        state = car_states[slot].brake_latch;
        car_states[slot].pedal = (uint16)(0xFF00);
        car_states[slot].braking = (uint8)(state);
    }
    else if (race_ai_cars[0].chase_brake != 0)
        chase_update_braking();
    else
    {
        car_states[slot].brake_latch = (uint8)(1);
        car_states[slot].braking = (uint8)(0);
        if ((controls & 0x20) != 0)
            car_states[slot].pedal = (uint16)(race_rules.catchup_scale[pad]);
        else
            car_states[slot].pedal = (uint16)(0);
    }
gears:
    if (car_states[slot].automatic_transmission == 0)
    {
        mask = controls & 0x6000;
        if (mask == 0x2000)
        {
            maximum_gear = car_states[slot].gear_count;
            gear = car_states[slot].gear;
            if ((sint32)gear < (sint32)maximum_gear - 1)
            {
                desc = car->physics_profile;
                car_states[slot].gear = (uint8)(gear + 1);
                apply_gear_impulse(car, desc);
                if (car_states[slot].gear == 2)
                {
                    desc = car->physics_profile;
                    val = desc->rpm_limit * 3 / 4;
                    if (val < (sint32)car->engine_speed)
                        car_states[slot].wheel_slip_flags = (uint8)((uint16)desc->drive_type);
                }
            }
        }
        if ((controls & 0x6000) == 0x4000)
        {
            gear = car_states[slot].gear;
            if (gear != 0)
            {
                state = car_states[slot].motion_mode;
                if (state == 0 && gear >= 3)
                {
                    desc = car->physics_profile;
                    val = (sint32)car->forward_speed;
                    amount = *(gear == 0 ? &desc->grip_strength : &desc->gear_ratios[gear - 1]);
                    val >>= 8;
                    if (val < 0)
                        val = -val;
                    val = (sint32)((uint32)val * (uint32)amount * 45) / 4096;
                    amount = desc->rpm_limit;
                    if ((sint32)((uint32)val + 400) < amount)
                    {
                        car_states[slot].gear = (uint8)(gear - 1);
                        goto final_action;
                    }
                }
                else
                {
                    gear = car_states[slot].gear;
                    if (gear != 1)
                    {
                        car_states[slot].gear = (uint8)(gear - 1);
                        goto final_action;
                    }
                    desc = car->physics_profile;
                    val = (sint16)(uint16)desc->rpm_limit;
                    amount = (sint32)car->engine_speed;
                    if (amount < val / 2)
                    {
                        car_states[slot].gear = (uint8)(gear - 1);
                        goto final_action;
                    }
                }
                audio_play_gear_grind_at(&car_states[slot].pos, &car_states[slot].velocity);
            }
        }
    }
final_action:
    if (input_combined_pedal_mode != 0)
    {
        if (car_states[slot].braking != 0)
            return 0x6000;
        result = pad * 2;
        if ((controls & 0x6000) == 0x6000)
        {
            result = (uint32)(sint16)race_rules.catchup_scale[pad] << 1;
            car_states[slot].pedal = (uint16)(result);
        }
        return result;
    }
    if ((controls & 0x6000) != 0x6000)
        return 0x6000;
    if (race_session.cop_chase != 0)
    {
        state = car_states[slot].slot;
        if (state == chase_state.pursuer_slot)
            return chase_raise_signal();
    }
    if (race_session.demo != 0 || session_flow.attract_active != 0)
        return audio_play_horn_at(&car_states[slot].pos, &car_states[slot].velocity);
    return audio_play_random_horn();
}

uint32 car_update_free_engine_speed(CAR_STATE *car)
{
    const CAR_PHYSICS_PROFILE *desc;
    uint32 braking;
    uint32 result;
    sint32 maximum;
    sint32 throttle;
    sint32 target;
    sint32 scaled;
    sint32 cur;
    sint32 step;

    FUNCTION_MARKER(0x8002527C, "SLUS_006.10");
    desc = car->physics_profile;
    braking = car->braking;
    maximum = desc->rpm_limit;
    target = 400;
    if (braking == 0)
    {
        throttle = (sint16)car->pedal;
        if (throttle >= 0)
        {
            scaled = (sint32)((uint32)(maximum - 400) * (uint32)throttle) / 256;
            target = (scaled < maximum ? scaled : maximum) + 400;
        }
    }
    cur = (sint32)car->engine_speed;
    step = (sint32)((uint32)target - (uint32)cur) / 16;
    if (cur < target)
    {
        if (step >= 401)
            step = 400;
    }
    else if (step >= 201)
        step = 200;
    result = (uint32)cur + (uint32)step;
    car->engine_speed = result;
    return result;
}

uint32 car_load_environment(CAR_STATE *car_state, uint32 selection)
{
    uint32 wire_word_7, wire_word_8, wire_word_9, wire_word_10, wire_word_12;
    uint32 type;
    const uint8 *desc;
    uint32 entry_offset;
    uint32 src_offset;
    uint32 compressed_size;
    uint32 decoded_bytes;
    const uint8 *compressed;
    const uint8 *tex;
    uint8 local_tex[16];
    uint8 *image;
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;
    uint32 bytes;
    uint32 src;
    uint32 val;
    uint32 result;

    FUNCTION_MARKER(0x8002EF1C, "SLUS_006.10");
    XASSERT(!(selection > car_environment_render.entries_capacity / 16));
    entry_offset = selection * 16;
    XASSERT(!(entry_offset > car_environment_render.entries_capacity || car_environment_render.entries_capacity - entry_offset < 16));
    desc = car_environment_render.entries + entry_offset;
    memcpy(&type, desc + 4, sizeof(type));
    car_state->environment.selection = (uint8)(selection);
    XASSERT(!(selection > car_environment_render.entries_capacity / 16));
    entry_offset = selection * 16;
    XASSERT(!(entry_offset > car_environment_render.entries_capacity || car_environment_render.entries_capacity - entry_offset < 16));
    desc = car_environment_render.entries + entry_offset;
    memcpy(&type, desc + 4, sizeof(type));
    if (type == 0 || type == 1)
    {
        tex = car_environment_render.tex;
        memcpy(&a, tex, sizeof(a));
        memcpy(&b, tex + 4, sizeof(b));
        memcpy(&c, tex + 8, sizeof(c));
        memcpy(&d, tex + 12, sizeof(d));
        (wire_word_7 = (uint32)(a), memcpy(local_tex, &wire_word_7, sizeof(wire_word_7)));
        (wire_word_8 = (uint32)(b), memcpy(local_tex + 4, &wire_word_8, sizeof(wire_word_8)));
        (wire_word_9 = (uint32)(c), memcpy(local_tex + 8, &wire_word_9, sizeof(wire_word_9)));
        (wire_word_10 = (uint32)(d), memcpy(local_tex + 12, &wire_word_10, sizeof(wire_word_10)));
        bytes = type == 0 ? 0x4100 : 0x2100;
        image = (uint8 *)malloc(bytes);
        XASSERT(!(image == NULL));
        memcpy(&src, desc, sizeof(src));
        src_offset = src - scene_level_image.wire_origin;
        XASSERT(!(src_offset > scene_level_image.size));
        compressed_size = scene_level_image.size - src_offset;
        XASSERT(!(compressed_size < 18));
        compressed = scene_level_image.bytes + src_offset;
        {
            sint32 operation_ok = (memcpy(&wire_word_12, compressed, sizeof(wire_word_12)), wire_word_12) == 0x02434E52u;
            XASSERT(operation_ok);
        }
        decoded_bytes = ((uint32)compressed[4] << 24) | ((uint32)compressed[5] << 16) | ((uint32)compressed[6] << 8) | (uint32)compressed[7];
        XASSERT(!(decoded_bytes < 4 || decoded_bytes > bytes));
        rnc_decode(compressed, compressed_size, image, (sint32)bytes);
        car_upload_environment_tex(image, decoded_bytes, local_tex);
        if (type == 1)
        {
            val = local_tex[15];
            local_tex[15] = (val + 64);
            car_upload_environment_tex(image, decoded_bytes, local_tex);
        }
        free(image);
        memcpy(&type, desc + 4, sizeof(type));
        car_state->environment.type = (uint8)(type);
    }
    else
        car_state->environment.type = (uint8)(type);
    memcpy(&val, desc + 8, sizeof(val));
    car_state->environment.parameter = (uint8)(val);
    memcpy(&val, desc + 12, sizeof(val));
    car_state->environment.level = (uint16)(0);
    result = 127 - val;
    car_state->environment.limit = (uint8)(result);
    return result;
}

void car_update_lighting(CAR_STATE *car)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;
    uint16 wire_halfword_13, wire_halfword_14, wire_halfword_15, wire_halfword_16;
    uint16 wire_halfword_17, wire_halfword_18, wire_halfword_19, wire_halfword_20;
    uint16 wire_halfword_21, wire_halfword_22, wire_halfword_23, wire_halfword_24;
    uint16 wire_halfword_25, wire_halfword_26, wire_halfword_27, wire_halfword_28;
    uint16 wire_halfword_29;

    uint32 wire_word_16;
    VEC3_16 light_dir;
    CAR_LIGHTING_WORK work;
    const uint8 *desc;
    uint32 desc_offset;
    uint32 kind;
    uint32 r;
    uint32 g;
    uint32 b;
    uint32 step;
    const uint8 *segs, *seg, *vertices, *vertex, *other_vertex;
    sint32 sampled_node;
    uint32 vertex_idx;
    uint32 far_idx;
    uint32 selector;
    uint32 height;
    VEC3 *point;
    uint32 val;
    uint32 pos;
    uint32 component;
    uint32 light;
    uint32 norm;
    sint32 node;
    sint32 origin;
    sint32 rounded;
    sint32 span;
    sint32 scaled;
    sint32 strength;
    sint32 channel;

    FUNCTION_MARKER(0x800314B8, "SLUS_006.10");
    val = car->lighting_zone;
    XASSERT(!(val >= car_lighting_zones.count));
    desc_offset = car_lighting_zones.offset + val * 32;
    XASSERT(!(desc_offset > car_lighting_zones.size || car_lighting_zones.size - desc_offset < 32));
    desc = car_lighting_zones.bytes + desc_offset;
    node = (sint16)car->track_location.node;
    while (node < (sint16)(memcpy(&wire_halfword_1, desc, sizeof(wire_halfword_1)), wire_halfword_1))
    {
        XASSERT(!(desc - (car_lighting_zones.bytes + car_lighting_zones.offset) < 32));
        desc -= 32;
        val = car->lighting_zone;
        car->lighting_zone = (uint8)(val - 1);
    }
    while ((sint16)(memcpy(&wire_halfword_2, desc + 2, sizeof(wire_halfword_2)), wire_halfword_2) < node)
    {
        val = car->lighting_zone;
        XASSERT(!(car_lighting_zones.count - (uint32)(desc - (car_lighting_zones.bytes + car_lighting_zones.offset)) / 32 < 2));
        desc += 32;
        car->lighting_zone = (uint8)(val + 1);
    }
    val = car->environment.selection;
    if (val != (memcpy(&wire_word_16, desc + 28, sizeof(wire_word_16)), wire_word_16))
        car->environment.phase = (uint8)(2);
    kind = desc[24];
    if (kind == 0)
    {
        r = desc[16];
        g = desc[17];
        b = desc[18];
        model_set_back_color(r, g, b);
        r = desc[12];
        g = desc[13];
        b = desc[14];
        light_dir.bits.x = (memcpy(&wire_halfword_3, desc + 4, sizeof(wire_halfword_3)), wire_halfword_3);
        light_dir.bits.y = (memcpy(&wire_halfword_4, desc + 6, sizeof(wire_halfword_4)), wire_halfword_4);
        light_dir.bits.z = (memcpy(&wire_halfword_5, desc + 8, sizeof(wire_halfword_5)), wire_halfword_5);
        model_set_light(0, &light_dir, r, g, b);
        light_dir.bits.x = (memcpy(&wire_halfword_6, desc + 4, sizeof(wire_halfword_6)), wire_halfword_6);
        light_dir.bits.y = (memcpy(&wire_halfword_7, desc + 6, sizeof(wire_halfword_7)), wire_halfword_7);
        light_dir.bits.z = (memcpy(&wire_halfword_8, desc + 8, sizeof(wire_halfword_8)), wire_halfword_8);
        model_set_light(1, &light_dir, 0, 0, 0);
        light_dir.bits.x = (memcpy(&wire_halfword_9, desc + 4, sizeof(wire_halfword_9)), wire_halfword_9);
        light_dir.bits.y = (memcpy(&wire_halfword_10, desc + 6, sizeof(wire_halfword_10)), wire_halfword_10);
        light_dir.bits.z = (memcpy(&wire_halfword_11, desc + 8, sizeof(wire_halfword_11)), wire_halfword_11);
        model_set_light(2, &light_dir, 0, 0, 0);
        model_apply_light_matrix(&car->transform);
        return;
    }
    if (kind == 3)
    {
        span = (sint16)(memcpy(&wire_halfword_12, desc + 2, sizeof(wire_halfword_12)), wire_halfword_12);
        origin = (sint16)(memcpy(&wire_halfword_13, desc, sizeof(wire_halfword_13)), wire_halfword_13);
        span = span - origin + 1;
        if (node - origin < span / 2)
            blend_lighting_start(car, desc, (sint32)((uint32)span << 9));
        else
            blend_lighting_end(car, desc, (sint32)((uint32)span << 9));
        return;
    }
    if (kind != 1 && kind != 2)
        return;
    if (node - (sint16)(memcpy(&wire_halfword_14, desc, sizeof(wire_halfword_14)), wire_halfword_14) < 3)
    {
        blend_lighting_start(car, desc, 2048);
        return;
    }
    if ((sint16)(memcpy(&wire_halfword_15, desc + 2, sizeof(wire_halfword_15)), wire_halfword_15) - node < 3)
    {
        blend_lighting_end(car, desc, 2048);
        return;
    }
    origin = (sint16)(memcpy(&wire_halfword_16, desc + 20, sizeof(wire_halfword_16)), wire_halfword_16);
    step = desc[25];
    rounded = fixed_divide_checked(node - origin + (sint32)(step >> 1), step);
    rounded = ((uint32)rounded * step + (uint32)origin);
    if (kind == 1)
    {
        r = desc[12];
        g = desc[13];
        segs = track_geometry.node_bytes;
        sampled_node = rounded;
        if ((uint32)sampled_node >= track_geometry.node_count)
        {
            sampled_node %= (sint32)track_geometry.main_node_count;
            if (sampled_node < 0)
                sampled_node += (sint32)track_geometry.main_node_count;
        }
        seg = segs + (uint32)sampled_node * 24;
        far_idx = (seg + 3)[0] & 15;
        vertex_idx = (memcpy(&wire_halfword_17, seg + 4, sizeof(wire_halfword_17)), wire_halfword_17);
        b = desc[14];
        vertices = track_geometry.vertex_bytes;
        far_idx += vertex_idx;
        XASSERT(!(vertex_idx >= track_geometry.vertex_count || far_idx >= track_geometry.vertex_count));
        vertex = vertices + vertex_idx * 6;
        height = 0;
        for (light = 0; light < 2; ++light)
        {
            if (light != 0)
                vertex = track_geometry.vertex_bytes + far_idx * 6;
            for (component = 0; component < 3; ++component)
            {
                memcpy(&val, seg + 12 + component * 4, sizeof(val));
                val += (memcpy(&wire_halfword_18, vertex + component * 2, sizeof(wire_halfword_18)), wire_halfword_18);
                pos = car->pos.u[component];
                if (light == 0 && component == 0)
                    height = (memcpy(&wire_halfword_19, desc + 22, sizeof(wire_halfword_19)), wire_halfword_19);
                val -= (uint32)((sint32)pos / 256);
                if (component == 1)
                    val += height;
                work.dir.component[component] = (sint16)(uint16)val;
            }
            norm = VectorNormalSS(&work.dir.sdk, &work.dir.sdk);
            scaled = (norm * desc[27]);
            scaled /= 16384;
            strength = scaled < 4097 ? (sint32)(4096 - (uint32)scaled) : 0;
            channel = (sint32)(r * (uint32)strength) / 4096;
            val = ((sint32)(g * (uint32)strength) / 4096);
            pos = ((sint32)(b * (uint32)strength) / 4096);
            light_dir.x = work.dir.sdk.vx;
            light_dir.y = work.dir.sdk.vy;
            light_dir.z = work.dir.sdk.vz;
            model_set_light(light, &light_dir, channel, val, pos);
        }
        r = desc[16];
        g = desc[17];
        b = desc[18];
        model_set_back_color(r, g, b);
        light_dir.bits.x = (memcpy(&wire_halfword_20, desc + 4, sizeof(wire_halfword_20)), wire_halfword_20);
        light_dir.bits.y = (memcpy(&wire_halfword_21, desc + 6, sizeof(wire_halfword_21)), wire_halfword_21);
        light_dir.bits.z = (memcpy(&wire_halfword_22, desc + 8, sizeof(wire_halfword_22)), wire_halfword_22);
        model_set_light(2, &light_dir, 0, 0, 0);
        model_apply_light_matrix(&car->transform);
        return;
    }
    segs = track_geometry.node_bytes;
    r = desc[12];
    selector = desc[26];
    g = desc[13];

    work.g = g;
    b = desc[14];
    height = (memcpy(&wire_halfword_23, desc + 22, sizeof(wire_halfword_23)), wire_halfword_23);
    if (selector < 3)
    {
        vertices = NULL;
        for (light = 0; light < 3; ++light)
        {
            if (light == 0)
                sampled_node = ((uint32)rounded - step);
            else if (light == 1)
                sampled_node = rounded;
            else
                sampled_node = ((uint32)rounded + desc[25]);
            if ((uint32)sampled_node >= track_geometry.node_count)
            {
                sampled_node %= (sint32)track_geometry.main_node_count;
                if (sampled_node < 0)
                    sampled_node += (sint32)track_geometry.main_node_count;
            }
            seg = segs + (uint32)sampled_node * 24;
            if (selector == 2)
            {
                far_idx = (memcpy(&wire_halfword_24, seg + 6, sizeof(wire_halfword_24)), wire_halfword_24) - 1;
                vertex_idx = (memcpy(&wire_halfword_25, seg + 4, sizeof(wire_halfword_25)), wire_halfword_25);
            }
            else if (selector == 1)
                vertex_idx = (memcpy(&wire_halfword_26, seg + 6, sizeof(wire_halfword_26)), wire_halfword_26) - 1;
            else
                vertex_idx = (memcpy(&wire_halfword_27, seg + 4, sizeof(wire_halfword_27)), wire_halfword_27);
            if (light == 0)
                vertices = track_geometry.vertex_bytes;
            XASSERT(!(vertex_idx >= track_geometry.vertex_count));
            vertex = vertices + vertex_idx * 6;
            XASSERT(!(selector == 2 && far_idx >= track_geometry.vertex_count));
            other_vertex = selector == 2 ? vertices + far_idx * 6 : NULL;
            point = &work.points[light];
            for (component = 0; component < 3; ++component)
            {
                scaled = (sint16)(memcpy(&wire_halfword_28, vertex + component * 2, sizeof(wire_halfword_28)), wire_halfword_28);
                if (selector == 2)
                    scaled += (sint16)(memcpy(&wire_halfword_29, other_vertex + component * 2, sizeof(wire_halfword_29)), wire_halfword_29);
                memcpy(&val, seg + 12 + component * 4, sizeof(val));
                if (selector == 2)
                    scaled /= 2;
                point->u[component] = (uint32)scaled + val;
            }
        }
    }
    else
    {
        for (light = 0; light < 3; ++light)
        {
            point = &work.points[light];
            for (component = 0; component < 3; ++component)
                point->u[component] = 0;
        }
    }
    for (light = 0; light < 3; ++light)
    {
        point = &work.points[light];
        for (component = 0; component < 3; ++component)
        {
            pos = car->pos.u[component];
            val = point->u[component];
            val -= (uint32)((sint32)pos / 256);
            if (component == 1)
                val += height;
            work.dir.component[component] = (sint16)(uint16)val;
        }
        norm = VectorNormalSS(&work.dir.sdk, &work.dir.sdk);
        scaled = (norm * desc[27]);
        scaled /= 16384;
        strength = scaled < 4097 ? (sint32)(4096 - (uint32)scaled) : 0;
        channel = (sint32)(r * (uint32)strength) / 4096;
        g = work.g;
        val = ((sint32)(g * (uint32)strength) / 4096);
        pos = ((sint32)(b * (uint32)strength) / 4096);
        light_dir.x = work.dir.sdk.vx;
        light_dir.y = work.dir.sdk.vy;
        light_dir.z = work.dir.sdk.vz;
        model_set_light(light, &light_dir, channel, val, pos);
    }
    r = desc[16];
    g = desc[17];
    b = desc[18];
    model_set_back_color(r, g, b);
    model_apply_light_matrix(&car->transform);
}

uint32 car_emit_rear_effect(CAR_STATE *car)
{
    VEC3 spawn_point;
    uint32 val0, val1, result;
    sint32 strength, denominator, quotient, remainder;

    FUNCTION_MARKER(0x80040EA4, "SLUS_006.10");
    strength = (sint32)car->engine_speed;
    if (strength >= 4000)
        return 0;
    if ((sint16)car->pedal < 201)
        return 1;
    if (strength <= 0)
        return 0;
    result = (uint32)randPSX();
    denominator = (sint32)car->engine_speed;
    quotient = fixed_divide_checked((sint32)result, denominator);
    remainder = (result - (uint32)quotient * (uint32)denominator);
    if (remainder >= 500)
        return result;
    val0 = car->body_contact_pos[2].bits.y;
    val1 = car->body_contact_pos[3].bits.y;
    val0 = (uint32)((sint32)(val0 + val1) / 2) + 30720;
    spawn_point.bits.y = val0;
    val0 = car->body_contact_pos[2].bits.x;
    val1 = car->body_contact_pos[3].bits.x;
    spawn_point.bits.x = (uint32)((sint32)(val0 + val1) / 2);
    val0 = car->body_contact_pos[2].bits.z;
    val1 = car->body_contact_pos[3].bits.z;
    spawn_point.bits.z = (uint32)((sint32)(val0 + val1) / 2);
    return effect_spawn_rear_spr(car, &spawn_point, 0);
}

uint32 car_update_skid_effects(CAR_STATE *car)
{
    uint32 val;
    uint32 incoming_result = 1;
    const CAR_PHYSICS_PROFILE *model;
    uint32 flags;
    uint32 slot;
    sint32 amount;
    sint32 other;
    sint32 type;

    FUNCTION_MARKER(0x80045388, "SLUS_006.10");
    val = race_control.start_hold;
    if (val != 0)
        return val;
    amount = (sint32)car->steered_grip_excess;
    other = (sint32)car->body_grip_excess;
    if (amount < other)
        amount = other;
    if (car->wheel_slip_flags != 0)
        amount = 0x7FFF;
    if (car->wheel_air_mask == 15)
        amount = 0;
    slot = car->slot;
    audio_set_car_engine_gain(amount, slot);
    amount = (sint32)car->steered_grip_excess;
    effect_update_front_grip_skids(car, amount);
    amount = (sint32)car->body_grip_excess;
    effect_update_rear_grip_skids(car, amount);
    model = car->physics_profile;
    type = model->drive_type;
    if (type == 2)
    {
        flags = car->wheel_slip_flags;
        return effect_update_front_slip_skids(car, flags, 1);
    }
    if (type < 3)
    {
        if (type != 1)
            return 1;
    }
    else
    {
        if (type != 3)
            return 3;
        flags = car->wheel_slip_flags;
        incoming_result = effect_update_front_slip_skids(car, flags, 3);
    }
    flags = car->wheel_slip_flags;
    return effect_update_rear_slip_skids(car, flags, incoming_result);
}

static void copy_car_metadata(const uint8 *data, uint32 capacity, uint32 slot)
{
    const uint8 *src;
    uint32 src_offset;
    CAR_PHYSICS_PROFILE *dst;
    uint32 idx;
    uint32 a;
    uint32 b;
    uint32 c;
    uint32 d;

    XASSERT(!(slot >= 6 || capacity < 88));
    memcpy(&src_offset, data + 84, sizeof(src_offset));
    XASSERT(!(src_offset > capacity || capacity - src_offset < 140));
    src = data + src_offset;
    for (idx = 0; idx < 8; ++idx)
    {
        memcpy(&a, src, sizeof(a));
        memcpy(&b, src + 4, sizeof(b));
        memcpy(&c, src + 8, sizeof(c));
        memcpy(&d, src + 12, sizeof(d));
        if (idx >= 4 && idx < 6)
        {
            car_suspension_points[slot][(idx - 4) * 2].vx = (sint16)(uint16)a;
            car_suspension_points[slot][(idx - 4) * 2].vy = (sint16)(uint16)(a >> 16);
            car_suspension_points[slot][(idx - 4) * 2].vz = (sint16)(uint16)b;
            car_suspension_points[slot][(idx - 4) * 2].pad = (sint16)(uint16)(b >> 16);
            car_suspension_points[slot][(idx - 4) * 2 + 1].vx = (sint16)(uint16)c;
            car_suspension_points[slot][(idx - 4) * 2 + 1].vy = (sint16)(uint16)(c >> 16);
            car_suspension_points[slot][(idx - 4) * 2 + 1].vz = (sint16)(uint16)d;
            car_suspension_points[slot][(idx - 4) * 2 + 1].pad = (sint16)(uint16)(d >> 16);
        }
        else if (idx >= 6)
        {
            car_lamp_points[slot][(idx - 6) * 2].vx = (sint16)(uint16)a;
            car_lamp_points[slot][(idx - 6) * 2].vy = (sint16)(uint16)(a >> 16);
            car_lamp_points[slot][(idx - 6) * 2].vz = (sint16)(uint16)b;
            car_lamp_points[slot][(idx - 6) * 2].pad = (sint16)(uint16)(b >> 16);
            car_lamp_points[slot][(idx - 6) * 2 + 1].vx = (sint16)(uint16)c;
            car_lamp_points[slot][(idx - 6) * 2 + 1].vy = (sint16)(uint16)(c >> 16);
            car_lamp_points[slot][(idx - 6) * 2 + 1].vz = (sint16)(uint16)d;
            car_lamp_points[slot][(idx - 6) * 2 + 1].pad = (sint16)(uint16)(d >> 16);
        }
        else
        {
            car_body_points[slot][idx * 2].vx = (sint16)(uint16)a;
            car_body_points[slot][idx * 2].vy = (sint16)(uint16)(a >> 16);
            car_body_points[slot][idx * 2].vz = (sint16)(uint16)b;
            car_body_points[slot][idx * 2].pad = (sint16)(uint16)(b >> 16);
            car_body_points[slot][idx * 2 + 1].vx = (sint16)(uint16)c;
            car_body_points[slot][idx * 2 + 1].vy = (sint16)(uint16)(c >> 16);
            car_body_points[slot][idx * 2 + 1].vz = (sint16)(uint16)d;
            car_body_points[slot][idx * 2 + 1].pad = (sint16)(uint16)(d >> 16);
        }
        src += 16;
    }
    memcpy(&a, src, sizeof(a));
    memcpy(&b, src + 4, sizeof(b));
    memcpy(&c, src + 8, sizeof(c));
    car_geometry_metrics[slot].col_radius = (uint16)a;
    car_geometry_metrics[slot].wheel_radius = (uint16)(a >> 16);
    car_geometry_metrics[slot].wheel_width = (uint16)b;
    car_geometry_metrics[slot].ground_offset = (uint16)(b >> 16);
    car_geometry_metrics[slot].impact_coefficient = (uint16)c;
    memcpy(&src_offset, data + 84, sizeof(src_offset));
    XASSERT(!(src_offset > capacity || capacity - src_offset < 268));
    src = data + src_offset + 140;
    dst = &profiles[slot];
    for (idx = 0; idx < 8; ++idx)
    {
        memcpy(&a, src, sizeof(a));
        memcpy(&b, src + 4, sizeof(b));
        memcpy(&c, src + 8, sizeof(c));
        memcpy(&d, src + 12, sizeof(d));
        switch (idx)
        {
            case 0:
                dst->torque_samples[0] = (sint16)(uint16)(a);
                dst->torque_samples[1] = (sint16)(uint16)(a >> 16);
                dst->torque_samples[2] = (sint16)(uint16)(b);
                dst->torque_samples[3] = (sint16)(uint16)(b >> 16);
                dst->torque_samples[4] = (sint16)(uint16)(c);
                dst->torque_samples[5] = (sint16)(uint16)(c >> 16);
                dst->torque_samples[6] = (sint16)(uint16)(d);
                dst->torque_samples[7] = (sint16)(uint16)(d >> 16);
                break;
            case 1:
                dst->torque_samples[8] = (sint16)(uint16)(a);
                dst->torque_samples[9] = (sint16)(uint16)(a >> 16);
                dst->torque_samples[10] = (sint16)(uint16)(b);
                dst->torque_samples[11] = (sint16)(uint16)(b >> 16);
                dst->torque_samples[12] = (sint16)(uint16)(c);
                dst->torque_samples[13] = (sint16)(uint16)(c >> 16);
                dst->torque_samples[14] = (sint16)(uint16)(d);
                dst->torque_samples[15] = (sint16)(uint16)(d >> 16);
                break;
            case 2:
                dst->inertia = a;
                dst->mass = b;
                dst->front_distance = (sint16)(uint16)(c);
                dst->rear_distance = (sint16)(uint16)(c >> 16);
                dst->grip_strength = (sint16)(uint16)(d);
                dst->gear_ratios[0] = (sint16)(uint16)(d >> 16);
                break;
            case 3:
                dst->gear_ratios[1] = (sint16)(uint16)(a);
                dst->gear_ratios[2] = (sint16)(uint16)(a >> 16);
                dst->gear_ratios[3] = (sint16)(uint16)(b);
                dst->gear_ratios[4] = (sint16)(uint16)(b >> 16);
                dst->gear_ratios[5] = (sint16)(uint16)(c);
                dst->gear_ratios[6] = (sint16)(uint16)(c >> 16);
                dst->gear_ratios[7] = (sint16)(uint16)(d);
                dst->upshift_rpm[0] = (sint16)(uint16)(d >> 16);
                break;
            case 4:
                dst->upshift_rpm[1] = (sint16)(uint16)(a);
                dst->upshift_rpm[2] = (sint16)(uint16)(a >> 16);
                dst->upshift_rpm[3] = (sint16)(uint16)(b);
                dst->upshift_rpm[4] = (sint16)(uint16)(b >> 16);
                dst->upshift_rpm[5] = (sint16)(uint16)(c);
                dst->upshift_rpm[6] = (sint16)(uint16)(c >> 16);
                dst->upshift_rpm[7] = (sint16)(uint16)(d);
                dst->downshift_rpm[0] = (sint16)(uint16)(d >> 16);
                break;
            case 5:
                dst->downshift_rpm[1] = (sint16)(uint16)(a);
                dst->downshift_rpm[2] = (sint16)(uint16)(a >> 16);
                dst->downshift_rpm[3] = (sint16)(uint16)(b);
                dst->downshift_rpm[4] = (sint16)(uint16)(b >> 16);
                dst->downshift_rpm[5] = (sint16)(uint16)(c);
                dst->downshift_rpm[6] = (sint16)(uint16)(c >> 16);
                dst->downshift_rpm[7] = (sint16)(uint16)(d);
                dst->spring = (sint16)(uint16)(d >> 16);
                break;
            case 6:
                dst->damping = (sint16)(uint16)(a);
                dst->suspension_response = (sint16)(uint16)(a >> 16);
                dst->suspension_limit = (sint16)(uint16)(b);
                dst->suspension_extra = (sint16)(uint16)(b >> 16);
                dst->drive_coefficient = (sint16)(uint16)(c);
                dst->steering_response = (sint16)(uint16)(c >> 16);
                dst->motion_coefficient_108 = (sint16)(uint16)(d);
                dst->front_force_weight = (sint16)(uint16)(d >> 16);
                break;
            case 7:
                dst->rear_force_weight = (sint16)(uint16)(a);
                dst->rpm_limit = (sint16)(uint16)(a >> 16);
                dst->motion_coefficient_116 = (sint16)(uint16)(b);
                dst->drive_type = (sint16)(uint16)(b >> 16);
                dst->control_drag = (sint16)(uint16)(c);
                dst->motion_coefficient_122 = (sint16)(uint16)(c >> 16);
                dst->slip_response = (sint16)(uint16)(d);
                break;
        }
        src += 16;
    }
}

void car_load_race_models(void)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;

    uint32 pal_y;
    const uint8 *tex_carrier;
    MODEL_TEXTURE_ATTRIBUTES local_tex;
    MODEL_TEXTURE_SOURCE primary_tex;
    uint32 saved[7];
    uint32 idx;
    uint32 offset;
    uint32 tex_number = 0;
    uint32 model_storage_used = 0;
    uint8 *model_bytes;
    uint32 slot = 0;
    sint32 count;
    uint32 master;
    sint32 primary;
    sint32 choice;
    const uint8 *choice_address;
    uint32 variant;
    uint32 root_cursor;
    const char *filename;
    const char *tex_name;
    uint32 size;
    uint32 compressed_size;
    uint32 table_offset;
    uint32 root_offset;
    uint32 work_size;
    uint8 *work_bytes = (uint8 *)race_model_workspace;
    uint8 *compressed_bytes;
    uint32 val;
    uint32 limit;
    uint32 flag;
    uint32 x;
    uint32 y;
    uint32 clut_x;
    uint32 tpage;
    uint32 clut;
    uint32 u;
    uint32 v;
    uint32 selector;
    uint32 tim;
    uint32 tim_offset;
    uint32 src_offset;

    uint32 name_len;

    FUNCTION_MARKER(0x80046638, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(race_model_storage, (uint32)sizeof(race_model_storage)));
        XASSERT(operation_ok);
    }
    memset(car_model_assets, 0, sizeof(car_model_assets));
    variant = session_options.model_variant;
    if (variant == 0)
    {
        for (idx = 0; idx < 6; ++idx)
        {
            offset = idx * 24;
            saved[0] = session_car_choices.cars[idx].tuning[0][0];
            saved[1] = session_car_choices.cars[idx].tuning[0][1];
            saved[2] = session_car_choices.cars[idx].tuning[0][2];
            saved[3] = session_car_choices.cars[idx].tuning[0][3];
            saved[4] = session_car_choices.cars[idx].model[0];
            saved[5] = session_car_choices.cars[idx].primary_color[0];
            saved[6] = session_car_choices.cars[idx].secondary_color[0];
            session_car_choices.cars[idx].tuning[1][0] = (uint16)(saved[0]);
            session_car_choices.cars[idx].tuning[1][1] = (uint16)(saved[1]);
            session_car_choices.cars[idx].tuning[1][2] = (uint16)(saved[2]);
            session_car_choices.cars[idx].tuning[1][3] = (uint16)(saved[3]);
            session_car_choices.cars[idx].model[1] = (uint8)(saved[4]);
            session_car_choices.cars[idx].primary_color[1] = (uint8)(saved[5]);
            session_car_choices.cars[idx].secondary_color[1] = (uint8)(saved[6]);
        }
    }
    (void)(race_view.primary_car);
    for (;;)
    {
        count = (sint32)race_control.car_count;
        if (count >= 7)
            count = 6;
        if ((sint32)slot >= count)
            break;
        master = race_view.primary_car;
        primary = slot == master;
        if (!primary && race_session.two_players != 0)
            primary = slot == master + 1;
        if (!primary && game_options.restrict_primary_models == 0)
            primary = race_session.demo != 0;
        choice_address = session_car_choices.cars[slot].model;
        variant = session_options.model_variant;
        choice = (sint8)choice_address[variant];
        if (!primary && choice == -1)
            goto next_car;
        XASSERT(!((uint32)choice >= sizeof(car_asset_names) / sizeof(car_asset_names[0])));
        filename = car_asset_names[(uint32)choice];
        size = archive_aligned_size(filename);
        variant = session_options.model_variant;
        choice = (sint8)choice_address[variant];
        compressed_bytes = (uint8 *)malloc(size ? size : 1);
        XASSERT(!(compressed_bytes == NULL));
        XASSERT(!((uint32)choice >= sizeof(car_asset_names) / sizeof(car_asset_names[0])));
        filename = car_asset_names[(uint32)choice];
        compressed_size = archive_read_file(filename, compressed_bytes, size);
        XASSERT(!(compressed_size > size || compressed_size < 18));
        XASSERT(!(compressed_bytes[0] != 'R' || compressed_bytes[1] != 'N' || compressed_bytes[2] != 'C' || compressed_bytes[3] != 2));
        work_size = rnc_allocation_size(compressed_bytes) - 1024;
        XASSERT(!(work_size < 88 || work_size > sizeof(race_model_workspace)));
        rnc_decode(compressed_bytes, compressed_size, work_bytes, 0x32000);
        hd_load_race_car(slot, work_bytes, work_size, primary);
        memcpy(&val, work_bytes, sizeof(val));
        idx = 1;
        if (val != 0)
        {
            root_cursor = 4;
            do
            {
                XASSERT(!(root_cursor > work_size || work_size - root_cursor < 4));
                memcpy(&val, work_bytes + root_cursor, sizeof(val));
                XASSERT(!(val > work_size));
                memcpy(&limit, work_bytes, sizeof(limit));
                ++idx;
                root_cursor += 4;
            } while (idx <= limit);
        }
        if (primary)
        {
            flag = race_session.demo;
            table_offset = 0;
            if (flag != 0 && game_options.restrict_primary_models == 0)
                goto dynamic_texture_logged;
            master = race_view.primary_car;
            if (slot == master)
            {
                tex_carrier = tex_find("Hicar");
                primary_tex.attributes = NULL;
                primary_tex.resource = tex_carrier;
                tpage = (memcpy(&wire_halfword_1, tex_carrier + 8, sizeof(wire_halfword_1)), wire_halfword_1);
                clut = (memcpy(&wire_halfword_2, tex_carrier + 10, sizeof(wire_halfword_2)), wire_halfword_2);
                x = (tpage << 6) & 0x3FF;
                y = ((tpage << 4) & 0x100) + ((tpage >> 2) & 0x200);
                clut_x = (clut & 63) << 4;
                pal_y = (clut >> 6) & 255;
                goto car_texture_ready;
            }
            x = (slot % 3) * 64 + 320;
            y = (slot / 3) * 256;
            clut_x = 512;
            pal_y = slot + 40;
            val = GetTPage(1, 0, (sint32)x, (sint32)y);
            local_tex.page = (uint16)val;
            val = GetClut(512, (sint32)pal_y);
            local_tex.clut = (uint16)val;
            primary_tex.attributes = &local_tex;
            primary_tex.resource = NULL;
            goto car_texture_ready;
        dynamic_texture_logged:
            x = (slot % 3) * 64 + 320;
            y = (slot / 3) * 256;
            clut_x = 512;
            pal_y = slot + 40;
            val = GetTPage(1, 0, (sint32)x, (sint32)y);
            local_tex.page = (uint16)val;
            val = GetClut(512, (sint32)pal_y);
            local_tex.clut = (uint16)val;
            primary_tex.attributes = &local_tex;
            primary_tex.resource = NULL;
            (void)(slot), (void)(x), (void)(y), (void)(512), (void)(pal_y);
        car_texture_ready:
            variant = session_options.model_variant;
            selector = session_car_choices.cars[slot].primary_color[variant];
            root_offset = table_offset + selector * 4 + 16;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&tim, work_bytes + root_offset, sizeof(tim));
            tim_offset = tim;
            XASSERT(!(tim_offset > work_size));
            tex_upload_tim(work_bytes + tim_offset, work_size - tim_offset, (sint16)x, (sint16)y, (sint16)clut_x, (sint16)pal_y);
            copy_car_metadata(work_bytes, work_size, slot);
            root_offset = table_offset + 4;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            {
                uint32 vertex_offset;
                uint32 vertex_bytes;
                sint32 vertex_count;
                const SVECTOR *vertices = NULL;
                XASSERT(!(src_offset > work_size || work_size - src_offset < 48));
                memcpy(&vertex_offset, work_bytes + src_offset + 44, sizeof(vertex_offset));
                vertex_count = (sint16)(memcpy(&wire_halfword_3, work_bytes + src_offset + 14, sizeof(wire_halfword_3)), wire_halfword_3);
                if (vertex_count > 0)
                {
                    vertex_bytes = (uint32)(vertex_count - 1) * sizeof(SVECTOR) + 3 * sizeof(sint16);
                    XASSERT(!(vertex_offset > work_size - src_offset || vertex_bytes > work_size - src_offset - vertex_offset));
                    vertices = (const SVECTOR *)(work_bytes + src_offset + vertex_offset);
                }
                car_fit_model_bounds(vertices, vertex_count, slot);
            }
            root_offset = table_offset + 8;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&val, work_bytes + root_offset, sizeof(val));
            root_offset = table_offset + 4;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            size = val - src_offset;
            XASSERT(!(src_offset > work_size || size > work_size - src_offset));
            XASSERT(!(size < 60 || size > sizeof(race_model_storage) - model_storage_used));
            model_bytes = (uint8 *)race_model_storage + model_storage_used;
            model_storage_used += (size + 3) & ~3u;
            car_model_assets[slot][0].bytes = model_bytes;
            car_model_assets[slot][0].capacity = size;
            root_offset = table_offset + 4;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            memcpy(car_model_assets[slot][0].bytes, work_bytes + src_offset, size);
            {
                uint32 groups_offset = ((const MODEL_HEADER *)model_bytes)->stream_offset;
                uint32 groups_bytes = ((const MODEL_HEADER *)model_bytes)->packet_offsets[0] - groups_offset;
                XASSERT(!(groups_offset > size || groups_bytes > size - groups_offset || size - groups_offset < 4));
                model_reset_prim_colors(model_bytes + groups_offset);
            }
            {
                MODEL_PRIMITIVE_BUFFERS streams;
                uint32 groups_offset = ((const MODEL_HEADER *)model_bytes)->stream_offset;
                uint32 packet_offset1 = ((const MODEL_HEADER *)model_bytes)->packet_offsets[1];
                uint32 packet_bytes;
                const MODEL_TEXTURE_SOURCE *native_tex;
                streams.packet_offset = ((const MODEL_HEADER *)model_bytes)->packet_offsets[0];
                XASSERT(!(groups_offset > size || streams.packet_offset - groups_offset > size - groups_offset || size - groups_offset < 4));
                streams.groups = model_bytes + groups_offset;
                if (streams.groups[3] == 0)
                {
                    streams.packets[0] = NULL;
                    streams.packets[1] = NULL;
                    native_tex = NULL;
                }
                else
                {
                    packet_bytes = packet_offset1 - streams.packet_offset;
                    XASSERT(!(streams.packet_offset > size || packet_bytes > size - streams.packet_offset || packet_offset1 > size || packet_bytes > size - packet_offset1));
                    streams.packets[0] = model_bytes + streams.packet_offset;
                    streams.packets[1] = model_bytes + packet_offset1;
                    native_tex = &primary_tex;
                }
                model_set_gouraud_tex(&streams, native_tex, 0);
            }
            master = race_view.primary_car;
            if (slot == master)
            {
                tex_carrier = tex_find("racehub");
                clut = (memcpy(&wire_halfword_4, tex_carrier + 10, sizeof(wire_halfword_4)), wire_halfword_4);
                tpage = (memcpy(&wire_halfword_5, tex_carrier + 8, sizeof(wire_halfword_5)), wire_halfword_5);
                u = tex_carrier[14];
                variant = session_options.model_variant;
                v = tex_carrier[15];
                pal_y = (clut >> 6) & 255;
                selector = session_car_choices.cars[slot].secondary_color[variant];
                x = ((tpage << 6) & 0x3FF) | (u >> 2);
                y = (((tpage << 4) & 0x100) + ((tpage >> 2) & 0x200)) | v;
                root_offset = table_offset + selector * 4 + 36;
                XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
                memcpy(&tim, work_bytes + root_offset, sizeof(tim));
                tim_offset = tim;
                XASSERT(!(tim_offset > work_size));
                tex_upload_tim(work_bytes + tim_offset, work_size - tim_offset, (sint16)x, (sint16)y, (sint16)((clut & 63) << 4), (sint16)pal_y);
                DrawSync(0);
            }
            if (race_session.two_players == 0 || slot >= 2)
                goto free_primary;
            root_offset = table_offset + 12;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&val, work_bytes + root_offset, sizeof(val));
            root_offset = table_offset + 8;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            size = val - src_offset;
            XASSERT(!(src_offset > work_size || size > work_size - src_offset));
            XASSERT(!(size < 60 || size > sizeof(race_model_storage) - model_storage_used));
            model_bytes = (uint8 *)race_model_storage + model_storage_used;
            model_storage_used += (size + 3) & ~3u;
            car_model_assets[slot][1].bytes = model_bytes;
            car_model_assets[slot][1].capacity = size;
            root_offset = table_offset + 8;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            memcpy(car_model_assets[slot][1].bytes, work_bytes + src_offset, size);
        }
        else
        {
            table_offset = 0;
            copy_car_metadata(work_bytes, work_size, slot);
            root_offset = table_offset + 4;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            {
                uint32 vertex_offset;
                uint32 vertex_bytes;
                sint32 vertex_count;
                const SVECTOR *vertices = NULL;
                XASSERT(!(src_offset > work_size || work_size - src_offset < 48));
                memcpy(&vertex_offset, work_bytes + src_offset + 44, sizeof(vertex_offset));
                vertex_count = (sint16)(memcpy(&wire_halfword_6, work_bytes + src_offset + 14, sizeof(wire_halfword_6)), wire_halfword_6);
                if (vertex_count > 0)
                {
                    vertex_bytes = (uint32)(vertex_count - 1) * sizeof(SVECTOR) + 3 * sizeof(sint16);
                    XASSERT(!(vertex_offset > work_size - src_offset || vertex_bytes > work_size - src_offset - vertex_offset));
                    vertices = (const SVECTOR *)(work_bytes + src_offset + vertex_offset);
                }
                car_fit_model_bounds(vertices, vertex_count, slot);
            }
            root_offset = table_offset + 12;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&val, work_bytes + root_offset, sizeof(val));
            root_offset = table_offset + 8;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            size = val - src_offset;
            XASSERT(!(src_offset > work_size || size > work_size - src_offset));
            XASSERT(!(size < 60 || size > sizeof(race_model_storage) - model_storage_used));
            model_bytes = (uint8 *)race_model_storage + model_storage_used;
            model_storage_used += (size + 3) & ~3u;
            car_model_assets[slot][0].bytes = model_bytes;
            car_model_assets[slot][0].capacity = size;
            root_offset = table_offset + 8;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            memcpy(car_model_assets[slot][0].bytes, work_bytes + src_offset, size);
            {
                uint32 groups_offset = ((const MODEL_HEADER *)model_bytes)->stream_offset;
                uint32 groups_bytes = ((const MODEL_HEADER *)model_bytes)->packet_offsets[0] - groups_offset;
                XASSERT(!(groups_offset > size || groups_bytes > size - groups_offset || size - groups_offset < 4));
                model_reset_prim_colors(model_bytes + groups_offset);
            }
            root_offset = table_offset + 16;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&val, work_bytes + root_offset, sizeof(val));
            root_offset = table_offset + 12;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            size = val - src_offset;
            XASSERT(!(src_offset > work_size || size > work_size - src_offset));
            XASSERT(!(size < 60 || size > sizeof(race_model_storage) - model_storage_used));
            model_bytes = (uint8 *)race_model_storage + model_storage_used;
            model_storage_used += (size + 3) & ~3u;
            car_model_assets[slot][1].bytes = model_bytes;
            car_model_assets[slot][1].capacity = size;
            root_offset = table_offset + 12;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&src_offset, work_bytes + root_offset, sizeof(src_offset));
            memcpy(car_model_assets[slot][1].bytes, work_bytes + src_offset, size);
            {
                uint32 groups_offset = ((const MODEL_HEADER *)model_bytes)->stream_offset;
                uint32 groups_bytes = ((const MODEL_HEADER *)model_bytes)->packet_offsets[0] - groups_offset;
                XASSERT(!(groups_offset > size || groups_bytes > size - groups_offset || size - groups_offset < 4));
                model_reset_prim_colors(model_bytes + groups_offset);
            }
        }
        for (idx = 0; idx < 7; ++idx)
        {
            tex_name = car_asset_names[38 + idx];
            name_len = 0;
            while (tex_name[name_len] != 0)
                ++name_len;
            strcpy(model_tex_name, tex_name);
            model_tex_name[0] = (char)(uint8)(tex_number + 49);
            tex_carrier = tex_find(model_tex_name);
            tpage = (memcpy(&wire_halfword_7, tex_carrier + 8, sizeof(wire_halfword_7)), wire_halfword_7);
            clut = (memcpy(&wire_halfword_8, tex_carrier + 10, sizeof(wire_halfword_8)), wire_halfword_8);
            u = tex_carrier[14];
            v = tex_carrier[15];
            pal_y = (clut >> 6) & 255;
            root_offset = table_offset + 56;
            XASSERT(!(root_offset > work_size || work_size - root_offset < 4));
            memcpy(&tim, work_bytes + root_offset, sizeof(tim));
            x = ((tpage << 6) & 0x3FF) | (u >> 2);
            y = (((tpage << 4) & 0x100) + ((tpage >> 2) & 0x200)) | v;
            tim_offset = tim;
            XASSERT(!(tim_offset > work_size));
            tex_upload_tim(work_bytes + tim_offset, work_size - tim_offset, (sint16)x, (sint16)y, (sint16)((clut & 63) << 4), (sint16)pal_y);
            tex_name = car_asset_names[38 + idx];
            table_offset += 4;
            {
                const MODEL_RESOURCE *owner = &car_model_assets[slot][primary ? 1 : 0];
                MODEL_RESOURCE tex_model;
                XASSERT(!(owner->bytes == NULL || owner->capacity < 60));
                tex_model = *owner;
                model_replace_tex_region(&tex_model, tex_name, model_tex_name);
            }
            if (!primary)
            {
                tex_name = car_asset_names[38 + idx];
                {
                    const MODEL_RESOURCE *owner = &car_model_assets[slot][1];
                    MODEL_RESOURCE tex_model;
                    XASSERT(!(owner->bytes == NULL || owner->capacity < 60));
                    tex_model = *owner;
                    model_replace_tex_region(&tex_model, tex_name, model_tex_name);
                }
            }
        }
        ++tex_number;
        if (primary)
        {
        free_primary:
            free(compressed_bytes);
        }
        else
        {
            free(compressed_bytes);
            DrawSync(0);
        }
    next_car:
        ++slot;
    }
}

void car_init_lighting_zones(void)
{
    const SCENE_LEVEL_DESCRIPTOR *level_desc;

    uint32 val, offset, end;

    FUNCTION_MARKER(0x80030D48, "SLUS_006.10");
    level_desc = &scene_level_image.desc;
    val = level_desc->lighting_offset;
    offset = val;
    XASSERT(!(offset > scene_level_image.size || scene_level_image.size - offset < 32));
    car_lighting_zones.bytes = scene_level_image.bytes;
    car_lighting_zones.size = scene_level_image.size;
    car_lighting_zones.offset = offset;
    end = level_desc->auxiliary_2_offset;
    XASSERT(!(end > scene_level_image.size || end < offset || end - offset < 32 || (end - offset) % 32 != 0));
    car_lighting_zones.count = (end - offset) / 32;
}

static void blend_lighting_end(CAR_STATE *car, const uint8 *desc, sint32 width)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;
    uint16 wire_halfword_13, wire_halfword_14, wire_halfword_15, wire_halfword_16;
    uint16 wire_halfword_17, wire_halfword_18;

    VEC3_16 light_dir;
    SVECTOR n;
    const uint8 *rec, *first, *last, *pool, *neighbor;
    uint32 node_idx, first_idx, last_idx;
    uint32 val, other, origin, product, near_weight, far_weight;
    uint32 color[3], light[3], i;
    sint32 distance, coord;
    sint16 dx, dz;

    FUNCTION_MARKER(0x8003110C, "SLUS_006.10");
    val = (sint16)(memcpy(&wire_halfword_1, desc + 2, sizeof(wire_halfword_1)), wire_halfword_1);
    pool = track_geometry.node_bytes;
    node_idx = val + 1;
    XASSERT(!(node_idx >= track_geometry.node_count));
    rec = pool + node_idx * 24;
    last_idx = (memcpy(&wire_halfword_2, rec + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    first_idx = (memcpy(&wire_halfword_3, rec + 4, sizeof(wire_halfword_3)), wire_halfword_3);
    pool = track_geometry.vertex_bytes;
    XASSERT(!(last_idx == 0 || last_idx - 1 >= track_geometry.vertex_count || first_idx >= track_geometry.vertex_count));
    last = pool + (last_idx - 1) * 6;
    first = pool + first_idx * 6;
    val = (memcpy(&wire_halfword_4, first + 4, sizeof(wire_halfword_4)), wire_halfword_4);
    other = (memcpy(&wire_halfword_5, last + 4, sizeof(wire_halfword_5)), wire_halfword_5);
    n.vy = 0;
    n.vx = (sint16)(val - other);
    val = (memcpy(&wire_halfword_6, last, sizeof(wire_halfword_6)), wire_halfword_6);
    other = (memcpy(&wire_halfword_7, first, sizeof(wire_halfword_7)), wire_halfword_7);
    n.vz = (sint16)(val - other);
    n.pad = 0;
    VectorNormalSS(&n, &n);
    neighbor = desc == car_lighting_zones.bytes + car_lighting_zones.offset + (car_lighting_zones.count - 1) * 32 ? car_lighting_zones.bytes + car_lighting_zones.offset : desc + 32;
    coord = car->pos.x;
    val = (memcpy(&wire_halfword_8, last, sizeof(wire_halfword_8)), wire_halfword_8);
    memcpy(&origin, rec + 12, sizeof(origin));
    dx = ((uint32)(coord / 256) - val - origin);
    coord = car->pos.z;
    product = (uint32)(sint32)n.vx * (uint32)(sint32)dx;
    val = (memcpy(&wire_halfword_9, last + 4, sizeof(wire_halfword_9)), wire_halfword_9);
    memcpy(&origin, rec + 20, sizeof(origin));
    dz = ((uint32)(coord / 256) - val - origin);
    product += (uint32)(sint32)n.vz * (uint32)(sint32)dz;
    distance = (sint32)product / 4096;
    if (distance < 0)
        distance = 0;
    if (width < distance)
        distance = width;
    near_weight = distance;
    far_weight = (uint32)width - near_weight;
    for (i = 0; i < 3; ++i)
    {
        val = desc[16 + i];
        other = neighbor[16 + i];
        product = val * near_weight + other * far_weight;
        color[i] = (uint32)fixed_divide_checked((sint32)product, width);
    }
    model_set_back_color((sint32)color[0], (sint32)color[1], (sint32)color[2]);
    for (i = 0; i < 3; ++i)
    {
        val = neighbor[12 + i];
        product = val * far_weight;
        light[i] = (uint32)fixed_divide_checked((sint32)product, width);
    }
    light_dir.bits.x = (memcpy(&wire_halfword_10, neighbor + 4, sizeof(wire_halfword_10)), wire_halfword_10);
    light_dir.bits.y = (memcpy(&wire_halfword_11, neighbor + 6, sizeof(wire_halfword_11)), wire_halfword_11);
    light_dir.bits.z = (memcpy(&wire_halfword_12, neighbor + 8, sizeof(wire_halfword_12)), wire_halfword_12);
    model_set_light(0, &light_dir, light[0], light[1], light[2]);
    light_dir.bits.x = (memcpy(&wire_halfword_13, desc + 4, sizeof(wire_halfword_13)), wire_halfword_13);
    light_dir.bits.y = (memcpy(&wire_halfword_14, desc + 6, sizeof(wire_halfword_14)), wire_halfword_14);
    light_dir.bits.z = (memcpy(&wire_halfword_15, desc + 8, sizeof(wire_halfword_15)), wire_halfword_15);
    model_set_light(1, &light_dir, 0, 0, 0);
    light_dir.bits.x = (memcpy(&wire_halfword_16, desc + 4, sizeof(wire_halfword_16)), wire_halfword_16);
    light_dir.bits.y = (memcpy(&wire_halfword_17, desc + 6, sizeof(wire_halfword_17)), wire_halfword_17);
    light_dir.bits.z = (memcpy(&wire_halfword_18, desc + 8, sizeof(wire_halfword_18)), wire_halfword_18);
    model_set_light(2, &light_dir, 0, 0, 0);
    model_apply_light_matrix(&car->transform);
}

static void blend_lighting_start(CAR_STATE *car, const uint8 *desc, sint32 width)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;
    uint16 wire_halfword_13, wire_halfword_14, wire_halfword_15, wire_halfword_16;
    uint16 wire_halfword_17, wire_halfword_18;

    VEC3_16 light_dir;
    SVECTOR n;
    const uint8 *rec, *first, *last, *pool, *neighbor;
    uint32 node_idx, first_idx, last_idx;
    uint32 val, other, origin, product, near_weight, far_weight;
    uint32 color[3], light[3], i;
    sint32 distance, coord;
    sint16 dx, dz;

    FUNCTION_MARKER(0x80030D68, "SLUS_006.10");
    val = (sint16)(memcpy(&wire_halfword_1, desc, sizeof(wire_halfword_1)), wire_halfword_1);
    pool = track_geometry.node_bytes;
    node_idx = val;
    XASSERT(!(node_idx >= track_geometry.node_count));
    rec = pool + node_idx * 24;
    last_idx = (memcpy(&wire_halfword_2, rec + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    first_idx = (memcpy(&wire_halfword_3, rec + 4, sizeof(wire_halfword_3)), wire_halfword_3);
    pool = track_geometry.vertex_bytes;
    XASSERT(!(last_idx == 0 || last_idx - 1 >= track_geometry.vertex_count || first_idx >= track_geometry.vertex_count));
    last = pool + (last_idx - 1) * 6;
    first = pool + first_idx * 6;
    val = (memcpy(&wire_halfword_4, first + 4, sizeof(wire_halfword_4)), wire_halfword_4);
    other = (memcpy(&wire_halfword_5, last + 4, sizeof(wire_halfword_5)), wire_halfword_5);
    n.vy = 0;
    n.vx = (sint16)(val - other);
    val = (memcpy(&wire_halfword_6, last, sizeof(wire_halfword_6)), wire_halfword_6);
    other = (memcpy(&wire_halfword_7, first, sizeof(wire_halfword_7)), wire_halfword_7);
    n.vz = (sint16)(val - other);
    n.pad = 0;
    VectorNormalSS(&n, &n);
    neighbor = desc == car_lighting_zones.bytes + car_lighting_zones.offset ? desc + (car_lighting_zones.count - 1) * 32 : desc - 32;
    coord = car->pos.x;
    val = (memcpy(&wire_halfword_8, last, sizeof(wire_halfword_8)), wire_halfword_8);
    memcpy(&origin, rec + 12, sizeof(origin));
    dx = ((uint32)(coord / 256) - val - origin);
    coord = car->pos.z;
    product = (uint32)(sint32)n.vx * (uint32)(sint32)dx;
    val = (memcpy(&wire_halfword_9, last + 4, sizeof(wire_halfword_9)), wire_halfword_9);
    memcpy(&origin, rec + 20, sizeof(origin));
    dz = ((uint32)(coord / 256) - val - origin);
    product += (uint32)(sint32)n.vz * (uint32)(sint32)dz;
    distance = (sint32)(0 - product) / 4096;
    if (distance < 0)
        distance = 0;
    if (width < distance)
        distance = width;
    near_weight = distance;
    far_weight = (uint32)width - near_weight;
    for (i = 0; i < 3; ++i)
    {
        val = desc[16 + i];
        other = neighbor[16 + i];
        product = val * near_weight + other * far_weight;
        color[i] = (uint32)fixed_divide_checked((sint32)product, width);
    }
    model_set_back_color((sint32)color[0], (sint32)color[1], (sint32)color[2]);
    for (i = 0; i < 3; ++i)
    {
        val = neighbor[12 + i];
        product = val * far_weight;
        light[i] = (uint32)fixed_divide_checked((sint32)product, width);
    }
    light_dir.bits.x = (memcpy(&wire_halfword_10, neighbor + 4, sizeof(wire_halfword_10)), wire_halfword_10);
    light_dir.bits.y = (memcpy(&wire_halfword_11, neighbor + 6, sizeof(wire_halfword_11)), wire_halfword_11);
    light_dir.bits.z = (memcpy(&wire_halfword_12, neighbor + 8, sizeof(wire_halfword_12)), wire_halfword_12);
    model_set_light(0, &light_dir, light[0], light[1], light[2]);
    light_dir.bits.x = (memcpy(&wire_halfword_13, desc + 4, sizeof(wire_halfword_13)), wire_halfword_13);
    light_dir.bits.y = (memcpy(&wire_halfword_14, desc + 6, sizeof(wire_halfword_14)), wire_halfword_14);
    light_dir.bits.z = (memcpy(&wire_halfword_15, desc + 8, sizeof(wire_halfword_15)), wire_halfword_15);
    model_set_light(1, &light_dir, 0, 0, 0);
    light_dir.bits.x = (memcpy(&wire_halfword_16, desc + 4, sizeof(wire_halfword_16)), wire_halfword_16);
    light_dir.bits.y = (memcpy(&wire_halfword_17, desc + 6, sizeof(wire_halfword_17)), wire_halfword_17);
    light_dir.bits.z = (memcpy(&wire_halfword_18, desc + 8, sizeof(wire_halfword_18)), wire_halfword_18);
    model_set_light(2, &light_dir, 0, 0, 0);
    model_apply_light_matrix(&car->transform);
}

sint32 car_heading_from_components(sint32 x, sint32 z)
{
    sint32 val0;
    sint32 val1;
    uint32 val;

    FUNCTION_MARKER(0x8004CF8C, "SLUS_006.10");
    if (x >= 0)
    {
        if (z >= 0)
            return fixed_angle_from_vector(x, z);
        if (x > 0)
        {
            val0 = (0 - (uint32)z);
            val = (uint32)fixed_angle_from_vector(val0, x);
            return (val + 1024);
        }
    }
    if (z < 0)
    {
        val0 = x < 0 ? (sint32)(0 - (uint32)x) : x;
        val1 = (0 - (uint32)z);
        val = (uint32)fixed_angle_from_vector(val0, val1);
        return (val + 2048);
    }
    val1 = x < 0 ? (sint32)(0 - (uint32)x) : x;
    val = (uint32)fixed_angle_from_vector(z, val1);
    return (val + 3072);
}

static sint32 track_heading(const TRACK_LOCATION *location, sint32 fallback_lane, sint32 fallback_y)
{
    uint16 wire_halfword_1, wire_halfword_2, wire_halfword_3, wire_halfword_4;
    uint16 wire_halfword_5, wire_halfword_6, wire_halfword_7, wire_halfword_8;
    uint16 wire_halfword_9, wire_halfword_10, wire_halfword_11, wire_halfword_12;
    uint16 wire_halfword_13, wire_halfword_14, wire_halfword_15, wire_halfword_16;
    uint16 wire_halfword_17, wire_halfword_18, wire_halfword_19, wire_halfword_20;
    uint16 wire_halfword_21, wire_halfword_22, wire_halfword_23, wire_halfword_24;
    uint16 wire_halfword_25, wire_halfword_26;

    sint32 node;
    const uint8 *desc;
    uint32 left_idx;
    uint32 right_idx;
    const uint8 *points;
    const uint8 *left_points;
    const uint8 *right_points;
    uint32 type;
    sint32 a;
    sint32 b;
    sint32 c;
    sint32 d;
    sint32 x;
    sint32 z;

    FUNCTION_MARKER(0x8004D030, "SLUS_006.10");
    node = (sint16)location->node;
    XASSERT(!(node < 0 || (uint32)node >= track_geometry.node_count));
    desc = track_geometry.node_bytes + (uint32)node * 24;
    left_idx = (memcpy(&wire_halfword_1, desc + 4, sizeof(wire_halfword_1)), wire_halfword_1);
    points = track_geometry.vertex_bytes;
    right_idx = (memcpy(&wire_halfword_2, desc + 6, sizeof(wire_halfword_2)), wire_halfword_2);
    type = desc[0];
    switch (type)
    {
        case 1:
        case 2:
        case 5:
            XASSERT(!(left_idx + 2 > track_geometry.vertex_count || right_idx + 2 > track_geometry.vertex_count));
            left_points = points + left_idx * 6;
            right_points = points + right_idx * 6;
            a = (sint16)(memcpy(&wire_halfword_3, left_points, sizeof(wire_halfword_3)), wire_halfword_3);
            b = (sint16)(memcpy(&wire_halfword_4, right_points, sizeof(wire_halfword_4)), wire_halfword_4);
            c = (sint16)(memcpy(&wire_halfword_5, left_points + 6, sizeof(wire_halfword_5)), wire_halfword_5);
            d = (sint16)(memcpy(&wire_halfword_6, right_points + 6, sizeof(wire_halfword_6)), wire_halfword_6);
            x = a - b + c - d;
            a = (sint16)(memcpy(&wire_halfword_7, left_points + 4, sizeof(wire_halfword_7)), wire_halfword_7);
            b = (sint16)(memcpy(&wire_halfword_8, right_points + 4, sizeof(wire_halfword_8)), wire_halfword_8);
            c = (sint16)(memcpy(&wire_halfword_9, left_points + 10, sizeof(wire_halfword_9)), wire_halfword_9);
            d = (sint16)(memcpy(&wire_halfword_10, right_points + 10, sizeof(wire_halfword_10)), wire_halfword_10);
            z = a - b + c - d;
            break;
        case 3:
        case 4:
            XASSERT(!(left_idx + 2 > track_geometry.vertex_count || right_idx + 3 > track_geometry.vertex_count));
            left_points = points + left_idx * 6;
            right_points = points + right_idx * 6;
            a = (sint16)(memcpy(&wire_halfword_11, left_points, sizeof(wire_halfword_11)), wire_halfword_11);
            b = (sint16)(memcpy(&wire_halfword_12, right_points + 6, sizeof(wire_halfword_12)), wire_halfword_12);
            c = (sint16)(memcpy(&wire_halfword_13, left_points + 6, sizeof(wire_halfword_13)), wire_halfword_13);
            d = (sint16)(memcpy(&wire_halfword_14, right_points + 12, sizeof(wire_halfword_14)), wire_halfword_14);
            x = a - b + c - d;
            a = (sint16)(memcpy(&wire_halfword_15, left_points + 4, sizeof(wire_halfword_15)), wire_halfword_15);
            b = (sint16)(memcpy(&wire_halfword_16, right_points + 10, sizeof(wire_halfword_16)), wire_halfword_16);
            c = (sint16)(memcpy(&wire_halfword_17, left_points + 10, sizeof(wire_halfword_17)), wire_halfword_17);
            d = (sint16)(memcpy(&wire_halfword_18, right_points + 16, sizeof(wire_halfword_18)), wire_halfword_18);
            z = a - b + c - d;
            break;
        case 6:
        case 7:
            XASSERT(!(left_idx + 3 > track_geometry.vertex_count || right_idx + 2 > track_geometry.vertex_count));
            left_points = points + left_idx * 6;
            right_points = points + right_idx * 6;
            a = (sint16)(memcpy(&wire_halfword_19, left_points + 6, sizeof(wire_halfword_19)), wire_halfword_19);
            b = (sint16)(memcpy(&wire_halfword_20, right_points, sizeof(wire_halfword_20)), wire_halfword_20);
            c = (sint16)(memcpy(&wire_halfword_21, left_points + 12, sizeof(wire_halfword_21)), wire_halfword_21);
            d = (sint16)(memcpy(&wire_halfword_22, right_points + 6, sizeof(wire_halfword_22)), wire_halfword_22);
            x = a - b + c - d;
            a = (sint16)(memcpy(&wire_halfword_23, left_points + 10, sizeof(wire_halfword_23)), wire_halfword_23);
            b = (sint16)(memcpy(&wire_halfword_24, right_points + 4, sizeof(wire_halfword_24)), wire_halfword_24);
            c = (sint16)(memcpy(&wire_halfword_25, left_points + 16, sizeof(wire_halfword_25)), wire_halfword_25);
            d = (sint16)(memcpy(&wire_halfword_26, right_points + 10, sizeof(wire_halfword_26)), wire_halfword_26);
            z = a - b + c - d;
            break;
        default:
            return car_heading_from_components(fallback_lane, fallback_y);
    }
    return car_heading_from_components(x / 4, z / 4);
}

uint32 car_place_on_track(uint32 slot, uint32 node, uint32 side, uint32 raised)
{
    uint32 offset = slot * 864;
    CAR_STATE *car = &car_states[(offset) / 864u];
    uint32 heading;
    TRACK_POSITION_RESULT pos;

    FUNCTION_MARKER(0x8004D984, "SLUS_006.10");
    hd_car_reset(slot);
    car_states[(offset) / 864u].track_location.node = (uint16)(node);
    car_states[(offset) / 864u].track_location.lane = (uint8)(side);
    pos = track_place_location(&car_states[(offset) / 864u].track_location, &car_states[(offset) / 864u].pos);
    heading = ((uint32)track_heading(&car->track_location, pos.lane, pos.right_vertex_y) + 2048) << 8;
    car_states[(offset) / 864u].orientation.bits.y = (uint32)(heading);
    if (raised != 0)
        car_states[(offset) / 864u].orientation.bits.y = (uint32)(heading + 0x80000);
    car_reset_grounding(car);
    return race_refresh_ai_route_offset(slot);
}
