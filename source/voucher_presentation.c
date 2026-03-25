#include "voucher.h"
#include "game.h"
#include "list.h"
#include "joker.h"
#include "graphic_utils.h"
#include <tonc.h>
#include <maxmod.h>
#include <string.h> // Needed to dynamically center the text!

extern void sprite_draw(void);
extern void affine_background_update(void); 
extern List _shop_jokers_list;
extern const Rect POP_MENU_ANIM_RECT;
void game_force_shop_background_redraw(void);

const int SPRITE_CENTER_X = 112; 
const int CENTER_VOUCHER_Y = 80;
const int TEXT_NAME_Y = 65; 
const int TEXT_REDEEMED_Y = 115; 

void present_voucher_redeemed_screen(const VoucherInfo* info) 
{
    if (info == NULL) return; 
    VoucherObject* v_obj = get_current_shop_voucher_object();

    // --- 0. THE SNAPSHOT ---
    u16 shop_bg_snapshot[1024]; 
    memcpy32(shop_bg_snapshot, &se_mem[MAIN_BG_SBB][0], 512);

    tte_erase_rect(72, 56, 200, 160); 

    // --- PHASE 1: SLIDE DOWN & HIDE JOKERS ---
    for (int i = 0; i < 20; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 

        main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_DOWN);

        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* j_obj;
        while ((j_obj = list_itr_next(&itr))) {
            j_obj->sprite_object->ty = int2fx(160); 
            j_obj->sprite_object->y = int2fx(160); 
            joker_object_update(j_obj);
        }

        if (v_obj) {
            v_obj->sprite_object->tx = int2fx(SPRITE_CENTER_X);
            v_obj->sprite_object->ty = int2fx(CENTER_VOUCHER_Y);
            sprite_object_update(v_obj->sprite_object);
        }
        
        sprite_draw();
    }

    // --- PHASE 2: THE REVEAL & SHAKE ---
    // Dynamically calculate the perfect X coordinates based on text length!
    // The visual center of our 32x32 sprite is at pixel 128 (112 + 16px).
    // The font is 8 pixels wide per character.
    int name_length = strlen(info->name);
    int text_name_x = 128 - (name_length * 4); // * 4 acts as (width * 8 / 2)
    int text_redeemed_x = 128 - (9 * 4); // "Redeemed!" is exactly 9 characters long

    tte_printf("#{P:%d,%d; cx:0x%X000}%s", text_name_x, TEXT_NAME_Y, TTE_WHITE_PB, info->name);
    tte_printf("#{P:%d,%d; cx:0x%X000}Redeemed!", text_redeemed_x, TEXT_REDEEMED_Y, TTE_WHITE_PB);

    int frame = 0;
    while(true) {
        VBlankIntrWait();
        mmFrame();  
        affine_background_update(); 
        key_poll(); 

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

        if (key_hit(KEY_A | KEY_B)) break;
    }

    // --- PHASE 3: SLIDE UP ---
    tte_erase_rect(72, 56, 200, 160); 

    // FIX: Instantly teleport the voucher off-screen so it disappears before the slide!
    if (v_obj) {
        v_obj->sprite_object->x = int2fx(SPRITE_CENTER_X);
        v_obj->sprite_object->y = int2fx(160); // Drops below the 160px screen boundary
        v_obj->sprite_object->tx = int2fx(SPRITE_CENTER_X);
        v_obj->sprite_object->ty = int2fx(160);
        sprite_object_update(v_obj->sprite_object);
    }

    for (int i = 0; i < 20; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 
        
        main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* j_obj;
        while ((j_obj = list_itr_next(&itr))) {
            j_obj->sprite_object->ty = int2fx(71); 
            joker_object_update(j_obj);
        }

        // We completely ignore the voucher object here so it doesn't return!
        
        sprite_draw();
    }
    
    // --- 4. THE RESTORE ---
    memcpy32(&se_mem[MAIN_BG_SBB][0], shop_bg_snapshot, 512);
    game_force_shop_background_redraw();
}