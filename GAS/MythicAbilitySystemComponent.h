#pragma once
#include "AbilitySystemComponent.h"
#include "MythicAbilityTagRelationshipMapping.h"
#include "Abilities/MythicGameplayAbility.h"
#include "AttributeSets/MythicAttributeSet.h"
#include "MythicAbilitySystemComponent.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicAbilitySystemComponent : public UAbilitySystemComponent {
    GENERATED_BODY()

public:
    UPROPERTY()
    TObjectPtr<UMythicAbilityTagRelationshipMapping> AbilityTagRelationshipMapping;

    void GetAdditionalActivationTagRequirements(const FGameplayTagContainer &AbilityTags, FGameplayTagContainer &OutActivationRequired,
                                                FGameplayTagContainer &OutActivationBlocked) const;

    virtual void BeginPlay() override;

    void SetTagRelationshipMapping(UMythicAbilityTagRelationshipMapping *NewMapping);

    // Returns all the AttributeSets that are currently registered to this AbilitySystemComponent
    UFUNCTION(BlueprintCallable, Category = MythicAbilitySystemComponent)
    const TArray<UMythicAttributeSet *> &GetAttributeSets() const;

    int32 ActivationGroupCounts[static_cast<uint8>(EMythicAbilityActivationGroup::MAX)];

    using TShouldCancelAbilityFunc = TFunctionRef<bool(const UMythicGameplayAbility *MythicAbility, FGameplayAbilitySpecHandle Handle)>;

    bool IsActivationGroupBlocked(EMythicAbilityActivationGroup Group) const;
    void AddAbilityToActivationGroup(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *MythicAbility);
    void RemoveAbilityFromActivationGroup(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *MythicAbility);
    void CancelActivationGroupAbilities(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *IgnoreMythicAbility, bool bReplicateCancelAbility);
    void CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility);

    void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo,
                              FGameplayAbilityTargetDataHandle &OutTargetDataHandle);

    void AbilityInputTagPressed(const FGameplayTag &InputTag);
    void AbilityInputTagReleased(const FGameplayTag &InputTag);
    void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
    void ClearAbilityInput();

    // Execute a gameplay cue on ALL clients via NetMulticast (for effects everyone should see)
    UFUNCTION(BlueprintCallable, Category = "GameplayCue")
    void ExecuteGameplayCueMulticast(FGameplayTag CueTag, const FGameplayCueParameters &CueParams);

protected:
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_ExecuteGameplayCue(FGameplayTag CueTag, FGameplayCueParameters CueParams);

protected:
    TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

    TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

    TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
