#include "GAS/Combat/MythicEntityCombatPresentationComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Settings/MythicCombatSettings.h"
#include "TimerManager.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythicEntityCombatPresentation, Log, All);

namespace {
EMythicCombatThreatRank ResolveThreatAssessmentRank(
    const EMythicPresentedCombatRank PresentedRank) {
    switch (PresentedRank) {
    case EMythicPresentedCombatRank::Elite:
    case EMythicPresentedCombatRank::Champion:
        return EMythicCombatThreatRank::Elite;
    case EMythicPresentedCombatRank::Boss:
        return EMythicCombatThreatRank::Boss;
    case EMythicPresentedCombatRank::WorldBoss:
        return EMythicCombatThreatRank::WorldBoss;
    case EMythicPresentedCombatRank::Unknown:
    case EMythicPresentedCombatRank::Standard:
    case EMythicPresentedCombatRank::Superior:
    default:
        return EMythicCombatThreatRank::Standard;
    }
}
}

void FMythicReplicatedEntityCombatPresentationArray::PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, const int32 FinalSize) {
    (void)RemovedIndices;
    (void)FinalSize;
    if (Owner.IsValid()) {
        Owner->QueueReplicatedRevision();
    }
}

void FMythicReplicatedEntityCombatPresentationArray::PostReplicatedAdd(const TArrayView<int32> &AddedIndices, const int32 FinalSize) {
    (void)AddedIndices;
    (void)FinalSize;
    if (Owner.IsValid()) {
        Owner->QueueReplicatedRevision();
    }
}

void FMythicReplicatedEntityCombatPresentationArray::PostReplicatedChange(const TArrayView<int32> &ChangedIndices, const int32 FinalSize) {
    (void)ChangedIndices;
    (void)FinalSize;
    if (Owner.IsValid()) {
        Owner->QueueReplicatedRevision();
    }
}

UMythicEntityCombatPresentationComponent::UMythicEntityCombatPresentationComponent() {
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
    ReplicatedPresentations.SetOwner(this);
}

void UMythicEntityCombatPresentationComponent::BeginPlay() {
    Super::BeginPlay();
    ReplicatedPresentations.SetOwner(this);
    if (IsAuthority()) {
        EnsurePresentationRegistryBinding();
        ScheduleAuthorityExpiryTimer();
    }
}

void UMythicEntityCombatPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(ExpiryTimerHandle);
    }
    RemovePresentationRegistryBinding();
    ReplicatedPresentations.SetOwner(nullptr);
    Super::EndPlay(EndPlayReason);
}

void UMythicEntityCombatPresentationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicEntityCombatPresentationComponent, ReplicatedPresentations, COND_OwnerOnly);
}

bool UMythicEntityCombatPresentationComponent::GetCombatPresentationForSubject(const FMythicEntityPresentationInstance Subject,
                                                                               FMythicReplicatedEntityCombatPresentation &OutPresentation) const {
    OutPresentation = FMythicReplicatedEntityCombatPresentation();
    const FMythicReplicatedEntityCombatPresentation *Found = FindCurrentCombatPresentation(Subject);
    if (!Found) {
        return false;
    }
    OutPresentation = *Found;
    return true;
}

int32 UMythicEntityCombatPresentationComponent::GetActiveCombatPresentationCount() const {
    const double Now = GetSynchronizedServerTimeSeconds();
    int32 Count = 0;
    for (const FMythicReplicatedEntityCombatPresentation &Presentation : ReplicatedPresentations.Items) {
        Count += Presentation.Subject.IsValid() && !Presentation.IsExpired(Now) ? 1 : 0;
    }
    return Count;
}

const FMythicReplicatedEntityCombatPresentation *
UMythicEntityCombatPresentationComponent::FindCurrentCombatPresentation(const FMythicEntityPresentationInstance &Subject) const {
    if (!Subject.IsValid()) {
        return nullptr;
    }

    const double Now = GetSynchronizedServerTimeSeconds();
    for (const FMythicReplicatedEntityCombatPresentation &Presentation : ReplicatedPresentations.Items) {
        if (Presentation.Subject == Subject && !Presentation.IsExpired(Now)) {
            return &Presentation;
        }
    }
    return nullptr;
}

