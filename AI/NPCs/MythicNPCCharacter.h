
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "MythicNPCData.h"
#include "Player/MythicCharacter.h"
#include "Interaction/IMythicInteractable.h"
#include "Interaction/ContextActions/MythicContextActionProvider.h"
#include "World/Entity/IMythicPresentableEntity.h"
#include "AI/NPCs/MythicSocialVerbs.h"
#include "World/LivingWorld/Appearance/AppearanceTypes.h"
#include "AI/MonsterAffixes/MonsterAffixGranter.h"
#include "MythicNPCCharacter.generated.h"

struct FMythicIdentityFragment;
struct FMythicPublicIdentitySnapshot;

class UMythicNPCManager;
class UMythicAttributeSet_Life;
class UMythicAttributeSet_Defense;
class UMythicAttributeSet_Offense;
class UMythicAttributeSet_Utility;
class UMythicCognitiveBrainComponent;
class UMythicLifeComponent;
class UMythicGameplayAbility;
class UGameplayEffect;
class UObjectiveDefinition;
class UItemDefinition;
class UMonsterAffixPool;
class UMythicEntityPresentationComponent;
class UMythicContextActionDefinition;
struct FMassEntityHandle;

USTRUCT(BlueprintType)
struct FMythicMerchantOffer {
    GENERATED_BODY()

    /** Item currency consumed when the authority accepts this authored barter offer. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
    TSoftObjectPtr<UItemDefinition> CostItem = nullptr;

    /** Number of cost items consumed per completed barter transaction. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade", meta = (ClampMin = "1"))
    int32 CostQty = 1;

    /** Item granted to the buyer after the cost has been validated and removed. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
    TSoftObjectPtr<UItemDefinition> RewardItem = nullptr;

    /** Number of reward items granted per completed barter transaction. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade", meta = (ClampMin = "1"))
    int32 RewardQty = 1;
};

UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API AMythicNPCCharacter : public AMythicCharacter,
                                      public IMythicInteractable,
                                      public IMythicPresentableEntity,
                                      public IMythicContextActionProvider {
    GENERATED_BODY()

public:
    /** Returns this NPC body's single replicated domain presentation adapter; it remains valid while the actor is pooled. */
    virtual UMythicEntityPresentationComponent *GetEntityPresentationComponent_Implementation() const override {
        return EntityPresentationComponent;
    }

    /** Gathers only current, viewer-specific talk, quest, and service offers on authority. */
    virtual void GatherContextActions_Implementation(
        AController *RequestingController, AActor *Subject,
        TArray<FMythicContextActionOffer> &OutOffers) const override;

    /** Revalidates the exact NPC action and opaque revision immediately before server execution. */
    virtual bool CanExecuteContextAction_Implementation(
        AController *RequestingController, AActor *Subject,
        FGameplayTag ActionTag, int64 ObservedOfferRevision,
        FGameplayTag &OutFailureReason) const override;

    /** Executes a revalidated NPC domain action without trusting client labels, quest state, or service state. */
    virtual bool ExecuteContextAction_Implementation(
        AController *RequestingController, AActor *Subject,
        FGameplayTag ActionTag, int64 ObservedOfferRevision,
        FGameplayTag &OutFailureReason) override;

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    FText SelectDialogueFor(APlayerController *Interactor) const;

    void FireBark(const FText &Line, APlayerController *Interactor);

    /** Opens this merchant's authored local trade surface after an authoritative contextual-action decision. */
    void OpenTradeForLocalController(APlayerController *Interactor);

    FMythicSocialReactionResult ResolveSocialVerb(EMythicSocialVerb Verb, APlayerController *Interactor) const;

    void ApplySocialReaction(const FMythicSocialReactionResult &Result, EMythicSocialVerb Verb, APlayerController *Interactor);

    void FireReaction(EMythicSocialVerb Verb, EMythicSocialReaction Reaction, const FText &Line, APlayerController *Interactor);

    void ServerSetActivity(FGameplayTag ActivityTag);

    // The NPC's current ambient activity (debugger getter). Server-side authoritative value; clients only ever receive
    // the transient cosmetic multicast, so this getter is meaningful on the server (where the debugger reads it).
    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Activity")
    FGameplayTag GetCurrentActivityTag() const { return CurrentActivityTag; }

