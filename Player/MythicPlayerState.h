#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Itemization/InventoryProviderInterface.h"
#include "MythicPlayerState.generated.h"

class UMythicAttributeSet_Life;
class UMythicAttributeSet_Offense;
class UMythicAttributeSet_Proficiencies;

class UMythicAttributeSet_Utility;
class UMythicAttributeSet_Defense;
class UMythicAttributeSet_Survival;
class UMythicSurvivalComponent;
class UMythicFactionStandingComponent;
class UMythicNarrativeStateComponent;
class UMythicQuestJournalComponent;
class UMythicStatLedgerComponent;
class UMythicAchievementComponent;
class UMythicUnlockComponent;
class UMythicRuneComponent;
class UMythicCodexComponent;
class UMythicRenownComponent;
class UMythicMountRosterComponent;
class UMythicDialogueComponent;
class UMythicAcquaintanceComponent;
class UMythicDossierComponent;
class UMythicTradeContractComponent;
UCLASS()
class MYTHIC_API AMythicPlayerState : public APlayerState, public IAbilitySystemInterface, public IInventoryProviderInterface {
    GENERATED_BODY()

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System")
    TObjectPtr<UMythicAbilitySystemComponent> MythicAbilitySystemComponent;

    // Default Ability Set
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UMythicGameplayAbility>> DefaultAbilities;

    // Default Gameplay Effects - Can also be used to initialize attributes.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
    TArray<TSubclassOf<UGameplayEffect>> DefaultGameplayEffects;

    // Life attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Life *LifeAttributes;

    // Offense attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Offense *OffenseAttributes;

    // Defense attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Defense *DefenseAttributes;

    // Utility attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Utility *UtilityAttributes;


    // Proficiency attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Proficiencies *ProficiencyAttributes;

    // Survival attributes (Nourishment/Hydration/Warmth/Wetness). Auto-registered on the ASC like the other sets;
    // replicated COND_OwnerOnly (only the owning player needs their own survival needs — co-op perf-safe).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System")
    UMythicAttributeSet_Survival *SurvivalAttributes;

    // Per-player survival-needs driver (server-authoritative; ONE repeating timer, no Tick). Reads the existing weather/
    // hazard tags; applies threshold GEs. Whole tick early-outs when the master switch is off (default = today's behaviour).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
    TObjectPtr<UMythicSurvivalComponent> SurvivalComponent;

    // Per-player faction standing (replicated component; drives NPC attitude toward this player).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
    TObjectPtr<UMythicFactionStandingComponent> FactionStanding;

    // Per-player narrative ledger (replicated component; earned story tags driving tag-gated branching). Mirrors the
    // FactionStanding component — server-authoritative, replicated COND_OwnerOnly.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative")
    TObjectPtr<UMythicNarrativeStateComponent> NarrativeState;

    // Per-player quest journal (replicated component) — the Storyline>Quest>Task aggregation/state machine. Reads task
    // states from the ObjectiveTracker (on the controller), rolls them into per-quest states, grants quest/outcome/arc
    // rewards + tags. Server-authoritative, replicated COND_OwnerOnly (mirrors the ledger).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative")
    TObjectPtr<UMythicQuestJournalComponent> QuestJournal;


    // Per-player META-PROGRESSION stat ledger (replicated COND_OwnerOnly component; server-authoritative lifetime
    // Stat.* counters). Mirrors the FactionStanding/Narrative components — created in the ctor, persisted via
    // FSerializedCharacterData::StatCounters. Achievements + the tag-unlock engine ride on it.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
    TObjectPtr<UMythicStatLedgerComponent> StatLedger;

    // Per-player ACHIEVEMENTS (replicated COND_OwnerOnly component; server-authoritative unlocked-achievement tag set).
    // Rides on the stat ledger + narrative ledger — created in the ctor beside StatLedger, persisted via
    // FSerializedCharacterData::UnlockedAchievements. Event-driven; unlocks mirror into the narrative ledger + feed the
    // unlock engine.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
    TObjectPtr<UMythicAchievementComponent> Achievements;

    // Per-player TAG-UNLOCK ENGINE (replicated component; server-authoritative granted title/cosmetic tags + applied
    // rules + active title). Created in the ctor beside StatLedger, persisted via FSerializedCharacterData
    // (GrantedUnlockTags / AppliedUnlockRules / ActiveTitle). Turns earned tags/achievements into concrete unlocks.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
    TObjectPtr<UMythicUnlockComponent> Unlocks;

    // Per-player RUNE SOCKETS (replicated COND_OwnerOnly component; server-authoritative equipped runes + unlocked
    // slot count, and the ability grant/clear that rides each socket). Created in the ctor beside Unlocks, whose
    // GrantPerkSlot effect opens sockets two through four.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
    TObjectPtr<UMythicRuneComponent> Runes;

