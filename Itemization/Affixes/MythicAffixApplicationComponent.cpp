#include "Itemization/Affixes/MythicAffixApplicationComponent.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"
#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Mythic/Mythic.h"
#include "Net/UnrealNetwork.h"
#include "Stats/MythicStatDefinition.h"

#include <limits>

namespace {
void SortGuids(TArray<FGuid> &Guids) {
    Guids.Sort([](const FGuid &A, const FGuid &B) {
        if (A.A != B.A) return A.A < B.A;
        if (A.B != B.B) return A.B < B.B;
        if (A.C != B.C) return A.C < B.C;
        return A.D < B.D;
    });
}
}

UMythicAffixApplicationComponent::UMythicAffixApplicationComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicAffixApplicationComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ThisClass, PermanentStatLayer, COND_OwnerOnly);
}

void UMythicAffixApplicationComponent::BeginPlay() {
    Super::BeginPlay();
    ResolveAbilitySystem();
    BindRegistryReadiness();
    RequestAuthoritativeReconciliation();
}

void UMythicAffixApplicationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    UnbindRegistryReadiness();
    if (HasServerAuthority() && AbilitySystemComponent) {
        FMythicPermanentStatReconcileResult ClearResult;
        if (!PermanentStatLedger.ClearTransactional(*AbilitySystemComponent, ClearResult)) {
            UE_LOG(Myth, Error,
                   TEXT("Permanent affix stat cleanup failed during EndPlay on %s: %s"),
                   *GetNameSafe(GetOwner()), *ClearResult.Error);
        }
    }
    PermanentStatLedger.Abandon();
    Ledger.Reset();
    PendingSemanticDataRevision = 0;
    RebuildIndexes();
    Super::EndPlay(EndPlayReason);
}

bool UMythicAffixApplicationComponent::HasServerAuthority() const {
    return GetOwner() && GetOwner()->HasAuthority();
}

bool UMythicAffixApplicationComponent::ResolveAbilitySystem() {
    UMythicAbilitySystemComponent *Resolved = nullptr;
    if (IAbilitySystemInterface *AbilityOwner = Cast<IAbilitySystemInterface>(GetOwner())) {
        Resolved = Cast<UMythicAbilitySystemComponent>(AbilityOwner->GetAbilitySystemComponent());
    }
    if (!Resolved) {
        Resolved = AbilitySystemComponent;
    }
    if (!Resolved) {
        return false;
    }

    if (AbilitySystemComponent && AbilitySystemComponent != Resolved) {
        UMythicAbilitySystemComponent *Previous = AbilitySystemComponent;
        FMythicPermanentStatReconcileResult ClearResult;
        if (!PermanentStatLedger.ClearTransactional(*Previous, ClearResult)) {
            bApplicationQuarantined = true;
            UE_LOG(Myth, Error,
                   TEXT("Could not detach permanent affix stats from the previous ASC on %s: %s"),
                   *GetNameSafe(GetOwner()), *ClearResult.Error);
            return false;
        }

        PermanentStatLedger.Abandon();
        PublishPermanentStatLayer();
    }
    AbilitySystemComponent = Resolved;
    return true;
}

void UMythicAffixApplicationComponent::BindRegistryReadiness() {
    if (!HasServerAuthority() || BoundRegistry.IsValid()) {
        return;
    }
    UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicItemizationDataRegistrySubsystem *Registry =
        GameInstance ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    if (!Registry) {
        return;
    }
    BoundRegistry = Registry;
    RegistryReadinessHandle = Registry->OnReadinessChanged().AddUObject(
        this, &ThisClass::HandleRegistryReadinessChanged);
    RegistrySemanticDataChangedHandle = Registry->OnSemanticDataChanged().AddUObject(
        this, &ThisClass::HandleRegistrySemanticDataChanged);

    TWeakObjectPtr<UMythicAffixApplicationComponent> WeakThis(this);
    Registry->RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateLambda(
        [WeakThis](const bool bReady) {
            if (bReady && WeakThis.IsValid()) {
                WeakThis->RequestAuthoritativeReconciliation();
            }
        }));
}

void UMythicAffixApplicationComponent::UnbindRegistryReadiness() {
    if (BoundRegistry.IsValid() && RegistryReadinessHandle.IsValid()) {
        BoundRegistry->OnReadinessChanged().Remove(RegistryReadinessHandle);
    }
    if (BoundRegistry.IsValid() && RegistrySemanticDataChangedHandle.IsValid()) {
        BoundRegistry->OnSemanticDataChanged().Remove(RegistrySemanticDataChangedHandle);
    }
    RegistryReadinessHandle.Reset();
    RegistrySemanticDataChangedHandle.Reset();
    BoundRegistry.Reset();
    PendingProfileClosures.Reset();
    PendingSemanticDataRevision = 0;
}

void UMythicAffixApplicationComponent::HandleRegistryReadinessChanged(
    const EMythicItemizationReadiness NewReadiness) {
    if (NewReadiness >= EMythicItemizationReadiness::CoreSemanticReady) {
        RequestAuthoritativeReconciliation();
    }
}

void UMythicAffixApplicationComponent::HandleRegistrySemanticDataChanged(
    const uint64 SemanticRevision) {
    if (!HasServerAuthority()) {
        return;
    }
    PendingSemanticDataRevision = FMath::Max(PendingSemanticDataRevision,
                                             SemanticRevision);
    if (bReconciliationInProgress || bAuthoritativeEnumerationInProgress) {
        bReconciliationRequested = true;
        return;
    }
    if (!BoundRegistry.IsValid() || !BoundRegistry->IsCoreSemanticReady()) {
        const uint64 RejectedRevision = PendingSemanticDataRevision;
        PendingSemanticDataRevision = 0;
        QuarantineApplicationAfterSemanticReconciliationFailure(RejectedRevision);
        return;
    }
    RequestAuthoritativeReconciliation();
}

bool UMythicAffixApplicationComponent::ApplySnapshotsTransactional(
    UMythicItemInstance *SourceItem, const TConstArrayView<FRolledAffix> Snapshots) {
    if (Snapshots.IsEmpty()) {
        return true;
    }
    if (bApplicationQuarantined || !HasServerAuthority() || !ResolveAbilitySystem() || !SourceItem) {
        return false;
    }

    const FGuid SourceItemGuid = SourceItem->GetItemInstanceGuid();
    if (!SourceItemGuid.IsValid()
        || AuthoritativeDataQuarantinedSourceItemGuids.Contains(SourceItemGuid)) {
        return false;
    }

    TMap<FGuid, FMythicAppliedAffixState> DesiredLedger = Ledger;
    TSet<FGuid> BatchRollGuids;
    for (const FRolledAffix &Snapshot : Snapshots) {
        if (!Snapshot.RollGuid.IsValid() || BatchRollGuids.Contains(Snapshot.RollGuid)) {
            return false;
        }
        BatchRollGuids.Add(Snapshot.RollGuid);

        if (const FMythicAppliedAffixState *Existing = DesiredLedger.Find(Snapshot.RollGuid)) {
            if (Existing->SourceItemGuid != SourceItemGuid
                || !SnapshotsEquivalent(Existing->Snapshot, Snapshot)) {
                return false;
            }
            continue;
        }

        FMythicAppliedAffixState Candidate;
        Candidate.RollGuid = Snapshot.RollGuid;
        Candidate.SourceItemGuid = SourceItemGuid;
        Candidate.SourceItem = SourceItem;
        Candidate.Snapshot = Snapshot;
        if (!ValidateCandidate(Candidate)) {
            return false;
        }
        DesiredLedger.Add(Candidate.RollGuid, MoveTemp(Candidate));
    }
    return TransitionLedgerTransactional(MoveTemp(DesiredLedger));
}

