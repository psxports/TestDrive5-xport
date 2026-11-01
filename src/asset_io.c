#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asset_io.h"
#include "psx_gpu.h"
#include "game.h"

typedef struct ARCHIVE_ENTRY
{
    char name[32];
    uint32 bytes;
    uint32 sector;
} ARCHIVE_ENTRY;

static ARCHIVE_ENTRY archive_entries[512];
static uint32 archive_count;
static char archive_path[512];
ASSET_STREAM asset_stream;
GAME_RESOURCE_STORAGE game_resource_storage;

void asset_register_gpu_storage(void)
{
    FUNCTION_MARKER(0x8007B41C, "SLUS_006.10");
    {
        sint32 operation_ok = !(!gpu_register_packet_range(&game_resource_storage, (uint32)sizeof(game_resource_storage)));
        XASSERT(operation_ok);
    }
}

static sint32 asset_use_extracted_files(void)
{
#if AP_WEB || AP_WIN
    // Temporarily enable extracted Windows assets for Web collection
    return 1;
#else
    return 0;
#endif
}

static void named_data_path(char *path, const char *name)
{
    size_t idx = 5;

    memcpy(path, "DATA/", 5);
    while (*name == '\\' || *name == '/')
        ++name;
    while (*name != 0 && *name != ';')
    {
        XASSERT(!(idx + 1 >= 512));
        path[idx++] = *name == '\\' ? '/' : *name;
        ++name;
    }
    path[idx] = 0;
}

static uint32 open_asset(const char *path, FILE **file)
{
    long len;

    *file = xport_fopen(path, "rb");
    XASSERT(!(*file == 0));
    {
        sint32 operation_ok = !(fseek(*file, 0, SEEK_END) != 0 || (len = ftell(*file)) < 0 || fseek(*file, 0, SEEK_SET) != 0);
        XASSERT(operation_ok);
    }
    return len;
}

static uint32 asset_file_size(const char *filename)
{
    char path[512];
    FILE *file;
    uint32 bytes;

    named_data_path(path, filename);
    bytes = open_asset(path, &file);
    fclose(file);
    return bytes;
}

static uint32 asset_read_file(const char *filename, void *dst, uint32 capacity)
{
    char path[512];
    FILE *file;
    uint32 bytes;
    size_t count;

    named_data_path(path, filename);
    bytes = open_asset(path, &file);
    XASSERT(!(bytes > capacity));
    count = xport_fread(dst, 1, bytes, file);
    fclose(file);
    XASSERT(!(count != bytes));
    return bytes;
}

static void asset_bind_stream(uint32 slot, const char *filename)
{
    XASSERT(!(slot >= 2));
    named_data_path(asset_stream.paths[slot], filename);
}

static void read_data_range(const char *path, uint32 offset, uint32 bytes, void *dst)
{
    FILE *file;
    long len = 0;
    size_t count;

    file = xport_fopen(path, "rb");
    XASSERT(!(file == 0));
    {
        sint32 operation_ok = !(fseek(file, 0, SEEK_END) != 0 || (len = ftell(file)) < 0);
        XASSERT(operation_ok);
    }
    {
        sint32 operation_ok = !((uint64)offset + bytes > (uint64)len || offset > 0x7FFFFFFF || fseek(file, (long)offset, SEEK_SET) != 0);
        XASSERT(operation_ok);
    }
    count = xport_fread(dst, 1, bytes, file);
    XASSERT(!(count != bytes));
    fclose(file);
}

static void asset_read_stream(uint32 slot, uint32 offset, uint32 bytes, void *dst)
{
    XASSERT(!(slot >= 2 || asset_stream.paths[slot][0] == 0));
    read_data_range(asset_stream.paths[slot], offset, bytes, dst);
}

static void asset_read_archive_sectors(uint32 sector, uint32 sectors, void *dst, uint32 capacity)
{
    uint64 offset = (uint64)sector * 2048;
    uint64 bytes = (uint64)sectors * 2048;

    XASSERT(!(archive_path[0] == 0));
    XASSERT(!(offset > 0x7FFFFFFF || bytes > 0xFFFFFFFF));
    XASSERT(!(bytes > capacity));
    read_data_range(archive_path, (uint32)offset, (uint32)bytes, dst);
}

typedef struct XA_FILE
{
    FILE *file;
    uint32 first_sector;
    uint32 sectors;
} XA_FILE;

