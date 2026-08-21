#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicAchievementComponent.generated.h"

class UMythicAchievementSet;
class UMythicAchievementDefinition;
class UMythicStatLedgerComponent;
class UMythicNarrativeStateComponent;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnAchievementUnlocked, FGameplayTag, AchievementTag);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicAchievementComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicAchievementComponent();

    // Pure: has this achievement tag been unlocked?
    UFUNCTION(BlueprintPure, Category = "Progression|Achievements")
    bool IsAchievementUnlocked(FGameplayTag AchievementTag) const { return UnlockedAchievements.HasTagExact(AchievementTag); }

    const FGameplayTagContainer &GetUnlockedAchievements() const { return UnlockedAchievements; }

    // Broadcast (server-side) the first time each achievement unlocks. See delegate doc above.
    UPROPERTY(BlueprintAssignable, Category = "Progression|Achievements")
    FMythicOnAchievementUnlocked OnAchievementUnlocked;

    void RestoreUnlockedAchievements(const FGameplayTagContainer &Saved);

    void SetRestoring(bool bInRestoring) { bIsRestoring = bInRestoring; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // Component-level override for the achievement set; if null, resolved from DeveloperSettings in BeginPlay.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Achievements")
    TSoftObjectPtr<UMythicAchievementSet> AchievementSet;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(Replicated)
    FGameplayTagContainer UnlockedAchievements;

    bool bIsRestoring = false;

private:
    UMythicStatLedgerComponent *ResolveLedger() const;
    UMythicNarrativeStateComponent *ResolveNarrative() const;
    APlayerController *ResolvePC() const;
    UMythicAchievementSet *ResolveSet();

    void BuildIndex();

    void QueueAffectedByStat(const FGameplayTag &StatTag);
    void QueueAffectedByTag(const FGameplayTag &Tag);
    void DrainPending();
    void EvaluateBatch(const TSet<int32> &Indices);
    void UnlockAchievement(const UMythicAchievementDefinition &Def);

    UFUNCTION()
    void HandleCounterChanged(FGameplayTag Tag, int64 NewValue);
    UFUNCTION()
    void HandleStoryTagEarned(FGameplayTag Tag);

    UPROPERTY(Transient)
    TObjectPtr<UMythicAchievementSet> ResolvedSet = nullptr;

    TMap<FGameplayTag, TArray<int32>> ExactStatIndex;
    TMap<FGameplayTag, TArray<int32>> HierStatIndex;
    TMap<FGameplayTag, TArray<int32>> TagTriggerIndex;
    bool bIndexBuilt = false;

    TSet<int32> PendingIndices;
    bool bEvaluating = false;
};
