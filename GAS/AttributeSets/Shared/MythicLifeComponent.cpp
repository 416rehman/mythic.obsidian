

#include "MythicLifeComponent.h"

#include "Mythic.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/MythicHealthBands.h"
#include "Settings/MythicCombatSettings.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/MythicGameMode.h"
#include "GAS/Effects/MythicEnemyScaling.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Engine/World.h"
#include "MythicAttributeSet_Defense.h"
#include "MythicAttributeSet_Utility.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/MythicPlayerController.h"
#include "Objectives/ObjectiveTracker.h"
#include "MythicAttributeSet_Life.h"
#include "Settings/MythicDeveloperSettings.h"
#include "TimerManager.h"
#include "AbilitySystemGlobals.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/Party/PartySubsystem.h"
#include "Player/MythicPlayerState.h"
#include "Itemization/Inventory/MythicEncumbrance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Player/MythicFactionStandingComponent.h"
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "Net/UnrealNetwork.h"
#include "World/Death/MythicCorpse.h"
#include "World/Death/MythicCemeterySubsystem.h"
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceComponent.h"
#include "World/LivingWorld/Acquaintance/MythicAvengerSubsystem.h"
#include "World/LivingWorld/Chronicle/MythicDossierComponent.h"
#include "World/Death/MythicPlayerGravestone.h"
#include "World/Death/MythicDeathStakeSettings.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Engine/GameInstance.h"
#include "AI/MythicTags_AI.h"
#include "Knowledge/MythicCodexTypes.h"
#include "Knowledge/MythicCodexComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "World/LivingWorld/Pressure/MythicRegionalPressureSubsystem.h"
#include "World/LivingWorld/Pressure/MythicTags_Pressure.h"


UMythicLifeComponent::UMythicLifeComponent(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);

    AbilitySystemComponent = nullptr;
    LifeSet = nullptr;

    CorpseClass = AMythicCorpse::StaticClass();
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

    LifeSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
    LifeSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);

    ResetKillContextCapture();

    if (OnDeathGameplayEventTag.IsValid()) {
        AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(OnDeathGameplayEventTag).AddUObject(this, &ThisClass::HandleDeathEvent);
    }

    if (OnReceivedHitGameplayEventTag.IsValid()) {
        AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(OnReceivedHitGameplayEventTag).AddUObject(this, &ThisClass::HandleReceivedHit);
    }

    if (OnDeliveredHitGameplayEventTag.IsValid()) {
        AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(OnDeliveredHitGameplayEventTag).AddUObject(this, &ThisClass::HandleDamageDelivered);
    }

    if (OnKillGameplayEventTag.IsValid()) {
        AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(OnKillGameplayEventTag).AddUObject(this, &ThisClass::HandleKill);
    }

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

    const FGameplayAttribute MovementAttributes[] = {
        UMythicAttributeSet_Utility::GetMovementSpeedMultiplierAttribute(),
        UMythicAttributeSet_Utility::GetBonusSprintSpeedAttribute(),
    };
    for (const FGameplayAttribute &MoveAttribute : MovementAttributes) {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MoveAttribute).AddUObject(
            this, &ThisClass::HandleMovementAttributeChanged);
    }

    BindEncumbranceInventoryDelegates();

    if (GetOwner()->HasAuthority() && RegenInterval > 0.0f && GetWorld()) {
        GetWorld()->GetTimerManager().SetTimer(RegenTimerHandle, this, &ThisClass::ApplyRegen, RegenInterval, true);
    }

    auto HealthAttr = UMythicAttributeSet_Life::GetHealthAttribute();
    auto MaxHealthAttr = UMythicAttributeSet_Life::GetMaxHealthAttribute();

    if (GetOwner()->HasAuthority()) {
        const_cast<UMythicAttributeSet_Life *>(LifeSet.Get())->ResetForRespawn();
        if (const UMythicAttributeSet_Utility *Util = AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>()) {
            AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(), Util->GetMaxStamina());
        }
    }

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(HealthAttr).AddUObject(this, &ThisClass::TriggerHealthChange);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxHealthAttr).AddUObject(this, &ThisClass::TriggerMaxHealthChange);

    auto Health = AbilitySystemComponent->GetNumericAttribute(HealthAttr);
    auto MaxHealth = AbilitySystemComponent->GetNumericAttribute(MaxHealthAttr);

    const FGameplayEffectContextHandle EmptyContextHandle = FGameplayEffectContextHandle();
    OnHealthChanged.Broadcast(Health, 0, HealthAttr, EmptyContextHandle);
    OnMaxHealthChanged.Broadcast(MaxHealth, 0, MaxHealthAttr, EmptyContextHandle);

    RefreshHealthBands();

    ReevaluateCrowdControl();
}

void UMythicLifeComponent::UninitializeFromAbilitySystem() {
    ClearGameplayTags();

    UnbindEncumbranceInventoryDelegates();

    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(RegenTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(BleedOutTimerHandle);
    }

    CancelReviveChannel();

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

    if (AbilitySystemComponent) {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetHealthAttribute()).RemoveAll(this);
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetMaxHealthAttribute()).RemoveAll(this);
    }

    LifeSet = nullptr;
    AbilitySystemComponent = nullptr;
}

void UMythicLifeComponent::HandleReceivedHit(const FGameplayEventData *Payload) {
    if (!Payload || !AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    if (bStaggered || AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return;
    }
    if (!IsHeavyHit(Payload->EventMagnitude, GetMaxHealth(), HeavyHitHealthFraction)) {
        return;
    }
    UWorld *W = GetWorld();
    const double Now = W ? W->GetTimeSeconds() : 0.0;
    if (W && IsStaggerImmune(Now, LastStaggerTime, StaggerImmunityWindow)) {
        return;
    }
    LastStaggerTime = Now;
    bStaggered = true;

    SetReplicatedStateTag(AbilitySystemComponent, GAS_DEBUFF_STUNNED, true);
    if (W) {
        W->GetTimerManager().SetTimer(StaggerTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
            if (AbilitySystemComponent) {
                SetReplicatedStateTag(AbilitySystemComponent, GAS_DEBUFF_STUNNED, false);
            }
            bStaggered = false;
        }), StaggerDuration,false);
    }
    else {
        SetReplicatedStateTag(AbilitySystemComponent, GAS_DEBUFF_STUNNED, false);
        bStaggered = false;
    }
}

void UMythicLifeComponent::HandleDeathEvent(const FGameplayEventData *Payload) {
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return;
    }
    AActor *Killer = Payload ? const_cast<AActor *>(Payload->Instigator.Get()) : nullptr;

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const bool bDownEnabled = Settings && Settings->bCoopDownStateEnabled;
    const EMythicLethalOutcome Outcome =
        UMythicAttributeSet_Life::ResolveLethalOutcome( true, bDownEnabled, bIsDowned, IsOwnerRevivablePlayer());
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
    return bTargetDowned && !bReviverDowned;
}

