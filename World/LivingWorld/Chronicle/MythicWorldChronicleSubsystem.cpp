#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h"

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h" // ReconstructNameFromHash — named permanent-death beats
#include "Engine/GameInstance.h"
#include "Engine/World.h" // World->GetNetMode() / NM_Client for the client authority gate in HandleWorldSimCommitted

void UMythicWorldChronicleSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    // Ensure the living world subsystem is initialized first so its OnWorldSimCommitted delegate exists to bind.
    Collection.InitializeDependency<UMythicLivingWorldSubsystem>();

    Super::Initialize(Collection);

    if (UGameInstance *GI = GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            LivingWorld = LWS;
            // The delegate fires only where the sim commits (server / standalone); harmless to bind on clients
            // (it simply never fires there until feed replication is added — a follow-up).
            CommitHandle = LWS->OnWorldSimCommitted.AddUObject(this, &UMythicWorldChronicleSubsystem::HandleWorldSimCommitted);
        }
    }
}

void UMythicWorldChronicleSubsystem::Deinitialize() {
    if (UMythicLivingWorldSubsystem *LWS = LivingWorld.Get()) {
        if (CommitHandle.IsValid()) {
            LWS->OnWorldSimCommitted.Remove(CommitHandle);
        }
    }
    CommitHandle.Reset();
    LivingWorld.Reset();

    Super::Deinitialize();
}

void UMythicWorldChronicleSubsystem::HandleWorldSimCommitted() {
    // AUTHORITY GATE: the living-world sim is authoritative — only the server/standalone/listen-server originates
    // chronicle entries. A true client receives the feed exclusively via UMythicChronicleRelayComponent
    // (COND_OwnerOnly) -> IngestReplicatedEntry; if it ALSO chronicled locally it would append DIVERGENT entries and
    // evict real replicated ones from the capped rolling buffer. A GameInstanceSubsystem has no HasAuthority(), so we
    // read the net mode off the owning GameInstance's world (same accessor the rest of the living world uses).
    if (const UGameInstance *GI = GetGameInstance()) {
        if (const UWorld *World = GI->GetWorld()) {
            if (World->GetNetMode() == NM_Client) {
                return;
            }
        }
    }

    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    if (!LWS || !LWS->IsSystemActive()) {
        return;
    }
    UMythicCausalFabric *Fabric = LWS->GetCausalFabric();
    if (!Fabric) {
        return;
    }

    // Player-facing MACRO categories only — the dramatic world beats, not every combat/trade tick.
    const uint16 MacroMask = EMythicEventCategory::Diplomacy | EMythicEventCategory::Territory
        | EMythicEventCategory::Scheme | EMythicEventCategory::Environment | EMythicEventCategory::Death
        | EMythicEventCategory::Encounter;

    // Seed past the pre-existing backlog on the first commit so the chronicle is a "news feed from now" rather
    // than dumping the entire world history (and flooding OnChronicleEntry) the first time it observes the sim.
    const uint32 NewestId = Fabric->GetTotalEventCount(); // == NextEventId; newest assigned id = NewestId - 1
    if (!bSeeded) {
        LastSeenEventId = (NewestId > 0) ? NewestId - 1 : 0;
        bSeeded = true;
        return;
    }
    if (NewestId == 0 || (NewestId - 1) <= LastSeenEventId) {
        return; // nothing new since the last commit
    }

    // Request EXACTLY the unseen window (capped to the ring), so a burst commit of many events isn't silently
    // skipped by advancing LastSeenEventId past un-ingested ids. GetRecentEvents returns an owned COPY taken under
    // the fabric read lock — no aliasing of the sim-thread-written buffer.
    const int32 Unseen = static_cast<int32>((NewestId - 1) - LastSeenEventId);
    const int32 Want = FMath::Min(Unseen, Fabric->GetCapacity());
    const TArray<FMythicWorldEvent> Recent = Fabric->GetRecentEvents(Want);

    // Collect only events newer than the last we processed (EventIds are monotonic), and track the highest id
    // across ALL of them so insignificant/non-macro events are also marked seen (never re-examined).
    uint32 MaxId = LastSeenEventId;
    TArray<const FMythicWorldEvent *> Fresh;
    for (const FMythicWorldEvent &E : Recent) {
        if (E.EventId <= LastSeenEventId) {
            continue;
        }
        MaxId = FMath::Max(MaxId, E.EventId);
        if (E.Significance < MinSignificance) {
            continue;
        }
        if ((E.CategoryFlags & MacroMask) == 0) {
            continue;
        }
        Fresh.Add(&E);
    }
    LastSeenEventId = MaxId;

    if (Fresh.Num() == 0) {
        return;
    }

    // Chronological order regardless of the ring's internal layout.
    Fresh.Sort([](const FMythicWorldEvent &A, const FMythicWorldEvent &B) { return A.EventId < B.EventId; });

    for (const FMythicWorldEvent *E : Fresh) {
        FMythicChronicleEntry Entry;
        Entry.Text = FormatEvent(*E, LWS);
        Entry.EventTag = E->EventTag;
        Entry.WorldTime = static_cast<float>(E->WorldTime);
        Entry.Significance = E->Significance;
        Entry.Sequence = NextSequence++; // server-assigned; the per-player relay carries it to clients as a dedup key

        AppendEntry(Entry);
    }
}

