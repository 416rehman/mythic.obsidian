#include "GAS/Abilities/MythicGA_Rune.h"

#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

#include "GAS/Abilities/MythicAbilityCost_Stamina.h"
#include "GAS/Abilities/MythicGA_Skill.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Mythic.h"
#include "Player/MythicCharacter.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "Progression/Runes/MythicTags_Rune.h"
#include "Rewards/LootScaling.h"
#include "Settings/MythicCombatSettings.h"

namespace {
const FName RuneRowElementName(TEXT("RuneRow"));
}

UMythicGA_Rune::UMythicGA_Rune(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer) {
    ActivationPolicy = EMythicAbilityActivationPolicy::OnSpawn;
    // A rune rewrites a rule of the game, so it resolves on the server only and is never predicted.
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UMythicGA_Rune::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) {
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UAbilitySystemComponent *ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    if (!ASC || !ActorInfo->IsNetAuthority()) {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Being downed and dying both CancelAllAbilities. A rune that ended there would switch off until respawn,
    // and a cheat-death rune would never hear the blow it exists for.
    SetCanBeCanceled(false);

    Definition = Cast<UMythicRuneDefinition>(GetCurrentSourceObject());
    RuneComponent = ResolveRuneComponent();

    const FGameplayTag Listened[] = {GAS_EVENT_DEATH_PRE, GAS_EVENT_DMG_PRE, GAS_EVENT_DMG_DELIVERED, GAS_EVENT_DMG_RECEIVED,
                                     GAS_EVENT_KILL, GAS_EVENT_MOVED, GAS_EVENT_ITEM_ACQUIRED, GAS_EVENT_ITEM_USED,
                                     GAS_EVENT_CURRENCY_SPENT, GAS_EVENT_HARVEST_STRUCK};
    for (const FGameplayTag &EventTag : Listened) {
        const FDelegateHandle Bound = ASC->GenericGameplayEventCallbacks.FindOrAdd(EventTag).AddUObject(
            this, &UMythicGA_Rune::HandleRuneGameplayEvent, EventTag);
        BoundEvents.Add(EventTag, Bound);
    }
    MovingTagHandle = ASC->RegisterGameplayTagEvent(GAS_STATE_MOVING, EGameplayTagEventType::NewOrRemoved)
                          .AddUObject(this, &UMythicGA_Rune::HandleMovingTagChanged);
    AbilityActivatedHandle = ASC->AbilityActivatedCallbacks.AddUObject(this, &UMythicGA_Rune::HandleAbilityActivated);
    AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(this, &UMythicGA_Rune::HandleAbilityEnded);
    BindAvatar(ActorInfo->AvatarActor.Get());
    BindLootManager();
    bRuneBound = true;

    OnRuneActivated();
}

void UMythicGA_Rune::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) {
    if (bRuneBound) {
        bRuneBound = false;
        OnRuneRemoved();

        // A guard the Blueprint did not close hands the Shield back quietly; the rune has already had its last word.
        if (bGuardActive) {
            FinishRuneGuard(false, false);
        }

        UnbindAll(ActorInfo);
        if (UWorld *World = GetWorld()) {
            World->GetTimerManager().ClearTimer(CooldownTimer);
        }
        CooldownEndTimeSeconds = 0.0;
        CueLastPlayedSeconds.Empty();

        // Tags the rune still holds would outlive its socket; the pipeline that reads them cannot know it left.
        if (UAbilitySystemComponent *ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr) {
            for (const FGameplayTag &Held : HeldStateTags) {
                UMythicLifeComponent::SetReplicatedStateTag(ASC, Held, false);
            }
        }
        HeldStateTags.Reset();

        SetRuneHudState(EMythicRuneHudState::Hidden);
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UMythicGA_Rune::OnAvatarSet(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) {
    Super::OnAvatarSet(ActorInfo, Spec);

    // The instance lives on the player state and outlives the pawn; each new pawn needs its seams bound again.
    if (bRuneBound) {
        BindAvatar(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    }
}

// ---- Native seams ----

void UMythicGA_Rune::NotifyRunePreDeath(const FGameplayEventData &Payload) {
    OnRunePreDeath(Payload);
}

void UMythicGA_Rune::NotifyRuneKill(const FGameplayEventData &Payload) {
    OnRuneKill(Payload);
}

void UMythicGA_Rune::NotifyRuneLanded(float ImpactSpeed, float FallDamage, bool bPrevented) {
    OnRuneLanded(ImpactSpeed, FallDamage, bPrevented);
}

void UMythicGA_Rune::NotifyRuneFallBegan() {
    OnRuneFallBegan();
}

void UMythicGA_Rune::NotifyRuneFallDepth(float Metres) {
    OnRuneFallDepth(Metres);
}

void UMythicGA_Rune::NotifyRuneStillBegan() {
    OnRuneStillBegan();
}

void UMythicGA_Rune::NotifyRuneSkillDashStarted(UMythicGA_Skill *Skill, const FVector &StartLocation) {
    OnRuneSkillDashStarted(Skill, StartLocation);
}

void UMythicGA_Rune::NotifyRuneSkillDashEnded(UMythicGA_Skill *Skill, const FVector &EndLocation) {
    OnRuneSkillDashEnded(Skill, EndLocation);
}

void UMythicGA_Rune::NotifyRuneGuardEnded() {
    OnRuneGuardEnded();
}

void UMythicGA_Rune::NotifyRunePreLootRoll(APlayerController *CreditedTo) {
    OnRunePreLootRoll(CreditedTo);
}

// ---- Handlers ----

void UMythicGA_Rune::HandleRuneGameplayEvent(const FGameplayEventData *Payload, FGameplayTag EventTag) {
    if (!IsRuneAuthority()) {
        return;
    }
    const FGameplayEventData Empty;
    const FGameplayEventData &Data = Payload ? *Payload : Empty;

    if (EventTag == GAS_EVENT_DEATH_PRE.GetTag()) {
        TGuardValue<bool> PreDeathScope(bInPreDeath, true);
        NotifyRunePreDeath(Data);
    }
    else if (EventTag == GAS_EVENT_DMG_PRE.GetTag()) {
        OnRuneOutgoingHit(Data);
    }
    else if (EventTag == GAS_EVENT_MOVED.GetTag()) {
        OnRuneDistance(Data.EventMagnitude);
    }
    else if (EventTag == GAS_EVENT_KILL.GetTag()) {
        NotifyRuneKill(Data);
    }
    else if (EventTag == GAS_EVENT_HARVEST_STRUCK.GetTag() && Data.InstigatorTags.HasTag(HARVEST_FELLED)) {
        const UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
        if (ASC && ASC->HasMatchingGameplayTag(RUNE_RULE_FELLEDISKILL)) {
            NotifyRuneKill(Data);
        }
    }
    OnRuneEvent(EventTag, Data);
}

void UMythicGA_Rune::HandleMovingTagChanged(const FGameplayTag Tag, int32 NewCount) {
    if (!IsRuneAuthority()) {
        return;
    }
    if (NewCount > 0) {
        OnRuneStartedMoving();
    }
    else {
        OnRuneStoppedMoving();
    }
}

void UMythicGA_Rune::HandleCooldownEnded() {
    CooldownEndTimeSeconds = 0.0;
    SetRuneHudState(EMythicRuneHudState::Hidden);
    OnRuneCooldownEnded();
}

void UMythicGA_Rune::HandleFallDamageResolved(float ImpactSpeed, float Damage, bool bPrevented) {
    if (IsRuneAuthority()) {
        NotifyRuneLanded(ImpactSpeed, Damage, bPrevented);
    }
}

void UMythicGA_Rune::HandleStillBegan() {
    if (IsRuneAuthority()) {
        NotifyRuneStillBegan();
    }
}

void UMythicGA_Rune::HandleFallDepthSampled(float Metres) {
    if (IsRuneAuthority()) {
        NotifyRuneFallDepth(Metres);
    }
}

void UMythicGA_Rune::HandleMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) {
    if (!IsRuneAuthority() || PrevMovementMode == MOVE_Falling) {
        return;
    }
    const AMythicCharacter *Character = BoundCharacter.Get();
    const UCharacterMovementComponent *Move = Character ? Character->GetCharacterMovement() : nullptr;
    if (Move && Move->MovementMode == MOVE_Falling) {
        NotifyRuneFallBegan();
    }
}

void UMythicGA_Rune::HandleAbilityActivated(UGameplayAbility *Ability) {
    UMythicGA_Skill *Skill = Cast<UMythicGA_Skill>(Ability);
    if (!Skill || Skill->Movement == EMythicSkillMovement::None || !IsRuneAuthority()) {
        return;
    }
    const AActor *Avatar = GetAvatarActorFromActorInfo();
    NotifyRuneSkillDashStarted(Skill, Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector);
}

void UMythicGA_Rune::HandleAbilityEnded(const FAbilityEndedData &Data) {
    UMythicGA_Skill *Skill = Cast<UMythicGA_Skill>(Data.AbilityThatEnded);
    if (!Skill || Skill->Movement == EMythicSkillMovement::None || !IsRuneAuthority()) {
        return;
    }
    const AActor *Avatar = GetAvatarActorFromActorInfo();
    NotifyRuneSkillDashEnded(Skill, Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector);
}

void UMythicGA_Rune::HandleGuardShieldRemoved(const FGameplayEffectRemovalInfo &RemovalInfo) {
    if (!bEndingGuard && bGuardActive) {
        FinishRuneGuard(true, true);
    }
}

void UMythicGA_Rune::HandlePreLootRoll(APlayerController *CreditedTo, FLootTierBonus &Bonus) {
    if (!IsRuneAuthority()) {
        return;
    }
    // Scoped both ways: whatever the graph does with the call, nothing may reach the roll's stack frame after it.
    TGuardValue<FLootTierBonus *> BonusScope(PendingLootBonus, &Bonus);
    TGuardValue<APlayerController *> CreditScope(PendingLootController, CreditedTo);
    NotifyRunePreLootRoll(CreditedTo);
}

// ---- Binding ----

void UMythicGA_Rune::BindAvatar(AActor *Avatar) {
    AMythicCharacter *Character = Cast<AMythicCharacter>(Avatar);
    UMythicLifeComponent *Life = UMythicLifeComponent::FindHealthComponent(Avatar);
    if (Character == BoundCharacter.Get() && Life == BoundLife.Get()) {
        return;
    }
    UnbindAvatar();
    if (Character) {
        FallResolvedHandle = Character->OnFallDamageResolved.AddUObject(this, &UMythicGA_Rune::HandleFallDamageResolved);
        Character->OnMythicMovementModeChange.AddUniqueDynamic(this, &UMythicGA_Rune::HandleMovementModeChanged);
    }
    if (Life) {
        StillBeganHandle = Life->OnStillBegan.AddUObject(this, &UMythicGA_Rune::HandleStillBegan);
        FallDepthHandle = Life->OnFallDepthSampled.AddUObject(this, &UMythicGA_Rune::HandleFallDepthSampled);
    }
    BoundCharacter = Character;
    BoundLife = Life;
}

void UMythicGA_Rune::UnbindAvatar() {
    if (AMythicCharacter *Character = BoundCharacter.Get()) {
        Character->OnFallDamageResolved.Remove(FallResolvedHandle);
        Character->OnMythicMovementModeChange.RemoveDynamic(this, &UMythicGA_Rune::HandleMovementModeChanged);
    }
    if (UMythicLifeComponent *Life = BoundLife.Get()) {
        Life->OnStillBegan.Remove(StillBeganHandle);
        Life->OnFallDepthSampled.Remove(FallDepthHandle);
    }
    FallResolvedHandle.Reset();
    StillBeganHandle.Reset();
    FallDepthHandle.Reset();
    BoundCharacter.Reset();
    BoundLife.Reset();
}

void UMythicGA_Rune::BindLootManager() {
    UMythicLootManagerSubsystem *LootManager = ResolveLootManager();
    if (!LootManager) {
        return;
    }
    BoundLootManager = LootManager;
    PreLootRollHandle = LootManager->OnPreLootRoll.AddUObject(this, &UMythicGA_Rune::HandlePreLootRoll);
}

void UMythicGA_Rune::UnbindLootManager() {
    if (UMythicLootManagerSubsystem *LootManager = BoundLootManager.Get()) {
        LootManager->OnPreLootRoll.Remove(PreLootRollHandle);
    }
    BoundLootManager.Reset();
    PreLootRollHandle.Reset();
}

void UMythicGA_Rune::UnbindAll(const FGameplayAbilityActorInfo *ActorInfo) {
    if (UAbilitySystemComponent *ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr) {
        for (const TPair<FGameplayTag, FDelegateHandle> &Bound : BoundEvents) {
            if (FGameplayEventMulticastDelegate *Delegate = ASC->GenericGameplayEventCallbacks.Find(Bound.Key)) {
                Delegate->Remove(Bound.Value);
            }
        }
        if (MovingTagHandle.IsValid()) {
            ASC->UnregisterGameplayTagEvent(MovingTagHandle, GAS_STATE_MOVING, EGameplayTagEventType::NewOrRemoved);
        }
        ASC->AbilityActivatedCallbacks.Remove(AbilityActivatedHandle);
        ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
    }
    BoundEvents.Empty();
    MovingTagHandle.Reset();
    AbilityActivatedHandle.Reset();
    AbilityEndedHandle.Reset();
    UnbindAvatar();
    UnbindLootManager();
}

// ---- Resolution ----

bool UMythicGA_Rune::IsRuneAuthority() const {
    const FGameplayAbilityActorInfo *Info = GetCurrentActorInfo();
    return Info && Info->IsNetAuthority();
}

UMythicRuneComponent *UMythicGA_Rune::ResolveRuneComponent() const {
    if (const AActor *Owner = GetOwningActorFromActorInfo()) {
        if (UMythicRuneComponent *Runes = Owner->FindComponentByClass<UMythicRuneComponent>()) {
            return Runes;
        }
    }
    const AActor *Avatar = GetAvatarActorFromActorInfo();
    return Avatar ? Avatar->FindComponentByClass<UMythicRuneComponent>() : nullptr;
}

UMythicLifeComponent *UMythicGA_Rune::ResolveLifeComponent() const {
    return UMythicLifeComponent::FindHealthComponent(GetAvatarActorFromActorInfo());
}

AMythicPlayerController *UMythicGA_Rune::ResolvePlayerController() const {
    if (AMythicPlayerController *PC = GetMythicPlayerControllerFromActorInfo()) {
        return PC;
    }
    const APawn *Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
    return Pawn ? Cast<AMythicPlayerController>(Pawn->GetController()) : nullptr;
}

UMythicLootManagerSubsystem *UMythicGA_Rune::ResolveLootManager() const {
    const UWorld *World = GetWorld();
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
}

UMythicStatLedgerComponent *UMythicGA_Rune::ResolveStatLedger() const {
    if (const AActor *Owner = GetOwningActorFromActorInfo()) {
        if (UMythicStatLedgerComponent *Ledger = Owner->FindComponentByClass<UMythicStatLedgerComponent>()) {
            return Ledger;
        }
    }
    const AMythicPlayerController *PC = ResolvePlayerController();
    const AMythicPlayerState *PlayerState = PC ? PC->GetPlayerState<AMythicPlayerState>() : nullptr;
    return PlayerState ? PlayerState->GetStatLedgerComponent() : nullptr;
}

// ---- Verbs ----

bool UMythicGA_Rune::PreventDeath(float RestoreHealthFraction) {
    if (!bInPreDeath) {
        UE_LOG(Myth, Warning, TEXT("Runes: %s called PreventDeath outside OnRunePreDeath; nothing was prevented"), *GetName());
        return false;
    }
    UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC || !IsRuneAuthority()) {
        return false;
    }
    // The pipeline removes one count of the tag when it honours it. A second count would silently save the next
    // death too, so a death another rune already handled is left to that rune.
    if (ASC->HasMatchingGameplayTag(GAS_PIPELINE_DEATH_HANDLED)) {
        return false;
    }
    ASC->AddLooseGameplayTag(GAS_PIPELINE_DEATH_HANDLED);

    if (UMythicLifeComponent *Life = ResolveLifeComponent()) {
        Life->ServerSetHealthFraction(RestoreHealthFraction);
    }
    else {
        UE_LOG(Myth, Warning, TEXT("Runes: %s prevented death but %s has no life component; the owner is left at 1 health"),
               *GetName(), *GetNameSafe(GetAvatarActorFromActorInfo()));
    }
    return true;
}

void UMythicGA_Rune::SetOutgoingHitMultiplier(FGameplayEffectContextHandle Context, float Multiplier, FGameplayTag HitTag) {
    FMythicGameplayEffectContext *MythicContext = FMythicGameplayEffectContext::ExtractEffectContext(Context);
    if (!MythicContext) {
        UE_LOG(Myth, Warning, TEXT("Runes: %s cannot empower a hit with no Mythic effect context"), *GetName());
        return;
    }
    MythicContext->SetBonusDamageMultiplier(MythicContext->GetBonusDamageMultiplier() * FMath::Max(0.0f, Multiplier));
    if (HitTag.IsValid()) {
        MythicContext->AddHitTag(HitTag);
    }
}

bool UMythicGA_Rune::IsOwnHit(FGameplayEffectContextHandle Context) const {
    return Definition && Context.IsValid() && Context.GetSourceObject() == Definition.Get();
}

float UMythicGA_Rune::ReadRolled(FGameplayTag Parameter, float Fallback) const {
    if (!Definition || !Parameter.IsValid()) {
        return Fallback;
    }
    float Rolled = 0.0f;
    const UMythicRuneComponent *Runes = RuneComponent.Get();
    if (Runes && Runes->GetRolledRuneValue(Definition, Parameter, Rolled)) {
        return Rolled;
    }
    return Definition->GetParameterMidpoint(Parameter, Fallback);
}

void UMythicGA_Rune::SetRuneHudState(EMythicRuneHudState State, float DurationSeconds, int32 Stacks) {
    UMythicRuneComponent *Runes = RuneComponent.Get();
    if (!Runes || !Definition || !IsRuneAuthority()) {
        return;
    }
    const EMythicRuneHudState Previous = Runes->GetRuneHudState(Runes->FindSlotOfRune(Definition));
    Runes->SetRuneHudStateForRune(Definition, State, DurationSeconds, Stacks);

    // A rune becoming ready is the one HUD moment worth pulling the eye to; every other change is a quiet redraw.
    if (Previous == EMythicRuneHudState::Hidden && State == EMythicRuneHudState::Ready) {
        if (AMythicPlayerController *PC = ResolvePlayerController()) {
            PC->ClientPokeHudElement(RuneRowElementName);
        }
    }
}

FActiveGameplayEffectHandle UMythicGA_Rune::ApplyRuneEffect(const TSoftClassPtr<UGameplayEffect> &EffectClass, float Magnitude,
                                                            float DurationSeconds, FGameplayTag HitTag) {
    UClass *Loaded = EffectClass.LoadSynchronous();
    if (!Loaded) {
        UE_LOG(Myth, Warning, TEXT("Runes: %s needs a rune effect that Mythic Combat settings do not name (%s)"), *GetName(),
               *EffectClass.ToString());
        return FActiveGameplayEffectHandle();
    }
    const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Loaded, GetAbilityLevel());
    if (!Spec.IsValid()) {
        return FActiveGameplayEffectHandle();
    }
    Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_GENERIC, Magnitude);
    if (DurationSeconds > 0.0f) {
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_DURATION, DurationSeconds);
    }
    if (HitTag.IsValid()) {
        if (FMythicGameplayEffectContext *Context = FMythicGameplayEffectContext::ExtractEffectContext(Spec.Data->GetContext())) {
            Context->AddHitTag(HitTag);
        }
    }
    return ApplyGameplayEffectSpecToOwner(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), Spec);
}

