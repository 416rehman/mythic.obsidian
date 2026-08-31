#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Interaction/Attention/MythicEntityAttentionRules.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"

#include "MythicEntityAttentionSubsystem.generated.h"

class APlayerController;
class UMythicEntityPresentationComponent;
class UMythicEntityPresentationRegistry;

DECLARE_MULTICAST_DELEGATE_OneParam(FMythicEntityAttentionUpdated, uint64);
DECLARE_MULTICAST_DELEGATE_TwoParams(
    FMythicFocusedEntityChanged,
    const FMythicEntityPresentationInstance &,
    const FMythicEntityPresentationInstance &);

/**
 * One viewer-local attention service shared by nameplates, interaction, targeting, and accessibility.
 *
 * The subsystem owns no canonical entity identity. It consumes the world's push presentation registry, discovers
 * ordinary subjects through narrow collision broadphases plus a hard-capped compatibility cursor, and keeps a bounded
 * recent-gaze working set so cursor cadence never becomes presentation cadence. It runs bounded 10 Hz decisions plus
 * throttled event invalidations and publishes at most the fixed pool capacity. Each LocalPlayer is independent; no
 * decision pass performs a full registry or world actor census.
 */
UCLASS()
class MYTHIC_API UMythicEntityAttentionSubsystem : public ULocalPlayerSubsystem,
                                                  public FTickableGameObject {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual UWorld *GetTickableGameObjectWorld() const override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return false; }
    virtual bool IsTickableInEditor() const override { return false; }

    /**
     * Atomically replaces local attention tuning after sanitizing hard budgets and numeric ranges. The canonical
     * nameplate policy should call this once when its Primary Data Asset becomes available.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Attention")
    void ConfigureAttention(const FMythicEntityAttentionConfig &InConfig);

    /** Returns the sanitized runtime policy currently driving this LocalPlayer's attention decisions. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Attention")
    FMythicEntityAttentionConfig GetAttentionConfiguration() const { return Config; }

    /**
     * Sets the interaction-owned subject and requests an immediate bounded pass. Passing an invalid or stale instance
     * clears the override; the service never converts this public handle into a persistent identity.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Attention")
    void SetInteractionTarget(FMythicEntityPresentationInstance Instance);

    /**
     * Sets the hard-combat-target subject and requests an immediate bounded pass. Passing an invalid or stale instance
     * clears the override; hard targets acquire stable attention without ordinary gaze dwell.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Attention")
    void SetHardTarget(FMythicEntityPresentationInstance Instance);

    /**
     * Sets the deliberate dossier-inspect subject and requests an immediate bounded pass. Passing an invalid or stale
     * instance clears the override; downstream knowledge policy still decides what the inspect surface may disclose.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Attention")
    void SetInspectTarget(FMythicEntityPresentationInstance Instance);

    /**
     * Refreshes a bounded event edge for one public embodiment. Duration is local monotonic time and Strength is a
     * normalized ordering hint; neither parameter grants presentation permission or exposes private state.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Attention", meta = (ClampMin = "0.0"))
    void NotifyAttentionSignal(FMythicEntityPresentationInstance Instance,
                               EMythicEntityAttentionSignalKind SignalKind,
                               float DurationSeconds = 1.0f,
                               float Strength = 1.0f);

    /** Requests a throttled event-driven decision before the next scheduled 10 Hz pass. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Attention")
    void RequestImmediateRefresh();

    /** Returns the one stable public focus instance, or an invalid instance when no subject has earned focus. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Attention")
    FMythicEntityPresentationInstance GetFocusedInstance() const { return FocusedInstance; }

    /**
     * Copies the current bounded observation set for infrequent Blueprint consumers. Native high-frequency consumers
     * should use GetObservationsView or OnAttentionUpdated to avoid allocation.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Attention")
    TArray<FMythicEntityAttentionObservation> GetAttentionObservations() const { return Observations; }

    /** Returns an allocation-free view valid until the next attention decision or registry invalidation. */
    TConstArrayView<FMythicEntityAttentionObservation> GetObservationsView() const {
        return Observations;
    }

    /** Copies one current observation by exact handle-generation pair; false rejects stale or absent instances. */
    bool FindObservation(const FMythicEntityPresentationInstance &Instance,
                         FMythicEntityAttentionObservation &OutObservation) const;

    /** Returns the monotonic local revision advanced after decisions and immediate instance invalidation. */
    uint64 GetAttentionRevision() const { return AttentionRevision; }

    /** Native event emitted after a bounded observation set is committed. */
    FMythicEntityAttentionUpdated OnAttentionUpdated;

    /** Native event emitted when the single stable focus changes; payloads are old then new public instances. */
    FMythicFocusedEntityChanged OnFocusedEntityChanged;

