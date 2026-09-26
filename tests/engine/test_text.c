/* src/engine/text.c: font, text printing, window, box, dialog_show, dialog_choice, menu_run */
#pragma bank 255
#include "unit.h"

extern const uint8_t font_main[], font_box_tiles[];

static const palette_color_t pal_x[4] = { RGB(1,2,3), RGB(4,5,6), RGB(7,8,9), RGB(10,11,12) };
static const uint8_t press_a[] = { 0, J_A };                  /* A pressed every other frame */
static const char * const yes_no_maybe[] = { "YES", "NO", "MAYBE" };
static const char * const start_options[] = { "START", "OPTIONS" };

void unit_setup(void) BANKED {
    unit_reset_engine();
    text_set_colors(pal_ui_default);
    fade_set_level(0);
}

TEST(font_tile_macro) {
    ASSERT_EQ(FONT_TILE(' '), 128);
    ASSERT_EQ(FONT_TILE('A'), 161);
    ASSERT_EQ(FONT_TILE('~'), 222);
    ASSERT_EQ(FONT_TILE(127), 223);
    ASSERT_EQ(FONT_TILE(31), 128);        /* out of range -> blank */
    ASSERT_EQ(FONT_TILE(200), 128);
    ASSERT_EQ(BOX_TILE(0), 224);
    ASSERT_EQ(BOX_TILE(9), 233);
}

TEST(text_init_loads_font_and_box_tiles) {
    static const uint8_t zero[16] = { 0 };
    uint8_t r, c, i;
    uint16_t a;
    VBK_REG = 1;
    set_bkg_data(FONT_TILE('0'), 1, zero);   /* damage a glyph */
    VBK_REG = 0;
    text_init();
    ASSERT_EQ(VBK_REG & 1, 0);
    for (i = 0; i < 3; i++) {
        c = i == 0 ? '0' : (i == 1 ? 'Z' : '~');
        a = 0x8000 + ((uint16_t)(128 + c - 32) << 4);
        for (r = 0; r < 8; r++) {
            ASSERT_EQ(hw_vram(1, a + (r << 1)), font_main[(c - 32) * 8 + r]);       /* low plane */
            ASSERT_EQ(hw_vram(1, a + (r << 1) + 1), font_main[(c - 32) * 8 + r]);   /* high plane: color 3 */
        }
    }
    for (a = 0; a < 160; a++) ASSERT_EQ(hw_vram(1, 0x8E00 + a), font_box_tiles[a]);
    ASSERT_EQ(pal_ram[28], pal_ui_default[0]);
}

TEST(set_colors_uses_bkg_palette_7) {
    uint8_t i;
    text_set_colors(pal_x);
    for (i = 0; i < 4; i++) {
        ASSERT_EQ(pal_ram[28 + i], pal_x[i]);
        ASSERT_EQ(hw_bkg_color(7, i), pal_x[i]);
    }
    ASSERT_EQ(pal_ui_default[3], RGB8(248, 248, 232));
}

TEST(print_writes_font_tiles_and_ui_attr) {
    text_print(2, 3, "Hi!");
    ASSERT_EQ(hw_bkg_tile(2, 3), FONT_TILE('H'));
    ASSERT_EQ(hw_bkg_tile(3, 3), FONT_TILE('i'));
    ASSERT_EQ(hw_bkg_tile(4, 3), FONT_TILE('!'));
    ASSERT_EQ(hw_bkg_attr(2, 3), ATTR_BLANK);
    ASSERT_EQ(hw_bkg_attr(4, 3), ATTR_BLANK);
    ASSERT_EQ(hw_bkg_tile(5, 3), TILE_BLANK);
    ASSERT_EQ(VBK_REG & 1, 0);
}

TEST(print_newline_returns_to_x) {
    text_print(5, 1, "AB\nC");
    ASSERT_EQ(hw_bkg_tile(5, 1), FONT_TILE('A'));
    ASSERT_EQ(hw_bkg_tile(6, 1), FONT_TILE('B'));
    ASSERT_EQ(hw_bkg_tile(5, 2), FONT_TILE('C'));
    ASSERT_EQ(hw_bkg_tile(7, 1), TILE_BLANK);
}

