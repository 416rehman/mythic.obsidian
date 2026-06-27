// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "MythicNPCData.h"
#include "Player/MythicCharacter.h"
#include "Interaction/IMythicInteractable.h"
#include "AI/NPCs/MythicSocialVerbs.h" // EMythicSocialVerb / EMythicSocialReaction / FMythicSocialReactionResult (UHT-visible in UFUNCTION sigs)
#include "World/LivingWorld/Appearance/AppearanceTypes.h" // FMythicAppearance (replicated member + UHT-visible in OnApplyAppearance sig)
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
struct FMassEntityHandle;

/**
 * One designer-authored barter offer: the player pays CostQty of CostItem and receives RewardQty of RewardItem.
 * Item-for-item (no currency system exists in v1 — see Docs/BACKLOG.md), so trade is barter at fixed,
 * designer-controlled terms rather than a computed price. Value is enforced by these explicit offers, not a
 * free-form swap (which would let a player give junk for treasure).
 */
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
    //~ IMythicInteractable — makes the NPC talkable via the existing interaction scanner (mirrors
    // AMythicStorageContainer's pattern). On primary interact the NPC surfaces one contextual line from its own
    // CognitiveBrain->SelectDialogue to the interacting player (client-local; no server mutation, so no RPC).
    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    // SERVER: pick a contextual dialogue line from this NPC's brain. MUST run server-side — the brain's dialogue
    // context (Faction/Role/pressure) is authoritative + non-replicated, so a client read returns only the
    // fallback. Called by the interacting player's PC (AMythicPlayerController::ServerRequestNpcDialogue).
    FText SelectDialogueFor(APlayerController *Interactor) const;

    // CLIENT: surface a chosen dialogue line via the OnNpcBark Blueprint event on the interacting client.
    void FireBark(const FText &Line, APlayerController *Interactor);

    // ─── Social interaction verbs (RDR2-style player→NPC verbs + trait-driven reactions) ───
    // SERVER: resolve how this NPC reacts to a social verb, reading its (authoritative, non-replicated) personality
    // VentWeights + the interactor's standing toward this NPC's faction. Pure mapping under the hood
    // (UMythicSocialVerbLibrary::ResolveReaction); this wrapper just gathers the server-side inputs. Returns a
    // neutral result off-authority or with a missing brain/personality.
    FMythicSocialReactionResult ResolveSocialVerb(EMythicSocialVerb Verb, APlayerController *Interactor) const;

    // SERVER: apply a resolved reaction — adjust the interactor's standing (ServerAdjustStanding), optionally turn
    // hostile (the NPC's AIController ForceEngageTarget), optionally alert nearby allied guards (bounded radius scan
    // → OnSignificantEvent + ForceEngageTarget), and surface the reaction LOCALLY (server/listen-host). The remote
    // client surfaces it via the PC's ClientReceiveSocialReaction → FireReaction.
    void ApplySocialReaction(const FMythicSocialReactionResult &Result, EMythicSocialVerb Verb, APlayerController *Interactor);

    // CLIENT: surface a resolved social reaction via the OnNpcReaction Blueprint event on the interacting client
    // (mirrors FireBark for dialogue). Called by AMythicPlayerController::ClientReceiveSocialReaction.
    void FireReaction(EMythicSocialVerb Verb, EMythicSocialReaction Reaction, const FText &Line, APlayerController *Interactor);

    // ─── Context-driven activity (Step 3) ───
    // SERVER: set this NPC's current ambient activity tag (chosen by the AIController's idle dispatch from the activity
    // catalog). Change-gated: a no-op when the tag is unchanged, so it never re-multicasts the same cosmetic. On a real
    // change it stores the tag (server-side only — NOT replicated) and Multicast_PerformActivity's the cosmetic to all
    // clients (+ runs OnPerformActivity locally on the server/listen-host). No-op off-authority.
    void ServerSetActivity(FGameplayTag ActivityTag);

    // The NPC's current ambient activity (debugger getter). Server-side authoritative value; clients only ever receive
    // the transient cosmetic multicast, so this getter is meaningful on the server (where the debugger reads it).
    UFUNCTION(BlueprintPure, Category = "Mythic NPC | Activity")
    FGameplayTag GetCurrentActivityTag() const { return CurrentActivityTag; }

    // Debugger getter for the last social reaction this NPC processed (server-side; see LastSocialVerb/LastSocialReaction).
    // Returns false until a social verb has ever been applied to this NPC (out-params then carry the defaults). Server-only
    // state (the debugger reads it on the authority); never replicated. Used by the Living World gameplay debugger.
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

    // SERVER→ALL cosmetic broadcast: carries the chosen activity tag to clients so each plays OnPerformActivity. Unreliable
    // (a dropped ambient cosmetic is harmless — the next activity change re-broadcasts) + bounded to an activity CHANGE
    // (ServerSetActivity is change-gated). Drives a purely cosmetic BP hook; no sim state rides it.
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PerformActivity(FGameplayTag ActivityTag);

    // Current ambient activity tag (server-side only — NOT replicated; the cosmetic rides Multicast_PerformActivity, and
    // a late-joiner simply picks up the next change). Change-gate source for ServerSetActivity + the debugger getter.
    FGameplayTag CurrentActivityTag;

    // Last social verb/reaction processed by ApplySocialReaction (server-side only — NOT replicated; surfaced by the
    // Living World gameplay debugger via GetLastSocialReaction). Pure observability; no sim state rides these.
    EMythicSocialVerb LastSocialVerb = EMythicSocialVerb::Greet;
    EMythicSocialReaction LastSocialReaction = EMythicSocialReaction::Neutral;
    double LastSocialReactionTime = 0.0; // World->GetTimeSeconds() at the time of the reaction
    bool bHasSocialReaction = false;     // false until the first social verb is applied to this NPC

    // ─── Appearance (Step 4): deterministic wardrobe descriptor + replication ───
