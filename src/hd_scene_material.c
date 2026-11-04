#include <stdlib.h>
#include <string.h>
#include "hd_scene_material.h"

typedef struct HD_SCENE_MATERIAL_VARIANT
{
    struct HD_SCENE_MATERIAL_VARIANT *next, *hash_next;
    VID_MATERIAL *source, *material;
    HD_SCENE_BINDINGS bindings;
    uint64 revision;
} HD_SCENE_MATERIAL_VARIANT;

static HD_SCENE_MATERIAL_VARIANT *variants, *lookup[1024];

void hd_scene_material_bind(VID_MATERIAL *source, const HD_SCENE_BINDINGS *bindings)
{
    if (bindings == NULL)
    {
        vid_pass(source);
        return;
    }
    uint32 bucket = (uint32)(((uintptr_t)source >> 4) ^ (bindings->pass << 4) ^ bindings->view) & 1023;
    HD_SCENE_MATERIAL_VARIANT *entry;
    for (entry = lookup[bucket]; entry; entry = entry->hash_next)
        if (entry->source == source && entry->bindings.pass == bindings->pass && entry->bindings.view == bindings->view)
            break;
    if (!entry)
    {
        entry = (HD_SCENE_MATERIAL_VARIANT *)calloc(1, sizeof(*entry));
        XASSERT(entry);
        entry->source = source;
        entry->next = variants;
        variants = entry;
        entry->hash_next = lookup[bucket];
        lookup[bucket] = entry;
    }
    uint64 revision = vid_mat_revision(source);
    if (!entry->material || entry->revision != revision || entry->bindings.mirrored != bindings->mirrored ||
        entry->bindings.textures[0] != bindings->textures[0] || entry->bindings.textures[1] != bindings->textures[1] ||
        entry->bindings.textures[2] != bindings->textures[2] || entry->bindings.textures[3] != bindings->textures[3])
    {
        vid_mat_free(entry->material);
        entry->material = vid_mat_create();
        XASSERT(entry->material);
        VID_MATERIAL_DATA *data = vid_mat_data(entry->material);
        const VID_MATERIAL_DATA *original = vid_mat_read(source);
        XASSERT(original);
        *data = *original;
        if (bindings->mirrored && data->cull)
            data->cull = data->cull == 1 ? 2 : 1;
        data->aux[1] = bindings->textures[0];
        data->aux[2] = bindings->textures[1];
        data->aux[3] = bindings->textures[2];
        data->aux[4] = bindings->textures[3];
        data->aux_filter[4] = VID_FILTER_TRILINEAR;
        vid_mat_commit(entry->material);
        entry->revision = revision;
        entry->bindings = *bindings;
    }
    vid_pass(entry->material);
}

void hd_scene_material_reset(void)
{
    while (variants)
    {
        HD_SCENE_MATERIAL_VARIANT *entry = variants;
        variants = entry->next;
        vid_mat_free(entry->material);
        free(entry);
    }
    memset(lookup, 0, sizeof(lookup));
}
