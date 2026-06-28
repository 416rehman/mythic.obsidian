// Copyright Stellar Games. All Rights Reserved.

#include "MythicDamageNumberSubsystem.h"
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
    DrawWorldCallouts(Canvas, PC);
    DrawNameplates(Canvas, PC);
    DrawPlayerHud(Canvas, PC);           // bottom-centre player resource bars (chip + contextual)
    DrawScreenNotifications(Canvas, PC); // screen UI (toasts/banners) layers on top of world-anchored elements
    DrawAmbient(Canvas);                 // auto-hiding time/weather corner cluster
}

// Swap-remove every expired entry. Declared in the header but previously never defined — the ONLY pruning was inside
// DrawDamageNumbers, which the engine stops calling when the HUD is hidden (AHUD::PostRender gates on bShowHUD), so the
// pool grew unbounded while hidden. Called on every Add (the sole growth source) so the array stays bounded regardless
// of whether the HUD is currently rendering.
void UMythicDamageNumberSubsystem::CleanupExpired() {
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

void UMythicDamageNumberSubsystem::AddDamageNumber(FVector WorldLocation, float Magnitude, const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) {
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

void UMythicDamageNumberSubsystem::AddDodgeNumber(FVector WorldLocation) {
    // Reuse the configured DodgeColor (designer-tunable data asset) — single source, no duplicated literal; the named
    // FLinearColor::Gray is only a degenerate fallback when no config asset is set. "DODGE" is the conventional miss label.
    const FLinearColor DodgeColor = Config ? Config->DodgeColor : FLinearColor::Gray;
    AddCombatText(WorldLocation, TEXT("DODGE"), DodgeColor, 1.0f);
}

void UMythicDamageNumberSubsystem::AddCombatText(FVector WorldLocation, const FString &Text, FLinearColor Color, float Lifetime) {
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

void UMythicDamageNumberSubsystem::SetConfig(UMythicDamageNumberConfig *NewConfig) {
    Config = NewConfig;
}

void UMythicDamageNumberSubsystem::ClearAll() {
    ActiveDamageNumbers.Empty();
    ActiveNotifications.Empty();
    ActiveWorldCallouts.Empty();
}

void UMythicDamageNumberSubsystem::AddWorldCallout(FVector WorldLocation, const FText &Text, FLinearColor Color, UTexture2D *Icon, float DurationOverride) {
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

void UMythicDamageNumberSubsystem::DrawWorldCallouts(UCanvas *Canvas, APlayerController *PC) {
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

void UMythicDamageNumberSubsystem::AddScreenToast(const FText &Text, FLinearColor Color, UTexture2D *Icon, float DurationOverride) {
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

float UMythicDamageNumberSubsystem::ComputeToastAlpha(float Elapsed, float Lifetime, float FadeInTime, float FadeOutTime) {
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

float UMythicDamageNumberSubsystem::ComputeToastSlideOffset(float Elapsed, float FadeInTime, float SlideDistance) {
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

float UMythicDamageNumberSubsystem::ComputeToastStackOffset(int32 SlotFromAnchor, float EntryStep) {
    return FMath::Max(0, SlotFromAnchor) * EntryStep;
}

void UMythicDamageNumberSubsystem::AddScreenBanner(const FText &Title, const FText &Subtitle, FLinearColor AccentColor, UTexture2D *Icon, float DurationOverride) {
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

float UMythicDamageNumberSubsystem::EaseOutBack(float T) {
    // Standard ease-out-back: settles to 1.0 with a small overshoot past it near the end (the "pop").
    const float C1 = 1.70158f;
    const float C3 = C1 + 1.0f;
    const float T1 = T - 1.0f;
    return 1.0f + C3 * T1 * T1 * T1 + C1 * T1 * T1;
}

float UMythicDamageNumberSubsystem::ComputeBannerScale(float Elapsed, float EntranceTime, float StartScale) {
    if (EntranceTime <= 0.0f || Elapsed >= EntranceTime) {
        return 1.0f;
    }
    if (Elapsed <= 0.0f) {
        return StartScale;
    }
    const float E = EaseOutBack(Elapsed / EntranceTime); // 0..~1 with a late overshoot
    return FMath::Lerp(StartScale, 1.0f, E);
}

float UMythicDamageNumberSubsystem::ComputeBannerSweepX(float Elapsed, float EntranceTime, float Width) {
    if (EntranceTime <= 0.0f) {
        return Width;
    }
    const float T = FMath::Clamp(Elapsed / EntranceTime, 0.0f, 1.0f);
    return T * Width;
}

void UMythicDamageNumberSubsystem::DrawScreenNotifications(UCanvas *Canvas, APlayerController *PC) {
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

FLinearColor UMythicDamageNumberSubsystem::GetColorForType(EMythicDamageNumberType Type) const {
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

EMythicDamageNumberAnimStyle UMythicDamageNumberSubsystem::GetAnimStyleForType(EMythicDamageNumberType Type) const {
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

// ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// Contextual nameplates — health bars shown ONLY for entities ENGAGED with the local player, faded by relevance/distance.
// ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────

float UMythicDamageNumberSubsystem::ComputeNameplateTargetAlpha(bool bRelevant, float Distance, float FullDistance, float CullDistance) {
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

float UMythicDamageNumberSubsystem::StepNameplateAlpha(float CurrentAlpha, float TargetAlpha, float DeltaSeconds, float FadeRate) {
    const float MaxStep = FMath::Max(0.0f, FadeRate) * FMath::Max(0.0f, DeltaSeconds);
    if (CurrentAlpha < TargetAlpha) {
        return FMath::Min(TargetAlpha, CurrentAlpha + MaxStep);
    }
    if (CurrentAlpha > TargetAlpha) {
        return FMath::Max(TargetAlpha, CurrentAlpha - MaxStep);
    }
    return CurrentAlpha;
}

float UMythicDamageNumberSubsystem::StepGhostFill(float GhostFrac, float TargetFrac, float DeltaSeconds, float DrainSpeed, float HoldRemaining) {
    if (TargetFrac >= GhostFrac) {
        return TargetFrac; // gained (or settled): the chip catches up to the fill immediately
    }
    if (HoldRemaining > 0.0f) {
        return GhostFrac; // the just-lost chip lingers during the hold
    }
    const float MaxDrain = FMath::Max(0.0f, DrainSpeed) * FMath::Max(0.0f, DeltaSeconds);
    return FMath::Max(TargetFrac, GhostFrac - MaxDrain); // drain toward the fill, no undershoot
}

void UMythicDamageNumberSubsystem::DrawAmbient(UCanvas *Canvas) {
    UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicEnvironmentSubsystem *Env = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
    if (!Env || !Env->GetEnvironmentController()) {
        return; // no env system (menu map) OR controller not registered yet — never show real-world wall-clock time
    }

    const FDateTime Now = Env->GetCurrentTime();
    const FGameplayTag Weather = Env->GetWeather();
    const int32 Hour = Now.GetHour();
    const float CurrentTime = World->GetTimeSeconds();

    // Surface the cluster whenever the hour or the weather changes (and on first sight); then it auto-hides.
    if (Hour != LastAmbientHour || Weather != LastAmbientWeather) {
        AmbientShownTime = CurrentTime;
        LastAmbientHour = Hour;
        LastAmbientWeather = Weather;
    }

    const float Hold = 3.0f;
    const float Fade = 1.5f;
    const float Elapsed = CurrentTime - AmbientShownTime;
    float Alpha = 0.0f;
    if (Elapsed < Hold) {
        Alpha = 1.0f;
    }
    else if (Elapsed < Hold + Fade) {
        Alpha = 1.0f - (Elapsed - Hold) / Fade;
    }
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

    const FString Text = FString::Printf(TEXT("%02d:00    %s    Day %d"), Hour, *WeatherName, Now.GetDay());
    const float FontScale = (Config ? Config->FontScaleMultiplier : 1.0f) * 0.85f;

    float TW = 0.0f, TH = 0.0f;
    Canvas->TextSize(Font, Text, TW, TH, FontScale, FontScale);
    const float Margin = 24.0f;
    const float Pad = 7.0f;
    const float X = Canvas->ClipX - TW - Margin;
    const float Y = Margin;

    FCanvasTileItem Bg(FVector2D(X - Pad, Y - Pad), GWhiteTexture, FVector2D(TW + Pad * 2.0f, TH + Pad * 2.0f),
                       FLinearColor(0.0f, 0.0f, 0.0f, 0.45f * Alpha));
    Bg.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(Bg);

    FCanvasTextItem TextItem(FVector2D(X, Y), FText::FromString(Text), Font, FLinearColor(0.9f, 0.92f, 0.96f, Alpha));
    TextItem.Scale = FVector2D(FontScale, FontScale);
    TextItem.bOutlined = Config ? Config->bEnableOutline : true;
    TextItem.OutlineColor = Config ? Config->OutlineColor : FLinearColor::Black;
    Canvas->DrawItem(TextItem);
}

void UMythicDamageNumberSubsystem::DrawPlayerHud(UCanvas *Canvas, APlayerController *PC) {
    APawn *Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) {
        return;
    }
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
    UpdateChip(PlayerHealthChip, HealthFrac);
    UpdateChip(PlayerStaminaChip, StaminaFrac);

    // Contextual visibility: health when hurt / a chip is draining; stamina when sprinting / exhausted / not-full.
    const bool bHealthRelevant = (HealthFrac < 0.995f) || (PlayerHealthChip.Ghost > HealthFrac + 0.001f) || (ShieldFrac > 0.0f);
    const bool bStaminaRelevant = bSprinting || bExhausted || (StaminaFrac < 0.995f);
    PlayerHealthVis = StepNameplateAlpha(PlayerHealthVis, bHealthRelevant ? 1.0f : 0.0f, Dt, 4.0f);
    PlayerStaminaVis = StepNameplateAlpha(PlayerStaminaVis, bStaminaRelevant ? 1.0f : 0.0f, Dt, 4.0f);

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
    DrawBar(BaseY, HealthFrac, PlayerHealthChip.Ghost, HealthCol, PlayerHealthVis);
    if (ShieldFrac > 0.0f && PlayerHealthVis > 0.01f) {
        const float X = CX - BarW * 0.5f;
        FCanvasTileItem Shield(FVector2D(X, BaseY - 5.0f), GWhiteTexture, FVector2D(BarW * ShieldFrac, 3.0f), FLinearColor(0.4f, 0.7f, 1.0f, PlayerHealthVis));
        Shield.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(Shield);
    }

    // Stamina below (amber; burnt-orange while exhausted).
    const FLinearColor StamCol = bExhausted ? FLinearColor(0.85f, 0.4f, 0.1f) : FLinearColor(0.85f, 0.75f, 0.3f);
    DrawBar(BaseY + BarH + Gap, StaminaFrac, PlayerStaminaChip.Ghost, StamCol, PlayerStaminaVis);
}

bool UMythicDamageNumberSubsystem::IsNameplateRelevant(AActor *Npc, AActor *LocalPawn) const {
    // ENGAGED = this NPC is fighting the local player. Read the NPC's REPLICATED EngagedTarget (mirrored from the server
    // AI's hostile target) so the plate appears on EVERY client — the AIController's target is server-only and is null on
    // remote clients. Broader signals (threat table, faction-hostile-and-near) are a follow-up.
    if (const AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(Npc)) {
        return NPC->GetEngagedTarget() == LocalPawn;
    }
    return false;
}

void UMythicDamageNumberSubsystem::DrawNameplates(UCanvas *Canvas, APlayerController *PC) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    APawn *LocalPawn = PC->GetPawn();
    if (!LocalPawn) {
        NameplateStates.Reset();
        LastNameplateTime = -1.0f; // restart the fade clock so the first frame after respawn doesn't spike Dt (no pop-in)
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();
    const float Dt = (LastNameplateTime < 0.0f) ? 0.0f : FMath::Max(0.0f, CurrentTime - LastNameplateTime);
    LastNameplateTime = CurrentTime;

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

        // Active status-buildup bars (below the health bar). Fraction = buildup / (100 + resistance*2) — the exact
        // trigger threshold from MythicAttributeSet_Defense. One colored mini-bar per type that has any buildup.
        if (ASC) {
            if (const UMythicAttributeSet_Defense *Def = ASC->GetSet<UMythicAttributeSet_Defense>()) {
                struct FBuildupVis {
                    float Buildup;
                    float Threshold;
                    FLinearColor Color;
                };
                const FBuildupVis Vis[] = {
                    {Def->GetBurnBuildup(), 100.0f + Def->GetBurnResistance() * 2.0f, FLinearColor(1.0f, 0.45f, 0.0f)},  // burn
                    {Def->GetBleedBuildup(), 100.0f + Def->GetBleedResistance() * 2.0f, FLinearColor(0.7f, 0.0f, 0.0f)}, // bleed
                    {Def->GetPoisonBuildup(), 100.0f + Def->GetPoisonResistance() * 2.0f, FLinearColor(0.4f, 0.85f, 0.1f)}, // poison
                    {Def->GetSlowBuildup(), 100.0f + Def->GetSlowResistance() * 2.0f, FLinearColor(0.4f, 0.6f, 0.9f)},   // slow
                    {Def->GetFreezeBuildup(), 100.0f + Def->GetFreezeResistance() * 2.0f, FLinearColor(0.5f, 0.9f, 1.0f)}, // freeze
                    {Def->GetStunBuildup(), 100.0f + Def->GetStunResistance() * 2.0f, FLinearColor(1.0f, 0.9f, 0.4f)},   // stun
                };
                const float PipW = 12.0f, PipH = 3.0f, PipGap = 2.0f;
                int32 ActiveCount = 0;
                for (const FBuildupVis &V : Vis) {
                    if (V.Buildup > 0.5f) {
                        ++ActiveCount;
                    }
                }
                if (ActiveCount > 0) {
                    const float RowW = ActiveCount * PipW + (ActiveCount - 1) * PipGap;
                    float PipX = ScreenPos.X - RowW * 0.5f;
                    const float PipY = Y + BarH + 2.0f;
                    for (const FBuildupVis &V : Vis) {
                        if (V.Buildup <= 0.5f) {
                            continue;
                        }
                        const float Frac = V.Threshold > 0.0f ? FMath::Clamp(V.Buildup / V.Threshold, 0.0f, 1.0f) : 0.0f;
                        FCanvasTileItem PipBg(FVector2D(PipX, PipY), GWhiteTexture, FVector2D(PipW, PipH), FLinearColor(0.0f, 0.0f, 0.0f, 0.55f * Alpha));
                        PipBg.BlendMode = SE_BLEND_Translucent;
                        Canvas->DrawItem(PipBg);
                        FLinearColor PipCol = V.Color;
                        PipCol.A = Alpha;
                        FCanvasTileItem PipFill(FVector2D(PipX, PipY), GWhiteTexture, FVector2D(PipW * Frac, PipH), PipCol);
                        PipFill.BlendMode = SE_BLEND_Translucent;
                        Canvas->DrawItem(PipFill);
                        PipX += PipW + PipGap;
                    }
                }
            }
        }
    };

    // Update existing states: fade toward their candidate target (0 if no longer a candidate), draw, prune.
    for (int32 i = NameplateStates.Num() - 1; i >= 0; --i) {
        FMythicNameplateState &S = NameplateStates[i];
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
            NameplateStates.RemoveAtSwap(i, EAllowShrinking::No);
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
        NameplateStates.Add(NewState);
    }
}
