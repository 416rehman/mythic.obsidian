#include "Interaction/Attention/MythicEntityAttentionSubsystem.h"

#include "CollisionQueryParams.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Interaction/Attention/MythicEntityAttentionRules.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/Entity/MythicEntityPresentationTags.h"

bool UMythicEntityAttentionSubsystem::FSignalState::IsExpired(
    const double NowSeconds) const {
    return AwarenessUntilSeconds <= NowSeconds
           && OpportunityUntilSeconds <= NowSeconds
           && SafetyUntilSeconds <= NowSeconds
           && CombatUntilSeconds <= NowSeconds
           && IncomingCombatUntilSeconds <= NowSeconds
           && ActionUntilSeconds <= NowSeconds;
}

bool UMythicEntityAttentionSubsystem::ShouldCreateSubsystem(
    UObject *Outer) const {
    const ULocalPlayer *LocalPlayer = Cast<ULocalPlayer>(Outer);
    return LocalPlayer && !LocalPlayer->IsTemplate();
}

void UMythicEntityAttentionSubsystem::Initialize(
    FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    Config = FMythicEntityAttentionRules::SanitizeConfig(Config);
    RegistryComponentScratch.Reserve(128);
    RegisteredSubjectOrder.Reserve(128);
    GazeHitScratch.Reserve(Config.MaxSpatialCandidatesPerPass);
    PersonalSpaceOverlapScratch.Reserve(Config.MaxSpatialCandidatesPerPass);
    SpatialActorScratch.Reserve(Config.MaxSpatialCandidatesPerPass);
    CandidateScratch.Reserve(Config.MaxEvaluatedCandidates);
    Observations.Reserve(Config.MaxPublishedObservations);
    RecentGazeResidencies.Reserve(Config.MaxEvaluatedCandidates);
    bImmediateRefreshRequested = true;
}

void UMythicEntityAttentionSubsystem::Deinitialize() {
    UnbindRegistry();
    ResetRuntimeState(false);
    OnAttentionUpdated.Clear();
    OnFocusedEntityChanged.Clear();
    Super::Deinitialize();
}

void UMythicEntityAttentionSubsystem::Tick(const float DeltaTime) {
    (void)DeltaTime;
    EnsureRegistryBinding();

    if (!BoundRegistry.IsValid()) {
        return;
    }

    const double NowSeconds = FPlatformTime::Seconds();
    const double ScheduledInterval = 1.0 / FMath::Max(1.0f, Config.DecisionRateHz);
    const bool bScheduledPassDue = NowSeconds - LastDecisionSeconds >= ScheduledInterval;
    const bool bImmediatePassDue = bImmediateRefreshRequested
        && NowSeconds - LastDecisionSeconds
               >= Config.MinimumImmediatePassIntervalSeconds;
    if (!bScheduledPassDue && !bImmediatePassDue) {
        return;
    }

    RunDecisionPass(NowSeconds);
}

TStatId UMythicEntityAttentionSubsystem::GetStatId() const {
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMythicEntityAttentionSubsystem,
                                    STATGROUP_Tickables);
}

UWorld *UMythicEntityAttentionSubsystem::GetTickableGameObjectWorld() const {
    const ULocalPlayer *LocalPlayer = GetLocalPlayer();
    return LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
}

bool UMythicEntityAttentionSubsystem::IsTickable() const {
    const UWorld *World = GetTickableGameObjectWorld();
    return !IsTemplate() && World && World->IsGameWorld();
}

void UMythicEntityAttentionSubsystem::ConfigureAttention(
    const FMythicEntityAttentionConfig &InConfig) {
    if (!IsInGameThread()) {
        return;
    }

    Config = FMythicEntityAttentionRules::SanitizeConfig(InConfig);
    CandidateScratch.Reserve(Config.MaxEvaluatedCandidates);
    Observations.Reserve(Config.MaxPublishedObservations);
    GazeHitScratch.Reserve(Config.MaxSpatialCandidatesPerPass);
    PersonalSpaceOverlapScratch.Reserve(Config.MaxSpatialCandidatesPerPass);
    SpatialActorScratch.Reserve(Config.MaxSpatialCandidatesPerPass);
    RecentGazeResidencies.Reserve(Config.MaxEvaluatedCandidates);
    TrimRecentGazeResidencies();
    RequestImmediateRefresh();
}

void UMythicEntityAttentionSubsystem::SetInteractionTarget(
    FMythicEntityPresentationInstance Instance) {
    if (!IsInGameThread()) {
        return;
    }
    EnsureRegistryBinding();
    if (!Instance.IsValid() || !IsRegisteredInstance(Instance)) {
        Instance.Reset();
    }
    if (InteractionTarget == Instance) {
        return;
    }
    InteractionTarget = Instance;
    RequestImmediateRefresh();
}

void UMythicEntityAttentionSubsystem::SetHardTarget(
    FMythicEntityPresentationInstance Instance) {
    if (!IsInGameThread()) {
        return;
    }
    EnsureRegistryBinding();
    if (!Instance.IsValid() || !IsRegisteredInstance(Instance)) {
        Instance.Reset();
    }
    if (HardTarget == Instance) {
        return;
    }
    HardTarget = Instance;
    RequestImmediateRefresh();
}

void UMythicEntityAttentionSubsystem::SetInspectTarget(
    FMythicEntityPresentationInstance Instance) {
    if (!IsInGameThread()) {
        return;
    }
    EnsureRegistryBinding();
    if (!Instance.IsValid() || !IsRegisteredInstance(Instance)) {
        Instance.Reset();
    }
    if (InspectTarget == Instance) {
        return;
    }
    InspectTarget = Instance;
    RequestImmediateRefresh();
}

void UMythicEntityAttentionSubsystem::NotifyAttentionSignal(
    const FMythicEntityPresentationInstance Instance,
    const EMythicEntityAttentionSignalKind SignalKind,
    const float DurationSeconds, const float Strength) {
    if (!IsInGameThread() || !Instance.IsValid()) {
        return;
    }
    EnsureRegistryBinding();
    if (!IsRegisteredInstance(Instance)) {
        return;
    }

    const float SafeDuration = FMath::IsFinite(DurationSeconds)
        ? FMath::Clamp(DurationSeconds, 0.0f, 60.0f)
        : 0.0f;
    if (SafeDuration <= 0.0f) {
        return;
    }

    const double NowSeconds = FPlatformTime::Seconds();
    if (!ReserveSignalSlot(Instance, SignalKind, NowSeconds)) {
        return;
    }

    FSignalState &State = Signals.FindOrAdd(Instance);
    const double UntilSeconds = NowSeconds + SafeDuration;
    const float SafeStrength = FMath::IsFinite(Strength)
        ? FMath::Clamp(Strength, 0.0f, 1.0f)
        : 0.0f;
    switch (SignalKind) {
    case EMythicEntityAttentionSignalKind::Awareness:
        State.AwarenessUntilSeconds = UntilSeconds;
        State.AwarenessStrength = SafeStrength;
        break;
    case EMythicEntityAttentionSignalKind::Opportunity:
        State.OpportunityUntilSeconds = UntilSeconds;
        State.OpportunityStrength = SafeStrength;
        break;
    case EMythicEntityAttentionSignalKind::Safety:
        State.SafetyUntilSeconds = UntilSeconds;
        State.SafetyStrength = SafeStrength;
        break;
    case EMythicEntityAttentionSignalKind::Combat:
        State.CombatUntilSeconds = UntilSeconds;
        State.CombatStrength = SafeStrength;
        break;
    case EMythicEntityAttentionSignalKind::CombatFromSubjectToViewer:
        State.CombatUntilSeconds = UntilSeconds;
        State.CombatStrength = SafeStrength;
        State.IncomingCombatUntilSeconds = UntilSeconds;
        State.IncomingCombatStrength = SafeStrength;
        break;
    case EMythicEntityAttentionSignalKind::Action:
        State.ActionUntilSeconds = UntilSeconds;
        State.ActionStrength = SafeStrength;
        break;
    default:
        return;
    }

    if (State.IsExpired(NowSeconds)) {
        Signals.Remove(Instance);
    }
    RequestImmediateRefresh();
}

void UMythicEntityAttentionSubsystem::RequestImmediateRefresh() {
    bImmediateRefreshRequested = true;
}

bool UMythicEntityAttentionSubsystem::FindObservation(
    const FMythicEntityPresentationInstance &Instance,
    FMythicEntityAttentionObservation &OutObservation) const {
    for (const FMythicEntityAttentionObservation &Observation : Observations) {
        if (Observation.Instance == Instance) {
            OutObservation = Observation;
            return true;
        }
    }
    OutObservation = FMythicEntityAttentionObservation();
    return false;
}