void UMythicLifeComponent::EnterDownedState(AActor *Killer) {
    if (!AbilitySystemComponent) {
        return;
    }
    AActor *Owner = GetOwner();
    bIsDowned = true;
    DownedKiller = Killer;
    CancelReviveChannel();

    SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_DOWNED, true);
    AbilitySystemComponent->CancelAllAbilities();
    ClearStagger();

    if (ACharacter *Char = Cast<ACharacter>(Owner)) {
        if (UCharacterMovementComponent *Move = Char->GetCharacterMovement()) {
            Move->StopMovementImmediately();
            Move->DisableMovement();
        }
    }

    UE_LOG(Myth, Log, TEXT("LifeComponent: %s is DOWNED (co-op)."), *GetNameSafe(Owner));
    OnDowned.Broadcast(Owner);

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float BleedOut = Settings ? Settings->DownedBleedOutSeconds : 30.0f;
    if (UWorld *W = GetWorld(); W && BleedOut > 0.0f) {
        W->GetTimerManager().SetTimer(BleedOutTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]() {
            if (!bIsDowned) {
                return;
            }
            bIsDowned = false;
            if (AbilitySystemComponent) {
                SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_DOWNED, false);
            }
            StartDeath(DownedKiller.Get());
        }), BleedOut,false);
    }
}

void UMythicLifeComponent::ServerReviveFromDowned() {
    if (!AbilitySystemComponent || !bIsDowned) {
        return;
    }
    APawn *Reviver = ActiveReviver.Get();
    const bool bPayReviver = bPayReviverOnNextRevive;
    bPayReviverOnNextRevive = false;
    if (UWorld *W = GetWorld()) {
        W->GetTimerManager().ClearTimer(BleedOutTimerHandle);
    }
    bIsDowned = false;
    DownedKiller = nullptr;
    SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_DOWNED, false);

    if (ACharacter *Char = Cast<ACharacter>(GetOwner())) {
        if (UCharacterMovementComponent *Move = Char->GetCharacterMovement()) {
            Move->SetMovementMode(MOVE_Walking);
        }
    }

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float Fraction = Settings ? Settings->ReviveHealthFraction : 0.5f;
    if (LifeSet) {
        const float NewHealth = FMath::Max(1.0f, LifeSet->GetMaxHealth() * Fraction);
        AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), NewHealth);
        // Without this the latch set on going down survives the revive and every later hit is zeroed.
        const_cast<UMythicAttributeSet_Life *>(LifeSet.Get())->RefreshOutOfHealthLatch();
    }

    CancelReviveChannel();
    UE_LOG(Myth, Log, TEXT("LifeComponent: %s REVIVED from downed."), *GetNameSafe(GetOwner()));

    AMythicPlayerController *ReviverPC = (bPayReviver && Reviver && Reviver != GetOwner())
        ? Cast<AMythicPlayerController>(Reviver->GetController())
        : nullptr;
    const float ReviveReward = ComputeReviveReward(ReviverPC != nullptr, ReviveXPReward);
    if (ReviveReward > 0.0f && ReviverPC) {
        if (UProficiencyComponent *ProfComp = const_cast<UProficiencyComponent *>(ReviverPC->GetProficiencyComponent())) {
            if (ReviveRewardProficiency) {
                ProfComp->GrantProficiencyXP(ReviveRewardProficiency, ReviveReward);
            }
            else {
                ProfComp->GrantCombatXP(ReviveReward);
            }
        }
    }

    if (bPayReviver && Reviver && Reviver != GetOwner()) {
        const FMythicMoralAction MercyVector = FMythicMoralSignature::MakeMercyActionMoralVector();

        if (UMythicActionEventSubsystem *ActionSub = GetWorld() ? GetWorld()->GetSubsystem<UMythicActionEventSubsystem>() : nullptr) {
            FMythicActionEvent MercyAction;
            MercyAction.Perpetrator = Reviver;
            MercyAction.Victim = GetOwner();
            if (const UMythicCognitiveBrainComponent *ReviverBrain = Reviver->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
                MercyAction.PerpFactionOverride = ReviverBrain->GetFaction();
            }
            MercyAction.ActionTag = TAG_LIVINGWORLD_ACTION_MERCY_TEND;
            MercyAction.CategoryFlags = EMythicEventCategory::Social;
            MercyAction.Significance = 1.0f;
            MercyAction.MoralVector = MercyVector;
            ActionSub->SubmitAction(MercyAction);
        }

        FString ReviverKey;
        if (const AMythicPlayerState *ReviverPS = Reviver->GetPlayerState<AMythicPlayerState>()) {
            ReviverKey = ReviverPS->GetCanonicalPlayerKey();
        }
        if (!ReviverKey.IsEmpty()) {
            if (UMythicPartySubsystem *PartySub = GetWorld() ? GetWorld()->GetSubsystem<UMythicPartySubsystem>() : nullptr) {
                PartySub->OnPlayerAction(ReviverKey, TAG_LIVINGWORLD_ACTION_MERCY_TEND, MercyVector);
            }
        }
    }

    OnRevived.Broadcast(GetOwner());
}

float UMythicLifeComponent::ComputeReviveReward(bool bReviverIsEligiblePlayer, float ConfiguredReward) {
    return bReviverIsEligiblePlayer ? FMath::Max(0.0f, ConfiguredReward) : 0.0f;
}

namespace {
    constexpr float ReviveChannelTickIntervalSeconds = 0.1f;
}

float UMythicLifeComponent::ComputeReviveProgressAfterTick(float CurrentSeconds, float DeltaSeconds, float ChannelSeconds) {
    const float MaxProgress = FMath::Max(0.0f, ChannelSeconds);
    return FMath::Clamp(CurrentSeconds + FMath::Max(0.0f, DeltaSeconds), 0.0f, MaxProgress);
}

bool UMythicLifeComponent::IsReviveComplete(float ProgressSeconds, float ChannelSeconds) {
    return ChannelSeconds > 0.0f && ProgressSeconds >= ChannelSeconds;
}

bool UMythicLifeComponent::ShouldContinueReviveChannel(bool bTargetDowned, bool bReviverValid, bool bReviverDowned, bool bReviverInRange) {
    return bTargetDowned && bReviverValid && !bReviverDowned && bReviverInRange;
}

bool UMythicLifeComponent::ShouldInterruptReviveOnDamage(float ReviverHealthNow, float ReviverHealthAtLastTick) {
    return ReviverHealthNow + KINDA_SMALL_NUMBER < ReviverHealthAtLastTick;
}

bool UMythicLifeComponent::IsDead() const {
    return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD);
}

float UMythicLifeComponent::GetReviveProgress01() const {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float ChannelSeconds = Settings ? Settings->ReviveChannelSeconds : 0.0f;
    if (ChannelSeconds <= 0.0f) {
        return 0.0f;
    }
    return FMath::Clamp(ReviveProgressSeconds / ChannelSeconds, 0.0f, 1.0f);
}

