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
#include "AI/NPCs/MythicNPCAIController.h"
#include "AI/Party/PartySubsystem.h" // remove a dead companion from its party
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"            // AppearanceLibrary + skin/hair palette fields (Step 4)
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Appearance/AppearanceTypes.h"     // FMythicAppearanceResolver + code-default outfit/palettes
#include "World/LivingWorld/Factions/FactionDatabase.h"       // GetFaction snapshot (faction color override flag)
#include "World/LivingWorld/Factions/FactionColor.h"          // MythicFactionColor::GetFactionColor (resolved tint)
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"                 // resolve interactor standing component
#include "Player/MythicFactionStandingComponent.h"    // ServerAdjustStanding / GetStanding / thresholds
#include "AI/NPCs/MythicAIController.h"                // ForceEngageTarget (social-verb aggro + guard alert)
#include "AI/NPCs/MythicSocialVerbs.h"                // pure ResolveReaction + DefaultBarkFor
#include "World/LivingWorld/MythicTags_LivingWorld.h" // TAG_LIVINGWORLD_ACTION_VIOLENCE_ATTACK (guard-alert signal)
#include "EngineUtils.h"                              // TActorIterator (bounded guard-alert scan)


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

void AMythicNPCCharacter::SleepToPool() {
    // Pool RELEASE teardown. Order is LOCKED (embodiment-service-LOCK-v1 §1) and the comments call out WHY each step
    // is where it is — reordering reintroduces the cross-thread / stale-state traps this pair was built to close.
    if (!HasAuthority()) {
        return;
    }

    // (2) Halt cognition FIRST. StopThinking() clears the think timer AND joins the in-flight async BDI worker, so by
    //     the time the steps below tear down GAS / clear beliefs, no background thread is still dereferencing `this`.
    //     A creature's brain is never InitializeBrain'd, but StopThinking is a safe no-op then (clears an empty timer,
    //     waits on an invalid task).
    if (CognitiveBrain) {
        CognitiveBrain->StopThinking();
    }

    // Capture the AI controller BEFORE OnReturnedToPool unpossesses it (UnPossess clears GetController() but does NOT
    // destroy the controller). WakeFromPool re-possesses this same controller, so a park/wake cycle reuses one
    // controller instead of spawning + leaking a fresh one each time (the whole point of pooling — no churn).
    PooledController = GetController();

    // (3) Existing pooled-NPC GAS / timer / controller teardown: cancel abilities, strip all GEs + loose tags,
    //     UninitializeFromAbilitySystem, ClearAllTimersForObject (self + controller), UnPossess, reset combat latches.
    OnReturnedToPool();

    // (4) Wipe per-life BDI state so the next occupant starts clean. Only safe AFTER the worker join in step 2.
    if (CognitiveBrain) {
        CognitiveBrain->ResetForReuse();
    }

    // (5) Park the actor: invisible, non-colliding, non-ticking until re-acquired. The pool keeps it alive (the LWS
    //     EmbodimentPool holds a weak ref; the actor stays rooted as a normal UWorld actor — it is hidden, not GC'd).
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
}

