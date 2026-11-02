#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "game.h"

typedef struct AUDIO_SAMPLE
{
    uint32 spu_address;
    uint32 rate;
    uint32 size_hint;
} AUDIO_SAMPLE;

typedef struct AUDIO_VOICE
{
    SpuVoiceAttr attr;
    uint32 sample;
    uint32 reserved;
    uint32 volume;
    uint32 pitch;
    uint32 paired;
    const VEC3 *pos;
    const VEC3 *velocity;
} AUDIO_VOICE;

typedef char AUDIO_VOICE_ATTR_SIZE_CHECK[sizeof(SpuVoiceAttr) == 64 ? 1 : -1];
typedef char AUDIO_VOICE_ATTR_LAYOUT_CHECK[offsetof(SpuVoiceAttr, pitch) == 20 && offsetof(SpuVoiceAttr, addr) == 28 ? 1 : -1];
typedef char AUDIO_VOLUME_SIZE_CHECK[sizeof(SpuVolume) == 4 ? 1 : -1];
typedef char AUDIO_COMMON_ATTR_SIZE_CHECK[sizeof(SpuCommonAttr) == 40 ? 1 : -1];

static SpuMallocRecord memory_recs[21];
static SpuCommonAttr audio_common;
static SpuVolume saved_volume[24];

static AUDIO_VOICE audio_voices[24];
static AUDIO_SAMPLE audio_samples[16];
AUDIO_STATE audio_state = {.initial_mix = 0x00FF00FF};

AUDIO_OPTIONS audio_options = {.music_volume = 3172, .effects_volume = 2048};
AUDIO_XA_STATE audio_xa;
const char *const audio_xa_files[2] = {"\\PRES.XA;1", "\\GAME.XA;1"};
static const uint32 xa_lens[16] = {4490, 4527, 5687, 4865, 4593, 4198, 3889, 5511, 5093, 4490, 4503, 4303, 4413, 4593, 4225, 4455};

typedef struct AUDIO_CAR_SAMPLE_SOURCE
{
    const char *engine;
    const char *revup;
} AUDIO_CAR_SAMPLE_SOURCE;

static const char *const menu_sample_files[16] = {
    "SOUND\\MENU2.VAG", "SOUND\\MENU3.VAG", "SOUND\\SKID2.VAG", "SOUND\\CRASH1.VAG", "SOUND\\BOTTOM3.VAG", "SOUND\\HHIT4.VAG", "SOUND\\LHIT2.VAG", "SOUND\\SCRAPE2.VAG", "SOUND\\SIREN4.VAG", "SOUND\\GEARCRNS.VAG", "SOUND\\HARLEY.VAG", "SOUND\\HARLEY.VAG", "SOUND\\HORN11.VAG", "SOUND\\TRK_03B.VAG", "SOUND\\REV5.VAG", "SOUND\\CHIRP1.VAG",
};

static const char *const short_song_files[13] = {
    "SOUND\\DEF_BEAT.VAG", "SOUND\\FALLING.VAG", "SOUND\\GENIUS1.VAG", "SOUND\\GENIUS2.VAG", "SOUND\\LEID1.VAG", "SOUND\\LEID2.VAG", "SOUND\\MEGA1.VAG", "SOUND\\MEGA2.VAG", "SOUND\\MICRO.VAG", "SOUND\\MICRO2.VAG", "SOUND\\TRK_03A.VAG", "SOUND\\TRK_03B.VAG", "SOUND\\MICRO2.VAG",
};

static const AUDIO_CAR_SAMPLE_SOURCE car_sample_srcs[39] = {
    {"VIPER", "REV7"}, {"98VETTE", "REV1"}, {"ASTON", "REV2"}, {"TVR", "REV6"}, {"JAG", "REV3"}, {"SALEEN", "REV5"}, {"Z28", "REV8"}, {"NISSAN", "REV4"}, {"NISSAN", "REV4"}, {"COBRA", "REV11"}, {"PONTIAC", "REV9"}, {"MUSTANG", "REV14"}, {"CHEVY", "REV10"}, {"STINGRAY", "REV16"}, {"CHEVY", "REV10"}, {"MOPAR", "REV12"}, {"MOPAR", "REV13"}, {"ASTON", "REV2"}, {"98VETTE", "REV9"}, {"TVR", "REV9"}, {"VIPER", "REV7"}, {"JAG", "REV3"}, {"98VETTE", "REV9"}, {"CHEVY", "REV7"}, {"98VETTE", "REV9"}, {"TVR", "REV6"}, {"MOPAR", "REV13"}, {"CHEVY", "REV9"}, {"MUSTANG", "REV14"}, {"Z28", "REV3"}, {"STINGRAY", "REV5"}, {"MUSTANG", "REV5"}, {"MUSTANG", "REV4"}, {"NISSAN", "REV4"}, {"MOPAR", "REV13"}, {"MOPAR", "REV1"}, {"MOPAR", "REV1"}, {"MOPAR", "REV1"}, {"MOPAR", "REV1"},
};

static uint32 upload_sample(const uint8 *src, uint32 file_size, AUDIO_SAMPLE *description);
static void update_voice(sint32 voice, sint32 pitch, sint32 pan, sint32 volume);
static sint32 find_voice(void);
static void update_voice_scaled(sint32 voice, sint32 pitch, sint32 pan, sint32 volume);
static void pan_pair(sint32 angle, sint32 voice, sint32 pitch, sint32 pan, sint32 volume);

sint32 audio_init(void)
{
    uint32 volume;
    CdlATV mix;
    uint32 idx;
    uint32 offset;

    FUNCTION_MARKER(0x800543E0, "SLUS_006.10");
    volume = audio_state.initial_mix;
    SpuInit();
    SpuInitMalloc(20, (char *)memory_recs);
    audio_common.mvol.left = 0x3FFC;
    audio_common.mvol.right = 0x3FFC;
    audio_common.mask = 3;
    SpuSetCommonAttr(&audio_common);
    mix.val0 = (uint8)volume;
    mix.val1 = (uint8)(volume >> 8);
    mix.val2 = (uint8)(volume >> 16);
    mix.val3 = (uint8)(volume >> 24);
    CdMix(&mix);
    for (idx = 0; idx < 24; ++idx)
    {
        offset = idx * 92;
        memset(&audio_voices[idx], 0, sizeof(audio_voices[idx]));
        audio_voices[(offset) / 92].attr.voice = (1 << idx);
        audio_voices[(offset) / 92].attr.volmode.left = (sint16)(0);
        audio_voices[(offset) / 92].attr.volmode.right = (sint16)(0);
        audio_voices[(offset) / 92].attr.mask = (159);
    }
    return 0;
}

sint32 audio_set_limit(sint32 limit)
{
    FUNCTION_MARKER(0x800542F8, "SLUS_006.10");
    if (limit > 4096)
        audio_state.limit = (4096);
    else if (limit < 0)
        audio_state.limit = (0);
    return 4096;
}

