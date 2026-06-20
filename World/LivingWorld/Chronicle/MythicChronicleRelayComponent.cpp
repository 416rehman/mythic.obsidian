#include "World/LivingWorld/Chronicle/MythicChronicleRelayComponent.h"

#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UMythicChronicleRelayComponent::UMythicChronicleRelayComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicChronicleRelayComponent::BeginPlay() {
    Super::BeginPlay();

    // SERVER only: subscribe for live entries, THEN seed the current backlog. Subscribe-before-seed means a
    // joining client never misses an entry chronicled around the seed (the seed assignment re-includes anything
    // already chronicled; the subscription catches everything after). On the game thread these run atomically
    // w.r.t. chronicle processing, and the client dedups by Sequence regardless. Clients receive all of this via
    // replication + OnRep, so they do nothing here.
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    UMythicWorldChronicleSubsystem *Chronicle = ResolveChronicle();
    if (!Chronicle) {
        return;
    }
    Chronicle->OnChronicleEntry.AddDynamic(this, &UMythicChronicleRelayComponent::HandleChronicleEntry);
    // Backlog: copy the recent feed (oldest-first) so a joining client starts populated via initial replication.
    ReplicatedChronicle = Chronicle->GetRecentChronicle(MaxRelayEntries);
}

void UMythicChronicleRelayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (GetOwner() && GetOwner()->HasAuthority()) {
        if (UMythicWorldChronicleSubsystem *Chronicle = ResolveChronicle()) {
            Chronicle->OnChronicleEntry.RemoveDynamic(this, &UMythicChronicleRelayComponent::HandleChronicleEntry);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void UMythicChronicleRelayComponent::HandleChronicleEntry(const FMythicChronicleEntry &Entry) {
    // SERVER: mirror into the replicated feed. In-place mutation of a replicated TArray replicates on the next net
    // update (same pattern as ObjectiveTracker::ActiveObjectives). Cap to match the subsystem's rolling buffer.
    ReplicatedChronicle.Add(Entry);
    if (ReplicatedChronicle.Num() > MaxRelayEntries) {
        ReplicatedChronicle.RemoveAt(0, ReplicatedChronicle.Num() - MaxRelayEntries, EAllowShrinking::No);
    }
}

void UMythicChronicleRelayComponent::OnRep_ReplicatedChronicle() {
    // CLIENT: ingest only entries newer than the last forwarded. The feed is capped (RemoveAt shifts indices), so
    // Sequence — not array position — is the stable dedup key. Entries arrive ascending by Sequence (append order),
    // so a running advance also drops any duplicate Sequence safely. Feeds the SAME subsystem the UI reads.
    UMythicWorldChronicleSubsystem *Chronicle = ResolveChronicle();
    if (!Chronicle) {
        return;
    }
    for (const FMythicChronicleEntry &Entry : ReplicatedChronicle) {
        if (Entry.Sequence > LastIngestedSequence) {
            Chronicle->IngestReplicatedEntry(Entry);
            LastIngestedSequence = Entry.Sequence;
        }
    }
}

UMythicWorldChronicleSubsystem *UMythicChronicleRelayComponent::ResolveChronicle() const {
    if (const UWorld *World = GetWorld()) {
        if (UGameInstance *GI = World->GetGameInstance()) {
            return GI->GetSubsystem<UMythicWorldChronicleSubsystem>();
        }
    }
    return nullptr;
}

void UMythicChronicleRelayComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicChronicleRelayComponent, ReplicatedChronicle, COND_OwnerOnly);
}
