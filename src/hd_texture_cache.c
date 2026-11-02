#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "hd_texture_cache.h"

typedef struct HD_SOURCE_BANK
{
    struct HD_SOURCE_BANK *next;
    VID_TEXTURE *texture;
    uint32 width, height, layers;
} HD_SOURCE_BANK;

typedef struct HD_SOURCE_ALLOCATION
{
    uint32 x, y, words, height, hash;
    const uint8 *payload;
} HD_SOURCE_ALLOCATION;

struct HD_SOURCE_TEXTURE
{
    struct HD_SOURCE_TEXTURE *next;
    struct HD_SOURCE_TEXTURE *base_next;
    uint32 width, height, classes, hash;
    uint32 base_depth, semi_used, layer, mode;
    HD_SOURCE_BANK *bank;
    uint16 *colors;
    uint8 *assigned;
};

typedef struct HD_SOURCE_BINDING
{
    struct HD_SOURCE_BINDING *next;
    uint32 allocation, depth, hash, palette_count;
    uint16 palette[256];
    HD_SOURCE_TEXTURE *texture;
} HD_SOURCE_BINDING;

typedef struct HD_SOURCE_NAMED
{
    struct HD_SOURCE_NAMED *next;
    uint16 page, clut;
    uint32 u, v, width, height;
    HD_SOURCE_TEXTURE *texture;
} HD_SOURCE_NAMED;

static struct
{
    uint32 *owners;
    HD_SOURCE_ALLOCATION *allocations;
    uint32 count, capacity;
    const uint16 *vram;
    uint64 material_refs, stream_refs, base_refs, unresolved_refs, decoded_images, decoded_bytes;
    HD_SOURCE_TEXTURE *textures[1024];
    HD_SOURCE_TEXTURE *base_textures;
    HD_SOURCE_BANK *banks;
    HD_SOURCE_NAMED *named;
    uint64 gpu_bytes, gpu_banks;
    HD_SOURCE_BINDING *bindings[1024];
} source_cache;

void hd_texture_sources_reset(void)
{
    uint32 bucket;
    for (bucket = 0; bucket < 1024; ++bucket)
        while (source_cache.textures[bucket] != NULL)
        {
            HD_SOURCE_TEXTURE *texture = source_cache.textures[bucket];
            source_cache.textures[bucket] = texture->next;
            free(texture->colors);
            free(texture->assigned);
            free(texture);
        }
    for (bucket = 0; bucket < 1024; ++bucket)
        while (source_cache.bindings[bucket] != NULL)
        {
            HD_SOURCE_BINDING *binding = source_cache.bindings[bucket];
            source_cache.bindings[bucket] = binding->next;
            free(binding);
        }
    while (source_cache.named != NULL)
    {
        HD_SOURCE_NAMED *named = source_cache.named;
        source_cache.named = named->next;
        free(named);
    }
    while (source_cache.banks != NULL)
    {
        HD_SOURCE_BANK *bank = source_cache.banks;
        source_cache.banks = bank->next;
        vid_tex_free(bank->texture);
        free(bank);
    }
    free(source_cache.allocations);
    free(source_cache.owners);
    memset(&source_cache, 0, sizeof(source_cache));
}

void hd_texture_sources_begin(const uint16 *vram)
{
    hd_texture_sources_reset();
    source_cache.vram = vram;
    source_cache.owners = (uint32 *)calloc(1024 * 512, sizeof(uint32));
    XASSERT(source_cache.owners != NULL);
}

void hd_texture_source_upload(uint32 x, uint32 y, uint32 words, uint32 height, const uint8 *payload, uint32 bytes)
{
    HD_SOURCE_ALLOCATION *allocation;
    uint32 row, column, hash = 2166136261u;
    XASSERT(source_cache.owners != NULL && payload != NULL);
    XASSERT(x <= 1024 && words <= 1024 - x && y <= 512 && height <= 512 - y);
    XASSERT(words != 0 && height != 0 && (uint64)words * height * 2 <= bytes);
    if (source_cache.count == source_cache.capacity)
    {
        uint32 capacity = source_cache.capacity != 0 ? source_cache.capacity * 2 : 256;
        source_cache.allocations = (HD_SOURCE_ALLOCATION *)realloc(source_cache.allocations, capacity * sizeof(*source_cache.allocations));
        XASSERT(source_cache.allocations != NULL);
        source_cache.capacity = capacity;
    }
    allocation = &source_cache.allocations[source_cache.count++];
    for (column = 0; column < words * height * 2; ++column)
        hash = (hash ^ payload[column]) * 16777619u;
    *allocation = (HD_SOURCE_ALLOCATION){x, y, words, height, hash, payload};
    for (row = 0; row < height; ++row)
        for (column = 0; column < words; ++column)
            source_cache.owners[(y + row) * 1024 + x + column] = source_cache.count;
}