static sint32 xa_read_sector(void *context, uint32 sector, uint8 buffer[2352], uint32 *sector_bytes)
{
    XA_FILE *src = (XA_FILE *)context;
    uint32 relative;

    if (sector < src->first_sector)
        return 0;
    relative = sector - src->first_sector;
    if (relative >= src->sectors || fseek(src->file, (long)(relative * 2352u), SEEK_SET) != 0 || xport_fread(buffer, 1, 2352, src->file) != 2352)
        return 0;
    *sector_bytes = 2352;
    return 1;
}

static void xa_close(void *context)
{
    XA_FILE *src = (XA_FILE *)context;

    fclose(src->file);
    free(src);
}

sint32 asset_mount_xa_file(const char *filename, CdlFILE *result)
{
    char path[512];
    uint8 header[16];
    XA_FILE *owner;
    XPORT_CD_SOURCE src;
    CdlLOC pos;
    FILE *file;
    long bytes;
    sint32 first_sector;
    uint32 idx;
    const char *name;

    named_data_path(path, filename);
    file = xport_fopen(path, "rb");
    if (file == NULL)
        return 0;
    if (fseek(file, 0, SEEK_END) != 0 || (bytes = ftell(file)) <= 0 || bytes % 2352 != 0 || fseek(file, 0, SEEK_SET) != 0 || xport_fread(header, 1, sizeof(header), file) != sizeof(header))
        goto failed;
    if (header[0] != 0 || header[11] != 0 || header[15] != 2)
        goto failed;
    for (idx = 1; idx < 11; ++idx)
        if (header[idx] != 0xFF)
            goto failed;
    for (idx = 12; idx < 15; ++idx)
        if ((header[idx] & 15) > 9 || (header[idx] >> 4) > 9)
            goto failed;
    if (header[13] >= 0x60 || header[14] >= 0x75)
        goto failed;
    pos.minute = header[12];
    pos.second = header[13];
    pos.sector = header[14];
    pos.track = 0;
    first_sector = CdPosToInt(&pos);
    if (first_sector < 0)
        goto failed;
    owner = (XA_FILE *)malloc(sizeof(*owner));
    if (owner == NULL)
        goto failed;
    owner->file = file;
    owner->first_sector = (uint32)first_sector;
    owner->sectors = (uint32)bytes / 2352;
    memset(&src, 0, sizeof(src));
    src.context = owner;
    src.sectors = owner->first_sector + owner->sectors;
    src.read_sector = xa_read_sector;
    src.close = xa_close;
    if (!xport_cd_mount_source(&src))
    {
        free(owner);
        goto failed;
    }
    memset(result, 0, sizeof(*result));
    result->pos = pos;
    result->size = owner->sectors * 2048;
    name = strrchr(path, '/');
    name = name != NULL ? name + 1 : path;
    for (idx = 0; idx < sizeof(result->name) - 1 && name[idx] != 0; ++idx)
        result->name[idx] = name[idx];
    return 1;

failed:
    fclose(file);
    return 0;
}

sint32 archive_open(const char *filename)
{
    ARCHIVE_ENTRY directory[512];
    uint32 idx;

    FUNCTION_MARKER(0x8007B838, "SLUS_006.10");
    (void)(filename);
    if (asset_use_extracted_files())
        return 0;
    named_data_path(archive_path, filename);
    read_data_range(archive_path, 0, sizeof(directory), directory);
    for (idx = 0; idx < 512; ++idx)
    {
        const ARCHIVE_ENTRY *rec = directory + idx;
        ARCHIVE_ENTRY *entry = archive_entries + idx;

        XASSERT(!(rec->bytes != 0 && memchr(rec->name, 0, sizeof(rec->name)) == NULL));
        *entry = *rec;
        entry->sector += 10;
        if (entry->bytes != 0)
            ++archive_count;
    }
    XASSERT(!(archive_count > 512));
    return 0;
}

uint32 archive_read_file(const char *filename, void *dst, uint32 capacity)
{
    uint32 sel;
    uint32 idx, sectors;
    sint32 found = 0;

    FUNCTION_MARKER(0x8007BA58, "SLUS_006.10");
    if (asset_use_extracted_files())
        return asset_read_file(filename, dst, capacity);
    if ((sint32)archive_count > 0)
    {
        for (idx = 0; (sint32)idx < (sint32)archive_count; ++idx)
        {
            if (strcmp(filename, archive_entries[idx].name) == 0)
            {
                sel = idx;
                found = 1;
                break;
            }
        }
    }
    XASSERT(!(found == 0));
    XASSERT(!(sel >= 512));
    sectors = (archive_entries[sel].bytes + 2047) >> 11;
    asset_read_archive_sectors(archive_entries[sel].sector, sectors, dst, capacity);
    return archive_entries[sel].bytes;
}

