#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "psx_gpu.h"

#if !defined(LOCKSTEP_DEBUG)
    #if AP_WEB
        #include <emscripten.h>

EM_JS(sint32, startup_random_seed, (uint32 * seed), {
    try
    {
        const words = new Uint32Array(1);
        globalThis.crypto.getRandomValues(words);
        HEAPU32[seed >> 2] = words[0];
        return 1;
    }
    catch (error)
    {
        return 0;
    }
});
    #else
        #include <windows.h>
        #include <bcrypt.h>

static sint32 startup_random_seed(uint32 *seed)
{
    return BCryptGenRandom(NULL, (PUCHAR)seed, sizeof(*seed), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
}
    #endif
#endif

/* Zero selects the existing native GPU owner */
const uint32 xport_gpu_graph_type_address = 0u;

/* No legacy CD storage is bound by the native game */
const uint32 xport_cd_sync_callback_address = 0u;
const uint32 xport_cd_ready_callback_address = 0u;
const uint32 xport_cd_status_address = 0u;
const uint32 xport_cd_setloc_table_address = 0u;

static sint32 game_run(void)
{
    sint32 random_val;
    uint32 unlock_idx;

    FUNCTION_MARKER(0x80015E38, "SLUS_006.10");
    ResetCallback();
    CdInit();
    movie_play_startup();
    disp_init_graphics();
    memory_card_init();
    archive_open("\\TD5.DAT;1");
    startup_show_images();
    audio_load_menu_samples(startup_buffer.bytes, sizeof(startup_buffer.bytes));
    car_load_s_table();
    audio_set_limit(4096);
    session_flow.phase = (0);
    session_flow.menu_state = (80);
    while (session_flow.exit_requested == 0)
    {
        ;
        switch (session_flow.phase)
        {
            case 0:
                if (session_flow.menu_initialized != 0)
                    presentation_show_loading();
                session_flow.attract_active = (0);
                memset(presentation_packets.menu_frame, 0, sizeof(presentation_packets.menu_frame));
                memset(presentation_packets.volume_background, 0, sizeof(presentation_packets.volume_background));
                memset(presentation_packets.volume_mask, 0, sizeof(presentation_packets.volume_mask));
                presentation_pres_text = presentation_pres_text_defaults;
                presentation_pres_recs = presentation_pres_rec_defaults;
                menu_history = (MENU_HISTORY){0};
                presentation_menu.save_load_attempted = 0;
                presentation_menu.finished = 0;
                presentation_menu.banner_x_angle = 0;
                presentation_menu.banner_y_angle = 0;
                presentation_menu.frame_counter = 0;
                menu_choices.panel_selection = 0;
                memset(input_menu, 0, sizeof(input_menu));
                presentation_name_editor = (PRESENTATION_NAME_EDITOR){0};
                recs_menu = (RECORDS_MENU_STATE){0};
                track_selection = (TRACK_SELECTION_STATE){0};
                credits = (CREDITS_STATE){0};
                card_menu = (MEMORY_CARD_MENU){0};
                presentation_carsel_text = presentation_carsel_text_defaults;
                presentation_carsel_recs = presentation_carsel_rec_defaults;
                car_selection = (CAR_SELECTION_STATE){0};
                for (unlock_idx = 0; unlock_idx < 5; ++unlock_idx)
                    race_menu.unlock_labels[unlock_idx] = NULL;
                race_menu.unlock_count = 0;
                results_menu = (RESULTS_MENU){.text = results_menu_text_defaults};
                memcpy(results_menu.text_slots, results_menu_default_slots, sizeof(results_menu.text_slots));
                presentation_render = presentation_render_defaults;
                memset(presentation_items, 0, sizeof(presentation_items));
                memset(presentation_menu.indices, 0, sizeof(presentation_menu.indices));
                presentation_menu.records = NULL;
                session_flow.loading_image_shown = (0);
                if (session_flow.menu_initialized == 0)
                {
                    session_reset_progression();
                    recs_generate_defaults();
                    recs_reset_candidates();
                    recs_sort();
                }
                presentation_load_menu();
                car_select_police_lights((sint8)session_car_choices.cars[0].model[0]);
                session_apply_menu_mode();
                input_configure_controls();
                session_flow.menu_initialized = (1);
                presentation_show_loading();
                audio_state.paired = (audio_options.paired);
                break;
            case 1:
                session_options.model_variant = (uint32)(0);
                replay_mode = (uint32)(0);
                if (race_session.two_players != 0)
                    session_flow.phase = (7);
                else
                {
                    presentation_show_loading();
                    scene_load_race();
                    session_flow.loading_image_shown = (0);
                    session_flow.phase = (2);
                }
                break;
            case 2:
                session_flow.loading_image_shown = (0);
                session_run_single_race();
                ;
                break;
            case 3:
                session_flow.loading_image_shown = (0);
                card_menu = (MEMORY_CARD_MENU){0};
                presentation_carsel_text = presentation_carsel_text_defaults;
                presentation_carsel_recs = presentation_carsel_rec_defaults;
                car_selection = (CAR_SELECTION_STATE){0};
                for (unlock_idx = 0; unlock_idx < 5; ++unlock_idx)
                    race_menu.unlock_labels[unlock_idx] = NULL;
                race_menu.unlock_count = 0;
                results_menu = (RESULTS_MENU){.text = results_menu_text_defaults};
                memcpy(results_menu.text_slots, results_menu_default_slots, sizeof(results_menu.text_slots));
                presentation_render = presentation_render_defaults;
                memset(presentation_items, 0, sizeof(presentation_items));
                memset(presentation_menu.indices, 0, sizeof(presentation_menu.indices));
                presentation_menu.records = NULL;
                race_menu_prepare();
                race_menu_run();
                audio_stop_xa();
                break;
            case 4:
                race_session.demo = (0);
                session_start_event();
                break;
            case 5:
                presentation_show_loading();
                session_advance_event();
                break;
            case 6:
                session_options.model_variant = (uint32)(1);
                replay_mode = (uint32)(1);
                scene_load_level_metadata();
                if (race_session.two_players != 0)
                    session_flow.phase = (7);
                else
                {
                    presentation_show_loading();
                    session_flow.loading_image_shown = (0);
                    race_session.demo = (1);
                    scene_load_race();
                    session_flow.phase = (2);
                }
                break;
            case 7:
                presentation_show_loading();
                scene_load_race();
                session_flow.loading_image_shown = (0);
                session_flow.phase = (8);
                break;
            case 8:
                session_flow.loading_image_shown = (0);
                session_run_two_player_race();
                break;
            case 9:
                session_flow.attract_active = (1);
                race_session.event_mode = (0);
                race_session.cup = (0);
                race_session.multiplayer_mode = (0);
                race_session.demo = (0);
                race_session.cop_chase = (0);
                race_session.two_players = (0);
                race_session.linear_course = (0);
                race_session.drag_race = (0);
                race_session.track = (session_attract_tracks[session_flow.attract_idx]);
                session_flow.attract_idx = ((session_flow.attract_idx + 1) & 3);
                random_val = randPSX();
                session_car_choices.cars[0].model[0] = (uint8)((uint8)(random_val % 15));
                scene_load_level_metadata();
                session_choose_rivals();
                race_session.demo = (1);
                session_options.model_variant = (uint32)(0);
                presentation_show_loading();
                session_flow.loading_image_shown = (0);
                scene_load_race();
                session_flow.phase = (2);
                break;
            case 10:
                movie_play_fear();
                session_flow.phase = (0);
                session_flow.menu_state = (80);
                presentation_show_loading();
                break;
            default:
                break;
        }
    };
    DrawSync(0);
    stopIntr();
    PadStopCom();
    ResetGraph(3);
    ResetCallback();
    return 0;
}

void xport_main(void)
{
    uint32 route_idx;
    CdlFILE initial_xa;
#if !defined(LOCKSTEP_DEBUG)
    uint32 random_seed;
#endif

    {
        sint32 operation_ok = !(!xport_window_init());
        XASSERT(operation_ok);
    }
#if !defined(LOCKSTEP_DEBUG)
    {
        sint32 operation_ok = !(!startup_random_seed(&random_seed));
        XASSERT(operation_ok);
    }
    srandPSX(random_seed);
    /* Randomize the 13-song native and eight-track Web playlist starts */
    audio_state.race_song_counter = random_seed % (13 * 8);
    #ifdef _DEBUG
    printf("td5_random_seed=%08X\n", random_seed);
    #endif
#endif
#if AP_WEB
    SpuInit();
#else
    {
        sint32 operation_ok = !(!asset_mount_xa_file(audio_xa_files[0], &initial_xa));
        XASSERT(operation_ok);
    }
#endif
    /* Enter the native game after shared runtime initialization */
    xport_set_exit_code(game_run());
    for (route_idx = 0; route_idx < 2; ++route_idx)
    {
        free(race_ai.routes[route_idx].samples);
        race_ai.routes[route_idx].samples = NULL;
        race_ai.routes[route_idx].count = 0;
    }
    free(race_ai.traffic.records);
    race_ai.traffic.records = NULL;
    race_ai.traffic.count = 0;
    race_ai.traffic.next = 0;
    free(track_geometry.branches);
    track_geometry.branches = NULL;
    track_geometry.branch_count = 0;
    free(camera_cues);
    camera_cues = NULL;
    camera_cue_count = 0;
    free(scene_pool.blocks);
    scene_pool = (SCENE_POOL){0};
    if (scene_level_image.src == SCENE_LEVEL_FILE)
        free(scene_level_image.bytes);
    scene_level_image = (SCENE_LEVEL_IMAGE){0};
    free(track_linear_image.bytes);
    track_linear_image = (TRACK_LINEAR_IMAGE){0};
    free(credits.archive.bytes);
    credits.archive = (CREDITS_ARCHIVE){0};
}
