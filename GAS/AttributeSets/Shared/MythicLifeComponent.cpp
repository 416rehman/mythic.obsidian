// 


#include "MythicLifeComponent.h"

#include "Mythic.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameModes/MythicGameMode.h"
#include "Engine/World.h"
#include "MythicAttributeSet_Defense.h"
#include "MythicAttributeSet_Utility.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/MythicPlayerController.h"
#include "MythicAttributeSet_Life.h"                  // EMythicLethalOutcome + ResolveLethalOutcome + Health attr
#include "Settings/MythicDeveloperSettings.h"         // co-op down policy (default-off)
#include "TimerManager.h"
#include "AbilitySystemGlobals.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/Party/PartySubsystem.h" // companion loyalty reacts to player kills (OnPlayerAction)
#include "Player/MythicPlayerState.h"
#include "Itemization/Inventory/MythicEncumbrance.h"        // ComputeTier / SpeedMultiplierForTier (carry-weight slow)
#include "Itemization/Inventory/MythicInventoryComponent.h" // GetTotalCarriedWeight
#include "Itemization/InventoryProviderInterface.h"         // reach the owner's inventory components
#include "Player/MythicFactionStandingComponent.h"
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"


UMythicLifeComponent::UMythicLifeComponent(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);

    AbilitySystemComponent = nullptr;
    LifeSet = nullptr;
}

void UMythicLifeComponent::OnUnregister() {
    UninitializeFromAbilitySystem();

    Super::OnUnregister();
}

void UMythicLifeComponent::InitializeWithAbilitySystem(UAbilitySystemComponent *InASC) {
    AActor *Owner = GetOwner();
    check(Owner);

    if (AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: Health component for owner [%s] has already been initialized with an ability system."),
               *GetNameSafe(Owner));
        return;
    }

    AbilitySystemComponent = InASC;
    if (!AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: Cannot initialize health component for owner [%s] with NULL ability system."), *GetNameSafe(Owner));
        return;
    }

    LifeSet = AbilitySystemComponent->GetSet<UMythicAttributeSet_Life>();
    if (!LifeSet) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: Cannot initialize health component for owner [%s] with NULL health set on the ability system."),
               *GetNameSafe(Owner));
        return;
    }

    // Register to listen for attribute changes.
    LifeSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
    LifeSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);

    // Consume the owner's death event (fired server-side by the Life set on lethal damage). The handler
    // self-gates on authority, so binding on clients is harmless (the event is only raised server-side).
    if (OnDeathGameplayEventTag.IsValid()) {
        AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(OnDeathGameplayEventTag).AddUObject(this, &ThisClass::HandleDeathEvent);
    }

    // Consume the owner's received-hit event to STAGGER on a heavy hit (a transient STUNNED that the CC handler
    // below turns into a movement halt). Same server-only-event + harmless-on-clients shape as the death bind.
    if (OnReceivedHitGameplayEventTag.IsValid()) {
        AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(OnReceivedHitGameplayEventTag).AddUObject(this, &ThisClass::HandleReceivedHit);
    }

    // Consume the owner's DELIVERED-hit event (fired server-side ON THE INSTIGATOR by the victim's Life set when this
    // owner lands damage) for LIFESTEAL: heal LifePerHit per hit. Same server-only-event + harmless-on-clients shape.
    if (OnDeliveredHitGameplayEventTag.IsValid()) {
        AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(OnDeliveredHitGameplayEventTag).AddUObject(this, &ThisClass::HandleDamageDelivered);
    }

    // Consume the owner's KILL event (fired server-side ON THE KILLER by the victim's Life set on a lethal blow) for
    // LIFESTEAL-ON-KILL. Same server-only-event + harmless-on-clients shape.
    if (OnKillGameplayEventTag.IsValid()) {
        AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(OnKillGameplayEventTag).AddUObject(this, &ThisClass::HandleKill);
    }

    // Enforce crowd-control debuffs in C++: react to the Stun/Freeze/Slow tags (applied by the debuff GE pipeline)
    // changing on the owning ASC and adjust movement. NewOrRemoved fires wherever the GE-granted tag replicates
    // (server + owning client) — exactly what client-predicted movement needs. Capture the base walk speed first so
    // Slow scales/restores against a stable value.
    if (const ACharacter *Char = Cast<ACharacter>(GetOwner())) {
        if (const UCharacterMovementComponent *Move = Char->GetCharacterMovement()) {
            BaseWalkSpeed = Move->MaxWalkSpeed;
        }
    }
    const FGameplayTag SprintTag = GAS_STATE_SPRINTING;
    const FGameplayTag MovementAffectingTags[] = {GAS_DEBUFF_STUNNED, GAS_DEBUFF_FROZEN, GAS_DEBUFF_SLOWED, GAS_BUFF_HASTE, SprintTag};
    for (const FGameplayTag &MoveTag : MovementAffectingTags) {
        if (MoveTag.IsValid()) {
            AbilitySystemComponent->RegisterGameplayTagEvent(MoveTag, EGameplayTagEventType::NewOrRemoved).AddUObject(
                this, &ThisClass::HandleCrowdControlTagChanged);
        }
    }

    // Encumbrance: recompute move speed when carried weight changes, not only on CC events. This init runs post-
    // possession (the ASC/avatar is ready), so the owner's controller + inventory provider exist for a player; binds
    // are skipped (no-op) for an owner with no inventory provider (NPC). Re-init re-binds cleanly (Unbind-first).
    BindEncumbranceInventoryDelegates();

    // Start the server-side regen tick (Health / Shield / Stamina toward max).
    if (GetOwner()->HasAuthority() && RegenInterval > 0.0f && GetWorld()) {
        GetWorld()->GetTimerManager().SetTimer(RegenTimerHandle, this, &ThisClass::ApplyRegen, RegenInterval, true);
    }

    auto HealthAttr = UMythicAttributeSet_Life::GetHealthAttribute();
    auto MaxHealthAttr = UMythicAttributeSet_Life::GetMaxHealthAttribute();

    // SERVER ONLY: start fresh - full health, clear the death latch + tags (also fixes respawn, since the
    // player's ASC lives on the persistent PlayerState and is reused across pawns). Clients only read + broadcast.
    if (GetOwner()->HasAuthority()) {
        const_cast<UMythicAttributeSet_Life *>(LifeSet.Get())->ResetForRespawn();
        // Seed stamina to full so it's usable immediately.
        if (const UMythicAttributeSet_Utility *Util = AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>()) {
            AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(), Util->GetMaxStamina());
        }
    }

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(HealthAttr).AddUObject(this, &ThisClass::TriggerHealthChange);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxHealthAttr).AddUObject(this, &ThisClass::TriggerMaxHealthChange);

    // Initial broadcast of health values.
    auto Health = AbilitySystemComponent->GetNumericAttribute(HealthAttr);
    auto MaxHealth = AbilitySystemComponent->GetNumericAttribute(MaxHealthAttr);

    const FGameplayEffectContextHandle EmptyContextHandle = FGameplayEffectContextHandle();
    OnHealthChanged.Broadcast(Health, 0, HealthAttr, EmptyContextHandle);
    OnMaxHealthChanged.Broadcast(MaxHealth, 0, MaxHealthAttr, EmptyContextHandle);

    // Reconcile movement against any CC tags ALREADY present on the (possibly reused/persistent PlayerState) ASC.
    // RegisterGameplayTagEvent(NewOrRemoved) only fires on a future 0<->1 transition, not on bind — so without this
    // a player who respawns while still Slowed/Stunned would get a fresh full-speed pawn that ignores the live tag.
    // Safe here: the SERVER-only ResetForRespawn above has already cleared the Dead tag, so the dead-guard inside
    // ReevaluateCrowdControl won't suppress a legitimate (re)application.
    ReevaluateCrowdControl();
}