sint32 hd_texture_source_region(uint16 page, uint16 min_u, uint16 min_v, uint16 max_u, uint16 max_v, HD_SOURCE_REGION *region)
{
    uint32 depth = (page >> 7) & 3, shift = depth == 0 ? 2 : depth == 1 ? 1 : 0;
    uint32 left, right, top, bottom, owner, row, column;
    const HD_SOURCE_ALLOCATION *allocation;
    sint32 u, v;
    if (source_cache.owners == NULL || min_u > max_u || min_v > max_v || max_u > 255 || max_v > 255)
        return 0;
    left = (((page & 15) * 64) + (min_u >> shift)) & 1023;
    right = (((page & 15) * 64) + (max_u >> shift)) & 1023;
    top = ((((page >> 4) & 1) * 256) + min_v) & 511;
    bottom = ((((page >> 4) & 1) * 256) + max_v) & 511;
    if (right < left || bottom < top)
        return 0;
    owner = source_cache.owners[top * 1024 + left];
    if (owner == 0)
        return 0;
    allocation = &source_cache.allocations[owner - 1];
    if (left < allocation->x || right >= allocation->x + allocation->words || top < allocation->y || bottom >= allocation->y + allocation->height)
        return 0;
    for (row = top; row <= bottom; ++row)
        for (column = left; column <= right; ++column)
            if (source_cache.owners[row * 1024 + column] != owner)
                return 0;
    u = ((sint32)allocation->x - (sint32)((page & 15) * 64)) * (1 << shift);
    v = (sint32)allocation->y - (sint32)(((page >> 4) & 1) * 256);
    // select the source address alias containing the original UV declaration
    while (u > min_u)
        u -= 1024 << shift;
    while (u + (sint32)(allocation->words << shift) <= max_u)
        u += 1024 << shift;
    while (v > min_v)
        v -= 512;
    while (v + (sint32)allocation->height <= max_v)
        v += 512;
    if (u > min_u || v > min_v)
        return 0;
    *region = (HD_SOURCE_REGION){owner, u, v, allocation->words << shift, allocation->height};
    return 1;
}

