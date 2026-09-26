/* src/engine/audio.c: music driver (4 channels, instruments, loops), sound effects, priorities.
   audio_update is taken off the VBlank interrupt and called by hand, one call = one frame.
   The emulator reads sound registers back exactly as written. */
#pragma bank 255
#include "unit.h"

#define WAVE ((volatile uint8_t *)0xFF30)

extern const sfx_t sfx_menu;

static const uint8_t m_basic[] = { INST(INST_SQUARE), N(C4, 2), R(1), MUS_END };
static const song_t song_basic = { 2, { m_basic, 0, 0, 0 } };

static const uint8_t m_pluck[] = { INST(INST_PLUCK), N(A4, 1), MUS_END };
static const song_t song_ch2 = { 1, { 0, m_pluck, 0, 0 } };

static const uint8_t m_wave[] = { INST(WAVE_SAW), N(C4, 1), INST(WAVE_TRI), N(C4, 1), INST(WAVE_SOFTTRI), N(C7, 1), MUS_END };
static const song_t song_wave = { 1, { 0, 0, m_wave, 0 } };

static const uint8_t m_drums[] = { D(SNARE, 1), D(KICK, 1), MUS_END };
static const song_t song_drums = { 1, { 0, 0, 0, m_drums } };

static const uint8_t m_looppt[] = { N(C4, 1), MUS_LOOP_POINT, N(D4, 1), N(E4, 1), MUS_LOOP };
static const song_t song_looppt = { 1, { m_looppt, 0, 0, 0 } };
static const uint8_t m_loop[] = { N(C4, 1), N(D4, 1), MUS_LOOP };
static const song_t song_loop = { 1, { m_loop, 0, 0, 0 } };

static const uint8_t m_note_rest[] = { N(C4, 1), R(1), MUS_END };
static const uint8_t m_drum_rest[] = { D(KICK, 1), R(1), MUS_END };
static const song_t song_all = { 1, { m_note_rest, m_note_rest, m_note_rest, m_drum_rest } };

static const uint8_t m_thin[] = { INST(INST_THIN), N(C4, 1), MUS_END };
static const song_t song_thin = { 1, { m_thin, 0, 0, 0 } };
static const uint8_t m_plain[] = { N(C4, 1), MUS_END };
static const song_t song_plain = { 1, { m_plain, 0, 0, 0 } };

static const uint8_t m_scale[] = { N(C4, 1), N(D4, 1), N(E4, 1), N(F4, 1), N(G4, 1), MUS_END };
static const song_t song_scale = { 1, { m_scale, 0, 0, 0 } };
static const song_t song_scale_ch2 = { 1, { 0, m_scale, 0, 0 } };

static const uint8_t d_tone[] = { SFX_TONE(2, 0x15, 0x40, 0xF0, C5), SFX_END };
static const sfx_t sfx_tone = { SFX_CH1, 1, d_tone };
static const uint8_t d_two[] = { SFX_TONE(1, 0, 0x80, 0xF0, C5), SFX_TONE(1, 0, 0x40, 0xA0, G5), SFX_END };
static const sfx_t sfx_two = { SFX_CH1, 1, d_two };
static const uint8_t d_noise[] = { SFX_NOISE(1, 0xA1, 0x33), SFX_END };
static const sfx_t sfx_noise = { SFX_CH4, 1, d_noise };
static const uint8_t d_long_noise[] = { SFX_NOISE(10, 0xA1, 0x33), SFX_END };
static const sfx_t sfx_long_noise = { SFX_CH4, 1, d_long_noise };
static const uint8_t d_hi[] = { SFX_TONE(3, 0, 0x80, 0xF0, C5), SFX_END };
static const uint8_t d_hi2[] = { SFX_TONE(3, 0, 0x00, 0xF0, C5), SFX_END };
static const uint8_t d_lo[] = { SFX_TONE(3, 0, 0x40, 0xF0, C5), SFX_END };
static const sfx_t sfx_hi = { SFX_CH1, 5, d_hi };
static const sfx_t sfx_hi2 = { SFX_CH1, 5, d_hi2 };
static const sfx_t sfx_lo = { SFX_CH1, 1, d_lo };

static void update_n(uint8_t n) { while (n--) audio_update(); }

