#include "pack.h"
#include "game.h"
#include "list.h"
#include "joker.h"
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

// --- UI MAP COORDINATES ---
const Rect PACK_INFO_SRC_RECT = {9, 6, 19, 10}; // 11x5 tiles
const Rect SKIP_BTN_SRC_RECT  = {21, 9, 24, 10}; // 4x2 tiles

// Screen Destinations (Tile Coordinates)
const BG_POINT PACK_INFO_DEST = {10, 14}; 
const BG_POINT SKIP_BTN_DEST  = {21, 14};

// Text Y-Coordinates inside the panel
const int TEXT_PACK_NAME_Y = 118;
const int TEXT_CHOOSE_Y = 132;

void present_pack_opened_screen(const PackInfo* info) 
{
    if (info == NULL) return; 

    // Clear any lingering shop text
    tte_erase_rect(72, 56, 240, 160); 

    // --- PHASE 1: SLIDE DOWN THE SHOP ---
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

    // --- PHASE 2: UI POP-UP ANIMATION ---
    // A crisp 6-frame animation sliding the UI up from the bottom border!
    for (int i = 0; i < 6; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 

        int offset = 5 - i; 

        // 1. Safely wipe the animation area (X: 10 to 24, Y: 14 to 19) with the green background
        for (int y = 14; y <= 19; y++) {
            for (int x = 10; x <= 24; x++) {
                se_mem[MAIN_BG_SBB][y * 32 + x] = background_shop_gfxMap[5 * 32 + x]; 
            }
        }

        // 2. Draw Pack Info Panel, dynamically clipping it if it falls below the screen (Y > 19)
        for (int row = 0; row < 5; row++) {
            int draw_y = PACK_INFO_DEST.y + offset + row;
            if (draw_y <= 19) {
                memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + PACK_INFO_DEST.x],
                         &background_shop_gfxMap[(PACK_INFO_SRC_RECT.top + row) * 32 + PACK_INFO_SRC_RECT.left],
                         11);
            }
        }

        // 3. Draw Skip Button, dynamically clipping it if it falls below the screen
        for (int row = 0; row < 2; row++) {
            int draw_y = SKIP_BTN_DEST.y + offset + row;
            if (draw_y <= 19) {
                memcpy16(&se_mem[MAIN_BG_SBB][draw_y * 32 + SKIP_BTN_DEST.x],
                         &background_shop_gfxMap[(SKIP_BTN_SRC_RECT.top + row) * 32 + SKIP_BTN_SRC_RECT.left],
                         4);
            }
        }
        
        // TODO: In the future, we will link the generated Pack Cards to this loop
        // so they perfectly slide up from the floor alongside the UI!
        
        sprite_draw();
    }

    // --- PHASE 3: TYPEWRITER REVEAL & INTERACTIVE LOOP ---
    play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);

    int name_length = strlen(info->name);
    
    char choose_str[16];
    snprintf(choose_str, sizeof(choose_str), "Choose %d", info->picks_allowed);
    int choose_length = strlen(choose_str);

    int text_name_x = 124 - (name_length * 4); 
    int text_choose_x = 124 - (choose_length * 4);

    int current_name_chars = 0;
    int current_choose_chars = 0;
    int frame = 0;
    bool typing_finished = false;

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
            } 
            else if (current_choose_chars < choose_length) {
                current_choose_chars++;
                text_updated = true;
            }

            if (text_updated) {
                char name_buf[32] = {0};
                char choose_buf[16] = {0};
                strncpy(name_buf, info->name, current_name_chars);
                strncpy(choose_buf, choose_str, current_choose_chars);

                tte_erase_rect(82, 116, 166, 148);
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_name_x, TEXT_PACK_NAME_Y, TTE_WHITE_PB, name_buf);
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_choose_x, TEXT_CHOOSE_Y, TTE_WHITE_PB, choose_buf);
            }

            if (current_name_chars == name_length && current_choose_chars == choose_length) {
                typing_finished = true;
            }
        }

        frame++;
        
        if (key_hit(KEY_A | KEY_B)) {
            if (!typing_finished) {
                current_name_chars = name_length;
                current_choose_chars = choose_length;
                typing_finished = true;
                
                tte_erase_rect(82, 116, 166, 148);
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
    tte_erase_rect(72, 56, 240, 160); 

    int width = POP_MENU_ANIM_RECT.right - POP_MENU_ANIM_RECT.left + 1;
    for (int y = POP_MENU_ANIM_RECT.top; y <= POP_MENU_ANIM_RECT.bottom; y++) {
        memcpy16(&se_mem[MAIN_BG_SBB][y * 32 + POP_MENU_ANIM_RECT.left],
                 &background_shop_gfxMap[y * 32 + POP_MENU_ANIM_RECT.left],
                 width);
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