HD_SOURCE_TEXTURE *hd_texture_source_bind(const HD_SOURCE_REGION *region, uint16 page, uint16 clut, const uint16 *vram)
{
    const HD_SOURCE_ALLOCATION *allocation;
    HD_SOURCE_TEXTURE *texture;
    HD_SOURCE_BINDING *binding;
    uint16 palette_words[256];
    uint32 key, palette_count;
    uint32 depth = (page >> 7) & 3, shift = depth == 0 ? 2 : depth == 1 ? 1 : 0;
    uint32 x, y, hash = 2166136261u, classes = 0, bytes;
    uint16 *colors;
    XASSERT(region != NULL && region->allocation != 0 && region->allocation <= source_cache.count);
    allocation = &source_cache.allocations[region->allocation - 1];
    XASSERT(allocation->payload != NULL && vram != NULL);
    XASSERT(region->width == allocation->words << shift && region->height == allocation->height);
    palette_count = depth == 0 ? 16 : depth == 1 ? 256 : 0;
    key = (allocation->hash ^ depth) * 16777619u;
    key = (key ^ region->width) * 16777619u;
    key = (key ^ region->height) * 16777619u;
    for (x = 0; x < palette_count; ++x)
    {
        palette_words[x] = vram[((clut >> 6) & 511) * 1024 + (((clut & 63) * 16 + x) & 1023)];
        key = (key ^ palette_words[x]) * 16777619u;
    }
    for (binding = source_cache.bindings[key & 1023]; binding != NULL; binding = binding->next)
    {
        const HD_SOURCE_ALLOCATION *other = &source_cache.allocations[binding->allocation - 1];
        if (binding->hash == key && binding->depth == depth && other->words == allocation->words && other->height == allocation->height && memcmp(binding->palette, palette_words, palette_count * sizeof(uint16)) == 0 && memcmp(other->payload, allocation->payload, allocation->words * allocation->height * 2) == 0)
            return binding->texture;
    }
    bytes = region->width * region->height * sizeof(uint16);
    colors = (uint16 *)malloc(bytes);
    XASSERT(colors != NULL);
    for (y = 0; y < region->height; ++y)
        for (x = 0; x < region->width; ++x)
        {
            uint16 word;
            uint32 index = y * region->width + x;
            memcpy(&word, allocation->payload + (y * allocation->words + (x >> shift)) * 2, sizeof(word));
            if (depth < 2)
            {
                uint32 palette = depth == 0 ? (word >> ((x & 3) * 4)) & 15 : (word >> ((x & 1) * 8)) & 255;
                word = palette_words[palette];
            }
            colors[index] = word;
            classes |= word == 0 ? 1u : (word & 32768) != 0 ? 4u : 2u;
        }
    hash = (hash ^ region->width) * 16777619u;
    hash = (hash ^ region->height) * 16777619u;
    for (x = 0; x < bytes; ++x)
        hash = (hash ^ ((const uint8 *)colors)[x]) * 16777619u;
    for (texture = source_cache.textures[hash & 1023]; texture != NULL; texture = texture->next)
        if (texture->assigned == NULL && texture->hash == hash && texture->width == region->width && texture->height == region->height && memcmp(texture->colors, colors, bytes) == 0)
        {
            free(colors);
            goto remember;
        }
    texture = (HD_SOURCE_TEXTURE *)calloc(1, sizeof(*texture));
    XASSERT(texture != NULL);
    texture->width = region->width;
    texture->height = region->height;
    texture->classes = classes;
    texture->hash = hash;
    texture->colors = colors;
    ++source_cache.decoded_images;
    source_cache.decoded_bytes += bytes;
    texture->next = source_cache.textures[hash & 1023];
    source_cache.textures[hash & 1023] = texture;
remember:
    binding = (HD_SOURCE_BINDING *)calloc(1, sizeof(*binding));
    XASSERT(binding != NULL);
    binding->allocation = region->allocation;
    binding->depth = depth;
    binding->hash = key;
    binding->palette_count = palette_count;
    memcpy(binding->palette, palette_words, palette_count * sizeof(uint16));
    binding->texture = texture;
    binding->next = source_cache.bindings[key & 1023];
    source_cache.bindings[key & 1023] = binding;
    return texture;
}

HD_SOURCE_TEXTURE *hd_texture_source_bind_base(uint16 page, uint16 clut, uint16 min_u, uint16 min_v, uint16 max_u, uint16 max_v, const uint16 *vram, HD_SOURCE_REGION *region)
{
    uint32 depth = (page >> 7) & 3, shift = depth == 0 ? 2 : depth == 1 ? 1 : 0;
    uint32 width = (uint32)max_u - min_u + 1, height = (uint32)max_v - min_v + 1;
    uint32 x, y, source_width = 512u << shift, classes = 0;
    uint16 *decoded;
    HD_SOURCE_TEXTURE *texture;
    XASSERT(vram != NULL && region != NULL);
    if (min_u > max_u || min_v > max_v || max_u > 255 || max_v > 255)
        return NULL;
    decoded = (uint16 *)malloc(width * height * sizeof(uint16));
    XASSERT(decoded != NULL);
    for (y = 0; y < height; ++y)
        for (x = 0; x < width; ++x)
        {
            uint32 u = min_u + x, v = min_v + y;
            uint32 wx = ((page & 15) * 64 + (u >> shift)) & 1023;
            uint32 wy = ((((page >> 4) & 1) * 256) + v) & 511;
            uint32 index = y * width + x;
            uint16 word;
            if (wx < 512 || (source_cache.owners != NULL && source_cache.owners[wy * 1024 + wx] != 0))
            {
                free(decoded);
                return NULL;
            }
            word = vram[wy * 1024 + wx];
            if (depth < 2)
            {
                uint32 palette = depth == 0 ? (word >> ((u & 3) * 4)) & 15 : (word >> ((u & 1) * 8)) & 255;
                word = vram[((clut >> 6) & 511) * 1024 + (((clut & 63) * 16 + palette) & 1023)];
            }
            decoded[index] = word;
            classes |= word == 0 ? 1u : (word & 32768) != 0 ? 4u : 2u;
        }
    // preserve the complete original base-image coordinates, without inferred texture bounds
    region->allocation = 0;
    region->u = (512 - (sint32)((page & 15) * 64)) * (1 << shift);
    region->v = -(sint32)(((page >> 4) & 1) * 256);
    region->width = source_width;
    region->height = 512;
    if ((sint32)min_u - region->u < 0 || (uint32)((sint32)max_u - region->u) >= source_width || (sint32)min_v - region->v < 0 || (sint32)max_v - region->v >= 512)
    {
        free(decoded);
        return NULL;
    }
    for (texture = source_cache.base_textures; texture != NULL; texture = texture->base_next)
    {
        sint32 compatible = 1;
        if (texture->base_depth != depth)
            continue;
        for (y = 0; y < height && compatible; ++y)
            for (x = 0; x < width; ++x)
            {
                uint32 target = (min_v + y - region->v) * source_width + min_u + x - region->u;
                if ((texture->assigned[target >> 3] & (1u << (target & 7))) != 0 && texture->colors[target] != decoded[y * width + x])
                {
                    compatible = 0;
                    break;
                }
            }
        if (compatible)
            break;
    }
    if (texture == NULL)
    {
        texture = (HD_SOURCE_TEXTURE *)calloc(1, sizeof(*texture));
        XASSERT(texture != NULL);
        texture->width = source_width;
        texture->height = 512;
        texture->base_depth = depth;
        ++source_cache.decoded_images;
        source_cache.decoded_bytes += (uint64)source_width * 512 * sizeof(uint16);
        texture->colors = (uint16 *)calloc(source_width * 512, sizeof(uint16));
        texture->assigned = (uint8 *)calloc(source_width * 512 / 8, 1);
        XASSERT(texture->colors != NULL && texture->assigned != NULL);
        texture->next = source_cache.textures[0];
        source_cache.textures[0] = texture;
        texture->base_next = source_cache.base_textures;
        source_cache.base_textures = texture;
    }
    for (y = 0; y < height; ++y)
        for (x = 0; x < width; ++x)
        {
            uint32 target = (min_v + y - region->v) * source_width + min_u + x - region->u;
            texture->colors[target] = decoded[y * width + x];
            texture->assigned[target >> 3] |= (uint8)(1u << (target & 7));
        }
    texture->classes |= classes;
    free(decoded);
    return texture;
}

