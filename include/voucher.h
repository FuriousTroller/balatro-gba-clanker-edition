#ifndef VOUCHER_H
#define VOUCHER_H

#include "sprite.h"

#include <stdbool.h>
#include <tonc.h>

typedef struct
{
    u8 id;
    const char* name;
    u8 upgraded_id;
    int cost;
    void (*on_buy_func)(void);
} VoucherInfo;

typedef struct
{
    const VoucherInfo* info;
    SpriteObject* sprite_object;
} VoucherObject;

void voucher_init(void);
const VoucherInfo* get_voucher_registry_entry(u8 id);
bool is_voucher_owned(u8 id);

void roll_new_shop_voucher(void);
void buy_current_shop_voucher(void);
VoucherObject* get_current_shop_voucher_object(void);

// NEW LIFECYCLE FUNCTIONS
void spawn_shop_voucher_sprite(void);
void despawn_shop_voucher_sprite(void);

// Declare the presentation function so voucher.c can use it
void present_voucher_redeemed_screen(const VoucherInfo* info);

#endif // VOUCHER_H