PROJECT_ROOT := ../../..
SHORT_NAME := TD5
FULL_NAME := Test Drive 5
WEB_EXCLUDE_FILES := \
    DATA/TD5.DAT \
    DATA/MOVPLAY.EXE \
    DATA/_ZYXWVUT.ZYX \
    DATA/GAME.XA \
    DATA/PRES.XA
APP_SOURCES := $(wildcard ../../../src/batch_*.c) ../../../src/game_main.c
APP_DEFINES := -DXPORT_NATIVE_CALLBACK_BOOTSTRAP=1 -DWND_TITLE=\"TD5\" -DWND_WIDTH=960 -DWND_HEIGHT=720 -DFIELD_RATE=60
ifeq ($(CONFIG),Release)
APP_DEFINES += -DXPORT_DISABLE_FUNCTION_MARKERS
endif

include ../../../../xport/src/platform/web/Makefile