void hd_texture_sources_native(const uint16 *vram)
{
    XASSERT(source_cache.banks == NULL && source_cache.owners == NULL);
    source_cache.vram = vram;
}

HD_SOURCE_TEXTURE *hd_texture_source_lookup(uint16 page, uint16 clut, uint32 u, uint32 v, uint32 width, uint32 height, HD_SOURCE_REGION *region)
{
    HD_SOURCE_NAMED *named, *best = NULL;
    uint32 depth = (page >> 7) & 3;
    page &= 0x19f;
    if (depth >= 2)
        clut = 0;
    for (named = source_cache.named; named != NULL; named = named->next)
        if (named->page == page && named->clut == clut && u >= named->u && v >= named->v && u + width <= named->u + named->width && v + height <= named->v + named->height && (best == NULL || named->width * named->height < best->width * best->height))
            best = named;
    if (best == NULL)
        return NULL;
    *region = (HD_SOURCE_REGION){0, (sint32)best->u, (sint32)best->v, best->width, best->height};
    return best->texture;
}

HD_SOURCE_TEXTURE *hd_texture_source_named(uint16 page, uint16 clut, uint32 u, uint32 v, uint32 width, uint32 height, const uint16 *vram)
{
    HD_SOURCE_NAMED *named;
    HD_SOURCE_TEXTURE *texture;
    uint32 x, y, classes = 0, hash = 2166136261u, bytes = width * height * 2;
    uint32 depth = (page >> 7) & 3, shift = depth == 0 ? 2 : depth == 1 ? 1 : 0;
    uint16 *colors;
    XASSERT(source_cache.banks == NULL && vram != NULL && width != 0 && height != 0);
    page &= 0x19f;
    if (depth >= 2)
        clut = 0;
    for (named = source_cache.named; named != NULL; named = named->next)
        if (named->page == page && named->clut == clut && named->u == u && named->v == v && named->width == width && named->height == height)
            return named->texture;
    colors = (uint16 *)malloc(bytes);
    XASSERT(colors != NULL);
    for (y = 0; y < height; ++y)
        for (x = 0; x < width; ++x)
        {
            uint32 tx = u + x, ty = v + y;
            uint16 word = vram[(((((page >> 4) & 1) * 256) + ty) & 511) * 1024 + (((page & 15) * 64 + (tx >> shift)) & 1023)];
            if (depth < 2)
            {
                uint32 palette = depth == 0 ? (word >> ((tx & 3) * 4)) & 15 : (word >> ((tx & 1) * 8)) & 255;
                word = vram[((clut >> 6) & 511) * 1024 + (((clut & 63) * 16 + palette) & 1023)];
            }
            colors[y * width + x] = word;
            classes |= word == 0 ? 1u : (word & 32768) != 0 ? 4u : 2u;
        }
    hash = (hash ^ width) * 16777619u;
    hash = (hash ^ height) * 16777619u;
    for (x = 0; x < bytes; ++x)
        hash = (hash ^ ((uint8 *)colors)[x]) * 16777619u;
    for (texture = source_cache.textures[hash & 1023]; texture != NULL; texture = texture->next)
        if (texture->assigned == NULL && texture->hash == hash && texture->width == width && texture->height == height && memcmp(texture->colors, colors, bytes) == 0)
            break;
    if (texture != NULL)
        free(colors);
    else
    {
        texture = (HD_SOURCE_TEXTURE *)calloc(1, sizeof(*texture));
        XASSERT(texture != NULL);
        texture->width = width;
        texture->height = height;
        texture->classes = classes;
        texture->hash = hash;
        texture->colors = colors;
        texture->next = source_cache.textures[hash & 1023];
        source_cache.textures[hash & 1023] = texture;
        ++source_cache.decoded_images;
        source_cache.decoded_bytes += bytes;
    }
    named = (HD_SOURCE_NAMED *)calloc(1, sizeof(*named));
    XASSERT(named != NULL);
    *named = (HD_SOURCE_NAMED){source_cache.named, page, clut, u, v, width, height, texture};
    source_cache.named = named;
    return texture;
}

