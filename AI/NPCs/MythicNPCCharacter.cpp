// 


#include "MythicNPCCharacter.h"

#include "Mythic.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/AttributeSets/NPC/MythicAttributeSet_NPCCombat.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "GameplayEffect.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Net/UnrealNetwork.h"
#include "MassEntitySubsystem.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/Party/PartySubsystem.h" // remove a dead companion from its party
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/MythicPlayerController.h"


const FMythicNPCData AMythicNPCCharacter::GetNPCData() const {
    return this->NPCData;
}

void AMythicNPCCharacter::OnSpawnedFromPool(const struct FMythicNPCData &InNPCData) {
    // OnSpawnedFromPool is called when the NPC is spawned from the pool and is ready to be used, any initialization should happen here.
    // Authority only
    if (!this->HasAuthority()) {
        return;
    }
    this->NPCData = InNPCData;
    this->InitializeASC();

    // Seed base attributes (health, defense, offense, etc.) from the NPC's authored data BEFORE restoring
    // health, so ResetForRespawn snaps Health up to the seeded MaxHealth instead of the stale 100 default.
    SeedAttributesFromData();

    // Grant the designer-assigned attack ability (idempotent across pool reuse).
    GrantAttackAbility();

    // A pooled NPC may have died in a previous life; clear the death latch and restore health so it doesn't
    // spawn already dead.
    if (LifeAttributes) {
        LifeAttributes->ResetForRespawn();
    }

    // A pooled NPC that died previously had its movement + collision disabled by StartDeath; re-enable them so
    // the reused NPC can move/attack/be-hit again (otherwise it spawns frozen).
    if (LifeComponent) {
        LifeComponent->RestoreAfterDeath();
    }
}

void AMythicNPCCharacter::GrantAttackAbility() {
    if (!HasAuthority() || !AbilitySystemComponent || !AttackAbility) {
        return;
    }
    if (AttackAbilityHandle.IsValid()) {
        return; // already granted (kept across pool reuse)
    }
    AttackAbilityHandle = AbilitySystemComponent->GiveAbility(
        FGameplayAbilitySpec(AttackAbility.GetDefaultObject(), 1, INDEX_NONE, this));
}

void AMythicNPCCharacter::CombatInit() {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return;
    }

    // Grant the designer attack ability (self-idempotent via AttackAbilityHandle). This is what makes an embodied
    // NPC - which never runs OnSpawnedFromPool - actually attack-capable.
    GrantAttackAbility();

    // Apply the per-class default stat-init effects exactly once. These set base MaxHealth / Offense / Defense so
    // the NPC has non-zero combat stats even with no NPCData.Proficiencies. Same idiom as AMythicPlayerState's
    // DefaultGameplayEffects. Latched because instant GEs are not idempotent (would stack on repeated InitializeASC).
    if (bCombatInitialized) {
        return;
    }
    for (const TSubclassOf<UGameplayEffect> &Effect : DefaultGameplayEffects) {
        if (!Effect) {
            continue;
        }
        FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
        Ctx.AddSourceObject(this);
        const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(Effect, 1.0f, Ctx);
        if (Spec.IsValid()) {
            AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        }
    }

    // Surface a misconfiguration (single-source-of-truth): DefaultGameplayEffects (the class baseline) and
    // NPCData.Proficiencies are meant for DISJOINT attributes. On the pooled path SeedAttributesFromData runs
    // after this and RESETS each proficiency attribute's base to its CDO default before applying the roll — which
    // silently discards any baseline a DefaultGE set on that same attribute. Detect the overlap once (first life,
    // before any proficiency reset has run) by comparing each proficiency attribute's post-GE base to its CDO
    // default; a difference means a DefaultGE already wrote it. Diagnostic only — no behavior change.
    if (DefaultGameplayEffects.Num() > 0) {
        for (const FRolledAttributeSpec &Roll : NPCData.Proficiencies) {
            if (!Roll.Attribute.IsValid() || !AbilitySystemComponent->HasAttributeSetForAttribute(Roll.Attribute)) {
                continue;
            }
            const UClass *SetClass = Roll.Attribute.GetAttributeSetClass();
            if (!SetClass) {
                continue;
            }
            const UAttributeSet *CDO = SetClass->GetDefaultObject<UAttributeSet>();
            const float CdoDefault = Roll.Attribute.GetNumericValue(CDO);
            const float CurrentBase = AbilitySystemComponent->GetNumericAttributeBase(Roll.Attribute);
            if (!FMath::IsNearlyEqual(CurrentBase, CdoDefault)) {
                UE_LOG(Myth, Warning,
                       TEXT("AMythicNPCCharacter::CombatInit: %s attribute %s is set by BOTH a DefaultGameplayEffect ")
                       TEXT("(base=%.2f) and an NPCData.Proficiency; the proficiency's CDO reset in SeedAttributesFromData ")
                       TEXT("will discard the DefaultGE baseline. Keep the two layers on disjoint attributes."),
                       *GetNameSafe(this), *Roll.Attribute.GetName(), CurrentBase);
            }
        }
    }

    bCombatInitialized = true;
}