void UMythicWorldChronicleSubsystem::AppendEntry(const FMythicChronicleEntry &Entry) {
    Entries.Add(Entry);
    OnChronicleEntry.Broadcast(Entry);

    // Cap the rolling buffer (drop oldest). Idempotent — running it per-append keeps the final state identical to
    // a single post-loop cap, so the server (burst) and client (per-entry) paths share one code path.
    if (Entries.Num() > MaxEntries) {
        Entries.RemoveAt(0, Entries.Num() - MaxEntries, EAllowShrinking::No);
    }
}

void UMythicWorldChronicleSubsystem::IngestReplicatedEntry(const FMythicChronicleEntry &Entry) {
    // CLIENT path: the entry already carries its server-assigned Sequence (do NOT touch NextSequence here — the
    // client never originates entries). Dedup at the subsystem (which persists across PlayerController/relay
    // re-creation), so a re-seeded backlog after non-seamless travel / reconnect isn't appended twice.
    if (Entry.Sequence != 0 && Entry.Sequence <= LastIngestedSequence) {
        return; // already ingested
    }
    LastIngestedSequence = FMath::Max(LastIngestedSequence, Entry.Sequence);

    // Single-source with the server via AppendEntry.
    AppendEntry(Entry);
}

FString UMythicWorldChronicleSubsystem::EventTagToReadable(const FGameplayTag &Tag) {
    if (!Tag.IsValid()) {
        return TEXT("World Event");
    }
    // Render the last TWO segments of a "Domain.Event.Category.Leaf" tag so the CATEGORY survives. A bare leaf like
    // "Completed" COLLIDES across families (Scheme.Completed vs Encounter.Completed) and reads ambiguously in the feed
    // + NPC {recent_event} dialogue. The parent is prepended only when it is a meaningful category (not a generic
    // container "Event"/"LivingWorld"/"World"), so a plain "Event.X" still reads "X". Single source for FormatEvent
    // AND the brain's {recent_event}. e.g. "...Event.Scheme.Completed" -> "Scheme Completed".
    const FString Name = Tag.GetTagName().ToString();
    int32 DotIdx = INDEX_NONE;
    if (!Name.FindLastChar(TEXT('.'), DotIdx)) {
        return Name;
    }
    const FString Leaf = Name.RightChop(DotIdx + 1);
    const FString Head = Name.Left(DotIdx);
    int32 PrevDot = INDEX_NONE;
    const FString Parent = Head.FindLastChar(TEXT('.'), PrevDot) ? Head.RightChop(PrevDot + 1) : Head;
    if (!Parent.IsEmpty() && Parent != TEXT("Event") && Parent != TEXT("LivingWorld") && Parent != TEXT("World")) {
        return Parent + TEXT(" ") + Leaf;
    }
    return Leaf;
}

FText UMythicWorldChronicleSubsystem::FormatEvent(const FMythicWorldEvent &Event, UMythicLivingWorldSubsystem *LWS) const {
    // Readable event name = the tag's leaf (single source — also used by the brain's {recent_event} dialogue variable).
    const FString EventName = EventTagToReadable(Event.EventTag);

    // For a permanent-death beat, lead with the dead NPC's reconstructed name. Single-source: the SAME pure static
    // ReconstructNameFromHash that the brain's GetDisplayName uses — deterministic, needs NO live entity (the NPC is
    // gone by now) and is stable across save/load. PerpEntityId carries the dead NPC's NameHash; PrimaryFaction is theirs.
    FString SubjectName;
    if ((Event.CategoryFlags & EMythicEventCategory::Death) != 0 && Event.PerpEntityId != 0) {
        SubjectName = FMythicNPCGenerator::ReconstructNameFromHash(Event.PerpEntityId, Event.PrimaryFaction.Index).ToString();
    }

    // Resolve faction display names (objective, real data — no fabricated text).
    auto FactionName = [LWS](const FMythicFactionId &Id) -> FString {
        if (LWS && Id.IsValid()) {
            if (UMythicFactionDatabase *DB = LWS->GetFactionDatabase()) {
                FMythicFactionData Data;
                if (DB->GetFaction(Id, Data)) {
                    return Data.DisplayName.ToString();
                }
            }
        }
        return FString();
    };
    const FString Primary = FactionName(Event.PrimaryFaction);
    const FString Secondary = FactionName(Event.SecondaryFaction);

    // Named permanent-death beat: "<Name> — Death Permanent (<Faction>)" or "<Name> — Death Permanent".
    if (!SubjectName.IsEmpty()) {
        if (!Primary.IsEmpty()) {
            return FText::FromString(FString::Printf(TEXT("%s — %s (%s)"), *SubjectName, *EventName, *Primary));
        }
        return FText::FromString(FString::Printf(TEXT("%s — %s"), *SubjectName, *EventName));
    }

    if (!Primary.IsEmpty() && !Secondary.IsEmpty()) {
        return FText::FromString(FString::Printf(TEXT("%s — %s and %s"), *EventName, *Primary, *Secondary));
    }
    if (!Primary.IsEmpty()) {
        return FText::FromString(FString::Printf(TEXT("%s — %s"), *EventName, *Primary));
    }
    return FText::FromString(EventName);
}

TArray<FMythicChronicleEntry> UMythicWorldChronicleSubsystem::GetRecentChronicle(int32 MaxCount) const {
    if (MaxCount <= 0 || Entries.Num() == 0) {
        return TArray<FMythicChronicleEntry>();
    }
    const int32 Start = FMath::Max(0, Entries.Num() - MaxCount);
    TArray<FMythicChronicleEntry> Result;
    Result.Reserve(Entries.Num() - Start);
    for (int32 i = Start; i < Entries.Num(); ++i) {
        Result.Add(Entries[i]);
    }
    return Result;
}
