#include "audio.h"

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "miniaudio.h"

#define AUDIO_SPLITFLAP_PATH "splitflap.wav"
#define AUDIO_BASE_FLIPS_PER_SECOND 20.f

static ma_engine audio_engine;
static ma_sound audio_splitflap_sound;

void audio_init() {
    ma_result result;

    result = ma_engine_init(NULL, &audio_engine);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "error: cannot initialize the audio engine");
        exit(EXIT_FAILURE);
    }

    result = ma_sound_init_from_file(&audio_engine, AUDIO_SPLITFLAP_PATH, 0,
                                     NULL, NULL, &audio_splitflap_sound);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "error: cannot initialize %s", AUDIO_SPLITFLAP_PATH);
    }

    ma_sound_set_pitch(&audio_splitflap_sound,
                       g_config.splitflap_fps / AUDIO_BASE_FLIPS_PER_SECOND);
    ma_sound_set_looping(&audio_splitflap_sound, MA_TRUE);
}

void audio_cleanup() {
    ma_sound_uninit(&audio_splitflap_sound);
    ma_engine_uninit(&audio_engine);
}

void audio_start_splitflap() {
    ma_sound_seek_to_pcm_frame(&audio_splitflap_sound, 0);
    ma_sound_start(&audio_splitflap_sound);
}

void audio_stop_splitflap() { ma_sound_stop(&audio_splitflap_sound); }