void UMythicLifeComponent::UninitializeFromAbilitySystem() {
    ClearGameplayTags();

    // Drop the encumbrance inventory-change subscriptions so no dangling delegate points at this torn-down component
    // (the inventories outlive the pawn — they live on the controller — so this must be explicit).
    UnbindEncumbranceInventoryDelegates();

    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(RegenTimerHandle);
    }

    // Tear down any in-flight stagger (timer + transient loose STUNNED tag) via the single-source helper, so the
    // tag doesn't dangle on a reused (persistent) ASC.
    ClearStagger();

    if (AbilitySystemComponent && OnDeathGameplayEventTag.IsValid()) {
        if (FGameplayEventMulticastDelegate *Del = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(OnDeathGameplayEventTag)) {
            Del->RemoveAll(this);
        }
    }

    if (AbilitySystemComponent && OnReceivedHitGameplayEventTag.IsValid()) {
        if (FGameplayEventMulticastDelegate *Del = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(OnReceivedHitGameplayEventTag)) {
            Del->RemoveAll(this);
        }
    }

    if (AbilitySystemComponent && OnDeliveredHitGameplayEventTag.IsValid()) {
        if (FGameplayEventMulticastDelegate *Del = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(OnDeliveredHitGameplayEventTag)) {
            Del->RemoveAll(this);
        }
    }

    if (AbilitySystemComponent && OnKillGameplayEventTag.IsValid()) {
        if (FGameplayEventMulticastDelegate *Del = AbilitySystemComponent->GenericGameplayEventCallbacks.Find(OnKillGameplayEventTag)) {
            Del->RemoveAll(this);
        }
    }

    if (AbilitySystemComponent) {
        // MUST mirror the registration array in InitializeWithAbilitySystem (Stun/Frozen/Slowed + Haste). A tag missing
        // here leaks a stale HandleCrowdControlTagChanged binding on the PERSISTENT PlayerState ASC across pawn reuse.
        const FGameplayTag SprintTag = GAS_STATE_SPRINTING;
        const FGameplayTag MovementAffectingTags[] = {GAS_DEBUFF_STUNNED, GAS_DEBUFF_FROZEN, GAS_DEBUFF_SLOWED, GAS_BUFF_HASTE, SprintTag};
        for (const FGameplayTag &MoveTag : MovementAffectingTags) {
            if (MoveTag.IsValid()) {
                AbilitySystemComponent->RegisterGameplayTagEvent(MoveTag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
            }
        }
    }

    if (LifeSet) {
        LifeSet->OnHealthChanged.RemoveAll(this);
        LifeSet->OnMaxHealthChanged.RemoveAll(this);
    }

    LifeSet = nullptr;
    AbilitySystemComponent = nullptr;
}

void UMythicLifeComponent::HandleReceivedHit(const FGameplayEventData *Payload) {
    // Server-authoritative (the received-hit event is raised server-side; defensive re-check).
    if (!Payload || !AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    // Don't stagger the dead, and don't stack staggers (the active one's recovery timer will clear it).
    if (bStaggered || AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return;
    }
    // Heavy hit? The threshold is a FRACTION of MaxHealth so it scales with the entity — a chip hit doesn't stagger.
    if (!IsHeavyHit(Payload->EventMagnitude, GetMaxHealth(), HeavyHitHealthFraction)) {
        return;
    }
    // Stagger immunity (measured from the last stagger's start) prevents stun-lock from rapid heavy hits.
    UWorld *W = GetWorld();
    const double Now = W ? W->GetTimeSeconds() : 0.0;
    if (W && IsStaggerImmune(Now, LastStaggerTime, StaggerImmunityWindow)) {
        return;
    }
    LastStaggerTime = Now;
    bStaggered = true;

    // Apply a transient STUNNED LOOSE tag (additive — balanced by the RemoveLooseGameplayTag below, so it composes
    // with any other STUNNED source instead of clobbering it). The iter-48 ReevaluateCrowdControl binding halts
    // movement on the 0->1 edge; the recovery timer clears it (1->0 edge restores movement).
    AbilitySystemComponent->AddLooseGameplayTag(GAS_DEBUFF_STUNNED);
    if (W) {
        W->GetTimerManager().SetTimer(StaggerTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
            if (AbilitySystemComponent) {
                AbilitySystemComponent->RemoveLooseGameplayTag(GAS_DEBUFF_STUNNED);
            }
            bStaggered = false;
        }), StaggerDuration, /*bLoop=*/false);
    }
    else {
        // No world to schedule recovery (shouldn't happen) — don't latch a permanent stun.
        AbilitySystemComponent->RemoveLooseGameplayTag(GAS_DEBUFF_STUNNED);
        bStaggered = false;
    }
}

