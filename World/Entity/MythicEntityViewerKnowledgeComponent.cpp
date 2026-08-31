#include "World/Entity/MythicEntityViewerKnowledgeComponent.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythicEntityViewerKnowledge, Log, All);

namespace {
void MarkOfflineSafeDossierRetention(
    const UWorld *World, const FMythicEntityId &EntityId) {
    if (!World || EntityId.GetDomain() != EMythicEntityDomain::LivingWorld) {
        return;
    }
    UGameInstance *GameInstance = World->GetGameInstance();
    UMythicLivingWorldSubsystem *LivingWorld = GameInstance
        ? GameInstance->GetSubsystem<UMythicLivingWorldSubsystem>()
        : nullptr;
    UMythicPersistentNPCRegistry *IdentityRegistry = LivingWorld
        ? LivingWorld->GetPersistentNPCRegistry()
        : nullptr;
    if (IdentityRegistry) {
        IdentityRegistry->MarkRetainedByLearnedDossier(EntityId);
    }
}
}

void FMythicReplicatedEntityRecognitionArray::PreReplicatedRemove(
    const TArrayView<int32> &RemovedIndices, const int32 FinalSize) {
    (void)RemovedIndices;
    (void)FinalSize;
    if (Owner.IsValid()) {
        Owner->QueueReplicatedRevision();
    }
}

void FMythicReplicatedEntityRecognitionArray::PostReplicatedAdd(
    const TArrayView<int32> &AddedIndices, const int32 FinalSize) {
    (void)AddedIndices;
    (void)FinalSize;
    if (Owner.IsValid()) {
        Owner->QueueReplicatedRevision();
    }
}

void FMythicReplicatedEntityRecognitionArray::PostReplicatedChange(
    const TArrayView<int32> &ChangedIndices, const int32 FinalSize) {
    (void)ChangedIndices;
    (void)FinalSize;
    if (Owner.IsValid()) {
        Owner->QueueReplicatedRevision();
    }
}

UMythicEntityViewerKnowledgeComponent::UMythicEntityViewerKnowledgeComponent() {
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    RecognitionBindings.SetOwner(this);
}

void UMythicEntityViewerKnowledgeComponent::BeginPlay() {
    Super::BeginPlay();
    RecognitionBindings.SetOwner(this);

    if (!IsAuthority()) {
        return;
    }
    if (UWorld *World = GetWorld()) {
        if (UMythicEntityPresentationRegistry *Registry =
                World->GetSubsystem<UMythicEntityPresentationRegistry>()) {
            PresentationUnregisteredHandle =
                Registry->OnPresentationUnregistered.AddUObject(
                    this,
                    &UMythicEntityViewerKnowledgeComponent::HandlePresentationUnregistered);
        }
    }
    ScheduleAuthorityExpiryTimer();
}

void UMythicEntityViewerKnowledgeComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(ExpiryTimerHandle);
        if (PresentationUnregisteredHandle.IsValid()) {
            if (UMythicEntityPresentationRegistry *Registry =
                    World->GetSubsystem<UMythicEntityPresentationRegistry>()) {
                Registry->OnPresentationUnregistered.Remove(
                    PresentationUnregisteredHandle);
            }
        }
    }
    PresentationUnregisteredHandle.Reset();
    RecognitionBindings.SetOwner(nullptr);
    Super::EndPlay(EndPlayReason);
}

void UMythicEntityViewerKnowledgeComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicEntityViewerKnowledgeComponent,
                            RecognitionBindings, COND_OwnerOnly);
}