TEST(print_is_relative_to_camera) {
    cam_x = 20; cam_y = 9;                 /* tile offset 2, 1 */
    text_print(0, 0, "X");
    ASSERT_EQ(hw_bkg_tile(2, 1), FONT_TILE('X'));
    ASSERT_EQ(hw_bkg_tile(0, 0), TILE_BLANK);
}

TEST(print_wraps_around_bg_map) {
    cam_x = 8 * 30;
    text_print(1, 0, "AB");
    ASSERT_EQ(hw_bkg_tile(31, 0), FONT_TILE('A'));
    ASSERT_EQ(hw_bkg_tile(0, 0), FONT_TILE('B'));
}

TEST(print_win_uses_window_map) {
    text_print_win(1, 2, "OK\nGO");
    ASSERT_EQ(hw_win_tile(1, 2), FONT_TILE('O'));
    ASSERT_EQ(hw_win_tile(2, 2), FONT_TILE('K'));
    ASSERT_EQ(hw_win_tile(1, 3), FONT_TILE('G'));
    ASSERT_EQ(hw_win_attr(1, 2), ATTR_BLANK);
    ASSERT_EQ(hw_bkg_tile(1, 2), TILE_BLANK);
}

TEST(print_num_zero_pads) {
    text_print_num(0, 0, 42, 5);
    ASSERT_EQ(hw_bkg_tile(0, 0), FONT_TILE('0'));
    ASSERT_EQ(hw_bkg_tile(2, 0), FONT_TILE('0'));
    ASSERT_EQ(hw_bkg_tile(3, 0), FONT_TILE('4'));
    ASSERT_EQ(hw_bkg_tile(4, 0), FONT_TILE('2'));
    ASSERT_EQ(hw_bkg_tile(5, 0), TILE_BLANK);
}

TEST(print_num_keeps_lowest_digits) {
    text_print_num(0, 1, 12345, 3);
    ASSERT_EQ(hw_bkg_tile(0, 1), FONT_TILE('3'));
    ASSERT_EQ(hw_bkg_tile(2, 1), FONT_TILE('5'));
    ASSERT_EQ(hw_bkg_tile(3, 1), TILE_BLANK);
}

TEST(print_num_max_5_digits) {
    text_print_num(0, 2, 65535, 9);
    ASSERT_EQ(hw_bkg_tile(0, 2), FONT_TILE('6'));
    ASSERT_EQ(hw_bkg_tile(4, 2), FONT_TILE('5'));
    ASSERT_EQ(hw_bkg_tile(5, 2), TILE_BLANK);
    text_print_num(0, 3, 0, 1);
    ASSERT_EQ(hw_bkg_tile(0, 3), FONT_TILE('0'));
    text_print_num(0, 4, 7, 0);
    ASSERT_EQ(hw_bkg_tile(0, 4), TILE_BLANK);
}

TEST(print_num_win) {
    text_print_num_win(3, 0, 907, 4);
    ASSERT_EQ(hw_win_tile(3, 0), FONT_TILE('0'));
    ASSERT_EQ(hw_win_tile(4, 0), FONT_TILE('9'));
    ASSERT_EQ(hw_win_tile(5, 0), FONT_TILE('0'));
    ASSERT_EQ(hw_win_tile(6, 0), FONT_TILE('7'));
}

TEST(clear_blanks_a_rectangle) {
    text_print(0, 0, "HELLO\nWORLD");
    text_clear(1, 0, 3, 2);
    ASSERT_EQ(hw_bkg_tile(0, 0), FONT_TILE('H'));
    ASSERT_EQ(hw_bkg_tile(1, 0), TILE_BLANK);
    ASSERT_EQ(hw_bkg_tile(3, 1), TILE_BLANK);
    ASSERT_EQ(hw_bkg_tile(4, 0), FONT_TILE('O'));
    ASSERT_EQ(hw_bkg_tile(4, 1), FONT_TILE('D'));
    ASSERT_EQ(hw_bkg_attr(2, 1), ATTR_BLANK);
}

TEST(clear_is_relative_to_camera) {
    cam_x = 16;
    text_print(0, 0, "AB");
    text_clear(0, 0, 1, 1);
    ASSERT_EQ(hw_bkg_tile(2, 0), TILE_BLANK);
    ASSERT_EQ(hw_bkg_tile(3, 0), FONT_TILE('B'));
}