HD_SOURCE_TEXTURE *hd_texture_source_material(uint16 page, uint16 clut, uint16 min_u, uint16 min_v, uint16 max_u, uint16 max_v, HD_SOURCE_REGION *region)
{
    HD_SOURCE_TEXTURE *texture;
    if (source_cache.vram == NULL)
        return NULL;
    ++source_cache.material_refs;
    texture = hd_texture_source_lookup(page, clut, min_u, min_v, max_u - min_u + 1, max_v - min_v + 1, region);
    if (texture != NULL)
        return texture;
    if (hd_texture_source_region(page, min_u, min_v, max_u, max_v, region))
    {
        ++source_cache.stream_refs;
        return hd_texture_source_bind(region, page, clut, source_cache.vram);
    }
    texture = hd_texture_source_bind_base(page, clut, min_u, min_v, max_u, max_v, source_cache.vram, region);
    if (texture != NULL)
        ++source_cache.base_refs;
    else
        ++source_cache.unresolved_refs;
    return texture;
}

HD_SOURCE_PIXELS hd_texture_source_pixels(const HD_SOURCE_TEXTURE *texture)
{
    XASSERT(texture != NULL);
    return (HD_SOURCE_PIXELS){texture->width, texture->height, texture->classes, texture->colors};
}

void hd_texture_source_semitransparent(HD_SOURCE_TEXTURE *texture)
{
    if (texture != NULL)
        texture->semi_used = 1;
}

static void fill_base_edges(HD_SOURCE_TEXTURE *texture)
{
    uint32 size = texture->width * texture->height, used = 0, cursor = 0, index;
    uint32 *queue;
    if (texture->assigned == NULL)
        return;
    queue = (uint32 *)malloc(size * sizeof(*queue));
    XASSERT(queue != NULL);
    for (index = 0; index < size; ++index)
        if (texture->assigned[index >> 3] & (1u << (index & 7)))
            queue[used++] = index;
    XASSERT(used != 0);
    while (cursor < used && used < size)
    {
        uint32 at = queue[cursor++], side;
        for (side = 0; side < 4; ++side)
        {
            uint32 target;
            if ((side == 0 && at % texture->width == 0) || (side == 1 && at % texture->width + 1 == texture->width) || (side == 2 && at < texture->width) || (side == 3 && at >= size - texture->width))
                continue;
            target = side == 0 ? at - 1 : side == 1 ? at + 1 : side == 2 ? at - texture->width : at + texture->width;
            if (!(texture->assigned[target >> 3] & (1u << (target & 7))))
            {
                texture->assigned[target >> 3] |= (uint8)(1u << (target & 7));
                texture->colors[target] = texture->colors[at];
                queue[used++] = target;
            }
        }
    }
    free(queue);
    free(texture->assigned);
    texture->assigned = NULL;
}

