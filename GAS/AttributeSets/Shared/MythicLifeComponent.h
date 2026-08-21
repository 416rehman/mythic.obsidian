
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExtension.h"
#include "MythicAttributeSet_Life.h"
#include "Components/GameFrameworkComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GameplayEffectTypes.h"
#include "Rewards/LootReward.h"
#include "Player/MythicRegistryInterface.h"
#include "MythicLifeComponent.generated.h"

class UMythicAbilitySystemComponent;
class APawn;
class AMythicCorpse;

UCLASS(Blueprintable, BlueprintType)
class UDamageResult : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float OldHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float NewHealth;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMythicHealthChanged, float, New, float, Old, FGameplayAttribute,
                                              Attribute, const FGameplayEffectContextHandle&, ContextHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnDeath, AActor*, DeadActor);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicLifeComponent : public UGameFrameworkComponent {
    GENERATED_BODY()

public:
    UMythicLifeComponent(const FObjectInitializer &ObjectInitializer);

    // Returns the health component if one exists on the specified actor.
    UFUNCTION(BlueprintPure, Category = "Mythic|Health")
    static UMythicLifeComponent *FindHealthComponent(const AActor *Actor) {
        if (!Actor) {
            return nullptr;
        }
        if (const IMythicRegistryInterface *Reg = Cast<IMythicRegistryInterface>(Actor)) {
            return Reg->GetCachedLife();
        }
        return Actor->FindComponentByClass<UMythicLifeComponent>();
    }

    // Initialize the component using an ability system component.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    void InitializeWithAbilitySystem(UAbilitySystemComponent *InASC);

    // True once InitializeWithAbilitySystem has run. Used to avoid re-init on repeated InitializeASC calls.
    UFUNCTION(BlueprintPure, Category = "Mythic|Health")
    bool IsInitialized() const { return AbilitySystemComponent != nullptr; }

    // Uninitialize the component, clearing any references to the ability system.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    void UninitializeFromAbilitySystem();

    // Returns the current health value.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    float GetHealth() const;

    // Returns the current maximum health value.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    float GetMaxHealth() const;

    // Returns the current health in the range [0.0, 1.0].
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    float GetHealthNormalized() const;

    void TriggerGameplayEvent_DeliveredHit(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                           NewValue) const;
    void TriggerGameplayEvent_DeliveredHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                            NewValue);
    void TriggerGameplayEvent_Kill(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue,
                                   float NewValue);
    void TriggerGameplayEvent_ReceivedHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                           NewValue);
    void TriggerGameplayEvent_Death(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                    NewValue);

