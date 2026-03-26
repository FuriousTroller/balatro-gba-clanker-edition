#include "pack.h"
#include "game.h"
#include "joker.h"
#include "list.h"
#include <stddef.h>

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

// The list holding the physical packs sitting on the shelf
List _shop_packs_list; 

void pack_init(void) {
    _pack_cards_list = list_create();
    _shop_packs_list = list_create(); // Initialize the shelf list!
}

// --- VRAM LOADING ---
void pack_load_gfx(void) {
    // Load Palettes into Sprite Banks 4 and 5
    dma3_cp(&pal_obj_mem[4 * 16], buffoon_packs_gfx0Pal, sizeof(buffoon_packs_gfx0Pal));
    dma3_cp(&pal_obj_mem[5 * 16], buffoon_packs_gfx1Pal, sizeof(buffoon_packs_gfx1Pal));

    // Load Tiles into Sprite VRAM Block 4 (Tiles 256 and 272)
    dma3_cp(&tile_mem[4][256], buffoon_packs_gfx0Tiles, sizeof(buffoon_packs_gfx0Tiles));
    dma3_cp(&tile_mem[4][272], buffoon_packs_gfx1Tiles, sizeof(buffoon_packs_gfx1Tiles));
}

// --- SHOP SPAWNING LOGIC ---
void pack_spawn_in_shop(int x, int y) {
    // Roll a 50/50 chance for Normal (ID 0) or Jumbo (ID 1)
    u8 pack_id = (qran_range(0, 100) > 50) ? 0 : 1;
    const PackInfo* info = get_pack_registry_entry(pack_id);

    PackObject* shop_pack = malloc(sizeof(PackObject));
    shop_pack->info = info;
    shop_pack->sprite_object = sprite_object_new();
    
    shop_pack->sprite_object->x = int2fx(x); 
    shop_pack->sprite_object->y = int2fx(y); 

    shop_pack->sprite_object->sprite->attr0 = ATTR0_SQUARE | ATTR0_Y(y);
    shop_pack->sprite_object->sprite->attr1 = ATTR1_SIZE_32 | ATTR1_X(x);
    
    // Assign the correct Palette and Tile based on the ID!
    if (pack_id == 0) {
        shop_pack->sprite_object->sprite->attr2 = ATTR2_PALBANK(4) | 256; 
    } else {
        shop_pack->sprite_object->sprite->attr2 = ATTR2_PALBANK(5) | 272; 
    }

    list_push_back(&_shop_packs_list, shop_pack);
    sprite_draw_single(shop_pack->sprite_object->sprite); 
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

// --- TEMPORARY PACK CARD LIST ---
List _pack_cards_list;

void pack_init(void) {
    _pack_cards_list = list_create();
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
        if (joker_id == UNDEFINED) break;

        JokerObject* card = joker_object_new(joker_new(joker_id));

        card->sprite_object->x = int2fx(start_x + (i * spacing));
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