void UMythicEntityAttentionSubsystem::EnsureRegistryBinding() {
    ULocalPlayer *LocalPlayer = GetLocalPlayer();
    UWorld *World = LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
    UMythicEntityPresentationRegistry *Registry =
        World && World->IsGameWorld()
            ? World->GetSubsystem<UMythicEntityPresentationRegistry>()
            : nullptr;

    if (BoundWorld.Get() != World || BoundRegistry.Get() != Registry) {
        UnbindRegistry();
        ResetRuntimeState(true);
        BoundWorld = World;
        BindRegistry(Registry);
        return;
    }

    if (Registry && LastRegistryRevision != Registry->GetRegistryRevision()) {
        RebuildRegisteredSubjects();
        RequestImmediateRefresh();
    }
}

void UMythicEntityAttentionSubsystem::BindRegistry(
    UMythicEntityPresentationRegistry *Registry) {
    BoundRegistry = Registry;
    if (!Registry) {
        return;
    }

    Registry->OnPresentationRegistered.AddUObject(
        this, &UMythicEntityAttentionSubsystem::HandlePresentationRegistered);
    Registry->OnPresentationUnregistered.AddUObject(
        this, &UMythicEntityAttentionSubsystem::HandlePresentationUnregistered);
    RebuildRegisteredSubjects();
    RequestImmediateRefresh();
}

void UMythicEntityAttentionSubsystem::UnbindRegistry() {
    if (UMythicEntityPresentationRegistry *Registry = BoundRegistry.Get()) {
        Registry->OnPresentationRegistered.RemoveAll(this);
        Registry->OnPresentationUnregistered.RemoveAll(this);
    }

    for (const TPair<FMythicEntityPresentationInstance,
                     TWeakObjectPtr<UMythicEntityPresentationComponent>> &Pair :
         RegisteredSubjects) {
        if (UMythicEntityPresentationComponent *Component = Pair.Value.Get()) {
            Component->OnPresentationRevision.RemoveAll(this);
        }
    }
    RegisteredSubjects.Reset();
    RegisteredSubjectOrder.Reset();
    RegistryFallbackCursor = 0;
    BoundRegistry.Reset();
    LastRegistryRevision = 0;
}

void UMythicEntityAttentionSubsystem::RebuildRegisteredSubjects() {
    UMythicEntityPresentationRegistry *Registry = BoundRegistry.Get();
    if (!Registry) {
        return;
    }

    for (const TPair<FMythicEntityPresentationInstance,
                     TWeakObjectPtr<UMythicEntityPresentationComponent>> &Pair :
         RegisteredSubjects) {
        if (UMythicEntityPresentationComponent *Component = Pair.Value.Get()) {
            Component->OnPresentationRevision.RemoveAll(this);
        }
    }
    RegisteredSubjects.Reset();
    RegisteredSubjectOrder.Reset();
    RegistryFallbackCursor = 0;

    Registry->GetRegisteredComponents(RegistryComponentScratch);
    for (UMythicEntityPresentationComponent *Component :
         RegistryComponentScratch) {
        if (!IsValid(Component)) {
            continue;
        }
        const FMythicEntityPresentationInstance Instance =
            Component->GetPresentationInstance();
        if (!Instance.IsValid() || !Component->RepresentsInstance(Instance)) {
            continue;
        }
        RegisteredSubjects.Add(Instance, Component);
        RegisteredSubjectOrder.Add(Instance);
        Component->OnPresentationRevision.AddWeakLambda(
            this, [this, Instance](const uint64 SubjectRevision) {
                HandlePresentationRevision(Instance, SubjectRevision);
            });
        RememberRevisionCandidate(Instance, FPlatformTime::Seconds());
    }
    LastRegistryRevision = Registry->GetRegistryRevision();
}

void UMythicEntityAttentionSubsystem::HandlePresentationRegistered(
    const FMythicEntityPresentationInstance &Instance,
    UMythicEntityPresentationComponent *Component) {
    if (!Instance.IsValid() || !IsValid(Component)
        || !Component->RepresentsInstance(Instance)) {
        return;
    }

    if (TWeakObjectPtr<UMythicEntityPresentationComponent> *Existing =
            RegisteredSubjects.Find(Instance)) {
        if (UMythicEntityPresentationComponent *OldComponent = Existing->Get();
            OldComponent && OldComponent != Component) {
            OldComponent->OnPresentationRevision.RemoveAll(this);
        }
    }
    RegisteredSubjects.Add(Instance, Component);
    RegisteredSubjectOrder.AddUnique(Instance);
    Component->OnPresentationRevision.RemoveAll(this);
    Component->OnPresentationRevision.AddWeakLambda(
        this, [this, Instance](const uint64 SubjectRevision) {
            HandlePresentationRevision(Instance, SubjectRevision);
        });
    RememberRevisionCandidate(Instance, FPlatformTime::Seconds());
    if (UMythicEntityPresentationRegistry *Registry = BoundRegistry.Get()) {
        LastRegistryRevision = Registry->GetRegistryRevision();
    }
    RequestImmediateRefresh();
}

void UMythicEntityAttentionSubsystem::HandlePresentationUnregistered(
    const FMythicEntityPresentationInstance &Instance,
    UMythicEntityPresentationComponent *Component) {
    TWeakObjectPtr<UMythicEntityPresentationComponent> StoredComponent;
    if (RegisteredSubjects.RemoveAndCopyValue(Instance, StoredComponent)) {
        if (UMythicEntityPresentationComponent *Resolved = StoredComponent.Get()) {
            Resolved->OnPresentationRevision.RemoveAll(this);
        }
    } else if (Component) {
        Component->OnPresentationRevision.RemoveAll(this);
    }
    RegisteredSubjectOrder.RemoveSingleSwap(Instance, EAllowShrinking::No);
    RegistryFallbackCursor = RegisteredSubjectOrder.IsEmpty()
        ? 0
        : RegistryFallbackCursor % RegisteredSubjectOrder.Num();

    if (UMythicEntityPresentationRegistry *Registry = BoundRegistry.Get()) {
        LastRegistryRevision = Registry->GetRegistryRevision();
    }
    InvalidateInstance(Instance);
    RequestImmediateRefresh();
}

void UMythicEntityAttentionSubsystem::HandlePresentationRevision(
    const FMythicEntityPresentationInstance &Instance,
    const uint64 SubjectRevision) {
    (void)SubjectRevision;
    RememberRevisionCandidate(Instance, FPlatformTime::Seconds());
    RequestImmediateRefresh();
}

void UMythicEntityAttentionSubsystem::InvalidateInstance(
    const FMythicEntityPresentationInstance &Instance) {
    LineOfSightCache.Remove(Instance);
    GazeStates.Remove(Instance);
    RecentGazeResidencies.Remove(Instance);
    Signals.Remove(Instance);
    RevisionCandidatesUntil.Remove(Instance);
    CandidateScratch.RemoveAllSwap(
        [&Instance](const FCandidate &Candidate) {
            return Candidate.Instance == Instance;
        }, EAllowShrinking::No);
    Observations.RemoveAllSwap(
        [&Instance](const FMythicEntityAttentionObservation &Observation) {
            return Observation.Instance == Instance;
        }, EAllowShrinking::No);

    if (InteractionTarget == Instance) {
        InteractionTarget.Reset();
    }
    if (HardTarget == Instance) {
        HardTarget.Reset();
    }
    if (InspectTarget == Instance) {
        InspectTarget.Reset();
    }
    if (PendingFocusInstance == Instance) {
        PendingFocusInstance.Reset();
        PendingFocusSinceSeconds = 0.0;
    }
    if (FocusedInstance == Instance) {
        SetFocusedInstance(FMythicEntityPresentationInstance(),
                           FPlatformTime::Seconds());
    }

    ++AttentionRevision;
    OnAttentionUpdated.Broadcast(AttentionRevision);
}

void UMythicEntityAttentionSubsystem::ResetRuntimeState(
    const bool bBroadcastFocusLoss) {
    const FMythicEntityPresentationInstance PreviousFocus = FocusedInstance;
    CandidateScratch.Reset();
    Observations.Reset();
    LineOfSightCache.Reset();
    GazeStates.Reset();
    RecentGazeResidencies.Reset();
    Signals.Reset();
    RevisionCandidatesUntil.Reset();
    GazeHitScratch.Reset();
    PersonalSpaceOverlapScratch.Reset();
    SpatialActorScratch.Reset();
    SafetyCriticalStatusCache.Reset();
    InteractionTarget.Reset();
    HardTarget.Reset();
    InspectTarget.Reset();
    FocusedInstance.Reset();
    PendingFocusInstance.Reset();
    PendingFocusSinceSeconds = 0.0;
    LastFocusedEligibleSeconds = 0.0;
    LastDecisionSeconds = -DBL_MAX;
    bImmediateRefreshRequested = true;

    if (bBroadcastFocusLoss && PreviousFocus.IsValid()) {
        OnFocusedEntityChanged.Broadcast(PreviousFocus, FocusedInstance);
        ++AttentionRevision;
        OnAttentionUpdated.Broadcast(AttentionRevision);
    }
}