sint32 audio_load_menu_samples(uint8 *buffer, uint32 capacity)
{
    uint32 idx;
    uint32 size;
    const char *filename;

    FUNCTION_MARKER(0x80052840, "SLUS_006.10");
    for (idx = 0; idx < 16; ++idx)
    {
        filename = menu_sample_files[idx];
        size = archive_file_size(filename);
        XASSERT(!(size < 80));
        filename = menu_sample_files[idx];
        {
            sint32 operation_ok = !(archive_read_file(filename, buffer, capacity) < size);
            XASSERT(operation_ok);
        }
        if (idx >= 10 && idx <= 14)
            size += 0x2000;
        XASSERT(!(size > capacity));
        upload_sample(buffer, size, &audio_samples[idx]);
    }
    return 0;
}

static uint32 upload_sample(const uint8 *src, uint32 file_size, AUDIO_SAMPLE *description)
{
    uint32 bytes;
    uint32 transfer_bytes;
    uint32 address;
    uint32 sample_rate;

    FUNCTION_MARKER(0x800541AC, "SLUS_006.10");
    bytes = file_size - 80;
    address = (uint32)SpuMalloc((sint32)bytes);
    XASSERT(!(address == 0xFFFFFFFFu));
    SpuSetTransferMode(0);
    {
        sint32 operation_ok = !(SpuSetTransferStartAddr(address) == 0);
        XASSERT(operation_ok);
    }
    transfer_bytes = bytes > 0x7EFF0 ? 0x7EFF0 : bytes;
    SpuWrite(src + 80, transfer_bytes);
    {
        sint32 operation_ok = !(SpuIsTransferCompleted(1) != 1);
        XASSERT(operation_ok);
    }
    description->spu_address = (address);
    description->size_hint = ((bytes << 5) / 7);
    sample_rate = ((uint32)src[18] << 8) + src[19];
    description->rate = (sample_rate);
    return sample_rate;
}

void audio_stop_voice(uint32 voice)
{
    FUNCTION_MARKER(0x80054288, "SLUS_006.10");
    SpuSetKey(0, (uint32)1 << (voice & 31));
    if (audio_state.paired != 0)
        SpuSetKey(0, (uint32)1 << ((voice + 1) & 31));
}

static sint32 audio_play(sint32 sample, sint32 voice, sint32 volume, sint32 pan, sint32 pitch)
{
    uint32 voice_offset;
    uint32 sample_offset;
    uint32 rate;
    uint32 product;
    sint32 n;
    sint32 ratio;
    sint32 frequency;

    FUNCTION_MARKER(0x80054720, "SLUS_006.10");
    if (voice == -1)
    {
        voice = find_voice();
        if (voice == -1)
            return -1;
    }
    voice_offset = (uint32)voice * 92;
    audio_voices[(voice_offset) / 92].sample = ((uint32)sample);
    audio_voices[(voice_offset) / 92].pos = (0);
    audio_voices[(voice_offset) / 92].pitch = ((uint32)pitch);
    if (volume != -1)
        audio_voices[(voice_offset) / 92].volume = ((uint32)volume);
    else
        volume = (sint32)audio_voices[(voice_offset) / 92].volume;
    sample_offset = (uint32)sample * 12;
    rate = audio_samples[(sample_offset) / 12].rate;
    product = rate * (uint32)pitch;
    n = (sint32)product / 4096;
    ratio = fixed_divide_checked(44100, (sint32)rate);
    product = ((uint32)n * (uint32)ratio) << 12;
    frequency = (sint32)product / 44100;
    if (frequency >= 16384)
        frequency = 16383;
    SpuSetKey(0, audio_voices[(voice_offset) / 92].attr.voice);
    audio_voices[(voice_offset) / 92].attr.addr = (audio_samples[(sample_offset) / 12].spu_address);
    update_voice(voice, frequency, pan, volume);
    SpuSetKey(1, audio_voices[(voice_offset) / 92].attr.voice);
    return voice;
}

static void update_voice(sint32 voice, sint32 pitch, sint32 pan, sint32 volume)
{
    uint32 offset;
    uint32 sample;
    uint32 product;
    sint32 n;
    sint32 frequency;
    sint32 delta;
    uint32 left;
    uint32 right;

    FUNCTION_MARKER(0x800553DC, "SLUS_006.10");
    offset = (uint32)voice * 92;
    sample = audio_voices[(offset) / 92].sample;
    product = audio_samples[sample].rate * (uint32)pitch;
    n = (sint32)product / 4096;
    frequency = (sint32)((uint32)n << 12) / 44100;
    if (frequency >= 16384)
        frequency = 16383;
    audio_voices[(offset) / 92].attr.pitch = ((uint16)frequency);
    if (pan < 0)
    {
        if (pan < -4096)
            pan = -4096;
        left = volume;
        delta = (sint32)((uint32)volume * (uint32)pan) / 4096;
        right = (uint32)volume + (uint32)delta;
    }
    else
    {
        if (pan > 4096)
            pan = 4096;
        delta = (sint32)((uint32)volume * (uint32)pan) / 4096;
        left = (uint32)volume - (uint32)delta;
        right = volume;
    }
    audio_voices[(offset) / 92].attr.volume.left = (sint16)((uint16)(left << 2));
    audio_voices[(offset) / 92].attr.volume.right = (sint16)((uint16)(right << 2));
    SpuSetVoiceAttr(&audio_voices[(offset) / 92].attr);
}

static sint32 find_voice(void)
{
    uint32 stereo;
    uint32 cursor;
    uint32 step;
    uint32 idx;
    uint32 limit;

    FUNCTION_MARKER(0x800552AC, "SLUS_006.10");
    stereo = audio_state.paired;
    cursor = audio_state.voice_cursor;
    if ((sint32)cursor < 24)
        cursor = 24;
    if (stereo != 0)
    {
        step = 2;
        cursor &= 0xFFFE;
    }
    else
        step = 1;
    audio_state.voice_cursor = (cursor);
    if (audio_state.paired != 0)
        return -1;
    idx = audio_state.voice_cursor;
    if ((sint32)idx < 24)
    {
        do
        {
            if (audio_voices[idx].reserved == 0)
            {
                audio_state.voice_cursor = (idx + 1);
                return idx;
            }
            idx += step;
        } while ((sint32)idx < 24);
    }
    limit = audio_state.voice_cursor;
    if ((sint32)limit > 0)
    {
        idx = 0;
        do
        {
            if (audio_voices[idx].reserved == 0)
            {
                audio_state.voice_cursor = (idx + 1);
                return idx;
            }
            idx += step;
        } while ((sint32)idx < (sint32)limit);
    }
    return -1;
}

