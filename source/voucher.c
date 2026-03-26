#include "voucher.h"

#include "pool.h"
#include "util.h"
#include "voucher_gfx0.h"
#include "voucher_gfx1.h"
#include "voucher_gfx2.h"
#include "voucher_gfx3.h"
#include "voucher_gfx4.h"
#include "voucher_gfx5.h"

#include <stdlib.h>
#include <string.h>

const unsigned int* const VOUCHER_TILES[] = {
    voucher_gfx0Tiles, voucher_gfx1Tiles, voucher_gfx2Tiles, 
    voucher_gfx3Tiles, voucher_gfx4Tiles, voucher_gfx5Tiles
};

const unsigned short* const VOUCHER_PALS[] = {
    voucher_gfx0Pal, voucher_gfx1Pal, voucher_gfx2Pal, 
    voucher_gfx3Pal, voucher_gfx4Pal, voucher_gfx5Pal
};

extern const VoucherInfo voucher_registry[];
#define VOUCHER_REGISTRY_SIZE 12

static bool _owned_vouchers[256] = {false};
static VoucherObject* _current_shop_voucher = NULL;

void voucher_init(void)
{
    for (int i = 0; i < 256; i++)
    {
        _owned_vouchers[i] = false;
    }
    if (_current_shop_voucher != NULL)
    {
        if (_current_shop_voucher->sprite_object != NULL)
        {
            sprite_object_destroy(&_current_shop_voucher->sprite_object);
        }
        POOL_FREE(VoucherObject, _current_shop_voucher);
        _current_shop_voucher = NULL;
    }
}

const VoucherInfo* get_voucher_registry_entry(u8 id)
{
    for (int i = 0; i < VOUCHER_REGISTRY_SIZE; i++)
    {
        if (voucher_registry[i].id == id)
            return &voucher_registry[i];
    }
    return NULL;
}

bool is_voucher_owned(u8 id)
{
    return _owned_vouchers[id];
}

void buy_current_shop_voucher(void)
{
    if (_current_shop_voucher == NULL)
        return;

    _owned_vouchers[_current_shop_voucher->info->id] = true;

    if (_current_shop_voucher->info->on_buy_func != NULL)
    {
        _current_shop_voucher->info->on_buy_func();
    }

    // This starts the menu-slide animation and blocks the loop 
    // until the player acknowledges the purchase.
    present_voucher_redeemed_screen(_current_shop_voucher->info);

    if (_current_shop_voucher->sprite_object != NULL)
    {
        sprite_object_destroy(&_current_shop_voucher->sprite_object);
    }
    POOL_FREE(VoucherObject, _current_shop_voucher);
    _current_shop_voucher = NULL;
}

void roll_new_shop_voucher(void)
{
    if (_current_shop_voucher != NULL)
    {
        if (_current_shop_voucher->sprite_object != NULL)
        {
            sprite_object_destroy(&_current_shop_voucher->sprite_object);
        }
        POOL_FREE(VoucherObject, _current_shop_voucher);
        _current_shop_voucher = NULL;
    }

    u8 eligible[VOUCHER_REGISTRY_SIZE];
    int num_eligible = 0;

    for (int i = 0; i < VOUCHER_REGISTRY_SIZE; i++)
    {
        u8 id = voucher_registry[i].id;

        // 1. Skip if the player already owns it
        if (_owned_vouchers[id])
            continue;

        // 2. UNIVERSAL UPGRADE FILTER
        // If the ID is an odd number (1, 3, 5, 7), it is an Upgrade.
        // It is ONLY eligible to spawn if the Base voucher (ID - 1) IS owned!
        if (id % 2 != 0 && !_owned_vouchers[id - 1])
            continue;

        // 3. Add to the RNG pool!
        eligible[num_eligible++] = id;
    }

    if (num_eligible == 0)
        return;

    // Roll the RNG!
    u8 chosen_id = eligible[random() % num_eligible];

    _current_shop_voucher = POOL_GET(VoucherObject);
    _current_shop_voucher->info = get_voucher_registry_entry(chosen_id);
    _current_shop_voucher->sprite_object = NULL; // Do not spawn the sprite yet!
}

void spawn_shop_voucher_sprite(void)
{
    if (_current_shop_voucher == NULL)
        return;
    if (_current_shop_voucher->sprite_object != NULL)
        return; // Already spawned

    _current_shop_voucher->sprite_object = sprite_object_new();

#define VOUCHER_TID 512 // Safely out of the way of card tiles!
#define VOUCHER_PB  14  // Keep this at 14 to protect the playing card colors!

    // ---> UNIVERSAL GRAPHICS ROUTING LOGIC <---
    u8 v_id = _current_shop_voucher->info->id;

    // The Math: Automatically finds the right sheet and frame based on ID
    int sheet = v_id / 2; 
    int frame = v_id % 2; 

    // Load Palette & Tiles automatically!
    memcpy16(&pal_obj_mem[PAL_ROW_LEN * VOUCHER_PB], VOUCHER_PALS[sheet], 16);
    memcpy32(
        &tile_mem[4][VOUCHER_TID], 
        &VOUCHER_TILES[sheet][frame * 128], 
        128
    );
    // ------------------------------------------

    sprite_object_set_sprite(
        _current_shop_voucher->sprite_object,
        sprite_new(ATTR0_SQUARE | ATTR0_4BPP | ATTR0_AFF, ATTR1_SIZE_32, VOUCHER_TID, VOUCHER_PB, 1)
    );

    sprite_object_reset_transform(_current_shop_voucher->sprite_object);

    // X coordinates stay locked in the box
    _current_shop_voucher->sprite_object->x = int2fx(88);
    _current_shop_voucher->sprite_object->tx = int2fx(88);

    // Start Y off the bottom of the screen, but set Target Y to the box!
    _current_shop_voucher->sprite_object->y = int2fx(160);
    _current_shop_voucher->sprite_object->ty = int2fx(112);
}

void despawn_shop_voucher_sprite(void)
{
    if (_current_shop_voucher != NULL && _current_shop_voucher->sprite_object != NULL)
    {
        sprite_object_destroy(&_current_shop_voucher->sprite_object);
        _current_shop_voucher->sprite_object = NULL;
    }
}

VoucherObject* get_current_shop_voucher_object(void)
{
    return _current_shop_voucher;
}