void UMythicEntityAttentionSubsystem::RunDecisionPass(
    const double NowSeconds) {
    APlayerController *Controller = nullptr;
    FVector ViewLocation = FVector::ZeroVector;
    FVector ViewForward = FVector::ForwardVector;
    FVector2D ViewportSize = FVector2D::ZeroVector;
    if (!ResolveLocalView(Controller, ViewLocation, ViewForward,
                          ViewportSize)) {
        CandidateScratch.Reset();
        Observations.Reset();
        if (FocusedInstance.IsValid()) {
            SetFocusedInstance(FMythicEntityPresentationInstance(),
                               NowSeconds);
        }
        LastDecisionSeconds = NowSeconds;
        bImmediateRefreshRequested = false;
        ++AttentionRevision;
        OnAttentionUpdated.Broadcast(AttentionRevision);
        return;
    }

    PurgeExpiredRuntimeState(NowSeconds);
    CandidateScratch.Reset();

    // Explicit ownership and active event edges are resolved directly; neither depends on collision discovery.
    EnsureExplicitCandidate(InspectTarget, ViewLocation, ViewForward,
                            NowSeconds);
    EnsureExplicitCandidate(HardTarget, ViewLocation, ViewForward,
                            NowSeconds);
    EnsureExplicitCandidate(InteractionTarget, ViewLocation, ViewForward,
                            NowSeconds);
    EnsureExplicitCandidate(FocusedInstance, ViewLocation, ViewForward,
                            NowSeconds);
    GatherEventCandidates(ViewLocation, ViewForward, NowSeconds);
    GatherRecentGazeCandidates(ViewLocation, ViewForward, NowSeconds);

    // Ordinary discovery is local and spatial: a narrow gaze corridor plus personal space, followed by a fixed
    // round-robin compatibility seed for registered subjects those probes missed. The bounded recent-gaze working set
    // absorbs cursor gaps, and there is never a per-pass registry census.
    GatherSpatialCandidates(*Controller, ViewLocation, ViewForward,
                            NowSeconds);
    GatherRegistryFallbackCandidates(ViewLocation, ViewForward, NowSeconds);

    const APawn *ViewerPawn = Controller->GetPawn();
    CandidateScratch.RemoveAllSwap(
        [ViewerPawn](const FCandidate &Candidate) {
            return Candidate.Actor.Get() == ViewerPawn;
        }, EAllowShrinking::No);

    CandidateScratch.Sort(
        [](const FCandidate &Left, const FCandidate &Right) {
            return IsCandidateHigherPriority(Left, Right);
        });
    ProjectAndSampleCandidates(*Controller, ViewLocation, ViewportSize,
                               NowSeconds);
    UpdateRecentGazeResidencies(NowSeconds);
    UpdateStableFocus(NowSeconds);
    PublishObservations();

    LastDecisionSeconds = NowSeconds;
    bImmediateRefreshRequested = false;
}

bool UMythicEntityAttentionSubsystem::ResolveLocalView(
    APlayerController *&OutController, FVector &OutViewLocation,
    FVector &OutViewForward, FVector2D &OutViewportSize) const {
    OutController = nullptr;
    OutViewLocation = FVector::ZeroVector;
    OutViewForward = FVector::ForwardVector;
    OutViewportSize = FVector2D::ZeroVector;

    ULocalPlayer *LocalPlayer = GetLocalPlayer();
    UWorld *World = LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
    APlayerController *Controller =
        LocalPlayer && World ? LocalPlayer->GetPlayerController(World) : nullptr;
    if (!Controller || !Controller->IsLocalController()) {
        return false;
    }

    FRotator ViewRotation = FRotator::ZeroRotator;
    Controller->GetPlayerViewPoint(OutViewLocation, ViewRotation);
    if (OutViewLocation.ContainsNaN() || ViewRotation.ContainsNaN()) {
        return false;
    }
    OutViewForward = ViewRotation.Vector().GetSafeNormal();
    if (OutViewForward.IsNearlyZero()) {
        return false;
    }

    if (LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->Viewport) {
        const FIntPoint FullViewportSize =
            LocalPlayer->ViewportClient->Viewport->GetSizeXY();
        OutViewportSize = FVector2D(
            static_cast<float>(FullViewportSize.X) * LocalPlayer->Size.X,
            static_cast<float>(FullViewportSize.Y) * LocalPlayer->Size.Y);
    }
    if (OutViewportSize.X <= 0.0f || OutViewportSize.Y <= 0.0f) {
        int32 Width = 0;
        int32 Height = 0;
        Controller->GetViewportSize(Width, Height);
        OutViewportSize = FVector2D(static_cast<float>(Width),
                                    static_cast<float>(Height));
    }

    OutController = Controller;
    return true;
}

void UMythicEntityAttentionSubsystem::GatherEventCandidates(
    const FVector &ViewLocation, const FVector &ViewForward,
    const double NowSeconds) {
    int32 RemainingBudget = Config.MaxRetainedEventSubjects;
    const auto ConsiderInstance =
        [this, &ViewLocation, &ViewForward, NowSeconds, &RemainingBudget](
            const FMythicEntityPresentationInstance &Instance) {
            if (RemainingBudget <= 0 || !Instance.IsValid()
                || FindCandidate(Instance)) {
                return;
            }

            UMythicEntityPresentationComponent *Component =
                RegisteredSubjects.FindRef(Instance).Get();
            FCandidate Candidate;
            if (BuildCandidate(Instance, Component, ViewLocation, ViewForward,
                               NowSeconds, Candidate)) {
                ConsiderCandidate(Candidate);
            }
            --RemainingBudget;
        };

    // Authored event signals are deliberate and take precedence over generic revision edges.
    for (const TPair<FMythicEntityPresentationInstance, FSignalState> &Pair : Signals) {
        if (RemainingBudget <= 0) {
            break;
        }
        if (!Pair.Value.IsExpired(NowSeconds)) {
            ConsiderInstance(Pair.Key);
        }
    }

    for (const TPair<FMythicEntityPresentationInstance, double> &Pair :
         RevisionCandidatesUntil) {
        if (RemainingBudget <= 0) {
            break;
        }
        if (Pair.Value > NowSeconds) {
            ConsiderInstance(Pair.Key);
        }
    }
}

void UMythicEntityAttentionSubsystem::GatherRecentGazeCandidates(
    const FVector &ViewLocation, const FVector &ViewForward,
    const double NowSeconds) {
    for (auto It = RecentGazeResidencies.CreateIterator(); It; ++It) {
        const FMythicEntityPresentationInstance Instance = It.Key();
        UMythicEntityPresentationComponent *Component =
            It.Value().Component.Get();
        if (!IsValid(Component) || !IsRegisteredInstance(Instance)
            || !Component->RepresentsInstance(Instance)) {
            It.RemoveCurrent();
            continue;
        }
        if (FindCandidate(Instance)) {
            continue;
        }

        FCandidate Candidate;
        if (!BuildCandidate(Instance, Component, ViewLocation, ViewForward,
                            NowSeconds, Candidate)) {
            It.RemoveCurrent();
            continue;
        }
        Candidate.bRecentGazeResident = true;
        ConsiderCandidate(Candidate);
    }
}