sint32 audio_play_positioned(sint32 sample, sint32 voice, uint32 pan, sint32 pitch, const VEC3 *pos, const VEC3 *velocity)
{
    uint32 coord;
    uint32 view;
    uint32 difference;
    uint32 offset;
    uint32 sample_offset;
    uint32 product;
    sint32 rate;
    sint32 step;
    sint32 frequency;
    uint32 next_voice;
    uint32 key;

    FUNCTION_MARKER(0x80054978, "SLUS_006.10");
    if (voice == -1)
    {
        voice = find_voice();
        if (voice == -1)
            return -1;
        coord = ((sint32)pos->bits.x >> 8);
        view = model_render.view.sdk.t[0];
        difference = view - coord;
        if ((sint32)difference < 0)
            difference = coord - view;
        if ((sint32)difference > 32767)
            return -1;
        coord = ((sint32)pos->bits.z >> 8);
        view = model_render.view.sdk.t[2];
        difference = view - coord;
        if ((sint32)difference < 0)
            difference = coord - view;
        if ((sint32)difference > 32767)
            return -1;
    }
    offset = (uint32)voice * 92;
    audio_voices[(offset) / 92].velocity = (velocity);
    sample_offset = (uint32)sample * 12;
    audio_voices[(offset) / 92].sample = ((uint32)sample);
    audio_voices[(offset) / 92].pos = (pos);
    audio_voices[(offset) / 92].volume = (pan);
    audio_voices[(offset) / 92].pitch = ((uint32)pitch);
    rate = (sint32)audio_samples[(sample_offset) / 12].rate;
    product = (uint32)rate * (uint32)pitch;
    step = (sint32)product / 4096;
    if (rate == 0)
        xport_mips_break(7);
    product = (uint32)step * (uint32)(44100 / rate);
    frequency = (sint32)(product << 12) / 44100;
    if (frequency >= 16384)
        frequency = 16383;
    SpuSetKey(0, audio_voices[(offset) / 92].attr.voice);
    audio_voices[(offset) / 92].attr.addr = (audio_samples[(sample_offset) / 12].spu_address);
    update_voice_scaled(voice, frequency, 0, 0);
    SpuSetKey(1, audio_voices[(offset) / 92].attr.voice);
    next_voice = (uint32)voice + 1;
    if (audio_state.paired != 0)
    {
        offset = next_voice * 92;
        key = audio_voices[(offset) / 92].attr.voice;
        audio_voices[(offset) / 92].sample = ((uint32)sample);
        audio_voices[(offset) / 92].volume = (0xFFFFF000);
        audio_voices[(offset) / 92].pitch = ((uint32)pitch);
        SpuSetKey(0, key);
        audio_voices[(offset) / 92].attr.addr = (audio_samples[(sample_offset) / 12].spu_address);
        update_voice_scaled((sint32)next_voice, frequency, 0, 0);
        SpuSetKey(1, audio_voices[(offset) / 92].attr.voice);
    }
    return voice;
}

sint32 audio_start_engine(const VEC3 *pos, const VEC3 *velocity)
{
    FUNCTION_MARKER(0x80052F1C, "SLUS_006.10");
    return audio_play_positioned(8, 16, 0, 5120, pos, velocity);
}

uint32 audio_set_engine_volume(uint32 val)
{
    uint32 stereo;

    FUNCTION_MARKER(0x80052EF4, "SLUS_006.10");
    stereo = audio_state.paired;
    audio_voices[16].volume = (val);
    if (stereo != 0)
        audio_voices[17].volume = (val);
    return stereo;
}

sint32 audio_play_request(sint32 sample, sint32 voice, sint32 volume, sint32 pan, sint32 pitch)
{
    FUNCTION_MARKER(0x8005490C, "SLUS_006.10");
    return audio_play(sample, voice, volume, pan, pitch);
}

static uint32 play_scaled(sint32 sample, sint32 voice, sint32 volume, sint32 pan, sint32 pitch)
{
    uint32 product;

    FUNCTION_MARKER(0x80054930, "SLUS_006.10");
    product = (uint32)volume * audio_options.effects_volume;
    volume = (sint32)product / 4096;
    return (uint32)audio_play(sample, voice, volume, pan, pitch);
}

static void update_voice_scaled(sint32 voice, sint32 pitch, sint32 pan, sint32 volume)
{
    uint32 product;
    uint32 offset;
    uint32 sample;
    sint32 scaled_volume;
    sint32 n;
    sint32 frequency;
    sint32 delta;
    uint32 left;
    uint32 right;

    FUNCTION_MARKER(0x8005552C, "SLUS_006.10");
    product = (uint32)volume * audio_options.effects_volume;
    scaled_volume = (sint32)product / 4096;
    offset = (uint32)voice * 92;
    sample = audio_voices[(offset) / 92].sample;
    product = audio_samples[sample].rate * (uint32)pitch;
    n = (sint32)product / 4096;
    frequency = (sint32)((uint32)n << 12) / 44100;
    if (frequency >= 16384)
        frequency = 16383;
    audio_voices[(offset) / 92].attr.pitch = ((uint16)frequency);
    if (pan < 0)
    {
        if (pan < -4096)
            pan = -4096;
        left = scaled_volume;
        delta = (sint32)((uint32)scaled_volume * (uint32)pan) / 4096;
        right = (uint32)scaled_volume + (uint32)delta;
    }
    else
    {
        if (pan > 4096)
            pan = 4096;
        delta = (sint32)((uint32)scaled_volume * (uint32)pan) / 4096;
        left = (uint32)scaled_volume - (uint32)delta;
        right = scaled_volume;
    }
    audio_voices[(offset) / 92].attr.volume.left = (sint16)((uint16)(left << 2));
    audio_voices[(offset) / 92].attr.volume.right = (sint16)((uint16)(right << 2));
    SpuSetVoiceAttr(&audio_voices[(offset) / 92].attr);
}

static uint32 reserve_voice(sint32 voice)
{
    uint32 offset = (uint32)voice * 92;
    uint32 paired;

    FUNCTION_MARKER(0x80054674, "SLUS_006.10");
    paired = audio_state.paired;
    audio_voices[(offset) / 92].reserved = (1);
    if (paired != 0)
    {
        offset = ((uint32)voice + 1) * 92;
        audio_voices[(offset) / 92].reserved = (1);
    }
    return offset;
}

uint32 audio_mute_voices(void)
{
    uint32 idx;
    AUDIO_VOICE *voice;
    SpuVolume *saved;
    uint32 val;

    FUNCTION_MARKER(0x800544F8, "SLUS_006.10");
    for (idx = 0; idx < 24; ++idx)
    {
        voice = &audio_voices[idx];
        saved = &saved_volume[idx];
        val = (uint16)voice->attr.volume.left;
        saved->left = (sint16)(val);
        val = (uint16)voice->attr.volume.right;
        saved->right = (sint16)(val);
        voice->attr.volume.left = (sint16)(0);
        voice->attr.volume.right = (sint16)(0);
        SpuSetVoiceAttr(&voice->attr);
    }
    return 0;
}

void audio_restore_voices(void)
{
    uint32 idx;
    AUDIO_VOICE *voice;
    SpuVolume *saved;
    uint32 val;

    FUNCTION_MARKER(0x800545A4, "SLUS_006.10");
    for (idx = 0; idx < 24; ++idx)
    {
        voice = &audio_voices[idx];
        saved = &saved_volume[idx];
        val = (uint16)saved->left;
        voice->attr.volume.left = (sint16)(val);
        val = (uint16)saved->right;
        voice->attr.volume.right = (sint16)(val);
        SpuSetVoiceAttr(&voice->attr);
    }
    val = audio_options.music_volume << 2;
    audio_voices[20].attr.volume.left = (sint16)(val);
    audio_voices[20].attr.volume.right = (sint16)(val);
    SpuSetVoiceAttr(&audio_voices[20].attr);
}

void audio_set_listener_pos(const VEC3 *pos)
{
    FUNCTION_MARKER(0x800542EC, "SLUS_006.10");
    audio_state.listener_pos = (pos);
}

void audio_set_listener_velocity(const VEC3 *velocity)
{
    FUNCTION_MARKER(0x800542E0, "SLUS_006.10");
    audio_state.listener_velocity = (velocity);
}

