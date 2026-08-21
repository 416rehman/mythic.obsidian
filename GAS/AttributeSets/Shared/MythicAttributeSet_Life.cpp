#include "MythicAttributeSet_Life.h"
#include "MythicAttributeSet_Defense.h"
#include "GameplayEffectExtension.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"
#include "Net/UnrealNetwork.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"

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

        const float DamageDone = GetDamage();

        const float NewHealth = FMath::Clamp(GetHealth() - DamageDone, 0.0f, GetMaxHealth());
        SetHealth(NewHealth);

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
        const float TotalDealt = DamageDone + (MythicCtx ? FMath::Max(0.0f, MythicCtx->GetShieldAbsorbed()) : 0.0f);

        if (TotalDealt > 0.0f) {
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
                                if (!Slot.bEquipmentSlot || !Slot.SlottedItemInstance) {
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