public:
    // The fully-resolved, replicated per-NPC look (which modular part fills each slot, skin/hair tone, faction tint).
    // Resolved server-side ONCE per embodiment from the stable Identity seed (ApplyAppearanceFromIdentity) and replicated
    // to clients via OnRep_Appearance → OnApplyAppearance. Clients NEVER re-resolve — they receive this descriptor — so
    // the look is byte-identical on every machine and stable across pool reuse / re-embody / save-load (Step 1 guarantee).
    // BlueprintReadOnly so an art Blueprint can read it from OnApplyAppearance to drive the skeletal-mesh merge.
    UPROPERTY(ReplicatedUsing = OnRep_Appearance, BlueprintReadOnly, Category = "Mythic NPC | Appearance")
    FMythicAppearance Appearance;

protected:
    // Replication callback: a client received a new Appearance descriptor → hand it to the art Blueprint. Fires only on
    // remote clients (OnRep never fires on the authority); the server/listen-host calls OnApplyAppearance directly after
    // assigning Appearance in ApplyAppearanceFromIdentity. No sim mutation here — pure cosmetic apply.
    UFUNCTION()
    void OnRep_Appearance();

    // Fired (via OnRep_Appearance on clients, and directly on server/listen-host) when this NPC's resolved look is ready,
    // so the Blueprint/art can build the modular skeletal-mesh merge + apply the part indices, skin/hair tones, and the
    // faction tint. Editor/art handoff (mirrors OnNpcBark / OnPerformActivity) — NO mesh or skeletal-merge invented in
    // C++ (that stays BP/art, keeping the C++ module free of an art dependency). If unbound, the descriptor is still
    // resolved + replicated but plays no visible wardrobe (honest art boundary).
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic NPC | Appearance")
    void OnApplyAppearance(const FMythicAppearance &Desc);

    // SERVER: resolve this NPC's appearance from its stable Identity seed (NameHash + demographics + role + faction) and
    // assign the replicated Appearance member, then (on server/listen-host) call OnApplyAppearance directly since OnRep
    // doesn't fire on the authority. Called from InitializeFromMassEntity after the identity read, inside the existing
    // HasAuthority guard. virtual so a non-humanoid subclass (AMythicCreatureCharacter) can skip wardrobe — though
    // creatures already skip it by overriding InitializeFromMassEntity without calling Super. No-op off-authority.
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

    // Handle of the granted attack ability (kept across pool reuse so re-grant is idempotent).
    FGameplayAbilitySpecHandle AttackAbilityHandle;

    // Designer-assigned default attribute-init effects applied to this NPC's own ASC on combat init (mirrors
    // AMythicPlayerState::DefaultGameplayEffects). This is the per-BP-class combat baseline (MaxHealth / Offense /
    // Defense) and is the ONLY stat source for MASS-embodied NPCs (which carry no NPCData.Proficiencies). Empty =
    // no class baseline; pooled NPCs then get stats only from NPCData.Proficiencies via SeedAttributesFromData.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic NPC | Combat")
    TArray<TSubclassOf<UGameplayEffect>> DefaultGameplayEffects;

    // SERVER: grant AttackAbility to the ASC (idempotent). Called from OnSpawnedFromPool after seeding.
    void GrantAttackAbility();

    // SERVER: shared combat-readiness step run by BOTH spawn paths via InitializeASC (pooled OnSpawnedFromPool and
    // MASS-embodied BeginPlay). Grants the attack ability and applies the designer DefaultGameplayEffects so an
    // embodied NPC - which never runs OnSpawnedFromPool - is still attack-capable + has non-zero stats. Idempotent.
    void CombatInit();

    // Latches once CombatInit has applied the (instant, non-idempotent) default stat-init GEs, so the repeated
    // InitializeASC calls (BeginPlay + PossessedBy + OnSpawnedFromPool) don't double-stack them.
    bool bCombatInitialized = false;

    // SERVER: bound once to LifeComponent->OnDeath in InitializeASC. For a MASS-embodied NPC, runs the living-world
    // PERMANENT-death contract (PersistentNPCRegistry::RegisterDeath), stops cognition, and timer-destroys the
    // corpse — preventing the zombie-entity leak where a combat-killed NPC stayed Tier2/embodied forever.
    UFUNCTION()
    void HandleNPCDeath(AActor *DeadActor);

    // Latches the one-shot OnDeath bind (InitializeASC runs up to 3x: BeginPlay + PossessedBy + OnSpawnedFromPool).
    bool bBoundDeath = false;

    // The AI controller retained across a pool park/wake cycle. SleepToPool captures the controller BEFORE
    // OnReturnedToPool unpossesses it, so WakeFromPool can RE-POSSESS the same controller instead of churning a new
    // one (and leaking the old). Weak so a controller destroyed during a level teardown is detected; WakeFromPool
    // falls back to SpawnDefaultController if it's gone. Only ever set/read on the server pool path.
    TWeakObjectPtr<AController> PooledController;

    // Timer that destroys the corpse after death cosmetics. Delayed (not immediate) because OnDeath broadcasts
    // BEFORE StartDeath finishes awarding XP/loot, so the actor must outlive the broadcast.
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

