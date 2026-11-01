#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "psx_gpu.h"
#include "car_selection.h"
#include "game.h"

typedef char CAR_SELECTION_G4_SIZE_CHECK[sizeof(POLY_G4) == 36 ? 1 : -1];

typedef char CAR_SELECTION_LINE_SIZE_CHECK[sizeof(LINE_F2) == 16 ? 1 : -1];

static char *const tuning_endpoints[8] = {
    "-", "+", "-", "+", "-", "+", "R", "F",
};

static char *const tuning_headings[4] = {
    "ACCELERATION",
    "TOP SPEED",
    "HANDLING",
    "BRAKE BIAS",
};

static const char *const specification_labels[2][8] = {
    {"LAYOUT\r", "ENGINE\r", "DISPLACEMENT CI/CC\r", "COMPRESSION RATIO\r", "BHP @ RPM\r", "TORQUE @ RPM\r", "TRANSMISSION\r", "TIRES\r"},
    {"APPROX PRICE\r", "0-60 MPH\r", "0-100 MPH\r", "QUARTER MILE\r", "TOP SPEED\r", "BRAKING FROM 60MPH\r", "LB PER BHP\r", "LATERAL ACCEL\r"},
};

static const char *const specification_vals[37][2][8] = {
    {
        {"FRONT/REAR", "ALUMINUM V10", "488/7998", "9.6:1", "450@5200 RPM", "490@3600 RPM", "6 SPEED MANUAL", "275/40ZR17 - 335/35ZR17"},
        {"$73,500", "4.4", "9.3", "12.3@114 MPH", "186 MPH", "110 FEET", "7.3", ".99G"},
    },
    {
        {"FRONT/REAR", "ALUMINUM BLOCK V8", "345/5655", "10.0:1", "345@5600 RPM", "350@4400 RPM", "6 SPEED MANUAL", "245/35ZR16 - 275/35ZR16"},
        {"$37,500", "4.9", "10.4", "13.2@109 MPH", "178 MPH", "108 FEET", "9.5", ".89G"},
    },
    {
        {"FRONT/REAR", "SUPERCHARGED 32 VALVE V8", "331/5340", "8.2:1", "550@6500 RPM", "550@4000 RPM", "6 SPEED MANUAL", "285/45 ZR18 - 285/45 ZR18 "},
        {"$350,000", "4.6", "10.0", "12.9@114.8 MPH", "186 MPH", "UNKNOWN", "7.9", "UNKNOWN"},
    },
    {
        {"FRONT/REAR", "ALUMINUM BLOCK V8", "255/4179", "10.5:1", "360@6500 RPM", "320@4500 RPM", "6 SPEED MANUAL", "245/35ZR15 - 245/40ZR15"},
        {"$55,000", "4.2", "9.9", "13.7@108MPH", "185MPH", "102 FEET", "8.7", ".97G"},
    },
    {
        {"FRONT/REAR", "SUPERCHARGED V8", "244/3996", "9.0:1", "370@6150 RPM", "387@3600 RPM", "5 SPEED AUTOMATIC", "245/45ZR-18 - 255/45ZR-18"},
        {"$80,000", "4.9", "12.0", "13.5@106MPH", "178MPH", "117 FEET", "9.8", ".91G"},
    },
    {
        {"FRONT/REAR", "SUPERCHARGED V8", "351/5753", "8.2:1", "495@5700 RPM", "490@3500 RPM", "6 SPEED MANUAL", "245/40ZR-18 - 245/40ZR-18"},
        {"$45,000", "4.6", "10.3", "12.7@122.0 MPH", "172MPH", "110 FEET", "8.4", ".94G"},
    },
    {
        {"FRONT/REAR", "ALUMINUM V8", "346/5671", "10.1:1", "330@5200 RPM", "345@4400 RPM", "6 SPEED MANUAL", "P245/45ZR17 - P245/45ZR17 "},
        {"$38,000", "4.8", "11.8", "13.4@108.3 MPH", "174 MPH", "120 FEET", "8.7", ".92G"},
    },
    {
        {"FRONT/4-WHEEL", "DOHC TWIN TURBO", "157/2568", "8.5:1", "290@6800 RPM", "260@4500 RPM", "5 SPEED MANUAL", "245/45ZR-17 - 245/45ZR-17 "},
        {"$45,000", "5.2", "13.1", "13.4@102 MPH", "172 MPH", "106 FEET", "11.7", ".88G"},
    },
    {
        {"FRONT/4-WHEEL", "V8", "157/2568", "8.5:1", "290@6800 RPM", "260@4500 RPM", "4 SPEED MANUAL", "245/45ZR-17"},
        {"$200,000", "2.8", "7.3", "7.3@110 MPH", "153MPH", "200 FEET", "11.7", ".80G"},
    },
    {
        {"FRONT/REAR", "FORD IRON BLOCK", "427/6998", "11.0:1", "490@6500 RPM", "510@3700 RPM", "4 SPEED MANUAL", "275/40ZR17 - 335/40ZR17"},
        {"$500,000", "3.5", "8.7", "11.8@119 MPH", "174 MPH", "122 FEET", "5.0", "1.0G"},
    },
    {
        {"FRONT/REAR", "PONTIAC IRON BLOCK", "455/7457", "11.0:1", "525@5200 RPM", "550@4200 RPM", "4 SPEED MANUAL", "275/50ZR17 - 275/50ZR17"},
        {"$27,000", "4.7", "9.4", "12.4@117 MPH", "188 MPH", "145 FEET", "6.2", ".91G"},
    },
    {
        {"FRONT/REAR", "FORD IRON BLOCK", "428/7015", "12.0:1", "510@5800 RPM", "545@3700 RPM", "4 SPEED MANUAL", "245/50ZR17 - 245/50ZR17"},
        {"$120,000", "4.8", "9.1", "12.1@121 MPH", "192 MPH", "137 FEET", "5.8", ".98G"},
    },
    {
        {"FRONT/REAR", "ALUMINUM BLOCK V8", "427/6998", "12.5:1", "550@5000 RPM", "550@4400 RPM", "MUNCIE M-22", "275/40ZR17 - 335/50ZR15"},
        {"$500,000", "4.2", "9.0", "11.3@122 MPH", "202 MPH", "125 FEET", "5.0", "1.0G"},
    },
    {
        {"FRONT/REAR", "ALUMINUM BLOCK V8", "427/6998", "12.5:1", "550@5500 RPM", "550@4400 RPM", "MUNCIE M-22", "275/40ZR17 - 335/50ZR15"},
        {"$500,000", "4.2", "9.2", "11.2@121 MPH", "205 MPH", "127 FEET", "5.1", ".99G"},
    },
    {
        {"FRONT/REAR", "IRON BLOCK V8", "454/7441", "11.0:1", "500@6500 RPM", "500@4800 RPM", "MUNCIE M-22", "245/60ZR15 - 245/60ZR15"},
        {"$225,000", "4.4", "9.6", "11.5@116 MPH", "186 MPH", "150 FEET", "5.9", ".98G"},
    },
    {
        {"FRONT/REAR", "IRON BLOCK V8 HEMI", "426/6982", "11.0:1", "425@5000 RPM", "490@4000 RPM", "4 SPEED MANUAL", "245/60ZR15 - 245/60ZR15"},
        {"$125,000", "5.0", "11.0", "13.4@109 MPH", "175 MPH", "148 FEET", "7.76", ".88G"},
    },
    {
        {"FRONT/REAR", "IRON BLOCK V8 HEMI", "426/6982", "11.0:1", "425@5000 RPM", "490@4000 RPM", "4 SPEED MANUAL", "245/60ZR15 - 245/60ZR15"},
        {"$110,000", "5.2", "11.3", "13.6@107.8 MPH", "178 MPH", "144 FEET", "7.59", ".87G"},
    },
    {
        {"FRONT/REAR", "48 VALVE V12", "366/6000 ", "10.5:1", "450@6800 RPM", "400@5000 RPM", "6 SPEED MANUAL", "255/40ZR19 - 285/40ZR19"},
        {"UNKNOWN", "4.0", "9.0", "10.0@124 MPH", "200 MPH", "UNKNOWN", "7.6", "UNKNOWN"},
    },
    {
        {"FRONT/REAR", "V8", "244/4000", "UNKNOWN", "350", "UNKNOWN", "6 SPEED MANUAL", "245/35ZR18 - 335/30ZR18"},
        {"$90,000", "4.7", "9.9", "12.8@113 MPH", "170 MPH", "105 FEET", "6.6", "0.99G"},
    },
    {
        {"FRONT/REAR", "V12", "472/7730", "12.5:1", "820@7000 RPM", "650@5750 RPM", "JERICO FOUR SPEED", "280/650R18 - 300/710R 18"},
        {"$246,000", "2.97", "6.1", "9.7@129 MPH", "235+ MPH", "UNKNOWN", "2.6", "2.1G"},
    },
    {
        {"FRONT/REAR", "90 DEGREE ALUMINUM V10", "488/7998", "12.0:1", "728@7200 RPM", "680@4800 RPM", "6 SPEED MANUAL", "275/40ZR17 - 335/35ZR17"},
        {"$350,000", "3.2", "7.5", "10.4@128 MPH", "225 MPH", "110 FEET", "3.6", "1.1G"},
    },
    {
        {"MID/REAR", "ALUMINUM BLOCK V12", "288/4720", "11.0:1", "549@7000 RPM", "475@4400 RPM", "6 SPEED MANUAL", "275/40ZR17 - 335/35ZR17"},
        {"$800,000", "3.8", "8.8", "11.2@120 MPH", "218 MPH", "117 FEET", "4.5", "1.0G"},
    },
    {
        {"FRONT/REAR", "4 CYLINDER", "110/1796", "UNKNOWN", "187@7000 RPM", "155@5750 RPM", "6 SPEED MANUAL", "UNKNOWN"},
        {"$40,300", "3.7", "9.8", "12.4@106 MPH", "147 MPH", "106 FEET", "6.5", "1.0G"},
    },
    {
        {"FRONT/FRONT", "SUPERCHARGED V6", "171/2800", "UNKNOWN", "450@6700 RPM", "406@4200 RPM", "6 SPEED MANUAL", "235/40ZR17 - 235/40ZR17"},
        {"$50,000", "3.5", "8.0", "10.6@118 MPH", "182 MPH", "110 FEET", "4.6", "1.02G"},
    },
    {
        {"FRONT/REAR", "4 CYLINDER", "110/1796", "UNKNOWN", "187@7000 RPM", "155@5750 RPM", "6 SPEED MANUAL", "UNKNOWN"},
        {"$49,500", "4.5", "10.8", "13.2@104 MPH", "140 MPH", "106 FEET", "6.5", ".97G"},
    },
    {
        {"FRONT/REAR", "ALUMINUM BLOCK V8", "255/4179", "11.0:1", "440@6500 RPM", "360@4500 RPM", "6 SPEED MANUAL", "245/35ZR15 - 245/40ZR15"},
        {"$55,000", "3.9", "9.1", "13.0@118MPH", "198MPH", "102 FEET", "8.7", ".97G"},
    },
    {
        {"FRONT/REAR", "IRON BLOCK V8", "440/7212", "11.5:1", "525@6300 RPM", "550@4400 RPM", "4 SPEED MANUAL", "245/60ZR17 - 245/60ZR17"},
        {"$350,000", "4.5", "11.0", "12.7@119 MPH", "201 MPH", "128 FEET", "6.9", ".92G"},
    },
    {
        {"FRONT/REAR", "IRON BLOCK V8 WITH BLOWER", "454/7441", "12.0:1", "750@5600 RPM", "825@3700 RPM", "4 SPEED MANUAL", "275/50ZR15 - 325/45ZR15"},
        {"$48,000", "3.2", "6.6", "10.0@129 MPH", "237 MPH", "123 FEET", "3.4", "1.0G"},
    },
    {
        {"FRONT/REAR", "IRON BLOCK V8 WITH BLOWER", "429/7031", "11.5:1", "755@6000 RPM", "800@3600 RPM", "4 SPEED MANUAL", "275/50ZR15 - 335/50ZR15"},
        {"$52,000", "3.4", "7.0", "10.34@127 MPH", "211 MPH", "116 FEET", "3.6", "1.0G"},
    },
    {
        {"MID/ALL", "SUPERCHARGED V10", "330/5408", "9.2:1", "590@7800 RPM", "545@4000 RPM", "6 SPEED MANUAL", "245/45ZR18 - 335/40ZR18"},
        {"$1,000,000", "2.8", "5.9", "9.8@134 MPH", "221 MPH", "99 FEET", "2.9", "1.0G"},
    },
    {
        {"FRONT/REAR", "ALUMINUM BLOCK V8", "345/5655", "10.0:1", "345@5600 RPM", "350@4400 RPM", "6 SPEED MANUAL", "245/35ZR16 - 275/35ZR16"},
        {"$37,500", "5.1", "10.6", "13.4@107 MPH", "175 MPH", "124 FEET", "10.1", ".88G"},
    },
    {
        {"FRONT/REAR", "DOHC V8", "281/4600", "UNKNOWN", "305@5800 RPM", "300@4800 RPM", "5 SPEED MANUAL", "UNKNOWN"},
        {"$25335", "5.7", "12.4", "14.2@106 MPH", "157 MPH", "121 FEET", "UNKNOWN", ".88G"},
    },
    {
        {"MID/REAR", "TWIN TURBO V8", "214/3500", "UNKNOWN", "UNKNOWN", "UNKNOWN", "6 SPEED SEQUENTIAL", "UNKNOWN"},
        {"$1,000,000", "3.2", "7.0", "10.0@129 MPH", "230 MPH", "106 FEET", "UNKNOWN", "3G"},
    },
    {
        {"FRONT/REAR", "DOHC V8", "281/4600", "UNKNOWN", "305@5800 RPM", "300@4800 RPM", "5 SPEED MANUAL", "UNKNOWN"},
        {"$25335", "5.7", "12.4", "14.2@106 MPH", "157 MPH", "121 FEET", "UNKNOWN", ".88G"},
    },
    {
        {"FRONT/REAR", "IRON BLOCK V8 HEMI", "426/6982", "11.0:1", "425@5000 RPM", "490@4000 RPM", "4 SPEED MANUAL", "245/60ZR15 - 245/60ZR15"},
        {"$110,000", "5.2", "11.3", "13.6@107.8 MPH", "178 MPH", "144 FEET", "7.59", ".87G"},
    },
    {
        {"FRONT/REAR", "ALUMINUM V8", "346/5100", "10.1:1", "330@5200 RPM", "345@4400 RPM", "6 SPEED MANUAL", "P245/45ZR17 - P245/45ZR17 "},
        {"$38,000", "4.8", "11.8", "13.4@108.3 MPH", "174 MPH", "120 FEET", "8.7", ".92G"},
    },
    {
        {"FRONT/FRONT", "IRON BLOCK V12", "610/9998", "9.0:1", "950@6000 RPM", "800@3500 RPM", "4 SPEED MANUAL", "285/45ZR16 - 285/45ZR16"},
        {"$95,000", "2.2", "5.0", "9.0@150 MPH", "260 MPH", "145 FEET", "2.0", ".90G"},
    },
};

