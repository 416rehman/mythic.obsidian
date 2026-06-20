// Mythic Living World — Scheme Types
// Data types for the background-thread scheme engine.
// Schemes are faction-level plots that progress over time on the simulation thread.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "SchemeTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythScheme, Log, All);

// ─────────────────────────────────────────────────────────────
// Scheme Types
// ─────────────────────────────────────────────────────────────

/**
 * Categories of faction schemes.
 * Each type has different generation prerequisites, progress rates, and execution effects.
 */
UENUM(BlueprintType)
enum class EMythicSchemeType : uint8 {
    /** Target a specific high-significance NPC in enemy faction */
    Assassination UMETA(DisplayName = "Assassination"),

    /** Disrupt trade routes — reduces target faction's trade income */
    TradeDisruption UMETA(DisplayName = "TradeDisruption"),

    /** Reclaim lost territory — military operation against specific cells */
    TerritoryReclaim UMETA(DisplayName = "TerritoryReclaim"),

    /** Place a spy — dual-identity NPC in target faction territory */
    SpyInfiltration UMETA(DisplayName = "SpyInfiltration"),

    /** Recruit a player companion — attempt to turn a party member */
    CompanionRecruitment UMETA(DisplayName = "CompanionRecruitment"),

    /** Military raid — direct assault on target faction territory */
    MilitaryRaid UMETA(DisplayName = "MilitaryRaid"),

    /** Diplomatic pressure — attempt to shift target faction's allies */
    DiplomaticPressure UMETA(DisplayName = "DiplomaticPressure"),

    COUNT UMETA(Hidden)
};

static constexpr int32 SchemeTypeCount = static_cast<int32>(EMythicSchemeType::COUNT);

// ─────────────────────────────────────────────────────────────
// Scheme State
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicSchemeState : uint8 {
    /** Being planned — not yet active */
    Planning UMETA(DisplayName = "Planning"),

    /** Actively progressing */
    InProgress UMETA(DisplayName = "InProgress"),

    /** Successfully completed — effects applied */
    Succeeded UMETA(DisplayName = "Succeeded"),

    /** Failed — detected, countered, or timed out */
    Failed UMETA(DisplayName = "Failed"),

    /** Discovered by target — may escalate or be countered */
    Discovered UMETA(DisplayName = "Discovered")
};

// ─────────────────────────────────────────────────────────────
// Scheme Data
// ─────────────────────────────────────────────────────────────

/**
 * A single faction scheme. Pure data — no UObject overhead.
 * Tracked and progressed on the background sim thread.
 *
 * ~48 bytes per scheme.
 */
USTRUCT()
struct FMythicScheme {
    GENERATED_BODY()

    /** Unique scheme ID (monotonically increasing) */
    uint32 SchemeId = 0;

    /** Faction originating the scheme */
    FMythicFactionId OriginFaction;

    /** Faction being targeted */
    FMythicFactionId TargetFaction;

    /** Type of scheme */
    EMythicSchemeType Type = EMythicSchemeType::Assassination;

    /** Current state */
    EMythicSchemeState State = EMythicSchemeState::Planning;

    /** Progress toward completion [0.0, 1.0]. 1.0 = executes. */
    float Progress = 0.0f;

    /**
     * Per-tick progress increment. Scaled by faction military strength and resources.
     * Higher = scheme completes faster.
     */
    float ProgressRate = 0.01f;

    /**
     * Per-tick detection chance [0.0, 1.0].
     * Rolled each sim tick. Affected by target faction's Cunning/intelligence.
     */
    float DetectionRisk = 0.05f;

    /** World time when the scheme was created */
    double StartGameTime = 0.0;

    /**
     * Target cell for territorial schemes (TerritoryReclaim, MilitaryRaid).
     * Invalid for non-territorial schemes.
     */
    FMythicCellCoord TargetCell;

    /** Whether the scheme has been discovered by the target faction */
    bool IsDiscovered() const { return State == EMythicSchemeState::Discovered; }

    /** Whether the scheme is still active (can be progressed) */
    bool IsActive() const {
        return State == EMythicSchemeState::Planning || State == EMythicSchemeState::InProgress;
    }
};
