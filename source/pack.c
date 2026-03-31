#include "pack.h"
#include "game.h"
#include "joker.h"
#include "list.h"
#include "audio_utils.h"
#include "soundbank.h"
#include "pool.h"

#include <stddef.h>
#include <stdlib.h>

// --- VRAM HARDWARE MAP ---
#define PACK_BASE_TILE 768 
#define PACK_TILE_SIZE 16  // Exactly 16 tiles needed for one 32x32 sprite frame!
#define PACK_BASE_PAL  12  

// --- THE MASTER PACK REGISTRY ---
static const PackInfo pack_registry[] = {
    {
        .id = 0,
        .name = "Buffoon Pack",
        .type = PACK_TYPE_BUFFOON,
        .cost = 4,
        .cards_to_spawn = 2,
        .picks_allowed = 1,
        .weight = 120 // Normal Buffoon Weight
    },
    {
        .id = 1,
        .name = "Jumbo Buffoon",
        .type = PACK_TYPE_BUFFOON,
        .cost = 6,
        .cards_to_spawn = 4, 
        .picks_allowed = 1,
        .weight = 60  // Jumbo Buffoon Weight
    }
};

extern const u32 buffoon_packs_gfx0Tiles[256];
extern const u16 buffoon_packs_gfx0Pal[16];
extern const u32 buffoon_packs_gfx1Tiles[256];
extern const u16 buffoon_packs_gfx1Pal[16];

int game_shop_get_rand_available_joker_id(void);

List _shop_packs_list; 
List _pack_cards_list;

void pack_init(void) {
    _pack_cards_list = list_create();
    _shop_packs_list = list_create(); 
}

// --- VRAM GRAPHICS ARRAYS ---
const u32* const PACK_TILES[] = {
    buffoon_packs_gfx0Tiles, buffoon_packs_gfx1Tiles
};

const u16* const PACK_PALS[] = {
    buffoon_packs_gfx0Pal, buffoon_packs_gfx1Pal
};

void pack_load_gfx(void) {
    // Intentionally left blank! We will dynamically load the graphics 
    // exactly when a pack spawns, just like voucher.c does!
}

// --- SHOP SPAWNING LOGIC ---
void pack_spawn_in_shop(int x, int y) {
    int total_weight = 0;
    int registry_size = get_pack_registry_size();
    for (int i = 0; i < registry_size; i++) {
        total_weight += pack_registry[i].weight;
    }

    int roll = qran_range(0, total_weight);
    int cumulative = 0;
    u8 pack_id = 0; 

    for (int i = 0; i < registry_size; i++) {
        cumulative += pack_registry[i].weight;
        if (roll < cumulative) {
            pack_id = pack_registry[i].id;
            break;
        }
    }
    
    const PackInfo* info = get_pack_registry_entry(pack_id);

    PackObject* shop_pack = POOL_GET(PackObject);
    shop_pack->info = info;
    shop_pack->sprite_object = sprite_object_new();
    
    int pack_tid = PACK_BASE_TILE;
    int pack_pb  = PACK_BASE_PAL;

    switch (info->type) {
        case PACK_TYPE_BUFFOON:
            pack_tid += (pack_id == 0) ? (0 * PACK_TILE_SIZE) : (1 * PACK_TILE_SIZE); 
            pack_pb  += (pack_id == 0) ? 0 : 1;              
            break;
        default:
            break;
    }

    // ---> THE FIX: Safely copy exact words using the proven voucher.c method! <---
    int gfx_idx = (pack_id < 2) ? pack_id : 0; 
    
    memcpy16(&pal_obj_mem[16 * pack_pb], PACK_PALS[gfx_idx], 16); // 16 halfwords (32 bytes)
    memcpy32(&tile_mem[4][pack_tid], PACK_TILES[gfx_idx], 128);   // 128 words (512 bytes = 1 frame)
    // -----------------------------------------------------------------------------

    sprite_object_set_sprite(
        shop_pack->sprite_object,
        sprite_new(ATTR0_SQUARE | ATTR0_4BPP, ATTR1_SIZE_32, pack_tid, pack_pb, 1)
    );
    
    // X coordinates stay locked in the box
    shop_pack->sprite_object->x = int2fx(x); 
    shop_pack->sprite_object->tx = int2fx(x);
    
    // Start Y off-screen, target Y is the shelf
    shop_pack->sprite_object->y = int2fx(160); 
    shop_pack->sprite_object->ty = int2fx(y);
    
    list_push_back(&_shop_packs_list, shop_pack);
}