static const char body_tex_name[8] = "Hicar";

static const uint16 base_models[17] = {0, 6, 5, 1, 7, 3, 2, 4, 10, 16, 14, 15, 9, 13, 12, 11, 65535};
const uint16 car_selection_standard_models[14] = {0, 6, 5, 1, 7, 3, 2, 4, 0, 6, 5, 1, 7, 3};
const uint16 car_selection_beast_models[14] = {10, 16, 14, 15, 9, 13, 12, 11, 10, 16, 14, 15, 9, 13};
static const uint16 cop_models[5] = {25, 33, 34, 35, 65535};
static const uint8 model_flags[44] = {3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 1, 1, 0, 1, 2, 3, 1, 3, 0, 0, 1, 3, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0};
static const uint8 cup_bonus_models[5] = {17, 19, 20, 21, 255};

typedef union CAR_SELECTION_MATRIX
{
    MATRIX sdk;
    uint32 words[8];
} CAR_SELECTION_MATRIX;

typedef char CAR_SELECTION_MATRIX_SIZE_CHECK[sizeof(CAR_SELECTION_MATRIX) == 32 ? 1 : -1];

static const CAR_SELECTION_MATRIX preview_matrix_default = {.sdk = {.m = {{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}}}};

CAR_SELECTION_STATE car_selection;

static uint32 preview_bank_storage[0x32000 / 4];

// Temporary heap identity until allocator migration

typedef struct CAR_SELECTION_COMPRESSED_INPUT
{
    uint8 *bytes;
    uint32 capacity;
} CAR_SELECTION_COMPRESSED_INPUT;

static CAR_SELECTION_COMPRESSED_INPUT compressed_input;

static uint32 draw_labels(void);
static uint32 upload_colors(void);
static uint32 car_selection_commit(void);
static uint32 draw_tuning(void);
static uint32 edit_tuning(void);
static uint32 init_tuning_packets(void);
static uint32 draw_preview(void);
static uint32 finish_load(void);
static uint8 *acquire_buffer(uint32 slot);
static uint32 init_wheel_geometry(void);

