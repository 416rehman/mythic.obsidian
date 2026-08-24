// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FieldNotificationId.h"
#include "MythicPlayerStatusWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UMythicPlayerStatusViewModel;

USTRUCT()
struct FMythicVitalBar {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UImage> Image;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> Material;

    float Target = 1.0f;
    float Chip = 1.0f;

    float HitFlash = 0.0f;
    float HealGlow = 0.0f;
};

UCLASS()
class MYTHIC_API UMythicPlayerStatusWidget : public UUserWidget {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Mythic|HUD")
    UMythicPlayerStatusViewModel *GetStatusViewModel() const { return ViewModel; }

    UFUNCTION(BlueprintCallable, Category = "Mythic|HUD")
    bool BindToLocalPlayer();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    void RefreshSalience();

    class UMythicHUDLayout *FindHUDLayout();

    /** Below this fraction a vital is worth interrupting for, not just glancing at. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UrgentVitalFraction = 0.35f;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> HealthBar;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> StaminaBar;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> ShieldBar;

    /** Numbers printed over the bars. A bar alone tells you a ratio; a player wants the actual figure. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<class UCommonTextBlock> Txt_Health;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<class UCommonTextBlock> Txt_Stamina;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<class UCommonTextBlock> Txt_Shield;

    /** Shield collapses entirely at zero — an empty bar reads as a broken one. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> ShieldGroup;

    // ── Bar colours (tunable rather than buried in code) ──
    // These go straight into a material parameter, so they are LINEAR. The old values were written as if they were
    // sRGB, which is why blood red painted as salmon and amber painted as milk: 0.86 linear leaves the shader at 0.94
    // sRGB. Every value below is the linear form of the colour actually wanted on screen.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Colours")
    FLinearColor HealthStart = FLinearColor(0.48f, 0.013f, 0.010f); // arterial red
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Colours")
    FLinearColor HealthEnd = FLinearColor(0.075f, 0.004f, 0.004f); // near-black at the bottom of the orb
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Colours")
    FLinearColor StaminaStart = FLinearColor(0.62f, 0.30f, 0.045f); // lamp amber
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Colours")
    FLinearColor StaminaEnd = FLinearColor(0.20f, 0.085f, 0.010f);
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Colours")
    FLinearColor ShieldStart = FLinearColor(0.34f, 0.58f, 0.88f);
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Colours")
    FLinearColor ShieldEnd = FLinearColor(0.06f, 0.15f, 0.30f);

    /**
     * Exhausted is a player state rather than an inflicted status, so it keeps its own pin. Every status badge is
     * found by name instead: a badge named Icon_<GrantedStateTag leaf> is shown whenever that status is active, so
     * adding a status is a data asset plus an image named to match, with no code change here.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Icon_Exhausted;

    /** How fast the delayed-damage chip catches up, in bar fractions per second. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "0.05"))
    float ChipDrainPerSecond = 0.55f;

    /** Beat before the chip starts draining, so the player sees the hit land. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "0.0"))
    float ChipHoldSeconds = 0.12f;

    /** How fast the hit flash fades. A hit should read instantly and be gone before the next one. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Orb", meta = (ClampMin = "0.1"))
    float HitFlashDecayPerSecond = 4.5f;

    /** How fast the heal glow fades. Slower than a hit — healing is a relief, not an alarm. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Orb", meta = (ClampMin = "0.1"))
    float HealGlowDecayPerSecond = 5.56f;

    /**
     * How big a jump upward counts as "healed". Stamina trickles back constantly; without a floor the orb would glow
     * and the state timer would never stop.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Orb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HealGlowMinJump = 0.06f;

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|HUD")
    void OnHealthDamaged(float Delta, float NewPercent);

private:
    void HandleFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId);
    void RefreshAll();
    void RetryBind();

    UFUNCTION()
    void HandleHealthDamaged(float Delta, float NewPercent);

    void InitBar(FMythicVitalBar &Bar, UImage *Image, const FLinearColor &Start, const FLinearColor &End, float Depth);
    void SetBarPercent(FMythicVitalBar &Bar, float Percent);

    void TickChip(float DeltaSeconds);
    void SetChipTicking(bool bEnabled);

    bool DecayStates(FMythicVitalBar &Bar, float DeltaSeconds);

    static void ApplyFlag(UWidget *Widget, bool bActive);

    void RefreshStatusBadges();

    UPROPERTY()
    TObjectPtr<UMythicPlayerStatusViewModel> ViewModel;

    UPROPERTY()
    FMythicVitalBar Health;
    UPROPERTY()
    FMythicVitalBar Stamina;
    UPROPERTY()
    FMythicVitalBar Shield;

    FTimerHandle BindRetryTimer;
    FTimerHandle ChipTimer;
    float ChipHoldRemaining = 0.0f;
    int32 BindAttempts = 0;

    UPROPERTY(Transient)
    TWeakObjectPtr<class UMythicHUDLayout> HUDLayout;
};