bool UMythicEntityViewerKnowledgeComponent::GetKnowledgeForSubject(
    const FMythicEntityPresentationInstance Subject,
    FMythicEntityKnowledgeView &OutKnowledge) const {
    OutKnowledge.Reset();
    const int32 BindingIndex = FindBindingIndex(Subject);
    if (!RecognitionBindings.Items.IsValidIndex(BindingIndex)) {
        return false;
    }

    const FMythicReplicatedEntityRecognition &Binding =
        RecognitionBindings.Items[BindingIndex];
    if (!Binding.EntityId.IsValid()
        || Binding.IsExpired(GetSynchronizedServerTimeSeconds())) {
        return false;
    }

    OutKnowledge = Binding.Knowledge;
    OutKnowledge.bRecognitionGranted = true;
    return true;
}

bool UMythicEntityViewerKnowledgeComponent::IsSubjectRecognized(
    const FMythicEntityPresentationInstance Subject) const {
    FMythicEntityKnowledgeView Knowledge;
    return GetKnowledgeForSubject(Subject, Knowledge);
}

int32 UMythicEntityViewerKnowledgeComponent::GetActiveRecognitionCount() const {
    const double Now = GetSynchronizedServerTimeSeconds();
    int32 Count = 0;
    for (const FMythicReplicatedEntityRecognition &Binding :
         RecognitionBindings.Items) {
        Count += Binding.Subject.IsValid() && Binding.EntityId.IsValid()
                         && !Binding.IsExpired(Now)
                     ? 1
                     : 0;
    }
    return Count;
}

bool UMythicEntityViewerKnowledgeComponent::ResolveRecognizedEntity(
    const FMythicEntityPresentationInstance &Subject,
    FMythicEntityId &OutEntityId) const {
    OutEntityId.Reset();
    const int32 BindingIndex = FindBindingIndex(Subject);
    if (!RecognitionBindings.Items.IsValidIndex(BindingIndex)) {
        return false;
    }

    const FMythicReplicatedEntityRecognition &Binding =
        RecognitionBindings.Items[BindingIndex];
    if (!Binding.EntityId.IsValid()
        || Binding.IsExpired(GetSynchronizedServerTimeSeconds())) {
        return false;
    }

    if (IsAuthority()
        && !ValidateAuthoritySubjectBinding(Subject, Binding.EntityId)) {
        return false;
    }

    OutEntityId = Binding.EntityId;
    return true;
}

bool UMythicEntityViewerKnowledgeComponent::AuthorityReplaceLearnedDossier(
    const FMythicEntityId &EntityId,
    const FMythicEntityKnowledgeView &LearnedKnowledge) {
    if (!IsAuthority() || !EntityId.IsValid()) {
        return false;
    }

    const FMythicEntityKnowledgeView SafeKnowledge =
        FMythicEntityKnowledgeRules::Sanitize(LearnedKnowledge);
    int32 DossierIndex = FindDossierIndex(EntityId);
    if (DossierIndex == INDEX_NONE) {
        const int32 EffectiveCap =
            FMath::Clamp(MaxLearnedDossiers, 64,
                         AbsoluteMaximumLearnedDossiers);
        if (LearnedDossiers.Num() >= EffectiveCap) {
            UE_LOG(LogMythicEntityViewerKnowledge, Error,
                   TEXT("Rejected a new learned dossier at the durable cap of %d; existing player knowledge was preserved."),
                   EffectiveCap);
            return false;
        }
        FMythicEntityLearnedDossier &Added = LearnedDossiers.AddDefaulted_GetRef();
        Added.EntityId = EntityId;
        Added.LearnedKnowledge = SafeKnowledge;
        Added.KnowledgeRevision = 1;
        MarkOfflineSafeDossierRetention(GetWorld(), EntityId);
        RefreshProjectedBindings(EntityId, Added);
        return true;
    }

    FMythicEntityLearnedDossier &Existing = LearnedDossiers[DossierIndex];
    FMythicEntityKnowledgeView Probe = Existing.LearnedKnowledge;
    const bool bPresentationChanged =
        FMythicEntityKnowledgeRules::MergeLearnedDelta(Probe, SafeKnowledge)
        || Probe.bNameKnown != SafeKnowledge.bNameKnown
        || !Probe.RecognizedName.EqualTo(SafeKnowledge.RecognizedName)
        || Probe.bFactionKnown != SafeKnowledge.bFactionKnown
        || Probe.KnownFactionTag != SafeKnowledge.KnownFactionTag
        || Probe.bRoleKnown != SafeKnowledge.bRoleKnown
        || Probe.KnownRoleTag != SafeKnowledge.KnownRoleTag
        || Probe.RelationshipBand != SafeKnowledge.RelationshipBand
        || Probe.StandingBand != SafeKnowledge.StandingBand
        || Probe.DiscoveredTraits != SafeKnowledge.DiscoveredTraits
        || Probe.DiscoveredHistory != SafeKnowledge.DiscoveredHistory
        || Probe.KnownLikes != SafeKnowledge.KnownLikes
        || Probe.KnownDislikes != SafeKnowledge.KnownDislikes;
    if (!bPresentationChanged) {
        return false;
    }

    Existing.LearnedKnowledge = SafeKnowledge;
    Existing.KnowledgeRevision =
        AdvanceKnowledgeRevision(Existing.KnowledgeRevision);
    MarkOfflineSafeDossierRetention(GetWorld(), EntityId);
    RefreshProjectedBindings(EntityId, Existing);
    return true;
}