bool AMythicNPCCharacter::TryActivateAttack() {
    if (!HasAuthority() || !AbilitySystemComponent || !AttackAbilityHandle.IsValid()) {
        return false;
    }
    return AbilitySystemComponent->TryActivateAbility(AttackAbilityHandle);
}

void AMythicNPCCharacter::SeedAttributesFromData() {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return;
    }

    // Apply each authored attribute roll to the ASC's base value. Uses the same live idiom as the
    // itemization affix system (UAffixesFragment::ApplyAffixes -> ApplyModToAttribute) so the authored
    // modifier op (Additive/Override/...) is honored - single source of truth for attribute application.
    for (FRolledAttributeSpec &Roll : NPCData.Proficiencies) {
        if (!Roll.Attribute.IsValid()) {
            UE_LOG(Myth, Error, TEXT("AMythicNPCCharacter::SeedAttributesFromData: invalid attribute on %s; skipping spec."),
                   *GetNameSafe(this));
            continue;
        }

        // Skip attributes whose set isn't present on this NPC's ASC (otherwise the apply silently no-ops).
        if (!AbilitySystemComponent->HasAttributeSetForAttribute(Roll.Attribute)) {
            UE_LOG(Myth, Warning, TEXT("AMythicNPCCharacter::SeedAttributesFromData: %s has no attribute set for %s; skipping seed."),
                   *GetNameSafe(this), *Roll.Attribute.GetName());
            continue;
        }

        // Reset to the attribute set's authored default first. NPCs are pooled and InitAbilityActorInfo does
        // NOT reset base values, so without this an Additive roll would accumulate across every pool reuse.
        if (UClass *SetClass = Roll.Attribute.GetAttributeSetClass()) {
            const UAttributeSet *CDO = SetClass->GetDefaultObject<UAttributeSet>();
            AbilitySystemComponent->SetNumericAttributeBase(Roll.Attribute, Roll.Attribute.GetNumericValue(CDO));
        }

        AbilitySystemComponent->ApplyModToAttribute(Roll.Attribute, Roll.Definition.Modifier, Roll.Value);
    }
}

void AMythicNPCCharacter::OnReturnedToPool() {
    if (!HasAuthority()) {
        return;
    }

    // cancel all ongoing abilities to abort active states or animations
    if (AbilitySystemComponent) {
        AbilitySystemComponent->CancelAllAbilities();
    }

    // remove all active gameplay effects applied to this npc
    if (AbilitySystemComponent) {
        FGameplayTagContainer AllTags;
        AbilitySystemComponent->GetOwnedGameplayTags(AllTags);
        AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(AllTags);
    }

    // clear loose gameplay tags to prevent leftover state
    if (AbilitySystemComponent) {
        FGameplayTagContainer TempTags;
        AbilitySystemComponent->GetOwnedGameplayTags(TempTags);
        for (const FGameplayTag &Tag : TempTags) {
            AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
        }
    }

    // uninitialize life component from ability system to release delegates and timers
    if (LifeComponent) {
        LifeComponent->UninitializeFromAbilitySystem();
    }

    // clear all registered timers for the npc and its controller
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearAllTimersForObject(this);
        if (AController *AIController = GetController()) {
            World->GetTimerManager().ClearAllTimersForObject(AIController);
            AIController->UnPossess();
        }
    }

    // clean up local cached flags
    bCombatInitialized = false;
    AttackAbilityHandle = FGameplayAbilitySpecHandle();
}

