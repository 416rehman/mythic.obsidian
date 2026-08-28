#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "MythicHarvestClaimMembershipSubsystem.generated.h"

/** Native authority identity used by the soft-claim gate; it is never replicated or persisted as node state. */
struct MYTHIC_API FMythicHarvestClaimIdentity {
    enum class EKind : uint8 {
        None,
        Player,
        Party,
    };

    EKind Kind = EKind::None;
    FString PersistentPlayerKey;
    FGuid PartyId;

    static FMythicHarvestClaimIdentity MakePlayer(
        const FString &InPersistentPlayerKey);
    static FMythicHarvestClaimIdentity MakeParty(const FGuid &InPartyId);

    bool IsValid() const;
    bool operator==(const FMythicHarvestClaimIdentity &Other) const;
};

/**
 * Server-only bridge between the online/co-op party authority and harvesting.
 *
 * Harvesting owns no social-party lifecycle. The authoritative party service publishes complete membership snapshots
 * here using persistent character keys and a typed party GUID. Until it does, players safely receive independent
 * claims. Replacing a party is atomic on the game thread, so a node transaction never observes a half-updated party.
 */
UCLASS()
class MYTHIC_API UMythicHarvestClaimMembershipSubsystem final
    : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;

    /**
     * Atomically replaces one party incarnation's authoritative membership when RosterRevision is newer than every
     * roster transition accepted from the social authority. The revision is globally monotonic across all parties,
     * not party-local, so a delayed old-party update cannot reclaim a player after a cross-party move.
     */
    bool ReplacePartyMembers(const FGuid &PartyId, uint64 RosterRevision,
                             TConstArrayView<FString> PersistentPlayerKeys);

    /** Removes one party with a globally monotonic roster tombstone so delayed membership cannot be resurrected. */
    bool RemoveParty(const FGuid &PartyId, uint64 RosterRevision);

    /** Resolves a typed party identity for one persistent player, or returns false when the player is party-less. */
    bool TryResolveParty(const FString &PersistentPlayerKey,
                         FGuid &OutPartyId) const;

private:
    static bool IsPersistentPlayerKeyValid(const FString &PersistentPlayerKey);

    TMap<FString, FGuid> PartyByPersistentPlayer;
    uint64 HighestAppliedRosterRevision = 0;
};