uint32 archive_file_size(const char *filename)
{
    uint32 sel;
    uint32 idx = 0;
    sint32 found = 0;

    FUNCTION_MARKER(0x8007BD44, "SLUS_006.10");
    if (asset_use_extracted_files())
        return asset_file_size(filename);
    if ((sint32)archive_count > 0)
    {
        for (;;)
        {
            if (strcmp(filename, archive_entries[idx].name) == 0)
            {
                sel = idx;
                found = 1;
                break;
            }
            ++idx;
            if ((sint32)idx >= (sint32)archive_count)
                break;
        }
    }
    XASSERT(!(found == 0));
    XASSERT(!(sel >= 512));
    return archive_entries[sel].bytes;
}

uint32 archive_aligned_size(const char *filename)
{
    uint32 sel;
    uint32 idx = 0;
    sint32 found = 0;

    FUNCTION_MARKER(0x8007BC84, "SLUS_006.10");
    if (asset_use_extracted_files())
        return asset_file_size(filename);
    if ((sint32)archive_count > 0)
    {
        for (;;)
        {
            if (strcmp(filename, archive_entries[idx].name) == 0)
            {
                sel = idx;
                found = 1;
                break;
            }
            ++idx;
            if ((sint32)idx >= (sint32)archive_count)
                break;
        }
    }
    XASSERT(!(found == 0));
    XASSERT(!(sel >= 512));
    return ((archive_entries[sel].bytes + 2047) >> 11) << 11;
}

static uint32 archive_find_file(const char *filename)
{
    sint32 count;
    uint32 idx = 0;
    uint32 result;

    FUNCTION_MARKER(0x8007B7B8, "SLUS_006.10");
    count = (sint32)archive_count;
    result = count;
    if (count > 0)
    {
        for (;;)
        {
            if (strcmp(filename, archive_entries[idx].name) == 0)
                return idx;
            count = (sint32)archive_count;
            ++idx;
            if ((sint32)idx >= count)
                break;
        }
        result = 0;
    }
    XASSERT(0);
    return result;
}

static void stream_complete_empty(void)
{
    FUNCTION_MARKER(0x8005A840, "SLUS_006.10");
}

uint32 archive_bind_stream(uint32 slot, const char *filename)
{
    uint32 idx;
    STREAM_REQUEST *request;

    FUNCTION_MARKER(0x8005A860, "SLUS_006.10");
    XASSERT(!(slot >= 2));
    request = asset_stream.requests + slot;
    if (asset_use_extracted_files())
    {
        asset_bind_stream(slot, filename);
        request->flags = 1;
        request->sector = 0;
        request->first_sector = 0;
        request->sector_count = 0;
        request->bytes = asset_file_size(filename);
        request->dst = NULL;
        request->dst_owner = NULL;
        request->complete = stream_complete_empty;
        return slot * 24;
    }
    idx = archive_find_file(filename);
    request->flags = 1;
    request->sector = 0;
    XASSERT(!(idx >= 512));
    request->first_sector = archive_entries[idx].sector;
    request->sector_count = 0;
    request->bytes = archive_entries[idx].bytes;
    request->dst = NULL;
    request->dst_owner = NULL;
    request->complete = stream_complete_empty;
    return slot * 24;
}

STREAM_REQUEST *stream_clear_slot(uint32 slot)
{
    STREAM_REQUEST empty = {0};
    STREAM_REQUEST *request;

    FUNCTION_MARKER(0x8005A9F0, "SLUS_006.10");
    XASSERT(!(slot >= 2));
    request = asset_stream.requests + slot;
    *request = empty;
    return request;
}

STREAM_REQUEST *stream_init(uint32 reset_mode)
{
    STREAM_REQUEST empty = {0};
    uint32 idx;

    FUNCTION_MARKER(0x8005AA54, "SLUS_006.10");
    asset_stream.reset_mode = reset_mode;
    CdInit();
    CdSetDebug(0);
    for (idx = 0; idx < 2; ++idx)
        asset_stream.requests[idx] = empty;
    asset_stream.completion_mask = 0;
    asset_stream.cur_slot = 0;
    asset_stream.retries = 0;
    asset_stream.active = 0;
    return asset_stream.requests;
}