    bool GetLastSocialReaction(EMythicSocialVerb &OutVerb, EMythicSocialReaction &OutReaction, double &OutWorldTime) const {
        OutVerb = LastSocialVerb;
        OutReaction = LastSocialReaction;
        OutWorldTime = LastSocialReactionTime;
        return bHasSocialReaction;
    }

protected:
    /** Builds only globally observable identity for a Mass embodiment; specializations may replace kind/archetype. */
    virtual void BuildMassPublicIdentity(const FMythicIdentityFragment &Identity,
                                         FMythicPublicIdentitySnapshot &OutIdentity) const;

    /** Builds public cover identity for an authored-world or unmanaged runtime body without exposing private NPC data. */
    virtual void BuildDirectPublicIdentity(
        FMythicPublicIdentitySnapshot &OutIdentity) const;

    // Fired (via Multicast_PerformActivity) on every client + the server/listen-host when this NPC begins a new ambient
    // activity, so the Blueprint can play the matching montage/anim/prop (a fishing-rod cast, a hammer swing, a market
    // browse). Editor handoff (mirrors OnNpcBark) — no activity montage invented in C++. If unbound, the activity is
    // still selected + steered (movement) but plays no cosmetic.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic NPC | Activity")
    void OnPerformActivity(FGameplayTag ActivityTag);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PerformActivity(FGameplayTag ActivityTag);

    FGameplayTag CurrentActivityTag;

    EMythicSocialVerb LastSocialVerb = EMythicSocialVerb::Greet;
    EMythicSocialReaction LastSocialReaction = EMythicSocialReaction::Neutral;
    double LastSocialReactionTime = 0.0;
    bool bHasSocialReaction = false;

public:
    // The fully-resolved, replicated per-NPC look (which modular part fills each slot, skin/hair tone, faction tint).
    // Resolved server-side ONCE per embodiment from the stable Identity seed (ApplyAppearanceFromIdentity) and replicated
    // to clients via OnRep_Appearance → OnApplyAppearance. Clients NEVER re-resolve — they receive this descriptor — so
    // the look is byte-identical on every machine and stable across pool reuse / re-embody / save-load (Step 1 guarantee).
    // BlueprintReadOnly so an art Blueprint can read it from OnApplyAppearance to drive the skeletal-mesh merge.
    UPROPERTY(ReplicatedUsing = OnRep_Appearance, BlueprintReadOnly, Category = "Mythic NPC | Appearance")
    FMythicAppearance Appearance;

protected:
    UFUNCTION()
    void OnRep_Appearance();

    // Fired (via OnRep_Appearance on clients, and directly on server/listen-host) when this NPC's resolved look is ready,
    // so the Blueprint/art can build the modular skeletal-mesh merge + apply the part indices, skin/hair tones, and the
    // faction tint. Editor/art handoff (mirrors OnNpcBark / OnPerformActivity) — NO mesh or skeletal-merge invented in
    // C++ (that stays BP/art, keeping the C++ module free of an art dependency). If unbound, the descriptor is still
    // resolved + replicated but plays no visible wardrobe (honest art boundary).
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic NPC | Appearance")
    void OnApplyAppearance(const FMythicAppearance &Desc);

    virtual void ApplyAppearanceFromIdentity(const FMythicIdentityFragment &Id);

protected:
    // Fired (via FireReaction, from the PC's ClientReceiveSocialReaction — which also runs locally on a listen-host)
    // with the NPC's reaction to a social verb, so the Blueprint can play the matching face/anim/bark UI. Editor
    // handoff (mirrors OnNpcBark) — no dedicated reaction widget invented in C++. If unbound, the reaction is still
    // computed + applied (standing/aggro/guards) but not surfaced.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic NPC | Social")
    void OnNpcReaction(EMythicSocialVerb Verb, EMythicSocialReaction Reaction, const FText &Line, APlayerController *Interactor);

    // Radius (cm) within which a hostile social verb that triggers CallGuards alerts allied NPCs. Designer-tunable.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Social", meta = (ClampMin = "0.0"))
    float GuardAlertRadius = 1500.0f;

    // Cap on how many allied NPCs a single guard-alert may rouse (bounds the one-shot radius scan).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Social", meta = (ClampMin = "0"))
    int32 GuardAlertMaxResponders = 8;

protected:
    /** Runtime NPC identity, affiliation, perception, and combat seed copied from the authoritative definition. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FMythicNPCData NPCData;

    /** Replicated GAS endpoint that owns this NPC's attributes, effects, tags, and granted abilities. */
    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
    UAbilitySystemComponent *AbilitySystemComponent;

