// Mythic Living World — World Chronicle
// Surfaces the living-world sim's significant macro-events (wars, schisms, scheme outcomes, famines, deaths) to
// the player. The CausalFabric records these every tick but nothing player-facing read them; this is the read
// side. The SERVER ingests from the sim/fabric (authority); the per-player UMythicChronicleRelayComponent
// replicates the feed to each owning client (COND_OwnerOnly), which calls IngestReplicatedEntry so the SAME
// client-side subsystem (GetRecentChronicle / OnChronicleEntry) is populated identically to the server.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "MythicWorldChronicleSubsystem.generated.h"

class UMythicLivingWorldSubsystem;
struct FMythicWorldEvent;

/** One player-facing chronicle line, distilled from an objective FMythicWorldEvent. */
USTRUCT(BlueprintType)
struct FMythicChronicleEntry {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Chronicle")
    FText Text;

    UPROPERTY(BlueprintReadOnly, Category = "Chronicle")
    FGameplayTag EventTag;

    UPROPERTY(BlueprintReadOnly, Category = "Chronicle")
    float WorldTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Chronicle")
    float Significance = 0.0f;

    // Server-assigned monotonic id. Used by the per-player replication relay to ingest only NEW entries on the
    // client (the replicated feed is capped, so RemoveAt shifts indices — a stable sequence id is the dedup key).
    UPROPERTY()
    int32 Sequence = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnChronicleEntry, const FMythicChronicleEntry &, Entry);

/**
 * Listens for sim commits (UMythicLivingWorldSubsystem::OnWorldSimCommitted, game thread) and, for newly-recorded
 * significant macro-events (Diplomacy / Territory / Scheme / Environment / Death above a significance threshold),
 * formats a readable line and appends it to a rolling chronicle + broadcasts OnChronicleEntry for UI toasts/feeds.
 * Read-only over the CausalFabric — no sim mutation, no new contract.
 */
UCLASS()
class MYTHIC_API UMythicWorldChronicleSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    // Fired (game thread) when a new significant world event is chronicled — for a UI toast / news ticker.
    UPROPERTY(BlueprintAssignable, Category = "World Chronicle")
    FMythicOnChronicleEntry OnChronicleEntry;

    // The most recent chronicle entries, oldest-first (for a chronicle / world-news panel).
    UFUNCTION(BlueprintCallable, Category = "World Chronicle")
    TArray<FMythicChronicleEntry> GetRecentChronicle(int32 MaxCount = 20) const;

    // CLIENT-side single-source entry point: the per-player replication relay (UMythicChronicleRelayComponent)
    // calls this for each replicated entry the client hasn't ingested yet. Routes through the same AppendEntry as
    // the server path, so GetRecentChronicle + OnChronicleEntry behave identically on client and server. Game
    // thread only (replication callbacks are). Entries arrive with their server-assigned Sequence intact.
    void IngestReplicatedEntry(const FMythicChronicleEntry &Entry);

    // Only events at or above this significance are chronicled (macro events, not every combat tick).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "World Chronicle")
    float MinSignificance = 0.5f;

    /** Distill an event gameplay tag into its readable leaf (e.g. "LivingWorld.Event.Faction.Resistance" -> "Resistance";
     *  an invalid tag -> "World Event"). Single source for FormatEvent AND the brain's {recent_event} dialogue variable. */
    static FString EventTagToReadable(const FGameplayTag &Tag);

protected:
    // Bound to the LWS commit delegate; ingests new fabric events on the game thread.
    void HandleWorldSimCommitted();

    // Distill one event into a readable line (tag leaf + faction display names). Generic + grounded; a designer
    // EventTag->template map is a polish follow-up.
    FText FormatEvent(const FMythicWorldEvent &Event, UMythicLivingWorldSubsystem *LWS) const;

    // Single source for "add a finished entry to the rolling feed": append, broadcast OnChronicleEntry, cap.
    // Used by BOTH the server ingest (HandleWorldSimCommitted) and the client ingest (IngestReplicatedEntry).
    // Game thread only — Entries / OnChronicleEntry have no thread guard.
    void AppendEntry(const FMythicChronicleEntry &Entry);

private:
    TWeakObjectPtr<UMythicLivingWorldSubsystem> LivingWorld;
    FDelegateHandle CommitHandle;

    // Dedupe: only ingest events with EventId greater than the highest already seen (EventIds are monotonic).
    uint32 LastSeenEventId = 0;

    // On the first commit we seed LastSeenEventId to the current newest id (skip pre-existing history) so the
    // chronicle is a feed of events from when it started observing, not a one-shot dump of the whole backlog.
    bool bSeeded = false;

    // Server-assigned monotonic sequence stamped onto each entry as it is chronicled (server path only). The
    // per-player replication relay carries it to clients as the dedup key. Starts at 1 (0 = "never sequenced").
    int32 NextSequence = 1;

    // CLIENT-side highest Sequence already ingested via IngestReplicatedEntry. The relay component owns a cursor
    // too, but the relay is recreated with the PlayerController (non-seamless travel / reconnect) while THIS
    // subsystem persists for the GameInstance lifetime — so the durable dedup must live here, or a re-seeded
    // backlog would be appended a second time. 0 = nothing ingested yet.
    int32 LastIngestedSequence = 0;

    // Rolling buffer of chronicled entries (oldest-first); capped.
    TArray<FMythicChronicleEntry> Entries;
    static constexpr int32 MaxEntries = 256;
};
