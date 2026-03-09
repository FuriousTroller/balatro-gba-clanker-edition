#include "joker.h"
#include "game.h"
#include <stddef.h>

#include "custom_joker_sheet_0.h"
#include "custom_joker_sheet_1.h"
#include "custom_joker_sheet_2.h"
#include "custom_joker_sheet_3.h"
#include "custom_joker_sheet_4.h"

// #include "custom_joker_sheet_x.h" // Add this when you make IDs 1xx & 1xx!

// Creates a local shared memory struct specifically for your modded cards to use
static JokerEffect shared_joker_effect = {0};

// Tells the compiler to go find this variable inside game.c
extern int overkill_payout;
extern int get_current_ante(void);
extern bool is_c_j_fusion_active(void);
#define MODDED_JOKER_START_ID 100
#define NUM_JOKERS_PER_SPRITESHEET 2
static JokerEffect custom_joker_effect_out = {0};

// Tracks which of the 7 Sins you have acquired this run!
// Bits: 0=Greedy, 1=Lusty, 2=Wrathful, 3=Gluttonous, 4=Vainglorious, 5=Sloth, 6=Envy
u8 acquired_sins_mask = 0;

// --- 0. LOCAL EFFECT OBJECT ---
// We use this local object so we don't conflict with the vanilla file's locked memory
static JokerEffect modded_shared_joker_effect = {0};


// --- 1. YOUR CUSTOM JOKER LOGIC ---

static u32 recursion_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    // You can add your actual Mobius logic here whenever you are ready!
    return JOKER_EFFECT_FLAG_NONE; 
}

static u32 last_dance_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    if (joker_event == JOKER_EVENT_INDEPENDENT) {
        // Point to our safe local variable
        *joker_effect = &modded_shared_joker_effect;
        
        // x3 Total Mult
        (*joker_effect)->xmult = 3; 
        
        // x2 Total Chips (By adding 100% of our current chips to the pool!)
        (*joker_effect)->chips = get_chips(); 
        
        return JOKER_EFFECT_FLAG_XMULT | JOKER_EFFECT_FLAG_CHIPS;
    }
    
    return JOKER_EFFECT_FLAG_NONE; 
}

static u32 jaker_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    // Jaker modifies hands at the start of the round, so his scoring effect is empty!
    return JOKER_EFFECT_FLAG_NONE; 
}

static u32 voor_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    // Start with 2 Mult when conjured or bought
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 2; 
    }
    
    // Add the stored Mult to the score!
    if (joker_event == JOKER_EVENT_INDEPENDENT && joker->persistent_state > 0) {
        *joker_effect = &modded_shared_joker_effect;
        (*joker_effect)->mult = joker->persistent_state; 
        return JOKER_EFFECT_FLAG_MULT;
    }
    return JOKER_EFFECT_FLAG_NONE; 
}

static u32 capacocha_joker_effect(Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
) 
{
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 2; // Starts with exactly 2 uses
    }
    
    // Check for expiration at the end of the round
    if (joker_event == JOKER_EVENT_ON_ROUND_END) {
        if (joker->persistent_state <= 0) {
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->message = "Sacrificed!";
            (*joker_effect)->expire = true;
            return JOKER_EFFECT_FLAG_MESSAGE | JOKER_EFFECT_FLAG_EXPIRE;
        }
    }
    return JOKER_EFFECT_FLAG_NONE;
}

static u32 overkill_joker_effect(Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
) 
{
    if (joker_event == JOKER_EVENT_ON_ROUND_END) {
        if (overkill_payout > 0) {
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->money = overkill_payout; // The engine reads this directly
            return JOKER_EFFECT_FLAG_MONEY;           // Triggers the popup in joker.c
        }
    }
    return JOKER_EFFECT_FLAG_NONE;
}

// --- CLANKER MODE EXCLUSIVES ---

static u32 jamming_joker_effect(Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
) 
{
    // Passive: Logic handled externally during AI's turn
    return JOKER_EFFECT_FLAG_NONE;
}

static u32 captcha_joker_effect(Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
) 
{
    // Passive: Logic handled externally during AI's scoring loop
    return JOKER_EFFECT_FLAG_NONE;
}

static u32 ddos_joker_effect(Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
) 
{
    return JOKER_EFFECT_FLAG_NONE; // Passive: Handled at AI Turn Start
}

