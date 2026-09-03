// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "MythicPlayerStatusViewModel.generated.h"

class UAbilitySystemComponent;
class UMythicStatusEffectDefinition;
class UTexture2D;
struct FOnAttributeChangeData;

/**
 * One status the game can inflict, with everything a badge needs to draw itself. The list holds EVERY authored
 * status, not just active ones, so the badge row can pool its widgets once at startup and toggle visibility
 * rather than adding and removing children.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatusBadgeEntry {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Status")
    FGameplayTag StatusType;

    // The tag the effect grants while active. Matching on this is how a tag change finds its badge.
    UPROPERTY(BlueprintReadOnly, Category = "Status")
    FGameplayTag GrantedStateTag;

    UPROPERTY(BlueprintReadOnly, Category = "Status")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Status")
    FText Description;

    UPROPERTY(BlueprintReadOnly, Category = "Status")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly, Category = "Status")
    FLinearColor DisplayColor = FLinearColor::White;

    // The status is on the player right now.
    UPROPERTY(BlueprintReadOnly, Category = "Status")
    bool bActive = false;

    // Progress toward the status landing, 0..1, so a badge can fill before the status hits.
    UPROPERTY(BlueprintReadOnly, Category = "Status")
    float BuildupFraction = 0.0f;
};

/** One worn rune with something to show: its icon and the state the server published for it. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicRuneBadgeEntry {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Rune")
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Rune")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly, Category = "Rune")
    FText Name;

    UPROPERTY(BlueprintReadOnly, Category = "Rune")
    EMythicRuneHudState State = EMythicRuneHudState::Hidden;

    // Local world seconds, so a view compares against the clock it already ticks with. End is 0 when nothing is timed.
    UPROPERTY(BlueprintReadOnly, Category = "Rune")
    double StartTimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Rune")
    double EndTimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category = "Rune")
    int32 Stacks = 0;
};

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

    /**
     * Every authored status, sorted by tag so the row never reshuffles between runs. Built from the status
     * registry, so a status added in data shows up with its icon, colour and text without any code change.
     */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter = GetStatusBadges, meta = (AllowPrivateAccess))
    TArray<FMythicStatusBadgeEntry> StatusBadges;

    const TArray<FMythicStatusBadgeEntry> &GetStatusBadges() const { return StatusBadges; }

    /**
     * Every worn rune the server has given a state other than Hidden, in socket order. Rebuilt only when the
     * sockets or their published states change, never per frame.
     */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter = GetRuneBadges, meta = (AllowPrivateAccess))
    TArray<FMythicRuneBadgeEntry> RuneBadges;

    const TArray<FMythicRuneBadgeEntry> &GetRuneBadges() const { return RuneBadges; }

    // InitializeForASC follows the rune component on the ASC's owner; call this when the sockets live elsewhere.
    UFUNCTION(BlueprintCallable, Category = "Mythic|HUD")
    void BindToRunes(UMythicRuneComponent *InRunes);

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

    virtual void BeginDestroy() override;

private:
    void HandleAttributeChanged(const FOnAttributeChangeData &Data);
    void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);

    void BuildStatusBadges(UAbilitySystemComponent *InASC);
    void HandleBadgeTagChanged(const FGameplayTag Tag, int32 NewCount);
    void HandleBadgeAttributeChanged(const FOnAttributeChangeData &Data);
    static float ComputeBuildupFraction(const UAbilitySystemComponent *InASC, const UMythicStatusEffectDefinition *Definition);

    TArray<FGameplayTag> BadgeBoundTags;
    TArray<FGameplayAttribute> BadgeBoundAttributes;
    void Unbind();

    UFUNCTION()
    void HandleRuneBadgesDirty();
    void RebuildRuneBadges();
    void UnbindRunes();

    TWeakObjectPtr<UAbilitySystemComponent> ASC;
    TWeakObjectPtr<UMythicRuneComponent> Runes;

    float CurHealth = 0.0f, MaxHealthV = 1.0f;
    float CurStamina = 0.0f, MaxStaminaV = 1.0f;
    float CurShield = 0.0f, MaxShieldV = 1.0f;
};