private:
    struct FCandidate {
        FMythicEntityPresentationInstance Instance;
        TWeakObjectPtr<UMythicEntityPresentationComponent> Component;
        TWeakObjectPtr<AActor> Actor;
        FVector AnchorWorldLocation = FVector::ZeroVector;
        FVector2D ScreenPosition = FVector2D::ZeroVector;
        float DistanceCentimeters = 0.0f;
        float ViewAlignment = -1.0f;
        float Score = 0.0f;
        float SignalStrength = 1.0f;
        float StableGazeSeconds = 0.0f;
        EMythicEntityAttentionPriorityClass PriorityClass =
            EMythicEntityAttentionPriorityClass::Ambient;
        bool bOnScreen = false;
        bool bHasLineOfSight = false;
        bool bLineOfSightDeferred = false;
        bool bGazeCandidate = false;
        bool bRecentGazeResident = false;
        bool bWithinAmbientRange = false;
        bool bInteractionTarget = false;
        bool bHardTarget = false;
        bool bInspectTarget = false;
        bool bSafetySignal = false;
        bool bOpportunitySignal = false;
        bool bAwarenessSignal = false;
        bool bRecentCombatSignal = false;
        bool bRecentIncomingCombatSignal = false;
        bool bRecentActionSignal = false;
        bool bPublicAwarenessEvidence = false;
        bool bPublicSafetyEvidence = false;
        bool bSafetyCriticalStatus = false;
        bool bProtectedCandidate = false;
    };

    struct FLineOfSightCacheEntry {
        FVector ViewWorldLocation = FVector::ZeroVector;
        FVector AnchorWorldLocation = FVector::ZeroVector;
        double SampleTimeSeconds = -DBL_MAX;
        FMythicEntityAttentionVisibilityState Visibility;
    };

    struct FRecentGazeResidency {
        TWeakObjectPtr<UMythicEntityPresentationComponent> Component;
        double LastEligibleSeconds = 0.0;
        float LastScore = 0.0f;
    };

    struct FGazeTemporalState {
        double EligibleSinceSeconds = 0.0;
        double LastEligibleSeconds = 0.0;
    };

    struct FSignalState {
        double AwarenessUntilSeconds = 0.0;
        double OpportunityUntilSeconds = 0.0;
        double SafetyUntilSeconds = 0.0;
        double CombatUntilSeconds = 0.0;
        double IncomingCombatUntilSeconds = 0.0;
        double ActionUntilSeconds = 0.0;
        float AwarenessStrength = 0.0f;
        float OpportunityStrength = 0.0f;
        float SafetyStrength = 0.0f;
        float CombatStrength = 0.0f;
        float IncomingCombatStrength = 0.0f;
        float ActionStrength = 0.0f;

        bool IsExpired(double NowSeconds) const;
    };

    void EnsureRegistryBinding();
    void BindRegistry(UMythicEntityPresentationRegistry *Registry);
    void UnbindRegistry();
    void RebuildRegisteredSubjects();
    void HandlePresentationRegistered(const FMythicEntityPresentationInstance &Instance,
                                      UMythicEntityPresentationComponent *Component);
    void HandlePresentationUnregistered(const FMythicEntityPresentationInstance &Instance,
                                        UMythicEntityPresentationComponent *Component);
    void HandlePresentationRevision(const FMythicEntityPresentationInstance &Instance,
                                    uint64 SubjectRevision);
    void InvalidateInstance(const FMythicEntityPresentationInstance &Instance);
    void ResetRuntimeState(bool bBroadcastFocusLoss);

    void RunDecisionPass(double NowSeconds);
    bool ResolveLocalView(APlayerController *&OutController, FVector &OutViewLocation,
                          FVector &OutViewForward, FVector2D &OutViewportSize) const;
    bool BuildCandidate(const FMythicEntityPresentationInstance &Instance,
                        UMythicEntityPresentationComponent *Component,
                        const FVector &ViewLocation, const FVector &ViewForward,
                        double NowSeconds, FCandidate &OutCandidate);
    void GatherEventCandidates(const FVector &ViewLocation, const FVector &ViewForward,
                               double NowSeconds);
    void GatherRecentGazeCandidates(const FVector &ViewLocation,
                                    const FVector &ViewForward,
                                    double NowSeconds);
    void GatherSpatialCandidates(APlayerController &Controller,
                                 const FVector &ViewLocation, const FVector &ViewForward,
                                 double NowSeconds);
    void GatherRegistryFallbackCandidates(const FVector &ViewLocation,
                                          const FVector &ViewForward,
                                          double NowSeconds);
    bool ConsiderActorCandidate(AActor *Actor, const FVector &ViewLocation,
                                const FVector &ViewForward, double NowSeconds);
    void RememberRevisionCandidate(const FMythicEntityPresentationInstance &Instance,
                                   double NowSeconds);
    bool ReserveSignalSlot(const FMythicEntityPresentationInstance &Instance,
                           EMythicEntityAttentionSignalKind SignalKind,
                           double NowSeconds);
    static int32 GetSignalPriorityRank(const FSignalState &State, double NowSeconds);
    static double GetSignalLatestExpiry(const FSignalState &State);
    void ConsiderCandidate(const FCandidate &Candidate);
    void EnsureExplicitCandidate(const FMythicEntityPresentationInstance &Instance,
                                 const FVector &ViewLocation, const FVector &ViewForward,
                                 double NowSeconds);
    void ProjectAndSampleCandidates(APlayerController &Controller,
                                    const FVector &ViewLocation,
                                    const FVector2D &ViewportSize,
                                    double NowSeconds);
    bool ResolveLineOfSight(FCandidate &Candidate, APlayerController &Controller,
                            const FVector &ViewLocation, double NowSeconds,
                            struct FMythicEntityAttentionTraceBudget &TraceBudget,
                            bool &bOutDeferred);
    void UpdateGazeState(FCandidate &Candidate, double NowSeconds);
    void UpdateRecentGazeResidencies(double NowSeconds);
    void RefreshRecentGazeResidency(const FCandidate &Candidate,
                                    double NowSeconds);
    void TrimRecentGazeResidencies();
    void UpdateStableFocus(double NowSeconds);
    void PublishObservations();
    void PurgeExpiredRuntimeState(double NowSeconds);
    void SetFocusedInstance(const FMythicEntityPresentationInstance &NewFocusedInstance,
                            double NowSeconds);
    FCandidate *FindCandidate(const FMythicEntityPresentationInstance &Instance);
    const FCandidate *FindCandidate(const FMythicEntityPresentationInstance &Instance) const;
    bool IsRegisteredInstance(const FMythicEntityPresentationInstance &Instance) const;
    bool IsSafetyCriticalStatus(FGameplayTag StatusType);
    static bool IsCandidateHigherPriority(const FCandidate &Left, const FCandidate &Right);

    FMythicEntityAttentionConfig Config;

    TWeakObjectPtr<UWorld> BoundWorld;
    TWeakObjectPtr<UMythicEntityPresentationRegistry> BoundRegistry;
    TMap<FMythicEntityPresentationInstance,
         TWeakObjectPtr<UMythicEntityPresentationComponent>> RegisteredSubjects;
    TArray<FMythicEntityPresentationInstance> RegisteredSubjectOrder;

    TArray<UMythicEntityPresentationComponent *> RegistryComponentScratch;
    TArray<FHitResult> GazeHitScratch;
    TArray<FOverlapResult> PersonalSpaceOverlapScratch;
    TSet<TWeakObjectPtr<AActor>> SpatialActorScratch;
    TArray<FCandidate> CandidateScratch;
    TArray<FMythicEntityAttentionObservation> Observations;

    TMap<FMythicEntityPresentationInstance, FLineOfSightCacheEntry> LineOfSightCache;
    TMap<FMythicEntityPresentationInstance, FGazeTemporalState> GazeStates;
    TMap<FMythicEntityPresentationInstance, FRecentGazeResidency>
        RecentGazeResidencies;
    TMap<FMythicEntityPresentationInstance, FSignalState> Signals;
    TMap<FMythicEntityPresentationInstance, double> RevisionCandidatesUntil;
    TMap<FGameplayTag, bool> SafetyCriticalStatusCache;

    FMythicEntityPresentationInstance InteractionTarget;
    FMythicEntityPresentationInstance HardTarget;
    FMythicEntityPresentationInstance InspectTarget;
    FMythicEntityPresentationInstance FocusedInstance;
    FMythicEntityPresentationInstance PendingFocusInstance;

    double PendingFocusSinceSeconds = 0.0;
    double LastFocusedEligibleSeconds = 0.0;
    double LastDecisionSeconds = -DBL_MAX;
    uint64 LastRegistryRevision = 0;
    uint64 AttentionRevision = 0;
    int32 RegistryFallbackCursor = 0;
    bool bImmediateRefreshRequested = true;
};
