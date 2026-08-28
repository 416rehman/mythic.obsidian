#include "World/Harvesting/MythicHarvestSaveTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicHarvestSaveTypes)

namespace {
bool GuidLess(const FGuid &Left, const FGuid &Right) {
    if (Left.A != Right.A) return Left.A < Right.A;
    if (Left.B != Right.B) return Left.B < Right.B;
    if (Left.C != Right.C) return Left.C < Right.C;
    return Left.D < Right.D;
}

bool IsUnavailableLifecycleState(const EMythicHarvestNodeState State) {
    return State == EMythicHarvestNodeState::Depleted
        || State == EMythicHarvestNodeState::Regrowing;
}

bool IsValidSavedContributor(
    const FMythicSavedHarvestContributorV1 &Contributor) {
    return !Contributor.ContributorKey.IsEmpty()
        && Contributor.ContributorKey.Len() <= 256
        && !Contributor.ContributorKey.StartsWith(
            TEXT("session:"), ESearchCase::CaseSensitive)
        && Contributor.ContributionQuanta > 0
        && Contributor.ItemLevel >= 1
        && Contributor.QuantityMultiplierQuanta >= 0
        && Contributor.ProficiencyLevel >= 0
        && Contributor.WorkRewardContract.IsValid();
}
}

bool FMythicHarvestWorldSaveV1::Validate(
    const FMythicHarvestWorldSaveV1 &Snapshot,
    FName &OutDiagnosticCode,
    const int32 ConfiguredMaximumNodes,
    const int32 ConfiguredMaximumContributorsPerNode,
    const int32 ConfiguredMaximumTotalContributors,
    const int32 ConfiguredMaximumReplicationCells,
    const int32 ConfiguredMaximumCellCoordinateMagnitude) {
    if (ConfiguredMaximumNodes < 1
        || ConfiguredMaximumNodes > AbsoluteMaximumNodes
        || ConfiguredMaximumContributorsPerNode < 1
        || ConfiguredMaximumContributorsPerNode
            > AbsoluteMaximumContributorsPerNode
        || ConfiguredMaximumTotalContributors < 1
        || ConfiguredMaximumTotalContributors
            > AbsoluteMaximumTotalContributors
        || ConfiguredMaximumReplicationCells < 1
        || ConfiguredMaximumReplicationCells
            > AbsoluteMaximumReplicationCells
        || ConfiguredMaximumCellCoordinateMagnitude < 1
        || ConfiguredMaximumCellCoordinateMagnitude
            > AbsoluteMaximumCellCoordinateMagnitude) {
        OutDiagnosticCode = TEXT("InvalidHarvestWorldValidationLimits");
        return false;
    }
    if (Snapshot.SchemaVersion != CurrentSchemaVersion) {
        OutDiagnosticCode = TEXT("UnsupportedHarvestWorldSchema");
        return false;
    }
    if (!Snapshot.WorldEpoch.IsValid()) {
        OutDiagnosticCode = TEXT("InvalidHarvestWorldEpoch");
        return false;
    }
    if (Snapshot.Nodes.Num() > ConfiguredMaximumNodes) {
        OutDiagnosticCode = TEXT("HarvestWorldNodeCapacityExceeded");
        return false;
    }

    TSet<FGuid> NodeIds;
    NodeIds.Reserve(Snapshot.Nodes.Num());
    TSet<FIntPoint> ReplicationCells;
    ReplicationCells.Reserve(FMath::Min(
        Snapshot.Nodes.Num(), ConfiguredMaximumReplicationCells));
    int32 TotalContributorRows = 0;
    for (const FMythicSavedHarvestNodeV1 &Node : Snapshot.Nodes) {
        if (!Node.NodeGuid.IsValid() || Node.WorldEpoch != Snapshot.WorldEpoch
            || Node.Generation == 0 || Node.Revision == 0
            || !FMath::IsFinite(Node.ReplicationCellCenterZ)
            || FMath::Abs(Node.ReplicationCellCenterZ)
                > AbsoluteMaximumCellCenterZCentimeters
            || Node.ReplicationCellCoordinate.X
                < -ConfiguredMaximumCellCoordinateMagnitude
            || Node.ReplicationCellCoordinate.X
                > ConfiguredMaximumCellCoordinateMagnitude
            || Node.ReplicationCellCoordinate.Y
                < -ConfiguredMaximumCellCoordinateMagnitude
            || Node.ReplicationCellCoordinate.Y
                > ConfiguredMaximumCellCoordinateMagnitude
            || !FMath::IsFinite(Node.RemainingRespawnSeconds)
            || Node.RemainingRespawnSeconds < 0.0
            || Node.RemainingRespawnSeconds
                > static_cast<double>(MAX_flt)) {
            OutDiagnosticCode = TEXT("InvalidSavedHarvestNode");
            return false;
        }
        if (NodeIds.Contains(Node.NodeGuid)) {
            OutDiagnosticCode = TEXT("DuplicateSavedHarvestNode");
            return false;
        }
        NodeIds.Add(Node.NodeGuid);
        ReplicationCells.Add(Node.ReplicationCellCoordinate);
        if (ReplicationCells.Num() > ConfiguredMaximumReplicationCells) {
            OutDiagnosticCode = TEXT("HarvestReplicationCellCapacityExceeded");
            return false;
        }

        if (Node.State == EMythicHarvestNodeState::Available) {
            if (Node.RemainingRespawnSeconds != 0.0
                || Node.CapturedMaximumWorkQuanta <= 0
                || Node.RemainingWorkQuanta <= 0
                || Node.RemainingWorkQuanta
                    >= Node.CapturedMaximumWorkQuanta
                || Node.Contributors.IsEmpty()) {
                OutDiagnosticCode = TEXT("InvalidSavedHarvestPartialNode");
                return false;
            }
            if (Node.Contributors.Num()
                > ConfiguredMaximumContributorsPerNode
                || Node.Contributors.Num()
                    > ConfiguredMaximumTotalContributors
                        - TotalContributorRows) {
                OutDiagnosticCode =
                    TEXT("HarvestContributorCapacityExceeded");
                return false;
            }
            TotalContributorRows += Node.Contributors.Num();

            int64 TotalContributionQuanta = 0;
            const FString *PreviousContributorKey = nullptr;
            for (const FMythicSavedHarvestContributorV1 &Contributor :
                 Node.Contributors) {
                if (!IsValidSavedContributor(Contributor)
                    || (PreviousContributorKey
                        && PreviousContributorKey->Compare(
                               Contributor.ContributorKey,
                               ESearchCase::CaseSensitive) >= 0)
                    || Contributor.ContributionQuanta
                        > MAX_int64 - TotalContributionQuanta) {
                    OutDiagnosticCode =
                        TEXT("InvalidSavedHarvestContributor");
                    return false;
                }
                TotalContributionQuanta += Contributor.ContributionQuanta;
                PreviousContributorKey = &Contributor.ContributorKey;
            }
            const int64 AppliedWorkQuanta =
                Node.CapturedMaximumWorkQuanta
                - Node.RemainingWorkQuanta;
            if (AppliedWorkQuanta <= 0
                || TotalContributionQuanta != AppliedWorkQuanta) {
                OutDiagnosticCode =
                    TEXT("HarvestContributorWorkMismatch");
                return false;
            }
            continue;
        }

        if (!IsUnavailableLifecycleState(Node.State)
            || Node.CapturedMaximumWorkQuanta != 0
            || Node.RemainingWorkQuanta != 0
            || !Node.Contributors.IsEmpty()) {
            OutDiagnosticCode = TEXT("InvalidSavedHarvestNode");
            return false;
        }
    }

    OutDiagnosticCode = NAME_None;
    return true;
}

void FMythicHarvestWorldSaveV1::SortCanonical() {
    for (FMythicSavedHarvestNodeV1 &Node : Nodes) {
        Node.Contributors.Sort(
            [](const FMythicSavedHarvestContributorV1 &Left,
               const FMythicSavedHarvestContributorV1 &Right) {
                return Left.ContributorKey.Compare(
                           Right.ContributorKey,
                           ESearchCase::CaseSensitive) < 0;
            });
    }
    Nodes.Sort([](const FMythicSavedHarvestNodeV1 &Left,
                  const FMythicSavedHarvestNodeV1 &Right) {
        return GuidLess(Left.NodeGuid, Right.NodeGuid);
    });
}
