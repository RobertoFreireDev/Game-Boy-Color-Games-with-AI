# Engine API

Read when: writing game code or scenes that call the engine, or changing `src/engine/`. The audio module (7.13) is in [audio.md](audio.md).

## 7. Engine API

Build the engine exactly with these names and semantics. Reference code is given where correctness is subtle; write the rest in the same style.

### 7.1 core (core.h / core.c)

```c
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <rand.h>
#include <gbdk/emu_debug.h>

/* fixed point 12.4 in int16_t: 1 pixel = 16 units, range ±2047 px */
#define FIX(px)     ((int16_t)((px) << 4))
#define UNFIX(v)    ((int16_t)(v) >> 4)
#define ABS(a)      ((a) < 0 ? -(a) : (a))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define CLAMP(v,lo,hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

void engine_init(void);
int16_t approach(int16_t v, int16_t target, int16_t step);  /* move v toward target by step */
void rand_seed(void);     /* initrand(DIV_REG | (sys_time << 8)); call when the player presses START */
```
`engine_init()`:
```c
void engine_init(void) {
    if (_cpu != CGB_TYPE) { while (1) vsync(); }   /* Color only */
    cpu_fast();
    DISPLAY_OFF;                 /* the only allowed DISPLAY_OFF */
    SPRITES_8x8;
    fade_init();                 /* all palettes in RAM, fade level = fully black */
    scene_reset_screen();        /* clear BG map + attributes, hide sprites/window */
    text_init();                 /* font + box tiles into VRAM bank 1, default UI palette */
    audio_init();                /* sound on, add_VBL(audio_update) */
    SHOW_BKG; SHOW_SPRITES; HIDE_WIN;
    DISPLAY_ON;
}
```

### 7.2 input

```c
extern uint8_t keys, keys_prev;
void input_update(void);                       /* keys_prev = keys; keys = joypad(); */
#define KEY_HELD(k)     (keys & (k))
#define KEY_PRESSED(k)  ((keys & (k)) && !(keys_prev & (k)))
#define KEY_RELEASED(k) (!(keys & (k)) && (keys_prev & (k)))
/* k: J_UP J_DOWN J_LEFT J_RIGHT J_A J_B J_START J_SELECT (can be OR-ed) */
uint8_t input_wait_press(uint8_t mask);        /* blocking; returns the key pressed */
```

### 7.3 gfx (palettes + sprite loading)

```c
typedef struct { uint8_t base, w, h, tpf, frames, pal; } sprite_t;   /* RAM handle (tpf = tiles per frame) */

void gfx_set_bkg_palette(uint8_t slot, const palette_color_t *c4);  /* goes through fade (7.10) */
void gfx_set_obj_palette(uint8_t slot, const palette_color_t *c4);
uint8_t gfx_load_sprite(sprite_t *out, const sprite_def_t *def, uint8_t bank);
    /* copies tiles into bank-0 VRAM at next free index (0..127), loads palette into def->pal_slot,
       fills *out, returns 0 if VRAM is full. bank = 0 for non-banked data (section 10, docs/banking.md) */
void gfx_reset(void);                     /* sprite VRAM allocator back to 0 (called on scene change) */
```
Loading the same `sprite_def_t` twice wastes VRAM — load each once per scene and share the handle.

### 7.4 sprites (OAM allocator + drawing)