bool UMythicEntityCombatPresentationComponent::AuthoritySetCombatPresentation(const FMythicEntityCombatPresentationAuthorityRequest &Request) {
    EnsurePresentationRegistryBinding();
    FMythicReplicatedEntityCombatPresentation Sanitized;
    if (!IsAuthority() || !BuildSanitizedPresentation(Request, Sanitized)) {
        return false;
    }

    FMythicReplicatedEntityCombatPresentation *Existing = ReplicatedPresentations.Items.FindByPredicate(
        [&Sanitized](const FMythicReplicatedEntityCombatPresentation &Entry) { return Entry.Subject == Sanitized.Subject; });
    if (Existing && Sanitized.SourceRevision < Existing->SourceRevision) {
        return false;
    }
    if (Existing && Sanitized.SourceRevision == Existing->SourceRevision && !Sanitized.HasSamePayload(*Existing)) {
        return false;
    }

    const double Now = GetSynchronizedServerTimeSeconds();
    if (Sanitized.IsExpired(Now)) {
        if (Existing) {
            const int32 ExistingIndex = static_cast<int32>(Existing - ReplicatedPresentations.Items.GetData());
            ReplicatedPresentations.Items.RemoveAtSwap(ExistingIndex, 1, EAllowShrinking::No);
            ReplicatedPresentations.MarkArrayDirty();
            PublishRevision();
            ScheduleAuthorityExpiryTimer();
        }
        return true;
    }

    const ESetResult Result = SetPresentationInternal(Sanitized);
    if (Result == ESetResult::Rejected) {
        return false;
    }
    if (Result == ESetResult::Changed) {
        PublishRevision();
        ScheduleAuthorityExpiryTimer();
    }
    return true;
}

bool UMythicEntityCombatPresentationComponent::AuthorityReplaceCombatPresentations(
    const uint32 ReplacementRevision, const TArrayView<const FMythicEntityCombatPresentationAuthorityRequest> Requests) {
    EnsurePresentationRegistryBinding();
    if (!IsAuthority() || ReplacementRevision == 0 || ReplacementRevision < AuthorityReplacementRevision || Requests.Num() > MaxReplicatedPresentations) {
        return false;
    }

    TArray<FMythicReplicatedEntityCombatPresentation> Desired;
    Desired.Reserve(Requests.Num());
    const double Now = GetSynchronizedServerTimeSeconds();
    for (int32 RequestIndex = 0; RequestIndex < Requests.Num(); ++RequestIndex) {
        const FMythicEntityCombatPresentationAuthorityRequest &Request = Requests[RequestIndex];
        for (int32 PreviousIndex = 0; PreviousIndex < RequestIndex; ++PreviousIndex) {
            if (Requests[PreviousIndex].Subject == Request.Subject) {
                return false;
            }
        }

        FMythicReplicatedEntityCombatPresentation Sanitized;
        if (!BuildSanitizedPresentation(Request, Sanitized)) {
            return false;
        }

        const FMythicReplicatedEntityCombatPresentation *Existing = ReplicatedPresentations.Items.FindByPredicate(
            [&Sanitized](const FMythicReplicatedEntityCombatPresentation &Entry) { return Entry.Subject == Sanitized.Subject; });
        if (Existing &&
            (Sanitized.SourceRevision < Existing->SourceRevision ||
             (Sanitized.SourceRevision == Existing->SourceRevision && !Sanitized.HasSamePayload(*Existing)))) {
            return false;
        }
        if (!Sanitized.IsExpired(Now)) {
            Desired.Add(MoveTemp(Sanitized));
        }
    }

    if (ReplacementRevision == AuthorityReplacementRevision) {
        if (Desired.Num() != ReplicatedPresentations.Items.Num()) {
            return false;
        }
        for (const FMythicReplicatedEntityCombatPresentation &Candidate : Desired) {
            const FMythicReplicatedEntityCombatPresentation *Existing = ReplicatedPresentations.Items.FindByPredicate(
                [&Candidate](const FMythicReplicatedEntityCombatPresentation &Entry) { return Entry.Subject == Candidate.Subject; });
            if (!Existing || !Candidate.HasSamePayload(*Existing)) {
                return false;
            }
        }
        return true;
    }

    bool bChanged = RemovePresentationsByPredicate([&Desired](const FMythicReplicatedEntityCombatPresentation &Existing) {
                        return !Desired.ContainsByPredicate(
                            [&Existing](const FMythicReplicatedEntityCombatPresentation &Candidate) { return Candidate.Subject == Existing.Subject; });
                    }) > 0;

    for (const FMythicReplicatedEntityCombatPresentation &Candidate : Desired) {
        const ESetResult Result = SetPresentationInternal(Candidate);
        if (Result == ESetResult::Rejected) {
            UE_LOG(LogMythicEntityCombatPresentation, Error,
                   TEXT("Validated combat-presentation replacement rejected "
                        "during commit; preserving the fail-closed partial "
                        "view."));
            if (bChanged) {
                PublishRevision();
                ScheduleAuthorityExpiryTimer();
            }
            return false;
        }
        bChanged |= Result == ESetResult::Changed;
    }

    if (bChanged) {
        PublishRevision();
        ScheduleAuthorityExpiryTimer();
    }
    AuthorityReplacementRevision = ReplacementRevision;
    return true;
}

