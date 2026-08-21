
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Containers/ArrayView.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "MythicBountyRules.generated.h"

USTRUCT(BlueprintType)
struct FMythicBountyConfig {
    GENERATED_BODY()

    /** ASCENDING notoriety thresholds, one per wanted tier (tier 0 at/above [0], tier 1 at/above [1], ...). Notoriety =
     *  the player's WORST faction standing magnitude (a Condemn crime ≈ 15, a faction-member kill ≈ 25). Below the first
     *  threshold there is no bounty. Defaults are HIGH on purpose (near-inert): ~tier 0 at a sustained rap sheet, not a
     *  bar brawl. Empty = no tier ever resolves (inert even when the master switch is on). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "0.0"))
    TArray<float> TierThresholds = {150.0f, 300.0f, 500.0f};

    /** Minimum seconds between hunter dispatches against the SAME player (armed when a pack actually dispatches). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "0.0"))
    float CooldownSeconds = 900.0f;

    /** Max simultaneously-live hunters per hunted player. A player already at cap is never re-dispatched. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "1"))
    int32 MaxSimultaneousHunters = 4;

    /** Chance [0,1] a qualifying check actually telegraphs a hunt (LOW by default — hunters are an event, not a faucet).
     *  Boundary rule shared with the combat rolls: <= 0 NEVER dispatches, >= 1 always does. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpawnChancePerCheck = 0.15f;

    /** Seconds between the TELEGRAPH beat ("hunters are asking about you") and the actual spawn — the player always gets
     *  a real warning window. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "0.0"))
    float TelegraphDelaySeconds = 45.0f;

    /** Seconds between bounty checks (ONE repeating server timer — never Tick; latency, not a frame budget). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "5.0"))
    float CheckIntervalSeconds = 30.0f;

    /** Hunters in a tier-0 pack. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "1"))
    int32 BaseHunters = 1;

    /** Extra hunters added per wanted tier above 0 (tier 2 pack = BaseHunters + 2×this, capped by MaxSimultaneousHunters). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "0"))
    int32 HuntersPerTier = 1;

    /** NPC-type tag handed to UMythicNPCManager::SpawnRandomNPC for each hunter (the hunter-flavored NPC template —
     *  CONTENT: needs a matching NPC definition; unset/unmatched falls back to the generic NPC the manager spawns). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters")
    FGameplayTag HunterNPCType;

    /** Hunters spawn on a ring near-but-not-on the player: min ring radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "500.0"))
    float MinSpawnDistance = 2200.0f;

    /** Max ring radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (ClampMin = "500.0"))
    float MaxSpawnDistance = 3800.0f;
};


namespace MythicBounty {
inline int32 ResolveBountyTier(float Notoriety, TConstArrayView<float> Thresholds) {
    int32 Tier = -1;
    for (int32 i = 0; i < Thresholds.Num(); ++i) {
        if (Notoriety >= Thresholds[i]) {
            Tier = i;
        }
    }
    return Tier;
}

inline bool ShouldDispatchHunters(int32 Tier, double TimeSinceLast, double CooldownSeconds, int32 LiveHunters,
                                  int32 MaxSimultaneous, float Roll01, float Chance) {
    if (Tier < 0) {
        return false;
    }
    if (TimeSinceLast < CooldownSeconds) {
        return false;
    }
    if (LiveHunters >= MaxSimultaneous) {
        return false;
    }
    return MythicCombat::RollSucceeds(Chance, Roll01);
}

inline int32 HunterCountForTier(int32 Tier, int32 BaseHunters, int32 HuntersPerTier, int32 MaxSimultaneous) {
    if (Tier < 0) {
        return 0;
    }
    const int32 Base = FMath::Max(1, BaseHunters);
    const int32 PerTier = FMath::Max(0, HuntersPerTier);
    const int32 Cap = FMath::Max(1, MaxSimultaneous);
    return FMath::Clamp(Base + Tier * PerTier, 1, Cap);
}
}
