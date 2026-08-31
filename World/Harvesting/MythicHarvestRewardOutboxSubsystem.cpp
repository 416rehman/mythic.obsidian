#include "World/Harvesting/MythicHarvestRewardOutboxSubsystem.h"

#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Itemization/Affixes/MythicItemizationHash.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemFactorySubsystem.h"
#include "Itemization/Inventory/MythicItemFactoryTypes.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/ItemizationSubsystem.h"
#include "Mythic.h"
#include "Objectives/ObjectiveTracker.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "Player/MythicPlayerState.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Subsystem/SaveSystem/MythicSaveGameSubsystem.h"
#include "System/MythicAssetManager.h"
#include "World/Harvesting/MythicHarvestReceiptLedgerComponent.h"
#include "World/Harvesting/MythicHarvestRewardEscrowComponent.h"
#include "World/Harvesting/MythicHarvestSettings.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"

namespace MythicHarvestRewardOutboxPrivate {

bool GuidLess(const FGuid &Left, const FGuid &Right) {
    if (Left.A != Right.A) return Left.A < Right.A;
    if (Left.B != Right.B) return Left.B < Right.B;
    if (Left.C != Right.C) return Left.C < Right.C;
    return Left.D < Right.D;
}

bool IsValidRewardChannel(const EMythicHarvestRewardChannel Channel) {
    return Channel == EMythicHarvestRewardChannel::PrimaryMaterial
        || Channel == EMythicHarvestRewardChannel::BonusLoot;
}

bool IsValidQuality(const EMythicYieldQuality Quality) {
    return Quality == EMythicYieldQuality::Ragged
        || Quality == EMythicYieldQuality::Common
        || Quality == EMythicYieldQuality::Fine
        || Quality == EMythicYieldQuality::Pristine;
}

bool IsExpectedPrimaryAssetType(const FPrimaryAssetId &Id,
                                const FPrimaryAssetType &ExpectedType) {
    return Id.IsValid() && Id.PrimaryAssetType == ExpectedType;
}

EMythicHarvestReceiptChannel ToReceiptChannel(
    const EMythicHarvestRewardChannel Channel) {
    return Channel == EMythicHarvestRewardChannel::BonusLoot
        ? EMythicHarvestReceiptChannel::BonusLoot
        : EMythicHarvestReceiptChannel::PrimaryMaterial;
}

uint32 PackQualityAuxiliary(const bool bHasResolvedQuality,
                            const EMythicYieldQuality Quality) {
    return static_cast<uint32>(static_cast<uint8>(Quality))
        | (bHasResolvedQuality ? 0x100u : 0u);
}

bool ContextIsValid(const FGameplayTagContainer &Context) {
    for (const FGameplayTag &Tag : Context) {
        if (!Tag.IsValid()) {
            return false;
        }
    }
    return true;
}

bool CompletionKeyLess(const FMythicHarvestRewardCompletionKey &Left,
                       const FMythicHarvestRewardCompletionKey &Right) {
    if (Left.WorldEpoch != Right.WorldEpoch) {
        return GuidLess(Left.WorldEpoch, Right.WorldEpoch);
    }
    if (Left.NodeId.GetGuid() != Right.NodeId.GetGuid()) {
        return GuidLess(Left.NodeId.GetGuid(), Right.NodeId.GetGuid());
    }
    return Left.Generation < Right.Generation;
}

bool ReceiptKeyLess(const FMythicHarvestReceiptKey &Left,
                    const FMythicHarvestReceiptKey &Right) {
    if (Left.WorldEpoch != Right.WorldEpoch) {
        return GuidLess(Left.WorldEpoch, Right.WorldEpoch);
    }
    if (Left.NodeId.GetGuid() != Right.NodeId.GetGuid()) {
        return GuidLess(Left.NodeId.GetGuid(), Right.NodeId.GetGuid());
    }
    if (Left.Generation != Right.Generation) {
        return Left.Generation < Right.Generation;
    }
    if (Left.Channel != Right.Channel) {
        return static_cast<uint8>(Left.Channel)
            < static_cast<uint8>(Right.Channel);
    }
    if (Left.EntryOrdinal != Right.EntryOrdinal) {
        return Left.EntryOrdinal < Right.EntryOrdinal;
    }
    return GuidLess(Left.SeriesGuid, Right.SeriesGuid);
}

bool AddContributorReceiptIdentity(
    TMap<FString, TSet<FMythicHarvestReceiptKey>> &Seen,
    const FString &ContributorKey,
    const FMythicHarvestReceiptKey &ReceiptKey) {
    TSet<FMythicHarvestReceiptKey> &ContributorRows =
        Seen.FindOrAdd(ContributorKey);
    if (ContributorRows.Contains(ReceiptKey)) {
        return false;
    }
    ContributorRows.Add(ReceiptKey);
    return true;
}

} // namespace MythicHarvestRewardOutboxPrivate

void FMythicHarvestRewardOutboxSaveV1::SortCanonical() {
    KnownCompletions.Sort([](
        const FMythicSavedHarvestRewardCompletionV1 &Left,
        const FMythicSavedHarvestRewardCompletionV1 &Right) {
        const FMythicHarvestRewardCompletionKey LeftKey{
            Left.WorldEpoch, FMythicHarvestNodeId(Left.NodeGuid),
            Left.Generation};
        const FMythicHarvestRewardCompletionKey RightKey{
            Right.WorldEpoch, FMythicHarvestNodeId(Right.NodeGuid),
            Right.Generation};
        return MythicHarvestRewardOutboxPrivate::CompletionKeyLess(
            LeftKey, RightKey);
    });
    GenerationHighWatermarks.Sort([](
        const FMythicSavedHarvestGenerationHighWaterV1 &Left,
        const FMythicSavedHarvestGenerationHighWaterV1 &Right) {
        if (Left.WorldEpoch != Right.WorldEpoch) {
            return MythicHarvestRewardOutboxPrivate::GuidLess(
                Left.WorldEpoch, Right.WorldEpoch);
        }
        return MythicHarvestRewardOutboxPrivate::GuidLess(
            Left.NodeGuid, Right.NodeGuid);
    });
    PendingGrants.Sort([](
        const FMythicSavedHarvestRewardGrantV1 &Left,
        const FMythicSavedHarvestRewardGrantV1 &Right) {
        if (Left.ReceiptKey == Right.ReceiptKey) {
            return Left.ContributorKey.Compare(
                       Right.ContributorKey,
                       ESearchCase::CaseSensitive) < 0;
        }
        return MythicHarvestRewardOutboxPrivate::ReceiptKeyLess(
            Left.ReceiptKey, Right.ReceiptKey);
    });
    PendingCompletionDeliveries.Sort([](
        const FMythicSavedHarvestCompletionDeliveryV1 &Left,
        const FMythicSavedHarvestCompletionDeliveryV1 &Right) {
        const FMythicHarvestRewardCompletionKey LeftKey{
            Left.WorldEpoch, FMythicHarvestNodeId(Left.NodeGuid),
            Left.Generation};
        const FMythicHarvestRewardCompletionKey RightKey{
            Right.WorldEpoch, FMythicHarvestNodeId(Right.NodeGuid),
            Right.Generation};
        if (!(LeftKey == RightKey)) {
            return MythicHarvestRewardOutboxPrivate::CompletionKeyLess(
                LeftKey, RightKey);
        }
        return Left.ContributorKey.Compare(
                   Right.ContributorKey,
                   ESearchCase::CaseSensitive) < 0;
    });
    PendingWorkDeliveries.Sort([](
        const FMythicSavedHarvestWorkDeliveryV1 &Left,
        const FMythicSavedHarvestWorkDeliveryV1 &Right) {
        if (Left.ReceiptKey == Right.ReceiptKey) {
            return Left.ContributorKey.Compare(
                       Right.ContributorKey,
                       ESearchCase::CaseSensitive) < 0;
        }
        return MythicHarvestRewardOutboxPrivate::ReceiptKeyLess(
            Left.ReceiptKey, Right.ReceiptKey);
    });
    DurabilityCosts.Sort([](
        const FMythicSavedHarvestDurabilityCostV1 &Left,
        const FMythicSavedHarvestDurabilityCostV1 &Right) {
        if (Left.ReceiptKey == Right.ReceiptKey) {
            return Left.ContributorKey.Compare(
                       Right.ContributorKey,
                       ESearchCase::CaseSensitive) < 0;
        }
        return MythicHarvestRewardOutboxPrivate::ReceiptKeyLess(
            Left.ReceiptKey, Right.ReceiptKey);
    });
    ContributorLedgerFences.Sort([](
        const FMythicSavedHarvestContributorLedgerFenceV1 &Left,
        const FMythicSavedHarvestContributorLedgerFenceV1 &Right) {
        return Left.ContributorKey.Compare(
                   Right.ContributorKey,
                   ESearchCase::CaseSensitive) < 0;
    });
}

namespace MHRewardOutboxPrivate = MythicHarvestRewardOutboxPrivate;

bool FMythicPreparedHarvestCompletion::IsValid() const {
    if (!CompletionKey.IsValid()
        || FirstObservableWorldSnapshotSequence == 0) {
        return false;
    }
    for (const FMythicHarvestPlannedRewardGrant &Grant : Grants) {
        if (!Grant.IsValid() || !(Grant.CompletionKey == CompletionKey)
            || !Grant.ReceiptKey.IsValid()
            || Grant.ReceiptKey.WorldEpoch != CompletionKey.WorldEpoch
            || Grant.ReceiptKey.NodeId != CompletionKey.NodeId
            || Grant.ReceiptKey.Generation != CompletionKey.Generation
            || !Grant.ReceiptPayloadFingerprint.IsValid()) {
            return false;
        }
    }
    for (const FMythicPendingHarvestCompletionDelivery &Delivery :
         CompletionDeliveries) {
        const bool bHasProficiency =
            Delivery.CompletionProficiencyXPQuanta > 0;
        const bool bHasQuest = Delivery.QuestCreditCount > 0;
        if (!(Delivery.CompletionKey == CompletionKey)
            || Delivery.ContributorKey.IsEmpty()
            || (!bHasProficiency && !bHasQuest)
            || Delivery.IsComplete()
            || (bHasProficiency
                && (!Delivery.ProficiencyReceiptKey.IsValid()
                    || !Delivery.ProficiencyReceiptPayloadFingerprint.IsValid()))
            || (bHasQuest
                && (!Delivery.QuestReceiptKey.IsValid()
                    || !Delivery.QuestReceiptPayloadFingerprint.IsValid()))) {
            return false;
        }
    }
    return true;
}

bool FMythicPreparedHarvestWorkDelivery::IsValid() const {
    if (!bHasDelivery) {
        return FirstObservableWorldSnapshotSequence == 0
            && !Delivery.ReceiptKey.IsValid()
            && (Delivery.WorkRewardContract.IsUnset()
                || Delivery.WorkRewardContract.IsValid());
    }
    int64 ExpectedCumulativeXP = 0;
    return FirstObservableWorldSnapshotSequence > 0
        && Delivery.NodeGenerationKey.IsValid()
        && !Delivery.ContributorKey.IsEmpty()
        && Delivery.WorkRewardContract.IsValid()
        && Delivery.WorkRewardContract.IsEnabled()
        && Delivery.CumulativeAppliedWorkQuanta > 0
        && Delivery.ProficiencyXPQuanta > 0
        && Delivery.ReceiptKey.IsValid()
        && Delivery.ReceiptKey == FMythicHarvestReceiptKey::MakeAppliedWork(
            Delivery.NodeGenerationKey.WorldEpoch,
            Delivery.NodeGenerationKey.NodeId,
            Delivery.NodeGenerationKey.Generation,
            Delivery.ContributorKey)
        && Delivery.ReceiptKey.Channel
            == EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP
        && FMythicHarvestReceiptQuantity::
            TryCalculateCumulativeAppliedWorkXP(
                Delivery.CumulativeAppliedWorkQuanta,
                Delivery.WorkRewardContract.
                    ProficiencyXPPerWorkUnitQuanta,
                ExpectedCumulativeXP)
        && Delivery.ProficiencyXPQuanta == ExpectedCumulativeXP
        && Delivery.ReceiptPayloadFingerprint.IsValid()
        && Delivery.ReceiptPayloadFingerprint
            == FMythicHarvestReceiptFingerprint::BuildAppliedWorkSeries(
                Delivery.ReceiptKey,
                Delivery.WorkRewardContract.ProficiencyDefinitionId,
                Delivery.WorkRewardContract.
                    ProficiencyXPPerWorkUnitQuanta,
                Delivery.WorkRewardContract.ContextTags)
        && MHRewardOutboxPrivate::ContextIsValid(
            Delivery.WorkRewardContract.ContextTags);
}

bool FMythicPreparedHarvestDurabilityCost::IsValid() const {
    if (!bHasCost) {
        return PreviousCumulativeWearTarget == 0
            && !Cost.ReceiptKey.IsValid();
    }
    return Cost.NodeGenerationKey.IsValid()
        && !Cost.ContributorKey.IsEmpty()
        && Cost.ToolItemInstanceGuid.IsValid()
        && PreviousCumulativeWearTarget >= 0
        && Cost.CumulativeWearTarget > PreviousCumulativeWearTarget
        && Cost.DurablyAppliedWearTarget >= 0
        && Cost.DurablyAppliedWearTarget
            <= PreviousCumulativeWearTarget
        && Cost.ReceiptKey
            == FMythicHarvestReceiptKey::MakeDurabilityCost(
                Cost.NodeGenerationKey.WorldEpoch,
                Cost.NodeGenerationKey.NodeId,
                Cost.NodeGenerationKey.Generation,
                Cost.ContributorKey, Cost.ToolItemInstanceGuid)
        && Cost.ReceiptPayloadFingerprint
            == FMythicHarvestReceiptFingerprint::
                BuildDurabilityCostSeries(
                    Cost.ReceiptKey, Cost.ToolItemInstanceGuid);
}

bool FMythicHarvestCharacterSaveCallbackPolicy::MatchesCurrentRequest(
    const FMythicHarvestCharacterSaveRequestIdentity &Latched,
    const FGuid &CallbackRequestToken,
    const FGuid &CallbackOperationId,
    const uint64 CallbackRestoreDomainEpoch,
    const uint64 ActiveRestoreDomainEpoch) {
    return Latched.IsValid() && CallbackRequestToken.IsValid()
        && CallbackOperationId.IsValid()
        && CallbackRestoreDomainEpoch > 0
        && CallbackRestoreDomainEpoch == ActiveRestoreDomainEpoch
        && Latched.RestoreDomainEpoch == CallbackRestoreDomainEpoch
        && Latched.RequestToken == CallbackRequestToken
        && Latched.OperationId == CallbackOperationId;
}

bool FMythicHarvestCharacterSaveCallbackPolicy::RequiresCorrectiveSave(
    const bool bPhysicalSaveSucceeded,
    const uint64 CallbackRestoreDomainEpoch,
    const uint64 ActiveRestoreDomainEpoch,
    const bool bHasCurrentDomainRequest) {
    return bPhysicalSaveSucceeded && !bHasCurrentDomainRequest
        && CallbackRestoreDomainEpoch > 0
        && CallbackRestoreDomainEpoch < ActiveRestoreDomainEpoch;
}

