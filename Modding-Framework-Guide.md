# Guide: Implementing New Jokers in GBAlatro

This document outlines the standard procedure for adding new Jokers to the game engine. Because this project supports both base-game (Vanilla) recreations and completely custom (Modded) content, the architecture is split into two distinct pipelines to prevent merge conflicts and preserve modularity.

## 1. Understanding Joker IDs

Every Joker in the game requires a unique numerical ID. To prevent custom cards from overwriting base-game cards during compilation, the engine strictly separates the ID pools:

* **Vanilla IDs (0 - 99):** Reserved strictly for replicating the original Balatro roster.
* **Modded IDs (100+):** Reserved for custom, community-created, or expansion Jokers.

Always verify the next available ID in your respective registry before initializing a new card.

---

## 2. Adding a Modded Card (Custom Content)

If you are designing a brand-new card that does not exist in the original game, follow this pipeline to keep your code cleanly isolated.

### Step 2.1: Adding Visuals

File: `source/modded_jokers_gfx.c`

1. **Include the Spritesheet:** Once you have exported your Game Boy Advance formatted image (usually via Grit), include the generated header or source file at the top of the file.
2. **Update the Arrays:** Immediately below your include statements, you must register the specific Tile and Palette data so the engine can load it into Video RAM.
* Add your new `...Tiles` variable to the `modded_jokers_tiles` array.
* Add your new `...Pal` variable to the `modded_jokers_pals` array.
* *Note: Ensure the order matches the ID sequence established in the registry.*



### Step 2.2: Implementing Card Logic

File: `source/modded_joker_effects.c`

1. **Write the Ability Function:** Create a `static u32` function for your Joker. This function receives the current `JokerEvent` (e.g., `JOKER_EVENT_INDEPENDENT`, `JOKER_EVENT_ON_HAND_SCORED_END`) and a pointer to the `JokerEffect` struct to modify scores, multipliers, or trigger unique behaviors.
2. **Register the Card:** Scroll to the bottom of the file and locate the `const JokerInfo modded_joker_registry[]` array. Add a new entry for your Joker here. You will define its rarity, base cost, and link the ability function you just created.

---

## 3. Adding a Vanilla Card (Base Game Content)

If you are recreating a card from the original Steam version of Balatro, you will integrate it directly into the core framework rather than the modded isolation files.

### Step 3.1: Adding Visuals

Files: `include/joker_gfx.h` and `include/def_joker_gfx_table.h`

1. Include the generated tile and palette references in `joker_gfx.h`.
2. Add the corresponding entries to the macro table in `def_joker_gfx_table.h` to ensure the core game compiles the sprites correctly.

### Step 3.2: Implementing Card Logic

File: `source/joker_effects.c`

1. Write the ability function mimicking the original card's behavior.
2. Add the card's metadata (rarity, cost, function reference) to the main `const JokerInfo joker_registry[]` array.

---

## 4. Testing: Adding to the Debug Menu

File: `source/debug.c`

To ensure you can easily test your new card without relying on random shop generation, you must add it to the debug spawner.

1. Open `debug.c` and locate the Joker registry arrays used for the debug menu.
2. Add your new Joker ID and a shortened string name to the list. This will allow you to instantly spawn the card into your hand via the in-game debug overlay.

---

## 5. Global Engine Changes vs. Card Logic

When developing complex mechanics, it is critical to know where your code belongs:

* **Card-Specific Rules:** Any logic that only occurs when a specific card is owned (e.g., granting +10 Mult, destroying another card, or modifying shop prices) belongs in either `modded_joker_effects.c` or `joker_effects.c`.
* **Core Engine Systems:** If you are changing how the fundamental game operates—such as altering how many cards can be held, modifying the mathematical limits of a Poker Hand, creating custom Boss Blind behaviors, or tracking global game history—these systemic changes must be made directly inside `source/game.c`.