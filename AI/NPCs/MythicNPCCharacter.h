
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "MythicNPCData.h"
#include "Player/MythicCharacter.h"
#include "Interaction/IMythicInteractable.h"
#include "AI/NPCs/MythicSocialVerbs.h"
#include "World/LivingWorld/Appearance/AppearanceTypes.h"
#include "AI/MonsterAffixes/MonsterAffixGranter.h"
#include "MythicNPCCharacter.generated.h"

struct FMythicIdentityFragment;

class UMythicNPCManager;
class UMythicAttributeSet_NPCCombat;
class UMythicAttributeSet_Life;
class UMythicAttributeSet_Defense;
class UMythicAttributeSet_Offense;
class UMythicCognitiveBrainComponent;
class UMythicLifeComponent;
class UMythicGameplayAbility;
class UGameplayEffect;
class UObjectiveDefinition;
class UItemDefinition;
class UMonsterAffixPool;
struct FMassEntityHandle;

USTRUCT(BlueprintType)
struct FMythicMerchantOffer {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
    TSoftObjectPtr<UItemDefinition> CostItem = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade", meta = (ClampMin = "1"))
    int32 CostQty = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade")
    TSoftObjectPtr<UItemDefinition> RewardItem = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trade", meta = (ClampMin = "1"))
    int32 RewardQty = 1;
};

UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API AMythicNPCCharacter : public AMythicCharacter, public IMythicInteractable {
    GENERATED_BODY()

public:
    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    FText SelectDialogueFor(APlayerController *Interactor) const;

    void FireBark(const FText &Line, APlayerController *Interactor);

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
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FMythicNPCData NPCData;

    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
    UAbilitySystemComponent *AbilitySystemComponent;

    // Replicated mirror of this NPC's current hostile target (set by AMythicAIController on the server; AI controllers
    // themselves do NOT replicate). Lets clients know who the NPC is fighting — drives the contextual nameplate
    // visibility so plates appear for ALL players, not just the listen-server host.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mythic NPC | Combat")
    TObjectPtr<AActor> EngagedTarget;

    // The LifeAttributeSet for the NPC
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_Life *LifeAttributes;

    // The Combat Attribute Set for the NPC
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_NPCCombat *CombatAttributes;

    // The Defense Attribute Set for the NPC. Lets the shared damage execution apply armor / resistances /
    // dodge / shield mitigation to NPC targets exactly as it does for players (single source of truth).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_Defense *DefenseAttributes;

    // The Offense Attribute Set for the NPC. The shared damage executions capture Power / DamagePerHit from the
    // SOURCE's Offense set, so an NPC needs this to deal non-zero damage. Seeded via NPCDefinition.Proficiencies.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_Offense *OffenseAttributes;

    // Designer-assigned attack ability granted to this NPC on spawn (reuse the player's GA_MeleeBase or an NPC
    // variant). Null = this NPC cannot attack. The AIController activates it when in melee range of its target.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Combat")
    TSubclassOf<UMythicGameplayAbility> AttackAbility;

    FGameplayAbilitySpecHandle AttackAbilityHandle;

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

    // Party/world-tier scaling tunables (C5). Each EXTRA party member (beyond the first) adds these fractions to the
    // health/damage mult; each world-tier step adds the per-tier fractions. Baseline (solo + world tier 0) => x1.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Combat | Scaling", meta = (ClampMin = "0.0"))
    float PerExtraMemberHealth = 0.15f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Combat | Scaling", meta = (ClampMin = "0.0"))
    float PerExtraMemberDamage = 0.10f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Combat | Scaling", meta = (ClampMin = "0.0"))
    float PerTierHealth = 0.25f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Combat | Scaling", meta = (ClampMin = "0.0"))
    float PerTierDamage = 0.15f;

    void ApplyCombatScaling();

    float BaseXPReward = 0.0f;
    bool bBaseXPRewardCaptured = false;

    void GrantAttackAbility();

    void CombatInit();

    bool bCombatInitialized = false;

    UFUNCTION()
    void HandleNPCDeath(AActor *DeadActor);

    bool bBoundDeath = false;

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Dialogue")
    FName PrimaryInteractionName = FName("Talk");

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

    // Fired on the interacting client when trade opens, so the vendor WBP can show the offer catalog
    // (GetMerchantOffers). Editor handoff, mirrors OnContainerOpened / OnNpcBark — no vendor widget invented in C++.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic NPC | Trade")
    void OnTradeOpened(APlayerController *Interactor);

public:
    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Trade")
    bool IsMerchant() const { return MerchantOffers.Num() > 0; }

    const TArray<FMythicMerchantOffer> &GetMerchantOffers() const { return MerchantOffers; }

    bool IsActorInTradeRange(const AActor *Actor) const;

    // True if this NPC may be recruited into a player's party (designer-set, mirrors the IsMerchant flag pattern).
    // The VALUE — which NPCs/factions/roles/quest-states are recruitable — is authored data, not decided in code.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Party")
    bool bRecruitable = false;

    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Party")
    bool IsRecruitable() const { return bRecruitable; }

    // Cognitive brain for Tier 2+ NPCs
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UMythicCognitiveBrainComponent *CognitiveBrain;

    // Consumes death/health events, runs regen, and grants kill XP for this NPC.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UMythicLifeComponent *LifeComponent;

    // Get NPC Data
    UFUNCTION(BlueprintCallable, Category = "Mythic NPC | Data")
    const FMythicNPCData GetNPCData() const;

    // The actor this NPC is currently fighting (replicated mirror of the server AI's hostile target; null = not engaged).
    // Client-visible, so the contextual nameplate system shows plates for everyone, not just the host.
    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Combat")
    AActor *GetEngagedTarget() const { return EngagedTarget; }

    void SetEngagedTarget(AActor *Target) { EngagedTarget = Target; }

    void SeedAttributesFromData();

public:
    virtual void OnSpawnedFromPool(const struct FMythicNPCData &InNPCData);
    virtual void OnReturnedToPool();


    virtual void SleepToPool();

    virtual void WakeFromPool();

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

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    virtual void InitializeASC() override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const override;

    virtual void PossessedBy(AController *NewController) override;

    friend class UMythicNPCManager;
};