void UMythicLifeComponent::ServerBeginReviveChannel(APawn *Reviver) {
    if (!AbilitySystemComponent || !bIsDowned || !Reviver || !GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float ChannelSeconds = Settings ? Settings->ReviveChannelSeconds : 0.0f;
    if (ChannelSeconds <= 0.0f) {
        ActiveReviver = Reviver;
        bPayReviverOnNextRevive = true;
        ServerReviveFromDowned();
        return;
    }
    UWorld *W = GetWorld();
    if (!W) {
        return;
    }
    if (W->GetTimerManager().IsTimerActive(ReviveChannelTimerHandle)) {
        return;
    }
    ActiveReviver = Reviver;
    ReviveProgressSeconds = 0.0f;
    ReviverHealthAtLastTick = 0.0f;
    if (const UMythicLifeComponent *ReviverLife = FindHealthComponent(Reviver)) {
        ReviverHealthAtLastTick = ReviverLife->GetHealth();
    }
    W->GetTimerManager().SetTimer(ReviveChannelTimerHandle, this, &UMythicLifeComponent::ReviveChannelTick,
                                  ReviveChannelTickIntervalSeconds,true);
}

void UMythicLifeComponent::ReviveChannelTick() {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float ChannelSeconds = Settings ? Settings->ReviveChannelSeconds : 0.0f;
    const float RangeSq = FMath::Square(Settings ? FMath::Max(1.0f, Settings->ReviveChannelRange) : 250.0f);

    const APawn *Reviver = ActiveReviver.Get();
    const bool bReviverValid = Reviver != nullptr;
    bool bReviverIncapacitated = false;
    bool bReviverInRange = false;
    float ReviverHealthNow = ReviverHealthAtLastTick;
    if (bReviverValid) {
        if (const UMythicLifeComponent *ReviverLife = FindHealthComponent(Reviver)) {
            bReviverIncapacitated = ReviverLife->IsDowned() || ReviverLife->IsDead();
            ReviverHealthNow = ReviverLife->GetHealth();
        }
        if (const AActor *Owner = GetOwner()) {
            bReviverInRange = FVector::DistSquared(Owner->GetActorLocation(), Reviver->GetActorLocation()) <= RangeSq;
        }
    }

    const bool bDamageInterrupt = Settings && Settings->bReviveInterruptOnReviverDamage
        && ShouldInterruptReviveOnDamage(ReviverHealthNow, ReviverHealthAtLastTick);

    if (bDamageInterrupt || !ShouldContinueReviveChannel(bIsDowned, bReviverValid, bReviverIncapacitated, bReviverInRange)) {
        CancelReviveChannel();
        return;
    }

    ReviverHealthAtLastTick = ReviverHealthNow;
    ReviveProgressSeconds = ComputeReviveProgressAfterTick(ReviveProgressSeconds, ReviveChannelTickIntervalSeconds, ChannelSeconds);
    if (IsReviveComplete(ReviveProgressSeconds, ChannelSeconds)) {
        bPayReviverOnNextRevive = true;
        ServerReviveFromDowned();
    }
}

void UMythicLifeComponent::CancelReviveChannel() {
    if (UWorld *W = GetWorld()) {
        W->GetTimerManager().ClearTimer(ReviveChannelTimerHandle);
    }
    ActiveReviver = nullptr;
    ReviveProgressSeconds = 0.0f;
    ReviverHealthAtLastTick = 0.0f;
}

void UMythicLifeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UMythicLifeComponent, ReviveProgressSeconds);
}

void UMythicLifeComponent::ClearStagger() {
    if (UWorld *W = GetWorld()) {
        W->GetTimerManager().ClearTimer(StaggerTimerHandle);
    }
    if (bStaggered && AbilitySystemComponent) {
        SetReplicatedStateTag(AbilitySystemComponent, GAS_DEBUFF_STUNNED, false);
    }
    bStaggered = false;
    LastStaggerTime = 0.0;
}

bool UMythicLifeComponent::IsEligibleForSharedKillCredit(bool bIsKiller, float DistSqToVictim, float RangeSq) {
    return bIsKiller || (RangeSq > 0.0f && DistSqToVictim <= RangeSq);
}

void UMythicLifeComponent::SetReplicatedStateTag(UAbilitySystemComponent *ASC, const FGameplayTag &Tag, bool bActive) {
    if (ASC && Tag.IsValid()) {
        ASC->SetLooseGameplayTagCount(Tag, bActive ? 1 : 0, EGameplayTagReplicationState::TagOnly);
    }
}

bool UMythicLifeComponent::IsKillCreditedToOther(const AActor *Victim, const AActor *Killer, const APawn *KillerPawn) {
    if (!Victim || !Killer) {
        return false;
    }
    // A player instigates from their PlayerState, so comparing the instigator alone lets a suicide pay out
    // kill rewards. The pawn behind the instigator is what has to differ from the victim.
    return Killer != Victim && KillerPawn != Victim;
}

namespace {
    void NotifyDeathMemorySystems(AActor *Owner, UAbilitySystemComponent *ASC, AActor *Killer) {
        if (!Owner || !ASC || !Owner->HasAuthority()) {
            return;
        }
        if (const APawn *OwnerPawn = Cast<APawn>(Owner); OwnerPawn && OwnerPawn->IsPlayerControlled()) {
            return;
        }
        const UMythicCognitiveBrainComponent *Brain = Owner->FindComponentByClass<UMythicCognitiveBrainComponent>();
        if (!Brain) {
            return;
        }
        UWorld *World = Owner->GetWorld();
        if (!World) {
            return;
        }

        FMythicCemeteryDeathRecord Record;
        Record.SourceNameHash = GetTypeHash(Owner->GetFName());
        Record.DisplayName = Brain->GetDisplayName();
        Record.RoleTag = Brain->GetRole();
        Record.HomeCell = Brain->GetHomeCell();
        Record.DeathLocation = Owner->GetActorLocation();
        Record.DeathTime = World->GetTimeSeconds();

        FGameplayTagContainer OwnedTags;
        ASC->GetOwnedGameplayTags(OwnedTags);
        static const FGameplayTag AffiliationParent = FGameplayTag::RequestGameplayTag(FName("AI.Affiliation"), false);
        static const FGameplayTag TierParent = FGameplayTag::RequestGameplayTag(FName("AI.Tier"), false);
        int32 TierRank = 0;
        for (const FGameplayTag &T : OwnedTags) {
            if (AffiliationParent.IsValid() && !Record.Faction.IsValid() && T.MatchesTag(AffiliationParent)) {
                Record.Faction = T;
            }
            if (TierParent.IsValid() && T.MatchesTag(TierParent)) {
                TierRank = FMath::Max(TierRank, GetAITierInt(T));
            }
        }
        Record.SourceTier = TierRank;
        Record.Significance = static_cast<float>(TierRank);

        APawn *KillerPawn = nullptr;
        AController *KillerController = nullptr;
        APlayerState *KillerBasePS = nullptr;
        UMythicGameplayEffectContextLibrary::ResolveInstigator(Killer, KillerPawn, KillerController, KillerBasePS);
        AMythicPlayerState *KillerPS = Cast<AMythicPlayerState>(KillerBasePS);
        AMythicPlayerController *KillerPC = Cast<AMythicPlayerController>(KillerController);
        uint32 KillerNameHash = 0;
        if (KillerPS) {
            KillerNameHash = GetTypeHash(KillerPS->GetCanonicalPlayerKey());
        }
        else if (Killer && Killer != Owner) {
            KillerNameHash = GetTypeHash(Killer->GetFName());
        }
        Record.KillerNameHash = KillerNameHash;

        if (UMythicCemeterySubsystem *Cemetery = World->GetSubsystem<UMythicCemeterySubsystem>()) {
            Cemetery->NotifyDeath(Record);
        }

        if (KillerPS) {
            if (UMythicAcquaintanceComponent *Acquaintance = KillerPS->GetAcquaintanceComponent()) {
                Acquaintance->ServerRecordInteraction(Record.SourceNameHash, Record.Faction, EMythicNpcInteraction::Killed);
            }
        }

        if (const AGameStateBase *GameState = World->GetGameState()) {
            for (APlayerState *PS : GameState->PlayerArray) {
                AMythicPlayerState *MythicPS = Cast<AMythicPlayerState>(PS);
                if (!MythicPS) {
                    continue;
                }
                if (UMythicDossierComponent *DossierComp = MythicPS->GetDossierComponent()) {
                    const bool bIsKiller = (MythicPS == KillerPS);
                    DossierComp->ServerRecordNpcDeath(Record.SourceNameHash, KillerNameHash, Record.DisplayName,
                                                      Record.Faction, Record.RoleTag,bIsKiller);
                }
            }
        }

        if (KillerPC) {
            if (UMythicAvengerSubsystem *Avengers = World->GetSubsystem<UMythicAvengerSubsystem>()) {
                Avengers->NotifyNpcKilledByPlayer(Record.SourceNameHash, Record.DisplayName, Record.RoleTag,
                                                  Record.Significance, Brain->GetFaction(), KillerPC);
            }
        }
    }
}

