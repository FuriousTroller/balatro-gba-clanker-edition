#include "joker.h"
#include "game.h"
#include "list.h"
#include "util.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "custom_joker_sheet_0.h"
#include "custom_joker_sheet_1.h"
#include "custom_joker_sheet_2.h"
#include "custom_joker_sheet_3.h"
#include "custom_joker_sheet_4.h"
#include "custom_joker_sheet_5.h"
#include "custom_joker_sheet_6.h"
#include "custom_joker_sheet_7.h"
#include "custom_joker_sheet_8.h"
#include "custom_joker_sheet_9.h"
#include "custom_joker_sheet_10.h"
#include "custom_joker_sheet_11.h"
#include "custom_joker_sheet_12.h"
#include "custom_joker_sheet_13.h"
#include "custom_joker_sheet_14.h"
#include "custom_joker_sheet_15.h"
#include "custom_joker_sheet_16.h"
#include "custom_joker_sheet_17.h"
#include "custom_joker_sheet_18.h"
#include "custom_joker_sheet_19.h"
#include "custom_joker_sheet_20.h"
#include "custom_joker_sheet_21.h"
#include "custom_joker_sheet_22.h"
#include "custom_joker_sheet_23.h"
// #include "custom_joker_sheet_x.h" // Add this when you make IDs 1xx & 1xx!

// Creates a local shared memory struct specifically for your modded cards to use
static JokerEffect shared_joker_effect = {0};

// Tells the compiler to go find this variable inside game.c
extern int overkill_payout;
extern int get_current_blind(void);
extern int get_current_joker_index(void);
extern void destroy_designated_joker(int joker_index);
extern int get_current_ante(void);
extern void add_common_joker(void);
extern void offset_current_joker_index(int offset);
extern void show_updated_level(void);
extern bool is_c_j_fusion_active(void);
extern bool check_probablity(int probability);
extern void improve_hand_level(int hand_to_improve);
extern int get_current_hand_type();
#define MODDED_JOKER_START_ID 100
#define NUM_JOKERS_PER_SPRITESHEET 2
#define CLAMP_CUSTOM(x, low, high) ((x) > (high) ? (high) : ((x) < (low) ? 0 : (x)))
static char xbuff[UINT_MAX_DIGITS+1];
static JokerEffect custom_joker_effect_out = {0};

// --- 0. LOCAL EFFECT OBJECT ---
// We use this local object so we don't conflict with the vanilla file's locked memory
static JokerEffect modded_shared_joker_effect = {0};

#define SCORE_ON_EVENT_ONLY(restricted_event, checked_event) \
    if (checked_event != restricted_event)                   \
    {                                                        \
        return JOKER_EFFECT_FLAG_NONE;                       \
    }
// --- 1. YOUR CUSTOM JOKER LOGIC ---
bool is_suit_equal(int suit1, int suit2){
    if (is_joker_owned(58)){
        return suit1 % 2 == suit2 % 2;
    }
    else{
        return suit1 == suit2;
    }
}
static u32 mobius_joker_effect(
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

static u32 test_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    s32* p_last_retriggered_index = &(joker->scoring_state); // Reuse scoring_state to keep track of the last index we retriggered on
    static const char* retrigger_messages[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
    switch (joker_event)
    {
        case JOKER_EVENT_ON_HAND_PLAYED:
            // start at -1 so that a first index of 0 can satisfy the retrigger condition below
            *p_last_retriggered_index = 9999;
            break;

        case JOKER_EVENT_ON_CARD_HELD_END:
            // Only retrigger current card if it's strictly after the last one we retriggered
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->retrigger = (*p_last_retriggered_index > get_scored_card_index());
            if ((*joker_effect)->retrigger)
            {
                *p_last_retriggered_index = get_scored_card_index();
                (*joker_effect)->message = (char*)retrigger_messages[*p_last_retriggered_index]; // Just a fun way to visualize the retrigger order
                effect_flags_ret = JOKER_EFFECT_FLAG_RETRIGGER | JOKER_EFFECT_FLAG_MESSAGE;
            }                
            break;
        default:
            break;
    
    }
    return effect_flags_ret;
}
static u32 baron_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    // SCORE_ON_EVENT_ONLY(JOKER_EVENT_ON_CARD_HELD, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (joker_event == JOKER_EVENT_ON_CARD_HELD && scored_card->rank == KING)
    {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->mult =  (get_mult() * 50 + 99) / 100;
        (*joker_effect)->message = "X1`";
        effect_flags_ret = JOKER_EFFECT_FLAG_XDECIMAL;
    }

    return effect_flags_ret;
}
static u32 shoot_moon_joker_effect(
    Joker* joker,
    Card* scored_card,
    enum JokerEvent joker_event,
    JokerEffect** joker_effect
)
{
    // SCORE_ON_EVENT_ONLY(JOKER_EVENT_ON_CARD_HELD, joker_event)

    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (joker_event == JOKER_EVENT_ON_CARD_HELD && scored_card->rank == QUEEN)
    {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->mult = 13;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }

    if (joker_event == JOKER_EVENT_ON_CARD_SCORED_END)
    {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->message = "Shoot!";
        effect_flags_ret = JOKER_EFFECT_FLAG_MESSAGE;
    }

    return effect_flags_ret;
}

