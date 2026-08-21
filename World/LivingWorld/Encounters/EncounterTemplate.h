
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/MythicDanger.h"
#include "EncounterTemplate.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythEncounter, Log, All);


UENUM(BlueprintType)
enum class EMythicEncounterState : uint8 {
    Pending UMETA(DisplayName = "Pending"),

    Spawning UMETA(DisplayName = "Spawning"),

    Active UMETA(DisplayName = "Active"),

    Completing UMETA(DisplayName = "Completing"),

    Completed UMETA(DisplayName = "Completed")
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEncounterTemplate {
    GENERATED_BODY()

    /** Unique tag identifying this encounter type */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTag EncounterTag;

    /** Display name for debugging */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FText DisplayName;

    /**
     * World state prerequisites (tag query).
     * Encounter only spawns when the query matches the current world state. The world state is composed of the live
     * Environment.Weather.* , Environment.Time.* , and Environment.Season.* tags (built in
     * EncounterDirector::EvaluateTemplate). An EMPTY query (the default) imposes no constraint.
     * Example: matches Environment.Time.Night AND NOT Environment.Weather.Snow
     *          (a night-only ambush that holds off during snow).
     * NOTE: only the env tags above are currently produced — do NOT author terms that REQUIRE faction/other tags
     * (e.g. an ALL-match on a Faction.* tag) here, as nothing adds them to the container and the query would never match.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTagQuery RequiredWorldState;

    /** Required faction relationship for the encounter (e.g., hostile factions nearby) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    EMythicFactionRelation MinFactionRelation = EMythicFactionRelation::Neutral;

    /** Minimum military strength of the originating faction [0.0, 1.0] */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinMilitaryStrength = 0.0f;

    /** Minimum population of the originating faction */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0"))
    int32 MinPopulation = 0;

    /** Cooldown between activations of this encounter type (game time seconds) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float CooldownSeconds = 300.0f;

    /** Max concurrent instances of this encounter type across the world */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", ClampMax = "10"))
    int32 MaxConcurrentInstances = 1;

    /** Base probability per evaluation tick [0.0, 1.0]. Gated by prerequisites. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseProbability = 0.1f;

    /** Entity count to spawn for this encounter */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", ClampMax = "20"))
    int32 EntityCount = 3;

    /** Max duration before the encounter auto-completes (game time seconds, 0 = infinite) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float MaxDurationSeconds = 600.0f;

    // ── Reputation-Driven Ambient Encounters (INERT-BY-DEFAULT) ──
    /**
     * OPT-IN reputation band this encounter keys off (Reputation.Band.Feared / Reputation.Band.Renowned). When EMPTY
     * (the default), the template ignores party reputation ENTIRELY and behaves byte-identically to today. When set, the
     * EncounterDirector gates + scales this template by whether the co-op party's reputation matches the band: a
     * Feared-band template (bounty-seekers / glory-challengers) fires only for an infamous or hunted party; a
     * Renowned-band template (rivals / help-seekers) fires only for a celebrated party. A party that doesn't match the
     * band gates the template OUT that evaluation (it doesn't spawn). See FMythicReputationEncounterMath.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Reputation.Band"))
    FGameplayTag RequiredReputationBand;

    /**
     * How hard the party's reputation amplifies this template's spawn probability + pack size once RequiredReputationBand
     * matches. 1.0 (the default) means: at full band intensity the effective weight/pack roughly doubles; at the band
     * floor it is barely raised. 0 = matched-but-no-amplification (pure gate). Only read when RequiredReputationBand is
     * set, so the default leaves opted-out templates untouched. Clamped to >= 0 in the math (amplify-only).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float ReputationWeightScale = 1.0f;

    // ── Cargo-heat ambush bias (WAVE O / O5, INERT-BY-DEFAULT) ──
    /**
     * OPT-IN: mark this template as an AMBUSH so it is biased by nearby CARGO HEAT — the risk a player manufactures by
     * hauling valuable goods through dangerous country. When true, the template's spawn probability is multiplied by
     * (1 + max cargo heat near the candidate cell). Heat is 0 below danger tier 2 and 0 without valuable cargo, so the
     * multiplier is exactly 1.0 for anyone not running loaded caravans through bad country — a light traveller is never
     * punished. FALSE (the default) leaves the template completely untouched.
     *
     * This is the "greed manufactures combat content" arrow: profit and danger are the same dial.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bCargoHeatAmbush = false;
};


struct FMythicActiveEncounter {
    uint32 EncounterId = 0;

    FGameplayTag TemplateTag;

    EMythicEncounterState State = EMythicEncounterState::Pending;

    FMythicCellCoord Cell;

    FMythicFactionId OriginFaction;

    double ActivationTime = 0.0;

    float MaxDurationSeconds = 600.0f;

    int32 EntityCount = 0;

    TArray<FMassEntityHandle> SpawnedEntities;

    bool HasTimedOut(double CurrentWorldTime) const {
        if (MaxDurationSeconds <= 0.0f) {
            return false;
        }
        return (CurrentWorldTime - ActivationTime) > static_cast<double>(MaxDurationSeconds);
    }
};


namespace MythicEncounterDefaults {
MYTHIC_API void BuildDefaultTemplates(TArray<FMythicEncounterTemplate>& Out);

MYTHIC_API int32 DangerScaledEntityCount(int32 BaseCount, EMythicDangerTier Tier, int32 MaxEntityCount = 20);
}
