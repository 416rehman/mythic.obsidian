// Copyright Stellar Games. All Rights Reserved.

#include "MythicPlayerStatusWidget.h"

#include "CommonTextBlock.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "TimerManager.h"
#include "UI/MythicHUDLayout.h"
#include "UI/MythicUIStyle.h"
#include "UI/ViewModels/MythicPlayerStatusViewModel.h"

namespace {
constexpr int32 MaxBindAttempts = 20;
constexpr float BindRetryInterval = 0.25f;
constexpr float ChipTickInterval = 1.0f / 30.0f;

const FName Vitals_Percent(TEXT("Percent"));
const FName Vitals_ChipPercent(TEXT("ChipPercent"));
const FName Vitals_FillStart(TEXT("FillColorStart"));
const FName Vitals_FillEnd(TEXT("FillColorEnd"));
const FName Vitals_HitFlash(TEXT("HitFlash"));
const FName Vitals_HealGlow(TEXT("HealGlow"));
const FName Vitals_Background(TEXT("BackgroundColor"));
const FName Vitals_ChipColor(TEXT("ChipColor"));
const FName Vitals_ChipAlpha(TEXT("ChipAlpha"));
const FName Vitals_HealFrom(TEXT("HealFrom"));
const FName Vitals_HealColor(TEXT("HealColor"));
const FName Vitals_LowPulse(TEXT("LowPulse"));
const FName Vitals_Pulse(TEXT("Pulse"));
const FName Vitals_PulseStart(TEXT("PulseStart"));

constexpr float StateEpsilon = 0.01f;

constexpr float FillDropSpeed = 16.0f;
constexpr float FillRiseSpeed = 14.0f;

constexpr float ChipEaseSpeed = 11.0f;
constexpr float ChipSnapEpsilon = 0.002f;

constexpr float HealHoldSeconds = 0.26f;

constexpr float LowHealthFraction = 0.25f;
constexpr float LowHealthClearFraction = 0.30f;

constexpr float FillEdgeValue = 0.42f;
constexpr float FillStartValue = 0.20f;
constexpr float ChipValue = 0.19f;
constexpr float TrackValue = 0.008f;
constexpr float HealthDepth = 1.0f;
constexpr float StaminaDepth = 0.60f;
constexpr float ShieldDepth = 0.80f;

constexpr float VitalsDimTint = 0.78f;


UMaterialInstanceDynamic *FindImageMID(UUserWidget *Owner, const FName &WidgetName) {
    if (!Owner) {
        return nullptr;
    }
    if (UImage *Img = Cast<UImage>(Owner->GetWidgetFromName(WidgetName))) {
        return Img->GetDynamicMaterial();
    }
    return nullptr;
}

void Vitals_EnsureCap(UUserWidget *Owner, const FName &GroupName, const FName &CapName, float SizePx, EHorizontalAlignment HAlign,
                      float Tint) {
    if (!Owner || Owner->GetWidgetFromName(CapName)) {
        return;
    }
    UOverlay *Group = Cast<UOverlay>(Owner->GetWidgetFromName(GroupName));
    if (!Group || !Owner->WidgetTree) {
        return;
    }
    static const FSoftObjectPath GemPath(TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_Gem_Terminal.MI_UI_Gem_Terminal"));
    UMaterialInterface *Gem = Cast<UMaterialInterface>(GemPath.TryLoad());
    if (!Gem) {
        return;
    }
    UImage *Cap = Owner->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), CapName);
    FSlateBrush Brush;
    Brush.SetResourceObject(Gem);
    Brush.ImageSize = FVector2D(SizePx, SizePx);
    Cap->SetBrush(Brush);
    Cap->SetColorAndOpacity(FLinearColor(Tint, Tint, Tint, 1.0f));
    Cap->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UOverlaySlot *Slot = Group->AddChildToOverlay(Cap)) {
        Slot->SetHorizontalAlignment(HAlign);
        Slot->SetVerticalAlignment(VAlign_Center);
        const float Half = -SizePx * 0.5f;
        Slot->SetPadding(HAlign == HAlign_Left ? FMargin(Half, 0.0f, 0.0f, 0.0f) : FMargin(0.0f, 0.0f, Half, 0.0f));
    }
}