static u32 loyalty_card_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    static const char* loyal_messages[]={"1","2","3","4","5"};
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 0; 
    }

    if (joker_event == JOKER_EVENT_ON_HAND_PLAYED) {
        joker->persistent_state++; 
    }
    
    if (joker_event == JOKER_EVENT_INDEPENDENT && joker->persistent_state % 6 == 0) {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->xmult = 4; 
        return JOKER_EFFECT_FLAG_XMULT;
    }
    else if (joker_event == JOKER_EVENT_INDEPENDENT && joker->persistent_state % 6 != 0) 
    {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->message = (char*)loyal_messages[(joker->persistent_state % 6) - 1];
        return JOKER_EFFECT_FLAG_MESSAGE;
    }
    return effect_flags_ret; 
}
static u32 egg_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
  
    if (joker_event == JOKER_EVENT_ON_ROUND_END) {
        joker->value+=6; 
        /*
        *joker_effect = &modded_shared_joker_effect;
        (*joker_effect)->message = "Hatched!";
        return JOKER_EFFECT_FLAG_MESSAGE;
        */
    }

    return effect_flags_ret; 
}
static u32 delayed_gratification_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
  
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 0;    
    }
    if (joker_event == JOKER_EVENT_ON_ROUND_END) {
        if (joker->persistent_state == 0) {
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->money = 2 * get_num_discards_remaining(); 
            return JOKER_EFFECT_FLAG_MONEY;     
        } 
        else {
            joker->persistent_state = 0; 
        }
    }
    return effect_flags_ret; 
}
static u32 credit_card_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    // You can add your actual Mobius logic here whenever you are ready!
    return JOKER_EFFECT_FLAG_NONE; 
}
static u32 invisible_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
  
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 2;    
    }
    if (joker_event == JOKER_EVENT_ON_ROUND_END) {        
        joker->persistent_state--;
        if (joker->persistent_state <= 0) {
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->message = "Can sell!";
            return JOKER_EFFECT_FLAG_MESSAGE;     
        }         
    }
    return effect_flags_ret; 
}

static u32 seeing_double_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    // 必须遍历记分牌，不能用score事件，因为有retrigger
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 0;    
    }

    if (joker_event == JOKER_EVENT_ON_HAND_PLAYED){
        joker->persistent_state = 0; 
        CardObject** card_objects = get_played_array();
        for (int i = 0; i <= get_played_top(); i++) {
            if (!card_objects[i]->selected) continue; // Only count cards that are actually selected for scoring
            u8 current_suit = card_objects[i]->card->suit;  
            if (is_suit_equal(current_suit, CLUBS) && 
                (joker->persistent_state & 1) == 0) {
                joker->persistent_state |= 1; 
                continue;
            } 
            if ((is_suit_equal(current_suit, SPADES) ||
                is_suit_equal(current_suit, HEARTS) ||
                is_suit_equal(current_suit, DIAMONDS)) && 
                (joker->persistent_state & (1<<1)) == 0) {
                joker->persistent_state |= (1<<1); 
            } 
        }
    }

    if (joker_event == JOKER_EVENT_INDEPENDENT && joker->persistent_state == 3) {        
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->xmult = 2; 
        return JOKER_EFFECT_FLAG_XMULT;        
    }
    return effect_flags_ret; 
}