bool UMythicAffixApplicationComponent::RemoveSnapshotsTransactional(
    UMythicItemInstance *SourceItem, const TConstArrayView<FRolledAffix> Snapshots) {
    if (Snapshots.IsEmpty()) {
        return true;
    }
    if (bApplicationQuarantined || !HasServerAuthority() || !ResolveAbilitySystem() || !SourceItem) {
        return false;
    }

    const FGuid SourceItemGuid = SourceItem->GetItemInstanceGuid();
    TMap<FGuid, FMythicAppliedAffixState> DesiredLedger = Ledger;
    TSet<FGuid> BatchRollGuids;
    for (const FRolledAffix &Snapshot : Snapshots) {
        if (!Snapshot.RollGuid.IsValid() || BatchRollGuids.Contains(Snapshot.RollGuid)) {
            return false;
        }
        BatchRollGuids.Add(Snapshot.RollGuid);
        const FMythicAppliedAffixState *Existing = DesiredLedger.Find(Snapshot.RollGuid);
        if (!Existing || Existing->SourceItemGuid != SourceItemGuid
            || !SnapshotsEquivalent(Existing->Snapshot, Snapshot)) {
            return false;
        }
        DesiredLedger.Remove(Snapshot.RollGuid);
    }
    return TransitionLedgerTransactional(MoveTemp(DesiredLedger));
}

void UMythicAffixApplicationComponent::GetActiveSourcesForStat(const FGameplayTag StatTag,
                                                               TArray<FGuid> &OutRollGuids) const {
    OutRollGuids.Reset();
    RollGuidsByStat.MultiFind(StatTag, OutRollGuids);
    OutRollGuids.RemoveAll([this](const FGuid &RollGuid) { return !ActiveRollGuids.Contains(RollGuid); });
    SortGuids(OutRollGuids);
}

void UMythicAffixApplicationComponent::ReconcileFromAuthoritativeSnapshots() {
    if (!HasServerAuthority()) {
        return;
    }
    if (bReconciliationInProgress || bAuthoritativeEnumerationInProgress) {
        bReconciliationRequested = true;
        return;
    }
    if (!ResolveAbilitySystem()) {
        if (PendingSemanticDataRevision != 0) {
            const uint64 RejectedRevision = PendingSemanticDataRevision;
            PendingSemanticDataRevision = 0;
            QuarantineApplicationAfterSemanticReconciliationFailure(RejectedRevision);
        }
        else {
            bReconciliationRequested = true;
        }
        return;
    }

    bAuthoritativeEnumerationInProgress = true;
    bReconciliationRequested = false;

    TArray<FMythicAffixApplicationCandidate> Candidates;
    TSet<FGuid> InvalidSources;
    const EAuthoritativeCollectionResult CollectionResult =
        BuildAuthoritativeCandidateSet(Candidates, InvalidSources);

    const bool bTransitioned = CommitCollectedCandidatesTransactional(
        CollectionResult, Candidates, InvalidSources, true);

    if (bTransitioned) {
        if (CollectionResult == EAuthoritativeCollectionResult::InvalidSourcesExcluded) {
            UE_LOG(Myth, Error,
                   TEXT("Affix reconciliation on %s excluded and quarantined %d corrupt equipped source item(s)."),
                   *GetNameSafe(GetOwner()), AuthoritativeDataQuarantinedSourceItemGuids.Num());
        }
    }
    else if (CollectionResult != EAuthoritativeCollectionResult::Deferred) {
        UE_LOG(Myth, Error, TEXT("Affix application reconciliation failed on %s"), *GetNameSafe(GetOwner()));
    }

    bAuthoritativeEnumerationInProgress = false;
    if (bReconciliationRequested) {
        bReconciliationRequested = false;
        ReconcileFromAuthoritativeSnapshots();
        return;
    }
    if (PendingSemanticDataRevision != 0) {
        const uint64 ReconciledRevision = PendingSemanticDataRevision;
        PendingSemanticDataRevision = 0;
        if (!bTransitioned) {
            QuarantineApplicationAfterSemanticReconciliationFailure(
                ReconciledRevision);
        }
    }
}

bool UMythicAffixApplicationComponent::ReconcileEquipmentMutationTransactional(
    const TConstArrayView<FMythicAffixEquipmentSlotOverride> SlotOverrides) {
    if (SlotOverrides.IsEmpty()) {
        return true;
    }
    return ReconcileCollectedSetTransactional(SlotOverrides, nullptr, {});
}

bool UMythicAffixApplicationComponent::ReconcileItemSnapshotMutationTransactional(
    UMythicItemInstance *SourceItem,
    const TConstArrayView<FRolledAffix> ProposedBaseSnapshots) {
    if (!SourceItem) {
        return false;
    }
    return ReconcileCollectedSetTransactional({}, SourceItem, ProposedBaseSnapshots);
}

bool UMythicAffixApplicationComponent::ReconcileCollectedSetTransactional(
    const TConstArrayView<FMythicAffixEquipmentSlotOverride> SlotOverrides,
    UMythicItemInstance *SnapshotOverrideSource,
    const TConstArrayView<FRolledAffix> ProposedBaseSnapshots) {
    if (!HasServerAuthority() || !ResolveAbilitySystem() || bReconciliationInProgress
        || bAuthoritativeEnumerationInProgress) {
        return false;
    }

    TGuardValue<bool> EnumerationGuard(bAuthoritativeEnumerationInProgress, true);
    TArray<FMythicAffixApplicationCandidate> Candidates;
    TSet<FGuid> InvalidSources;
    const EAuthoritativeCollectionResult Result = BuildAuthoritativeCandidateSet(
        Candidates, InvalidSources, SlotOverrides, SnapshotOverrideSource,
        ProposedBaseSnapshots);

    // A staged mutation is stricter than background repair: never publish a newly equipped/crafted corrupt source,
    // and never alter the prior ledger while any item in the full equipment closure is deferred.
    if (Result != EAuthoritativeCollectionResult::Ready || !InvalidSources.IsEmpty()) {
        return false;
    }
    return CommitCollectedCandidatesTransactional(Result, Candidates, InvalidSources, false);
}

bool UMythicAffixApplicationComponent::CommitCollectedCandidatesTransactional(
    const EAuthoritativeCollectionResult Result,
    const TConstArrayView<FMythicAffixApplicationCandidate> Candidates,
    const TSet<FGuid> &InvalidSourceItems,
    const bool bRepairOrphans) {
    if (!CollectionPermitsTransition(Result)) {
        return false;
    }
    (void)bRepairOrphans;
    if (!ReplaceAuthoritativeSetTransactional(Candidates)) {
        return false;
    }
    AuthoritativeDataQuarantinedSourceItemGuids = InvalidSourceItems;
    return true;
}

void UMythicAffixApplicationComponent::RequestAuthoritativeReconciliation() {
    if (!HasServerAuthority()) {
        return;
    }
    if (bReconciliationInProgress || bAuthoritativeEnumerationInProgress) {
        bReconciliationRequested = true;
        return;
    }
    ReconcileFromAuthoritativeSnapshots();
}

void UMythicAffixApplicationComponent::NotifyAbilitySystemActorInfoChanged(
    UMythicAbilitySystemComponent *InAbilitySystemComponent) {
    if (!HasServerAuthority() || !InAbilitySystemComponent) {
        return;
    }
    if (AbilitySystemComponent && AbilitySystemComponent != InAbilitySystemComponent) {
        UMythicAbilitySystemComponent *Previous = AbilitySystemComponent;
        FMythicPermanentStatReconcileResult ClearResult;
        if (!PermanentStatLedger.ClearTransactional(*Previous, ClearResult)) {
            bApplicationQuarantined = true;
            UE_LOG(Myth, Error,
                   TEXT("ASC recreation could not restore external stat bases on %s: %s"),
                   *GetNameSafe(GetOwner()), *ClearResult.Error);
            return;
        }

        PermanentStatLedger.Abandon();
        PublishPermanentStatLayer();
        AbilitySystemComponent = InAbilitySystemComponent;
    }
    else {
        AbilitySystemComponent = InAbilitySystemComponent;
    }
    BindRegistryReadiness();
    RequestAuthoritativeReconciliation();
}

