
#include "World/Secrets/MythicTags_Secrets.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Stat_Secrets_Found, "Stat.Secrets.Found",
                               "Lifetime count of secrets/easter-eggs this player has revealed (achievement threshold)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_World_SecretRevealed, "GameplayCue.World.SecretRevealed",
                               "A hidden secret/easter-egg was revealed (fired on the finder's ASC)");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Secret_Found, "Secret.Found",
                               "ROOT/authoring-hint tag only — designers create per-secret FoundTag children under it "
                               "(Secret.Found.<Name>) for the one-shot latch + secret chains. Never fired directly.");
