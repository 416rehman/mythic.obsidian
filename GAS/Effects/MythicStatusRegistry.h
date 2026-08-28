
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Itemization/MythicDataAsset.h"
#include "NativeGameplayTags.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicStatusRegistry.generated.h"

// Magnitudes an authored status effect reads so the same effect can carry a different number per application.
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_STATUS_DAMAGE);
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_STATUS_DURATION);
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_STATUS_CONTROL_MAGNITUDE);

class UAbilitySystemComponent;
class UMythicStatusEffectDefinition;
enum class EMythicStatusControlOperation : uint8;

UCLASS(BlueprintType)
class MYTHIC_API UMythicStatusEffectLibrary : public UMythicDataAsset {
    GENERATED_BODY()

public:
    /** Every canonical status definition the game can apply and expose to UI or tools. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
    TArray<TObjectPtr<UMythicStatusEffectDefinition>> Statuses;
};

UCLASS()
class MYTHIC_API UMythicStatusRegistry : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    /** Returns the canonical definition for a Status.Type.* tag, or null when the tag is not authored. */
    UFUNCTION(BlueprintPure, Category = "Status")
    UMythicStatusEffectDefinition *FindStatus(FGameplayTag StatusType) const;

    // Definition whose buildup attribute matches, used to turn a buildup change back into its status.
    UMythicStatusEffectDefinition *FindStatusByBuildupAttribute(const FGameplayAttribute &BuildupAttribute) const;

    /** Returns every canonical authored status for deterministic UI and tooling enumeration. */
    UFUNCTION(BlueprintPure, Category = "Status")
    TArray<UMythicStatusEffectDefinition *> GetAllStatuses() const;

    /** Applies a canonical status directly to a target, bypassing buildup while retaining cues and reactions. */
    UFUNCTION(BlueprintCallable, Category = "Status", meta = (DefaultToSelf = "Instigator"))
    static bool ApplyStatusToActor(AActor *Target, FGameplayTag StatusType, AActor *Instigator);

    /**
     * Resolves the ASC that owns an applied status stack. Player/NPC sources retain their gameplay ASC; an ASC-less
     * actor receives a lightweight, server-only source ASC so distinct hazards keep independent rolls, caps, and
     * attribution. Only truly actor-less world damage shares the GameState ASC. The defender is never substituted.
     */
    static UAbilitySystemComponent *ResolveStatusEffectSourceASC(AActor *Instigator, UAbilitySystemComponent *TargetASC);

    /** Applies an already-resolved status to an ASC. Returns false when the status has no effect authored. */
    static bool ApplyStatusEffect(UAbilitySystemComponent *TargetASC, const UMythicStatusEffectDefinition *Definition, AActor *Instigator, AActor *Causer);

    // Plays a cue on the target through the Mythic ASC multicast path. No-op for an unset tag.
    static void PlayStatusCue(UAbilitySystemComponent *TargetASC, const FGameplayTag &CueTag);

    /**
      * Whether this application is the one that should teach the player what the status is. True only for a player
      * meeting it for the first time, and only when there is something authored to read.
      */
    static bool ShouldTeachStatus(bool bTargetIsPlayer, bool bAlreadyKnown, bool bHasDescription);

    /**
     * What the applier's gear scales a status by, after its diminishing curve. Returns 1.0 - no change - when the
     * status names no stat, when there is no applier, or when the applier has no ability system.
     *
     * Two flavours because the project runs two stat conventions and the name picks which: a *Multiplier stat is
     * 1.0-based and read bare, a Bonus* stat is 0.0-based and read as (1 + x). Passing one to the other is off by
     * exactly one, in a direction nothing would catch.
     */
    static float ResolveApplierMultiplier(const AActor *Instigator, const FGameplayAttribute &Attribute);

    static float ResolveApplierBonus(const AActor *Instigator, const FGameplayAttribute &Attribute);

    /**
     * How much the applier's Power lifts a status's base damage, through the same authored contribution a weapon
     * roll rides. Returns 1.0 for a source with no Power (a trap, a hazard, a scripted tick), so those still deal
     * their authored band. This is what keeps a damage-over-time build's ticks scaling with the character instead
     * of dealing the same few points at level 200.
     */
    static float ResolveApplierPowerMultiplier(const AActor *Instigator);

    /**
     * The combined strength of every active control-status handle of one kind on a target. Each handle carries the
     * exact multiplicative aggregate snapshotted from all of its application rolls. Reductions (Slow, Weaken) are
     * floored so they can never reach a full stop; bonuses (Terrify) multiply upward. When no authored aggregate is
     * present, the pre-band constant is used once. Returns 1.0 when the target carries no status of this kind.
     */
    static float GetControlReductionMultiplier(const UAbilitySystemComponent *TargetASC, FGameplayTag StateTag, float FallbackMagnitude);
    static float GetControlBonusMultiplier(const UAbilitySystemComponent *TargetASC, FGameplayTag StateTag, float FallbackMagnitude);

    // Shared by both: the applier's raw stat value, or a sentinel when there is no stat to read.
    static bool TryReadApplierStat(const AActor *Instigator, const FGameplayAttribute &Attribute, float &OutRaw);

    /**
     * The status's own band, or the global baseline when it authors none, scaled by everything the applier brings.
     * A status with no authored band is playable before it is tuned rather than silently dealing nothing.
     */
    static float RollMagnitudeOrBase(const FRollDefinition &Range, float BaseWhenUnauthored, float Scale, float Roll01);

    /**
     * Adds one rolled stack to the snapshotted aggregate tick magnitude. Once the authored cap is reached,
     * reapplication preserves the aggregate so refreshing duration cannot reroll or rewrite existing stacks.
     */
    static float ResolveStackedDamageMagnitude(float ExistingAggregate, float NewStackRoll,
                                               int32 ExistingStackCount, int32 StackLimit);

    /**
     * Folds one rolled control stack into its equivalent aggregate fraction. This snapshots every roll, so UE's
     * newest-spec stack replacement cannot make earlier Slow/Weaken/Terrify applications drift with the newest roll.
     */
    static float ResolveStackedControlMagnitude(float ExistingAggregate, float NewStackRoll,
                                                int32 ExistingStackCount, int32 StackLimit,
                                                EMythicStatusControlOperation Operation);

    // Rolls a range, scales it by the applier's multiplier, and never returns a negative.
    static float RollScaledMagnitude(const FRollDefinition &Range, int32 Level, float SourceMultiplier, float Roll01);

private:
    void EnsureIndexed() const { if (!bIndexed) { const_cast<UMythicStatusRegistry *>(this)->BuildIndex(); } }
    void BuildIndex();

    bool bIndexed = false;

    UPROPERTY(Transient)
    TObjectPtr<UMythicStatusEffectLibrary> Library;

    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<UMythicStatusEffectDefinition>> StatusByType;
};
