
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Containers/ArrayView.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "MythicCampsiteCore.generated.h"


USTRUCT(BlueprintType)
struct FMythicComfortSource {
    GENERATED_BODY()

    /** Which comfort category this source belongs to (Comfort.Fire / Comfort.Shelter / Comfort.Rack / K's decor set).
     *  Diminishing returns apply PER CATEGORY — diversity beats duplication. Invalid-tag sources are skipped. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comfort")
    FGameplayTag CategoryTag;

    /** Points this source contributes before diminishing returns. Negative values clamp to 0. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comfort", meta = (ClampMin = "0.0"))
    float Points = 1.0f;

    FMythicComfortSource() = default;
    FMythicComfortSource(const FGameplayTag &InCategory, float InPoints) : CategoryTag(InCategory), Points(InPoints) {}
};

USTRUCT(BlueprintType)
struct FMythicComfortScale {
    GENERATED_BODY()

    /** Within one category, sources are sorted by descending points and the n-th contributes Points × Factor^n
     *  (first full, second halved, ...). 0 = only the best source per category counts; 1 = no diminishing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comfort", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DiminishingFactor = 0.5f;

    /** Cap on any single category's folded contribution. <= 0 = uncapped. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comfort", meta = (ClampMin = "0.0"))
    float PerCategoryCap = 0.0f;

    /** ASCENDING folded-points thresholds; the tier = how many thresholds the folded total has passed (>=), so the
     *  tier range is [0, TierThresholds.Num()]. Camp() authors 3 thresholds (tiers 0–3), Homestead() authors 5 (0–5). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comfort")
    TArray<float> TierThresholds;

    static FMythicComfortScale Camp() {
        FMythicComfortScale S;
        S.DiminishingFactor = 0.5f;
        S.PerCategoryCap = 2.0f;
        S.TierThresholds = {1.0f, 2.0f, 3.0f};
        return S;
    }

    static FMythicComfortScale Homestead() {
        FMythicComfortScale S;
        S.DiminishingFactor = 0.5f;
        S.PerCategoryCap = 4.0f;
        S.TierThresholds = {1.0f, 3.0f, 6.0f, 10.0f, 15.0f};
        return S;
    }
};

USTRUCT(BlueprintType)
struct FMythicRestBonusConfig {
    GENERATED_BODY()

    /** XP multiplier at comfort tier 0 (>= 1 — Rested is positive-only; survival-lite law). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest", meta = (ClampMin = "1.0"))
    float BaseXpMultiplier = 1.05f;

    /** Extra XP multiplier per comfort tier above 0. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest", meta = (ClampMin = "0.0"))
    float XpMultiplierPerTier = 0.05f;

    /** Hard ceiling on the XP multiplier (K's bed raises tier, never breaks this rail). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest", meta = (ClampMin = "1.0"))
    float MaxXpMultiplier = 1.35f;

    /** Rested duration (seconds) at comfort tier 0. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest", meta = (ClampMin = "0.0"))
    float BaseDurationSeconds = 300.0f;

    /** Extra duration (seconds) per comfort tier above 0. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest", meta = (ClampMin = "0.0"))
    float DurationPerTierSeconds = 180.0f;

    /** Hard ceiling on the Rested duration (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rest", meta = (ClampMin = "0.0"))
    float MaxDurationSeconds = 1800.0f;
};

struct FMythicRestBonus {
    float XpMultiplier = 1.0f;
    float DurationSeconds = 0.0f;
};


USTRUCT(BlueprintType)
struct FMythicCampEventConfig {
    GENERATED_BODY()

    /** Seconds between camp-event checks (ONE repeating server timer, armed only while >= 1 camp anchor is live). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "10.0"))
    float CheckIntervalSeconds = 60.0f;

    /** Ambush chance per check at danger tier 1 BEFORE the per-tier ramp (Safe/tier-0 cells NEVER ambush). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AmbushBaseChance = 0.0f;

    /** Extra ambush chance per danger tier (Low=1 .. Extreme=4): greed into danger manufactures combat content. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AmbushChancePerDangerTier = 0.04f;

    /** Hard ceiling on the per-check ambush chance. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AmbushMaxChance = 0.25f;

    /** Hostile camp events fire at NIGHT only (Environment.Time.Night) when true — the classic night-watch beat. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events")
    bool bAmbushNightOnly = true;

    /** Seconds between the TELEGRAPH beat (chronicle line — "something circles the firelight") and the spawn: the
     *  party ALWAYS gets a real warning window (Raid Gates law b). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "0.0"))
    float TelegraphDelaySeconds = 25.0f;

    /** Minimum seconds between events at the SAME camp (armed when an event actually dispatches). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "0.0"))
    float PerCampCooldownSeconds = 900.0f;

    /** A camp only rolls events while at least one player pawn is within this radius (cm) of its fire —
     *  the online/proximity gate (Raid Gates law c). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "500.0"))
    float PlayerNearRadius = 3000.0f;

    /** Ambushers in a danger-tier-1 pack (danger scales it up via MythicEncounterDefaults::DangerScaledEntityCount). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "1"))
    int32 AmbushBaseCount = 2;

    /** Hard cap on ambushers per event. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "1"))
    int32 AmbushMaxCount = 5;

    /** NPC-type tag handed to UMythicNPCManager::SpawnRandomNPC per ambusher (CONTENT — unset = ambushes stay silent,
     *  warned once). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events")
    FGameplayTag AmbushNPCType;

    /** Ambushers spawn on a ring near-but-not-on the camp: min ring radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "500.0"))
    float MinSpawnDistance = 1400.0f;

    /** Max ring radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "500.0"))
    float MaxSpawnDistance = 2600.0f;

    /** FRIENDLY traveling-merchant visitor: chance per check (independent of danger; no night gate — a fire at dusk
     *  draws travelers). 0 disables. Requires MerchantNPCType (CONTENT). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MerchantChancePerCheck = 0.02f;

    /** NPC-type tag for the vendor-flavored visitor (CONTENT — unset = no merchant visits, warned once). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp Events")
    FGameplayTag MerchantNPCType;
};

USTRUCT(BlueprintType)
struct FMythicCampingConfig {
    GENERATED_BODY()

    /** Camp CLUSTER radius (cm): a camp = a campfire anchor + every camp-category placeable within this radius. Also
     *  the party-share radius for the Rested grant (co-op-generous: everyone at the fire benefits). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camping", meta = (ClampMin = "200.0"))
    float CampRadius = 1500.0f;

    /** ANTI-LITTER: max live camp pieces per player; deploying past it collapses that player's OLDEST piece (its
     *  source item is refunded when the piece declares one). <= 0 = unlimited (no tracking cost). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camping", meta = (ClampMin = "0"))
    int32 MaxCampPiecesPerPlayer = 8;

    /** The P2 rest payoff ladder (tier → XP multiplier + duration). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camping")
    FMythicRestBonusConfig RestBonus;

    /** Camp-event tuning (only read while bEnableCampEvents is TRUE). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camping")
    FMythicCampEventConfig Events;
};


namespace MythicCampsite {
inline float FoldComfortPoints(TConstArrayView<FMythicComfortSource> Sources, const FMythicComfortScale &Scale) {
    TMap<FGameplayTag, TArray<float>> ByCategory;
    for (const FMythicComfortSource &Source : Sources) {
        if (!Source.CategoryTag.IsValid()) {
            continue;
        }
        ByCategory.FindOrAdd(Source.CategoryTag).Add(FMath::Max(0.0f, Source.Points));
    }

    const float Factor = FMath::Clamp(Scale.DiminishingFactor, 0.0f, 1.0f);
    float Total = 0.0f;
    for (TPair<FGameplayTag, TArray<float>> &Pair : ByCategory) {
        Pair.Value.Sort(TGreater<float>());
        float CategorySum = 0.0f;
        float Multiplier = 1.0f;
        for (const float Points : Pair.Value) {
            CategorySum += Points * Multiplier;
            Multiplier *= Factor;
        }
        if (Scale.PerCategoryCap > 0.0f) {
            CategorySum = FMath::Min(CategorySum, Scale.PerCategoryCap);
        }
        Total += CategorySum;
    }
    return Total;
}

inline int32 ComputeComfortTier(TConstArrayView<FMythicComfortSource> Sources, const FMythicComfortScale &Scale) {
    const float Points = FoldComfortPoints(Sources, Scale);
    int32 Tier = 0;
    for (int32 i = 0; i < Scale.TierThresholds.Num(); ++i) {
        if (Points >= Scale.TierThresholds[i]) {
            Tier = i + 1;
        }
    }
    return Tier;
}

inline FMythicRestBonus ResolveRestBonus(int32 ComfortTier, const FMythicRestBonusConfig &Config) {
    const int32 Tier = FMath::Max(0, ComfortTier);
    FMythicRestBonus Bonus;
    const float MaxMult = FMath::Max(1.0f, Config.MaxXpMultiplier);
    Bonus.XpMultiplier = FMath::Clamp(Config.BaseXpMultiplier + Tier * FMath::Max(0.0f, Config.XpMultiplierPerTier), 1.0f, MaxMult);
    const float MaxDuration = FMath::Max(0.0f, Config.MaxDurationSeconds);
    Bonus.DurationSeconds =
        FMath::Clamp(Config.BaseDurationSeconds + Tier * FMath::Max(0.0f, Config.DurationPerTierSeconds), 0.0f, MaxDuration);
    return Bonus;
}

inline bool CanFireHostileCampEvent(bool bPacingRestPhase, bool bAnyMemberOnlineNear, bool bTelegraphed, bool bMasterEnabled,
                                    float Roll01, float Chance) {
    if (bPacingRestPhase) {
        return false;
    }
    if (!bAnyMemberOnlineNear) {
        return false;
    }
    if (!bTelegraphed) {
        return false;
    }
    if (!bMasterEnabled) {
        return false;
    }
    return MythicCombat::RollSucceeds(Chance, Roll01);
}

inline float ComputeAmbushChance(int32 DangerTier, const FMythicCampEventConfig &Config) {
    if (DangerTier <= 0) {
        return 0.0f;
    }
    const float Max = FMath::Clamp(Config.AmbushMaxChance, 0.0f, 1.0f);
    return FMath::Clamp(Config.AmbushBaseChance + DangerTier * FMath::Max(0.0f, Config.AmbushChancePerDangerTier), 0.0f, Max);
}

inline float RoadSpeedMultiplier(bool bOnRoad, float ConfiguredRoadMultiplier = 1.0f) {
    return bOnRoad ? FMath::Max(0.1f, ConfiguredRoadMultiplier) : 1.0f;
}
}