    // Per-player CODEX (replicated COND_OwnerOnly component; server-authoritative bestiary kill/encounter records +
    // discovered glossary terms). Mirrors the StatLedger/Narrative components — created in the ctor, persisted via
    // FSerializedCharacterData::CodexBestiary/CodexTerms. Content (lore/art/thresholds) resolves client-side through
    // UMythicCodexRegistry; only PROGRESS lives here.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Knowledge")
    TObjectPtr<UMythicCodexComponent> Codex;

    // Per-player RENOWN (replicated COND_OwnerOnly component; server-authoritative scoped-reputation values + the
    // global aggregate). Mirrors the StatLedger/Narrative components — created in the ctor, persisted via
    // FSerializedCharacterData::RenownEntries/GlobalRenown. Tier crossings mirror Renown.* story tags into the
    // narrative ledger (feeding the unlock rules) and grant authored tier payloads.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
    TObjectPtr<UMythicRenownComponent> Renown;

    // Per-player MOUNT ROSTER (replicated COND_OwnerOnly component; server-authoritative tamed-mount records + active
    // selection + whistle-summon/stash verbs). Mirrors the StatLedger/Perk components — created in the ctor, persisted
    // via FSerializedCharacterData::MountRoster/ActiveMountId. Lives on the PlayerState (not the do-not-edit PC) so the
    // whistle RPC rides an owned, replicated component.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mounts")
    TObjectPtr<UMythicMountRosterComponent> MountRoster;

    // Per-player DIALOGUE SESSION (replicated COND_OwnerOnly component; server-authoritative branching-conversation
    // state — resolves an NPC's authored graph, gates nodes/choices on the narrative ledger's story tags, applies
    // choice consequences: tags/rewards/quest offers). Created in the ctor like every sibling component — the ROBUST
    // replication path (a runtime AddComponent'd replicated component was the recon fallback; the ctor is editable, so
    // the default-subobject path that all 15 other components already prove out is the right one). Session-only state:
    // nothing persists (story CONSEQUENCES persist via the narrative ledger).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative")
    TObjectPtr<UMythicDialogueComponent> DialogueComponent;

    // Per-player ACQUAINTANCE/GRUDGE ledger (replicated COND_OwnerOnly component; server-authoritative per-NPC warmth
    // relations, LRU-bounded). NPCs remember THIS player: fed by dialogue opens / kills (vendor trades documented),
    // read by the dialogue warmth pseudo-tags + the dossier. Created in the ctor like every sibling; persisted via
    // FSerializedCharacterData::NpcRelations.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acquaintance")
    TObjectPtr<UMythicAcquaintanceComponent> Acquaintance;

    // Per-player NPC DOSSIER codex (replicated COND_OwnerOnly component; server-authoritative per-notable-NPC records:
    // times met, deeds, last warmth, death + killer). The player-scoped Chronicle counterpart — created in the ctor
    // beside Acquaintance (whose relation feed it rides), persisted via FSerializedCharacterData::NpcDossiers.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acquaintance")
    TObjectPtr<UMythicDossierComponent> Dossier;

    // Per-player TRADE CONTRACTS (WAVE O; replicated COND_OwnerOnly component; server-authoritative accepted delivery
    // contracts + the Accept/Abandon/Deliver RPCs). Lives on the PlayerState — deliberately NOT the do-not-edit PC —
    // like the MountRoster sibling; persisted via FSerializedCharacterData::TradeContracts.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trading")
    TObjectPtr<UMythicTradeContractComponent> TradeContracts;

    // The canonical PERSISTENT player identity = the save-slot CharacterID this player's character was loaded from
    // (a stable GUID-string across sessions). Empty until a character is loaded onto this player. Server sets it from
    // the save-load path; replicated so clients can display/attribute by a stable key. This is the cross-session key
    // the party/companion system and the player registry resolve against (vs the session-transient GetPlayerId()).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Identity")
    FString PersistentCharacterId;

    AMythicPlayerState();

public:
    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    UMythicAbilitySystemComponent *GetMythicAbilitySystemComponent() const;

    // Per-player faction standing store (server-authoritative; replicated to owner).
    UFUNCTION(BlueprintPure, Category = "Faction")
    UMythicFactionStandingComponent *GetFactionStanding() const { return FactionStanding; }

    // Per-player narrative ledger (server-authoritative; replicated to owner). Earned story tags; feeds tag-gated
    // objective/dialogue branching and persists via the character save (FSerializedCharacterData::StoryTags).
    UFUNCTION(BlueprintPure, Category = "Narrative")
    UMythicNarrativeStateComponent *GetNarrativeState() const { return NarrativeState; }

    // Per-player quest journal (server-authoritative; replicated to owner). The quest/storyline state machine.
    UFUNCTION(BlueprintPure, Category = "Narrative")
    UMythicQuestJournalComponent *GetQuestJournal() const { return QuestJournal; }