uint32 car_selection_begin(uint32 mode)
{
    uint32 game_mode, val, idx, offset, count, tuning0, tuning1, tuning2, tuning3, flag;

    FUNCTION_MARKER(0x800B3F24, "SLUS_006.10");
    game_mode = race_session.multiplayer_mode;
    presentation_items[3].active = (0);
    presentation_items[4].active = (0);
    car_selection.banner_wiggle = (0);
    presentation_menu.sel = (0xFFFFFFFF);
    car_selection.input_port = (0);
    if (game_mode != 0)
    {
        val = car_selection.stage;
        if (val == 0)
            car_selection.stage = (1);
        else
        {
            car_selection.stage = (2);
            car_selection.input_port = (1);
        }
    }
    if (race_session.event_mode == 3)
    {
        val = car_selection.stage;
        car_selection.stage = (val == 0 ? 1 : 2);
    }
    car_selection.mode = (mode);
    car_selection_build_list();
    if ((sint32)car_selection.stage < 2)
    {
        audio_stop_xa();
        val = audio_options.music_volume;
        audio_play_request(13, 20, (sint32)val, 0, 0x1400);
    }
    car_selection.intro_progress = (0);
    car_selection.phase = (0);
    car_selection.transition_depth = (0x1000);
    car_selection.vertical_offset = (0);
    free(compressed_input.bytes);
    compressed_input.bytes = NULL;
    compressed_input.capacity = 0;
    car_selection.refresh_frames = (0);
    if (mode == 0)
    {
        car_selection.tuning[0] = (uint16)(0);
        car_selection.disp_tuning[0] = (uint16)(0);
        car_selection.tuning[1] = (uint16)(0);
        car_selection.disp_tuning[1] = (uint16)(0);
        car_selection.tuning[2] = (uint16)(0);
        car_selection.disp_tuning[2] = (uint16)(0);
        car_selection.tuning[3] = (uint16)(0);
        car_selection.disp_tuning[3] = (uint16)(0);
        car_selection.manual_transmission = (0);
    }
    else
    {
        tuning0 = session_car_choices.cars[0].tuning[0][0];
        tuning1 = session_car_choices.cars[0].tuning[0][1];
        tuning2 = session_car_choices.cars[0].tuning[0][2];
        tuning3 = session_car_choices.cars[0].tuning[0][3];
        flag = session_car_choices.cars[0].manual_transmission;
        car_selection.tuning[0] = (uint16)(tuning0);
        car_selection.disp_tuning[0] = (uint16)(tuning0);
        car_selection.tuning[1] = (uint16)(tuning1);
        car_selection.disp_tuning[1] = (uint16)(tuning1);
        car_selection.tuning[2] = (uint16)(tuning2);
        car_selection.disp_tuning[2] = (uint16)(tuning2);
        car_selection.tuning[3] = (uint16)(tuning3);
        car_selection.disp_tuning[3] = (uint16)(tuning3);
        car_selection.manual_transmission = (flag);
    }
    if (race_session.drag_race != 0)
        car_selection.manual_transmission = (1);
    car_init_environment_packets();
    presentation_load_items(&presentation_rec_lists[PRESENTATION_LIST_CAR]);
    presentation_items[0].active = (0);
    if (race_session.cup == 3 && mode != 0)
    {
        presentation_items[7].active = (0);
        presentation_items[8].active = (0);
    }
    car_selection.bank_count = (uint32)(1);
    {
        sint32 operation_ok = !(!gpu_register_packet_range(preview_bank_storage, (uint32)sizeof(preview_bank_storage)));
        XASSERT(operation_ok);
    }
    idx = 0;
    offset = 0;
    do
    {
        (void)(idx);
        car_selection.banks[offset >> 3].model = (uint8)(0xFF);
        count = car_selection.bank_count;
        car_selection.banks[offset >> 3].buffer = (uint8 *)preview_bank_storage;
        ++idx;
        offset += 8;
    } while ((sint32)idx < (sint32)count);
    ;
    for (idx = 0; idx < 44; ++idx)
    {
        offset = idx * 12;
        car_selection.models[idx].state = (uint8)(0);
        car_selection.models[idx].age = (uint16)(0);
        car_selection.models[idx].bank = (uint8)(0xFF);
    }
    return wheels_init_preview();
}

void car_selection_update(uint32 mode)
{
    uint32 angle, sel, idx, count, state, val, height, buttons;
    const char *text;
    uint32 size;
    uint8 *allocation;
    const char *const *filename_slot;
    const char *filename;
    sint32 s, signed_selection;

    FUNCTION_MARKER(0x800B633C, "SLUS_006.10");
    angle = car_selection.banner_angle;
    presentation_items[3].active = (0);
    presentation_items[4].active = (0);
    angle += 128;
    car_selection.banner_angle = (angle);
    s = rsin((sint32)angle);
    car_selection.banner_wiggle = ((uint32)(s / 600));
    finish_load();
    sel = (uint8)car_selection.sel_entry;
    for (idx = 0; idx < 44; ++idx)
    {
        if (idx != sel)
        {
            val = car_selection.models[idx].age;
            car_selection.models[idx].age = (uint16)(val > 0xFFFE ? 0 : val + 1);
        }
    }
    state = car_selection.phase;
    switch (state)
    {
        case 0:
            idx = car_selection.list_idx;
            presentation_items[0].active = (0);
            presentation_items[1].active = (1);
            presentation_items[2].active = (1);
            if (idx == 0)
                presentation_items[1].active = (0);
            count = car_selection.list_count;
            if (idx == count - 1)
                presentation_items[2].active = (0);
            signed_selection = (sint16)car_selection.entries[idx];
            car_selection.transition_depth = (0);
            car_selection.sel_entry = ((uint32)signed_selection);
            draw_preview();
            draw_labels();
            presentation_select_pal(3);
            idx = car_selection.list_idx;
            sel = (uint8)car_selection.entries[idx];
            height = disp_state.pal;
            text = ((uint32)(sel) < 44 ? session_model_names[sel] : "");
            presentation_draw_text(128, 256, (sint32)((height << 3) + 140), text);
            if ((car_selection.sel_entry & 0x100) != 0)
            {
                presentation_select_pal(3);
                height = disp_state.pal;
                text = "(LOCKED)";
                presentation_draw_text(128, 256, (sint32)((height << 3) + 148), text);
            }
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x20) != 0)
            {
                count = car_selection.list_count;
                idx = car_selection.list_idx;
                if (idx < count - 1)
                {
                    car_selection.refresh_frames = (0);
                    car_selection.list_idx = (idx + 1);
                    audio_play_menu_high();
                }
            }
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x80) != 0)
            {
                idx = car_selection.list_idx;
                if (idx != 0)
                {
                    car_selection.refresh_frames = (0);
                    car_selection.list_idx = (idx - 1);
                    audio_play_menu_high();
                }
            }
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x1000) != 0)
            {
                audio_play_menu_accept();
                car_selection.refresh_frames = (0);
                if (mode == 0 || race_session.cup != 3 || race_session.event_mode != 2)
                {
                    car_selection.phase = (13);
                    return;
                }
            }
            idx = car_selection.list_idx;
            signed_selection = (sint16)car_selection.entries[idx];
            car_selection.primary_color = (0);
            car_selection.secondary_color = (0);
            sel = (uint32)signed_selection & 255;
            if (car_selection.models[sel].state == 0)
            {
                car_selection.models[sel].state = (uint8)(1);
                filename_slot = car_asset_names + sel;
                filename = *filename_slot;
                size = archive_aligned_size(filename);
                allocation = (uint8 *)malloc(size ? size : 1);
                XASSERT(!(allocation == NULL));
                filename = *filename_slot;
                compressed_input.bytes = allocation;
                compressed_input.capacity = size;
                car_selection.loading_model = (uint32)(sel);
                compressed_input.capacity = archive_read_file(filename, compressed_input.bytes, size);
                val = car_selection.loading_model;
                car_selection.models[val].state = (uint8)(2);
                stream_clear_slot(0);
                ;
                finish_load();
            }
            car_selection.sel_entry = ((uint32)signed_selection);
            val = car_selection.refresh_frames + 1;
            car_selection.refresh_frames = (val);
            /* Populate both retained text buffers before leaving the refresh phase */
            if (val < 3)
                return;
            if (car_selection.models[sel].state != 0)
            {
                car_selection.phase = (2);
                presentation_items[0].active = (0);
            }
            return;
        case 1:
            draw_preview();
            val = car_selection.transition_depth;
            if ((sint32)val > 0)
                car_selection.transition_depth = (val - 128);
            val = car_selection.intro_progress;
            if ((sint32)val < 0x1100)
                car_selection.intro_progress = (val + 128);
            else
            {
                car_selection.transition_depth = (0);
                car_selection.phase = (0);
                car_selection.retained_transition_code = (4);
            }
            return;
        case 2:
            idx = car_selection.list_idx;
            presentation_items[1].active = (1);
            presentation_items[2].active = (1);
            if (idx == 0)
                presentation_items[1].active = (0);
            count = car_selection.list_count;
            if (idx == count - 1)
                presentation_items[2].active = (0);
            draw_preview();
            sel = (uint8)car_selection.sel_entry;
            if (car_selection.models[sel].state != 3)
                return;
            val = car_selection.transition_depth;
            if ((sint32)val > 0)
            {
                car_selection.transition_depth = (val - 256);
                return;
            }
            val = car_selection.input_port;
            buttons = input_ports[val].pressed;
            car_selection.transition_depth = (0);
            if ((buttons & 0x1000) != 0)
            {
                audio_play_menu_accept();
                if (mode == 0 || race_session.cup != 3 || race_session.event_mode != 2)
                    car_selection.phase = (13);
            }
            if (race_session.two_players == 0 && (race_session.event_mode != 3 || car_selection.stage != 2))
            {
                sel = (uint8)car_selection.sel_entry;
                if (((uint8)model_flags[sel] & 1) != 0)
                {
                    val = car_selection.input_port;
                    if ((input_ports[val].pressed & 0x10) != 0)
                    {
                        val = (car_selection.primary_color + 1) & 3;
                        car_selection.primary_color = (val);
                        car_selection.secondary_color = (val);
                        upload_colors();
                        audio_play_menu_move();
                    }
                    val = car_selection.input_port;
                    if ((input_ports[val].pressed & 0x40) != 0)
                    {
                        val = (car_selection.primary_color - 1) & 3;
                        car_selection.primary_color = (val);
                        car_selection.secondary_color = (val);
                        upload_colors();
                        audio_play_menu_move();
                    }
                }
            }
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x4008) != 0)
            {
                if ((car_selection.sel_entry & 0x100) != 0)
                    audio_play_menu_error();
                else
                {
                    audio_play_menu_accept();
                    val = race_session.drag_race;
                    car_selection.phase = (val != 0 ? 12 : 6);
                }
            }
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x20) != 0)
            {
                count = car_selection.list_count;
                idx = car_selection.list_idx;
                if (idx < count - 1)
                {
                    audio_play_menu_high();
                    idx = car_selection.list_idx + 1;
                    signed_selection = (sint16)car_selection.entries[idx];
                    car_selection.phase = (0);
                    car_selection.refresh_frames = (0);
                    car_selection.list_idx = (idx);
                    car_selection.sel_entry = ((uint32)signed_selection);
                    draw_labels();
                }
            }
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x80) != 0 && car_selection.list_idx != 0)
            {
                audio_play_menu_high();
                idx = car_selection.list_idx - 1;
                signed_selection = (sint16)car_selection.entries[idx];
                car_selection.phase = (0);
                car_selection.refresh_frames = (0);
                car_selection.list_idx = (idx);
                car_selection.sel_entry = ((uint32)signed_selection);
                draw_labels();
            }
            return;
        case 6:
            draw_preview();
            val = car_selection.transition_depth;
            presentation_items[0].active = (0);
            presentation_items[1].active = (0);
            presentation_items[2].active = (0);
            if ((sint32)val < 160)
            {
                height = car_selection.vertical_offset;
                car_selection.transition_depth = (val + 32);
                car_selection.vertical_offset = (height - 2);
            }
            else
            {
                car_selection.transition_depth = (160);
                car_selection.phase = (7);
            }
            return;
        case 7:
            draw_preview();
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x50) != 0)
            {
                audio_play_menu_move();
                car_selection.manual_transmission = (car_selection.manual_transmission ^ 1);
            }
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x1000) != 0)
            {
                car_selection.phase = (8);
                audio_play_menu_accept();
            }
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x4008) == 0)
                return;
            audio_play_menu_accept();
            if (race_session.event_mode == 0)
            {
                car_selection.phase = (9);
                return;
            }
            val = race_session.cup;
            car_selection.phase = (12);
            if (val == 3)
            {
                sel = (uint8)car_selection.sel_entry;
                for (idx = 0; idx < 13; ++idx)
                    if ((sint16)session_cup_models[idx] == (sint32)sel)
                        session_cup_models[idx] = (uint16)(0xFFFE);
            }
            return;
        case 8:
            draw_preview();
            val = car_selection.transition_depth;
            if ((sint32)val > 0)
            {
                height = car_selection.vertical_offset;
                car_selection.transition_depth = (val - 32);
                car_selection.vertical_offset = (height + 2);
            }
            else
            {
                car_selection.vertical_offset = (0);
                car_selection.transition_depth = (0);
                car_selection.phase = (2);
            }
            return;
        case 9:
            draw_preview();
            init_tuning_packets();
            car_selection.tuning_row = (0);
            car_selection.phase = (10);
            return;
        case 10:
            draw_preview();
            draw_tuning();
            edit_tuning();
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x1000) != 0)
            {
                audio_play_menu_accept();
                car_selection.phase = (11);
            }
            val = car_selection.input_port;
            if ((input_ports[val].pressed & 0x4008) != 0)
            {
                audio_play_menu_accept();
                car_selection.phase = (12);
            }
            return;
        case 11:
            draw_preview();
            draw_tuning();
            car_selection.phase = (6);
            return;
        case 12:
        case 13:
            goto release_carousel;
        default:
            return;
    }

