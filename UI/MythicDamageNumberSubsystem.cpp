// Copyright Stellar Games. All Rights Reserved.

#include "MythicDamageNumberSubsystem.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythicDamageNumbers, Log, All);

void UMythicDamageNumberSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    // Reserve space for typical damage number count
    ActiveDamageNumbers.Reserve(256);

    // Load config from developer settings
    if (const UMythicDeveloperSettings *DevSettings = GetDefault<UMythicDeveloperSettings>()) {
        Config = DevSettings->DamageNumberConfig.LoadSynchronous();
        if (Config) {
            UE_LOG(LogMythicDamageNumbers, Log, TEXT("Loaded DamageNumberConfig: %s"), *Config->GetName());
        }
    }

    // Bind to HUD drawing - this delegate is called every frame for each local player
    HUDDrawDelegateHandle = AHUD::OnHUDPostRender.AddUObject(this, &UMythicDamageNumberSubsystem::OnHUDPostRender);

    UE_LOG(LogMythicDamageNumbers, Log, TEXT("DamageNumberSubsystem initialized"));
}

void UMythicDamageNumberSubsystem::Deinitialize() {
    AHUD::OnHUDPostRender.Remove(HUDDrawDelegateHandle);
    ActiveDamageNumbers.Empty();

    Super::Deinitialize();
}

