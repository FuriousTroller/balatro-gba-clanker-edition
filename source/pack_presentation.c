#include "pack.h"
#include "game.h"
#include "list.h"
#include "joker.h"
#include "voucher.h"
#include "graphic_utils.h"
#include <tonc.h>
#include <maxmod.h>
#include <string.h>
#include <stdio.h> 
#include "audio_utils.h"
#include "soundbank.h"

extern void sprite_draw(void);
extern void affine_background_update(void); 
extern List _shop_jokers_list;
extern const Rect POP_MENU_ANIM_RECT;
extern const u16 background_shop_gfxMap[1024];

void game_force_shop_background_redraw(void);

extern int max_jokers;
extern List _pack_cards_list;

// --- UI MAP COORDINATES ---
// Source rects in the background_shop_gfxMap tilemap (tile col, tile row, inclusive end col, inclusive end row)
const Rect PACK_INFO_SRC_RECT = {9, 6, 19, 10}; // 11x5 tiles
// SKIP button: PNG pixel (208,160) -> tile (26,20), 24x16px = 3 wide x 2 tall
const Rect SKIP_BTN_SRC_RECT   = {26, 20, 28, 21}; // 3x2 tiles
// SELECT button: PNG pixel (208,176) -> tile (26,22), 24x16px = 3 wide x 2 tall
const Rect SELECT_BTN_SRC_RECT  = {26, 22, 28, 23}; // 3x2 tiles

// Screen Destinations (Tile Coordinates)
// Pack info panel: bottom of screen (rows 14-18, cols 10-20)
const BG_POINT PACK_INFO_DEST   = {10, 14};
// Buttons: right side of joker area (x=208px = tile col 26), at joker height
// SKIP on top (tile row 7 = y=56px), SELECT below (tile row 9 = y=72px)
const BG_POINT SKIP_BTN_DEST    = {26, 7};
const BG_POINT SELECT_BTN_DEST  = {26, 9};

// Text Y-Coordinates inside the panel
const int TEXT_PACK_NAME_Y = 132;
const int TEXT_CHOOSE_Y = 142;

// --- BUTTON HIGHLIGHT COLORS (GBA BGR555) ---
// SKIP border: 0x00FF3A -> R=0,G=31,B=7
#define SKIP_BORDER_DEFAULT   RGB15(0, 31, 7)
// SKIP unselected: 0x636765 gray -> R=12,G=12,B=12
#define SKIP_BORDER_UNSEL     RGB15(12, 12, 12)
// SELECT border: 0xFFB6EB -> R=31,G=22,B=29
#define SELECT_BORDER_DEFAULT RGB15(31, 22, 29)
// SELECT unselected: 0xFF8F00 orange -> R=31,G=17,B=0
#define SELECT_BORDER_UNSEL   RGB15(31, 17, 0)
// Highlight color: white
#define BTN_HIGHLIGHT_CLR     CLR_WHITE

// Cached palette indices for button borders (found at runtime)
static int skip_border_pal_idx = -1;
static int select_border_pal_idx = -1;

// Find palette indices for button border colors by scanning pal_bg_mem
static void find_button_palette_indices(void)
{
    skip_border_pal_idx = -1;
    select_border_pal_idx = -1;
    for (int i = 0; i < 256; i++) {
        if (pal_bg_mem[i] == SKIP_BORDER_DEFAULT && skip_border_pal_idx < 0)
            skip_border_pal_idx = i;
        if (pal_bg_mem[i] == SELECT_BORDER_DEFAULT && select_border_pal_idx < 0)
            select_border_pal_idx = i;
    }
}

// Set button border highlight based on which button is focused
// focus: 0 = on cards (default colors), 1 = SKIP, 2 = SELECT
static void update_button_highlight(int focus)
{
    if (focus == 1) {
        // SKIP selected
        if (skip_border_pal_idx >= 0) pal_bg_mem[skip_border_pal_idx] = BTN_HIGHLIGHT_CLR;
        if (select_border_pal_idx >= 0) pal_bg_mem[select_border_pal_idx] = SELECT_BORDER_UNSEL;
    } else if (focus == 2) {
        // SELECT selected
        if (select_border_pal_idx >= 0) pal_bg_mem[select_border_pal_idx] = BTN_HIGHLIGHT_CLR;
        if (skip_border_pal_idx >= 0) pal_bg_mem[skip_border_pal_idx] = SKIP_BORDER_UNSEL;
    } else {
        // Default (on cards) - set both to their unselected colors
        if (skip_border_pal_idx >= 0) pal_bg_mem[skip_border_pal_idx] = SKIP_BORDER_UNSEL;
        if (select_border_pal_idx >= 0) pal_bg_mem[select_border_pal_idx] = SELECT_BORDER_UNSEL;
    }
}