bool UMythicHarvestRewardOutboxSubsystem::ShouldCreateSubsystem(
    UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

void UMythicHarvestRewardOutboxSubsystem::Initialize(
    FSubsystemCollectionBase &Collection) {
    Collection.InitializeDependency<UMythicPlayerRegistrySubsystem>();
    Super::Initialize(Collection);
    RestoredSaveSnapshotFingerprint = FSHA256Signature{};
    bHasRestoredSaveSnapshotFingerprint = false;
    RestoreDomainEpoch = 1;
}

uint64 UMythicHarvestRewardOutboxSubsystem::
GetFirstObservableSnapshotSequence() const {
    return LastIssuedWorldSnapshotSequence == MAX_uint64
        ? 0 : LastIssuedWorldSnapshotSequence + 1;
}

bool UMythicHarvestRewardOutboxSubsystem::HasPendingEscrowWork() const {
    for (const TWeakObjectPtr<AMythicPlayerState> &WeakPlayerState :
         TrackedReceiptOwners) {
        const AMythicPlayerState *PlayerState = WeakPlayerState.Get();
        const UMythicHarvestRewardEscrowComponent *Escrow = PlayerState
            ? PlayerState->GetHarvestRewardEscrow() : nullptr;
        if (Escrow && Escrow->HasPendingDelivery()) {
            return true;
        }
    }
    return false;
}

int32 UMythicHarvestRewardOutboxSubsystem::
GetPendingEscrowRowCount() const {
    int64 Total = 0;
    for (const TWeakObjectPtr<AMythicPlayerState> &WeakPlayerState :
         TrackedReceiptOwners) {
        const AMythicPlayerState *PlayerState = WeakPlayerState.Get();
        const UMythicHarvestRewardEscrowComponent *Escrow = PlayerState
            ? PlayerState->GetHarvestRewardEscrow() : nullptr;
        Total += Escrow ? Escrow->GetPendingRowCount() : 0;
        if (Total >= MAX_int32) return MAX_int32;
    }
    return static_cast<int32>(Total);
}

bool UMythicHarvestRewardOutboxSubsystem::HasPendingWork() const {
    return GetPendingWorkCount() > 0 || HasPendingEscrowWork();
}

FMythicSavedHarvestItemEscrowRowV1
UMythicHarvestRewardOutboxSubsystem::BuildEscrowRow(
    const FMythicPendingHarvestRewardDelivery &Pending,
    const uint64 FirstObservedWorldSnapshotSequence) {
    FMythicSavedHarvestItemEscrowRowV1 Row;
    Row.ReceiptKey = Pending.Grant.ReceiptKey;
    Row.ReceiptPayloadFingerprint =
        Pending.Grant.ReceiptPayloadFingerprint;
    Row.ItemDefinitionId = Pending.Grant.ItemDefinitionId;
    Row.OriginalQuantity = Pending.Grant.Quantity;
    Row.RemainingQuantity = Pending.Grant.Quantity;
    Row.ItemLevel = Pending.Grant.ItemLevel;
    Row.bHasResolvedQuality = Pending.Grant.bHasResolvedQuality;
    Row.ResolvedQuality = Pending.Grant.ResolvedQuality;
    Row.ItemSeed = Pending.Grant.ItemSeed;
    Row.FirstObservedWorldSnapshotSequence =
        FirstObservedWorldSnapshotSequence;
    Row.MutationRevision = 1;
    return Row;
}

bool UMythicHarvestRewardOutboxSubsystem::WouldExceedPendingCapacity(
    const int32 AdditionalRows) const {
    if (AdditionalRows < 0) {
        return true;
    }
    const int64 Existing = static_cast<int64>(PendingDeliveries.Num())
        + PendingCompletionDeliveries.Num() + PendingWorkDeliveries.Num();
    return Existing + AdditionalRows
        > FMythicHarvestRewardOutboxSaveV1::AbsoluteMaximumPendingDeliveries;
}

bool UMythicHarvestRewardOutboxSubsystem::
WouldExceedPerContributorPendingCapacity(
    const int32 CurrentRows, const int32 AdditionalRows) {
    return CurrentRows < 0 || AdditionalRows < 0
        || static_cast<int64>(CurrentRows) + AdditionalRows
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumPendingDeliveriesPerContributor;
}

int32 UMythicHarvestRewardOutboxSubsystem::AdvanceRetryRowCursor(
    const int32 CurrentRow, const int32 RemainingRowCount,
    const bool bCurrentRowRemoved) {
    if (RemainingRowCount <= 0) {
        return 0;
    }
    const int32 CanonicalCurrent =
        FMath::Max(0, CurrentRow) % RemainingRowCount;
    return bCurrentRowRemoved
        ? CanonicalCurrent : (CanonicalCurrent + 1) % RemainingRowCount;
}

int32 UMythicHarvestRewardOutboxSubsystem::
GetPendingRowCountForContributor(const FString &ContributorKey) const {
    if (ContributorKey.IsEmpty()) return MAX_int32;
    const int32 *Count = PendingRowCountByContributor.Find(ContributorKey);
    return Count ? *Count : 0;
}

void UMythicHarvestRewardOutboxSubsystem::AdjustPendingContributorRows(
    const FString &ContributorKey, const int32 Delta) {
    if (ContributorKey.IsEmpty() || Delta == 0) return;
    int32 &Count = PendingRowCountByContributor.FindOrAdd(ContributorKey);
    Count += Delta;
    checkf(Count >= 0, TEXT("Harvest contributor pending-row index underflow."));
    if (Count <= 0) PendingRowCountByContributor.Remove(ContributorKey);
}

void UMythicHarvestRewardOutboxSubsystem::
AdjustPendingItemContributorRows(
    const FString &ContributorKey, const int32 Delta) {
    if (ContributorKey.IsEmpty() || Delta == 0) return;
    int32 &Count =
        PendingItemRowCountByContributor.FindOrAdd(ContributorKey);
    Count += Delta;
    checkf(Count >= 0,
           TEXT("Harvest contributor pending-item index underflow."));
    if (Count <= 0) {
        PendingItemRowCountByContributor.Remove(ContributorKey);
    }
}

void UMythicHarvestRewardOutboxSubsystem::
AdjustDurabilityContributorSeries(
    const FString &ContributorKey, const int32 Delta) {
    if (ContributorKey.IsEmpty() || Delta == 0) return;
    int32 &Count =
        DurabilitySeriesCountByContributor.FindOrAdd(ContributorKey);
    Count += Delta;
    checkf(Count >= 0,
           TEXT("Harvest durability-series contributor index underflow."));
    if (Count <= 0) {
        DurabilitySeriesCountByContributor.Remove(ContributorKey);
    }
}

bool UMythicHarvestRewardOutboxSubsystem::
WouldExceedDurabilityContributorCapacity(
    const FString &ContributorKey, const int32 AdditionalRows) const {
    if (ContributorKey.IsEmpty() || AdditionalRows < 0) return true;
    const int32 *Existing =
        DurabilitySeriesCountByContributor.Find(ContributorKey);
    return static_cast<int64>(Existing ? *Existing : 0) + AdditionalRows
        > FMythicHarvestRewardOutboxSaveV1::
            AbsoluteMaximumDurabilityCostSeriesPerContributor;
}

bool UMythicHarvestRewardOutboxSubsystem::
WouldExceedContributorPendingCapacity(
    const FString &ContributorKey, const int32 AdditionalRows) const {
    return WouldExceedPerContributorPendingCapacity(
        GetPendingRowCountForContributor(ContributorKey), AdditionalRows);
}

bool UMythicHarvestRewardOutboxSubsystem::
PreparedCompletionWouldExceedContributorCapacity(
    const FMythicPreparedHarvestCompletion &PreparedCompletion) const {
    TMap<FString, int32> AdditionalByContributor;
    for (const FMythicHarvestPlannedRewardGrant &Grant :
         PreparedCompletion.Grants) {
        ++AdditionalByContributor.FindOrAdd(Grant.ContributorKey);
    }
    for (const FMythicPendingHarvestCompletionDelivery &Delivery :
         PreparedCompletion.CompletionDeliveries) {
        ++AdditionalByContributor.FindOrAdd(Delivery.ContributorKey);
    }
    for (const TPair<FString, int32> &Pair : AdditionalByContributor) {
        if (WouldExceedContributorPendingCapacity(Pair.Key, Pair.Value)) {
            return true;
        }
    }
    return false;
}

bool UMythicHarvestRewardOutboxSubsystem::
PreparedCompletionWouldExceedEscrowCapacity(
    const FMythicPreparedHarvestCompletion &PreparedCompletion) const {
    TMap<FString, int32> AdditionalItemRowsByContributor;
    for (const FMythicHarvestPlannedRewardGrant &Grant :
         PreparedCompletion.Grants) {
        ++AdditionalItemRowsByContributor.FindOrAdd(Grant.ContributorKey);
    }
    for (const TPair<FString, int32> &Pair :
         AdditionalItemRowsByContributor) {
        AMythicPlayerState *PlayerState =
            ResolveContributorPlayerState(Pair.Key);
        const UMythicHarvestRewardEscrowComponent *Escrow =
            ResolveRewardEscrow(PlayerState);
        if (!Escrow) {
            continue;
        }
        const int32 *PendingItemRows =
            PendingItemRowCountByContributor.Find(Pair.Key);
        const int64 ReservedRows = PendingItemRows ? *PendingItemRows : 0;
        if (ReservedRows + Pair.Value
            > Escrow->GetAvailableRowCapacity()) {
            return true;
        }
    }
    return false;
}

FMythicHarvestRewardPrepareResult
UMythicHarvestRewardOutboxSubsystem::PrepareCompletion(
    const UMythicHarvestableDefinition &Definition,
    const FGuid &WorldEpoch,
    const FMythicHarvestNodeId &NodeId,
    const uint32 Generation,
    const TConstArrayView<FMythicHarvestParticipantSnapshot> Participants,
    FMythicPreparedHarvestCompletion &OutPreparedCompletion) const {
    FMythicHarvestRewardPrepareResult Result;
    OutPreparedCompletion = FMythicPreparedHarvestCompletion();
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) {
        Result.DiagnosticCode = TEXT("AuthorityWorldUnavailable");
        return Result;
    }
    if (!WorldEpoch.IsValid()) {
        Result.Status = EMythicHarvestRewardPrepareStatus::InvalidWorldEpoch;
        Result.DiagnosticCode = TEXT("InvalidRewardWorldEpoch");
        return Result;
    }

    const FMythicHarvestRewardCompletionKey CompletionKey{
        WorldEpoch, NodeId, Generation};
    if (!CompletionKey.IsValid()) {
        Result.Status = EMythicHarvestRewardPrepareStatus::PlanningFailed;
        Result.DiagnosticCode = TEXT("InvalidCompletionKey");
        return Result;
    }
    if (KnownCompletions.Contains(CompletionKey)) {
        OutPreparedCompletion.CompletionKey = CompletionKey;
        Result.Status = EMythicHarvestRewardPrepareStatus::AlreadyKnown;
        Result.PlanStatus = EMythicHarvestRewardPlanStatus::Success;
        return Result;
    }

    const FMythicHarvestEpochNodeKey EpochNode{WorldEpoch, NodeId};
    const uint32 *Highest = HighestKnownGenerationByNode.Find(EpochNode);
    if (Highest && Generation <= *Highest) {
        Result.Status = EMythicHarvestRewardPrepareStatus::AlreadyKnown;
        Result.PlanStatus = EMythicHarvestRewardPlanStatus::Success;
        Result.DiagnosticCode = TEXT("GenerationCoveredByHighWater");
        return Result;
    }
    const uint32 ExpectedGeneration = Highest ? *Highest + 1u : 1u;
    if ((Highest && *Highest == MAX_uint32)
        || Generation != ExpectedGeneration) {
        Result.Status = EMythicHarvestRewardPrepareStatus::PlanningFailed;
        Result.DiagnosticCode = TEXT("NonContiguousCompletionGeneration");
        return Result;
    }
    const uint64 FirstObservableSequence =
        GetFirstObservableSnapshotSequence();
    if (FirstObservableSequence == 0) {
        Result.Status = EMythicHarvestRewardPrepareStatus::SequenceExhausted;
        Result.DiagnosticCode = TEXT("RewardOutboxSequenceExhausted");
        return Result;
    }
    if (!Highest
        && HighestKnownGenerationByNode.Num()
            >= FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumKnownCompletions) {
        Result.Status = EMythicHarvestRewardPrepareStatus::CapacityExceeded;
        Result.DiagnosticCode = TEXT("RewardCompletionCapacityExceeded");
        return Result;
    }

    TArray<FMythicHarvestRewardParticipant> RewardParticipants;
    RewardParticipants.Reserve(Participants.Num());
    for (const FMythicHarvestParticipantSnapshot &Snapshot : Participants) {
        if (!Snapshot.IsValid()) {
            Result.Status =
                EMythicHarvestRewardPrepareStatus::InvalidContributor;
            Result.PlanStatus =
                EMythicHarvestRewardPlanStatus::InvalidContributor;
            Result.DiagnosticCode = TEXT("InvalidRewardContributor");
            return Result;
        }
        FMythicHarvestRewardParticipant &Participant =
            RewardParticipants.AddDefaulted_GetRef();
        Participant.ContributorKey = Snapshot.ContributorKey;
        Participant.ContributionQuanta = Snapshot.ContributionQuanta;
        Participant.ItemLevel = Snapshot.ItemLevel;
        Participant.QuantityMultiplierQuanta =
            Snapshot.QuantityMultiplierQuanta;
        Participant.ProficiencyLevel = Snapshot.ProficiencyLevel;
        Participant.InitialController = Snapshot.CurrentController;
    }

    const UMythicDeveloperSettings *DeveloperSettings =
        GetDefault<UMythicDeveloperSettings>();
    if (!DeveloperSettings) {
        Result.Status = EMythicHarvestRewardPrepareStatus::PlanningFailed;
        Result.PlanStatus = EMythicHarvestRewardPlanStatus::InvalidDefinition;
        Result.DiagnosticCode = TEXT("YieldQualitySettingsUnavailable");
        return Result;
    }
    FMythicHarvestRewardPlanResult Plan =
        FMythicHarvestRewardPlanner::PlanCompletion(
            Definition, WorldEpoch, NodeId, Generation,
            DeveloperSettings->YieldQuality, RewardParticipants);
    Result.PlanStatus = Plan.Status;
    Result.DiagnosticCode = Plan.DiagnosticCode;
    if (!Plan.IsSuccess()) {
        Result.Status = EMythicHarvestRewardPrepareStatus::PlanningFailed;
        return Result;
    }

    for (FMythicHarvestPlannedRewardGrant &Grant : Plan.Grants) {
        if (Grant.RewardRowIndex < 0) {
            Result.Status = EMythicHarvestRewardPrepareStatus::PlanningFailed;
            Result.DiagnosticCode = TEXT("InvalidRewardReceiptOrdinal");
            return Result;
        }
        Grant.ReceiptKey = FMythicHarvestReceiptKey::MakeCompletion(
            WorldEpoch, NodeId, Generation,
            MHRewardOutboxPrivate::ToReceiptChannel(Grant.Channel),
            static_cast<uint32>(Grant.RewardRowIndex));
        Grant.ReceiptPayloadFingerprint =
            FMythicHarvestReceiptFingerprint::Build(
                Grant.ReceiptKey, Grant.ItemDefinitionId, Grant.Quantity,
                Grant.ItemSeed, static_cast<uint32>(Grant.ItemLevel),
                MHRewardOutboxPrivate::PackQualityAuxiliary(
                    Grant.bHasResolvedQuality, Grant.ResolvedQuality));
    }

    if (!BuildCompletionDeliveries(
            Definition, CompletionKey, Participants,
            FirstObservableSequence,
            OutPreparedCompletion.CompletionDeliveries,
            Result.DiagnosticCode)) {
        OutPreparedCompletion = FMythicPreparedHarvestCompletion();
        Result.Status = EMythicHarvestRewardPrepareStatus::PlanningFailed;
        Result.PlanStatus = EMythicHarvestRewardPlanStatus::InvalidDefinition;
        return Result;
    }
    OutPreparedCompletion.CompletionKey = CompletionKey;
    OutPreparedCompletion.Grants = MoveTemp(Plan.Grants);
    OutPreparedCompletion.FirstObservableWorldSnapshotSequence =
        FirstObservableSequence;
    if (WouldExceedPendingCapacity(
            OutPreparedCompletion.Grants.Num()
            + OutPreparedCompletion.CompletionDeliveries.Num())) {
        OutPreparedCompletion = FMythicPreparedHarvestCompletion();
        Result.Status = EMythicHarvestRewardPrepareStatus::CapacityExceeded;
        Result.DiagnosticCode = TEXT("RewardDeliveryCapacityExceeded");
        return Result;
    }
    if (PreparedCompletionWouldExceedContributorCapacity(
            OutPreparedCompletion)) {
        OutPreparedCompletion = FMythicPreparedHarvestCompletion();
        Result.Status = EMythicHarvestRewardPrepareStatus::CapacityExceeded;
        Result.DiagnosticCode = TEXT("RewardContributorBackpressure");
        return Result;
    }
    if (PreparedCompletionWouldExceedEscrowCapacity(
            OutPreparedCompletion)) {
        OutPreparedCompletion = FMythicPreparedHarvestCompletion();
        Result.Status = EMythicHarvestRewardPrepareStatus::CapacityExceeded;
        Result.DiagnosticCode = TEXT("RewardContributorEscrowBackpressure");
        return Result;
    }

    if (!OutPreparedCompletion.IsValid()) {
        OutPreparedCompletion = FMythicPreparedHarvestCompletion();
        Result.Status = EMythicHarvestRewardPrepareStatus::PlanningFailed;
        Result.PlanStatus = EMythicHarvestRewardPlanStatus::InvalidCompletion;
        Result.DiagnosticCode = TEXT("InvalidPreparedCompletion");
        return Result;
    }

    Result.Status = EMythicHarvestRewardPrepareStatus::Prepared;
    Result.PlannedGrantCount = OutPreparedCompletion.Grants.Num();
    Result.DiagnosticCode = NAME_None;
    return Result;
}

bool UMythicHarvestRewardOutboxSubsystem::BuildCompletionDeliveries(
    const UMythicHarvestableDefinition &Definition,
    const FMythicHarvestRewardCompletionKey &CompletionKey,
    const TConstArrayView<FMythicHarvestParticipantSnapshot> Participants,
    const uint64 FirstObservableWorldSnapshotSequence,
    TArray<FMythicPendingHarvestCompletionDelivery> &OutDeliveries,
    FName &OutDiagnosticCode) {
    OutDeliveries.Reset();
    OutDiagnosticCode = NAME_None;
    if (!CompletionKey.IsValid() || Participants.IsEmpty()
        || FirstObservableWorldSnapshotSequence == 0
        || !FMath::IsFinite(Definition.CompletionProficiencyXP)
        || Definition.CompletionProficiencyXP < 0.0f
        || (Definition.QuestCredit.bEmitCompletionCredit
            && Definition.QuestCredit.CreditCount <= 0)) {
        OutDiagnosticCode = TEXT("InvalidCompletionDeliveryDefinition");
        return false;
    }

    const bool bHasProficiencyChannel =
        Definition.CompletionProficiencyXP > 0.0f;
    const bool bHasQuestChannel =
        Definition.QuestCredit.bEmitCompletionCredit;
    UProficiencyDefinition *ProficiencyDefinition =
        Definition.ProficiencyDefinition;
    const FPrimaryAssetId ProficiencyDefinitionId = ProficiencyDefinition
        ? ProficiencyDefinition->GetPrimaryAssetId() : FPrimaryAssetId();
    const FPrimaryAssetId HarvestableDefinitionId =
        Definition.GetPrimaryAssetId();
    if ((bHasProficiencyChannel
         && (!ProficiencyDefinition
             || !MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
                 ProficiencyDefinitionId,
                 UMythicAssetManager::ProficiencyDefinitionType)))
        || (bHasQuestChannel
            && !MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
                HarvestableDefinitionId,
                UMythicAssetManager::HarvestableDefinitionType))) {
        OutDiagnosticCode = TEXT("InvalidCompletionDeliveryAssetIdentity");
        return false;
    }

    int64 TotalContributionQuanta = 0;
    TSet<FString> ContributorKeys;
    ContributorKeys.Reserve(Participants.Num());
    for (const FMythicHarvestParticipantSnapshot &Participant :
         Participants) {
        if (!Participant.IsValid()
            || ContributorKeys.Contains(Participant.ContributorKey)
            || Participant.ContributionQuanta
                > MAX_int64 - TotalContributionQuanta) {
            OutDiagnosticCode = TEXT("InvalidCompletionDeliveryContributor");
            return false;
        }
        ContributorKeys.Add(Participant.ContributorKey);
        TotalContributionQuanta += Participant.ContributionQuanta;
    }
    if (TotalContributionQuanta <= 0) {
        OutDiagnosticCode = TEXT("InvalidCompletionDeliveryContribution");
        return false;
    }

    OutDeliveries.Reserve(Participants.Num());
    for (const FMythicHarvestParticipantSnapshot &Participant :
         Participants) {
        const double ProportionalXP = bHasProficiencyChannel
            ? FMythicHarvestContributionMath::CalculateProportionalShare(
                Participant.ContributionQuanta, TotalContributionQuanta,
                static_cast<double>(Definition.CompletionProficiencyXP))
            : 0.0;
        int64 XPQuanta = 0;
        if (ProportionalXP > 0.0
            && !FMythicHarvestReceiptQuantity::TryFromUnits(
                ProportionalXP, XPQuanta)) {
            OutDeliveries.Reset();
            OutDiagnosticCode = TEXT("InvalidCompletionDeliveryXP");
            return false;
        }
        const int32 QuestCreditCount = bHasQuestChannel
            ? Definition.QuestCredit.CreditCount : 0;
        if (XPQuanta <= 0 && QuestCreditCount <= 0) {
            continue;
        }

        FMythicPendingHarvestCompletionDelivery &Delivery =
            OutDeliveries.AddDefaulted_GetRef();
        Delivery.CompletionKey = CompletionKey;
        Delivery.ContributorKey = Participant.ContributorKey;
        Delivery.CompletionProficiencyXPQuanta = XPQuanta;
        Delivery.bProficiencyDelivered = XPQuanta <= 0;
        if (!Delivery.bProficiencyDelivered) {
            Delivery.ProficiencyDefinitionId = ProficiencyDefinitionId;
            Delivery.ProficiencyDefinition = ProficiencyDefinition;
            if (Definition.ResourceTaxonomyTag.IsValid()) {
                Delivery.ProficiencyContextTags.AddTag(
                    Definition.ResourceTaxonomyTag);
            }
            Delivery.ProficiencyReceiptKey =
                FMythicHarvestReceiptKey::MakeCompletion(
                    CompletionKey.WorldEpoch, CompletionKey.NodeId,
                    CompletionKey.Generation,
                    EMythicHarvestReceiptChannel::CompletionProficiencyXP);
            Delivery.ProficiencyReceiptPayloadFingerprint =
                FMythicHarvestReceiptFingerprint::Build(
                    Delivery.ProficiencyReceiptKey,
                    ProficiencyDefinitionId, XPQuanta, 0, 0, 0,
                    Delivery.ProficiencyContextTags);
        }
        Delivery.QuestCreditCount = QuestCreditCount;
        Delivery.bQuestCreditDelivered = QuestCreditCount <= 0;
        if (!Delivery.bQuestCreditDelivered) {
            Delivery.HarvestableDefinitionId = HarvestableDefinitionId;
            Delivery.HarvestableDefinition =
                const_cast<UMythicHarvestableDefinition *>(&Definition);
            Delivery.QuestReceiptKey =
                FMythicHarvestReceiptKey::MakeCompletion(
                    CompletionKey.WorldEpoch, CompletionKey.NodeId,
                    CompletionKey.Generation,
                    EMythicHarvestReceiptChannel::CompletionQuestCredit);
            Delivery.QuestReceiptPayloadFingerprint =
                FMythicHarvestReceiptFingerprint::Build(
                    Delivery.QuestReceiptKey, HarvestableDefinitionId,
                    QuestCreditCount, 0, 0, 0);
        }
        Delivery.CurrentController = Participant.CurrentController;
    }
    SortPendingCompletionDeliveries(OutDeliveries);
    return true;
}

EMythicHarvestCompletionAdmission
UMythicHarvestRewardOutboxSubsystem::CommitPreparedCompletion(
    FMythicPreparedHarvestCompletion &&PreparedCompletion) {
    if (!PreparedCompletion.IsValid()) {
        return EMythicHarvestCompletionAdmission::Invalid;
    }
    if (KnownCompletions.Contains(PreparedCompletion.CompletionKey)) {
        return EMythicHarvestCompletionAdmission::AlreadyKnown;
    }
    const FMythicHarvestEpochNodeKey EpochNode{
        PreparedCompletion.CompletionKey.WorldEpoch,
        PreparedCompletion.CompletionKey.NodeId};
    const uint32 *Highest = HighestKnownGenerationByNode.Find(EpochNode);
    if ((!Highest
         && HighestKnownGenerationByNode.Num()
             >= FMythicHarvestRewardOutboxSaveV1::
                 AbsoluteMaximumKnownCompletions)
        || WouldExceedPendingCapacity(
            PreparedCompletion.Grants.Num()
            + PreparedCompletion.CompletionDeliveries.Num())
        || PreparedCompletionWouldExceedContributorCapacity(
            PreparedCompletion)
        || PreparedCompletionWouldExceedEscrowCapacity(
            PreparedCompletion)) {
        return EMythicHarvestCompletionAdmission::CapacityExceeded;
    }
    const uint32 Expected = Highest ? *Highest + 1u : 1u;
    if ((Highest && *Highest == MAX_uint32)
        || PreparedCompletion.CompletionKey.Generation != Expected) {
        return Highest
                && PreparedCompletion.CompletionKey.Generation <= *Highest
            ? EMythicHarvestCompletionAdmission::AlreadyKnown
            : EMythicHarvestCompletionAdmission::Invalid;
    }

    const EMythicHarvestCompletionAdmission Admission =
        TryCommitCompletionKey(KnownCompletions,
                               PreparedCompletion.CompletionKey);
    if (Admission != EMythicHarvestCompletionAdmission::Committed) {
        return Admission;
    }
    if (Highest) {
        const FMythicHarvestRewardCompletionKey PreviousWitness{
            PreparedCompletion.CompletionKey.WorldEpoch,
            PreparedCompletion.CompletionKey.NodeId, *Highest};
        if (KnownCompletions.Remove(PreviousWitness) != 1) {
            UE_LOG(Myth, Fatal,
                   TEXT("Harvest completion witness/high-water invariant failed for node %s generation %u."),
                   *PreparedCompletion.CompletionKey.NodeId.GetGuid().ToString(),
                   *Highest);
            return EMythicHarvestCompletionAdmission::Invalid;
        }
    }
    HighestKnownGenerationByNode.Add(
        EpochNode, PreparedCompletion.CompletionKey.Generation);

    PendingDeliveries.Reserve(
        PendingDeliveries.Num() + PreparedCompletion.Grants.Num());
    for (FMythicHarvestPlannedRewardGrant &Grant :
         PreparedCompletion.Grants) {
        FMythicPendingHarvestRewardDelivery &Pending =
            PendingDeliveries.AddDefaulted_GetRef();
        Pending.RemainingQuantity = Grant.Quantity;
        Pending.Grant = MoveTemp(Grant);
        AdjustPendingContributorRows(Pending.Grant.ContributorKey, 1);
        AdjustPendingItemContributorRows(
            Pending.Grant.ContributorKey, 1);
    }
    SortPendingDeliveries(PendingDeliveries);
    for (const FMythicPendingHarvestCompletionDelivery &Delivery :
         PreparedCompletion.CompletionDeliveries) {
        AdjustPendingContributorRows(Delivery.ContributorKey, 1);
    }
    PendingCompletionDeliveries.Append(
        MoveTemp(PreparedCompletion.CompletionDeliveries));
    SortPendingCompletionDeliveries(PendingCompletionDeliveries);

    TArray<FMythicHarvestReceiptKey> CompletedDurabilityKeys;
    DurabilityReceiptsByNodeGeneration.MultiFind(
        PreparedCompletion.CompletionKey, CompletedDurabilityKeys);
    for (const FMythicHarvestReceiptKey &ReceiptKey :
         CompletedDurabilityKeys) {
        const FMythicPendingHarvestDurabilityCost *Cost =
            DurabilityCostsByReceipt.Find(ReceiptKey);
        if (Cost && !Cost->HasPendingApplication()) {
            RemoveDurabilityCostSeries(ReceiptKey);
        }
    }
    return EMythicHarvestCompletionAdmission::Committed;
}