void UMythicLifeComponent::StartDeath(AActor *Killer) {
    if (!AbilitySystemComponent) {
        return;
    }
    AActor *Owner = GetOwner();

    APawn *KillerPawn = nullptr;
    AController *KillerController = nullptr;
    APlayerState *KillerBasePS = nullptr;
    UMythicGameplayEffectContextLibrary::ResolveInstigator(Killer, KillerPawn, KillerController, KillerBasePS);
    AMythicPlayerState *KillerPS = Cast<AMythicPlayerState>(KillerBasePS);
    const bool bKilledByOther = IsKillCreditedToOther(Owner, Killer, KillerPawn);

    SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_DYING, true);
    SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_DEAD, true);
    AbilitySystemComponent->CancelAllAbilities();

    ClearStagger();

    bIsDowned = false;
    CancelReviveChannel();

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

    BP_OnDeath();
    OnDeath.Broadcast(Owner);

    NotifyDeathMemorySystems(Owner, AbilitySystemComponent, Killer);

    if (XPReward > 0.0f && bKilledByOther) {
        if (Cast<AMythicPlayerController>(KillerController)) {
            const FVector VictimLocation = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
            const float RangeSq = FMath::Square(FMath::Max(0.0f, SharedKillCreditRange));
            if (const UWorld *World = GetWorld()) {
                if (const AGameStateBase *GameState = World->GetGameState()) {
                    for (APlayerState *PS : GameState->PlayerArray) {
                        if (!PS) {
                            continue;
                        }
                        AController *ThisController = PS->GetOwningController();
                        AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(ThisController);
                        if (!MythicPC) {
                            continue;
                        }
                        const bool bIsKiller = (ThisController == KillerController);
                        const APawn *PlayerPawn = PS->GetPawn();
                        const float DistSq = PlayerPawn
                            ? FVector::DistSquared(PlayerPawn->GetActorLocation(), VictimLocation)
                            : TNumericLimits<float>::Max();
                        if (!IsEligibleForSharedKillCredit(bIsKiller, DistSq, RangeSq)) {
                            continue;
                        }
                        if (UProficiencyComponent *ProfComp = const_cast<UProficiencyComponent *>(MythicPC->GetProficiencyComponent())) {
                            ProfComp->GrantCombatXP(XPReward);
                        }
                    }
                }
            }
        }
    }

    if (bKilledByOther) {
        if (UMythicCognitiveBrainComponent *VictimBrain = Owner->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
            const FMythicFactionId VictimFaction = VictimBrain->GetFaction();
            if (VictimFaction.IsValid()) {
                if (KillerPS) {
                    if (UMythicFactionStandingComponent *Standing = KillerPS->GetFactionStanding()) {
                        Standing->ServerApplyKillStanding(VictimFaction);
                    }
                }
            }
        }
    }

    if (bKilledByOther && AbilitySystemComponent) {
        if (KillerPS) {
            if (UMythicCodexComponent *KillerCodex = KillerPS->GetCodexComponent()) {
                FGameplayTagContainer VictimOwnedTags;
                AbilitySystemComponent->GetOwnedGameplayTags(VictimOwnedTags);
                const FGameplayTag BestiaryKey = FMythicBestiaryRules::MakeBestiaryKeyFromOwnedTags(VictimOwnedTags);
                if (BestiaryKey.IsValid()) {
                    KillerCodex->ServerRegisterBestiaryKill(BestiaryKey);
                }
            }
        }

        if (KillerPS) {
            FGameplayTagContainer VictimOwnedTags;
            AbilitySystemComponent->GetOwnedGameplayTags(VictimOwnedTags);
            if (VictimOwnedTags.HasTag(AI_KIND_CREATURE)) {
                if (UMythicRegionalPressureSubsystem *Pressure = GetWorld() ? GetWorld()->GetSubsystem<UMythicRegionalPressureSubsystem>() : nullptr) {
                    const FVector KillLoc = Owner->GetActorLocation();
                    Pressure->NotifyHuntingKillNear(KillLoc);

                    FGameplayTag HuntChannel = TAG_Pressure_Hunt;
                    const FGameplayTag BestiaryKey = FMythicBestiaryRules::MakeBestiaryKeyFromOwnedTags(VictimOwnedTags);
                    if (BestiaryKey.IsValid()) {
                        FString Leaf;
                        BestiaryKey.ToString().Split(TEXT("."), nullptr, &Leaf, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
                        if (!Leaf.IsEmpty()) {
                            const FGameplayTag SpeciesChannel = FGameplayTag::RequestGameplayTag(
                                FName(*(FString(TEXT("Pressure.Hunt.")) + Leaf)), false);
                            if (SpeciesChannel.IsValid()) {
                                HuntChannel = SpeciesChannel;
                            }
                        }
                    }
                    const UMythicDeveloperSettings *WaveP = GetDefault<UMythicDeveloperSettings>();
                    const float HuntAmount = WaveP ? WaveP->ApexHunts.HuntPressurePerKill : 1.0f;
                    Pressure->AddPressure(KillLoc, HuntChannel, HuntAmount);
                }
            }
        }
    }

    if (bKilledByOther) {
        if (UMythicActionEventSubsystem *ActionSub = GetWorld() ? GetWorld()->GetSubsystem<UMythicActionEventSubsystem>() : nullptr) {
            FMythicActionEvent KillAction;
            KillAction.Perpetrator = KillerPawn ? static_cast<AActor *>(KillerPawn) : Killer;
            KillAction.Victim = Owner;
            if (UMythicCognitiveBrainComponent *VictimBrain = Owner->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
                KillAction.VictimFactionOverride = VictimBrain->GetFaction();
            }
            AActor *KillerBrainSource = KillerPawn ? static_cast<AActor *>(KillerPawn) : Killer;
            if (UMythicCognitiveBrainComponent *KillerBrain = KillerBrainSource->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
                KillAction.PerpFactionOverride = KillerBrain->GetFaction();
            }
            KillAction.ActionTag = TAG_LIVINGWORLD_ACTION_VIOLENCE_KILL;
            KillAction.CategoryFlags = EMythicEventCategory::Combat | EMythicEventCategory::Death;
            KillAction.Significance = 1.0f;
            KillAction.MoralVector = FMythicMoralSignature::MakeKillActionMoralVector();

            const FString KillerKey = KillerPS ? KillerPS->GetCanonicalPlayerKey() : FString();
            KillAction.PerpPlayerKey = KillerKey;
            ActionSub->SubmitAction(KillAction);

            if (!KillerKey.IsEmpty()) {
                if (UMythicPartySubsystem *PartySub = GetWorld()->GetSubsystem<UMythicPartySubsystem>()) {
                    PartySub->OnPlayerAction(KillerKey, KillAction.ActionTag, KillAction.MoralVector);
                }
            }
        }
    }

    if (LootDrop.LootTables.Num() > 0 && bKilledByOther) {
        APlayerController *KillerPC = Cast<APlayerController>(KillerController);
        if (KillerPC) {
            const FVector DropLoc = Owner->GetActorLocation();

            int32 EnemyTierInt = 0;
            FGameplayTag EnemyTierTag;
            {
                FGameplayTagContainer OwnedTags;
                AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
                static const FGameplayTag TierParent = FGameplayTag::RequestGameplayTag(FName("AI.Tier"), false);
                if (TierParent.IsValid()) {
                    for (const FGameplayTag &T : OwnedTags) {
                        if (T.MatchesTag(TierParent)) {
                            const int32 Rank = GetAITierInt(T);
                            if (Rank > 0) {
                                EnemyTierInt = Rank;
                                EnemyTierTag = T;
                                break;
                            }
                        }
                    }
                }
            }

            /**
             * Without this every drop kept ItemLevel 0, and an item level of zero fails every affix tier gate
             * (lowest MinItemLevel in any shipped pool is 1) and every socket cap. Three affix pools, 70 defs
             * and 134 tiers were invisible in play, and it read as a design choice rather than a bug.
             */
            float WorldItemLevelBase = 1.0f;
            if (const UWorld *World = GetWorld()) {
                if (const AMythicGameState *GS = World->GetGameState<AMythicGameState>()) {
                    if (const UWorldTierAttributes *WTA = GS->WorldTierAttributes) {
                        const float Base = WTA->GetItemLevelBase();
                        // A tier attribute that has not replicated yet reads as zero; that must not mean
                        // "drop nothing worth having".
                        WorldItemLevelBase = Base > 0.0f ? Base : WorldItemLevelBase;
                    }
                }
            }
            const int32 DropItemLevel = FMythicEnemyScaling::ComputeDropItemLevel(WorldItemLevelBase, EnemyTierTag);

            AMythicCorpse *Corpse = nullptr;
            if (UWorld *World = GetWorld()) {
                const TSubclassOf<AMythicCorpse> SpawnClass = CorpseClass ? CorpseClass : TSubclassOf<AMythicCorpse>(AMythicCorpse::StaticClass());
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                const FTransform DeathTransform(Owner->GetActorRotation(), DropLoc);
                Corpse = World->SpawnActor<AMythicCorpse>(SpawnClass, DeathTransform, SpawnParams);
                if (Corpse) {
                    FMythicCorpseIdentity Identity;
                    Identity.SourceNameHash = GetTypeHash(Owner->GetFName());
                    Identity.SourceTier = EnemyTierInt;

                    Identity.KillContext.bCriticalKill = bLethalCritical;
                    Identity.KillContext.bBurnKill = bLethalBurn;
                    Identity.KillContext.bBleedKill = bLethalBleed;
                    Identity.KillContext.bPoisonKill = bLethalPoison;
                    Identity.KillContext.OverkillFraction = LethalOverkillFraction;
                    Identity.KillContext.HitsTaken = DamageEventsTaken;
                    if (const UMythicCognitiveBrainComponent *Brain = Owner->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
                        Identity.RoleTag = Brain->GetRole();
                    }
                    static const FGameplayTag AffiliationParent = FGameplayTag::RequestGameplayTag(FName("AI.Affiliation"), false);
                    static const FGameplayTag KindParent = FGameplayTag::RequestGameplayTag(FName("AI.Kind"), false);
                    if (AffiliationParent.IsValid() || KindParent.IsValid()) {
                        FGameplayTagContainer OwnedTags;
                        AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
                        for (const FGameplayTag &T : OwnedTags) {
                            if (AffiliationParent.IsValid() && !Identity.Faction.IsValid() && T.MatchesTag(AffiliationParent)) {
                                Identity.Faction = T;
                            }
                            if (KindParent.IsValid() && !Identity.SourceKind.IsValid() && T.MatchesTag(KindParent)) {
                                Identity.SourceKind = T;
                            }
                        }
                    }
                    Corpse->ServerInitializeFromDeath(Identity, EnemyTierInt, DeathTransform, nullptr);
                }
            }

            if (SharedKillCreditRange > 0.0f) {
                const float RangeSq = FMath::Square(SharedKillCreditRange);
                FLootTableOverride PersonalSource = LootDrop;
                PersonalSource.IsPrivate = true;
                if (const UWorld *World = GetWorld()) {
                    if (const AGameStateBase *GameState = World->GetGameState()) {
                        for (APlayerState *PS : GameState->PlayerArray) {
                            if (!PS) {
                                continue;
                            }
                            AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PS->GetOwningController());
                            if (!MythicPC) {
                                continue;
                            }
                            const bool bIsKiller = (MythicPC == KillerPC);
                            const APawn *PlayerPawn = PS->GetPawn();
                            const float DistSq = PlayerPawn
                                ? FVector::DistSquared(PlayerPawn->GetActorLocation(), DropLoc)
                                : TNumericLimits<float>::Max();
                            if (!IsEligibleForSharedKillCredit(bIsKiller, DistSq, RangeSq)) {
                                continue;
                            }
                            ULootReward *Reward = NewObject<ULootReward>(this);
                            Reward->OverridenLootSource = PersonalSource;
                            FLootRewardContext Ctx;
                            Ctx.PlayerController = MythicPC;
                            Ctx.PutInInventory = nullptr;
                            Ctx.SpawnLocation = DropLoc;
                            Ctx.EnemyTierInt = EnemyTierInt;
                            Ctx.ItemLevel = DropItemLevel;
                            Reward->Give(Ctx);
                        }
                    }
                }
            }
            else {
                ULootReward *Reward = NewObject<ULootReward>(this);
                Reward->OverridenLootSource = LootDrop;
                FLootRewardContext Ctx;
                Ctx.PlayerController = KillerPC;
                Ctx.PutInInventory = Corpse ? Corpse->GetContainerInventory() : nullptr;
                Ctx.SpawnLocation = DropLoc;
                Ctx.EnemyTierInt = EnemyTierInt;
                Ctx.ItemLevel = DropItemLevel;
                Reward->Give(Ctx);
            }
        }
    }

    if (const APawn *Pawn = Cast<APawn>(Owner)) {
        if (AController *Controller = Pawn->GetController()) {
            if (Controller->IsPlayerController()) {
                const UMythicDeveloperSettings *DSettings = GetDefault<UMythicDeveloperSettings>();
                const float PenaltyFrac = DSettings ? DSettings->DeathProficiencyPenaltyFraction : 0.0f;
                if (PenaltyFrac > 0.0f) {
                    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(Controller)) {
                        if (UProficiencyComponent *ProfComp = const_cast<UProficiencyComponent*>(MythicPC->GetProficiencyComponent())) {
                            ProfComp->ApplyDeathPenalty(PenaltyFrac);
                        }
                    }
                }
                SpawnDeathStakeGravestone(Controller, Owner);
                if (AMythicGameMode *GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMythicGameMode>() : nullptr) {
                    GM->RequestRespawn(Controller, RespawnDelay);
                }
            }
        }
    }
}