public:
    void TriggerHealthChange(const FOnAttributeChangeData &OnAttributeChangeData) const {
        if (OnHealthChanged.IsBound()) {
            FGameplayEffectContextHandle Context = FGameplayEffectContextHandle();
            if (auto ModData = OnAttributeChangeData.GEModData) {
                Context = ModData->EffectSpec.GetEffectContext();
            }

            OnHealthChanged.Broadcast(OnAttributeChangeData.NewValue, OnAttributeChangeData.OldValue, OnAttributeChangeData.Attribute, Context);
        }
    }

    void TriggerMaxHealthChange(const FOnAttributeChangeData &OnAttributeChangeData) const {
        if (OnMaxHealthChanged.IsBound()) {
            FGameplayEffectContextHandle Context = FGameplayEffectContextHandle();
            if (auto ModData = OnAttributeChangeData.GEModData) {
                Context = ModData->EffectSpec.GetEffectContext();
            }

            OnMaxHealthChanged.Broadcast(OnAttributeChangeData.NewValue, OnAttributeChangeData.OldValue, OnAttributeChangeData.Attribute, Context);
        }
    }

    UPROPERTY(BlueprintAssignable, Blueprintable)
    FMythicHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Blueprintable)
    FMythicHealthChanged OnMaxHealthChanged;

    // SERVER: broadcast once when the owner dies. NPCs bind this to return to pool / drop loot.
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Health")
    FMythicOnDeath OnDeath;

    // SERVER: broadcast when a revivable owner enters / leaves the co-op downed state (cosmetics: down pose, revive
    // prompt, get-up montage). Same one-actor signature as OnDeath. Only fires when co-op down is enabled.
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Health")
    FMythicOnDeath OnDowned;

    UPROPERTY(BlueprintAssignable, Category = "Mythic|Health")
    FMythicOnDeath OnRevived;

    // Blueprint hook for death cosmetics (ragdoll, montage, VFX). Runs on the server; cosmetics should be driven
    // off the replicated GAS.State.Dead tag on clients.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Health")
    void BP_OnDeath();

    // Seconds before a player-controlled owner is respawned by the GameMode (0 = immediate). NPCs ignore this.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Health")
    float RespawnDelay = 5.0f;

    // combat proficiency XP granted to the killer when this owner dies (0 = none)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Health")
    float XPReward = 0.0f;

    // CO-OP shared kill credit: when a PLAYER kills this owner, every OTHER player whose pawn is within this radius (cm)
    // of the victim also receives the full XPReward — so a partner who fought the kill isn't denied credit by not landing
    // the last hit. The killer is ALWAYS credited regardless of distance. 0 (default) = killer-only (byte-identical to the
    // prior behaviour); set > 0 (e.g. 3000 = 30 m) to enable co-op sharing. Each eligible player gets the FULL reward (no
    // split) — the co-op-friendly default; a proportional/by-contribution split is a logged follow-up.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Health")
    float SharedKillCreditRange = 0.0f;

    // SERVER: loot table(s) rolled when this owner dies, dropped as world items at its location. Designer-
    // assigned per-owner (empty = no drop). Only drops when the killer resolves to a player, since the loot
    // rarity curves are keyed to the killing player's level.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Health")
    FLootTableOverride LootDrop;

    // SERVER (Death keystone): on a lootable NPC death, spawn this corpse actor at the death transform and route the
    // rolled loot INTO its searchable, decaying inventory instead of ejecting loose world items. Defaults to the C++
    // AMythicCorpse (runs unauthored); assign a BP subclass for ragdoll/skeletal visuals + a loot InventoryProfile.
    // If unset or the spawn fails, loot falls back to loose world items exactly as before.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Health")
    TSubclassOf<AMythicCorpse> CorpseClass;

    // SERVER: re-enable the movement + collision that StartDeath disabled, so a revived / pooled-and-reused
    // owner can move again. Mirror of StartDeath's disable (kept colocated as the single source of truth).
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    void RestoreAfterDeath();

    static bool IsEligibleForSharedKillCredit(bool bIsKiller, float DistSqToVictim, float RangeSq);

    static bool IsKillCreditedToOther(const AActor *Victim, const AActor *Killer, const APawn *KillerPawn);

    // True while the owner is in the co-op downed state (incapacitated, bleeding out, revivable).
    UFUNCTION(BlueprintPure, Category = "Mythic|Health")
    bool IsDowned() const { return bIsDowned; }

    // True once the owner has latched the dead state (GAS.State.Dead). Distinct from downed: a dead pawn is NOT revivable.
    UFUNCTION(BlueprintPure, Category = "Mythic|Health")
    bool IsDead() const;

    // SERVER: revive a downed owner — clears the bleed-out timer + downed tag, restores movement, and heals to the
    // configured ReviveHealthFraction (which clears the out-of-health latch). No-op if not downed / off authority.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    void ServerReviveFromDowned();

    static bool CanReviveTarget(bool bTargetDowned, bool bReviverDowned);

    // Proficiency XP paid to the teammate who revives this owner (co-op incentive). 0 (default) = no reward,
    // byte-identical to the prior behaviour.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Health")
    float ReviveXPReward = 0.0f;

    // Which proficiency the revive reward feeds. null (default) = COMBAT XP (the prior behaviour). Set a dedicated
    // support/medic proficiency definition to credit reviving as its OWN skill (a revive is a support act, not combat) —
    // closes the iter-28 logged domain choice without changing the default.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Health")
    TObjectPtr<class UProficiencyDefinition> ReviveRewardProficiency = nullptr;

    bool bPayReviverOnNextRevive = false;

    static float ComputeReviveReward(bool bReviverIsEligiblePlayer, float ConfiguredReward);

    void ServerBeginReviveChannel(APawn *Reviver);

    // Normalized revive-channel progress [0,1] for the UI bar (0 when no channel / ReviveChannelSeconds<=0). Reads the
    // replicated progress so both the downed player and the reviver can show it.
    UFUNCTION(BlueprintPure, Category = "Mythic|Health")
    float GetReviveProgress01() const;

    static float ComputeReviveProgressAfterTick(float CurrentSeconds, float DeltaSeconds, float ChannelSeconds);
    static bool IsReviveComplete(float ProgressSeconds, float ChannelSeconds);
    static bool ShouldContinueReviveChannel(bool bTargetDowned, bool bReviverValid, bool bReviverDowned, bool bReviverInRange);

    static bool ShouldInterruptReviveOnDamage(float ReviverHealthNow, float ReviverHealthAtLastTick);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // SERVER: seconds between regen ticks. Each tick regenerates Health / Shield / Stamina toward their max at
    // the corresponding *RegenRate attribute. 0 disables regen.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Health")
    float RegenInterval = 0.5f;

    // Fraction of the owner's base walk speed while GAS.Debuff.Slowed is active (designer-tunable, 0..1).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|CrowdControl", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SlowMultiplier = 0.5f;

    // Multiple of the owner's base walk speed while GAS.Buff.Haste is active (designer-tunable, >= 1 to speed up).
    // Composes multiplicatively with SlowMultiplier in ReevaluateCrowdControl.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|CrowdControl", meta = (ClampMin = "1.0"))
    float HasteMultiplier = 1.5f;

    // Stagger/hitstun: a received hit dealing >= this FRACTION of MaxHealth briefly applies STUNNED (a halt), with
    // an immunity window to prevent stun-lock. Scales with the entity (a chip hit never staggers). Designer-tunable.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|CrowdControl", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HeavyHitHealthFraction = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|CrowdControl", meta = (ClampMin = "0.0"))
    float StaggerDuration = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|CrowdControl", meta = (ClampMin = "0.0"))
    float StaggerImmunityWindow = 1.5f;

    // ---- Combat state ----
    // SERVER: put the owner in combat and keep them there for CombatTagDuration, refreshing on every fresh hit.
    //
    // GAS.State.InCombat was declared, and READ in four separate places -- the fast-travel block, mount summoning,
    // POI discovery and the HUD -- but was applied by NOTHING. Every one of those "you can't do that in combat"
    // checks was therefore dead code that always said "sure, go ahead". This is the missing half.
    //
    // The tag is a REPLICATED loose tag, not a plain loose one: those stay server-side, and the owning client needs
    // to see this to drive the HUD. It is not a Gameplay Effect because there is no magnitude, no stacking and no
    // duration curve here -- just a flag with a timeout, which is what a loose tag is for.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Combat")
    void MarkInCombat();

    // SERVER: drop out of combat immediately, whatever the timer says (death, respawn, pool recycle).
    UFUNCTION(BlueprintCallable, Category = "Mythic|Combat")
    void ClearInCombat();

    // How long a single hit keeps you "in combat". Long enough that a pause between exchanges is not read as the
    // fight ending, short enough that walking away from one clears promptly.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Combat", meta = (ClampMin = "0.0"))
    float CombatTagDuration = 8.0f;

    // SERVER: attempt to spend stamina (StaminaCostReduction is applied). Returns false (and spends nothing) if
    // the owner does not have enough stamina. Abilities / sprint / dodge call this to gate actions.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    bool TrySpendStamina(float Cost);

    // READ-ONLY affordability check matching TrySpendStamina's reduction rule (client-safe; no spend, no auth gate).
    // The stamina ability cost's CheckCost calls this so check + apply agree (single source of the stamina spend rule).
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    bool CanSpendStamina(float Cost) const;

    static float EffectiveStaminaCost(float RawCost, float StaminaCostReduction);

    static bool IsHeavyHit(float EventMagnitude, float MaxHealth, float HeavyHitHealthFraction);

    static bool IsStaggerImmune(double Now, double LastStaggerTime, float ImmunityWindow);

    static float ComputeRegenTarget(float Cur, float Max, float Rate, float DeltaSeconds);

    static float ComputeStaminaAfterSprintTick(float Cur, float DrainPerSecond, float DeltaSeconds);

    static bool ShouldRecoverFromExhaustion(float CurrentStamina, float MaxStamina, float RecoverFraction);

    void ClearStagger();

    // If set, gameplay event with this tag will be triggered on the Instigator AbilitySystemComponent when the health value goes down (if currently: 0 < health)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Abilities activated by this event have magnitude set to the amount of damage dealt, and OptionalObject2 set to a UDamageResult object.
    // Use Case: On delivering hit, heal for X amount
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Instigator")
    FGameplayTag OnDeliveredHitGameplayEventTag = GAS_EVENT_DMG_DELIVERED;

    // If set, gameplay event with this tag will be triggered on the Instigator AbilitySystemComponent when the health value goes to 0 or below (if currently: 0 < health)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Use Case: Killing enemies has a chance to restore health for X amount
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Instigator")
    FGameplayTag OnKillGameplayEventTag = GAS_EVENT_KILL;

    // If set, gameplay event with this tag will be triggered on the Instigator AbilitySystemComponent when the health value goes up (if currently: 0 < health <= MaxHealth)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Use Case: Healing others restores your health for X amount
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Instigator")
    FGameplayTag OnDeliveredHealGameplayEventTag = GAS_EVENT_HEAL_DELIVERED;

    // If set, gameplay event with this tag will be triggered on the owning AbilitySystemComponent when the health value goes down (if currently: health > 0)
    // To disable this, unset the tag (OnReceivedHit can be used to trigger similar GameplayEvents).
    // Abilities activated by this event have magnitude set to the amount of damage dealt, and OptionalObject2 set to a UDamageResult object.
    // Use Case: When health below threshold, +30% crit chance.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
    FGameplayTag OnReceivedHitGameplayEventTag = GAS_EVENT_DMG_RECEIVED;

    // If set, gameplay event with this tag will be triggered on the owning AbilitySystemComponent when the health value goes up (if currently: 0 < health <= MaxHealth)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Use Case: When healed, increase critical chance by X amount for Y seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
    FGameplayTag OnHealReceivedGameplayEventTag = GAS_EVENT_HEAL_RECEIVED;

    // If set, gameplay event with this tag will be triggered on the owning AbilitySystemComponent when the health value goes to 0 or below (if currently: 0 < health)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Use Case: When you die and you have allies nearby, create an aura at your death location that heals allies for X amount
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
    FGameplayTag OnDeathGameplayEventTag = GAS_EVENT_DEATH;