UMythicAffixApplicationComponent::EAuthoritativeCollectionResult
UMythicAffixApplicationComponent::BuildAuthoritativeCandidateSet(
    TArray<FMythicAffixApplicationCandidate> &OutCandidates,
    TSet<FGuid> &OutInvalidSourceItems,
    const TConstArrayView<FMythicAffixEquipmentSlotOverride> SlotOverrides,
    UMythicItemInstance *SnapshotOverrideSource,
    const TConstArrayView<FRolledAffix> ProposedBaseSnapshots) {
    OutCandidates.Reset();
    OutInvalidSourceItems.Reset();

    UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicItemizationDataRegistrySubsystem *Registry =
        GameInstance ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    if (!Registry || !Registry->IsCoreSemanticReady()) {
        BindRegistryReadiness();
        return EAuthoritativeCollectionResult::Deferred;
    }

    IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(GetOwner());
    if (!Provider) {
        if (const APawn *Pawn = Cast<APawn>(GetOwner())) {
            Provider = Cast<IInventoryProviderInterface>(Pawn->GetController());
            if (!Provider) Provider = Cast<IInventoryProviderInterface>(Pawn->GetPlayerState());
        }
        else if (const APlayerState *PlayerState = Cast<APlayerState>(GetOwner())) {
            Provider = Cast<IInventoryProviderInterface>(PlayerState->GetPlayerController());
        }
    }
    if (!Provider) {
        return EAuthoritativeCollectionResult::Deferred;
    }

    const TArray<UMythicInventoryComponent *> Inventories = Provider->GetAllInventoryComponents();
    if (Inventories.IsEmpty()) {
        // A PlayerState can begin play before possession establishes its controller/inventory provider. An actually
        // initialized provider with zero equipment is authoritative; an unpossessed PlayerState is not.
        if (const APlayerState *PlayerState = Cast<APlayerState>(GetOwner());
            PlayerState && !PlayerState->GetPlayerController()) {
            return EAuthoritativeCollectionResult::Deferred;
        }
    }

    TSet<const FMythicAffixEquipmentSlotOverride *> ConsumedOverrides;
    for (int32 OverrideIndex = 0; OverrideIndex < SlotOverrides.Num(); ++OverrideIndex) {
        const FMythicAffixEquipmentSlotOverride &Override = SlotOverrides[OverrideIndex];
        if (!Override.Inventory || Override.SlotIndex < 0
            || !Override.Inventory->GetAllSlots().IsValidIndex(Override.SlotIndex)
            || !Override.Inventory->GetAllSlots()[Override.SlotIndex].IsGearSlot()) {
            return EAuthoritativeCollectionResult::InvalidSourcesExcluded;
        }
        for (int32 PriorIndex = 0; PriorIndex < OverrideIndex; ++PriorIndex) {
            if (SlotOverrides[PriorIndex].Inventory == Override.Inventory
                && SlotOverrides[PriorIndex].SlotIndex == Override.SlotIndex) {
                return EAuthoritativeCollectionResult::InvalidSourcesExcluded;
            }
        }
    }

    TMap<FGuid, UMythicItemInstance *> ItemsByGuid;
    bool bDeferred = false;
    bool bHadUnidentifiedInvalidSource = false;
    bool bConsumedSnapshotOverride = SnapshotOverrideSource == nullptr;
    for (UMythicInventoryComponent *Inventory : Inventories) {
        if (!Inventory) {
            continue;
        }
        const TArray<FMythicInventorySlotEntry> &InventorySlots = Inventory->GetAllSlots();
        for (int32 SlotIndex = 0; SlotIndex < InventorySlots.Num(); ++SlotIndex) {
            const FMythicInventorySlotEntry &Slot = InventorySlots[SlotIndex];
            UMythicItemInstance *Item = Slot.SlottedItemInstance.Get();
            for (const FMythicAffixEquipmentSlotOverride &Override : SlotOverrides) {
                if (Override.Inventory == Inventory && Override.SlotIndex == SlotIndex) {
                    Item = Override.ProposedItem;
                    ConsumedOverrides.Add(&Override);
                    break;
                }
            }
            if (!Slot.IsGearSlot()) {
                Item = nullptr;
            }
            if (!Item) {
                continue;
            }

            const FGuid ItemGuid = Item->GetItemInstanceGuid();
            if (!ItemGuid.IsValid()) {
                bHadUnidentifiedInvalidSource = true;
                UE_LOG(Myth, Error, TEXT("Equipped item %s has no stable ItemInstanceGuid; affixes are excluded."),
                       *GetNameSafe(Item));
                continue;
            }
            if (UMythicItemInstance **Existing = ItemsByGuid.Find(ItemGuid)) {
                if (*Existing != Item) {
                    OutInvalidSourceItems.Add(ItemGuid);
                    UE_LOG(Myth, Error,
                           TEXT("Two equipped items claim ItemInstanceGuid %s; both sources are quarantined."),
                           *ItemGuid.ToString());
                }
                continue;
            }
            ItemsByGuid.Add(ItemGuid, Item);

            const bool bHasSnapshotOverride = Item == SnapshotOverrideSource;
            bConsumedSnapshotOverride |= bHasSnapshotOverride;

            if (const UDurabilityFragment *Durability = Item->GetFragment<UDurabilityFragment>();
                Durability && Durability->IsBroken()) {
                continue; // A broken equipped item contributes neither base nor socket/gem affixes.
            }

            const bool bClosureReady = RequestItemDataClosure(*Item, *Registry);
            const int32 CandidateStart = OutCandidates.Num();
            bool bAppendDeferred = false;
            if (!AppendEquippedItemCandidates(*Item, *Registry, OutCandidates, bAppendDeferred,
                                              ProposedBaseSnapshots, bHasSnapshotOverride)) {
                OutCandidates.SetNum(CandidateStart, EAllowShrinking::No);
                OutInvalidSourceItems.Add(ItemGuid);
                continue;
            }
            if (!bClosureReady || bAppendDeferred) {
                // Item activation is atomic: never publish a partial base/gem/socket set while one exact semantic
                // closure is still loading.
                OutCandidates.SetNum(CandidateStart, EAllowShrinking::No);
                bDeferred = true;
            }
        }
    }

    if (ConsumedOverrides.Num() != SlotOverrides.Num() || !bConsumedSnapshotOverride) {
        return EAuthoritativeCollectionResult::InvalidSourcesExcluded;
    }

    TMap<FGuid, FGuid> SourceByRollGuid;
    for (const FMythicAffixApplicationCandidate &Candidate : OutCandidates) {
        UMythicItemInstance *Item = Candidate.SourceItem.Get();
        if (!Item || !Candidate.Snapshot.RollGuid.IsValid()) {
            if (Item) OutInvalidSourceItems.Add(Item->GetItemInstanceGuid());
            continue;
        }
        const FGuid SourceGuid = Item->GetItemInstanceGuid();
        if (const FGuid *ExistingSource = SourceByRollGuid.Find(Candidate.Snapshot.RollGuid)) {
            OutInvalidSourceItems.Add(*ExistingSource);
            OutInvalidSourceItems.Add(SourceGuid);
        }
        else {
            SourceByRollGuid.Add(Candidate.Snapshot.RollGuid, SourceGuid);
        }
    }
    OutCandidates.RemoveAll([&OutInvalidSourceItems](const FMythicAffixApplicationCandidate &Candidate) {
        const UMythicItemInstance *Item = Candidate.SourceItem.Get();
        return !Item || OutInvalidSourceItems.Contains(Item->GetItemInstanceGuid());
    });

    // Incompleteness always wins over corruption reporting. Quarantine/removal is itself a state transition and must
    // wait until every required item closure is complete.
    if (bDeferred) {
        return EAuthoritativeCollectionResult::Deferred;
    }
    if (!OutInvalidSourceItems.IsEmpty() || bHadUnidentifiedInvalidSource) {
        return EAuthoritativeCollectionResult::InvalidSourcesExcluded;
    }
    return EAuthoritativeCollectionResult::Ready;
}

