#include "SurvivalTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Starving, "Status.Starving", "Nourishment critically low — survival debuff");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_WellFed, "Status.WellFed", "Nourishment high — survival buff");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Dehydrated, "Status.Dehydrated", "Hydration critically low — survival debuff");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Cold, "Status.Cold", "Warmth critically low — survival debuff");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Warm, "Status.Warm", "Near a warm source (campfire aura) — warms + suppresses cold hazards");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Sheltered, "Status.Sheltered", "Indoors/under cover — blocks wetting + suppresses weather hazards");