void UMythicLifeComponent::HandleDeathEvent(const FGameplayEventData *Payload) {
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return; // server-authoritative
    }
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return; // already dead - idempotent (guards the Damage-path + Health-path both raising death)
    }
    // The death event carries the killer as its instigator.
    AActor *Killer = Payload ? const_cast<AActor *>(Payload->Instigator.Get()) : nullptr;

    // Co-op down/revive gate (default OFF → always Die, byte-identical to before). When enabled, a revivable player
    // taking a lethal blow enters the downed state instead of dying. The decision is the unit-tested ResolveLethalOutcome.
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const bool bDownEnabled = Settings && Settings->bCoopDownStateEnabled;
    const EMythicLethalOutcome Outcome =
        UMythicAttributeSet_Life::ResolveLethalOutcome(/*bWouldBeLethal*/ true, bDownEnabled, bIsDowned, IsOwnerRevivablePlayer());
    if (Outcome == EMythicLethalOutcome::EnterDownState) {
        EnterDownedState(Killer);
        return;
    }
    StartDeath(Killer);
}

bool UMythicLifeComponent::IsOwnerRevivablePlayer() const {
    const APawn *OwnerPawn = Cast<APawn>(GetOwner());
    return OwnerPawn && OwnerPawn->IsPlayerControlled();
}

bool UMythicLifeComponent::CanReviveTarget(const bool bTargetDowned, const bool bReviverDowned) {
    // Only a downed target can be revived, and only by a reviver who isn't downed themselves.
    return bTargetDowned && !bReviverDowned;
}

void UMythicLifeComponent::EnterDownedState(AActor *Killer) {
    if (!AbilitySystemComponent) {
        return;
    }
    AActor *Owner = GetOwner();
    bIsDowned = true;
    DownedKiller = Killer;

    // Incapacitate, but DON'T latch Dead — this is recoverable. Cancel abilities + clear any stagger so the downed
    // owner can't act or carry a leftover stun.
    AbilitySystemComponent->SetLooseGameplayTagCount(GAS_STATE_DOWNED, 1);
    AbilitySystemComponent->CancelAllAbilities();
    ClearStagger();

    // Disable MOVEMENT only — keep collision so the body stays present and reachable by a reviving teammate.
    if (ACharacter *Char = Cast<ACharacter>(Owner)) {
        if (UCharacterMovementComponent *Move = Char->GetCharacterMovement()) {
            Move->StopMovementImmediately();
            Move->DisableMovement();
        }
    }

    UE_LOG(Myth, Log, TEXT("LifeComponent: %s is DOWNED (co-op)."), *GetNameSafe(Owner));
    OnDowned.Broadcast(Owner); // cosmetics: down pose + revive prompt (client cosmetics off the OnRep/delegate)

    // Bleed-out: die for real if no revive arrives in time.
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float BleedOut = Settings ? Settings->DownedBleedOutSeconds : 30.0f;
    if (UWorld *W = GetWorld(); W && BleedOut > 0.0f) {
        W->GetTimerManager().SetTimer(BleedOutTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
            if (!bIsDowned) {
                return; // already revived
            }
            bIsDowned = false;
            if (AbilitySystemComponent) {
                AbilitySystemComponent->SetLooseGameplayTagCount(GAS_STATE_DOWNED, 0);
            }
            StartDeath(DownedKiller.Get());
        }), BleedOut, /*bLoop=*/false);
    }
}

void UMythicLifeComponent::ServerReviveFromDowned() {
    if (!AbilitySystemComponent || !bIsDowned) {
        return;
    }
    if (UWorld *W = GetWorld()) {
        W->GetTimerManager().ClearTimer(BleedOutTimerHandle);
    }
    bIsDowned = false;
    DownedKiller = nullptr;
    AbilitySystemComponent->SetLooseGameplayTagCount(GAS_STATE_DOWNED, 0);

    // Restore movement (the downed state only disabled movement; collision was kept).
    if (ACharacter *Char = Cast<ACharacter>(GetOwner())) {
        if (UCharacterMovementComponent *Move = Char->GetCharacterMovement()) {
            Move->SetMovementMode(MOVE_Walking);
        }
    }

    // Heal to the configured revive fraction — restoring Health above zero clears the out-of-health death latch via
    // the attribute machinery (same path a recovery heal uses).
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float Fraction = Settings ? Settings->ReviveHealthFraction : 0.5f;
    if (LifeSet) {
        const float NewHealth = FMath::Max(1.0f, LifeSet->GetMaxHealth() * Fraction);
        AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), NewHealth);
    }

    UE_LOG(Myth, Log, TEXT("LifeComponent: %s REVIVED from downed."), *GetNameSafe(GetOwner()));
    OnRevived.Broadcast(GetOwner());
}

void UMythicLifeComponent::ClearStagger() {
    // Single source for the stagger teardown (StartDeath / Uninitialize / pool-return all call this). Clears the
    // recovery timer, removes the transient loose STUNNED tag if one is held, and resets the stagger state — so a
    // reused (persistent) ASC never carries a leftover STUNNED tag or an orphaned weak-lambda timer onto its next life.
    if (UWorld *W = GetWorld()) {
        W->GetTimerManager().ClearTimer(StaggerTimerHandle);
    }
    if (bStaggered && AbilitySystemComponent) {
        AbilitySystemComponent->RemoveLooseGameplayTag(GAS_DEBUFF_STUNNED);
    }
    bStaggered = false;
    LastStaggerTime = 0.0;
}