void hd_texture_sources_build_gpu(void)
{
    uint32 bucket;
    HD_SOURCE_BANK *bank;
    XASSERT(source_cache.vram == NULL && source_cache.banks == NULL);
    for (bucket = 0; bucket < 1024; ++bucket)
    {
        HD_SOURCE_TEXTURE *texture;
        for (texture = source_cache.textures[bucket]; texture != NULL; texture = texture->next)
        {
            uint32 layers;
            fill_base_edges(texture);
            texture->mode = !texture->semi_used ? 0 : (texture->classes & 6) == 6 ? 3 : (texture->classes & 4) ? 2 : 1;
            layers = texture->mode == 3 ? 2 : 1;
            for (bank = source_cache.banks; bank != NULL; bank = bank->next)
                if (bank->width == texture->width && bank->height == texture->height && bank->layers + layers <= 128)
                    break;
            if (bank == NULL)
            {
                bank = (HD_SOURCE_BANK *)calloc(1, sizeof(*bank));
                XASSERT(bank != NULL);
                bank->width = texture->width;
                bank->height = texture->height;
                bank->next = source_cache.banks;
                source_cache.banks = bank;
                ++source_cache.gpu_banks;
            }
            texture->bank = bank;
            texture->layer = bank->layers;
            bank->layers += layers;
        }
    }
    for (bank = source_cache.banks; bank != NULL; bank = bank->next)
    {
        bank->texture = vid_tex_create_array_fmt(bank->width, bank->height, bank->layers, VID_RGBA5551);
        XASSERT(bank->texture != NULL);
        vid_tex_set_mip_levels(bank->texture, 3);
        source_cache.gpu_bytes += (uint64)bank->width * bank->height * bank->layers * 2;
    }
    for (bucket = 0; bucket < 1024; ++bucket)
    {
        HD_SOURCE_TEXTURE *texture;
        for (texture = source_cache.textures[bucket]; texture != NULL; texture = texture->next)
        {
            uint32 pixel, size = texture->width * texture->height;
            uint16 *target = (uint16 *)vid_tex_data(texture->bank->texture) + texture->layer * size;
            for (pixel = 0; pixel < size; ++pixel)
            {
                uint32 word = texture->colors[pixel];
                uint16 color = (uint16)(((word & 31) << 11) | (((word >> 5) & 31) << 6) | (((word >> 10) & 31) << 1) | (word != 0));
                if (texture->mode == 3)
                {
                    target[pixel] = (word & 32768) == 0 ? color : 0;
                    target[size + pixel] = (word & 32768) != 0 ? color : 0;
                }
                else
                    target[pixel] = color;
            }
            free(texture->colors);
            texture->colors = NULL;
        }
    }
    for (bank = source_cache.banks; bank != NULL; bank = bank->next)
    {
        vid_tex_commit(bank->texture);
        vid_tex_static(bank->texture);
    }
}

void hd_texture_sources_visit_banks(void (*visitor)(VID_TEXTURE *, void *), void *context)
{
    HD_SOURCE_BANK *bank;
    for (bank = source_cache.banks; bank != NULL; bank = bank->next)
        visitor(bank->texture, context);
}

void hd_texture_source_bank_size(VID_TEXTURE *texture, uint32 *width, uint32 *height)
{
    HD_SOURCE_BANK *bank;
    for (bank = source_cache.banks; bank != NULL; bank = bank->next)
        if (bank->texture == texture)
        {
            *width = bank->width;
            *height = bank->height;
            return;
        }
    XASSERT(0);
}

HD_SOURCE_GPU hd_texture_source_gpu(const HD_SOURCE_TEXTURE *texture)
{
    XASSERT(texture != NULL && texture->bank != NULL);
    return (HD_SOURCE_GPU){texture->bank->texture, texture->layer, texture->layer + (texture->mode == 3), texture->mode, texture->classes, texture->width, texture->height};
}

void hd_texture_sources_seal(void)
{
    uint32 index;
    for (index = 0; index < 1024; ++index)
        while (source_cache.bindings[index] != NULL)
        {
            HD_SOURCE_BINDING *binding = source_cache.bindings[index];
            source_cache.bindings[index] = binding->next;
            free(binding);
        }
    for (index = 0; index < source_cache.count; ++index)
        source_cache.allocations[index].payload = NULL;
    free(source_cache.owners);
    source_cache.owners = NULL;
    source_cache.vram = NULL;
}
