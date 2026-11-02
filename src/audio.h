#ifndef AUDIO_H
#define AUDIO_H

#include "psx.h"
#include "fixed_math.h"

typedef struct AUDIO_STATE
{
    uint32 paired;
    uint32 voice_cursor;
    uint32 initial_mix;
    uint32 race_song_counter;
    uint32 cd_mix;
    uint32 limit;
    uint8 engine_effect_delay;
    const VEC3 *listener_pos;
    const VEC3 *listener_velocity;
} AUDIO_STATE;

typedef struct AUDIO_OPTIONS
{
    uint32 music_volume;
    uint32 effects_volume;
    uint32 paired;
} AUDIO_OPTIONS;

typedef struct AUDIO_XA_STATE
{
    uint32 begin;
    uint32 end;
    uint32 cur;
    uint32 channel;
    uint32 poll_counter;
    uint8 track;
} AUDIO_XA_STATE;

extern AUDIO_OPTIONS audio_options;
extern AUDIO_XA_STATE audio_xa;
extern const char *const audio_xa_files[2];
extern AUDIO_STATE audio_state;

sint32 audio_init(void);
sint32 audio_set_limit(sint32 limit);
sint32 audio_load_menu_samples(uint8 *buffer, uint32 capacity);
void audio_stop_voice(uint32 voice);
sint32 audio_play_request(sint32 sample, sint32 voice, sint32 volume, sint32 pan, sint32 pitch);
sint32 audio_play_positioned(sint32 sample, sint32 voice, uint32 pan, sint32 pitch, const VEC3 *pos, const VEC3 *velocity);
sint32 audio_start_engine(const VEC3 *pos, const VEC3 *velocity);
uint32 audio_set_engine_volume(uint32 val);
uint32 audio_mute_voices(void);
void audio_restore_voices(void);
void audio_set_listener_pos(const VEC3 *pos);
void audio_set_listener_velocity(const VEC3 *velocity);
void audio_update_spatial(void);
uint32 audio_bind_engine_src(uint32 slot);
uint32 audio_set_car_engine_gain(sint32 amount, uint32 slot);

void audio_fade(uint32 volume);
sint32 audio_update_cd_volume(void);
sint32 audio_play_xa(sint32 track);
sint32 audio_poll_xa(void);

uint32 audio_replay_xa_track(sint32 track);
uint32 audio_pause_xa(void);
uint32 audio_resume_xa(void);
sint32 audio_stop_xa(void);

void audio_load_car_samples(sint32 car);
void audio_load_short_song(sint32 song);

uint32 audio_update_engines(void);

uint32 audio_update_split_engines(uint32 view);

uint32 audio_play_gear_grind_at(const VEC3 *pos, const VEC3 *velocity);

uint32 audio_play_random_horn(void);

uint32 audio_play_horn_at(const VEC3 *pos, const VEC3 *velocity);

sint32 audio_play_menu_move(void);

sint32 audio_play_menu_accept(void);

sint32 audio_play_menu_high(void);

uint32 audio_play_menu_error(void);

uint32 audio_play_gear_shift(void);

uint32 audio_prepare_aux_voices(void);

uint32 audio_prepare_race_voices(void);

void audio_key_off_all_voices(void);

void audio_shutdown(void);

#endif