release_carousel:
    allocation = compressed_input.bytes;
    if (allocation != NULL)
        free(allocation);
    compressed_input.bytes = NULL;
    compressed_input.capacity = 0;
    if (state == 12)
    {
        car_selection_commit();
        if (mode != 0)
        {
            if (car_selection.stage == 1)
            {
                race_menu.state = (uint32)(10);
                return;
            }
            val = race_session.cup;
            car_selection.stage = (0);
            if (val == 3 && race_session.event_mode == 2)
                goto exit_to_game;
            race_menu_select_default();
            goto exit_to_menu;
        }
        if (car_selection.stage == 1)
        {
            session_push_menu(20);
            return;
        }
        car_selection.stage = (0);
        session_flow.phase = (4);
        presentation_menu.finished = (1);
        return;
    }
    if (mode != 0)
    {
        if (car_selection.stage == 2)
        {
            race_menu.state = (uint32)(10);
            car_selection.stage = (0);
            return;
        }
        if (race_session.cup == 3 && race_session.event_mode == 2)
            goto exit_to_game;
        race_menu_select_default();
        goto exit_to_menu;
    }
    session_pop_menu();
    if (car_selection.stage == 2)
        car_selection.stage = (0);
    if ((sint32)car_selection.stage >= 2)
        return;
    car_selection.stage = (0);
exit_to_menu:
    audio_replay_xa_track((sint32)audio_xa.track);
    audio_stop_voice(20);
    return;
exit_to_game:
    race_control.results_finished = (uint16)(1);
    session_flow.phase = (5);
}

static uint32 draw_labels(void)
{
    char *text;
    const char *label, *val;
    size_t bytes;
    uint32 sel, column, idx, y, x;

    FUNCTION_MARKER(0x800B44F8, "SLUS_006.10");
    idx = car_selection.list_idx;
    sel = (uint8)car_selection.entries[idx];
    presentation_select_pal(0);
    for (column = 0; column < 2; ++column)
    {
        y = (disp_state.pal << 3) + 160;
        presentation_render.tab_stop = column == 0 ? 264 : 482;
        x = column == 0 ? 30 : 280;
        for (idx = 0; idx < 8; ++idx)
        {
            label = specification_labels[column][idx];
            val = sel < 37 ? specification_vals[sel][column][idx] : "";
            bytes = strlen(label) + strlen(val) + 1;
            text = (char *)malloc(bytes);
            XASSERT(!(text == NULL));
            sprintf(text, "%s%s", label, val);
            presentation_draw_text(1, x, (sint32)y, text);
            free(text);
            y += 8;
        }
    }
    return 0;
}

static uint32 upload_colors(void)
{
    uint32 clut, page, idx, image, u, v, image_x, image_y;
    uint32 image_offset;
    const CAR_SELECTION_BUFFER *buffer;
    const uint8 *tex;

    FUNCTION_MARKER(0x800B4CBC, "SLUS_006.10");
    tex = tex_find(body_tex_name);
    clut = xport_load_le16(tex + 10);
    page = xport_load_le16(tex + 8);
    idx = car_selection.primary_color;
    buffer = &car_selection.active_buffer;
    image_offset = (idx << 2) + 16;
    XASSERT(!(image_offset > buffer->capacity || buffer->capacity - image_offset < 4));
    memcpy(&image, buffer->bytes + image_offset, sizeof(image));
    image_x = (page << 6) & 0x3C0;
    image_y = ((page << 4) & 0x100) | ((page >> 2) & 0x200);
    image_offset = image;
    XASSERT(!(image_offset > buffer->capacity));
    tex_upload_tim(buffer->bytes + image_offset, buffer->capacity - image_offset, (sint16)image_x, (sint16)image_y, (sint16)((clut & 63) << 4), (sint16)((clut >> 6) & 255));
    tex = tex_find(wheel_standard_tex_name);
    page = xport_load_le16(tex + 8);
    u = tex[14];
    clut = xport_load_le16(tex + 10);
    v = tex[15];
    idx = car_selection.secondary_color;
    buffer = &car_selection.active_buffer;
    image_offset = (idx << 2) + 36;
    XASSERT(!(image_offset > buffer->capacity || buffer->capacity - image_offset < 4));
    memcpy(&image, buffer->bytes + image_offset, sizeof(image));
    image_x = ((page << 6) & 0x3FF) | (u >> 2);
    image_y = (((page << 4) & 0x100) + ((page >> 2) & 0x200)) | v;
    image_offset = image;
    XASSERT(!(image_offset > buffer->capacity));
    return (uint32)tex_upload_tim(buffer->bytes + image_offset, buffer->capacity - image_offset, (sint16)image_x, (sint16)image_y, (sint16)((clut & 63) << 4), (sint16)((clut >> 6) & 255));
}