void UMythicGA_Rune::BeginRuneGuard(float ShieldAmount, float DurationSeconds) {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC || ShieldAmount <= 0.0f || DurationSeconds <= 0.0f || !IsRuneAuthority()) {
        return;
    }
    if (!ASC->HasAttributeSetForAttribute(UMythicAttributeSet_Defense::GetShieldAttribute())) {
        UE_LOG(Myth, Warning, TEXT("Runes: %s cannot guard %s, which carries no Shield"), *GetName(), *GetNameSafe(ASC->GetOwner()));
        return;
    }

    if (bGuardActive) {
        // The Shield to hand back is still the one from before the first guard, not what that guard left.
        TGuardValue<bool> Ending(bEndingGuard, true);
        ASC->RemoveActiveGameplayEffect(GuardShieldHandle);
        ASC->RemoveActiveGameplayEffect(GuardMaxShieldHandle);
        bGuardActive = false;
    }
    else {
        PreGuardShield = ASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute());
    }

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    // Ceiling first: the Shield add is clamped against whatever MaxShield stands when it lands.
    GuardMaxShieldHandle = ApplyRuneEffect(Settings->RuneGuardMaxShieldEffect, ShieldAmount, DurationSeconds, FGameplayTag());
    GuardShieldHandle = ApplyRuneEffect(Settings->RuneGuardShieldEffect, ShieldAmount, DurationSeconds, FGameplayTag());
    if (!GuardShieldHandle.IsValid()) {
        ASC->RemoveActiveGameplayEffect(GuardMaxShieldHandle);
        GuardMaxShieldHandle.Invalidate();
        return;
    }
    bGuardActive = true;
    if (FOnActiveGameplayEffectRemoved_Info *Removed = ASC->OnGameplayEffectRemoved_InfoDelegate(GuardShieldHandle)) {
        Removed->AddUObject(this, &UMythicGA_Rune::HandleGuardShieldRemoved);
    }
}