void UMythicLifeComponent::SpawnDeathStakeGravestone(AController *PlayerController, AActor *DeadActor) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PlayerController);
    UWorld *World = GetWorld();
    if (!MythicPC || !World) {
        return;
    }

    const UMythicDeathStakeSettings *Settings = GetDefault<UMythicDeathStakeSettings>();
    const FMythicDeathStakeConfig Config = Settings ? Settings->Config : FMythicDeathStakeConfig();

    int32 CarriedGold = 0;
    UItemDefinition *CurrencyDef = nullptr;
    for (UMythicInventoryComponent *Inv : MythicPC->GetAllInventoryComponents()) {
        if (!Inv) {
            continue;
        }
        CarriedGold += Inv->GetTotalCurrency();
        if (!CurrencyDef) {
            for (const FMythicInventorySlotEntry &Entry : Inv->GetAllSlots()) {
                UMythicItemInstance *Inst = Entry.SlottedItemInstance;
                if (Inst && Inst->GetItemDefinition() && Inst->GetItemDefinition()->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
                    CurrencyDef = Inst->GetItemDefinition();
                    break;
                }
            }
        }
    }

    const FVector DeathLocation = DeadActor
        ? DeadActor->GetActorLocation()
        : (MythicPC->GetPawn() ? MythicPC->GetPawn()->GetActorLocation() : FVector::ZeroVector);
    float RegionDanger01 = 0.0f;
    if (UGameInstance *GI = World->GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            if (const UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
                const EMythicDangerTier Tier = Grid->GetCellDangerTier(Grid->WorldToCell(DeathLocation));
                const float Denom = static_cast<float>(static_cast<uint8>(EMythicDangerTier::COUNT) - 1);
                RegionDanger01 = (Denom > 0.0f) ? static_cast<float>(static_cast<uint8>(Tier)) / Denom : 0.0f;
            }
        }
    }

    const int32 StakeAmount = FMythicDeathStakeRules::ComputeStakeAmount(CarriedGold, RegionDanger01, Config);

    TSubclassOf<AMythicPlayerGravestone> SpawnClass = AMythicPlayerGravestone::StaticClass();
    if (Settings && !Settings->GravestoneClass.IsNull()) {
        if (UClass *Loaded = Settings->GravestoneClass.LoadSynchronous()) {
            SpawnClass = Loaded;
        }
    }
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FTransform DeathTransform(DeadActor ? DeadActor->GetActorRotation() : FRotator::ZeroRotator, DeathLocation);
    AMythicPlayerGravestone *Stone = World->SpawnActor<AMythicPlayerGravestone>(SpawnClass, DeathTransform, SpawnParams);
    if (!Stone) {
        return;
    }

    int32 Deducted = 0;
    if (StakeAmount > 0) {
        int32 Remaining = StakeAmount;
        for (UMythicInventoryComponent *Inv : MythicPC->GetAllInventoryComponents()) {
            if (Remaining <= 0) {
                break;
            }
            if (Inv) {
                Remaining -= Inv->SpendCurrency(Remaining);
            }
        }
        Deducted = StakeAmount - Remaining;
    }

    AMythicPlayerState *OwnerPS = MythicPC->GetPlayerState<AMythicPlayerState>();
    Stone->ServerInitializeStake(OwnerPS, Deducted, CurrencyDef, DeathTransform);
}

