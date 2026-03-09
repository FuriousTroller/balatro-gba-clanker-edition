#include "voucher.h"

// Extern this so we don't have to #include "game.h" and cause circular dependencies
extern int current_shop_joker_slots;

void effect_overstock(void)
{
    if (current_shop_joker_slots < 3)
    {
        current_shop_joker_slots = 3;
    }
}

void effect_overstock_plus(void)
{
    if (current_shop_joker_slots < 4)
    {
        current_shop_joker_slots = 4;
    }
}

// { ID, Upgraded_ID, Cost, Effect_Function }
const VoucherInfo voucher_registry[] = {
    {0, 1,   10, effect_overstock     },
    {1, 255, 10, effect_overstock_plus},
};