void UMythicEntityAttentionSubsystem::GatherSpatialCandidates(
    APlayerController &Controller, const FVector &ViewLocation,
    const FVector &ViewForward, const double NowSeconds) {
    UWorld *World = Controller.GetWorld();
    if (!World) {
        return;
    }

    GazeHitScratch.Reset();
    PersonalSpaceOverlapScratch.Reset();
    SpatialActorScratch.Reset();

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(MythicEntityAttentionSpatial), false);
    if (const APawn *ViewerPawn = Controller.GetPawn()) {
        QueryParams.AddIgnoredActor(ViewerPawn);
    }
    const FCollisionObjectQueryParams ObjectParams(
        FCollisionObjectQueryParams::InitType::AllObjects);

    int32 RemainingBudget = Config.MaxSpatialCandidatesPerPass;
    const auto ConsumeActor =
        [this, &ViewLocation, &ViewForward, NowSeconds, &RemainingBudget](
            AActor *Actor) {
            if (RemainingBudget <= 0 || !IsValid(Actor)
                || SpatialActorScratch.Contains(Actor)) {
                return;
            }
            SpatialActorScratch.Add(Actor);
            --RemainingBudget;
            ConsiderActorCandidate(Actor, ViewLocation, ViewForward,
                                   NowSeconds);
        };

    const float SweepDistance = Config.MaximumAttentionDistanceCentimeters
        + Config.DistanceDeadbandCentimeters;
    if (SweepDistance > 0.0f && Config.GazeProbeRadiusCentimeters > 0.0f) {
        const FVector SweepEnd = ViewLocation + ViewForward * SweepDistance;
        World->SweepMultiByObjectType(
            GazeHitScratch, ViewLocation, SweepEnd, FQuat::Identity,
            ObjectParams,
            FCollisionShape::MakeSphere(Config.GazeProbeRadiusCentimeters),
            QueryParams);
        for (const FHitResult &Hit : GazeHitScratch) {
            ConsumeActor(Hit.GetActor());
            if (RemainingBudget <= 0) {
                return;
            }
        }
    }

    const float PersonalSpaceRadius =
        Config.AmbientPersonalSpaceDistanceCentimeters
        + Config.DistanceDeadbandCentimeters;
    if (PersonalSpaceRadius <= 0.0f || RemainingBudget <= 0) {
        return;
    }

    const APawn *ViewerPawn = Controller.GetPawn();
    const FVector PersonalSpaceCenter = ViewerPawn
        ? ViewerPawn->GetActorLocation()
        : ViewLocation;
    World->OverlapMultiByObjectType(
        PersonalSpaceOverlapScratch, PersonalSpaceCenter, FQuat::Identity,
        ObjectParams, FCollisionShape::MakeSphere(PersonalSpaceRadius),
        QueryParams);
    for (const FOverlapResult &Overlap : PersonalSpaceOverlapScratch) {
        ConsumeActor(Overlap.GetActor());
        if (RemainingBudget <= 0) {
            break;
        }
    }
}

void UMythicEntityAttentionSubsystem::GatherRegistryFallbackCandidates(
    const FVector &ViewLocation, const FVector &ViewForward,
    const double NowSeconds) {
    const int32 SubjectCount = RegisteredSubjectOrder.Num();
    if (SubjectCount <= 0 || Config.MaxRegistryFallbackChecksPerPass <= 0) {
        RegistryFallbackCursor = 0;
        return;
    }

    const int32 Checks = FMath::Min(
        SubjectCount, Config.MaxRegistryFallbackChecksPerPass);
    for (int32 CheckIndex = 0; CheckIndex < Checks; ++CheckIndex) {
        if (RegistryFallbackCursor >= SubjectCount) {
            RegistryFallbackCursor = 0;
        }
        const FMythicEntityPresentationInstance Instance =
            RegisteredSubjectOrder[RegistryFallbackCursor++];
        if (!Instance.IsValid() || FindCandidate(Instance)) {
            continue;
        }

        UMythicEntityPresentationComponent *Component =
            RegisteredSubjects.FindRef(Instance).Get();
        FCandidate Candidate;
        if (BuildCandidate(Instance, Component, ViewLocation, ViewForward,
                           NowSeconds, Candidate)) {
            // The registry cursor only seeds the bounded recent-gaze working set. A qualifying subject is retained and
            // re-evaluated every subsequent decision pass instead of inheriting the cursor's intermittent cadence.
            ConsiderCandidate(Candidate);
        }
    }
}

bool UMythicEntityAttentionSubsystem::ConsiderActorCandidate(
    AActor *Actor, const FVector &ViewLocation, const FVector &ViewForward,
    const double NowSeconds) {
    if (!IsValid(Actor)) {
        return false;
    }

    UMythicEntityPresentationComponent *Component =
        Actor->FindComponentByClass<UMythicEntityPresentationComponent>();
    if (!IsValid(Component)) {
        return false;
    }

    const FMythicEntityPresentationInstance Instance =
        Component->GetPresentationInstance();
    if (!IsRegisteredInstance(Instance)) {
        return false;
    }

    FCandidate Candidate;
    if (!BuildCandidate(Instance, Component, ViewLocation, ViewForward,
                        NowSeconds, Candidate)) {
        return false;
    }
    ConsiderCandidate(Candidate);
    return true;
}

void UMythicEntityAttentionSubsystem::RememberRevisionCandidate(
    const FMythicEntityPresentationInstance &Instance,
    const double NowSeconds) {
    if (!Instance.IsValid()) {
        return;
    }

    RevisionCandidatesUntil.Add(Instance, NowSeconds + 1.0);
    while (RevisionCandidatesUntil.Num() > Config.MaxRetainedEventSubjects) {
        FMythicEntityPresentationInstance EarliestInstance;
        double EarliestExpiry = DBL_MAX;
        for (const TPair<FMythicEntityPresentationInstance, double> &Pair :
             RevisionCandidatesUntil) {
            if (Pair.Key != Instance && Pair.Value < EarliestExpiry) {
                EarliestInstance = Pair.Key;
                EarliestExpiry = Pair.Value;
            }
        }
        if (!EarliestInstance.IsValid()) {
            break;
        }
        RevisionCandidatesUntil.Remove(EarliestInstance);
    }
}

bool UMythicEntityAttentionSubsystem::ReserveSignalSlot(
    const FMythicEntityPresentationInstance &Instance,
    const EMythicEntityAttentionSignalKind SignalKind,
    const double NowSeconds) {
    if (Signals.Contains(Instance)) {
        return true;
    }

    for (auto It = Signals.CreateIterator(); It; ++It) {
        if (It.Value().IsExpired(NowSeconds)) {
            It.RemoveCurrent();
        }
    }

    for (auto It = RevisionCandidatesUntil.CreateIterator(); It; ++It) {
        if (It.Value() <= NowSeconds || !IsRegisteredInstance(It.Key())) {
            It.RemoveCurrent();
        }
    }
    if (Signals.Num() < Config.MaxRetainedEventSubjects) {
        return true;
    }

    int32 NewRank = 0;
    switch (SignalKind) {
    case EMythicEntityAttentionSignalKind::Safety:
    case EMythicEntityAttentionSignalKind::Combat:
    case EMythicEntityAttentionSignalKind::CombatFromSubjectToViewer:
        NewRank = 3;
        break;
    case EMythicEntityAttentionSignalKind::Opportunity:
    case EMythicEntityAttentionSignalKind::Action:
        NewRank = 2;
        break;
    case EMythicEntityAttentionSignalKind::Awareness:
        NewRank = 1;
        break;
    default:
        return false;
    }

    FMythicEntityPresentationInstance EvictionCandidate;
    int32 EvictionRank = MAX_int32;
    double EvictionExpiry = DBL_MAX;
    for (const TPair<FMythicEntityPresentationInstance, FSignalState> &Pair :
         Signals) {
        const int32 Rank = GetSignalPriorityRank(Pair.Value, NowSeconds);
        const double Expiry = GetSignalLatestExpiry(Pair.Value);
        if (Rank < EvictionRank
            || (Rank == EvictionRank && Expiry < EvictionExpiry)) {
            EvictionCandidate = Pair.Key;
            EvictionRank = Rank;
            EvictionExpiry = Expiry;
        }
    }
    if (!EvictionCandidate.IsValid() || NewRank < EvictionRank) {
        return false;
    }

    Signals.Remove(EvictionCandidate);
    return true;
}

int32 UMythicEntityAttentionSubsystem::GetSignalPriorityRank(
    const FSignalState &State, const double NowSeconds) {
    if (State.SafetyUntilSeconds > NowSeconds
        || State.CombatUntilSeconds > NowSeconds
        || State.IncomingCombatUntilSeconds > NowSeconds) {
        return 3;
    }
    if (State.OpportunityUntilSeconds > NowSeconds
        || State.ActionUntilSeconds > NowSeconds) {
        return 2;
    }
    return State.AwarenessUntilSeconds > NowSeconds ? 1 : 0;
}

double UMythicEntityAttentionSubsystem::GetSignalLatestExpiry(
    const FSignalState &State) {
    return FMath::Max(
        FMath::Max(State.AwarenessUntilSeconds,
                   State.OpportunityUntilSeconds),
        FMath::Max(
            FMath::Max(State.SafetyUntilSeconds, State.CombatUntilSeconds),
            FMath::Max(State.IncomingCombatUntilSeconds,
                       State.ActionUntilSeconds)));
}

