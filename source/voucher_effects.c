#include "voucher.h"

// --------------------------------------------------------
// IMPORT YOUR GAME'S STAT VARIABLES HERE!
// Note: Double check your game.c to see exactly what you
// named these variables (e.g., _max_hands, max_jokers, etc.)
// --------------------------------------------------------
extern int current_shop_joker_slots;
extern int max_hands;    // Your base hands variable
extern int max_discards; // Your base discards variable
extern int max_jokers;   // Your max joker slots variable

// Import the native UI update functions from game.c!
int get_num_hands_remaining(void);
void set_num_hands_remaining(int n);
int get_num_discards_remaining(void);
void set_num_discards_remaining(int n);

// --- OVERSTOCK ---
void effect_overstock(void)
{
    if (current_shop_joker_slots < 3)
        current_shop_joker_slots = 3;
}
void effect_overstock_plus(void)
{
    if (current_shop_joker_slots < 4)
        current_shop_joker_slots = 4;
}

// --- PASSIVES (Handled in game.c, so these stay empty!) ---
void effect_clearance_sale(void)
{
}
void effect_liquidation(void)
{
}
void effect_reroll_surplus(void)
{
}
void effect_reroll_glut(void)
{
}

// --- ONE-TIME STAT BOOSTS ---
void effect_grabber(void) { 
    max_hands++; 
    set_num_hands_remaining(get_num_hands_remaining() + 1); 
}
void effect_nacho_tong(void) { 
    max_hands++; 
    set_num_hands_remaining(get_num_hands_remaining() + 1); 
}
void effect_wasteful(void) { 
    max_discards++; 
    set_num_discards_remaining(get_num_discards_remaining() + 1); 
}
void effect_recyclomancy(void) { 
    max_discards++; 
    set_num_discards_remaining(get_num_discards_remaining() + 1); 
}

// --- BLANK / DARK MATTER ---
void effect_blank(void)
{
} // Literally does nothing! (Unlocks ID 11)
void effect_antimatter(void)
{
    max_jokers++;
}

// --------------------------------------------------------
// REGISTRY
// --------------------------------------------------------
const VoucherInfo voucher_registry[] = {
    {0,  1,   10, effect_overstock     },
    {1,  255, 10, effect_overstock_plus},
    {2,  3,   10, effect_clearance_sale},
    {3,  255, 10, effect_liquidation   },
    {4,  5,   10, effect_reroll_surplus},
    {5,  255, 10, effect_reroll_glut   },
    {6,  7,   10, effect_grabber       },
    {7,  255, 10, effect_nacho_tong    },
    {8,  9,   10, effect_wasteful      },
    {9,  255, 10, effect_recyclomancy  },
    {10, 11,  10, effect_blank         },
    {11, 255, 10, effect_antimatter    },
};