static void clear_regs(void) {
    NR10_REG = NR11_REG = NR12_REG = NR13_REG = NR14_REG = 0;
    NR21_REG = NR22_REG = NR23_REG = NR24_REG = 0;
    NR30_REG = NR31_REG = NR32_REG = NR33_REG = NR34_REG = 0;
    NR41_REG = NR42_REG = NR43_REG = NR44_REG = 0;
}

void unit_setup(void) BANKED {
    remove_VBL(audio_update);
    music_stop();
    update_n(20);                          /* let any sound effect finish */
    clear_regs();
}

TEST(first_note_on_next_update) {
    music_play(&song_basic);
    NR10_REG = 0x55;
    ASSERT_EQ(NR13_REG, 0);
    audio_update();
    ASSERT_EQ(NR10_REG, 0);
    ASSERT_EQ(NR11_REG, 0x80);             /* INST_SQUARE duty */
    ASSERT_EQ(NR12_REG, 0xA0);             /* INST_SQUARE envelope */
    ASSERT_EQ(NR13_REG, 0x0B);             /* C4 = 1547 = 0x60B */
    ASSERT_EQ(NR14_REG, 0x86);             /* trigger + high bits */
}

TEST(frames_per_tick_lengths_rest_end) {
    music_play(&song_basic);               /* fpt 2: C4 for 2 ticks, rest 1 tick, end */
    audio_update();
    NR12_REG = 0x55;
    update_n(3);
    ASSERT_EQ(NR12_REG, 0x55);             /* still holding the note */
    update_n(1);
    ASSERT_EQ(NR12_REG, 0);                /* rest */
    NR12_REG = 0x55;
    update_n(2);
    ASSERT_EQ(NR12_REG, 0);                /* end silences */
    NR12_REG = 0x55; NR13_REG = 0x55;
    update_n(6);
    ASSERT_EQ(NR12_REG, 0x55);
    ASSERT_EQ(NR13_REG, 0x55);
}

TEST(pulse_channel_2) {
    music_play(&song_ch2);
    NR11_REG = 0x55;
    audio_update();
    ASSERT_EQ(NR21_REG, 0x40);
    ASSERT_EQ(NR22_REG, 0xF1);
    ASSERT_EQ(NR23_REG, 0xD6);             /* A4 = 1750 = 0x6D6 */
    ASSERT_EQ(NR24_REG, 0x86);
    ASSERT_EQ(NR11_REG, 0x55);
}

TEST(wave_channel_loads_wave_ram_on_change) {
    music_play(&song_wave);
    audio_update();                        /* saw, C4 one octave up */
    ASSERT_EQ(WAVE[0], 0x00);
    ASSERT_EQ(WAVE[15], 0xFF);
    ASSERT_EQ(NR30_REG, 0x80);
    ASSERT_EQ(NR32_REG, 0x40);
    ASSERT_EQ(NR33_REG, 0x06);             /* C5 = 1798 = 0x706 */
    ASSERT_EQ(NR34_REG, 0x87);
    audio_update();                        /* triangle */
    ASSERT_EQ(WAVE[0], 0x01);
    ASSERT_EQ(WAVE[8], 0xFE);
    ASSERT_EQ(NR32_REG, 0x20);
    WAVE[0] = 0x77;
    audio_update();                        /* soft triangle: same wave, not reloaded */
    ASSERT_EQ(WAVE[0], 0x77);
    ASSERT_EQ(NR32_REG, 0x40);
    ASSERT_EQ(NR33_REG, 0xDF);             /* C7 + octave clamps to B7 = 2015 = 0x7DF */
    ASSERT_EQ(NR34_REG, 0x87);
}

TEST(noise_channel_drums) {
    music_play(&song_drums);
    audio_update();
    ASSERT_EQ(NR42_REG, 0xD2);
    ASSERT_EQ(NR43_REG, 0x42);
    ASSERT_EQ(NR44_REG, 0x80);
    audio_update();
    ASSERT_EQ(NR42_REG, 0xF1);
    ASSERT_EQ(NR43_REG, 0x6D);
}