bool UMythicAffixApplicationComponent::AppendEquippedItemCandidates(
    UMythicItemInstance &SourceItem,
    UMythicItemizationDataRegistrySubsystem &Registry,
    TArray<FMythicAffixApplicationCandidate> &OutCandidates,
    bool &bOutDeferred,
    const TConstArrayView<FRolledAffix> ProposedBaseSnapshots,
    const bool bHasBaseSnapshotOverride) const {
    // Collection owns the exact loaded semantic closure; ValidateCandidate resolves through this same subsystem.
    (void)Registry;
    bOutDeferred = false;
    if (!SourceItem.GetItemInstanceGuid().IsValid()) {
        return false;
    }

    TArray<const FRolledAffix *> Snapshots;
    if (const UAffixesFragment *Affixes = SourceItem.GetFragment<UAffixesFragment>()) {
        if (Affixes->IsCorrupted() || !Affixes->AffixesConfig.AffixProfile.IsValid()) {
            return false;
        }
        const int32 BaseSnapshotCount = bHasBaseSnapshotOverride
            ? ProposedBaseSnapshots.Num()
            : Affixes->GetAffixSnapshots().Items.Num();
        if (BaseSnapshotCount > MythicAffixSerialization::MaxAffixesPerContainer) {
            return false;
        }
        if (bHasBaseSnapshotOverride) {
            for (const FRolledAffix &Snapshot : ProposedBaseSnapshots) {
                Snapshots.Add(&Snapshot);
            }
        }
        else {
            for (const FMythicReplicatedAffixItem &Item : Affixes->GetAffixSnapshots().Items) {
                Snapshots.Add(&Item.Affix);
            }
        }
    }
    if (const UMythicGemFragment *Gem = SourceItem.GetFragment<UMythicGemFragment>()) {
        if (Gem->GrantedAffixSnapshots.Items.Num() > MythicAffixSerialization::MaxAffixesPerContainer) {
            return false;
        }
        if (!Gem->GrantSpecs.IsEmpty() && Gem->GrantedAffixSnapshots.Items.IsEmpty()) {
            bOutDeferred = true;
        }
        for (const FMythicReplicatedAffixItem &Item : Gem->GrantedAffixSnapshots.Items) {
            Snapshots.Add(&Item.Affix);
        }
    }
    if (const USocketsFragment *Sockets = SourceItem.GetFragment<USocketsFragment>()) {
        if (Sockets->SocketStates.Items.Num() > MythicSocketSerialization::MaxSocketsPerItem) {
            return false;
        }
        for (const FMythicReplicatedSocketItem &Socket : Sockets->SocketStates.Items) {
            if (!Socket.bFilled) {
                if (Socket.SourceGemItemGuid.IsValid() || Socket.SocketedGemType.IsValid()
                    || !Socket.SocketedAffixSnapshots.IsEmpty()) {
                    return false;
                }
                continue;
            }
            if (!Socket.SocketGuid.IsValid() || !Socket.SourceGemItemGuid.IsValid()
                || !Socket.SocketedGemType.IsValid() || Socket.SocketedAffixSnapshots.IsEmpty()
                || Socket.SocketedAffixSnapshots.Num() > MythicSocketSerialization::MaxAffixesPerSocket) {
                return false;
            }
            for (const FRolledAffix &Snapshot : Socket.SocketedAffixSnapshots) {
                if (Snapshot.Provenance.SourceKind != AFFIX_SOURCE_SOCKET
                    || Snapshot.Provenance.OriginSocketGuid != Socket.SocketGuid
                    || Snapshot.Provenance.SourceItemGuid != Socket.SourceGemItemGuid) {
                    return false;
                }
                Snapshots.Add(&Snapshot);
            }
        }
    }

    TArray<FMythicAffixApplicationCandidate> ItemCandidates;
    ItemCandidates.Reserve(Snapshots.Num());
    TSet<FGuid> ItemRollGuids;
    for (const FRolledAffix *Snapshot : Snapshots) {
        if (!Snapshot || !Snapshot->RollGuid.IsValid() || ItemRollGuids.Contains(Snapshot->RollGuid)) {
            return false;
        }
        ItemRollGuids.Add(Snapshot->RollGuid);

        FMythicAppliedAffixState ValidationState;
        ValidationState.RollGuid = Snapshot->RollGuid;
        ValidationState.SourceItemGuid = SourceItem.GetItemInstanceGuid();
        ValidationState.SourceItem = &SourceItem;
        ValidationState.Snapshot = *Snapshot;
        if (!ValidateCandidate(ValidationState)) {
            return false;
        }

        FMythicAffixApplicationCandidate &Candidate = ItemCandidates.AddDefaulted_GetRef();
        Candidate.SourceItem = &SourceItem;
        Candidate.Snapshot = *Snapshot;
    }
    if (bOutDeferred) {
        return true;
    }
    OutCandidates.Append(MoveTemp(ItemCandidates));
    return true;
}

bool UMythicAffixApplicationComponent::RequestItemDataClosure(
    UMythicItemInstance &SourceItem,
    UMythicItemizationDataRegistrySubsystem &Registry) {
    bool bReady = true;
    if (UAffixesFragment *Affixes = const_cast<UAffixesFragment *>(SourceItem.GetFragment<UAffixesFragment>())) {
        auto RequestProfile = [this, &Registry, &bReady](const FPrimaryAssetId ProfileId) {
            if (!ProfileId.IsValid() || Registry.IsProfileReady(ProfileId)) return;
            bReady = false;
            if (!PendingProfileClosures.Contains(ProfileId)) {
                PendingProfileClosures.Add(ProfileId);
                TWeakObjectPtr<UMythicAffixApplicationComponent> WeakThis(this);
                Registry.RequestProfileClosureAsync(
                    ProfileId,
                    FOnMythicItemizationDataReady::CreateLambda(
                        [WeakThis, ProfileId](const bool bSuccess) {
                            if (!WeakThis.IsValid()) return;
                            WeakThis->PendingProfileClosures.Remove(ProfileId);
                            if (bSuccess) {
                                WeakThis->RequestAuthoritativeReconciliation();
                            }
                            else {
                                UE_LOG(Myth, Error,
                                       TEXT("Authoritative equipment closure %s failed; its item remains fail-closed."),
                                       *ProfileId.ToString());
                            }
                        }));
            }
        };

        // Already-rolled equipment applies from the core Affix/Stat Definition closure only. A profile closure is
        // generation input and must not be an application-time dependency (especially not an indirect GE manifest).
        if (Affixes->GetAffixSnapshots().Items.IsEmpty()) {
            RequestProfile(Affixes->AffixesConfig.AffixProfile.GetPrimaryAssetId());
        }
    }
    if (UMythicGemFragment *Gem = const_cast<UMythicGemFragment *>(SourceItem.GetFragment<UMythicGemFragment>())) {
        const bool bNeedsGemClosure =
            !Gem->GrantSpecs.IsEmpty() && Gem->GrantedAffixSnapshots.Items.IsEmpty();
        if (bNeedsGemClosure) {
            bReady = false;
            Gem->RequestRuntimeData();
        }
    }
    if (USocketsFragment *Sockets = const_cast<USocketsFragment *>(SourceItem.GetFragment<USocketsFragment>())) {
        const bool bNeedsSocketClosure =
            !Sockets->SocketStates.Items.IsEmpty() && !Sockets->IsRuntimeDataReady();
        if (bNeedsSocketClosure) {
            bReady = false;
            Sockets->RequestRuntimeData();
        }
    }
    return bReady;
}