void AMythicNPCCharacter::WakeFromPool() {
    // Pool ACQUIRE re-arm. Runs AFTER AcquireEmbodiedActor has repositioned + un-hidden + re-enabled collision, and
    // BEFORE the spawn processor calls InitializeFromMassEntity. Order is LOCKED (embodiment-service-LOCK-v1 §1).
    if (!HasAuthority()) {
        return;
    }

    // (2) Re-init the ASC: re-grant the attack ability (idempotent via AttackAbilityHandle) + re-wire the life
    //     component (idempotent via LifeComponent->IsInitialized). The OnDeath bind is SKIPPED by the bBoundDeath
    //     latch — it persists with the object across pooling, so HandleNPCDeath is bound exactly once for the
    //     actor's whole lifetime (no double-fire of the perma-death contract on reuse).
    InitializeASC();

    // (2b) Re-establish AI possession. SleepToPool's OnReturnedToPool ran AIController->UnPossess(), and BeginPlay's
    //      AutoPossessAI=PlacedInWorldOrSpawned does NOT re-fire for a recycled actor — so without this the woken NPC
    //      has no controller and would sit inert (no perception / patrol / companion-follow). Re-possess the SAME
    //      controller captured at park time (no churn); if it was destroyed (level teardown), spawn a fresh default
    //      one. Possess/SpawnDefaultController re-run PossessedBy -> InitializeASC, but InitializeASC is fully
    //      idempotent (CombatInit/life/death-bind all latched), so that second call is a safe no-op.
    if (!GetController()) {
        if (AController *Retained = PooledController.Get()) {
            Retained->Possess(this);
        }
        else {
            SpawnDefaultController();
        }
    }
    PooledController = nullptr;

    // (3) A previously-pooled actor may have died in its last life; snap Health back up to the (re-seeded) MaxHealth
    //     so it does not wake already dead. CombatInit ran inside InitializeASC above, so MaxHealth is in place first.
    if (LifeAttributes) {
        LifeAttributes->ResetForRespawn();
    }

    // (4) Re-enable movement + collision that a prior StartDeath disabled, so the reused actor can move/attack/be-hit.
    if (LifeComponent) {
        LifeComponent->RestoreAfterDeath();
    }

    // (5) Restore tick per the class default. PrimaryActorTick.bCanEverTick gates this, so for the base class (after
    //     the STEP 7 tick audit flips it false) this is a harmless no-op; a subclass that re-enabled tick in its ctor
    //     gets its tick back.
    SetActorTickEnabled(PrimaryActorTick.bCanEverTick);

    // Re-arm the staggered think loop (BeginPlay — the only OTHER arm site — does not re-run for a recycled actor).
    // The brain is re-bound to its new source entity by InitializeFromMassEntity, which the processor calls next.
    if (CognitiveBrain) {
        CognitiveBrain->StartThinking();
    }
}

const FGuid &AMythicNPCCharacter::GetNPCId() const {
    return this->NPCData.NPCId;
}

const FGameplayTag &AMythicNPCCharacter::GetNPCType() const {
    return this->NPCData.NPCType;
}