```c
void spr_begin(void);                          /* called by scene_update */
void spr_end(void);                            /* hides OAM entries not used this frame */
uint8_t spr_put(uint8_t tile, int16_t sx, int16_t sy, uint8_t prop);   /* one 8x8, screen coords */
void spr_draw(const sprite_t *s, uint8_t frame, int16_t sx, int16_t sy, uint8_t flags);
void spr_hide_all(void);
#define SPR_FLIPX 0x20
#define SPR_FLIPY 0x40
#define SPR_BEHIND 0x80
```
Reference:
```c
static uint8_t spr_next, spr_last;
void spr_begin(void) { spr_next = 0; }
void spr_end(void) {
    uint8_t i;
    for (i = spr_next; i < spr_last; i++) hide_sprite(i);
    spr_last = spr_next;
}
uint8_t spr_put(uint8_t tile, int16_t sx, int16_t sy, uint8_t prop) {
    if (spr_next >= 40 || sx <= -8 || sx >= 160 || sy <= -8 || sy >= 144) return 0;
    set_sprite_tile(spr_next, tile);
    set_sprite_prop(spr_next, prop);
    move_sprite(spr_next, (uint8_t)(sx + 8), (uint8_t)(sy + 16));
    spr_next++;
    return 1;
}
void spr_draw(const sprite_t *s, uint8_t frame, int16_t sx, int16_t sy, uint8_t flags) {
    uint8_t r, c, sr, sc;
    uint8_t first = s->base + frame * s->tpf;
    uint8_t prop = s->pal | flags;
    for (r = 0; r < s->h; r++) {
        sr = (flags & SPR_FLIPY) ? (uint8_t)(s->h - 1 - r) : r;
        for (c = 0; c < s->w; c++) {
            sc = (flags & SPR_FLIPX) ? (uint8_t)(s->w - 1 - c) : c;
            spr_put(first + sr * s->w + sc, sx + (c << 3), sy + (r << 3), prop);
        }
    }
}
```
Draw order = priority: sprites drawn **first appear on top**. Draw the player (and UI cursor) before enemies and particles.

### 7.5 anim

```c
typedef struct { const uint8_t *frames; uint8_t len; uint8_t speed; uint8_t loop; } anim_def_t; /* speed = frames per step */
typedef struct { const anim_def_t *def; uint8_t i, timer, done; } anim_t;
void anim_play(anim_t *a, const anim_def_t *def);   /* restarts only if def changed */
void anim_update(anim_t *a);                        /* once per frame */
#define anim_frame(a) ((a)->def->frames[(a)->i])
```
Defined in game code:
```c
static const uint8_t walk_frames[] = { 0, 1, 0, 2 };
static const anim_def_t anim_walk = { walk_frames, 4, 8, 1 };
```

### 7.6 map + camera

```c
void map_load(const map_def_t *m, uint8_t bank);   /* builds lookup tables, loads tileset (BG 128+) and BG palettes */
char    map_char(uint16_t tx, uint16_t ty);
uint8_t map_flags(uint16_t tx, uint16_t ty);       /* outside left/right/top = TF_SOLID, below = 0 */
uint8_t map_flags_px(int16_t px, int16_t py);      /* same, world pixel coords */
uint8_t map_find(char ch, uint8_t n, uint16_t *tx, uint16_t *ty);  /* n-th occurrence, returns 0 if none */
void    map_set_tile(uint16_t tx, uint16_t ty, char ch);  /* changes the VRAM tile only (visual: open door, collected coin);
                                                            keep game-side state for logic */
extern uint16_t map_w_px, map_h_px;

extern int16_t cam_x, cam_y;                       /* world pixel of screen top-left */
void cam_set(int16_t x, int16_t y);                /* clamp + redraw full screen (use at scene start) */
void cam_follow(int16_t wx, int16_t wy);           /* center on point with small dead zone, clamp, stream */
void cam_shake(uint8_t frames, uint8_t strength);  /* applied by cam_follow */
void cam_reset(void);                              /* called by the scene manager */
#define W2S_X(wx) ((wx) - cam_x)                   /* world -> screen */
#define W2S_Y(wy) ((wy) - cam_y)
```
Implementation notes:
- Lookup tables in RAM, built by `map_load`: `lut_tile[128]` (`128 + legend.tile`), `lut_attr[128]` (`pal | (flags & 0xE0)`), `lut_flags[128]`. Unknown chars → blank, flags 0.
- `map_char` = `rows[ty][tx] & 0x7F` (with bank switching when `bank != 0`).
- Maps smaller than the screen are drawn once and centered or top-left; clamp camera to `0..map_w_px-160` / `0..map_h_px-144` (0 if negative).
- Camera may move at most 8 px per frame per axis.

