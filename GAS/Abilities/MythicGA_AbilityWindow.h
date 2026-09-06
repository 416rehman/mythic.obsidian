#pragma once
#include "CoreMinimal.h"
#include "MythicGA_Skill.h"
#include "MythicAbilityWindowTypes.h"
#include "MythicAbilityActionTypes.h"
#include "GAS/Effects/MythicStatusGroupReservation.h"
#include "GAS/Cues/MythicAbilityPoseRemnant.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicGA_AbilityWindow.generated.h"

class UMythicAbilitySystemComponent;
class ACharacter;
class UAbilityTask_ApplyRootMotionMoveToForce;
struct FMythicLootCreditContext;
struct FLootTierBonus;
class UMythicEntityPresentationComponent;

USTRUCT(BlueprintType)
struct FMythicAbilityPayload {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FMythicDamageContainer Damage;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FGameplayTag Status;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float Chance = 1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="0")) float StatusDuration = 0;
};

UCLASS(Abstract,Blueprintable)
class MYTHIC_API UMythicGA_AbilityWindow : public UMythicGA_Skill {
    GENERATED_BODY()
public:
    UMythicGA_AbilityWindow();
    virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled) override;
    virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayTagContainer* SourceTags=nullptr,const FGameplayTagContainer* TargetTags=nullptr,FGameplayTagContainer* OptionalRelevantTags=nullptr) const override;
    virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,FGameplayTagContainer* OptionalRelevantTags) const override;
    virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,FGameplayAbilityActivationInfo ActivationInfo) const override;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") float Duration = 30;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") float WindupSeconds = 0;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") float IntentRange = 1200;
    /** Pure self utility can admit movement or defense even when no camera ray can converge. */
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") bool bRequiresCameraAim = true;
    /** Half-angle for single hostile target assistance after the authority's exact aim ray misses. */
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window",meta=(ClampMin="0",ClampMax="12")) float HostileAimAssistDegrees = 6;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") EMythicAbilityIntent InitialIntent = EMythicAbilityIntent::Self;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") TMap<FGameplayTag,EMythicAbilityIntent> IntentOverrides;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") TMap<FName,int32> Budgets;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") TMap<FName,FMythicAbilityPayload> Payloads;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") TSubclassOf<UGameplayEffect> HealingEffect;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") TSubclassOf<UGameplayEffect> StaminaEffect;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") bool bAcceptRecast = false;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") bool bListenForLootCredits = false;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") FGameplayTag CastCue;
    UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Window") FMythicAbilityQuery LifecycleTargetQuery;

    UFUNCTION(BlueprintImplementableEvent,Category="Window") void OnWindowStarted();
    UFUNCTION(BlueprintImplementableEvent,BlueprintCosmetic,Category="Window") void OnPredictedWindowStarted();
    UFUNCTION(BlueprintImplementableEvent,Category="Window") void OnWindowSignal(FName Name,const FMythicAbilitySignal& Signal);
    UFUNCTION(BlueprintImplementableEvent,Category="Window") void OnWindowEnding(EMythicAbilityWindowEnd Reason);
    UFUNCTION(BlueprintNativeEvent,Category="Window") bool ValidateAbilityIntent();
    UFUNCTION(BlueprintPure,Category="Window") bool ValidateAbilityGroundPoint(FVector Point,FVector& GroundPoint) const;
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") bool ScheduleSignal(FName Name,float Delay);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") void EndAbilityWindow(EMythicAbilityWindowEnd Reason);
    UFUNCTION(BlueprintPure,Category="Window") float GetAbilityNumber(FGameplayTag Key,float Fallback = 0) const;
    UFUNCTION(BlueprintPure,Category="Window") float GetAbilityParameter(FName Name,float Fallback = 0) const;
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") FGuid BeginAbilityAction(FName ActionId);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") void EndAbilityAction(FGuid ActionHandle);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") FMythicAbilityDamageResult ApplyAbilityAction(AActor* Target,FGuid ActionHandle,int32 ShareIndex = 0);
    UFUNCTION(BlueprintPure,Category="Window|Utility") bool TryGetResolvedUtilityParameter(FName ActionId,FName Name,float& Value) const;
    UFUNCTION(BlueprintPure,Category="Window|Utility") bool TryGetUtilityActionParameter(FGuid ActionHandle,FName Name,float& Value) const;
    UFUNCTION(BlueprintPure,Category="Window|Utility") bool TryReadUtilityActionSourceAttribute(FGuid ActionHandle,FGameplayAttribute Attribute,float& Value) const;
    UFUNCTION(BlueprintPure,Category="Window") bool HasAbilityFlag(FGameplayTag Tag) const;
    UFUNCTION(BlueprintPure,Category="Window") FVector GetWindowOrigin() const { return Origin; }
    UFUNCTION(BlueprintPure,Category="Window") AActor* GetWindowTarget() const { return CheckTargetIncarnation(IntentTarget.Get()) ? IntentTarget.Get():nullptr; }
    UFUNCTION(BlueprintPure,Category="Window") AActor* GetWindowAvatar() const { return WindowAvatar.Get(); }
    UFUNCTION(BlueprintPure,Category="Window") FVector GetAimDirection() const;
    /** Frozen world movement input; a neutral request uses the activation's facing or its opposite. */
    UFUNCTION(BlueprintPure,Category="Window|Movement") FVector GetMovementIntentDirection(bool bBackwardWhenNeutral = false) const;
    UFUNCTION(BlueprintPure,Category="Window") float GetWindowElapsed() const;
    UFUNCTION(BlueprintPure,Category="Window") float GetAbilityHealthFraction(AActor* Target) const;
    UFUNCTION(BlueprintPure,Category="Window") float GetAbilityMaxHealth(AActor* Target) const;
    UFUNCTION(BlueprintPure,Category="Window") bool HasAbilityStatus(AActor* Target,FGameplayTag Status) const;
    UFUNCTION(BlueprintPure,Category="Window") int32 CountAbilityConditions(AActor* Target) const;
    UFUNCTION(BlueprintPure,Category="Window") int32 GetAbilityConditionMask(AActor* Target) const;
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") bool TrySpendAbilityBudget(FName Key,int32 Count = 1);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") bool TryVisitAbilityTarget(FName Set,AActor* Target,int32 Maximum = 32);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") void ClearAbilityVisitedSet(FName Set);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") TArray<AActor*> QueryAbilityTargets(FMythicAbilityQuery Query,FVector Origin,FVector Direction) const;
    UFUNCTION(BlueprintPure,Category="Window") bool IsValidAbilityTarget(AActor* Target,bool bParty,bool bRequireLOS = true) const;
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") void ApplyAbilityPayload(AActor* Target,FName PayloadRow,float Scalar = 1);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") FMythicAbilityHealResult ApplyAbilityHeal(AActor* Target,float Amount);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") bool ApplyAbilityResource(AActor* Target,float Amount);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") FGuid GrantOwnedShield(AActor* Target,FName Key,float Amount,float ShieldDuration);
    UFUNCTION(BlueprintPure,Category="Window") float GetOwnedShieldRemaining(FGuid Handle) const;
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") void RemoveOwnedShield(FGuid Handle);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") void CleanseAbilityStatuses(AActor* Target,FGameplayTagContainer Allowlist);
    /** Preparation only, before commitment. Selects one explicitly removable self status group in priority order. */
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window|Status") FGuid ReserveSelfStatusGroup(const TArray<FGameplayTag>& Priority);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window|Status") FMythicStatusGroupRemoval ConsumeSelfStatusGroup(FGuid Reservation);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window|Utility") FMythicStatusGroupRemoval ConsumeUtilityStatusGroup(FGuid ActionHandle,FGuid Reservation);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window|Status") void ReleaseStatusGroupReservation(FGuid Reservation);
    /** Capsule and walkable-floor admission for a short grounded escape. Returns only the reachable path. */
    UFUNCTION(BlueprintPure,Category="Window|Movement") bool ValidateAbilityEscapePath(FVector Direction,float Distance,TArray<FVector>& Points) const;
    /** Read-only bounded geometry query, also usable after a failed window has cleaned up its avatar.
     * StopReason describes the first obstruction even when a shorter reachable prefix is admitted. */
    UFUNCTION(BlueprintPure,Category="Window|Movement")
    static void InspectAbilityEscapePath(ACharacter* Character,FVector Direction,float Distance,bool& bAdmitted,TArray<FVector>& Points,FName& StopReason,FHitResult& StoppingHit);
    static bool QueryAbilityEscapePath(ACharacter* Character,FVector Direction,float Distance,TArray<FVector>& Points,FName& StopReason,FHitResult& StoppingHit);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window|Utility") FGuid PrepareUtilityEscape(FName ActionId,FVector Direction);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window|Utility") bool StartPreparedUtilityEscape(FGuid ActionHandle,FGuid Preparation);
    /** Consumes only a native successful removal receipt; cosmetic failure never changes paid gameplay. */
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window|Utility") bool PresentUtilityStatusRemnant(FGuid RemovalReceiptId,FGuid EscapePreparation,UMythicAbilityPoseRemnantStyle* Style);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window|Utility") bool PlayPreparedUtilityDepartureCue(FGuid Preparation,FGameplayTag Cue);
    UFUNCTION(BlueprintImplementableEvent,Category="Window|Utility") void OnUtilityMovementFinished(const FMythicAbilityMovementReceipt& Receipt);
    FMythicAbilityUtilityMovementEvent OnUtilityMovementResolved;
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") bool ListenToActor(AActor* Actor,FGameplayTag Event,FName SignalName);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") bool ListenToStatus(AActor* Actor,FGameplayTag Status,FName SignalName);
    UFUNCTION(BlueprintCallable,Category="Window") bool StartAbilityMovement(FVector Destination,float MovementSeconds,FName SignalName);
    UFUNCTION(BlueprintCallable,Category="Window") bool StartAbilityPathMovement(const TArray<FVector>& Points,float MovementSeconds,FName SignalName);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") FActiveGameplayEffectHandle ApplyTrackedAbilityEffect(AActor* Target,TSubclassOf<UGameplayEffect> EffectClass,float EffectDuration,FName RemovedSignal);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") void PlayAbilityCue(FGameplayTag Tag,FVector Location,bool bPersistent,AActor* Target = nullptr);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") void RemoveAbilityCue(FGameplayTag Tag,AActor* Target = nullptr);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") bool ModifyAbilityOutgoingHit(float Multiplier);
    UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly,Category="Window") bool TryModifyAbilityLootCredit(FGuid Receipt,float Scale,FName Budget);
    FGuid GetWindowActivationId() const { return ActivationId; }
    FGameplayTagContainer GetWindowClassification() const { return Classification; }
    bool IsWindowAuthority() const;