bool UMythicAffixApplicationComponent::ReplaceAuthoritativeSetTransactional(
    const TConstArrayView<FMythicAffixApplicationCandidate> Candidates) {
    if (!HasServerAuthority() || !ResolveAbilitySystem()) {
        return false;
    }

    TMap<FGuid, FMythicAppliedAffixState> DesiredLedger;
    DesiredLedger.Reserve(Candidates.Num());
    for (const FMythicAffixApplicationCandidate &Input : Candidates) {
        UMythicItemInstance *SourceItem = Input.SourceItem.Get();
        if (!SourceItem || DesiredLedger.Contains(Input.Snapshot.RollGuid)) {
            return false;
        }

        FMythicAppliedAffixState Candidate;
        Candidate.RollGuid = Input.Snapshot.RollGuid;
        Candidate.SourceItemGuid = SourceItem->GetItemInstanceGuid();
        Candidate.SourceItem = SourceItem;
        Candidate.Snapshot = Input.Snapshot;
        if (!ValidateCandidate(Candidate)) {
            return false;
        }
        DesiredLedger.Add(Candidate.RollGuid, MoveTemp(Candidate));
    }
    return TransitionLedgerTransactional(MoveTemp(DesiredLedger));
}

bool UMythicAffixApplicationComponent::IsRegistered(const FGuid RollGuid) const {
    return Ledger.Contains(RollGuid);
}

bool UMythicAffixApplicationComponent::IsActive(const FGuid RollGuid) const {
    return ActiveRollGuids.Contains(RollGuid);
}

void UMythicAffixApplicationComponent::GetQuarantinedSourceItems(TArray<FGuid> &OutSourceItemGuids) const {
    TSet<FGuid> Combined = AuthoritativeDataQuarantinedSourceItemGuids;
    Combined.Append(FatalQuarantinedSourceItemGuids);
    OutSourceItemGuids = Combined.Array();
    OutSourceItemGuids.RemoveAll([](const FGuid &Guid) { return !Guid.IsValid(); });
    SortGuids(OutSourceItemGuids);
}

bool UMythicAffixApplicationComponent::ValidateCandidate(FMythicAppliedAffixState &Candidate) const {
    if (!Candidate.RollGuid.IsValid() || !Candidate.SourceItemGuid.IsValid() ||
        Candidate.Snapshot.RollGuid != Candidate.RollGuid || !Candidate.Snapshot.IsGameplayValid()) {
        return false;
    }
    if (Candidate.Snapshot.Provenance.SourceItemGuid.IsValid()
        && Candidate.Snapshot.Provenance.SourceItemGuid != Candidate.SourceItemGuid) {
        const bool bValidSocketCopy = Candidate.Snapshot.Provenance.SourceKind == AFFIX_SOURCE_SOCKET
            && Candidate.Snapshot.Provenance.OriginSocketGuid.IsValid()
            && Candidate.Snapshot.Provenance.SourceItemGuid.IsValid();
        if (!bValidSocketCopy) return false;
    }
    const UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    const FPrimaryAssetId DefinitionId = Candidate.Snapshot.AffixDefinition.GetPrimaryAssetId();
    const UMythicAffixDefinition *Definition = Registry ? Registry->FindAffix(DefinitionId) : nullptr;
    if (!Definition || !MythicAffix::IsSupportedModifierOp(Definition->ModifierOp)) {
        return false;
    }
    const UMythicStatDefinition *Stat = Registry->FindStat(Definition->TargetStat.GetPrimaryAssetId());
    if (!Stat || !Stat->bCanBeAffixTarget || !Stat->StatTag.IsValid() || !Stat->Attribute.IsValid()
        || !FMath::IsFinite(Candidate.Snapshot.Magnitude)
        || (MythicAffix::ModifierRequiresNonZeroMagnitude(Definition->ModifierOp)
            && FMath::IsNearlyZero(Candidate.Snapshot.Magnitude))) {
        return false;
    }
    Candidate.TargetStatTag = Stat->StatTag;
    Candidate.StackingGroup = Definition->GetEffectiveStackingGroup();
    Candidate.StackingRule = Definition->StackingRule;
    Candidate.ConflictGroups = Definition->ConflictGroups;
    Candidate.ModifierOp = Definition->ModifierOp;
    Candidate.ComparisonDirection = Stat->ComparisonDirection;
    Candidate.NeutralValue = Stat->NeutralValue;
    Candidate.Magnitude = Candidate.Snapshot.Magnitude;
    if (Candidate.StackingRule != EMythicAffixStackingRule::StackAll
        && !Candidate.StackingGroup.IsValid()) return false;
    if ((Candidate.StackingRule == EMythicAffixStackingRule::HighestPerItem
         || Candidate.StackingRule == EMythicAffixStackingRule::HighestOverall)
        && Candidate.ComparisonDirection == EMythicStatComparisonDirection::Neutral) return false;
    return true;
}

bool UMythicAffixApplicationComponent::ResolveCandidateContributions(
    const FMythicAppliedAffixState &Candidate,
    TArray<FMythicPermanentStatContribution> &OutContributions,
    FString *OutFailureReason) const {
    OutContributions.Reset();
    auto Fail = [OutFailureReason](const FString &Reason) {
        if (OutFailureReason) {
            *OutFailureReason = Reason;
        }
        return false;
    };

    const UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UMythicItemizationDataRegistrySubsystem *Registry =
        GameInstance ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    if (!Registry) {
        return Fail(TEXT("The canonical itemization registry is unavailable."));
    }

    const UMythicAffixDefinition *Definition = Registry->FindAffix(
        Candidate.Snapshot.AffixDefinition.GetPrimaryAssetId());
    if (!Definition || !MythicAffix::IsSupportedModifierOp(Definition->ModifierOp)) {
        return Fail(TEXT("The direct Affix Definition reference is unloaded or has an unsupported operation."));
    }

    const FPrimaryAssetId TargetStatId = Definition->TargetStat.GetPrimaryAssetId();
    const UMythicStatDefinition *Stat = TargetStatId.IsValid()
                                            ? Registry->FindStat(TargetStatId)
                                            : Definition->TargetStat.GetAsset();
    if (!Stat || !Stat->bCanBeAffixTarget || !Stat->StatTag.IsValid() || !Stat->Attribute.IsValid()) {
        return Fail(TEXT("The current Affix Definition does not resolve to an affix-safe Stat Definition."));
    }
    if (!FMath::IsFinite(Candidate.Snapshot.Magnitude)
        || (MythicAffix::ModifierRequiresNonZeroMagnitude(Definition->ModifierOp)
            && FMath::IsNearlyZero(Candidate.Snapshot.Magnitude))) {
        return Fail(TEXT("Rolled magnitude is invalid for the current Affix Definition's operation."));
    }

    FMythicPermanentStatContribution &Contribution = OutContributions.AddDefaulted_GetRef();
    Contribution.SourceGuid = Candidate.RollGuid;
    Contribution.Attribute = Stat->Attribute;
    Contribution.ModifierOp = Definition->ModifierOp;
    Contribution.Magnitude = Candidate.Snapshot.Magnitude;
    Contribution.Layer = EMythicPermanentStatContributionLayer::Equipment;

    return true;
}

bool UMythicAffixApplicationComponent::ResolvePermanentStatSource(
    const FMythicPermanentStatSourceSpec &Spec,
    FMythicPermanentStatContribution &OutContribution) const {
    const UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    const FPrimaryAssetId StatId = Spec.StatDefinition.GetPrimaryAssetId();
    const UMythicStatDefinition *AuthoredStat = Spec.StatDefinition.GetAsset();
    const UMythicStatDefinition *RegisteredStat =
        Registry && Registry->IsCoreSemanticReady() && StatId.IsValid()
            ? Registry->FindStat(StatId) : nullptr;
    if (!Spec.SourceGuid.IsValid() || !RegisteredStat || RegisteredStat != AuthoredStat
        || !RegisteredStat->Attribute.IsValid() || !AbilitySystemComponent
        || !AbilitySystemComponent->HasAttributeSetForAttribute(RegisteredStat->Attribute)
        || !MythicAffix::IsSupportedModifierOp(Spec.ModifierOp.GetValue())
        || !FMath::IsFinite(Spec.Magnitude)
        || (MythicAffix::ModifierRequiresNonZeroMagnitude(Spec.ModifierOp.GetValue())
            && FMath::IsNearlyZero(Spec.Magnitude))) {
        return false;
    }

    OutContribution.SourceGuid = Spec.SourceGuid;
    OutContribution.Attribute = RegisteredStat->Attribute;
    OutContribution.ModifierOp = Spec.ModifierOp;
    OutContribution.Magnitude = Spec.Magnitude;
    OutContribution.Layer = EMythicPermanentStatContributionLayer::Progression;
    return true;
}