void audio_update_spatial(void)
{
    uint32 voice = 0, offset = 0;
    uint32 angle, pitch, pan;
    uint32 x, y, z, dx, dy, dz, val, other;
    const VEC3 *pos;
    uint32 absolute_x, absolute_z, volume, numerator, denominator, proj;
    uint32 root, rotated_x, rotated_y, rotated_z;
    const VEC3 *velocity;
    uint32 dir_xy, dir_z;

    const sint32 *listener_pos;

    FUNCTION_MARKER(0x80054C58, "SLUS_006.10");
    SetRotMatrix(&model_render.view.sdk);
    do
    {
        pos = audio_voices[(offset) / 92].pos;
        if (pos != 0)
        {
            listener_pos = model_render.view.sdk.t;
            if (race_session.two_players != 0)
            {
                if (pos == &car_states[0].pos)
                    listener_pos = model_render.listener_view[0].sdk.t;
                else if (pos == &car_states[1].pos)
                    listener_pos = model_render.listener_view[1].sdk.t;
            }
            x = pos->bits.x;
            other = listener_pos[0];
            dx = other - (uint32)((sint32)x >> 8);
            y = pos->bits.y;
            val = listener_pos[1];
            z = pos->bits.z;
            other = listener_pos[2];
            dy = val - (uint32)((sint32)y >> 8);
            dz = other - (uint32)((sint32)z >> 8);
            val = (sint32)dx < 0 ? 0 - dx : dx;
            if ((sint32)val < 512)
                dx = (sint32)dx < 0 ? 0 - 512 : 512;
            val = (sint32)dy < 0 ? 0 - dy : dy;
            if ((sint32)val < 512)
                dy = (sint32)dy < 0 ? 0 - 512 : 512;
            val = (sint32)dz < 0 ? 0 - dz : dz;
            if ((sint32)val < 512)
                dz = (sint32)dz < 0 ? 0 - 512 : 512;
            absolute_x = (sint32)dx < 0 ? 0 - dx : dx;
            absolute_z = (sint32)dz < 0 ? 0 - dz : dz;
            volume = 0;
            if (absolute_x <= 0x7FFF && absolute_z <= 0x7FFF)
            {
                val = SquareRoot0((sint32)(absolute_x * absolute_x + absolute_z * absolute_z));
                val = (uint32)cln((sint32)(val << 12));
                volume = 2708 - (uint32)((sint32)val / 16);
                if ((sint32)volume < 0)
                    volume = 0;
                if ((sint32)volume >= 1025)
                    volume = 1024;
                val = audio_voices[(offset) / 92].volume;
                val *= volume;
                volume = ((sint32)val / 1024);
            }
            if (volume != 0)
            {
                dir_xy = (uint16)(0 - dx) | ((uint32)(uint16)(0 - dy) << 16);
                dir_z = (uint16)(0 - dz);
                xport_gte_write_data(0, dir_xy);
                xport_gte_write_data(1, dir_z);
                xport_gte_execute(0x486012);
                rotated_x = xport_gte_read_data(9);
                rotated_y = xport_gte_read_data(10);
                rotated_z = xport_gte_read_data(11);
                x = (sint16)rotated_z;
                y = (sint16)rotated_x;
                angle = (uint32)fixed_angle_from_vector((sint32)(x << 4), (sint32)(y << 4));
                if ((sint32)angle < 1025)
                    pan = 4096 - angle * 4;
                else if ((sint32)angle < 2049)
                    pan = (1024 - angle) << 2;
                else
                    pan = (angle - 3072) << 2;
                if (audio_voices[(offset) / 92].velocity != 0)
                {
                    root = SquareRoot0((sint32)(dx * dx + dz * dz));
                    velocity = audio_state.listener_velocity;
                    val = velocity->bits.x;
                    numerator = dx * (uint32)((sint32)val >> 8);
                    val = velocity->bits.z;
                    denominator = root + 1;
                    numerator += dz * (uint32)((sint32)val >> 8);
                    proj = (uint32)fixed_divide_checked((sint32)numerator, (sint32)denominator);
                    velocity = audio_voices[(offset) / 92].velocity;
                    val = velocity->bits.x;
                    numerator = dx * (uint32)((sint32)val >> 8);
                    val = velocity->bits.z;
                    numerator += dz * (uint32)((sint32)val >> 8);
                    other = (uint32)fixed_divide_checked((sint32)numerator, (sint32)denominator);
                    denominator = proj + 4096;
                    numerator = other + 4096;
                    if (denominator == 0)
                        denominator = 1;
                    numerator <<= 12;
                    val = (uint32)fixed_divide_checked((sint32)numerator, (sint32)denominator);
                    pitch = 128;
                    if ((sint32)val >= 128)
                    {
                        other = audio_voices[(offset) / 92].pitch;
                        val *= other;
                        pitch = ((sint32)val / 4096);
                    }
                }
                else
                    pitch = audio_voices[(offset) / 92].pitch;
            }
            else
            {
                angle = 0;
                pan = 0;
                pitch = audio_voices[(offset) / 92].pitch;
            }
            if (audio_state.paired != 0 && audio_voices[(offset) / 92].paired != 0)
            {
                pan_pair((sint32)angle, (sint32)voice, (sint32)pitch, (sint32)pan, (sint32)volume);
                offset += 92;
                ++voice;
            }
            else
                update_voice_scaled((sint32)voice, (sint32)pitch, 200, (sint32)volume);
        }
        ++voice;
        offset += 92;
    } while ((sint32)voice < 24);
}

uint32 audio_bind_engine_src(uint32 slot)
{
    uint32 enabled;
    const VEC3 *pos;
    uint32 velocity;

    FUNCTION_MARKER(0x80052E94, "SLUS_006.10");
    enabled = audio_state.paired;
    pos = &car_states[slot].pos;
    velocity = 0x80098634 + slot * 864;
    audio_voices[16].pos = (pos);
    audio_voices[16].velocity = (&car_states[slot].velocity);
    if (enabled != 0)
    {
        audio_voices[17].pos = (pos);
        audio_voices[17].velocity = (&car_states[slot].velocity);
    }
    return velocity;
}

uint32 audio_set_car_engine_gain(sint32 amount, uint32 slot)
{
    uint32 result, offset, duplicate, mirror;
    const VEC3 *src_vector;
    sint32 scaled, volume, pitch;

    FUNCTION_MARKER(0x80053158, "SLUS_006.10");
    if (race_session.two_players != 0)
    {
        if ((sint32)slot >= 2)
            return 0x55550000;
    }
    else if (slot != race_view.primary_car)
        return 0x55550000;
    scaled = amount / 3;
    volume = scaled < 0 ? 0 : (scaled > 4096 ? 4096 : scaled);
    pitch = scaled < 3072 ? 3072 : (scaled > 4096 ? 4096 : scaled);
    result = slot * 2 + 8;
    mirror = audio_state.paired;
    offset = result * 92;
    audio_voices[(offset) / 92].volume = ((uint32)volume);
    audio_voices[(offset) / 92].pitch = ((uint32)pitch);
    if (mirror != 0)
    {
        duplicate = (slot * 2 + 9) * 92;
        result = duplicate;
        audio_voices[(duplicate) / 92].pitch = ((uint32)pitch);
        audio_voices[(duplicate) / 92].volume = ((uint32)volume);
        src_vector = audio_voices[(offset) / 92].pos;
        audio_voices[(duplicate) / 92].pos = (src_vector);
        src_vector = audio_voices[(offset) / 92].velocity;
        audio_voices[(duplicate) / 92].velocity = (src_vector);
    }
    return result;
}

