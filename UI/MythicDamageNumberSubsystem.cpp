// Copyright Stellar Games. All Rights Reserved.

#include "MythicDamageNumberSubsystem.h"
#include "UI/Settings/MythicUserSettings.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythicDamageNumbers, Log, All);

void UMythicDamageNumberSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    ActiveDamageNumbers.Reserve(256);

    if (const UMythicDeveloperSettings *DevSettings = GetDefault<UMythicDeveloperSettings>()) {
        Config = DevSettings->DamageNumberConfig.LoadSynchronous();
        if (Config) {
            UE_LOG(LogMythicDamageNumbers, Log, TEXT("Loaded DamageNumberConfig: %s"), *Config->GetName());
        }
    }

    HUDDrawDelegateHandle = AHUD::OnHUDPostRender.AddUObject(this, &UMythicDamageNumberSubsystem::OnHUDPostRender);

    UE_LOG(LogMythicDamageNumbers, Log, TEXT("DamageNumberSubsystem initialized"));
}

void UMythicDamageNumberSubsystem::Deinitialize() {
    AHUD::OnHUDPostRender.Remove(HUDDrawDelegateHandle);
    ActiveDamageNumbers.Empty();

    Super::Deinitialize();
}

bool UMythicDamageNumberSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    if (const UWorld *World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicDamageNumberSubsystem::OnHUDPostRender(AHUD *HUD, UCanvas *Canvas) {
    if (!HUD || !Canvas) {
        return;
    }

    if (HUD->GetWorld() != GetWorld()) {
        return;
    }

    APlayerController *PC = HUD->GetOwningPlayerController();
    if (!PC) {
        return;
    }

    DrawDamageNumbers(Canvas, PC);
}

void UMythicDamageNumberSubsystem::CleanupExpired() {
    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    for (int32 i = ActiveDamageNumbers.Num() - 1; i >= 0; --i) {
        if (ActiveDamageNumbers[i].IsExpired(CurrentTime)) {
            ActiveDamageNumbers.RemoveAtSwap(i, EAllowShrinking::No);
        }
    }
}

void UMythicDamageNumberSubsystem::AddDamageNumber(FVector WorldLocation, float Magnitude, const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) {
    // The accessibility settings own whether numbers show at all. Mode 0 = off; scale rides the font multiplier
    // where the numbers are drawn. Read per-add: it is a few loads, and the settings screen applies instantly.
    if (const UMythicUserSettings *UserSettings = UMythicUserSettings::Get()) {
        if (UserSettings->GetDamageNumberMode() == 0) {
            return;
        }
    }
    CleanupExpired();
    FMythicDamageNumberData NewData;
    NewData.WorldLocation = WorldLocation;
    NewData.CachedText = FText::FromString(FormatMagnitude(Magnitude));
    NewData.SpawnTime = GetWorld()->GetTimeSeconds();
    NewData.Lifetime = Config ? Config->DefaultLifetime : 1.0f;
    NewData.ID = NextID++;

    NewData.DamageType = DetermineDamageType(EffectContext, bIsHeal);
    NewData.Color = GetColorForType(NewData.DamageType);
    NewData.AnimStyle = GetAnimStyleForType(NewData.DamageType);
    NewData.bIsCritical = (NewData.DamageType == EMythicDamageNumberType::Critical);

    if (Config) {
        NewData.RandomOffsetX = FMath::RandRange(-Config->RandomHorizontalOffsetRange, Config->RandomHorizontalOffsetRange);
        NewData.ExtraVerticalSpeed = FMath::RandRange(0.0f, Config->RandomVerticalSpeedRange);
    }

    ActiveDamageNumbers.Add(MoveTemp(NewData));

    UE_LOG(LogMythicDamageNumbers, Verbose, TEXT("Added damage number at %s (Type: %d)"), *WorldLocation.ToString(), (int32)NewData.DamageType);
}

void UMythicDamageNumberSubsystem::AddDodgeNumber(FVector WorldLocation) {
    const FLinearColor DodgeColor = Config ? Config->DodgeColor : FLinearColor::Gray;
    AddCombatText(WorldLocation, TEXT("DODGE"), DodgeColor, 1.0f);
}

void UMythicDamageNumberSubsystem::AddCombatText(FVector WorldLocation, const FString &Text, FLinearColor Color, float Lifetime) {
    CleanupExpired();
    FMythicDamageNumberData NewData;
    NewData.WorldLocation = WorldLocation;
    NewData.CachedText = FText::FromString(Text);
    NewData.Color = Color;
    NewData.SpawnTime = GetWorld()->GetTimeSeconds();
    NewData.Lifetime = Lifetime;
    NewData.ID = NextID++;
    NewData.bIsCritical = false;

    if (Config) {
        NewData.RandomOffsetX = FMath::RandRange(-Config->RandomHorizontalOffsetRange, Config->RandomHorizontalOffsetRange);
        NewData.ExtraVerticalSpeed = FMath::RandRange(0.0f, Config->RandomVerticalSpeedRange);
    }

    ActiveDamageNumbers.Add(MoveTemp(NewData));
}

void UMythicDamageNumberSubsystem::SetConfig(UMythicDamageNumberConfig *NewConfig) {
    Config = NewConfig;
}

void UMythicDamageNumberSubsystem::ClearAll() {
    ActiveDamageNumbers.Empty();
}

void UMythicDamageNumberSubsystem::DrawDamageNumbers(UCanvas *Canvas, APlayerController *PC) {
    if (ActiveDamageNumbers.Num() == 0) {
        return;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    UFont *Font = nullptr;
    if (Config && Config->Font.Get()) {
        Font = Config->Font.Get();
    }
    if (!Font) {
        Font = GEngine->GetSmallFont();
    }

    float BaseScale = Config ? Config->FontScaleMultiplier : 1.0f;
    if (const UMythicUserSettings *UserSettings = UMythicUserSettings::Get()) {
        BaseScale *= UserSettings->GetDamageNumberScale();
    }
    const bool bOutline = Config ? Config->bEnableOutline : true;
    const FLinearColor OutlineColor = Config ? Config->OutlineColor : FLinearColor::Black;
    const float CritScaleMultiplier = Config ? Config->CriticalHitScaleMultiplier : 1.3f;

    for (int32 i = ActiveDamageNumbers.Num() - 1; i >= 0; --i) {
        FMythicDamageNumberData &Data = ActiveDamageNumbers[i];

        if (Data.IsExpired(CurrentTime)) {
            ActiveDamageNumbers.RemoveAtSwap(i, EAllowShrinking::No);
            continue;
        }

        FVector2D ScreenPos;
        if (!UGameplayStatics::ProjectWorldToScreen(PC, Data.WorldLocation, ScreenPos, true)) {
            continue;
        }

        const FVector2D AnimOffset = CalculateAnimationOffset(Data, CurrentTime);
        ScreenPos.X += AnimOffset.X;
        ScreenPos.Y += AnimOffset.Y;

        const float Alpha = Data.GetAlpha(CurrentTime);

        const float BurstScale = Config ? Config->BurstScaleMultiplier : 1.5f;
        const float BurstDuration = Config ? Config->BurstDuration : 0.15f;
        const float BurstScaleFactor = Data.GetBurstScale(CurrentTime, BurstScale, BurstDuration);

        const float AnimScaleFactor = CalculateAnimationScale(Data, CurrentTime);

        float FinalScale = BaseScale * BurstScaleFactor * AnimScaleFactor;
        if (Data.bIsCritical) {
            FinalScale *= CritScaleMultiplier;
        }

        FLinearColor FinalColor = Data.Color;
        FinalColor.A *= Alpha;

        FCanvasTextItem TextItem(
            FVector2D(ScreenPos.X, ScreenPos.Y),
            Data.CachedText,
            Font,
            FinalColor
            );
        TextItem.Scale = FVector2D(FinalScale, FinalScale);
        TextItem.bCentreX = true;
        TextItem.bCentreY = true;
        TextItem.bOutlined = bOutline;
        TextItem.OutlineColor = OutlineColor;

        Canvas->DrawItem(TextItem);
    }
}

FString UMythicDamageNumberSubsystem::FormatMagnitude(float Magnitude) const {
    const float AbsMagnitude = FMath::Abs(Magnitude);

    if (Config && Config->bAbbreviateLargeNumbers) {
        if (AbsMagnitude >= Config->MillionThreshold) {
            return FString::Printf(TEXT("%.1fM"), AbsMagnitude / 1000000.0f);
        }
        else if (AbsMagnitude >= Config->ThousandThreshold) {
            return FString::Printf(TEXT("%.1fK"), AbsMagnitude / 1000.0f);
        }
    }

    return FString::Printf(TEXT("%d"), FMath::RoundToInt(AbsMagnitude));
}

FLinearColor UMythicDamageNumberSubsystem::DetermineColor(const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) const {
    const FLinearColor DefaultColor = Config ? Config->DefaultColor : FLinearColor::White;
    const FLinearColor HealColor = Config ? Config->HealColor : FLinearColor(0.0f, 1.0f, 0.3f);

    if (bIsHeal) {
        return HealColor;
    }

    const FMythicGameplayEffectContext *MythicContext = nullptr;
    if (EffectContext.IsValid()) {
        MythicContext = static_cast<const FMythicGameplayEffectContext *>(EffectContext.Get());
    }

    if (!MythicContext) {
        return DefaultColor;
    }

    if (MythicContext->IsCriticalHit()) {
        return Config ? Config->CriticalHitColor : FLinearColor::Yellow;
    }

    return DefaultColor;
}

bool UMythicDamageNumberSubsystem::IsCriticalHit(const FGameplayEffectContextHandle &EffectContext) const {
    if (EffectContext.IsValid()) {
        const FMythicGameplayEffectContext *MythicContext = static_cast<const FMythicGameplayEffectContext *>(EffectContext.Get());
        if (MythicContext) {
            return MythicContext->IsCriticalHit();
        }
    }
    return false;
}

EMythicDamageNumberType UMythicDamageNumberSubsystem::DetermineDamageType(const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) const {
    if (bIsHeal) {
        return EMythicDamageNumberType::Heal;
    }

    if (!EffectContext.IsValid()) {
        return EMythicDamageNumberType::Default;
    }

    const FMythicGameplayEffectContext *MythicContext = static_cast<const FMythicGameplayEffectContext *>(EffectContext.Get());
    if (!MythicContext) {
        return EMythicDamageNumberType::Default;
    }

    if (MythicContext->IsCriticalHit()) { return EMythicDamageNumberType::Critical; }
    if (MythicContext->IsDodged()) { return EMythicDamageNumberType::Dodge; }
    if (MythicContext->IsBurn()) { return EMythicDamageNumberType::Burn; }
    if (MythicContext->IsPoison()) { return EMythicDamageNumberType::Poison; }
    if (MythicContext->IsBleed()) { return EMythicDamageNumberType::Bleed; }
    if (MythicContext->IsFreeze()) { return EMythicDamageNumberType::Freeze; }
    if (MythicContext->IsStun()) { return EMythicDamageNumberType::Stun; }
    if (MythicContext->IsTerrify()) { return EMythicDamageNumberType::Terrify; }
    if (MythicContext->IsWeaken()) { return EMythicDamageNumberType::Weaken; }
    if (MythicContext->IsSlow()) { return EMythicDamageNumberType::Slow; }

    return EMythicDamageNumberType::Default;
}

FLinearColor UMythicDamageNumberSubsystem::GetColorForType(EMythicDamageNumberType Type) const {
    if (!Config) {
        switch (Type) {
        case EMythicDamageNumberType::Critical:
            return FLinearColor::Yellow;
        case EMythicDamageNumberType::Heal:
            return FLinearColor(0.0f, 1.0f, 0.3f);
        case EMythicDamageNumberType::Bleed:
            return FLinearColor(0.7f, 0.0f, 0.0f);
        case EMythicDamageNumberType::Burn:
            return FLinearColor(1.0f, 0.45f, 0.0f);
        case EMythicDamageNumberType::Poison:
            return FLinearColor(0.4f, 0.85f, 0.1f);
        case EMythicDamageNumberType::Stun:
            return FLinearColor(1.0f, 0.9f, 0.4f);
        case EMythicDamageNumberType::Slow:
            return FLinearColor(0.4f, 0.6f, 0.9f);
        case EMythicDamageNumberType::Weaken:
            return FLinearColor(0.6f, 0.45f, 0.7f);
        case EMythicDamageNumberType::Freeze:
            return FLinearColor(0.5f, 0.9f, 1.0f);
        case EMythicDamageNumberType::Terrify:
            return FLinearColor(0.7f, 0.2f, 0.7f);
        case EMythicDamageNumberType::Dodge:
            return FLinearColor(0.8f, 0.8f, 0.85f);
        default:
            return FLinearColor::White;
        }
    }

    switch (Type) {
    case EMythicDamageNumberType::Critical:
        return Config->CriticalHitColor;
    case EMythicDamageNumberType::Heal:
        return Config->HealColor;
    case EMythicDamageNumberType::Bleed:
        return Config->BleedColor;
    case EMythicDamageNumberType::Burn:
        return Config->BurnColor;
    case EMythicDamageNumberType::Poison:
        return Config->PoisonColor;
    case EMythicDamageNumberType::Stun:
        return Config->StunColor;
    case EMythicDamageNumberType::Slow:
        return Config->SlowColor;
    case EMythicDamageNumberType::Weaken:
        return Config->WeakenColor;
    case EMythicDamageNumberType::Freeze:
        return Config->FreezeColor;
    case EMythicDamageNumberType::Terrify:
        return Config->TerrifyColor;
    case EMythicDamageNumberType::Dodge:
        return Config->DodgeColor;
    default:
        return Config->DefaultColor;
    }
}

EMythicDamageNumberAnimStyle UMythicDamageNumberSubsystem::GetAnimStyleForType(EMythicDamageNumberType Type) const {
    if (!Config) {
        switch (Type) {
        case EMythicDamageNumberType::Critical:
            return EMythicDamageNumberAnimStyle::Bounce;
        case EMythicDamageNumberType::Heal:
            return EMythicDamageNumberAnimStyle::Pulse;
        case EMythicDamageNumberType::Dodge:
            return EMythicDamageNumberAnimStyle::FloatUpSlow;
        case EMythicDamageNumberType::Bleed:
        case EMythicDamageNumberType::Burn:
        case EMythicDamageNumberType::Poison:
        case EMythicDamageNumberType::Stun:
        case EMythicDamageNumberType::Slow:
        case EMythicDamageNumberType::Weaken:
        case EMythicDamageNumberType::Freeze:
        case EMythicDamageNumberType::Terrify:
            return EMythicDamageNumberAnimStyle::Shake;
        default:
            return EMythicDamageNumberAnimStyle::FloatUp;
        }
    }

    switch (Type) {
    case EMythicDamageNumberType::Critical:
        return Config->CriticalAnimStyle;
    case EMythicDamageNumberType::Heal:
        return Config->HealAnimStyle;
    case EMythicDamageNumberType::Dodge:
        return Config->DodgeAnimStyle;
    case EMythicDamageNumberType::Bleed:
    case EMythicDamageNumberType::Burn:
    case EMythicDamageNumberType::Poison:
    case EMythicDamageNumberType::Stun:
    case EMythicDamageNumberType::Slow:
    case EMythicDamageNumberType::Weaken:
    case EMythicDamageNumberType::Freeze:
    case EMythicDamageNumberType::Terrify:
        return Config->StatusAnimStyle;
    default:
        return Config->DefaultAnimStyle;
    }
}

FVector2D UMythicDamageNumberSubsystem::CalculateAnimationOffset(const FMythicDamageNumberData &Data, float CurrentTime) const {
    const float Age = CurrentTime - Data.SpawnTime;
    const float NormalizedAge = FMath::Clamp(Age / Data.Lifetime, 0.0f, 1.0f);
    const float BaseVerticalSpeed = Config ? Config->VerticalFloatSpeed : 50.0f;

    FVector2D Offset(Data.RandomOffsetX, 0.0f);

    switch (Data.AnimStyle) {
    case EMythicDamageNumberAnimStyle::FloatUp:
        Offset.Y = -Age * (BaseVerticalSpeed + Data.ExtraVerticalSpeed);
        break;

    case EMythicDamageNumberAnimStyle::FloatUpSlow:
        Offset.Y = -Age * (BaseVerticalSpeed * 0.5f + Data.ExtraVerticalSpeed);
        break;

    case EMythicDamageNumberAnimStyle::Bounce: {
        const float BouncePhase = FMath::Clamp(Age * 4.0f, 0.0f, 1.0f);
        const float BounceCurve = FMath::Sin(BouncePhase * PI) * 0.3f;
        Offset.Y = -Age * BaseVerticalSpeed - BounceCurve * 30.0f;
    }
    break;

    case EMythicDamageNumberAnimStyle::ArcLeft: {
        Offset.Y = -Age * BaseVerticalSpeed;
        Offset.X += -FMath::Sin(NormalizedAge * PI) * 40.0f;
    }
    break;

    case EMythicDamageNumberAnimStyle::ArcRight: {
        Offset.Y = -Age * BaseVerticalSpeed;
        Offset.X += FMath::Sin(NormalizedAge * PI) * 40.0f;
    }
    break;

    case EMythicDamageNumberAnimStyle::Shake: {
        const float ShakeIntensity = 4.0f * (1.0f - NormalizedAge);
        const float ShakeFrequency = 20.0f;
        Offset.X += FMath::Sin(Age * ShakeFrequency) * ShakeIntensity;
        Offset.Y = -Age * BaseVerticalSpeed;
    }
    break;

    case EMythicDamageNumberAnimStyle::Pulse:
        Offset.Y = -Age * BaseVerticalSpeed;
        break;
    }

    return Offset;
}

float UMythicDamageNumberSubsystem::CalculateAnimationScale(const FMythicDamageNumberData &Data, float CurrentTime) const {
    if (Data.AnimStyle != EMythicDamageNumberAnimStyle::Pulse) {
        return 1.0f;
    }

    const float Age = CurrentTime - Data.SpawnTime;
    const float NormalizedAge = FMath::Clamp(Age / Data.Lifetime, 0.0f, 1.0f);
    const float PulseFrequency = 6.0f;
    const float PulseAmount = 0.1f * (1.0f - NormalizedAge);
    return 1.0f + FMath::Sin(Age * PulseFrequency) * PulseAmount;
}
