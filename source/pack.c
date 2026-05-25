#include "pack.h"
#include "game.h"
#include "joker.h"
#include "list.h"
#include "util.h"
#include <stddef.h>
#include <stdlib.h>

// --- THE MASTER PACK REGISTRY ---
static const PackInfo pack_registry[] = {
    {
        .id = 0,
        .name = "Buffoon Pack",
        .type = PACK_TYPE_BUFFOON,
        .cost = 4,
        .cards_to_spawn = 2,
        .picks_allowed = 1
    },
    {
        .id = 1,
        .name = "Jumbo Buffoon",
        .type = PACK_TYPE_BUFFOON,
        .cost = 6,
        .cards_to_spawn = 4, 
        .picks_allowed = 1
    }
};

extern const u32 buffoon_packs_gfx0Tiles[256];
extern const u16 buffoon_packs_gfx0Pal[16];
extern const u32 buffoon_packs_gfx1Tiles[256];
extern const u16 buffoon_packs_gfx1Pal[16];

// The lists holding the physical packs sitting on the shelf and spawned cards inside the opened pack
List _shop_packs_list; 
List _pack_cards_list;

void pack_init(void) {
    _pack_cards_list = list_create();
    _shop_packs_list = list_create(); // Initialize the shelf list!
}

// --- VRAM LOADING ---
void pack_load_gfx(void) {
    // Load Palettes into Sprite Banks 1 and 2 (16 colors = 16 halfwords)
    memcpy16(&pal_obj_mem[PAL_ROW_LEN * 1], buffoon_packs_gfx0Pal, 16);
    memcpy16(&pal_obj_mem[PAL_ROW_LEN * 2], buffoon_packs_gfx1Pal, 16);

    // Load Tiles into Sprite VRAM Block 4 (Tiles 256 and 272)
    // Each sprite tile sheet is 256 u32s.
    memcpy32(&tile_mem[4][256], buffoon_packs_gfx0Tiles, 256);
    memcpy32(&tile_mem[4][272], buffoon_packs_gfx1Tiles, 256);
}

// --- SHOP SPAWNING LOGIC ---
void pack_spawn_in_shop(int x, int y, u8 pack_id, int sprite_index) {
    const PackInfo* info = get_pack_registry_entry(pack_id);
    if (info == NULL) return;

    PackObject* shop_pack = malloc(sizeof(PackObject));
    shop_pack->info = info;
    shop_pack->sprite_object = sprite_object_new();
    
    u32 tid = (pack_id == 0) ? 256 : 272;
    u32 pb = (pack_id == 0) ? 1 : 2;
    
    sprite_object_set_sprite(
        shop_pack->sprite_object,
        sprite_new(ATTR0_SQUARE | ATTR0_4BPP | ATTR0_AFF, ATTR1_SIZE_32, tid, pb, sprite_index)
    );
    
    sprite_object_reset_transform(shop_pack->sprite_object);
    
    shop_pack->sprite_object->x = int2fx(x); 
    shop_pack->sprite_object->tx = int2fx(x); 
    shop_pack->sprite_object->y = int2fx(160); // Start offscreen bottom
    shop_pack->sprite_object->ty = int2fx(y);   // Target shelf height

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

    // Play area: x=80 (after HUD) to x=208 (before buttons) = 128px wide
    // Center of play area = 80 + 128/2 = 144
    // Card sprites are 32px wide, so card visual center = position + 16
    int area_center = 144;
    int card_width = 32;

    // Spacing between card centers
    int spacing = 40;
    if (count == 2) spacing = 50;
    else if (count == 3) spacing = 44;
    else if (count == 4) spacing = 32;
    else if (count >= 5) spacing = 24;

    for (int i = 0; i < count; i++) {
        // TEMPORARY: Spawns Jokers to test the UI layout
        int joker_id = game_shop_get_rand_available_joker_id();
        if (joker_id == UNDEFINED) break;

        JokerObject* card = joker_object_new(joker_new(joker_id));

        // Symmetrical centering:
        // For odd count:  middle card (i = count/2) has center at area_center
        // For even count: cards straddle the center evenly
        int card_center_x;
        if (count % 2 == 1) {
            // Odd: middle card index = count/2
            int mid = count / 2;
            card_center_x = area_center + (i - mid) * spacing;
        } else {
            // Even: no card at center, straddle it
            // Card 0 gets offset -(count/2 - 1) - 0.5, etc.
            // i.e. offset = (i - count/2) * spacing + spacing/2
            card_center_x = area_center + (i - count / 2) * spacing + spacing / 2;
        }

        // Convert center to top-left position
        int pos_x = card_center_x - card_width / 2;

        card->sprite_object->x = int2fx(pos_x);
        card->sprite_object->y = int2fx(180); // Hide below the floor
        
        card->sprite_object->tx = card->sprite_object->x;
        card->sprite_object->ty = int2fx(70); // Target floating height

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

void pack_despawn_shop_packs(void) {
    ListItr itr = list_itr_create(&_shop_packs_list);
    PackObject* pack;
    while ((pack = list_itr_next(&itr))) {
        if (pack != NULL) {
            if (pack->sprite_object != NULL) {
                sprite_object_destroy(&pack->sprite_object);
            }
            free(pack);
        }
    }
    list_clear(&_shop_packs_list);
}