static u32 trojan_joker_effect(Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
) 
{
    return JOKER_EFFECT_FLAG_NONE; // Passive: Handled at Score Compare
}

// C JOKER (Left Half - The Brawn)
static u32 c_joker_effect(Joker* joker, Card* scored_card, enum JokerEvent joker_event, JokerEffect** joker_effect) {
    // Always gives +100 Chips
    if (joker_event == JOKER_EVENT_INDEPENDENT) {
        custom_joker_effect_out.chips = 100; 
        *joker_effect = &custom_joker_effect_out; // Pass our manual struct back to the engine
        return JOKER_EFFECT_FLAG_CHIPS;
    }
    return JOKER_EFFECT_FLAG_NONE;
}

// J JOKER (Right Half - The Brains & Fusion Driver)
static u32 j_joker_effect(Joker* joker, Card* scored_card, enum JokerEvent joker_event, JokerEffect** joker_effect) {
    
    // --- PHASE 1: SCORING (Build the stack & apply it) ---
    if (joker_event == JOKER_EVENT_INDEPENDENT) {
        
        if (is_c_j_fusion_active()) {
            
            // If it's a Pair, juice up the multiplier BEFORE we score!
            if (*get_hand_type() == PAIR) {
                int current_ante = get_current_ante();
                int max_cap = 10; // Default cap for Ante 1 to 3
                
                // Scale the cap based on Ante!
                if (current_ante >= 4) {
                    max_cap = 10 + ((current_ante - 3) * 2);
                    if (max_cap > 20) max_cap = 20; // Hard cap at x20 for Ante 8+
                }
                
                joker->persistent_state += 2; // Adds x2 per Pair
                
                if (joker->persistent_state > max_cap) {
                    joker->persistent_state = max_cap;
                }
            }
            
            // Apply the base +10 Mult AND our stacked X-Mult
            custom_joker_effect_out.mult = 10;
            custom_joker_effect_out.xmult = joker->persistent_state; // Spelled exactly as the compiler requested!
            *joker_effect = &custom_joker_effect_out;
            
            if (joker->persistent_state > 0) {
                return JOKER_EFFECT_FLAG_MULT | JOKER_EFFECT_FLAG_XMULT;
            }
            return JOKER_EFFECT_FLAG_MULT;
        } 
        else {
            // Not fused: Just give the base +10 Mult and clear any ghost xmult
            custom_joker_effect_out.mult = 10;
            custom_joker_effect_out.xmult = 0; 
            *joker_effect = &custom_joker_effect_out;
            return JOKER_EFFECT_FLAG_MULT;
        }
    }

    // --- PHASE 2: DISCHARGE (Reset after the hand is completely done) ---
    if (joker_event == JOKER_EVENT_ON_HAND_SCORED_END) {
        
        if (is_c_j_fusion_active()) {
            // We just played a Straight/Full House/etc! 
            // The massive multiplier was already cashed out in Phase 1.
            // Reset the driver back to 0.
            if (*get_hand_type() != PAIR) {
                joker->persistent_state = 0; 
            }
        }
    }
    
    return JOKER_EFFECT_FLAG_NONE;
}

// --- THE SINS ---

// 1. VAINGLORIOUS JOKER (Pride)
// Gives x2 Mult per scored Face Card, ONLY if the entire played hand consists of Face Cards.
static u32 vainglorious_joker_effect(Joker* joker, Card* scored_card, enum JokerEvent joker_event, JokerEffect** joker_effect) {
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED) {
        CardObject** played = get_played_array();
        int played_size = get_played_top() + 1;
        bool only_faces = true;

        // Check the entire played hand for numbered peasant cards
        for (int i = 0; i < played_size; i++) {
            if (!card_is_face(played[i]->card)) {
                only_faces = false;
                break; // A number card was found, break the loop and ruin the bonus
            }
        }

        // If the hand is pure royalty, and the currently scoring card is a face card:
        if (only_faces && card_is_face(scored_card)) {
            *joker_effect = &modded_shared_joker_effect;
            (*joker_effect)->xmult = 2; // GBA uses whole integers, so X2 Mult!
            return JOKER_EFFECT_FLAG_XMULT;
        }
    }
    return JOKER_EFFECT_FLAG_NONE;
}