void UMythicLifeComponent::RestoreAfterDeath() {
    if (ACharacter *Char = Cast<ACharacter>(GetOwner())) {
        if (UCharacterMovementComponent *Move = Char->GetCharacterMovement()) {
            Move->SetMovementMode(MOVE_Walking);
        }
        if (UCapsuleComponent *Capsule = Char->GetCapsuleComponent()) {
            Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }
    }

    ReevaluateCrowdControl();

    ResetKillContextCapture();
}

void UMythicLifeComponent::ResetKillContextCapture() {
    DamageEventsTaken = 0;
    bLethalCritical = false;
    bLethalBurn = false;
    bLethalBleed = false;
    bLethalPoison = false;
    LethalOverkillFraction = 0.0f;
}

void UMythicLifeComponent::CaptureLethalKillContext(const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue) {
    if (!DamageEffectSpec) {
        return;
    }
    const FGameplayEffectContextHandle ContextHandle = DamageEffectSpec->GetContext();
    if (const FMythicGameplayEffectContext *MythicCtx = FMythicGameplayEffectContext::ExtractEffectContext(ContextHandle)) {
        bLethalCritical = MythicCtx->IsCriticalHit();
        bLethalBurn = MythicCtx->IsBurn();
        bLethalBleed = MythicCtx->IsBleed();
        bLethalPoison = MythicCtx->IsPoison();
    }
    const float MaxHealth = FMath::Max(1.0f, GetMaxHealth());
    LethalOverkillFraction = FMath::Max(0.0f, (DamageMagnitude - FMath::Max(0.0f, OldValue)) / MaxHealth);
}

void UMythicLifeComponent::HandleCrowdControlTagChanged(const FGameplayTag Tag, int32 NewCount) {
    ReevaluateCrowdControl();
}

void UMythicLifeComponent::HandleMovementAttributeChanged(const FOnAttributeChangeData &ChangeData) {
    ReevaluateCrowdControl();
}

