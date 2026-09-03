
#include "GAS/Feedback/MythicTags_FeedbackCues.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Damage_Hit, "GameplayCue.Damage.Hit", "A canonical damage application landed on a target");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Combat_KillConfirm, "GameplayCue.Combat.KillConfirm", "A credited external kill landed (fired on the killer's ASC)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Combat_Crit, "GameplayCue.Combat.Crit", "A critical hit landed (fired on the attacker's ASC)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Combat_Reaction, "GameplayCue.Combat.Reaction", "An elemental status reaction fired (fired on the target's ASC)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Combat_Resisted, "GameplayCue.Combat.Resisted", "An intended status was resisted (fired on the target's ASC)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Combat_Immune, "GameplayCue.Combat.Immune", "A hard-CC was silently eaten by immunity (fired on the target's ASC)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Status_Burn_Onset, "GameplayCue.Status.Burn.Onset", "Burn status crossed threshold and applied");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Status_Bleed_Onset, "GameplayCue.Status.Bleed.Onset", "Bleed status crossed threshold and applied");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Status_Poison_Onset, "GameplayCue.Status.Poison.Onset", "Poison status crossed threshold and applied");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Status_Slow_Onset, "GameplayCue.Status.Slow.Onset", "Slow/chill status crossed threshold and applied");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Status_Freeze_Onset, "GameplayCue.Status.Freeze.Onset", "Freeze status crossed threshold and applied");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Status_Stun_Onset, "GameplayCue.Status.Stun.Onset", "Stun status crossed threshold and applied");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Status_Weaken_Onset, "GameplayCue.Status.Weaken.Onset", "Weaken status crossed threshold and applied");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Status_Terrify_Onset, "GameplayCue.Status.Terrify.Onset", "Terrify status crossed threshold and applied");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_World_POIDiscovered, "GameplayCue.World.POIDiscovered", "A landmark POI was discovered/unlocked");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_World_TreasureUnearthed, "GameplayCue.World.TreasureUnearthed", "A buried dig-site reward was unearthed");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_World_FastTravel_Depart, "GameplayCue.World.FastTravel.Depart", "Fast-travel teleport departs the source anchor");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_World_FastTravel_Arrive, "GameplayCue.World.FastTravel.Arrive", "Fast-travel teleport arrives at the destination anchor");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_World_EnteringDanger, "GameplayCue.World.EnteringDanger", "The owning player crossed into a region of HIGHER danger tier (fired on their ASC)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Mount_TameSuccess, "GameplayCue.Mount.TameSuccess", "A wild creature was successfully tamed into a mount");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Mount_Summon, "GameplayCue.Mount.Summon", "A mount was whistle-summoned into the world");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Necro_Raise, "GameplayCue.Necro.Raise", "A minion was raised from a corpse");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Affix_Proc, "GameplayCue.Affix.Proc", "A conditional trigger-affix (proc-mod) fired (on the source's ASC)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_READY, "GameplayCue.Rune.Ready", "A rune's empowered moment became ready (owner-only)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_ACTIVATE, "GameplayCue.Rune.Activate", "A rune fired its rule change (on the owner's ASC)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_CHEATDEATH, "GameplayCue.Rune.CheatDeath", "A rune intercepted a lethal blow and the owner survived (on the owner's ASC)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_FIRSTSTRIKE, "GameplayCue.Rune.FirstStrike", "A first-strike rune empowered an outgoing hit (on the attacker's ASC)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_FEATHERFALL, "GameplayCue.Rune.Featherfall", "A landing whose fall damage a rune prevented (multicast at the avatar)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_PARRY, "GameplayCue.Rune.Parry", "A blow was parried on the beat (multicast at the impact point)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_SLAM, "GameplayCue.Rune.Slam", "A heavy landing slammed the ground (multicast at the landing point)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_PLANT, "GameplayCue.Rune.Plant", "The owner planted after standing still (multicast at the avatar)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_THREAD, "GameplayCue.Rune.Thread", "An Evade threaded past an enemy (multicast along the dash)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_WHIFF, "GameplayCue.Rune.Whiff", "An Evade passed no one (owner-only)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_GAMBIT, "GameplayCue.Rune.Gambit", "An empty purse boosted a kill's loot roll (multicast at the corpse)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_MEND, "GameplayCue.Rune.Mend", "A kill mended worn gear instead of dropping loot (owner-only, throttled)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_EMBER, "GameplayCue.Rune.Ember", "The ember lit: wounds are owed, not dealt (multicast at the avatar)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_EMBEROUT, "GameplayCue.Rune.EmberOut", "The ember went out and any owed wounds landed (multicast at the avatar)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_WAGERBANK, "GameplayCue.Rune.WagerBank", "A staked purse was banked when the fight ended (multicast at the avatar)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_WAGERLOST, "GameplayCue.Rune.WagerLost", "A landed hit destroyed the stake (owner-only)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAMEPLAYCUE_RUNE_IRONBARK, "GameplayCue.Rune.Ironbark", "A harvest swing was paid for in health (multicast at the node)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Reaction_Combo_Assist, "GameplayCue.Reaction.Combo.Assist", "A co-op cross-player reaction detonated (fired on the detonating player's ASC)");
