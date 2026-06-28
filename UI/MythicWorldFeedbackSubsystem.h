// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DataAsset.h"
#include "MythicWorldFeedbackSubsystem.generated.h"

class AHUD;
class UCanvas;
class UFont;
class UTexture2D;
class APlayerController;

/**
 * World-feedback subsystem — the NON-COMBAT counterpart to the damage-number subsystem.
 *
 * The damage-number subsystem (UMythicDamageNumberSubsystem) is for COMBAT only (damage values, crits, status pops like
 * "STUNNED!"). Everything else — loot pickups, quest/objective updates, party events, resource depletion, proficiency
 * level-ups, faction-standing shifts, hazard warnings — belongs HERE.
 *
 * Like the damage-number subsystem it is EXTREMELY optimized and NON-WBP: immediate-mode Canvas drawing via
 * AHUD::OnHUDPostRender, a pooled cache-friendly flat array, no UMG widgets. Unlike it, a callout can carry an ICON
 * (and an optional translucent background panel) in addition to text, so the feedback reads at a glance.
 */

/** Category of a world callout — drives default styling (color/icon) and lets the HUD filter by type later. */
UENUM(BlueprintType)
enum class EMythicFeedbackCategory : uint8 {
    Generic,
    Loot,        // item pickups
    Quest,       // objective progress / completion
    Party,       // companion join/leave/loyalty
    Resource,    // gathering / node depletion
    Proficiency, // skill level-ups
    Faction,     // standing shifts (now Hostile/Friendly)
    Hazard,      // environmental warnings
};

/**
 * One active world-feedback callout. Plain struct for cache-friendly iteration (same rationale as the damage numbers).
 */
USTRUCT(BlueprintType)
struct FMythicWorldFeedbackEntry {
    GENERATED_BODY()

    // World location the callout floats from.
    UPROPERTY()
    FVector WorldLocation = FVector::ZeroVector;

    // Label, cached as FText at creation (not per-frame) for draw performance.
    UPROPERTY()
    FText CachedText;

    // Optional icon drawn to the left of the label (null = text only).
    UPROPERTY()
    TObjectPtr<UTexture2D> Icon = nullptr;

    // Tint for the label (and the icon, if it is a white/mask texture).
    UPROPERTY()
    FLinearColor Color = FLinearColor::White;

    // World time this callout was spawned.
    UPROPERTY()
    float SpawnTime = 0.0f;

    // Seconds the callout is visible.
    UPROPERTY()
    float Lifetime = 2.0f;

    // Unique id (for potential targeting/dedup).
    UPROPERTY()
    int32 ID = 0;

    // Category (styling/filtering).
    UPROPERTY()
    EMythicFeedbackCategory Category = EMythicFeedbackCategory::Generic;

    // Draw the translucent background panel behind icon+text.
    UPROPERTY()
    bool bDrawBackground = true;

    bool IsExpired(float CurrentTime) const {
        return (CurrentTime - SpawnTime) >= Lifetime;
    }
};

/**
 * Designer-tunable appearance for world feedback. Optional — the subsystem falls back to sane code defaults when no
 * config asset is assigned (so it works before any .uasset exists).
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicWorldFeedbackConfig : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font")
    TObjectPtr<UFont> Font;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float FontScale = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font")
    bool bEnableOutline = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font", meta = (EditCondition = "bEnableOutline"))
    FLinearColor OutlineColor = FLinearColor(0.f, 0.f, 0.f, 1.f);

    /** Default seconds a callout lives. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.1", ClampMax = "30.0"))
    float DefaultLifetime = 2.0f;

    /** Seconds to fade IN at the start. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float FadeInTime = 0.15f;

    /** Seconds to fade OUT at the end. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float FadeOutTime = 0.5f;

    /** World units the callout rises per second. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0", ClampMax = "500.0"))
    float RiseSpeed = 40.0f;

    /** Icon square size in pixels. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Icon", meta = (ClampMin = "4.0", ClampMax = "256.0"))
    float IconSize = 28.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background")
    bool bDrawBackgroundByDefault = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background")
    FLinearColor BackgroundTint = FLinearColor(0.f, 0.f, 0.f, 0.5f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Background", meta = (ClampMin = "0.0", ClampMax = "64.0"))
    float BackgroundPadding = 6.0f;
};

UCLASS()
class MYTHIC_API UMythicWorldFeedbackSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;

    /**
     * Queue a world-anchored callout (icon + text, floating up from WorldLocation). Server code should route through a
     * Client RPC first — this is a local presentation subsystem.
     * @param Icon  optional; null = text only.
     * @param DurationOverride  <= 0 uses the config DefaultLifetime.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|WorldFeedback")
    void AddWorldCallout(FVector WorldLocation, const FText &Text, FLinearColor Color,
                         UTexture2D *Icon = nullptr, EMythicFeedbackCategory Category = EMythicFeedbackCategory::Generic,
                         float DurationOverride = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Mythic|WorldFeedback")
    void SetConfig(UMythicWorldFeedbackConfig *NewConfig);

    UFUNCTION(BlueprintCallable, Category = "Mythic|WorldFeedback")
    void ClearAll();

    // ─── Pure presentation math (public + static so it is unit-testable; Mythic.UI.WorldFeedback) ───

    /**
     * Callout opacity over its lifetime: ramps 0->1 over FadeInTime at the start, holds 1 in the middle, ramps 1->0 over
     * FadeOutTime at the end. Always clamped to [0,1]; Lifetime<=0 -> 0 (degenerate).
     */
    static float ComputeAlpha(float Elapsed, float Lifetime, float FadeInTime, float FadeOutTime);

    /** World-units the callout has risen after Elapsed seconds (monotonic, never negative). */
    static float ComputeRise(float Elapsed, float RiseSpeed);

private:
    void OnHUDPostRender(AHUD *HUD, UCanvas *Canvas);
    void CleanupExpired();
    void DrawCallouts(UCanvas *Canvas, APlayerController *PC);

    UPROPERTY()
    TArray<FMythicWorldFeedbackEntry> ActiveCallouts;

    UPROPERTY()
    TObjectPtr<UMythicWorldFeedbackConfig> Config;

    int32 NextID = 0;

    FDelegateHandle HUDDrawDelegateHandle;
};
