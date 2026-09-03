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

    /** Rebuild persistent equipment-affix handles after possession, avatar swaps and reconnect actor-info setup. */
    virtual void InitAbilityActorInfo(AActor *InOwnerActor, AActor *InAvatarActor) override;

    /** Registers an activated Mythic ability in its local activation group before activation callbacks run. */
    virtual void NotifyAbilityActivated(FGameplayAbilitySpecHandle Handle, UGameplayAbility *Ability) override;

    /** Unregisters an ended Mythic ability from its local activation group before end callbacks run. */
    virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility *Ability,
                                    bool bWasCancelled) override;

    void SetTagRelationshipMapping(UMythicAbilityTagRelationshipMapping *NewMapping);

    /** Returns every Mythic attribute set currently registered to this ability-system component. */
    UFUNCTION(BlueprintCallable, Category = MythicAbilitySystemComponent)
    const TArray<UMythicAttributeSet *> &GetAttributeSets() const;

    /** Reconciles native Current/Maximum contracts after actor-info setup or bulk attribute initialization. */
    void ReconcileAllBoundedAttributes();

    /** Active ability count for each activation group on this local authority or predicting client ASC. */
    int32 ActivationGroupCounts[static_cast<uint8>(EMythicAbilityActivationGroup::MAX)] = {};

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

    /** Queues one already-authorized exact ability spec for the normal input processing lifecycle. */
    void AbilityInputSpecPressed(FGameplayAbilitySpecHandle SpecHandle);

    /** Queues release for one previously pressed exact ability spec without matching any dynamic input tags. */
    void AbilityInputSpecReleased(FGameplayAbilitySpecHandle SpecHandle);

    void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
    void ClearAbilityInput();

    /** Executes a transient gameplay cue on every client through the authoritative unreliable multicast path. */
    UFUNCTION(BlueprintCallable, Category = "GameplayCue")
    void ExecuteGameplayCueMulticast(FGameplayTag CueTag, const FGameplayCueParameters &CueParams);

    /** Executes a transient gameplay cue on the owning client only; partners never see or hear it. */
    UFUNCTION(BlueprintCallable, Category = "GameplayCue")
    void ExecuteGameplayCueOwnerOnly(FGameplayTag CueTag, const FGameplayCueParameters &CueParams);

    UFUNCTION(Client, Unreliable, Category = "GameplayCue")
    void Client_ExecuteGameplayCue(FGameplayTag CueTag, FGameplayCueParameters Parameters);

protected:
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_ExecuteGameplayCue(FGameplayTag CueTag, FGameplayCueParameters CueParams);

protected:
    TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

    TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

    TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