public:
    // The objective this NPC offers on interact (or null). Read server-side by the interacting player's PC.
    UObjectiveDefinition *GetQuestOffer() const { return QuestOffer; }

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

    // True if Actor is within trade range (always true when TradeRangeSq <= 0). Server-side gate for barter.
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

    // SERVER: seed base attributes from NPCData.Proficiencies onto the ASC. Idempotent across pooled reuse
    // (resets each target attribute to its default before applying the authored modifier).
    void SeedAttributesFromData();

public:
    //~ Begin IPoolableNPC Interface
    virtual void OnSpawnedFromPool(const struct FMythicNPCData &InNPCData);
    virtual void OnReturnedToPool();
    //~ End IPoolableNPC Interface

    // ─── Embodiment pool lifecycle (embodiment-service-LOCK-v1 §1) ───
    // The reuse-pool teardown/re-arm pair used by UMythicLivingWorldSubsystem::Release/AcquireEmbodiedActor. They
    // bracket the existing OnReturnedToPool / InitializeASC GAS plumbing with the cognition-brain join+reset and the
    // park/un-park (hide + collision + tick) so a recycled actor carries NO stale state from its previous occupant.
    // AMythicCreatureCharacter inherits both unchanged: its CognitiveBrain is never InitializeBrain'd, so the
    // brain calls are guarded no-ops. virtual so a mesh-bearing BP subclass can extend (must call Super).

    /**
     * RELEASE teardown — return this actor to the parked pool. Order is LOCKED:
     *   (1) HasAuthority guard;
     *   (2) StopThinking() FIRST (joins the async BDI worker before anything else touches the actor);
     *   (3) OnReturnedToPool() (existing GAS / timer / UnPossess teardown);
     *   (4) ResetForReuse() (clear beliefs/intention/source-entity — only safe after the worker join in step 2);
     *   (5) park: disable collision, hide in game, disable tick.
     */
    virtual void SleepToPool();

    /**
     * ACQUIRE re-arm — wake this actor from the parked pool. Runs BEFORE the spawn processor's InitializeFromMassEntity.
     * The caller (AcquireEmbodiedActor) has already un-hidden + re-enabled collision + repositioned the actor. Order
     * is LOCKED:
     *   (1) HasAuthority guard;
     *   (2) InitializeASC() (re-grant attack ability, re-wire LifeComponent idempotently; death-bind is SKIPPED by the
     *       bBoundDeath latch so HandleNPCDeath is never double-bound);
     *   (3) ResetForRespawn() on LifeAttributes (snap Health to the re-seeded MaxHealth);
     *   (4) RestoreAfterDeath() on LifeComponent (re-enable movement/collision a prior StartDeath disabled);
     *   (5) re-enable tick per the ctor's bCanEverTick.
     * StartThinking() is re-armed last (BeginPlay does not re-run on reuse).
     */
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

    /**
     * Initializes the character from its Mass entity counterpart upon promotion to a fully simulated actor.
     * Hooks up the Cognitive Brain, Faction bindings, and Personality.
     */
    virtual void InitializeFromMassEntity(const FMassEntityHandle &InEntityHandle);

    // Sets default values for this character's properties
    AMythicNPCCharacter();

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // SAFETY NET: any destroy path (population far-despawn, encounter cleanup, level teardown, future callers) routes
    // through here, guaranteeing a destroyed companion always leaves its party (clears the despawn-exemption + slot +
    // follow timer). HandleNPCDeath covers the GAS-death path; this covers every OTHER destroy so RemoveMemberAt is a
    // truly unbypassable sink. No-op for non-companions.
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    virtual void InitializeASC() override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const override;

    // PossessedBy - Server side
    virtual void PossessedBy(AController *NewController) override;

    friend class UMythicNPCManager;
};
