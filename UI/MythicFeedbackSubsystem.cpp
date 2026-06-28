// Copyright Stellar Games. All Rights Reserved.

#include "MythicFeedbackSubsystem.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h" // TActorIterator (nameplate candidate sweep)
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/MythicTags_GAS.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "Engine/GameInstance.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "Player/MythicPlayerController.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Objectives/ObjectiveTracker.h"
#include "InputCoreTypes.h" // EKeys for the hold-to-reveal contextual layer

DEFINE_LOG_CATEGORY_STATIC(LogMythicDamageNumbers, Log, All);

void UMythicFeedbackSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
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
    HUDDrawDelegateHandle = AHUD::OnHUDPostRender.AddUObject(this, &UMythicFeedbackSubsystem::OnHUDPostRender);

    UE_LOG(LogMythicDamageNumbers, Log, TEXT("DamageNumberSubsystem initialized"));
}

void UMythicFeedbackSubsystem::Deinitialize() {
    AHUD::OnHUDPostRender.Remove(HUDDrawDelegateHandle);
    ActiveDamageNumbers.Empty();

    Super::Deinitialize();
}

bool UMythicFeedbackSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    // Only create for game worlds, not editor preview worlds
    if (const UWorld *World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicFeedbackSubsystem::OnHUDPostRender(AHUD *HUD, UCanvas *Canvas) {
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

    // COMBAT damage numbers stay on Canvas (transient world VFX — the right tool, and never the ugly part). EVERYTHING
    // else (player HUD, nameplates, toasts/banners, world callouts, quest tracker, ambient) is migrating to UMG/MVVM
    // widgets driven by ViewModels + gameplay events, so it is no longer drawn here. The other Draw* methods + their
    // pools/state are retired as each element's UMG widget lands.
    DrawDamageNumbers(Canvas, PC);
}

// Swap-remove every expired entry. Declared in the header but previously never defined — the ONLY pruning was inside
// DrawDamageNumbers, which the engine stops calling when the HUD is hidden (AHUD::PostRender gates on bShowHUD), so the
// pool grew unbounded while hidden. Called on every Add (the sole growth source) so the array stays bounded regardless
// of whether the HUD is currently rendering.
void UMythicFeedbackSubsystem::CleanupExpired() {
    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    for (int32 i = ActiveDamageNumbers.Num() - 1; i >= 0; --i) {
        if (ActiveDamageNumbers[i].IsExpired(CurrentTime)) {
            ActiveDamageNumbers.RemoveAtSwap(i, EAllowShrinking::No);
        }
    }
    // Same backward-RemoveAtSwap pruning for the screen notifications (toasts/banners share the pool-bounding contract).
    for (int32 i = ActiveNotifications.Num() - 1; i >= 0; --i) {
        if (ActiveNotifications[i].IsExpired(CurrentTime)) {
            ActiveNotifications.RemoveAtSwap(i, EAllowShrinking::No);
        }
    }
    // ...and the world-anchored non-combat callouts.
    for (int32 i = ActiveWorldCallouts.Num() - 1; i >= 0; --i) {
        if (ActiveWorldCallouts[i].IsExpired(CurrentTime)) {
            ActiveWorldCallouts.RemoveAtSwap(i, EAllowShrinking::No);
        }
    }
}

void UMythicFeedbackSubsystem::AddDamageNumber(FVector WorldLocation, float Magnitude, const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) {
    CleanupExpired(); // bound the pool even when the HUD isn't rendering
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

void UMythicFeedbackSubsystem::AddDodgeNumber(FVector WorldLocation) {
    // Reuse the configured DodgeColor (designer-tunable data asset) — single source, no duplicated literal; the named
    // FLinearColor::Gray is only a degenerate fallback when no config asset is set. "DODGE" is the conventional miss label.
    const FLinearColor DodgeColor = Config ? Config->DodgeColor : FLinearColor::Gray;
    AddCombatText(WorldLocation, TEXT("DODGE"), DodgeColor, 1.0f);
}

void UMythicFeedbackSubsystem::AddCombatText(FVector WorldLocation, const FString &Text, FLinearColor Color, float Lifetime) {
    CleanupExpired(); // bound the pool even when the HUD isn't rendering (AddDodgeNumber routes through here too)
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

void UMythicFeedbackSubsystem::SetConfig(UMythicDamageNumberConfig *NewConfig) {
    Config = NewConfig;
}

void UMythicFeedbackSubsystem::ClearAll() {
    ActiveDamageNumbers.Empty();
    ActiveNotifications.Empty();
    ActiveWorldCallouts.Empty();
}

void UMythicFeedbackSubsystem::AddWorldCallout(FVector WorldLocation, const FText &Text, FLinearColor Color, UTexture2D *Icon, float DurationOverride) {
    CleanupExpired();

    FMythicWorldCallout C;
    C.WorldLocation = WorldLocation;
    C.CachedText = Text;
    C.Icon = Icon;
    C.Color = Color;
    C.SpawnTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    C.Lifetime = DurationOverride > 0.0f ? DurationOverride : 2.0f;
    C.ID = NextID++;

    ActiveWorldCallouts.Add(MoveTemp(C));
}

void UMythicFeedbackSubsystem::DrawWorldCallouts(UCanvas *Canvas, APlayerController *PC) {
    if (ActiveWorldCallouts.Num() == 0) {
        return;
    }
    const float CurrentTime = GetWorld()->GetTimeSeconds();

    UFont *Font = (Config && Config->Font) ? Config->Font.Get() : nullptr;
    if (!Font) {
        Font = GEngine->GetMediumFont();
    }
    if (!Font) {
        return;
    }

    const float FontScale = Config ? Config->FontScaleMultiplier : 1.0f;
    const float FadeIn = 0.15f;
    const float FadeOut = 0.5f;
    const float RiseSpeed = 36.0f; // world units/sec the callout drifts up off its anchor
    const float IconSize = 22.0f;
    const float IconGap = 4.0f;
    const bool bOutline = Config ? Config->bEnableOutline : true;
    const FLinearColor OutlineColor = Config ? Config->OutlineColor : FLinearColor::Black;

    for (int32 i = ActiveWorldCallouts.Num() - 1; i >= 0; --i) {
        FMythicWorldCallout &C = ActiveWorldCallouts[i];
        if (C.IsExpired(CurrentTime)) {
            ActiveWorldCallouts.RemoveAtSwap(i, EAllowShrinking::No);
            continue;
        }
        const float Elapsed = CurrentTime - C.SpawnTime;
        const float Rise = FMath::Max(0.0f, Elapsed) * RiseSpeed;
        const FVector RisenWorld = C.WorldLocation + FVector(0.0f, 0.0f, Rise);

        FVector2D ScreenPos;
        if (!UGameplayStatics::ProjectWorldToScreen(PC, RisenWorld, ScreenPos, true)) {
            continue; // off-screen / behind camera
        }
        const float Alpha = ComputeToastAlpha(Elapsed, C.Lifetime, FadeIn, FadeOut);

        float TextW = 0.0f, TextH = 0.0f;
        Canvas->TextSize(Font, C.CachedText.ToString(), TextW, TextH, FontScale, FontScale);

        const bool bHasIcon = (C.Icon != nullptr);
        const float IconW = bHasIcon ? IconSize : 0.0f;
        const float Gap = bHasIcon ? IconGap : 0.0f;
        const float TotalW = IconW + Gap + TextW;
        const float TotalH = FMath::Max(bHasIcon ? IconSize : 0.0f, TextH);
        const float OriginX = ScreenPos.X - TotalW * 0.5f;
        const float OriginY = ScreenPos.Y - TotalH * 0.5f;

        if (bHasIcon && C.Icon->GetResource()) {
            FLinearColor IconColor = C.Color;
            IconColor.A *= Alpha;
            FCanvasTileItem IconItem(
                FVector2D(OriginX, OriginY + (TotalH - IconSize) * 0.5f),
                C.Icon->GetResource(),
                FVector2D(IconSize, IconSize),
                IconColor);
            IconItem.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(IconItem);
        }

        FLinearColor TextColor = C.Color;
        TextColor.A *= Alpha;
        FCanvasTextItem TextItem(
            FVector2D(OriginX + IconW + Gap, OriginY + (TotalH - TextH) * 0.5f),
            C.CachedText,
            Font,
            TextColor);
        TextItem.Scale = FVector2D(FontScale, FontScale);
        TextItem.bOutlined = bOutline;
        TextItem.OutlineColor = OutlineColor;
        Canvas->DrawItem(TextItem);
    }
}

void UMythicFeedbackSubsystem::AddScreenToast(const FText &Text, FLinearColor Color, UTexture2D *Icon, float DurationOverride) {
    CleanupExpired(); // bound both pools even when the HUD isn't rendering

    FMythicScreenNotification N;
    N.CachedText = Text;
    N.Icon = Icon;
    N.Color = Color;
    N.SpawnTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    N.Lifetime = DurationOverride > 0.0f ? DurationOverride : 3.0f; // toasts linger a touch longer than combat numbers
    N.Kind = EMythicScreenNotifyKind::Toast;
    N.ID = NextID++;

    ActiveNotifications.Add(MoveTemp(N));
}

// ─── Pure presentation math for screen notifications (mirrors the world-callout fade; unit-tested Mythic.UI.Feedback) ───

float UMythicFeedbackSubsystem::ComputeToastAlpha(float Elapsed, float Lifetime, float FadeInTime, float FadeOutTime) {
    if (Lifetime <= 0.0f) {
        return 0.0f;
    }
    Elapsed = FMath::Clamp(Elapsed, 0.0f, Lifetime);

    float A = 1.0f;
    if (FadeInTime > 0.0f && Elapsed < FadeInTime) {
        A = Elapsed / FadeInTime; // ramp 0 -> 1
    }
    const float FadeOutStart = Lifetime - FadeOutTime;
    if (FadeOutTime > 0.0f && Elapsed > FadeOutStart) {
        A = FMath::Min(A, (Lifetime - Elapsed) / FadeOutTime); // ramp 1 -> 0
    }
    return FMath::Clamp(A, 0.0f, 1.0f);
}

float UMythicFeedbackSubsystem::ComputeToastSlideOffset(float Elapsed, float FadeInTime, float SlideDistance) {
    if (FadeInTime <= 0.0f || Elapsed >= FadeInTime) {
        return 0.0f; // settled
    }
    if (Elapsed <= 0.0f) {
        return SlideDistance; // fully offset at spawn
    }
    const float T = Elapsed / FadeInTime;                    // 0..1
    const float EaseOut = 1.0f - FMath::Pow(1.0f - T, 3.0f); // cubic ease-out
    return SlideDistance * (1.0f - EaseOut);                 // SlideDistance -> 0
}

float UMythicFeedbackSubsystem::ComputeToastStackOffset(int32 SlotFromAnchor, float EntryStep) {
    return FMath::Max(0, SlotFromAnchor) * EntryStep;
}

void UMythicFeedbackSubsystem::AddScreenBanner(const FText &Title, const FText &Subtitle, FLinearColor AccentColor, UTexture2D *Icon, float DurationOverride) {
    CleanupExpired();

    FMythicScreenNotification N;
    N.CachedText = Title;
    N.Subtitle = Subtitle;
    N.Icon = Icon;
    N.Color = AccentColor;
    N.SpawnTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    N.Lifetime = DurationOverride > 0.0f ? DurationOverride : 3.5f; // major beats hold a touch longer than toasts
    N.Kind = EMythicScreenNotifyKind::Banner;
    N.ID = NextID++;

    ActiveNotifications.Add(MoveTemp(N));
}

float UMythicFeedbackSubsystem::EaseOutBack(float T) {
    // Standard ease-out-back: settles to 1.0 with a small overshoot past it near the end (the "pop").
    const float C1 = 1.70158f;
    const float C3 = C1 + 1.0f;
    const float T1 = T - 1.0f;
    return 1.0f + C3 * T1 * T1 * T1 + C1 * T1 * T1;
}

float UMythicFeedbackSubsystem::ComputeBannerScale(float Elapsed, float EntranceTime, float StartScale) {
    if (EntranceTime <= 0.0f || Elapsed >= EntranceTime) {
        return 1.0f;
    }
    if (Elapsed <= 0.0f) {
        return StartScale;
    }
    const float E = EaseOutBack(Elapsed / EntranceTime); // 0..~1 with a late overshoot
    return FMath::Lerp(StartScale, 1.0f, E);
}

float UMythicFeedbackSubsystem::ComputeBannerSweepX(float Elapsed, float EntranceTime, float Width) {
    if (EntranceTime <= 0.0f) {
        return Width;
    }
    const float T = FMath::Clamp(Elapsed / EntranceTime, 0.0f, 1.0f);
    return T * Width;
}

void UMythicFeedbackSubsystem::DrawScreenNotifications(UCanvas *Canvas, APlayerController *PC) {
    if (ActiveNotifications.Num() == 0) {
        return;
    }
    const float CurrentTime = GetWorld()->GetTimeSeconds();

    UFont *Font = (Config && Config->Font) ? Config->Font.Get() : nullptr;
    if (!Font) {
        Font = GEngine->GetMediumFont();
    }
    if (!Font) {
        return;
    }

    // Code-default toast styling (no asset needed — works before any config .uasset exists). Bottom-left anchor; newest
    // toast sits nearest the bottom, older ones stack upward. Banners (Kind == Banner) are drawn by a later slice.
    const float FontScale = Config ? Config->FontScaleMultiplier : 1.0f;
    const float FadeIn = 0.2f;
    const float FadeOut = 0.4f;
    const float SlideDist = 60.0f; // slides in from the left edge
    const float MarginX = 48.0f;
    const float MarginY = 96.0f;
    const float IconSize = 24.0f;
    const float IconGap = 6.0f;
    const float RowPad = 6.0f; // inner background padding
    const float RowGap = 8.0f; // vertical gap between toasts
    const FLinearColor BgTint(0.0f, 0.0f, 0.0f, 0.55f);
    const bool bOutline = Config ? Config->bEnableOutline : true;
    const FLinearColor OutlineColor = Config ? Config->OutlineColor : FLinearColor::Black;

    const float AnchorX = MarginX;
    const float AnchorBottomY = Canvas->ClipY - MarginY;

    // Uniform row height/step so the stack aligns even when text heights differ (width still varies per entry).
    float ProbeW = 0.0f, ProbeH = 0.0f;
    Canvas->TextSize(Font, TEXT("Ag"), ProbeW, ProbeH, FontScale, FontScale);
    const float RowH = FMath::Max(IconSize, ProbeH);
    const float Step = RowH + RowPad * 2.0f + RowGap;

    // Collect the live toast entries in insertion order (oldest first). Newest gets stack slot 0 (nearest the bottom).
    TArray<int32, TInlineAllocator<16>> ToastIdx;
    for (int32 i = 0; i < ActiveNotifications.Num(); ++i) {
        const FMythicScreenNotification &N = ActiveNotifications[i];
        if (N.Kind == EMythicScreenNotifyKind::Toast && !N.IsExpired(CurrentTime)) {
            ToastIdx.Add(i);
        }
    }
    const int32 NumToasts = ToastIdx.Num();

    for (int32 Order = 0; Order < NumToasts; ++Order) {
        const FMythicScreenNotification &N = ActiveNotifications[ToastIdx[Order]];
        const float Elapsed = CurrentTime - N.SpawnTime;
        const float Alpha = ComputeToastAlpha(Elapsed, N.Lifetime, FadeIn, FadeOut);
        const float Slide = ComputeToastSlideOffset(Elapsed, FadeIn, SlideDist);

        float TextW = 0.0f, TextH = 0.0f;
        Canvas->TextSize(Font, N.CachedText.ToString(), TextW, TextH, FontScale, FontScale);

        const bool bHasIcon = (N.Icon != nullptr);
        const float IconW = bHasIcon ? IconSize : 0.0f;
        const float Gap = bHasIcon ? IconGap : 0.0f;
        const float RowW = IconW + Gap + TextW;

        const int32 SlotFromBottom = (NumToasts - 1) - Order; // newest -> 0
        const float StackUp = ComputeToastStackOffset(SlotFromBottom, Step);
        const float RowTopY = AnchorBottomY - StackUp - (RowH + RowPad * 2.0f);
        const float RowLeftX = AnchorX - Slide; // slides in from the left

        // Background panel.
        if (BgTint.A > 0.0f) {
            FLinearColor Bg = BgTint;
            Bg.A *= Alpha;
            FCanvasTileItem BgItem(
                FVector2D(RowLeftX - RowPad, RowTopY),
                GWhiteTexture,
                FVector2D(RowW + RowPad * 2.0f, RowH + RowPad * 2.0f),
                Bg);
            BgItem.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(BgItem);
        }

        // Icon (left), vertically centered in the row.
        if (bHasIcon && N.Icon->GetResource()) {
            FLinearColor IconColor = N.Color;
            IconColor.A *= Alpha;
            FCanvasTileItem IconItem(
                FVector2D(RowLeftX, RowTopY + RowPad + (RowH - IconSize) * 0.5f),
                N.Icon->GetResource(),
                FVector2D(IconSize, IconSize),
                IconColor);
            IconItem.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(IconItem);
        }

        // Label (right of the icon), vertically centered.
        FLinearColor TextColor = N.Color;
        TextColor.A *= Alpha;
        FCanvasTextItem TextItem(
            FVector2D(RowLeftX + IconW + Gap, RowTopY + RowPad + (RowH - TextH) * 0.5f),
            N.CachedText,
            Font,
            TextColor);
        TextItem.Scale = FVector2D(FontScale, FontScale);
        TextItem.bOutlined = bOutline;
        TextItem.OutlineColor = OutlineColor;
        Canvas->DrawItem(TextItem);
    }

    // ── Hero banners (major beats) — center-screen, procedurally animated. Drawn over the toasts. ──
    const float CX = Canvas->ClipX * 0.5f;
    const float BannerBaseY = Canvas->ClipY * 0.28f; // upper third
    const float BEntrance = 0.35f;
    const float BFadeIn = 0.2f;
    const float BFadeOut = 0.5f;
    const float TitleScaleBase = FontScale * 1.9f;
    const float SubScaleBase = FontScale * 1.0f;
    int32 BannerSlot = 0;
    for (int32 i = 0; i < ActiveNotifications.Num(); ++i) {
        const FMythicScreenNotification &N = ActiveNotifications[i];
        if (N.Kind != EMythicScreenNotifyKind::Banner || N.IsExpired(CurrentTime)) {
            continue;
        }
        const float Elapsed = CurrentTime - N.SpawnTime;
        const float Alpha = ComputeToastAlpha(Elapsed, N.Lifetime, BFadeIn, BFadeOut);
        const float PopScale = ComputeBannerScale(Elapsed, BEntrance, 0.85f);
        const float Drop = ComputeToastSlideOffset(Elapsed, BEntrance, 40.0f); // slides down into place
        const float TitleScale = TitleScaleBase * PopScale;

        float TW = 0.0f, TH = 0.0f;
        Canvas->TextSize(Font, N.CachedText.ToString(), TW, TH, TitleScale, TitleScale);
        const bool bSub = !N.Subtitle.IsEmpty();
        float SW = 0.0f, SH = 0.0f;
        if (bSub) {
            Canvas->TextSize(Font, N.Subtitle.ToString(), SW, SH, SubScaleBase, SubScaleBase);
        }
        const float ContentW = FMath::Max(TW, SW);
        const float ContentH = TH + (bSub ? SH + 4.0f : 0.0f);
        const float PadX = 28.0f, PadY = 14.0f;
        const float BarW = ContentW + PadX * 2.0f;
        const float BarH = ContentH + PadY * 2.0f;
        const float BarX = CX - BarW * 0.5f;
        const float BarY = (BannerBaseY - Drop) + BannerSlot * (BarH + 12.0f);

        // Backing panel.
        FLinearColor Bg(0.04f, 0.05f, 0.07f, 0.82f * Alpha);
        FCanvasTileItem BgItem(FVector2D(BarX, BarY), GWhiteTexture, FVector2D(BarW, BarH), Bg);
        BgItem.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(BgItem);

        // Left accent strip (category color).
        FLinearColor Accent = N.Color;
        Accent.A *= Alpha;
        FCanvasTileItem AccItem(FVector2D(BarX, BarY), GWhiteTexture, FVector2D(4.0f, BarH), Accent);
        AccItem.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(AccItem);

        // Entrance specular sweep (only while entering).
        if (Elapsed < BEntrance) {
            const float SweepX = ComputeBannerSweepX(Elapsed, BEntrance, BarW);
            const float SweepW = 26.0f;
            FLinearColor Sweep(1.0f, 1.0f, 1.0f, 0.10f * Alpha);
            FCanvasTileItem SwItem(FVector2D(BarX + SweepX - SweepW * 0.5f, BarY), GWhiteTexture, FVector2D(SweepW, BarH), Sweep);
            SwItem.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(SwItem);
        }

        // Title (centered) with a drop shadow for legibility over any scene.
        const float TitleX = CX - TW * 0.5f;
        const float TitleY = BarY + PadY;
        FCanvasTextItem Shadow(FVector2D(TitleX + 2.0f, TitleY + 2.0f), N.CachedText, Font, FLinearColor(0.0f, 0.0f, 0.0f, 0.6f * Alpha));
        Shadow.Scale = FVector2D(TitleScale, TitleScale);
        Canvas->DrawItem(Shadow);
        FLinearColor TitleCol = N.Color;
        TitleCol.A *= Alpha;
        FCanvasTextItem TitleItem(FVector2D(TitleX, TitleY), N.CachedText, Font, TitleCol);
        TitleItem.Scale = FVector2D(TitleScale, TitleScale);
        TitleItem.bOutlined = bOutline;
        TitleItem.OutlineColor = OutlineColor;
        Canvas->DrawItem(TitleItem);

        // Subtitle (centered, muted).
        if (bSub) {
            const float SubX = CX - SW * 0.5f;
            const float SubY = TitleY + TH + 4.0f;
            FCanvasTextItem SubItem(FVector2D(SubX, SubY), N.Subtitle, Font, FLinearColor(0.82f, 0.82f, 0.86f, Alpha));
            SubItem.Scale = FVector2D(SubScaleBase, SubScaleBase);
            Canvas->DrawItem(SubItem);
        }

        ++BannerSlot;
    }
}

void UMythicFeedbackSubsystem::DrawDamageNumbers(UCanvas *Canvas, APlayerController *PC) {
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

FString UMythicFeedbackSubsystem::FormatMagnitude(float Magnitude) const {
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

FLinearColor UMythicFeedbackSubsystem::DetermineColor(const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) const {
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

bool UMythicFeedbackSubsystem::IsCriticalHit(const FGameplayEffectContextHandle &EffectContext) const {
    if (EffectContext.IsValid()) {
        const FMythicGameplayEffectContext *MythicContext = static_cast<const FMythicGameplayEffectContext *>(EffectContext.Get());
        if (MythicContext) {
            return MythicContext->IsCriticalHit();
        }
    }
    return false;
}

EMythicDamageNumberType UMythicFeedbackSubsystem::DetermineDamageType(const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) const {
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

    // Priority: Critical (the headline) > Dodge (a miss) > status effects > plain hit. A single hit can flag several
    // statuses at once; we surface the most salient one for the number's color, in a fixed precedence — damage-over-time
    // (Burn/Poison/Bleed), then hard CC (Freeze/Stun), then debuffs (Terrify/Weaken/Slow).
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

FLinearColor UMythicFeedbackSubsystem::GetColorForType(EMythicDamageNumberType Type) const {
    if (!Config) {
        // Fallback colors when no config (mirror the config defaults so behavior is identical sans data asset).
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

EMythicDamageNumberAnimStyle UMythicFeedbackSubsystem::GetAnimStyleForType(EMythicDamageNumberType Type) const {
    if (!Config) {
        // Sensible defaults when no config
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

FVector2D UMythicFeedbackSubsystem::CalculateAnimationOffset(const FMythicDamageNumberData &Data, float CurrentTime) const {
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

float UMythicFeedbackSubsystem::CalculateAnimationScale(const FMythicDamageNumberData &Data, float CurrentTime) const {
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

// ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// Contextual nameplates — health bars shown ONLY for entities ENGAGED with the local player, faded by relevance/distance.
// ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────

float UMythicFeedbackSubsystem::ComputeNameplateTargetAlpha(bool bRelevant, float Distance, float FullDistance, float CullDistance) {
    if (!bRelevant) {
        return 0.0f;
    }
    if (CullDistance <= FullDistance) {
        return Distance <= FullDistance ? 1.0f : 0.0f; // degenerate band -> hard cutoff
    }
    if (Distance <= FullDistance) {
        return 1.0f;
    }
    if (Distance >= CullDistance) {
        return 0.0f;
    }
    return 1.0f - (Distance - FullDistance) / (CullDistance - FullDistance); // linear fade in the band
}

float UMythicFeedbackSubsystem::StepNameplateAlpha(float CurrentAlpha, float TargetAlpha, float DeltaSeconds, float FadeRate) {
    const float MaxStep = FMath::Max(0.0f, FadeRate) * FMath::Max(0.0f, DeltaSeconds);
    if (CurrentAlpha < TargetAlpha) {
        return FMath::Min(TargetAlpha, CurrentAlpha + MaxStep);
    }
    if (CurrentAlpha > TargetAlpha) {
        return FMath::Max(TargetAlpha, CurrentAlpha - MaxStep);
    }
    return CurrentAlpha;
}

float UMythicFeedbackSubsystem::StepGhostFill(float GhostFrac, float TargetFrac, float DeltaSeconds, float DrainSpeed, float HoldRemaining) {
    if (TargetFrac >= GhostFrac) {
        return TargetFrac; // gained (or settled): the chip catches up to the fill immediately
    }
    if (HoldRemaining > 0.0f) {
        return GhostFrac; // the just-lost chip lingers during the hold
    }
    const float MaxDrain = FMath::Max(0.0f, DrainSpeed) * FMath::Max(0.0f, DeltaSeconds);
    return FMath::Max(TargetFrac, GhostFrac - MaxDrain); // drain toward the fill, no undershoot
}

void UMythicFeedbackSubsystem::DrawAmbient(UCanvas *Canvas, APlayerController *PC) {
    UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicEnvironmentSubsystem *Env = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
    if (!Env || !Env->GetEnvironmentController()) {
        return; // no env system (menu map) OR controller not registered yet — never show real-world wall-clock time
    }
    FMythicLocalHud &Hud = GetLocalHud(PC); // per-local-player surfacing state (split-screen safe)

    const FDateTime Now = Env->GetCurrentTime();
    const FGameplayTag Weather = Env->GetWeather();
    const int32 Hour = Now.GetHour();
    const float CurrentTime = World->GetTimeSeconds();

    // Surface the cluster whenever the hour or the weather changes (and on first sight); then it auto-hides.
    if (Hour != Hud.LastAmbientHour || Weather != Hud.LastAmbientWeather) {
        Hud.AmbientShownTime = CurrentTime;
        Hud.LastAmbientHour = Hour;
        Hud.LastAmbientWeather = Weather;
    }

    const float Hold = 3.0f;
    const float Fade = 1.5f;
    const float Elapsed = CurrentTime - Hud.AmbientShownTime;
    float Alpha = 0.0f;
    if (Elapsed < Hold) {
        Alpha = 1.0f;
    }
    else if (Elapsed < Hold + Fade) {
        Alpha = 1.0f - (Elapsed - Hold) / Fade;
    }
    Alpha = FMath::Max(Alpha, Hud.RevealAlpha); // hold-to-reveal also shows the ambient cluster
    if (Alpha <= 0.01f) {
        return; // auto-hidden between changes
    }

    UFont *Font = (Config && Config->Font) ? Config->Font.Get() : nullptr;
    if (!Font) {
        Font = GEngine->GetMediumFont();
    }
    if (!Font) {
        return;
    }

    // Weather leaf name (Environment.Weather.Rain -> "Rain").
    FString WeatherName = TEXT("Clear");
    if (Weather.IsValid()) {
        const FString Full = Weather.GetTagName().ToString();
        int32 DotIdx = INDEX_NONE;
        WeatherName = Full.FindLastChar(TEXT('.'), DotIdx) ? Full.RightChop(DotIdx + 1) : Full;
    }

    const float FontScale = (Config ? Config->FontScaleMultiplier : 1.0f) * 0.8f;
    const float Margin = 22.0f;
    const float PanelW = 134.0f;
    const float PanelH = 50.0f;
    const float PX = Canvas->ClipX - PanelW - Margin;
    const float PY = Margin;

    FCanvasTileItem Panel(FVector2D(PX, PY), GWhiteTexture, FVector2D(PanelW, PanelH), FLinearColor(0.0f, 0.0f, 0.0f, 0.4f * Alpha));
    Panel.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(Panel);

    auto Disc = [&](float cx, float cy, float r, FLinearColor col) {
        FCanvasNGonItem D(FVector2D(cx, cy), FVector2D(r, r), 18, col);
        D.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(D);
    };
    auto Seg = [&](float ax, float ay, float bx, float by, FLinearColor col, float th) {
        FCanvasLineItem L(FVector2D(ax, ay), FVector2D(bx, by));
        L.SetColor(col);
        L.LineThickness = th;
        L.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(L);
    };

    // Time icon: a rayed sun by day, a crescent moon by night.
    const bool bDay = (Hour >= 6 && Hour < 19);
    const float IconCX = PX + 22.0f;
    const float IconCY = PY + 18.0f;
    if (bDay) {
        const FLinearColor Sun(1.0f, 0.82f, 0.3f, Alpha);
        Disc(IconCX, IconCY, 7.0f, Sun);
        for (int32 i = 0; i < 8; ++i) {
            const float A = i * (PI / 4.0f);
            Seg(IconCX + FMath::Cos(A) * 9.0f, IconCY + FMath::Sin(A) * 9.0f, IconCX + FMath::Cos(A) * 12.0f, IconCY + FMath::Sin(A) * 12.0f, Sun, 1.4f);
        }
    }
    else {
        // Crescent — a light disc with a panel-toned disc offset over it.
        Disc(IconCX, IconCY, 8.0f, FLinearColor(0.85f, 0.88f, 0.95f, Alpha));
        Disc(IconCX + 3.5f, IconCY - 1.5f, 7.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f * Alpha));
    }

    // Weather glyph: rain/snow fall from a small cloud; overcast = cloud; clear = nothing extra.
    const float Wx = PX + 66.0f;
    const float Wy = PY + 15.0f;
    auto Cloud = [&]() {
        const FLinearColor Cl(0.8f, 0.83f, 0.88f, Alpha);
        Disc(Wx - 6.0f, Wy, 5.0f, Cl);
        Disc(Wx + 1.0f, Wy - 3.0f, 6.0f, Cl);
        Disc(Wx + 7.0f, Wy, 5.0f, Cl);
    };
    if (WeatherName == TEXT("Rain")) {
        Cloud();
        const FLinearColor Drop(0.5f, 0.7f, 1.0f, Alpha);
        for (int32 i = -1; i <= 1; ++i) {
            Seg(Wx + i * 5.0f, Wy + 7.0f, Wx + i * 5.0f - 2.0f, Wy + 12.0f, Drop, 1.4f);
        }
    }
    else if (WeatherName == TEXT("Snow")) {
        Cloud();
        const FLinearColor Flake(0.9f, 0.95f, 1.0f, Alpha);
        for (int32 i = -1; i <= 1; ++i) {
            Disc(Wx + i * 5.0f, Wy + 9.0f, 1.4f, Flake);
        }
    }
    else if (WeatherName == TEXT("Overcast")) {
        Cloud();
    }

    // Time + day text along the bottom of the panel.
    const FString TimeStr = FString::Printf(TEXT("%02d:00   Day %d"), Hour, Now.GetDay());
    FCanvasTextItem TextItem(FVector2D(PX + 10.0f, PY + 32.0f), FText::FromString(TimeStr), Font, FLinearColor(0.9f, 0.92f, 0.96f, Alpha));
    TextItem.Scale = FVector2D(FontScale, FontScale);
    TextItem.bOutlined = Config ? Config->bEnableOutline : true;
    TextItem.OutlineColor = Config ? Config->OutlineColor : FLinearColor::Black;
    Canvas->DrawItem(TextItem);
}

FMythicLocalHud &UMythicFeedbackSubsystem::GetLocalHud(APlayerController *PC) {
    // Prune buckets whose local player went away (controller destroyed), then find-or-add this PC's bucket.
    for (auto It = LocalHuds.CreateIterator(); It; ++It) {
        if (!It.Key().IsValid()) {
            It.RemoveCurrent();
        }
    }
    return LocalHuds.FindOrAdd(TWeakObjectPtr<APlayerController>(PC));
}

void UMythicFeedbackSubsystem::DrawStatusBadges(UCanvas *Canvas, float CenterX, float RowCenterY, UAbilitySystemComponent *ASC, float Alpha) {
    const UMythicAttributeSet_Defense *Def = ASC ? ASC->GetSet<UMythicAttributeSet_Defense>() : nullptr;
    if (!Def) {
        return;
    }
    struct FBadge {
        float Buildup;
        float Threshold;
        FLinearColor Color;
        int32 Glyph; // 0=X(poison) 1=dot(bleed) 2=snowflake(freeze) 3=flame(burn) 4=slow 5=star(stun)
    };
    const FBadge Badges[] = {
        {Def->GetPoisonBuildup(), 100.0f + Def->GetPoisonResistance() * 2.0f, FLinearColor(0.3f, 0.8f, 0.2f), 0},
        {Def->GetBleedBuildup(), 100.0f + Def->GetBleedResistance() * 2.0f, FLinearColor(0.85f, 0.12f, 0.12f), 1},
        {Def->GetFreezeBuildup(), 100.0f + Def->GetFreezeResistance() * 2.0f, FLinearColor(0.5f, 0.85f, 1.0f), 2},
        {Def->GetBurnBuildup(), 100.0f + Def->GetBurnResistance() * 2.0f, FLinearColor(1.0f, 0.5f, 0.1f), 3},
        {Def->GetSlowBuildup(), 100.0f + Def->GetSlowResistance() * 2.0f, FLinearColor(0.72f, 0.62f, 0.42f), 4},
        {Def->GetStunBuildup(), 100.0f + Def->GetStunResistance() * 2.0f, FLinearColor(0.55f, 0.72f, 1.0f), 5},
    };

    int32 ActiveCount = 0;
    for (const FBadge &B : Badges) {
        if (B.Buildup > 0.5f) {
            ++ActiveCount;
        }
    }
    if (ActiveCount == 0) {
        return;
    }

    const float R = 8.0f;
    const float Step = R * 2.0f + 5.0f;
    float X = CenterX - (ActiveCount * Step - 5.0f) * 0.5f + R; // center of the first badge

    for (const FBadge &B : Badges) {
        if (B.Buildup <= 0.5f) {
            continue;
        }
        const float Frac = B.Threshold > 0.0f ? FMath::Clamp(B.Buildup / B.Threshold, 0.0f, 1.0f) : 0.0f;
        const float BadgeA = Alpha * FMath::Lerp(0.45f, 1.0f, Frac); // faint while building, solid near the proc
        const FVector2D C(X, RowCenterY);

        FCanvasNGonItem Back(C, FVector2D(R + 1.0f, R + 1.0f), 20, FLinearColor(0.03f, 0.04f, 0.05f, 0.85f * Alpha));
        Back.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Back);
        FLinearColor Col = B.Color;
        Col.A = BadgeA;
        FCanvasNGonItem Disc(C, FVector2D(R, R), 20, Col);
        Disc.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Disc);

        // Minimal white glyph so each badge reads as its status even before designer icon textures are assigned.
        const FLinearColor G(0.97f, 0.97f, 0.97f, BadgeA);
        const float g = R * 0.55f;
        auto Line = [&](float ax, float ay, float bx, float by) {
            FCanvasLineItem L(FVector2D(X + ax, RowCenterY + ay), FVector2D(X + bx, RowCenterY + by));
            L.SetColor(G);
            L.LineThickness = 1.3f;
            L.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(L);
        };
        switch (B.Glyph) {
        case 0: // poison — X
            Line(-g, -g, g, g);
            Line(-g, g, g, -g);
            break;
        case 1: { // bleed — drop dot
            FCanvasNGonItem Dot(C, FVector2D(g * 0.6f, g * 0.6f), 12, G);
            Dot.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Dot);
            break;
        }
        case 2: // freeze — snowflake
            Line(0.0f, -g, 0.0f, g);
            Line(-g, 0.0f, g, 0.0f);
            Line(-g * 0.7f, -g * 0.7f, g * 0.7f, g * 0.7f);
            Line(-g * 0.7f, g * 0.7f, g * 0.7f, -g * 0.7f);
            break;
        case 3: { // burn — flame triangle
            FCanvasNGonItem Tri(FVector2D(X, RowCenterY + g * 0.2f), FVector2D(g, g), 3, G);
            Tri.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Tri);
            break;
        }
        case 4: // slow — offset marks
            Line(-g, -g * 0.4f, 0.0f, -g * 0.4f);
            Line(0.0f, g * 0.4f, g, g * 0.4f);
            break;
        default: // stun — star
            Line(0.0f, -g, 0.0f, g);
            Line(-g, 0.0f, g, 0.0f);
            Line(-g * 0.6f, -g * 0.6f, g * 0.6f, g * 0.6f);
            Line(-g * 0.6f, g * 0.6f, g * 0.6f, -g * 0.6f);
            break;
        }

        X += Step;
    }
}

void UMythicFeedbackSubsystem::DrawQuestTracker(UCanvas *Canvas, APlayerController *PC) {
    const AMythicPlayerController *MPC = Cast<AMythicPlayerController>(PC);
    const UObjectiveTracker *Tracker = MPC ? MPC->GetObjectiveTracker() : nullptr;
    if (!Tracker) {
        return;
    }
    const TArray<FObjectiveSummary> Summaries = Tracker->GetActiveObjectiveSummaries();
    if (Summaries.Num() == 0) {
        return;
    }
    UFont *Font = (Config && Config->Font) ? Config->Font.Get() : nullptr;
    if (!Font) {
        Font = GEngine->GetMediumFont();
    }
    if (!Font) {
        return;
    }

    const float Scale = (Config ? Config->FontScaleMultiplier : 1.0f) * 0.8f;
    const float HeaderScale = Scale * 1.08f;
    const float X = 32.0f;
    float Y = 96.0f;
    const float LineH = 18.0f;
    const float Indent = 16.0f;
    const FLinearColor Black(0.0f, 0.0f, 0.0f, 1.0f);

    auto Text = [&](float tx, float ty, const FString &Str, FLinearColor Col, float Sc) -> float {
        FCanvasTextItem T(FVector2D(tx, ty), FText::FromString(Str), Font, Col);
        T.Scale = FVector2D(Sc, Sc);
        T.bOutlined = true;
        T.OutlineColor = Black;
        Canvas->DrawItem(T);
        float w = 0.0f, h = 0.0f;
        Canvas->TextSize(Font, Str, w, h, Sc, Sc);
        return w;
    };

    // Group by quest in first-seen order so each quest header is drawn ONCE and its objectives stay contiguous, even
    // when the active-objective list interleaves objectives from different quests (it isn't grouped upstream).
    TArray<FText> QuestOrder;
    TMap<FString, TArray<int32>> Groups;
    for (int32 i = 0; i < Summaries.Num(); ++i) {
        const FString Key = Summaries[i].QuestName.ToString();
        if (!Groups.Contains(Key)) {
            QuestOrder.Add(Summaries[i].QuestName);
        }
        Groups.FindOrAdd(Key).Add(i);
    }

    for (const FText &QName : QuestOrder) {
        if (!QName.IsEmpty()) {
            Text(X, Y, QName.ToString(), FLinearColor(1.0f, 0.85f, 0.3f, 1.0f), HeaderScale);
            Y += LineH + 2.0f;
        }
        for (const int32 Idx : Groups[QName.ToString()]) {
            const FObjectiveSummary &S = Summaries[Idx];

            const FLinearColor Col = S.bCompleted
                                         ? FLinearColor(0.55f, 0.6f, 0.55f, 1.0f)
                                         : (S.bOptional ? FLinearColor(0.62f, 0.64f, 0.7f, 1.0f) : FLinearColor(0.9f, 0.92f, 0.95f, 1.0f));
            FString Line = S.DisplayText.ToString();
            if (!S.bCompleted && S.RequiredCount > 1) {
                Line += FString::Printf(TEXT("  %d/%d"), S.CurrentCount, S.RequiredCount);
            }
            if (S.bOptional && !S.bCompleted) {
                Line += TEXT("  (optional)");
            }

            const float MX = X + Indent;
            const float MY = Y + LineH * 0.5f;
            // Marker: a green check for completed, a filled dot otherwise.
            if (S.bCompleted) {
                const FLinearColor Check(0.5f, 0.85f, 0.5f, 1.0f);
                FCanvasLineItem C1(FVector2D(MX - 4.0f, MY), FVector2D(MX - 1.0f, MY + 3.0f));
                C1.SetColor(Check);
                C1.LineThickness = 1.5f;
                Canvas->DrawItem(C1);
                FCanvasLineItem C2(FVector2D(MX - 1.0f, MY + 3.0f), FVector2D(MX + 4.0f, MY - 3.0f));
                C2.SetColor(Check);
                C2.LineThickness = 1.5f;
                Canvas->DrawItem(C2);
            }
            else {
                FCanvasNGonItem Dot(FVector2D(MX, MY), FVector2D(2.2f, 2.2f), 10, Col);
                Dot.BlendMode = SE_BLEND_Translucent;
                Canvas->DrawItem(Dot);
            }

            const float LineW = Text(MX + 10.0f, Y, Line, Col, Scale);
            // Strikethrough on completed objectives.
            if (S.bCompleted) {
                FCanvasLineItem Strike(FVector2D(MX + 10.0f, Y + LineH * 0.42f), FVector2D(MX + 10.0f + LineW, Y + LineH * 0.42f));
                Strike.SetColor(Col);
                Strike.LineThickness = 1.2f;
                Strike.BlendMode = SE_BLEND_Translucent;
                Canvas->DrawItem(Strike);
            }
            Y += LineH;
        }
    }
}

void UMythicFeedbackSubsystem::DrawPlayerHud(UCanvas *Canvas, APlayerController *PC) {
    APawn *Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) {
        return;
    }
    FMythicLocalHud &Hud = GetLocalHud(PC); // per-local-player state (split-screen safe)
    UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
    if (!ASC) {
        return;
    }
    UWorld *World = GetWorld();
    const float Dt = World ? World->GetDeltaSeconds() : 0.0f;

    // Read the local player's resources.
    float HealthFrac = 1.0f, StaminaFrac = 1.0f, ShieldFrac = 0.0f;
    if (const UMythicAttributeSet_Life *Life = ASC->GetSet<UMythicAttributeSet_Life>()) {
        const float M = Life->GetMaxHealth();
        HealthFrac = M > 0.0f ? FMath::Clamp(Life->GetHealth() / M, 0.0f, 1.0f) : 0.0f;
    }
    if (const UMythicAttributeSet_Utility *Util = ASC->GetSet<UMythicAttributeSet_Utility>()) {
        const float M = Util->GetMaxStamina();
        StaminaFrac = M > 0.0f ? FMath::Clamp(Util->GetCurrentStamina() / M, 0.0f, 1.0f) : 0.0f;
    }
    if (const UMythicAttributeSet_Defense *Def = ASC->GetSet<UMythicAttributeSet_Defense>()) {
        const float M = Def->GetMaxShield();
        ShieldFrac = M > 0.0f ? FMath::Clamp(Def->GetShield() / M, 0.0f, 1.0f) : 0.0f;
    }
    const bool bSprinting = ASC->HasMatchingGameplayTag(GAS_STATE_SPRINTING);
    const bool bExhausted = ASC->HasMatchingGameplayTag(GAS_STATE_EXHAUSTED);

    // Advance the chip bars (reusing the tested StepGhostFill).
    const float ChipHoldTime = 0.35f;
    const float ChipDrainSpeed = 0.6f;
    auto UpdateChip = [&](FMythicChipBar &C, float Target) {
        if (C.Last < 0.0f) {
            C.Ghost = Target;
        }
        else if (Target < C.Last) {
            C.Hold = ChipHoldTime;
        }
        C.Hold = FMath::Max(0.0f, C.Hold - Dt);
        C.Ghost = StepGhostFill(C.Ghost, Target, Dt, ChipDrainSpeed, C.Hold);
        C.Last = Target;
    };
    UpdateChip(Hud.HealthChip, HealthFrac);
    UpdateChip(Hud.StaminaChip, StaminaFrac);

    // Contextual visibility: health when hurt / a chip is draining; stamina when sprinting / exhausted / not-full.
    // Combat forces health/resources visible (don't hide or dim in a fight); chip/hurt/shield also keep it up.
    const bool bHealthRelevant = (HealthFrac < 0.995f) || (Hud.HealthChip.Ghost > HealthFrac + 0.001f) || (ShieldFrac > 0.0f) || Hud.bInCombat;
    const bool bStaminaRelevant = bSprinting || bExhausted || (StaminaFrac < 0.995f);
    Hud.HealthVis = StepNameplateAlpha(Hud.HealthVis, bHealthRelevant ? 1.0f : 0.0f, Dt, 4.0f);
    Hud.StaminaVis = StepNameplateAlpha(Hud.StaminaVis, bStaminaRelevant ? 1.0f : 0.0f, Dt, 4.0f);
    // Hold-to-reveal overrides the contextual fades — everything shows at full while the reveal key is held.
    const float HealthShow = FMath::Max(Hud.HealthVis, Hud.RevealAlpha);
    const float StaminaShow = FMath::Max(Hud.StaminaVis, Hud.RevealAlpha);

    const float CX = Canvas->ClipX * 0.5f;
    const float BaseY = Canvas->ClipY - 70.0f;
    const float BarW = 300.0f;
    const float BarH = 13.0f;
    const float Gap = 6.0f;

    // Shared bar drawer (bg + chip + fill) at a given vis alpha.
    auto DrawBar = [&](float Y, float Frac, float GhostFrac, FLinearColor FillCol, float Vis) {
        if (Vis <= 0.01f) {
            return;
        }
        const float X = CX - BarW * 0.5f;
        FCanvasTileItem Bg(FVector2D(X - 1.0f, Y - 1.0f), GWhiteTexture, FVector2D(BarW + 2.0f, BarH + 2.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.55f * Vis));
        Bg.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Bg);
        if (GhostFrac > Frac + 0.001f) {
            FCanvasTileItem Chip(FVector2D(X + BarW * Frac, Y), GWhiteTexture, FVector2D(BarW * (GhostFrac - Frac), BarH), FLinearColor(0.95f, 0.85f, 0.7f, 0.7f * Vis));
            Chip.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Chip);
        }
        FLinearColor C = FillCol;
        C.A = Vis;
        FCanvasTileItem Fill(FVector2D(X, Y), GWhiteTexture, FVector2D(BarW * Frac, BarH), C);
        Fill.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Fill);
    };

    // Health (green at full -> red when low), with a thin shield sliver above it.
    const FLinearColor HealthCol = FMath::Lerp(FLinearColor(0.85f, 0.2f, 0.15f), FLinearColor(0.3f, 0.8f, 0.35f), HealthFrac);
    DrawBar(BaseY, HealthFrac, Hud.HealthChip.Ghost, HealthCol, HealthShow);
    if (ShieldFrac > 0.0f && HealthShow > 0.01f) {
        const float X = CX - BarW * 0.5f;
        FCanvasTileItem Shield(FVector2D(X, BaseY - 5.0f), GWhiteTexture, FVector2D(BarW * ShieldFrac, 3.0f), FLinearColor(0.4f, 0.7f, 1.0f, HealthShow));
        Shield.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Shield);
    }

    // Stamina below (amber; burnt-orange while exhausted).
    const FLinearColor StamCol = bExhausted ? FLinearColor(0.85f, 0.4f, 0.1f) : FLinearColor(0.85f, 0.75f, 0.3f);
    DrawBar(BaseY + BarH + Gap, StaminaFrac, Hud.StaminaChip.Ghost, StamCol, StaminaShow);

    // Player status badges above the bars — always shown while a status is building (you must know you're poisoned).
    DrawStatusBadges(Canvas, CX, BaseY - 16.0f, ASC, 1.0f);

    // Currency: surfaces + rolls the count on a balance change, then auto-hides (with a +N / -N flyout).
    if (const AMythicPlayerController *MPC = Cast<AMythicPlayerController>(PC)) {
        if (const UMythicInventoryComponent *Inv = MPC->GetInventoryComponent()) {
            const int32 Balance = Inv->GetTotalCurrency();
            const float Now = World ? World->GetTimeSeconds() : 0.0f;
            if (Hud.CurrencyLast < 0) {
                Hud.CurrencyDisplayed = static_cast<float>(Balance);
                Hud.CurrencyLast = Balance;
            }
            else if (Balance != Hud.CurrencyLast) {
                Hud.CurrencyDelta = Balance - Hud.CurrencyLast;
                Hud.CurrencyLast = Balance;
                Hud.CurrencyShownTime = Now;
            }
            // Roll the displayed value toward the real balance (faster the bigger the gap) — the count-up/down animation.
            const float RollSpeed = FMath::Max(8.0f, FMath::Abs(static_cast<float>(Balance) - Hud.CurrencyDisplayed) * 6.0f);
            Hud.CurrencyDisplayed = FMath::FInterpConstantTo(Hud.CurrencyDisplayed, static_cast<float>(Balance), Dt, RollSpeed);

            const float CElapsed = Now - Hud.CurrencyShownTime;
            const float CSurfacing = (CElapsed < 2.5f) ? 1.0f : (CElapsed < 4.0f ? 1.0f - (CElapsed - 2.5f) / 1.5f : 0.0f);
            const float CAlpha = FMath::Max(CSurfacing, Hud.RevealAlpha); // hold-to-reveal also shows the wallet
            if (CAlpha > 0.01f) {
                UFont *Font = (Config && Config->Font) ? Config->Font.Get() : nullptr;
                if (!Font) {
                    Font = GEngine->GetMediumFont();
                }
                if (Font) {
                    const FString Amount = FString::Printf(TEXT("%d"), FMath::RoundToInt(Hud.CurrencyDisplayed));
                    const float FontScale = Config ? Config->FontScaleMultiplier : 1.0f;
                    float TW = 0.0f, TH = 0.0f;
                    Canvas->TextSize(Font, Amount, TW, TH, FontScale, FontScale);
                    const float CoinR = 6.0f;
                    const float TotalW = CoinR * 2.0f + 6.0f + TW;
                    const float OX = CX - TotalW * 0.5f;
                    const float OY = BaseY - 40.0f;

                    FCanvasNGonItem Coin(FVector2D(OX + CoinR, OY + TH * 0.5f), FVector2D(CoinR, CoinR), 18, FLinearColor(0.85f, 0.7f, 0.28f, CAlpha));
                    Coin.BlendMode = SE_BLEND_Translucent;
                    Canvas->DrawItem(Coin);

                    FCanvasTextItem Amt(FVector2D(OX + CoinR * 2.0f + 6.0f, OY), FText::FromString(Amount), Font, FLinearColor(0.95f, 0.9f, 0.75f, CAlpha));
                    Amt.Scale = FVector2D(FontScale, FontScale);
                    Amt.bOutlined = true;
                    Amt.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, CAlpha);
                    Canvas->DrawItem(Amt);

                    // +N / -N flyout: floats up and fades over the first ~1.2s after a change.
                    if (CElapsed < 1.2f && Hud.CurrencyDelta != 0) {
                        const float FlyA = CAlpha * FMath::Clamp(1.0f - CElapsed / 1.2f, 0.0f, 1.0f);
                        const FString DeltaStr = FString::Printf(TEXT("%s%d"), Hud.CurrencyDelta > 0 ? TEXT("+") : TEXT(""), Hud.CurrencyDelta);
                        const FLinearColor DeltaCol = Hud.CurrencyDelta > 0 ? FLinearColor(0.4f, 0.9f, 0.45f, FlyA) : FLinearColor(0.95f, 0.4f, 0.3f, FlyA);
                        FCanvasTextItem Fly(FVector2D(OX + TotalW + 8.0f, OY - CElapsed * 18.0f), FText::FromString(DeltaStr), Font, DeltaCol);
                        Fly.Scale = FVector2D(FontScale * 0.85f, FontScale * 0.85f);
                        Canvas->DrawItem(Fly);
                    }
                }
            }
        }
    }
}

bool UMythicFeedbackSubsystem::IsNameplateRelevant(AActor *Npc, AActor *LocalPawn) const {
    // ENGAGED = this NPC is fighting the local player. Read the NPC's REPLICATED EngagedTarget (mirrored from the server
    // AI's hostile target) so the plate appears on EVERY client — the AIController's target is server-only and is null on
    // remote clients. Broader signals (threat table, faction-hostile-and-near) are a follow-up.
    if (const AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(Npc)) {
        return NPC->GetEngagedTarget() == LocalPawn;
    }
    return false;
}

void UMythicFeedbackSubsystem::DrawNameplates(UCanvas *Canvas, APlayerController *PC) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    FMythicLocalHud &Hud = GetLocalHud(PC); // per-local-player state (split-screen safe)
    APawn *LocalPawn = PC->GetPawn();
    if (!LocalPawn) {
        Hud.NameplateStates.Reset();
        Hud.LastNameplateTime = -1.0f; // restart the fade clock so the first frame after respawn doesn't spike Dt (no pop-in)
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();
    const float Dt = (Hud.LastNameplateTime < 0.0f) ? 0.0f : FMath::Max(0.0f, CurrentTime - Hud.LastNameplateTime);
    Hud.LastNameplateTime = CurrentTime;

    // Code-default tuning (no asset needed). Distances in cm; FadeRate is alpha/sec.
    const float FullDistance = 1500.0f;
    const float CullDistance = 3500.0f;
    const float FadeRate = 3.5f;
    const float HeadZ = 110.0f;
    const float BarW = 88.0f;
    const float BarH = 7.0f;
    const float FocusRadiusPx = 220.0f; // a plate within this many px of screen-centre is "focused"
    const float DimFactor = 0.5f;       // non-focused plates dim to this when something is focused
    const float ChipHoldTime = 0.35f;   // delayed-damage chip lingers this long after a hit before draining
    const float ChipDrainSpeed = 0.6f;  // then drains the lost slice at this fraction/sec

    const FVector LocalLoc = LocalPawn->GetActorLocation();

    // This frame's candidate target alphas (raw ptr keys valid only within this frame).
    TMap<AActor *, float> Targets;
    for (TActorIterator<AMythicNPCCharacter> It(World); It; ++It) {
        AActor *Npc = *It;
        if (!IsValid(Npc) || Npc == LocalPawn) {
            continue;
        }
        const float Dist = FVector::Dist(Npc->GetActorLocation(), LocalLoc);
        if (Dist > CullDistance) {
            continue; // far enough that even a fading plate is gone
        }
        const bool bRelevant = IsNameplateRelevant(Npc, LocalPawn);
        Targets.Add(Npc, ComputeNameplateTargetAlpha(bRelevant, Dist, FullDistance, CullDistance));
    }

    // In-combat = any engaged (relevant) NPC is in range — drives the contextual "always show health in combat" rule.
    Hud.bInCombat = false;
    for (const TPair<AActor *, float> &P : Targets) {
        if (P.Value > 0.0f) {
            Hud.bInCombat = true;
            break;
        }
    }

    // FOCUS: among the relevant on-screen plates, the one nearest screen-centre is what the player is looking at.
    // It stays full while the others dim — so attention reads at a glance without hiding the rest.
    AActor *FocusedActor = nullptr;
    {
        const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
        float BestD2 = FocusRadiusPx * FocusRadiusPx;
        for (const TPair<AActor *, float> &P : Targets) {
            if (P.Value <= 0.0f || !IsValid(P.Key)) {
                continue;
            }
            FVector2D SP;
            if (UGameplayStatics::ProjectWorldToScreen(PC, P.Key->GetActorLocation() + FVector(0.0f, 0.0f, HeadZ), SP, true)) {
                const float D2 = FVector2D::DistSquared(SP, Center);
                if (D2 < BestD2) {
                    BestD2 = D2;
                    FocusedActor = P.Key;
                }
            }
        }
    }

    UFont *Font = (Config && Config->Font) ? Config->Font.Get() : nullptr;
    if (!Font) {
        Font = GEngine->GetMediumFont();
    }
    const float NameScale = (Config ? Config->FontScaleMultiplier : 1.0f) * 0.75f; // small label above the bar

    // Read an NPC's display name (once, at state creation — it doesn't change). Empty for nameless/MASS creatures.
    auto ReadNpcName = [](AActor *Npc) -> FText {
        if (const AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(Npc)) {
            return FText::FromString(NPC->GetNPCData().NPCName); // NPCName is an FString
        }
        return FText::GetEmpty();
    };

    // Draw a full nameplate (name + health bar + active status-buildup bars) above an entity's head at the given alpha.
    auto DrawPlate = [&](AActor *Npc, FMythicNameplateState &S, float Alpha, bool bFocused) {
        UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Npc);
        float HealthFrac = 1.0f;
        if (ASC) {
            if (const UMythicAttributeSet_Life *Life = ASC->GetSet<UMythicAttributeSet_Life>()) {
                const float MaxH = Life->GetMaxHealth();
                HealthFrac = MaxH > 0.0f ? FMath::Clamp(Life->GetHealth() / MaxH, 0.0f, 1.0f) : 0.0f;
            }
        }

        // Delayed-damage "chip": the ghost frac trails HealthFrac — lingers on a hit (hold), then drains the lost slice.
        if (S.LastHealthFrac < 0.0f) {
            S.GhostHealthFrac = HealthFrac; // first sight: no chip
        }
        else if (HealthFrac < S.LastHealthFrac) {
            S.HealthChipHold = ChipHoldTime; // took damage -> (re)start the hold
        }
        S.HealthChipHold = FMath::Max(0.0f, S.HealthChipHold - Dt);
        S.GhostHealthFrac = StepGhostFill(S.GhostHealthFrac, HealthFrac, Dt, ChipDrainSpeed, S.HealthChipHold);
        S.LastHealthFrac = HealthFrac;

        FVector2D ScreenPos;
        const FVector HeadWorld = Npc->GetActorLocation() + FVector(0.0f, 0.0f, HeadZ);
        if (!UGameplayStatics::ProjectWorldToScreen(PC, HeadWorld, ScreenPos, true)) {
            return; // off-screen / behind camera
        }
        const float X = ScreenPos.X - BarW * 0.5f;
        const float Y = ScreenPos.Y;

        // Name (centered above the bar).
        if (Font && !S.CachedName.IsEmpty()) {
            float NW = 0.0f, NH = 0.0f;
            Canvas->TextSize(Font, S.CachedName.ToString(), NW, NH, NameScale, NameScale);
            FCanvasTextItem NameItem(FVector2D(ScreenPos.X - NW * 0.5f, Y - NH - 3.0f), S.CachedName, Font, FLinearColor(0.92f, 0.93f, 0.96f, Alpha));
            NameItem.Scale = FVector2D(NameScale, NameScale);
            NameItem.bOutlined = true;
            NameItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, Alpha);
            Canvas->DrawItem(NameItem);
        }

        // Focus highlight — a bright frame behind the bar (the dark bg draws on top, leaving a 1px bright border).
        if (bFocused) {
            FCanvasTileItem Frame(FVector2D(X - 2.0f, Y - 2.0f), GWhiteTexture, FVector2D(BarW + 4.0f, BarH + 4.0f),
                                  FLinearColor(1.0f, 0.95f, 0.7f, 0.9f * Alpha));
            Frame.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Frame);
        }

        // Health bar background (1px inset frame).
        FCanvasTileItem Bg(FVector2D(X - 1.0f, Y - 1.0f), GWhiteTexture, FVector2D(BarW + 2.0f, BarH + 2.0f),
                           FLinearColor(0.0f, 0.0f, 0.0f, 0.6f * Alpha));
        Bg.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Bg);

        // Chip (recently-lost slice): from the current fill edge out to the trailing ghost edge, dim warm-white.
        if (S.GhostHealthFrac > HealthFrac + 0.001f) {
            const float ChipX = X + BarW * HealthFrac;
            const float ChipW = BarW * (S.GhostHealthFrac - HealthFrac);
            FCanvasTileItem Chip(FVector2D(ChipX, Y), GWhiteTexture, FVector2D(ChipW, BarH), FLinearColor(0.95f, 0.85f, 0.7f, 0.7f * Alpha));
            Chip.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(Chip);
        }

        // Health fill — green at full, red when low.
        const FLinearColor LowCol(0.85f, 0.2f, 0.15f);
        const FLinearColor FullCol(0.25f, 0.85f, 0.35f);
        FLinearColor FillCol = FMath::Lerp(LowCol, FullCol, HealthFrac);
        FillCol.A = Alpha;
        FCanvasTileItem Fill(FVector2D(X, Y), GWhiteTexture, FVector2D(BarW * HealthFrac, BarH), FillCol);
        Fill.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Fill);

        // Status-effect badges (circular, color-coded, opacity ramps with buildup-toward-proc) above the plate.
        DrawStatusBadges(Canvas, ScreenPos.X, Y - 28.0f, ASC, Alpha);
    };

    // Update existing states: fade toward their candidate target (0 if no longer a candidate), draw, prune.
    for (int32 i = Hud.NameplateStates.Num() - 1; i >= 0; --i) {
        FMythicNameplateState &S = Hud.NameplateStates[i];
        AActor *Npc = S.Actor.Get();
        float Target = 0.0f;
        if (Npc) {
            if (const float *Found = Targets.Find(Npc)) {
                Target = *Found;
                Targets.Remove(Npc); // mark handled
            }
        }
        S.CurrentAlpha = StepNameplateAlpha(S.CurrentAlpha, Target, Dt, FadeRate);
        if (!Npc || (S.CurrentAlpha <= 0.01f && Target <= 0.0f)) {
            Hud.NameplateStates.RemoveAtSwap(i, EAllowShrinking::No);
            continue;
        }
        if (S.CurrentAlpha > 0.01f) {
            const bool bFocused = (Npc == FocusedActor);
            const float Dim = (FocusedActor && !bFocused) ? DimFactor : 1.0f;
            DrawPlate(Npc, S, S.CurrentAlpha * Dim, bFocused);
        }
    }

    // New candidates that don't yet have a state (start at 0 and fade in).
    for (const TPair<AActor *, float> &Pair : Targets) {
        if (Pair.Value <= 0.0f) {
            continue; // disengaged-but-near: nothing to show, don't spawn an invisible state
        }
        FMythicNameplateState NewState;
        NewState.Actor = Pair.Key;
        NewState.CachedName = ReadNpcName(Pair.Key);
        NewState.CurrentAlpha = StepNameplateAlpha(0.0f, Pair.Value, Dt, FadeRate);
        if (NewState.CurrentAlpha > 0.01f) {
            const bool bFocused = (Pair.Key == FocusedActor);
            const float Dim = (FocusedActor && !bFocused) ? DimFactor : 1.0f;
            DrawPlate(Pair.Key, NewState, NewState.CurrentAlpha * Dim, bFocused);
        }
        Hud.NameplateStates.Add(NewState);
    }
}