bool UMythicEntityViewerKnowledgeComponent::AuthorityMergeLearnedKnowledge(
    const FMythicEntityId &EntityId,
    const FMythicEntityKnowledgeView &LearnedDelta) {
    if (!IsAuthority() || !EntityId.IsValid()) {
        return false;
    }

    int32 DossierIndex = FindDossierIndex(EntityId);
    if (DossierIndex == INDEX_NONE) {
        FMythicEntityKnowledgeView Empty;
        FMythicEntityKnowledgeRules::MergeLearnedDelta(Empty, LearnedDelta);
        return AuthorityReplaceLearnedDossier(EntityId, Empty);
    }

    FMythicEntityLearnedDossier &Existing = LearnedDossiers[DossierIndex];
    if (!FMythicEntityKnowledgeRules::MergeLearnedDelta(
            Existing.LearnedKnowledge, LearnedDelta)) {
        return false;
    }
    Existing.KnowledgeRevision =
        AdvanceKnowledgeRevision(Existing.KnowledgeRevision);
    MarkOfflineSafeDossierRetention(GetWorld(), EntityId);
    RefreshProjectedBindings(EntityId, Existing);
    return true;
}

bool UMythicEntityViewerKnowledgeComponent::AuthorityGrantRecognition(
    const FMythicEntityPresentationInstance Subject,
    const FMythicEntityId &EntityId, const float LeaseSeconds) {
    if (!IsAuthority() || !Subject.IsValid() || !EntityId.IsValid()
        || !FMath::IsFinite(LeaseSeconds)
        || !ValidateAuthoritySubjectBinding(Subject, EntityId)) {
        return false;
    }

    const int32 DossierIndex = FindDossierIndex(EntityId);
    if (!LearnedDossiers.IsValidIndex(DossierIndex)) {
        UE_LOG(LogMythicEntityViewerKnowledge, Warning,
               TEXT("Rejected recognition without a durable learned dossier for %s."),
               *Subject.ToDebugString());
        return false;
    }

    const float EffectiveMaximum =
        FMath::Clamp(MaximumRecognitionLeaseSeconds, 1.0f, 300.0f);
    const float RequestedLease = LeaseSeconds == 0.0f
                                     ? DefaultRecognitionLeaseSeconds
                                     : LeaseSeconds;
    if (RequestedLease <= 0.0f) {
        return false;
    }
    const float EffectiveLease =
        FMath::Clamp(RequestedLease, 1.0f, EffectiveMaximum);
    const double Expiry = GetSynchronizedServerTimeSeconds()
                          + static_cast<double>(EffectiveLease);

    const FMythicEntityLearnedDossier &Dossier =
        LearnedDossiers[DossierIndex];

    // The authority registry permits only one current embodiment per canonical entity. Drop any lease that belonged
    // to a prior unregistered allocation even when that allocation never reached public component registration.
    RemoveBindingsByPredicate(
        [&EntityId, &Subject](const FMythicReplicatedEntityRecognition &Binding) {
            return Binding.EntityId == EntityId && Binding.Subject != Subject;
        });

    int32 BindingIndex = FindBindingIndex(Subject);
    if (RecognitionBindings.Items.IsValidIndex(BindingIndex)) {
        FMythicReplicatedEntityRecognition &Existing =
            RecognitionBindings.Items[BindingIndex];
        Existing.EntityId = EntityId;
        Existing.Knowledge = Dossier.LearnedKnowledge;
        Existing.Knowledge.bRecognitionGranted = true;
        Existing.KnowledgeRevision = Dossier.KnowledgeRevision;
        Existing.ExpiryServerTimeSeconds = Expiry;
        Existing.AuthorityTouchSerial = AllocateBindingTouchSerial();
        RecognitionBindings.MarkItemDirty(Existing);
        PublishRevision();
        ScheduleAuthorityExpiryTimer();
        return true;
    }

    AuthorityPruneExpiredRecognition(GetSynchronizedServerTimeSeconds());
    const int32 EffectiveCap =
        FMath::Clamp(MaxRecognitionBindings, 8,
                     AbsoluteMaximumRecognitionBindings);
    if (RecognitionBindings.Items.Num() >= EffectiveCap) {
        int32 LeastRecentIndex = 0;
        for (int32 Index = 1; Index < RecognitionBindings.Items.Num(); ++Index) {
            if (RecognitionBindings.Items[Index].AuthorityTouchSerial
                < RecognitionBindings.Items[LeastRecentIndex]
                      .AuthorityTouchSerial) {
                LeastRecentIndex = Index;
            }
        }
        RecognitionBindings.Items.RemoveAtSwap(LeastRecentIndex, 1,
                                                EAllowShrinking::No);
        RecognitionBindings.MarkArrayDirty();
    }

    FMythicReplicatedEntityRecognition &Added =
        RecognitionBindings.Items.AddDefaulted_GetRef();
    Added.Subject = Subject;
    Added.EntityId = EntityId;
    Added.Knowledge = Dossier.LearnedKnowledge;
    Added.Knowledge.bRecognitionGranted = true;
    Added.KnowledgeRevision = Dossier.KnowledgeRevision;
    Added.ExpiryServerTimeSeconds = Expiry;
    Added.AuthorityTouchSerial = AllocateBindingTouchSerial();
    RecognitionBindings.MarkItemDirty(Added);
    PublishRevision();
    ScheduleAuthorityExpiryTimer();
    return true;
}

