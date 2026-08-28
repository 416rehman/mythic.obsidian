#include "World/Harvesting/MythicHarvestParticipantSnapshot.h"

bool FMythicHarvestParticipantIdentityPolicy::TryResolveReadyContributorKey(
    const FString &PersistentCharacterId,
    const FString &CanonicalPlayerKey,
    const FString &RegisteredControllerKey,
    FString &OutContributorKey) {
    OutContributorKey.Reset();
    if (PersistentCharacterId.IsEmpty()
        || CanonicalPlayerKey != PersistentCharacterId
        || RegisteredControllerKey != PersistentCharacterId) {
        return false;
    }

    OutContributorKey = PersistentCharacterId;
    return true;
}

bool FMythicHarvestParticipantSnapshot::IsValid() const {
    return !ContributorKey.IsEmpty() && ContributionQuanta > 0
        && ItemLevel >= 1 && QuantityMultiplierQuanta >= 0
        && ProficiencyLevel >= 0
        && (WorkRewardContract.IsUnset() || WorkRewardContract.IsValid());
}

bool FMythicHarvestParticipantLedger::TryAccumulate(
    TMap<FString, FMythicHarvestParticipantSnapshot> &InOutLedger,
    const FMythicHarvestParticipantSnapshot &AcceptedWorkSnapshot) {
    if (!AcceptedWorkSnapshot.IsValid()) {
        return false;
    }

    FMythicHarvestParticipantSnapshot *Existing =
        InOutLedger.Find(AcceptedWorkSnapshot.ContributorKey);
    if (!Existing) {
        InOutLedger.Add(AcceptedWorkSnapshot.ContributorKey,
                        AcceptedWorkSnapshot);
        return true;
    }
    if (!Existing->IsValid()
        || Existing->ContributorKey != AcceptedWorkSnapshot.ContributorKey
        || (!Existing->WorkRewardContract.IsUnset()
            && !AcceptedWorkSnapshot.WorkRewardContract.IsUnset()
            && !(Existing->WorkRewardContract
                 == AcceptedWorkSnapshot.WorkRewardContract))
        || Existing->ContributionQuanta
            > MAX_int64 - AcceptedWorkSnapshot.ContributionQuanta) {
        return false;
    }

    if (Existing->WorkRewardContract.IsUnset()
        && !AcceptedWorkSnapshot.WorkRewardContract.IsUnset()) {
        Existing->WorkRewardContract =
            AcceptedWorkSnapshot.WorkRewardContract;
    }
    Existing->ContributionQuanta += AcceptedWorkSnapshot.ContributionQuanta;
    Existing->ItemLevel = AcceptedWorkSnapshot.ItemLevel;
    Existing->QuantityMultiplierQuanta =
        AcceptedWorkSnapshot.QuantityMultiplierQuanta;
    Existing->ProficiencyLevel = AcceptedWorkSnapshot.ProficiencyLevel;
    Existing->CurrentController = AcceptedWorkSnapshot.CurrentController;
    return true;
}

bool FMythicHarvestParticipantLedger::BuildEligibleSnapshots(
    const TMap<FString, FMythicHarvestParticipantSnapshot> &Ledger,
    const int64 MinimumContributionQuanta,
    TArray<FMythicHarvestParticipantSnapshot> &OutEligibleSnapshots,
    int64 *OutTotalEligibleContributionQuanta) {
    OutEligibleSnapshots.Reset();
    if (OutTotalEligibleContributionQuanta) {
        *OutTotalEligibleContributionQuanta = 0;
    }
    if (MinimumContributionQuanta < 0) {
        return false;
    }

    int64 TotalEligibleContributionQuanta = 0;
    OutEligibleSnapshots.Reserve(Ledger.Num());
    for (const TPair<FString, FMythicHarvestParticipantSnapshot> &Pair :
         Ledger) {
        const FMythicHarvestParticipantSnapshot &Snapshot = Pair.Value;
        if (Pair.Key != Snapshot.ContributorKey || !Snapshot.IsValid()) {
            OutEligibleSnapshots.Reset();
            return false;
        }
        if (Snapshot.ContributionQuanta < MinimumContributionQuanta) {
            continue;
        }
        if (Snapshot.ContributionQuanta
            > MAX_int64 - TotalEligibleContributionQuanta) {
            OutEligibleSnapshots.Reset();
            return false;
        }
        TotalEligibleContributionQuanta += Snapshot.ContributionQuanta;
        OutEligibleSnapshots.Add(Snapshot);
    }

    OutEligibleSnapshots.Sort(
        [](const FMythicHarvestParticipantSnapshot &Left,
           const FMythicHarvestParticipantSnapshot &Right) {
            return Left.ContributorKey < Right.ContributorKey;
        });
    if (OutTotalEligibleContributionQuanta) {
        *OutTotalEligibleContributionQuanta =
            TotalEligibleContributionQuanta;
    }
    return true;
}
