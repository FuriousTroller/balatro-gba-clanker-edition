#include "voucher.h"
#include "game.h"
#include "list.h"
#include "joker.h"
#include "graphic_utils.h"
#include <tonc.h>
#include <maxmod.h>
#include <string.h>

#include "audio_utils.h"
#include "soundbank.h"

#include "sprite.h"
#include "affine_background.h"
extern List _shop_jokers_list;
extern const Rect POP_MENU_ANIM_RECT;
void game_force_shop_background_redraw(void);

extern const u16 background_shop_gfxMap[1024];

const int SPRITE_CENTER_X = 140; 
const int TEXT_CENTER_X = 156;
const int CENTER_VOUCHER_Y = 80;
const int TEXT_NAME_Y = 65; 

// ---> LOWERED SPACING: Pushed down from 115 to 124! <---
const int TEXT_REDEEMED_Y = 124; 

void present_voucher_redeemed_screen(const VoucherInfo* info) 
{
    if (info == NULL) return; 
    VoucherObject* v_obj = get_current_shop_voucher_object();

    tte_erase_rect(72, 56, 240, 160); 

    // --- PHASE 1: SLIDE DOWN ---
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

        if (v_obj) {
            v_obj->sprite_object->tx = int2fx(SPRITE_CENTER_X);
            v_obj->sprite_object->ty = int2fx(CENTER_VOUCHER_Y);
            sprite_object_update(v_obj->sprite_object);
        }
        
        sprite_draw();
    }

    // --- PHASE 2: THE REVEAL, SHAKE & TYPEWRITER TEXT ---
    int name_length = strlen(info->name);
    int text_name_x = TEXT_CENTER_X - (name_length * 4); 
    int text_redeemed_x = TEXT_CENTER_X - (9 * 4); 

    play_sfx(SFX_CARD_SELECT, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);

    int current_name_chars = 0;
    int current_red_chars = 0;
    int frame = 0;

    while(true) {
        VBlankIntrWait();
        mmFrame();  
        affine_background_update(); 
        key_poll(); 

        // 1. Typewriter Logic (Updates every 2 frames for a smooth, readable speed)
        if (frame % 2 == 0) {
            bool text_updated = false;
            
            // Type the name first...
            if (current_name_chars < name_length) {
                current_name_chars++;
                text_updated = true;
            } 
            // ...then type "Redeemed!"
            else if (current_red_chars < 9) {
                current_red_chars++;
                text_updated = true;
            }

            if (text_updated) {
                char name_buf[32] = {0};
                char red_buf[16] = {0};
                strncpy(name_buf, info->name, current_name_chars);
                strncpy(red_buf, "Redeemed!", current_red_chars);

                // Safely erase ONLY the text rows inside the menu area to prevent ghosting
                // X starts at 72 to perfectly protect your left-side HUD!
                tte_erase_rect(72, TEXT_NAME_Y, 240, TEXT_NAME_Y + 16);
                tte_erase_rect(72, TEXT_REDEEMED_Y, 240, TEXT_REDEEMED_Y + 16);

                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_name_x, TEXT_NAME_Y, TTE_WHITE_PB, name_buf);
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_redeemed_x, TEXT_REDEEMED_Y, TTE_WHITE_PB, red_buf);
            }
        }

        // 2. Shake Logic
        if (v_obj) {
            if (frame <= 12) {
                int shake_x = (frame < 12) ? ((frame % 2 == 0) ? 2 : -2) : 0;
                int shake_y = (frame < 12) ? ((frame % 3 == 0) ? 2 : -2) : 0;
                
                v_obj->sprite_object->x = int2fx(SPRITE_CENTER_X + shake_x);
                v_obj->sprite_object->y = int2fx(CENTER_VOUCHER_Y + shake_y);
                v_obj->sprite_object->tx = int2fx(SPRITE_CENTER_X + shake_x);
                v_obj->sprite_object->ty = int2fx(CENTER_VOUCHER_Y + shake_y);
            }
            sprite_object_update(v_obj->sprite_object);
        }
        frame++;
        
        sprite_draw();

        // 3. QoL Skip Logic
        if (key_hit(KEY_A | KEY_B)) {
            // If text is still typing, instantly finish it!
            if (current_name_chars < name_length || current_red_chars < 9) {
                current_name_chars = name_length;
                current_red_chars = 9;
                
                tte_erase_rect(72, TEXT_NAME_Y, 240, TEXT_NAME_Y + 16);
                tte_erase_rect(72, TEXT_REDEEMED_Y, 240, TEXT_REDEEMED_Y + 16);
                
                tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_name_x, TEXT_NAME_Y, TTE_WHITE_PB, info->name);
                tte_printf("#{P:%d,%d; cx:0x%X000}Redeemed!", text_redeemed_x, TEXT_REDEEMED_Y, TTE_WHITE_PB);
            } 
            // If text is already done, proceed to slide up!
            else {
                break;
            }
        }
    }

    // --- PHASE 3: SLIDE UP ---
    tte_erase_rect(72, 56, 240, 160); 

    if (v_obj) {
        v_obj->sprite_object->x = int2fx(SPRITE_CENTER_X);
        v_obj->sprite_object->y = int2fx(160);
        v_obj->sprite_object->tx = int2fx(SPRITE_CENTER_X);
        v_obj->sprite_object->ty = int2fx(160);
        sprite_object_update(v_obj->sprite_object);
    }

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