// Copyright Stellar Games. All Rights Reserved.

#include "MythicWorldFeedbackSubsystem.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogMythicWorldFeedback, Log, All);

void UMythicWorldFeedbackSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    ActiveCallouts.Reserve(64);

    if (const UMythicDeveloperSettings *DevSettings = GetDefault<UMythicDeveloperSettings>()) {
        Config = DevSettings->WorldFeedbackConfig.LoadSynchronous();
        if (Config) {
            UE_LOG(LogMythicWorldFeedback, Log, TEXT("Loaded WorldFeedbackConfig: %s"), *Config->GetName());
        }
    }

    // Bind to HUD drawing (called every frame for each local player). Same immediate-mode path as the damage numbers.
    HUDDrawDelegateHandle = AHUD::OnHUDPostRender.AddUObject(this, &UMythicWorldFeedbackSubsystem::OnHUDPostRender);

    UE_LOG(LogMythicWorldFeedback, Log, TEXT("WorldFeedbackSubsystem initialized"));
}

void UMythicWorldFeedbackSubsystem::Deinitialize() {
    // Mirror the damage-number teardown: remove the global HUD-render binding so it can't dangle into a freed subsystem
    // (gotcha g — the OnHUDPostRender delegate outlives the world subsystem).
    AHUD::OnHUDPostRender.Remove(HUDDrawDelegateHandle);
    ActiveCallouts.Empty();

    Super::Deinitialize();
}

bool UMythicWorldFeedbackSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    // Only game worlds (not editor preview), matching the damage-number subsystem.
    if (const UWorld *World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicWorldFeedbackSubsystem::OnHUDPostRender(AHUD *HUD, UCanvas *Canvas) {
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
    DrawCallouts(Canvas, PC);
}

void UMythicWorldFeedbackSubsystem::CleanupExpired() {
    // Bound the pool even when the HUD is hidden (AHUD::PostRender gates on bShowHUD, so DrawCallouts may not run).
    // Called on every Add (the sole growth source). Backward loop so RemoveAtSwap can't skip an element.
    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    for (int32 i = ActiveCallouts.Num() - 1; i >= 0; --i) {
        if (ActiveCallouts[i].IsExpired(CurrentTime)) {
            ActiveCallouts.RemoveAtSwap(i, EAllowShrinking::No);
        }
    }
}

void UMythicWorldFeedbackSubsystem::AddWorldCallout(FVector WorldLocation, const FText &Text, FLinearColor Color,
                                                    UTexture2D *Icon, EMythicFeedbackCategory Category,
                                                    float DurationOverride) {
    CleanupExpired();

    FMythicWorldFeedbackEntry Entry;
    Entry.WorldLocation = WorldLocation;
    Entry.CachedText = Text;
    Entry.Icon = Icon;
    Entry.Color = Color;
    Entry.SpawnTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    Entry.Lifetime = DurationOverride > 0.0f ? DurationOverride : (Config ? Config->DefaultLifetime : 2.0f);
    Entry.Category = Category;
    Entry.bDrawBackground = Config ? Config->bDrawBackgroundByDefault : true;
    Entry.ID = NextID++;

    ActiveCallouts.Add(MoveTemp(Entry));
}

void UMythicWorldFeedbackSubsystem::SetConfig(UMythicWorldFeedbackConfig *NewConfig) {
    Config = NewConfig;
}

void UMythicWorldFeedbackSubsystem::ClearAll() {
    ActiveCallouts.Empty();
}

float UMythicWorldFeedbackSubsystem::ComputeAlpha(float Elapsed, float Lifetime, float FadeInTime, float FadeOutTime) {
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

float UMythicWorldFeedbackSubsystem::ComputeRise(float Elapsed, float RiseSpeed) {
    return FMath::Max(0.0f, Elapsed) * RiseSpeed;
}

void UMythicWorldFeedbackSubsystem::DrawCallouts(UCanvas *Canvas, APlayerController *PC) {
    if (ActiveCallouts.Num() == 0) {
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

    const float FontScale = Config ? Config->FontScale : 1.0f;
    const float FadeIn = Config ? Config->FadeInTime : 0.15f;
    const float FadeOut = Config ? Config->FadeOutTime : 0.5f;
    const float RiseSpeed = Config ? Config->RiseSpeed : 40.0f;
    const float IconSize = Config ? Config->IconSize : 28.0f;
    const bool bOutline = Config ? Config->bEnableOutline : true;
    const FLinearColor OutlineColor = Config ? Config->OutlineColor : FLinearColor::Black;
    const FLinearColor BgTint = Config ? Config->BackgroundTint : FLinearColor(0.f, 0.f, 0.f, 0.5f);
    const float BgPad = Config ? Config->BackgroundPadding : 6.0f;
    const float IconGap = 4.0f;

    for (int32 i = ActiveCallouts.Num() - 1; i >= 0; --i) {
        FMythicWorldFeedbackEntry &Entry = ActiveCallouts[i];

        if (Entry.IsExpired(CurrentTime)) {
            ActiveCallouts.RemoveAtSwap(i, EAllowShrinking::No);
            continue;
        }

        const float Elapsed = CurrentTime - Entry.SpawnTime;

        // Rise in WORLD space so the callout lifts off its anchor, then project.
        const float Rise = ComputeRise(Elapsed, RiseSpeed);
        const FVector RisenWorld = Entry.WorldLocation + FVector(0.0f, 0.0f, Rise);

        FVector2D ScreenPos;
        if (!UGameplayStatics::ProjectWorldToScreen(PC, RisenWorld, ScreenPos, true)) {
            continue; // off-screen / behind camera
        }

        const float Alpha = ComputeAlpha(Elapsed, Entry.Lifetime, FadeIn, FadeOut);

        // Measure text for layout.
        float TextW = 0.0f, TextH = 0.0f;
        Canvas->TextSize(Font, Entry.CachedText.ToString(), TextW, TextH, FontScale, FontScale);

        const bool bHasIcon = (Entry.Icon != nullptr);
        const float IconW = bHasIcon ? IconSize : 0.0f;
        const float Gap = bHasIcon ? IconGap : 0.0f;
        const float TotalW = IconW + Gap + TextW;
        const float TotalH = FMath::Max(bHasIcon ? IconSize : 0.0f, TextH);

        // Center the icon+text block on the projected screen position.
        const float OriginX = ScreenPos.X - TotalW * 0.5f;
        const float OriginY = ScreenPos.Y - TotalH * 0.5f;

        // Background panel.
        if (Entry.bDrawBackground && BgTint.A > 0.0f) {
            FLinearColor Bg = BgTint;
            Bg.A *= Alpha;
            FCanvasTileItem BgItem(
                FVector2D(OriginX - BgPad, OriginY - BgPad),
                GWhiteTexture,
                FVector2D(TotalW + BgPad * 2.0f, TotalH + BgPad * 2.0f),
                Bg);
            BgItem.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(BgItem);
        }

        // Icon (left), vertically centered in the block.
        if (bHasIcon && Entry.Icon->GetResource()) {
            FLinearColor IconColor = Entry.Color;
            IconColor.A *= Alpha;
            FCanvasTileItem IconItem(
                FVector2D(OriginX, OriginY + (TotalH - IconSize) * 0.5f),
                Entry.Icon->GetResource(),
                FVector2D(IconSize, IconSize),
                IconColor);
            IconItem.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(IconItem);
        }

        // Label (right of the icon), vertically centered.
        FLinearColor TextColor = Entry.Color;
        TextColor.A *= Alpha;
        FCanvasTextItem TextItem(
            FVector2D(OriginX + IconW + Gap, OriginY + (TotalH - TextH) * 0.5f),
            Entry.CachedText,
            Font,
            TextColor);
        TextItem.Scale = FVector2D(FontScale, FontScale);
        TextItem.bOutlined = bOutline;
        TextItem.OutlineColor = OutlineColor;
        Canvas->DrawItem(TextItem);
    }
}