void audio_fade(uint32 volume)
{
    uint32 initial = audio_options.music_volume;

    FUNCTION_MARKER(0x80054108, "SLUS_006.10");
    audio_options.music_volume = volume;
    audio_update_cd_volume();
    volume <<= 2;
    audio_voices[20].attr.volume.left = (sint16)volume;
    audio_voices[20].attr.volume.right = (sint16)volume;
    SpuSetVoiceAttr(&audio_voices[20].attr);
    audio_options.music_volume = initial;
}

sint32 audio_update_cd_volume(void)
{
    sint32 volume;
    uint32 initial;
    CdlATV mix;

    FUNCTION_MARKER(0x800556AC, "SLUS_006.10");
    volume = (sint32)audio_options.music_volume;
    initial = audio_state.cd_mix;
    volume /= 16;
    if (volume < 0)
        volume = 0;
    else if (volume > 255)
        volume = 255;
    mix.val0 = (uint8)volume;
    mix.val1 = (uint8)(initial >> 8);
    mix.val2 = (uint8)volume;
    mix.val3 = (uint8)(initial >> 24);
    CdMix(&mix);
    return 1;
}

sint32 audio_play_xa(sint32 track)
{
    uint32 duration;
    const char *filename;
    sint32 channel;
    sint32 sector;
    sint32 volume;
    uint32 initial;
    CdlFILE file;
    CdlLOC pos;
    uint8 mode;
    uint8 filter[2];
    CdlATV mix;

    FUNCTION_MARKER(0x800558C8, "SLUS_006.10");
#if AP_WEB
    {
        char path[64];
        sint32 result;
        SpuCommonAttr sound = {0};
        audio_xa.track = ((uint8)track);
        sound.mask = SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR | SPU_COMMON_CDMIX;
        sound.cd.volume.left = sound.cd.volume.right = 0x3FFF;
        sound.cd.mix = SPU_ON;
        SpuSetCommonAttr(&sound);
        snprintf(path, sizeof(path), "MUSIC/%s_%d.XA", track < 8 ? "PRES" : "GAME", track < 8 ? track : track - 8);
        result = xport_xa_play_file(path, 1);
        XASSERT(!(!result));
        CdControl(CdlDemute, 0, 0);
        audio_update_cd_volume();
        return result;
    }
#endif
    duration = xa_lens[(uint32)track];
    audio_xa.track = ((uint8)track);
    duration <<= 3;
    channel = track;
    if (track < 8)
        filename = audio_xa_files[0];
    else
    {
        filename = audio_xa_files[1];
        channel = (sint32)((uint32)track - 8);
    }
    CdInit();
    if (asset_mount_xa_file(filename, &file) == 0)
    {
        (void)(filename);
        return 0;
    }
    sector = CdPosToInt(&file.pos);
    mode = 0xCC;
    CdControlB(14, &mode, 0);
    audio_xa.end = ((uint32)sector + duration);
    audio_xa.begin = ((uint32)sector);
    audio_xa.cur = ((uint32)sector);
    audio_xa.channel = ((uint32)channel);
    filter[0] = 1;
    filter[1] = (uint8)channel;
    CdControlF(13, filter);
    CdIntToPos((sint32)audio_xa.begin, &pos);
    CdControlF(27, (uint8 *)&pos);
    volume = (sint32)audio_options.music_volume;
    initial = audio_state.cd_mix;
    volume /= 16;
    if (volume < 0)
        volume = 0;
    else if (volume > 255)
        volume = 255;
    mix.val0 = (uint8)volume;
    mix.val1 = (uint8)(initial >> 8);
    mix.val2 = (uint8)volume;
    mix.val3 = (uint8)(initial >> 24);
    CdMix(&mix);
    return 1;
}

sint32 audio_poll_xa(void)
{
#if AP_WEB
    // the file player owns stream position and looping
    return 0;
#else
    uint32 counter;
    uint8 response[8];
    CdlLOC pos = {0};
    uint8 parameters[2];
    CdlLOC seek_pos = {0};
    sint32 sector;
    sint32 end;
    sint32 begin;
    uint32 mode;

    FUNCTION_MARKER(0x800557F0, "SLUS_006.10");
    counter = audio_xa.poll_counter;
    audio_xa.poll_counter = (counter + 1);
    if ((sint32)counter < 11)
        return 1;
    audio_xa.poll_counter = (0);
    CdControl(0x10, 0, response);
    pos.minute = response[0];
    pos.second = response[1];
    pos.sector = response[2];
    pos.track = 0;
    sector = CdPosToInt(&pos);
    end = (sint32)audio_xa.end;
    audio_xa.cur = ((uint32)sector);
    if (end >= sector)
    {
        begin = (sint32)audio_xa.begin;
        if (sector >= begin)
            return 0;
    }
    begin = (sint32)audio_xa.begin;
    mode = audio_xa.channel;
    audio_xa.end = ((uint32)end);
    parameters[0] = 1;
    audio_xa.begin = ((uint32)begin);
    audio_xa.cur = ((uint32)begin);
    audio_xa.channel = (mode);
    parameters[1] = mode;
    CdControlF(0x0D, parameters);
    begin = (sint32)audio_xa.begin;
    CdIntToPos(begin, &seek_pos);
    return CdControlF(0x1B, (uint8 *)&seek_pos);
#endif
}

uint32 audio_replay_xa_track(sint32 track)
{
    FUNCTION_MARKER(0x80055A38, "SLUS_006.10");
    return (uint32)audio_play_xa(track);
}

uint32 audio_pause_xa(void)
{
    FUNCTION_MARKER(0x80055A58, "SLUS_006.10");
#if AP_WEB
    return (uint32)CdControl(CdlPause, 0, 0);
#else
    return (uint32)CdControl(11, 0, 0);
#endif
}

uint32 audio_resume_xa(void)
{
    uint32 template_word;
    sint32 volume;
    CdlATV mix;

    FUNCTION_MARKER(0x80055A80, "SLUS_006.10");
#if AP_WEB
    CdControl(CdlPlay, 0, 0);
#else
    CdControl(12, 0, 0);
#endif
    volume = (sint32)audio_options.music_volume;
    template_word = audio_state.cd_mix;
    template_word = audio_state.cd_mix;
    volume /= 16;
    if (volume < 0)
        volume = 0;
    else if (volume >= 256)
        volume = 255;
    mix.val0 = (uint8)volume;
    mix.val1 = (uint8)(template_word >> 8);
    mix.val2 = (uint8)volume;
    mix.val3 = (uint8)(template_word >> 24);
    CdMix(&mix);
    return 1;
}

sint32 audio_stop_xa(void)
{
    uint8 mode = 128;

    FUNCTION_MARKER(0x800557C0, "SLUS_006.10");
#if AP_WEB
    CdControl(CdlStop, 0, 0);
#endif
    return CdControlB(14, &mode, 0);
}