bool UMythicEntityViewerKnowledgeComponent::AuthorityLearnAndGrantRecognition(
    const FMythicEntityPresentationInstance Subject,
    const FMythicEntityId &EntityId,
    const FMythicEntityKnowledgeView &LearnedDelta,
    const float LeaseSeconds) {
    if (!IsAuthority() || !ValidateAuthoritySubjectBinding(Subject, EntityId)) {
        return false;
    }
    if (FindDossierIndex(EntityId) == INDEX_NONE) {
        FMythicEntityKnowledgeView Initial;
        FMythicEntityKnowledgeRules::MergeLearnedDelta(Initial, LearnedDelta);
        if (!AuthorityReplaceLearnedDossier(EntityId, Initial)) {
            return false;
        }
    }
    else {
        AuthorityMergeLearnedKnowledge(EntityId, LearnedDelta);
    }
    return AuthorityGrantRecognition(Subject, EntityId, LeaseSeconds);
}

bool UMythicEntityViewerKnowledgeComponent::AuthorityRevokeRecognition(
    const FMythicEntityPresentationInstance Subject) {
    if (!IsAuthority() || !Subject.IsValid()) {
        return false;
    }
    if (RemoveBindingsByPredicate(
            [&Subject](const FMythicReplicatedEntityRecognition &Binding) {
                return Binding.Subject == Subject;
            })
        == 0) {
        return false;
    }
    PublishRevision();
    ScheduleAuthorityExpiryTimer();
    return true;
}

