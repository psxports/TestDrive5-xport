#ifndef TD5_HD_SCENE_MATERIAL_H
#define TD5_HD_SCENE_MATERIAL_H
#include "vid.h"
typedef struct HD_SCENE_BINDINGS
{
    VID_TEXTURE *textures[4];
    uint32 pass, view, mirrored;
} HD_SCENE_BINDINGS;
void hd_scene_material_bind(VID_MATERIAL *source, const HD_SCENE_BINDINGS *bindings);
void hd_scene_material_reset(void);
#endif