void audio_load_car_samples(sint32 car)
{
    const uint8 *selection;
    sint32 car_id;
    uint32 desc;
    const char *suffix;
    uint32 file_size;
    uint32 loaded_size;
    uint8 *buffer;
    uint32 sound;
    uint32 transfer_address;
    uint32 payload;
    char filename[256];

    FUNCTION_MARKER(0x80052924, "SLUS_006.10");
    buffer = (uint8 *)malloc(0x20000);
    XASSERT(!(buffer == NULL));
    selection = &session_car_choices.cars[(uint32)car].model[session_options.model_variant];
    car_id = (sint8)*selection;
    if (car_id < 0)
    {
        free(buffer);
        return;
    }
    (void)(car);
    desc = car_id;
    XASSERT(!(desc >= sizeof(car_sample_srcs) / sizeof(car_sample_srcs[0])));
    suffix = car_sample_srcs[desc].engine;
    sprintf(filename, "SOUND\\%s.VAG", suffix);
    file_size = archive_file_size(filename);
    XASSERT(!(file_size < 80 || file_size > 0x20000));
    loaded_size = archive_read_file(filename, buffer, 0x20000);
    XASSERT(!(loaded_size < file_size || loaded_size > 0x20000));
    sound = ((uint32)car + 10) * 12;
    transfer_address = audio_samples[(sound) / 12].spu_address;
    SpuSetTransferMode(0);
    {
        sint32 operation_ok = !(SpuSetTransferStartAddr(transfer_address) == 0);
        XASSERT(operation_ok);
    }
    payload = file_size - 80;
    SpuWrite(buffer + 80, payload);
    {
        sint32 operation_ok = !(SpuIsTransferCompleted(1) != 1);
        XASSERT(operation_ok);
    }
    audio_samples[(sound) / 12].size_hint = ((sint32)(payload << 5) / 7);
    if (car == 0)
    {
        (void)(0);
        suffix = car_sample_srcs[desc].revup;
        sprintf(filename, "SOUND\\%s.VAG", suffix);
        file_size = archive_file_size(filename);
        XASSERT(!(file_size < 80 || file_size > 0x20000));
        loaded_size = archive_read_file(filename, buffer, 0x20000);
        XASSERT(!(loaded_size < file_size || loaded_size > 0x20000));
        transfer_address = audio_samples[14].spu_address;
        SpuSetTransferMode(0);
        {
            sint32 operation_ok = !(SpuSetTransferStartAddr(transfer_address) == 0);
            XASSERT(operation_ok);
        }
        payload = file_size - 80;
        SpuWrite(buffer + 80, payload);
        {
            sint32 operation_ok = !(SpuIsTransferCompleted(1) != 1);
            XASSERT(operation_ok);
        }
        audio_samples[14].size_hint = ((sint32)(payload << 5) / 7);
    }
    free(buffer);
}

void audio_load_short_song(sint32 song)
{
    uint32 entry;
    const char *filename;
    uint32 file_size;
    uint32 loaded_size;
    uint8 *buffer;
    uint32 transfer_address;
    uint32 payload;

    FUNCTION_MARKER(0x80052B74, "SLUS_006.10");
    buffer = (uint8 *)malloc(0x4B000);
    XASSERT(!(buffer == NULL));
    (void)(song);
    entry = song;
    XASSERT(!(entry >= sizeof(short_song_files) / sizeof(short_song_files[0])));
    filename = short_song_files[entry];
    file_size = archive_file_size(filename);
    filename = short_song_files[entry];
    XASSERT(!(file_size < 80 || file_size > 0x4B000));
    loaded_size = archive_read_file(filename, buffer, 0x4B000);
    XASSERT(!(loaded_size < file_size || loaded_size > 0x4B000));
    transfer_address = audio_samples[13].spu_address;
    SpuSetTransferMode(0);
    {
        sint32 operation_ok = !(SpuSetTransferStartAddr(transfer_address) == 0);
        XASSERT(operation_ok);
    }
    payload = file_size - 80;
    SpuWrite(buffer + 80, payload);
    {
        sint32 operation_ok = !(SpuIsTransferCompleted(1) != 1);
        XASSERT(operation_ok);
    }
    audio_samples[13].size_hint = ((sint32)(payload << 5) / 7);
    free(buffer);
}

uint32 audio_update_split_engines(uint32 view)
{
    uint32 val0, val1, val2, val3, val, other, dx, dz;
    uint32 count, car = 0, car_offset = 0;
    uint32 output, companion, sel, sel_offset, result, saved_speed;
    uint32 override;

    FUNCTION_MARKER(0x80053AAC, "SLUS_006.10");
    val0 = model_render.view.words[0];
    val1 = model_render.view.words[1];
    val2 = model_render.view.words[2];
    val3 = model_render.view.words[3];
    model_render.listener_view[view].words[0] = val0;
    model_render.listener_view[view].words[1] = val1;
    model_render.listener_view[view].words[2] = val2;
    model_render.listener_view[view].words[3] = val3;
    val0 = model_render.view.words[4];
    val1 = model_render.view.sdk.t[0];
    val2 = model_render.view.sdk.t[1];
    val3 = model_render.view.sdk.t[2];
    model_render.listener_view[view].words[4] = val0;
    model_render.listener_view[view].words[5] = val1;
    model_render.listener_view[view].words[6] = val2;
    model_render.listener_view[view].words[7] = val3;
    count = race_control.car_count;
    if ((sint32)count > 0)
    {
        do
        {
            if (car == view)
            {
                val = car_states[(car_offset) / 864u].pos.bits.x;
                other = model_render.view.sdk.t[0];
                val = ((sint32)val >> 8);
                dx = other - val;
                if ((sint32)dx < 0)
                    dx = val - other;
                val = car_states[(car_offset) / 864u].pos.bits.z;
                other = model_render.view.sdk.t[2];
                val = ((sint32)val >> 8);
                dz = other - val;
                if ((sint32)dz < 0)
                    dz = val - other;
                val = 0x10000;
                if ((sint32)dx <= 0x7FFF && (sint32)dz <= 0x7FFF)
                    val = SquareRoot0((sint32)(dx * dx + dz * dz));
            }
            count = race_control.car_count;
            ++car;
            car_offset += 864;
        } while ((sint32)car < (sint32)count);
        count = race_control.car_count;
    }
    car_offset = view * 864;
    val0 = (sint16)car_states[car_offset / 864u].pedal;
    val1 = car_states[car_offset / 864u].engine_speed;
    val = 256 - val0;
    other = val1 - 4000;
    val = other - (uint32)((sint32)val / 4);
    val <<= 12;
    output = view * 184;
    audio_voices[(output) / 92].pitch = ((uint32)((sint32)val / 5000) + 4096);
    val0 = car_states[0].engine_speed;
    val1 = (sint16)car_states[car_offset / 864u].pedal;
    val = (256 - val1) * 2;
    other = ((sint32)val0 / 4);
    if ((sint32)(other + 3072) < 4097)
        val = other - (val - 3072);
    else
        val = 4096 - val;
    other = audio_state.paired;
    audio_voices[(output) / 92].volume = (val);
    if (other != 0)
    {
        val = audio_voices[(output) / 92].pitch;
        companion = (view * 2 + 1) * 92;
        audio_voices[(companion) / 92].volume = (0xFFFFF000);
        audio_voices[(companion) / 92].pitch = (val);
    }
    sel = view;
    output = (view + 2) * 184;
    companion = ((view + 2) * 2 + 1) * 92;
    if ((sint32)sel < 6)
    {
        override = race_control.start_hold;
        if (override != 0)
        {
            sel_offset = sel * 864;
            saved_speed = car_states[sel_offset / 864u].engine_speed;
            car_states[sel_offset / 864u].engine_speed = 2000;
        }
        sel_offset = sel * 864;
        val = car_states[sel_offset / 864u].engine_speed;
        val = (val - 4000) << 12;
        audio_voices[(output) / 92].pitch = ((uint32)((sint32)val / 5000) + 4096);
        val = car_states[sel_offset / 864u].engine_speed;
        val = (uint32)((sint32)val / 4) + 3072;
        if ((sint32)val > 4096)
            val = 4096;
        other = audio_state.paired;
        audio_voices[(output) / 92].volume = (val);
        if (other != 0)
        {
            val = audio_voices[(output) / 92].pitch;
            audio_voices[(companion) / 92].volume = (0xFFFFF000);
            audio_voices[(companion) / 92].pos = (&car_states[(sel_offset) / 864u].pos);
            audio_voices[(companion) / 92].velocity = (&car_states[(sel_offset) / 864u].velocity);
            audio_voices[(companion) / 92].pitch = (val);
        }
        result = race_control.start_hold;
        audio_voices[(output) / 92].pos = (&car_states[(sel_offset) / 864u].pos);
        audio_voices[(output) / 92].velocity = (&car_states[(sel_offset) / 864u].velocity);
        if (override != 0 && result != 0)
            car_states[sel_offset / 864u].engine_speed = saved_speed;
        return result;
    }
    other = audio_state.paired;
    audio_voices[(output) / 92].pitch = (4096);
    audio_voices[(output) / 92].volume = (4096);
    if (other != 0)
    {
        val = audio_voices[(output) / 92].pitch;
        audio_voices[(companion) / 92].volume = (0xFFFFF000);
        sel_offset = sel * 864;
        audio_voices[(companion) / 92].pos = (&car_states[(sel_offset) / 864u].pos);
        audio_voices[(companion) / 92].velocity = (&car_states[(sel_offset) / 864u].velocity);
        audio_voices[(companion) / 92].pitch = (val);
    }
    sel_offset = sel * 864;
    result = 0x80098634 + sel_offset;
    audio_voices[(output) / 92].pos = (&car_states[(sel_offset) / 864u].pos);
    audio_voices[(output) / 92].velocity = (&car_states[sel_offset / 864u].velocity);
    return result;
}

