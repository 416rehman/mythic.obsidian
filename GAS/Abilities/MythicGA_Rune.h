#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "UI/HUD/MythicHudNotice.h"
#include "MythicGA_Rune.generated.h"

class AMythicCharacter;
class AMythicPlayerController;
class APlayerController;
class UGameplayEffect;
class UMythicGA_Skill;
class UMythicLifeComponent;
class UMythicLootManagerSubsystem;
class UMythicRuneDefinition;
class UMythicStatLedgerComponent;
struct FGameplayEventData;
struct FLootTierBonus;

DECLARE_DYNAMIC_DELEGATE(FMythicRuneDeferredDelegate);

/**
 * Base of every rune. A rune is a Blueprint on this class that changes one rule of the game; C++ holds only the
 * seams the Blueprint listens on and the verbs it may call back into. UMythicRuneComponent grants it with the rune
 * definition as the spec's source object; it stays active from grant to unequip and refuses the downed / death
 * cancel sweep, so a cheat-death rune is still listening when the lethal blow lands.
 *
 * Server only. Everything a client sees leaves through the rune HUD channel, a cue, or a Client RPC.
 */
UCLASS(Abstract, Blueprintable)
class MYTHIC_API UMythicGA_Rune : public UMythicGameplayAbility {
    GENERATED_BODY()

public:
    UMythicGA_Rune(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                            const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

    virtual void OnAvatarSet(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) override;

    // ---- Seams the rune Blueprint implements. All fire on the server only. ----

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneActivated();

    // The rune is leaving its socket. The ability is still active here, so every verb below still works.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneRemoved();

    // A lethal blow landed and the owner is at zero health for the length of this call. PreventDeath works here
    // and nowhere else. Payload: Instigator is the killer, EventMagnitude the damage, ContextHandle the hit.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRunePreDeath(const FGameplayEventData &Payload);

    // Every event the rune listens to (PreDeath, Dmg.Pre, Dmg.Delivered, Dmg.Received, Kill, Moved, Item.Acquired,
    // Item.Used, Currency.Spent, Harvest.Struck), including the ones that also have a dedicated seam.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneEvent(FGameplayTag EventTag, const FGameplayEventData &Payload);

    // GAS.Event.Kill, and a Harvest.Struck that felled its node while the owner holds Rune.Rule.FelledIsKill.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneKill(const FGameplayEventData &Payload);

    // Dmg.Pre: the owner's swing is about to land. Payload.ContextHandle is the live context every target of the
    // swing will be hit with, so SetOutgoingHitMultiplier on it empowers the whole swing.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneOutgoingHit(const FGameplayEventData &Payload);

    // The avatar touched down and fall damage was decided. ImpactSpeed in cm/s; FallDamage is what the landing was
    // worth before the gate; bPrevented when an immunity or a zero roll kept it off the owner.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneLanded(float ImpactSpeed, float FallDamage, bool bPrevented);

    // The avatar's movement mode entered Falling.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneFallBegan();

    // Metres fallen so far, sampled while the avatar is Falling.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneFallDepth(float Metres);

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneStartedMoving();

    // The first still sample after travel, before the grace decides the journey is over. Once per stop.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneStillBegan();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneStoppedMoving();

    // Ground travel accrued since the previous report, in centimetres.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneDistance(float CentimetresAccrued);

    // A skill that moves the caster (Dash or Teleport) started; StartLocation is where the avatar stood.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneSkillDashStarted(UMythicGA_Skill *Skill, FVector StartLocation);

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneSkillDashEnded(UMythicGA_Skill *Skill, FVector EndLocation);

    // The guard from BeginRuneGuard ran out or was ended. The pre-guard Shield is already back.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneGuardEnded();

    // A cooldown started by StartRuneCooldown ran out. The HUD state is already Hidden; set Ready here if the rune
    // has a ready moment.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRuneCooldownEnded();

    /**
     * A slain enemy's tables are about to roll for CreditedTo, with the tier and find bonus already in hand. The four
     * loot verbs below work here and nowhere else. Every rune in the session hears every credit, so a rune that pays
     * its own wearer must ask IsRuneLootForOwner first.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Rune")
    void OnRunePreLootRoll(APlayerController *CreditedTo);

    // ---- Verbs. Server only; each one delegates to the system that owns the outcome. ----

    /**
     * Inside OnRunePreDeath only: the owner survives the blow at RestoreHealthFraction (0..1) of max health, and the
     * death pipeline sends no Death event. False when called anywhere else, or when another rune already saved
     * this death.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    bool PreventDeath(float RestoreHealthFraction);

    /**
     * Multiplies Multiplier onto the hit the context is about to deal and stamps HitTag on it so damage events and
     * combat text can tell the hit apart. Multiplies rather than overwrites, so two runes on one swing stack.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void SetOutgoingHitMultiplier(FGameplayEffectContextHandle Context, float Multiplier, FGameplayTag HitTag);

    // True when this rune's own definition is the context's source object: its own container or self-wound.
    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    bool IsOwnHit(FGameplayEffectContextHandle Context) const;

    // The owner's roll for Parameter; the definition's range midpoint when nothing was rolled; Fallback when the
    // definition has no such parameter.
    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    float ReadRolled(FGameplayTag Parameter, float Fallback) const;

    // What the owner's HUD shows on this rune's cell. DurationSeconds > 0 draws a timer that ends then.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void SetRuneHudState(EMythicRuneHudState State, float DurationSeconds = 0.0f, int32 Stacks = 0);

    /**
     * Raises MaxShield then Shield by ShieldAmount for DurationSeconds. The Shield the owner had before is handed
     * back when the guard ends, whatever the guard absorbed. A guard begun while one holds replaces it silently.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void BeginRuneGuard(float ShieldAmount, float DurationSeconds);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void EndRuneGuard();

    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    bool IsRuneGuardActive() const { return bGuardActive; }

    // Amount through the Damage meta on the owner with HitTag stamped on the hit, so numbers, Dmg.Received and
    // PreDeath all see it. No armour by design: the wound was already decided.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void DealRuneSelfDamage(float Amount, FGameplayTag HitTag);

    // Amount through the Healing meta on the owner, so the death latch refreshes the way any heal does.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void HealRune(float Amount);

    // All-or-nothing debit across the owner's purses. False charges nothing.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    bool TryChargeCurrency(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    int32 GetCarriedGold() const;

    // Mints Amount into the owner's purse; returns what was minted.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    int32 GrantGold(int32 Amount);

    // Inside OnRunePreLootRoll only: Extra more drops from every table this credit rolls. Elsewhere it warns and
    // changes nothing, because the bonus it would edit only exists for the length of that call.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void AddRuneLootDrops(int32 Extra);

    // Inside OnRunePreLootRoll only: floors the rarity this credit pays. A floor below the one the roll already
    // carries is ignored, so two runes cannot argue the rarity back down.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void SetRuneLootMinRarity(int32 MinRarity);

    // Inside OnRunePreLootRoll only: multiplies the drop count every table of this credit pays. 0 pays nothing.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void SetRuneLootDropScale(float Scale);

    // Inside OnRunePreLootRoll only: the credit being rolled belongs to this rune's own wearer, not a party member's.
    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    bool IsRuneLootForOwner() const;

    // Owner-only text over the pawn through the damage-number system. 0 seconds reads the Mythic Combat default.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void ShowRuneCallout(FText Text, FLinearColor Color, float LifetimeSeconds = 0.0f);

    // One line on the owner's HUD feed or banner, by Kind.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void RaiseRuneNotice(EMythicNoticeKind Kind, FText Text);

    // At the avatar. bOwnerOnly plays the cue on the owning client alone; otherwise everyone nearby hears it.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void PlayRuneCue(FGameplayTag CueTag, bool bOwnerOnly);

    // At Location. Both cue verbs refuse a repeat of the same tag inside RuneCueThrottleSeconds.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void PlayRuneCueAt(FGameplayTag CueTag, FVector Location, bool bOwnerOnly);

    // Holds or releases a replicated state tag on the owner (GAS.Immune.FallDamage, Rune.Rule.FelledIsKill). Every
    // tag still held when the rune leaves its socket is released with it.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void SetRuneStateTag(FGameplayTag Tag, bool bOn);

    // Starts a cooldown of Seconds, shortened by the owner's cooldown reduction, and shows it on the HUD. Survives
    // death and respawn with the ability instance.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    bool StartRuneCooldown(float Seconds);

    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    bool IsRuneOnCooldown() const;

    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    float GetRuneCooldownRemaining() const;

    // Centimetres of ground travel since the owner was last still, the unit OnRuneDistance reports in.
    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    float GetDistanceSinceStill() const;

    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    float GetSecondsStill() const;

    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    float GetFallDepthMetres() const;

    // Adds Metres to the odometer as if walked, so a threaded dash counts toward a distance rune.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void CreditRuneTravel(float Metres);

    // What the owner pays to cast Skill: its authored stamina costs at the skill's level after the owner's
    // stamina-cost reduction. 0 for a skill with no stamina cost.
    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    float GetSkillStaminaCost(UMythicGA_Skill *Skill) const;

    // Runs Callback on the next server tick, for work that must not run inside the callback that asked for it.
    // Dropped if the rune leaves its socket first.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void DeferToNextTick(FMythicRuneDeferredDelegate Callback);

    // Adds Delta to a Stat.* counter on the owner's ledger.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    void RecordRuneStat(FGameplayTag Stat, int32 Delta);

    /**
     * Mends every worn piece of gear that can wear out by Amount and returns how many were mended. A weapon counts:
     * the rune pays upkeep on whatever the player is carrying into the fight.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Rune")
    int32 RepairRuneGear(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    UMythicRuneDefinition *GetRuneDefinition() const { return Definition; }

    // The socket this rune sits in, or INDEX_NONE when it was granted outside a rune component.
    UFUNCTION(BlueprintPure, Category = "Mythic|Rune")
    int32 GetRuneSlotIndex() const;

    // Seconds after cooldown reduction: Reduction is clamped to [0, MaxReduction] before it takes its share.
    static float ScaleRuneCooldown(float Seconds, float CooldownReduction, float MaxReduction);

    // A cue tag played again inside the throttle is dropped.
    static bool ShouldThrottleRuneCue(double SecondsSinceLastPlay, float ThrottleSeconds);

protected:
    // Native word before the Blueprint's on each seam. A C++ rune overrides these; the base raises the event.
    virtual void NotifyRunePreDeath(const FGameplayEventData &Payload);
    virtual void NotifyRuneKill(const FGameplayEventData &Payload);
    virtual void NotifyRuneLanded(float ImpactSpeed, float FallDamage, bool bPrevented);
    virtual void NotifyRuneFallBegan();
    virtual void NotifyRuneFallDepth(float Metres);
    virtual void NotifyRuneStillBegan();
    virtual void NotifyRuneSkillDashStarted(UMythicGA_Skill *Skill, const FVector &StartLocation);
    virtual void NotifyRuneSkillDashEnded(UMythicGA_Skill *Skill, const FVector &EndLocation);
    virtual void NotifyRuneGuardEnded();
    virtual void NotifyRunePreLootRoll(APlayerController *CreditedTo);

private:
    void HandleRuneGameplayEvent(const FGameplayEventData *Payload, FGameplayTag EventTag);
    void HandleMovingTagChanged(const FGameplayTag Tag, int32 NewCount);
    void HandleCooldownEnded();
    void HandleFallDamageResolved(float ImpactSpeed, float Damage, bool bPrevented);
    void HandleStillBegan();
    void HandleFallDepthSampled(float Metres);
    void HandleAbilityActivated(UGameplayAbility *Ability);
    void HandleAbilityEnded(const FAbilityEndedData &Data);
    void HandleGuardShieldRemoved(const FGameplayEffectRemovalInfo &RemovalInfo);
    void HandlePreLootRoll(APlayerController *CreditedTo, FLootTierBonus &Bonus);

    UFUNCTION()
    void HandleMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

    void BindAvatar(AActor *Avatar);
    void UnbindAvatar();
    void BindLootManager();
    void UnbindLootManager();
    void UnbindAll(const FGameplayAbilityActorInfo *ActorInfo);

    // Null when the rune runs outside a game instance that owns loot, which is every client and most tests.
    UMythicLootManagerSubsystem *ResolveLootManager() const;

    // The bonus the running loot roll is holding, or null with a warning naming the verb that asked too late.
    FLootTierBonus *ResolvePendingLootBonus(const TCHAR *Verb);

    // Applies one of the Mythic Combat rune effects to the owner with SetByCaller.Generic = Magnitude and, when
    // DurationSeconds > 0, SetByCaller.Duration; HitTag lands on the effect context.
    FActiveGameplayEffectHandle ApplyRuneEffect(const TSoftClassPtr<UGameplayEffect> &EffectClass, float Magnitude, float DurationSeconds,
                                                FGameplayTag HitTag);
    void FinishRuneGuard(bool bShieldEffectAlreadyRemoved, bool bNotify);
    void ExecuteRuneCue(FGameplayTag CueTag, const FVector &Location, bool bOwnerOnly);
    float ReadCooldownReduction() const;

    bool IsRuneAuthority() const;
    UMythicRuneComponent *ResolveRuneComponent() const;
    UMythicLifeComponent *ResolveLifeComponent() const;
    AMythicPlayerController *ResolvePlayerController() const;
    UMythicStatLedgerComponent *ResolveStatLedger() const;

    UPROPERTY(Transient)
    TObjectPtr<UMythicRuneDefinition> Definition;

    TWeakObjectPtr<UMythicRuneComponent> RuneComponent;
    TWeakObjectPtr<AMythicCharacter> BoundCharacter;
    TWeakObjectPtr<UMythicLifeComponent> BoundLife;

    TMap<FGameplayTag, FDelegateHandle> BoundEvents;
    FDelegateHandle MovingTagHandle;
    FDelegateHandle AbilityActivatedHandle;
    FDelegateHandle AbilityEndedHandle;
    FDelegateHandle FallResolvedHandle;
    FDelegateHandle StillBeganHandle;
    FDelegateHandle FallDepthHandle;
    FDelegateHandle PreLootRollHandle;

    TWeakObjectPtr<UMythicLootManagerSubsystem> BoundLootManager;

    // Both live only for the length of one OnRunePreLootRoll: the bonus sits on the loot roll's own stack frame,
    // and the controller is whoever that credit belongs to.
    FLootTierBonus *PendingLootBonus = nullptr;
    APlayerController *PendingLootController = nullptr;

    FGameplayTagContainer HeldStateTags;

    // Server world seconds the cooldown ends; 0 when none is running.
    double CooldownEndTimeSeconds = 0.0;
    FTimerHandle CooldownTimer;

    FActiveGameplayEffectHandle GuardMaxShieldHandle;
    FActiveGameplayEffectHandle GuardShieldHandle;
    float PreGuardShield = 0.0f;

    // Server world seconds each cue tag last played, for the throttle.
    TMap<FGameplayTag, double> CueLastPlayedSeconds;

    bool bRuneBound = false;
    bool bInPreDeath = false;
    bool bGuardActive = false;
    bool bEndingGuard = false;
};