// Sets default values
AMythicNPCCharacter::AMythicNPCCharacter() {
    // Tick audit (embodiment-service-LOCK-v1 §3 STEP 7): per-frame AActor::Tick is UNUSED on this class, so disable it.
    // There is no AActor::Tick / TickActor override on AMythicNPCCharacter (or its only subclass AMythicCreatureCharacter,
    // which already defaults bCanEverTick=false). All AI is fully timer-driven and self-throttling, NOT per-frame:
    //   - AMythicAIController: Idle / Attack / Companion-follow run on IdleTimerHandle / AttackTimerHandle /
    //     FollowTimerHandle (the "Tick*" method names there are timer callbacks, not engine tick).
    //   - UMythicCognitiveBrainComponent: PrimaryComponentTick.bCanEverTick=false; the BDI loop runs on ThinkTimerHandle.
    // CharacterMovementComponent ticks on its OWN component tick (independent of the actor's bCanEverTick), so locomotion
    // and animation are unaffected — this only removes a dead per-actor tick that would otherwise scale with embodied
    // NPC count. WakeFromPool's SetActorTickEnabled(PrimaryActorTick.bCanEverTick) therefore becomes a no-op for the base
    // while still honoring any future BP/native subclass that re-enables tick in its own ctor for authored per-frame work.
    PrimaryActorTick.bCanEverTick = false;

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

    // AI possession: embodied Living-World NPCs (spawned by ActorSpawnProcessor as this class or a mesh-bearing BP
    // subclass) auto-possess a CONCRETE controller so perception / idle-patrol / companion-follow actually run. The
    // base AMythicAIController is Abstract (built to be subclassed by a combat BP); AMythicNPCAIController is its
    // spawnable concretization. A combat NPC BP may override AIControllerClass for authored attack behavior.
    AIControllerClass = AMythicNPCAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

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

// ─── Social interaction verbs ─────────────────────────────────

FMythicSocialReactionResult AMythicNPCCharacter::ResolveSocialVerb(EMythicSocialVerb Verb, APlayerController *Interactor) const {
    FMythicSocialReactionResult Result; // neutral default

    // The personality VentWeights live on the (authoritative, non-replicated) brain — resolution is SERVER-side.
    if (!HasAuthority() || !CognitiveBrain) {
        return Result;
    }

    // The interacting player's standing toward THIS NPC's faction (0 if no entry / no component / no faction). The
    // standing component also owns the Hostile/Friendly thresholds the friendly-verb bands key on.
    float Standing = 0.0f;
    float HostileThreshold = -50.0f; // component defaults; overwritten below when the component is present
    float FriendlyThreshold = 50.0f;
    const FMythicFactionId MyFaction = CognitiveBrain->GetFaction();
    if (Interactor) {
        if (const AMythicPlayerState *PS = Interactor->GetPlayerState<AMythicPlayerState>()) {
            if (const UMythicFactionStandingComponent *Standings = PS->GetFactionStanding()) {
                HostileThreshold = Standings->GetHostileThreshold();
                FriendlyThreshold = Standings->GetFriendlyThreshold();
                if (MyFaction.IsValid()) {
                    Standing = Standings->GetStanding(MyFaction);
                }
            }
        }
    }

    Result = UMythicSocialVerbLibrary::ResolveReaction(Verb, CognitiveBrain->GetPersonality(), Standing,
                                                       HostileThreshold, FriendlyThreshold);
    return Result;
}

void AMythicNPCCharacter::ApplySocialReaction(const FMythicSocialReactionResult &Result, EMythicSocialVerb Verb, APlayerController *Interactor) {
    if (!HasAuthority()) {
        return;
    }

    // (0) Cache the verb/reaction for the gameplay debugger (server-side observability only; not replicated, no sim state).
    LastSocialVerb = Verb;
    LastSocialReaction = Result.Reaction;
    LastSocialReactionTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    bHasSocialReaction = true;

    // (1) Reputation: move the interactor's standing toward this NPC's faction by the resolved (signed) delta.
    if (Result.StandingDelta != 0.0f && Interactor && CognitiveBrain) {
        const FMythicFactionId MyFaction = CognitiveBrain->GetFaction();
        if (MyFaction.IsValid()) {
            if (AMythicPlayerState *PS = Interactor->GetPlayerState<AMythicPlayerState>()) {
                if (UMythicFactionStandingComponent *Standings = PS->GetFactionStanding()) {
                    Standings->ServerAdjustStanding(MyFaction, Result.StandingDelta);
                }
            }
        }
    }

    APawn *InteractorPawn = Interactor ? Interactor->GetPawn() : nullptr;

    // (2) Aggro: an Angered NPC turns hostile on the interacting player via the shared engage path.
    if (Result.bSetHostile && InteractorPawn) {
        if (AMythicAIController *AI = Cast<AMythicAIController>(GetController())) {
            AI->ForceEngageTarget(InteractorPawn);
        }
    }

    // (3) Guard alert: rouse nearby ALLIED NPCs (those whose AI treats the interacting player as Hostile). Bounded by
    // radius + responder cap; a rare one-shot social event, never a tick — TActorIterator is acceptable here.
    if (Result.bAlertGuards && InteractorPawn) {
        const float RadiusSq = GuardAlertRadius * GuardAlertRadius;
        const FVector MyLoc = GetActorLocation();
        int32 Roused = 0;
        for (TActorIterator<AMythicNPCCharacter> It(GetWorld()); It; ++It) {
            if (Roused >= GuardAlertMaxResponders) {
                break;
            }
            AMythicNPCCharacter *Responder = *It;
            if (!IsValid(Responder) || Responder == this) {
                continue;
            }
            if (GuardAlertRadius > 0.0f &&
                FVector::DistSquared(MyLoc, Responder->GetActorLocation()) > RadiusSq) {
                continue;
            }
            AMythicAIController *RespAI = Cast<AMythicAIController>(Responder->GetController());
            if (!RespAI) {
                continue;
            }
            // Only allies respond: an ally is an NPC whose attitude toward the interacting player is Hostile (faction-
            // + reputation-aware, the single source of truth). This naturally excludes the player's own companions and
            // factions the player is in good standing with.
            if (RespAI->GetTeamAttitudeTowards(*InteractorPawn) != ETeamAttitude::Hostile) {
                continue;
            }
            // Force an immediate re-think (cognitive context) AND commit the responder to the fight.
            if (Responder->CognitiveBrain) {
                Responder->CognitiveBrain->OnSignificantEvent(TAG_LIVINGWORLD_ACTION_VIOLENCE_ATTACK,
                                                              Responder->CognitiveBrain->GetHomeCell());
            }
            RespAI->ForceEngageTarget(InteractorPawn);
            ++Roused;
        }
    }

    // (4) NOTE: surfacing the bark/reaction to the player is done by the PC's ClientReceiveSocialReaction RPC (which
    // runs locally on a listen-host too), mirroring the dialogue path — so we do NOT fire OnNpcReaction here, to avoid
    // a double-fire on a listen-host. ApplySocialReaction is sim-mutation only.
}

void AMythicNPCCharacter::FireReaction(EMythicSocialVerb Verb, EMythicSocialReaction Reaction, const FText &Line, APlayerController *Interactor) {
    OnNpcReaction(Verb, Reaction, Line, Interactor);
}

// ─── Context-driven activity (Step 3) ───

void AMythicNPCCharacter::ServerSetActivity(FGameplayTag ActivityTag) {
    // SERVER-only (the AIController that drives this is server-side). Change-gated: only broadcast on a real change so we
    // never re-multicast the same cosmetic every idle dispatch while the NPC keeps performing the same activity.
    if (!HasAuthority()) {
        return;
    }
    if (CurrentActivityTag == ActivityTag) {
        return;
    }
    CurrentActivityTag = ActivityTag;
    // Cosmetic to all clients (Unreliable). _Implementation runs OnPerformActivity on each client; on the server/listen-host
    // a NetMulticast also executes locally, so the host's own NPC plays the cosmetic too (no separate direct call needed).
    Multicast_PerformActivity(ActivityTag);
}

void AMythicNPCCharacter::Multicast_PerformActivity_Implementation(FGameplayTag ActivityTag) {
    // Purely cosmetic: hand the tag to the Blueprint so it can play the matching montage/anim/prop. No sim state.
    OnPerformActivity(ActivityTag);
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
                // Kill → world-sim feedback: durably decrement the governing faction's Population (and Arms reserve for
                // armed roles) so this death lowers future spawn density + military strength. Composes with — does not
                // duplicate — the kill→standing (MythicLifeComponent::StartDeath) + perma-death (RegisterDeath above)
                // paths; reuses the already-resolved Id + LWS (zero new lookups). Internally SimulationLock-guarded and
                // a no-op for a factionless NPC, so no guard is needed here.
                LWS->ReportNpcDeath(Id->Faction, Id->RoleTag);
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
    // Step 4: the resolved wardrobe descriptor. The ONLY new replicated property this cluster adds (single edit, per the
    // GetLifetimeReplicatedProps single-owner contract). Server resolves + assigns it; clients apply via OnRep_Appearance.
    DOREPLIFETIME(AMythicNPCCharacter, Appearance);
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

    // ─── Appearance (Step 4) ───
    // Resolve + assign + replicate the wardrobe from the stable identity seed. Server-only (we're already inside the
    // HasAuthority guard at the top of this function). Runs on EVERY embodiment, so a pooled actor reused for a new
    // occupant overwrites the previous look (no carry-over). Skipped naturally by AMythicCreatureCharacter, which
    // overrides InitializeFromMassEntity without calling Super.
    if (IdentityFrag) {
        ApplyAppearanceFromIdentity(*IdentityFrag);
    }
}

void AMythicNPCCharacter::OnRep_Appearance() {
    // Client received a new descriptor → hand it to the art Blueprint. The replication system already dirty-checks via the
    // struct's operator==, so a redundant assignment of an identical descriptor won't fire this. Pure cosmetic; no auth needed.
    OnApplyAppearance(Appearance);
}

void AMythicNPCCharacter::ApplyAppearanceFromIdentity(const FMythicIdentityFragment &Id) {
    // SERVER ONLY. Caller (InitializeFromMassEntity) is already authority-guarded, but guard again so a direct call is safe.
    if (!HasAuthority()) {
        return;
    }

    // ─── Resolve the faction colors (null-tolerant) ───
    // Default to neutral white tints; upgraded to the faction's resolved color when the faction DB has this faction.
    FColor PrimaryColor = FColor::White;
    const uint8 FactionIndex = Id.Faction.Index;

    UMythicLivingWorldSubsystem *LWS = nullptr;
    if (const UWorld *World = GetWorld()) {
        if (UGameInstance *GI = World->GetGameInstance()) {
            LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
        }
    }

    if (LWS) {
        if (const UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase()) {
            FMythicFactionData FactionData;
            if (Id.Faction.IsValid() && FactionDB->GetFaction(Id.Faction, FactionData)) {
                // Resolved faction display color (authored override or deterministic-from-id). Single source of truth
                // shared with the war-map / UI so a faction's livery is identical everywhere.
                PrimaryColor = MythicFactionColor::GetFactionColor(FactionData, FactionIndex);
            } else if (Id.Faction.IsValid()) {
                // Faction has an index but no DB row (resistance/transient) → still give it a stable deterministic color.
                PrimaryColor = MythicFactionColor::DeterministicColorForId(FactionIndex);
            }
        }
    }

    // Derive a secondary/accent tint from the primary: a darkened shade (trim/accents read as a deeper version of the
    // livery). Deterministic (pure function of the primary), so it's stable wherever the appearance is rebuilt.
    const FColor SecondaryColor = (FLinearColor(PrimaryColor) * 0.6f).ToFColor(true);

    // ─── Resolve the outfit-set + palette source (authored library, else code defaults) ───
    // Hoist the soft-ptr load: LoadSynchronous is a one-time resolve of the configured library; we do NOT load per-NPC
    // per-frame (this whole function runs once per embodiment). Null library / null settings → code defaults.
    TConstArrayView<FMythicOutfitSet> OutfitSets = MythicAppearanceDefaults::GetCodeDefaultOutfitSets();
    TConstArrayView<FColor> SkinPalette = MythicAppearanceDefaults::GetCodeDefaultSkinTonePalette();
    TConstArrayView<FColor> HairPalette = MythicAppearanceDefaults::GetCodeDefaultHairTonePalette();

    if (LWS) {
        if (const UMythicLivingWorldSettings *Settings = LWS->GetSettings()) {
            if (!Settings->AppearanceLibrary.IsNull()) {
                if (const UMythicAppearanceLibrary *Library = Settings->AppearanceLibrary.LoadSynchronous()) {
                    if (Library->OutfitSets.Num() > 0) {
                        OutfitSets = Library->OutfitSets;
                    }
                }
            }
            if (Settings->DefaultSkinTonePalette.Num() > 0) {
                SkinPalette = Settings->DefaultSkinTonePalette;
            }
            if (Settings->DefaultHairTonePalette.Num() > 0) {
                HairPalette = Settings->DefaultHairTonePalette;
            }
        }
    }

    // ─── Pure deterministic resolve (no allocations beyond the resolver's locals) ───
    const uint8 WealthTier = FMythicAppearanceResolver::WealthTierFromHash(Id.NameHash);
    const FMythicAppearance Resolved = FMythicAppearanceResolver::Resolve(
        Id.NameHash,
        Id.DemographicFlags,
        Id.RoleTag,
        FactionIndex,
        WealthTier,
        PrimaryColor,
        SecondaryColor,
        OutfitSets,
        SkinPalette,
        HairPalette);

    Appearance = Resolved;

    // OnRep doesn't fire on the authority, so apply directly on the server/listen-host. The art BP no-ops on a dedicated
    // server (no mesh), so this is harmless there; on a listen-host it builds the host's view of the NPC.
    OnApplyAppearance(Appearance);
}
