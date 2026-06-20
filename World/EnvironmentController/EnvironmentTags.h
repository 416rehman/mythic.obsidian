#pragma once
#include "NativeGameplayTags.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Weather);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Weather_Clear);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Weather_Overcast);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Weather_Rain);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Weather_Snow);

// Time-of-day tags mirror the EDayTime enum (Morning/Afternoon/Evening/Night). Surfaced as world-state tags so
// systems like the EncounterDirector can gate on the clock (e.g. a night-only ambush) via FGameplayTagQuery.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Time);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Time_Morning);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Time_Afternoon);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Time_Evening);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Time_Night);

// Season tags mirror the ESeason enum (Spring/Summer/Autumn/Winter). Same world-state-gating purpose.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Season);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Season_Spring);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Season_Summer);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Season_Autumn);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Environment_Season_Winter);