bool UMythicEntityAttentionSubsystem::BuildCandidate(
    const FMythicEntityPresentationInstance &Instance,
    UMythicEntityPresentationComponent *Component,
    const FVector &ViewLocation, const FVector &ViewForward,
    const double NowSeconds, FCandidate &OutCandidate) {
    if (!Instance.IsValid() || !IsValid(Component)
        || !Component->RepresentsInstance(Instance)) {
        return false;
    }

    AActor *Actor = Component->GetOwner();
    if (!IsValid(Actor) || Actor->IsHidden()) {
        return false;
    }

    const FVector AnchorLocation =
        Component->GetPresentationAnchorWorldLocation();
    if (AnchorLocation.ContainsNaN()) {
        return false;
    }
    const FVector ToSubject = AnchorLocation - ViewLocation;
    const float Distance = ToSubject.Size();
    if (!FMath::IsFinite(Distance)) {
        return false;
    }
    const bool bInteractionTarget = InteractionTarget == Instance;
    const bool bHardTarget = HardTarget == Instance;
    const bool bInspectTarget = InspectTarget == Instance;
    const bool bCurrentFocus = FocusedInstance == Instance;
    const bool bExplicit = bInteractionTarget || bHardTarget || bInspectTarget;
    if (Distance > Config.MaximumAttentionDistanceCentimeters
            + Config.DistanceDeadbandCentimeters
        && !bExplicit && !bCurrentFocus) {
        return false;
    }

    OutCandidate.Instance = Instance;
    OutCandidate.Component = Component;
    OutCandidate.Actor = Actor;
    OutCandidate.AnchorWorldLocation = AnchorLocation;
    OutCandidate.DistanceCentimeters = Distance;
    OutCandidate.ViewAlignment = Distance > UE_SMALL_NUMBER
        ? FVector::DotProduct(ViewForward, ToSubject / Distance)
        : 1.0f;
    OutCandidate.bInteractionTarget = bInteractionTarget;
    OutCandidate.bHardTarget = bHardTarget;
    OutCandidate.bInspectTarget = bInspectTarget;
    OutCandidate.bWithinAmbientRange =
        Distance <= Config.AmbientPersonalSpaceDistanceCentimeters;
    OutCandidate.bProtectedCandidate = bExplicit || bCurrentFocus;
    OutCandidate.bRecentGazeResident =
        RecentGazeResidencies.Contains(Instance);

    if (const FSignalState *Signal = Signals.Find(Instance)) {
        OutCandidate.bAwarenessSignal =
            Signal->AwarenessUntilSeconds > NowSeconds;
        OutCandidate.bOpportunitySignal =
            Signal->OpportunityUntilSeconds > NowSeconds
            || Signal->ActionUntilSeconds > NowSeconds;
        OutCandidate.bSafetySignal =
            Signal->SafetyUntilSeconds > NowSeconds
            || Signal->CombatUntilSeconds > NowSeconds
            || Signal->IncomingCombatUntilSeconds > NowSeconds;
        OutCandidate.bRecentCombatSignal =
            Signal->CombatUntilSeconds > NowSeconds;
        OutCandidate.bRecentIncomingCombatSignal =
            Signal->IncomingCombatUntilSeconds > NowSeconds;
        OutCandidate.bRecentActionSignal =
            Signal->ActionUntilSeconds > NowSeconds;

        if (OutCandidate.bSafetySignal) {
            OutCandidate.SignalStrength = FMath::Max(
                Signal->SafetyUntilSeconds > NowSeconds
                    ? Signal->SafetyStrength
                    : 0.0f,
                FMath::Max(
                    Signal->CombatUntilSeconds > NowSeconds
                        ? Signal->CombatStrength
                        : 0.0f,
                    Signal->IncomingCombatUntilSeconds > NowSeconds
                        ? Signal->IncomingCombatStrength
                        : 0.0f));
        } else if (OutCandidate.bOpportunitySignal) {
            OutCandidate.SignalStrength = FMath::Max(
                Signal->OpportunityUntilSeconds > NowSeconds
                    ? Signal->OpportunityStrength
                    : 0.0f,
                Signal->ActionUntilSeconds > NowSeconds
                    ? Signal->ActionStrength
                    : 0.0f);
        } else if (OutCandidate.bAwarenessSignal) {
            OutCandidate.SignalStrength = Signal->AwarenessStrength;
        }
    }

    // Public facts are already executed, redacted, embodiment-stamped state. Reading them here only protects relevant
    // candidates from the bounded crowd cut; final nameplate policy still decides disclosure and headline precedence.
    for (const FMythicObservableFactItem &Fact :
         Component->GetObservableFactsView()) {
        const bool bCurrentRow = Fact.Subject == Instance.Handle
            && Fact.EmbodimentGeneration == Instance.EmbodimentGeneration
            && Fact.Revision != 0;
        if (!bCurrentRow) {
            continue;
        }

        if (Fact.ValueTag
                == MythicEntityPresentationTags::ObservableLifeDowned
            || Fact.ValueTag
                == MythicEntityPresentationTags::ObservableLifeDying) {
            OutCandidate.bPublicSafetyEvidence = true;
        } else if (
            Fact.ValueTag
                == MythicEntityPresentationTags::ObservableBehaviorFighting
            || Fact.ValueTag
                == MythicEntityPresentationTags::ObservableBehaviorFleeing
            || Fact.ValueTag
                == MythicEntityPresentationTags::ObservableBehaviorSurrendering) {
            OutCandidate.bPublicAwarenessEvidence = true;
        }
    }

    for (const FMythicPublicStatusPresentationItem &Status :
         Component->GetPublicStatusesView()) {
        const bool bCurrentRow = Status.Subject == Instance.Handle
            && Status.EmbodimentGeneration == Instance.EmbodimentGeneration
            && Status.Revision != 0;
        if (bCurrentRow && IsSafetyCriticalStatus(Status.StatusType)) {
            OutCandidate.bSafetyCriticalStatus = true;
            break;
        }
    }

    if (OutCandidate.bPublicSafetyEvidence
        || OutCandidate.bSafetyCriticalStatus
        || OutCandidate.bPublicAwarenessEvidence) {
        OutCandidate.SignalStrength = 1.0f;
    }

    if (bHardTarget || OutCandidate.bSafetySignal
        || OutCandidate.bPublicSafetyEvidence
        || OutCandidate.bSafetyCriticalStatus) {
        OutCandidate.PriorityClass =
            EMythicEntityAttentionPriorityClass::Safety;
    } else if (bInteractionTarget || OutCandidate.bOpportunitySignal) {
        OutCandidate.PriorityClass =
            EMythicEntityAttentionPriorityClass::Opportunity;
    } else if (bInspectTarget || OutCandidate.bAwarenessSignal
               || OutCandidate.bPublicAwarenessEvidence) {
        OutCandidate.PriorityClass =
            EMythicEntityAttentionPriorityClass::Awareness;
    }

    FMythicEntityAttentionScoreInput ScoreInput;
    ScoreInput.DistanceCentimeters = OutCandidate.DistanceCentimeters;
    ScoreInput.ViewAlignment = OutCandidate.ViewAlignment;
    ScoreInput.SignalStrength = OutCandidate.SignalStrength;
    ScoreInput.PriorityClass = OutCandidate.PriorityClass;
    ScoreInput.bOnScreen = OutCandidate.ViewAlignment > 0.0f;
    ScoreInput.bInteractionTarget = bInteractionTarget;
    ScoreInput.bHardTarget = bHardTarget;
    ScoreInput.bInspectTarget = bInspectTarget;
    OutCandidate.Score =
        FMythicEntityAttentionRules::CalculateScore(ScoreInput, Config);
    return true;
}

void UMythicEntityAttentionSubsystem::ConsiderCandidate(
    const FCandidate &Candidate) {
    if (FindCandidate(Candidate.Instance)) {
        return;
    }
    if (CandidateScratch.Num() < Config.MaxEvaluatedCandidates) {
        CandidateScratch.Add(Candidate);
        return;
    }

    int32 WorstReplaceableIndex = INDEX_NONE;
    for (int32 Index = 0; Index < CandidateScratch.Num(); ++Index) {
        if (CandidateScratch[Index].bProtectedCandidate) {
            continue;
        }
        if (WorstReplaceableIndex == INDEX_NONE
            || IsCandidateHigherPriority(
                CandidateScratch[WorstReplaceableIndex],
                CandidateScratch[Index])) {
            WorstReplaceableIndex = Index;
        }
    }
    if (WorstReplaceableIndex == INDEX_NONE) {
        return;
    }
    if (Candidate.bProtectedCandidate
        || IsCandidateHigherPriority(
            Candidate, CandidateScratch[WorstReplaceableIndex])) {
        CandidateScratch[WorstReplaceableIndex] = Candidate;
    }
}

