
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/EmergentQuests/MythicEmergentQuestRules.h"
#include "MythicEmergentQuestSubsystem.generated.h"

class UMythicLivingWorldSubsystem;
class UObjectiveDefinition;
class UObjectiveTracker;
class AMythicPlayerController;
struct FMythicWorldEvent;

UCLASS()
class MYTHIC_API UMythicEmergentQuestSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Deinitialize() override;

    int32 GetActiveQuestCount() const { return ActiveQuests.Num(); }

private:
    void HandleWorldSimCommitted();

    void HandleCleanupTimer();

    void ReconcileCompletions();

    void BuildDefaultRulePool();

    UObjectiveDefinition *BuildEmergentObjective(const FMythicEmergentQuestRule &Rule, const FMythicEmergentReward &Reward,
                                                 const FVector &MarkerLocation, const FText &FactionName);

    void PostDeliveryContractOffer(const FMythicEmergentQuestRule &Rule, const FMythicWorldEvent &Event, int32 DangerTier);

    EMythicFactionRelation ResolvePlayerRelationToFaction(AMythicPlayerController *PC, FMythicFactionId Faction) const;

    bool IsAuthority() const;

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorld = nullptr;

    FDelegateHandle CommitHandle;
    FTimerHandle CleanupTimerHandle;

    uint32 LastSeenEventId = 0;
    bool bSeeded = false;

    TArray<FMythicEmergentQuestRule> RulePool;

    struct FMythicActiveEmergentQuest {
        UObjectiveDefinition *Objective = nullptr;
        FGameplayTag QuestKind;
        FMythicFactionId RewardFaction;
        float StandingReward = 0.0f;
        double ExpireTimeSeconds = 0.0;
        TSet<TWeakObjectPtr<UObjectiveTracker>> RewardedTrackers;
    };
    TArray<FMythicActiveEmergentQuest> ActiveQuests;

    UPROPERTY()
    TArray<TObjectPtr<UObjectiveDefinition>> ActiveObjectiveRoots;

    float OfferRadius = 30000.0f;
    float OfferLifetimeSeconds = 300.0f;
    float CleanupIntervalSeconds = 15.0f;
    float MinConsideredSignificance = 0.01f;
};
