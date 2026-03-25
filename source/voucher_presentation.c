#include "voucher.h"
#include <tonc.h>
#include <stddef.h>
#include "graphic_utils.h"

#define COLOR_WHITE RGB8(255, 255, 255)

// Pull in your existing voucher getter so we can grab the sprite!
extern VoucherObject* get_current_shop_voucher_object(void);

void present_voucher_redeemed_screen(const VoucherInfo* info) 
{
    if (info == NULL) return; 

    // 1. Grab the actual live voucher object so we can shake its sprite!
    VoucherObject* v_obj = get_current_shop_voucher_object();

    // 2. Erase the text layer
    tte_erase_rect(0, 0, 240, 160); 

    // 3. Draw the Text
    tte_printf("#{P:120,40; cx:0x%X000}%s", TTE_WHITE_PB, info->name);
    tte_printf("#{P:120,120; cx:0x%X000}Redeemed!", TTE_WHITE_PB);

    // 4. THE MODAL LOOP WITH SHAKE ANIMATION
    u16 prev_keys = ~REG_KEYINPUT & KEY_MASK;
    int frame_timer = 0; // Tracks how long the shake lasts
    
    while(true) 
    {
        VBlankIntrWait(); // Wait for screen refresh

        // --- THE JUICE (Sprite Shake Logic) ---
        if (v_obj != NULL && v_obj->sprite_object != NULL) {
            
            if (frame_timer < 12) { 
                // Shake aggressively for ~1/5th of a second
                // Alternating pixel offsets for a violent "thump"
                int shake_x = (frame_timer % 2 == 0) ? 2 : -2;
                int shake_y = (frame_timer % 3 == 0) ? 2 : -2;
                
                // As the timer gets closer to 12, cut the shake intensity in half to "settle" it
                if (frame_timer > 6) { shake_x /= 2; shake_y /= 2; }
                
                // Apply offset to the base coordinates you set in voucher.c (X:88, Y:112)
                v_obj->sprite_object->x = int2fx(88 + shake_x);
                v_obj->sprite_object->y = int2fx(112 + shake_y);
                
            } else if (frame_timer == 12) {
                // Snap back to perfect center exactly when the shake ends!
                v_obj->sprite_object->x = int2fx(88);
                v_obj->sprite_object->y = int2fx(112);
            }
            frame_timer++;
        }

        // NOTE: Because we bypassed your game's main update loop, if your engine 
        // requires a manual function call to push sprite updates to the GBA's OAM 
        // (like `obj_sync()` or `draw_sprites()`), you will need to paste that right here!

        // --- INPUT HANDLING ---
        u16 keys_now = ~REG_KEYINPUT & KEY_MASK;
        u16 keys_hit = keys_now & ~prev_keys;

        if ((keys_hit & KEY_A) || (keys_hit & KEY_B) || (keys_hit & KEY_START)) {
            break; 
        }

        prev_keys = keys_now;
    }

    // 5. Cleanup
    tte_erase_rect(0, 0, 240, 160);
}