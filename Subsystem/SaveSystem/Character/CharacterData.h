#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SavedInventory.h"
#include "SavedProficiency.h"
#include "SavedObjective.h"
#include "SavedQuestJournal.h"
#include "SavedFactionStanding.h"
#include "Progression/MythicStatCounterTypes.h"
#include "Knowledge/MythicCodexTypes.h"
#include "GAS/Progression/MythicRenownTypes.h"
#include "GAS/Mounts/MythicMountTypes.h"
#include "World/Trading/MythicTradeContractTypes.h"
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceTypes.h"
#include "World/LivingWorld/Chronicle/MythicDossierComponent.h"
#include "CharacterData.generated.h"

UENUM(BlueprintType)
enum class EMythicCharacterSaveVersion : uint8 {
    InitialVersion = 0,
    PreFactionStandings,
    PreStoryTags,
    PrePerks,
    PreSkillLoadout,
    PreStatLedger,
    PreAchievements,
    PreUnlocks,
    PreCodex,
    PreMounts,
    PreAcquaintance,
    PreTradeContracts,
    PreQuestJournal,
    LatestVersion,
    VersionPlusOne
};

constexpr EMythicCharacterSaveVersion CurrentCharacterSaveVersion = EMythicCharacterSaveVersion::LatestVersion;

