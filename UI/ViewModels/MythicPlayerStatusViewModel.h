// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "MythicPlayerStatusViewModel.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMythicHealthDamaged, float, Delta, float, NewPercent);

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
    /**
     * The player is in a fight (GAS.State.InCombat). Drives the contextual HUD: the vitals are shown while this is
     * true and stand down when it clears. Server-applied on any landed hit and replicated, so it is the same answer
     * the fast-travel and mount-summon blocks use — the HUD cannot disagree with the rules.
     */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter = SetInCombat, Getter = GetInCombat, meta = (AllowPrivateAccess))
    bool bInCombat = false;

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

    void SetHealthPercent(float V);
    float GetHealthPercent() const { return HealthPercent; }
    void SetStaminaPercent(float V);
    float GetStaminaPercent() const { return StaminaPercent; }
    void SetShieldPercent(float V);
    float GetShieldPercent() const { return ShieldPercent; }

    // Raw values, so the HUD can print "132 / 500" instead of only drawing a fraction. Deliberately plain getters
    // rather than more FieldNotify fields: they change on exactly the same beat as the percents, so anything already
    // listening to a percent can read these in the same handler.
    UFUNCTION(BlueprintPure, Category = "Vitals")
    float GetCurrentHealth() const { return CurHealth; }
    UFUNCTION(BlueprintPure, Category = "Vitals")
    float GetMaxHealth() const { return MaxHealthV; }
    UFUNCTION(BlueprintPure, Category = "Vitals")
    float GetCurrentStamina() const { return CurStamina; }
    UFUNCTION(BlueprintPure, Category = "Vitals")
    float GetMaxStamina() const { return MaxStaminaV; }
    UFUNCTION(BlueprintPure, Category = "Vitals")
    float GetCurrentShield() const { return CurShield; }
    UFUNCTION(BlueprintPure, Category = "Vitals")
    float GetMaxShield() const { return MaxShieldV; }
    void SetInCombat(bool V);
    bool GetInCombat() const { return bInCombat; }

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

    float CurHealth = 0.0f, MaxHealthV = 1.0f;
    float CurStamina = 0.0f, MaxStaminaV = 1.0f;
    float CurShield = 0.0f, MaxShieldV = 1.0f;
};