int32 UMythicEntityViewerKnowledgeComponent::AuthorityRevokeRecognitionForEntity(
    const FMythicEntityId &EntityId) {
    if (!IsAuthority() || !EntityId.IsValid()) {
        return 0;
    }
    const int32 Removed = RemoveBindingsByPredicate(
        [&EntityId](const FMythicReplicatedEntityRecognition &Binding) {
            return Binding.EntityId == EntityId;
        });
    if (Removed > 0) {
        PublishRevision();
        ScheduleAuthorityExpiryTimer();
    }
    return Removed;
}

int32 UMythicEntityViewerKnowledgeComponent::AuthorityClearRecognitionBindings() {
    if (!IsAuthority() || RecognitionBindings.Items.IsEmpty()) {
        return 0;
    }
    const int32 Removed = RecognitionBindings.Items.Num();
    RecognitionBindings.Items.Reset();
    RecognitionBindings.MarkArrayDirty();
    PublishRevision();
    ScheduleAuthorityExpiryTimer();
    return Removed;
}

int32 UMythicEntityViewerKnowledgeComponent::AuthorityPruneExpiredRecognition(
    const double ServerTimeSeconds) {
    if (!IsAuthority() || !FMath::IsFinite(ServerTimeSeconds)
        || ServerTimeSeconds < 0.0) {
        return 0;
    }
    const int32 Removed = RemoveBindingsByPredicate(
        [ServerTimeSeconds](const FMythicReplicatedEntityRecognition &Binding) {
            return Binding.IsExpired(ServerTimeSeconds);
        });
    if (Removed > 0) {
        PublishRevision();
    }
    ScheduleAuthorityExpiryTimer();
    return Removed;
}

bool UMythicEntityViewerKnowledgeComponent::AuthorityRestoreLearnedDossiers(
    const TArray<FMythicEntityLearnedDossier> &SavedDossiers) {
    if (!IsAuthority()) {
        return false;
    }

    const int32 EffectiveCap =
        FMath::Clamp(MaxLearnedDossiers, 64,
                     AbsoluteMaximumLearnedDossiers);
    TArray<FMythicEntityLearnedDossier> Restored;
    Restored.Reserve(FMath::Min(SavedDossiers.Num(), EffectiveCap));
    for (const FMythicEntityLearnedDossier &Saved : SavedDossiers) {
        if (!Saved.EntityId.IsValid()) {
            continue;
        }
        const FMythicEntityKnowledgeView Safe =
            FMythicEntityKnowledgeRules::Sanitize(Saved.LearnedKnowledge);
        const int32 DuplicateIndex =
            FMythicEntityKnowledgeRules::FindDossierIndex(Restored,
                                                           Saved.EntityId);
        if (Restored.IsValidIndex(DuplicateIndex)) {
            Restored[DuplicateIndex].LearnedKnowledge = Safe;
            Restored[DuplicateIndex].KnowledgeRevision =
                Saved.KnowledgeRevision == 0 ? 1 : Saved.KnowledgeRevision;
            continue;
        }
        if (Restored.Num() >= EffectiveCap) {
            break;
        }
        FMythicEntityLearnedDossier &Added = Restored.AddDefaulted_GetRef();
        Added.EntityId = Saved.EntityId;
        Added.LearnedKnowledge = Safe;
        Added.KnowledgeRevision =
            Saved.KnowledgeRevision == 0 ? 1 : Saved.KnowledgeRevision;
    }
    LearnedDossiers = MoveTemp(Restored);
    for (const FMythicEntityLearnedDossier &Dossier : LearnedDossiers) {
        MarkOfflineSafeDossierRetention(GetWorld(), Dossier.EntityId);
    }

    bool bBindingsChanged = false;
    for (int32 Index = RecognitionBindings.Items.Num() - 1; Index >= 0;
         --Index) {
        FMythicReplicatedEntityRecognition &Binding =
            RecognitionBindings.Items[Index];
        const int32 RestoredIndex = FindDossierIndex(Binding.EntityId);
        if (!LearnedDossiers.IsValidIndex(RestoredIndex)) {
            RecognitionBindings.Items.RemoveAtSwap(Index, 1,
                                                    EAllowShrinking::No);
            RecognitionBindings.MarkArrayDirty();
            bBindingsChanged = true;
            continue;
        }
        const FMythicEntityLearnedDossier &Dossier =
            LearnedDossiers[RestoredIndex];
        Binding.Knowledge = Dossier.LearnedKnowledge;
        Binding.Knowledge.bRecognitionGranted = true;
        Binding.KnowledgeRevision = Dossier.KnowledgeRevision;
        RecognitionBindings.MarkItemDirty(Binding);
        bBindingsChanged = true;
    }
    if (bBindingsChanged) {
        PublishRevision();
    }
    ScheduleAuthorityExpiryTimer();
    return true;
}