void UMythicLifeComponent::StartDeath(AActor *Killer) {
    if (!AbilitySystemComponent) {
        return;
    }
    AActor *Owner = GetOwner();

    // Latch dead state (server-side; drives regen-pause + ability blocking). NOTE: these loose tags are not
    // replicated, so client death cosmetics must be driven by the OnDeath delegate / BP_OnDeath until a
    // replicated death signal is added (deferred follow-up).
    AbilitySystemComponent->SetLooseGameplayTagCount(GAS_STATE_DYING, 1);
    AbilitySystemComponent->SetLooseGameplayTagCount(GAS_STATE_DEAD, 1);
    AbilitySystemComponent->CancelAllAbilities();

    // Tear down any in-flight stagger (single-source helper) so its transient STUNNED loose tag + pending recovery
    // timer don't survive onto a reused pooled actor (which would spawn movement-locked via ReevaluateCrowdControl).
    ClearStagger();

    if (ACharacter *Char = Cast<ACharacter>(Owner)) {
        if (UCharacterMovementComponent *Move = Char->GetCharacterMovement()) {
            Move->StopMovementImmediately();
            Move->DisableMovement();
        }
        if (UCapsuleComponent *Capsule = Char->GetCapsuleComponent()) {
            Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    UE_LOG(Myth, Log, TEXT("LifeComponent: %s died."), *GetNameSafe(Owner));

    // Hooks: BP cosmetics (ragdoll/montage) + C++ listeners (NPC pooling, loot drop).
    BP_OnDeath();
    OnDeath.Broadcast(Owner);

    // award combat proficiency XP to the killer via their ProficiencyComponent
    if (XPReward > 0.0f && Killer && Killer != Owner) {
        APlayerController *KillerPC = nullptr;
        if (const APawn *KillerPawn = Cast<APawn>(Killer)) {
            KillerPC = Cast<APlayerController>(KillerPawn->GetController());
        }
        else {
            KillerPC = Cast<APlayerController>(Killer);
        }
        if (KillerPC) {
            if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(KillerPC)) {
                if (UProficiencyComponent *ProfComp = const_cast<UProficiencyComponent*>(MythicPC->GetProficiencyComponent())) {
                    ProfComp->GrantCombatXP(XPReward);
                }
            }
        }
    }

    // Faction reputation: if a player killed a member of a faction, apply that player's standing consequence — a direct
    // penalty with the victim's faction PLUS data-driven propagation across the faction-relationship graph (allies
    // resent the kill, enemies approve). ServerApplyKillStanding owns the politics; we just supply the victim faction.
    // The victim's faction comes from its cognitive brain; the killer's standing store lives on its PlayerState.
    if (Killer && Killer != Owner && Owner) {
        if (UMythicCognitiveBrainComponent *VictimBrain = Owner->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
            const FMythicFactionId VictimFaction = VictimBrain->GetFaction();
            if (VictimFaction.IsValid()) {
                AMythicPlayerState *KillerPS = nullptr;
                if (const APawn *KillerPawn = Cast<APawn>(Killer)) {
                    KillerPS = KillerPawn->GetPlayerState<AMythicPlayerState>();
                }
                else if (const AController *KillerController = Cast<AController>(Killer)) {
                    KillerPS = KillerController->GetPlayerState<AMythicPlayerState>();
                }
                if (KillerPS) {
                    if (UMythicFactionStandingComponent *Standing = KillerPS->GetFactionStanding()) {
                        Standing->ServerApplyKillStanding(VictimFaction);
                    }
                }
            }
        }
    }

    // Feed this kill into the Living World witness/crime pipeline as a real moral action (perpetrator = killer,
    // victim = this actor). Uses the canonical kill action tag + the SAME death-event moral vector the persistent-
    // NPC registry records (Violence/Mercy), so nothing is fabricated; the embodied path additionally carries the
    // real perpetrator + gets on-the-spot witness resolution. Server-only: the ActionEventSubsystem only exists on
    // authority (and StartDeath runs server-side), so GetSubsystem returns null on clients and this no-ops.
    if (Killer && Killer != Owner) {
        if (UMythicActionEventSubsystem *ActionSub = GetWorld() ? GetWorld()->GetSubsystem<UMythicActionEventSubsystem>() : nullptr) {
            FMythicActionEvent KillAction;
            KillAction.Perpetrator = Killer;
            KillAction.Victim = Owner;
            // Carry the REAL factions (ResolveActorFaction in the subsystem is a stub that returns invalid). Read
            // them from the actors' cognitive brains — the same source the faction-standing block above uses. A
            // player killer has no NPC-style faction membership (its culpability is tracked via FactionStanding on
            // the PlayerState), so an invalid perp override there is acceptable; the victim attribution is the win.
            if (UMythicCognitiveBrainComponent *VictimBrain = Owner->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
                KillAction.VictimFactionOverride = VictimBrain->GetFaction();
            }
            if (UMythicCognitiveBrainComponent *KillerBrain = Killer->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
                KillAction.PerpFactionOverride = KillerBrain->GetFaction();
            }
            KillAction.ActionTag = TAG_LIVINGWORLD_ACTION_VIOLENCE_KILL;
            KillAction.CategoryFlags = EMythicEventCategory::Combat | EMythicEventCategory::Death;
            KillAction.Significance = 1.0f;
            // Canonical kill moral vector (Rule 3 single source). CONVENTION: harm is POSITIVE Violence, so an
            // anti-violence faction/companion condemns the kill (Severity = -DotProduct > 0). The prior inline -0.9
            // INVERTED this — pacifist factions/companions silently APPROVED of murder (batch 147-150 review, HIGH).
            KillAction.MoralVector = FMythicMoralSignature::MakeKillActionMoralVector();
            ActionSub->SubmitAction(KillAction);

            // Companion loyalty (wakes the dormant party loyalty/betrayal system): when the killer is a PLAYER, their
            // party companions judge this kill against their OWN faction ideology — fed the SAME moral vector submitted
            // above (single source, nothing fabricated). A companion whose faction abhors violence loses loyalty / gains
            // betrayal pressure; one aligned with the kill is unmoved or pleased. The party is keyed by the killer's
            // CANONICAL player key (GetCanonicalPlayerKey), so in co-op only the KILLER's companions judge their own
            // leader's act. Server-only by construction (StartDeath runs on authority; the party subsystem is server-side).
            FString KillerKey;
            if (const APawn *KillerPawn = Cast<APawn>(Killer)) {
                if (const AMythicPlayerState *KillerPS = KillerPawn->GetPlayerState<AMythicPlayerState>()) {
                    KillerKey = KillerPS->GetCanonicalPlayerKey();
                }
            }
            else if (const APlayerController *KillerPC = Cast<APlayerController>(Killer)) {
                if (const AMythicPlayerState *KillerPS = KillerPC->GetPlayerState<AMythicPlayerState>()) {
                    KillerKey = KillerPS->GetCanonicalPlayerKey();
                }
            }
            if (!KillerKey.IsEmpty()) {
                if (UMythicPartySubsystem *PartySub = GetWorld()->GetSubsystem<UMythicPartySubsystem>()) {
                    PartySub->OnPlayerAction(KillerKey, KillAction.ActionTag, KillAction.MoralVector);
                }
            }
        }
    }

    // Loot drop: roll this owner's assigned loot table(s) and drop the results as world items at its location,
    // crediting the killing player. Reuses the existing loot pipeline (ULootReward::Give -> world-item spawn).
    // Requires a player killer (the loot rarity curves are keyed to the killer's level); NPC/environment kills
    // don't drop loot (deferred).
    if (Owner && LootDrop.LootTables.Num() > 0 && Killer && Killer != Owner) {
        APlayerController *KillerPC = nullptr;
        if (const APawn *KillerPawn = Cast<APawn>(Killer)) {
            KillerPC = Cast<APlayerController>(KillerPawn->GetController());
        }
        else {
            KillerPC = Cast<APlayerController>(Killer);
        }
        if (KillerPC) {
            ULootReward *Reward = NewObject<ULootReward>(this);
            Reward->OverridenLootSource = LootDrop;
            FLootRewardContext Ctx;
            Ctx.PlayerController = KillerPC;
            Ctx.PutInInventory = nullptr; // null => drop into the world (not into an inventory)
            Ctx.SpawnLocation = Owner->GetActorLocation(); // drop on the corpse, not the killer
            Reward->Give(Ctx);
        }
    }

    // Player-controlled owners: apply the death penalty, then the GameMode respawns them after a delay.
    if (const APawn *Pawn = Cast<APawn>(Owner)) {
        if (AController *Controller = Pawn->GetController()) {
            if (Controller->IsPlayerController()) {
                // death penalty (default OFF): lose a fraction of combat proficiency XP progress
                const UMythicDeveloperSettings *DSettings = GetDefault<UMythicDeveloperSettings>();
                const float PenaltyFrac = DSettings ? DSettings->DeathProficiencyPenaltyFraction : 0.0f;
                if (PenaltyFrac > 0.0f) {
                    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(Controller)) {
                        if (UProficiencyComponent *ProfComp = const_cast<UProficiencyComponent*>(MythicPC->GetProficiencyComponent())) {
                            ProfComp->ApplyDeathPenalty(PenaltyFrac);
                        }
                    }
                }
                if (AMythicGameMode *GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMythicGameMode>() : nullptr) {
                    GM->RequestRespawn(Controller, RespawnDelay);
                }
            }
        }
    }
}