    // Authority-only exact combat target. Public presentation replicates only observable Fighting; exact opponent
    // identity belongs in viewer-entitled owner signals and never broadcasts from this shared actor.
    UPROPERTY(Transient)
    TObjectPtr<AActor> EngagedTarget;

    // The LifeAttributeSet for the NPC
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_Life *LifeAttributes;

    // The Defense Attribute Set for the NPC. Lets the shared damage execution apply armor / resistances /
    // dodge / shield mitigation to NPC targets exactly as it does for players (single source of truth).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_Defense *DefenseAttributes;

    // The Offense Attribute Set for the NPC. The shared damage executions capture Power / DamagePerHit from the
    // SOURCE's Offense set, so an NPC needs this to deal non-zero damage. Seeded via NPCDefinition.Proficiencies.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_Offense *OffenseAttributes;

    /**
     * Canonical Utility set required by generic stat derivation captures such as Resolve. NPC archetypes keep
     * unused utility stats at neutral values rather than omitting the set and invalidating the entire GAS spec.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_Utility *UtilityAttributes;

    // Designer-assigned attack ability granted to this NPC on spawn (reuse the player's GA_MeleeBase or an NPC
    // variant). Null = this NPC cannot attack. The AIController activates it when in melee range of its target.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Combat")
    TSubclassOf<UMythicGameplayAbility> AttackAbility;

    FGameplayAbilitySpecHandle AttackAbilityHandle;

    // The one live combat-scaling effect. Re-applying scaling (pool reuse, the post-stamp re-run after
    // OnSpawnedFromPool, Mass embodiment) removes this first so multipliers can never stack.
    FActiveGameplayEffectHandle CombatScalingHandle;

    // Handles for the applied DefaultGameplayEffects. A baseline GE that grants no tags survives the
    // owned-tags sweep on pool return, so these are removed explicitly — without this every pool cycle
    // stacked another baseline (the same leak ClearAbility fixed for ability specs).
    TArray<FActiveGameplayEffectHandle> DefaultEffectHandles;

    // Pool transitions for CharacterMovement. SetActorTickEnabled does not touch component ticks: a parked
    // character keeps simulating falling (and can be KillZ-destroyed out of the pool) unless the CMC itself
    // is stopped.
    void ParkMovementForPool();
    void RestoreMovementFromPool();

    // Manager-spawn brain wiring: resolves NPCData.Faction to a faction id and initializes the cognitive
    // brain (no Mass entity), so definition-spawned NPCs join perception, diplomacy, standing and witness
    // systems instead of living as forced-engagement Neutrals.
    void InitializeBrainFromNPCData();

public:
    /**
     * The attack this NPC was authored with, and the effects that give it its stats. Public because
     * "can this pawn actually fight?" is worth asserting: an NPC with no attack ability and no stat
     * baseline spawns, walks up to you and does nothing, which reads as broken AI rather than empty data.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Combat")
    TSubclassOf<UMythicGameplayAbility> GetAttackAbility() const { return AttackAbility; }

    // No-copy access for hot paths (perception attitude queries read the affiliation map every sense event).
    const FMythicNPCData &GetNPCDataRef() const { return NPCData; }

    /** Returns the authored baseline effects applied once when this NPC initializes its combat state. */
    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Combat")
    const TArray<TSubclassOf<UGameplayEffect>> &GetDefaultGameplayEffects() const { return DefaultGameplayEffects; }

    /** Returns the authored enemy-tier tag used by combat scaling, rewards, and contextual danger assessment. */
    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Combat")
    FGameplayTag GetEnemyTier() const { return EnemyTier; }

protected:

    // Designer-assigned default attribute-init effects applied to this NPC's own ASC on combat init (mirrors
    // AMythicPlayerState::DefaultGameplayEffects). This is the per-BP-class combat baseline (MaxHealth / Offense /
    // Defense) and is the ONLY stat source for MASS-embodied NPCs (which carry no NPCData.Proficiencies). Empty =
    // no class baseline; pooled NPCs then get stats only from NPCData.Proficiencies via SeedAttributesFromData.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Combat")
    TArray<TSubclassOf<UGameplayEffect>> DefaultGameplayEffects;

