#ifndef ASSET_IO_H
#define ASSET_IO_H

#include "psx.h"

typedef struct GAME_RESOURCE_STORAGE
{
    uint32 file_words[0x60000 / 4];
    uint32 scene_words[0xC0000 / 4];
} GAME_RESOURCE_STORAGE;

extern GAME_RESOURCE_STORAGE game_resource_storage;

void asset_register_gpu_storage(void);

typedef void (*STREAM_CALLBACK)(void);

typedef struct STREAM_REQUEST
{
    uint16 flags;
    uint16 sector;
    uint32 first_sector;
    uint32 sector_count;
    uint32 bytes;
    void *dst;
    uint8 **dst_owner;
    STREAM_CALLBACK complete;
} STREAM_REQUEST;

typedef struct ASSET_STREAM
{
    STREAM_REQUEST requests[2];
    char paths[2][512];
    uint32 active;
    uint32 cur_slot;
    uint32 completion_mask;
    uint32 retries;
    uint32 reset_mode;
} ASSET_STREAM;

extern ASSET_STREAM asset_stream;

STREAM_REQUEST *stream_clear_slot(uint32 slot);
STREAM_REQUEST *stream_init(uint32 reset_mode);
void asset_prepare_stream_cache(uint32 slot);
uint32 stream_update(void);
uint32 stream_queue_read(uint32 slot, void *dst, uint32 sector_count, STREAM_CALLBACK complete, uint16 sector, uint8 **dst_owner);

sint32 asset_mount_xa_file(const char *filename, CdlFILE *result);

sint32 archive_open(const char *filename);
uint32 archive_read_file(const char *filename, void *dst, uint32 capacity);
uint32 archive_file_size(const char *filename);
uint32 archive_aligned_size(const char *filename);
uint32 archive_bind_stream(uint32 slot, const char *filename);

#endif