TEST(rest_silences_each_channel) {
    music_play(&song_all);
    audio_update();
    ASSERT_EQ(NR12_REG, 0xC4);             /* default instrument INST_LEAD */
    ASSERT_EQ(NR22_REG, 0xC4);
    ASSERT_EQ(NR30_REG, 0x80);
    ASSERT_EQ(NR42_REG, 0xF1);
    audio_update();
    ASSERT_EQ(NR12_REG, 0);
    ASSERT_EQ(NR22_REG, 0);
    ASSERT_EQ(NR30_REG, 0);
    ASSERT_EQ(NR42_REG, 0);
}

TEST(loop_point) {
    music_play(&song_looppt);
    audio_update(); ASSERT_EQ(NR13_REG, 0x0B);   /* C4 */
    audio_update(); ASSERT_EQ(NR13_REG, 0x42);   /* D4 */
    audio_update(); ASSERT_EQ(NR13_REG, 0x72);   /* E4 */
    audio_update(); ASSERT_EQ(NR13_REG, 0x42);   /* back to D4 */
    audio_update(); ASSERT_EQ(NR13_REG, 0x72);
}

TEST(loop_without_point_restarts_song) {
    music_play(&song_loop);
    audio_update(); ASSERT_EQ(NR13_REG, 0x0B);
    audio_update(); ASSERT_EQ(NR13_REG, 0x42);
    audio_update(); ASSERT_EQ(NR13_REG, 0x0B);
}

TEST(play_silences_every_channel) {
    NR12_REG = NR22_REG = NR30_REG = NR42_REG = 0x55;
    music_play(&song_basic);
    ASSERT_EQ(NR12_REG, 0);
    ASSERT_EQ(NR22_REG, 0);
    ASSERT_EQ(NR30_REG, 0);
    ASSERT_EQ(NR42_REG, 0);
}

TEST(unused_channels_untouched) {
    music_play(&song_basic);
    NR21_REG = NR22_REG = NR23_REG = NR24_REG = NR42_REG = 0x55;
    update_n(8);
    ASSERT_EQ(NR21_REG, 0x55);
    ASSERT_EQ(NR23_REG, 0x55);
    ASSERT_EQ(NR24_REG, 0x55);
    ASSERT_EQ(NR42_REG, 0x55);
}

TEST(stop_silences_and_halts) {
    music_play(&song_looppt);
    audio_update();
    NR12_REG = NR22_REG = NR30_REG = NR42_REG = 0x55;
    music_stop();
    ASSERT_EQ(NR12_REG, 0);
    ASSERT_EQ(NR22_REG, 0);
    ASSERT_EQ(NR30_REG, 0);
    ASSERT_EQ(NR42_REG, 0);
    NR13_REG = 0x55;
    update_n(6);
    ASSERT_EQ(NR13_REG, 0x55);
}

TEST(play_resets_instruments) {
    music_play(&song_thin);
    audio_update();
    ASSERT_EQ(NR11_REG, 0x00);             /* INST_THIN */
    ASSERT_EQ(NR12_REG, 0xA3);
    music_play(&song_plain);
    audio_update();
    ASSERT_EQ(NR11_REG, 0x80);             /* back to INST_LEAD */
    ASSERT_EQ(NR12_REG, 0xC4);
}

TEST(sfx_tone_on_channel_1) {
    sfx_play(&sfx_tone);
    ASSERT_EQ(NR13_REG, 0);                /* starts on the next update */
    audio_update();
    ASSERT_EQ(NR10_REG, 0x15);
    ASSERT_EQ(NR11_REG, 0x40);
    ASSERT_EQ(NR12_REG, 0xF0);
    ASSERT_EQ(NR13_REG, 0x06);
    ASSERT_EQ(NR14_REG, 0x87);
    audio_update();
    ASSERT_EQ(NR12_REG, 0xF0);             /* 2 frames long */
    audio_update();
    ASSERT_EQ(NR12_REG, 0);                /* SFX_END silences */
}

TEST(sfx_steps_in_sequence) {
    sfx_play(&sfx_two);
    audio_update();
    ASSERT_EQ(NR11_REG, 0x80);
    audio_update();
    ASSERT_EQ(NR11_REG, 0x40);
    ASSERT_EQ(NR12_REG, 0xA0);
    ASSERT_EQ(NR13_REG, 0x59);             /* G5 = 1881 = 0x759 */
    audio_update();
    ASSERT_EQ(NR12_REG, 0);
}