bool UMythicEntityViewerKnowledgeComponent::IsAuthority() const {
    return GetOwner() && GetOwner()->HasAuthority();
}

double UMythicEntityViewerKnowledgeComponent::GetSynchronizedServerTimeSeconds() const {
    const UWorld *World = GetWorld();
    if (!World) {
        return 0.0;
    }
    if (const AGameStateBase *GameState = World->GetGameState()) {
        return static_cast<double>(GameState->GetServerWorldTimeSeconds());
    }
    return static_cast<double>(World->GetTimeSeconds());
}

int32 UMythicEntityViewerKnowledgeComponent::FindBindingIndex(
    const FMythicEntityPresentationInstance &Subject) const {
    if (!Subject.IsValid()) {
        return INDEX_NONE;
    }
    return RecognitionBindings.Items.IndexOfByPredicate(
        [&Subject](const FMythicReplicatedEntityRecognition &Binding) {
            return Binding.Subject == Subject;
        });
}

int32 UMythicEntityViewerKnowledgeComponent::FindDossierIndex(
    const FMythicEntityId &EntityId) const {
    return FMythicEntityKnowledgeRules::FindDossierIndex(LearnedDossiers,
                                                          EntityId);
}

uint32 UMythicEntityViewerKnowledgeComponent::AdvanceKnowledgeRevision(
    const uint32 CurrentRevision) const {
    const uint32 Next = CurrentRevision + 1;
    return Next == 0 ? 1 : Next;
}

uint64 UMythicEntityViewerKnowledgeComponent::AllocateBindingTouchSerial() {
    if (NextBindingTouchSerial == 0) {
        uint64 Serial = 1;
        for (FMythicReplicatedEntityRecognition &Binding :
             RecognitionBindings.Items) {
            Binding.AuthorityTouchSerial = Serial++;
        }
        NextBindingTouchSerial = Serial;
    }
    return NextBindingTouchSerial++;
}

bool UMythicEntityViewerKnowledgeComponent::ValidateAuthoritySubjectBinding(
    const FMythicEntityPresentationInstance &Subject,
    const FMythicEntityId &EntityId) const {
    if (!IsAuthority() || !Subject.IsValid() || !EntityId.IsValid()) {
        return false;
    }
    const UWorld *World = GetWorld();
    const UMythicEntityPresentationRegistry *Registry =
        World ? World->GetSubsystem<UMythicEntityPresentationRegistry>()
              : nullptr;
    FMythicEntityId Resolved;
    return Registry && Registry->ResolveAuthorityEntity(Subject, Resolved)
           && Resolved == EntityId;
}