TEST(box_draw_win_frame) {
    box_draw_win(1, 1, 4, 3);
    ASSERT_EQ(hw_win_tile(1, 1), BOX_TILE(0));
    ASSERT_EQ(hw_win_tile(2, 1), BOX_TILE(1));
    ASSERT_EQ(hw_win_tile(3, 1), BOX_TILE(1));
    ASSERT_EQ(hw_win_tile(4, 1), BOX_TILE(2));
    ASSERT_EQ(hw_win_tile(1, 2), BOX_TILE(3));
    ASSERT_EQ(hw_win_tile(2, 2), BOX_TILE(4));
    ASSERT_EQ(hw_win_tile(4, 2), BOX_TILE(5));
    ASSERT_EQ(hw_win_tile(1, 3), BOX_TILE(6));
    ASSERT_EQ(hw_win_tile(3, 3), BOX_TILE(7));
    ASSERT_EQ(hw_win_tile(4, 3), BOX_TILE(8));
    ASSERT_EQ(hw_win_attr(4, 3), ATTR_BLANK);
    ASSERT_EQ(hw_win_tile(5, 2), TILE_BLANK);
}

TEST(dialog_shows_text_then_hides) {
    pad_loop(press_a, 2);
    dialog_show("HELLO");
    ASSERT(!(LCDC_REG & LCDCF_WINON));
    ASSERT_EQ(WX_REG, 7);
    ASSERT_EQ(WY_REG, 104);
    ASSERT_EQ(hw_win_tile(0, 0), BOX_TILE(0));
    ASSERT_EQ(hw_win_tile(19, 4), BOX_TILE(8));
    ASSERT_EQ(hw_win_tile(1, 1), FONT_TILE('H'));
    ASSERT_EQ(hw_win_tile(5, 1), FONT_TILE('O'));
    ASSERT_EQ(hw_win_tile(6, 1), BOX_TILE(4));
    ASSERT_EQ(hw_win_tile(18, 4), BOX_TILE(7));   /* "next" arrow put back to border */
}

TEST(dialog_waits_for_a_press) {
    static const uint8_t late_a[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, J_A };
    pad_script(late_a, sizeof(late_a));
    dialog_show("A");
    ASSERT_EQ(pad_reads, sizeof(late_a));
}

TEST(dialog_word_wrap) {
    pad_loop(press_a, 2);
    dialog_show("AAAA BBBB CCCC DDDD EEEE");
    ASSERT_EQ(hw_win_tile(11, 1), FONT_TILE('C'));
    ASSERT_EQ(hw_win_tile(15, 1), FONT_TILE(' '));
    ASSERT_EQ(hw_win_tile(16, 1), BOX_TILE(4));   /* DDDD did not fit in 18 columns */
    ASSERT_EQ(hw_win_tile(1, 2), FONT_TILE('D'));
    ASSERT_EQ(hw_win_tile(6, 2), FONT_TILE('E'));
}

TEST(dialog_newline_and_long_word_break) {
    pad_loop(press_a, 2);
    dialog_show("AB\nCDEFGHIJKLMNOPQRSTUV");         /* 20 letters: cut after 18 */
    ASSERT_EQ(hw_win_tile(1, 1), FONT_TILE('A'));
    ASSERT_EQ(hw_win_tile(1, 2), FONT_TILE('C'));
    ASSERT_EQ(hw_win_tile(18, 2), FONT_TILE('T'));
    ASSERT_EQ(hw_win_tile(1, 3), FONT_TILE('U'));
}

TEST(dialog_form_feed_starts_new_page) {
    pad_loop(press_a, 2);
    dialog_show("ONE\fTWO");
    ASSERT_EQ(hw_win_tile(1, 1), FONT_TILE('T'));
    ASSERT_EQ(hw_win_tile(3, 1), FONT_TILE('O'));
    ASSERT_EQ(hw_win_tile(4, 1), BOX_TILE(4));
}

TEST(dialog_full_box_continues_on_next_page) {
    pad_loop(press_a, 2);
    dialog_show("A\nB\nC\nD");
    ASSERT_EQ(hw_win_tile(1, 1), FONT_TILE('D'));
    ASSERT_EQ(hw_win_tile(1, 2), BOX_TILE(4));
    ASSERT_EQ(hw_win_tile(1, 3), BOX_TILE(4));
}