TEST(sfx_noise_on_channel_4) {
    sfx_play(&sfx_noise);
    audio_update();
    ASSERT_EQ(NR42_REG, 0xA1);
    ASSERT_EQ(NR43_REG, 0x33);
    ASSERT_EQ(NR44_REG, 0x80);
    audio_update();
    ASSERT_EQ(NR42_REG, 0);
}

TEST(sfx_owns_channel_music_resumes) {
    music_play(&song_scale);
    sfx_play(&sfx_tone);
    audio_update();
    ASSERT_EQ(NR13_REG, 0x06);             /* sfx C5, not music C4 */
    audio_update();
    ASSERT_EQ(NR13_REG, 0x06);             /* music D4 skipped */
    audio_update();
    ASSERT_EQ(NR12_REG, 0);                /* sfx over, music E4 was skipped */
    ASSERT_EQ(NR13_REG, 0x06);
    audio_update();
    ASSERT_EQ(NR13_REG, 0x89);             /* music F4 */
    ASSERT_EQ(NR11_REG, 0x80);
}

TEST(sfx_leaves_other_channels_to_music) {
    music_play(&song_scale_ch2);
    sfx_play(&sfx_tone);
    audio_update();
    ASSERT_EQ(NR23_REG, 0x0B);
    audio_update();
    ASSERT_EQ(NR23_REG, 0x42);
}

TEST(sfx_priority) {
    sfx_play(&sfx_hi);
    audio_update();
    ASSERT_EQ(NR11_REG, 0x80);
    sfx_play(&sfx_lo);                     /* lower priority: ignored */
    audio_update(); ASSERT_EQ(NR11_REG, 0x80);
    audio_update(); ASSERT_EQ(NR11_REG, 0x80);
    audio_update(); ASSERT_EQ(NR12_REG, 0);  /* hi finished after 3 frames */
    sfx_play(&sfx_lo);                     /* nothing playing: any priority starts */
    audio_update(); ASSERT_EQ(NR11_REG, 0x40);
    sfx_play(&sfx_hi);                     /* higher replaces lower */
    audio_update(); ASSERT_EQ(NR11_REG, 0x80);
    sfx_play(&sfx_hi2);                    /* equal replaces */
    audio_update(); ASSERT_EQ(NR11_REG, 0x00);
}

TEST(sfx_menu_asset_plays_on_channel_1) {
    ASSERT_EQ(sfx_menu.ch, SFX_CH1);
    sfx_play(&sfx_menu);
    audio_update();
    ASSERT(NR14_REG & 0x80);
    ASSERT(NR12_REG != 0);
}

TEST(audio_init_powers_on_and_hooks_vblank) {
    NR50_REG = 0; NR51_REG = 0;
    audio_init();
    ASSERT(NR52_REG & 0x80);
    ASSERT_EQ(NR50_REG, 0x77);
    ASSERT_EQ(NR51_REG, 0xFF);
    sfx_play(&sfx_long_noise);
    vsync();
    vsync();
    remove_VBL(audio_update);
    ASSERT_EQ(NR42_REG, 0xA1);             /* written by the VBlank handler */
}

void unit_tests(void) BANKED {
    RUN(first_note_on_next_update);
    RUN(frames_per_tick_lengths_rest_end);
    RUN(pulse_channel_2);
    RUN(wave_channel_loads_wave_ram_on_change);
    RUN(noise_channel_drums);
    RUN(rest_silences_each_channel);
    RUN(loop_point);
    RUN(loop_without_point_restarts_song);
    RUN(play_silences_every_channel);
    RUN(unused_channels_untouched);
    RUN(stop_silences_and_halts);
    RUN(play_resets_instruments);
    RUN(sfx_tone_on_channel_1);
    RUN(sfx_steps_in_sequence);
    RUN(sfx_noise_on_channel_4);
    RUN(sfx_owns_channel_music_resumes);
    RUN(sfx_leaves_other_channels_to_music);
    RUN(sfx_priority);
    RUN(sfx_menu_asset_plays_on_channel_1);
    RUN(audio_init_powers_on_and_hooks_vblank);
}