    // Per-player survival-needs driver (server-authoritative). The survival attribute set it drives lives on the ASC.
    UFUNCTION(BlueprintPure, Category = "Survival")
    UMythicSurvivalComponent *GetSurvivalComponent() const { return SurvivalComponent; }

    // Per-player meta-progression stat ledger (server-authoritative; replicated to owner). Lifetime Stat.* counters;
    // persists via the character save (FSerializedCharacterData::StatCounters).
    UFUNCTION(BlueprintPure, Category = "Progression")
    UMythicStatLedgerComponent *GetStatLedgerComponent() const { return StatLedger; }

    // Per-player achievements (server-authoritative; replicated to owner). Unlocked-achievement tags; persists via the
    // character save (FSerializedCharacterData::UnlockedAchievements).
    UFUNCTION(BlueprintPure, Category = "Progression")
    UMythicAchievementComponent *GetAchievementComponent() const { return Achievements; }

    // Per-player tag-unlock engine (server-authoritative; granted set replicated to owner, active title to all peers).
    // Persists via the character save (GrantedUnlockTags / AppliedUnlockRules / ActiveTitle).
    UFUNCTION(BlueprintPure, Category = "Progression")
    UMythicUnlockComponent *GetUnlockComponent() const { return Unlocks; }

    // Per-player rune sockets (server-authoritative; replicated to owner). Equipped runes + the unlocked slot count;
    // the unlock engine's GrantPerkSlot effect calls GrantSlot() on it.
    UFUNCTION(BlueprintPure, Category = "Progression")
    UMythicRuneComponent *GetRuneComponent() const { return Runes; }

    // Per-player codex (server-authoritative; replicated to owner). Bestiary kill/encounter records + discovered
    // glossary terms; persists via the character save (CodexBestiary / CodexTerms).
    UFUNCTION(BlueprintPure, Category = "Knowledge")
    UMythicCodexComponent *GetCodexComponent() const { return Codex; }

    // Per-player renown (server-authoritative; replicated to owner). Scoped reputation + global aggregate; persists
    // via the character save (RenownEntries / GlobalRenown).
    UFUNCTION(BlueprintPure, Category = "Progression")
    UMythicRenownComponent *GetRenownComponent() const { return Renown; }

    // Per-player mount roster (server-authoritative; replicated to owner). Tamed-mount records + the active selection;
    // persists via the character save (MountRoster / ActiveMountId).
    UFUNCTION(BlueprintPure, Category = "Mounts")
    UMythicMountRosterComponent *GetMountRosterComponent() const { return MountRoster; }

    // Per-player dialogue session (server-authoritative; current node replicated to owner). The NPC BP's interact hook
    // calls GetDialogueComponent()->ServerStartDialogue(NPC); the dialogue WBP binds its OnDialogueNodeChanged/Ended.
    UFUNCTION(BlueprintPure, Category = "Narrative")
    UMythicDialogueComponent *GetDialogueComponent() const { return DialogueComponent; }

    // Per-player acquaintance/grudge ledger (server-authoritative; replicated to owner). Per-NPC warmth relations;
    // persists via the character save (NpcRelations).
    UFUNCTION(BlueprintPure, Category = "Acquaintance")
    UMythicAcquaintanceComponent *GetAcquaintanceComponent() const { return Acquaintance; }

    // Per-player NPC dossier codex (server-authoritative; replicated to owner). Persists via the character save
    // (NpcDossiers). A dossier UI binds GetDossiers()/OnDossiersChanged.
    UFUNCTION(BlueprintPure, Category = "Acquaintance")
    UMythicDossierComponent *GetDossierComponent() const { return Dossier; }

    // Per-player trade contracts (WAVE O; server-authoritative; replicated to owner). Accepted delivery contracts +
    // the Accept/Abandon/Deliver RPCs; persists via the character save (TradeContracts).
    UFUNCTION(BlueprintPure, Category = "Trading")
    UMythicTradeContractComponent *GetTradeContractComponent() const { return TradeContracts; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void SetPersistentCharacterId(const FString &InCharacterId);

    UFUNCTION(BlueprintPure, Category = "Identity")
    const FString &GetPersistentCharacterId() const { return PersistentCharacterId; }

    // The canonical key this player is addressed by across the party/companion + registry systems: the persistent
    // CharacterID when one has been loaded, else a session-stable fallback. Built from this player's live state.
    UFUNCTION(BlueprintPure, Category = "Identity")
    FString GetCanonicalPlayerKey() const;

    static FString ResolveCanonicalPlayerKey(const FString &PersistentId, int32 SessionPlayerId);

    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;
    virtual UAbilitySystemComponent *GetSchematicsASC() const override;
    virtual UMythicInventoryComponent *GetInventoryForItemType(const FGameplayTag &ItemType) const override;
};