static uint32 car_selection_commit(void)
{
    uint32 sel, flag, offset;

    FUNCTION_MARKER(0x800B42F0, "SLUS_006.10");
    sel = car_selection.stage - 1;
    if ((sint32)sel < 0)
        sel = 0;
    else if ((sint32)sel >= 2)
        sel = 1;
    flag = car_selection.manual_transmission;
    offset = sel * 24;
    session_options.retained_model = (uint32)(sel);
    session_car_choices.cars[sel].manual_transmission = (uint8)(flag);
    session_car_choices.cars[sel].model[0] = (uint8)(car_selection.sel_entry);
    session_car_choices.cars[sel].primary_color[0] = (uint8)(car_selection.primary_color);
    session_car_choices.cars[sel].secondary_color[0] = (uint8)(car_selection.secondary_color);
    session_car_choices.cars[sel].tuning[0][0] = (uint16)(car_selection.tuning[0]);
    session_car_choices.cars[sel].tuning[0][1] = (uint16)(car_selection.tuning[1]);
    session_car_choices.cars[sel].tuning[0][2] = (uint16)(car_selection.tuning[2]);
    session_car_choices.cars[sel].tuning[0][3] = (uint16)(car_selection.tuning[3]);
    return offset;
}

static uint32 draw_tuning(void)
{
    DISPLAY_ORDERING head_address;
    uint32 sel, wiggle, row, idx, bank, head, tag;
    POLY_G4 *gradient;
    LINE_F2 *line;
    const char *text;
    uint32 x, y;

    DISPLAY_FRAME *cur_frame;

    FUNCTION_MARKER(0x800B4F78, "SLUS_006.10");
    sel = car_selection.tuning_row;
    wiggle = car_selection.banner_wiggle;
    presentation_items[3].active = (1);
    presentation_items[4].active = (1);
    presentation_items[3].y = (sint16)(sel * 13 + 99);
    presentation_items[4].y = (sint16)(sel * 13 + 99);
    presentation_items[3].x_flags = (99 - wiggle);
    presentation_items[4].x_flags = (wiggle + 413);
    y = 96;
    for (row = 0; row < 4; ++row)
    {
        x = 256 + (uint32)((sint16)car_selection.disp_tuning[row] / 32) - 2;
        bank = disp_state.bank;
        cur_frame = disp_state.cur;
        for (idx = 0; idx < 5; ++idx)
        {
            line = &car_selection.tuning_packets.markers[row * 5 + idx][bank];
            line->p[0].x = (sint16)(uint16)(x);
            line->p[0].y = (sint16)(uint16)(idx == 2 ? y - 1 : y);
            line->p[1].x = (sint16)(uint16)(x);
            line->p[1].y = (sint16)(uint16)(idx == 2 ? y + 4 : y + 3);
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = 0;
            tag = line->tag;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            line->tag = (uint32)((tag & 0xFF000000) | (head & 0xFFFFFF));
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = 0;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            head_address.tags[(sint32)head_address.byte_offset / 4] = (head & 0xFF000000) | gpu_packet_link((uintptr_t)line);
            ++x;
        }
        bank = disp_state.bank;
        cur_frame = disp_state.cur;
        for (idx = 0; idx < 17; ++idx)
        {
            line = &car_selection.tuning_packets.ticks[row * 17 + idx][bank];
            x = 128 + idx * 16;
            line->p[0].x = (sint16)(uint16)(x);
            line->p[0].y = (sint16)(uint16)(y - 1);
            line->p[1].x = (sint16)(uint16)(x);
            line->p[1].y = (sint16)(uint16)(y + 4);
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = 0;
            tag = line->tag;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            line->tag = (uint32)((tag & 0xFF000000) | (head & 0xFFFFFF));
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = 0;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            head_address.tags[(sint32)head_address.byte_offset / 4] = (head & 0xFF000000) | gpu_packet_link((uintptr_t)line);
        }
        bank = disp_state.bank;
        cur_frame = disp_state.cur;
        for (idx = 0; idx < 2; ++idx)
        {
            gradient = &car_selection.tuning_packets.gradients[row * 2 + idx][bank];
            x = idx == 0 ? 128 : 256;
            gradient->p[0].xy.x = (sint16)(uint16)(x);
            gradient->p[0].xy.y = (sint16)(uint16)(y);
            gradient->p[1].xy.x = (sint16)(uint16)(x + 128);
            gradient->p[1].xy.y = (sint16)(uint16)(y);
            gradient->p[2].xy.x = (sint16)(uint16)(x);
            gradient->p[2].xy.y = (sint16)(uint16)(y + 4);
            gradient->p[3].xy.x = (sint16)(uint16)(x + 128);
            gradient->p[3].xy.y = (sint16)(uint16)(y + 4);
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = 0;
            tag = gradient->tag;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            gradient->tag = (uint32)((tag & 0xFF000000) | (head & 0xFFFFFF));
            head_address.tags = cur_frame->ordering;
            head_address.byte_offset = 0;
            head = head_address.tags[(sint32)head_address.byte_offset / 4];
            head_address.tags[(sint32)head_address.byte_offset / 4] = (head & 0xFF000000) | gpu_packet_link((uintptr_t)gradient);
        }
        presentation_select_pal(car_selection.tuning_row == row ? 1 : 0);
        text = tuning_headings[row];
        presentation_draw_text(129, 256, (sint32)(y - 9), text);
        presentation_select_pal(0);
        text = tuning_endpoints[row * 2];
        presentation_draw_text(129, 122, (sint32)(y - 2), text);
        text = tuning_endpoints[row * 2 + 1];
        presentation_draw_text(129, 390, (sint32)(y - 2), text);
        y += 13;
    }
    return 0;
}

static uint32 balance_tuning(uint16 *values)
{
    sint32 tuning0, tuning1, tuning2, sum, half, sel;
    uint32 idx, candidate;
    uint16 *dst, *fallback;

    FUNCTION_MARKER(0x800B539C, "SLUS_006.10");
    tuning0 = (sint16)values[0];
    tuning1 = (sint16)values[1];
    tuning2 = (sint16)values[2];
    sum = tuning0 + tuning1 + tuning2;
    if (sum == 0)
        return (tuning0 + tuning1);
    sel = (sint32)car_selection.tuning_row;
    if (sel < 0)
        return 1;
    if (sel > 2)
        return 2;
    half = sum / 2;
    for (idx = 0; idx < 2; ++idx)
    {
        if (sel == 0)
            dst = values + (idx + 1);
        else if (sel == 1)
            dst = values + idx * 2;
        else
            dst = values + idx;
        candidate = (uint32)(sint16)*dst - (uint32)half;
        if (candidate + 4096 < 8193)
            *dst = (candidate);
        else
        {
            fallback = values + (uint32)sel;
            *fallback = (*fallback - (uint32)half);
        }
    }
    return 0;
}

static uint32 edit_tuning(void)
{
    uint16 values[4];
    uint16 *target, *src;
    uint32 idx, player, sel, val;
    sint32 cur, desired;

    FUNCTION_MARKER(0x800B5524, "SLUS_006.10");
    for (idx = 0; idx < 4; ++idx)
    {
        target = car_selection.tuning + idx;
        src = car_selection.disp_tuning + idx;
        values[idx] = (*target);
        cur = (sint16)*src;
        desired = (sint16)*target;
        if (cur < desired)
            *src = ((uint32)cur + 32);
        cur = (sint16)*src;
        desired = (sint16)*target;
        if (desired < cur)
            *src = ((uint32)cur - 32);
    }
    player = car_selection.input_port;
    if ((input_ports[player].pressed & 0x10) != 0)
    {
        if ((sint32)car_selection.tuning_row > 0)
        {
            audio_play_menu_move();
            car_selection.tuning_row = (car_selection.tuning_row - 1);
        }
    }
    player = car_selection.input_port;
    if ((input_ports[player].pressed & 0x40) != 0)
    {
        if ((sint32)car_selection.tuning_row < 3)
        {
            audio_play_menu_move();
            car_selection.tuning_row = (car_selection.tuning_row + 1);
        }
    }
    player = car_selection.input_port;
    if ((input_ports[player].pressed & 0x80) != 0)
    {
        sel = car_selection.tuning_row;
        if ((sint16)values[sel] >= -4095)
        {
            audio_play_menu_high();
            sel = car_selection.tuning_row;
            target = values + sel;
            *target = (*target - 512);
        }
    }
    player = car_selection.input_port;
    if ((input_ports[player].pressed & 0x20) != 0)
    {
        sel = car_selection.tuning_row;
        if ((sint16)values[sel] < 4096)
        {
            audio_play_menu_high();
            sel = car_selection.tuning_row;
            target = values + sel;
            *target = (*target + 512);
        }
    }
    sel = car_selection.tuning_row;
    target = values + sel;
    cur = (sint16)*target;
    if (cur < -4096)
        cur = -4096;
    else if (cur > 4096)
        cur = 4096;
    *target = (cur);
    balance_tuning(values);
    for (idx = 0; idx < 4; ++idx)
    {
        val = values[idx];
        car_selection.tuning[idx] = (uint16)(val);
    }
    return 0;
}