void UMythicEntityAttentionSubsystem::EnsureExplicitCandidate(
    const FMythicEntityPresentationInstance &Instance,
    const FVector &ViewLocation, const FVector &ViewForward,
    const double NowSeconds) {
    if (!Instance.IsValid() || FindCandidate(Instance)) {
        return;
    }
    UMythicEntityPresentationRegistry *Registry = BoundRegistry.Get();
    UMythicEntityPresentationComponent *Component =
        Registry ? Registry->ResolvePresentationComponent(Instance) : nullptr;
    FCandidate Candidate;
    if (BuildCandidate(Instance, Component, ViewLocation, ViewForward,
                       NowSeconds, Candidate)) {
        Candidate.bProtectedCandidate = true;
        ConsiderCandidate(Candidate);
    }
}

void UMythicEntityAttentionSubsystem::ProjectAndSampleCandidates(
    APlayerController &Controller, const FVector &ViewLocation,
    const FVector2D &ViewportSize, const double NowSeconds) {
    FMythicEntityAttentionTraceBudget TraceBudget(
        Config.MaxLineOfSightTracesPerPass);
    const float Padding = Config.ScreenEdgePaddingPixels;

    for (FCandidate &Candidate : CandidateScratch) {
        FVector2D ScreenPosition = FVector2D::ZeroVector;
        const bool bProjected = Controller.ProjectWorldLocationToScreen(
            Candidate.AnchorWorldLocation, ScreenPosition, true);
        if (ScreenPosition.ContainsNaN()) {
            ScreenPosition = FVector2D::ZeroVector;
        }
        Candidate.ScreenPosition = ScreenPosition;
        Candidate.bOnScreen = bProjected
            && Candidate.ViewAlignment > 0.0f
            && ScreenPosition.X >= -Padding
            && ScreenPosition.Y >= -Padding
            && (ViewportSize.X <= 0.0f
                || ScreenPosition.X <= ViewportSize.X + Padding)
            && (ViewportSize.Y <= 0.0f
                || ScreenPosition.Y <= ViewportSize.Y + Padding);

        Candidate.bHasLineOfSight = ResolveLineOfSight(
            Candidate, Controller, ViewLocation, NowSeconds, TraceBudget,
            Candidate.bLineOfSightDeferred);
        const bool bRetainingRecentGaze = Candidate.bRecentGazeResident
            || GazeStates.Contains(Candidate.Instance);
        const float GazeMinimumViewDot =
            FMythicEntityAttentionRules::GetGazeMinimumViewDot(
                bRetainingRecentGaze, Config);
        const float GazeMaximumDistance =
            Config.MaximumAttentionDistanceCentimeters
            + (bRetainingRecentGaze
                   ? Config.DistanceDeadbandCentimeters : 0.0f);
        Candidate.bGazeCandidate = Candidate.bOnScreen
            && Candidate.bHasLineOfSight
            && Candidate.ViewAlignment >= GazeMinimumViewDot
            && Candidate.DistanceCentimeters <= GazeMaximumDistance;
        UpdateGazeState(Candidate, NowSeconds);

        FMythicEntityAttentionScoreInput ScoreInput;
        ScoreInput.DistanceCentimeters = Candidate.DistanceCentimeters;
        ScoreInput.ViewAlignment = Candidate.ViewAlignment;
        ScoreInput.SignalStrength = Candidate.SignalStrength;
        ScoreInput.PriorityClass = Candidate.PriorityClass;
        ScoreInput.bOnScreen = Candidate.bOnScreen;
        ScoreInput.bHasLineOfSight = Candidate.bHasLineOfSight;
        ScoreInput.bInteractionTarget = Candidate.bInteractionTarget;
        ScoreInput.bHardTarget = Candidate.bHardTarget;
        ScoreInput.bInspectTarget = Candidate.bInspectTarget;
        Candidate.Score =
            FMythicEntityAttentionRules::CalculateScore(ScoreInput, Config);
    }

    CandidateScratch.Sort(
        [](const FCandidate &Left, const FCandidate &Right) {
            return IsCandidateHigherPriority(Left, Right);
        });
}

bool UMythicEntityAttentionSubsystem::ResolveLineOfSight(
    FCandidate &Candidate, APlayerController &Controller,
    const FVector &ViewLocation, const double NowSeconds,
    FMythicEntityAttentionTraceBudget &TraceBudget,
    bool &bOutDeferred) {
    bOutDeferred = false;
    FLineOfSightCacheEntry *Cached =
        LineOfSightCache.Find(Candidate.Instance);
    const float MovementInvalidationSquared = FMath::Square(
        Config.LineOfSightMovementInvalidationCentimeters);
    if (Cached
        && NowSeconds - Cached->SampleTimeSeconds
               <= Config.LineOfSightCacheLifetimeSeconds
        && FVector::DistSquared(Cached->ViewWorldLocation, ViewLocation)
               <= MovementInvalidationSquared
        && FVector::DistSquared(Cached->AnchorWorldLocation,
                                 Candidate.AnchorWorldLocation)
               <= MovementInvalidationSquared) {
        return FMythicEntityAttentionRules::UpdateStableLineOfSight(
            Cached->Visibility, Cached->Visibility.bRawHasLineOfSight,
            NowSeconds, Config);
    }

    if (!TraceBudget.TryConsume()) {
        bOutDeferred = true;
        return Cached
            && FMythicEntityAttentionRules::ShouldPreserveDeferredLineOfSight(
                Cached->Visibility,
                FMath::Max(0.0, NowSeconds - Cached->SampleTimeSeconds),
                Config);
    }

    bool bHasLineOfSight = true;
    if (!Candidate.AnchorWorldLocation.Equals(ViewLocation, 1.0f)) {
        FCollisionQueryParams QueryParams(
            SCENE_QUERY_STAT(MythicEntityAttentionLineOfSight), false);
        if (const APawn *ViewerPawn = Controller.GetPawn()) {
            QueryParams.AddIgnoredActor(ViewerPawn);
        }
        if (AActor *SubjectActor = Candidate.Actor.Get()) {
            QueryParams.AddIgnoredActor(SubjectActor);
        }
        UWorld *World = Controller.GetWorld();
        bHasLineOfSight = World
            && !World->LineTraceTestByChannel(
                ViewLocation, Candidate.AnchorWorldLocation, ECC_Visibility,
                QueryParams);
    }

    FLineOfSightCacheEntry &NewCache =
        LineOfSightCache.FindOrAdd(Candidate.Instance);
    NewCache.ViewWorldLocation = ViewLocation;
    NewCache.AnchorWorldLocation = Candidate.AnchorWorldLocation;
    NewCache.SampleTimeSeconds = NowSeconds;
    return FMythicEntityAttentionRules::UpdateStableLineOfSight(
        NewCache.Visibility, bHasLineOfSight, NowSeconds, Config);
}

void UMythicEntityAttentionSubsystem::UpdateGazeState(
    FCandidate &Candidate, const double NowSeconds) {
    FGazeTemporalState *Existing = GazeStates.Find(Candidate.Instance);
    if (Candidate.bLineOfSightDeferred) {
        if (Candidate.bHasLineOfSight) {
            Candidate.StableGazeSeconds = Existing
                ? static_cast<float>(FMath::Max(
                      0.0, NowSeconds - Existing->EligibleSinceSeconds))
                : 0.0f;
        } else {
            GazeStates.Remove(Candidate.Instance);
            Candidate.StableGazeSeconds = 0.0f;
        }
        return;
    }

    if (!Candidate.bGazeCandidate) {
        GazeStates.Remove(Candidate.Instance);
        Candidate.StableGazeSeconds = 0.0f;
        return;
    }

    if (!Existing) {
        FGazeTemporalState &State = GazeStates.Add(
            Candidate.Instance, FGazeTemporalState());
        State.EligibleSinceSeconds = NowSeconds;
        State.LastEligibleSeconds = NowSeconds;
        Candidate.StableGazeSeconds = 0.0f;
        return;
    }

    const double MaximumExpectedGap =
        FMath::Max(0.25, 2.5 / FMath::Max(1.0f, Config.DecisionRateHz));
    if (NowSeconds - Existing->LastEligibleSeconds > MaximumExpectedGap) {
        Existing->EligibleSinceSeconds = NowSeconds;
    }
    Existing->LastEligibleSeconds = NowSeconds;
    Candidate.StableGazeSeconds = static_cast<float>(
        FMath::Max(0.0, NowSeconds - Existing->EligibleSinceSeconds));
}

