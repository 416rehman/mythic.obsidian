#include "MythicAttributeSet_Life.h"
#include "GameplayEffectExtension.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
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

    // Save the current health
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
    // Handle Damage meta attribute - converts to -Health
    if (Data.EvaluatedData.Attribute == GetDamageAttribute()) {
        // if already out of health, do nothing
        if (bOutOfHealth) {
            SetDamage(0.0f); // Reset meta attribute
            return;
        }

        // Store the damage magnitude before we clear it
        const float DamageDone = GetDamage();

        // Convert damage to -Health and clamp
        const float NewHealth = FMath::Clamp(GetHealth() - DamageDone, 0.0f, GetMaxHealth());
        SetHealth(NewHealth);
        // Check for death (shared latch rule)
        if (ComputeOutOfHealthLatch(GetHealth()) && !bOutOfHealth) {
            bOutOfHealth = true;
        }

        SetDamage(0.0f); // Reset meta attribute

        // Broadcast health change with the DAMAGE magnitude (for UI/cues)
        if (DamageDone > 0.0f) {
            // OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, -DamageDone, HealthBeforeAttributeChange, GetHealth());

            // DMG_DELIVERED credits a DISCRETE external hit (consumed by lifesteal). Skip it for periodic damage (a
            // single applied DoT would otherwise re-fire every tick → heal the attacker off a FLAT LifePerHit per tick)
            // and for self/environmental damage (Instigator == victim would heal the victim off its own damage). The
            // victim genuinely RECEIVED the damage either way, so DMG_RECEIVED is NOT gated (stagger/UI still want it).
            const bool bDirectExternalHit =
                (Data.EffectSpec.GetPeriod() <= 0.0f) && Instigator && (Instigator != ASC->GetOwnerActor());
            if (bDirectExternalHit) {
                SendEventToInstigator(Data, Instigator, InstigatorASC, ASC, GAS_EVENT_DMG_DELIVERED, DamageDone);
            }
            SendEventToOwner(Data, ASC, Instigator, GAS_EVENT_DMG_RECEIVED, DamageDone);

            // Equipment durability: wear the victim's equipped armor on a landed hit (1 per piece). Reuses the
            // same UDurabilityFragment::ServerApplyWear as weapons (single source of wear). Player victims only -
            // the ASC owner (PlayerState) provides inventory via IInventoryProviderInterface; NPCs have no
            // inventory so this no-ops for them (NPC armor wear deferred, see BACKLOG).
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
                                // Only worn gear (armor) wears from TAKING a hit. Weapons/tools expose an
                                // AttackFragment and wear from being USED (the attack path), so skip them here -
                                // otherwise a weapon would degrade from its owner being attacked.
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

            // Check for death (server-authoritative; PostGameplayEffectExecute is authority-only, guard defensively)
            if (bOutOfHealth && ASC && ASC->IsOwnerActorAuthoritative()) {
                SendEventToOwner(Data, ASC, Instigator, GAS_EVENT_DEATH, DamageDone);

                // Credit the KILL to an EXTERNAL killer. A kill is a DISCRETE event (fires once, on the lethal blow),
                // so — unlike per-hit lifesteal — fire it even when the lethal blow was a periodic/DoT tick (it credits
                // the DoT's original instigator). Self/environmental deaths (no instigator, or instigator == victim)
                // credit no one. Reusable kill primitive: lifesteal-on-kill consumes it now; XP/bounties/kill-streaks
                // can bind GAS_EVENT_KILL the same way.
                if (Instigator && Instigator != ASC->GetOwnerActor()) {
                    SendEventToInstigator(Data, Instigator, InstigatorASC, ASC, GAS_EVENT_KILL, DamageDone);
                }
            }
        }
    }
    // Handle Healing meta attribute - converts to +Health
    else if (Data.EvaluatedData.Attribute == GetHealingAttribute()) {
        const float HealingDone = GetHealing();

        // Convert healing to +Health and clamp
        const float NewHealth = FMath::Clamp(GetHealth() + HealingDone, 0.0f, GetMaxHealth());
        SetHealth(NewHealth);
        SetHealing(0.0f); // Reset meta attribute

        // Keep the death latch in sync: a heal that restores health above 0 must CLEAR bOutOfHealth, exactly as the
        // direct-Health path does on recovery. Without this, an entity healed back from 0 (e.g. a co-op AoE heal / HoT
        // landing on a downed ally) stays "out of health" → the Damage path's early-out makes it permanently
        // damage-immune and unable to die again. (Whether such a heal should ALSO clear the DYING/DEAD tags or fire a
        // revive is a separate design question — see backlog; this is the pure latch-sync correctness fix.)
        bOutOfHealth = ComputeOutOfHealthLatch(GetHealth());

        if (HealingDone > 0.0f) {
            // OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, HealingDone, HealthBeforeAttributeChange, GetHealth());
            SendEventToInstigator(Data, Instigator, InstigatorASC, ASC, GAS_EVENT_HEAL_DELIVERED, HealingDone);
            SendEventToOwner(Data, ASC, Instigator, GAS_EVENT_HEAL_RECEIVED, HealingDone);
        }
    }
    // Handle direct Health changes
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
        // Clamp health
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
        // OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());

        if (ComputeOutOfHealthLatch(GetHealth()) && !bOutOfHealth) {
            bOutOfHealth = true;
            // Direct lethal Health changes (environmental / scripted kills) must trigger death like the Damage path
            // does, otherwise such kills are silent (no respawn / loot / XP).
            if (ASC && ASC->IsOwnerActorAuthoritative()) {
                SendEventToOwner(Data, ASC, Instigator, GAS_EVENT_DEATH, FMath::Max(0.0f, HealthBeforeAttributeChange - GetHealth()));
            }
        }
        else if (bOutOfHealth && !ComputeOutOfHealthLatch(GetHealth())) {
            bOutOfHealth = false;
        }
    }
    // Handle MaxHealth changes
    else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute()) {
        // Clamp current health to new max
        if (GetHealth() > GetMaxHealth()) {
            SetHealth(GetMaxHealth());
        }
        // OnMaxHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxHealthBeforeAttributeChange, GetMaxHealth());
    }
}


