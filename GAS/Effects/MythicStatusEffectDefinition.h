
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

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatusReaction {
    GENERATED_BODY()

    // Reaction fires only when the target already carries this tag as the status lands (e.g. Burn onto Status.State.Poisoned).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    FGameplayTag RequiredTargetTag;

    // Gameplay event broadcast on the target so abilities can react. Leave unset for a purely cosmetic reaction.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    FGameplayTag ReactionEventTag;

    // Cue played on the target when the reaction fires.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction", meta = (Categories = "GameplayCue"))
    FGameplayTag ReactionCueTag;

    // Remove the effects granting RequiredTargetTag: the reaction consumes what it combined with.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    bool bConsumeExistingStatus = true;

    // Skip applying this status because the reaction replaces it. Clear this to have both land.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    bool bSuppressStatusApplication = true;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicStatusEffectDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    // Identity of this status (Status.Type.*). Abilities, weapons and procs request a status by this tag.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Identity", meta = (Categories = "Status.Type"))
    FGameplayTag StatusType;

    // Tag the applied effect grants on the target while it is active (GAS.Debuff.*). Drives UI, AI and damage modifiers.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Identity")
    FGameplayTag GrantedStateTag;

    // Effect applied once buildup crosses the threshold. Author it as a Blueprint GameplayEffect.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    TSubclassOf<UGameplayEffect> EffectToApply;

    // Attribute that accumulates toward this status. Hits add to it; crossing the threshold applies the effect.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute BuildupAttribute;

    // Attribute in 0..1 that resists this status. Raises the buildup needed and gates the on-hit roll.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute ResistanceAttribute;

    /**
     * Damage each tick, rolled per application. Leave Min and Max at zero for a status that deals no damage. The
     * effect must read it through the SetByCaller.Status.Damage tag for this to reach the target.
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
     * Strength of a control status, rolled per application - the slow's bite, the weaken's penalty, the terrify's
     * damage bump - as a 0..1 fraction. This is the third axis, separate from damage and duration, so a slow build
     * can bite harder rather than only last longer. Leave Min and Max at zero to fall back to the effect's own
     * authored constant, so nothing regresses before a band is tuned. Reaches the effect through the
     * SetByCaller.Status.ControlMagnitude tag.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FRollDefinition ControlMagnitude;

    /**
     * Applier stat added on top of Power-scaled base damage, named by data exactly as BuildupAttribute and
     * ResistanceAttribute are. This is a Bonus stat, so it is a fraction: 0.4 means +40%. Leave it unset on a
     * status with no damage band - a stat pointed at a band of zero would validate green and do nothing.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute BonusDamageAttribute;

    // Applier stat that scales DurationSeconds. Same rule: unset means the authored band is the whole story.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute DurationMultiplierAttribute;

    // Applier stat that scales ControlMagnitude, so gear can make a slow bite harder. A Bonus* fraction, diminished
    // like the others. Unset means the authored band is the whole story; a control status with no band ignores it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    FGameplayAttribute ControlMagnitudeAttribute;

    // Hard crowd control. Obeys HardCC immunity and the diminishing-returns escalation rules.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Effect")
    bool bHardCrowdControl = false;

    // Cue fired on the target the moment the status lands.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Feedback", meta = (Categories = "GameplayCue"))
    FGameplayTag OnsetCueTag;

    // Combinations with statuses already on the target. First match wins.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Reactions")
    TArray<FMythicStatusReaction> Reactions;

    // Player-facing name shown on badges and tooltips.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Presentation")
    FText DisplayName;

    // Player-facing description. Supports rich text markup.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Presentation", meta = (MultiLine = true))
    FText Description;

    // Badge icon.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Presentation")
    TSoftObjectPtr<UTexture2D> Icon;

    // Tint used for the badge, damage numbers and cue colouring.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status|Presentation")
    FLinearColor DisplayColor = FLinearColor::White;
};