void UMythicLifeComponent::RestoreAfterDeath() {
    // Mirror of StartDeath's movement/collision disable. Without this, a pooled-and-reused NPC stays frozen
    // (MOVE_None + NoCollision) and can never move to melee range or be hit again.
    if (ACharacter *Char = Cast<ACharacter>(GetOwner())) {
        if (UCharacterMovementComponent *Move = Char->GetCharacterMovement()) {
            Move->SetMovementMode(MOVE_Walking);
        }
        if (UCapsuleComponent *Capsule = Char->GetCapsuleComponent()) {
            Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }
    }

    // Recompute MaxWalkSpeed from the live CC tags. StartDeath never restores the Slow speed-scale, and a Slow tag
    // that cleared while GAS.State.Dead was set had its recompute swallowed by the dead-guard — so a reused pooled
    // NPC could otherwise stay stuck at the slowed speed. Callers run ResetForRespawn (clears Dead) before this, so
    // the dead-guard no longer trips; the SetMovementMode above stays as an unconditional safety net.
    ReevaluateCrowdControl();
}

void UMythicLifeComponent::HandleCrowdControlTagChanged(const FGameplayTag Tag, int32 NewCount) {
    // The tag count is already updated when NewOrRemoved fires; recompute from the live tag set (handles any
    // combination of Stun/Freeze/Slow + their overlap, rather than pairing per-tag apply/restore).
    ReevaluateCrowdControl();
}

void UMythicLifeComponent::ReevaluateCrowdControl() {
    if (!AbilitySystemComponent) {
        return;
    }
    // Death owns movement (StartDeath set MOVE_None + the Dead tag) — never resurrect movement from here. Respawn
    // clears the ASC tags, so a respawned owner re-evaluates cleanly when a CC tag next changes.
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return;
    }
    ACharacter *Char = Cast<ACharacter>(GetOwner());
    UCharacterMovementComponent *Move = Char ? Char->GetCharacterMovement() : nullptr;
    if (!Move) {
        return; // non-character owners have no walk movement to constrain
    }

    const bool bCannotAct = AbilitySystemComponent->HasMatchingGameplayTag(GAS_DEBUFF_STUNNED)
        || AbilitySystemComponent->HasMatchingGameplayTag(GAS_DEBUFF_FROZEN);

    if (bCannotAct) {
        // Stun / Freeze ("cannot act"): fully halt — mirrors StartDeath's movement disable.
        if (Move->MovementMode != MOVE_None) {
            Move->StopMovementImmediately();
            Move->DisableMovement();
        }
        return;
    }

    // Not hard-CC'd: restore walking if we had disabled it, then apply (or clear) the Slow speed scale against the
    // captured base. Idempotent — safe to run on every CC tag change.
    if (Move->MovementMode == MOVE_None) {
        Move->SetMovementMode(MOVE_Walking);
    }
    // slow and haste compose cleanly as multipliers on the captured base speed
    const bool bSlowed = AbilitySystemComponent->HasMatchingGameplayTag(GAS_DEBUFF_SLOWED);
    const bool bHasted = AbilitySystemComponent->HasMatchingGameplayTag(GAS_BUFF_HASTE);
    float SpeedScale = 1.0f;
    if (bSlowed) {
        SpeedScale *= SlowMultiplier;
    }
    if (bHasted) {
        SpeedScale *= HasteMultiplier;
    }

    // apply sprinting speed scaling factor if active
    const FGameplayTag SprintTag = GAS_STATE_SPRINTING;
    if (SprintTag.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(SprintTag)) {
        if (const UMythicAttributeSet_Utility *Util = AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>()) {
            SpeedScale *= (1.0f + Util->GetBonusSprintSpeed());
        }
    }

    // encumbrance: an over-capacity carry load composes as one more multiplier on the captured base, stacking with slow/haste
    SpeedScale *= ComputeEncumbranceSpeedScale();
    Move->MaxWalkSpeed = BaseWalkSpeed * SpeedScale;
}

