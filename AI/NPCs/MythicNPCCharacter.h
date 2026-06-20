// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "MythicNPCData.h"
#include "Player/MythicCharacter.h"
#include "Interaction/IMythicInteractable.h"
#include "MythicNPCCharacter.generated.h"

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
    void InitializeFromMassEntity(const FMassEntityHandle &InEntityHandle);

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
