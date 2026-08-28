#include "MythicAttributeSet_Life.h"
#include "MythicAttributeSet_Defense.h"
#include "GameplayEffectExtension.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Feedback/MythicCombatTextTypes.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Player/MythicPlayerController.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"

namespace {
AMythicPlayerController *ResolveCombatTextPlayerFromActor(AActor *Actor) {
    APawn *Pawn = nullptr;
    AController *Controller = nullptr;
    APlayerState *PlayerState = nullptr;
    UMythicGameplayEffectContextLibrary::ResolveInstigator(Actor, Pawn, Controller, PlayerState);
    return Cast<AMythicPlayerController>(Controller);
}

AMythicPlayerController *ResolveCombatTextPlayer(UAbilitySystemComponent *ASC, AActor *FallbackActor) {
    if (ASC) {
        if (AMythicPlayerController *OwnerPC = ResolveCombatTextPlayerFromActor(ASC->GetOwnerActor())) {
            return OwnerPC;
        }
        if (AMythicPlayerController *AvatarPC = ResolveCombatTextPlayerFromActor(ASC->GetAvatarActor())) {
            return AvatarPC;
        }
    }
    return ResolveCombatTextPlayerFromActor(FallbackActor);
}

void RouteResolvedCombatText(const FMythicResolvedCombatTextEvent &BaseEvent,
                             UAbilitySystemComponent *SourceASC,
                             UAbilitySystemComponent *TargetASC) {
    AMythicPlayerController *SourcePC = ResolveCombatTextPlayer(SourceASC, BaseEvent.SourceActor);
    AMythicPlayerController *TargetPC = ResolveCombatTextPlayer(TargetASC, BaseEvent.TargetActor);
    const bool bSourceIsTargetViewer = SourcePC && SourcePC == TargetPC;

    if (UMythicAttributeSet_Life::ShouldRouteResolvedCombatTextToSource(SourcePC != nullptr,
                                                                       bSourceIsTargetViewer)) {
        FMythicResolvedCombatTextEvent OutgoingEvent = BaseEvent;
        OutgoingEvent.bOutgoingForViewer = true;
        SourcePC->QueueResolvedCombatText(OutgoingEvent);
    }

    // A self-authored hit is still damage this viewer received. Route one incoming copy so outgoing-only mode does
    // not turn fall, survival, or other self damage into damage dealt.
    if (UMythicAttributeSet_Life::ShouldRouteResolvedCombatTextToTarget(TargetPC != nullptr)) {
        FMythicResolvedCombatTextEvent IncomingEvent = BaseEvent;
        IncomingEvent.bOutgoingForViewer = false;
        TargetPC->QueueResolvedCombatText(IncomingEvent);
    }
}
}

UMythicAttributeSet_Life::UMythicAttributeSet_Life()
    : MaxHealth(100.0f)
      , Health(100.0f)
      , Damage(0.0f)
      , Healing(0.0f)
      , HealthBeforeAttributeChange(0.0f)
      , MaxHealthBeforeAttributeChange(0.0f) {}

void UMythicAttributeSet_Life::OnRep_MaxHealth(const FGameplayAttributeData &OldMaxHealth) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Life, MaxHealth, OldMaxHealth);
}

void UMythicAttributeSet_Life::OnRep_Health(const FGameplayAttributeData &OldHealth) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Life, Health, OldHealth);
}

void UMythicAttributeSet_Life::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Life, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Life, Health, COND_None, REPNOTIFY_Always);
}

void UMythicAttributeSet_Life::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttributes(Attribute, NewValue);
}

bool UMythicAttributeSet_Life::PreGameplayEffectExecute(FGameplayEffectModCallbackData &Data) {
    if (!Super::PreGameplayEffectExecute(Data)) {
        return false;
    }

    HealthBeforeAttributeChange = GetHealth();
    MaxHealthBeforeAttributeChange = GetMaxHealth();

    return true;
}

void UMythicAttributeSet_Life::AppendHitTags(const FGameplayEffectContextHandle &Context, FGameplayTagContainer &OutTags) {
    if (const FMythicGameplayEffectContext *MythicCtx = FMythicGameplayEffectContext::ExtractEffectContext(Context)) {
        if (MythicCtx->IsCriticalHit()) {
            OutTags.AddTag(GAS_HIT_CRITICAL);
        }
    }
}

