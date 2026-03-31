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
extern List _shop_packs_list;
extern const Rect POP_MENU_ANIM_RECT;
extern const u16 background_shop_gfxMap[1024];

void game_force_shop_background_redraw(void);

// --- UI MAP COORDINATES (STORAGE) ---
const Rect SKIP_BTN_SRC_RECT   = {26, 20, 29, 22}; 
const Rect SELECT_BTN_SRC_RECT = {26, 22, 29, 24}; 

// --- SCREEN DESTINATIONS (PERFECTLY ALIGNED) ---
const BG_POINT SKIP_BTN_DEST   = {26, 7};  
const BG_POINT SELECT_BTN_DEST = {26, 10}; 

const int TEXT_PACK_NAME_Y = 126;
const int TEXT_CHOOSE_Y = 140;

void present_pack_opened_screen(const PackInfo* info) 
{
    if (info == NULL) return; 

    tte_erase_rect(72, 56, 240, 160); 

    VoucherObject* v_obj = get_current_shop_voucher_object();
    if (v_obj != NULL && v_obj->sprite_object != NULL) {
        v_obj->sprite_object->y = int2fx(160); 
        sprite_position(v_obj->sprite_object->sprite, fx2int(v_obj->sprite_object->tx), 160);
    }

    ListItr p_itr = list_itr_create(&_shop_packs_list);
    PackObject* p_obj;
    while ((p_obj = list_itr_next(&p_itr))) {
        p_obj->sprite_object->y = int2fx(160);
        sprite_position(p_obj->sprite_object->sprite, fx2int(p_obj->sprite_object->tx), 160);
    }

    for (int i = 0; i < 20; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 

        main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_DOWN);

        int offset = i + 1; 
        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* j_obj;
        while ((j_obj = list_itr_next(&itr))) {
            int current_y = 71 + (offset * 8); 
            if (current_y > 160) current_y = 160; 
            j_obj->sprite_object->y = int2fx(current_y);
            j_obj->sprite_object->ty = int2fx(current_y);
            joker_object_update(j_obj);
        }
        sprite_draw();
    }

    for (int i = 0; i < 6; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 

        int offset = 5 - i; 

        for (int y = 7; y <= 12; y++) {
            for (int x = 26; x <= 29; x++) { 
                se_mem[MAIN_BG_SBB][y * 32 + x] = background_shop_gfxMap[y * 32 + x]; 
            }
        }

        for (int row = 0; row < 2; row++) {
            int draw_y = SKIP_BTN_DEST.y + offset + row;
            if (draw_y <= 12) {
                memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + SKIP_BTN_DEST.x],
                         &background_shop_gfxMap[(SKIP_BTN_SRC_RECT.top + row) * 32 + SKIP_BTN_SRC_RECT.left],
                         3);
            }
        }

        for (int row = 0; row < 2; row++) {
            int draw_y = SELECT_BTN_DEST.y + offset + row;
            if (draw_y <= 12) {
                memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + SELECT_BTN_DEST.x],
                         &background_shop_gfxMap[(SELECT_BTN_SRC_RECT.top + row) * 32 + SELECT_BTN_SRC_RECT.left],
                         3); 
            }
        }
        sprite_draw();
    }

    play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);

    int name_length = strlen(info->name);
    char choose_str[16];
    snprintf(choose_str, sizeof(choose_str), "Choose %d", info->picks_allowed);
    int choose_length = strlen(choose_str);

    int text_name_x = 128 - (name_length * 4); 
    int text_choose_x = 128 - (choose_length * 4);

    int current_name_chars = 0;
    int current_choose_chars = 0;
    int frame = 0;
    bool typing_finished = false;

    // --- DYNAMIC PALETTE FINDER ---
    int skip_border_pid = -1;
    int select_border_pid = -1;
    u16 original_skip_color = 0;
    u16 original_select_color = 0;

    for (int i = 0; i < 256; i++) {
        u16 c = pal_bg_mem[i];
        int r = c & 0x1F;
        int g = (c >> 5) & 0x1F;
        int b = (c >> 10) & 0x1F;

        // Find Neon Green (Skip Border)
        if (r <= 2 && g >= 29 && b >= 5 && b <= 9) {
            skip_border_pid = i;
            original_skip_color = c;
        }
        // Find Pink (Select Border)
        if (r >= 29 && g >= 20 && g <= 24 && b >= 27 && b <= 31) {
            select_border_pid = i;
            original_select_color = c;
        }
    }

    // Set initial unselected states: Skip gets Gray (#636765), Select gets Orange (#ff8f00)
    if (skip_border_pid != -1) pal_bg_mem[skip_border_pid] = 0x318C; 
    if (select_border_pid != -1) pal_bg_mem[select_border_pid] = 0x025F; 
    
    int selected_btn = 1; // Default cursor hover to SELECT (1)

    while(true) {
        VBlankIntrWait();
        mmFrame();  
        affine_background_update(); 
        key_poll(); 

        if (!typing_finished && frame % 2 == 0) {
            bool text_updated = false;
            
            if (current_name_chars < name_length) {
                current_name_chars++;
                text_updated = true;
            } else if (current_choose_chars < choose_length) {
                current_choose_chars++;
                text_updated = true;
            }

            if (text_updated) {
                char name_buf[32] = {0};
                char choose_buf[16] = {0};
                strncpy(name_buf, info->name, current_name_chars);
                strncpy(choose_buf, choose_str, current_choose_chars);

                tte_erase_rect(72, 120, 180, 150); 
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_name_x, TEXT_PACK_NAME_Y, TTE_WHITE_PB, name_buf);
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_choose_x, TEXT_CHOOSE_Y, TTE_WHITE_PB, choose_buf);
            }

            if (current_name_chars == name_length && current_choose_chars == choose_length) {
                typing_finished = true;
            }
        }

        // --- D-PAD CURSOR LOGIC ---
        if (key_hit(KEY_UP) && selected_btn != 0) {
            selected_btn = 0; // Move to SKIP
            play_sfx(SFX_CARD_FOCUS, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
        } 
        else if (key_hit(KEY_DOWN) && selected_btn != 1) {
            selected_btn = 1; // Move to SELECT
            play_sfx(SFX_CARD_FOCUS, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
        }

        // Apply Exact User Border Colors!
        if (skip_border_pid != -1) {
            pal_bg_mem[skip_border_pid] = (selected_btn == 0) ? 0x7FFF : 0x318C; // White or Gray
        }
        if (select_border_pid != -1) {
            pal_bg_mem[select_border_pid] = (selected_btn == 1) ? 0x7FFF : 0x025F; // White or Orange
        }

        frame++;
        
        if (key_hit(KEY_A | KEY_B)) {
            if (!typing_finished) {
                current_name_chars = name_length;
                current_choose_chars = choose_length;
                typing_finished = true;
                
                tte_erase_rect(72, 120, 180, 150);
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_name_x, TEXT_PACK_NAME_Y, TTE_WHITE_PB, info->name);
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_choose_x, TEXT_CHOOSE_Y, TTE_WHITE_PB, choose_str);
            } 
            else {
                break;
            }
        }
        sprite_draw();
    }

    // --- PHASE 4: CLEAR UI & SLIDE UP ---
    // Restore the exact original colors before sliding the shop back up!
    if (skip_border_pid != -1) pal_bg_mem[skip_border_pid] = original_skip_color; 
    if (select_border_pid != -1) pal_bg_mem[select_border_pid] = original_select_color;
    
    for (int y = 7; y <= 12; y++) {
        for (int x = 26; x <= 29; x++) {
            se_mem[MAIN_BG_SBB][y * 32 + x] = background_shop_gfxMap[y * 32 + x]; 
        }
    }
    tte_erase_rect(72, 120, 180, 150); 

    int width = POP_MENU_ANIM_RECT.right - POP_MENU_ANIM_RECT.left + 1;
    for (int y = POP_MENU_ANIM_RECT.top; y <= POP_MENU_ANIM_RECT.bottom; y++) {
        memcpy16(&se_mem[MAIN_BG_SBB][y * 32 + POP_MENU_ANIM_RECT.left],
                 &background_shop_gfxMap[y * 32 + POP_MENU_ANIM_RECT.left],
                 width);
    }

    if (v_obj != NULL && v_obj->sprite_object != NULL) {
        v_obj->sprite_object->y = int2fx(128);
    }
    p_itr = list_itr_create(&_shop_packs_list);
    while ((p_obj = list_itr_next(&p_itr))) {
        if (p_obj != NULL && p_obj->sprite_object != NULL) {
            p_obj->sprite_object->y = int2fx(128);
        }
    }

    for (int i = 0; i < 12; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 
        
        main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

        int offset = 11 - i; 
        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* j_obj;
        while ((j_obj = list_itr_next(&itr))) {
            int current_y = 71 + (offset * 8);
            if (current_y > 160) current_y = 160;
            j_obj->sprite_object->y = int2fx(current_y);
            j_obj->sprite_object->ty = int2fx(current_y);
            joker_object_update(j_obj);
        }
        sprite_draw();
    }
    
    game_force_shop_background_redraw();
}

