#ifndef ASSETS_H
#define ASSETS_H
#include "engine/engine.h"

/* Add one extern line for EVERY asset the game defines (see CLAUDE.md 6.9). */

/* palettes */

/* sprites */

/* tilesets + maps */

/* font + engine UI (assets/fonts/font_main.c, shared by every game) */
extern const uint8_t font_main[], font_box_tiles[];

/* music */

/* sfx (sfx_menu lives in font_main.c: the engine's dialogs and menus use it) */
extern const sfx_t sfx_menu;
#endif
