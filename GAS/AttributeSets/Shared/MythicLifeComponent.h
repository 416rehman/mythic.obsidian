// 

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

UCLASS(Blueprintable, BlueprintType)
class UDamageResult : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float OldHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float NewHealth;

};

// Event for HealthChange
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMythicHealthChanged, float, New, float, Old, FGameplayAttribute,
                                              Attribute, const FGameplayEffectContextHandle&, ContextHandle);

// SERVER: fired when the owner dies (Health reached 0). Bind to diverge behaviour (NPC pooling, drop loot, etc.).
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

    // SERVER: On Instigator ASC, trigger gameplay event with tag OnDeliveredHitGameplayEventTag.
    void TriggerGameplayEvent_DeliveredHit(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                           NewValue) const;
    // SERVER: On Instigator ASC, trigger gameplay event with tag OnDeliveredHealGameplayEventTag.
    void TriggerGameplayEvent_DeliveredHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                            NewValue);
    // SERVER: On Instigator ASC, trigger gameplay event with tag OnDeathGameplayEventTag.
    void TriggerGameplayEvent_Kill(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue,
                                   float NewValue);
    // SERVER: On owning ASC, trigger gameplay event with tag OnHealReceivedGameplayEventTag.
    void TriggerGameplayEvent_ReceivedHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                           NewValue);
    // SERVER: On owning ASC, trigger gameplay event with tag OnDeathGameplayEventTag.
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

    // SERVER: loot table(s) rolled when this owner dies, dropped as world items at its location. Designer-
    // assigned per-owner (empty = no drop). Only drops when the killer resolves to a player, since the loot
    // rarity curves are keyed to the killing player's level.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic|Health")
    FLootTableOverride LootDrop;

    // SERVER: re-enable the movement + collision that StartDeath disabled, so a revived / pooled-and-reused
    // owner can move again. Mirror of StartDeath's disable (kept colocated as the single source of truth).
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    void RestoreAfterDeath();

    // True while the owner is in the co-op downed state (incapacitated, bleeding out, revivable).
    UFUNCTION(BlueprintPure, Category = "Mythic|Health")
    bool IsDowned() const { return bIsDowned; }

    // SERVER: revive a downed owner — clears the bleed-out timer + downed tag, restores movement, and heals to the
    // configured ReviveHealthFraction (which clears the out-of-health latch). No-op if not downed / off authority.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    void ServerReviveFromDowned();

    // Pure revive-eligibility gate: a downed target can be revived only by a reviver who is NOT themselves downed.
    // (Range is enforced separately by the interaction RPC.) Static + no engine state for unit testing.
    static bool CanReviveTarget(bool bTargetDowned, bool bReviverDowned);

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

    // SERVER: attempt to spend stamina (StaminaCostReduction is applied). Returns false (and spends nothing) if
    // the owner does not have enough stamina. Abilities / sprint / dodge call this to gate actions.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    bool TrySpendStamina(float Cost);

    // READ-ONLY affordability check matching TrySpendStamina's reduction rule (client-safe; no spend, no auth gate).
    // The stamina ability cost's CheckCost calls this so check + apply agree (single source of the stamina spend rule).
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    bool CanSpendStamina(float Cost) const;

    // The actual stamina a RawCost spends after StaminaCostReduction: RawCost * (1 - Clamp(Reduction, 0, 1)). Single
    // source for the reduction math so CanSpendStamina (affordability) and TrySpendStamina (deduction) cannot drift
    // apart. Pure + static so the reduction rule is unit-testable. Reduction 1.0 → free; 0.0 → full cost; clamped so
    // an over-/under-range attribute can't make the cost negative or amplified.
    static float EffectiveStaminaCost(float RawCost, float StaminaCostReduction);

    // Stagger trigger threshold: a hit staggers only if its magnitude is at least HeavyHitHealthFraction of MaxHealth,
    // so the bar scales with the entity (a chip hit never staggers; a boss needs a proportionally bigger hit). A
    // non-positive MaxHealth never staggers. Pure + static for unit testing.
    static bool IsHeavyHit(float EventMagnitude, float MaxHealth, float HeavyHitHealthFraction);

    // Stagger-immunity (anti-stun-lock) gate: immune if the previous stagger started less than ImmunityWindow seconds
    // ago. A non-positive LastStaggerTime means "never staggered yet" → NOT immune, so the first heavy hit always
    // staggers even when it lands before ImmunityWindow seconds of world time have elapsed (a load-into-combat arena).
    // Pure + static for unit testing.
    static bool IsStaggerImmune(double Now, double LastStaggerTime, float ImmunityWindow);

    // One regeneration step for a Cur→Max attribute: Cur + Rate*DeltaSeconds, CLAMPED to Max (never overshoots).
    // Returns Cur UNCHANGED when there's nothing to do (Rate <= 0, or already at/above Max), so a caller can cheaply
    // detect "did it regen?" with `result > Cur` and skip a redundant attribute set. Single source for the regen
    // math shared by Health/Shield/Stamina in ApplyRegen. Pure + static for unit testing.
    static float ComputeRegenTarget(float Cur, float Max, float Rate, float DeltaSeconds);

    // Tear down any in-flight stagger: clear the recovery timer, remove the transient loose STUNNED tag, reset state.
    // Single source for the stagger teardown — called by StartDeath, Uninitialize, AND the pool-return path (so a
    // reused actor never inherits a leftover loose STUNNED tag + orphaned timer and spawn movement-frozen).
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

    // Reacts to crowd-control debuff tags (GAS.Debuff.Stunned/Frozen = cannot move; GAS.Debuff.Slowed = reduced
    // speed) changing on the owning ASC. Bound for NewOrRemoved, so it fires wherever the GE-granted tag is present
    // (server + owning client — correct for client-predicted movement). Delegates to the idempotent recompute.
    void HandleCrowdControlTagChanged(const FGameplayTag Tag, int32 NewCount);

    // Recomputes the owner's movement from the live CC tags (idempotent — no apply/restore pairing). Stun/Frozen
    // disable movement (mirrors StartDeath); Slow scales MaxWalkSpeed by SlowMultiplier off the captured base.
    // No-op while dead (death owns movement) or without a character/movement component.
    void ReevaluateCrowdControl();

    // The encumbrance move-speed multiplier folded into ReevaluateCrowdControl's SpeedScale. 1.0 when encumbrance is
    // disabled (default) or the owner carries within capacity; <1 when Heavy/Overloaded. Pulls the owner's carried
    // weight from its inventory provider (players); NPCs/inventory-less owners are always 1.0.
    float ComputeEncumbranceSpeedScale() const;

    // The owner's inventory components, reached via its controller's inventory provider (players). Empty for an owner
    // with no provider (NPC / inventory-less). Single source for both the weight read and the change-binding below.
    TArray<class UMythicInventoryComponent *> GetOwnerInventoryComponents() const;

    // Bound to each owner inventory's OnSlotUpdated so carried-weight changes (pickup/drop/stack) recompute move speed
    // immediately, not only on a CC tag change. UFUNCTION (the inventory delegate is dynamic). Recompute is gated on
    // bEncumbranceEnabled so the default-off path stays zero-cost.
    UFUNCTION()
    void HandleInventoryChanged(int32 Slot);

    // The inventories this component bound HandleInventoryChanged to (for a clean RemoveDynamic on teardown / re-init).
    TArray<TWeakObjectPtr<class UMythicInventoryComponent>> EncumbranceBoundInventories;

    // (Un)subscribe HandleInventoryChanged from the owner's inventory OnSlotUpdated delegates. Bind on ASC init (post-
    // possession, when the provider exists); unbind on uninit (re-possession / teardown) so no dangling delegate.
    void BindEncumbranceInventoryDelegates();
    void UnbindEncumbranceInventoryDelegates();

    // SERVER: consumes the owner's received-hit event and STAGGERS on a heavy hit (transient STUNNED loose tag,
    // immunity-windowed against stun-lock). The CC handler above turns the tag into a movement halt + restore.
    void HandleReceivedHit(const struct FGameplayEventData *Payload);

    // SERVER: consumes the owner's delivered-hit event (fired on the instigator) and applies LIFESTEAL — heals a flat
    // LifePerHit per landed hit, capped at MaxHealth (mirrors ApplyRegen).
    void HandleDamageDelivered(const struct FGameplayEventData *Payload);

    // SERVER: consumes the owner's KILL event (fired on the killer by the victim's Life set on a lethal blow) and
    // applies LIFESTEAL-ON-KILL — heals a flat LifePerKill, capped at MaxHealth (mirrors HandleDamageDelivered).
    void HandleKill(const struct FGameplayEventData *Payload);

    // SERVER: consumes the owner's death gameplay event and runs the death sequence (idempotent).
    void HandleDeathEvent(const struct FGameplayEventData *Payload);
    // SERVER: applies the Dead state, disables movement, cancels abilities, fires hooks, awards kill XP to Killer,
    // requests player respawn.
    void StartDeath(AActor *Killer);

    // SERVER: enter the co-op downed state instead of dying — downed tag, cancel abilities, disable MOVEMENT (collision
    // kept so the body is reachable for revive), broadcast OnDowned, and start the bleed-out timer that calls StartDeath
    // if no revive arrives in time. Reached from HandleDeathEvent only when co-op down is enabled + the owner is a
    // revivable player.
    void EnterDownedState(AActor *Killer);

    // True if the owner is a player-controlled pawn (the only thing that can be downed/revived; NPCs die outright).
    bool IsOwnerRevivablePlayer() const;

    // SERVER: periodic regen tick (Health / Shield / Stamina toward max). Paused while dead.
    void ApplyRegen();
    FTimerHandle RegenTimerHandle;

    // Internal handler which when the health is changed, sends out the gameplay events.
    virtual void HandleHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                     float OldValue, float NewValue);

    // Internal handler which when the health is changed, sends out the gameplay events.
    virtual void HandleMaxHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                        float OldValue, float NewValue);

protected:
    // Ability system used by this component.
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    // Health set used by this component.
    UPROPERTY()
    TObjectPtr<const UMythicAttributeSet_Life> LifeSet;

    // The owner's un-slowed MaxWalkSpeed, captured at init — the value Slow scales and restore returns to.
    float BaseWalkSpeed = 0.0f;

    // Stagger state: the recovery timer that clears the transient STUNNED, whether a stagger is currently active
    // (no stacking), and the world-time of the last stagger's START (for the immunity window).
    FTimerHandle StaggerTimerHandle;
    bool bStaggered = false;
    double LastStaggerTime = 0.0;

    // Co-op downed state (server-authoritative; only entered for revivable players when the policy is enabled).
    bool bIsDowned = false;
    FTimerHandle BleedOutTimerHandle;
    TWeakObjectPtr<AActor> DownedKiller; // remembered so a bleed-out attributes the eventual death correctly
};
