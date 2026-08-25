#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Whether a player may recruit an NPC. Pure, so both gates are testable without a world, a party subsystem or a
 * Mass entity - which is what let them go unenforced: the only place they could be checked was a server RPC
 * nothing could reach from a test.
 */
struct MYTHIC_API FMythicRecruitRules {
    /** Every tag the NPC demands must be one the player owns. An empty requirement asks nothing. */
    static bool MeetsTagGate(const FGameplayTagContainer &PlayerTags, const FGameplayTagContainer &RequiredTags) {
        if (RequiredTags.IsEmpty()) {
            return true;
        }
        return PlayerTags.HasAll(RequiredTags);
    }

    /** Standing and threshold share the [-100, 100] scale the standing component clamps to. */
    static bool MeetsStandingGate(float Standing, float Threshold) {
        return Standing >= Threshold;
    }
};