void UMythicGA_Rune::EndRuneGuard() {
    if (bGuardActive && IsRuneAuthority()) {
        FinishRuneGuard(false, true);
    }
}

void UMythicGA_Rune::FinishRuneGuard(bool bShieldEffectAlreadyRemoved, bool bNotify) {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    bGuardActive = false;
    {
        TGuardValue<bool> Ending(bEndingGuard, true);
        if (ASC) {
            // The add goes before its ceiling, or the ceiling's fall clamps the add away first and the bookkeeping lies.
            if (!bShieldEffectAlreadyRemoved) {
                ASC->RemoveActiveGameplayEffect(GuardShieldHandle);
            }
            ASC->RemoveActiveGameplayEffect(GuardMaxShieldHandle);
        }
    }
    GuardShieldHandle.Invalidate();
    GuardMaxShieldHandle.Invalidate();

    if (ASC && ASC->HasAttributeSetForAttribute(UMythicAttributeSet_Defense::GetShieldAttribute())) {
        const float MaxShield = ASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetMaxShieldAttribute());
        ASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetShieldAttribute(), FMath::Clamp(PreGuardShield, 0.0f, MaxShield));
    }
    PreGuardShield = 0.0f;

    if (bNotify) {
        NotifyRuneGuardEnded();
    }
}