bool UMythicEntityCombatPresentationComponent::AuthorityRevokeCombatPresentation(const FMythicEntityPresentationInstance Subject) {
    if (!IsAuthority() || !Subject.IsValid()) {
        return false;
    }
    if (RemovePresentationsByPredicate([&Subject](const FMythicReplicatedEntityCombatPresentation &Entry) { return Entry.Subject == Subject; }) == 0) {
        return false;
    }
    PublishRevision();
    ScheduleAuthorityExpiryTimer();
    return true;
}

int32 UMythicEntityCombatPresentationComponent::AuthorityRevokeAllCombatPresentations() {
    if (!IsAuthority()) {
        return 0;
    }
    const int32 Removed = ReplicatedPresentations.Items.Num();
    AuthorityReplacementRevision = 0;
    if (Removed == 0) {
        ScheduleAuthorityExpiryTimer();
        return 0;
    }
    ReplicatedPresentations.Items.Reset();
    ReplicatedPresentations.MarkArrayDirty();
    PublishRevision();
    ScheduleAuthorityExpiryTimer();
    return Removed;
}

int32 UMythicEntityCombatPresentationComponent::AuthorityPruneExpiredCombatPresentations(const double ServerTimeSeconds) {
    if (!IsAuthority() || !FMath::IsFinite(ServerTimeSeconds) || ServerTimeSeconds < 0.0) {
        return 0;
    }
    const int32 Removed = RemovePresentationsByPredicate(
        [ServerTimeSeconds](const FMythicReplicatedEntityCombatPresentation &Entry) { return Entry.IsExpired(ServerTimeSeconds); });
    if (Removed > 0) {
        PublishRevision();
    }
    ScheduleAuthorityExpiryTimer();
    return Removed;
}

bool UMythicEntityCombatPresentationComponent::IsAuthority() const { return GetOwner() && GetOwner()->HasAuthority(); }

double UMythicEntityCombatPresentationComponent::GetSynchronizedServerTimeSeconds() const {
    const UWorld *World = GetWorld();
    if (!World) {
        return 0.0;
    }
    if (const AGameStateBase *GameState = World->GetGameState()) {
        return static_cast<double>(GameState->GetServerWorldTimeSeconds());
    }
    return IsAuthority() ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
}

