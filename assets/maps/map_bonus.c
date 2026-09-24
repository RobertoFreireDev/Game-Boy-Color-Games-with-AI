#include "engine/engine.h"
extern const tileset_def_t ts_bonus;

/* Bonus run: side-scrolling warehouse, 80x24 tiles (wider and taller than the 32x32 BG map,
   so the camera streams columns and rows). Row 23 sits under the HUD.
   P player start · o coin · R rat · S sign · Hh/Jj exit door (opens when every coin is taken) */
static const char * const rows[] = {
 /*           1111111111222222222233333333334444444444555555555566666666667777777777 */
 /* 01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
   "################################################################################",   /* 0 */
   "################################################################################",   /* 1 */
   "##        I                I                I                I                ##",   /* 2 */
   "##        I                I                I                I                ##",   /* 3 */
   "##        I                I                I                I                ##",   /* 4 */
   "##        I                I                I                I                ##",   /* 5 */
   "##        I                I     o          I                I                ##",   /* 6 */
   "##        I                I                I                I                ##",   /* 7 */
   "##        I                I  ======        I                I                ##",   /* 8 */
   "##        I              o I                I                I                ##",   /* 9 */
   "##        I                I                I                I                ##",   /* 10 */
   "##        I            =====                I                I                ##",   /* 11 */
   "##        I                I    o           I                I                ##",   /* 12 */
   "##        I                I                I                I                ##",   /* 13 */
   "##        I                I  =====         I                I                ##",   /* 14 */
   "##        I               oI                I  o             I                ##",   /* 15 */
   "##        I                I          o     I            o   I                ##",   /* 16 */
   "##        I             ======              I======         RI                ##",   /* 17 */
   "##        I   o      BC    I                I             #####   o           ##",   /* 18 */
   "##        I          DE    I      FFFFFFFF  I           #######         o     ##",   /* 19 */
   "##  P     I       BC BC    I      ffffffff BC         #########            Hh ##",   /* 20 */
   "##     S  I  ^^^  DE DE    I   R  ffffffff DE       ###########  ^^^   R   Jj ##",   /* 21 */
   "##############################################   ###############################",   /* 22 */
   "##############################################   ###############################",   /* 23 */
};

static const map_legend_t legend[] = {
    { ' ', 0, 1, 0 },                          /* back wall */
    { 'I', 15, 1, 0 },                         /* support beam (decor) */
    { '#', 1, 0, TF_SOLID },                   /* brick */
    { '=', 2, 2, TF_ONEWAY },                  /* shelf: stand on it, jump through from below */
    { '^', 3, 3, TF_HAZARD },                  /* spikes */
    { 'B', 4, 2, TF_SOLID },                   /* crate, 4 quarters */
    { 'C', 5, 2, TF_SOLID },
    { 'D', 6, 2, TF_SOLID },
    { 'E', 7, 2, TF_SOLID },
    { 'F', 8, 5, TF_OVER },                    /* fence in front of sprites */
    { 'f', 9, 5, TF_OVER },
    { 'H', 10, 4, TF_TRIGGER },                /* exit door, closed */
    { 'h', 10, 4, TF_TRIGGER | TF_FLIPX },
    { 'J', 11, 4, TF_TRIGGER },
    { 'j', 11, 4, TF_TRIGGER | TF_FLIPX },
    { 'K', 12, 4, TF_TRIGGER },                /* exit door, open (only via map_set_tile) */
    { 'k', 12, 4, TF_TRIGGER | TF_FLIPX },
    { 'L', 13, 4, TF_TRIGGER },
    { 'l', 13, 4, TF_TRIGGER | TF_FLIPX },
    { 'S', 14, 2, TF_TRIGGER },                /* sign: UP to read */
    { 'P', 0, 1, TF_SPAWN },                   /* player start */
    { 'o', 0, 1, TF_SPAWN },                   /* coin */
    { 'R', 0, 1, TF_SPAWN },                   /* rat */
    { 0, 0, 0, 0 }
};

const map_def_t map_bonus = { 80, 24, rows, legend, &ts_bonus };
