// --- source/modded_jokers_gfx.c ---
#include "modded_jokers_gfx.h"
#include <stddef.h>

// 1. Include your 64x32 spritesheet header here
// (Make sure you ran 'make' so grit generates this from your .png!)
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

// Add this when you make IDs 1xx & 1xx!

#define MODDED_JOKER_START_ID 100
#define NUM_JOKERS_PER_SPRITESHEET 2

#define PALETTE_SIZE 16

// static unsigned short reversed_pals[14][PALETTE_SIZE];

// 2. Put your sheets into the arrays
static const unsigned int* modded_joker_tiles[] = { 
    custom_joker_sheet_0Tiles, 
    custom_joker_sheet_1Tiles,
    custom_joker_sheet_2Tiles,
    custom_joker_sheet_3Tiles,
    custom_joker_sheet_4Tiles,
    custom_joker_sheet_5Tiles,
    custom_joker_sheet_6Tiles,
    custom_joker_sheet_7Tiles,
    custom_joker_sheet_8Tiles,
    custom_joker_sheet_9Tiles,
    custom_joker_sheet_10Tiles,
    custom_joker_sheet_11Tiles,
    custom_joker_sheet_12Tiles, 
    custom_joker_sheet_13Tiles,  
    custom_joker_sheet_14Tiles,   
    custom_joker_sheet_15Tiles,
    custom_joker_sheet_16Tiles,  
    custom_joker_sheet_17Tiles,
    custom_joker_sheet_18Tiles,
    custom_joker_sheet_19Tiles,  
    custom_joker_sheet_20Tiles,
    custom_joker_sheet_21Tiles,    
    custom_joker_sheet_22Tiles, 
    custom_joker_sheet_23Tiles,
};

static const unsigned short* modded_joker_pals[] = { 
    custom_joker_sheet_0Pal, 
    custom_joker_sheet_1Pal,
    custom_joker_sheet_2Pal,
    custom_joker_sheet_3Pal,
    custom_joker_sheet_4Pal,
    custom_joker_sheet_5Pal,
    custom_joker_sheet_6Pal,
    custom_joker_sheet_7Pal,
    custom_joker_sheet_8Pal,
    custom_joker_sheet_9Pal,
    custom_joker_sheet_10Pal,
    custom_joker_sheet_11Pal,
    custom_joker_sheet_12Pal, 
    custom_joker_sheet_13Pal, 
    custom_joker_sheet_14Pal,  
    custom_joker_sheet_15Pal, 
    custom_joker_sheet_16Pal,
    custom_joker_sheet_17Pal,
    custom_joker_sheet_18Pal,
    custom_joker_sheet_19Pal,
    custom_joker_sheet_20Pal,
    custom_joker_sheet_21Pal,
    custom_joker_sheet_22Pal,
    custom_joker_sheet_23Pal,
};

#define NUM_MODDED_SHEETS (sizeof(modded_joker_tiles) / sizeof(modded_joker_tiles[0]))

// 3. The Bypass Function the linker is looking for!
bool get_modded_joker_gfx(int joker_id, 
    const unsigned int** out_tiles, 
    const unsigned short** out_pal
) 
{
    if (joker_id >= MODDED_JOKER_START_ID) 
    {
        // Math to figure out WHICH sheet the Joker is on
        int local_idx = joker_id - MODDED_JOKER_START_ID;
        int sheet_idx = local_idx / NUM_JOKERS_PER_SPRITESHEET;
        
        if (sheet_idx >= NUM_MODDED_SHEETS) 
        {
            sheet_idx = 0; // Fallback to sheet 0 to prevent a crash
        }
        
        // Pass back the whole sheet!
        *out_tiles = modded_joker_tiles[sheet_idx];
        *out_pal = modded_joker_pals[sheet_idx];
        /*
        // Reverse the palette colors
        const unsigned short* original_pal = modded_joker_pals[sheet_idx];
        for(int i = 0; i < PALETTE_SIZE; i++) {
            unsigned short color = original_pal[i];
            int r = (color >> 10) & 31;
            int g = (color >> 5) & 31;
            int b = color & 31;
            r = 31 - r;
            g = 31 - g;
            b = 31 - b;
            reversed_pals[sheet_idx][i] = (b << 10) | (g << 5) | r;
        }
        *out_pal = reversed_pals[sheet_idx];*/
        return true; 
    }
    
    return false;
}