void UMythicAttributeSet_Life::SendEventToInstigator(const FGameplayEffectModCallbackData &Data, AActor *Instigator, UAbilitySystemComponent *InstigatorASC,
                                                     UAbilitySystemComponent *OwnerASC, FGameplayTag EventTag, float Magnitude) {
    if (InstigatorASC && OwnerASC) {
        FGameplayEventData Payload;
        Payload.EventTag = EventTag;
        Payload.Instigator = Instigator;
        Payload.Target = OwnerASC->GetAvatarActor();
        Payload.OptionalObject = Data.EffectSpec.Def;
        Payload.ContextHandle = Data.EffectSpec.GetContext();
        Payload.InstigatorTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
        AppendHitTags(Data.EffectSpec.GetContext(), Payload.InstigatorTags);
        Payload.TargetTags = *Data.EffectSpec.CapturedTargetTags.GetAggregatedTags();
        Payload.EventMagnitude = Magnitude;
        InstigatorASC->HandleGameplayEvent(EventTag, &Payload);
    }
}

void UMythicAttributeSet_Life::SendEventToOwner(const FGameplayEffectModCallbackData &Data, UAbilitySystemComponent *OwnerASC, AActor *Instigator,
                                                FGameplayTag EventTag, float Magnitude) {
    if (OwnerASC) {
        FGameplayEventData Payload;
        Payload.EventTag = EventTag;
        Payload.Instigator = Instigator;
        Payload.Target = Instigator;
        Payload.OptionalObject = Data.EffectSpec.Def;
        Payload.ContextHandle = Data.EffectSpec.GetContext();
        Payload.InstigatorTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
        AppendHitTags(Data.EffectSpec.GetContext(), Payload.InstigatorTags);
        Payload.TargetTags = *Data.EffectSpec.CapturedTargetTags.GetAggregatedTags();
        Payload.EventMagnitude = Magnitude;
        OwnerASC->HandleGameplayEvent(EventTag, &Payload);
    }
}

