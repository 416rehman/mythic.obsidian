#pragma once

#include "CoreMinimal.h"
#include "MythicCombatThreatAssessment.generated.h"

/** Viewer-relative, presentation-safe combat pressure band; raw pressure values never cross this boundary. */
UENUM(BlueprintType)
enum class EMythicThreatBand : uint8 {
    /** The viewer is not permitted to assess this subject or the assessment inputs are not yet trustworthy. */
    Unknown,

    /** No exceptional threat warning is warranted. */
    None,

    /** The subject exerts at least 1.35 times the viewer's effective combat pressure by default. */
    Risky,

    /** The subject exerts at least 2.25 times the viewer's effective combat pressure by default. */
    Deadly,

    /** The subject exerts at least 4 times the viewer's pressure or is known to be unhittable. */
    Overwhelming,
};

/** Public or learned combat rank used only to apply a minimum warning when that rank is known to the viewer. */
UENUM(BlueprintType)
enum class EMythicCombatThreatRank : uint8 {
    /** A subject known not to be combat-capable. */
    NonCombatant,

    /** An ordinary combatant with no rank-based warning floor. */
    Standard,

    /** An elite combatant that is never presented below Risky while its rank is known. */
    Elite,

    /** A boss combatant that is never presented below Deadly while its rank is known. */
    Boss,

    /** A world-scale boss that is always presented as Overwhelming while its rank is known. */
    WorldBoss,
};

/** Monotonic pressure-ratio boundaries owned by combat rather than duplicated in nameplate presentation code. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCombatThreatThresholds {
    GENERATED_BODY()

    /** Subject-to-viewer pressure ratio at which Risky begins. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Threat", meta = (ClampMin = "1.0"))
    float RiskyPressureRatio = 1.35f;

    /** Subject-to-viewer pressure ratio at which Deadly begins. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Threat", meta = (ClampMin = "1.0"))
    float DeadlyPressureRatio = 2.25f;

    /** Subject-to-viewer pressure ratio at which Overwhelming begins. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Threat", meta = (ClampMin = "1.0"))
    float OverwhelmingPressureRatio = 4.0f;
};

/**
 * Ephemeral inputs for one authority-approved viewer/subject assessment. Effective pressure values are computation
 * inputs only: they must not be copied into replicated presentation snapshots; only EMythicThreatBand may leave the
 * assessment boundary.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCombatThreatAssessmentInputs {
    GENERATED_BODY()

    /** Whether knowledge, visibility, and game-mode policy allow this viewer to assess the subject. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat")
    bool bAssessmentPermitted = false;

    /** Whether the subject can perform meaningful combat actions. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat")
    bool bCombatCapable = false;

    /** Whether the subject currently accepts damage from this viewer. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat")
    bool bDamageable = true;

    /** Whether the viewer has learned or observed the subject's current damageability. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat")
    bool bDamageabilityKnownToViewer = false;

    /** Whether the subject is currently immune to every damage channel available to this viewer. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat")
    bool bImmuneToViewerDamage = false;

    /** Whether the viewer has learned or observed the subject's immunity. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat")
    bool bImmunityKnownToViewer = false;

    /** Combat rank resolved by the canonical encounter or combat profile. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat")
    EMythicCombatThreatRank Rank = EMythicCombatThreatRank::Standard;

    /** Whether Rank is public or learned for this viewer and may therefore apply a warning floor. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat")
    bool bRankKnownToViewer = false;

    /** Viewer effective pressure computed transiently by combat from current offensive and defensive capability. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat", meta = (ClampMin = "0.0"))
    double ViewerEffectivePressure = 0.0;

    /** Subject effective pressure computed transiently by combat from current offensive and defensive capability. */
    UPROPERTY(BlueprintReadWrite, Category = "Combat|Threat", meta = (ClampMin = "0.0"))
    double SubjectEffectivePressure = 0.0;
};

/** Pure combat-owned threat classifier that emits only presentation-safe categorical output. */
struct MYTHIC_API FMythicCombatThreatAssessment {
    /** Returns the minimum warning imposed by a viewer-known combat rank. */
    static EMythicThreatBand RankFloor(EMythicCombatThreatRank Rank, bool bRankKnownToViewer);

    /** Resolves permission, capability, known immunity/damageability, rank, and pressure into one safe band. */
    static EMythicThreatBand Assess(const FMythicCombatThreatAssessmentInputs &Inputs,
                                    const FMythicCombatThreatThresholds &Thresholds = {});
};