static uint32 stream_check_media(void)
{
    FUNCTION_MARKER(0x8005AAC8, "SLUS_006.10");
    // Named files do not poll disc status
    return 0;
}

static uint32 stream_reset(void)
{
    uint32 result;
    uint32 idx;

    FUNCTION_MARKER(0x8005ACA4, "SLUS_006.10");
    result = stream_check_media();
    if (result == 1)
        return result;
    asset_stream.retries = 0;
    asset_stream.active = 0;
    if (asset_stream.reset_mode == 0)
        track_stream_release_pending();
    else
        car_selection_cancel_load();
    for (idx = 0; idx < 2; ++idx)
    {
        STREAM_REQUEST *request = asset_stream.requests + idx;

        request->flags = 1;
        request->sector = 0;
        request->sector_count = 0;
        request->dst = NULL;
        request->dst_owner = NULL;
        request->complete = stream_complete_empty;
    }
    return idx;
}

uint32 stream_update(void)
{
    uint32 result;
    uint32 idx;
    STREAM_REQUEST *request;
    uint32 sector;
    uint32 count;
    void *dst;

    FUNCTION_MARKER(0x8005AE14, "SLUS_006.10");
    if (asset_stream.retries != 0)
        stream_reset();
    while ((sint32)asset_stream.retries >= 50)
    {
        VSync(0);
        stream_reset();
    }
    result = stream_check_media();
    if (result == 1)
        return result;
    if (asset_stream.active != 1)
        goto scan_start;
    if (asset_stream.completion_mask == 0xFFFFFFFB)
    {
        (void)(asset_stream.cur_slot);
        request = asset_stream.requests + asset_stream.cur_slot;
        XASSERT(!(request->complete == NULL));
        request->complete();
    }
    (void)(asset_stream.cur_slot);
    request = asset_stream.requests + asset_stream.cur_slot;
    request->flags &= (uint16)asset_stream.completion_mask;
    request = asset_stream.requests + asset_stream.cur_slot;
    if ((request->flags & 0x10) == 0)
        goto scan_start;
    asset_stream.active = 0;
    idx = asset_stream.cur_slot;
    goto process_request;

scan_start:
    asset_stream.active = 0;
    idx = 0;
scan_check:
    if ((sint32)idx >= 2)
    {
        result = asset_stream.active;
        if (result == 1)
            goto scan_start;
        return result;
    }
process_request:
    request = asset_stream.requests + idx;
    if ((request->flags & 0x10) != 0)
    {
        asset_stream.active = 1;
        asset_stream.cur_slot = idx;
        if ((request->flags & 2) != 0)
        {
            (void)(idx);
            asset_stream.completion_mask = 0xFFFFFFFD;
            return 1;
        }
        if ((request->flags & 4) != 0)
        {
            (void)(idx), (void)((const void *)request->dst);
            count = request->sector_count;
            dst = request->dst_owner != NULL ? *request->dst_owner : request->dst;
            XASSERT(!(count != 0 && dst == NULL));
            if (asset_use_extracted_files())
                asset_read_stream(idx, request->sector * 2048, count * 2048, dst);
            else
            {
                sector = request->sector + request->first_sector;
                asset_read_archive_sectors(sector, count, dst, count * 2048);
            }
            asset_stream.completion_mask = 0xFFFFFFFB;
            return 0xFFFFFFFB;
        }
        request->flags &= 0xFFEF;
    }
    ++idx;
    goto scan_check;
}

uint32 stream_queue_read(uint32 slot, void *dst, uint32 sector_count, STREAM_CALLBACK complete, uint16 sector, uint8 **dst_owner)
{
    STREAM_REQUEST *request;

    FUNCTION_MARKER(0x8005B300, "SLUS_006.10");
    if (stream_check_media() == 1)
        return 1;
    if (asset_stream.retries != 0)
        return 1;
    XASSERT(!(slot >= 2));
    request = asset_stream.requests + slot;
    request->flags |= 0x16;
    request->sector_count = sector_count;
    request->dst = dst_owner != NULL ? NULL : dst;
    request->dst_owner = dst_owner;
    request->complete = complete;
    request->sector = sector;
    return 0;
}
