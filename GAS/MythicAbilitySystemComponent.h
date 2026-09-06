#pragma once
#include "AbilitySystemComponent.h"
#include "MythicAbilityTagRelationshipMapping.h"
#include "Abilities/MythicGameplayAbility.h"
#include "AttributeSets/MythicAttributeSet.h"
#include "Abilities/MythicAbilityWindowTypes.h"
#include "Cues/MythicAbilityPoseRemnant.h"
#include "MythicAbilitySystemComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FMythicOwnedShieldAbsorbed, FGuid, float, const FGameplayEffectContextHandle&);
DECLARE_MULTICAST_DELEGATE_OneParam(FMythicOwnedShieldRemoved, FGuid);
class UMythicGA_Skill;
class UMythicStatusEffectDefinition;
DECLARE_MULTICAST_DELEGATE_TwoParams(FMythicAbilityMovement, UMythicGA_Skill*, const FVector&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMythicStatusGroupWillApply, const UMythicStatusEffectDefinition*, const UAbilitySystemComponent*);

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicAbilitySystemComponent : public UAbilitySystemComponent {
    GENERATED_BODY()

public:
    virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
    virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
    void CancelAbilityWindows();
    int32 AddAbilityGameplayCue(FGameplayTag Tag,const FGameplayCueParameters& Parameters);
    bool RemoveAbilityGameplayCue(int32 ReplicationId);
    FGuid GrantOwnedShield(UObject* Source, FGuid ActivationId, FName LocalKey, float Amount, float Duration);
    bool RemoveOwnedShield(FGuid Handle);
    float GetOwnedShieldRemaining(FGuid Handle) const;
    void ReconcileOwnedShields(float ActualShield,float AttributeLoss = 0);
    void ConsumeOwnedShieldDamage(float Amount, const FGameplayEffectContextHandle& Context);
    void PublishOwnedShieldDamage(const FGameplayEffectContextHandle& Context);
    void BeginOwnedShieldDamage() { bProcessingShieldDamage = true; }
    FMythicOwnedShieldAbsorbed OnOwnedShieldAbsorbed;
    FMythicOwnedShieldRemoved OnOwnedShieldRemoved;
    FMythicAbilityMovement OnAbilityMovementStarted;
    FMythicAbilityMovement OnAbilityMovementEnded;
    /** Invalidates exact status reservations before canonical applications can invoke GAS callbacks. */
    FMythicStatusGroupWillApply OnStatusGroupWillApply;
    /** Local projection invalidation after avatar, grant/removal or input suppression changes. No replicated loadout. */
    FSimpleMulticastDelegate OnAbilityProjectionChanged;
    bool CanAcceptAbilityInput() const;
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
    void AbilityInputTagCanceled(const FGameplayTag &InputTag);
    void AbilityInputSpecCanceled(FGameplayAbilitySpecHandle SpecHandle);
    void SetAbilityInputSuppressed(bool bSuppressed);

    /** Queues one already-authorized exact ability spec for the normal input processing lifecycle. */
    void AbilityInputSpecPressed(FGameplayAbilitySpecHandle SpecHandle);

    /** Queues release for one previously pressed exact ability spec without matching any dynamic input tags. */
    void AbilityInputSpecReleased(FGameplayAbilitySpecHandle SpecHandle);

    void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
    void ClearAbilityInput();

    /** Executes a transient gameplay cue on every client through the authoritative unreliable multicast path. */
    UFUNCTION(BlueprintCallable, Category = "GameplayCue")
    void ExecuteGameplayCueMulticast(FGameplayTag CueTag, const FGameplayCueParameters &CueParams);
    /** Native authority-only cosmetic publication; ordered create/cancel for a finite pose lease. */
    void PublishAbilityPoseRemnant(const FMythicAbilityPoseRemnantEvent& Event,bool bCancel);

    /** Executes a transient gameplay cue on the owning client only; partners never see or hear it. */
    UFUNCTION(BlueprintCallable, Category = "GameplayCue")
    void ExecuteGameplayCueOwnerOnly(FGameplayTag CueTag, const FGameplayCueParameters &CueParams);

    UFUNCTION(Client, Unreliable, Category = "GameplayCue")
    void Client_ExecuteGameplayCue(FGameplayTag CueTag, FGameplayCueParameters Parameters);

protected:
    struct FOwnedShield {
        FGuid Handle;
        TWeakObjectPtr<UObject> Source;
        FGuid ActivationId;
        FName LocalKey;
        float Remaining = 0;
        double Expiry = 0;
        uint64 Sequence = 0;
        FActiveGameplayEffectHandle MaximumEffect;
        float RemovalClamp = 0;
    };
    TArray<FOwnedShield> OwnedShields;
    FTimerHandle OwnedShieldTimer;
    uint64 ShieldSequence = 0;
    bool bWritingOwnedShield = false;
    bool bProcessingShieldDamage = false;
    void ScheduleShieldExpiry();
    void ExpireOwnedShields();
    bool RemoveOwnedShieldInternal(FGuid Handle,bool bMaximumAlreadyRemoved);
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_ExecuteGameplayCue(FGameplayTag CueTag, FGameplayCueParameters CueParams);

protected:
    virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
    virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;
    UFUNCTION(Server, Reliable)
    void ServerAbilityInputCanceled(FGameplayAbilitySpecHandle SpecHandle, FPredictionKey ActivationKey);
    void CancelActiveAbilityInput(FGameplayAbilitySpecHandle SpecHandle, const FPredictionKey* RequiredKey = nullptr);
    struct FAbilityInputEdge { FGameplayAbilitySpecHandle Handle; bool bPressed = false; uint32 CancelEpoch = 0; };
    TArray<FAbilityInputEdge> AbilityInputEdges;
    TMap<FGameplayAbilitySpecHandle,uint32> InputCancelEpochs;
    TArray<FGameplayAbilitySpecHandle> ProcessingInputOwners;
    uint32 InputClearEpoch = 0;
    TMap<FGameplayTag,TArray<FGameplayAbilitySpecHandle>> PressedInputOwners;
    TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
    bool bAbilityInputSuppressed = false;
    bool bProcessingAbilityInput = false;
    bool bClearingAbilityInput = false;
    bool bInputWasBlocked = false;
};