bool UMythicAffixApplicationComponent::BuildDesiredPermanentContributions(
    const TMap<FGuid, FMythicAppliedAffixState> &Candidates,
    const TSet<FGuid> &ActiveRolls,
    const TMap<FGuid, FMythicPermanentStatContribution> &PermanentSources,
    TArray<FMythicPermanentStatContribution> &OutContributions,
    FString *OutFailureReason) const {
    OutContributions.Reset();
    TArray<FGuid> OrderedRolls = ActiveRolls.Array();
    SortGuids(OrderedRolls);
    for (const FGuid RollGuid : OrderedRolls) {
        const FMythicAppliedAffixState *Candidate = Candidates.Find(RollGuid);
        if (!Candidate) {
            if (OutFailureReason) *OutFailureReason = TEXT("Stacking winner is absent from the candidate ledger.");
            return false;
        }
        TArray<FMythicPermanentStatContribution> CandidateContributions;
        if (!ResolveCandidateContributions(*Candidate, CandidateContributions, OutFailureReason)) {
            return false;
        }
        OutContributions.Append(MoveTemp(CandidateContributions));
    }
    TArray<FGuid> OrderedPermanentSources;
    PermanentSources.GetKeys(OrderedPermanentSources);
    SortGuids(OrderedPermanentSources);
    for (const FGuid SourceGuid : OrderedPermanentSources) {
        const FMythicPermanentStatContribution *Contribution = PermanentSources.Find(SourceGuid);
        if (!Contribution || Contribution->SourceGuid != SourceGuid
            || Contribution->Layer != EMythicPermanentStatContributionLayer::Progression) {
            if (OutFailureReason) {
                *OutFailureReason = TEXT("Permanent progression/reward source map is internally inconsistent.");
            }
            return false;
        }
        OutContributions.Add(*Contribution);
    }
    return true;
}

bool UMythicAffixApplicationComponent::ComputeDesiredActiveRolls(
    TMap<FGuid, FMythicAppliedAffixState> &Candidates,
    TSet<FGuid> &OutActiveRolls) const {
    OutActiveRolls.Reset();
    for (TPair<FGuid, FMythicAppliedAffixState> &Pair : Candidates) {
        if (!ValidateCandidate(Pair.Value)) {
            return false;
        }
    }
    return ComputeStackingWinners(Candidates, OutActiveRolls);
}

bool UMythicAffixApplicationComponent::ComputeStackingWinners(
    const TMap<FGuid, FMythicAppliedAffixState> &Candidates,
    TSet<FGuid> &OutActiveRolls) {
    OutActiveRolls.Reset();
    TSet<FGuid> StackingActiveRolls;
    for (const TPair<FGuid, FMythicAppliedAffixState> &Pair : Candidates) {
        const FMythicAppliedAffixState &Candidate = Pair.Value;

        if (Candidate.StackingRule == EMythicAffixStackingRule::StackAll) {
            StackingActiveRolls.Add(Candidate.RollGuid);
            continue;
        }

        const FMythicAppliedAffixState *Winner = &Candidate;
        for (const TPair<FGuid, FMythicAppliedAffixState> &OtherPair : Candidates) {
            const FMythicAppliedAffixState &Other = OtherPair.Value;
            if (Other.StackingGroup != Candidate.StackingGroup) {
                continue;
            }
            if (Other.StackingRule != Candidate.StackingRule) {
                return false; // One stacking group cannot have contradictory control semantics.
            }

            if (Candidate.StackingRule == EMythicAffixStackingRule::HighestPerItem ||
                Candidate.StackingRule == EMythicAffixStackingRule::HighestOverall) {
                if (Candidate.TargetStatTag != Other.TargetStatTag
                    || Candidate.ComparisonDirection != Other.ComparisonDirection
                    || Candidate.NeutralValue != Other.NeutralValue
                    || Candidate.ComparisonDirection == EMythicStatComparisonDirection::Neutral) {
                    return false;
                }
            }

            const bool bSameItem = Other.SourceItemGuid == Candidate.SourceItemGuid;
            if ((Candidate.StackingRule == EMythicAffixStackingRule::UniquePerItem ||
                 Candidate.StackingRule == EMythicAffixStackingRule::HighestPerItem) && !bSameItem) {
                continue;
            }

            if (Candidate.StackingRule == EMythicAffixStackingRule::UniquePerItem) {
                if (GuidLexicalLess(Other.RollGuid, Winner->RollGuid)) {
                    Winner = &Other;
                }
            }
            else if (IsBetterHighestCandidate(Other, *Winner)) {
                Winner = &Other;
            }
        }
        StackingActiveRolls.Add(Winner->RollGuid);
    }

    // Conflict groups are mutual exclusion across the complete equipped set, not just within one generated item.
    // Different conflict members may target incomparable stats, so inventing a magnitude score here would make
    // balance depend on presentation semantics. A stable physical-item/roll ordering provides a deterministic,
    // save-stable winner and naturally promotes the next candidate when the incumbent leaves the authoritative set.
    TArray<const FMythicAppliedAffixState *> ConflictOrder;
    ConflictOrder.Reserve(StackingActiveRolls.Num());
    for (const FGuid RollGuid : StackingActiveRolls) {
        ConflictOrder.Add(&Candidates.FindChecked(RollGuid));
    }
    ConflictOrder.Sort([](const FMythicAppliedAffixState &Left,
                          const FMythicAppliedAffixState &Right) {
        if (Left.SourceItemGuid != Right.SourceItemGuid) {
            return UMythicAffixApplicationComponent::GuidLexicalLess(
                Left.SourceItemGuid, Right.SourceItemGuid);
        }
        return UMythicAffixApplicationComponent::GuidLexicalLess(Left.RollGuid, Right.RollGuid);
    });

    TSet<FGameplayTag> ClaimedConflictGroups;
    for (const FMythicAppliedAffixState *Candidate : ConflictOrder) {
        bool bConflicts = false;
        for (const FGameplayTag &ConflictGroup : Candidate->ConflictGroups) {
            if (ClaimedConflictGroups.Contains(ConflictGroup)) {
                bConflicts = true;
                break;
            }
        }
        if (bConflicts) {
            continue;
        }
        OutActiveRolls.Add(Candidate->RollGuid);
        for (const FGameplayTag &ConflictGroup : Candidate->ConflictGroups) {
            ClaimedConflictGroups.Add(ConflictGroup);
        }
    }
    return true;
}