const FGuid &AMythicNPCCharacter::GetNPCId() const {
    return this->NPCData.NPCId;
}

const FGameplayTag &AMythicNPCCharacter::GetNPCType() const {
    return this->NPCData.NPCType;
}

// Sets default values
AMythicNPCCharacter::AMythicNPCCharacter() {
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    AbilitySystemComponent = CreateDefaultSubobject<UMythicAbilitySystemComponent>("AbilitySystemComponent");
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    // Attributes
    LifeAttributes = CreateDefaultSubobject<UMythicAttributeSet_Life>("LifeAttributes");
    CombatAttributes = CreateDefaultSubobject<UMythicAttributeSet_NPCCombat>("CombatAttributes");
    DefenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Defense>("DefenseAttributes");
    OffenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Offense>("OffenseAttributes");

    // Cognition
    CognitiveBrain = CreateDefaultSubobject<UMythicCognitiveBrainComponent>("CognitiveBrain");

    // Vitals: death consumer + regen + kill XP.
    LifeComponent = CreateDefaultSubobject<UMythicLifeComponent>("LifeComponent");

    // Re-assert the Pawn-profile default (Visibility = Block) so the player's interaction scanner (sphere-sweeps
    // on ECC_Visibility) can focus this NPC even if a BP subclass overrode the capsule to ignore Visibility. The
    // base ACharacter capsule already blocks Visibility (Pawn profile), so on the C++ default this is a harmless
    // no-op safety net rather than a fix.
    if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
        Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
}

// ─── IMythicInteractable: talk to the NPC → contextual dialogue bark ───
// Entirely client-local: the interaction scanner runs only on the interacting player's controller and the line
// is a const read of the NPC's own brain state, so there is no server mutation and no RPC (unlike storage/
// conversion, which mutate authoritative state). The bark reaches only the interacting player.