// Restore button palette to defaults
static void restore_button_palette(void)
{
    if (skip_border_pal_idx >= 0) pal_bg_mem[skip_border_pal_idx] = SKIP_BORDER_DEFAULT;
    if (select_border_pal_idx >= 0) pal_bg_mem[select_border_pal_idx] = SELECT_BORDER_DEFAULT;
}

static void restore_shop_bottom_ui(void)
{
    // 1. Clear the bottom panel area (rows 14-19, cols 10-25) with transparent tiles
    //    Transparent (0x0000) lets the affine green checkerboard BG show through
    for (int y = 14; y <= 19; y++) {
        for (int x = 10; x <= 25; x++) {
            se_mem[MAIN_BG_SBB][y * 32 + x] = 0x0000;
        }
    }

    // 2. Clear the button area (rows 7-10, cols 26-28) with transparent tiles
    for (int y = SKIP_BTN_DEST.y; y <= SELECT_BTN_DEST.y + 1; y++) {
        for (int x = SKIP_BTN_DEST.x; x <= SKIP_BTN_DEST.x + 2; x++) {
            se_mem[MAIN_BG_SBB][y * 32 + x] = 0x0000;
        }
    }

    // 3. Redraw the Pack Info Panel
    for (int row = 0; row < 5; row++) {
        int draw_y = PACK_INFO_DEST.y + row;
        if (draw_y <= 19) {
            memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + PACK_INFO_DEST.x],
                     &background_shop_gfxMap[(PACK_INFO_SRC_RECT.top + row) * 32 + PACK_INFO_SRC_RECT.left],
                     11);
        }
    }

    // 4. Redraw SKIP button (top, at tile row 7)
    for (int row = 0; row < 2; row++) {
        int draw_y = SKIP_BTN_DEST.y + row;
        if (draw_y <= 19) {
            memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + SKIP_BTN_DEST.x],
                     &background_shop_gfxMap[(SKIP_BTN_SRC_RECT.top + row) * 32 + SKIP_BTN_SRC_RECT.left],
                     3);
        }
    }

    // 5. Redraw SELECT button (below SKIP, at tile row 9)
    for (int row = 0; row < 2; row++) {
        int draw_y = SELECT_BTN_DEST.y + row;
        if (draw_y <= 19) {
            memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + SELECT_BTN_DEST.x],
                     &background_shop_gfxMap[(SELECT_BTN_SRC_RECT.top + row) * 32 + SELECT_BTN_SRC_RECT.left],
                     3);
        }
    }
}