void UMythicAttributeSet_Life::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) {
    Super::PostGameplayEffectExecute(Data);

    const FGameplayEffectContextHandle &EffectContext = Data.EffectSpec.GetEffectContext();
    AActor *Instigator = EffectContext.GetOriginalInstigator();
    AActor *Causer = EffectContext.GetEffectCauser();
    auto InstigatorASC = EffectContext.GetOriginalInstigatorAbilitySystemComponent();
    auto ASC = this->GetOwningAbilitySystemComponent();
    if (Data.EvaluatedData.Attribute == GetDamageAttribute()) {
        if (bOutOfHealth) {
            SetDamage(0.0f);
            return;
        }

        // Damage is a one-way meta attribute. Invalid or negative values fail closed instead of corrupting Health
        // or turning an accidental sign error into untracked healing.
        const float RawDamage = GetDamage();
        const float DamageDone = FMath::IsFinite(RawDamage) ? FMath::Max(0.0f, RawDamage) : 0.0f;

        const float NewHealth = FMath::Clamp(GetHealth() - DamageDone, 0.0f, GetMaxHealth());
        SetHealth(NewHealth);
        const float AppliedHealthDamage = ResolveAppliedHealthDamage(HealthBeforeAttributeChange, NewHealth);

        if (ComputeOutOfHealthLatch(GetHealth()) && !bOutOfHealth && !bInDeathPreHook
            && ASC && ASC->IsOwnerActorAuthoritative()) {
            TGuardValue<bool> ReentryGuard(bInDeathPreHook, true);
            SendEventToOwner(Data, ASC, Instigator, GAS_EVENT_DEATH_PRE, DamageDone);
        }

        if (ComputeOutOfHealthLatch(GetHealth()) && !bOutOfHealth) {
            bOutOfHealth = true;
        }

        SetDamage(0.0f);

        const FMythicGameplayEffectContext *MythicCtx = FMythicGameplayEffectContext::ExtractEffectContext(EffectContext);
        const float RawShieldAbsorbed = MythicCtx ? MythicCtx->GetShieldAbsorbed() : 0.0f;
        const float ShieldAbsorbed = ResolveAppliedShieldDamage(RawShieldAbsorbed);
        const float TotalDealt = DamageDone + ShieldAbsorbed;

        if (TotalDealt > 0.0f) {
            const bool bAuthoritative = ASC && ASC->IsOwnerActorAuthoritative();
            const UMythicStatusEffectDefinition *StatusDefinition =
                ResolvePeriodicStatusDefinition(Data.EffectSpec.GetPeriod(), EffectContext);
            AActor *CombatTextSource = Instigator;
            if (ASC && StatusDefinition && ASC->GetWorld()
                && Instigator == ASC->GetWorld()->GetGameState()) {
                // Only a truly actor-less world status uses the GameState source. Physical hazards own their source
                // identity; the causer here is therefore optional player-facing attribution for world-authored damage.
                CombatTextSource = Causer;
            }

            if (ShouldEmitResolvedCombatText(AppliedHealthDamage, bAuthoritative)) {
                FMythicResolvedCombatTextEvent CombatText;
                CombatText.SourceActor = CombatTextSource;
                CombatText.TargetActor = ASC->GetAvatarActor();
                CombatText.WorldLocation = CombatText.TargetActor
                    ? CombatText.TargetActor->GetActorLocation()
                    : FVector::ZeroVector;
                CombatText.Magnitude = AppliedHealthDamage;
                CombatText.StatusDefinition = const_cast<UMythicStatusEffectDefinition *>(StatusDefinition);
                CombatText.Origin = StatusDefinition
                    ? EMythicCombatTextOrigin::StatusTick
                    : (Instigator ? EMythicCombatTextOrigin::DirectDamage
                                  : EMythicCombatTextOrigin::EnvironmentalDamage);
                CombatText.bCritical = !StatusDefinition && MythicCtx && MythicCtx->IsCriticalHit();
                RouteResolvedCombatText(CombatText, InstigatorASC, ASC);
            }

            if (ShouldEmitResolvedCombatText(ShieldAbsorbed, bAuthoritative)) {
                FMythicResolvedCombatTextEvent ShieldText;
                ShieldText.SourceActor = CombatTextSource;
                ShieldText.TargetActor = ASC->GetAvatarActor();
                ShieldText.WorldLocation = ShieldText.TargetActor
                    ? ShieldText.TargetActor->GetActorLocation()
                    : FVector::ZeroVector;
                ShieldText.Magnitude = ShieldAbsorbed;
                ShieldText.Origin = EMythicCombatTextOrigin::ShieldAbsorption;
                RouteResolvedCombatText(ShieldText, InstigatorASC, ASC);
            }

            const bool bDirectExternalHit =
                (Data.EffectSpec.GetPeriod() <= 0.0f) && Instigator && (Instigator != ASC->GetOwnerActor());
            if (bDirectExternalHit) {
                SendEventToInstigator(Data, Instigator, InstigatorASC, ASC, GAS_EVENT_DMG_DELIVERED, TotalDealt);
            }
            SendEventToOwner(Data, ASC, Instigator, GAS_EVENT_DMG_RECEIVED, TotalDealt);

            if (ASC && ASC->IsOwnerActorAuthoritative()) {
                if (UMythicLifeComponent *VictimLife = UMythicLifeComponent::FindHealthComponent(ASC->GetAvatarActor())) {
                    VictimLife->MarkInCombat();
                }
                if (bDirectExternalHit) {
                    if (UMythicLifeComponent *AttackerLife = UMythicLifeComponent::FindHealthComponent(Instigator)) {
                        AttackerLife->MarkInCombat();
                    }
                }
            }

            if (ASC && ASC->IsOwnerActorAuthoritative()) {
                if (AActor *OwnerActor = ASC->GetOwnerActor()) {
                    if (IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(OwnerActor)) {
                        for (UMythicInventoryComponent *Inv : Provider->GetAllInventoryComponents()) {
                            if (!Inv) {
                                continue;
                            }
                            for (const FMythicInventorySlotEntry &Slot : Inv->GetAllSlots()) {
                                if (!Slot.IsGearSlot()
                                    || !Slot.SlottedItemInstance) {
                                    continue;
                                }
                                if (Slot.SlottedItemInstance->GetFragment<UAttackFragment>()) {
                                    continue;
                                }
                                if (const UDurabilityFragment *Dur = Slot.SlottedItemInstance->GetFragment<UDurabilityFragment>()) {
                                    const_cast<UDurabilityFragment *>(Dur)->ServerApplyWear(1);
                                }
                            }
                        }
                    }
                }
            }

            if (bOutOfHealth && ASC && ASC->IsOwnerActorAuthoritative()) {
                SendEventToOwner(Data, ASC, Instigator, GAS_EVENT_DEATH, DamageDone);

                if (Instigator && Instigator != ASC->GetOwnerActor()) {
                    SendEventToInstigator(Data, Instigator, InstigatorASC, ASC, GAS_EVENT_KILL, DamageDone);

                    if (UMythicAbilitySystemComponent *KillerASC = Cast<UMythicAbilitySystemComponent>(InstigatorASC)) {
                        FGameplayCueParameters CueParams;
                        if (const AActor *VictimActor = ASC->GetAvatarActor()) {
                            CueParams.Location = VictimActor->GetActorLocation();
                        }
                        CueParams.Instigator = Instigator;
                        KillerASC->ExecuteGameplayCueMulticast(TAG_GameplayCue_Combat_KillConfirm, CueParams);
                    }
                }
            }
        }
    }
    else if (Data.EvaluatedData.Attribute == GetHealingAttribute()) {
        const float HealingDone = GetHealing();

        const float NewHealth = FMath::Clamp(GetHealth() + HealingDone, 0.0f, GetMaxHealth());
        SetHealth(NewHealth);
        SetHealing(0.0f);

        bOutOfHealth = ComputeOutOfHealthLatch(GetHealth());

        if (HealingDone > 0.0f) {
            SendEventToInstigator(Data, Instigator, InstigatorASC, ASC, GAS_EVENT_HEAL_DELIVERED, HealingDone);
            SendEventToOwner(Data, ASC, Instigator, GAS_EVENT_HEAL_RECEIVED, HealingDone);
        }
    }
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

        if (ComputeOutOfHealthLatch(GetHealth()) && !bOutOfHealth) {
            bOutOfHealth = true;
            if (ASC && ASC->IsOwnerActorAuthoritative()) {
                SendEventToOwner(Data, ASC, Instigator, GAS_EVENT_DEATH, FMath::Max(0.0f, HealthBeforeAttributeChange - GetHealth()));
            }
        }
        else if (bOutOfHealth && !ComputeOutOfHealthLatch(GetHealth())) {
            bOutOfHealth = false;
        }
    }
    else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute()) {
        if (GetHealth() > GetMaxHealth()) {
            SetHealth(GetMaxHealth());
        }
    }
}