void UMythicEntityAttentionSubsystem::UpdateRecentGazeResidencies(
    const double NowSeconds) {
    for (const FCandidate &Candidate : CandidateScratch) {
        if (Candidate.bGazeCandidate && Candidate.bOnScreen
            && Candidate.Component.IsValid()) {
            RefreshRecentGazeResidency(Candidate, NowSeconds);
        }
    }

    for (auto It = RecentGazeResidencies.CreateIterator(); It; ++It) {
        const FMythicEntityPresentationInstance Instance = It.Key();
        UMythicEntityPresentationComponent *Component =
            It.Value().Component.Get();
        if (!IsValid(Component) || !IsRegisteredInstance(Instance)
            || !Component->RepresentsInstance(Instance)) {
            It.RemoveCurrent();
            continue;
        }

        const FCandidate *Candidate = FindCandidate(Instance);
        if (Candidate && Candidate->bGazeCandidate
            && Candidate->bOnScreen) {
            continue;
        }
        if (!FMythicEntityAttentionRules::CanRetainRecentGaze(
                FMath::Max(0.0,
                           NowSeconds - It.Value().LastEligibleSeconds),
                Config)) {
            It.RemoveCurrent();
        }
    }
    TrimRecentGazeResidencies();
}

void UMythicEntityAttentionSubsystem::RefreshRecentGazeResidency(
    const FCandidate &Candidate, const double NowSeconds) {
    if (!Candidate.Instance.IsValid() || !Candidate.Component.IsValid()
        || !FMath::IsFinite(NowSeconds)) {
        return;
    }

    if (FRecentGazeResidency *Existing =
            RecentGazeResidencies.Find(Candidate.Instance)) {
        Existing->Component = Candidate.Component;
        Existing->LastEligibleSeconds = NowSeconds;
        Existing->LastScore = Candidate.Score;
        return;
    }

    const int32 MaximumResidents = FMath::Max(
        1, Config.MaxEvaluatedCandidates);
    if (RecentGazeResidencies.Num() >= MaximumResidents) {
        FMythicEntityPresentationInstance WorstInstance;
        const FRecentGazeResidency *WorstResidency = nullptr;
        for (const TPair<FMythicEntityPresentationInstance,
                         FRecentGazeResidency> &Pair :
             RecentGazeResidencies) {
            const bool bWorseScore = !WorstResidency
                || Pair.Value.LastScore < WorstResidency->LastScore;
            const bool bEqualScore = WorstResidency
                && FMath::IsNearlyEqual(Pair.Value.LastScore,
                                        WorstResidency->LastScore);
            const bool bOlder = bEqualScore
                && Pair.Value.LastEligibleSeconds
                       < WorstResidency->LastEligibleSeconds;
            const bool bStableTieBreak = bEqualScore
                && Pair.Value.LastEligibleSeconds
                       == WorstResidency->LastEligibleSeconds
                && GetTypeHash(Pair.Key) > GetTypeHash(WorstInstance);
            if (bWorseScore || bOlder || bStableTieBreak) {
                WorstInstance = Pair.Key;
                WorstResidency = &Pair.Value;
            }
        }
        if (!WorstResidency
            || Candidate.Score <= WorstResidency->LastScore) {
            return;
        }
        RecentGazeResidencies.Remove(WorstInstance);
    }

    FRecentGazeResidency &Residency =
        RecentGazeResidencies.Add(Candidate.Instance);
    Residency.Component = Candidate.Component;
    Residency.LastEligibleSeconds = NowSeconds;
    Residency.LastScore = Candidate.Score;
}

void UMythicEntityAttentionSubsystem::TrimRecentGazeResidencies() {
    const int32 MaximumResidents = FMath::Max(
        1, Config.MaxEvaluatedCandidates);
    while (RecentGazeResidencies.Num() > MaximumResidents) {
        FMythicEntityPresentationInstance WorstInstance;
        const FRecentGazeResidency *WorstResidency = nullptr;
        for (const TPair<FMythicEntityPresentationInstance,
                         FRecentGazeResidency> &Pair :
             RecentGazeResidencies) {
            const bool bWorseScore = !WorstResidency
                || Pair.Value.LastScore < WorstResidency->LastScore;
            const bool bEqualScore = WorstResidency
                && FMath::IsNearlyEqual(Pair.Value.LastScore,
                                        WorstResidency->LastScore);
            const bool bOlder = bEqualScore
                && Pair.Value.LastEligibleSeconds
                       < WorstResidency->LastEligibleSeconds;
            const bool bStableTieBreak = bEqualScore
                && Pair.Value.LastEligibleSeconds
                       == WorstResidency->LastEligibleSeconds
                && GetTypeHash(Pair.Key) > GetTypeHash(WorstInstance);
            if (bWorseScore || bOlder || bStableTieBreak) {
                WorstInstance = Pair.Key;
                WorstResidency = &Pair.Value;
            }
        }
        if (!WorstResidency) {
            RecentGazeResidencies.Reset();
            return;
        }
        RecentGazeResidencies.Remove(WorstInstance);
    }
}

void UMythicEntityAttentionSubsystem::UpdateStableFocus(
    const double NowSeconds) {
    FCandidate *ForcedCandidate = nullptr;
    if (InspectTarget.IsValid()) {
        ForcedCandidate = FindCandidate(InspectTarget);
    }
    if (!ForcedCandidate && HardTarget.IsValid()) {
        ForcedCandidate = FindCandidate(HardTarget);
    }
    if (!ForcedCandidate && InteractionTarget.IsValid()) {
        ForcedCandidate = FindCandidate(InteractionTarget);
    }
    if (ForcedCandidate) {
        SetFocusedInstance(ForcedCandidate->Instance, NowSeconds);
        return;
    }

    FCandidate *BestOrdinaryCandidate = nullptr;
    for (FCandidate &Candidate : CandidateScratch) {
        if (Candidate.bOnScreen && Candidate.bHasLineOfSight
            && Candidate.ViewAlignment >= Config.FocusMinimumViewDot) {
            BestOrdinaryCandidate = &Candidate;
            break;
        }
    }

    if (FocusedInstance.IsValid()) {
        FCandidate *Incumbent = FindCandidate(FocusedInstance);
        if (!Incumbent) {
            SetFocusedInstance(FMythicEntityPresentationInstance(),
                               NowSeconds);
        } else {
            const bool bIncumbentEligible = Incumbent->bOnScreen
                && Incumbent->bHasLineOfSight
                && Incumbent->ViewAlignment >= Config.FocusMinimumViewDot;
            if (bIncumbentEligible) {
                LastFocusedEligibleSeconds = NowSeconds;
                if (!BestOrdinaryCandidate
                    || BestOrdinaryCandidate->Instance == FocusedInstance) {
                    PendingFocusInstance.Reset();
                    PendingFocusSinceSeconds = 0.0;
                    return;
                }

                const float RequiredScore = Incumbent->Score
                    * Config.ReplacementScoreMultiplier;
                if (BestOrdinaryCandidate->Score < RequiredScore) {
                    PendingFocusInstance.Reset();
                    PendingFocusSinceSeconds = 0.0;
                    return;
                }

                if (PendingFocusInstance
                    != BestOrdinaryCandidate->Instance) {
                    PendingFocusInstance = BestOrdinaryCandidate->Instance;
                    PendingFocusSinceSeconds = NowSeconds;
                    return;
                }

                if (FMythicEntityAttentionRules::ShouldReplaceFocus(
                        Incumbent->Score, BestOrdinaryCandidate->Score,
                        static_cast<float>(NowSeconds
                                           - PendingFocusSinceSeconds),
                        false, Config)) {
                    SetFocusedInstance(BestOrdinaryCandidate->Instance,
                                       NowSeconds);
                }
                return;
            }

            if (FMythicEntityAttentionRules::CanRetainFocus(
                    static_cast<float>(NowSeconds
                                       - LastFocusedEligibleSeconds),
                    Config)) {
                return;
            }
            SetFocusedInstance(FMythicEntityPresentationInstance(),
                               NowSeconds);
        }
    }

    if (!BestOrdinaryCandidate) {
        PendingFocusInstance.Reset();
        PendingFocusSinceSeconds = 0.0;
        return;
    }

    if (PendingFocusInstance != BestOrdinaryCandidate->Instance) {
        PendingFocusInstance = BestOrdinaryCandidate->Instance;
        PendingFocusSinceSeconds = NowSeconds;
        return;
    }

    if (FMythicEntityAttentionRules::CanAcquireFocus(
            static_cast<float>(NowSeconds - PendingFocusSinceSeconds), false,
            Config)) {
        SetFocusedInstance(BestOrdinaryCandidate->Instance, NowSeconds);
    }
}