bool UMythicEntityCombatPresentationComponent::BuildSanitizedPresentation(const FMythicEntityCombatPresentationAuthorityRequest &Request,
                                                                          FMythicReplicatedEntityCombatPresentation &OutPresentation) const {
    OutPresentation = FMythicReplicatedEntityCombatPresentation();
    if (!Request.Subject.IsValid() || Request.SourceRevision == 0 || !FMath::IsFinite(Request.ExpiryServerTimeSeconds) ||
        Request.ExpiryServerTimeSeconds < 0.0 || (Request.bExactCombatLevelPermitted && Request.SubjectCombatLevel <= 0)) {
        return false;
    }

    const UWorld *World = GetWorld();
    const UMythicEntityPresentationRegistry *Registry = World ? World->GetSubsystem<UMythicEntityPresentationRegistry>() : nullptr;
    FMythicEntityId PrivateEntityId;
    if (!Registry || !Registry->ResolveAuthorityEntity(Request.Subject, PrivateEntityId)) {
        return false;
    }

    const bool bPresentedRankIsValid = static_cast<uint8>(Request.PresentedCombatRank) <=
        static_cast<uint8>(EMythicPresentedCombatRank::WorldBoss);
    FMythicCombatThreatAssessmentInputs SanitizedAssessmentInputs =
        Request.AssessmentInputs;
    const bool bRankPresentationPermitted =
        SanitizedAssessmentInputs.bAssessmentPermitted &&
        Request.bRankPresentationPermitted && bPresentedRankIsValid &&
        Request.PresentedCombatRank != EMythicPresentedCombatRank::Unknown;
    SanitizedAssessmentInputs.Rank = ResolveThreatAssessmentRank(
        bPresentedRankIsValid ? Request.PresentedCombatRank
                              : EMythicPresentedCombatRank::Unknown);
    SanitizedAssessmentInputs.bRankKnownToViewer =
        bRankPresentationPermitted;

    OutPresentation.Subject = Request.Subject;
    const UMythicCombatSettings *CombatSettings = GetDefault<UMythicCombatSettings>();
    const FMythicCombatThreatThresholds Thresholds = CombatSettings
        ? CombatSettings->CombatPresentationThreatThresholds
        : FMythicCombatThreatThresholds();
    OutPresentation.ThreatBand = FMythicCombatThreatAssessment::Assess(
        SanitizedAssessmentInputs, Thresholds);
    OutPresentation.bCombatCapable =
        SanitizedAssessmentInputs.bAssessmentPermitted &&
        SanitizedAssessmentInputs.bCombatCapable;
    OutPresentation.PresentedCombatRank = OutPresentation.bCombatCapable && bRankPresentationPermitted
        ? Request.PresentedCombatRank
        : EMythicPresentedCombatRank::Unknown;
    OutPresentation.bHasExactCombatLevel = Request.bExactCombatLevelPermitted && OutPresentation.bCombatCapable;
    OutPresentation.ExactCombatLevel = OutPresentation.bHasExactCombatLevel ? Request.SubjectCombatLevel : 0;
    OutPresentation.SourceRevision = Request.SourceRevision;
    OutPresentation.ExpiryServerTimeSeconds = Request.ExpiryServerTimeSeconds;
    return true;
}

UMythicEntityCombatPresentationComponent::ESetResult
UMythicEntityCombatPresentationComponent::SetPresentationInternal(const FMythicReplicatedEntityCombatPresentation &NewPresentation) {
    FMythicReplicatedEntityCombatPresentation *Existing = ReplicatedPresentations.Items.FindByPredicate(
        [&NewPresentation](const FMythicReplicatedEntityCombatPresentation &Entry) { return Entry.Subject == NewPresentation.Subject; });
    if (Existing) {
        if (NewPresentation.SourceRevision < Existing->SourceRevision) {
            return ESetResult::Rejected;
        }
        if (NewPresentation.SourceRevision == Existing->SourceRevision) {
            return Existing->HasSamePayload(NewPresentation) ? ESetResult::Unchanged : ESetResult::Rejected;
        }

        Existing->ThreatBand = NewPresentation.ThreatBand;
        Existing->bCombatCapable = NewPresentation.bCombatCapable;
        Existing->PresentedCombatRank = NewPresentation.PresentedCombatRank;
        Existing->bHasExactCombatLevel = NewPresentation.bHasExactCombatLevel;
        Existing->ExactCombatLevel = NewPresentation.ExactCombatLevel;
        Existing->SourceRevision = NewPresentation.SourceRevision;
        Existing->ExpiryServerTimeSeconds = NewPresentation.ExpiryServerTimeSeconds;
        ReplicatedPresentations.MarkItemDirty(*Existing);
        return ESetResult::Changed;
    }

    if (ReplicatedPresentations.Items.Num() >= MaxReplicatedPresentations) {
        UE_LOG(LogMythicEntityCombatPresentation, Warning,
               TEXT("Rejected combat presentation because the owner-only "
                    "bound of %d was reached."),
               MaxReplicatedPresentations);
        return ESetResult::Rejected;
    }

    FMythicReplicatedEntityCombatPresentation &Added = ReplicatedPresentations.Items.Add_GetRef(NewPresentation);
    ReplicatedPresentations.MarkItemDirty(Added);
    return ESetResult::Changed;
}

