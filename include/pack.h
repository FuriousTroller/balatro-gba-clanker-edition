#ifndef PACK_H
#define PACK_H

#include <tonc.h>
#include "sprite.h"
#include "list.h"

extern List _shop_packs_list;

// 1. The universal pack types
typedef enum {
    PACK_TYPE_BUFFOON,
    PACK_TYPE_ARCANA,
    PACK_TYPE_CELESTIAL,
    PACK_TYPE_SPECTRAL,
    PACK_TYPE_STANDARD
} PackType;

// 2. The blueprint for every pack in the game
typedef struct {
    u8 id;
    const char* name;
    PackType type;
    int cost;
    int cards_to_spawn; 
    int picks_allowed;  
} PackInfo;

// 3. The physical object when a pack is sitting in the shop
typedef struct {
    const PackInfo* info;
    SpriteObject* sprite_object;
} PackObject;

// Core Engine Hooks
void pack_init(void);
const PackInfo* get_pack_registry_entry(u8 id);
int get_pack_registry_size(void);

// Presentation & Card Spawning Hooks
void present_pack_opened_screen(const PackInfo* info);
void spawn_pack_cards(const PackInfo* info);
void despawn_pack_cards(void);

void pack_load_gfx(void);
void pack_spawn_in_shop(int x, int y, u8 pack_id, int sprite_index);
void pack_despawn_shop_packs(void);

#endif // PACK_H