uint32 audio_update_engines(void)
{
    uint32 distances[14];
    uint32 sel_cars[14];
    uint32 idx;
    uint32 count;
    uint32 collected;
    uint32 car_offset;
    uint32 entry;
    uint32 coord;
    uint32 view;
    uint32 x;
    uint32 z;
    uint32 val;
    uint32 pass;
    uint32 cur;
    uint32 next;
    uint32 val0;
    uint32 val1;
    uint32 limit;
    uint32 fixed_limit;
    uint32 sel;
    uint32 focus;
    uint32 channel;
    uint32 mirror;
    uint32 pitch;
    uint32 volume;
    uint32 enabled;
    uint32 override;
    uint32 restore;
    uint32 old_speed;
    const VEC3 *src_pos;
    sint32 random;
    sint32 throttle;

    FUNCTION_MARKER(0x80053348, "SLUS_006.10");
    val = audio_state.engine_effect_delay;
    if (val < 255)
        audio_state.engine_effect_delay = (uint8)(val + 1);
    count = race_control.car_count;
    collected = 0;
    idx = 0;
    car_offset = 0;
    if ((sint32)count > 0)
    {
        do
        {
            focus = race_view.primary_car;
            if (idx != focus)
            {
                entry = collected;
                sel_cars[entry] = idx;
                coord = (uint32)(car_states[(car_offset) / 864u].pos.x >> 8);
                view = model_render.view.sdk.t[0];
                x = view - coord;
                if ((sint32)x < 0)
                    x = coord - view;
                coord = (uint32)(car_states[(car_offset) / 864u].pos.z >> 8);
                view = model_render.view.sdk.t[2];
                z = view - coord;
                if ((sint32)z < 0)
                    z = coord - view;
                val = 0x10000;
                if ((sint32)x <= 32767 && (sint32)z <= 32767)
                    val = (uint32)SquareRoot0((sint32)(x * x + z * z));
                distances[entry] = val;
                ++collected;
            }
            count = race_control.car_count;
            ++idx;
            car_offset += 864;
        } while ((sint32)idx < (sint32)count);
        count = race_control.car_count;
    }
    fixed_limit = count - 2;
    for (pass = 0; pass < 6; ++pass)
    {
        if ((sint32)fixed_limit > 1)
        {
            limit = race_control.car_count - 2;
            entry = 1;
            idx = 1;
            do
            {
                cur = distances[entry];
                next = distances[entry + 1];
                if ((sint32)next < (sint32)cur)
                {
                    val0 = sel_cars[entry + 1];
                    val1 = sel_cars[entry];
                    sel_cars[entry] = val0;
                    sel_cars[entry + 1] = val1;
                    next = distances[entry + 1];
                    cur = distances[entry];
                    distances[entry] = next;
                    distances[entry + 1] = cur;
                }
                ++entry;
                ++idx;
            } while ((sint32)idx < (sint32)limit);
        }
    }
    if (race_control.start_hold != 0 && (sint32)race_control.car_count >= 6)
    {
        random = randPSX();
        if (random % 1000 < 100)
        {
            random = randPSX();
            src_pos = &car_states[(uint32)(random % 6)].pos;
            audio_play_positioned(14, 18, 2048, 4096, src_pos, 0);
        }
    }
    focus = race_view.primary_car;
    car_offset = focus * 864;
    if ((sint32)car_states[car_offset / 864u].engine_speed < 1000 && (sint16)car_states[car_offset / 864u].pedal == 256 && audio_state.engine_effect_delay >= 41)
    {
        src_pos = &car_states[car_offset / 864u].pos;
        audio_play_positioned(14, 18, 2048, 4096, src_pos, 0);
        audio_state.engine_effect_delay = 0;
    }
    focus = race_view.primary_car;
    car_offset = focus * 864;
    throttle = (sint16)car_states[car_offset / 864u].pedal;
    val = car_states[car_offset / 864u].engine_speed;
    val = (val - 4000 - (uint32)((256 - throttle) / 4)) << 12;
    pitch = (uint32)((sint32)val / 5000) + 4096;
    audio_voices[0].pitch = (pitch);
    val = car_states[car_offset / 864u].engine_speed;
    throttle = (sint16)car_states[car_offset / 864u].pedal;
    val0 = (uint32)(256 - throttle) * 2;
    val1 = car_truncate_shift(val, 2);
    volume = (sint32)(val1 + 3072) < 4097 ? val1 - (val0 - 3072) : 4096 - val0;
    enabled = audio_state.paired;
    audio_voices[0].volume = (volume);
    if (enabled != 0)
    {
        pitch = audio_voices[0].pitch;
        audio_voices[1].volume = ((uint32)-4096);
        audio_voices[1].pitch = (pitch);
    }
    count = race_control.car_count;
    idx = 1;
    while ((sint32)count >= 5 ? (sint32)idx < 4 : (sint32)idx < (sint32)count)
    {
        sel = sel_cars[idx - 1];
        channel = idx * 184;
        mirror = (idx * 2 + 1) * 92;
        car_offset = sel * 864;
        if ((sint32)sel < 6)
        {
            override = race_control.start_hold;
            if (override != 0)
            {
                old_speed = car_states[car_offset / 864u].engine_speed;
                car_states[car_offset / 864u].engine_speed = 2000;
            }
            val = car_states[car_offset / 864u].engine_speed;
            val = (val - 4000) << 12;
            pitch = (uint32)((sint32)val / 5000) + 4096;
            audio_voices[(channel) / 92].pitch = (pitch);
            val = car_states[car_offset / 864u].engine_speed;
            volume = car_truncate_shift(val, 2) + 3072;
            if ((sint32)volume >= 4097)
                volume = 4096;
            enabled = audio_state.paired;
            audio_voices[(channel) / 92].volume = (volume);
            if (enabled != 0)
            {
                pitch = audio_voices[(channel) / 92].pitch;
                audio_voices[(mirror) / 92].volume = ((uint32)-4096);
                audio_voices[(mirror) / 92].pos = (&car_states[(car_offset) / 864u].pos);
                audio_voices[(mirror) / 92].velocity = (&car_states[(car_offset) / 864u].velocity);
                audio_voices[(mirror) / 92].pitch = (pitch);
            }
            restore = race_control.start_hold;
            audio_voices[(channel) / 92].pos = (&car_states[(car_offset) / 864u].pos);
            audio_voices[(channel) / 92].velocity = (&car_states[(car_offset) / 864u].velocity);
            if (override != 0 && restore != 0)
                car_states[car_offset / 864u].engine_speed = old_speed;
        }
        else
        {
            enabled = audio_state.paired;
            audio_voices[(channel) / 92].pitch = (4096);
            audio_voices[(channel) / 92].volume = (4096);
            if (enabled != 0)
            {
                pitch = audio_voices[(channel) / 92].pitch;
                audio_voices[(mirror) / 92].volume = ((uint32)-4096);
                audio_voices[(mirror) / 92].pos = (&car_states[(car_offset) / 864u].pos);
                audio_voices[(mirror) / 92].velocity = (&car_states[(car_offset) / 864u].velocity);
                audio_voices[(mirror) / 92].pitch = (pitch);
            }
            audio_voices[(channel) / 92].pos = (&car_states[(car_offset) / 864u].pos);
            audio_voices[(channel) / 92].velocity = (&car_states[(car_offset) / 864u].velocity);
        }
        count = race_control.car_count;
        ++idx;
    }
    return idx * 4;
}