void UMythicGA_Rune::DealRuneSelfDamage(float Amount, FGameplayTag HitTag) {
    if (Amount <= 0.0f || !IsRuneAuthority()) {
        return;
    }
    ApplyRuneEffect(GetDefault<UMythicCombatSettings>()->RuneSelfDamageEffect, Amount, 0.0f, HitTag);
}

void UMythicGA_Rune::HealRune(float Amount) {
    if (Amount <= 0.0f || !IsRuneAuthority()) {
        return;
    }
    ApplyRuneEffect(GetDefault<UMythicCombatSettings>()->RuneHealEffect, Amount, 0.0f, FGameplayTag());
}

bool UMythicGA_Rune::TryChargeCurrency(int32 Amount) {
    AMythicPlayerController *PC = ResolvePlayerController();
    return PC && IsRuneAuthority() && PC->TryChargeCurrency(Amount);
}

int32 UMythicGA_Rune::GetCarriedGold() const {
    const AMythicPlayerController *PC = ResolvePlayerController();
    return PC ? PC->GetCarriedCurrency() : 0;
}

int32 UMythicGA_Rune::GrantGold(int32 Amount) {
    AMythicPlayerController *PC = ResolvePlayerController();
    if (!PC || Amount <= 0 || !IsRuneAuthority()) {
        return 0;
    }
    return PC->GrantCurrency(Amount);
}