TEST(choice_down_down_a) {
    static const uint8_t s[] = { 0, J_DOWN, 0, J_DOWN, 0, J_A };
    pad_script(s, sizeof(s));
    ASSERT_EQ(dialog_choice("PICK", yes_no_maybe, 3), 2);
    ASSERT(!(LCDC_REG & LCDCF_WINON));
    ASSERT_EQ(WY_REG, 144 - 6 * 8);
    ASSERT_EQ(hw_win_tile(1, 1), FONT_TILE('P'));
    ASSERT_EQ(hw_win_tile(3, 2), FONT_TILE('Y'));
    ASSERT_EQ(hw_win_tile(3, 4), FONT_TILE('M'));
    ASSERT_EQ(hw_win_tile(1, 4), FONT_TILE('>'));
    ASSERT_EQ(hw_win_tile(1, 2), TILE_BLANK);
    ASSERT_EQ(hw_win_tile(0, 5), BOX_TILE(6));
}

TEST(choice_wraps_both_ways) {
    static const uint8_t up[] = { 0, J_UP, 0, J_A };
    static const uint8_t down3[] = { 0, J_DOWN, 0, J_DOWN, 0, J_DOWN, 0, J_START };
    pad_script(up, sizeof(up));
    ASSERT_EQ(dialog_choice("?", yes_no_maybe, 3), 2);
    pad_script(down3, sizeof(down3));
    ASSERT_EQ(dialog_choice("?", yes_no_maybe, 3), 0);
}

TEST(menu_run_selects_and_draws_cursor) {
    static const uint8_t s[] = { 0, J_DOWN, 0, J_A };
    pad_script(s, sizeof(s));
    ASSERT_EQ(menu_run(4, 5, start_options, 2), 1);
    ASSERT_EQ(hw_bkg_tile(6, 5), FONT_TILE('S'));
    ASSERT_EQ(hw_bkg_tile(6, 6), FONT_TILE('O'));
    ASSERT_EQ(hw_bkg_tile(4, 6), FONT_TILE('>'));
    ASSERT_EQ(hw_bkg_tile(4, 5), FONT_TILE(' '));
}

TEST(menu_run_wraps_and_accepts_start) {
    static const uint8_t up[] = { 0, J_UP, 0, J_A };
    static const uint8_t down2[] = { 0, J_DOWN, 0, J_DOWN, 0, J_START };
    pad_script(up, sizeof(up));
    ASSERT_EQ(menu_run(0, 0, start_options, 2), 1);
    pad_script(down2, sizeof(down2));
    ASSERT_EQ(menu_run(0, 0, start_options, 2), 0);
}

TEST(menu_run_ignores_other_keys) {
    static const uint8_t s[] = { 0, J_B, 0, J_LEFT, 0, J_SELECT, 0, J_A };
    pad_script(s, sizeof(s));
    ASSERT_EQ(menu_run(0, 0, start_options, 2), 0);
    ASSERT_EQ(pad_reads, sizeof(s));
}

void unit_tests(void) BANKED {
    RUN(font_tile_macro);
    RUN(text_init_loads_font_and_box_tiles);
    RUN(set_colors_uses_bkg_palette_7);
    RUN(print_writes_font_tiles_and_ui_attr);
    RUN(print_newline_returns_to_x);
    RUN(print_is_relative_to_camera);
    RUN(print_wraps_around_bg_map);
    RUN(print_win_uses_window_map);
    RUN(print_num_zero_pads);
    RUN(print_num_keeps_lowest_digits);
    RUN(print_num_max_5_digits);
    RUN(print_num_win);
    RUN(clear_blanks_a_rectangle);
    RUN(clear_is_relative_to_camera);
    RUN(box_draw_win_frame);
    RUN(dialog_shows_text_then_hides);
    RUN(dialog_waits_for_a_press);
    RUN(dialog_word_wrap);
    RUN(dialog_newline_and_long_word_break);
    RUN(dialog_form_feed_starts_new_page);
    RUN(dialog_full_box_continues_on_next_page);
    RUN(choice_down_down_a);
    RUN(choice_wraps_both_ways);
    RUN(menu_run_selects_and_draws_cursor);
    RUN(menu_run_wraps_and_accepts_start);
    RUN(menu_run_ignores_other_keys);
}
