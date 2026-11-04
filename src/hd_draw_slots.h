#ifndef TD5_HD_DRAW_SLOTS_H
#define TD5_HD_DRAW_SLOTS_H

// one packed layout is shared by the C producer and GLSL consumer
#define HD_DRAW_SLOTS(X) \
    X(OBJECT0, 0) \
    X(OBJECT1, 1) \
    X(OBJECT2, 2) \
    X(PROJECTION, 3) \
    X(VIEWPORT, 4) \
    X(DRAW_COLOR, 5) \
    X(LIGHT0, 6) \
    X(LIGHT1, 7) \
    X(COLOR0, 8) \
    X(COLOR1, 9) \
    X(COLOR2, 10) \
    X(AMBIENT, 11) \
    X(ENV_MODE, 12) \
    X(ENV_TRANSFORM, 13) \
    X(ENV_PERIOD, 14) \
    X(ENV_ORIGIN, 15) \
    X(ENV_SCALE, 16) \
    X(ENV_TEXTURE, 17) \
    X(ENV_REGION, 18) \
    X(HUB_NORMAL, 19) \
    X(HUB_ANCHOR, 20) \
    X(HUB_FAR, 21) \
    X(HUB_CUE, 22) \
    X(PREV_LIGHT0, 23) \
    X(PREV_LIGHT1, 24) \
    X(PREV_LIGHT2, 25) \
    X(PREV_COLOR0, 26) \
    X(PREV_COLOR1, 27) \
    X(PREV_COLOR2, 28) \
    X(PREV_AMBIENT, 29) \
    X(DISTANCE_LIGHT, 30) \
    X(OBLIQUE, 31) \
    X(CAR_SHADOW0, 32) \
    X(CAR_SHADOW1, 33) \
    X(CAR_SHADOW2, 34) \
    X(SHADOW_FILTER, 35) \
    X(SUN_AMBIENT, 36) \
    X(SUN_DIRECTION, 37) \
    X(SUN_COLOR, 38) \
    X(SHADOW_COUNTS, 39) \
    X(CAR_SAMPLE0, 40) \
    X(CAR_SAMPLE1, 41) \
    X(CAR_SAMPLE2, 42) \
    X(CAR_SAMPLE3, 43) \
    X(CAR_SAMPLE4, 44) \
    X(CAR_SAMPLE5, 45) \
    X(CAR_SAMPLE6, 46) \
    X(CAR_SAMPLE7, 47) \
    X(CAR_SAMPLE8, 48) \
    X(CAR_SAMPLE9, 49) \
    X(CAR_SAMPLE10, 50) \
    X(CAR_SAMPLE11, 51) \
    X(CAR_SAMPLE12, 52) \
    X(CAR_SAMPLE13, 53) \
    X(SURFACE, 54) \
    X(MATERIAL_SUN, 55) \
    X(MATERIAL_TINT, 56) \
    X(LIGHT_WORLD0, 57) \
    X(LIGHT_WORLD1, 58) \
    X(LIGHT_WORLD2, 59) \
    X(LIGHT_ORIGIN, 60) \
    X(ENV_SHADOW0, 61) \
    X(ENV_SHADOW1, 62) \
    X(ENV_SHADOW2, 63) \
    X(PLANAR0, 64) \
    X(PLANAR1, 65) \
    X(PLANAR2, 66) \
    X(PLANAR_WORLD0, 67) \
    X(PLANAR_WORLD1, 68) \
    X(PLANAR_WORLD2, 69) \
    X(PLANAR_SETTINGS, 70) \
    X(SKY, 71)

enum
{
#define HD_DRAW_SLOT_ENUM(name, value) HD_U_##name = value,
    HD_DRAW_SLOTS(HD_DRAW_SLOT_ENUM)
#undef HD_DRAW_SLOT_ENUM
    HD_U_COUNT = 72
};

#define HD_DRAW_SLOT_GLSL(name, value) "#define HD_U_" #name " " #value "\n"
#define HD_DRAW_SLOTS_GLSL HD_DRAW_SLOTS(HD_DRAW_SLOT_GLSL)

#endif