TArray<UMythicInventoryComponent *> UMythicLifeComponent::GetOwnerInventoryComponents() const {
    const APawn *OwnerPawn = Cast<APawn>(GetOwner());
    AController *OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
    if (IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(OwnerController)) {
        return Provider->GetAllInventoryComponents(); // players; NPC / inventory-less → empty
    }
    return TArray<UMythicInventoryComponent *>();
}

float UMythicLifeComponent::ComputeEncumbranceSpeedScale() const {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bEncumbranceEnabled) {
        return 1.0f; // feature off → no penalty
    }
    float TotalWeight = 0.0f;
    for (const UMythicInventoryComponent *Inv : GetOwnerInventoryComponents()) {
        if (Inv) {
            TotalWeight += Inv->GetTotalCarriedWeight();
        }
    }
    const EMythicEncumbranceTier Tier =
        MythicEncumbrance::ComputeTier(TotalWeight, Settings->EncumbranceSoftCapacity, Settings->EncumbranceHardCapacity);
    return MythicEncumbrance::SpeedMultiplierForTier(Tier, Settings->EncumbranceHeavySpeedMultiplier, Settings->EncumbranceOverloadedSpeedMultiplier);
}

void UMythicLifeComponent::HandleInventoryChanged(int32 /*Slot*/) {
    // Carried weight just changed (pickup/drop/stack). Recompute move speed — but only when encumbrance is enabled, so
    // the default-off path adds zero work on every inventory mutation. ReevaluateCrowdControl is idempotent.
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (Settings && Settings->bEncumbranceEnabled) {
        ReevaluateCrowdControl();
    }
}

void UMythicLifeComponent::BindEncumbranceInventoryDelegates() {
    UnbindEncumbranceInventoryDelegates(); // idempotent: drop any prior binds (re-init / re-possession safe)
    for (UMythicInventoryComponent *Inv : GetOwnerInventoryComponents()) {
        if (Inv) {
            Inv->OnSlotUpdated.AddDynamic(this, &UMythicLifeComponent::HandleInventoryChanged);
            EncumbranceBoundInventories.Add(Inv);
        }
    }
}

void UMythicLifeComponent::UnbindEncumbranceInventoryDelegates() {
    for (const TWeakObjectPtr<UMythicInventoryComponent> &InvPtr : EncumbranceBoundInventories) {
        if (UMythicInventoryComponent *Inv = InvPtr.Get()) {
            Inv->OnSlotUpdated.RemoveDynamic(this, &UMythicLifeComponent::HandleInventoryChanged);
        }
    }
    EncumbranceBoundInventories.Reset();
}

void UMythicLifeComponent::HandleDamageDelivered(const struct FGameplayEventData *Payload) {
    // LIFESTEAL. Fired on the INSTIGATOR's ASC when this owner lands damage (SendEventToInstigator from the victim's
    // Life set, GAS_EVENT_DMG_DELIVERED — only raised when DamageDone > 0, so every call is a real landed hit). Heal a
    // FLAT LifePerHit per hit (the Defense attr's documented meaning: "life restored when dealing damage"). Server-
    // authoritative + dead-gated + capped at MaxHealth, mirroring ApplyRegen's health heal. The per-KILL sibling is
    // HandleKill below, which consumes GAS_EVENT_KILL emitted by UMythicAttributeSet_Life::PostGameplayEffectExecute.
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return; // a corpse drains no life
    }
    const UMythicAttributeSet_Defense *Def = AbilitySystemComponent->GetSet<UMythicAttributeSet_Defense>();
    if (!LifeSet || !Def) {
        return;
    }
    const float LifePerHit = Def->GetLifePerHit();
    if (LifePerHit <= 0.0f) {
        return; // no lifesteal stat — nothing to do
    }
    const float Cur = LifeSet->GetHealth();
    const float Max = LifeSet->GetMaxHealth();
    if (Cur > 0.0f && Cur < Max) {
        AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),
                                                        FMath::Min(Cur + LifePerHit, Max));
    }
}

void UMythicLifeComponent::HandleKill(const struct FGameplayEventData *Payload) {
    // LIFESTEAL-ON-KILL. Fired on the KILLER's ASC when this owner lands a lethal blow (SendEventToInstigator from the
    // victim's Life set, GAS_EVENT_KILL, once per kill). Heal a FLAT LifePerKill, server-authoritative + dead-gated +
    // capped at MaxHealth (mirrors HandleDamageDelivered). One consumer of the reusable kill event — future
    // XP/bounty/kill-streak systems can bind GAS_EVENT_KILL the same way.
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return; // a corpse drains no life
    }
    const UMythicAttributeSet_Defense *Def = AbilitySystemComponent->GetSet<UMythicAttributeSet_Defense>();
    if (!LifeSet || !Def) {
        return;
    }
    const float LifePerKill = Def->GetLifePerKill();
    if (LifePerKill <= 0.0f) {
        return; // no kill-lifesteal stat — nothing to do
    }
    const float Cur = LifeSet->GetHealth();
    const float Max = LifeSet->GetMaxHealth();
    if (Cur > 0.0f && Cur < Max) {
        AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),
                                                        FMath::Min(Cur + LifePerKill, Max));
    }
}

