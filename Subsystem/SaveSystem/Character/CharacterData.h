#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SavedInventory.h"
#include "SavedProficiency.h"
#include "SavedObjective.h"
#include "SavedQuestJournal.h"
#include "SavedFactionStanding.h"
#include "Progression/MythicStatCounterTypes.h"
#include "Progression/Skills/MythicSkillProgressTypes.h"
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
    PreWholeNumberRolls,
    PreRunes,
    PreSkills,
    PreSkillModifiers,
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

    // Per-player RUNE SOCKETS — the rune worn in each socket (definition soft path; a null entry is an empty socket, so
    // the array index is the slot index) plus how many sockets are open. Persists UMythicRuneComponent, whose SaveGame
    // flags are inert here (no component archive targets the PlayerState). Without this a reload strips every rune AND
    // leaves the sockets shut for good, because the GrantPerkSlot rules that opened them are latched in
    // AppliedUnlockRules and never re-fire. Restored via RestoreRunes, which re-grants each rune's passive ability.
    // Zero open sockets means the count is absent, which FixupData repairs before load rather than treating as one
    // socket: GrantPerkSlot was a no-op before this existed, yet its rules still latched, so a save that earned
    // sockets carries the rules and no count.
    UPROPERTY(BlueprintReadWrite)
    TArray<FSoftObjectPath> EquippedRunes;

    UPROPERTY(BlueprintReadWrite)
    int32 UnlockedRuneSlots = 0;

    // Per-player SKILL SLOTS — the skill bound to each slot (definition soft path; a null entry is an empty slot, so
    // the array index is the slot index) plus how many slots are open. Persists UMythicSkillComponent, whose SaveGame
    // flags are inert here for the same reason the rune ones are. Restored via RestoreSkills, which re-grants each
    // skill's ability under that slot's input tag — restoring the array alone would leave filled slots whose keys do
    // nothing. Zero open slots means the count is absent, which FixupData repairs before load.
    UPROPERTY(BlueprintReadWrite)
    TArray<FSoftObjectPath> EquippedSkills;

    UPROPERTY(BlueprintReadWrite)
    int32 UnlockedSkillSlots = 0;

    // Per-player SKILL GROWTH — one row per skill that has been levelled or has a modifier switched on, keyed on the
    // definition rather than a slot so a skill keeps its build while unequipped. Persists UMythicSkillComponent's
    // levels and active modifier indices. Empty for pre-SkillModifiers saves, which is exactly right: every skill
    // reads back at level one with nothing active. Restored via RestoreSkillProgress, which re-runs the capacity and
    // point gates rather than trusting the file.
    UPROPERTY(BlueprintReadWrite)
    TArray<FMythicSkillProgress> SkillProgress;

    // How many modifiers one skill may carry at once. Zero means the count is absent, which FixupData rebuilds from
    // the applied Unlock.Rule.SkillModifier* rules before load — the same trap the rune sockets hit, since those
    // rules latch in AppliedUnlockRules and never fire twice.
    UPROPERTY(BlueprintReadWrite)
    int32 SkillModifierCapacity = 0;


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

/** Repairs for saves written before a field existed, kept pure so each is testable without a save file. */
struct MYTHIC_API FMythicCharacterSaveMigration {
    /**
     * How many rune sockets a pre-Runes save had earned. GrantPerkSlot did nothing back then, but its rules still
     * latched into AppliedUnlockRules and RestoreUnlockState re-latches them, so the count has to be rebuilt from
     * the rules or those sockets never reopen. One socket is free; each applied Unlock.Rule.RuneSlot* adds another.
     */
    static int32 RuneSlotsFromAppliedRules(TConstArrayView<FGameplayTag> AppliedRules) {
        static const FName RuleParent(TEXT("Unlock.Rule.RuneSlot"));
        int32 Slots = 1;
        for (const FGameplayTag &Rule : AppliedRules) {
            if (Rule.IsValid() && Rule.GetTagName().ToString().StartsWith(RuleParent.ToString(), ESearchCase::CaseSensitive)) {
                ++Slots;
            }
        }
        return Slots;
    }

    /**
     * How many skill slots a pre-Skills save had earned. Same trap as the rune sockets: GrantSkillSlot was a no-op,
     * yet DA_Unlock_SkillSlot2 still latched Unlock.Rule.SkillSlot2 into AppliedUnlockRules and RestoreUnlockState
     * re-latches it, so the rule can never fire again and the count has to be rebuilt from the rules. One slot is
     * free; each applied Unlock.Rule.SkillSlot* adds another.
     */
    static int32 SkillSlotsFromAppliedRules(TConstArrayView<FGameplayTag> AppliedRules) {
        static const FName RuleParent(TEXT("Unlock.Rule.SkillSlot"));
        int32 Slots = 1;
        for (const FGameplayTag &Rule : AppliedRules) {
            if (Rule.IsValid() && Rule.GetTagName().ToString().StartsWith(RuleParent.ToString(), ESearchCase::CaseSensitive)) {
                ++Slots;
            }
        }
        return Slots;
    }

    /**
     * How many modifiers a pre-SkillModifiers save had earned the right to carry at once. Rebuilt from the rules for
     * the same reason the two counts above are: an applied rule is latched and never fires again. One is free; each
     * applied Unlock.Rule.SkillModifier* adds another.
     *
     * Unlock.Rule.SkillSlot* shares the Unlock.Rule parent and must not be counted here — a loose prefix would hand
     * out a modifier slot for every ability slot.
     */
    static int32 SkillModifierCapacityFromAppliedRules(TConstArrayView<FGameplayTag> AppliedRules) {
        static const FName RuleParent(TEXT("Unlock.Rule.SkillModifier"));
        int32 Capacity = 1;
        for (const FGameplayTag &Rule : AppliedRules) {
            if (Rule.IsValid() && Rule.GetTagName().ToString().StartsWith(RuleParent.ToString(), ESearchCase::CaseSensitive)) {
                ++Capacity;
            }
        }
        return Capacity;
    }
};