static u32 ride_bus_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
) 
{
    u32 flags = JOKER_EFFECT_FLAG_NONE;

    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 2; 
    }
    if (joker_event == JOKER_EVENT_ON_HAND_PLAYED) {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->message = "All Aboard!";
        flags |= JOKER_EFFECT_FLAG_MESSAGE;
    }
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED && card_is_face(scored_card)) {
        joker->persistent_state = 1; 
    }
    if (joker_event == JOKER_EVENT_ON_HAND_SCORED_END) {
        if (joker->persistent_state % 2 == 0) {
            joker->persistent_state += 2; 
        }
        else {
            joker->persistent_state = 2; // Reset the bus if it never got past the starting point
        }                
    }
    if (joker_event == JOKER_EVENT_INDEPENDENT) {
        if (joker->persistent_state > 1) {
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->mult = joker->persistent_state / 2;
            flags |= JOKER_EFFECT_FLAG_MULT; 
        }      
    }
    return flags;
}
static u32 todo_list_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
) 
{
    u32 flags = JOKER_EFFECT_FLAG_NONE;

    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = random() % 8 + 1;   
    }

    if (joker_event == JOKER_EVENT_ON_HAND_PLAYED) {
        if (joker->persistent_state == *get_hand_type()) {
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->money = 5;  
            flags |= JOKER_EFFECT_FLAG_MONEY; 
        }
    }
    if (joker_event == JOKER_EVENT_ON_ROUND_END){
        joker->persistent_state = random() % 8 + 1;
    }
    if (joker_event == JOKER_EVENT_INDEPENDENT) {             
        *joker_effect = &shared_joker_effect;
        switch (joker->persistent_state)
        {
        case HIGH_CARD:
            (*joker_effect)->message = "!HC";
            break;
        case PAIR:
            (*joker_effect)->message = "!1P";
            break;
        case TWO_PAIR:
            (*joker_effect)->message = "!2P";
            break;
        case THREE_OF_A_KIND:
            (*joker_effect)->message = "!3K";
            break;
        case STRAIGHT:
            (*joker_effect)->message = "!ST";
            break;
        case FLUSH:
            (*joker_effect)->message = "!FL";
            break;
        case FULL_HOUSE:
            (*joker_effect)->message = "!FH";
            break;
        case FOUR_OF_A_KIND:
            (*joker_effect)->message = "!4K";
            break;
        case STRAIGHT_FLUSH:
            (*joker_effect)->message = "!SF";
            break;
        default:
            (*joker_effect)->message = "!??";
            break;
        }
        flags |= JOKER_EFFECT_FLAG_MESSAGE;
        
    }
    return flags;
}
static u32 gift_card_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
  
    if (joker_event == JOKER_EVENT_ON_ROUND_END) {
        List* jokers_list = get_jokers_list();
        for (int i = 0; i < list_get_len(jokers_list); i++) {
            JokerObject* j_obj = list_get_at_idx(jokers_list, i);
            j_obj->joker->value+=2; 
        }
    }

    return effect_flags_ret; 
}
static u32 chaos_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    return JOKER_EFFECT_FLAG_NONE; 
}
static u32 stuntman_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED){
        joker->persistent_state = -2;
    }
    if (joker_event == JOKER_EVENT_INDEPENDENT) {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->chips = 250;       
        effect_flags_ret |= JOKER_EFFECT_FLAG_CHIPS;
    }
    return effect_flags_ret;  
}
static u32 baseball_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_INDEPENDENT_END) {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->mult =  (get_mult() * 50 + 99) / 100;
        (*joker_effect)->message = "X1`";
        effect_flags_ret = JOKER_EFFECT_FLAG_XDECIMAL;
    }
    return effect_flags_ret;  
}
static u32 riff_raff_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_BLIND_SELECTED) {    
        int joker_can_add = CLAMP_CUSTOM(5 - list_get_len(get_jokers_list()), 1, 2);
        for (int i = 0; i < joker_can_add; i++){            
            add_common_joker();
        }
    }
    return effect_flags_ret;  
}
static u32 rocket_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 1;   
    }

    if (joker_event == JOKER_EVENT_ON_ROUND_END) {
        *joker_effect = &shared_joker_effect;
        if (get_current_blind()==2)
        {
            joker->persistent_state += 2;
        }     
        (*joker_effect)-> money = joker->persistent_state;
        effect_flags_ret = JOKER_EFFECT_FLAG_MONEY;
    }
    return effect_flags_ret;  
}