void Vitals_PlaceReadout(UCommonTextBlock *Text, float InsetPx) {
    if (!Text) {
        return;
    }
    if (UOverlaySlot *Slot = Cast<UOverlaySlot>(Text->Slot)) {
        Slot->SetHorizontalAlignment(HAlign_Right);
        Slot->SetVerticalAlignment(VAlign_Center);
        Slot->SetPadding(FMargin(0.0f, 0.0f, InsetPx, 1.0f));
    }
}
}

void UMythicPlayerStatusWidget::NativeConstruct() {
    Super::NativeConstruct();

    Vitals_EnsureCap(this, TEXT("HealthBarGroup"), TEXT("Terminal_Health_R"), 14.0f, HAlign_Right, 0.80f);
    Vitals_EnsureCap(this, TEXT("StaminaBarGroup"), TEXT("Terminal_Stamina"), 12.0f, HAlign_Left, 0.70f);
    if (StaminaBar) {
        StaminaBar->SetDesiredSizeOverride(FVector2D(430.0f, 6.0f));
    }
    if (UImage *StaminaFrame = Cast<UImage>(GetWidgetFromName(TEXT("Frame_StaminaBar")))) {
        StaminaFrame->SetDesiredSizeOverride(FVector2D(430.0f, 12.0f));
    }
    Vitals_PlaceReadout(Txt_Health, 10.0f);
    if (Txt_Health) {
        Txt_Health->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().Ink));
    }

    InitBar(Health, HealthBar, HealthStart, HealthEnd, HealthDepth);
    InitBar(Stamina, StaminaBar, StaminaStart, StaminaEnd, StaminaDepth);
    InitBar(Shield, ShieldBar, ShieldStart, ShieldEnd, ShieldDepth);

    if (Health.Material) {
        Health.Material->SetScalarParameterValue(Vitals_ChipAlpha, 1.0f);
        Health.Material->SetScalarParameterValue(Vitals_HealFrom, 1.0f);
        Health.Material->SetScalarParameterValue(Vitals_LowPulse, 0.0f);
    }
    if (UMaterialInstanceDynamic *Plate = FindImageMID(this, TEXT("VitalsPlate"))) {
        Plate->SetScalarParameterValue(Vitals_HitFlash, 0.0f);
    }
    if (UMaterialInstanceDynamic *Gem = FindImageMID(this, TEXT("Terminal_Health"))) {
        Gem->SetScalarParameterValue(Vitals_Pulse, 0.0f);
    }
    for (const TCHAR *Cap : {TEXT("Terminal_Health_R"), TEXT("Terminal_Stamina")}) {
        if (UMaterialInstanceDynamic *Gem = FindImageMID(this, Cap)) {
            Gem->SetScalarParameterValue(Vitals_Pulse, 0.0f);
        }
    }

    if (UMythicHUDLayout *Layout = FindHUDLayout()) {
        Layout->RegisterHUDElement(this, EMythicHUDSalience::Hidden);
        Layout->SetElementDimTint(this, VitalsDimTint);
    }

    if (!BindToLocalPlayer()) {
        RetryBind();
    }
}

void UMythicPlayerStatusWidget::NativeDestruct() {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(BindRetryTimer);
        World->GetTimerManager().ClearTimer(ChipTimer);
    }
    if (ViewModel) {
        ViewModel->RemoveAllFieldValueChangedDelegates(this);
        ViewModel->OnHealthDamaged.RemoveAll(this);
    }
    if (UMythicHUDLayout *Layout = HUDLayout.Get()) {
        Layout->UnregisterHUDElement(this);
    }
    Super::NativeDestruct();
}

