#ifndef AUDIO_H
#define AUDIO_H
#include "core.h"

enum {
    NOTE_C2, NOTE_Cs2, NOTE_D2, NOTE_Ds2, NOTE_E2, NOTE_F2, NOTE_Fs2, NOTE_G2, NOTE_Gs2, NOTE_A2, NOTE_As2, NOTE_B2,
    NOTE_C3, NOTE_Cs3, NOTE_D3, NOTE_Ds3, NOTE_E3, NOTE_F3, NOTE_Fs3, NOTE_G3, NOTE_Gs3, NOTE_A3, NOTE_As3, NOTE_B3,
    NOTE_C4, NOTE_Cs4, NOTE_D4, NOTE_Ds4, NOTE_E4, NOTE_F4, NOTE_Fs4, NOTE_G4, NOTE_Gs4, NOTE_A4, NOTE_As4, NOTE_B4,
    NOTE_C5, NOTE_Cs5, NOTE_D5, NOTE_Ds5, NOTE_E5, NOTE_F5, NOTE_Fs5, NOTE_G5, NOTE_Gs5, NOTE_A5, NOTE_As5, NOTE_B5,
    NOTE_C6, NOTE_Cs6, NOTE_D6, NOTE_Ds6, NOTE_E6, NOTE_F6, NOTE_Fs6, NOTE_G6, NOTE_Gs6, NOTE_A6, NOTE_As6, NOTE_B6,
    NOTE_C7, NOTE_Cs7, NOTE_D7, NOTE_Ds7, NOTE_E7, NOTE_F7, NOTE_Fs7, NOTE_G7, NOTE_Gs7, NOTE_A7, NOTE_As7, NOTE_B7
};
enum { INST_LEAD, INST_SQUARE, INST_THIN, INST_PLUCK, INST_SOFT, INST_ECHO };
enum { WAVE_TRI, WAVE_SAW, WAVE_SQUARE, WAVE_SOFTTRI };
enum { DRUM_KICK, DRUM_SNARE, DRUM_HAT, DRUM_OHAT, DRUM_CRASH, DRUM_TOM };

typedef struct { uint8_t fpt; const uint8_t *ch[4]; } song_t;   /* ch[i] may be 0 (unused) */

/* music commands */
#define N(note,len)   NOTE_##note, (len)
#define D(drum,len)   DRUM_##drum, (len)
#define R(len)        0xF0, (len)
#define INST(i)       0xF1, (i)
#define MUS_LOOP_POINT 0xFD
#define MUS_LOOP       0xFE
#define MUS_END        0xFF

/* lengths in ticks (1 tick = 16th note) */
#define L1 16
#define L2D 12
#define L2 8
#define L4D 6
#define L4 4
#define L8D 3
#define L8 2
#define L16 1

/* sound effects */
#define SFX_CH1 0
#define SFX_CH4 3
typedef struct { uint8_t ch; uint8_t prio; const uint8_t *data; } sfx_t;
#define SFX_TONE(frames, sweep, duty, env, note) (frames), (sweep), (duty), (env), NOTE_##note
#define SFX_NOISE(frames, env, poly)             (frames), (env), (poly)
#define SFX_END 0

void audio_init(void);
void audio_update(void);        /* VBL interrupt */
void music_play(const song_t *s);
void music_stop(void);
void sfx_play(const sfx_t *s);
#endif