void present_pack_opened_screen(const PackInfo* info) 
{
    if (info == NULL) return; 

    // Clear any lingering shop text
    tte_erase_rect(72, 56, 240, 160); 

    // Spawn the pack cards
    spawn_pack_cards(info);

    // Find button palette indices for highlight effects
    find_button_palette_indices();
    update_button_highlight(0);

    // --- PHASE 1: SLIDE DOWN THE SHOP ---
    for (int i = 0; i < 20; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 

        main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_DOWN);

        int offset = i + 1; 

        // 1. Slide down shop Jokers
        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* j_obj;
        while ((j_obj = list_itr_next(&itr))) {
            int current_y = 71 + (offset * 8); 
            if (current_y > 160) current_y = 160; 
            j_obj->sprite_object->y = int2fx(current_y);
            j_obj->sprite_object->ty = int2fx(current_y);
            joker_object_update(j_obj);
        }

        // 2. Slide down Voucher
        VoucherObject* v = get_current_shop_voucher_object();
        if (v != NULL && v->sprite_object != NULL) {
            int current_y = 112 + (offset * 8);
            if (current_y > 160) current_y = 160;
            v->sprite_object->y = int2fx(current_y);
            v->sprite_object->ty = int2fx(current_y);
            sprite_object_update(v->sprite_object);
        }

        // 3. Slide down other Booster Packs
        ListItr pack_itr = list_itr_create(&_shop_packs_list);
        PackObject* pack;
        while ((pack = list_itr_next(&pack_itr))) {
            if (pack != NULL && pack->sprite_object != NULL) {
                int current_y = 112 + (offset * 8);
                if (current_y > 160) current_y = 160;
                pack->sprite_object->y = int2fx(current_y);
                pack->sprite_object->ty = int2fx(current_y);
                sprite_object_update(pack->sprite_object);
            }
        }
        
        sprite_draw();
    }

    // --- PHASE 2: UI POP-UP ANIMATION ---
    // A crisp 6-frame animation sliding the UI up from the bottom border!
    for (int i = 0; i < 6; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 

        int offset = 5 - i; 

        // 1. Clear bottom panel area with transparent tiles
        for (int y = 14; y <= 19; y++) {
            for (int x = 10; x <= 25; x++) {
                se_mem[MAIN_BG_SBB][y * 32 + x] = 0x0000;
            }
        }

        // 2. Clear button area with transparent tiles
        for (int y = SKIP_BTN_DEST.y; y <= SELECT_BTN_DEST.y + 1; y++) {
            for (int x = SKIP_BTN_DEST.x; x <= SKIP_BTN_DEST.x + 2; x++) {
                se_mem[MAIN_BG_SBB][y * 32 + x] = 0x0000;
            }
        }

        // 3. Draw Pack Info Panel, dynamically clipping (Y > 19)
        for (int row = 0; row < 5; row++) {
            int draw_y = PACK_INFO_DEST.y + offset + row;
            if (draw_y <= 19) {
                memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + PACK_INFO_DEST.x],
                         &background_shop_gfxMap[(PACK_INFO_SRC_RECT.top + row) * 32 + PACK_INFO_SRC_RECT.left],
                         11);
            }
        }

        // 4. Draw SKIP button at fixed position (right side, top)
        for (int row = 0; row < 2; row++) {
            int draw_y = SKIP_BTN_DEST.y + row;
            if (draw_y <= 19) {
                memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + SKIP_BTN_DEST.x],
                         &background_shop_gfxMap[(SKIP_BTN_SRC_RECT.top + row) * 32 + SKIP_BTN_SRC_RECT.left],
                         3);
            }
        }

        // 5. Draw SELECT button at fixed position (right side, below SKIP)
        for (int row = 0; row < 2; row++) {
            int draw_y = SELECT_BTN_DEST.y + row;
            if (draw_y <= 19) {
                memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + SELECT_BTN_DEST.x],
                         &background_shop_gfxMap[(SELECT_BTN_SRC_RECT.top + row) * 32 + SELECT_BTN_SRC_RECT.left],
                         3);
            }
        }
        
        // Update pack cards to let them slide up nicely alongside the UI!
        ListItr card_itr = list_itr_create(&_pack_cards_list);
        JokerObject* card;
        while ((card = list_itr_next(&card_itr))) {
            joker_object_update(card);
        }
        
        sprite_draw();
    }

    // --- PHASE 3: TYPEWRITER REVEAL & INTERACTIVE LOOP ---
    play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);

    char choose_str[64];
    snprintf(choose_str, sizeof(choose_str), "Choose %d", info->picks_allowed);
    int choose_length = strlen(choose_str);

    // Center text within the pack info panel inner area
    // Panel spans cols 10-20 (88px), inner area ~cols 11-19 (72px), center at pixel 120
    int text_center = 120;
    int text_name_x = text_center - (strlen(info->name) * 3);
    int text_choose_x = text_center - (choose_length * 3);

    int name_length = strlen(info->name);
    int current_name_chars = 0;
    int current_choose_chars = 0;
    int frame = 0;
    bool typing_finished = false;

    // Typewriter loop
    while (!typing_finished) {
        VBlankIntrWait();
        mmFrame();  
        affine_background_update(); 
        key_poll(); 

        // Update card positions so they continue sliding up
        ListItr card_itr = list_itr_create(&_pack_cards_list);
        JokerObject* card;
        while ((card = list_itr_next(&card_itr))) {
            joker_object_update(card);
        }

        if (frame % 2 == 0) {
            bool text_updated = false;
            
            if (current_name_chars < name_length) {
                current_name_chars++;
                text_updated = true;
            } 
            else if (current_choose_chars < choose_length) {
                current_choose_chars++;
                text_updated = true;
            }

            if (text_updated) {
                char name_buf[64] = {0};
                char choose_buf[64] = {0};
                strncpy(name_buf, info->name, current_name_chars);
                strncpy(choose_buf, choose_str, current_choose_chars);

                restore_shop_bottom_ui();
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_name_x, TEXT_PACK_NAME_Y, TTE_WHITE_PB, name_buf);
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_choose_x, TEXT_CHOOSE_Y, TTE_WHITE_PB, choose_buf);
            }

            if (current_name_chars == name_length && current_choose_chars == choose_length) {
                typing_finished = true;
            }
        }

        frame++;
        
        // Any key hit will skip the typewriter
        if (key_hit(KEY_A | KEY_B | KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)) {
            typing_finished = true;
        }
        
        sprite_draw();
    }

    // Ensure final text is fully drawn
    restore_shop_bottom_ui();
    tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_name_x, TEXT_PACK_NAME_Y, TTE_WHITE_PB, info->name);
    tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_choose_x, TEXT_CHOOSE_Y, TTE_WHITE_PB, choose_str);

    // Interactive Loop
    // Cursor layout: 0..count-1 = cards, count = SKIP, count+1 = SELECT
    int cursor = 0;
    int last_card_cursor = 0; // Track last highlighted card for SELECT action
    bool needs_update = true;

    while(true) {
        VBlankIntrWait();
        mmFrame();  
        affine_background_update(); 
        key_poll(); 

        int count = list_get_len(&_pack_cards_list);

        if (needs_update) {
            // Update card targets based on cursor
            for (int i = 0; i < count; i++) {
                JokerObject* card = (JokerObject*)list_get_at_idx(&_pack_cards_list, i);
                if (card != NULL) {
                    if (cursor < count && cursor == i) {
                        card->sprite_object->ty = int2fx(60); // lift up
                        last_card_cursor = i;
                    } else if (cursor >= count && last_card_cursor == i) {
                        card->sprite_object->ty = int2fx(60); // keep last selected card lifted up when navigating to buttons
                    } else {
                        card->sprite_object->ty = int2fx(70); // down
                    }
                }
            }

            // Update button highlight based on cursor position
            if (cursor == count) {
                update_button_highlight(1); // SKIP focused
            } else if (cursor == count + 1) {
                update_button_highlight(2); // SELECT focused
            } else {
                update_button_highlight(0); // On cards, default
            }

            // Restore bottom background and text to avoid leaving marks
            restore_shop_bottom_ui();
            tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_name_x, TEXT_PACK_NAME_Y, TTE_WHITE_PB, info->name);
            tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_choose_x, TEXT_CHOOSE_Y, TTE_WHITE_PB, choose_str);

            needs_update = false;
        }

        // Handle navigation
        if (key_hit(KEY_LEFT)) {
            if (cursor > 0) {
                // If on buttons (count or count+1), jump back to last card
                if (cursor >= count) {
                    cursor = count - 1;
                } else {
                    cursor--;
                }
                needs_update = true;
                play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
            }
        }
        else if (key_hit(KEY_RIGHT)) {
            if (cursor < count + 1) {
                cursor++;
                needs_update = true;
                play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
            }
        }

        // UP/DOWN to navigate between SKIP and SELECT when on buttons
        if (key_hit(KEY_UP)) {
            if (cursor == count + 1) {
                cursor = count; // SELECT -> SKIP
                needs_update = true;
                play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
            }
        }
        else if (key_hit(KEY_DOWN)) {
            if (cursor == count) {
                cursor = count + 1; // SKIP -> SELECT
                needs_update = true;
                play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
            }
        }

        // Handle Confirmation
        if (key_hit(KEY_A)) {
            if (cursor < count) {
                // Pressing A on a card selects/highlights it and jumps cursor to the SELECT button
                last_card_cursor = cursor;
                cursor = count + 1; // Jump to SELECT button
                needs_update = true;
                play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
            } else if (cursor == count) {
                // Confirmed SKIP button!
                break;
            } else if (cursor == count + 1) {
                // Confirmed SELECT button - pick the last highlighted card
                if (last_card_cursor >= 0 && last_card_cursor < count) {
                    int owned_count = list_get_len(get_jokers_list());
                    if (owned_count < max_jokers) {
                        JokerObject* chosen_card = (JokerObject*)list_get_at_idx(&_pack_cards_list, last_card_cursor);
                        
                        joker_object_shake(chosen_card, SFX_CARD_SELECT);
                        
                        for (int f = 0; f < 30; f++) {
                            VBlankIntrWait();
                            mmFrame();
                            affine_background_update();
                            
                            ListItr card_itr = list_itr_create(&_pack_cards_list);
                            JokerObject* card;
                            while ((card = list_itr_next(&card_itr))) {
                                joker_object_update(card);
                            }
                            sprite_draw();
                        }
                        
                        list_remove_at_idx(&_pack_cards_list, last_card_cursor);
                        chosen_card->sprite_object->ty = int2fx(HELD_JOKERS_POS.y);
                        add_joker(chosen_card);
                        
                        break;
                    } else {
                        JokerObject* chosen_card = (JokerObject*)list_get_at_idx(&_pack_cards_list, last_card_cursor);
                        joker_object_shake(chosen_card, SFX_CARD_SELECT);
                    }
                }
            }
        }

        // Press B to skip
        if (key_hit(KEY_B)) {
            break;
        }

        // Lerp positions
        ListItr card_itr = list_itr_create(&_pack_cards_list);
        JokerObject* card;
        while ((card = list_itr_next(&card_itr))) {
            joker_object_update(card);
        }
        
        sprite_draw();
    }

    // --- PHASE 4: CLEAR UI & SLIDE UP ---
    // Restore button palette to defaults before leaving
    restore_button_palette();

    tte_erase_rect(72, 56, 240, 160); 

    int width = POP_MENU_ANIM_RECT.right - POP_MENU_ANIM_RECT.left + 1;
    for (int y = POP_MENU_ANIM_RECT.top; y <= POP_MENU_ANIM_RECT.bottom; y++) {
        memcpy16(&se_mem[MAIN_BG_SBB][y * 32 + POP_MENU_ANIM_RECT.left],
                 &background_shop_gfxMap[y * 32 + POP_MENU_ANIM_RECT.left],
                 width);
    }

    // Also restore the SKIP/SELECT button area (rows 7-10, cols 26-28) back to shop background
    for (int y = SKIP_BTN_DEST.y; y <= SELECT_BTN_DEST.y + 1; y++) {
        for (int x = SKIP_BTN_DEST.x; x <= SKIP_BTN_DEST.x + 2; x++) {
            se_mem[MAIN_BG_SBB][y * 32 + x] = background_shop_gfxMap[y * 32 + x];
        }
    }

    for (int i = 0; i < 12; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 
        
        main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

        int offset = 11 - i; 

        // 1. Slide up shop Jokers
        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* j_obj;
        while ((j_obj = list_itr_next(&itr))) {
            int current_y = 71 + (offset * 8);
            if (current_y > 160) current_y = 160;
            j_obj->sprite_object->y = int2fx(current_y);
            j_obj->sprite_object->ty = int2fx(current_y);
            joker_object_update(j_obj);
        }

        // 2. Slide up Voucher
        VoucherObject* v = get_current_shop_voucher_object();
        if (v != NULL && v->sprite_object != NULL) {
            int current_y = 112 + (offset * 8);
            if (current_y > 160) current_y = 160;
            v->sprite_object->y = int2fx(current_y);
            v->sprite_object->ty = int2fx(current_y);
            sprite_object_update(v->sprite_object);
        }

        // 3. Slide up Booster Packs
        ListItr pack_itr = list_itr_create(&_shop_packs_list);
        PackObject* pack;
        while ((pack = list_itr_next(&pack_itr))) {
            if (pack != NULL && pack->sprite_object != NULL) {
                int current_y = 112 + (offset * 8);
                if (current_y > 160) current_y = 160;
                pack->sprite_object->y = int2fx(current_y);
                pack->sprite_object->ty = int2fx(current_y);
                sprite_object_update(pack->sprite_object);
            }
        }
        
        sprite_draw();
    }
    
    // Clear any remaining card sprites
    despawn_pack_cards();
    
    game_force_shop_background_redraw();
}