Streaming reference (the BG map is 32×32 and wraps; we redraw only the new column/row):
```c
static uint16_t drawn_tx, drawn_ty;   /* tile of the currently drawn top-left */

static void put_cell(uint16_t mx, uint16_t my) {
    uint8_t t = TILE_BLANK, a = ATTR_BLANK;
    if (mx < map_w && my < map_h) { char ch = map_char(mx, my); t = lut_tile[ch]; a = lut_attr[ch]; }
    VBK_REG = 1; set_bkg_tile_xy(mx & 31, my & 31, a);
    VBK_REG = 0; set_bkg_tile_xy(mx & 31, my & 31, t);
}
static void draw_col(uint16_t mx, uint16_t my) { uint8_t i; for (i = 0; i < 19; i++) put_cell(mx, my + i); }
static void draw_row(uint16_t mx, uint16_t my) { uint8_t i; for (i = 0; i < 21; i++) put_cell(mx + i, my); }

static void cam_apply(void) {          /* after cam_x/cam_y changed and were clamped */
    uint16_t tx = cam_x >> 3, ty = cam_y >> 3;
    while (drawn_tx < tx) { drawn_tx++; draw_col(drawn_tx + 20, drawn_ty); }
    while (drawn_tx > tx) { drawn_tx--; draw_col(drawn_tx,      drawn_ty); }
    while (drawn_ty < ty) { drawn_ty++; draw_row(drawn_tx, drawn_ty + 18); }
    while (drawn_ty > ty) { drawn_ty--; draw_row(drawn_tx, drawn_ty); }
    move_bkg((uint8_t)(cam_x + shake_dx), (uint8_t)(cam_y + shake_dy));
}
/* cam_set: set drawn_tx/ty = cam tile, draw rows ty..ty+18 fully (21 cells each), then move_bkg */
```

### 7.7 collide

```c
typedef struct { int16_t x, y, vx, vy; uint8_t w, h; } body_t;   /* x,y,vx,vy fixed 12.4; w,h hitbox px */
#define HIT_LEFT 1
#define HIT_RIGHT 2
#define HIT_UP 4
#define HIT_DOWN 8
uint8_t rect_overlap(int16_t ax, int16_t ay, uint8_t aw, uint8_t ah,
                     int16_t bx, int16_t by, uint8_t bw, uint8_t bh);   /* pixels */
uint8_t body_overlap(const body_t *a, const body_t *b);
uint8_t body_move(body_t *b);           /* moves with tile collision, returns HIT_* bits */
uint8_t body_on_ground(const body_t *b);
uint8_t body_touch_flags(const body_t *b); /* OR of flags of all tiles the hitbox covers (hazard, trigger) */
```
Reference (speed must stay below 8 px/frame, i.e. |v| < FIX(8)):
```c
static uint8_t col_hit(int16_t x, int16_t y, uint8_t h, uint8_t mask) {   /* vertical edge */
    int16_t yy = y, end = y + h - 1;
    for (;;) {
        if (map_flags_px(x, yy) & mask) return 1;
        if (yy == end) return 0;
        yy += 8; if (yy > end) yy = end;
    }
}
static uint8_t row_hit(int16_t x, int16_t y, uint8_t w, uint8_t mask) {   /* horizontal edge */
    int16_t xx = x, end = x + w - 1;
    for (;;) {
        if (map_flags_px(xx, y) & mask) return 1;
        if (xx == end) return 0;
        xx += 8; if (xx > end) xx = end;
    }
}
uint8_t body_move(body_t *b) {
    uint8_t hit = 0, mask;
    int16_t px, py, bottom, old_bottom;

    b->x += b->vx;
    px = UNFIX(b->x); py = UNFIX(b->y);
    if (b->vx > 0 && col_hit(px + b->w - 1, py, b->h, TF_SOLID)) {
        px = ((px + b->w - 1) & ~7) - b->w; b->x = FIX(px); b->vx = 0; hit |= HIT_RIGHT;
    } else if (b->vx < 0 && col_hit(px, py, b->h, TF_SOLID)) {
        px = (px & ~7) + 8; b->x = FIX(px); b->vx = 0; hit |= HIT_LEFT;
    }

    old_bottom = py + b->h - 1;
    b->y += b->vy;
    py = UNFIX(b->y);
    if (b->vy > 0) {
        bottom = py + b->h - 1;
        mask = TF_SOLID;
        if ((old_bottom >> 3) < (bottom >> 3)) mask |= TF_ONEWAY;   /* entered the row from above */
        if (row_hit(px, bottom, b->w, mask)) {
            py = (bottom & ~7) - b->h; b->y = FIX(py); b->vy = 0; hit |= HIT_DOWN;
        }
    } else if (b->vy < 0 && row_hit(px, py, b->w, TF_SOLID)) {
        py = (py & ~7) + 8; b->y = FIX(py); b->vy = 0; hit |= HIT_UP;
    }
    return hit;
}
uint8_t body_on_ground(const body_t *b) {
    return row_hit(UNFIX(b->x), UNFIX(b->y) + b->h, b->w, TF_SOLID | TF_ONEWAY);
}
```
Platformer: `vy += GRAVITY` (e.g. 5), clamp `vy` to `FIX(4)`, jump `vy = -FIX(3)`, variable jump: if A released while `vy < 0`, `vy >>= 1`. Top-down: no gravity, set `vx/vy` from the d-pad.