bool UMythicAffixApplicationComponent::TransitionLedgerTransactional(
    TMap<FGuid, FMythicAppliedAffixState> &&DesiredLedger,
    const bool bForceFullRecompose) {
    if (bReconciliationInProgress || !HasServerAuthority() || !ResolveAbilitySystem()) {
        return false;
    }

    TGuardValue<bool> ReconciliationGuard(bReconciliationInProgress, true);
    TSet<FGuid> DesiredActiveRolls;
    if (!ComputeDesiredActiveRolls(DesiredLedger, DesiredActiveRolls)) {
        return false;
    }

    for (TPair<FGuid, FMythicAppliedAffixState> &Pair : DesiredLedger) {
        FMythicAppliedAffixState &Desired = Pair.Value;
        const bool bShouldBeActive = DesiredActiveRolls.Contains(Pair.Key);
        Desired.State = bShouldBeActive ? EMythicAffixApplyResult::Active : EMythicAffixApplyResult::Suppressed;
    }

    TArray<FMythicPermanentStatContribution> DesiredContributions;
    FString ResolutionFailure;
    if (!BuildDesiredPermanentContributions(
            DesiredLedger, DesiredActiveRolls, PermanentStatSources,
            DesiredContributions, &ResolutionFailure)) {
        UE_LOG(Myth, Error, TEXT("Permanent affix contribution resolution failed on %s: %s"),
               *GetNameSafe(GetOwner()), *ResolutionFailure);
        return false;
    }

#if WITH_DEV_AUTOMATION_TESTS
    bool bRemovesPermanentSource = bForceFullRecompose;
    bool bAddsPermanentSource = bForceFullRecompose;
    for (const TPair<FGuid, FMythicAppliedAffixState> &CurrentPair : Ledger) {
        if (CurrentPair.Value.State != EMythicAffixApplyResult::Active) continue;
        const FMythicAppliedAffixState *Desired = DesiredLedger.Find(CurrentPair.Key);
        if (!Desired || Desired->State != EMythicAffixApplyResult::Active
            || !SnapshotsEquivalent(CurrentPair.Value.Snapshot, Desired->Snapshot)) {
            bRemovesPermanentSource = true;
            break;
        }
    }
    for (const TPair<FGuid, FMythicAppliedAffixState> &DesiredPair : DesiredLedger) {
        if (DesiredPair.Value.State != EMythicAffixApplyResult::Active) continue;
        const FMythicAppliedAffixState *Current = Ledger.Find(DesiredPair.Key);
        if (!Current || Current->State != EMythicAffixApplyResult::Active
            || !SnapshotsEquivalent(Current->Snapshot, DesiredPair.Value.Snapshot)) {
            bAddsPermanentSource = true;
            break;
        }
    }
    if (bRemovesPermanentSource && ConsumeTestFailure(TestRemoveFailureCountdown)) {
        return false;
    }
    if (bAddsPermanentSource && ConsumeTestFailure(TestApplyFailureCountdown)) {
        return false;
    }
#else
    (void)bForceFullRecompose;
#endif

    FMythicPermanentStatReconcileResult ReconcileResult;
    if (!PermanentStatLedger.ReconcileTransactional(
            *AbilitySystemComponent, DesiredContributions, ReconcileResult)) {
        UE_LOG(Myth, Error, TEXT("Permanent affix stat transaction failed on %s: %s"),
               *GetNameSafe(GetOwner()), *ReconcileResult.Error);
        if (!ReconcileResult.bRollbackSucceeded) {
            QuarantineLedgerAfterRestoreFailure(MoveTemp(DesiredLedger));
            ensureAlwaysMsgf(false,
                TEXT("Permanent affix stat rollback failed on %s; application quarantined"),
                *GetNameSafe(GetOwner()));
        }
        return false;
    }
    Ledger = MoveTemp(DesiredLedger);
    ClearApplicationQuarantine();
    RebuildIndexes();
    PublishPermanentStatLayer();
    return true;
}

bool UMythicAffixApplicationComponent::GetPermanentStatLayerValues(
    const FGameplayAttribute Attribute,
    float &OutNonEquipmentBaseValue,
    float &OutEquipmentBaseValue) const {
    for (const FMythicReplicatedPermanentStatLayer &Entry : PermanentStatLayer) {
        if (Entry.Attribute == Attribute) {
            OutNonEquipmentBaseValue = Entry.NonEquipmentBaseValue;
            OutEquipmentBaseValue = Entry.EquipmentBaseValue;
            return true;
        }
    }
    return false;
}

bool UMythicAffixApplicationComponent::SetPermanentStatSourceTransactional(
    const FGuid SourceGuid,
    const FMythicStatDefinitionHandle StatDefinition,
    const TEnumAsByte<EGameplayModOp::Type> ModifierOp,
    const float Magnitude) {
    const FMythicPermanentStatSourceSpec Desired{
        SourceGuid, StatDefinition, ModifierOp, Magnitude};
    const TArray<FGuid> Owned{SourceGuid};
    const TArray<FMythicPermanentStatSourceSpec> Sources{Desired};
    return ReplacePermanentStatSourceSetTransactional(Owned, Sources);
}

bool UMythicAffixApplicationComponent::RemovePermanentStatSourceTransactional(
    const FGuid SourceGuid) {
    const TArray<FGuid> Owned{SourceGuid};
    return ReplacePermanentStatSourceSetTransactional(Owned, {});
}

bool UMythicAffixApplicationComponent::ReplacePermanentStatSourceSetTransactional(
    const TConstArrayView<FGuid> OwnedSourceGuids,
    const TConstArrayView<FMythicPermanentStatSourceSpec> DesiredSources) {
    if (bReconciliationInProgress || !HasServerAuthority() || bApplicationQuarantined
        || !ResolveAbilitySystem()) {
        return false;
    }

    TSet<FGuid> Owned;
    for (const FGuid SourceGuid : OwnedSourceGuids) {
        if (!SourceGuid.IsValid() || Owned.Contains(SourceGuid)) {
            return false;
        }
        Owned.Add(SourceGuid);
    }

    TMap<FGuid, FMythicPermanentStatContribution> StagedSources = PermanentStatSources;
    for (const FGuid SourceGuid : Owned) {
        StagedSources.Remove(SourceGuid);
    }
    TSet<FGuid> DesiredIdentities;
    for (const FMythicPermanentStatSourceSpec &Spec : DesiredSources) {
        if (!Owned.Contains(Spec.SourceGuid) || DesiredIdentities.Contains(Spec.SourceGuid)
            || StagedSources.Contains(Spec.SourceGuid)) {
            return false;
        }
        FMythicPermanentStatContribution Contribution;
        if (!ResolvePermanentStatSource(Spec, Contribution)) {
            return false;
        }
        DesiredIdentities.Add(Spec.SourceGuid);
        StagedSources.Add(Spec.SourceGuid, MoveTemp(Contribution));
    }

    TArray<FMythicPermanentStatContribution> DesiredContributions;
    FString ResolutionFailure;
    if (!BuildDesiredPermanentContributions(
            Ledger, ActiveRollGuids, StagedSources, DesiredContributions, &ResolutionFailure)) {
        UE_LOG(Myth, Error, TEXT("Permanent stat source resolution failed on %s: %s"),
               *GetNameSafe(GetOwner()), *ResolutionFailure);
        return false;
    }

    TGuardValue<bool> ReconciliationGuard(bReconciliationInProgress, true);
    FMythicPermanentStatReconcileResult Result;
    if (!PermanentStatLedger.ReconcileTransactional(
            *AbilitySystemComponent, DesiredContributions, Result)) {
        if (!Result.bRollbackSucceeded) {
            bApplicationQuarantined = true;
        }
        UE_LOG(Myth, Error, TEXT("Permanent stat source transaction failed on %s: %s"),
               *GetNameSafe(GetOwner()), *Result.Error);
        return false;
    }

    PermanentStatSources = MoveTemp(StagedSources);
    PublishPermanentStatLayer();
    return true;
}

void UMythicAffixApplicationComponent::PublishPermanentStatLayer() {
    TArray<FMythicPermanentStatLayerSnapshot> LedgerSnapshots;
    PermanentStatLedger.GetLayerSnapshots(LedgerSnapshots);

    TArray<FMythicReplicatedPermanentStatLayer> NewLayer;
    NewLayer.Reserve(LedgerSnapshots.Num());
    for (const FMythicPermanentStatLayerSnapshot &Snapshot : LedgerSnapshots) {
        FMythicReplicatedPermanentStatLayer &Entry = NewLayer.AddDefaulted_GetRef();
        Entry.Attribute = Snapshot.Attribute;
        Entry.NonEquipmentBaseValue = Snapshot.NonEquipmentBase;
        Entry.EquipmentBaseValue = Snapshot.EquipmentBase;
    }
    if (PermanentStatLayer == NewLayer) {
        return;
    }
    PermanentStatLayer = MoveTemp(NewLayer);
    PermanentStatLayerChanged.Broadcast();
    if (AActor *Owner = GetOwner()) {
        Owner->ForceNetUpdate();
    }
}