bool UMythicHarvestRewardOutboxSubsystem::PrepareAppliedWorkDelivery(
    const UMythicHarvestableDefinition &Definition,
    const FGuid &WorldEpoch, const FMythicHarvestNodeId &NodeId,
    const uint32 Generation,
    const FMythicHarvestParticipantSnapshot &Participant,
    const double AppliedWorkUnits,
    FMythicPreparedHarvestWorkDelivery &OutPreparedDelivery,
    FName &OutDiagnosticCode,
    const int32 ReservedAdditionalRows) const {
    OutPreparedDelivery = FMythicPreparedHarvestWorkDelivery();
    OutDiagnosticCode = NAME_None;
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client
        || !WorldEpoch.IsValid() || !NodeId.IsValid() || Generation == 0
        || !Participant.IsValid() || !FMath::IsFinite(AppliedWorkUnits)
        || AppliedWorkUnits <= 0.0
        || ReservedAdditionalRows < 0
        || ReservedAdditionalRows
            >= FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumPendingDeliveries
        || !FMath::IsFinite(Definition.ProficiencyXPPerAppliedWork)
        || Definition.ProficiencyXPPerAppliedWork < 0.0f) {
        OutDiagnosticCode = TEXT("InvalidAppliedWorkDelivery");
        return false;
    }
    FMythicHarvestWork AppliedWork;
    if (!FMythicHarvestWork::TryFromWorkUnits(
            AppliedWorkUnits, AppliedWork)
        || AppliedWork.IsZero()
        || Participant.ContributionQuanta < AppliedWork.GetQuanta()) {
        OutDiagnosticCode = TEXT("InvalidAppliedWorkXP");
        return false;
    }

    FMythicHarvestWorkRewardContract WorkRewardContract =
        Participant.WorkRewardContract;
    UProficiencyDefinition *ProficiencyDefinition = nullptr;
    if (WorkRewardContract.IsUnset()) {
        WorkRewardContract.bInitialized = true;
        if (Definition.ProficiencyXPPerAppliedWork > 0.0f) {
            ProficiencyDefinition = Definition.ProficiencyDefinition;
            WorkRewardContract.ProficiencyDefinitionId =
                ProficiencyDefinition
                ? ProficiencyDefinition->GetPrimaryAssetId()
                : FPrimaryAssetId();
            if (!FMythicHarvestReceiptQuantity::TryFromUnits(
                    Definition.ProficiencyXPPerAppliedWork,
                    WorkRewardContract.
                        ProficiencyXPPerWorkUnitQuanta)) {
                OutDiagnosticCode = TEXT("InvalidAppliedWorkXP");
                return false;
            }
            if (Definition.ResourceTaxonomyTag.IsValid()) {
                WorkRewardContract.ContextTags.AddTag(
                    Definition.ResourceTaxonomyTag);
            }
        }
    }
    if (!WorkRewardContract.IsValid()) {
        OutDiagnosticCode = TEXT("InvalidAppliedWorkRewardContract");
        return false;
    }

    FMythicPendingHarvestWorkDelivery &Delivery =
        OutPreparedDelivery.Delivery;
    Delivery.NodeGenerationKey = {WorldEpoch, NodeId, Generation};
    Delivery.ContributorKey = Participant.ContributorKey;
    Delivery.WorkRewardContract = WorkRewardContract;
    Delivery.CumulativeAppliedWorkQuanta = Participant.ContributionQuanta;
    Delivery.CurrentController = Participant.CurrentController;
    if (!WorkRewardContract.IsEnabled()) {
        return OutPreparedDelivery.IsValid();
    }
    if (!MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
            WorkRewardContract.ProficiencyDefinitionId,
            UMythicAssetManager::ProficiencyDefinitionType)) {
        OutDiagnosticCode = TEXT("InvalidAppliedWorkProficiencyIdentity");
        return false;
    }
    if (!ProficiencyDefinition
        && Definition.ProficiencyDefinition
        && Definition.ProficiencyDefinition->GetPrimaryAssetId()
            == WorkRewardContract.ProficiencyDefinitionId) {
        ProficiencyDefinition = Definition.ProficiencyDefinition;
    }
    int64 CumulativeXPQuanta = 0;
    if (!FMythicHarvestReceiptQuantity::
            TryCalculateCumulativeAppliedWorkXP(
                Participant.ContributionQuanta,
                WorkRewardContract.ProficiencyXPPerWorkUnitQuanta,
                CumulativeXPQuanta)) {
        OutDiagnosticCode = TEXT("InvalidAppliedWorkXP");
        return false;
    }
    const uint64 FirstObservableSequence =
        GetFirstObservableSnapshotSequence();
    if (FirstObservableSequence == 0) {
        OutDiagnosticCode = TEXT("RewardOutboxSequenceExhausted");
        return false;
    }

    Delivery.ProficiencyDefinition = ProficiencyDefinition;
    Delivery.ProficiencyXPQuanta = CumulativeXPQuanta;
    Delivery.ReceiptKey = FMythicHarvestReceiptKey::MakeAppliedWork(
        WorldEpoch, NodeId, Generation, Participant.ContributorKey);
    Delivery.ReceiptPayloadFingerprint =
        FMythicHarvestReceiptFingerprint::BuildAppliedWorkSeries(
            Delivery.ReceiptKey,
            WorkRewardContract.ProficiencyDefinitionId,
            WorkRewardContract.ProficiencyXPPerWorkUnitQuanta,
            WorkRewardContract.ContextTags);
    OutPreparedDelivery.FirstObservableWorldSnapshotSequence =
        FirstObservableSequence;
    OutPreparedDelivery.bHasDelivery = true;
    if (!OutPreparedDelivery.IsValid()) {
        OutPreparedDelivery = FMythicPreparedHarvestWorkDelivery();
        OutDiagnosticCode = TEXT("InvalidPreparedAppliedWorkDelivery");
        return false;
    }
    const bool bExtendsExistingSeries =
        PendingWorkDeliveries.ContainsByPredicate(
            [&Delivery](const FMythicPendingHarvestWorkDelivery &Existing) {
                return Existing.ContributorKey == Delivery.ContributorKey
                    && Existing.ReceiptKey == Delivery.ReceiptKey;
            });
    if (const FMythicPendingHarvestWorkDelivery *Existing =
            PendingWorkDeliveries.FindByPredicate(
                [&Delivery](
                    const FMythicPendingHarvestWorkDelivery &Candidate) {
                    return Candidate.ContributorKey
                            == Delivery.ContributorKey
                        && Candidate.ReceiptKey == Delivery.ReceiptKey;
                });
        Existing
        && !(Existing->WorkRewardContract
             == Delivery.WorkRewardContract)) {
        OutPreparedDelivery = FMythicPreparedHarvestWorkDelivery();
        OutDiagnosticCode = TEXT("AppliedWorkRewardContractDrift");
        return false;
    }
    if (WouldExceedPendingCapacity(
            ReservedAdditionalRows + (bExtendsExistingSeries ? 0 : 1))) {
        OutPreparedDelivery = FMythicPreparedHarvestWorkDelivery();
        OutDiagnosticCode = TEXT("RewardDeliveryCapacityExceeded");
        return false;
    }
    if (WouldExceedContributorPendingCapacity(
            Participant.ContributorKey,
            ReservedAdditionalRows + (bExtendsExistingSeries ? 0 : 1))) {
        OutPreparedDelivery = FMythicPreparedHarvestWorkDelivery();
        OutDiagnosticCode = TEXT("RewardContributorBackpressure");
        return false;
    }
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::
CommitPreparedAppliedWorkDelivery(
    FMythicPreparedHarvestWorkDelivery &&PreparedDelivery) {
    if (!PreparedDelivery.IsValid()) {
        return false;
    }
    if (!PreparedDelivery.bHasDelivery) {
        return true;
    }
    FMythicPendingHarvestWorkDelivery *Existing =
        PendingWorkDeliveries.FindByPredicate(
            [&PreparedDelivery](
                const FMythicPendingHarvestWorkDelivery &Candidate) {
                return Candidate.ContributorKey
                        == PreparedDelivery.Delivery.ContributorKey
                    && Candidate.ReceiptKey
                        == PreparedDelivery.Delivery.ReceiptKey;
            });
    if (Existing) {
        const FMythicPendingHarvestWorkDelivery &Incoming =
            PreparedDelivery.Delivery;
        if (!(Existing->WorkRewardContract
              == Incoming.WorkRewardContract)
            || Existing->ReceiptPayloadFingerprint
                != Incoming.ReceiptPayloadFingerprint) {
            return false;
        }
        if (Incoming.CumulativeAppliedWorkQuanta
            <= Existing->CumulativeAppliedWorkQuanta) {
            return true;
        }
        Existing->CumulativeAppliedWorkQuanta =
            Incoming.CumulativeAppliedWorkQuanta;
        Existing->ProficiencyXPQuanta = Incoming.ProficiencyXPQuanta;
        Existing->ProficiencyDefinition = Incoming.ProficiencyDefinition;
        Existing->CurrentController = Incoming.CurrentController;
        return true;
    }
    if (WouldExceedPendingCapacity(1)) {
        return false;
    }
    if (WouldExceedContributorPendingCapacity(
            PreparedDelivery.Delivery.ContributorKey, 1)) {
        return false;
    }
    PendingWorkDeliveries.Add(MoveTemp(PreparedDelivery.Delivery));
    AdjustPendingContributorRows(
        PendingWorkDeliveries.Last().ContributorKey, 1);
    SortPendingWorkDeliveries(PendingWorkDeliveries);
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::PrepareDurabilityCost(
    const FGuid &WorldEpoch, const FMythicHarvestNodeId &NodeId,
    const uint32 Generation, const FString &ContributorKey,
    UMythicItemInstance &Tool, const int32 WearAmount,
    AMythicPlayerController &Controller,
    FMythicPreparedHarvestDurabilityCost &OutPreparedCost,
    FName &OutDiagnosticCode) const {
    OutPreparedCost = FMythicPreparedHarvestDurabilityCost();
    OutDiagnosticCode = NAME_None;
    UWorld *World = GetWorld();
    const FGuid ToolGuid = Tool.GetItemInstanceGuid();
    if (!World || World->GetNetMode() == NM_Client
        || !WorldEpoch.IsValid() || !NodeId.IsValid() || Generation == 0
        || ContributorKey.IsEmpty() || !ToolGuid.IsValid()
        || WearAmount <= 0 || !Controller.HasAuthority()
        || Tool.GetInventoryOwner() != &Controller
        || GetFirstObservableSnapshotSequence() == 0) {
        OutDiagnosticCode = TEXT("InvalidHarvestDurabilityCost");
        return false;
    }

    FMythicPendingHarvestDurabilityCost &Cost = OutPreparedCost.Cost;
    Cost.NodeGenerationKey = {WorldEpoch, NodeId, Generation};
    Cost.ContributorKey = ContributorKey;
    Cost.ToolItemInstanceGuid = ToolGuid;
    Cost.ReceiptKey = FMythicHarvestReceiptKey::MakeDurabilityCost(
        WorldEpoch, NodeId, Generation, ContributorKey, ToolGuid);
    Cost.ReceiptPayloadFingerprint =
        FMythicHarvestReceiptFingerprint::BuildDurabilityCostSeries(
            Cost.ReceiptKey, ToolGuid);
    Cost.CurrentTool = &Tool;
    Cost.CurrentController = &Controller;
    if (!Cost.ReceiptKey.IsValid()
        || !Cost.ReceiptPayloadFingerprint.IsValid()) {
        OutDiagnosticCode = TEXT("InvalidHarvestDurabilityReceipt");
        return false;
    }

    const FMythicPendingHarvestDurabilityCost *Existing =
        DurabilityCostsByReceipt.Find(Cost.ReceiptKey);
    if (Existing) {
        if (Existing->ContributorKey != ContributorKey
            || Existing->ToolItemInstanceGuid != ToolGuid
            || Existing->ReceiptPayloadFingerprint
                != Cost.ReceiptPayloadFingerprint
            || !(Existing->NodeGenerationKey
                 == Cost.NodeGenerationKey)
            || Existing->CumulativeWearTarget <= 0
            || Existing->DurablyAppliedWearTarget < 0
            || Existing->DurablyAppliedWearTarget
                > Existing->CumulativeWearTarget) {
            OutDiagnosticCode = TEXT("HarvestDurabilityCostContractDrift");
            return false;
        }
        OutPreparedCost.PreviousCumulativeWearTarget =
            Existing->CumulativeWearTarget;
        Cost.DurablyAppliedWearTarget =
            Existing->DurablyAppliedWearTarget;
    }
    else {
        if (DurabilityCostsByReceipt.Num()
                >= FMythicHarvestRewardOutboxSaveV1::
                    AbsoluteMaximumDurabilityCostSeries
            || WouldExceedDurabilityContributorCapacity(
                ContributorKey, 1)
            || (!ContributorLedgerFenceByKey.Contains(ContributorKey)
                && ContributorLedgerFenceByKey.Num()
                    >= FMythicHarvestRewardOutboxSaveV1::
                        AbsoluteMaximumContributorLedgerFences)) {
            OutDiagnosticCode = TEXT("HarvestDurabilityCostBackpressure");
            return false;
        }
    }
    if (OutPreparedCost.PreviousCumulativeWearTarget
        > MAX_int64 - static_cast<int64>(WearAmount)) {
        OutDiagnosticCode = TEXT("HarvestDurabilityCostOverflow");
        return false;
    }
    Cost.CumulativeWearTarget =
        OutPreparedCost.PreviousCumulativeWearTarget + WearAmount;
    OutPreparedCost.bHasCost = true;
    if (!OutPreparedCost.IsValid()) {
        OutPreparedCost = FMythicPreparedHarvestDurabilityCost();
        OutDiagnosticCode = TEXT("InvalidPreparedHarvestDurabilityCost");
        return false;
    }
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::
CommitPreparedDurabilityCost(
    FMythicPreparedHarvestDurabilityCost &&PreparedCost) {
    if (!PreparedCost.IsValid() || !PreparedCost.bHasCost) {
        return false;
    }
    FMythicPendingHarvestDurabilityCost *Existing =
        DurabilityCostsByReceipt.Find(PreparedCost.Cost.ReceiptKey);
    if (Existing) {
        if (Existing->ContributorKey != PreparedCost.Cost.ContributorKey
            || Existing->ToolItemInstanceGuid
                != PreparedCost.Cost.ToolItemInstanceGuid
            || Existing->ReceiptPayloadFingerprint
                != PreparedCost.Cost.ReceiptPayloadFingerprint) {
            return false;
        }
        if (Existing->CumulativeWearTarget
            == PreparedCost.Cost.CumulativeWearTarget) {
            return true;
        }
        if (Existing->CumulativeWearTarget
                != PreparedCost.PreviousCumulativeWearTarget
            || PreparedCost.Cost.CumulativeWearTarget
                <= Existing->CumulativeWearTarget) {
            return false;
        }
        const bool bWasPending = Existing->HasPendingApplication();
        Existing->CumulativeWearTarget =
            PreparedCost.Cost.CumulativeWearTarget;
        Existing->CurrentTool = PreparedCost.Cost.CurrentTool;
        Existing->CurrentController =
            PreparedCost.Cost.CurrentController;
        if (!bWasPending) {
            PendingDurabilityReceiptOrder.Add(Existing->ReceiptKey);
            PendingDurabilityReceiptsByContributor.Add(
                Existing->ContributorKey, Existing->ReceiptKey);
        }
        return true;
    }
    if (PreparedCost.PreviousCumulativeWearTarget != 0
        || DurabilityCostsByReceipt.Num()
            >= FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumDurabilityCostSeries
        || WouldExceedDurabilityContributorCapacity(
            PreparedCost.Cost.ContributorKey, 1)) {
        return false;
    }
    const FString ContributorKey = PreparedCost.Cost.ContributorKey;
    const FMythicHarvestReceiptKey ReceiptKey =
        PreparedCost.Cost.ReceiptKey;
    DurabilityCostsByReceipt.Add(
        ReceiptKey, MoveTemp(PreparedCost.Cost));
    DurabilityReceiptsByNodeGeneration.Add(
        DurabilityCostsByReceipt.FindChecked(ReceiptKey).NodeGenerationKey,
        ReceiptKey);
    AdjustDurabilityContributorSeries(ContributorKey, 1);
    PendingDurabilityReceiptOrder.Add(ReceiptKey);
    PendingDurabilityReceiptsByContributor.Add(
        ContributorKey, ReceiptKey);
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::CanAccrueDurabilityCost(
    const FString &ContributorKey) const {
    if (ContributorKey.IsEmpty()) return false;
    TArray<FMythicHarvestReceiptKey> PendingKeys;
    PendingDurabilityReceiptsByContributor.MultiFind(
        ContributorKey, PendingKeys);
    if (PendingKeys.IsEmpty()) return true;
    AMythicPlayerState *PlayerState =
        ResolveContributorPlayerState(ContributorKey);
    UMythicHarvestReceiptLedgerComponent *Ledger =
        ResolveReceiptLedger(PlayerState);
    if (!Ledger) return false;
    for (const FMythicHarvestReceiptKey &Key : PendingKeys) {
        const FMythicPendingHarvestDurabilityCost *Cost =
            DurabilityCostsByReceipt.Find(Key);
        if (!Cost || !Cost->HasPendingApplication()
            || Ledger->GetAppliedQuantity(Key)
                < Cost->CumulativeWearTarget) {
            return false;
        }
    }
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::
IsContributorAwaitingCurrentCharacterSave(
    const FString &ContributorKey) const {
    const FMythicHarvestCharacterSaveRequestIdentity *Request =
        CharacterSaveRequestsByContributor.Find(ContributorKey);
    return Request && Request->RestoreDomainEpoch == RestoreDomainEpoch;
}

bool UMythicHarvestRewardOutboxSubsystem::IsGenerationCompleted(
    const FMythicHarvestRewardCompletionKey &Key) const {
    return Key.IsValid() && HasKnownCompletion(
        Key.WorldEpoch, Key.NodeId, Key.Generation);
}

void UMythicHarvestRewardOutboxSubsystem::RemoveDurabilityCostSeries(
    const FMythicHarvestReceiptKey &ReceiptKey) {
    FMythicPendingHarvestDurabilityCost Removed;
    if (!DurabilityCostsByReceipt.RemoveAndCopyValue(
            ReceiptKey, Removed)) {
        return;
    }
    AdjustDurabilityContributorSeries(Removed.ContributorKey, -1);
    DurabilityReceiptsByNodeGeneration.RemoveSingle(
        Removed.NodeGenerationKey, ReceiptKey);
    RemovePendingDurabilityReceipt(Removed.ContributorKey, ReceiptKey);
}

void UMythicHarvestRewardOutboxSubsystem::
RemovePendingDurabilityReceipt(
    const FString &ContributorKey,
    const FMythicHarvestReceiptKey &ReceiptKey) {
    const int32 PendingIndex =
        PendingDurabilityReceiptOrder.Find(ReceiptKey);
    if (PendingIndex != INDEX_NONE) {
        PendingDurabilityReceiptOrder.RemoveAt(
            PendingIndex, 1, EAllowShrinking::No);
        PendingDurabilityReceiptsByContributor.RemoveSingle(
            ContributorKey, ReceiptKey);
        if (DurabilityRetryRowCursor > PendingIndex) {
            --DurabilityRetryRowCursor;
        }
        if (PendingDurabilityReceiptOrder.IsEmpty()) {
            DurabilityRetryRowCursor = 0;
        }
        else {
            DurabilityRetryRowCursor %=
                PendingDurabilityReceiptOrder.Num();
        }
    }
}

EMythicHarvestCompletionAdmission
UMythicHarvestRewardOutboxSubsystem::TryCommitCompletionKey(
    TSet<FMythicHarvestRewardCompletionKey> &InOutKnownCompletions,
    const FMythicHarvestRewardCompletionKey &CompletionKey) {
    if (!CompletionKey.IsValid()) {
        return EMythicHarvestCompletionAdmission::Invalid;
    }
    if (InOutKnownCompletions.Contains(CompletionKey)) {
        return EMythicHarvestCompletionAdmission::AlreadyKnown;
    }
    InOutKnownCompletions.Add(CompletionKey);
    return EMythicHarvestCompletionAdmission::Committed;
}

int32 UMythicHarvestRewardOutboxSubsystem::
CalculateRemainingQuantityAfterInsertion(const int32 RemainingQuantity,
                                         const int32 InsertedQuantity) {
    const int32 CanonicalRemaining = FMath::Max(0, RemainingQuantity);
    return CanonicalRemaining - FMath::Clamp(
        InsertedQuantity, 0, CanonicalRemaining);
}

FMythicHarvestRewardRetryBudgets
UMythicHarvestRewardOutboxSubsystem::CalculateRetryBudgets(
    const int32 MaximumAttempts,
    const bool bHasCompletionQueue,
    const bool bHasItemQueue,
    const bool bHasWorkQueue,
    const bool bHasDurabilityQueue,
    const bool bHasEscrowQueue,
    const uint8 QueueCursor) {
    FMythicHarvestRewardRetryBudgets Result;
    Result.NextQueueCursor = QueueCursor % 5;
    if (MaximumAttempts <= 0) {
        return Result;
    }

    const bool Active[5] = {
        bHasCompletionQueue, bHasItemQueue, bHasWorkQueue,
        bHasDurabilityQueue, bHasEscrowQueue};
    int32 *Budgets[5] = {
        &Result.CompletionBudget, &Result.ItemBudget,
        &Result.WorkBudget, &Result.DurabilityBudget,
        &Result.EscrowBudget};
    const int32 ActiveCount =
        static_cast<int32>(bHasCompletionQueue)
        + static_cast<int32>(bHasItemQueue)
        + static_cast<int32>(bHasWorkQueue)
        + static_cast<int32>(bHasDurabilityQueue)
        + static_cast<int32>(bHasEscrowQueue);
    if (ActiveCount == 0) {
        return Result;
    }

    const int32 Base = MaximumAttempts / ActiveCount;
    for (int32 Index = 0; Index < 5; ++Index) {
        if (Active[Index]) {
            *Budgets[Index] = Base;
        }
    }
    int32 Remaining = MaximumAttempts % ActiveCount;
    while (Remaining > 0) {
        for (int32 Probe = 0; Probe < 5; ++Probe) {
            const uint8 Candidate =
                (Result.NextQueueCursor + Probe) % 5;
            if (!Active[Candidate]) {
                continue;
            }
            ++*Budgets[Candidate];
            --Remaining;
            Result.NextQueueCursor = (Candidate + 1) % 5;
            break;
        }
    }
    return Result;
}

bool UMythicHarvestRewardOutboxSubsystem::ValidateReceiptInSnapshot(
    const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
    const FMythicHarvestReceiptKey &Key, const FGuid &Fingerprint,
    const int64 TargetQuantity, int64 &OutAppliedQuantity,
    EMythicHarvestQuestReceiptDisposition *OutQuestDisposition) {
    OutAppliedQuantity = 0;
    if (OutQuestDisposition) {
        *OutQuestDisposition =
            EMythicHarvestQuestReceiptDisposition::None;
    }
    const FMythicSavedHarvestReceiptRowV1 *Row = Snapshot.FindRow(Key);
    if (!Row || Row->PayloadFingerprint != Fingerprint
        || Row->TargetQuantity != TargetQuantity
        || Row->AppliedQuantity < 0
        || Row->AppliedQuantity > TargetQuantity) {
        return false;
    }
    OutAppliedQuantity = Row->AppliedQuantity;
    if (OutQuestDisposition) {
        *OutQuestDisposition = Row->QuestDisposition;
    }
    return true;
}

void UMythicHarvestRewardOutboxSubsystem::
ReconcileContributorFromDurableSnapshot(
    const FString &ContributorKey,
    const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
    const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot) {
    for (int32 Index = PendingDeliveries.Num() - 1; Index >= 0;
         --Index) {
        FMythicPendingHarvestRewardDelivery &Pending =
            PendingDeliveries[Index];
        if (Pending.Grant.ContributorKey != ContributorKey) {
            continue;
        }
        int64 Applied = 0;
        if (ValidateReceiptInSnapshot(
                Snapshot, Pending.Grant.ReceiptKey,
                Pending.Grant.ReceiptPayloadFingerprint,
                Pending.Grant.Quantity, Applied)) {
            const FMythicSavedHarvestReceiptRowV1 *ReceiptRow =
                Snapshot.FindRow(Pending.Grant.ReceiptKey);
            const FMythicSavedHarvestItemEscrowRowV1 *EscrowRow =
                EscrowSnapshot.FindRow(Pending.Grant.ReceiptKey);
            if (EscrowRow
                && (!ReceiptRow
                    || !EscrowRow->HasSameContract(BuildEscrowRow(
                        Pending,
                        ReceiptRow->FirstObservedWorldSnapshotSequence)))) {
                continue;
            }
            Pending.RemainingQuantity = Pending.Grant.Quantity
                - static_cast<int32>(Applied);
            if (Pending.RemainingQuantity <= 0) {
                AdjustPendingContributorRows(
                    Pending.Grant.ContributorKey, -1);
                AdjustPendingItemContributorRows(
                    Pending.Grant.ContributorKey, -1);
                PendingDeliveries.RemoveAt(
                    Index, 1, EAllowShrinking::No);
            }
        }
    }

    for (int32 Index = PendingCompletionDeliveries.Num() - 1;
         Index >= 0; --Index) {
        FMythicPendingHarvestCompletionDelivery &Delivery =
            PendingCompletionDeliveries[Index];
        if (Delivery.ContributorKey != ContributorKey) {
            continue;
        }
        if (!Delivery.bProficiencyDelivered) {
            int64 Applied = 0;
            if (ValidateReceiptInSnapshot(
                    Snapshot, Delivery.ProficiencyReceiptKey,
                    Delivery.ProficiencyReceiptPayloadFingerprint,
                    Delivery.CompletionProficiencyXPQuanta, Applied)
                && Applied == Delivery.CompletionProficiencyXPQuanta) {
                Delivery.bProficiencyDelivered = true;
            }
        }
        if (!Delivery.bQuestCreditDelivered) {
            int64 Applied = 0;
            EMythicHarvestQuestReceiptDisposition Disposition =
                EMythicHarvestQuestReceiptDisposition::None;
            if (ValidateReceiptInSnapshot(
                    Snapshot, Delivery.QuestReceiptKey,
                    Delivery.QuestReceiptPayloadFingerprint,
                    Delivery.QuestCreditCount, Applied, &Disposition)
                && Applied == Delivery.QuestCreditCount
                && (Disposition
                        == EMythicHarvestQuestReceiptDisposition::Matched
                    || Disposition
                        == EMythicHarvestQuestReceiptDisposition::NoMatch)) {
                Delivery.bQuestCreditDelivered = true;
            }
        }
        if (Delivery.IsComplete()) {
            AdjustPendingContributorRows(Delivery.ContributorKey, -1);
            PendingCompletionDeliveries.RemoveAt(
                Index, 1, EAllowShrinking::No);
        }
    }

    for (int32 Index = PendingWorkDeliveries.Num() - 1; Index >= 0;
         --Index) {
        const FMythicPendingHarvestWorkDelivery &Delivery =
            PendingWorkDeliveries[Index];
        if (Delivery.ContributorKey != ContributorKey) {
            continue;
        }
        const FMythicSavedHarvestReceiptRowV1 *Row =
            Snapshot.FindRow(Delivery.ReceiptKey);
        if (Row
            && Row->PayloadFingerprint
                == Delivery.ReceiptPayloadFingerprint
            && Row->AppliedQuantity >= Delivery.ProficiencyXPQuanta
            && Row->AppliedQuantity == Row->TargetQuantity) {
            AdjustPendingContributorRows(Delivery.ContributorKey, -1);
            PendingWorkDeliveries.RemoveAt(
                Index, 1, EAllowShrinking::No);
        }
    }

    TArray<FMythicHarvestReceiptKey> DurabilityKeys;
    PendingDurabilityReceiptsByContributor.MultiFind(
        ContributorKey, DurabilityKeys);
    for (const FMythicHarvestReceiptKey &ReceiptKey : DurabilityKeys) {
        FMythicPendingHarvestDurabilityCost *Cost =
            DurabilityCostsByReceipt.Find(ReceiptKey);
        const FMythicSavedHarvestReceiptRowV1 *Row =
            Snapshot.FindRow(ReceiptKey);
        if (!Cost || !Row
            || Row->PayloadFingerprint
                != Cost->ReceiptPayloadFingerprint
            || Row->AppliedQuantity < Cost->CumulativeWearTarget
            || Row->AppliedQuantity != Row->TargetQuantity) {
            continue;
        }
        const FMythicHarvestRewardCompletionKey NodeGenerationKey =
            Cost->NodeGenerationKey;
        Cost->DurablyAppliedWearTarget =
            Cost->CumulativeWearTarget;
        RemovePendingDurabilityReceipt(ContributorKey, ReceiptKey);
        if (IsGenerationCompleted(NodeGenerationKey)) {
            RemoveDurabilityCostSeries(ReceiptKey);
        }
    }
}

bool UMythicHarvestRewardOutboxSubsystem::
ValidateCharacterReceiptSnapshotAgainstWorld(
    const FMythicHarvestRewardOutboxSaveV1 &WorldSnapshot,
    const FString &ContributorKey,
    const FMythicHarvestReceiptLedgerSaveV1 &CharacterSnapshot,
    const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot,
    FName &OutDiagnosticCode) {
    if (ContributorKey.IsEmpty()
        || !FMythicHarvestReceiptLedgerSaveV1::Validate(
            CharacterSnapshot, OutDiagnosticCode)
        || !FMythicHarvestItemEscrowSaveV1::Validate(
            EscrowSnapshot, OutDiagnosticCode)
        || !FMythicHarvestItemEscrowSaveV1::ValidateReceiptBinding(
            EscrowSnapshot, CharacterSnapshot, OutDiagnosticCode)) {
        if (OutDiagnosticCode.IsNone()) {
            OutDiagnosticCode = TEXT("InvalidHarvestContributorSnapshot");
        }
        return false;
    }
    const FMythicSavedHarvestContributorLedgerFenceV1 *Fence =
        WorldSnapshot.ContributorLedgerFences.FindByPredicate(
            [&ContributorKey](
                const FMythicSavedHarvestContributorLedgerFenceV1 &Row) {
                return Row.ContributorKey == ContributorKey;
            });
    if (!Fence) {
        OutDiagnosticCode = NAME_None;
        return true;
    }
    if (Fence->LedgerEpoch != CharacterSnapshot.LedgerEpoch) {
        OutDiagnosticCode = TEXT("HarvestCharacterLedgerLineageMismatch");
        return false;
    }
    if (CharacterSnapshot.LedgerRevision
        < Fence->MinimumLedgerRevision) {
        OutDiagnosticCode = TEXT("HarvestCharacterSnapshotOlderThanWorld");
        return false;
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::
ValidateCharacterReceiptSnapshot(
    const FString &ContributorKey,
    const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
    const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot,
    FName &OutDiagnosticCode) const {
    if (ContributorKey.IsEmpty()
        || !FMythicHarvestReceiptLedgerSaveV1::Validate(
            Snapshot, OutDiagnosticCode)
        || !FMythicHarvestItemEscrowSaveV1::Validate(
            EscrowSnapshot, OutDiagnosticCode)
        || !FMythicHarvestItemEscrowSaveV1::ValidateReceiptBinding(
            EscrowSnapshot, Snapshot, OutDiagnosticCode)) {
        if (OutDiagnosticCode.IsNone()) {
            OutDiagnosticCode = TEXT("InvalidHarvestContributorSnapshot");
        }
        return false;
    }
    const FMythicSavedHarvestContributorLedgerFenceV1 *Fence =
        ContributorLedgerFenceByKey.Find(ContributorKey);
    if (!Fence) {
        OutDiagnosticCode = NAME_None;
        return true;
    }
    if (Fence->LedgerEpoch != Snapshot.LedgerEpoch) {
        OutDiagnosticCode = TEXT("HarvestCharacterLedgerLineageMismatch");
        return false;
    }
    if (Snapshot.LedgerRevision < Fence->MinimumLedgerRevision) {
        OutDiagnosticCode = TEXT("HarvestCharacterSnapshotOlderThanWorld");
        return false;
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::
ObserveDurableCharacterReceiptSnapshot(
    const FString &ContributorKey,
    const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
    const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot,
    FName &OutDiagnosticCode) {
    if (!ValidateCharacterReceiptSnapshot(
            ContributorKey, Snapshot, EscrowSnapshot,
            OutDiagnosticCode)) {
        return false;
    }
    if (Snapshot.LedgerRevision > 0) {
        FMythicSavedHarvestContributorLedgerFenceV1 *Fence =
            ContributorLedgerFenceByKey.Find(ContributorKey);
        if (!Fence) {
            if (ContributorLedgerFenceByKey.Num()
                >= FMythicHarvestRewardOutboxSaveV1::
                    AbsoluteMaximumContributorLedgerFences) {
                OutDiagnosticCode =
                    TEXT("HarvestContributorLedgerFenceCapacityExceeded");
                return false;
            }
            FMythicSavedHarvestContributorLedgerFenceV1 NewFence;
            NewFence.ContributorKey = ContributorKey;
            NewFence.LedgerEpoch = Snapshot.LedgerEpoch;
            NewFence.MinimumLedgerRevision = Snapshot.LedgerRevision;
            ContributorLedgerFenceByKey.Add(
                ContributorKey, MoveTemp(NewFence));
            ReservedContributorLedgerFenceKeys.Remove(ContributorKey);
        }
        else {
            Fence->MinimumLedgerRevision = FMath::Max(
                Fence->MinimumLedgerRevision,
                Snapshot.LedgerRevision);
        }
        ReservedContributorLedgerFenceKeys.Remove(ContributorKey);
    }
    if (AMythicPlayerState *PlayerState =
            ResolveContributorPlayerState(ContributorKey)) {
        TrackedReceiptOwners.Add(PlayerState);
    }
    ReconcileContributorFromDurableSnapshot(
        ContributorKey, Snapshot, EscrowSnapshot);
    OutDiagnosticCode = NAME_None;
    return true;
}

void UMythicHarvestRewardOutboxSubsystem::
HandleDurableCharacterSaveResult(
    FString ExpectedContributorKey,
    const FGuid ExpectedRequestToken,
    const TSharedRef<FGuid> ExpectedOperationId,
    const uint64 ExpectedRestoreDomainEpoch,
    const FMythicDurableCharacterSaveResult &Result) {
    FMythicHarvestCharacterSaveRequestIdentity *Latched =
        CharacterSaveRequestsByContributor.Find(ExpectedContributorKey);
    if (Latched && !Latched->OperationId.IsValid()
        && Latched->RequestToken == ExpectedRequestToken
        && Latched->RestoreDomainEpoch == ExpectedRestoreDomainEpoch) {
        Latched->OperationId = *ExpectedOperationId;
    }
    const bool bCurrent = Latched
        && FMythicHarvestCharacterSaveCallbackPolicy::
            MatchesCurrentRequest(
                *Latched, ExpectedRequestToken, Result.OperationId,
                ExpectedRestoreDomainEpoch, RestoreDomainEpoch)
        && Result.OperationId == *ExpectedOperationId
        && Result.RoutingCharacterId == ExpectedContributorKey;
    if (Latched && Latched->RequestToken == ExpectedRequestToken
        && Latched->RestoreDomainEpoch == ExpectedRestoreDomainEpoch) {
        CharacterSaveRequestsByContributor.Remove(
            ExpectedContributorKey);
    }
    if (!bCurrent) {
        const bool bExactPhysicalWrite =
            Result.OperationId == *ExpectedOperationId
            && Result.RoutingCharacterId == ExpectedContributorKey;
        if (bExactPhysicalWrite
            && FMythicHarvestCharacterSaveCallbackPolicy::
                RequiresCorrectiveSave(
                    Result.bSuccess, ExpectedRestoreDomainEpoch,
                    RestoreDomainEpoch,
                    IsContributorAwaitingCurrentCharacterSave(
                        ExpectedContributorKey))) {
            if (AMythicPlayerState *PlayerState =
                    ResolveContributorPlayerState(
                        ExpectedContributorKey)) {
                RequestContributorDurabilitySave(
                    ExpectedContributorKey, *PlayerState);
            }
        }
        return;
    }
    if (!Result.bSuccess) {
        return;
    }

    FName Diagnostic;
    if (!FMythicHarvestReceiptLedgerSaveV1::Validate(
            Result.CapturedHarvestReceipts, Diagnostic)
        || !FMythicHarvestItemEscrowSaveV1::Validate(
            Result.CapturedHarvestItemEscrow, Diagnostic)
        || !FMythicHarvestItemEscrowSaveV1::ValidateReceiptBinding(
            Result.CapturedHarvestItemEscrow,
            Result.CapturedHarvestReceipts, Diagnostic)) {
        UE_LOG(Myth, Error,
               TEXT("Harvest receipt durability callback rejected for %s: %s"),
               *ExpectedContributorKey, *Diagnostic.ToString());
        return;
    }
    if (AMythicPlayerState *PlayerState =
            ResolveContributorPlayerState(ExpectedContributorKey)) {
        if (UMythicHarvestReceiptLedgerComponent *Ledger =
                ResolveReceiptLedger(PlayerState)) {
            if (!Ledger->MarkSnapshotDurable(
                    Result.CapturedHarvestReceipts, Diagnostic)) {
                UE_LOG(Myth, Error,
                       TEXT("Live harvest receipt ledger rejected durable snapshot for %s: %s"),
                       *ExpectedContributorKey, *Diagnostic.ToString());
                return;
            }
        }
        if (UMythicHarvestRewardEscrowComponent *Escrow =
                ResolveRewardEscrow(PlayerState)) {
            if (!Escrow->MarkSnapshotDurable(
                    Result.CapturedHarvestItemEscrow, Diagnostic)) {
                UE_LOG(Myth, Error,
                       TEXT("Live harvest item escrow rejected durable snapshot for %s: %s"),
                       *ExpectedContributorKey, *Diagnostic.ToString());
                return;
            }
        }
    }
    if (!ObserveDurableCharacterReceiptSnapshot(
            ExpectedContributorKey, Result.CapturedHarvestReceipts,
            Result.CapturedHarvestItemEscrow,
            Diagnostic)) {
        UE_LOG(Myth, Error,
               TEXT("Durable harvest character snapshot could not advance its world fence for %s: %s"),
               *ExpectedContributorKey, *Diagnostic.ToString());
    }
}

bool UMythicHarvestRewardOutboxSubsystem::
RequestContributorDurabilitySave(const FString &ContributorKey,
                                 AMythicPlayerState &PlayerState) {
    if (ContributorKey.IsEmpty()
        || PlayerState.GetPersistentCharacterId() != ContributorKey
        || !PlayerState.HasAuthority()
        || IsContributorAwaitingCurrentCharacterSave(ContributorKey)) {
        return false;
    }
    UGameInstance *GameInstance = GetWorld()
        ? GetWorld()->GetGameInstance() : nullptr;
    UMythicSaveGameSubsystem *SaveSubsystem = GameInstance
        ? GameInstance->GetSubsystem<UMythicSaveGameSubsystem>() : nullptr;
    if (!SaveSubsystem) {
        return false;
    }
    TWeakObjectPtr<AMythicPlayerController> ControllerFastPath;
    AMythicPlayerController *Controller = ResolveContributorController(
        ContributorKey, ControllerFastPath);
    if (!Controller || Controller->GetPlayerState<AMythicPlayerState>()
            != &PlayerState) {
        return false;
    }

    FMythicHarvestCharacterSaveRequestIdentity RequestIdentity;
    do {
        RequestIdentity.RequestToken = FGuid::NewGuid();
    } while (!RequestIdentity.RequestToken.IsValid());
    RequestIdentity.RestoreDomainEpoch = RestoreDomainEpoch;
    TSharedRef<FGuid> ExpectedOperationId = MakeShared<FGuid>();
    FMythicDurableCharacterSaveComplete Completion;
    Completion.BindWeakLambda(
        this,
        [this, ContributorKey,
         RequestToken = RequestIdentity.RequestToken,
         ExpectedOperationId,
         ExpectedDomain = RequestIdentity.RestoreDomainEpoch](
            const FMythicDurableCharacterSaveResult &Result) {
            HandleDurableCharacterSaveResult(
                ContributorKey, RequestToken, ExpectedOperationId,
                ExpectedDomain, Result);
        });
    CharacterSaveRequestsByContributor.Add(
        ContributorKey, RequestIdentity);
    const bool bQueued = SaveSubsystem->RequestDurableCharacterSave(
        Controller, ContributorKey, MoveTemp(Completion),
        *ExpectedOperationId);
    if (FMythicHarvestCharacterSaveRequestIdentity *Pending =
            CharacterSaveRequestsByContributor.Find(ContributorKey);
        Pending
        && Pending->RequestToken == RequestIdentity.RequestToken
        && Pending->RestoreDomainEpoch == RequestIdentity.RestoreDomainEpoch) {
        if (!bQueued || !ExpectedOperationId->IsValid()) {
            CharacterSaveRequestsByContributor.Remove(ContributorKey);
        }
        else {
            Pending->OperationId = *ExpectedOperationId;
        }
    }
    return bQueued;
}

FMythicHarvestRewardRetryResult
UMythicHarvestRewardOutboxSubsystem::RetryPendingDeliveries(
    const int32 MaxGrantAttempts) {
    FMythicHarvestRewardRetryResult Result;
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client
        || MaxGrantAttempts <= 0) {
        Result.PendingGrantCount = PendingDeliveries.Num();
        Result.PendingCompletionDeliveryCount =
            PendingCompletionDeliveries.Num();
        Result.PendingAppliedWorkDeliveryCount =
            PendingWorkDeliveries.Num();
        return Result;
    }

    RequestPendingDefinitionLoads();
    UGameInstance *GameInstance = World->GetGameInstance();
    UMythicItemFactorySubsystem *Factory = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemFactorySubsystem>() : nullptr;
    const FMythicHarvestRewardRetryBudgets RetryBudgets =
        CalculateRetryBudgets(
            MaxGrantAttempts, !PendingCompletionDeliveries.IsEmpty(),
            !PendingDeliveries.IsEmpty() && Factory,
            !PendingWorkDeliveries.IsEmpty(), HasPendingDurabilityCost(),
            HasPendingEscrowWork() && Factory,
            RetryQueueCursor);
    const int32 CompletionBudget = RetryBudgets.CompletionBudget;
    const int32 WorkBudget = RetryBudgets.WorkBudget;
    const int32 ItemBudget = RetryBudgets.ItemBudget;
    const int32 DurabilityBudget = RetryBudgets.DurabilityBudget;
    const int32 EscrowBudget = RetryBudgets.EscrowBudget;
    RetryQueueCursor = RetryBudgets.NextQueueCursor;

    TSet<FString> ContributorsWithLiveMutation;
    const uint64 FirstObservableSequence =
        GetFirstObservableSnapshotSequence();

    const int32 CompletionRowsAtStart =
        PendingCompletionDeliveries.Num();
    int32 CompletionRowsVisited = 0;
    int32 CompletionIndex = CompletionRowsAtStart > 0
        ? CompletionRetryRowCursor % CompletionRowsAtStart : 0;
    auto AdvanceCompletionIndex = [this, &CompletionIndex]() {
        CompletionIndex = AdvanceRetryRowCursor(
            CompletionIndex, PendingCompletionDeliveries.Num(), false);
    };
    while (!PendingCompletionDeliveries.IsEmpty()
           && CompletionRowsVisited < CompletionRowsAtStart
           && Result.AttemptedCompletionDeliveryCount
               < CompletionBudget) {
        FMythicPendingHarvestCompletionDelivery &Delivery =
            PendingCompletionDeliveries[CompletionIndex];
        ++CompletionRowsVisited;
        ++Result.AttemptedCompletionDeliveryCount;
        if (IsContributorAwaitingCurrentCharacterSave(
                Delivery.ContributorKey)) {
            AdvanceCompletionIndex();
            continue;
        }

        AMythicPlayerController *Controller =
            ResolveContributorController(
                Delivery.ContributorKey, Delivery.CurrentController);
        AMythicPlayerState *PlayerState =
            ResolveContributorPlayerState(Delivery.ContributorKey);
        UMythicHarvestReceiptLedgerComponent *Ledger =
            ResolveReceiptLedger(PlayerState);
        if (!Controller || !Controller->HasAuthority() || !PlayerState
            || !Ledger || FirstObservableSequence == 0) {
            AdvanceCompletionIndex();
            continue;
        }
        TrackedReceiptOwners.Add(PlayerState);

        if (!Delivery.bProficiencyDelivered) {
            const int64 Target = Delivery.CompletionProficiencyXPQuanta;
            if (Ledger->GetDurableAppliedQuantity(
                    Delivery.ProficiencyReceiptKey) == Target) {
                Delivery.bProficiencyDelivered = true;
            }
            else {
                FMythicHarvestReceiptApplyPlan Plan;
                const EMythicHarvestReceiptPlanStatus Status =
                    Ledger->TryPlanApply(
                        Delivery.ProficiencyReceiptKey,
                        Delivery.ProficiencyReceiptPayloadFingerprint,
                        Target, FirstObservableSequence, Plan);
                if (Status == EMythicHarvestReceiptPlanStatus::Ready) {
                    UProficiencyDefinition *Definition =
                        ResolveProficiencyDefinition(Delivery);
                    UProficiencyComponent *Proficiencies =
                        Controller->GetProficiencyComponent();
                    const double XPUnits =
                        FMythicHarvestReceiptQuantity::ToUnits(
                            Plan.RemainingQuantity);
                    const bool bApplied = Definition && Proficiencies
                        && XPUnits <= static_cast<double>(MAX_flt)
                        && Proficiencies->TryGrantProficiencyXPWithContext(
                            Definition, static_cast<float>(XPUnits),
                            Delivery.ProficiencyContextTags);
                    if (bApplied) {
                        const bool bCommitted = Ledger->CommitPlannedApply(
                            Plan, Plan.RemainingQuantity);
                        if (!bCommitted) {
                            UE_LOG(Myth, Fatal,
                                   TEXT("Validated completion XP receipt commit failed after side effect."));
                            return Result;
                        }
                        ContributorsWithLiveMutation.Add(
                            Delivery.ContributorKey);
                    }
                    else {
                        Ledger->CancelPlannedApply(Plan);
                    }
                }
                else if (Status
                         == EMythicHarvestReceiptPlanStatus::AlreadyApplied) {
                    ContributorsWithLiveMutation.Add(
                        Delivery.ContributorKey);
                }
            }
        }

        if (!Delivery.bQuestCreditDelivered) {
            const int64 Target = Delivery.QuestCreditCount;
            if (Ledger->GetDurableAppliedQuantity(
                    Delivery.QuestReceiptKey) == Target) {
                Delivery.bQuestCreditDelivered = true;
            }
            else {
                FMythicHarvestReceiptApplyPlan Plan;
                const EMythicHarvestReceiptPlanStatus Status =
                    Ledger->TryPlanApply(
                        Delivery.QuestReceiptKey,
                        Delivery.QuestReceiptPayloadFingerprint,
                        Target, FirstObservableSequence, Plan);
                if (Status == EMythicHarvestReceiptPlanStatus::Ready) {
                    UMythicHarvestableDefinition *Definition =
                        ResolveHarvestableDefinition(Delivery);
                    UObjectiveTracker *Objectives =
                        Controller->GetObjectiveTracker();
                    EMythicHarvestQuestCreditConsumeResult ConsumeResult =
                        EMythicHarvestQuestCreditConsumeResult::Rejected;
                    if (Definition && Objectives) {
                        ConsumeResult =
                            Objectives->ConsumeHarvestCompletionCredit(
                                *Definition,
                                static_cast<int32>(Plan.RemainingQuantity));
                    }
                    if (ConsumeResult
                        != EMythicHarvestQuestCreditConsumeResult::Rejected) {
                        const EMythicHarvestQuestReceiptDisposition
                            Disposition = ConsumeResult
                                == EMythicHarvestQuestCreditConsumeResult::
                                    ConsumedMatched
                            ? EMythicHarvestQuestReceiptDisposition::Matched
                            : EMythicHarvestQuestReceiptDisposition::NoMatch;
                        const bool bCommitted = Ledger->CommitPlannedApply(
                            Plan, Plan.RemainingQuantity, Disposition);
                        if (!bCommitted) {
                            UE_LOG(Myth, Fatal,
                                   TEXT("Validated quest receipt commit failed after side effect."));
                            return Result;
                        }
                        ContributorsWithLiveMutation.Add(
                            Delivery.ContributorKey);
                    }
                    else {
                        Ledger->CancelPlannedApply(Plan);
                    }
                }
                else if (Status
                         == EMythicHarvestReceiptPlanStatus::AlreadyApplied) {
                    ContributorsWithLiveMutation.Add(
                        Delivery.ContributorKey);
                }
            }
        }

        if (Delivery.IsComplete()) {
            AdjustPendingContributorRows(Delivery.ContributorKey, -1);
            PendingCompletionDeliveries.RemoveAt(
                CompletionIndex, 1, EAllowShrinking::No);
            ++Result.CompletedCompletionDeliveryCount;
            CompletionIndex = AdvanceRetryRowCursor(
                CompletionIndex, PendingCompletionDeliveries.Num(), true);
            continue;
        }
        AdvanceCompletionIndex();
    }
    CompletionRetryRowCursor = PendingCompletionDeliveries.IsEmpty()
        ? 0 : CompletionIndex % PendingCompletionDeliveries.Num();

    const int32 WorkRowsAtStart = PendingWorkDeliveries.Num();
    int32 WorkRowsVisited = 0;
    int32 WorkIndex = WorkRowsAtStart > 0
        ? WorkRetryRowCursor % WorkRowsAtStart : 0;
    auto AdvanceWorkIndex = [this, &WorkIndex]() {
        WorkIndex = AdvanceRetryRowCursor(
            WorkIndex, PendingWorkDeliveries.Num(), false);
    };
    while (!PendingWorkDeliveries.IsEmpty()
           && WorkRowsVisited < WorkRowsAtStart
           && Result.AttemptedWorkDeliveryCount < WorkBudget) {
        FMythicPendingHarvestWorkDelivery &Delivery =
            PendingWorkDeliveries[WorkIndex];
        ++WorkRowsVisited;
        ++Result.AttemptedWorkDeliveryCount;
        if (IsContributorAwaitingCurrentCharacterSave(
                Delivery.ContributorKey)) {
            AdvanceWorkIndex();
            continue;
        }
        AMythicPlayerController *Controller =
            ResolveContributorController(
                Delivery.ContributorKey, Delivery.CurrentController);
        AMythicPlayerState *PlayerState =
            ResolveContributorPlayerState(Delivery.ContributorKey);
        UMythicHarvestReceiptLedgerComponent *Ledger =
            ResolveReceiptLedger(PlayerState);
        if (!Controller || !Controller->HasAuthority() || !PlayerState
            || !Ledger || FirstObservableSequence == 0) {
            AdvanceWorkIndex();
            continue;
        }
        TrackedReceiptOwners.Add(PlayerState);
        if (Ledger->GetDurableAppliedQuantity(Delivery.ReceiptKey)
            >= Delivery.ProficiencyXPQuanta) {
            AdjustPendingContributorRows(Delivery.ContributorKey, -1);
            PendingWorkDeliveries.RemoveAt(
                WorkIndex, 1, EAllowShrinking::No);
            ++Result.CompletedWorkDeliveryCount;
            WorkIndex = AdvanceRetryRowCursor(
                WorkIndex, PendingWorkDeliveries.Num(), true);
            continue;
        }

        FMythicHarvestReceiptApplyPlan Plan;
        const EMythicHarvestReceiptPlanStatus Status =
            Ledger->TryPlanApply(
                Delivery.ReceiptKey, Delivery.ReceiptPayloadFingerprint,
                Delivery.ProficiencyXPQuanta,
                FirstObservableSequence, Plan);
        if (Status == EMythicHarvestReceiptPlanStatus::Ready) {
            UProficiencyDefinition *Definition =
                ResolveProficiencyDefinition(Delivery);
            UProficiencyComponent *Proficiencies =
                Controller->GetProficiencyComponent();
            const double XPUnits = FMythicHarvestReceiptQuantity::ToUnits(
                Plan.RemainingQuantity);
            const bool bApplied = Definition && Proficiencies
                && XPUnits <= static_cast<double>(MAX_flt)
                && Proficiencies->TryGrantProficiencyXPWithContext(
                    Definition, static_cast<float>(XPUnits),
                    Delivery.WorkRewardContract.ContextTags);
            if (bApplied) {
                const bool bCommitted = Ledger->CommitPlannedApply(
                    Plan, Plan.RemainingQuantity);
                if (!bCommitted) {
                    UE_LOG(Myth, Fatal,
                           TEXT("Validated applied-work XP receipt commit failed after side effect."));
                    return Result;
                }
                ContributorsWithLiveMutation.Add(
                    Delivery.ContributorKey);
            }
            else {
                Ledger->CancelPlannedApply(Plan);
            }
        }
        else if (Status
                 == EMythicHarvestReceiptPlanStatus::AlreadyApplied) {
            ContributorsWithLiveMutation.Add(Delivery.ContributorKey);
        }
        AdvanceWorkIndex();
    }
    WorkRetryRowCursor = PendingWorkDeliveries.IsEmpty()
        ? 0 : WorkIndex % PendingWorkDeliveries.Num();

    const int32 DurabilityRowsAtStart =
        PendingDurabilityReceiptOrder.Num();
    int32 DurabilityRowsVisited = 0;
    int32 DurabilityIndex = DurabilityRowsAtStart > 0
        ? DurabilityRetryRowCursor % DurabilityRowsAtStart : 0;
    auto AdvanceDurabilityIndex = [this, &DurabilityIndex]() {
        DurabilityIndex = AdvanceRetryRowCursor(
            DurabilityIndex, PendingDurabilityReceiptOrder.Num(), false);
    };
    while (!PendingDurabilityReceiptOrder.IsEmpty()
           && DurabilityRowsVisited < DurabilityRowsAtStart
           && Result.AttemptedDurabilityCostCount
               < DurabilityBudget) {
        const FMythicHarvestReceiptKey ReceiptKey =
            PendingDurabilityReceiptOrder[DurabilityIndex];
        FMythicPendingHarvestDurabilityCost *Cost =
            DurabilityCostsByReceipt.Find(ReceiptKey);
        ++DurabilityRowsVisited;
        ++Result.AttemptedDurabilityCostCount;
        if (!Cost || !Cost->HasPendingApplication()) {
            UE_LOG(Myth, Fatal,
                   TEXT("Harvest durability pending index lost its authoritative cost row."));
            return Result;
        }
        if (IsContributorAwaitingCurrentCharacterSave(
                Cost->ContributorKey)) {
            AdvanceDurabilityIndex();
            continue;
        }

        AMythicPlayerController *Controller =
            ResolveContributorController(
                Cost->ContributorKey, Cost->CurrentController);
        AMythicPlayerState *PlayerState =
            ResolveContributorPlayerState(Cost->ContributorKey);
        UMythicHarvestReceiptLedgerComponent *Ledger =
            ResolveReceiptLedger(PlayerState);
        if (!Controller || !Controller->HasAuthority() || !PlayerState
            || !Ledger || FirstObservableSequence == 0) {
            AdvanceDurabilityIndex();
            continue;
        }
        TrackedReceiptOwners.Add(PlayerState);
        if (Ledger->GetDurableAppliedQuantity(ReceiptKey)
            >= Cost->CumulativeWearTarget) {
            const FString ContributorKey = Cost->ContributorKey;
            const FMythicHarvestRewardCompletionKey NodeGenerationKey =
                Cost->NodeGenerationKey;
            Cost->DurablyAppliedWearTarget =
                Cost->CumulativeWearTarget;
            RemovePendingDurabilityReceipt(ContributorKey, ReceiptKey);
            if (IsGenerationCompleted(NodeGenerationKey)) {
                RemoveDurabilityCostSeries(ReceiptKey);
            }
            ++Result.CompletedDurabilityCostCount;
            DurabilityIndex = PendingDurabilityReceiptOrder.IsEmpty()
                ? 0
                : DurabilityIndex
                    % PendingDurabilityReceiptOrder.Num();
            continue;
        }

        FMythicHarvestReceiptApplyPlan Plan;
        const EMythicHarvestReceiptPlanStatus Status =
            Ledger->TryPlanApply(
                ReceiptKey, Cost->ReceiptPayloadFingerprint,
                Cost->CumulativeWearTarget,
                FirstObservableSequence, Plan);
        if (Status == EMythicHarvestReceiptPlanStatus::Ready) {
            UMythicItemInstance *Tool =
                ResolveDurabilityTool(*Cost, *Controller);
            UDurabilityFragment *Durability = Tool
                ? const_cast<UDurabilityFragment *>(
                    Tool->GetFragment<UDurabilityFragment>())
                : nullptr;
            const bool bApplied = Durability
                && Durability->ServerConsumeReceiptWear(
                    Plan.RemainingQuantity);
            if (bApplied) {
                if (!Ledger->CommitPlannedApply(
                        Plan, Plan.RemainingQuantity)) {
                    UE_LOG(Myth, Fatal,
                           TEXT("Validated durability-cost receipt commit failed after item wear."));
                    return Result;
                }
                ContributorsWithLiveMutation.Add(Cost->ContributorKey);
            }
            else {
                Ledger->CancelPlannedApply(Plan);
            }
        }
        else if (Status
                 == EMythicHarvestReceiptPlanStatus::AlreadyApplied) {
            ContributorsWithLiveMutation.Add(Cost->ContributorKey);
        }
        AdvanceDurabilityIndex();
    }
    DurabilityRetryRowCursor = PendingDurabilityReceiptOrder.IsEmpty()
        ? 0 : DurabilityIndex % PendingDurabilityReceiptOrder.Num();

    const int32 ItemRowsAtStart = PendingDeliveries.Num();
    int32 ItemRowsVisited = 0;
    int32 ItemIndex = ItemRowsAtStart > 0
        ? ItemRetryRowCursor % ItemRowsAtStart : 0;
    auto AdvanceItemIndex = [this, &ItemIndex]() {
        ItemIndex = AdvanceRetryRowCursor(
            ItemIndex, PendingDeliveries.Num(), false);
    };
    while (Factory && !PendingDeliveries.IsEmpty()
           && ItemRowsVisited < ItemRowsAtStart
           && Result.AttemptedGrantCount < ItemBudget) {
        FMythicPendingHarvestRewardDelivery &Pending =
            PendingDeliveries[ItemIndex];
        ++ItemRowsVisited;
        ++Result.AttemptedGrantCount;
        if (IsContributorAwaitingCurrentCharacterSave(
                Pending.Grant.ContributorKey)) {
            AdvanceItemIndex();
            continue;
        }
        AMythicPlayerController *Controller =
            ResolveContributorController(Pending.Grant);
        AMythicPlayerState *PlayerState = ResolveContributorPlayerState(
            Pending.Grant.ContributorKey);
        UMythicHarvestReceiptLedgerComponent *Ledger =
            ResolveReceiptLedger(PlayerState);
        UMythicHarvestRewardEscrowComponent *Escrow =
            ResolveRewardEscrow(PlayerState);
        if (!Controller || !Controller->HasAuthority() || !PlayerState
            || Controller->GetPlayerState<AMythicPlayerState>()
                != PlayerState
            || !Ledger || !Escrow
            || FirstObservableSequence == 0) {
            AdvanceItemIndex();
            continue;
        }
        TrackedReceiptOwners.Add(PlayerState);

        const int64 DurableApplied = Ledger->GetDurableAppliedQuantity(
            Pending.Grant.ReceiptKey);
        if (DurableApplied > 0) {
            Pending.RemainingQuantity = Pending.Grant.Quantity
                - static_cast<int32>(FMath::Min<int64>(
                    DurableApplied, Pending.Grant.Quantity));
        }
        if (Pending.RemainingQuantity <= 0) {
            AdjustPendingContributorRows(
                Pending.Grant.ContributorKey, -1);
            AdjustPendingItemContributorRows(
                Pending.Grant.ContributorKey, -1);
            PendingDeliveries.RemoveAt(
                ItemIndex, 1, EAllowShrinking::No);
            ++Result.CompletedGrantCount;
            ItemIndex = AdvanceRetryRowCursor(
                ItemIndex, PendingDeliveries.Num(), true);
            continue;
        }

        FMythicHarvestReceiptApplyPlan Plan;
        const EMythicHarvestReceiptPlanStatus Status =
            Ledger->TryPlanApply(
                Pending.Grant.ReceiptKey,
                Pending.Grant.ReceiptPayloadFingerprint,
                Pending.Grant.Quantity, FirstObservableSequence, Plan);
        if (Status == EMythicHarvestReceiptPlanStatus::AlreadyApplied) {
            ContributorsWithLiveMutation.Add(
                Pending.Grant.ContributorKey);
            AdvanceItemIndex();
            continue;
        }
        if (Status != EMythicHarvestReceiptPlanStatus::Ready) {
            AdvanceItemIndex();
            continue;
        }

        const FMythicSavedHarvestItemEscrowRowV1 EscrowRow =
            BuildEscrowRow(Pending, FirstObservableSequence);
        const EMythicHarvestEscrowStageStatus StageStatus =
            Escrow->TryStage(EscrowRow);
        if (StageStatus
                == EMythicHarvestEscrowStageStatus::CapacityExceeded
            || StageStatus
                == EMythicHarvestEscrowStageStatus::RevisionExhausted) {
            Ledger->CancelPlannedApply(Plan);
            AdvanceItemIndex();
            continue;
        }
        if (StageStatus != EMythicHarvestEscrowStageStatus::Staged
            && StageStatus
                != EMythicHarvestEscrowStageStatus::AlreadyStaged) {
            Ledger->CancelPlannedApply(Plan);
            UE_LOG(Myth, Fatal,
                   TEXT("Harvest item escrow contract conflict for contributor %s."),
                   *Pending.Grant.ContributorKey);
            return Result;
        }
        if (!Ledger->CommitPlannedApply(
                Plan, Plan.RemainingQuantity)) {
            UE_LOG(Myth, Fatal,
                   TEXT("Validated item receipt commit failed after escrow custody admission."));
            return Result;
        }
        ContributorsWithLiveMutation.Add(Pending.Grant.ContributorKey);
        int64 DeliveredQuantity = 0;
        bool bEscrowRowCompleted = false;
        if (TryDeliverOneEscrowRow(
                *PlayerState, *Factory, DeliveredQuantity,
                bEscrowRowCompleted)) {
            ++Result.AttemptedEscrowDeliveryCount;
            Result.DeliveredQuantity += DeliveredQuantity;
            if (bEscrowRowCompleted) {
                ++Result.CompletedEscrowDeliveryCount;
            }
        }
        AdvanceItemIndex();
    }
    ItemRetryRowCursor = PendingDeliveries.IsEmpty()
        ? 0 : ItemIndex % PendingDeliveries.Num();

    for (const FString &ContributorKey : ContributorsWithLiveMutation) {
        AMythicPlayerState *PlayerState =
            ResolveContributorPlayerState(ContributorKey);
        if (PlayerState
            && RequestContributorDurabilitySave(
                ContributorKey, *PlayerState)) {
            ++Result.CharacterSaveRequestCount;
        }
    }

    for (auto It = TrackedReceiptOwners.CreateIterator(); It; ++It) {
        AMythicPlayerState *PlayerState = It->Get();
        if (!PlayerState) {
            It.RemoveCurrent();
            continue;
        }
        UMythicHarvestReceiptLedgerComponent *Ledger =
            ResolveReceiptLedger(PlayerState);
        const FString &ContributorKey =
            PlayerState->GetPersistentCharacterId();
        if (Ledger && Ledger->HasUndurableMutation()
            && !IsContributorAwaitingCurrentCharacterSave(ContributorKey)
            && RequestContributorDurabilitySave(
                ContributorKey, *PlayerState)) {
            ++Result.CharacterSaveRequestCount;
        }
    }

    Result.PendingGrantCount = PendingDeliveries.Num();
    Result.PendingCompletionDeliveryCount =
        PendingCompletionDeliveries.Num();
    Result.PendingAppliedWorkDeliveryCount = PendingWorkDeliveries.Num();
    return Result;
}

bool UMythicHarvestRewardOutboxSubsystem::BuildSaveSnapshotFingerprint(
    const FMythicHarvestRewardOutboxSaveV1 &Snapshot,
    FSHA256Signature &OutFingerprint) {
    OutFingerprint = FSHA256Signature{};
    FMythicHarvestRewardOutboxSaveV1 Canonical = Snapshot;
    Canonical.SortCanonical();
    FBufferArchive Payload;
    FObjectAndNameAsStringProxyArchive Archive(Payload, false);
    Archive.ArIsSaveGame = true;
    FMythicHarvestRewardOutboxSaveV1::StaticStruct()->SerializeItem(
        Archive, &Canonical, nullptr);
    return !Archive.IsError() && !Payload.IsEmpty()
        && MythicItemizationHash::Sha256(
            MakeArrayView(Payload), OutFingerprint);
}

bool UMythicHarvestRewardOutboxSubsystem::
TryGetRestoredSaveSnapshotFingerprint(
    FSHA256Signature &OutFingerprint) const {
    OutFingerprint = FSHA256Signature{};
    if (!bHasRestoredSaveSnapshotFingerprint) {
        return false;
    }
    OutFingerprint = RestoredSaveSnapshotFingerprint;
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::MatchesRestoredSaveSnapshot(
    const FMythicHarvestRewardOutboxSaveV1 &Snapshot) const {
    FSHA256Signature Candidate{};
    return bHasRestoredSaveSnapshotFingerprint
        && BuildSaveSnapshotFingerprint(Snapshot, Candidate)
        && FMemory::Memcmp(
               Candidate.Signature,
               RestoredSaveSnapshotFingerprint.Signature,
               sizeof(Candidate.Signature)) == 0;
}

bool UMythicHarvestRewardOutboxSubsystem::BuildSaveSnapshot(
    const FGuid &WorldEpoch,
    FMythicHarvestRewardOutboxSaveV1 &OutSnapshot,
    FName &OutDiagnosticCode) {
    OutSnapshot = FMythicHarvestRewardOutboxSaveV1();
    if (!WorldEpoch.IsValid()) {
        OutDiagnosticCode = TEXT("InvalidRewardOutboxEpoch");
        return false;
    }
    if (LastIssuedWorldSnapshotSequence == MAX_uint64) {
        OutDiagnosticCode = TEXT("RewardOutboxSequenceExhausted");
        return false;
    }
    OutSnapshot.WorldEpoch = WorldEpoch;
    OutSnapshot.SnapshotSequence =
        LastIssuedWorldSnapshotSequence + 1;
    OutSnapshot.RetryQueueCursor = RetryQueueCursor;
    OutSnapshot.CompletionRetryRowCursor = CompletionRetryRowCursor;
    OutSnapshot.ItemRetryRowCursor = ItemRetryRowCursor;
    OutSnapshot.WorkRetryRowCursor = WorkRetryRowCursor;
    OutSnapshot.DurabilityRetryRowCursor =
        DurabilityRetryRowCursor;

    OutSnapshot.KnownCompletions.Reserve(KnownCompletions.Num());
    for (const FMythicHarvestRewardCompletionKey &Completion :
         KnownCompletions) {
        FMythicSavedHarvestRewardCompletionV1 &Saved =
            OutSnapshot.KnownCompletions.AddDefaulted_GetRef();
        Saved.WorldEpoch = Completion.WorldEpoch;
        Saved.NodeGuid = Completion.NodeId.GetGuid();
        Saved.Generation = Completion.Generation;
    }
    OutSnapshot.KnownCompletions.Sort([](
        const FMythicSavedHarvestRewardCompletionV1 &Left,
        const FMythicSavedHarvestRewardCompletionV1 &Right) {
        const FMythicHarvestRewardCompletionKey LeftKey{
            Left.WorldEpoch, FMythicHarvestNodeId(Left.NodeGuid),
            Left.Generation};
        const FMythicHarvestRewardCompletionKey RightKey{
            Right.WorldEpoch, FMythicHarvestNodeId(Right.NodeGuid),
            Right.Generation};
        return MHRewardOutboxPrivate::CompletionKeyLess(
            LeftKey, RightKey);
    });

    OutSnapshot.GenerationHighWatermarks.Reserve(
        HighestKnownGenerationByNode.Num());
    for (const TPair<FMythicHarvestEpochNodeKey, uint32> &Pair :
         HighestKnownGenerationByNode) {
        FMythicSavedHarvestGenerationHighWaterV1 &Saved =
            OutSnapshot.GenerationHighWatermarks.AddDefaulted_GetRef();
        Saved.WorldEpoch = Pair.Key.WorldEpoch;
        Saved.NodeGuid = Pair.Key.NodeId.GetGuid();
        Saved.HighestKnownGeneration = Pair.Value;
    }
    OutSnapshot.GenerationHighWatermarks.Sort([](
        const FMythicSavedHarvestGenerationHighWaterV1 &Left,
        const FMythicSavedHarvestGenerationHighWaterV1 &Right) {
        if (Left.WorldEpoch != Right.WorldEpoch) {
            return MHRewardOutboxPrivate::GuidLess(
                Left.WorldEpoch, Right.WorldEpoch);
        }
        return MHRewardOutboxPrivate::GuidLess(
            Left.NodeGuid, Right.NodeGuid);
    });

    OutSnapshot.PendingGrants.Reserve(PendingDeliveries.Num());
    for (const FMythicPendingHarvestRewardDelivery &Pending :
         PendingDeliveries) {
        FMythicSavedHarvestRewardGrantV1 &Saved =
            OutSnapshot.PendingGrants.AddDefaulted_GetRef();
        Saved.WorldEpoch = Pending.Grant.CompletionKey.WorldEpoch;
        Saved.NodeGuid = Pending.Grant.CompletionKey.NodeId.GetGuid();
        Saved.Generation = Pending.Grant.CompletionKey.Generation;
        Saved.Channel = Pending.Grant.Channel;
        Saved.RewardRowIndex = Pending.Grant.RewardRowIndex;
        Saved.ContributorKey = Pending.Grant.ContributorKey;
        Saved.ItemDefinitionId = Pending.Grant.ItemDefinitionId;
        Saved.OriginalQuantity = Pending.Grant.Quantity;
        Saved.RemainingQuantity = Pending.RemainingQuantity;
        Saved.ItemLevel = Pending.Grant.ItemLevel;
        Saved.bHasResolvedQuality = Pending.Grant.bHasResolvedQuality;
        Saved.ResolvedQuality = Pending.Grant.ResolvedQuality;
        Saved.ItemSeed = Pending.Grant.ItemSeed;
        Saved.ReceiptKey = Pending.Grant.ReceiptKey;
        Saved.ReceiptPayloadFingerprint =
            Pending.Grant.ReceiptPayloadFingerprint;
    }

    OutSnapshot.PendingCompletionDeliveries.Reserve(
        PendingCompletionDeliveries.Num());
    for (const FMythicPendingHarvestCompletionDelivery &Pending :
         PendingCompletionDeliveries) {
        FMythicSavedHarvestCompletionDeliveryV1 &Saved =
            OutSnapshot.PendingCompletionDeliveries.
                AddDefaulted_GetRef();
        Saved.WorldEpoch = Pending.CompletionKey.WorldEpoch;
        Saved.NodeGuid = Pending.CompletionKey.NodeId.GetGuid();
        Saved.Generation = Pending.CompletionKey.Generation;
        Saved.ContributorKey = Pending.ContributorKey;
        Saved.ProficiencyDefinitionId =
            Pending.ProficiencyDefinitionId;
        Saved.HarvestableDefinitionId =
            Pending.HarvestableDefinitionId;
        Saved.CompletionProficiencyXPQuanta =
            Pending.CompletionProficiencyXPQuanta;
        Saved.ProficiencyContextTags =
            Pending.ProficiencyContextTags;
        Saved.QuestCreditCount = Pending.QuestCreditCount;
        Saved.bProficiencyDelivered = Pending.bProficiencyDelivered;
        Saved.bQuestCreditDelivered = Pending.bQuestCreditDelivered;
        Saved.ProficiencyReceiptKey = Pending.ProficiencyReceiptKey;
        Saved.ProficiencyReceiptPayloadFingerprint =
            Pending.ProficiencyReceiptPayloadFingerprint;
        Saved.QuestReceiptKey = Pending.QuestReceiptKey;
        Saved.QuestReceiptPayloadFingerprint =
            Pending.QuestReceiptPayloadFingerprint;
    }

    OutSnapshot.PendingWorkDeliveries.Reserve(
        PendingWorkDeliveries.Num());
    for (const FMythicPendingHarvestWorkDelivery &Pending :
         PendingWorkDeliveries) {
        FMythicSavedHarvestWorkDeliveryV1 &Saved =
            OutSnapshot.PendingWorkDeliveries.AddDefaulted_GetRef();
        Saved.WorldEpoch = Pending.NodeGenerationKey.WorldEpoch;
        Saved.NodeGuid = Pending.NodeGenerationKey.NodeId.GetGuid();
        Saved.Generation = Pending.NodeGenerationKey.Generation;
        Saved.ContributorKey = Pending.ContributorKey;
        Saved.WorkRewardContract = Pending.WorkRewardContract;
        Saved.CumulativeAppliedWorkQuanta =
            Pending.CumulativeAppliedWorkQuanta;
        Saved.ProficiencyXPQuanta = Pending.ProficiencyXPQuanta;
        Saved.ReceiptKey = Pending.ReceiptKey;
        Saved.ReceiptPayloadFingerprint =
            Pending.ReceiptPayloadFingerprint;
    }

    OutSnapshot.DurabilityCosts.Reserve(
        DurabilityCostsByReceipt.Num());
    for (const TPair<FMythicHarvestReceiptKey,
                     FMythicPendingHarvestDurabilityCost> &Pair :
         DurabilityCostsByReceipt) {
        const FMythicPendingHarvestDurabilityCost &Cost = Pair.Value;
        FMythicSavedHarvestDurabilityCostV1 &Saved =
            OutSnapshot.DurabilityCosts.AddDefaulted_GetRef();
        Saved.WorldEpoch = Cost.NodeGenerationKey.WorldEpoch;
        Saved.NodeGuid = Cost.NodeGenerationKey.NodeId.GetGuid();
        Saved.Generation = Cost.NodeGenerationKey.Generation;
        Saved.ContributorKey = Cost.ContributorKey;
        Saved.ToolItemInstanceGuid = Cost.ToolItemInstanceGuid;
        Saved.CumulativeWearTarget = Cost.CumulativeWearTarget;
        Saved.DurablyAppliedWearTarget =
            Cost.DurablyAppliedWearTarget;
        Saved.ReceiptKey = Cost.ReceiptKey;
        Saved.ReceiptPayloadFingerprint =
            Cost.ReceiptPayloadFingerprint;
    }

    OutSnapshot.ContributorLedgerFences.Reserve(
        ContributorLedgerFenceByKey.Num());
    for (const TPair<
             FString,
             FMythicSavedHarvestContributorLedgerFenceV1> &Pair :
         ContributorLedgerFenceByKey) {
        OutSnapshot.ContributorLedgerFences.Add(Pair.Value);
    }

    OutSnapshot.PendingGrants.Sort([](
        const FMythicSavedHarvestRewardGrantV1 &Left,
        const FMythicSavedHarvestRewardGrantV1 &Right) {
        if (Left.ReceiptKey == Right.ReceiptKey) {
            return Left.ContributorKey.Compare(
                       Right.ContributorKey,
                       ESearchCase::CaseSensitive) < 0;
        }
        return MHRewardOutboxPrivate::ReceiptKeyLess(
            Left.ReceiptKey, Right.ReceiptKey);
    });
    OutSnapshot.PendingCompletionDeliveries.Sort([](
        const FMythicSavedHarvestCompletionDeliveryV1 &Left,
        const FMythicSavedHarvestCompletionDeliveryV1 &Right) {
        const FMythicHarvestRewardCompletionKey LeftKey{
            Left.WorldEpoch, FMythicHarvestNodeId(Left.NodeGuid),
            Left.Generation};
        const FMythicHarvestRewardCompletionKey RightKey{
            Right.WorldEpoch, FMythicHarvestNodeId(Right.NodeGuid),
            Right.Generation};
        if (!(LeftKey == RightKey)) {
            return MHRewardOutboxPrivate::CompletionKeyLess(
                LeftKey, RightKey);
        }
        return Left.ContributorKey.Compare(
                   Right.ContributorKey,
                   ESearchCase::CaseSensitive) < 0;
    });
    OutSnapshot.PendingWorkDeliveries.Sort([](
        const FMythicSavedHarvestWorkDeliveryV1 &Left,
        const FMythicSavedHarvestWorkDeliveryV1 &Right) {
        if (Left.ReceiptKey == Right.ReceiptKey) {
            return Left.ContributorKey.Compare(
                       Right.ContributorKey,
                       ESearchCase::CaseSensitive) < 0;
        }
        return MHRewardOutboxPrivate::ReceiptKeyLess(
            Left.ReceiptKey, Right.ReceiptKey);
    });

    OutSnapshot.SortCanonical();
    if (!ValidateSaveSnapshot(OutSnapshot, OutDiagnosticCode)) {
        OutSnapshot = FMythicHarvestRewardOutboxSaveV1();
        return false;
    }
    LastIssuedWorldSnapshotSequence = OutSnapshot.SnapshotSequence;
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::RestoreSaveSnapshot(
    const FMythicHarvestRewardOutboxSaveV1 &Snapshot,
    FName &OutDiagnosticCode) {
    if (!ValidateSaveSnapshot(Snapshot, OutDiagnosticCode)) {
        return false;
    }
    if (RestoreDomainEpoch == MAX_uint64) {
        OutDiagnosticCode = TEXT("HarvestOutboxRestoreDomainExhausted");
        return false;
    }
    FSHA256Signature CandidateFingerprint{};
    if (!BuildSaveSnapshotFingerprint(Snapshot, CandidateFingerprint)) {
        OutDiagnosticCode = TEXT("RewardOutboxSnapshotFingerprintFailed");
        return false;
    }

    TSet<FMythicHarvestRewardCompletionKey> RestoredCompletions;
    RestoredCompletions.Reserve(Snapshot.KnownCompletions.Num());
    for (const FMythicSavedHarvestRewardCompletionV1 &Saved :
         Snapshot.KnownCompletions) {
        RestoredCompletions.Add({
            Saved.WorldEpoch, FMythicHarvestNodeId(Saved.NodeGuid),
            Saved.Generation});
    }
    TMap<FMythicHarvestEpochNodeKey, uint32> RestoredHighWater;
    RestoredHighWater.Reserve(
        Snapshot.GenerationHighWatermarks.Num());
    for (const FMythicSavedHarvestGenerationHighWaterV1 &Saved :
         Snapshot.GenerationHighWatermarks) {
        RestoredHighWater.Add(
            {Saved.WorldEpoch, FMythicHarvestNodeId(Saved.NodeGuid)},
            Saved.HighestKnownGeneration);
    }

    TArray<FMythicPendingHarvestRewardDelivery> RestoredGrants;
    RestoredGrants.Reserve(Snapshot.PendingGrants.Num());
    for (const FMythicSavedHarvestRewardGrantV1 &Saved :
         Snapshot.PendingGrants) {
        FMythicPendingHarvestRewardDelivery &Pending =
            RestoredGrants.AddDefaulted_GetRef();
        Pending.Grant.CompletionKey = {
            Saved.WorldEpoch, FMythicHarvestNodeId(Saved.NodeGuid),
            Saved.Generation};
        Pending.Grant.Channel = Saved.Channel;
        Pending.Grant.RewardRowIndex = Saved.RewardRowIndex;
        Pending.Grant.ContributorKey = Saved.ContributorKey;
        Pending.Grant.ItemDefinitionId = Saved.ItemDefinitionId;
        Pending.Grant.Quantity = Saved.OriginalQuantity;
        Pending.Grant.ItemLevel = Saved.ItemLevel;
        Pending.Grant.bHasResolvedQuality = Saved.bHasResolvedQuality;
        Pending.Grant.ResolvedQuality = Saved.ResolvedQuality;
        Pending.Grant.ItemSeed = Saved.ItemSeed;
        Pending.Grant.ReceiptKey = Saved.ReceiptKey;
        Pending.Grant.ReceiptPayloadFingerprint =
            Saved.ReceiptPayloadFingerprint;
        Pending.RemainingQuantity = Saved.RemainingQuantity;
        ResolveItemDefinition(Pending.Grant);
    }
    SortPendingDeliveries(RestoredGrants);

    TArray<FMythicPendingHarvestCompletionDelivery>
        RestoredCompletionDeliveries;
    RestoredCompletionDeliveries.Reserve(
        Snapshot.PendingCompletionDeliveries.Num());
    for (const FMythicSavedHarvestCompletionDeliveryV1 &Saved :
         Snapshot.PendingCompletionDeliveries) {
        FMythicPendingHarvestCompletionDelivery &Pending =
            RestoredCompletionDeliveries.AddDefaulted_GetRef();
        Pending.CompletionKey = {
            Saved.WorldEpoch, FMythicHarvestNodeId(Saved.NodeGuid),
            Saved.Generation};
        Pending.ContributorKey = Saved.ContributorKey;
        Pending.ProficiencyDefinitionId =
            Saved.ProficiencyDefinitionId;
        Pending.HarvestableDefinitionId =
            Saved.HarvestableDefinitionId;
        Pending.CompletionProficiencyXPQuanta =
            Saved.CompletionProficiencyXPQuanta;
        Pending.ProficiencyContextTags =
            Saved.ProficiencyContextTags;
        Pending.QuestCreditCount = Saved.QuestCreditCount;
        Pending.bProficiencyDelivered = Saved.bProficiencyDelivered;
        Pending.bQuestCreditDelivered = Saved.bQuestCreditDelivered;
        Pending.ProficiencyReceiptKey = Saved.ProficiencyReceiptKey;
        Pending.ProficiencyReceiptPayloadFingerprint =
            Saved.ProficiencyReceiptPayloadFingerprint;
        Pending.QuestReceiptKey = Saved.QuestReceiptKey;
        Pending.QuestReceiptPayloadFingerprint =
            Saved.QuestReceiptPayloadFingerprint;
    }
    SortPendingCompletionDeliveries(RestoredCompletionDeliveries);

    TArray<FMythicPendingHarvestWorkDelivery> RestoredWorkDeliveries;
    RestoredWorkDeliveries.Reserve(
        Snapshot.PendingWorkDeliveries.Num());
    for (const FMythicSavedHarvestWorkDeliveryV1 &Saved :
         Snapshot.PendingWorkDeliveries) {
        FMythicPendingHarvestWorkDelivery &Pending =
            RestoredWorkDeliveries.AddDefaulted_GetRef();
        Pending.NodeGenerationKey = {
            Saved.WorldEpoch, FMythicHarvestNodeId(Saved.NodeGuid),
            Saved.Generation};
        Pending.ContributorKey = Saved.ContributorKey;
        Pending.WorkRewardContract = Saved.WorkRewardContract;
        Pending.CumulativeAppliedWorkQuanta =
            Saved.CumulativeAppliedWorkQuanta;
        Pending.ProficiencyXPQuanta = Saved.ProficiencyXPQuanta;
        Pending.ReceiptKey = Saved.ReceiptKey;
        Pending.ReceiptPayloadFingerprint =
            Saved.ReceiptPayloadFingerprint;
    }
    SortPendingWorkDeliveries(RestoredWorkDeliveries);

    TMap<FMythicHarvestReceiptKey, FMythicPendingHarvestDurabilityCost>
        RestoredDurabilityCosts;
    TArray<FMythicHarvestReceiptKey> RestoredPendingDurabilityOrder;
    TMultiMap<FString, FMythicHarvestReceiptKey>
        RestoredPendingDurabilityByContributor;
    TMultiMap<FMythicHarvestRewardCompletionKey,
              FMythicHarvestReceiptKey>
        RestoredDurabilityByNodeGeneration;
    TMap<FString, int32> RestoredDurabilitySeriesCounts;
    RestoredDurabilityCosts.Reserve(Snapshot.DurabilityCosts.Num());
    for (const FMythicSavedHarvestDurabilityCostV1 &Saved :
         Snapshot.DurabilityCosts) {
        FMythicPendingHarvestDurabilityCost Cost;
        Cost.NodeGenerationKey = {
            Saved.WorldEpoch, FMythicHarvestNodeId(Saved.NodeGuid),
            Saved.Generation};
        Cost.ContributorKey = Saved.ContributorKey;
        Cost.ToolItemInstanceGuid = Saved.ToolItemInstanceGuid;
        Cost.CumulativeWearTarget = Saved.CumulativeWearTarget;
        Cost.DurablyAppliedWearTarget =
            Saved.DurablyAppliedWearTarget;
        Cost.ReceiptKey = Saved.ReceiptKey;
        Cost.ReceiptPayloadFingerprint =
            Saved.ReceiptPayloadFingerprint;
        RestoredDurabilityByNodeGeneration.Add(
            Cost.NodeGenerationKey, Cost.ReceiptKey);
        ++RestoredDurabilitySeriesCounts.FindOrAdd(
            Cost.ContributorKey);
        if (Cost.HasPendingApplication()) {
            RestoredPendingDurabilityOrder.Add(Cost.ReceiptKey);
            RestoredPendingDurabilityByContributor.Add(
                Cost.ContributorKey, Cost.ReceiptKey);
        }
        RestoredDurabilityCosts.Add(Cost.ReceiptKey, MoveTemp(Cost));
    }
    RestoredPendingDurabilityOrder.Sort([](
        const FMythicHarvestReceiptKey &Left,
        const FMythicHarvestReceiptKey &Right) {
        return MHRewardOutboxPrivate::ReceiptKeyLess(Left, Right);
    });

    TMap<FString, FMythicSavedHarvestContributorLedgerFenceV1>
        RestoredContributorFences;
    RestoredContributorFences.Reserve(
        Snapshot.ContributorLedgerFences.Num());
    for (const FMythicSavedHarvestContributorLedgerFenceV1 &Fence :
         Snapshot.ContributorLedgerFences) {
        RestoredContributorFences.Add(Fence.ContributorKey, Fence);
    }

    TMap<FString, int32> RestoredPendingContributorCounts;
    for (const FMythicPendingHarvestRewardDelivery &Pending :
         RestoredGrants) {
        ++RestoredPendingContributorCounts.FindOrAdd(
            Pending.Grant.ContributorKey);
    }
    for (const FMythicPendingHarvestCompletionDelivery &Pending :
         RestoredCompletionDeliveries) {
        ++RestoredPendingContributorCounts.FindOrAdd(
            Pending.ContributorKey);
    }
    for (const FMythicPendingHarvestWorkDelivery &Pending :
         RestoredWorkDeliveries) {
        ++RestoredPendingContributorCounts.FindOrAdd(
            Pending.ContributorKey);
    }

    KnownCompletions = MoveTemp(RestoredCompletions);
    HighestKnownGenerationByNode = MoveTemp(RestoredHighWater);
    PendingDeliveries = MoveTemp(RestoredGrants);
    PendingCompletionDeliveries =
        MoveTemp(RestoredCompletionDeliveries);
    PendingWorkDeliveries = MoveTemp(RestoredWorkDeliveries);
    DurabilityCostsByReceipt = MoveTemp(RestoredDurabilityCosts);
    PendingDurabilityReceiptOrder =
        MoveTemp(RestoredPendingDurabilityOrder);
    PendingDurabilityReceiptsByContributor =
        MoveTemp(RestoredPendingDurabilityByContributor);
    DurabilityReceiptsByNodeGeneration =
        MoveTemp(RestoredDurabilityByNodeGeneration);
    DurabilitySeriesCountByContributor =
        MoveTemp(RestoredDurabilitySeriesCounts);
    ContributorLedgerFenceByKey =
        MoveTemp(RestoredContributorFences);
    PendingRowCountByContributor =
        MoveTemp(RestoredPendingContributorCounts);
    ++RestoreDomainEpoch;
    TrackedReceiptOwners.Reset();
    InFlightDefinitionLoads.Reset();
    DefinitionLoadHandles.Reset();
    LastIssuedWorldSnapshotSequence = Snapshot.SnapshotSequence;
    RetryQueueCursor = Snapshot.RetryQueueCursor;
    CompletionRetryRowCursor = Snapshot.CompletionRetryRowCursor;
    ItemRetryRowCursor = Snapshot.ItemRetryRowCursor;
    WorkRetryRowCursor = Snapshot.WorkRetryRowCursor;
    DurabilityRetryRowCursor = Snapshot.DurabilityRetryRowCursor;
    LastDurableWorldSnapshotSequence = Snapshot.SnapshotSequence;
    LastDurableWorldEpoch = Snapshot.WorldEpoch;
    LastDurablePendingReceiptKeys.Reset();
    LastDurableCompletedGenerationByNode.Reset();
    for (const FMythicSavedHarvestGenerationHighWaterV1 &Saved :
         Snapshot.GenerationHighWatermarks) {
        LastDurableCompletedGenerationByNode.Add(
            FMythicHarvestNodeId(Saved.NodeGuid),
            Saved.HighestKnownGeneration);
    }
    for (const FMythicSavedHarvestRewardGrantV1 &Saved :
         Snapshot.PendingGrants) {
        LastDurablePendingReceiptKeys.Add(Saved.ReceiptKey);
    }
    for (const FMythicSavedHarvestCompletionDeliveryV1 &Saved :
         Snapshot.PendingCompletionDeliveries) {
        if (!Saved.bProficiencyDelivered) {
            LastDurablePendingReceiptKeys.Add(
                Saved.ProficiencyReceiptKey);
        }
        if (!Saved.bQuestCreditDelivered) {
            LastDurablePendingReceiptKeys.Add(Saved.QuestReceiptKey);
        }
    }
    for (const FMythicSavedHarvestWorkDeliveryV1 &Saved :
         Snapshot.PendingWorkDeliveries) {
        LastDurablePendingReceiptKeys.Add(Saved.ReceiptKey);
    }
    for (const FMythicSavedHarvestDurabilityCostV1 &Saved :
         Snapshot.DurabilityCosts) {
        if (Saved.DurablyAppliedWearTarget
            < Saved.CumulativeWearTarget) {
            LastDurablePendingReceiptKeys.Add(Saved.ReceiptKey);
        }
    }
    RefreshLoadedDefinitions();
    RequestPendingDefinitionLoads();
    RestoredSaveSnapshotFingerprint = CandidateFingerprint;
    bHasRestoredSaveSnapshotFingerprint = true;
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
    const FMythicHarvestRewardOutboxSaveV1 &Snapshot,
    FName &OutDiagnosticCode) {
    if (Snapshot.SchemaVersion
            != FMythicHarvestRewardOutboxSaveV1::CurrentSchemaVersion
        || !Snapshot.WorldEpoch.IsValid()
        || Snapshot.SnapshotSequence == 0
        || Snapshot.RetryQueueCursor >= 5
        || Snapshot.CompletionRetryRowCursor < 0
        || Snapshot.ItemRetryRowCursor < 0
        || Snapshot.WorkRetryRowCursor < 0
        || Snapshot.DurabilityRetryRowCursor < 0
        || Snapshot.CompletionRetryRowCursor
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumPendingDeliveries
        || Snapshot.ItemRetryRowCursor
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumPendingDeliveries
        || Snapshot.WorkRetryRowCursor
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumPendingDeliveries
        || Snapshot.DurabilityRetryRowCursor
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumDurabilityCostSeries) {
        OutDiagnosticCode = TEXT("InvalidRewardOutboxHeader");
        return false;
    }
    if (Snapshot.KnownCompletions.Num()
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumKnownCompletions
        || Snapshot.GenerationHighWatermarks.Num()
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumKnownCompletions
        || static_cast<int64>(Snapshot.PendingGrants.Num())
                + Snapshot.PendingCompletionDeliveries.Num()
                + Snapshot.PendingWorkDeliveries.Num()
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumPendingDeliveries
        || Snapshot.DurabilityCosts.Num()
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumDurabilityCostSeries
        || Snapshot.ContributorLedgerFences.Num()
            > FMythicHarvestRewardOutboxSaveV1::
                AbsoluteMaximumContributorLedgerFences) {
        OutDiagnosticCode = TEXT("RewardOutboxCapacityExceeded");
        return false;
    }

    TMap<FString, int32> PendingRowsByContributor;
    auto TrackContributorRow = [&PendingRowsByContributor](
        const FString &ContributorKey) {
        if (ContributorKey.IsEmpty()) {
            return false;
        }
        int32 &Count = PendingRowsByContributor.FindOrAdd(ContributorKey);
        ++Count;
        return !UMythicHarvestRewardOutboxSubsystem::
            WouldExceedPerContributorPendingCapacity(Count, 0);
    };
    for (const FMythicSavedHarvestRewardGrantV1 &Grant :
         Snapshot.PendingGrants) {
        if (!TrackContributorRow(Grant.ContributorKey)) {
            OutDiagnosticCode = TEXT("RewardContributorCapacityExceeded");
            return false;
        }
    }
    for (const FMythicSavedHarvestCompletionDeliveryV1 &Delivery :
         Snapshot.PendingCompletionDeliveries) {
        if (!TrackContributorRow(Delivery.ContributorKey)) {
            OutDiagnosticCode = TEXT("RewardContributorCapacityExceeded");
            return false;
        }
    }
    for (const FMythicSavedHarvestWorkDeliveryV1 &Delivery :
         Snapshot.PendingWorkDeliveries) {
        if (!TrackContributorRow(Delivery.ContributorKey)) {
            OutDiagnosticCode = TEXT("RewardContributorCapacityExceeded");
            return false;
        }
    }

    TMap<FString, FMythicSavedHarvestContributorLedgerFenceV1>
        LedgerFencesByContributor;
    LedgerFencesByContributor.Reserve(
        Snapshot.ContributorLedgerFences.Num());
    for (const FMythicSavedHarvestContributorLedgerFenceV1 &Fence :
         Snapshot.ContributorLedgerFences) {
        if (Fence.ContributorKey.IsEmpty()
            || !Fence.LedgerEpoch.IsValid()
            || Fence.MinimumLedgerRevision == 0
            || LedgerFencesByContributor.Contains(
                Fence.ContributorKey)) {
            OutDiagnosticCode =
                TEXT("InvalidHarvestContributorLedgerFence");
            return false;
        }
        LedgerFencesByContributor.Add(Fence.ContributorKey, Fence);
    }

    TMap<FMythicHarvestEpochNodeKey, uint32> HighWaterByNode;
    HighWaterByNode.Reserve(
        Snapshot.GenerationHighWatermarks.Num());
    for (const FMythicSavedHarvestGenerationHighWaterV1 &Saved :
         Snapshot.GenerationHighWatermarks) {
        const FMythicHarvestEpochNodeKey Key{
            Saved.WorldEpoch, FMythicHarvestNodeId(Saved.NodeGuid)};
        if (!Key.IsValid() || Saved.WorldEpoch != Snapshot.WorldEpoch
            || Saved.HighestKnownGeneration == 0
            || HighWaterByNode.Contains(Key)) {
            OutDiagnosticCode = TEXT("InvalidHarvestGenerationHighWater");
            return false;
        }
        HighWaterByNode.Add(Key, Saved.HighestKnownGeneration);
    }

    TSet<FMythicHarvestEpochNodeKey> WitnessedNodes;
    WitnessedNodes.Reserve(Snapshot.KnownCompletions.Num());
    for (const FMythicSavedHarvestRewardCompletionV1 &Saved :
         Snapshot.KnownCompletions) {
        const FMythicHarvestRewardCompletionKey Key{
            Saved.WorldEpoch, FMythicHarvestNodeId(Saved.NodeGuid),
            Saved.Generation};
        const FMythicHarvestEpochNodeKey EpochNode{
            Saved.WorldEpoch, FMythicHarvestNodeId(Saved.NodeGuid)};
        const uint32 *HighWater = HighWaterByNode.Find(EpochNode);
        if (!Key.IsValid() || Saved.WorldEpoch != Snapshot.WorldEpoch
            || !HighWater || WitnessedNodes.Contains(EpochNode)) {
            OutDiagnosticCode = TEXT("InvalidSavedRewardCompletion");
            return false;
        }
        if (Saved.Generation != *HighWater) {
            OutDiagnosticCode = TEXT("HarvestGenerationHighWaterMismatch");
            return false;
        }
        WitnessedNodes.Add(EpochNode);
    }
    if (WitnessedNodes.Num() != HighWaterByNode.Num()) {
        OutDiagnosticCode = TEXT("UnbackedHarvestGenerationHighWater");
        return false;
    }
    const auto IsAdmittedCompletion = [&HighWaterByNode](
        const FMythicHarvestRewardCompletionKey &CompletionKey) {
        const FMythicHarvestEpochNodeKey EpochNode{
            CompletionKey.WorldEpoch, CompletionKey.NodeId};
        const uint32 *HighWater = HighWaterByNode.Find(EpochNode);
        return HighWater && CompletionKey.Generation <= *HighWater;
    };

    TMap<FString, TSet<FMythicHarvestReceiptKey>>
        SeenContributorReceipts;
    for (const FMythicSavedHarvestRewardGrantV1 &Grant :
         Snapshot.PendingGrants) {
        const FMythicHarvestRewardCompletionKey CompletionKey{
            Grant.WorldEpoch, FMythicHarvestNodeId(Grant.NodeGuid),
            Grant.Generation};
        if (!CompletionKey.IsValid()
            || Grant.WorldEpoch != Snapshot.WorldEpoch
            || !IsAdmittedCompletion(CompletionKey)
            || !MHRewardOutboxPrivate::IsValidRewardChannel(
                Grant.Channel)
            || Grant.RewardRowIndex < 0
            || Grant.ContributorKey.IsEmpty()
            || !MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
                Grant.ItemDefinitionId,
                UMythicAssetManager::ItemDefinitionType)
            || Grant.OriginalQuantity <= 0
            || Grant.RemainingQuantity <= 0
            || Grant.RemainingQuantity > Grant.OriginalQuantity
            || Grant.ItemLevel < 1 || Grant.ItemSeed == 0
            || !MHRewardOutboxPrivate::IsValidQuality(
                Grant.ResolvedQuality)
            || (Grant.bHasResolvedQuality
                    ? Grant.ResolvedQuality
                        == EMythicYieldQuality::Ragged
                    : Grant.ResolvedQuality
                        != EMythicYieldQuality::Common)) {
            OutDiagnosticCode = TEXT("InvalidSavedRewardGrant");
            return false;
        }
        const FMythicHarvestReceiptKey ExpectedKey =
            FMythicHarvestReceiptKey::MakeCompletion(
                Grant.WorldEpoch,
                FMythicHarvestNodeId(Grant.NodeGuid), Grant.Generation,
                MHRewardOutboxPrivate::ToReceiptChannel(Grant.Channel),
                static_cast<uint32>(Grant.RewardRowIndex));
        const FGuid ExpectedFingerprint =
            FMythicHarvestReceiptFingerprint::Build(
                ExpectedKey, Grant.ItemDefinitionId,
                Grant.OriginalQuantity, Grant.ItemSeed,
                static_cast<uint32>(Grant.ItemLevel),
                MHRewardOutboxPrivate::PackQualityAuxiliary(
                    Grant.bHasResolvedQuality, Grant.ResolvedQuality));
        if (!(Grant.ReceiptKey == ExpectedKey)
            || Grant.ReceiptPayloadFingerprint != ExpectedFingerprint
            || !MHRewardOutboxPrivate::AddContributorReceiptIdentity(
                SeenContributorReceipts, Grant.ContributorKey,
                Grant.ReceiptKey)) {
            OutDiagnosticCode = TEXT("InvalidSavedRewardGrantReceipt");
            return false;
        }
    }

    for (const FMythicSavedHarvestCompletionDeliveryV1 &Delivery :
         Snapshot.PendingCompletionDeliveries) {
        const FMythicHarvestRewardCompletionKey CompletionKey{
            Delivery.WorldEpoch,
            FMythicHarvestNodeId(Delivery.NodeGuid),
            Delivery.Generation};
        const bool bHasProficiency =
            Delivery.CompletionProficiencyXPQuanta > 0;
        const bool bHasQuest = Delivery.QuestCreditCount > 0;
        if (!CompletionKey.IsValid()
            || Delivery.WorldEpoch != Snapshot.WorldEpoch
            || !IsAdmittedCompletion(CompletionKey)
            || Delivery.ContributorKey.IsEmpty()
            || Delivery.CompletionProficiencyXPQuanta < 0
            || Delivery.QuestCreditCount < 0
            || (!bHasProficiency && !bHasQuest)
            || (Delivery.bProficiencyDelivered
                && Delivery.bQuestCreditDelivered)
            || !MHRewardOutboxPrivate::ContextIsValid(
                Delivery.ProficiencyContextTags)) {
            OutDiagnosticCode = TEXT("InvalidSavedCompletionDelivery");
            return false;
        }

        if (bHasProficiency) {
            const FMythicHarvestReceiptKey ExpectedKey =
                FMythicHarvestReceiptKey::MakeCompletion(
                    Delivery.WorldEpoch,
                    FMythicHarvestNodeId(Delivery.NodeGuid),
                    Delivery.Generation,
                    EMythicHarvestReceiptChannel::
                        CompletionProficiencyXP);
            const FGuid ExpectedFingerprint =
                FMythicHarvestReceiptFingerprint::Build(
                    ExpectedKey, Delivery.ProficiencyDefinitionId,
                    Delivery.CompletionProficiencyXPQuanta, 0, 0, 0,
                    Delivery.ProficiencyContextTags);
            if (!MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
                    Delivery.ProficiencyDefinitionId,
                    UMythicAssetManager::ProficiencyDefinitionType)
                || !(Delivery.ProficiencyReceiptKey == ExpectedKey)
                || Delivery.ProficiencyReceiptPayloadFingerprint
                    != ExpectedFingerprint
                || !MHRewardOutboxPrivate::AddContributorReceiptIdentity(
                    SeenContributorReceipts, Delivery.ContributorKey,
                    Delivery.ProficiencyReceiptKey)) {
                OutDiagnosticCode =
                    TEXT("InvalidSavedCompletionProficiencyReceipt");
                return false;
            }
        }
        else if (!Delivery.bProficiencyDelivered
                 || Delivery.ProficiencyDefinitionId.IsValid()
                 || !Delivery.ProficiencyContextTags.IsEmpty()
                 || Delivery.ProficiencyReceiptKey.IsValid()
                 || Delivery.ProficiencyReceiptPayloadFingerprint.IsValid()) {
            OutDiagnosticCode =
                TEXT("UnexpectedSavedCompletionProficiencyChannel");
            return false;
        }

        if (bHasQuest) {
            const FMythicHarvestReceiptKey ExpectedKey =
                FMythicHarvestReceiptKey::MakeCompletion(
                    Delivery.WorldEpoch,
                    FMythicHarvestNodeId(Delivery.NodeGuid),
                    Delivery.Generation,
                    EMythicHarvestReceiptChannel::CompletionQuestCredit);
            const FGuid ExpectedFingerprint =
                FMythicHarvestReceiptFingerprint::Build(
                    ExpectedKey, Delivery.HarvestableDefinitionId,
                    Delivery.QuestCreditCount, 0, 0, 0);
            if (!MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
                    Delivery.HarvestableDefinitionId,
                    UMythicAssetManager::HarvestableDefinitionType)
                || !(Delivery.QuestReceiptKey == ExpectedKey)
                || Delivery.QuestReceiptPayloadFingerprint
                    != ExpectedFingerprint
                || !MHRewardOutboxPrivate::AddContributorReceiptIdentity(
                    SeenContributorReceipts, Delivery.ContributorKey,
                    Delivery.QuestReceiptKey)) {
                OutDiagnosticCode =
                    TEXT("InvalidSavedCompletionQuestReceipt");
                return false;
            }
        }
        else if (!Delivery.bQuestCreditDelivered
                 || Delivery.HarvestableDefinitionId.IsValid()
                 || Delivery.QuestReceiptKey.IsValid()
                 || Delivery.QuestReceiptPayloadFingerprint.IsValid()) {
            OutDiagnosticCode =
                TEXT("UnexpectedSavedCompletionQuestChannel");
            return false;
        }
    }

    for (const FMythicSavedHarvestWorkDeliveryV1 &Delivery :
         Snapshot.PendingWorkDeliveries) {
        const FMythicHarvestRewardCompletionKey NodeGenerationKey{
            Delivery.WorldEpoch,
            FMythicHarvestNodeId(Delivery.NodeGuid),
            Delivery.Generation};
        const FMythicHarvestEpochNodeKey EpochNode{
            Delivery.WorldEpoch,
            FMythicHarvestNodeId(Delivery.NodeGuid)};
        const uint32 *HighWater = HighWaterByNode.Find(EpochNode);
        const uint32 MaximumIssuedGeneration = HighWater
            ? (*HighWater == MAX_uint32 ? MAX_uint32 : *HighWater + 1u)
            : 1u;
        int64 ExpectedCumulativeXP = 0;
        if (!NodeGenerationKey.IsValid()
            || Delivery.WorldEpoch != Snapshot.WorldEpoch
            || Delivery.Generation > MaximumIssuedGeneration
            || Delivery.ContributorKey.IsEmpty()
            || !Delivery.WorkRewardContract.IsValid()
            || !Delivery.WorkRewardContract.IsEnabled()
            || !MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
                Delivery.WorkRewardContract.ProficiencyDefinitionId,
                UMythicAssetManager::ProficiencyDefinitionType)
            || Delivery.CumulativeAppliedWorkQuanta <= 0
            || Delivery.ProficiencyXPQuanta <= 0
            || !FMythicHarvestReceiptQuantity::
                TryCalculateCumulativeAppliedWorkXP(
                    Delivery.CumulativeAppliedWorkQuanta,
                    Delivery.WorkRewardContract.
                        ProficiencyXPPerWorkUnitQuanta,
                    ExpectedCumulativeXP)
            || Delivery.ProficiencyXPQuanta != ExpectedCumulativeXP
            || !MHRewardOutboxPrivate::ContextIsValid(
                Delivery.WorkRewardContract.ContextTags)) {
            OutDiagnosticCode = TEXT("InvalidSavedAppliedWorkDelivery");
            return false;
        }
        const FMythicHarvestReceiptKey ExpectedReceiptKey =
            FMythicHarvestReceiptKey::MakeAppliedWork(
                Delivery.WorldEpoch,
                FMythicHarvestNodeId(Delivery.NodeGuid),
                Delivery.Generation, Delivery.ContributorKey);
        if (!Delivery.ReceiptKey.IsValid()
            || !(Delivery.ReceiptKey == ExpectedReceiptKey)
            || Delivery.ReceiptKey.WorldEpoch != Delivery.WorldEpoch
            || Delivery.ReceiptKey.NodeId
                != FMythicHarvestNodeId(Delivery.NodeGuid)
            || Delivery.ReceiptKey.Generation != Delivery.Generation
            || Delivery.ReceiptKey.Channel
                != EMythicHarvestReceiptChannel::
                    AppliedWorkProficiencyXP) {
            OutDiagnosticCode = TEXT("InvalidSavedAppliedWorkReceiptKey");
            return false;
        }
        const FGuid ExpectedFingerprint =
            FMythicHarvestReceiptFingerprint::BuildAppliedWorkSeries(
                Delivery.ReceiptKey,
                Delivery.WorkRewardContract.ProficiencyDefinitionId,
                Delivery.WorkRewardContract.
                    ProficiencyXPPerWorkUnitQuanta,
                Delivery.WorkRewardContract.ContextTags);
        if (Delivery.ReceiptPayloadFingerprint != ExpectedFingerprint
            || !MHRewardOutboxPrivate::AddContributorReceiptIdentity(
                SeenContributorReceipts, Delivery.ContributorKey,
                Delivery.ReceiptKey)) {
            OutDiagnosticCode = TEXT("InvalidSavedAppliedWorkReceipt");
            return false;
        }
    }

    TMap<FString, int32> DurabilitySeriesByContributor;
    TSet<FMythicHarvestReceiptKey> SeenDurabilityReceipts;
    for (const FMythicSavedHarvestDurabilityCostV1 &Cost :
         Snapshot.DurabilityCosts) {
        const FMythicHarvestRewardCompletionKey NodeGenerationKey{
            Cost.WorldEpoch, FMythicHarvestNodeId(Cost.NodeGuid),
            Cost.Generation};
        const FMythicHarvestEpochNodeKey EpochNode{
            Cost.WorldEpoch, FMythicHarvestNodeId(Cost.NodeGuid)};
        const uint32 *HighWater = HighWaterByNode.Find(EpochNode);
        const uint32 MaximumIssuedGeneration = HighWater
            ? (*HighWater == MAX_uint32 ? MAX_uint32
                                        : *HighWater + 1u)
            : 1u;
        int32 &ContributorSeries =
            DurabilitySeriesByContributor.FindOrAdd(
                Cost.ContributorKey);
        ++ContributorSeries;
        if (!NodeGenerationKey.IsValid()
            || Cost.WorldEpoch != Snapshot.WorldEpoch
            || Cost.Generation > MaximumIssuedGeneration
            || Cost.ContributorKey.IsEmpty()
            || !Cost.ToolItemInstanceGuid.IsValid()
            || Cost.CumulativeWearTarget <= 0
            || Cost.DurablyAppliedWearTarget < 0
            || Cost.DurablyAppliedWearTarget
                > Cost.CumulativeWearTarget
            || ContributorSeries
                > FMythicHarvestRewardOutboxSaveV1::
                    AbsoluteMaximumDurabilityCostSeriesPerContributor
            || SeenDurabilityReceipts.Contains(Cost.ReceiptKey)) {
            OutDiagnosticCode = TEXT("InvalidSavedHarvestDurabilityCost");
            return false;
        }
        const FMythicHarvestReceiptKey ExpectedKey =
            FMythicHarvestReceiptKey::MakeDurabilityCost(
                Cost.WorldEpoch, FMythicHarvestNodeId(Cost.NodeGuid),
                Cost.Generation, Cost.ContributorKey,
                Cost.ToolItemInstanceGuid);
        const FGuid ExpectedFingerprint =
            FMythicHarvestReceiptFingerprint::
                BuildDurabilityCostSeries(
                    ExpectedKey, Cost.ToolItemInstanceGuid);
        const bool bCompleted = HighWater
            && Cost.Generation <= *HighWater;
        if (!(Cost.ReceiptKey == ExpectedKey)
            || Cost.ReceiptPayloadFingerprint != ExpectedFingerprint
            || (Cost.DurablyAppliedWearTarget > 0
                && !LedgerFencesByContributor.Contains(
                    Cost.ContributorKey))
            || (bCompleted
                && Cost.DurablyAppliedWearTarget
                    == Cost.CumulativeWearTarget)
            || !MHRewardOutboxPrivate::AddContributorReceiptIdentity(
                SeenContributorReceipts, Cost.ContributorKey,
                Cost.ReceiptKey)) {
            OutDiagnosticCode =
                TEXT("InvalidSavedHarvestDurabilityReceipt");
            return false;
        }
        SeenDurabilityReceipts.Add(Cost.ReceiptKey);
    }

    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::TryResolveNextGeneration(
    const FGuid &WorldEpoch, const FMythicHarvestNodeId &NodeId,
    uint32 &OutGeneration) const {
    OutGeneration = 0;
    const FMythicHarvestEpochNodeKey Key{WorldEpoch, NodeId};
    if (!Key.IsValid()) {
        return false;
    }
    const uint32 *Highest = HighestKnownGenerationByNode.Find(Key);
    if (!Highest) {
        OutGeneration = 1;
        return true;
    }
    if (*Highest == MAX_uint32) {
        return false;
    }
    OutGeneration = *Highest + 1u;
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::TryGetHighestKnownGeneration(
    const FGuid &WorldEpoch, const FMythicHarvestNodeId &NodeId,
    uint32 &OutGeneration) const {
    OutGeneration = 0;
    const FMythicHarvestEpochNodeKey Key{WorldEpoch, NodeId};
    if (!Key.IsValid()) {
        return false;
    }
    const uint32 *Highest = HighestKnownGenerationByNode.Find(Key);
    if (!Highest || *Highest == 0) {
        return false;
    }
    OutGeneration = *Highest;
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::HasKnownCompletion(
    const FGuid &WorldEpoch, const FMythicHarvestNodeId &NodeId,
    const uint32 Generation) const {
    if (Generation == 0) {
        return false;
    }
    const FMythicHarvestEpochNodeKey EpochNode{WorldEpoch, NodeId};
    const uint32 *HighWater =
        HighestKnownGenerationByNode.Find(EpochNode);
    return HighWater && Generation <= *HighWater;
}

bool UMythicHarvestRewardOutboxSubsystem::MarkWorldSnapshotDurable(
    const FMythicHarvestRewardOutboxSaveV1 &Snapshot,
    FName &OutDiagnosticCode) {
    if (!ValidateSaveSnapshot(Snapshot, OutDiagnosticCode)
        || Snapshot.SnapshotSequence > LastIssuedWorldSnapshotSequence) {
        if (OutDiagnosticCode.IsNone()) {
            OutDiagnosticCode = TEXT("UnissuedDurableWorldSnapshot");
        }
        return false;
    }
    if (Snapshot.SnapshotSequence < LastDurableWorldSnapshotSequence) {
        OutDiagnosticCode = TEXT("StaleDurableWorldSnapshotCallback");
        return false;
    }

    LastDurableWorldSnapshotSequence = Snapshot.SnapshotSequence;
    LastDurableWorldEpoch = Snapshot.WorldEpoch;
    LastDurablePendingReceiptKeys.Reset();
    LastDurableCompletedGenerationByNode.Reset();
    for (const FMythicSavedHarvestGenerationHighWaterV1 &Saved :
         Snapshot.GenerationHighWatermarks) {
        LastDurableCompletedGenerationByNode.Add(
            FMythicHarvestNodeId(Saved.NodeGuid),
            Saved.HighestKnownGeneration);
    }
    for (const FMythicSavedHarvestRewardGrantV1 &Saved :
         Snapshot.PendingGrants) {
        LastDurablePendingReceiptKeys.Add(Saved.ReceiptKey);
    }
    for (const FMythicSavedHarvestCompletionDeliveryV1 &Saved :
         Snapshot.PendingCompletionDeliveries) {
        if (!Saved.bProficiencyDelivered) {
            LastDurablePendingReceiptKeys.Add(
                Saved.ProficiencyReceiptKey);
        }
        if (!Saved.bQuestCreditDelivered) {
            LastDurablePendingReceiptKeys.Add(Saved.QuestReceiptKey);
        }
    }
    for (const FMythicSavedHarvestWorkDeliveryV1 &Saved :
         Snapshot.PendingWorkDeliveries) {
        LastDurablePendingReceiptKeys.Add(Saved.ReceiptKey);
    }
    for (const FMythicSavedHarvestDurabilityCostV1 &Saved :
         Snapshot.DurabilityCosts) {
        if (Saved.DurablyAppliedWearTarget
            < Saved.CumulativeWearTarget) {
            LastDurablePendingReceiptKeys.Add(Saved.ReceiptKey);
        }
    }

    for (auto It = TrackedReceiptOwners.CreateIterator(); It; ++It) {
        AMythicPlayerState *PlayerState = It->Get();
        if (!PlayerState) {
            It.RemoveCurrent();
            continue;
        }
        FName CompactionDiagnostic;
        TryCompactReceiptLedgerForPlayer(
            *PlayerState, CompactionDiagnostic);
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestRewardOutboxSubsystem::
TryCompactReceiptLedgerForPlayer(AMythicPlayerState &PlayerState,
                                 FName &OutDiagnosticCode) {
    OutDiagnosticCode = NAME_None;
    if (!LastDurableWorldEpoch.IsValid()
        || LastDurableWorldSnapshotSequence == 0) {
        return true;
    }
    UMythicHarvestReceiptLedgerComponent *Ledger =
        ResolveReceiptLedger(&PlayerState);
    const FString &ContributorKey =
        PlayerState.GetPersistentCharacterId();
    if (!Ledger || ContributorKey.IsEmpty()
        || IsContributorAwaitingCurrentCharacterSave(ContributorKey)) {
        return true;
    }
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    const int32 Threshold = Settings
        ? Settings->RewardReceiptCompactionThreshold : 2048;
    if (Ledger->GetReceiptRowCount() < FMath::Max(1, Threshold)) {
        return true;
    }
    if (!Ledger->ValidateWorldSnapshotMinimum(
            LastDurableWorldEpoch, LastDurableWorldSnapshotSequence,
            OutDiagnosticCode)) {
        return false;
    }
    int32 Removed = 0;
    TSet<FMythicHarvestReceiptKey> ProtectedReceiptKeys =
        LastDurablePendingReceiptKeys;
    if (const UMythicHarvestRewardEscrowComponent *Escrow =
            ResolveRewardEscrow(&PlayerState)) {
        Escrow->AppendReceiptKeys(ProtectedReceiptKeys);
    }
    if (!Ledger->CompactCompletedRows(
            LastDurableWorldEpoch,
            LastDurableWorldSnapshotSequence,
            ProtectedReceiptKeys,
            LastDurableCompletedGenerationByNode, Removed,
            OutDiagnosticCode)) {
        return false;
    }
    if (Removed > 0) {
        RequestContributorDurabilitySave(ContributorKey, PlayerState);
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

AMythicPlayerController *
UMythicHarvestRewardOutboxSubsystem::ResolveContributorController(
    FMythicHarvestPlannedRewardGrant &Grant) const {
    return ResolveContributorController(
        Grant.ContributorKey, Grant.InitialController);
}

UMythicItemInstance *
UMythicHarvestRewardOutboxSubsystem::ResolveDurabilityTool(
    FMythicPendingHarvestDurabilityCost &Cost,
    AMythicPlayerController &Controller) const {
    if (!Cost.ToolItemInstanceGuid.IsValid()
        || Cost.ContributorKey.IsEmpty()) {
        Cost.CurrentTool.Reset();
        return nullptr;
    }
    UMythicItemInstance *Match = nullptr;
    for (UMythicInventoryComponent *Inventory :
         Controller.GetAllInventoryComponents()) {
        if (!Inventory || Inventory->GetOwner() != &Controller) continue;
        for (const FMythicInventorySlotEntry &Slot :
             Inventory->GetAllSlots()) {
            UMythicItemInstance *Candidate = Slot.SlottedItemInstance;
            if (!Candidate
                || Candidate->GetItemInstanceGuid()
                    != Cost.ToolItemInstanceGuid) {
                continue;
            }
            if (Match && Match != Candidate) {
                UE_LOG(Myth, Error,
                       TEXT("Duplicate item identity %s quarantined while applying harvest durability cost for %s."),
                       *Cost.ToolItemInstanceGuid.ToString(),
                       *Cost.ContributorKey);
                Cost.CurrentTool.Reset();
                return nullptr;
            }
            Match = Candidate;
        }
    }
    Cost.CurrentTool = Match;
    return Match;
}

AMythicPlayerController *
UMythicHarvestRewardOutboxSubsystem::ResolveContributorController(
    const FString &ContributorKey,
    TWeakObjectPtr<AMythicPlayerController> &InOutCurrentController) const {
    UMythicPlayerRegistrySubsystem *Registry = GetWorld()
        ? GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>()
        : nullptr;
    if (!Registry || ContributorKey.IsEmpty()) {
        InOutCurrentController.Reset();
        return nullptr;
    }
    AMythicPlayerController *Registered =
        Registry->GetPlayerControllerForKey(ContributorKey);
    if (AMythicPlayerController *FastPath =
            InOutCurrentController.Get()) {
        if (FastPath == Registered && FastPath->HasAuthority()
            && FastPath->GetWorld() == GetWorld()) {
            return FastPath;
        }
        InOutCurrentController.Reset();
    }
    if (Registered && Registered->HasAuthority()
        && Registered->GetWorld() == GetWorld()) {
        InOutCurrentController = Registered;
        return Registered;
    }
    return nullptr;
}

AMythicPlayerState *
UMythicHarvestRewardOutboxSubsystem::ResolveContributorPlayerState(
    const FString &ContributorKey) const {
    UMythicPlayerRegistrySubsystem *Registry = GetWorld()
        ? GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>()
        : nullptr;
    AMythicPlayerState *PlayerState = Registry
        ? Registry->GetPlayerStateForKey(ContributorKey) : nullptr;
    return PlayerState && PlayerState->HasAuthority()
            && PlayerState->GetWorld() == GetWorld()
            && PlayerState->GetPersistentCharacterId() == ContributorKey
        ? PlayerState : nullptr;
}

UMythicHarvestReceiptLedgerComponent *
UMythicHarvestRewardOutboxSubsystem::ResolveReceiptLedger(
    AMythicPlayerState *PlayerState) const {
    return PlayerState && PlayerState->HasAuthority()
        ? PlayerState->GetHarvestReceiptLedger() : nullptr;
}

UMythicHarvestRewardEscrowComponent *
UMythicHarvestRewardOutboxSubsystem::ResolveRewardEscrow(
    AMythicPlayerState *PlayerState) const {
    return PlayerState && PlayerState->HasAuthority()
        ? PlayerState->GetHarvestRewardEscrow() : nullptr;
}

UItemDefinition *UMythicHarvestRewardOutboxSubsystem::
ResolveItemDefinition(FMythicHarvestPlannedRewardGrant &Grant) const {
    if (IsValid(Grant.ItemDefinition)
        && Grant.ItemDefinition->GetPrimaryAssetId()
            == Grant.ItemDefinitionId) {
        return Grant.ItemDefinition;
    }
    Grant.ItemDefinition = nullptr;
    UGameInstance *GameInstance = GetWorld()
        ? GetWorld()->GetGameInstance() : nullptr;
    UItemizationSubsystem *Itemization = GameInstance
        ? GameInstance->GetSubsystem<UItemizationSubsystem>() : nullptr;
    UItemDefinition *Definition = Itemization
        ? Itemization->GetItemDefinition(Grant.ItemDefinitionId) : nullptr;
    if (Definition
        && Definition->GetPrimaryAssetId() == Grant.ItemDefinitionId) {
        Grant.ItemDefinition = Definition;
        return Definition;
    }
    return nullptr;
}

bool UMythicHarvestRewardOutboxSubsystem::TryDeliverOneEscrowRow(
    AMythicPlayerState &PlayerState,
    UMythicItemFactorySubsystem &Factory,
    int64 &OutDeliveredQuantity,
    bool &bOutRowCompleted) {
    OutDeliveredQuantity = 0;
    bOutRowCompleted = false;
    const FString ContributorKey =
        PlayerState.GetPersistentCharacterId();
    TWeakObjectPtr<AMythicPlayerController> ControllerFastPath;
    AMythicPlayerController *Controller = ResolveContributorController(
        ContributorKey, ControllerFastPath);
    UMythicHarvestRewardEscrowComponent *Escrow =
        ResolveRewardEscrow(&PlayerState);
    UMythicInventoryComponent *Inventory = Controller
        ? Controller->GetInventoryComponent() : nullptr;
    if (!Controller || Controller->GetPlayerState<AMythicPlayerState>()
            != &PlayerState
        || !Escrow || !Inventory || !Inventory->GetOwner()
        || !Inventory->GetOwner()->HasAuthority()) {
        return false;
    }

    FMythicHarvestEscrowDeliveryPlan Plan;
    FMythicSavedHarvestItemEscrowRowV1 Row;
    if (!Escrow->TryPlanNextDelivery(MAX_int32, Plan, Row)) {
        return false;
    }
    UGameInstance *GameInstance = GetWorld()
        ? GetWorld()->GetGameInstance() : nullptr;
    UItemizationSubsystem *Itemization = GameInstance
        ? GameInstance->GetSubsystem<UItemizationSubsystem>() : nullptr;
    UItemDefinition *Definition = Itemization
        ? Itemization->GetItemDefinition(Row.ItemDefinitionId) : nullptr;
    if (!Definition
        || Definition->GetPrimaryAssetId() != Row.ItemDefinitionId) {
        Escrow->CancelPlannedDelivery(Plan);
        return false;
    }

    FMythicCreateItemRequest Request;
    Request.ItemDefinition = Definition;
    Request.Quantity = Plan.RequestedQuantity;
    Request.ItemLevel = Row.ItemLevel;
    Request.OwningActor = Controller;
    Request.ServerSeed = Row.ItemSeed;
    if (Row.bHasResolvedQuality) {
        Request.YieldQualityOverride = Row.ResolvedQuality;
    }
    FMythicCreateItemResult CreateResult = Factory.CreateItemReady(Request);
    if (!CreateResult.IsSuccess()) {
        Escrow->CancelPlannedDelivery(Plan);
        return false;
    }
    UMythicItemInstance *CreatedItem = CreateResult.Item;
    const int32 InsertedQuantity = FMath::Clamp(
        Inventory->AddToAnySlot(CreatedItem), 0, Plan.RequestedQuantity);
    if (!Escrow->CommitPlannedDelivery(Plan, InsertedQuantity)) {
        UE_LOG(Myth, Fatal,
               TEXT("Validated harvest escrow delivery failed after inventory insertion for %s."),
               *ContributorKey);
        return false;
    }
    if (InsertedQuantity < Plan.RequestedQuantity
        && IsValid(CreatedItem)) {
        CreatedItem->Destroy();
    }
    // Reported after the escrow commit, so the feed is exactly as idempotent as the insertion it describes:
    // a retry that already applied never reaches here, and a full bag inserts nothing and stays silent.
    if (InsertedQuantity > 0) {
        Inventory->NotifyOwnerItemAcquired(Definition, InsertedQuantity);
    }
    OutDeliveredQuantity = InsertedQuantity;
    bOutRowCompleted =
        Escrow->FindRow(Plan.ReceiptKey) == nullptr;
    return true;
}

UProficiencyDefinition *
UMythicHarvestRewardOutboxSubsystem::ResolveProficiencyDefinition(
    FMythicPendingHarvestCompletionDelivery &Delivery) const {
    if (IsValid(Delivery.ProficiencyDefinition)
        && Delivery.ProficiencyDefinition->GetPrimaryAssetId()
            == Delivery.ProficiencyDefinitionId) {
        return Delivery.ProficiencyDefinition;
    }
    Delivery.ProficiencyDefinition = nullptr;
    if (!MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
            Delivery.ProficiencyDefinitionId,
            UMythicAssetManager::ProficiencyDefinitionType)) {
        return nullptr;
    }
    UProficiencyDefinition *Definition = Cast<UProficiencyDefinition>(
        UMythicAssetManager::Get().GetPrimaryAssetObject(
            Delivery.ProficiencyDefinitionId));
    if (Definition
        && Definition->GetPrimaryAssetId()
            == Delivery.ProficiencyDefinitionId) {
        Delivery.ProficiencyDefinition = Definition;
        return Definition;
    }
    return nullptr;
}

UProficiencyDefinition *
UMythicHarvestRewardOutboxSubsystem::ResolveProficiencyDefinition(
    FMythicPendingHarvestWorkDelivery &Delivery) const {
    if (IsValid(Delivery.ProficiencyDefinition)
        && Delivery.ProficiencyDefinition->GetPrimaryAssetId()
            == Delivery.WorkRewardContract.ProficiencyDefinitionId) {
        return Delivery.ProficiencyDefinition;
    }
    Delivery.ProficiencyDefinition = nullptr;
    if (!MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
            Delivery.WorkRewardContract.ProficiencyDefinitionId,
            UMythicAssetManager::ProficiencyDefinitionType)) {
        return nullptr;
    }
    UProficiencyDefinition *Definition = Cast<UProficiencyDefinition>(
        UMythicAssetManager::Get().GetPrimaryAssetObject(
            Delivery.WorkRewardContract.ProficiencyDefinitionId));
    if (Definition
        && Definition->GetPrimaryAssetId()
            == Delivery.WorkRewardContract.ProficiencyDefinitionId) {
        Delivery.ProficiencyDefinition = Definition;
        return Definition;
    }
    return nullptr;
}

UMythicHarvestableDefinition *
UMythicHarvestRewardOutboxSubsystem::ResolveHarvestableDefinition(
    FMythicPendingHarvestCompletionDelivery &Delivery) const {
    if (IsValid(Delivery.HarvestableDefinition)
        && Delivery.HarvestableDefinition->GetPrimaryAssetId()
            == Delivery.HarvestableDefinitionId) {
        return Delivery.HarvestableDefinition;
    }
    Delivery.HarvestableDefinition = nullptr;
    if (!MHRewardOutboxPrivate::IsExpectedPrimaryAssetType(
            Delivery.HarvestableDefinitionId,
            UMythicAssetManager::HarvestableDefinitionType)) {
        return nullptr;
    }
    UMythicHarvestableDefinition *Definition =
        Cast<UMythicHarvestableDefinition>(
            UMythicAssetManager::Get().GetPrimaryAssetObject(
                Delivery.HarvestableDefinitionId));
    if (Definition
        && Definition->GetPrimaryAssetId()
            == Delivery.HarvestableDefinitionId) {
        Delivery.HarvestableDefinition = Definition;
        return Definition;
    }
    return nullptr;
}

void UMythicHarvestRewardOutboxSubsystem::RefreshLoadedDefinitions() {
    for (FMythicPendingHarvestCompletionDelivery &Delivery :
         PendingCompletionDeliveries) {
        if (!Delivery.bProficiencyDelivered) {
            ResolveProficiencyDefinition(Delivery);
        }
        if (!Delivery.bQuestCreditDelivered) {
            ResolveHarvestableDefinition(Delivery);
        }
    }
    for (FMythicPendingHarvestWorkDelivery &Delivery :
         PendingWorkDeliveries) {
        ResolveProficiencyDefinition(Delivery);
    }
}

void UMythicHarvestRewardOutboxSubsystem::
RequestPendingDefinitionLoads() {
    RefreshLoadedDefinitions();
    TSet<FPrimaryAssetId> UniqueIds;
    for (const FMythicPendingHarvestCompletionDelivery &Delivery :
         PendingCompletionDeliveries) {
        if (!Delivery.bProficiencyDelivered
            && !IsValid(Delivery.ProficiencyDefinition)
            && Delivery.ProficiencyDefinitionId.IsValid()
            && !InFlightDefinitionLoads.Contains(
                Delivery.ProficiencyDefinitionId)) {
            UniqueIds.Add(Delivery.ProficiencyDefinitionId);
        }
        if (!Delivery.bQuestCreditDelivered
            && !IsValid(Delivery.HarvestableDefinition)
            && Delivery.HarvestableDefinitionId.IsValid()
            && !InFlightDefinitionLoads.Contains(
                Delivery.HarvestableDefinitionId)) {
            UniqueIds.Add(Delivery.HarvestableDefinitionId);
        }
    }
    for (const FMythicPendingHarvestWorkDelivery &Delivery :
         PendingWorkDeliveries) {
        if (!IsValid(Delivery.ProficiencyDefinition)
            && Delivery.WorkRewardContract.ProficiencyDefinitionId.IsValid()
            && !InFlightDefinitionLoads.Contains(
                Delivery.WorkRewardContract.ProficiencyDefinitionId)) {
            UniqueIds.Add(
                Delivery.WorkRewardContract.ProficiencyDefinitionId);
        }
    }
    if (UniqueIds.IsEmpty()) {
        return;
    }

    TArray<FPrimaryAssetId> RequestedIds = UniqueIds.Array();
    RequestedIds.Sort([](const FPrimaryAssetId &Left,
                         const FPrimaryAssetId &Right) {
        if (Left.PrimaryAssetType != Right.PrimaryAssetType) {
            return Left.PrimaryAssetType.ToString()
                < Right.PrimaryAssetType.ToString();
        }
        return Left.PrimaryAssetName.LexicalLess(
            Right.PrimaryAssetName);
    });
    for (const FPrimaryAssetId &Id : RequestedIds) {
        InFlightDefinitionLoads.Add(Id);
    }

    TWeakObjectPtr<UMythicHarvestRewardOutboxSubsystem> WeakThis(this);
    const FStreamableDelegate OnLoaded =
        FStreamableDelegate::CreateLambda([WeakThis, RequestedIds]() {
            if (UMythicHarvestRewardOutboxSubsystem *Self =
                    WeakThis.Get()) {
                for (const FPrimaryAssetId &Id : RequestedIds) {
                    Self->InFlightDefinitionLoads.Remove(Id);
                }
                Self->RefreshLoadedDefinitions();
            }
        });
    TSharedPtr<FStreamableHandle> Handle =
        UMythicAssetManager::Get().LoadPrimaryAssets(
            RequestedIds, TArray<FName>(), OnLoaded,
            FStreamableManager::AsyncLoadHighPriority);
    if (Handle.IsValid()) {
        DefinitionLoadHandles.Add(MoveTemp(Handle));
    }
    else {
        for (const FPrimaryAssetId &Id : RequestedIds) {
            InFlightDefinitionLoads.Remove(Id);
        }
    }
}

bool UMythicHarvestRewardOutboxSubsystem::IsSameGrantIdentity(
    const FMythicPendingHarvestRewardDelivery &Left,
    const FMythicPendingHarvestRewardDelivery &Right) {
    return Left.Grant.ContributorKey == Right.Grant.ContributorKey
        && Left.Grant.ReceiptKey == Right.Grant.ReceiptKey;
}

void UMythicHarvestRewardOutboxSubsystem::SortPendingDeliveries(
    TArray<FMythicPendingHarvestRewardDelivery> &Deliveries) {
    Deliveries.Sort([](
        const FMythicPendingHarvestRewardDelivery &Left,
        const FMythicPendingHarvestRewardDelivery &Right) {
        if (Left.Grant.ReceiptKey == Right.Grant.ReceiptKey) {
            return Left.Grant.ContributorKey.Compare(
                       Right.Grant.ContributorKey,
                       ESearchCase::CaseSensitive) < 0;
        }
        return MHRewardOutboxPrivate::ReceiptKeyLess(
            Left.Grant.ReceiptKey, Right.Grant.ReceiptKey);
    });
}

void UMythicHarvestRewardOutboxSubsystem::
SortPendingCompletionDeliveries(
    TArray<FMythicPendingHarvestCompletionDelivery> &Deliveries) {
    Deliveries.Sort([](
        const FMythicPendingHarvestCompletionDelivery &Left,
        const FMythicPendingHarvestCompletionDelivery &Right) {
        if (!(Left.CompletionKey == Right.CompletionKey)) {
            return MHRewardOutboxPrivate::CompletionKeyLess(
                Left.CompletionKey, Right.CompletionKey);
        }
        return Left.ContributorKey.Compare(
                   Right.ContributorKey,
                   ESearchCase::CaseSensitive) < 0;
    });
}

void UMythicHarvestRewardOutboxSubsystem::SortPendingWorkDeliveries(
    TArray<FMythicPendingHarvestWorkDelivery> &Deliveries) {
    Deliveries.Sort([](
        const FMythicPendingHarvestWorkDelivery &Left,
        const FMythicPendingHarvestWorkDelivery &Right) {
        if (Left.ReceiptKey == Right.ReceiptKey) {
            return Left.ContributorKey.Compare(
                       Right.ContributorKey,
                       ESearchCase::CaseSensitive) < 0;
        }
        return MHRewardOutboxPrivate::ReceiptKeyLess(
            Left.ReceiptKey, Right.ReceiptKey);
    });
}
