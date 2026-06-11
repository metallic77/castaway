/*
 * sound.c - YM2149 sound emulation for Castaway
 * Uses emu2149 by Mitsutaka Okazaki (MIT licence)
 * Atari ST YM2149 master clock: 2 MHz (8 MHz / 4)
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "SDL.h"
#include "../emu2149.h"
#include "config.h"
#include "st.h"

#define YM_CLOCK    2000000
#define SAMPLE_RATE 44100
#define AUDIO_BUF   2048

static PSG            *psg        = NULL;
static SDL_AudioDeviceID audio_dev = 0;
static uint8           selected_reg = 0;

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    int16_t *out     = (int16_t *)stream;
    int      samples = len / sizeof(int16_t);
    int      i;
    (void)userdata;
    for (i = 0; i < samples; i++)
        out[i] = PSG_calc(psg);
}

void sound_init(void)
{
    SDL_AudioSpec want, have;

    psg = PSG_new(YM_CLOCK, SAMPLE_RATE);
    if (!psg) { fprintf(stderr, "PSG_new failed\n"); return; }

    PSG_setClockDivider(psg, 0);
    PSG_setVolumeMode(psg, 1);    /* 1 = YM2149 volume table */
    PSG_reset(psg);

    memset(&want, 0, sizeof(want));
    want.freq     = SAMPLE_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = 1;
    want.samples  = AUDIO_BUF;
    want.callback = audio_callback;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_dev == 0) {
        fprintf(stderr, "SDL audio failed: %s\n", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(audio_dev, 0);
    fprintf(stderr, "YM2149 sound initialised at %d Hz\n", have.freq);
}

void sound_write_select(uint8 reg)
{
    selected_reg = reg & 0xf;
    if (psg) PSG_writeIO(psg, 0, selected_reg);
}

void sound_write_data(uint8 val)
{
    if (psg) PSG_writeIO(psg, 1, val);
}

void sound_reset(void)
{
    if (psg) PSG_reset(psg);
}
