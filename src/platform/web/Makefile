PROJECT_ROOT := ../../..
SHORT_NAME := TD5
FULL_NAME := Test Drive 5
WEB_EXCLUDE_FILES := \
    DATA/TD5.DAT \
    DATA/MOVPLAY.EXE \
    DATA/SLUS_006.10 \
    DATA/_ZYXWVUT.ZYX \
    DATA/GAME.XA \
    DATA/PRES.XA \
    DATA/PRES/PRES.BIN \
    DATA/PRES/CARSEL.BIN \
    DATA/PRES/USALOAD.TIM
APP_SOURCES := ../../../src/game_main.c ../../../src/asset_io.c ../../../src/scene_memory.c ../../../src/track_stream.c ../../../src/rnc.c ../../../src/fixed_math.c ../../../src/save.c ../../../src/timing.c ../../../src/texture.c ../../../src/track_geometry.c ../../../src/car.c ../../../src/car_collision.c ../../../src/car_render.c ../../../src/chase.c ../../../src/hud.c ../../../src/minimap.c ../../../src/model_render.c ../../../src/sky.c ../../../src/movies.c ../../../src/memory_card.c ../../../src/display.c ../../../src/effects.c ../../../src/audio.c ../../../src/startup.c ../../../src/wheels.c ../../../src/car_selection.c ../../../src/session.c ../../../src/replay.c ../../../src/race.c ../../../src/camera.c ../../../src/records.c ../../../src/car_random.c ../../../src/results_menu.c ../../../src/records_menu.c ../../../src/track_selection.c ../../../src/race_menu.c ../../../src/credits.c ../../../src/input.c ../../../src/presentation_render.c
APP_DEFINES := -DXPORT_NATIVE_CALLBACK_BOOTSTRAP=1 -DWND_TITLE=\"TD5\" -DWND_WIDTH=960 -DWND_HEIGHT=720 -DFIELD_RATE=60
APP_SOURCES += ../../../src/hd.c ../../../src/hd_track.c ../../../src/hd_texture_cache.c
ifeq ($(CONFIG),Release)
APP_DEFINES += -DXPORT_DISABLE_FUNCTION_MARKERS
endif

include ../../../../xport/src/platform/web/Makefile
