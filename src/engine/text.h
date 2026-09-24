#ifndef TEXT_H
#define TEXT_H
#include "core.h"
#define FONT_TILE(c) ((uint8_t)(((uint8_t)(c) >= 32 && (uint8_t)(c) < 128) ? 128 + (uint8_t)(c) - 32 : 128))
#define TILE_BLANK 128
#define ATTR_BLANK 0x0F                    /* bank 1 + palette 7 */
#define BOX_TILE0  224                     /* TL, T, TR, L, FILL, R, BL, B, BR, NEXT (bank 1) */
#define BOX_TILE(n) ((uint8_t)(BOX_TILE0 + (n)))

extern const palette_color_t pal_ui_default[4];   /* engine default UI colors (restore after text_set_colors) */

void text_init(void);
void text_set_colors(const palette_color_t *c4);   /* BG palette 7 */
void text_print(uint8_t x, uint8_t y, const char *s);      /* BG layer, relative to camera */
void text_print_win(uint8_t x, uint8_t y, const char *s);  /* window layer */
void text_print_num(uint8_t x, uint8_t y, uint16_t n, uint8_t digits);       /* BG, zero padded */
void text_print_num_win(uint8_t x, uint8_t y, uint16_t n, uint8_t digits);   /* window, zero padded */
void text_clear(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void box_draw_win(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

void dialog_show(const char *text);        /* BLOCKING */
uint8_t dialog_choice(const char *prompt, const char * const *options, uint8_t count);  /* BLOCKING */
uint8_t menu_run(uint8_t x, uint8_t y, const char * const *options, uint8_t count);    /* BLOCKING */
#endif