### 7.8 text, dialogs, menus

```c
#define FONT_TILE(c) ((uint8_t)(((c) >= 32 && (c) < 128) ? 128 + (c) - 32 : 128))
#define TILE_BLANK 128
#define ATTR_BLANK 0x0F                    /* bank 1 + palette 7 */
#define BOX_TILE0  224                     /* TL, T, TR, L, FILL, R, BL, B, BR, NEXT (bank 1) */
#define BOX_TILE(n) ((uint8_t)(BOX_TILE0 + (n)))
extern const palette_color_t pal_ui_default[4];   /* engine default UI colors (restore after text_set_colors) */
void text_init(void);                      /* expand 1bpp font to 2bpp glyph by glyph (16-byte buffer),
                                              VBK_REG = 1, set_bkg_data(128 + i, 1, buf); box tiles at 224 */
void text_set_colors(const palette_color_t *c4);   /* BG palette 7 */
void text_print(uint8_t x, uint8_t y, const char *s);      /* BG layer, tile coords relative to camera
                                                              top-left: x + (cam_x>>3), wrapped &31 */
void text_print_win(uint8_t x, uint8_t y, const char *s);  /* window layer */
void text_print_num(uint8_t x, uint8_t y, uint16_t n, uint8_t digits);       /* BG, zero padded */
void text_print_num_win(uint8_t x, uint8_t y, uint16_t n, uint8_t digits);   /* window, zero padded */
void text_clear(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void box_draw_win(uint8_t x, uint8_t y, uint8_t w, uint8_t h);           /* bordered box on window */

void dialog_show(const char *text);        /* BLOCKING */
uint8_t dialog_choice(const char *prompt, const char * const *options, uint8_t count);  /* BLOCKING, returns index */
uint8_t menu_run(uint8_t x, uint8_t y, const char * const *options, uint8_t count);    /* BLOCKING, on BG, arrow cursor */
```
Writing a character: write tile `FONT_TILE(c)` with `VBK_REG = 0` and attribute `ATTR_BLANK` with `VBK_REG = 1`, then leave `VBK_REG = 0`.

`dialog_show` behavior: window box 20×5 tiles at the bottom (`move_win(7, 104)`, `SHOW_WIN`), 3 lines × 18 chars; typewriter 1 char every 2 frames (every frame while A held); automatic word wrap; `\n` = new line; `\f` = new page; when a page is full or text ends, blink the NEXT arrow and wait for `A`; plays `sfx_menu` per page; `HIDE_WIN` at the end. Its loop calls `vsync(); input_update();` itself (audio keeps playing via interrupt). Sprites stay frozen as last drawn.

HUD: the window always covers everything from its top line down to the bottom of the screen, so it can't be a top bar. Put the HUD at the bottom (`move_win(7, 136)` = 1 row), or draw a small HUD with sprites. While a dialog is open, the dialog takes over the window.

### 7.9 scene manager

```c
typedef struct { void (*enter)(void); void (*update)(void); void (*leave)(void); } scene_t;  /* leave may be 0 */
#define TRANS_NONE 0
#define TRANS_FADE_BLACK 1
#define TRANS_FADE_WHITE 2
void scene_start(const scene_t *first);
void scene_goto(const scene_t *next, uint8_t transition);   /* request; performed at end of this frame */
void scene_update(void);
void scene_reset_screen(void);   /* hide sprites + window, fill BG map with TILE_BLANK/ATTR_BLANK
                                    (fill_bkg_rect on VRAM bank 0 and 1), move_bkg(0,0) */
```
Reference:
```c
static const scene_t *cur, *pending;
static uint8_t pending_trans;

static void run_frame(void) { spr_begin(); cur->update(); spr_end(); }

void scene_start(const scene_t *first) {
    cur = first; cur->enter(); run_frame(); fade_in(4, 0);
}
void scene_update(void) {
    run_frame();
    if (!pending) return;
    uint8_t white = (pending_trans == TRANS_FADE_WHITE);
    if (pending_trans != TRANS_NONE) fade_out(4, white);
    if (cur->leave) cur->leave();
    scene_reset_screen(); gfx_reset(); tween_clear(); particles_clear(); cam_reset();
    cur = pending; pending = 0;
    cur->enter();
    run_frame();
    if (pending_trans != TRANS_NONE) fade_in(4, white); else fade_set_level(0);
}
```

