#ifndef MAP_H
#define MAP_H
#include "core.h"

typedef struct {
    const uint8_t *tiles; uint8_t count;                     /* count <= 128 */
    const palette_color_t * const *pals; uint8_t pal_count;  /* BG slots 0..pal_count-1 (max 7) */
} tileset_def_t;

typedef struct { char ch; uint8_t tile; uint8_t pal; uint8_t flags; } map_legend_t; /* list ends with ch 0 */
typedef struct {
    uint8_t w, h;                      /* in tiles, 20..200 each */
    const char * const *rows;          /* h strings, each exactly w chars */
    const map_legend_t *legend;
    const tileset_def_t *tileset;
} map_def_t;

/* tile flags */
#define TF_SOLID    0x01   /* blocks movement */
#define TF_HAZARD   0x02   /* hurts */
#define TF_ONEWAY   0x04   /* platform solid only from above */
#define TF_TRIGGER  0x08   /* game-defined (door, sign, exit) */
#define TF_SPAWN    0x10   /* object marker: drawn as its tile, found with map_find() */
#define TF_FLIPX    0x20   /* visual, copied to attribute */
#define TF_FLIPY    0x40
#define TF_OVER     0x80   /* BG tile drawn over sprites */

void    map_load(const map_def_t *m, uint8_t bank);
char    map_char(uint16_t tx, uint16_t ty);
uint8_t map_flags(uint16_t tx, uint16_t ty);       /* outside left/right/top = TF_SOLID, below = 0 */
uint8_t map_flags_px(int16_t px, int16_t py);      /* same, world pixel coords */
uint8_t map_find(char ch, uint8_t n, uint16_t *tx, uint16_t *ty);  /* n-th occurrence, returns 0 if none */
void    map_set_tile(uint16_t tx, uint16_t ty, char ch);  /* changes the VRAM tile only (visual) */
extern uint16_t map_w_px, map_h_px;

extern int16_t cam_x, cam_y;                       /* world pixel of screen top-left */
void cam_set(int16_t x, int16_t y);                /* clamp + redraw full screen (use at scene start) */
void cam_follow(int16_t wx, int16_t wy);           /* center on point with small dead zone, clamp, stream */
void cam_shake(uint8_t frames, uint8_t strength);  /* applied by cam_follow */
void cam_reset(void);                              /* called by the scene manager */
#define W2S_X(wx) ((wx) - cam_x)                   /* world -> screen */
#define W2S_Y(wy) ((wy) - cam_y)
#endif
