
#include "World/Camping/MythicTags_Camping.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Status_Rested, "Status.Rested",
                               "Rested after camping/sleeping: XP gain multiplied (read from the GE's SetByCaller magnitude by the XPReward hook).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Camping_RestedXpMult, "Data.Camping.RestedXpMult",
                               "SetByCaller channel on the Rested GE spec: the XP multiplier granted by the camp's comfort tier.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_FieldActivity_Event_Rest, "FieldActivity.Event.Rest",
                               "GameplayEvent that starts the rest channel (UMythicGA_Rest) — requires being at a live camp.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Influence_Shelter, "Influence.Shelter",
                               "Influence-source role: shelter (tents/bedrolls). Qualifying players inside gain Status.Sheltered.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Comfort_Fire, "Comfort.Fire", "Camp comfort category: a lit campfire.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Comfort_Shelter, "Comfort.Shelter", "Camp comfort category: bedroll/tent shelter.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Comfort_Rack, "Comfort.Rack", "Camp comfort category: repair/drying rack utility.");
