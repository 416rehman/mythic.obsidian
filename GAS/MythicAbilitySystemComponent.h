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
    // If set, this table is used to look up tag relationships for activate and cancel
    UPROPERTY()
    TObjectPtr<UMythicAbilityTagRelationshipMapping> AbilityTagRelationshipMapping;

    /** Looks at ability tags and gathers additional required and blocking tags */
    // For MythicAbilityTagRelationshipMapping
    void GetAdditionalActivationTagRequirements(const FGameplayTagContainer &AbilityTags, FGameplayTagContainer &OutActivationRequired,
                                                FGameplayTagContainer &OutActivationBlocked) const;

    virtual void BeginPlay() override;

    void SetTagRelationshipMapping(UMythicAbilityTagRelationshipMapping *NewMapping);

    // Returns all the AttributeSets that are currently registered to this AbilitySystemComponent
    UFUNCTION(BlueprintCallable, Category = MythicAbilitySystemComponent)
    const TArray<UMythicAttributeSet *> &GetAttributeSets() const;

    // Number of abilities running in each activation group.
    int32 ActivationGroupCounts[static_cast<uint8>(EMythicAbilityActivationGroup::MAX)];

    using TShouldCancelAbilityFunc = TFunctionRef<bool(const UMythicGameplayAbility *MythicAbility, FGameplayAbilitySpecHandle Handle)>;

    bool IsActivationGroupBlocked(EMythicAbilityActivationGroup Group) const;
    void AddAbilityToActivationGroup(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *MythicAbility);
    void RemoveAbilityFromActivationGroup(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *MythicAbility);
    void CancelActivationGroupAbilities(EMythicAbilityActivationGroup Group, UMythicGameplayAbility *IgnoreMythicAbility, bool bReplicateCancelAbility);
    void CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility);

    /** Gets the ability target data associated with the given ability handle and activation info */
    void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo,
                              FGameplayAbilityTargetDataHandle &OutTargetDataHandle);
};