    // ─── Enemy scaling (C4 tiers + C5 party/world-tier) ───
    // Per-actor difficulty tier (Normal/Superior/Elite/Champion/Boss). Designer/spawner-set on the NPC BP. Default =
    // Normal (set in the ctor). Folds into the CombatScaling GE via FMythicEnemyScaling::GetTierScaling; also keys the
    // kill-XP reward. An empty/invalid tag falls back to Normal, so a mis-set value is never fatal.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Combat", meta = (Categories = "AI.Tier"))
    FGameplayTag EnemyTier;

    // ─── Identity publish (activates skinning F2 / bestiary G2 / tier-loot C2 / corpse decay E1 / hunt pressure P) ───
    // Coarse kind. Creature => its corpse is skinnable and it feeds pelt-quality + hunt pressure; Humanoid => not
    // skinnable. Default = Humanoid (set in the ctor), matching the documented "absent AI.Kind => not skinnable" rule,
    // so existing NPC BPs behave exactly as before until a designer flips a beast to Creature.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Identity", meta = (Categories = "AI.Kind"))
    FGameplayTag CreatureKind;

    /**
     * Stable, editor-baked identity seed for a directly placed world actor. It is generated from Unreal's actor GUID,
     * retained in cooked content, hidden from Blueprint, and never used as a player-facing or replicated identifier.
     */
    UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Mythic NPC | Identity",
              meta = (DisplayName = "Authored World Identity"))
    FGuid AuthoredWorldIdentityGuid;

    // OPTIONAL explicit bestiary key. Leave EMPTY for the normal case: the kill hook then derives the key from
    // NPC.Type.X (=> Codex.Bestiary.Humanoid.X) or falls back to the coarse AI.Kind.* generic. Only set this to
    // override that derivation for a specific species entry.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Identity", meta = (Categories = "Codex.Bestiary"))
    FGameplayTag CodexBestiaryKey;

    // ─── Monster elite affixes (C5) ───
    // OPTIONAL authored affix pool. Null => FMonsterAffixSelector falls back to UMonsterAffixPool::GetDefaultPool(),
    // so Molten/Frozen/Vortex/Shielded elites work with ZERO authored content.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Combat")
    TObjectPtr<UMonsterAffixPool> MonsterAffixPool;

    void ApplyMonsterAffixes();

    FMonsterAffixGrantHandles MonsterAffixHandles;

    void PublishIdentityTags();

    // Each EXTRA party member beyond the first adds these fractions to this NPC's health/damage multiplier. World
    // tier scaling is not here — it comes from UWorldTierAttributes so one curve drives the whole tier ladder.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Combat | Scaling", meta = (ClampMin = "0.0"))
    float PerExtraMemberHealth = 0.15f;

    /** Fractional damage multiplier added for every party member beyond the first. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Combat | Scaling", meta = (ClampMin = "0.0"))
    float PerExtraMemberDamage = 0.10f;

    void ApplyCombatScaling(bool bPreserveHealthRatio = true);

    float BaseXPReward = 0.0f;
    bool bBaseXPRewardCaptured = false;

    void GrantAttackAbility();

    bool bCombatInitialized = false;

    // Blueprint subclasses can author different initial AttributeSet bases. Capture the actual instance once before
    // any effect runs, then restore that exact state on every pooled embodiment instead of consulting native CDOs.
    TMap<FGameplayAttribute, float> PristineAttributeBases;
    bool bPristineAttributeBasesCaptured = false;

    // Stable logical-entity percentiles keep level-band variation deterministic across live rescale and pool reuse.
    bool bScalingPercentilesInitialized = false;
    float HealthLevelPercentile = 0.5f;
    float DamageLevelPercentile = 0.5f;

    void CapturePristineAttributeBases();
    void RestorePristineAttributeBases();
    void ResetCombatRuntimeStateToPristine();
    bool HasCanonicalCombatAttributeSets() const;
    bool CommitCombatInitializationForEmbodiment();
    float ResolveStableScalingPercentile(uint32 Salt) const;

    UFUNCTION()
    void HandleNPCDeath(AActor *DeadActor);

    TWeakObjectPtr<AController> PooledController;

    FTimerHandle CorpseTimerHandle;

    // Designer-tunable corpse-fade delay before the embodied NPC actor is destroyed (RespawnDelay on the
    // LifeComponent is documented NPC-ignored, so the corpse lifetime needs its own field).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Death")
    float CorpseLifetime = 5.0f;

    // Interaction prompt data (same mechanism as AMythicStorageContainer). Assign a data table with a "Talk" row
    // on the NPC Blueprint so the CommonUI prompt shows the bound key.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Dialogue")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    /** Row name used by the legacy interaction prompt when contextual actions are unavailable. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Dialogue")
    FName PrimaryInteractionName = FName("Talk");

    /** Canonical ordinary conversation action; null keeps legacy interaction but projects no contextual Talk row. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
              Category = "World Presentation|Context Actions")
    TObjectPtr<UMythicContextActionDefinition> TalkContextActionDefinition;

    /** Canonical viewer-private quest-offer action shown only when this player's objective rules currently allow it. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
              Category = "World Presentation|Context Actions")
    TObjectPtr<UMythicContextActionDefinition> QuestOfferContextActionDefinition;

    /** Canonical viewer-private quest-progress action shown only when talking or delivery can advance a task. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
              Category = "World Presentation|Context Actions")
    TObjectPtr<UMythicContextActionDefinition> QuestTurnInContextActionDefinition;

    // Fired on the local interacting client with the chosen dialogue line, so the Blueprint can surface it through
    // the existing HUD/bark UI layer (editor handoff, mirrors AMythicStorageContainer::OnContainerOpened — no
    // dedicated bark widget is invented in C++). If unbound, the line is computed but not shown (honest no-op).
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic NPC | Dialogue")
    void OnNpcBark(const FText &Line, APlayerController *Interactor);

    // Designer-assigned quest this NPC offers when talked to. Null = not a quest-giver. Offered (once) on
    // interact via the same server-side path that picks the dialogue line; the player's ObjectiveTracker dedupes
    // so repeated conversations don't re-add it.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Dialogue")
    TObjectPtr<UObjectiveDefinition> QuestOffer;

    // Designer-assigned identity tag for "talk to X" objectives (e.g. Objective.NPC.VillageElder). EMPTY (default) =
    // this NPC is not a talk-objective target. When set, talking to this NPC fires GAS.Event.TalkedToNPC with this tag,
    // advancing an objective whose RequiredPayloadTag matches. Distinct from GetNPCType() (a TYPE, not a unique id).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Dialogue")
    FGameplayTag QuestNpcTag;

public:
    UObjectiveDefinition *GetQuestOffer() const { return QuestOffer; }

    const FGameplayTag &GetQuestNpcTag() const { return QuestNpcTag; }

protected:
    // Designer-authored barter offers. Non-empty = this NPC is a merchant (offers trade on secondary-interact).
    // These are EditDefaultsOnly class data, so the client has them directly — opening the trade UI is client-local;
    // only executing an offer round-trips to the server (ServerExecuteBarterOffer).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Trade")
    TArray<FMythicMerchantOffer> MerchantOffers;

    // Squared range within which a player may trade with this merchant. <= 0 disables the range gate.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Trade")
    float TradeRangeSq = 250000.0f; // 500cm

    /** Canonical merchant/service action; null or an empty offer catalog projects no contextual Service row. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
              Category = "World Presentation|Context Actions")
    TObjectPtr<UMythicContextActionDefinition> ServiceContextActionDefinition;

    // Fired on the interacting client when trade opens, so the vendor WBP can show the offer catalog
    // (GetMerchantOffers). Editor handoff, mirrors OnContainerOpened / OnNpcBark — no vendor widget invented in C++.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic NPC | Trade")
    void OnTradeOpened(APlayerController *Interactor);

public:
    /** True when this NPC has at least one authored barter offer and can project a valid trade service. */
    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Trade")
    bool IsMerchant() const { return MerchantOffers.Num() > 0; }

    const TArray<FMythicMerchantOffer> &GetMerchantOffers() const { return MerchantOffers; }

    bool IsActorInTradeRange(const AActor *Actor) const;

    // True if this NPC may be recruited into a player's party (designer-set, mirrors the IsMerchant flag pattern).
    // The VALUE — which NPCs/factions/roles/quest-states are recruitable — is authored data, not decided in code.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Party")
    bool bRecruitable = false;

    /** True when authored recruitment rules permit this NPC to enter a player's party. */
    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Party")
    bool IsRecruitable() const { return bRecruitable; }

    // Cognitive brain for Tier 2+ NPCs
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UMythicCognitiveBrainComponent *CognitiveBrain;

    // Consumes death/health events, runs regen, and grants kill XP for this NPC.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UMythicLifeComponent *LifeComponent;

    /**
     * Shared replicated adapter for safe public identity, executed observable facts, and bounded status presentation.
     * It contains no viewer-specific relationship, quest, danger, action, or learned-person state.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UMythicEntityPresentationComponent> EntityPresentationComponent;

    // Get NPC Data
    UFUNCTION(BlueprintCallable, Category = "Mythic NPC | Data")
    const FMythicNPCData GetNPCData() const;

    /**
     * Write the combat level and re-apply the scaling effect with it. Every spawn path that does not run
     * through OnSpawnedFromPool (Mass embodiment, creatures, designer spawns) stamps through this one door,
     * so the level a fight carries can never depend on which system placed the entity.
     */
    void StampCombatLevel(int32 Level);

    // Authority-only combat-domain query. UI consumes public/owner-sanitized DTOs instead of this raw actor pointer.
    AActor *GetEngagedTarget() const { return EngagedTarget; }

    /** Sets the authoritative combat target and publishes only the already-executed, publicly observable fight state. */
    void SetEngagedTarget(AActor *Target);

    /** Switches the public behavior slot between visibly fleeing and fighting without exposing the private desire score. */
    void SetFleeingPresentation(bool bIsFleeing);

    /** Publishes the authoritative revivable down state through the single observable life-state slot. */
    UFUNCTION()
    void HandleNPCDowned(AActor *DownedActor);

    /** Clears the observable down state after the authoritative life component completes revival. */
    UFUNCTION()
    void HandleNPCRevived(AActor *RevivedActor);

    void SeedAttributesFromData();

