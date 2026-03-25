#include "voucher.h"
#include "game.h"
#include "list.h"
#include "joker.h"
#include "graphic_utils.h"
#include <tonc.h>

// Link to the shop items from game.c
extern List _shop_jokers_list;
extern const Rect POP_MENU_ANIM_RECT;

void present_voucher_redeemed_screen(const VoucherInfo* info) 
{
    if (info == NULL) return; 
    VoucherObject* v_obj = get_current_shop_voucher_object();

    // --- PHASE 1: SLIDE DOWN ---
    // We replicate your 'game_shop_outro' logic but we DON'T destroy the cards!
    for (int i = 0; i < 20; i++) {
        VBlankIntrWait();
        
        // 1. Slide the Background Tiles down
        main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_DOWN);

        // 2. Tell the Jokers to slide off-screen
        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* j_obj;
        while ((j_obj = list_itr_next(&itr))) {
            j_obj->sprite_object->ty = int2fx(160); // Target: Floor
            joker_object_update(j_obj);
        }

        // 3. Move the Voucher to the center of the screen
        if (v_obj) {
            v_obj->sprite_object->ty = int2fx(80); // Target: Center
            sprite_object_update(v_obj->sprite_object);
        }
    }

    // --- PHASE 2: THE REVEAL ---
    tte_erase_rect(0, 0, 240, 160); 
    tte_printf("#{P:120,40; cx:0x%X000}%s", TTE_WHITE_PB, info->name);
    tte_printf("#{P:120,120; cx:0x%X000}Redeemed!", TTE_WHITE_PB);

    // Shake and Wait
    u16 prev_keys = ~REG_KEYINPUT & KEY_MASK;
    int frame = 0;
    while(true) {
        VBlankIntrWait();
        
        // Keep the "Juice" going
        if (frame < 12 && v_obj) {
            int s = (frame % 2 == 0) ? 2 : -2;
            v_obj->sprite_object->x = int2fx(120 + s); // Shake at center
            sprite_object_update(v_obj->sprite_object);
        }
        frame++;

        u16 keys_hit = (~REG_KEYINPUT & KEY_MASK) & ~prev_keys;
        if (keys_hit & (KEY_A | KEY_B)) break;
        prev_keys = ~REG_KEYINPUT & KEY_MASK;
    }

    // --- PHASE 3: SLIDE UP ---
    tte_erase_rect(0, 0, 240, 160);
    for (int i = 0; i < 20; i++) {
        VBlankIntrWait();
        
        // 1. Pull the Tiles back up
        main_bg_se_move_rect_1_tile_vert(POP_MENU_ANIM_RECT, SCREEN_UP);

        // 2. Pull the Jokers back to their shelf (ITEM_SHOP_Y = 71)
        ListItr itr = list_itr_create(&_shop_jokers_list);
        JokerObject* j_obj;
        while ((j_obj = list_itr_next(&itr))) {
            j_obj->sprite_object->ty = int2fx(71); 
            joker_object_update(j_obj);
        }

        // 3. Slide the Voucher back to its buy-box (Y:112)
        if (v_obj) {
            v_obj->sprite_object->ty = int2fx(112);
            sprite_object_update(v_obj->sprite_object);
        }
    }
}