static u32 ceremonial_dagger_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    int owned_joker_idx = get_current_joker_index();
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 0;   
    }
    if (joker_event == JOKER_EVENT_ON_BLIND_SELECTED &&
        owned_joker_idx < list_get_len(get_jokers_list()) - 1 &&
        joker->id != BLUEPRINT_JOKER_ID && joker->id != BRAINSTORM_JOKER_ID) // Only trigger if we aren't the last Joker in the list (to avoid out of bounds)
    {   
        JokerObject* next_joker_obj = list_get_at_idx(get_jokers_list(), owned_joker_idx + 1);
        joker->persistent_state += next_joker_obj->joker->value;
        destroy_designated_joker(owned_joker_idx + 1); // This will remove the next Joker from our owned list and push it to the expired list, effectively "sacrificing" it. The engine will handle the rest based on the expire flag we set in the effect below.
        list_push_back(get_expired_jokers_list(), next_joker_obj); 
        // For madness, if the previous clown is eaten, use offset_current_joker_idx by -1.
    }
    if (joker_event==JOKER_EVENT_INDEPENDENT && joker->persistent_state > 0)
    {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->mult = joker->persistent_state;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;  
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 castle_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = random() % 4 ; // USE LOW DIGIT TO TRACK SUIT, HIGH DIGITS FOR CHIPS. This way we can have a random suit each round while still tracking chips in the same variable without needing extra state variables!
    }
    if (joker_event == JOKER_EVENT_ON_HAND_DISCARDED)   {
        CardObject** current_hand = get_hand_array();    
        // Scan the cards you are currently holding BEFORE they vanish
        for (int i = 0; i <= get_hand_top(); i++) {
            if (current_hand[i] != NULL && card_object_is_selected(current_hand[i])) {
                if (is_suit_equal(current_hand[i]->card->suit, joker->persistent_state % 4)){ // If the card belongs to the suit we chose at random when conjuring this Joker{
                    joker->persistent_state += (3 << 2);
                }
            }
        }
    }
    if (joker_event==JOKER_EVENT_INDEPENDENT && (joker->persistent_state >> 2) > 0)
    {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->chips = joker->persistent_state >> 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;  
    }
    if (joker_event==JOKER_EVENT_ON_HAND_PLAYED)
    {
        static const char* end_suits[] = {"Diamond", "Club", "Heart", "Spade" };
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->message = (char*)end_suits[joker->persistent_state % 4];
        effect_flags_ret = JOKER_EFFECT_FLAG_MESSAGE;   
    }
    if (joker_event==JOKER_EVENT_ON_ROUND_END)
    {
        joker->persistent_state = joker->persistent_state / 4 * 4 + random() % 4;
    }
    return effect_flags_ret;  
}
static u32 triboulet_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED && (scored_card->rank == QUEEN || scored_card->rank == KING)) {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->xmult  = 2;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 yorick_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_HAND_DISCARDED)   {   
        joker->persistent_state += (get_hand_top()+1);
    }
    if (joker_event == JOKER_EVENT_INDEPENDENT && joker->persistent_state>=23) {
         *joker_effect = &shared_joker_effect;
        (*joker_effect)->xmult = joker->persistent_state / 23 + 1;
        effect_flags_ret = JOKER_EFFECT_FLAG_XMULT;
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 wee_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED && scored_card->rank==TWO)   {   
        joker->persistent_state += 8;
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->message = "UP!";
        effect_flags_ret = JOKER_EFFECT_FLAG_MESSAGE;
    }
    if (joker_event == JOKER_EVENT_INDEPENDENT && joker->persistent_state > 0) {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->chips = joker->persistent_state;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 RoughGem_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED && is_suit_equal(scored_card->suit, DIAMONDS)){   
         *joker_effect = &shared_joker_effect;
        (*joker_effect)->money=1;
        effect_flags_ret = JOKER_EFFECT_FLAG_MONEY;
    }
    return effect_flags_ret;  
}
static u32 Bloodstone_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED && is_suit_equal(scored_card->suit, HEARTS) 
        && check_probablity(500)){   
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->mult =  (get_mult() * 50 + 99) / 100;
        (*joker_effect)->message = "X1`";
        effect_flags_ret = JOKER_EFFECT_FLAG_XDECIMAL;
    }
    return effect_flags_ret;  
}
static u32 OnyxAgate_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED && is_suit_equal(scored_card->suit, CLUBS)){   
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->mult = 7;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }
    return effect_flags_ret;  
}
static u32 Arrowhead_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_CARD_SCORED && is_suit_equal(scored_card->suit, SPADES)){   
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->chips = 50;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 faceless_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_HAND_DISCARDED)   {
        CardObject** current_hand = get_hand_array();    
        // Scan the cards you are currently holding BEFORE they vanish
        int face_number=0;
        for (int i = 0; i <= get_hand_top(); i++) {
            if (current_hand[i] != NULL && card_object_is_selected(current_hand[i])) {
                if (card_is_face(current_hand[i]->card)){ 
                    face_number++;
                }
            }
        }
        if (face_number>=3){
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->money=5;
            effect_flags_ret = JOKER_EFFECT_FLAG_MONEY;
        }
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 to_moon_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED){
        joker->persistent_state = 1;
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 spare_trousers_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event==JOKER_EVENT_ON_HAND_PLAYED && get_contained_hands()->TWO_PAIR){
        joker->persistent_state += 2;
    }
    if (joker_event==JOKER_EVENT_INDEPENDENT && joker->persistent_state>0){
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->mult = joker->persistent_state;
        effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 burglar_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event==JOKER_EVENT_ON_BLIND_SELECTED){
        int burg_hands = get_num_hands_remaining();
        set_num_hands_remaining(burg_hands+3);
        set_num_discards_remaining(0);
    }
    return effect_flags_ret;  
} 
GBAL_UNUSED
static u32 swashbuckler_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event==JOKER_EVENT_INDEPENDENT){
        int total_value = 0;
        List* jokers_list = get_jokers_list();
        for (int i = 0; i < list_get_len(jokers_list); i++) {
            JokerObject* j_obj = list_get_at_idx(jokers_list, i);
            total_value+=j_obj->joker->value; 
        }
        total_value-=joker->value;
        if (total_value>0){
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->mult = total_value/2;
            effect_flags_ret = JOKER_EFFECT_FLAG_MULT;
        }
          
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 turtle_bean_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    if (joker_event==JOKER_EVENT_ON_JOKER_CREATED){
        joker->persistent_state = 5;
    }
    if (joker_event==JOKER_EVENT_ON_ROUND_END){
        if (joker->persistent_state > 1){
            joker->persistent_state -= 1;
        }
        else {
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->expire = true;
            effect_flags_ret = JOKER_EFFECT_FLAG_EXPIRE;
        }    
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 madness_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    int owned_joker_idx = get_current_joker_index();
    if (joker_event == JOKER_EVENT_ON_JOKER_CREATED) {
        joker->persistent_state = 0;   
    }
    if (joker_event == JOKER_EVENT_ON_BLIND_SELECTED && get_current_blind()!=2 &&
       joker->id != BLUEPRINT_JOKER_ID && joker->id != BRAINSTORM_JOKER_ID) 
    {      
        int destroy_idx = random() % (list_get_len(get_jokers_list()) - 1);
        if (destroy_idx >= owned_joker_idx) {
            destroy_idx +=1;      
        }
        else{
            offset_current_joker_index(-1); 
        }
        JokerObject* destroy_joker_obj = list_get_at_idx(get_jokers_list(), destroy_idx);
        destroy_designated_joker(destroy_idx); // This will remove the next Joker from our owned list and push it to the expired list, effectively "sacrificing" it. The engine will handle the rest based on the expire flag we set in the effect below.
        list_push_back(get_expired_jokers_list(), destroy_joker_obj); 
        joker->persistent_state += 50;     
    }
    if (joker_event==JOKER_EVENT_INDEPENDENT && joker->persistent_state > 0)
    {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->mult = (get_mult() * joker->persistent_state + 99) / 100;
        truncate_uint_to_suffixed_str((100+joker->persistent_state)*10, 3, xbuff);
        int len = strlen(xbuff);
        if (len > 0) {
            char temp[UINT_MAX_DIGITS + 1];
            xbuff[len - 1] = '\0'; // 先删掉最后一个
            snprintf(temp, sizeof(temp), "X%s", xbuff); // 拼接新的到 temp
            strcpy(xbuff, temp); // 拷回 xbuff
        }
        (*joker_effect)->message = xbuff;
        effect_flags_ret = JOKER_EFFECT_FLAG_XDECIMAL;  
    }
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 space_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;

    if (joker_event == JOKER_EVENT_ON_HAND_PLAYED && check_probablity(250)) { // 30% chance to trigger when you play a hand
        improve_hand_level(get_current_hand_type()); 
        show_updated_level();  
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->message = "UP!";
        effect_flags_ret = JOKER_EFFECT_FLAG_MESSAGE;  
    }
   
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 card_sharp_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;
    
    if (joker_event == JOKER_EVENT_INDEPENDENT) {
        int flg = 1 << get_current_hand_type(); 
        if (joker->persistent_state & flg){
            *joker_effect = &shared_joker_effect;
            (*joker_effect)->xmult = 3;
            effect_flags_ret = JOKER_EFFECT_FLAG_XMULT; 
        }
        else{
            joker->persistent_state |= flg;
        }
    }
    if (joker_event == JOKER_EVENT_ON_ROUND_END) {
        joker->persistent_state = 0;
    }
   
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 oops_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE;   
    return effect_flags_ret;  
}
GBAL_UNUSED
static u32 runner_joker_effect(
    Joker* joker, 
    Card* scored_card, 
    enum JokerEvent joker_event, 
    JokerEffect** joker_effect
)
{
    u32 effect_flags_ret = JOKER_EFFECT_FLAG_NONE; 
    if (joker_event == JOKER_EVENT_ON_HAND_PLAYED && get_contained_hands()->STRAIGHT) {
        joker -> persistent_state += 15; 
    }  
    if (joker_event == JOKER_EVENT_INDEPENDENT && joker->persistent_state > 0) {
        *joker_effect = &shared_joker_effect;
        (*joker_effect)->chips = joker->persistent_state;
        effect_flags_ret = JOKER_EFFECT_FLAG_CHIPS;
    }       
    
    return effect_flags_ret;  
}
// --- 2. YOUR MODDED REGISTRY ---

// The engine knows to start reading this array at ID 100.
// So Index 0 is ID 100 (Mobius), Index 1 is ID 101 (Last Dance).
// Because we set NUM_JOKERS_PER_SPRITESHEET to 2, 
// Mobius reads the Left half, Last Dance reads the Right half!
const JokerInfo modded_joker_registry[] = {
    { UNCOMMON_JOKER,        7,      mobius_joker_effect                }, // Index 0 -> ID 100 (Mobius)
    { RARE_JOKER,            20,     last_dance_joker_effect            }, // Index 1 -> ID 101 (Last Dance)
    { COMMON_JOKER,          7,      voor_joker_effect                  }, // Index 2 -> ID 102 (Voor)
    { UNCOMMON_JOKER,        10,     jaker_joker_effect                 }, // Index 3 -> ID 103 (Jaker)
    { RARE_JOKER,            18,     capacocha_joker_effect             }, // Index 4 -> ID 104 (Capacocha)
    { COMMON_JOKER,          6,      overkill_joker_effect              }, // Index 5 -> ID 105 (Overkill)
    { RARE_JOKER,            17,     jamming_joker_effect               }, // ID 106 (Jamming) Clanker
    { RARE_JOKER,            13,     captcha_joker_effect               }, // ID 107 (CaptchA) Clanker
    { RARE_JOKER,            15,     ddos_joker_effect                  }, // ID 108 (DDoS Attack) Clanker
    { UNCOMMON_JOKER,        12,     trojan_joker_effect                }, // ID 109 (Trojan Joker) Clanker
    { RARE_JOKER,            16,     c_joker_effect                     }, // ID 110 (Cyclone Joker)
    { RARE_JOKER,            16,     j_joker_effect                     }, // ID 111 (Joker Joker)
    { COMMON_JOKER,          5,      test_joker_effect                  }, // ID 112 (test Joker, used for testing various flags and conditions, feel free to change the logic as you see fit!)
    { UNCOMMON_JOKER,        6,      turtle_bean_joker_effect           }, // ID 113
    { COMMON_JOKER,          4,      egg_joker_effect                   }, // ID 114 (Egg Joker)
    { COMMON_JOKER,          4,      delayed_gratification_joker_effect }, // ID 115 (Delayed Gratification)
    { COMMON_JOKER,          1,      credit_card_joker_effect           }, // ID 116 (Credit Card)
    { RARE_JOKER,            10,     invisible_joker_effect             }, // ID 117 (Invisible Joker)
    { UNCOMMON_JOKER,        6,      seeing_double_joker_effect         }, // ID 118 (Seeing Double)
    { COMMON_JOKER,          6,      ride_bus_joker_effect              }, // ID 119 (Ride the Bus)
    { COMMON_JOKER,          5,      todo_list_joker_effect             }, // ID 120 (To-Do List)
    { UNCOMMON_JOKER,        6,      gift_card_joker_effect             }, // ID 121 (Gift Card)
    { COMMON_JOKER,          4,      chaos_joker_effect                 }, // ID 122 (Chaos Joker)
    { RARE_JOKER,            7,      stuntman_joker_effect              }, // ID 123 (Stuntman Joker)
    { COMMON_JOKER,          5,      shoot_moon_joker_effect            }, // ID 124 (Shoot the Moon)
    { UNCOMMON_JOKER,        5,      loyalty_card_joker_effect          }, // ID 125 (Loyalty Card)
    { RARE_JOKER,            8,      baseball_joker_effect              }, // ID 126 (Baseball Joker)
    { COMMON_JOKER,          6,      riff_raff_joker_effect             }, // ID 127 (Riff Raff Joker)
    { UNCOMMON_JOKER,        5,      rocket_joker_effect                }, // ID 128 (Rocket Joker)
    { COMMON_JOKER,          8,      ceremonial_dagger_joker_effect     }, // ID 129 (Ceremonial Dagger)
    { UNCOMMON_JOKER,        6,      castle_joker_effect                }, // ID 130 (Castle Joker)
    { LEGENDARY_JOKER,       20,     triboulet_joker_effect             }, // ID 131 (Triboulet Joker)
    { RARE_JOKER,            8,      wee_joker_effect                   },
    { LEGENDARY_JOKER,       20,     yorick_joker_effect                }, 
    { UNCOMMON_JOKER,        7,      Bloodstone_joker_effect            },
    { UNCOMMON_JOKER,        7,      RoughGem_joker_effect              },
    { UNCOMMON_JOKER,        7,      OnyxAgate_joker_effect             },
    { UNCOMMON_JOKER,        7,      Arrowhead_joker_effect             },
    { UNCOMMON_JOKER,        5,      to_moon_joker_effect               },
    { UNCOMMON_JOKER,        6,      spare_trousers_joker_effect        },
    { UNCOMMON_JOKER,        6,      burglar_joker_effect               },
    { COMMON_JOKER,          4,      swashbuckler_joker_effect          },
    { RARE_JOKER,            8,      baron_joker_effect                 }, // ID 142
    { UNCOMMON_JOKER,        7,      madness_joker_effect               }, 
    { UNCOMMON_JOKER,        5,      space_joker_effect                 },
    { UNCOMMON_JOKER,        6,      card_sharp_joker_effect            }, // ID 145  
    { UNCOMMON_JOKER,        4,      oops_joker_effect                  }, // ID 146 (Oops Joker)
    { COMMON_JOKER,          5,      runner_joker_effect                }, // ID 147 (Runner Joker)
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