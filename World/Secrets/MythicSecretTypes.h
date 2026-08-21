#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Narrative/MythicStoryCondition.h"
#include "Rewards/RewardBase.h"
#include "MythicSecretTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicSecretTrigger : uint8 {
    Location UMETA(DisplayName = "Location (enter volume)"),
    Interact UMETA(DisplayName = "Interact (pull/examine)"),
};

USTRUCT(BlueprintType)
struct FMythicSecretDef {
    GENERATED_BODY()

    // Stable designer id — used only for logs / identification. Not gameplay state (the FoundTag is the persisted latch).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret")
    FName SecretId;

    // Precondition over the finder's OWNED story tags. Empty (default) = ungated ("anyone standing here finds it"). Author
    // a prior secret's FoundTag in RequireAll to build an ordered CHAIN/ritual; author a quest/faction tag to interleave
    // with the narrative graph. Evaluated by the shared FMythicStoryCondition::Evaluate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret")
    FMythicStoryCondition RequireCondition;

    // OPTIONAL one-shot latch + story flag. When valid: the secret reveals at most once PER PLAYER (persisted — a reload
    // stays found), and the tag becomes an owned story flag other content (secrets/quests/dialogue) can require. Author a
    // child of Secret.Found (e.g. Secret.Found.SunkenIdol) so it reads clearly in the ledger. UNSET = a repeatable find
    // (still session-guarded by the trigger so it doesn't spam on re-entry; see the trigger actors).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret")
    FGameplayTag FoundTag;

    // What the finder receives — the SAME reward struct quests/dig-sites use. Any subset (XP/item/loot/ability/attribute/
    // renown/paragon) or none. Delivered private + at player level, dropped at the reveal location.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret")
    FRewardsToGive Rewards;

    // OPTIONAL codex/glossary term this find discovers (ServerDiscoverTerm — idempotent). A lore secret unlocks a page.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret")
    FGameplayTag LoreTermTag;

    // OPTIONAL story tag stamped on reveal for an ACHIEVEMENT/unlock rule to consume (ServerSetStoryTag). Distinct from
    // FoundTag so a secret can drive an achievement without also being the latch (or vice-versa), though FoundTag alone
    // already feeds the achievement engine via OnStoryTagEarned.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret")
    FGameplayTag AchievementStoryTag;

    // OPTIONAL override for the reveal cue. UNSET = the default GameplayCue.World.SecretRevealed. Firing it is a safe
    // no-op until content authors a matching GameplayCueNotify (VFX/SFX/camera-shake).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret")
    FGameplayTag RevealCueTag;
};

struct FMythicSecretRules {
    static bool CanReveal(bool bAlreadyFound, const FMythicStoryCondition &Cond, const FGameplayTagContainer &OwnedTags) {
        if (bAlreadyFound) {
            return false;
        }
        return FMythicStoryCondition::Evaluate(Cond, OwnedTags);
    }
};