void UMythicLifeComponent::ApplyRegen() {
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return; // corpses don't regenerate
    }

    const UMythicAttributeSet_Defense *Def = AbilitySystemComponent->GetSet<UMythicAttributeSet_Defense>();

    // Health (rate lives on the Defense set; value on the Life set). Only regen while ALIVE (Cur > 0 — never
    // self-revive) and below max.
    if (LifeSet && Def) {
        const float Cur = LifeSet->GetHealth();
        const float NewV = ComputeRegenTarget(Cur, LifeSet->GetMaxHealth(), Def->GetHealthRegenRate(), RegenInterval);
        if (Cur > 0.0f && NewV > Cur) {
            AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), NewV);
        }
    }

    // Shield (recharges from 0 — no alive gate).
    if (Def) {
        const float Cur = Def->GetShield();
        const float NewV = ComputeRegenTarget(Cur, Def->GetMaxShield(), Def->GetShieldRegenRate(), RegenInterval);
        if (NewV > Cur) {
            AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetShieldAttribute(), NewV);
        }
    }

    // Stamina (recharges from 0 — no alive gate).
    if (const UMythicAttributeSet_Utility *Util = AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>()) {
        const float Cur = Util->GetCurrentStamina();
        const float NewV = ComputeRegenTarget(Cur, Util->GetMaxStamina(), Util->GetStaminaRegenRate(), RegenInterval);
        if (NewV > Cur) {
            AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(), NewV);
        }
    }
}

float UMythicLifeComponent::ComputeRegenTarget(float Cur, float Max, float Rate, float DeltaSeconds) {
    // Nothing to do: no (positive) rate, or already at/above max. Return Cur so callers see `result > Cur` == false.
    if (Rate <= 0.0f || Cur >= Max) {
        return Cur;
    }
    return FMath::Min(Cur + Rate * DeltaSeconds, Max); // clamp so a large rate/interval never overshoots Max
}

bool UMythicLifeComponent::CanSpendStamina(float Cost) const {
    if (Cost <= 0.0f) {
        return true; // a zero/negative cost is trivially affordable
    }
    const UMythicAttributeSet_Utility *Util = AbilitySystemComponent ? AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>() : nullptr;
    if (!Util) {
        return false;
    }
    return Util->GetCurrentStamina() >= EffectiveStaminaCost(Cost, Util->GetStaminaCostReduction());
}

float UMythicLifeComponent::EffectiveStaminaCost(float RawCost, float StaminaCostReduction) {
    // Clamp the reduction so an out-of-range attribute can't make the spend negative (Reduction>1) or amplified (<0).
    return RawCost * (1.0f - FMath::Clamp(StaminaCostReduction, 0.0f, 1.0f));
}

bool UMythicLifeComponent::IsHeavyHit(float EventMagnitude, float MaxHealth, float HeavyHitHealthFraction) {
    // Scales with the entity: a hit staggers only at >= a fraction of MaxHealth. Non-positive MaxHealth never staggers.
    return MaxHealth > 0.0f && EventMagnitude >= HeavyHitHealthFraction * MaxHealth;
}

bool UMythicLifeComponent::IsStaggerImmune(double Now, double LastStaggerTime, float ImmunityWindow) {
    // "Never staggered yet" (sentinel <= 0) is NOT immune — the first heavy hit must stagger even if it lands before
    // ImmunityWindow seconds of world time have elapsed (otherwise Now - 0 < Window wrongly suppresses it).
    if (LastStaggerTime <= 0.0) {
        return false;
    }
    return (Now - LastStaggerTime) < ImmunityWindow;
}

bool UMythicLifeComponent::TrySpendStamina(float Cost) {
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative() || Cost <= 0.0f) {
        return Cost <= 0.0f; // a zero/negative cost trivially "succeeds" without spending
    }
    const UMythicAttributeSet_Utility *Util = AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>();
    if (!Util) {
        return false;
    }
    const float Effective = EffectiveStaminaCost(Cost, Util->GetStaminaCostReduction());
    const float Cur = Util->GetCurrentStamina();
    if (Cur < Effective) {
        return false; // not enough stamina
    }
    AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(), Cur - Effective);
    return true;
}

void UMythicLifeComponent::ClearGameplayTags() const {
    // Replicated tag counts are authority-only.
    if (AbilitySystemComponent && AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        AbilitySystemComponent->SetLooseGameplayTagCount(GAS_STATE_DYING, 0);
        AbilitySystemComponent->SetLooseGameplayTagCount(GAS_STATE_DEAD, 0);
    }
}

float UMythicLifeComponent::GetHealth() const {
    return (LifeSet ? LifeSet->GetHealth() : 0.0f);
}

float UMythicLifeComponent::GetMaxHealth() const {
    return (LifeSet ? LifeSet->GetMaxHealth() : 0.0f);
}

float UMythicLifeComponent::GetHealthNormalized() const {
    if (LifeSet) {
        const float Health = LifeSet->GetHealth();
        const float MaxHealth = LifeSet->GetMaxHealth();

        return ((MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f);
    }

    return 0.0f;
}

// Trigger a gameplay event on the source ASC when the health value goes down.
// Target: is the owner of the health component.
// ContextHandle: is the context of the damage effect.
// EventMagnitude: is the amount of damage dealt.
void UMythicLifeComponent::TriggerGameplayEvent_DeliveredHit(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                             float OldValue, float NewValue) const {
    if (!OnDeliveredHitGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_DeliveredHit: OnDeliveredHitGameplayEventTag is not set."));
        return;
    }

    if (auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent()) {
        if (!AbilitySystemComponent) {
            UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_DeliveredHit: AbilitySystemComponent is NULL."));
            return;
        }

        UDamageResult *DamageResult = NewObject<UDamageResult>();
        DamageResult->OldHealth = OldValue;
        DamageResult->NewHealth = NewValue;

        // Send a gameplay event to the source ASC.
        FGameplayEventData Payload;
        Payload.EventTag = OnDeliveredHitGameplayEventTag;
        Payload.Instigator = DamageInstigator;
        Payload.Target = AbilitySystemComponent->GetAvatarActor();
        Payload.OptionalObject = DamageEffectSpec->Def;
        Payload.OptionalObject2 = DamageResult;
        Payload.ContextHandle = DamageEffectSpec->GetContext();
        Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
        Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
        Payload.EventMagnitude = DamageMagnitude;

        FScopedPredictionWindow NewScopedWindow(SourceASC, true);

        // Source ASC
        SourceASC->HandleGameplayEvent(OnDeliveredHitGameplayEventTag, &Payload);
    }
    else {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_DeliveredHit: Source ASC is NULL."));
    }
}

void UMythicLifeComponent::TriggerGameplayEvent_DeliveredHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                              float OldValue, float NewValue) {
    if (!OnDeliveredHealGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_DeliveredHeal: OnDeliveredHealGameplayEventTag is not set."));
        return;
    }

    if (auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent()) {
        if (!AbilitySystemComponent) {
            UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_DeliveredHeal: AbilitySystemComponent is NULL."));
            return;
        }

        UDamageResult *DamageResult = NewObject<UDamageResult>();
        DamageResult->OldHealth = OldValue;
        DamageResult->NewHealth = NewValue;

        FGameplayEventData Payload;
        Payload.EventTag = OnDeliveredHealGameplayEventTag;
        Payload.Instigator = DamageInstigator;
        Payload.Target = AbilitySystemComponent->GetAvatarActor();
        Payload.OptionalObject = DamageEffectSpec->Def;
        Payload.OptionalObject2 = DamageResult;
        Payload.ContextHandle = DamageEffectSpec->GetContext();
        Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
        Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
        Payload.EventMagnitude = DamageMagnitude;

        FScopedPredictionWindow NewScopedWindow(SourceASC, true);

        // Source ASC
        SourceASC->HandleGameplayEvent(OnDeliveredHealGameplayEventTag, &Payload);
    }
    else {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_DeliveredHeal: Source ASC is NULL."));
    }
}