bool UMythicEntityViewerKnowledgeComponent::RefreshProjectedBindings(
    const FMythicEntityId &EntityId,
    const FMythicEntityLearnedDossier &Dossier) {
    bool bChanged = false;
    for (FMythicReplicatedEntityRecognition &Binding :
         RecognitionBindings.Items) {
        if (Binding.EntityId != EntityId) {
            continue;
        }
        Binding.Knowledge = Dossier.LearnedKnowledge;
        Binding.Knowledge.bRecognitionGranted = true;
        Binding.KnowledgeRevision = Dossier.KnowledgeRevision;
        RecognitionBindings.MarkItemDirty(Binding);
        bChanged = true;
    }
    if (bChanged) {
        PublishRevision();
    }
    return bChanged;
}

int32 UMythicEntityViewerKnowledgeComponent::RemoveBindingsByPredicate(
    TFunctionRef<bool(const FMythicReplicatedEntityRecognition &)> Predicate) {
    const int32 Before = RecognitionBindings.Items.Num();
    RecognitionBindings.Items.RemoveAllSwap(Predicate, EAllowShrinking::No);
    const int32 Removed = Before - RecognitionBindings.Items.Num();
    if (Removed > 0) {
        RecognitionBindings.MarkArrayDirty();
    }
    return Removed;
}

void UMythicEntityViewerKnowledgeComponent::HandlePresentationUnregistered(
    const FMythicEntityPresentationInstance &Subject,
    UMythicEntityPresentationComponent *PresentationComponent) {
    (void)PresentationComponent;
    AuthorityRevokeRecognition(Subject);
}

void UMythicEntityViewerKnowledgeComponent::PublishRevision() {
    ++LocalRevision;
    if (LocalRevision == 0
        || LocalRevision > static_cast<uint32>(MAX_int32)) {
        LocalRevision = 1;
    }
    NativeRevisionDelegate.Broadcast(LocalRevision);
    OnViewerKnowledgeChanged.Broadcast(static_cast<int32>(LocalRevision));

    if (IsAuthority()) {
        if (AActor *OwnerActor = GetOwner()) {
            OwnerActor->FlushNetDormancy();
            OwnerActor->ForceNetUpdate();
        }
    }
}

void UMythicEntityViewerKnowledgeComponent::QueueReplicatedRevision() {
    if (bReplicatedRevisionQueued) {
        return;
    }
    bReplicatedRevisionQueued = true;
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimerForNextTick(
            this,
            &UMythicEntityViewerKnowledgeComponent::PublishQueuedReplicatedRevision);
    }
    else {
        PublishQueuedReplicatedRevision();
    }
}

void UMythicEntityViewerKnowledgeComponent::PublishQueuedReplicatedRevision() {
    if (!bReplicatedRevisionQueued) {
        return;
    }
    bReplicatedRevisionQueued = false;
    PublishRevision();
}

void UMythicEntityViewerKnowledgeComponent::ScheduleAuthorityExpiryTimer() {
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
    for (const FMythicReplicatedEntityRecognition &Binding :
         RecognitionBindings.Items) {
        if (Binding.ExpiryServerTimeSeconds > 0.0) {
            EarliestExpiry =
                FMath::Min(EarliestExpiry, Binding.ExpiryServerTimeSeconds);
        }
    }
    if (EarliestExpiry == TNumericLimits<double>::Max()) {
        return;
    }

    const double Now = GetSynchronizedServerTimeSeconds();
    const float Delay = static_cast<float>(
        FMath::Clamp(EarliestExpiry - Now, 0.01, 300.0));
    TimerManager.SetTimer(
        ExpiryTimerHandle, this,
        &UMythicEntityViewerKnowledgeComponent::HandleAuthorityExpiryTimer,
        Delay, false);
}

void UMythicEntityViewerKnowledgeComponent::HandleAuthorityExpiryTimer() {
    AuthorityPruneExpiredRecognition(GetSynchronizedServerTimeSeconds());
}
