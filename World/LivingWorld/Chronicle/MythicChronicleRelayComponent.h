// Mythic Living World — World Chronicle replication relay (per-player).
// The World Chronicle subsystem runs SERVER-side (the sim only ticks on authority), so without a relay clients
// never receive the world-news feed. This component mirrors Mythic's established per-player replicated-feed
// pattern (UObjectiveTracker / UMythicFactionStandingComponent): a COND_OwnerOnly replicated TArray.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h" // FMythicChronicleEntry
#include "MythicChronicleRelayComponent.generated.h"

class UMythicWorldChronicleSubsystem;

/**
 * Hosted on AMythicPlayerController. On the SERVER it seeds the current chronicle backlog at BeginPlay (free
 * join-in-progress: the replicated array's initial state replicates to the owning client) and appends each newly
 * chronicled entry; on the OWNING CLIENT OnRep_ReplicatedChronicle feeds every not-yet-seen entry into the
 * client's own chronicle subsystem via IngestReplicatedEntry, so GetRecentChronicle / OnChronicleEntry behave
 * identically client-side with zero consumer changes. Owner-only so each client only carries its own copy.
 *
 * Listen-server hosts receive entries directly from the server subsystem (OnRep never fires on authority), so
 * there is no double-delivery and no listen-server gating is needed.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicChronicleRelayComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicChronicleRelayComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // SERVER: a new entry was chronicled — mirror it into the replicated feed (capped) for the owning client.
    UFUNCTION()
    void HandleChronicleEntry(const FMythicChronicleEntry &Entry);

    // CLIENT: the replicated feed changed — ingest entries newer than the last forwarded into the local subsystem.
    UFUNCTION()
    void OnRep_ReplicatedChronicle();

    // Owner-only replicated mirror of the server chronicle feed (capped, oldest-first by Sequence).
    UPROPERTY(ReplicatedUsing = OnRep_ReplicatedChronicle)
    TArray<FMythicChronicleEntry> ReplicatedChronicle;

private:
    // Resolve the per-process chronicle subsystem (same accessor on server + client).
    UMythicWorldChronicleSubsystem *ResolveChronicle() const;

    // CLIENT: highest Sequence already ingested locally — dedup key across the capped (index-shifting) array.
    int32 LastIngestedSequence = 0;

    // Match the subsystem's rolling-buffer cap so the replicated feed doesn't grow unbounded.
    static constexpr int32 MaxRelayEntries = 256;
};