void UMythicLifeComponent::ReevaluateCrowdControl() {
    if (!AbilitySystemComponent) {
        return;
    }
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)
        || AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DOWNED)) {
        return;
    }
    ACharacter *Char = Cast<ACharacter>(GetOwner());
    UCharacterMovementComponent *Move = Char ? Char->GetCharacterMovement() : nullptr;
    if (!Move) {
        return;
    }

    const bool bCannotAct = AbilitySystemComponent->HasMatchingGameplayTag(GAS_DEBUFF_STUNNED)
        || AbilitySystemComponent->HasMatchingGameplayTag(GAS_DEBUFF_FROZEN);

    if (bCannotAct) {
        if (Move->MovementMode != MOVE_None) {
            Move->StopMovementImmediately();
            Move->DisableMovement();
        }
        return;
    }

    if (Move->MovementMode == MOVE_None) {
        Move->SetMovementMode(MOVE_Walking);
    }
    const bool bHasted = AbilitySystemComponent->HasMatchingGameplayTag(GAS_BUFF_HASTE);
    float SpeedScale = 1.0f;
    // Each active slow carries its own rolled bite; they stack multiplicatively and can never fully stop the target.
    // Falls back to the flat SlowMultiplier when no active slow has an authored band. Gated on the tag so an
    // unslowed pawn never pays for the active-effect walk.
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_DEBUFF_SLOWED)) {
        SpeedScale *= UMythicStatusRegistry::GetControlReductionMultiplier(AbilitySystemComponent, GAS_DEBUFF_SLOWED, 1.0f - SlowMultiplier);
    }
    if (bHasted) {
        SpeedScale *= HasteMultiplier;
    }

    const FGameplayTag SprintTag = GAS_STATE_SPRINTING;
    if (SprintTag.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(SprintTag)
        && !AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_EXHAUSTED)) {
        if (const UMythicAttributeSet_Utility *Util = AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>()) {
            SpeedScale *= (1.0f + Util->GetBonusSprintSpeed());
        }
    }

    if (const UMythicAttributeSet_Utility *SpeedUtil = AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>()) {
        SpeedScale *= FMath::Max(0.0f, SpeedUtil->GetMovementSpeedMultiplier());
    }

    SpeedScale *= ComputeEncumbranceSpeedScale();
    Move->MaxWalkSpeed = BaseWalkSpeed * SpeedScale;
}

TArray<UMythicInventoryComponent *> UMythicLifeComponent::GetOwnerInventoryComponents() const {
    const APawn *OwnerPawn = Cast<APawn>(GetOwner());
    AController *OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
    if (IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(OwnerController)) {
        return Provider->GetAllInventoryComponents();
    }
    return TArray<UMythicInventoryComponent *>();
}

float UMythicLifeComponent::ComputeEncumbranceSpeedScale() const {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bEncumbranceEnabled) {
        return 1.0f;
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

void UMythicLifeComponent::HandleInventoryChanged(int32) {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (Settings && Settings->bEncumbranceEnabled) {
        ReevaluateCrowdControl();
    }
}

void UMythicLifeComponent::BindEncumbranceInventoryDelegates() {
    UnbindEncumbranceInventoryDelegates();
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
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return;
    }
    const UMythicAttributeSet_Defense *Def = AbilitySystemComponent->GetSet<UMythicAttributeSet_Defense>();
    if (!LifeSet || !Def) {
        return;
    }
    const float LifePerHit = Def->GetLifePerHit();
    if (LifePerHit <= 0.0f) {
        return;
    }
    const float Cur = LifeSet->GetHealth();
    const float Max = LifeSet->GetMaxHealth();
    if (Cur > 0.0f && Cur < Max) {
        AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),
                                                        FMath::Min(Cur + LifePerHit, Max));
    }
}

void UMythicLifeComponent::HandleKill(const struct FGameplayEventData *Payload) {
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    if (AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
        return;
    }
    const UMythicAttributeSet_Defense *Def = AbilitySystemComponent->GetSet<UMythicAttributeSet_Defense>();
    if (!LifeSet || !Def) {
        return;
    }
    const float LifePerKill = Def->GetLifePerKill();
    if (LifePerKill <= 0.0f) {
        return;
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
        return;
    }

    const UMythicAttributeSet_Defense *Def = AbilitySystemComponent->GetSet<UMythicAttributeSet_Defense>();

    if (LifeSet && Def) {
        const float Cur = LifeSet->GetHealth();
        const float NewV = ComputeRegenTarget(Cur, LifeSet->GetMaxHealth(), Def->GetHealthRegenRate(), RegenInterval);
        if (Cur > 0.0f && NewV > Cur) {
            AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), NewV);
        }
    }

    if (Def) {
        const float Cur = Def->GetShield();
        const float NewV = ComputeRegenTarget(Cur, Def->GetMaxShield(), Def->GetShieldRegenRate(), RegenInterval);
        if (NewV > Cur) {
            AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetShieldAttribute(), NewV);
        }
    }

    if (const UMythicAttributeSet_Utility *Util = AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>()) {
        const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
        const bool bSprintGate = Settings && Settings->bStaminaGatedSprint;
        const bool bExhausted = AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_EXHAUSTED);

        bool bActivelySprinting = false;
        if (bSprintGate && !bExhausted && Util->GetStaminaRegenRate() > 0.0f
            && AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_SPRINTING)) {
            const AActor *OwnerActor = GetOwner();
            bActivelySprinting = OwnerActor && OwnerActor->GetVelocity().SizeSquared2D() > 100.0f;
        }

        const float Cur = Util->GetCurrentStamina();
        if (bActivelySprinting) {
            const float NewV = ComputeStaminaAfterSprintTick(Cur, Settings->SprintStaminaDrainPerSecond, RegenInterval);
            if (NewV != Cur) {
                AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(), NewV);
            }
            if (NewV <= 0.0f) {
                SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_EXHAUSTED, true);
                ReevaluateCrowdControl();
                if (const APawn *OwnerPawn = Cast<APawn>(GetOwner())) {
                    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(OwnerPawn->GetController())) {
                        PC->ClientNotifyExhausted(true);
                    }
                }
            }
        }
        else {
            const float NewV = ComputeRegenTarget(Cur, Util->GetMaxStamina(), Util->GetStaminaRegenRate(), RegenInterval);
            if (NewV > Cur) {
                AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(), NewV);
            }
            if (bSprintGate && bExhausted && ShouldRecoverFromExhaustion(NewV, Util->GetMaxStamina(), Settings->SprintRecoverStaminaFraction)) {
                SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_EXHAUSTED, false);
                ReevaluateCrowdControl();
                if (const APawn *OwnerPawn = Cast<APawn>(GetOwner())) {
                    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(OwnerPawn->GetController())) {
                        PC->ClientNotifyExhausted(false);
                    }
                }
            }
        }
    }

    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        if (Settings->StatusBuildupDecayPerSecond > 0.0f) {
            UMythicAttributeSet_Defense::DecayAllBuildups(AbilitySystemComponent, Settings->StatusBuildupDecayPerSecond, RegenInterval);
        }
    }
}