void UMythicPlayerStatusWidget::InitBar(FMythicVitalBar &Bar, UImage *Image, const FLinearColor &Start, const FLinearColor &End,
                                        float Depth) {
    Bar.Image = Image;
    if (!Image) {
        return;
    }
    Image->SetVisibility(ESlateVisibility::HitTestInvisible);

    Bar.Material = Image->GetDynamicMaterial();
    if (!Bar.Material) {
        return;
    }
    const FLinearColor Base = Start.LinearRGBToHSV();
    const float Hue = Base.R;
    const float Sat = FMath::Clamp(Base.G, 0.55f, 1.0f);
    const float D = FMath::Clamp(Depth, 0.2f, 1.0f);
    auto Shade = [Hue](float S, float V, float A) {
        FLinearColor C = FLinearColor(Hue, S, V).HSVToLinearRGB();
        C.A = A;
        return C;
    };

    Bar.Material->SetVectorParameterValue(Vitals_Background, Shade(Sat * 0.90f, TrackValue, 0.85f));
    Bar.Material->SetVectorParameterValue(Vitals_ChipColor, Shade(Sat * 0.90f, ChipValue * D, 1.0f));
    Bar.Material->SetVectorParameterValue(Vitals_FillStart, Shade(Sat, FillStartValue * D, 1.0f));
    Bar.Material->SetVectorParameterValue(Vitals_FillEnd, Shade(Sat, FillEdgeValue * D, 1.0f));
    Bar.Material->SetVectorParameterValue(Vitals_HealColor, Shade(Sat * 0.72f, FMath::Min(0.72f * D + 0.1f, 0.9f), 1.0f));
    Bar.Material->SetScalarParameterValue(Vitals_Percent, 1.0f);
    Bar.Material->SetScalarParameterValue(Vitals_ChipPercent, 1.0f);
    Bar.Material->SetScalarParameterValue(Vitals_HitFlash, 0.0f);
    Bar.Material->SetScalarParameterValue(Vitals_HealGlow, 0.0f);
}

void UMythicPlayerStatusWidget::SetBarPercent(FMythicVitalBar &Bar, float Percent) {
    const float Previous = Bar.Target;
    Bar.Target = FMath::Clamp(Percent, 0.0f, 1.0f);
    if (!Bar.Material) {
        return;
    }

    if (&Bar == &Health && Bar.Target - Previous >= HealGlowMinJump) {
        float Shown = Previous;
        Bar.Material->GetScalarParameterValue(FMaterialParameterInfo(Vitals_Percent), Shown);
        float From = FMath::Clamp(Shown, 0.0f, 1.0f);
        if (Bar.HealGlow > 0.0f) {
            float Existing = From;
            Bar.Material->GetScalarParameterValue(FMaterialParameterInfo(Vitals_HealFrom), Existing);
            From = FMath::Min(From, Existing);
        }
        Bar.Material->SetScalarParameterValue(Vitals_HealFrom, From);
        Bar.HealGlow = 1.0f + HealHoldSeconds * HealGlowDecayPerSecond;
        Bar.Material->SetScalarParameterValue(Vitals_HealGlow, Bar.HealGlow);
        SetChipTicking(true);
    }
    SetChipTicking(true);
    if (Bar.Chip < Bar.Target) {
        Bar.Chip = Bar.Target;
        Bar.Material->SetScalarParameterValue(Vitals_ChipPercent, Bar.Chip);
    }
}

void UMythicPlayerStatusWidget::RetryBind() {
    const UWorld *World = GetWorld();
    if (!World || ++BindAttempts > MaxBindAttempts) {
        return;
    }
    World->GetTimerManager().SetTimer(BindRetryTimer, FTimerDelegate::CreateWeakLambda(this, [this]() {
        if (!BindToLocalPlayer()) {
            RetryBind();
        }
    }), BindRetryInterval, false);
}

