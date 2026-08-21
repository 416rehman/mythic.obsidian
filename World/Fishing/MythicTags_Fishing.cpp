
#include "World/Fishing/MythicTags_Fishing.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_FieldActivity_Fishing, "FieldActivity.Fishing");
UE_DEFINE_GAMEPLAY_TAG(TAG_FieldActivity_Drink, "FieldActivity.Drink");
UE_DEFINE_GAMEPLAY_TAG(TAG_Surface_Water, "Surface.Water");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_FieldActivity_Fishing_Input_Hook, "FieldActivity.Fishing.Input.Hook",
                               "Client intent: set the hook (scored against the bite window server-side)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_FieldActivity_Fishing_Input_Pull, "FieldActivity.Fishing.Input.Pull",
                               "Client intent: pull the line (reels between surges; breaks the line during one)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_Bite, "GameplayCue.Fishing.Bite", "Bite window opened - hook now!");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_Hooked, "GameplayCue.Fishing.Hooked", "Hook set - the FIGHT begins");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_Whiff, "GameplayCue.Fishing.Whiff", "Bite missed / jumped the gun - cost-free whiff");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_Surge_Start, "GameplayCue.Fishing.Surge.Start", "The fish surges - DON'T pull");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_Surge_End, "GameplayCue.Fishing.Surge.End", "Surge over - reel!");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_Reel, "GameplayCue.Fishing.Reel", "A good pull - reel progress");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_LineBreak, "GameplayCue.Fishing.LineBreak", "Pulled during a surge - line snapped, bait lost");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_Escape, "GameplayCue.Fishing.Escape", "Fight timed out - the fish escaped, bait lost");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_Landed, "GameplayCue.Fishing.Landed", "Catch landed");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Fishing_Record, "GameplayCue.Fishing.Record", "New personal size record");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Bait, "Bait", "Bait tag family root - a UBaitFragment's tags live under here");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Bait_Worm, "Bait.Worm", "Worm bait (L6 crop-byproduct recipe - CONTENT)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Bait_Grub, "Bait.Grub", "Grub bait (L6 crop-byproduct recipe - CONTENT)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Bait_Offal, "Bait.Offal", "Offal bait (skinning-yield recipe - CONTENT)");