FLootTierBonus *UMythicGA_Rune::ResolvePendingLootBonus(const TCHAR *Verb) {
    if (!PendingLootBonus) {
        UE_LOG(Myth, Warning, TEXT("Runes: %s called %s outside OnRunePreLootRoll; the loot roll was left alone"), *GetName(), Verb);
    }
    return PendingLootBonus;
}

void UMythicGA_Rune::AddRuneLootDrops(int32 Extra) {
    if (FLootTierBonus *Bonus = ResolvePendingLootBonus(TEXT("AddRuneLootDrops"))) {
        Bonus->ExtraDropCount += Extra;
    }
}

void UMythicGA_Rune::SetRuneLootMinRarity(int32 MinRarity) {
    if (FLootTierBonus *Bonus = ResolvePendingLootBonus(TEXT("SetRuneLootMinRarity"))) {
        Bonus->GuaranteedMinRarity = FMath::Max(Bonus->GuaranteedMinRarity, MinRarity);
    }
}

void UMythicGA_Rune::SetRuneLootDropScale(float Scale) {
    if (FLootTierBonus *Bonus = ResolvePendingLootBonus(TEXT("SetRuneLootDropScale"))) {
        // Multiplies rather than overwrites, so two runes on one credit both get their say.
        Bonus->DropCountScale *= FMath::Max(0.0f, Scale);
    }
}