bool UMythicPlayerStatusWidget::BindToLocalPlayer() {
    APlayerController *PC = GetOwningPlayer();
    if (!PC) {
        return false;
    }

    UAbilitySystemComponent *ASC = nullptr;
    if (APlayerState *PS = PC->PlayerState) {
        ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS);
    }
    if (!ASC) {
        if (APawn *Pawn = PC->GetPawn()) {
            ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
        }
    }
    if (!ASC) {
        return false;
    }

    if (!ViewModel) {
        ViewModel = NewObject<UMythicPlayerStatusViewModel>(this);
    }
    else {
        ViewModel->RemoveAllFieldValueChangedDelegates(this);
        ViewModel->OnHealthDamaged.RemoveAll(this);
    }

    ViewModel->InitializeForASC(ASC);

    const INotifyFieldValueChanged::FFieldValueChangedDelegate Delegate =
        INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UMythicPlayerStatusWidget::HandleFieldChanged);

    using FDesc = UMythicPlayerStatusViewModel::FFieldNotificationClassDescriptor;
    ViewModel->AddFieldValueChangedDelegate(FDesc::HealthPercent, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::StaminaPercent, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::ShieldPercent, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::bInCombat, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::bExhausted, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::bBurning, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::bBleeding, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::bPoisoned, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::bStunned, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::bSlowed, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::bFrozen, Delegate);

    ViewModel->OnHealthDamaged.AddDynamic(this, &UMythicPlayerStatusWidget::HandleHealthDamaged);

    Health.Chip = ViewModel->GetHealthPercent();
    RefreshAll();
    return true;
}

void UMythicPlayerStatusWidget::HandleFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId) {
    RefreshAll();
}

void UMythicPlayerStatusWidget::HandleHealthDamaged(float Delta, float NewPercent) {
    ChipHoldRemaining = ChipHoldSeconds;
    Health.HitFlash = 1.0f;
    Health.HealGlow = 0.0f;
    if (Health.Material) {
        Health.Material->SetScalarParameterValue(Vitals_HitFlash, 1.0f);
        Health.Material->SetScalarParameterValue(Vitals_HealGlow, 0.0f);
    }
    if (UMaterialInstanceDynamic *Plate = FindImageMID(this, TEXT("VitalsPlate"))) {
        Plate->SetScalarParameterValue(Vitals_HitFlash, 1.0f);
    }
    SetChipTicking(true);
    OnHealthDamaged(Delta, NewPercent);
}

bool UMythicPlayerStatusWidget::DecayStates(FMythicVitalBar &Bar, float DeltaSeconds) {
    if (!Bar.Material) {
        return false;
    }

    bool bAlive = false;

    {
        float Shown = Bar.Target;
        static const FName PercentName = Vitals_Percent;
        if (Bar.Material->GetScalarParameterValue(FMaterialParameterInfo(PercentName), Shown)) {
            const float Delta = Bar.Target - Shown;
            if (FMath::Abs(Delta) > StateEpsilon * 0.1f) {
                const float Speed = (Delta < 0.0f) ? FillDropSpeed : FillRiseSpeed;
                const float Next = FMath::FInterpTo(Shown, Bar.Target, DeltaSeconds, Speed);
                Bar.Material->SetScalarParameterValue(Vitals_Percent, Next);
                bAlive = true;
            }
            else if (!FMath::IsNearlyEqual(Shown, Bar.Target)) {
                Bar.Material->SetScalarParameterValue(Vitals_Percent, Bar.Target);
            }
        }
    }

    if (Bar.HitFlash > 0.0f) {
        Bar.HitFlash = FMath::Max(0.0f, Bar.HitFlash - HitFlashDecayPerSecond * DeltaSeconds);
        Bar.Material->SetScalarParameterValue(Vitals_HitFlash, Bar.HitFlash);
        if (&Bar == &Health) {
            if (UMaterialInstanceDynamic *Plate = FindImageMID(this, TEXT("VitalsPlate"))) {
                Plate->SetScalarParameterValue(Vitals_HitFlash, Bar.HitFlash);
            }
        }
        bAlive |= Bar.HitFlash > StateEpsilon;
    }
    if (Bar.HealGlow > 0.0f) {
        Bar.HealGlow = FMath::Max(0.0f, Bar.HealGlow - HealGlowDecayPerSecond * DeltaSeconds);
        Bar.Material->SetScalarParameterValue(Vitals_HealGlow, Bar.HealGlow);
        bAlive |= Bar.HealGlow > StateEpsilon;
    }
    return bAlive;
}

