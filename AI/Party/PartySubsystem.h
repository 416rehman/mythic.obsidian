
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AI/Party/MythicPartyTypes.h"
#include "World/EnvironmentController/EnvironmentTypes.h"
#include "Containers/Ticker.h"
#include "PartySubsystem.generated.h"

class AMythicNPCCharacter;
class UMythicCausalFabric;
class UMythicLivingWorldSubsystem;
class UMythicSocialGraph;
class UMythicLivingWorldSettings;
class UMythicCognitiveBrainComponent;
class APawn;
class AActor;
struct FMythicMoralAction;
struct FMythicBelief;
class AMythicEnvironmentController;

UCLASS()
class MYTHIC_API UMythicPartySubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;


    bool AddCompanion(const FString &PlayerKey, AMythicNPCCharacter *NPC, FMassEntityHandle SourceEntity);

    static bool AnyPartyContainsEntityIdentity(
        const TMap<FString, TArray<FMythicPartyMember>> &AllParties,
        const FMythicEntityId &EntityId);

    /** Returns true when any live or save-restored party slot durably owns this canonical person. */
    bool ReferencesEntityIdentity(const FMythicEntityId &EntityId) const {
        return AnyPartyContainsEntityIdentity(PlayerParties, EntityId);
    }

    static FString MakeLegacyPartyKey(int32 LegacyPlayerId);

    bool RemoveCompanion(const FString &PlayerKey, AMythicNPCCharacter *NPC, bool bVoluntary = false);

    bool RemoveCompanionFromAnyParty(AMythicNPCCharacter *NPC, bool bVoluntary = false);

    int32 GetPartyMembers(const FString &PlayerKey, TArray<FMythicPartyMember> &OutMembers) const;

    int32 GetPartySize(const FString &PlayerKey) const;

    bool IsInParty(const AMythicNPCCharacter *NPC) const;

    static bool ShouldShareBelief(const FMythicBelief &Belief, int32 MaxHops);

    static float ComputeLoyaltyDelta(EMythicMoralSeverity Severity, float MercyAxisValue, float TendWeight);

    static float ComputeRestedLoyalty(float CurrentLoyalty, float Recovery);
    static float ComputeDecayedBetrayal(float CurrentPressure, float Decay);

    static float ComputeBetrayalPressureGain(float LoyaltyDelta, float TriggerDelta, float Multiplier);

    static void SerializeBelief(FArchive &Ar, FMythicBelief &Belief, int32 Version);

    static void SerializePartyKey(FArchive &Ar, FString &Key, int32 Version);

    APawn *GetLeaderPawn(const FString &PlayerKey) const;

    bool IsCompanionEntity(FMassEntityHandle Entity) const;


    bool IssueCompanionOrder(const FString &PlayerKey, AMythicNPCCharacter *Companion, EMythicCompanionOrder Order, AActor *OrderTarget);

    static float ComputeForcedComplianceGain(float LoyaltyDelta, float TriggerDelta, float BaseMultiplier, float ForcedComplianceScale);


    void OnPlayerAction(const FString &PlayerKey, const FGameplayTag &EventTag, const FMythicMoralAction &MoralAction);

    void EnterRestPhase(const FString &PlayerKey);

    void ExitRestPhase(const FString &PlayerKey);


    virtual void Serialize(FArchive &Ar) override;

private:
    UFUNCTION()
    void OnEnvironmentControllerRegistered(AMythicEnvironmentController *Controller);
    UFUNCTION()
    void OnDaytimeChanged(EDayTime PrevDayTime, EDayTime NewDayTime);
    void BindEnvironmentController(AMythicEnvironmentController *Controller);
    TWeakObjectPtr<AMythicEnvironmentController> BoundEnvController;


    void PropagateBeliefs(const FString &PlayerKey);

    float EvaluateLoyaltyImpact(const FMythicPartyMember &Member, const FMythicMoralAction &MoralAction) const;

    void CheckCompanionThresholds(const FString &PlayerKey, int32 MemberIndex);

    bool ResolveOrderBelief(const FString &PlayerKey, const FMythicPartyMember &Member, EMythicCompanionOrder Order,
                            AActor *OrderTarget, FGameplayTag &OutTag, FMythicCellCoord &OutCell) const;

    bool DoesOrderConflict(const FMythicPartyMember &Member, EMythicCompanionOrder Order, AActor *OrderTarget,
                           float &OutLoyaltyDelta) const;

    void HandleCompanionDeparture(const FString &PlayerKey, int32 MemberIndex);

    void HandleCompanionBetrayal(const FString &PlayerKey, int32 MemberIndex);

    void RemoveMemberAt(const FString &PlayerKey, TArray<FMythicPartyMember> &Party, int32 Index);

    void NotifyCompanionLost(const FString &PlayerKey, const FMythicPartyMember &Member, bool bBetrayed);


    void RebindCompanionsAfterLoad();

    bool TickCompanionRebuild(float DeltaTime);

    FMassEntityHandle CreateLoadedCompanionEntity(const FMythicPartyMember &Member);

    void RebindLoadedCompanion(const FString &PlayerKey, FMythicPartyMember &Member, AMythicNPCCharacter *Actor);

    FTSTicker::FDelegateHandle CompanionRebuildTickHandle;

    int32 CompanionRebuildAttempts = 0;


    TMap<FString, TArray<FMythicPartyMember>> PlayerParties;

    TSet<FMassEntityHandle> CompanionEntities;

    int32 MaxPartySize = 4;

    float LoyaltyDepartureThreshold = 0.15f;

    float BetrayalThreshold = 5.0f;

    float RestLoyaltyRecovery = 0.02f;
    float RestBetrayalDecay = 0.1f;

    float BetrayalTriggerDelta = -0.1f;
    float BetrayalPressureMultiplier = 2.0f;

    float ForcedComplianceScale = 2.0f;

    float CompanionCommentaryLoyaltyDelta = 0.15f;

    float BeliefPropagationDecay = 0.3f;

    int32 MaxBeliefPropagationHops = 3;


    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> CausalFabric;

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorld;

    UMythicSocialGraph *SocialGraph = nullptr;
    const UMythicLivingWorldSettings *Settings = nullptr;
};