USTRUCT(BlueprintType)
struct FSerializedCharacterData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 DataVersion = static_cast<int32>(EMythicCharacterSaveVersion::InitialVersion);

    UPROPERTY(BlueprintReadWrite)
    FString CharacterID;

    UPROPERTY(BlueprintReadWrite)
    FString CharacterName;

    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedInventoryData> Inventories;

    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedProficiencyData> Proficiencies;

    // Per-player quest/objective progress (definition asset + count + completed). Persists the otherwise session-only
    // UObjectiveTracker so a reload doesn't wipe in-progress and completed quests.
    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedObjectiveData> Objectives;

    // Per-player faction reputation (faction DB index + standing). Persists the otherwise session-only
    // UMythicFactionStandingComponent::Standings so reputation survives a reload. Only valid, non-neutral entries are
    // stored (FSerializedFactionStandingHelper::ShouldPersist). Empty for pre-FactionStandings saves → no-op on load.
    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedFactionStandingData> FactionStandings;

    // Per-player narrative story tags (earned Story.*/Choice.*/Outcome.*/Faction.Sided.* tags). Persists the otherwise
    // session-only UMythicNarrativeStateComponent so a reload preserves branching choices. Snapshotted from
    // GetOwnedTags() on save and reapplied via ServerSetStoryTag on load. Empty for pre-StoryTags saves → no-op on load.
    UPROPERTY(BlueprintReadWrite)
    TArray<FGameplayTag> StoryTags;

    // Per-player lifetime meta-progression Stat.* counters. Persists the otherwise session-only
    // UMythicStatLedgerComponent character counters (kills, deaths, gold earned, discoveries, quests, deeds, ...) so a
    // reload preserves progress toward achievements/unlocks. Snapshotted from GetCharacterCounters() on save and
    // restored via RestoreCharacterCounters (which guards bIsRestoring) on load. Empty for pre-StatLedger saves ->
    // no-op on load. Mirrors the StoryTags/SlottedPerks save+restore precedent. Account counters are NOT persisted
    // (account persistence deferred).
    UPROPERTY(BlueprintReadWrite)
    TArray<FMythicStatCounter> StatCounters;

    // Per-player unlocked ACHIEVEMENT tags. Persists UMythicAchievementComponent::UnlockedAchievements so a reload keeps
    // earned achievements (and does NOT re-give their rewards — restore is guarded). The mirrored achievement tags also
    // live in StoryTags (they are stamped into the narrative ledger on unlock), so branching keys stay consistent. Empty
    // for pre-achievements saves → no-op on load.
    UPROPERTY(BlueprintReadWrite)
    FGameplayTagContainer UnlockedAchievements;

    // Per-player granted TITLE/COSMETIC tags (the tag-unlock engine's GrantTitle/GrantCosmetic effects). Persists
    // UMythicUnlockComponent::GrantedUnlockTags. Empty for pre-unlocks saves → no-op on load.
    UPROPERTY(BlueprintReadWrite)
    FGameplayTagContainer GrantedUnlockTags;

    // Stable ids of the unlock rules already applied — so a rule never re-applies (or re-gives its reward) across a
    // reload. Persists UMythicUnlockComponent::AppliedUnlockRules. Empty for pre-unlocks saves → no-op on load.
    UPROPERTY(BlueprintReadWrite)
    TArray<FGameplayTag> AppliedUnlockRules;

    // The player's selected active title (a subset of GrantedUnlockTags). Persists UMythicUnlockComponent::ActiveTitle.
    // Invalid for pre-unlocks saves → no active title on load.
    UPROPERTY(BlueprintReadWrite)
    FGameplayTag ActiveTitle;

    // Per-player codex bestiary progress (archetype key + kill count + encountered flag). Persists the otherwise
    // session-only UMythicCodexComponent::Bestiary so creature intel tiers survive a reload. Snapshotted from
    // GetAllBestiaryRecords() on save, restored via RestoreCodex (guarded by bIsRestoring — no re-toast) on load.
    // Empty for pre-Codex saves → no-op on load. Mirrors the StatCounters save+restore precedent.
    UPROPERTY(BlueprintReadWrite)
    TArray<FMythicBestiaryRecord> CodexBestiary;

    // Per-player discovered glossary terms (Codex.Term.* tags). Same component/precedent as CodexBestiary.
    UPROPERTY(BlueprintReadWrite)
    TArray<FGameplayTag> CodexTerms;

    // Per-player scoped RENOWN values (+ the global aggregate). Persists the otherwise session-only
    // UMythicRenownComponent so long-horizon reputation survives a reload. Keyed on stable scope TAGS (never faction DB
    // indices). Restored by DIRECT assignment (RestoreRenown) — no tier re-fires, no reward re-gives; the mirrored
    // Renown.* story tags replay through the StoryTags restore above.
    UPROPERTY(BlueprintReadWrite)
    TArray<FMythicRenownEntry> RenownEntries;

    UPROPERTY(BlueprintReadWrite)
    float GlobalRenown = 0.0f;

    // Per-player MOUNT ROSTER (tamed-mount records) + the active whistle selection. Persists the otherwise
    // session-only UMythicMountRosterComponent so tamed mounts survive a reload. Snapshotted from GetRoster()/
    // GetActiveMountId() on save, restored via RestoreRoster (empty = no-op, so pre-Mounts saves load cleanly; a
    // missing/invalid active id self-heals to the flagged/first record). Mirrors the StatCounters precedent. The
    // live summoned actor is NOT persisted — a mount is re-materialized from its record by the whistle (stateless).
    UPROPERTY(BlueprintReadWrite)
    TArray<FMythicMountRecord> MountRoster;

    UPROPERTY(BlueprintReadWrite)
    FGuid ActiveMountId;

    // Per-player NPC acquaintance/grudge ledger (per-NPC warmth relations, LRU-bounded). Persists the otherwise
    // session-only UMythicAcquaintanceComponent so NPCs still remember this player after a reload. Snapshotted from
    // GetRelations() on save, restored via RestoreRelations (empty = no-op, so pre-Acquaintance saves load cleanly).
    // Mirrors the MountRoster/StatCounters precedent.
    UPROPERTY(BlueprintReadWrite)
    TArray<FMythicNpcRelation> NpcRelations;

    // Per-player NPC dossiers (the player-scoped chronicle codex: times met, deeds, warmth, death + killer).
    // Persists the otherwise session-only UMythicDossierComponent. Same save+restore precedent as NpcRelations.
    UPROPERTY(BlueprintReadWrite)
    TArray<FMythicNpcDossier> NpcDossiers;

    // Per-player TRADE CONTRACTS (Wave O: accepted, still-active delivery contracts). Persists the otherwise
    // session-only UMythicTradeContractComponent so a relief run survives a reload. Snapshotted from GetContracts()
    // on save, restored via RestoreContracts (empty = no-op, so pre-Trading saves load cleanly). Mirrors the
    // MountRoster precedent.
    UPROPERTY(BlueprintReadWrite)
    TArray<FMythicTradeContract> TradeContracts;

    // Per-player QUEST JOURNAL — every tracked quest (definition soft path + its rolled-up terminal/live state). Persists
    // the otherwise session-only UMythicQuestJournalComponent::Quests so a reload preserves the journal UI, keeps
    // completed arcs completed, and — critically — restores the terminal-state LATCH so ApplyQuestCompleted (rewards/XP/
    // loot) is NOT re-run for an already-completed quest. Snapshotted via GetSerializableJournal on save, restored via
    // RestoreQuests (empty = no-op, so pre-QuestJournal saves load cleanly). Mirrors the Objectives block above.
    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedQuestJournalEntry> QuestJournal;

    // Active storyline (arc) soft paths — the arcs currently being advanced. The runtime carries no separate per-arc
    // position (arc progress is implicit in each quest's restored State), so the soft path is the whole persistent record.
    UPROPERTY(BlueprintReadWrite)
    TArray<FSoftObjectPath> ActiveStorylines;

    // Completed storyline (arc) soft paths — arcs whose arc-completion rewards already fired. Restored so a finished arc
    // stays finished and never re-grants its arc rewards.
    UPROPERTY(BlueprintReadWrite)
    TArray<FSoftObjectPath> CompletedStorylines;


    // Last world transform of the player's pawn, so a reload restores position/rotation instead of respawning at
    // the default PlayerStart. Gated by bHasSavedTransform so saves written before this field existed (which would
    // deserialize to Identity) don't teleport the player to the world origin.
    UPROPERTY(BlueprintReadWrite)
    FTransform SavedTransform = FTransform::Identity;

    UPROPERTY(BlueprintReadWrite)
    bool bHasSavedTransform = false;

    static bool Serialize(AActor *SourceActor, FSerializedCharacterData &OutData);

    static bool Deserialize(AActor *TargetActor, const FSerializedCharacterData &InData);
};