void UMythicPlayerStatusWidget::SetChipTicking(bool bEnabled) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (bEnabled) {
        if (!World->GetTimerManager().IsTimerActive(ChipTimer)) {
            World->GetTimerManager().SetTimer(ChipTimer, FTimerDelegate::CreateWeakLambda(this, [this]() {
                TickChip(ChipTickInterval);
            }), ChipTickInterval, true);
        }
        for (const FMythicVitalBar *Bar : {&Health, &Stamina, &Shield}) {
            if (Bar->Image) {
                Bar->Image->ForceVolatile(true);
            }
        }
    }
    else {
        World->GetTimerManager().ClearTimer(ChipTimer);
        for (const FMythicVitalBar *Bar : {&Health, &Stamina, &Shield}) {
            if (Bar->Image) {
                Bar->Image->ForceVolatile(false);
            }
        }
    }
}

void UMythicPlayerStatusWidget::TickChip(float DeltaSeconds) {
    bool bStatesAlive = DecayStates(Health, DeltaSeconds);
    bStatesAlive |= DecayStates(Stamina, DeltaSeconds);
    bStatesAlive |= DecayStates(Shield, DeltaSeconds);

    const bool bHoldingHealth = ChipHoldRemaining > 0.0f;
    if (bHoldingHealth) {
        ChipHoldRemaining -= DeltaSeconds;
    }

    bool bChipAlive = false;
    auto DrainChip = [&](FMythicVitalBar &Bar, bool bHold, float Rate) {
        if (!Bar.Material || bHold) {
            return;
        }
        if (Bar.Chip <= Bar.Target + KINDA_SMALL_NUMBER) {
            if (!FMath::IsNearlyEqual(Bar.Chip, Bar.Target)) {
                Bar.Chip = Bar.Target;
                Bar.Material->SetScalarParameterValue(Vitals_ChipPercent, Bar.Chip);
            }
            return;
        }
        Bar.Chip = FMath::Max(Bar.Target, Bar.Chip - Rate * DeltaSeconds);
        Bar.Material->SetScalarParameterValue(Vitals_ChipPercent, Bar.Chip);
        bChipAlive = true;
    };

    if (Health.Material && !bHoldingHealth && Health.Chip > Health.Target + KINDA_SMALL_NUMBER) {
        const float Next = FMath::FInterpTo(Health.Chip, Health.Target, DeltaSeconds, ChipEaseSpeed);
        Health.Chip = (Next - Health.Target <= ChipSnapEpsilon) ? Health.Target : Next;
        Health.Material->SetScalarParameterValue(Vitals_ChipPercent, Health.Chip);
        bChipAlive |= Health.Chip > Health.Target;
    }
    else {
        DrainChip(Health, bHoldingHealth, ChipDrainPerSecond);
    }
    DrainChip(Stamina, false, ChipDrainPerSecond * 6.0f);
    DrainChip(Shield, false, ChipDrainPerSecond * 6.0f);

    if (!bChipAlive && !bStatesAlive && !bHoldingHealth) {
        SetChipTicking(false);
    }
}