// 2. SLOTH JOKER
// X4 Mult if you are lazy enough to play exactly 1 card!
static u32 sloth_joker_effect(Joker* joker, Card* scored_card, enum JokerEvent joker_event, JokerEffect** joker_effect) {
    // We use JOKER_EVENT_INDEPENDENT so it multiplies the score AFTER the base chips/mult are tallied
    if (joker_event == JOKER_EVENT_INDEPENDENT) {
        int played_size = get_played_top() + 1;
        
        // Check if the player was lazy enough to only put a single card on the table
        if (played_size == 1) {
            *joker_effect = &modded_shared_joker_effect;
            
            (*joker_effect)->xmult = 4; // Massive X4 Mult reward!
            
            return JOKER_EFFECT_FLAG_XMULT;
        }
    }
    return JOKER_EFFECT_FLAG_NONE;
}

// 3. ENVIOUS JOKER (ID 114)
// Jealous of the Strong: The lowest value card played becomes 2x the value of the highest card!
static u32 envious_joker_effect(Joker* joker, Card* scored_card, enum JokerEvent joker_event, JokerEffect** joker_effect) {
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED) {
        CardObject** played = get_played_array();
        int played_size = get_played_top() + 1;
        if (played_size == 0) return JOKER_EFFECT_FLAG_NONE;

        u8 max_val = 0;
        u8 min_val = 255; 

        for (int i = 0; i < played_size; i++) {
            if (played[i] != NULL && played[i]->card != NULL) {
                u8 val = card_get_value(played[i]->card);
                if (val > max_val) max_val = val;
                if (val < min_val) min_val = val;
            }
        }

        if (card_get_value(scored_card) == min_val) {
            int target_value = max_val * 2;
            int bonus_chips = target_value - min_val;
            if (bonus_chips > 0) {
                *joker_effect = &modded_shared_joker_effect;
                (*joker_effect)->chips = bonus_chips;
                return JOKER_EFFECT_FLAG_CHIPS;
            }
        }
    }
    return JOKER_EFFECT_FLAG_NONE;
}

// 4. THE PENTACLE (IDs 115, 116, 117)
// The Exodia Engine: Combines the Absolved (Unrestricted) powers of every Sin it eats.
static u32 pentacle_joker_effect(Joker* joker, Card* scored_card, enum JokerEvent joker_event, JokerEffect** joker_effect) {
    // Form 1 (ID 115) is dormant. It does absolutely nothing until it evolves!
    if (joker->id == 115) return JOKER_EFFECT_FLAG_NONE;

    u32 flags = JOKER_EFFECT_FLAG_NONE;
    int added_mult = 0;
    int added_chips = 0;
    int total_xmult = 1;

    // --- PHASE 1: SCORING INDIVIDUAL CARDS ---
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED) {
        
        // Absolved Suit Sins: +3 Mult per card, IGNORING what suit it is!
        if (acquired_sins_mask & (1 << 0)) added_mult += 3; // Greedy
        if (acquired_sins_mask & (1 << 1)) added_mult += 3; // Lusty
        if (acquired_sins_mask & (1 << 2)) added_mult += 3; // Wrathful
        if (acquired_sins_mask & (1 << 3)) added_mult += 3; // Gluttonous

        // Absolved Vainglorious: X2 Mult per face card, IGNORING if numbered cards are in the hand!
        if ((acquired_sins_mask & (1 << 4)) && card_is_face(scored_card)) {
            total_xmult *= 2;
        }

        // Absolved Envy: EVERY card played steals double the power of the highest card!
        if (acquired_sins_mask & (1 << 6)) {
            CardObject** played = get_played_array();
            int played_size = get_played_top() + 1;
            u8 max_val = 0;
            for (int i = 0; i < played_size; i++) {
                if (played[i] && played[i]->card) {
                    u8 val = card_get_value(played[i]->card);
                    if (val > max_val) max_val = val;
                }
            }
            int target_value = max_val * 2;
            int base_val = card_get_value(scored_card);
            if (target_value > base_val) added_chips += (target_value - base_val);
        }
    }

    // --- PHASE 2: INDEPENDENT SCORING ---
    if (joker_event == JOKER_EVENT_INDEPENDENT) {
        // Absolved Sloth: X4 Mult globally, IGNORING the 1-card limit!
        if (acquired_sins_mask & (1 << 5)) total_xmult *= 4;

        // Form 3 (ID 117): The Final Awakening Global X7!
        if (joker->id == 117) total_xmult *= 7;
    }

    // --- PHASE 3: ROUND END PAYOUT ---
    if (joker_event == JOKER_EVENT_ON_ROUND_END) {
        if (joker->id == 117) {
            *joker_effect = &modded_shared_joker_effect;
            (*joker_effect)->money = 777;
            return JOKER_EFFECT_FLAG_MONEY;
        }
    }

    // Accumulate all the Absolved math and send it to the engine!
    if (added_mult > 0 || added_chips > 0 || total_xmult > 1) {
        *joker_effect = &modded_shared_joker_effect;
        (*joker_effect)->mult = added_mult;
        (*joker_effect)->chips = added_chips;
        if (total_xmult > 1) (*joker_effect)->xmult = total_xmult;
        
        if (added_mult > 0) flags |= JOKER_EFFECT_FLAG_MULT;
        if (added_chips > 0) flags |= JOKER_EFFECT_FLAG_CHIPS;
        if (total_xmult > 1) flags |= JOKER_EFFECT_FLAG_XMULT;
    }

    return flags;
}