float UMythicLifeComponent::ComputeRegenTarget(float Cur, float Max, float Rate, float DeltaSeconds) {
    if (Rate <= 0.0f || Cur >= Max) {
        return Cur;
    }
    return FMath::Min(Cur + Rate * DeltaSeconds, Max);
}

float UMythicLifeComponent::ComputeStaminaAfterSprintTick(float Cur, float DrainPerSecond, float DeltaSeconds) {
    const float Drain = FMath::Max(0.0f, DrainPerSecond) * FMath::Max(0.0f, DeltaSeconds);
    return FMath::Max(0.0f, Cur - Drain);
}

bool UMythicLifeComponent::ShouldRecoverFromExhaustion(float CurrentStamina, float MaxStamina, float RecoverFraction) {
    if (MaxStamina <= 0.0f) {
        return false;
    }
    const float Threshold = FMath::Clamp(RecoverFraction, 0.0f, 1.0f) * MaxStamina;
    return CurrentStamina >= Threshold;
}

bool UMythicLifeComponent::CanSpendStamina(float Cost) const {
    if (Cost <= 0.0f) {
        return true;
    }
    const UMythicAttributeSet_Utility *Util = AbilitySystemComponent ? AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>() : nullptr;
    if (!Util) {
        return false;
    }
    return Util->GetCurrentStamina() >= EffectiveStaminaCost(Cost, Util->GetStaminaCostReduction());
}

float UMythicLifeComponent::EffectiveStaminaCost(float RawCost, float StaminaCostReduction) {
    return RawCost * (1.0f - FMath::Clamp(StaminaCostReduction, 0.0f, 1.0f));
}

bool UMythicLifeComponent::IsHeavyHit(float EventMagnitude, float MaxHealth, float HeavyHitHealthFraction) {
    return MaxHealth > 0.0f && EventMagnitude >= HeavyHitHealthFraction * MaxHealth;
}

bool UMythicLifeComponent::IsStaggerImmune(double Now, double LastStaggerTime, float ImmunityWindow) {
    if (LastStaggerTime <= 0.0) {
        return false;
    }
    return (Now - LastStaggerTime) < ImmunityWindow;
}

bool UMythicLifeComponent::TrySpendStamina(float Cost) {
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative() || Cost <= 0.0f) {
        return Cost <= 0.0f;
    }
    const UMythicAttributeSet_Utility *Util = AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>();
    if (!Util) {
        return false;
    }
    const float Effective = EffectiveStaminaCost(Cost, Util->GetStaminaCostReduction());
    const float Cur = Util->GetCurrentStamina();
    if (Cur < Effective) {
        return false;
    }
    AbilitySystemComponent->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(), Cur - Effective);
    return true;
}

void UMythicLifeComponent::ClearGameplayTags() const {
    if (AbilitySystemComponent && AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_DYING, false);
        SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_DEAD, false);
        SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_EXHAUSTED, false);
        SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_INCOMBAT, false);
    }
}

void UMythicLifeComponent::MarkInCombat() {
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    if (!AbilitySystemComponent->HasMatchingGameplayTag(GAS_STATE_INCOMBAT)) {
        SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_INCOMBAT, true);
    }
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(CombatTagTimerHandle, this, &UMythicLifeComponent::ClearInCombat,
                                          FMath::Max(CombatTagDuration, 0.1f), false);
    }
}

void UMythicLifeComponent::ClearInCombat() {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(CombatTagTimerHandle);
    }
    if (AbilitySystemComponent && AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        SetReplicatedStateTag(AbilitySystemComponent, GAS_STATE_INCOMBAT, false);
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

    if (SharedKillCreditRange > 0.0f) {
        const AActor *KillerAvatar = SourceASC->GetAvatarActor();
        const AController *KillerController = nullptr;
        if (const APawn *KillerPawn = Cast<APawn>(KillerAvatar)) {
            KillerController = KillerPawn->GetController();
        }
        if (Cast<AMythicPlayerController>(KillerController)) {
            const AActor *VictimActor = AbilitySystemComponent->GetAvatarActor();
            const FVector VictimLocation = VictimActor ? VictimActor->GetActorLocation() : FVector::ZeroVector;
            const float RangeSq = FMath::Square(SharedKillCreditRange);
            if (const UWorld *World = GetWorld()) {
                if (const AGameStateBase *GameState = World->GetGameState()) {
                    for (APlayerState *PS : GameState->PlayerArray) {
                        if (!PS) {
                            continue;
                        }
                        AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PS->GetOwningController());
                        if (!MythicPC || MythicPC == KillerController) {
                            continue;
                        }
                        const APawn *PlayerPawn = PS->GetPawn();
                        const float DistSq = PlayerPawn
                            ? FVector::DistSquared(PlayerPawn->GetActorLocation(), VictimLocation)
                            : TNumericLimits<float>::Max();
                        if (!IsEligibleForSharedKillCredit(false, DistSq, RangeSq)) {
                            continue;
                        }
                        if (UObjectiveTracker *Tracker = MythicPC->GetObjectiveTracker()) {
                            Tracker->ApplySharedKillCredit(Payload);
                        }
                    }
                }
            }
        }
    }
}

void UMythicLifeComponent::HandleHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec,
                                               float DamageMagnitude, float OldValue, float NewValue) {
    if (NewValue < OldValue && AbilitySystemComponent && AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        ++DamageEventsTaken;
    }

    if (NewValue > OldValue && OldValue > 0.0f) {
        TriggerGameplayEvent_DeliveredHeal(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
        TriggerGameplayEvent_ReceivedHeal(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
    }
    else if (NewValue <= 0.0f) {
        if (AbilitySystemComponent && AbilitySystemComponent->IsOwnerActorAuthoritative()) {
            CaptureLethalKillContext(DamageEffectSpec, DamageMagnitude, OldValue);
        }
        TriggerGameplayEvent_Death(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
        TriggerGameplayEvent_Kill(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
    }
    else if (NewValue < OldValue && OldValue > 0.0f) {
        TriggerGameplayEvent_DeliveredHit(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
    }

    auto ContextHandle = DamageEffectSpec->GetContext();
}

void UMythicLifeComponent::HandleMaxHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec,
                                                  float DamageMagnitude, float OldValue, float NewValue) {}

void UMythicLifeComponent::RefreshHealthBands() const {
    if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative()) {
        return;
    }
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings || !Settings->HealthBands.IsConfigured()) {
        return;
    }
    const UMythicAttributeSet_Life *Life = AbilitySystemComponent->GetSet<UMythicAttributeSet_Life>();
    if (!Life) {
        return;
    }

    const float Fraction = FMythicHealthBandRules::FractionOf(Life->GetHealth(), Life->GetMaxHealth());
    FGameplayTagContainer Active;
    FMythicHealthBandRules::ResolveBands(Settings->HealthBands.Bands, Fraction, Active);

    for (const FMythicHealthBand &Band : Settings->HealthBands.Bands) {
        if (Band.Tag.IsValid()) {
            SetReplicatedStateTag(AbilitySystemComponent, Band.Tag, Active.HasTagExact(Band.Tag));
        }
    }
}