uint32 audio_play_horn_at(const VEC3 *pos, const VEC3 *velocity)
{
    FUNCTION_MARKER(0x80052F58, "SLUS_006.10");
    return (uint32)audio_play_positioned(12, 12, 4096, 4096, pos, velocity);
}

uint32 audio_play_gear_grind_at(const VEC3 *pos, const VEC3 *velocity)
{
    FUNCTION_MARKER(0x80052FEC, "SLUS_006.10");
    return (uint32)audio_play_positioned(9, 14, 4096, 4096, pos, velocity);
}

uint32 audio_play_random_horn(void)
{
    sint32 val;

    FUNCTION_MARKER(0x80052F94, "SLUS_006.10");
    val = randPSX();
    return (uint32)audio_play(12, 12, val % 1024 + 1024, 0, 4096);
}

sint32 audio_play_menu_move(void)
{
    FUNCTION_MARKER(0x80053030, "SLUS_006.10");
    return audio_play_request(0, 23, 4096, 0, 4096);
}

sint32 audio_play_menu_accept(void)
{
    FUNCTION_MARKER(0x800530F0, "SLUS_006.10");
    return audio_play_request(1, 23, 4096, 0, 5120);
}

sint32 audio_play_menu_high(void)
{
    FUNCTION_MARKER(0x80053064, "SLUS_006.10");
    return audio_play_request(0, 23, 4096, 0, 6144);
}

uint32 audio_play_gear_shift(void)
{
    sint32 random_val;

    FUNCTION_MARKER(0x80053098, "SLUS_006.10");
    random_val = randPSX();
    return play_scaled(15, -1, random_val % 1024 + 1024, 0, 4096);
}

uint32 audio_play_menu_error(void)
{
    FUNCTION_MARKER(0x80053124, "SLUS_006.10");
    return (uint32)audio_play_request(1, 23, 4096, 0, 1024);
}

void audio_shutdown(void)
{
    FUNCTION_MARKER(0x80065990, "SLUS_006.10");
    SpuQuit();
}

uint32 audio_prepare_aux_voices(void)
{
    sint32 idx;
    sint32 voice = 8;

    FUNCTION_MARKER(0x800532A8, "SLUS_006.10");
    for (idx = 0; idx < 2; ++idx)
    {
        audio_play_positioned(2, voice, 0, 4096, &car_states[idx / 2].pos, 0);
        voice += 2;
    }
    return 0;
}

uint32 audio_prepare_race_voices(void)
{
    uint32 offset;
    sint32 idx;
    sint32 count;

    FUNCTION_MARKER(0x80052D80, "SLUS_006.10");
    offset = race_view.primary_car * 864;
    audio_play_positioned(10, 0, 0, 4096, &car_states[(offset) / 864u].pos, &car_states[(offset) / 864u].velocity);
    count = (sint32)race_control.car_count;
    idx = 1;
    for (;;)
    {
        if (count >= 5)
        {
            if (idx >= 4)
                break;
        }
        else if (idx >= count)
            break;
        audio_play_positioned(11, idx * 2, 0, 4096, &car_states[idx].pos, &car_states[idx].velocity);
        count = (sint32)race_control.car_count;
        ++idx;
    }
    for (idx = 0; idx < 21; ++idx)
        reserve_voice(idx);
    return 0;
}

void audio_key_off_all_voices(void)
{
    FUNCTION_MARKER(0x80054260, "SLUS_006.10");
    SpuSetKey(0, 0xFFFFFF);
}

static void pan_pair(sint32 angle, sint32 voice, sint32 pitch, sint32 pan, sint32 volume)
{
    uint32 half;
    uint32 distance;
    uint32 product;
    uint32 volume0;
    uint32 volume1;
    sint32 side;

    FUNCTION_MARKER(0x80055184, "SLUS_006.10");
    if (angle < 2048)
    {
        update_voice_scaled(voice, pitch, pan, volume);
        update_voice_scaled((sint32)((uint32)voice + 1), pitch, pan, 0);
        return;
    }
    side = pan < 0 ? -4096 : 4096;
    half = (volume / 2);
    distance = (uint32)angle - 3072;
    if ((sint32)distance < 0)
        distance = 3072 - (uint32)angle;
    product = car_truncate_shift(distance * half, 10);
    volume0 = half + product;
    half = (volume / 2);
    distance = (uint32)angle - 3072;
    if ((sint32)distance < 0)
        distance = 3072 - (uint32)angle;
    product = car_truncate_shift(distance * half, 10);
    volume1 = half - product;
    update_voice_scaled(voice, pitch, side, (sint32)volume0);
    update_voice_scaled((sint32)((uint32)voice + 1), pitch, -side, (sint32)(0 - volume1));
}
