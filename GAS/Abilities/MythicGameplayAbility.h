
#pragma once

#include "CoreMinimal.h"
#include "MythicAbilityCost.h"
#include "MythicDamageContainer.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/TimerHandle.h"
#include "GAS/MythicAbilitySourceInterface.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "MythicGameplayAbility.generated.h"

class UMythicAbilitySystemComponent;
class AMythicPlayerController;

UENUM(BlueprintType)
enum class EMythicAbilityActivationPolicy : uint8 {
    OnInputTriggered,

    WhileInputActive,

    OnSpawn
};

UENUM(BlueprintType)
enum class EMythicAbilityActivationGroup : uint8 {
    Independent,

    Exclusive_Replaceable,

    Exclusive_Blocking,

    MAX UMETA(Hidden)
};

UCLASS()
class MYTHIC_API UMythicGameplayAbility : public UGameplayAbility {
    GENERATED_BODY()

    void SendEvent(FGameplayAbilityTargetDataHandle TargetData, FGameplayEffectContextHandle EffectContextHandle, FGameplayTag EventTag);

public:
    UMythicGameplayAbility(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());
    void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) const;

    virtual void OnAvatarSet(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) override;


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


    EMythicAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
    EMythicAbilityActivationGroup GetActivationGroup() const { return ActivationGroup; }

    // Returns true if the requested activation group is a valid transition.
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Mythic|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
    bool CanChangeActivationGroup(EMythicAbilityActivationGroup NewGroup) const;

    // Tries to change the activation group.  Returns true if it successfully changed.
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Mythic|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
    bool ChangeActivationGroup(EMythicAbilityActivationGroup NewGroup);

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
    virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo) const override;
    virtual FGameplayEffectContextHandle MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo) const override;
    virtual void GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, float &OutSourceLevel,
                                  const IMythicAbilitySourceInterface *&OutAbilitySource, AActor *&OutEffectCauser) const;
    virtual void ApplyAbilityTagsToGameplayEffectSpec(FGameplayEffectSpec &Spec, FGameplayAbilitySpec *AbilitySpec) const override;


protected:
    FGameplayTagContainer BuildAbilityContextTags() const;

public:
    virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent &AbilitySystemComponent, const FGameplayTagContainer *SourceTags = nullptr,
                                                   const FGameplayTagContainer *TargetTags = nullptr,
                                                   OUT FGameplayTagContainer *OptionalRelevantTags = nullptr) const override;

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

    // retrieve the active attack speed scaling factor clamped to a safe range
    UFUNCTION(BlueprintCallable, Category = "Mythic|Ability")
    float GetClampedAttackSpeedPlayRate() const;

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

    // Number of stored uses before the ability is gated. <=1 => legacy single-use behavior (charge system disabled).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Ability|Charges", meta = (ClampMin = "1"))
    int32 MaxCharges = 1;

    // Seconds to regenerate one charge. <=0 with MaxCharges>1 => charges never regenerate on their own.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Ability|Charges", meta = (ClampMin = "0.0"))
    float RechargeSeconds = 0.0f;

    // Optional shared-block tag. When this ability exhausts its charges it adds this loose tag to the owner ASC (and
    // removes it once a charge is available again); other abilities can list it in their ActivationBlockedTags to share
    // a group cooldown via the existing tag machinery — no owner-side wiring required.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Ability|Charges")
    FGameplayTag CooldownCategoryTag;

    bool AreChargesEnabled() const { return MaxCharges > 1; }

    // Current stored charges (server-authoritative runtime state).
    UFUNCTION(BlueprintPure, Category = "Mythic|Ability|Charges")
    int32 GetCurrentCharges() const { return CurrentCharges; }

private:
    UPROPERTY(Transient)
    int32 CurrentCharges = 1;

    FTimerHandle ChargeRechargeTimer;

    void InitializeCharges();
    void ConsumeChargeOnActivation();
    void EnsureRechargeTimer();
    void HandleChargeRecharge();
    void SetCategoryBlock(bool bBlocked);
};