public:
    virtual bool OnSpawnedFromPool(const struct FMythicNPCData &InNPCData);
    virtual void OnReturnedToPool();


    virtual void SleepToPool();

    virtual void WakeFromPool();

    /** Keeps a reused Mass body hidden/unregistered while restoring runtime components for its next logical entity. */
    void PrepareForEmbodiment();

    /** Activates the fully initialized presentation and reveals/collision-enables the body last; authority only. */
    bool ActivatePreparedEmbodiment();

    // Get the NPC Id
    UFUNCTION(BlueprintCallable, Category = "Mythic NPC | Data")
    const FGuid &GetNPCId() const;

    // Get the NPC Type
    UFUNCTION(BlueprintCallable, Category = "Mythic NPC | Data")
    const FGameplayTag &GetNPCType() const;

    // SERVER: try to activate the granted attack ability (the GAS Cooldown GE gates rate). Returns true if it
    // activated. Driven by the AIController when in melee range of the current hostile target.
    UFUNCTION(BlueprintCallable, Category = "Mythic NPC | Combat")
    bool TryActivateAttack();

    virtual void InitializeFromMassEntity(const FMassEntityHandle &InEntityHandle);

    AMythicNPCCharacter();

    virtual void BeginPlay() override;

#if WITH_EDITOR
    virtual void PostLoad() override;
    virtual void PostActorCreated() override;
    virtual void PostEditImport() override;
#endif

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    virtual void InitializeASC() override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const override;

    virtual void PossessedBy(AController *NewController) override;

    friend class UMythicNPCManager;
    friend struct FMythicDirectEntityPresentationTestAccess;

private:
    void ConfigureEntityPresentationAnchor();
    void TryActivateDirectEntityPresentation();

#if WITH_EDITOR
    void RefreshAuthoredWorldIdentityFromActorGuid();
#endif

    const UMythicContextActionDefinition *ResolveContextActionDefinition(
        FGameplayTag ActionTag) const;
    bool IsContextActionAvailable(
        AController *RequestingController,
        const UMythicContextActionDefinition *Definition) const;
    uint32 BuildContextActionRevision(
        const UMythicContextActionDefinition *Definition) const;
    bool ValidateContextAction(
        AController *RequestingController, AActor *Subject,
        FGameplayTag ActionTag, int64 ObservedOfferRevision,
        FGameplayTag &OutFailureReason) const;
};
