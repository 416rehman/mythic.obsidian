
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
};