void unload_shop_sprites(void) {
    // 1. Unload all Jokers from the renderer
    ListItr j_itr = list_itr_create(&_shop_jokers_list);
    JokerObject* j_obj;
    while ((j_obj = list_itr_next(&j_itr))) {
        if (j_obj != NULL && j_obj->sprite_object != NULL) {
            j_obj->sprite_object->sprite->attr0 |= ATTR0_HIDE; // Hardware hide
        }
    }

    // 2. Unload the Voucher from the renderer
    VoucherObject* v_obj = get_current_shop_voucher_object();
    if (v_obj != NULL && v_obj->sprite_object != NULL) {
        v_obj->sprite_object->sprite->attr0 |= ATTR0_HIDE; // Hardware hide
    }
}

void reload_shop_sprites(void) {
    // 1. Bring Jokers back to the renderer
    ListItr j_itr = list_itr_create(&_shop_jokers_list);
    JokerObject* j_obj;
    while ((j_obj = list_itr_next(&j_itr))) {
        if (j_obj != NULL && j_obj->sprite_object != NULL) {
            j_obj->sprite_object->sprite->attr0 &= ~ATTR0_HIDE; // Remove hide flag
        }
    }

    // 2. Bring Voucher back to the renderer
    VoucherObject* v_obj = get_current_shop_voucher_object();
    if (v_obj != NULL && v_obj->sprite_object != NULL) {
        v_obj->sprite_object->sprite->attr0 &= ~ATTR0_HIDE; // Remove hide flag
    }
}