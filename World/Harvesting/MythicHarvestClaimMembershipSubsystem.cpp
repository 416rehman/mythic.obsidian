#include "World/Harvesting/MythicHarvestClaimMembershipSubsystem.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicHarvestClaimMembershipSubsystem)

FMythicHarvestClaimIdentity FMythicHarvestClaimIdentity::MakePlayer(
    const FString &InPersistentPlayerKey) {
    FMythicHarvestClaimIdentity Result;
    Result.Kind = EKind::Player;
    Result.PersistentPlayerKey = InPersistentPlayerKey;
    return Result;
}

FMythicHarvestClaimIdentity FMythicHarvestClaimIdentity::MakeParty(
    const FGuid &InPartyId) {
    FMythicHarvestClaimIdentity Result;
    Result.Kind = EKind::Party;
    Result.PartyId = InPartyId;
    return Result;
}

bool FMythicHarvestClaimIdentity::IsValid() const {
    switch (Kind) {
        case EKind::Player:
            return !PersistentPlayerKey.IsEmpty()
                && !PersistentPlayerKey.StartsWith(TEXT("session:"),
                                                   ESearchCase::CaseSensitive)
                && !PartyId.IsValid();
        case EKind::Party:
            return PartyId.IsValid() && PersistentPlayerKey.IsEmpty();
        default:
            return false;
    }
}

bool FMythicHarvestClaimIdentity::operator==(
    const FMythicHarvestClaimIdentity &Other) const {
    if (Kind != Other.Kind) {
        return false;
    }
    switch (Kind) {
        case EKind::Player:
            return PersistentPlayerKey.Equals(
                Other.PersistentPlayerKey, ESearchCase::CaseSensitive);
        case EKind::Party:
            return PartyId == Other.PartyId;
        default:
            return true;
    }
}

bool UMythicHarvestClaimMembershipSubsystem::ShouldCreateSubsystem(
    UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

bool UMythicHarvestClaimMembershipSubsystem::IsPersistentPlayerKeyValid(
    const FString &PersistentPlayerKey) {
    return !PersistentPlayerKey.IsEmpty()
        && PersistentPlayerKey.Len() <= 256
        && !PersistentPlayerKey.StartsWith(TEXT("session:"),
                                           ESearchCase::CaseSensitive);
}

bool UMythicHarvestClaimMembershipSubsystem::ReplacePartyMembers(
    const FGuid &PartyId, const uint64 RosterRevision,
    const TConstArrayView<FString> PersistentPlayerKeys) {
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || !PartyId.IsValid()
        || RosterRevision == 0) {
        return false;
    }

    TSet<FString> UniqueMembers;
    UniqueMembers.Reserve(PersistentPlayerKeys.Num());
    for (const FString &PlayerKey : PersistentPlayerKeys) {
        if (!IsPersistentPlayerKeyValid(PlayerKey)
            || UniqueMembers.Contains(PlayerKey)) {
            return false;
        }
        UniqueMembers.Add(PlayerKey);
    }
    if (RosterRevision <= HighestAppliedRosterRevision) {
        return false;
    }

    TMap<FString, FGuid> Staged = PartyByPersistentPlayer;
    for (auto It = Staged.CreateIterator(); It; ++It) {
        if (It.Value() == PartyId) {
            It.RemoveCurrent();
        }
    }
    for (const FString &PlayerKey : UniqueMembers) {
        Staged.Add(PlayerKey, PartyId);
    }
    PartyByPersistentPlayer = MoveTemp(Staged);
    HighestAppliedRosterRevision = RosterRevision;
    return true;
}

bool UMythicHarvestClaimMembershipSubsystem::RemoveParty(
    const FGuid &PartyId, const uint64 RosterRevision) {
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || !PartyId.IsValid()
        || RosterRevision == 0) {
        return false;
    }
    if (RosterRevision <= HighestAppliedRosterRevision) {
        return false;
    }
    for (auto It = PartyByPersistentPlayer.CreateIterator(); It; ++It) {
        if (It.Value() == PartyId) {
            It.RemoveCurrent();
        }
    }
    HighestAppliedRosterRevision = RosterRevision;
    return true;
}

bool UMythicHarvestClaimMembershipSubsystem::TryResolveParty(
    const FString &PersistentPlayerKey, FGuid &OutPartyId) const {
    OutPartyId.Invalidate();
    if (!IsPersistentPlayerKeyValid(PersistentPlayerKey)) {
        return false;
    }
    const FGuid *Found = PartyByPersistentPlayer.Find(PersistentPlayerKey);
    if (!Found || !Found->IsValid()) {
        return false;
    }
    OutPartyId = *Found;
    return true;
}