void AMythicNPCCharacter::OnPrimaryInteract_Implementation(AActor *Interactor) {
    // Resolve the interacting controller (Interactor may be a controller or a pawn) — same logic as
    // AMythicStorageContainer::ResolveController.
    AController *C = Cast<AController>(Interactor);
    if (!C) {
        if (const APawn *P = Cast<APawn>(Interactor)) {
            C = P->GetController();
        }
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(C);
    if (!PC || !PC->IsLocalController()) {
        return;
    }

    // A recruit-eligible NPC routes the primary verb to the server recruit path. The 2-verb interaction cap is full
    // (Talk / Trade), so recruit folds into the primary verb rather than a new prompt key. The server re-validates
    // range + eligibility and dup-gates, and falls THROUGH to a normal dialogue bark for an already-recruited
    // companion — so you can still talk to a party member.
    if (IsRecruitable()) {
        PC->ServerRecruitNpc(this);
        return;
    }

    // Ask the SERVER to pick the line: the brain's dialogue context (Faction/Role/pressure) is authoritative +
    // non-replicated, so a client-side SelectDialogue would only ever produce the fallback. The PC round-trips
    // server->client (ServerRequestNpcDialogue -> SelectDialogueFor -> ClientReceiveNpcDialogue -> FireBark ->
    // OnNpcBark). On a listen-host/standalone the Server RPC runs locally with authority, so one path serves all
    // net modes.
    PC->ServerRequestNpcDialogue(this);
}

FText AMythicNPCCharacter::SelectDialogueFor(APlayerController *Interactor) const {
    return CognitiveBrain ? CognitiveBrain->SelectDialogue(Interactor) : FText::GetEmpty();
}

void AMythicNPCCharacter::FireBark(const FText &Line, APlayerController *Interactor) {
    OnNpcBark(Line, Interactor);
}

void AMythicNPCCharacter::OnSecondaryInteract_Implementation(AActor *Interactor) {
    // Open barter with a merchant NPC. Client-local: the offer catalog is EditDefaultsOnly class data the client
    // already has, so opening the UI needs no server round-trip — only executing an offer does
    // (AMythicPlayerController::ServerExecuteBarterOffer). Non-merchants no-op (Blueprints may still override).
    if (!IsMerchant()) {
        return;
    }
    AController *C = Cast<AController>(Interactor);
    if (!C) {
        if (const APawn *P = Cast<APawn>(Interactor)) {
            C = P->GetController();
        }
    }
    APlayerController *PC = Cast<APlayerController>(C);
    if (PC && PC->IsLocalController()) {
        OnTradeOpened(PC);
    }
}

bool AMythicNPCCharacter::IsActorInTradeRange(const AActor *Actor) const {
    if (TradeRangeSq <= 0.0f) {
        return true;
    }
    if (!Actor) {
        return false;
    }
    return FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= TradeRangeSq;
}

USceneComponent *AMythicNPCCharacter::GetWidgetAttachmentComponent_Implementation() const {
    return GetCapsuleComponent();
}

bool AMythicNPCCharacter::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    // Only offer the Talk prompt for NPCs with a cognitive brain (the dialogue source); mirrors the
    // conversion-station readiness gate.
    if (!CognitiveBrain) {
        return false;
    }
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicNPCCharacter::OnFocused_Implementation(AActor *Interactor) {
    // Visual feedback handled in Blueprint.
}

void AMythicNPCCharacter::OnUnfocused_Implementation(AActor *Interactor) {
    // Visual feedback handled in Blueprint.
}

UAbilitySystemComponent *AMythicNPCCharacter::GetAbilitySystemComponent() const {
    return AbilitySystemComponent;
}

void AMythicNPCCharacter::InitializeASC() {
    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    // Single-source combat readiness for BOTH spawn paths (pooled + MASS-embodied): grant the attack ability +
    // apply the default stat-init GEs. MUST run BEFORE the life wiring below, because InitializeWithAbilitySystem
    // calls ResetForRespawn -> SetHealth(GetMaxHealth()); if a default GE raises MaxHealth it has to be in place
    // first, otherwise an embodied NPC (no later OnSpawnedFromPool reset) would spawn at the ctor-default health.
    CombatInit();

    // Wire the vitals component to the ASC (binds the death consumer + starts regen). Idempotent.
    if (LifeComponent && !LifeComponent->IsInitialized()) {
        LifeComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
    }

    // Bind the living-world death teardown ONCE, server-only. OnDeath only ever broadcasts authority-side, and
    // InitializeASC runs up to 3x (BeginPlay/PossessedBy/OnSpawnedFromPool), so latch the bind. This is what
    // prevents a combat-killed embodied NPC from leaking as a zombie entity (see HandleNPCDeath).
    if (HasAuthority() && LifeComponent && !bBoundDeath) {
        LifeComponent->OnDeath.AddDynamic(this, &AMythicNPCCharacter::HandleNPCDeath);
        bBoundDeath = true;
    }
}

void AMythicNPCCharacter::HandleNPCDeath(AActor *DeadActor) {
    // Server-only; only MASS-embodied NPCs participate in the living-world perma-death contract + bridge teardown.
    // A pooled / designer-placed NPC (no valid source entity) keeps its existing behavior — out of scope here.
    if (!HasAuthority() || !CognitiveBrain) {
        return;
    }
    const FMassEntityHandle Entity = CognitiveBrain->GetSourceEntity();
    UMassEntitySubsystem *EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld());
    if (!EntitySubsystem || !Entity.IsSet() || !EntitySubsystem->GetEntityManager().IsEntityValid(Entity)) {
        return;
    }
    const FMythicIdentityFragment *Id =
        EntitySubsystem->GetEntityManager().GetFragmentDataPtr<FMythicIdentityFragment>(Entity);
    if (!Id) {
        return;
    }

    // Stop cognition immediately so the corpse doesn't keep thinking / spawning async work before it's destroyed.
    CognitiveBrain->StopThinking();

    // If this NPC was a player's companion, drop it from the party NOW — otherwise its slice-1b despawn-exemption keeps
    // it pinned Tier2, blocking the perma-death dehydration below from ever freeing its cognitive slot (and leaving a
    // stale party slot the player can't refill). RemoveMemberAt also stops its follow timer.
    if (UMythicPartySubsystem *Party = GetWorld()->GetSubsystem<UMythicPartySubsystem>()) {
        Party->RemoveCompanionFromAnyParty(this, /*bVoluntary*/ false);
    }

    // Invoke the Tier 2-3 PERMANENT-death contract: perma-dead set + causal Death.Permanent event + role
    // succession. IsPermaDead then permanently blocks re-promotion/re-embodiment, and the SignificanceProcessor's
    // perma-dead branch dehydrates this entity out of the cognitive set (freeing its slot) on its next tick.
    if (UGameInstance *GI = GetWorld()->GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            if (LWS->IsSystemActive()) {
                if (UMythicPersistentNPCRegistry *Reg = LWS->GetPersistentNPCRegistry()) {
                    Reg->RegisterDeath(Id->NameHash, Id->Faction, Id->RoleTag, Id->Cell,
                                       GetWorld()->GetTimeSeconds(), LWS);
                }
            }
        }
    }

    // Destroy the corpse after a delay so StartDeath's loot/XP tail (which runs AFTER the OnDeath broadcast)
    // completes on a live actor. Destroy routes through the CognitiveBrain's EndPlay -> UnregisterEmbodiedActor
    // cleanup (iter-30 fix). WeakLambda no-ops if the actor was already destroyed by the despawn consumer.
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(CorpseTimerHandle,
                                          FTimerDelegate::CreateWeakLambda(this, [this]() { Destroy(); }),
                                          CorpseLifetime, false);
    }
}

void AMythicNPCCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    // Safety net: guarantee a DESTROYED companion always leaves its party, regardless of which destroy path fired
    // (population far-despawn, encounter cleanup, level teardown, any future caller). HandleNPCDeath covers the GAS
    // death path; this covers every OTHER destroy so RemoveMemberAt is a truly unbypassable sink. Server-only (the
    // party subsystem is server-side); idempotent with HandleNPCDeath; no-op for non-companions.
    if (HasAuthority()) {
        if (UWorld *World = GetWorld()) {
            if (UMythicPartySubsystem *Party = World->GetSubsystem<UMythicPartySubsystem>()) {
                Party->RemoveCompanionFromAnyParty(this, /*bVoluntary*/ false);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AMythicNPCCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicNPCCharacter, AbilitySystemComponent);
}

void AMythicNPCCharacter::PossessedBy(AController *NewController) {
    Super::PossessedBy(NewController);
    InitializeASC();
}

// Called when the game starts or when spawned
void AMythicNPCCharacter::BeginPlay() {
    Super::BeginPlay();

    InitializeASC();
    // NOTE: base attributes are seeded in OnSpawnedFromPool (SeedAttributesFromData), which is the lifecycle
    // point where NPCData is populated; BeginPlay runs before NPCData is assigned for pooled actors.
}

void AMythicNPCCharacter::InitializeFromMassEntity(const FMassEntityHandle &InEntityHandle) {
    if (!HasAuthority()) {
        return;
    }

    UMassEntitySubsystem *EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld());
    if (!EntitySubsystem || !EntitySubsystem->GetEntityManager().IsEntityValid(InEntityHandle)) {
        return;
    }

    const FMassEntityManager &EntityManager = EntitySubsystem->GetEntityManager();

    const FMythicIdentityFragment *IdentityFrag = EntityManager.GetFragmentDataPtr<FMythicIdentityFragment>(InEntityHandle);
    const FMythicPersonalityFragment *PersonalityFrag = EntityManager.GetFragmentDataPtr<FMythicPersonalityFragment>(InEntityHandle);
    // FROZEN-CELL #34-r2: the brain's HomeCell anchor must come from the spawn-origin, NOT the now-live Identity.Cell.
    // Identity.Cell is refreshed to the actor's live cell for every moving embodied NPC (AMythicAIController::RefreshLiveCell),
    // so an NPC embodying mid-flight would otherwise snapshot its CURRENT cell as home (a Rest/Defend territory regression).
    // Schedule.HomeCell is set to the spawn-origin at every spawn site + is never live-mutated → the stable home anchor.
    const FMythicScheduleFragment *ScheduleFrag = EntityManager.GetFragmentDataPtr<FMythicScheduleFragment>(InEntityHandle);

    if (CognitiveBrain && IdentityFrag && PersonalityFrag && ScheduleFrag) {
        CognitiveBrain->InitializeBrain(
            IdentityFrag->Faction,
            ScheduleFrag->HomeCell,
            *PersonalityFrag,
            InEntityHandle,
            IdentityFrag->TrueFaction,
            IdentityFrag->RoleTag
            );
    }
}
