
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

class UAbilitySystemComponent;
class UMythicStatusEffectDefinition;

UCLASS(BlueprintType)
class MYTHIC_API UMythicStatusEffectLibrary : public UMythicDataAsset {
    GENERATED_BODY()

public:
    // Every status the game can apply. Adding a status here is all that is required to make it live.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
    TArray<TObjectPtr<UMythicStatusEffectDefinition>> Statuses;
};

UCLASS()
class MYTHIC_API UMythicStatusRegistry : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    // Definition for a Status.Type.* tag. Null when no status is authored for the tag.
    UFUNCTION(BlueprintPure, Category = "Status")
    UMythicStatusEffectDefinition *FindStatus(FGameplayTag StatusType) const;

    // Definition whose buildup attribute matches, used to turn a buildup change back into its status.
    UMythicStatusEffectDefinition *FindStatusByBuildupAttribute(const FGameplayAttribute &BuildupAttribute) const;

    // Every authored status, for UI enumeration and cheats.
    UFUNCTION(BlueprintPure, Category = "Status")
    TArray<UMythicStatusEffectDefinition *> GetAllStatuses() const;

    // Applies a status straight to a target, bypassing buildup. Fires the onset cue and honours reactions.
    UFUNCTION(BlueprintCallable, Category = "Status", meta = (DefaultToSelf = "Instigator"))
    static bool ApplyStatusToActor(AActor *Target, FGameplayTag StatusType, AActor *Instigator);

    // Applies an already-resolved status to an ASC. Returns false when the status has no effect authored.
    static bool ApplyStatusEffect(UAbilitySystemComponent *TargetASC, const UMythicStatusEffectDefinition *Definition, AActor *Instigator, AActor *Causer);

    // Plays a cue on the target through the Mythic ASC multicast path. No-op for an unset tag.
    static void PlayStatusCue(UAbilitySystemComponent *TargetASC, const FGameplayTag &CueTag);

    /**
      * Whether this application is the one that should teach the player what the status is. True only for a player
      * meeting it for the first time, and only when there is something authored to read.
      */
    static bool ShouldTeachStatus(bool bTargetIsPlayer, bool bAlreadyKnown, bool bHasDescription);

    /**
     * What the applier's gear multiplies a status by, after its diminishing curve. 1.0 when the status names no
     * stat, when there is no applier, or when the applier has no ability system.
     */
    static float ResolveApplierScale(const AActor *Instigator, const FGameplayAttribute &Attribute);

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