bool UMythicGA_Rune::IsRuneLootForOwner() const {
    if (!PendingLootBonus) {
        UE_LOG(Myth, Warning, TEXT("Runes: %s asked IsRuneLootForOwner outside OnRunePreLootRoll; there is no credit to own"),
               *GetName());
        return false;
    }
    const FGameplayAbilityActorInfo *Info = GetCurrentActorInfo();
    const APlayerController *Owner = Info ? Info->PlayerController.Get() : nullptr;
    if (!Owner) {
        Owner = ResolvePlayerController();
    }
    return PendingLootController != nullptr && PendingLootController == Owner;
}

void UMythicGA_Rune::ShowRuneCallout(FText Text, FLinearColor Color, float LifetimeSeconds) {
    AMythicPlayerController *PC = ResolvePlayerController();
    if (!PC || !IsRuneAuthority()) {
        return;
    }
    const float Lifetime = LifetimeSeconds > 0.0f ? LifetimeSeconds : GetDefault<UMythicCombatSettings>()->RuneCalloutLifetimeSeconds;
    PC->ClientShowCombatCallout(Text, Color, Lifetime);
}

void UMythicGA_Rune::RaiseRuneNotice(EMythicNoticeKind Kind, FText Text) {
    AMythicPlayerController *PC = ResolvePlayerController();
    if (!PC || !IsRuneAuthority()) {
        return;
    }
    FMythicHudNotice Notice;
    Notice.Kind = Kind;
    Notice.Text = Text;
    PC->ClientRaiseHudNotice(Notice);
}

