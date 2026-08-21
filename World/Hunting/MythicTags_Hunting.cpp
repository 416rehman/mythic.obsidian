
#include "World/Hunting/MythicTags_Hunting.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_FieldActivity_ReadTracks, "FieldActivity.ReadTracks",
                               "GameplayEvent: channel-read a spoor trail node (UMythicGA_ReadTracks)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Hunting_Spoor_Revealed, "GameplayCue.Hunting.Spoor.Revealed",
                               "The next trail node revealed - follow the sign");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Hunting_Spoor_Faded, "GameplayCue.Hunting.Spoor.Faded",
                               "The trail went cold (stale/washed out)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Hunting_Spoor_TrailEnd, "GameplayCue.Hunting.Spoor.TrailEnd",
                               "The final node - the quarry's site is here");