### 7.10 fade

The engine keeps a RAM copy of all 16 palettes (`pal_ram[64]`: 0–31 BG, 32–63 OBJ) and a fade level 0 (normal) … 8 (fully black/white). `gfx_set_*_palette` writes to RAM and then applies the current level, so palettes loaded while faded out stay invisible.

```c
extern palette_color_t pal_ram[64];                    /* 0-31 BG, 32-63 OBJ */
void fade_init(void);                                  /* all black, level 8 */
void fade_out(uint8_t frames_per_step, uint8_t to_white);   /* BLOCKING, 8 steps */
void fade_in(uint8_t frames_per_step, uint8_t from_white);  /* BLOCKING */
void fade_set_level(uint8_t level);
void fade_apply(void);
```
Reference:
```c
static palette_color_t blend(palette_color_t c, uint8_t s, uint8_t white) {
    uint8_t r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    if (white) { r += ((31 - r) * s) >> 3; g += ((31 - g) * s) >> 3; b += ((31 - b) * s) >> 3; }
    else       { r -= (r * s) >> 3;        g -= (g * s) >> 3;        b -= (b * s) >> 3; }
    return RGB(r, g, b);
}
void fade_apply(void) {
    static palette_color_t tmp[64];
    uint8_t i;
    for (i = 0; i < 64; i++) tmp[i] = fade_level ? blend(pal_ram[i], fade_level, fade_white) : pal_ram[i];
    set_bkg_palette(0, 8, tmp);
    set_sprite_palette(0, 8, tmp + 32);
}
void fade_out(uint8_t fps, uint8_t to_white) {
    uint8_t s, f;
    fade_white = to_white;
    for (s = 1; s <= 8; s++) { fade_level = s; vsync(); fade_apply(); for (f = 1; f < fps; f++) vsync(); }
}
/* fade_in: same with s = 7 down to 0 */
```

### 7.11 tween

```c
#define EASE_LINEAR 0
#define EASE_IN 1
#define EASE_OUT 2
#define EASE_INOUT 3
uint8_t tween_start(int16_t *target, int16_t to, uint8_t frames, uint8_t ease);  /* from = *target now */
void tween_update(void);            /* once per frame */
uint8_t tween_busy(const int16_t *target);
void tween_clear(void);             /* pool of 8 */
```
Easing tables (17 entries, progress 0..16 → 0..256), interpolate between entries:
```c
static const uint16_t ease_tbl[4][17] = {
  {0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,256},
  {0,1,4,9,16,25,36,49,64,81,100,121,144,169,196,225,256},
  {0,31,60,87,112,135,156,175,192,207,220,231,240,247,252,255,256},
  {0,2,8,18,32,50,72,98,128,158,184,206,224,238,248,254,256},
};
/* p = (t << 8) / dur (0..256); i = p >> 4; e = tbl[i] + (((tbl[i+1]-tbl[i]) * (p & 15)) >> 4) (i < 16);
   *target = from + (int16_t)(((int32_t)(to - from) * e) >> 8); on the last frame set *target = to exactly */
```
Use for menus sliding in, cameras, UI bounce, moving platforms on rails, title logos.

### 7.12 particles

```c
typedef struct {
    const sprite_t *spr;   /* 1x1 sprite; frames play across the lifetime */
    uint8_t speed;         /* max initial speed, 1/16 px per frame (24 = 1.5 px) */
    int8_t  up;            /* added to initial vy (negative = upward burst) */
    int8_t  gravity;       /* added to vy every frame */
    uint8_t life;          /* frames */
} particle_style_t;
void particles_emit(int16_t wx, int16_t wy, uint8_t count, const particle_style_t *st);  /* world px */
void particles_update(void);   /* move + draw (call inside update, after the important sprites) */
void particles_clear(void);    /* pool of 12; emitting when full recycles the oldest */
```
Random velocity: `(int8_t)(rand() % (2 * speed + 1)) - speed`. Keep bursts ≤ 8 particles because of the 10-per-line limit.
