#pragma once
#include "NativeGameplayTags.h"

/* ----------------------------- STATION ---------------------------- */
/* Tags identifying the kind of a conversion station. Recipes filter against these
 * via their StationTagQuery, and a station enforces them via MatchesStation(). */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_STATION_CRAFTINGBENCH);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_STATION_FORGE);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_STATION_FURNACE);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_STATION_FIREPLACE);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_STATION_ALCHEMYTABLE);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_STATION_COOKINGPOT);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_STATION_SMELTER);

/* ----------------------------- SCHEMATIC ---------------------------- */
/* Root for instigator-gate tags (granted by learning a schematic/recipe item, or by a
 * proficiency). Concrete child tags are data-defined (and may be mod-defined). */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_SCHEMATIC);

/* ----------------------------- PROCESS ---------------------------- */
/* Categorizes the kind of transformation a recipe performs - used for UI grouping. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_PROCESS_CRAFT);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_PROCESS_SMELT);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_PROCESS_COOK);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_PROCESS_BREW);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_PROCESS_BURN);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(ITEMIZATION_PROCESS_REFINE);

/* ----------------------------- INVENTORY GROUPS ---------------------------- */
/* Default group-tag keys a station's UInventoryProfile uses to mark Input/Fuel/
 * Catalyst/Output slot groups. The component reads from these groups by tag. */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(INVENTORY_GROUP_STATION_INPUT);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(INVENTORY_GROUP_STATION_FUEL);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(INVENTORY_GROUP_STATION_CATALYST);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(INVENTORY_GROUP_STATION_OUTPUT);
