
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Mass/EntityHandle.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "AI/Cognition/CognitiveTypes.h"
#include "Tasks/Task.h"
#include "CognitiveBrainComponent.generated.h"

class UMythicCausalFabric;
class UMythicFactionDatabase;
class UMythicSocialGraph;
class UMythicLivingWorldSettings;
enum class EMythicSchedulePhase : uint8;

UCLASS(ClassGroup=(LivingWorld), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicCognitiveBrainComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicCognitiveBrainComponent();

    /**
     * Resolves the best dialogue template for the NPC's current state.
     * Integrates faction, role, active intention, and emotional pressure.
     * bCompanionCommentary=true selects the COMMENTARY template family (a companion remarking on the player's moral
     * action), gated by PlayerActionMoralScore vs each template's CommentaryMoralThreshold; in that mode a no-match
     * returns EMPTY (caller skips the bark). Defaults reproduce the normal player-initiated bark exactly.
     */
    UFUNCTION(BlueprintCallable, Category = "Living World|Dialogue")
    FText SelectDialogue(AActor *InteractingPlayer = nullptr, bool bCompanionCommentary = false, float PlayerActionMoralScore = 0.0f) const;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


    static constexpr float RoutineDesireCeiling = 0.65f;

    static float ScoreRoutineDesire(float Weight, float Pressure, float Multiplier);

    static float DecayBeliefConfidence(float Confidence, float DecayRate, double DeltaSeconds);

    static bool ShouldOverrideIntention(float BestUtility, float CurrentUtility, float Hysteresis);


    void InitializeBrain(
        FMythicFactionId Faction,
        FMythicCellCoord HomeCell,
        const FMythicPersonalityFragment &Personality,
        FMassEntityHandle SourceEntity,
        FMythicFactionId TrueFaction = FMythicFactionId(),
        FGameplayTag Role = FGameplayTag());


    const FMythicIntention &GetCurrentIntention() const { return CurrentIntention; }

    const TArray<FMythicBelief> &GetBeliefs() const { return Beliefs; }

    TArray<FMythicBelief> GetBeliefsCopy() const;

    const TArray<FMythicDesire> &GetLastDesires() const { return LastDesires; }

    TArray<FMythicDesire> GetLastDesiresCopy() const;


    bool IsSpyBrain() const { return TrueFaction.IsValid() && TrueFaction.Index != Faction.Index; }

    FMythicFactionId GetTrueFaction() const { return TrueFaction; }

    FGameplayTag GetRole() const { return Role; }

    FMythicCellCoord GetHomeCell() const { return HomeCell; }

    EMythicSchedulePhase GetCachedSchedulePhase() const { return CachedSchedulePhase; }

    FMythicCellCoord GetCachedWorkCell() const { return CachedWorkCell; }

    FMassEntityHandle GetSourceEntity() const { return SourceEntity; }

    FMythicFactionId GetFaction() const { return Faction; }

    FText GetDisplayName() const;

    const FMythicPersonalityFragment &GetPersonality() const { return Personality; }


    void InjectBelief(const FMythicBelief &Belief);

    void OnSignificantEvent(const FGameplayTag &EventTag, FMythicCellCoord EventCell);

    void StopThinking();

    void StartThinking();

    void ResetForReuse();

private:

    void Think();

    void UpdateBeliefs(double WorldTime);

    void InjectBeliefInternal(const FMythicBelief &Belief);

    void ScoreDesires(double WorldTime);

    void CommitIntention(double WorldTime);

    void ValidateIntention(double WorldTime);

    void OnAsyncThinkCompleted(double WorldTime);


    float ScoreSurvive(double WorldTime) const;
    float ScoreDefend(double WorldTime) const;
    float ScoreAvenge(double WorldTime) const;
    float ScorePatrol(double WorldTime) const;
    float ScoreTrade(double WorldTime) const;
    float ScoreSocialize(double WorldTime) const;
    float ScoreJoinPlayer(double WorldTime) const;
    float ScoreFlee(double WorldTime) const;
    float ScoreRest(double WorldTime) const;
    float ScoreExploit(double WorldTime) const;
    float ScoreRally(double WorldTime) const;
    float ScoreReport(double WorldTime) const;
    float ScoreFollowSchedule(double WorldTime) const;


    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> CausalFabric;

    UPROPERTY()
    TObjectPtr<UMythicFactionDatabase> FactionDB;

    UMythicSocialGraph *SocialGraph = nullptr;
    const UMythicLivingWorldSettings *Settings = nullptr;


    FMythicFactionId Faction;
    FMythicFactionId TrueFaction;
    FGameplayTag Role;
    FMythicCellCoord HomeCell;
    FMythicPersonalityFragment Personality;
    FMassEntityHandle SourceEntity;
    float PressureChannels[PressureChannelCount] = {};

    EMythicSchedulePhase CachedSchedulePhase{};

    FMythicCellCoord CachedWorkCell;


    TArray<FMythicBelief> Beliefs;
    mutable FCriticalSection BeliefsLock;
    TArray<FMythicDesire> LastDesires;
    FMythicIntention CurrentIntention;

    UE::Tasks::FTask AsyncThinkTask;

    std::atomic<bool> bIsThinkingAsync{false};


    FTimerHandle ThinkTimerHandle;
    float ThinkInterval = 1.0f;
    bool bInitialized = false;
};