protected:
    virtual bool ValidateAbilityIntent_Implementation();
    UFUNCTION() void HandleRecast(float TimeWaited);
    UFUNCTION() void HandleMovementFinished();
    UFUNCTION() void HandleUtilityMovementReached();
    UFUNCTION() void HandleUtilityMovementTimedOut();
private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FAbilityCueOwnershipTest;
    friend class FAbilityMassRecipientRecycleTest;
    friend class FAbilityUtilityActionTest;
#endif
    bool CheckTargetIncarnation(AActor* Actor,bool bRetain = true) const;
    bool IsEligibleAbilityTarget(AActor* Target,bool bParty,bool bRequireLOS,bool bRetain) const;
    struct FRetainedActor { TWeakObjectPtr<UMythicEntityPresentationComponent> Presentation; FMythicEntityPresentationInstance Instance; FDelegateHandle Deactivated; };
    mutable TMap<TWeakObjectPtr<AActor>,FRetainedActor> RetainedActors;
    mutable TSet<TWeakObjectPtr<AActor>> InvalidatedActors;
    bool ResolveIntent();
    bool ResolveHostileIntent(float Range);
    void BeginAuthorityWindow();
    void BeginPredictedSequence();
    void ReceiveAimData(const FGameplayAbilityTargetDataHandle& Data,FGameplayTag ApplicationTag);
    void ResolveSnapshot();
    bool CaptureActionSource();
    bool ValidatePreparedUtilityEscapes() const;
    void FinishUtilityMovement(EMythicAbilityMovementOutcome Outcome);
    bool ValidatePreparedUtilityEscape(FGuid Preparation) const;
    void BeginSequence();
    void PublishLifecycleEvent(FGameplayTag EventTag);
    void ArmRecast();
    void DispatchSignal(FName Name,FMythicAbilitySignal Signal);
    void CleanupWindow();
    void HandleActorEvent(const FGameplayEventData* Event,TWeakObjectPtr<AActor> Actor,FName Name);
    void HandleShieldAbsorption(FGuid Handle,float Amount,const FGameplayEffectContextHandle& Context);
    void HandleLootCredit(const FMythicLootCreditContext& Credit,FLootTierBonus& Bonus);
    void SampleMovement();
    TWeakObjectPtr<AActor> WindowAvatar;
    TWeakObjectPtr<AActor> IntentTarget;
    FGuid ActivationId;
    FVector Origin = FVector::ZeroVector;
    FVector PreviousMovementLocation = FVector::ZeroVector;
    double StartTime = 0;
    bool bCommitted = false;
    bool bSequenceStarted = false;
    bool bLifecycleEnded = false;
    bool bEnding = false;
    bool bCleaned = true;
    bool bPredictingMotion = false;
    FVector ActivationAim = FVector::ZeroVector;
    FVector ActivationMovement = FVector::ZeroVector;
    FVector ActivationFacing = FVector::ForwardVector;
    FDelegateHandle AimDataListener;
    FTimerHandle AimDataTimeout;
    int32 ScheduledSignals = 0;
    int32 DispatchCount = 0;
    int32 CurrentGeneration = 0;
    bool bInsideReaction = false;
    bool bHasReactionLineage = true;
    FMythicAbilitySignal CaptureSignalLineage() const;
    TMap<FGameplayTag,float> Numbers;
    TMap<FName,float> Parameters;
    UPROPERTY() TMap<FName,FMythicAbilityActionProfile> ActionProfiles;
    TSharedPtr<const FMythicAbilityActionSource> ActionSource;
    TMap<FName,TSharedPtr<const FMythicAbilityActionSource>> ActionSources;
    struct FActionGroup {
        FGuid Activation;
        FMythicAbilityActionProfile Profile;
        FGameplayEffectSpecHandle Application;
        FGameplayEffectContextHandle Context;
        TArray<TSet<TWeakObjectPtr<AActor>>> ClaimedTargets;
        TSet<TWeakObjectPtr<AActor>> AllTargets;
        bool bStatusIntent = false;
    };
    TMap<FGuid,TSharedPtr<FActionGroup>> ActionGroups;
    TMap<FGuid,TSharedPtr<FMythicStatusGroupReservation>> StatusReservations;
    struct FPreparedUtilityEscape {
        FName ActionId;
        FGuid Activation;
        TWeakObjectPtr<AActor> Avatar;
        TSharedPtr<const FMythicAbilityActionSource> Source;
        FVector Start=FVector::ZeroVector,End=FVector::ZeroVector,Direction=FVector::ZeroVector;
        FTransform Departure;
        bool bDeparturePresented=false;
        float Distance=0,Transit=0,Radius=0,HalfHeight=0;
    };
    TMap<FGuid,FPreparedUtilityEscape> PreparedUtilityEscapes;
    struct FSuccessfulUtilityRemoval { FGuid Activation; double ServerTime=0; float Duration=0; };
    TMap<FGuid,FSuccessfulUtilityRemoval> SuccessfulUtilityRemovals;
    UPROPERTY() TArray<FMythicAbilityPoseRemnantEvent> ActivePoseRemnants;
    TOptional<FMythicAbilityMovementReceipt> PendingUtilityMovement;
    bool bUtilityMovementStarted=false;
    int32 UtilityCallbackDepth=0;
    FGameplayTagContainer Flags;
    FGameplayTagContainer Classification;
    TMap<FName,int32> RemainingBudgets;
    TMap<FName,TSet<TWeakObjectPtr<AActor>>> Visited;
    TArray<FTimerHandle> Timers;
    struct FListener { TWeakObjectPtr<UAbilitySystemComponent> ASC; FGameplayTag Tag; FDelegateHandle Handle; bool bStatus = false; };
    TArray<FListener> Listeners;
    struct FShield { TWeakObjectPtr<UMythicAbilitySystemComponent> ASC; FGuid Handle; };
    TArray<FShield> Shields;
    TMap<TWeakObjectPtr<UMythicAbilitySystemComponent>,FDelegateHandle> ShieldListeners;
    struct FCue { TWeakObjectPtr<UMythicAbilitySystemComponent> ASC; FGameplayTag Tag; int32 ReplicationId=INDEX_NONE; };
    TArray<FCue> Cues;
    FDelegateHandle LootListener;
    TSet<FGuid> LootReceipts;
    const FMythicLootCreditContext* CurrentLootCredit = nullptr;
    FLootTierBonus* CurrentLootBonus = nullptr;
    FGameplayEffectContextHandle CurrentOutgoingContext;
    UPROPERTY() TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> MovementTask;
    FTimerHandle MovementSampleTimer;
    FName MovementSignal;
    FMythicAbilitySignal MovementLineage;
    struct FTrackedEffect { TWeakObjectPtr<UAbilitySystemComponent> ASC; FActiveGameplayEffectHandle Handle; };
    TArray<FTrackedEffect> Effects;
    TArray<FVector> MovementPath;
    float PathSpeed = 0;
    int32 MovementPoint = 0;
    bool StartNextMovementSegment();
};