protected:
    virtual void OnUnregister() override;
    void BroadcastInitialValues();

    void ClearGameplayTags() const;

    void HandleCrowdControlTagChanged(const FGameplayTag Tag, int32 NewCount);

    void HandleMovementAttributeChanged(const FOnAttributeChangeData &ChangeData);

    void ReevaluateCrowdControl();

    float ComputeEncumbranceSpeedScale() const;

    TArray<class UMythicInventoryComponent *> GetOwnerInventoryComponents() const;

    UFUNCTION()
    void HandleInventoryChanged(int32 Slot);

    TArray<TWeakObjectPtr<class UMythicInventoryComponent>> EncumbranceBoundInventories;

    void BindEncumbranceInventoryDelegates();
    void UnbindEncumbranceInventoryDelegates();

    void HandleReceivedHit(const struct FGameplayEventData *Payload);

    void HandleDamageDelivered(const struct FGameplayEventData *Payload);

    void HandleKill(const struct FGameplayEventData *Payload);

    void HandleDeathEvent(const struct FGameplayEventData *Payload);
    void StartDeath(AActor *Killer);

    void SpawnDeathStakeGravestone(class AController *PlayerController, AActor *DeadActor);

    void EnterDownedState(AActor *Killer);

    bool IsOwnerRevivablePlayer() const;

    void ApplyRegen();
    FTimerHandle RegenTimerHandle;

    virtual void HandleHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                     float OldValue, float NewValue);

    virtual void HandleMaxHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                        float OldValue, float NewValue);

protected:
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<const UMythicAttributeSet_Life> LifeSet;

    float BaseWalkSpeed = 0.0f;

    FTimerHandle StaggerTimerHandle;
    bool bStaggered = false;
    double LastStaggerTime = 0.0;

    FTimerHandle CombatTagTimerHandle;

    bool bIsDowned = false;
    FTimerHandle BleedOutTimerHandle;
    TWeakObjectPtr<AActor> DownedKiller;

    void ResetKillContextCapture();
    void CaptureLethalKillContext(const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue);
    int32 DamageEventsTaken = 0;
    bool bLethalCritical = false;
    bool bLethalBurn = false;
    bool bLethalBleed = false;
    bool bLethalPoison = false;
    float LethalOverkillFraction = 0.0f;

    UPROPERTY(Replicated)
    float ReviveProgressSeconds = 0.0f;
    TWeakObjectPtr<APawn> ActiveReviver;
    float ReviverHealthAtLastTick = 0.0f;
    FTimerHandle ReviveChannelTimerHandle;
    void ReviveChannelTick();
    void CancelReviveChannel();
};