void UMythicGA_Rune::PlayRuneCue(FGameplayTag CueTag, bool bOwnerOnly) {
    const AActor *Avatar = GetAvatarActorFromActorInfo();
    ExecuteRuneCue(CueTag, Avatar ? Avatar->GetActorLocation() : FVector::ZeroVector, bOwnerOnly);
}

void UMythicGA_Rune::PlayRuneCueAt(FGameplayTag CueTag, FVector Location, bool bOwnerOnly) {
    ExecuteRuneCue(CueTag, Location, bOwnerOnly);
}

bool UMythicGA_Rune::ShouldThrottleRuneCue(double SecondsSinceLastPlay, float ThrottleSeconds) {
    return ThrottleSeconds > 0.0f && SecondsSinceLastPlay < ThrottleSeconds;
}

void UMythicGA_Rune::ExecuteRuneCue(FGameplayTag CueTag, const FVector &Location, bool bOwnerOnly) {
    UMythicAbilitySystemComponent *ASC = GetMythicAbilitySystemComponentFromActorInfo();
    const UWorld *World = GetWorld();
    if (!ASC || !World || !CueTag.IsValid() || !IsRuneAuthority()) {
        return;
    }
    const double Now = World->GetTimeSeconds();
    if (const double *Last = CueLastPlayedSeconds.Find(CueTag)) {
        if (ShouldThrottleRuneCue(Now - *Last, GetDefault<UMythicCombatSettings>()->RuneCueThrottleSeconds)) {
            return;
        }
    }
    CueLastPlayedSeconds.Add(CueTag, Now);

    AActor *Avatar = GetAvatarActorFromActorInfo();
    FGameplayCueParameters Params;
    Params.Instigator = Avatar;
    Params.EffectCauser = Avatar;
    Params.SourceObject = Definition.Get();
    Params.Location = Location;
    if (bOwnerOnly) {
        ASC->ExecuteGameplayCueOwnerOnly(CueTag, Params);
    }
    else {
        ASC->ExecuteGameplayCueMulticast(CueTag, Params);
    }
}

void UMythicGA_Rune::SetRuneStateTag(FGameplayTag Tag, bool bOn) {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC || !Tag.IsValid() || !IsRuneAuthority()) {
        return;
    }
    UMythicLifeComponent::SetReplicatedStateTag(ASC, Tag, bOn);
    if (bOn) {
        HeldStateTags.AddTag(Tag);
    }
    else {
        HeldStateTags.RemoveTag(Tag);
    }
}

float UMythicGA_Rune::ScaleRuneCooldown(float Seconds, float CooldownReduction, float MaxReduction) {
    const float Reduction = FMath::Clamp(CooldownReduction, 0.0f, FMath::Max(0.0f, MaxReduction));
    return Seconds * (1.0f - Reduction);
}

float UMythicGA_Rune::ReadCooldownReduction() const {
    const UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC || !ASC->HasAttributeSetForAttribute(UMythicAttributeSet_Utility::GetCooldownReductionAttribute())) {
        return 0.0f;
    }
    // The attribute's own ceiling and the authored ceiling both bind; the tighter one wins.
    const float MaxReduction = FMath::Min(ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetMaxCooldownReductionAttribute()),
                                          GetDefault<UMythicCombatSettings>()->MaxCooldownReduction);
    return FMath::Clamp(ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCooldownReductionAttribute()), 0.0f,
                        FMath::Max(0.0f, MaxReduction));
}

bool UMythicGA_Rune::StartRuneCooldown(float Seconds) {
    UWorld *World = GetWorld();
    if (!World || Seconds <= 0.0f || !IsRuneAuthority()) {
        return false;
    }
    const float Scaled = FMath::Max(ScaleRuneCooldown(Seconds, ReadCooldownReduction(), 1.0f), KINDA_SMALL_NUMBER);
    CooldownEndTimeSeconds = World->GetTimeSeconds() + Scaled;
    World->GetTimerManager().SetTimer(CooldownTimer, this, &UMythicGA_Rune::HandleCooldownEnded, Scaled, false);
    SetRuneHudState(EMythicRuneHudState::Cooldown, Scaled);
    return true;
}

bool UMythicGA_Rune::IsRuneOnCooldown() const {
    return GetRuneCooldownRemaining() > 0.0f;
}