void UMythicAttributeSet_Life::ResetForRespawn() {
    SetHealth(GetMaxHealth());
    bOutOfHealth = false;
    // Clear the death latch's replicated tags too, so a reused pawn / persistent player ASC isn't stuck dead
    // and clients drop their death cosmetics. Replicated tag counts are authority-only.
    if (UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent()) {
        if (ASC->IsOwnerActorAuthoritative()) {
            ASC->SetLooseGameplayTagCount(GAS_STATE_DYING, 0);
            ASC->SetLooseGameplayTagCount(GAS_STATE_DEAD, 0);
        }
    }
}

bool UMythicAttributeSet_Life::ComputeOutOfHealthLatch(float NewHealth) {
    return NewHealth <= 0.0f;
}

EMythicLethalOutcome UMythicAttributeSet_Life::ResolveLethalOutcome(const bool bWouldBeLethal, const bool bCoopDownStateEnabled,
                                                                    const bool bAlreadyDowned, const bool bRevivablePawn) {
    // A hit that doesn't bring the entity to zero health is just damage.
    if (!bWouldBeLethal) {
        return EMythicLethalOutcome::Survive;
    }
    // Down-state disabled (the default) → lethal hits kill, exactly as today.
    if (!bCoopDownStateEnabled) {
        return EMythicLethalOutcome::Die;
    }
    // Only revivable pawns (players) can be downed; NPCs / creatures die outright.
    if (!bRevivablePawn) {
        return EMythicLethalOutcome::Die;
    }
    // Being hit while already downed finishes the entity off.
    if (bAlreadyDowned) {
        return EMythicLethalOutcome::Die;
    }
    // Co-op down enabled, a revivable pawn taking its FIRST lethal hit → downed, not dead (teammates can revive).
    return EMythicLethalOutcome::EnterDownState;
}

void UMythicAttributeSet_Life::ClampAttributes(const FGameplayAttribute &Attribute, float &NewValue) {
    if (Attribute == GetHealthAttribute()) {
        // Do not allow health to go negative or above max health.
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetMaxHealthAttribute()) {
        // Do not allow max health to drop below 1.
        NewValue = FMath::Max(NewValue, 1.0f);
    }
}