void UMythicLifeComponent::TriggerGameplayEvent_ReceivedHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                             float OldValue, float NewValue) {
    if (!OnHealReceivedGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_ReceivedHeal: OnHealReceivedGameplayEventTag is not set."));
        return;
    }

    if (!AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_ReceivedHeal: AbilitySystemComponent is NULL."));
        return;
    }

    auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent();
    if (!SourceASC) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_ReceivedHeal: Source ASC is NULL."));
        return;
    }

    // Create the damage result uobject.
    UDamageResult *DamageResult = NewObject<UDamageResult>();
    DamageResult->OldHealth = OldValue;
    DamageResult->NewHealth = NewValue;

    FGameplayEventData Payload;
    Payload.EventTag = OnHealReceivedGameplayEventTag;
    Payload.Instigator = DamageInstigator;
    Payload.Target = SourceASC->GetAvatarActor();
    Payload.OptionalObject = DamageEffectSpec->Def;
    Payload.OptionalObject2 = DamageResult;
    Payload.ContextHandle = DamageEffectSpec->GetContext();
    Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
    Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
    Payload.EventMagnitude = DamageMagnitude;

    FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);

    AbilitySystemComponent->HandleGameplayEvent(OnHealReceivedGameplayEventTag, &Payload);
}

void UMythicLifeComponent::TriggerGameplayEvent_Death(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                      float OldValue, float NewValue) {
    if (!OnDeathGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_Death: OnDeathGameplayEventTag is not set."));
        return;
    }
    if (!AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_Death: AbilitySystemComponent is NULL."));
        return;
    }

    auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent();
    if (!SourceASC) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_Death: Source ASC is NULL."));
        return;
    }

    UDamageResult *DamageResult = NewObject<UDamageResult>();
    DamageResult->OldHealth = OldValue;
    DamageResult->NewHealth = NewValue;

    FGameplayEventData Payload;
    Payload.EventTag = OnDeathGameplayEventTag;
    Payload.Instigator = DamageInstigator;
    Payload.Target = SourceASC->GetAvatarActor();
    Payload.OptionalObject = DamageEffectSpec->Def;
    Payload.OptionalObject2 = DamageResult;
    Payload.ContextHandle = DamageEffectSpec->GetContext();
    Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
    Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
    Payload.EventMagnitude = DamageMagnitude;

    FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);

    AbilitySystemComponent->HandleGameplayEvent(OnDeathGameplayEventTag, &Payload);
}

void UMythicLifeComponent::TriggerGameplayEvent_Kill(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                     float OldValue, float NewValue) {
    // Guard the tag this function actually FIRES (OnKillGameplayEventTag) — not OnDeathGameplayEventTag. The wrong tag
    // here meant: if the kill tag was unset we'd still HandleGameplayEvent() an INVALID tag, and if only the death tag
    // was unset we'd wrongly skip a configured kill event. The log already named the intended tag.
    if (!OnKillGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_Kill: OnKillGameplayEventTag is not set."));
        return;
    }
    if (!AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_Kill: AbilitySystemComponent is NULL."));
        return;
    }

    auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent();
    if (!SourceASC) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_Kill: Source ASC is NULL."));
        return;
    }

    UDamageResult *DamageResult = NewObject<UDamageResult>();
    DamageResult->OldHealth = OldValue;
    DamageResult->NewHealth = NewValue;

    FGameplayEventData Payload;
    Payload.EventTag = OnKillGameplayEventTag;
    Payload.Instigator = DamageInstigator;
    Payload.Target = AbilitySystemComponent->GetAvatarActor();
    Payload.OptionalObject = DamageEffectSpec->Def;
    Payload.OptionalObject2 = DamageResult;
    Payload.ContextHandle = DamageEffectSpec->GetContext();
    Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
    Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
    Payload.EventMagnitude = DamageMagnitude;

    FScopedPredictionWindow NewScopedWindow(SourceASC, true);

    SourceASC->HandleGameplayEvent(OnKillGameplayEventTag, &Payload);
}

void UMythicLifeComponent::HandleHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec,
                                               float DamageMagnitude, float OldValue, float NewValue) {
    // Trigger Heal Received and Heal Delivered events if health is going up and the old value is greater than 0.
    if (NewValue > OldValue && OldValue > 0.0f) {
        TriggerGameplayEvent_DeliveredHeal(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
        TriggerGameplayEvent_ReceivedHeal(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
    }
    // Trigger Death event if health is going down and the new value is less than or equal to 0.
    else if (NewValue <= 0.0f) {
        TriggerGameplayEvent_Death(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
        TriggerGameplayEvent_Kill(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
    }
    // Trigger Hit Delivered event if health is going down and the old value is greater than 0.
    else if (NewValue < OldValue && OldValue > 0.0f) {
        TriggerGameplayEvent_DeliveredHit(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
    }

    auto ContextHandle = DamageEffectSpec->GetContext();

}

void UMythicLifeComponent::HandleMaxHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec,
                                                  float DamageMagnitude, float OldValue, float NewValue) {}
