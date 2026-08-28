
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AbilitySystemInterface.h"
#include "DetourCrowdAIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "World/LivingWorld/Activities/ActivityTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicAIController.generated.h"

class UMythicAbilitySystemComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UMythicActivityCatalog;

struct FMythicAIDebugState {
    FVector EngageAnchorLocation = FVector::ZeroVector;
    float LeashRange = 0.0f;
    int32 PatrolLegIndex = 0;
    bool bFleeingMove = false;
    bool bCompanionFollowActive = false;
    FString CompanionLeaderKey;
    bool bHasHostileTarget = false;
};

UCLASS(Blueprintable, BlueprintType, Abstract)
class MYTHIC_API AMythicAIController : public ADetourCrowdAIController, public IAbilitySystemInterface {
    GENERATED_BODY()

public:
    AMythicAIController();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnPossess(APawn *InPawn) override;
    virtual void OnUnPossess() override;

    static void SanitizePerception(float &SightRadius, float &LoseSightRadius, float &PeripheralAngleDegrees);

    static int32 SelectClosestHostileIndex(TConstArrayView<float> DistancesSq);

    static int32 SelectHighestThreatIndex(TConstArrayView<float> Threats);

    static float ComputeThreatDelta(float Damage, float ThreatPerDamage, float BonusThreat);

    static bool ShouldReleaseLeash(float DistSqFromAnchor, float LeashRangeSq);

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor &Other) const override;
    virtual FGenericTeamId GetGenericTeamId() const override;

    // The actor this NPC currently considers a hostile target (server-only). Null when none perceived.
    UFUNCTION(BlueprintCallable, Category = "Mythic AI|Combat")
    AActor *GetCurrentHostileTarget() const { return CurrentHostileTarget; }

    int32 CopyThreatTable(TArray<TPair<TWeakObjectPtr<AActor>, float>> &OutThreats) const;

    void CopyAIDebugState(FMythicAIDebugState &Out) const;

    void ForceEngageTarget(AActor *Target);

    void SetCompanionFollow(bool bActive, const FString &LeaderKey);

    static FMythicCellCoord GetPatrolCell(FMythicCellCoord Anchor, int32 LegIndex);

    static bool IsDayHour(float Hour);

protected:
    // Sight perception that detects only enemies (affiliation filtered via GetTeamAttitudeTowards).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic AI|Perception")
    TObjectPtr<UAIPerceptionComponent> AIPerception;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    // Current hostile target selected from perception (server-only; AI controllers don't replicate).
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    TObjectPtr<AActor> CurrentHostileTarget;

    TMap<TWeakObjectPtr<AActor>, float> ThreatTable;

    TWeakObjectPtr<UAbilitySystemComponent> ThreatBoundASC;

    void HandleThreatFromHit(const struct FGameplayEventData *Payload);

    void PruneThreatTable();

    void UnbindThreatEvent();

    FVector EngageAnchorLocation = FVector::ZeroVector;

    // Distance (cm) within which the NPC attempts its melee attack on the current target.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    float MeleeAttackRange = 180.0f;

    // Leash: if the NPC is pulled farther than this (cm) from where it engaged (EngageAnchorLocation), it gives up the
    // target and resets — prevents infinite cross-map pursuit / dragging enemy trains. 0 (default) = NO leash (infinite
    // pursuit, the prior behaviour — zero regression); a designer sets a positive range per NPC class to enable it.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat", meta = (ClampMin = "0.0"))
    float LeashRange = 0.0f;

    // How often (seconds) to attempt an attack while engaged. The ability's Cooldown GE gates the real rate.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    float AttackAttemptInterval = 0.5f;

    // Distance (cm) the NPC retreats toward when its cognitive brain commits a Flee intention instead of attacking.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    float FleeDistance = 800.0f;

    // AcceptanceRadius (cm) for CLOSING on a target (engage pursuit + Avenge re-pursue). MUST stay well below
    // MeleeAttackRange minus the DetourCrowd reach padding (~agent-radius*1.1 ≈ 37cm) — otherwise the crowd reach
    // test halts the agent just OUTSIDE MeleeAttackRange and it never reaches swing range. 120 leaves a safe margin.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    float PursueAcceptanceRadius = 120.0f;

    FTimerHandle AttackTimerHandle;

    bool bFleeingMove = false;

    // ── Idle (out-of-combat) dispatch ──
    // How often (seconds) to re-evaluate the committed non-combat intention and steer the NPC toward a home-anchored
    // desire's cell. Combat (a valid CurrentHostileTarget) always preempts this.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Idle")
    float IdleDispatchInterval = 2.0f;

    // AcceptanceRadius (cm) for idle home moves; also the "already home" distance so the move isn't re-issued every
    // tick once the NPC has arrived.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Idle")
    float IdleMoveAcceptanceRadius = 150.0f;

    int32 PatrolLegIndex = 0;

    // Stop-band (cm) a recruited companion holds from its party leader. Default is a shippable value; the tuned
    // follow distance / re-path cadence is a balance pass.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Idle")
    float FollowAcceptanceRadius = 250.0f;

    FTimerHandle IdleTimerHandle;

    void TickIdleBehavior();

    bool TickActivityBehavior(class UMythicCognitiveBrainComponent *Brain, class UMythicLivingWorldSubsystem *LW,
                              const class UMythicTerritoryGrid *Grid, FMythicCellCoord LiveCell);

    AActor *ScanNearbyMerchant(float Radius, bool &bOutFound) const;

    float ResolveGameHour() const;

    // Radius (cm) within which a context activity's nearby-merchant gate searches. Designer-tunable.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Idle", meta = (ClampMin = "0.0"))
    float MerchantScanRadius = 1500.0f;

    TWeakObjectPtr<UMythicActivityCatalog> CachedActivityCatalog;

    TArray<FMythicActivityDef> DefaultActivities;

    bool bActivitySourceResolved = false;

    float CompanionFollowInterval = 0.3f;
    bool bCompanionFollowActive = false;
    FString CompanionLeaderKey;
    FTimerHandle FollowTimerHandle;
    void TickCompanionFollow();

    void RefreshLiveCell();

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor *Actor, FAIStimulus Stimulus);

    void TryAttackCurrentTarget();

    void ReleaseHostileTarget();

    // Content hooks (Blueprint/BT) to complete the loop: actually attack / move to the target. C++ has no NPC
    // attack ability to invoke (deferred - see Docs/BACKLOG.md), so execution is left to an authored ability.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic AI|Combat")
    void OnEngageHostileTarget(AActor *Target);

    /** Blueprint hook fired after the controller releases its previously tracked hostile target. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic AI|Combat")
    void OnHostileTargetLost(AActor *PreviousTarget);
};