void UMythicEntityAttentionSubsystem::PublishObservations() {
    CandidateScratch.Sort(
        [this](const FCandidate &Left, const FCandidate &Right) {
            const bool bLeftFocused = Left.Instance == FocusedInstance;
            const bool bRightFocused = Right.Instance == FocusedInstance;
            if (bLeftFocused != bRightFocused) {
                return bLeftFocused;
            }
            return IsCandidateHigherPriority(Left, Right);
        });

    Observations.Reset();
    for (const FCandidate &Candidate : CandidateScratch) {
        UMythicEntityPresentationComponent *Component =
            Candidate.Component.Get();
        if (!Component || !Component->RepresentsInstance(Candidate.Instance)) {
            continue;
        }

        const bool bFocused = Candidate.Instance == FocusedInstance;
        if (!Candidate.bOnScreen
            && Candidate.PriorityClass
                   == EMythicEntityAttentionPriorityClass::Ambient
            && !bFocused && !Candidate.bProtectedCandidate) {
            continue;
        }

        FMythicEntityAttentionObservation &Observation =
            Observations.AddDefaulted_GetRef();
        Observation.Instance = Candidate.Instance;
        Observation.PriorityClass = Candidate.PriorityClass;
        Observation.Score = Candidate.Score;
        Observation.DistanceCentimeters = Candidate.DistanceCentimeters;
        Observation.ViewAlignment = Candidate.ViewAlignment;
        Observation.ScreenPosition = Candidate.ScreenPosition;
        Observation.AnchorWorldLocation = Candidate.AnchorWorldLocation;
        Observation.StableGazeSeconds = Candidate.StableGazeSeconds;
        Observation.bOnScreen = Candidate.bOnScreen;
        Observation.bHasLineOfSight = Candidate.bHasLineOfSight;
        Observation.bFocused = bFocused;
        Observation.bGazeCandidate = Candidate.bGazeCandidate;
        Observation.bWithinAmbientRange = Candidate.bWithinAmbientRange;
        Observation.bInteractionTarget = Candidate.bInteractionTarget;
        Observation.bHardTarget = Candidate.bHardTarget;
        Observation.bInspectTarget = Candidate.bInspectTarget;
        Observation.bSafetySignal = Candidate.bSafetySignal;
        Observation.bOpportunitySignal = Candidate.bOpportunitySignal;
        Observation.bAwarenessSignal = Candidate.bAwarenessSignal;
        Observation.bRecentCombatSignal = Candidate.bRecentCombatSignal;
        Observation.bRecentIncomingCombatSignal =
            Candidate.bRecentIncomingCombatSignal;
        Observation.bRecentActionSignal = Candidate.bRecentActionSignal;
        Observation.bPublicAwarenessEvidence =
            Candidate.bPublicAwarenessEvidence;
        Observation.bPublicSafetyEvidence = Candidate.bPublicSafetyEvidence;
        Observation.bSafetyCriticalStatus =
            Candidate.bSafetyCriticalStatus;
        Observation.Actor = Candidate.Actor;
        Observation.Component = Candidate.Component;

        if (Observations.Num() >= Config.MaxPublishedObservations) {
            break;
        }
    }

    ++AttentionRevision;
    OnAttentionUpdated.Broadcast(AttentionRevision);
}

void UMythicEntityAttentionSubsystem::PurgeExpiredRuntimeState(
    const double NowSeconds) {
    for (auto It = Signals.CreateIterator(); It; ++It) {
        if (It.Value().IsExpired(NowSeconds)
            || !IsRegisteredInstance(It.Key())) {
            It.RemoveCurrent();
        }
    }

    for (auto It = RevisionCandidatesUntil.CreateIterator(); It; ++It) {
        if (It.Value() <= NowSeconds || !IsRegisteredInstance(It.Key())) {
            It.RemoveCurrent();
        }
    }

    const double GazeStaleSeconds =
        FMath::Max(0.5, 3.0 / FMath::Max(1.0f, Config.DecisionRateHz));
    for (auto It = GazeStates.CreateIterator(); It; ++It) {
        if (NowSeconds - It.Value().LastEligibleSeconds > GazeStaleSeconds
            || !IsRegisteredInstance(It.Key())) {
            It.RemoveCurrent();
        }
    }

    for (auto It = RecentGazeResidencies.CreateIterator(); It; ++It) {
        UMythicEntityPresentationComponent *Component =
            It.Value().Component.Get();
        if (!IsValid(Component) || !IsRegisteredInstance(It.Key())
            || !Component->RepresentsInstance(It.Key())) {
            It.RemoveCurrent();
        }
    }

    const double LosStaleSeconds = FMath::Max(
        1.0, static_cast<double>(Config.LineOfSightCacheLifetimeSeconds) * 8.0);
    for (auto It = LineOfSightCache.CreateIterator(); It; ++It) {
        if (NowSeconds - It.Value().SampleTimeSeconds > LosStaleSeconds
            || !IsRegisteredInstance(It.Key())) {
            It.RemoveCurrent();
        }
    }

    if (InteractionTarget.IsValid()
        && !IsRegisteredInstance(InteractionTarget)) {
        InteractionTarget.Reset();
    }
    if (HardTarget.IsValid() && !IsRegisteredInstance(HardTarget)) {
        HardTarget.Reset();
    }
    if (InspectTarget.IsValid() && !IsRegisteredInstance(InspectTarget)) {
        InspectTarget.Reset();
    }
}

void UMythicEntityAttentionSubsystem::SetFocusedInstance(
    const FMythicEntityPresentationInstance &NewFocusedInstance,
    const double NowSeconds) {
    if (FocusedInstance == NewFocusedInstance) {
        return;
    }

    const FMythicEntityPresentationInstance PreviousFocus = FocusedInstance;
    FocusedInstance = NewFocusedInstance;
    PendingFocusInstance.Reset();
    PendingFocusSinceSeconds = 0.0;
    LastFocusedEligibleSeconds = NowSeconds;
    OnFocusedEntityChanged.Broadcast(PreviousFocus, FocusedInstance);
}

UMythicEntityAttentionSubsystem::FCandidate *
UMythicEntityAttentionSubsystem::FindCandidate(
    const FMythicEntityPresentationInstance &Instance) {
    return CandidateScratch.FindByPredicate(
        [&Instance](const FCandidate &Candidate) {
            return Candidate.Instance == Instance;
        });
}

const UMythicEntityAttentionSubsystem::FCandidate *
UMythicEntityAttentionSubsystem::FindCandidate(
    const FMythicEntityPresentationInstance &Instance) const {
    return CandidateScratch.FindByPredicate(
        [&Instance](const FCandidate &Candidate) {
            return Candidate.Instance == Instance;
        });
}

bool UMythicEntityAttentionSubsystem::IsRegisteredInstance(
    const FMythicEntityPresentationInstance &Instance) const {
    if (!Instance.IsValid()) {
        return false;
    }
    const TWeakObjectPtr<UMythicEntityPresentationComponent> *Found =
        RegisteredSubjects.Find(Instance);
    UMythicEntityPresentationComponent *Component =
        Found ? Found->Get() : nullptr;
    return Component && Component->RepresentsInstance(Instance);
}

bool UMythicEntityAttentionSubsystem::IsSafetyCriticalStatus(
    const FGameplayTag StatusType) {
    if (!StatusType.IsValid()) {
        return false;
    }
    if (const bool *Cached = SafetyCriticalStatusCache.Find(StatusType)) {
        return *Cached;
    }

    UWorld *World = GetTickableGameObjectWorld();
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    UMythicStatusRegistry *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicStatusRegistry>()
        : nullptr;
    if (!Registry) {
        return false;
    }

    const UMythicStatusEffectDefinition *Definition =
        Registry->FindStatus(StatusType);
    const bool bSafetyCritical = Definition
        && Definition->WorldVisibility
               == EMythicStatusWorldVisibility::SafetyCritical
        && Definition->bPromotesContextWhenObserved;
    SafetyCriticalStatusCache.Add(StatusType, bSafetyCritical);
    return bSafetyCritical;
}

bool UMythicEntityAttentionSubsystem::IsCandidateHigherPriority(
    const FCandidate &Left, const FCandidate &Right) {
    if (Left.bProtectedCandidate != Right.bProtectedCandidate) {
        return Left.bProtectedCandidate;
    }
    const int32 LeftPriority =
        FMythicEntityAttentionRules::GetPriorityRank(Left.PriorityClass);
    const int32 RightPriority =
        FMythicEntityAttentionRules::GetPriorityRank(Right.PriorityClass);
    if (LeftPriority != RightPriority) {
        return LeftPriority > RightPriority;
    }
    if (Left.bRecentGazeResident != Right.bRecentGazeResident) {
        return Left.bRecentGazeResident;
    }
    if (Left.Score != Right.Score) {
        return Left.Score > Right.Score;
    }
    const uint32 LeftHash = GetTypeHash(Left.Instance);
    const uint32 RightHash = GetTypeHash(Right.Instance);
    if (LeftHash != RightHash) {
        return LeftHash < RightHash;
    }
    return Left.Instance.ToDebugString() < Right.Instance.ToDebugString();
}