bool UMythicDamageNumberSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    // Only create for game worlds, not editor preview worlds
    if (const UWorld *World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicDamageNumberSubsystem::OnHUDPostRender(AHUD *HUD, UCanvas *Canvas) {
    if (!HUD || !Canvas) {
        return;
    }

    // Only draw for HUDs in our world
    if (HUD->GetWorld() != GetWorld()) {
        return;
    }

    APlayerController *PC = HUD->GetOwningPlayerController();
    if (!PC) {
        return;
    }

    DrawDamageNumbers(Canvas, PC);
}

void UMythicDamageNumberSubsystem::AddDamageNumber(FVector WorldLocation, float Magnitude, const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) {
    FMythicDamageNumberData NewData;
    NewData.WorldLocation = WorldLocation;
    NewData.CachedText = FText::FromString(FormatMagnitude(Magnitude)); // Cache FText at creation, not per-frame
    NewData.SpawnTime = GetWorld()->GetTimeSeconds();
    NewData.Lifetime = Config ? Config->DefaultLifetime : 1.0f;
    NewData.ID = NextID++;

    // Determine damage type and corresponding color/animation
    NewData.DamageType = DetermineDamageType(EffectContext, bIsHeal);
    NewData.Color = GetColorForType(NewData.DamageType);
    NewData.AnimStyle = GetAnimStyleForType(NewData.DamageType);
    NewData.bIsCritical = (NewData.DamageType == EMythicDamageNumberType::Critical);

    // Add random offsets for visual variety
    if (Config) {
        NewData.RandomOffsetX = FMath::RandRange(-Config->RandomHorizontalOffsetRange, Config->RandomHorizontalOffsetRange);
        NewData.ExtraVerticalSpeed = FMath::RandRange(0.0f, Config->RandomVerticalSpeedRange);
    }

    ActiveDamageNumbers.Add(MoveTemp(NewData));

    UE_LOG(LogMythicDamageNumbers, Verbose, TEXT("Added damage number at %s (Type: %d)"), *WorldLocation.ToString(), (int32)NewData.DamageType);
}

void UMythicDamageNumberSubsystem::AddDamageNumberCustom(FVector WorldLocation, const FString &Text, FLinearColor Color, float Lifetime) {
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

    // Get camera info for world-to-screen projection
    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    // Get font - use Slate font which properly supports font size
    // FSlateFontInfo::GetFont() returns the UFont, but we use the full FSlateFontInfo for proper sizing
    UFont *Font = nullptr;
    if (Config && Config->Font.Get()) {
        Font = Config->Font.Get();
    }
    if (!Font) {
        Font = GEngine->GetSmallFont();
    }

    const float BaseScale = Config ? Config->FontScaleMultiplier : 1.0f;
    const bool bOutline = Config ? Config->bEnableOutline : true;
    const FLinearColor OutlineColor = Config ? Config->OutlineColor : FLinearColor::Black;
    const float CritScaleMultiplier = Config ? Config->CriticalHitScaleMultiplier : 1.3f;

    // Draw each active damage number
    for (int32 i = ActiveDamageNumbers.Num() - 1; i >= 0; --i) {
        FMythicDamageNumberData &Data = ActiveDamageNumbers[i];

        // Check if expired
        if (Data.IsExpired(CurrentTime)) {
            // Swap-remove for efficiency
            ActiveDamageNumbers.RemoveAtSwap(i, EAllowShrinking::No);
            continue;
        }

        // Project world location to screen
        FVector2D ScreenPos;
        if (!UGameplayStatics::ProjectWorldToScreen(PC, Data.WorldLocation, ScreenPos, true)) {
            // Off screen or behind camera, skip
            continue;
        }

        // Apply animation-style-specific offsets
        const FVector2D AnimOffset = CalculateAnimationOffset(Data, CurrentTime);
        ScreenPos.X += AnimOffset.X;
        ScreenPos.Y += AnimOffset.Y;

        // Calculate alpha for fade out
        const float Alpha = Data.GetAlpha(CurrentTime);

        // Calculate burst scale (starts big, settles to 1.0)
        const float BurstScale = Config ? Config->BurstScaleMultiplier : 1.5f;
        const float BurstDuration = Config ? Config->BurstDuration : 0.15f;
        const float BurstScaleFactor = Data.GetBurstScale(CurrentTime, BurstScale, BurstDuration);

        // Calculate animation-style-specific scale (e.g., Pulse)
        const float AnimScaleFactor = CalculateAnimationScale(Data, CurrentTime);

        // Calculate final scale: base * burst * anim * crit multiplier
        float FinalScale = BaseScale * BurstScaleFactor * AnimScaleFactor;
        if (Data.bIsCritical) {
            FinalScale *= CritScaleMultiplier;
        }

        // Create color with alpha - multiply base color alpha with animation alpha
        FLinearColor FinalColor = Data.Color;
        FinalColor.A *= Alpha; // Preserve configured alpha, apply fade animation on top

        // Draw the text - use cached FText to avoid per-frame allocation
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
    // Default colors if no config
    const FLinearColor DefaultColor = Config ? Config->DefaultColor : FLinearColor::White;
    const FLinearColor HealColor = Config ? Config->HealColor : FLinearColor(0.0f, 1.0f, 0.3f);

    if (bIsHeal) {
        return HealColor;
    }

    // Extract our custom effect context
    const FMythicGameplayEffectContext *MythicContext = nullptr;
    if (EffectContext.IsValid()) {
        MythicContext = static_cast<const FMythicGameplayEffectContext *>(EffectContext.Get());
    }

    if (!MythicContext) {
        return DefaultColor;
    }

    // Only check for critical hit
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

    // Only check for critical hit
    if (MythicContext->IsCriticalHit()) {
        return EMythicDamageNumberType::Critical;
    }

    return EMythicDamageNumberType::Default;
}

FLinearColor UMythicDamageNumberSubsystem::GetColorForType(EMythicDamageNumberType Type) const {
    if (!Config) {
        // Fallback colors when no config
        switch (Type) {
        case EMythicDamageNumberType::Critical:
            return FLinearColor::Yellow;
        case EMythicDamageNumberType::Heal:
            return FLinearColor(0.0f, 1.0f, 0.3f);
        default:
            return FLinearColor::White;
        }
    }

    switch (Type) {
    case EMythicDamageNumberType::Critical:
        return Config->CriticalHitColor;
    case EMythicDamageNumberType::Heal:
        return Config->HealColor;
    default:
        return Config->DefaultColor;
    }
}

EMythicDamageNumberAnimStyle UMythicDamageNumberSubsystem::GetAnimStyleForType(EMythicDamageNumberType Type) const {
    if (!Config) {
        // Sensible defaults when no config
        switch (Type) {
        case EMythicDamageNumberType::Critical:
            return EMythicDamageNumberAnimStyle::Bounce;
        case EMythicDamageNumberType::Heal:
            return EMythicDamageNumberAnimStyle::Pulse;
        default:
            return EMythicDamageNumberAnimStyle::FloatUp;
        }
    }

    switch (Type) {
    case EMythicDamageNumberType::Critical:
        return Config->CriticalAnimStyle;
    case EMythicDamageNumberType::Heal:
        return Config->HealAnimStyle;
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
        // Standard float up
        Offset.Y = -Age * (BaseVerticalSpeed + Data.ExtraVerticalSpeed);
        break;

    case EMythicDamageNumberAnimStyle::FloatUpSlow:
        // Slower, more graceful rise
        Offset.Y = -Age * (BaseVerticalSpeed * 0.5f + Data.ExtraVerticalSpeed);
        break;

    case EMythicDamageNumberAnimStyle::Bounce: {
        // Bouncy effect: quick up, slight down, settle up
        const float BouncePhase = FMath::Clamp(Age * 4.0f, 0.0f, 1.0f);
        const float BounceCurve = FMath::Sin(BouncePhase * PI) * 0.3f;
        Offset.Y = -Age * BaseVerticalSpeed - BounceCurve * 30.0f;
    }
    break;

    case EMythicDamageNumberAnimStyle::ArcLeft: {
        // Arc to the left while floating up
        Offset.Y = -Age * BaseVerticalSpeed;
        Offset.X += -FMath::Sin(NormalizedAge * PI) * 40.0f;
    }
    break;

    case EMythicDamageNumberAnimStyle::ArcRight: {
        // Arc to the right while floating up
        Offset.Y = -Age * BaseVerticalSpeed;
        Offset.X += FMath::Sin(NormalizedAge * PI) * 40.0f;
    }
    break;

    case EMythicDamageNumberAnimStyle::Shake: {
        // Small horizontal shake that dampens over time
        const float ShakeIntensity = 4.0f * (1.0f - NormalizedAge);
        const float ShakeFrequency = 20.0f;
        Offset.X += FMath::Sin(Age * ShakeFrequency) * ShakeIntensity;
        Offset.Y = -Age * BaseVerticalSpeed;
    }
    break;

    case EMythicDamageNumberAnimStyle::Pulse:
        // Float up normally, scale is handled separately
        Offset.Y = -Age * BaseVerticalSpeed;
        break;
    }

    return Offset;
}

float UMythicDamageNumberSubsystem::CalculateAnimationScale(const FMythicDamageNumberData &Data, float CurrentTime) const {
    if (Data.AnimStyle != EMythicDamageNumberAnimStyle::Pulse) {
        return 1.0f;
    }

    // Pulse animation: gentle scale oscillation
    const float Age = CurrentTime - Data.SpawnTime;
    const float NormalizedAge = FMath::Clamp(Age / Data.Lifetime, 0.0f, 1.0f);
    const float PulseFrequency = 6.0f;
    const float PulseAmount = 0.1f * (1.0f - NormalizedAge); // Dampen over time
    return 1.0f + FMath::Sin(Age * PulseFrequency) * PulseAmount;
}