float UMythicGA_Rune::GetRuneCooldownRemaining() const {
    const UWorld *World = GetWorld();
    if (!World || CooldownEndTimeSeconds <= 0.0) {
        return 0.0f;
    }
    return static_cast<float>(FMath::Max(0.0, CooldownEndTimeSeconds - World->GetTimeSeconds()));
}

float UMythicGA_Rune::GetDistanceSinceStill() const {
    const UMythicLifeComponent *Life = ResolveLifeComponent();
    return Life ? Life->GetDistanceSinceStill() : 0.0f;
}

float UMythicGA_Rune::GetSecondsStill() const {
    const UMythicLifeComponent *Life = ResolveLifeComponent();
    return Life ? Life->GetSecondsStill() : 0.0f;
}

float UMythicGA_Rune::GetFallDepthMetres() const {
    const UMythicLifeComponent *Life = ResolveLifeComponent();
    return Life ? Life->GetFallDepthMetres() : 0.0f;
}

void UMythicGA_Rune::CreditRuneTravel(float Metres) {
    UMythicLifeComponent *Life = ResolveLifeComponent();
    if (Life && Metres > 0.0f && IsRuneAuthority()) {
        Life->AddDistanceSinceStill(Metres * 100.0f);
    }
}

float UMythicGA_Rune::GetSkillStaminaCost(UMythicGA_Skill *Skill) const {
    if (!Skill) {
        return 0.0f;
    }
    // The cost keeps its number to itself; reflection is the one door it leaves open.
    static const FStructProperty *CostProperty = FindFProperty<FStructProperty>(UMythicAbilityCost_Stamina::StaticClass(), TEXT("Cost"));
    float Raw = 0.0f;
    for (const UMythicAbilityCost *Cost : Skill->AdditionalCosts) {
        const UMythicAbilityCost_Stamina *Stamina = Cast<UMythicAbilityCost_Stamina>(Cost);
        if (!Stamina || !CostProperty) {
            continue;
        }
        if (const FScalableFloat *Value = CostProperty->ContainerPtrToValuePtr<FScalableFloat>(Stamina)) {
            Raw += Value->GetValueAtLevel(Skill->GetAbilityLevel());
        }
    }
    if (Raw <= 0.0f) {
        return 0.0f;
    }
    float Reduction = 0.0f;
    const UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC && ASC->HasAttributeSetForAttribute(UMythicAttributeSet_Utility::GetStaminaCostReductionAttribute())) {
        Reduction = FMath::Min(ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetStaminaCostReductionAttribute()),
                               GetDefault<UMythicCombatSettings>()->MaxStaminaCostReduction);
    }
    return UMythicLifeComponent::EffectiveStaminaCost(Raw, Reduction);
}

void UMythicGA_Rune::DeferToNextTick(FMythicRuneDeferredDelegate Callback) {
    UWorld *World = GetWorld();
    if (!World || !IsRuneAuthority()) {
        return;
    }
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Callback]() {
        if (bRuneBound) {
            Callback.ExecuteIfBound();
        }
    }));
}

void UMythicGA_Rune::RecordRuneStat(FGameplayTag Stat, int32 Delta) {
    if (!Stat.IsValid() || Delta == 0 || !IsRuneAuthority()) {
        return;
    }
    if (UMythicStatLedgerComponent *Ledger = ResolveStatLedger()) {
        Ledger->RecordStat(Stat, Delta);
    }
    else {
        UE_LOG(Myth, Warning, TEXT("Runes: %s has no stat ledger to record %s on"), *GetName(), *Stat.ToString());
    }
}

int32 UMythicGA_Rune::RepairRuneGear(int32 Amount) {
    if (Amount <= 0 || !IsRuneAuthority()) {
        return 0;
    }
    AMythicPlayerController *PC = ResolvePlayerController();
    if (!PC) {
        return 0;
    }

    int32 Mended = 0;
    for (UMythicInventoryComponent *Inventory : PC->GetAllInventoryComponents()) {
        if (!Inventory) {
            continue;
        }
        for (const FMythicInventorySlotEntry &Slot : Inventory->GetAllSlots()) {
            if (!Slot.IsGearSlot() || !Slot.SlottedItemInstance) {
                continue;
            }
            if (const UDurabilityFragment *Durability = Slot.SlottedItemInstance->GetFragment<UDurabilityFragment>()) {
                const_cast<UDurabilityFragment *>(Durability)->ServerRepair(Amount);
                ++Mended;
            }
        }
    }
    return Mended;
}

int32 UMythicGA_Rune::GetRuneSlotIndex() const {
    const UMythicRuneComponent *Runes = RuneComponent.Get();
    return Runes && Definition ? Runes->FindSlotOfRune(Definition) : INDEX_NONE;
}
