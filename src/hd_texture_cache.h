#ifndef TD5_HD_TEXTURE_CACHE_H
#define TD5_HD_TEXTURE_CACHE_H

#include "xport.h"
#include "vid.h"

typedef struct HD_SOURCE_TEXTURE HD_SOURCE_TEXTURE;

typedef struct HD_SOURCE_REGION
{
    uint32 allocation;
    sint32 u, v;
    uint32 width, height;
} HD_SOURCE_REGION;

typedef struct HD_SOURCE_GPU
{
    VID_TEXTURE *texture;
    uint32 opaque_layer, stp_layer, mode, classes, width, height;
} HD_SOURCE_GPU;

typedef struct HD_SOURCE_PIXELS
{
    uint32 width, height, classes;
    const uint16 *colors;
} HD_SOURCE_PIXELS;

// allocations borrow archive payloads until source collection is sealed
void hd_texture_sources_begin(const uint16 *vram);
void hd_texture_sources_reset(void);
void hd_texture_source_upload(uint32 x, uint32 y, uint32 words, uint32 height, const uint8 *payload, uint32 bytes);
sint32 hd_texture_source_region(uint16 page, uint16 min_u, uint16 min_v, uint16 max_u, uint16 max_v, HD_SOURCE_REGION *region);
HD_SOURCE_TEXTURE *hd_texture_source_bind(const HD_SOURCE_REGION *region, uint16 page, uint16 clut, const uint16 *vram);
HD_SOURCE_TEXTURE *hd_texture_source_bind_base(uint16 page, uint16 clut, uint16 min_u, uint16 min_v, uint16 max_u, uint16 max_v, const uint16 *vram, HD_SOURCE_REGION *region);
// native PS1 color words retain zero transparency and the STP bit
HD_SOURCE_TEXTURE *hd_texture_source_material(uint16 page, uint16 clut, uint16 min_u, uint16 min_v, uint16 max_u, uint16 max_v, HD_SOURCE_REGION *region);
HD_SOURCE_PIXELS hd_texture_source_pixels(const HD_SOURCE_TEXTURE *texture);
HD_SOURCE_TEXTURE *hd_texture_source_tim(const uint8 *image, uint32 bytes);
void hd_texture_source_semitransparent(HD_SOURCE_TEXTURE *texture);
void hd_texture_sources_build_gpu(void);
void hd_texture_sources_visit_banks(void (*visitor)(VID_TEXTURE *, void *), void *context);
HD_SOURCE_GPU hd_texture_source_gpu(const HD_SOURCE_TEXTURE *texture);
void hd_texture_source_bank_size(VID_TEXTURE *texture, uint32 *width, uint32 *height);
void hd_texture_sources_native(const uint16 *vram);
HD_SOURCE_TEXTURE *hd_texture_source_named(uint16 page, uint16 clut, uint32 u, uint32 v, uint32 width, uint32 height, const uint16 *vram);
HD_SOURCE_TEXTURE *hd_texture_source_lookup(uint16 page, uint16 clut, uint32 u, uint32 v, uint32 width, uint32 height, HD_SOURCE_REGION *region);
void hd_texture_sources_seal(void);

#endif
