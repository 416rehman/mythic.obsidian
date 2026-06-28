// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "MythicPlayerStatusViewModel.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

// Fired when health DECREASES. The view plays its delayed-damage / "chip" animation off this (no C++ tick / interp).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMythicHealthDamaged, float, Delta, float, NewPercent);

/**
 * Player status ViewModel (MVVM) — health / stamina / shield fractions + status flags for the player HUD.
 *
 * Driven 100% by EVENTS: GAS attribute-change delegates (GetGameplayAttributeValueChangeDelegate) and tag events
 * (RegisterGameplayTagEvent). NO Tick, no polling. The View (a Widget Blueprint) binds to these FieldNotify properties
 * via the UMG ViewModel binding panel and plays its own animations on OnHealthDamaged for the chip/delayed-damage.
 *
 * Usage: the WBP creates this VM (UMG ViewModel "Create Instance") and calls InitializeForASC on construct with the
 * local player's ASC (e.g. PlayerState/Pawn ASC). Unbinds itself on destroy (the player ASC lives on the persistent
 * PlayerState and is reused across pawns, so dangling delegates must be cleaned up).
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicPlayerStatusViewModel : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // Bind to the local player's ASC: seed current values, then subscribe to change events (event-driven, no tick).
    UFUNCTION(BlueprintCallable, Category = "Mythic|HUD")
    void InitializeForASC(UAbilitySystemComponent *InASC);

    // Health just dropped — (amount lost as a fraction 0..1, new health fraction). Drives the view's chip animation.
    UPROPERTY(BlueprintAssignable, Category = "Mythic|HUD")
    FMythicHealthDamaged OnHealthDamaged;

    // ── Bars (0..1 fractions) ──
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    float HealthPercent = 1.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    float StaminaPercent = 1.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    float ShieldPercent = 0.0f;

    // ── Status flags (b-prefixed -> explicit accessor names so the getters/setters drop the 'b') ──
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter = SetExhausted, Getter = GetExhausted, meta = (AllowPrivateAccess))
    bool bExhausted = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter = SetBurning, Getter = GetBurning, meta = (AllowPrivateAccess))
    bool bBurning = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter = SetBleeding, Getter = GetBleeding, meta = (AllowPrivateAccess))
    bool bBleeding = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter = SetPoisoned, Getter = GetPoisoned, meta = (AllowPrivateAccess))
    bool bPoisoned = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter = SetStunned, Getter = GetStunned, meta = (AllowPrivateAccess))
    bool bStunned = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter = SetSlowed, Getter = GetSlowed, meta = (AllowPrivateAccess))
    bool bSlowed = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter = SetFrozen, Getter = GetFrozen, meta = (AllowPrivateAccess))
    bool bFrozen = false;

    // Setters (broadcast field-notify) + trivial getters.
    void SetHealthPercent(float V);
    float GetHealthPercent() const { return HealthPercent; }
    void SetStaminaPercent(float V);
    float GetStaminaPercent() const { return StaminaPercent; }
    void SetShieldPercent(float V);
    float GetShieldPercent() const { return ShieldPercent; }
    void SetExhausted(bool V);
    bool GetExhausted() const { return bExhausted; }
    void SetBurning(bool V);
    bool GetBurning() const { return bBurning; }
    void SetBleeding(bool V);
    bool GetBleeding() const { return bBleeding; }
    void SetPoisoned(bool V);
    bool GetPoisoned() const { return bPoisoned; }
    void SetStunned(bool V);
    bool GetStunned() const { return bStunned; }
    void SetSlowed(bool V);
    bool GetSlowed() const { return bSlowed; }
    void SetFrozen(bool V);
    bool GetFrozen() const { return bFrozen; }

    virtual void BeginDestroy() override;

private:
    void HandleAttributeChanged(const FOnAttributeChangeData &Data);
    void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
    void Unbind();

    TWeakObjectPtr<UAbilitySystemComponent> ASC;

    // Cached raw values so a percent can be recomputed when either the current OR the max attribute changes.
    float CurHealth = 0.0f, MaxHealthV = 1.0f;
    float CurStamina = 0.0f, MaxStaminaV = 1.0f;
    float CurShield = 0.0f, MaxShieldV = 1.0f;
};
