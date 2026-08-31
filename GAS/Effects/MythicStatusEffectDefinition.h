
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Itemization/MythicDataAsset.h"
#include "Templates/SubclassOf.h"
#include "MythicStatusEffectDefinition.generated.h"

class UGameplayEffect;
class UTexture2D;

/** How a control magnitude changes its consumer's baseline multiplier. */
UENUM(BlueprintType)
enum class EMythicStatusControlOperation : uint8 {
    /** Multiplies downward, such as Slow or Weaken. */
    Reduction,

    /** Multiplies upward, such as Terrify's damage-taken amplification. */
    Bonus,
};

/** Controls whether an active status may leave the private combat model and enter world presentation. */
UENUM(BlueprintType)
enum class EMythicStatusWorldVisibility : uint8 {
    /** Never expose this status above an entity; it may still appear in owner-only character UI. */
    Hidden,

    /** Expose the status only on the viewer's stable focus/current-target presentation. */
    FocusOnly,

    /** Expose the status after the subject has independently earned a contextual plate. */
    Contextual,

    /** Expose the status on an earned plate and allow it to create a safety context when observed. */
    SafetyCritical,
};

/** Semantic bucket used to rank a bounded set of status badges without inspecting GameplayEffect classes. */
UENUM(BlueprintType)
enum class EMythicStatusPresentationCategory : uint8 {
    /** Hard loss of control such as stun, freeze, or knockdown. */
    HardControl,

    /** Ongoing damaging condition such as burn, bleed, or poison. */
    Damage,

    /** Soft control or movement restriction such as slow or root. */
    Control,

    /** Harmful non-control modifier such as weaken or vulnerability. */
    Debuff,

    /** Beneficial or protective state whose visibility is explicitly authored. */
    Buff,

    /** Low-priority cosmetic or world-readable condition. */
    Cosmetic,
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatusReaction {
    GENERATED_BODY()

    /** Tag that must already be present on the target when this status lands for the reaction to fire. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    FGameplayTag RequiredTargetTag;

    /** Gameplay event broadcast on the target; leave unset for a purely cosmetic reaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    FGameplayTag ReactionEventTag;

    /** Gameplay cue played on the target when the reaction fires. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction", meta = (Categories = "GameplayCue"))
    FGameplayTag ReactionCueTag;

    /** Whether the reaction removes effects granting RequiredTargetTag after combining the statuses. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    bool bConsumeExistingStatus = true;

    /** Whether the reaction replaces this incoming status instead of allowing both statuses to remain. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    bool bSuppressStatusApplication = true;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicStatusEffectDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    /** Canonical Status.Type.* identity requested by abilities, weapons, procs, UI, and tooling. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Identity", meta = (Categories = "Status.Type"))
    FGameplayTag StatusType;

    /** State tag granted while active; drives UI, AI, reactions, and conditional damage rules. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Identity")
    FGameplayTag GrantedStateTag;

    /** GameplayEffect applied after buildup crosses the threshold or a direct application succeeds. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    TSubclassOf<UGameplayEffect> EffectToApply;

    /** Attribute that accumulates buildup until the effective threshold applies this status. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute BuildupAttribute;

    /** Resistance attribute that raises required buildup and gates the status-application roll. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute ResistanceAttribute;

    /**
     * Damage each tick contributed by one application. Aggregate-by-source statuses snapshot and sum each roll into
     * one bounded per-source tick magnitude; a capped reapplication can refresh duration without rewriting prior
     * rolls. The GameplayEffect must read SetByCaller.Status.Damage with stack-count factoring disabled.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FRollDefinition DamagePerTick;

    /**
     * Seconds the status lasts, rolled per application. Leave Min and Max at zero to keep whatever duration the
     * effect authors itself.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FRollDefinition DurationSeconds;

    /**
     * Strength contributed by one application - the slow's bite, the weaken's penalty, the terrify's damage bump -
     * as a 0..1 fraction. Every roll is snapshotted into one equivalent multiplicative aggregate, so UE replacing a
     * stacked spec cannot rewrite earlier rolls. ControlOperation chooses whether the product moves below or above
     * 1.0. Leave Min and Max at zero to fall back to the effect's own authored constant.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FRollDefinition ControlMagnitude;

    /** Whether stacked ControlMagnitude rolls combine below or above the consumer's 1.0 baseline. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    EMythicStatusControlOperation ControlOperation = EMythicStatusControlOperation::Reduction;

    /**
     * Applier stat added on top of Power-scaled base damage, named by data exactly as BuildupAttribute and
     * ResistanceAttribute are. This is a Bonus stat, so it is a fraction: 0.4 means +40%. Leave it unset on a
     * status with no damage band - a stat pointed at a band of zero would validate green and do nothing.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute BonusDamageAttribute;

    /** Applier multiplier attribute that scales DurationSeconds; unset leaves the authored band unchanged. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute DurationMultiplierAttribute;

    /**
     * Applier bonus attribute that scales ControlMagnitude after diminishing rules; unset leaves the authored band
     * unchanged, and a status with no control band ignores it.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute ControlMagnitudeAttribute;

    /** Whether this status obeys hard-CC immunity and diminishing-return escalation rules. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    bool bHardCrowdControl = false;

    /** Gameplay cue fired on the target at status onset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Feedback", meta = (Categories = "GameplayCue"))
    FGameplayTag OnsetCueTag;

    /** Ordered reactions against statuses already on the target; the first matching entry wins. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Reactions")
    TArray<FMythicStatusReaction> Reactions;

    /** Player-facing localized name shown on badges, combat teaching, and tooltips. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Presentation")
    FText DisplayName;

    /** Player-facing localized description; rich-text markup is supported. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Presentation", meta = (MultiLine = true))
    FText Description;

    /** Soft badge icon used by status UI without forcing eager texture loading. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Presentation")
    TSoftObjectPtr<UTexture2D> Icon;

    /** Canonical tint used by badges, periodic damage numbers, and status-aware cue presentation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Presentation")
    FLinearColor DisplayColor = FLinearColor::White;

    /**
     * Defines whether this status may be replicated in a subject's bounded public world-status projection. Hidden
     * never leaves authority/owner UI; other values are still filtered by nameplate tier and viewer policy.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|World Presentation")
    EMythicStatusWorldVisibility WorldVisibility = EMythicStatusWorldVisibility::Contextual;

    /**
     * Semantic ranking bucket for overhead badges. The authority projection copies this authored classification;
     * clients never infer it from an effect class, display string, or tag spelling.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|World Presentation")
    EMythicStatusPresentationCategory PresentationCategory = EMythicStatusPresentationCategory::Debuff;

    /**
     * Deterministic tie-break priority within PresentationCategory; larger values win. Values are unitless and are
     * clamped by consumers to avoid content data creating unbounded scores.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|World Presentation", meta = (ClampMin = "-1000", ClampMax = "1000"))
    int32 WorldPresentationPriority = 0;

    /**
     * Allows an observed active status to earn contextual presentation when its visibility is SafetyCritical.
     * Authority derives the fact from the canonical active status; designers never mirror it into a UI flag.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|World Presentation")
    bool bPromotesContextWhenObserved = false;

    /** Whether Focus may show a quantized remaining-time label when the authority projection has a known deadline. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|World Presentation")
    bool bShowRemainingDuration = true;

    /** Whether Focus may show a bounded public stack count; false hides stacks even if gameplay internally stacks. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|World Presentation")
    bool bShowStackCount = true;
};