static uint32 init_tuning_packets(void)
{
    uint32 idx, bank;
    POLY_G4 *gradient;
    LINE_F2 *line;

    FUNCTION_MARKER(0x800B4DB8, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&car_selection.tuning_packets, (uint32)sizeof(car_selection.tuning_packets)));
        XASSERT(operation_ok);
    }
    for (idx = 0; idx < 8; ++idx)
    {
        for (bank = 0; bank < 2; ++bank)
        {
            gradient = &car_selection.tuning_packets.gradients[idx][bank];
            gradient->tag = (gradient->tag & 0xFFFFFF) | ((uint32)(uint8)(8) << 24);
            gradient->p[0].rgbc.code = (uint8)(0x38);
            if ((idx & 1) == 0)
            {
                gradient->p[0].rgbc.r = (uint8)(255);
                gradient->p[0].rgbc.g = (uint8)(0);
                gradient->p[0].rgbc.b = (uint8)(0);
                gradient->p[2].rgbc.r = (uint8)(255);
                gradient->p[2].rgbc.g = (uint8)(0);
                gradient->p[2].rgbc.b = (uint8)(0);
                gradient->p[1].rgbc.r = (uint8)(0);
                gradient->p[1].rgbc.g = (uint8)(255);
                gradient->p[1].rgbc.b = (uint8)(0);
                gradient->p[3].rgbc.r = (uint8)(0);
                gradient->p[3].rgbc.g = (uint8)(255);
                gradient->p[3].rgbc.b = (uint8)(0);
            }
            else
            {
                gradient->p[1].rgbc.r = (uint8)(255);
                gradient->p[1].rgbc.g = (uint8)(0);
                gradient->p[1].rgbc.b = (uint8)(0);
                gradient->p[3].rgbc.r = (uint8)(255);
                gradient->p[3].rgbc.g = (uint8)(0);
                gradient->p[3].rgbc.b = (uint8)(0);
                gradient->p[0].rgbc.r = (uint8)(0);
                gradient->p[0].rgbc.g = (uint8)(255);
                gradient->p[0].rgbc.b = (uint8)(0);
                gradient->p[2].rgbc.r = (uint8)(0);
                gradient->p[2].rgbc.g = (uint8)(255);
                gradient->p[2].rgbc.b = (uint8)(0);
            }
        }
    }
    for (idx = 0; idx < 68; ++idx)
    {
        for (bank = 0; bank < 2; ++bank)
        {
            line = &car_selection.tuning_packets.ticks[idx][bank];
            line->tag = (line->tag & 0xFFFFFF) | ((uint32)(uint8)(3) << 24);
            line->rgbc.code = (uint8)(0x42);
            line->rgbc.r = (uint8)(255);
            line->rgbc.g = (uint8)(255);
            line->rgbc.b = (uint8)(255);
        }
    }
    for (idx = 0; idx < 20; ++idx)
    {
        for (bank = 0; bank < 2; ++bank)
        {
            line = &car_selection.tuning_packets.markers[idx][bank];
            line->tag = (line->tag & 0xFFFFFF) | ((uint32)(uint8)(3) << 24);
            line->rgbc.code = (uint8)(0x40);
            if (idx % 5 == 2)
            {
                line->rgbc.r = (uint8)(255);
                line->rgbc.g = (uint8)(0);
                line->rgbc.b = (uint8)(255);
            }
            else
            {
                line->rgbc.r = (uint8)(0);
                line->rgbc.g = (uint8)(0);
                line->rgbc.b = (uint8)(0);
            }
        }
    }
    return 0;
}

static uint32 draw_preview(void)
{
    VEC3 model_pos;
    const char *text;
    uint32 height, val0, val1, val2, val, reference, object, sel, state, flag;
    sint32 dx, dy, dz;
    CAR_SELECTION_MATRIX matrix;
    MODEL_RESOURCE model;
    uint32 model_offset;
    VECTOR relative = {0};

    FUNCTION_MARKER(0x800B46D8, "SLUS_006.10");
    height = car_selection.vertical_offset;
    val0 = preview_matrix_default.words[0];
    val1 = preview_matrix_default.words[1];
    val2 = preview_matrix_default.words[2];
    matrix.words[0] = (uint32)(val0);
    matrix.words[1] = (uint32)(val1);
    matrix.words[2] = (uint32)(val2);
    val0 = preview_matrix_default.words[3];
    val1 = preview_matrix_default.words[4];
    val2 = preview_matrix_default.words[5];
    matrix.words[3] = (uint32)(val0);
    matrix.words[4] = (uint32)(val1);
    matrix.words[5] = (uint32)(val2);
    val0 = preview_matrix_default.words[6];
    val1 = preview_matrix_default.words[7];
    matrix.words[6] = (uint32)(val0);
    matrix.words[7] = (uint32)(val1);
    model_set_view_center(256, (sint32)(height + 60));
    tex_find("Tree");
    val = car_selection.transition_depth;
    if ((sint32)val < 3841)
    {
        sel = (uint8)car_selection.sel_entry;
        if (car_selection.models[sel].state == 3)
        {
            object = car_selection.active_model_offset;
            model_offset = object;
            XASSERT(!(model_offset > car_selection.active_buffer.capacity || car_selection.active_buffer.capacity - model_offset < 60));
            model.capacity = car_selection.active_buffer.capacity - model_offset;
            model.bytes = car_selection.active_buffer.bytes + model_offset;
            val *= 40000;
            ((MODEL_HEADER *)model.bytes)->position.bits.x = 0;
            ((MODEL_HEADER *)model.bytes)->position.bits.y = 0;
            val = ((sint32)val / 4096);
            ((MODEL_HEADER *)model.bytes)->position.bits.z = (val + 1600) << 8;
            val = car_selection.model_angle + 16;
            car_selection.model_angle = (uint16)(val);
            RotMatrixY((sint16)val, &matrix.sdk);
            RotMatrixX(-240, &matrix.sdk);
            model_apply_light_matrix(&matrix.sdk);
            model_pos.bits.x = ((const MODEL_HEADER *)model.bytes)->position.bits.x;
            model_pos.bits.y = ((const MODEL_HEADER *)model.bytes)->position.bits.y;
            model_pos.bits.z = ((const MODEL_HEADER *)model.bytes)->position.bits.z;
            model_transform_matrix(&matrix.sdk, &model_pos);
            val = ((const MODEL_HEADER *)model.bytes)->position.bits.x;
            reference = model_render.view.sdk.t[0];
            val = (uint32)((sint32)val / 256) - reference;
            dx = (sint32)val / 4;
            relative.vx = (sint32)(uint32)((uint32)dx);
            val = ((const MODEL_HEADER *)model.bytes)->position.bits.y;
            reference = model_render.view.sdk.t[1];
            val = (uint32)((sint32)val / 256) - reference;
            dy = (sint32)val / 4;
            relative.vy = (sint32)(uint32)((uint32)dy);
            val = ((const MODEL_HEADER *)model.bytes)->position.bits.z;
            reference = model_render.view.sdk.t[2];
            val = (uint32)((sint32)val / 256) - reference;
            dz = (sint32)val / 4;
            relative.vz = (sint32)(uint32)((uint32)dz);
            SetRotMatrix(&model_render.view.sdk);
            ApplyRotMatrixLV(&relative, &relative);
            height = car_selection.vertical_offset;
            car_environment_render.mapping.center_x = (sint16)(uint16)(256);
            car_environment_render.tint.r = (uint8)(64);
            car_environment_render.tint.g = (uint8)(64);
            car_environment_render.tint.b = (uint8)(64);
            car_environment_render.mapping.center_y = (sint16)(uint16)(height + 70);
            model_pos.bits.x = ((const MODEL_HEADER *)model.bytes)->position.bits.x;
            model_pos.bits.y = ((const MODEL_HEADER *)model.bytes)->position.bits.y;
            model_pos.bits.z = ((const MODEL_HEADER *)model.bytes)->position.bits.z;
            model_transform_matrix(&matrix.sdk, &model_pos);
            val = (uint32)dx * (uint32)dx + (uint32)dy * (uint32)dy + (uint32)dz * (uint32)dz;
            val = fixed_sqrt(val);
            car_draw_environment_mapping(&model, &car_environment_render.mapping, val << 2);
            model_draw(&model);
            wheels_draw_preview();
        }
    }
    state = car_selection.phase;
    if ((sint32)state < 12)
        car_selection.label_phase = (uint32)(state);
    if (race_session.two_players != 0)
    {
        presentation_select_pal(2);
        state = car_selection.stage;
        if (state == 1)
        {
            presentation_draw_text(131, 256, 32, "PLAYER 1");
            state = car_selection.stage;
        }
        if (state == 2)
            presentation_draw_text(131, 256, 32, "PLAYER 2");
    }
    if (car_selection.label_phase < 3)
    {
        presentation_select_pal(2);
        if (race_session.event_mode == 3 && car_selection.stage == 2)
            presentation_draw_text(131, 256, 20, "PLEASE SELECT YOUR OPPONENT");
        else
        {
            presentation_draw_text(131, 256, 20, "PLEASE SELECT YOUR CAR");
            if (race_session.event_mode - 4 >= 2)
            {
                sel = (uint8)car_selection.sel_entry;
                if (((uint8)model_flags[sel] & 1) != 0 && car_selection.models[sel].state == 3 && car_selection.phase == 2)
                    presentation_draw_text(129, 256, 135, "USE UP/DOWN TO CHOOSE CAR COLOR");
            }
        }
    }
    if (car_selection.label_phase - 6 < 3)
    {
        presentation_select_pal(2);
        presentation_draw_text(131, 256, 20, "PLEASE SELECT TRANSMISSION");
        if (car_selection.label_phase == 7)
            presentation_items[3].active = (1);
        flag = car_selection.manual_transmission;
        presentation_items[4].active = (1);
        val = car_selection.banner_wiggle;
        if (flag == 0)
        {
            presentation_items[3].y = (sint16)(100);
            presentation_items[4].y = (sint16)(100);
            val0 = 184 - val;
            val1 = val + 328;
        }
        else
        {
            presentation_items[3].y = (sint16)(120);
            presentation_items[4].y = (sint16)(120);
            val0 = 202 - val;
            val1 = val + 310;
        }
        presentation_items[3].x_flags = (val0);
        presentation_items[4].x_flags = (val1);
        presentation_select_pal(car_selection.manual_transmission == 0 ? 1 : 2);
        presentation_draw_text(130, 256, 90, "AUTOMATIC");
        presentation_select_pal(car_selection.manual_transmission != 0 ? 1 : 2);
        presentation_draw_text(130, 256, 110, "MANUAL");
    }
    if (car_selection.label_phase - 9 >= 3)
        return 0;
    presentation_select_pal(2);
    text = "PLEASE USE THE WORKSHOP";
    return presentation_draw_text(131, 256, 20, text);
}