void UMythicPlayerStatusWidget::RefreshAll() {
    if (!ViewModel) {
        return;
    }

    const float HealthPct = ViewModel->GetHealthPercent();
    SetBarPercent(Health, HealthPct);
    SetBarPercent(Stamina, ViewModel->GetStaminaPercent());

    if (UMaterialInstanceDynamic *Gem = FindImageMID(this, TEXT("Terminal_Health"))) {
        float Pulsing = 0.0f;
        Gem->GetScalarParameterValue(FMaterialParameterInfo(Vitals_Pulse), Pulsing);
        if (Pulsing < 0.5f && HealthPct < LowHealthFraction) {
            Gem->SetScalarParameterValue(Vitals_PulseStart, static_cast<float>(FApp::GetCurrentTime() - GStartTime));
            Gem->SetScalarParameterValue(Vitals_Pulse, 1.0f);
            if (Health.Material) {
                Health.Material->SetScalarParameterValue(Vitals_LowPulse, 1.0f);
            }
        }
        else if (Pulsing >= 0.5f && HealthPct >= LowHealthClearFraction) {
            Gem->SetScalarParameterValue(Vitals_Pulse, 0.0f);
            if (Health.Material) {
                Health.Material->SetScalarParameterValue(Vitals_LowPulse, 0.0f);
            }
        }
    }

    const float ShieldPct = ViewModel->GetShieldPercent();
    SetBarPercent(Shield, ShieldPct);
    const ESlateVisibility ShieldVis = ShieldPct > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
    if (ShieldGroup) {
        ShieldGroup->SetVisibility(ShieldVis);
    }
    else if (Shield.Image) {
        Shield.Image->SetVisibility(ShieldVis);
    }

    const auto Readout = [](UCommonTextBlock *Text, float Current, float Max) {
        if (!Text) {
            return;
        }
        Text->SetText(FText::AsNumber(FMath::RoundToInt(Current)));
    };
    if (Txt_Health) {
        Txt_Health->SetText(FText::Format(NSLOCTEXT("Mythic", "HealthReadout", "{0} / {1}"),
                                          FText::AsNumber(FMath::RoundToInt(ViewModel->GetCurrentHealth())),
                                          FText::AsNumber(FMath::RoundToInt(ViewModel->GetMaxHealth()))));
    }
    Readout(Txt_Stamina, ViewModel->GetCurrentStamina(), ViewModel->GetMaxStamina());
    Readout(Txt_Shield, ViewModel->GetCurrentShield(), ViewModel->GetMaxShield());

    ApplyFlag(Icon_Exhausted, ViewModel->GetExhausted());
    ApplyFlag(Icon_Burning, ViewModel->GetBurning());
    ApplyFlag(Icon_Bleeding, ViewModel->GetBleeding());
    ApplyFlag(Icon_Poisoned, ViewModel->GetPoisoned());
    ApplyFlag(Icon_Stunned, ViewModel->GetStunned());
    ApplyFlag(Icon_Slowed, ViewModel->GetSlowed());
    ApplyFlag(Icon_Frozen, ViewModel->GetFrozen());

    RefreshSalience();
}

void UMythicPlayerStatusWidget::ApplyFlag(UWidget *Widget, bool bActive) {
    if (Widget) {
        Widget->SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

UMythicHUDLayout *UMythicPlayerStatusWidget::FindHUDLayout() {
    if (UMythicHUDLayout *Cached = HUDLayout.Get()) {
        return Cached;
    }
    if (UMythicHUDLayout *Outer = GetTypedOuter<UMythicHUDLayout>()) {
        HUDLayout = Outer;
        return Outer;
    }
    if (const APlayerController *PC = GetOwningPlayer()) {
        for (TObjectIterator<UMythicHUDLayout> It; It; ++It) {
            UMythicHUDLayout *Layout = *It;
            if (IsValid(Layout) && !Layout->HasAnyFlags(RF_ClassDefaultObject) && Layout->GetOwningPlayer() == PC) {
                HUDLayout = Layout;
                return Layout;
            }
        }
    }
    return nullptr;
}

void UMythicPlayerStatusWidget::RefreshSalience() {
    UMythicHUDLayout *Layout = FindHUDLayout();
    if (!Layout || !ViewModel) {
        return;
    }

    const float LowestVital = FMath::Min(ViewModel->GetHealthPercent(), ViewModel->GetStaminaPercent());
    const bool bAfflicted = ViewModel->GetExhausted() || ViewModel->GetBurning() || ViewModel->GetBleeding()
                            || ViewModel->GetPoisoned() || ViewModel->GetStunned() || ViewModel->GetSlowed()
                            || ViewModel->GetFrozen();

    EMythicHUDSalience Want = EMythicHUDSalience::Hidden;
    if (ViewModel->GetInCombat() || LowestVital < UrgentVitalFraction || bAfflicted) {
        Want = EMythicHUDSalience::Lit;
    }
    else if (LowestVital < 0.999f || ViewModel->GetShieldPercent() > 0.001f) {
        Want = EMythicHUDSalience::Dim;
    }

    Layout->SetElementSalience(this, Want);
}
