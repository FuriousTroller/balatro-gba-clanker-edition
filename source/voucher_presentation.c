#include "voucher.h"
#include "game.h"
#include "list.h"
#include "joker.h"
#include "graphic_utils.h"
#include <tonc.h>
#include <maxmod.h>
#include <string.h>

extern void sprite_draw(void);
extern void affine_background_update(void); 
extern List _shop_jokers_list;
extern const Rect POP_MENU_ANIM_RECT;
void game_force_shop_background_redraw(void);

// Link directly to the pristine Shop Map in the ROM!
extern const u16 background_shop_gfxMap[1024];

const int SPRITE_CENTER_X = 140; 
const int TEXT_CENTER_X = 156;
const int CENTER_VOUCHER_Y = 80;
const int TEXT_NAME_Y = 65; 
const int TEXT_REDEEMED_Y = 115; 

void present_voucher_redeemed_screen(const VoucherInfo* info) 
{
    if (info == NULL) return; 
    VoucherObject* v_obj = get_current_shop_voucher_object();

    tte_erase_rect(72, 56, 240, 160); 

    // --- PHASE 1: SLIDE DOWN (The game_shop_outro method) ---
    // The game natively slides the menu down for 20 frames
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

    // --- PHASE 2: THE REVEAL & SHAKE ---
    int name_length = strlen(info->name);
    int text_name_x = TEXT_CENTER_X - (name_length * 4); 
    int text_redeemed_x = TEXT_CENTER_X - (9 * 4); 

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

    // --- PHASE 3: SLIDE UP (The game_shop_intro method) ---
    tte_erase_rect(72, 56, 240, 160); 

    // Make the voucher instantly vanish into the floor
    if (v_obj) {
        v_obj->sprite_object->x = int2fx(SPRITE_CENTER_X);
        v_obj->sprite_object->y = int2fx(160);
        v_obj->sprite_object->tx = int2fx(SPRITE_CENTER_X);
        v_obj->sprite_object->ty = int2fx(160);
        sprite_object_update(v_obj->sprite_object);
    }

    // THE MAGIC RESTORE: Safely copy the pure ROM map into the sliding area!
    // This perfectly sets up the "lowered" menu state before sliding up.
    // memcpy16 is a safe hardware copy that completely ignores compiler quirks!
    int width = POP_MENU_ANIM_RECT.right - POP_MENU_ANIM_RECT.left + 1;
    for (int y = POP_MENU_ANIM_RECT.top; y <= POP_MENU_ANIM_RECT.bottom; y++) {
        memcpy16(&se_mem[MAIN_BG_SBB][y * 32 + POP_MENU_ANIM_RECT.left],
                 &background_shop_gfxMap[y * 32 + POP_MENU_ANIM_RECT.left],
                 width);
    }

    // The game natively slides the menu UP for exactly 12 frames!
    for (int i = 0; i < 12; i++) {
        VBlankIntrWait();
        mmFrame(); 
        affine_background_update(); 
        
        main_bg_se_copy_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

        int offset = 11 - i; // 11 down to 0 perfectly aligns with the 12 frames!

        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* j_obj;
        while ((j_obj = list_itr_next(&itr))) {
            int current_y = 71 + (offset * 8);
            
            // Safe bounds check to keep Jokers off the ceiling
            if (current_y > 160) current_y = 160;

            j_obj->sprite_object->y = int2fx(current_y);
            j_obj->sprite_object->ty = int2fx(current_y);
            joker_object_update(j_obj);
        }
        
        sprite_draw();
    }
    
    // --- 4. TEXT REFRESH ---
    game_force_shop_background_redraw();
}