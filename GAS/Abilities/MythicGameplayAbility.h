// 

#pragma once

#include "CoreMinimal.h"
#include "MythicAbilityCost.h"
#include "MythicDamageContainer.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/MythicAbilitySourceInterface.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "MythicGameplayAbility.generated.h"

class UMythicAbilitySystemComponent;
class AMythicPlayerController;

/**
 * EMythicAbilityActivationPolicy
 *
 *	Defines how an ability is meant to activate.
 */
UENUM(BlueprintType)
enum class EMythicAbilityActivationPolicy : uint8 {
    // Try to activate the ability when the input is triggered.
    OnInputTriggered,

    // Continually try to activate the ability while the input is active.
    WhileInputActive,

    // Try to activate the ability when an avatar is assigned.
    OnSpawn
};

/**
 * EMythicAbilityActivationGroup
 *
 *	Defines how an ability activates in relation to other abilities.
 */
UENUM(BlueprintType)
enum class EMythicAbilityActivationGroup : uint8 {
    // Ability runs independently of all other abilities.
    Independent,

    // Ability is canceled and replaced by other exclusive abilities.
    Exclusive_Replaceable,

    // Ability blocks all other exclusive abilities from activating.
    Exclusive_Blocking,

    MAX UMETA(Hidden)
};

UCLASS()
class MYTHIC_API UMythicGameplayAbility : public UGameplayAbility {
    GENERATED_BODY()

    void SendEvent(FGameplayAbilityTargetDataHandle TargetData, FGameplayEffectContextHandle EffectContextHandle, FGameplayTag EventTag);

public:
    // Constructor
    UMythicGameplayAbility(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());
    void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) const;

    // Override OnAvatarSet
    virtual void OnAvatarSet(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) override;

    /// --- DAMAGE ---

    /** Creates gameplay effect container spec to be applied later via ApplyDamageContainerSpec */
    UFUNCTION(BlueprintCallable, Category = "MythicAbility", meta=(AutoCreateRefTerm = "EventData"))
    virtual FMythicDamageContainerSpec MakeDamageContainerSpec(const FMythicDamageContainer &Container, int32 OverrideGameplayLevel = -1);

    /** Applies a gameplay effect container spec that was previously created */
    UFUNCTION(BlueprintCallable, Category = "MythicAbility")
    virtual TArray<FActiveGameplayEffectHandle> ApplyDamageContainerSpec(const FMythicDamageContainerSpec &ContainerSpec);

    /** Applies a gameplay effect container, by creating and applying the spec */
    UFUNCTION(BlueprintCallable, Category = "MythicAbility", meta = (AutoCreateRefTerm = "EventData"))
    virtual TArray<FActiveGameplayEffectHandle> ApplyDamageContainer(const FMythicDamageContainer &Container, const TArray<FHitResult> &HitResults,
                                                                     const TArray<AActor *> &TargetActors, int32 OverrideGameplayLevel = -1);
    /** Add targets to a damage container spec */
    UFUNCTION(BlueprintCallable, Category = "MythicAbility")
    virtual void AddTargetsToDamageContainerSpec(UPARAM(ref) FMythicDamageContainerSpec &ContainerSpec, const TArray<FHitResult> &HitResults,
                                                 const TArray<AActor *> &TargetActors);

    /// ~~~ END DAMAGE ~~~

    EMythicAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
    EMythicAbilityActivationGroup GetActivationGroup() const { return ActivationGroup; }

    // Returns true if the requested activation group is a valid transition.
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Mythic|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
    bool CanChangeActivationGroup(EMythicAbilityActivationGroup NewGroup) const;

    // Tries to change the activation group.  Returns true if it successfully changed.
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Mythic|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
    bool ChangeActivationGroup(EMythicAbilityActivationGroup NewGroup);

    //~UGameplayAbility interface
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                    const FGameplayTagContainer *SourceTags, const FGameplayTagContainer *TargetTags,
                                    FGameplayTagContainer *OptionalRelevantTags) const override;
    virtual void SetCanBeCanceled(bool bCanBeCanceled) override;
    virtual void OnGiveAbility(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) override;
    virtual void OnRemoveAbility(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) override;
    virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;
    virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo) const override;
    virtual FGameplayEffectContextHandle MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo) const override;
    virtual void GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, float &OutSourceLevel,
                                  const IMythicAbilitySourceInterface *&OutAbilitySource, AActor *&OutEffectCauser) const;
    virtual void ApplyAbilityTagsToGameplayEffectSpec(FGameplayEffectSpec &Spec, FGameplayAbilitySpec *AbilitySpec) const override;
    virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent &AbilitySystemComponent, const FGameplayTagContainer *SourceTags = nullptr,
                                                   const FGameplayTagContainer *TargetTags = nullptr,
                                                   OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;
    //~End of UGameplayAbility interface

    /** Called when this ability is granted to the ability system component. */
    UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
    void K2_OnAbilityAdded();

    /** Called when this ability is removed from the ability system component. */
    UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityRemoved")
    void K2_OnAbilityRemoved();

    /** Called when the ability system is initialized with a pawn avatar. */
    UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnPawnAvatarSet")
    void K2_OnAvatarSet();

    UFUNCTION(BlueprintCallable, Category = "Mythic|Ability")
    UMythicAbilitySystemComponent *GetMythicAbilitySystemComponentFromActorInfo() const;

    UFUNCTION(BlueprintCallable, Category = "Mythic|Ability")
    AMythicPlayerController *GetMythicPlayerControllerFromActorInfo() const;

    UFUNCTION(BlueprintCallable, Category = "Mythic|Ability")
    AController *GetControllerFromActorInfo() const;

    // Defines how this ability is meant to activate.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Ability Activation")
    EMythicAbilityActivationPolicy ActivationPolicy;

    // Defines the relationship between this ability activating and other abilities activating.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Ability Activation")
    EMythicAbilityActivationGroup ActivationGroup;

    // Additional costs that must be paid to activate this ability
    UPROPERTY(EditDefaultsOnly, Instanced, Category = Costs)
    TArray<TObjectPtr<UMythicAbilityCost>> AdditionalCosts;

    // Map of failure tags to anim montages that should be played with them
    UPROPERTY(EditDefaultsOnly, Category = "Advanced")
    TMap<FGameplayTag, TObjectPtr<UAnimMontage>> FailureTagToAnimMontage;
};