// --- 2. YOUR MODDED REGISTRY ---

// The engine knows to start reading this array at ID 100.
// So Index 0 is ID 100 (Mobius), Index 1 is ID 101 (Last Dance).
// Because we set NUM_JOKERS_PER_SPRITESHEET to 2, 
// Mobius reads the Left half, Last Dance reads the Right half!
const JokerInfo modded_joker_registry[] = {
    { UNCOMMON_JOKER,         5,     recursion_joker_effect        }, // Index 0 -> ID 100 (Recursion)
    { RARE_JOKER,            12,     last_dance_joker_effect       }, // Index 1 -> ID 101 (Last Dance)
    { COMMON_JOKER,           4,     voor_joker_effect             }, // Index 2 -> ID 102 (Voor)
    { UNCOMMON_JOKER,         7,     jaker_joker_effect            }, // Index 3 -> ID 103 (Jaker)
    { RARE_JOKER,            13,     capacocha_joker_effect        }, // Index 4 -> ID 104 (Capacocha)
    { COMMON_JOKER,           5,     overkill_joker_effect         }, // Index 5 -> ID 105 (Overkill)
    { RARE_JOKER,             6,     jamming_joker_effect          }, // ID 106 (Jamming) Clanker
    { RARE_JOKER,             5,     captcha_joker_effect          }, // ID 107 (CaptchA) Clanker
    { RARE_JOKER,             5,     ddos_joker_effect             }, // ID 108 (DDoS Attack) Clanker
    { UNCOMMON_JOKER,         7,     trojan_joker_effect           }, // ID 109 (Trojan Joker) Clanker
    { RARE_JOKER,            10,     c_joker_effect                }, // ID 110 (Cyclone Joker)
    { RARE_JOKER,            10,     j_joker_effect                }, // ID 111 (Joker Joker)
    { RARE_JOKER,            10,     vainglorious_joker_effect     }, // ID 112 (Vainglorious)
    { COMMON_JOKER,           8,     sloth_joker_effect            }, // ID 113 (Sloth)
    { UNCOMMON_JOKER,        10,     envious_joker_effect          }, // ID 114 (Envious)
    { RARE_JOKER,             7,      pentacle_joker_effect        }, // ID 115 (Form 1 - Dormant)
    { RARE_JOKER,             7,      pentacle_joker_effect        }, // ID 116 (Form 2 - Awakened)
    { RARE_JOKER,             7,      pentacle_joker_effect        }, // ID 117 (Form 3 - Final)
};


// --- 3. HELPER FUNCTIONS FOR THE ENGINE ---
// (Do not change these! The vanilla game uses them to read your mods safely)

size_t get_modded_registry_size(void) 
{
    return sizeof(modded_joker_registry) / sizeof(modded_joker_registry[0]);
}

const JokerInfo* get_modded_registry_entry(int local_id) 
{
    return &modded_joker_registry[local_id];
}