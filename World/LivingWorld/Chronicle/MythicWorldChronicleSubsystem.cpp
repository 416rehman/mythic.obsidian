#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h"

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UMythicWorldChronicleSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Collection.InitializeDependency<UMythicLivingWorldSubsystem>();

    Super::Initialize(Collection);

    if (UGameInstance *GI = GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            LivingWorld = LWS;
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

    const uint16 MacroMask = EMythicEventCategory::Diplomacy | EMythicEventCategory::Territory
        | EMythicEventCategory::Scheme | EMythicEventCategory::Environment | EMythicEventCategory::Death
        | EMythicEventCategory::Encounter;

    const uint32 NewestId = Fabric->GetTotalEventCount();
    if (!bSeeded) {
        LastSeenEventId = (NewestId > 0) ? NewestId - 1 : 0;
        bSeeded = true;
        return;
    }
    if (NewestId == 0 || (NewestId - 1) <= LastSeenEventId) {
        return;
    }

    const int32 Unseen = static_cast<int32>((NewestId - 1) - LastSeenEventId);
    const int32 Want = FMath::Min(Unseen, Fabric->GetCapacity());
    const TArray<FMythicWorldEvent> Recent = Fabric->GetRecentEvents(Want);

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

    Fresh.Sort([](const FMythicWorldEvent &A, const FMythicWorldEvent &B) { return A.EventId < B.EventId; });

    for (const FMythicWorldEvent *E : Fresh) {
        FMythicChronicleEntry Entry;
        Entry.Text = FormatEvent(*E, LWS);
        Entry.EventTag = E->EventTag;
        Entry.WorldTime = static_cast<float>(E->WorldTime);
        Entry.Significance = E->Significance;
        Entry.Sequence = NextSequence++;

        AppendEntry(Entry);
    }
}

void UMythicWorldChronicleSubsystem::AppendEntry(const FMythicChronicleEntry &Entry) {
    Entries.Add(Entry);
    OnChronicleEntry.Broadcast(Entry);

    if (Entries.Num() > MaxEntries) {
        Entries.RemoveAt(0, Entries.Num() - MaxEntries, EAllowShrinking::No);
    }
}

void UMythicWorldChronicleSubsystem::IngestReplicatedEntry(const FMythicChronicleEntry &Entry) {
    if (Entry.Sequence != 0 && Entry.Sequence <= LastIngestedSequence) {
        return;
    }
    LastIngestedSequence = FMath::Max(LastIngestedSequence, Entry.Sequence);

    AppendEntry(Entry);
}

FString UMythicWorldChronicleSubsystem::EventTagToReadable(const FGameplayTag &Tag) {
    if (!Tag.IsValid()) {
        return TEXT("World Event");
    }
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
    const FString EventName = EventTagToReadable(Event.EventTag);

    FString SubjectName;
    if ((Event.CategoryFlags & EMythicEventCategory::Death) != 0 && Event.PerpEntityId != 0) {
        SubjectName = FMythicNPCGenerator::ReconstructNameFromHash(Event.PerpEntityId, Event.PrimaryFaction.Index).ToString();
    }

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