const PackInfo* get_pack_registry_entry(u8 id) {
    int registry_size = sizeof(pack_registry) / sizeof(pack_registry[0]);
    for (int i = 0; i < registry_size; i++) {
        if (pack_registry[i].id == id) {
            return &pack_registry[i];
        }
    }
    return NULL;
}

int get_pack_registry_size(void) {
    return sizeof(pack_registry) / sizeof(pack_registry[0]);
}

// --- CARD SPAWNING MATH ---
void spawn_pack_cards(const PackInfo* info) {
    list_clear(&_pack_cards_list);
    _pack_cards_list = list_create();

    if (info == NULL) return;

    int count = info->cards_to_spawn;
    int center_x = 156; 
    
    int spacing = (count >= 5) ? 32 : 40; 
    int total_width = (count - 1) * spacing;
    int start_x = center_x - (total_width / 2);

    for (int i = 0; i < count; i++) {
        // TEMPORARY: Spawns Jokers to test the UI layout
        int joker_id = game_shop_get_rand_available_joker_id();
        if (joker_id == -1) break;

        JokerObject* card = joker_object_new(joker_new(joker_id));

        card->sprite_object->x = int2fx(start_x + (i * spacing));
        card->sprite_object->y = int2fx(180); 
        
        card->sprite_object->tx = card->sprite_object->x;
        card->sprite_object->ty = int2fx(70); 

        sprite_position(card->sprite_object->sprite, fx2int(card->sprite_object->x), fx2int(card->sprite_object->y));

        list_push_back(&_pack_cards_list, card);
    }
}

void despawn_pack_cards(void) {
    ListItr itr = list_itr_create(&_pack_cards_list);
    JokerObject* card;
    while ((card = list_itr_next(&itr))) {
        joker_object_destroy(&card);
    }
    list_clear(&_pack_cards_list);
}

// --- SELECTION GRID LOGIC ---
int shop_packs_row_get_size(void) {
    return list_get_len(&_shop_packs_list);
}

bool shop_packs_row_on_selection_changed(SelectionGrid* selection_grid, int row_idx, const Selection* prev_selection, const Selection* new_selection) {
    if (prev_selection->y == row_idx && prev_selection->x >= 0 && prev_selection->x < shop_packs_row_get_size()) {
        PackObject* old_pack = (PackObject*)list_get_at_idx(&_shop_packs_list, prev_selection->x);
        if (old_pack != NULL) {
            sprite_object_set_focus(old_pack->sprite_object, false);
        }
    }
    if (new_selection->y == row_idx && new_selection->x >= 0 && new_selection->x < shop_packs_row_get_size()) {
        PackObject* new_pack = (PackObject*)list_get_at_idx(&_shop_packs_list, new_selection->x);
        if (new_pack != NULL) {
            sprite_object_set_focus(new_pack->sprite_object, true);
        }
    }
    return true;
}

void shop_packs_row_on_key_transit(SelectionGrid* selection_grid, Selection* selection) {
    if (!key_hit(SELECT_CARD)) return;

    int pack_idx = selection->x;
    PackObject* selected_pack = (PackObject*)list_get_at_idx(&_shop_packs_list, pack_idx);
    if (selected_pack == NULL) return;

    // Check if player has enough money
    if (get_money() >= selected_pack->info->cost) {
        set_money(get_money() - selected_pack->info->cost);
        display_money();
        play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, 200);

        // TRIGGER THE PRESENTATION SCREEN!
        present_pack_opened_screen(selected_pack->info);

        // Remove pack from shelf
        list_remove_at_idx(&_shop_packs_list, pack_idx);
        
        // THE FIX: Use the native engine pool destructors instead of free()!
        sprite_object_destroy(&selected_pack->sprite_object);
        POOL_FREE(PackObject, selected_pack);

        // Move cursor to prevent crashing
        selection_grid_move_selection_horz(selection_grid, -1);
    }
}