#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "World/Harvesting/MythicHarvestReceiptTypes.h"

class AMythicPlayerController;

/**
 * Pure authority identity gate for harvest contribution commits.
 *
 * A session fallback is useful while a player connects, but it must never own persistent work, rewards, XP, or
 * quest credit. Acceptance therefore requires the loaded persistent character id, the PlayerState's live canonical
 * key, and the registry's reverse key for the controller to agree exactly. This also closes the synchronous rekey
 * boundary: a transaction observes either not-ready or one fully registered persistent identity, never a key that
 * can later split the contribution ledger.
 */
struct MYTHIC_API FMythicHarvestParticipantIdentityPolicy {
    /** Resolves the persistent contributor key only when character load and registry rekey are both complete. */
    static bool TryResolveReadyContributorKey(
        const FString &PersistentCharacterId,
        const FString &CanonicalPlayerKey,
        const FString &RegisteredControllerKey,
        FString &OutContributorKey);
};

/**
 * Authority-frozen inputs for one canonical harvest contributor.
 *
 * ContributorKey is the persistent character key accepted by FMythicHarvestParticipantIdentityPolicy. It is
 * captured only from live authoritative player state after registry rekey and is subsequently used solely with
 * UMythicPlayerRegistrySubsystem. Balance inputs are refreshed on that contributor's latest accepted work so
 * completion planning never depends on a live controller. CurrentController is only a delivery fast path and is
 * deliberately not identity.
 */
struct MYTHIC_API FMythicHarvestParticipantSnapshot {
    FString ContributorKey;
    int64 ContributionQuanta = 0;
    int32 ItemLevel = 1;
    int32 QuantityMultiplierQuanta = 0;
    int32 ProficiencyLevel = 0;
    /** First-hit-frozen typed work-XP semantics; unset is legal only before that hit's prepare phase completes. */
    FMythicHarvestWorkRewardContract WorkRewardContract;
    TWeakObjectPtr<AMythicPlayerController> CurrentController;

    /** Validates the frozen, controller-independent participant payload. A disconnected controller remains valid. */
    bool IsValid() const;
};

/** Pure canonical-ledger operations shared by authority and automation. */
struct MYTHIC_API FMythicHarvestParticipantLedger {
    /**
     * Adds one accepted-work snapshot by its opaque canonical key with checked integer arithmetic. Existing balance
     * inputs and the weak delivery fast path are refreshed from the latest authoritative accepted hit.
     */
    static bool TryAccumulate(
        TMap<FString, FMythicHarvestParticipantSnapshot> &InOutLedger,
        const FMythicHarvestParticipantSnapshot &AcceptedWorkSnapshot);

    /**
     * Produces a deterministic canonical-key-ordered eligible set. Weak controllers are intentionally ignored so a
     * disconnect after accepted work cannot erase reward, completion-XP, or typed quest-credit entitlement.
     */
    static bool BuildEligibleSnapshots(
        const TMap<FString, FMythicHarvestParticipantSnapshot> &Ledger,
        int64 MinimumContributionQuanta,
        TArray<FMythicHarvestParticipantSnapshot> &OutEligibleSnapshots,
        int64 *OutTotalEligibleContributionQuanta = nullptr);
};