int32 UMythicEntityCombatPresentationComponent::RemovePresentationsByPredicate(
    TFunctionRef<bool(const FMythicReplicatedEntityCombatPresentation &)> Predicate) {
    const int32 Before = ReplicatedPresentations.Items.Num();
    ReplicatedPresentations.Items.RemoveAllSwap(Predicate, EAllowShrinking::No);
    const int32 Removed = Before - ReplicatedPresentations.Items.Num();
    if (Removed > 0) {
        ReplicatedPresentations.MarkArrayDirty();
    }
    return Removed;
}

void UMythicEntityCombatPresentationComponent::PublishRevision() {
    ++LocalRevision;
    if (LocalRevision == 0 || LocalRevision > static_cast<uint32>(MAX_int32)) {
        LocalRevision = 1;
    }
    NativeRevisionDelegate.Broadcast(LocalRevision);

    if (IsAuthority()) {
        if (AActor *OwnerActor = GetOwner()) {
            OwnerActor->FlushNetDormancy();
            OwnerActor->ForceNetUpdate();
        }
    }
}

void UMythicEntityCombatPresentationComponent::QueueReplicatedRevision() {
    if (bReplicatedRevisionQueued) {
        return;
    }
    bReplicatedRevisionQueued = true;
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimerForNextTick(this, &UMythicEntityCombatPresentationComponent::PublishQueuedReplicatedRevision);
    }
    else {
        PublishQueuedReplicatedRevision();
    }
}

void UMythicEntityCombatPresentationComponent::PublishQueuedReplicatedRevision() {
    if (!bReplicatedRevisionQueued) {
        return;
    }
    bReplicatedRevisionQueued = false;
    PublishRevision();
}

void UMythicEntityCombatPresentationComponent::ScheduleAuthorityExpiryTimer() {
    if (!IsAuthority()) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    FTimerManager &TimerManager = World->GetTimerManager();
    TimerManager.ClearTimer(ExpiryTimerHandle);

    double EarliestExpiry = TNumericLimits<double>::Max();
    for (const FMythicReplicatedEntityCombatPresentation &Presentation : ReplicatedPresentations.Items) {
        if (Presentation.ExpiryServerTimeSeconds > 0.0) {
            EarliestExpiry = FMath::Min(EarliestExpiry, Presentation.ExpiryServerTimeSeconds);
        }
    }
    if (EarliestExpiry == TNumericLimits<double>::Max()) {
        return;
    }

    const double Now = GetSynchronizedServerTimeSeconds();
    const float DelaySeconds = static_cast<float>(FMath::Clamp(EarliestExpiry - Now, 0.01, 86400.0));
    TimerManager.SetTimer(ExpiryTimerHandle, this, &UMythicEntityCombatPresentationComponent::HandleAuthorityExpiryTimer, DelaySeconds, false);
}

void UMythicEntityCombatPresentationComponent::HandleAuthorityExpiryTimer() { AuthorityPruneExpiredCombatPresentations(GetSynchronizedServerTimeSeconds()); }

void UMythicEntityCombatPresentationComponent::EnsurePresentationRegistryBinding() {
    if (!IsAuthority() || BoundPresentationRegistry.IsValid()) {
        return;
    }
    UWorld *World = GetWorld();
    UMythicEntityPresentationRegistry *Registry =
        World ? World->GetSubsystem<UMythicEntityPresentationRegistry>() : nullptr;
    if (!Registry) {
        return;
    }

    BoundPresentationRegistry = Registry;
    PresentationUnregisteredHandle = Registry->OnPresentationUnregistered.AddUObject(
        this, &ThisClass::HandlePresentationUnregistered);
}

void UMythicEntityCombatPresentationComponent::RemovePresentationRegistryBinding() {
    if (UMythicEntityPresentationRegistry *Registry = BoundPresentationRegistry.Get();
        Registry && PresentationUnregisteredHandle.IsValid()) {
        Registry->OnPresentationUnregistered.Remove(PresentationUnregisteredHandle);
    }
    PresentationUnregisteredHandle.Reset();
    BoundPresentationRegistry.Reset();
}

void UMythicEntityCombatPresentationComponent::HandlePresentationUnregistered(
    const FMythicEntityPresentationInstance &Instance,
    UMythicEntityPresentationComponent *PresentationComponent) {
    (void)PresentationComponent;
    // The registry emits this edge before an exact pooled/deactivated instance stops resolving. Revocation does not
    // attempt a new resolution, so it remains safe during epoch resets and cannot alias a later embodiment generation.
    AuthorityRevokeCombatPresentation(Instance);
}
