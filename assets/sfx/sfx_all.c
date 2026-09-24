#include "engine/engine.h"

static const uint8_t d_menu[]  = { SFX_TONE(3, 0x00, 0x40, 0xA1, C6), SFX_END };
static const uint8_t d_start[] = { SFX_TONE(4, 0x00, 0x80, 0xF1, C5), SFX_TONE(12, 0x15, 0x80, 0xF3, G5), SFX_END };
static const uint8_t d_step[]  = { SFX_NOISE(3, 0x41, 0x6D), SFX_END };                 /* soft footstep */
static const uint8_t d_push[]  = { SFX_NOISE(8, 0x92, 0x55), SFX_END };                 /* crate scrape */
static const uint8_t d_bump[]  = { SFX_TONE(6, 0x00, 0x80, 0x81, C3), SFX_END };        /* blocked */
static const uint8_t d_goal[]  = { SFX_TONE(4, 0x00, 0x80, 0xF1, E6), SFX_TONE(10, 0x00, 0x80, 0xF3, A6), SFX_END };
static const uint8_t d_undo[]  = { SFX_TONE(6, 0x1D, 0x40, 0xA1, G5), SFX_END };        /* falling blip */

const sfx_t sfx_menu  = { SFX_CH1, 1, d_menu };
const sfx_t sfx_start = { SFX_CH1, 3, d_start };
const sfx_t sfx_step  = { SFX_CH4, 1, d_step };
const sfx_t sfx_push  = { SFX_CH4, 2, d_push };
const sfx_t sfx_bump  = { SFX_CH1, 1, d_bump };
const sfx_t sfx_goal  = { SFX_CH1, 2, d_goal };
const sfx_t sfx_undo  = { SFX_CH1, 1, d_undo };