void UMythicAttributeSet_Life::ResetForRespawn() {
    SetHealth(GetMaxHealth());
    bOutOfHealth = false;
    if (UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent()) {
        if (ASC->IsOwnerActorAuthoritative()) {
            UMythicLifeComponent::SetReplicatedStateTag(ASC, GAS_STATE_DYING, false);
            UMythicLifeComponent::SetReplicatedStateTag(ASC, GAS_STATE_DEAD, false);

            if (const UMythicAttributeSet_Defense *DefenseConst = ASC->GetSet<UMythicAttributeSet_Defense>()) {
                const_cast<UMythicAttributeSet_Defense *>(DefenseConst)->ResetCcAndBuildupState();
            }
        }
    }
}

void UMythicAttributeSet_Life::RefreshOutOfHealthLatch() {
    bOutOfHealth = ComputeOutOfHealthLatch(GetHealth());
}

bool UMythicAttributeSet_Life::ComputeOutOfHealthLatch(float NewHealth) {
    return NewHealth <= 0.0f;
}

float UMythicAttributeSet_Life::ResolveAppliedHealthDamage(const float OldHealth, const float NewHealth) {
    if (!FMath::IsFinite(OldHealth) || !FMath::IsFinite(NewHealth)) {
        return 0.0f;
    }
    return FMath::Max(0.0f, OldHealth - NewHealth);
}

float UMythicAttributeSet_Life::ResolveAppliedShieldDamage(const float RawShieldAbsorbed) {
    return FMath::IsFinite(RawShieldAbsorbed) ? FMath::Max(0.0f, RawShieldAbsorbed) : 0.0f;
}

bool UMythicAttributeSet_Life::ShouldEmitResolvedCombatText(const float ResolvedMagnitude, const bool bAuthoritative) {
    return bAuthoritative && FMath::IsFinite(ResolvedMagnitude) && ResolvedMagnitude > 0.0f;
}

bool UMythicAttributeSet_Life::ShouldRouteResolvedCombatTextToSource(const bool bHasSourceViewer,
                                                                    const bool bSourceIsTargetViewer) {
    return bHasSourceViewer && !bSourceIsTargetViewer;
}

bool UMythicAttributeSet_Life::ShouldRouteResolvedCombatTextToTarget(const bool bHasTargetViewer) {
    return bHasTargetViewer;
}

const UMythicStatusEffectDefinition *UMythicAttributeSet_Life::ResolvePeriodicStatusDefinition(
    const float Period, const FGameplayEffectContextHandle &Context) {
    if (!FMath::IsFinite(Period) || Period <= 0.0f || !Context.IsValid()) {
        return nullptr;
    }
    return Cast<UMythicStatusEffectDefinition>(Context.GetSourceObject());
}

EMythicLethalOutcome UMythicAttributeSet_Life::ResolveLethalOutcome(const bool bWouldBeLethal, const bool bCoopDownStateEnabled,
                                                                    const bool bAlreadyDowned, const bool bRevivablePawn) {
    if (!bWouldBeLethal) {
        return EMythicLethalOutcome::Survive;
    }
    if (!bCoopDownStateEnabled) {
        return EMythicLethalOutcome::Die;
    }
    if (!bRevivablePawn) {
        return EMythicLethalOutcome::Die;
    }
    if (bAlreadyDowned) {
        return EMythicLethalOutcome::Die;
    }
    return EMythicLethalOutcome::EnterDownState;
}

void UMythicAttributeSet_Life::ClampAttributes(const FGameplayAttribute &Attribute, float &NewValue) {
    if (Attribute == GetHealthAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetMaxHealthAttribute()) {
        NewValue = FMath::Max(NewValue, 1.0f);
    }
}