void car_selection_cancel_load(void)
{
    uint32 slot;
    uint8 *allocation;

    FUNCTION_MARKER(0x800B3BEC, "SLUS_006.10");
    ;
    slot = car_selection.loading_model;
    car_selection.models[slot].state = (uint8)(0);
    stream_clear_slot(0);
    allocation = compressed_input.bytes;
    free(allocation);
    compressed_input.bytes = NULL;
    compressed_input.capacity = 0;
}

static const char preview_tex_name[] = "blk";

static intptr prepare_preview_tex(const MODEL_RESOURCE *model)
{
    uint32 records, offset0, offset1, offset, type, clut, val;
    const uint8 *tex;
    POLY_FT3 *tri0;
    POLY_FT3 *tri1;
    POLY_FT4 *quad0;
    POLY_FT4 *quad1;
    sint32 count, idx;

    FUNCTION_MARKER(0x800B7D6C, "SLUS_006.10");
    XASSERT(!(model->capacity < 60));
    records = ((const MODEL_HEADER *)model->bytes)->stream_offset;
    offset = ((const MODEL_HEADER *)model->bytes)->packet_offsets[0];
    offset0 = offset;
    offset = ((const MODEL_HEADER *)model->bytes)->packet_offsets[1];
    offset1 = offset;
    tex = tex_find(preview_tex_name);
    XASSERT(!(records > model->capacity || model->capacity - records < 4));
    if (model->bytes[records + 3] == 0)
        return (intptr)tex;
    do
    {
        type = model->bytes[records + 2];
        count = (sint16)xport_load_le16(model->bytes + records);
        records += 4;
        for (idx = 0; type <= 3 && idx < count; ++idx)
        {
            if (type == 0)
            {
                clut = xport_load_le16(tex + 10);
                XASSERT(!(offset0 > model->capacity || sizeof(*tri0) > model->capacity - offset0));
                tri0 = (POLY_FT3 *)(model->bytes + offset0);
                tri0->p[0].attr = (uint16)clut;
                XASSERT(!(offset1 > model->capacity || sizeof(*tri1) > model->capacity - offset1));
                tri1 = (POLY_FT3 *)(model->bytes + offset1);
                tri1->p[0].attr = (uint16)clut;
                offset0 += 32;
                offset1 += 32;
                records += 20;
            }
            else if (type == 1)
            {
                clut = xport_load_le16(tex + 10);
                XASSERT(!(offset0 > model->capacity || sizeof(*quad0) > model->capacity - offset0));
                quad0 = (POLY_FT4 *)(model->bytes + offset0);
                quad0->p[0].attr = (uint16)clut;
                XASSERT(!(offset1 > model->capacity || sizeof(*quad1) > model->capacity - offset1));
                quad1 = (POLY_FT4 *)(model->bytes + offset1);
                quad1->p[0].attr = (uint16)clut;
                val = quad0->rgbc.code;
                quad0->rgbc.code = (uint8)(val | 1);
                val = quad1->rgbc.code;
                quad1->rgbc.code = (uint8)(val & 0xFE);
                offset0 += 40;
                offset1 += 40;
                records += 20;
            }
            else if (type == 2)
            {
                offset0 += 40;
                offset1 += 40;
                records += 44;
            }
            else if (type == 3)
            {
                offset0 += 52;
                offset1 += 52;
                records += 56;
            }
        }
        XASSERT(!(records > model->capacity || model->capacity - records < 4));
    } while (model->bytes[records + 3] != 0);
    return 0;
}

static uint32 finish_load(void)
{
    uint32 wire_word_4;
    uint32 slot, size, idx, cursor, count, model, result;
    uint8 *buffer;
    const uint8 *tex;
    uint8 *buffer_bytes;
    CAR_SELECTION_MODEL *entry;

    FUNCTION_MARKER(0x800B3C50, "SLUS_006.10");
    slot = car_selection.loading_model;
    entry = car_selection.models + slot;
    if (entry->state != 2)
        return 2;
    XASSERT(!(compressed_input.capacity < 8));
    size = rnc_allocation_size(compressed_input.bytes);
    buffer = acquire_buffer(slot);
    entry->buffer = buffer;
    XASSERT(!(size > 0x32000));
    rnc_decode(compressed_input.bytes, compressed_input.capacity, buffer, (sint32)size);
    // Publish only the decoded model payload
    size -= 1024;
    free(compressed_input.bytes);
    buffer = entry->buffer;
    compressed_input.bytes = NULL;
    compressed_input.capacity = 0;
    buffer_bytes = buffer;
    XASSERT(!(size < 4));
    memcpy(&count, buffer_bytes, sizeof(count));
    if (count != 0)
    {
        idx = 1;
        cursor = 4;
        do
        {
            XASSERT(!(cursor > size || size - cursor < 4));
            {
                sint32 operation_ok = (memcpy(&wire_word_4, buffer_bytes + cursor, sizeof(wire_word_4)), wire_word_4) <= size;
                XASSERT(operation_ok);
            }
            memcpy(&count, buffer_bytes, sizeof(count));
            ++idx;
            cursor += 4;
        } while (count >= idx);
    }
    buffer = entry->buffer;
    entry->state = (uint8)(3);
    XASSERT(!(size < 8));
    memcpy(&model, buffer_bytes + 4, sizeof(model));
    car_selection.active_buffer.capacity = size;
    car_selection.active_buffer.bytes = buffer_bytes;
    car_selection.active_model_offset = (uint32)(model);
    {
        const CAR_SELECTION_BUFFER *asset = &car_selection.active_buffer;
        uint32 model_offset = model;
        const uint8 *model_bytes;
        uint32 groups_offset;
        uint32 groups_bytes;
        uint32 groups_address;
        XASSERT(!(model_offset > asset->capacity || asset->capacity - model_offset < 56));
        model_bytes = asset->bytes + model_offset;
        groups_offset = ((const MODEL_HEADER *)model_bytes)->stream_offset;
        groups_bytes = ((const MODEL_HEADER *)model_bytes)->packet_offsets[0] - groups_offset;
        groups_address = (model_offset + groups_offset);
        XASSERT(!(groups_address > asset->capacity || groups_bytes > asset->capacity - groups_address || asset->capacity - groups_address < 4));
        model_reset_prim_colors(asset->bytes + groups_address);
    }
    {
        MODEL_RESOURCE preview_model;
        uint32 model_offset = car_selection.active_model_offset;
        XASSERT(!(model_offset > car_selection.active_buffer.capacity));
        preview_model.bytes = car_selection.active_buffer.bytes + model_offset;
        preview_model.capacity = car_selection.active_buffer.capacity - model_offset;
        prepare_preview_tex(&preview_model);
    }
    upload_colors();
    tex = tex_find(body_tex_name);
    buffer_bytes = car_selection.active_buffer.bytes;
    XASSERT(!(car_selection.active_buffer.capacity < 8));
    memcpy(&model, buffer_bytes + 4, sizeof(model));
    {
        MODEL_PRIMITIVE_BUFFERS streams;
        const CAR_SELECTION_BUFFER *asset = &car_selection.active_buffer;
        uint32 model_offset = model;
        const uint8 *model_bytes;
        uint32 groups_offset;
        uint32 packet_offset1;
        uint32 groups_address;
        uint32 groups_bytes;
        uint32 packet_bytes;
        MODEL_TEXTURE_SOURCE tex_src;
        const MODEL_TEXTURE_SOURCE *native_tex;
        XASSERT(!(model_offset > asset->capacity || asset->capacity - model_offset < 60));
        model_bytes = asset->bytes + model_offset;
        groups_offset = ((const MODEL_HEADER *)model_bytes)->stream_offset;
        packet_offset1 = ((const MODEL_HEADER *)model_bytes)->packet_offsets[1];
        streams.packet_offset = ((const MODEL_HEADER *)model_bytes)->packet_offsets[0];
        groups_address = (model_offset + groups_offset);
        groups_bytes = streams.packet_offset - groups_offset;
        XASSERT(!(groups_address > asset->capacity || groups_bytes > asset->capacity - groups_address || asset->capacity - groups_address < 4));
        streams.groups = asset->bytes + groups_address;
        if (streams.groups[3] == 0)
        {
            streams.packets[0] = NULL;
            streams.packets[1] = NULL;
            native_tex = NULL;
        }
        else
        {
            uint32 asset_packet_offset0 = (uint32)(model_offset + streams.packet_offset);
            uint32 asset_packet_offset1 = (model_offset + packet_offset1);
            packet_bytes = packet_offset1 - streams.packet_offset;
            XASSERT(!(asset_packet_offset0 > asset->capacity || packet_bytes > asset->capacity - asset_packet_offset0));
            streams.packets[0] = asset->bytes + asset_packet_offset0;
            XASSERT(!(asset_packet_offset1 > asset->capacity || packet_bytes > asset->capacity - asset_packet_offset1));
            streams.packets[1] = asset->bytes + asset_packet_offset1;
            tex_src.attributes = NULL;
            tex_src.resource = tex;
            native_tex = &tex_src;
        }
        model_set_gouraud_tex(&streams, native_tex, 0);
    }
    result = init_wheel_geometry();
    ;
    return result;
}