void UMythicAffixApplicationComponent::OnRep_PermanentStatLayer() {
    PermanentStatLayerChanged.Broadcast();
}

#if WITH_DEV_AUTOMATION_TESTS
bool UMythicAffixApplicationComponent::ConsumeTestFailure(int32 &Countdown) {
    if (Countdown == INDEX_NONE) {
        return false;
    }
    if (Countdown == 0) {
        Countdown = INDEX_NONE;
        return true;
    }
    --Countdown;
    return false;
}
#endif

void UMythicAffixApplicationComponent::QuarantineLedgerAfterRestoreFailure(
    TMap<FGuid, FMythicAppliedAffixState> &&FailedLedger) {
    FatalQuarantinedSourceItemGuids.Reset();
    for (TPair<FGuid, FMythicAppliedAffixState> &Pair : FailedLedger) {
        Pair.Value.State = EMythicAffixApplyResult::Suppressed;
        if (Pair.Value.SourceItemGuid.IsValid()) {
            FatalQuarantinedSourceItemGuids.Add(Pair.Value.SourceItemGuid);
        }
    }

    Ledger = MoveTemp(FailedLedger);
    bApplicationQuarantined = true;
    RebuildIndexes();
    UE_LOG(Myth, Error,
           TEXT("Affix application ledger on %s quarantined %d source item(s) after permanent-base rollback failure"),
           *GetNameSafe(GetOwner()), FatalQuarantinedSourceItemGuids.Num());
}

void UMythicAffixApplicationComponent::QuarantineApplicationAfterSemanticReconciliationFailure(
    const uint64 SemanticRevision) {
    FatalQuarantinedSourceItemGuids.Reset();
    for (TPair<FGuid, FMythicAppliedAffixState> &Pair : Ledger) {
        Pair.Value.State = EMythicAffixApplyResult::Suppressed;
        if (Pair.Value.SourceItemGuid.IsValid()) {
            FatalQuarantinedSourceItemGuids.Add(Pair.Value.SourceItemGuid);
        }
    }

    bool bPermanentLayerCleared = PermanentStatLedger.IsEmpty();
    FMythicPermanentStatReconcileResult ClearResult;
    if (!bPermanentLayerCleared && IsValid(AbilitySystemComponent)) {
        bPermanentLayerCleared = PermanentStatLedger.ClearTransactional(
            *AbilitySystemComponent, ClearResult);
    }
    else if (!bPermanentLayerCleared) {
        ClearResult.Error = TEXT("The authoritative Ability System Component is unavailable.");
    }
    if (bPermanentLayerCleared) {
        PermanentStatLedger.Abandon();
    }

    bApplicationQuarantined = true;
    RebuildIndexes();
    PublishPermanentStatLayer();
    UE_LOG(Myth, Error,
           TEXT("Affix application on %s quarantined %d source item(s) because semantic revision %llu could not reconcile; permanent layer cleared: %s%s%s"),
           *GetNameSafe(GetOwner()), FatalQuarantinedSourceItemGuids.Num(),
           SemanticRevision, bPermanentLayerCleared ? TEXT("yes") : TEXT("no"),
           ClearResult.Error.IsEmpty() ? TEXT("") : TEXT("; "),
           *ClearResult.Error);
}

void UMythicAffixApplicationComponent::ClearApplicationQuarantine() {
    bApplicationQuarantined = false;
    FatalQuarantinedSourceItemGuids.Reset();
}

void UMythicAffixApplicationComponent::RebuildIndexes() {
    RollGuidsBySourceItem.Reset();
    RollGuidsByStackingGroup.Reset();
    RollGuidsByConflictGroup.Reset();
    RollGuidsByStat.Reset();
    ActiveRollGuids.Reset();
    SuppressedRollGuids.Reset();

    for (const TPair<FGuid, FMythicAppliedAffixState> &Pair : Ledger) {
        const FGuid RollGuid = Pair.Key;
        const FMythicAppliedAffixState &State = Pair.Value;
        RollGuidsBySourceItem.Add(State.SourceItemGuid, RollGuid);
        if (State.StackingGroup.IsValid()) {
            RollGuidsByStackingGroup.Add(State.StackingGroup, RollGuid);
        }
        for (const FGameplayTag &ConflictGroup : State.ConflictGroups) {
            RollGuidsByConflictGroup.Add(ConflictGroup, RollGuid);
        }
        if (State.TargetStatTag.IsValid()) RollGuidsByStat.Add(State.TargetStatTag, RollGuid);
        if (State.State == EMythicAffixApplyResult::Active) {
            ActiveRollGuids.Add(RollGuid);
        }
        else {
            SuppressedRollGuids.Add(RollGuid);
        }
    }
}

bool UMythicAffixApplicationComponent::SnapshotsEquivalent(const FRolledAffix &A, const FRolledAffix &B) {
    return A.RollGuid == B.RollGuid && A.AffixDefinition == B.AffixDefinition
        && A.TierRank == B.TierRank
        && A.Magnitude == B.Magnitude
        && A.Provenance.SourceItemGuid == B.Provenance.SourceItemGuid
        && A.Provenance.OriginSocketGuid == B.Provenance.OriginSocketGuid
        && A.Provenance.SourceKind == B.Provenance.SourceKind
        && A.Provenance.RollGroup == B.Provenance.RollGroup
        && A.Provenance.MutationRevision == B.Provenance.MutationRevision
        && A.bIsLocked == B.bIsLocked;
}

bool UMythicAffixApplicationComponent::GuidLexicalLess(const FGuid &A, const FGuid &B) {
    if (A.A != B.A) return A.A < B.A;
    if (A.B != B.B) return A.B < B.B;
    if (A.C != B.C) return A.C < B.C;
    return A.D < B.D;
}

double UMythicAffixApplicationComponent::GetNormalizedContributionDelta(
    const FMythicAppliedAffixState &Candidate) {
    const double Magnitude = static_cast<double>(Candidate.Magnitude);
    switch (Candidate.ModifierOp.GetValue()) {
    case EGameplayModOp::AddBase:
    case EGameplayModOp::AddFinal:
        return Magnitude;
    case EGameplayModOp::MultiplyAdditive:
    case EGameplayModOp::MultiplyCompound:
        return Magnitude - 1.0;
    case EGameplayModOp::DivideAdditive:
        return 1.0 / Magnitude - 1.0;
    case EGameplayModOp::Override:
        return Magnitude - static_cast<double>(Candidate.NeutralValue);
    default:
        return std::numeric_limits<double>::quiet_NaN();
    }
}

bool UMythicAffixApplicationComponent::IsBetterHighestCandidate(
    const FMythicAppliedAffixState &Candidate,
    const FMythicAppliedAffixState &Incumbent) {
    if (Candidate.ComparisonDirection == EMythicStatComparisonDirection::Neutral ||
        Incumbent.ComparisonDirection == EMythicStatComparisonDirection::Neutral) {
        return false;
    }

    const double CandidateDelta = GetNormalizedContributionDelta(Candidate);
    const double IncumbentDelta = GetNormalizedContributionDelta(Incumbent);
    if (!FMath::IsFinite(CandidateDelta) || !FMath::IsFinite(IncumbentDelta)) {
        return false;
    }
    const double CandidateBenefit =
        Candidate.ComparisonDirection == EMythicStatComparisonDirection::HigherIsBetter
            ? CandidateDelta : -CandidateDelta;
    const double IncumbentBenefit =
        Incumbent.ComparisonDirection == EMythicStatComparisonDirection::HigherIsBetter
            ? IncumbentDelta : -IncumbentDelta;
    if (CandidateBenefit != IncumbentBenefit) {
        return CandidateBenefit > IncumbentBenefit;
    }
    if (Candidate.SourceItemGuid != Incumbent.SourceItemGuid) {
        return GuidLexicalLess(Candidate.SourceItemGuid, Incumbent.SourceItemGuid);
    }
    return GuidLexicalLess(Candidate.RollGuid, Incumbent.RollGuid);
}
