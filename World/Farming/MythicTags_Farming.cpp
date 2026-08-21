#include "MythicTags_Farming.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Farming_Stage_Empty, "Farming.Stage.Empty", "Farm plot is empty — accepts a seed");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Farming_Stage_Growing, "Farming.Stage.Growing", "Crop is planted and still growing");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Farming_Stage_Mature, "Farming.Stage.Mature", "Crop is fully grown — ready to harvest");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Farming_Stage_Withered, "Farming.Stage.Withered", "Crop died of drought — harvest yields compost feedstock, never nothing (C6)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Item_Seed, "Item.Seed", "Root tag for plantable seed items (leaves e.g. Item.Seed.Wheat)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Item_Fertilizer, "Item.Fertilizer", "Root tag for fertilizer items (leaves e.g. Item.Fertilizer.Compost / .Bonemeal) — applied by the plot tend verb");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Item_Livestock, "Item.Livestock", "Root tag for livestock items (leaves e.g. Item.Livestock.Chicken) — vendor-bought starter stock consumed by the animal pen");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Item_Feed, "Item.Feed", "Root tag for animal feed items (leaves e.g. Item.Feed.Grain) — consumed by the pen feed verb; def-level quality tag drives produce tier");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Crop_Type, "Crop.Type", "Root tag for crop identity (leaves e.g. Crop.Type.Wheat) — pollination-diversity honey counts distinct leaves");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Influence_Irrigation, "Influence.Irrigation", "P4 role: irrigation well/sprinkler coverage — plots inside auto-refill moisture at sample time");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Influence_Pollination, "Influence.Pollination", "P4 role: bee-hive pollination coverage — crop quality input + extra-yield roll");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Influence_Deterrence, "Influence.Deterrence", "P4 role: scarecrow deterrence — raises the farm-raid pressure threshold (habituates)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Farming_GraveEssence, "Farming.GraveEssence", "Generic Gravebloom essence stamped by burying an identity-less corpse at a plot");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_Farming_Harvests, "Stat.Farming.Harvests", "Lifetime crops harvested (unlock rules are content)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_Farming_Graveblooms, "Stat.Farming.Graveblooms", "Lifetime corpses buried at plots (Gravebloom)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_Beekeeping_HoneyCollected, "Stat.Beekeeping.HoneyCollected", "Lifetime honey units collected from hives");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_Husbandry_ProduceCollected, "Stat.Husbandry.ProduceCollected", "Lifetime livestock produce units collected from pens");