static uint8 *acquire_buffer(uint32 slot)
{
    uint32 count, idx = 0, offset = 0, age, oldest = 0, chosen = 0, bank;
    CAR_SELECTION_MODEL *entry;

    FUNCTION_MARKER(0x800B61BC, "SLUS_006.10");
    XASSERT(!(slot >= 44 || car_selection.bank_count != 1));
    count = car_selection.bank_count;
    if ((sint32)count > 0)
    {
        do
        {
            if ((sint8)car_selection.banks[offset >> 3].model == -1)
            {
                car_selection.models[slot].bank = (uint8)(idx);
                car_selection.banks[offset >> 3].model = (uint8)(slot);
                return car_selection.banks[offset >> 3].buffer;
            }
            count = car_selection.bank_count;
            offset += 8;
            ++idx;
        } while ((sint32)idx < (sint32)count);
    }
    chosen = car_selection.banks[0].model;
    XASSERT(!(chosen >= 44));
    for (idx = 0; idx < 44; ++idx)
    {
        entry = car_selection.models + idx;
        if ((sint8)entry->bank == -1)
            continue;
        age = entry->age;
        if ((sint32)oldest < (sint32)age)
        {
            oldest = age;
            chosen = idx;
        }
    }
    (void)(chosen);
    entry = car_selection.models + chosen;
    bank = (sint8)entry->bank;
    car_selection.models[slot].bank = (uint8)(bank);
    XASSERT(!(bank >= car_selection.bank_count));
    car_selection.banks[bank].model = (uint8)(slot);
    if (chosen != slot)
    {
        entry->bank = (uint8)(0xFF);
        entry->state = (uint8)(0);
    }
    return car_selection.banks[bank].buffer;
}

static uint32 init_wheel_geometry(void)
{
    const CAR_SELECTION_BUFFER *buffer;
    const uint8 *data;
    uint32 offset;

    FUNCTION_MARKER(0x800B5944, "SLUS_006.10");
    buffer = &car_selection.active_buffer;
    XASSERT(!(buffer->capacity < 0x58));
    memcpy(&offset, buffer->bytes + 0x54, sizeof(offset));
    XASSERT(!(offset > buffer->capacity || buffer->capacity - offset < 134));
    data = buffer->bytes + offset;
    return wheels_init_sides(0, (sint16)xport_load_le16(data + 130), (sint16)xport_load_le16(data + 132));
}

uint32 car_selection_build_list(void)
{
    const uint16 *list = base_models;
    uint32 category = 1, limit = 44;
    uint32 idx, prev, count, cursor, sel, grp, result;
    sint32 val;

    FUNCTION_MARKER(0x800B36E8, "SLUS_006.10");
    if (race_session.event_mode == 2)
    {
        sel = race_session.cup;
        if (sel == 3)
        {
            if (car_selection.mode == 0)
            {
                for (idx = 0; idx < 13; ++idx)
                    session_cup_models[12 - idx] = (uint16)(0xFFFF);
                idx = 0;
                while (idx < 10)
                {
                    val = randPSX() % 16;
                    val = (sint16)base_models[(uint32)val];
                    for (prev = 0; prev < idx; ++prev)
                    {
                        if ((sint16)session_cup_models[prev] == val)
                            break;
                    }
                    if (prev != idx)
                        continue;
                    session_cup_models[idx] = (uint16)((uint32)val);
                    ++idx;
                }
            }
            list = session_cup_models;
            category = 0;
        }
        else if (sel == 5)
        {
            sel = session_options.beast_cars;
            limit = 8;
            list = sel != 0 ? car_selection_beast_models : car_selection_standard_models;
            category = 0;
        }
        else
            category = 2;
    }
    if (race_session.cop_chase != 0)
    {
        list = cop_models;
        category = 0;
        limit = 44;
    }
    if (race_session.two_players != 0)
        category = 3;
    if (car_selection.stage == 1 && race_session.multiplayer_mode == 3)
    {
        list = cop_models;
        category = 0;
        limit = 44;
    }
    if (race_session.drag_race != 0)
        category = 0;
    car_selection.list_count = (uint32)(0);
    for (idx = 0; idx < limit; ++idx)
    {
        val = (sint16)list[idx];
        if (val == -2)
            continue;
        if (val == -1)
            break;
        count = car_selection.list_count;
        XASSERT(!(count >= sizeof(car_selection.entries) / sizeof(car_selection.entries[0])));
        car_selection.entries[count] = (uint16)((uint32)val);
        car_selection.list_count = (uint32)(count + 1);
    }
    if (category == 1 || category == 3)
    {
        for (grp = 2; grp != 0; --grp)
        {
            for (idx = 0; idx < 44; ++idx)
            {
                if (progression_profile.model_availability[idx] != grp)
                    continue;
                if (category == 3 && ((uint8)model_flags[idx] & 2) == 0)
                    continue;
                count = car_selection.list_count;
                XASSERT(!(count >= sizeof(car_selection.entries) / sizeof(car_selection.entries[0])));
                val = (grp == 1 ? idx | 0x100 : idx);
                car_selection.entries[count] = (uint16)((uint32)val);
                car_selection.list_count = (uint32)(count + 1);
            }
        }
    }
    else if (category == 2)
    {
        cursor = 0;
        for (idx = 0; idx < 44; ++idx)
        {
            if (progression_profile.model_availability[idx] != 2)
                continue;
            if ((uint8)cup_bonus_models[cursor] != idx)
                continue;
            count = car_selection.list_count;
            XASSERT(!(count >= sizeof(car_selection.entries) / sizeof(car_selection.entries[0])));
            ++cursor;
            car_selection.entries[count] = (uint16)(idx);
            car_selection.list_count = (uint32)(count + 1);
        }
    }
    count = car_selection.list_count;
    car_selection.list_idx = (0);
    if ((sint32)count <= 0)
        return count;
    sel = session_options.retained_model;
    for (idx = 0; (sint32)idx < (sint32)count; ++idx)
    {
        result = (uint8)car_selection.entries[idx];
        if (sel == result)
        {
            car_selection.list_idx = (idx);
            return result;
        }
    }
    return 0;
}
