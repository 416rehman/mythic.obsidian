// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/Canvas.h"
#include "Fonts/SlateFontInfo.h"
#include "GameplayEffectTypes.h"
#include "MythicDamageNumberSubsystem.generated.h"

class AHUD;
class UFont;

/**
 * Animation styles for damage numbers.
 * Each style provides a distinct visual feel while remaining readable.
 */
UENUM(BlueprintType)
enum class EMythicDamageNumberAnimStyle : uint8 {
    // Floats straight up at normal speed (default)
    FloatUp UMETA(DisplayName = "Float Up"),

    // Floats up slowly with a gentle fade
    FloatUpSlow UMETA(DisplayName = "Float Up (Slow)"),

    // Bounces up then settles (good for crits)
    Bounce UMETA(DisplayName = "Bounce"),

    // Arcs to the left while floating up
    ArcLeft UMETA(DisplayName = "Arc Left"),

    // Arcs to the right while floating up
    ArcRight UMETA(DisplayName = "Arc Right"),

    // Small horizontal shake while floating (good for status effects)
    Shake UMETA(DisplayName = "Shake"),

    // Pulses in scale while floating (good for heals)
    Pulse UMETA(DisplayName = "Pulse"),
};

/**
 * Damage type category for animation/color selection.
 */
UENUM(BlueprintType)
enum class EMythicDamageNumberType : uint8 {
    Default,
    Critical,
    Heal,
};

/**
 * Data for a single damage number instance.
 * Kept as a simple struct for cache-friendly iteration.
 */
USTRUCT(BlueprintType)
struct FMythicDamageNumberData {
    GENERATED_BODY()

    // World location where the damage occurred
    UPROPERTY()
    FVector WorldLocation = FVector::ZeroVector;

    // The text to display (formatted damage value) - cached as FText for drawing performance
    UPROPERTY()
    FText CachedText;

    // Color of the text
    UPROPERTY()
    FLinearColor Color = FLinearColor::White;

    // Time when this damage number was spawned (world time)
    UPROPERTY()
    float SpawnTime = 0.0f;

    // How long this damage number should be visible
    UPROPERTY()
    float Lifetime = 1.0f;

    // Unique ID for this damage number (for potential targeting/updates)
    UPROPERTY()
    int32 ID = 0;

    // Optional: Random horizontal offset for visual variety
    UPROPERTY()
    float RandomOffsetX = 0.0f;

    // Optional: Extra vertical velocity for variety
    UPROPERTY()
    float ExtraVerticalSpeed = 0.0f;

    // Is this a critical hit (for scaled rendering)
    UPROPERTY()
    bool bIsCritical = false;

    // Animation style for this damage number
    UPROPERTY()
    EMythicDamageNumberAnimStyle AnimStyle = EMythicDamageNumberAnimStyle::FloatUp;

    // Damage type category
    UPROPERTY()
    EMythicDamageNumberType DamageType = EMythicDamageNumberType::Default;

    bool IsExpired(float CurrentTime) const {
        return (CurrentTime - SpawnTime) >= Lifetime;
    }

    float GetAlpha(float CurrentTime) const {
        const float Age = CurrentTime - SpawnTime;
        const float NormalizedAge = FMath::Clamp(Age / Lifetime, 0.0f, 1.0f);
        // Fade out in the last 30% of lifetime
        return (NormalizedAge > 0.7f) ? FMath::Lerp(1.0f, 0.0f, (NormalizedAge - 0.7f) / 0.3f) : 1.0f;
    }

    float GetVerticalOffset(float CurrentTime) const {
        const float Age = CurrentTime - SpawnTime;
        // Float upward over time
        return Age * (50.0f + ExtraVerticalSpeed);
    }

    // Returns a scale multiplier for the burst effect (starts big, settles to 1.0)
    float GetBurstScale(float CurrentTime, float BurstScale, float BurstDuration) const {
        const float Age = CurrentTime - SpawnTime;
        if (Age >= BurstDuration || BurstDuration <= 0.0f) {
            return 1.0f;
        }
        // Exponential ease-out: starts at BurstScale, quickly settles to 1.0
        const float T = Age / BurstDuration;
        const float EaseOut = 1.0f - FMath::Pow(1.0f - T, 3.0f); // Cubic ease-out
        return FMath::Lerp(BurstScale, 1.0f, EaseOut);
    }
};

/**
 * Configuration data asset for damage number appearance.
 * 
 * IMPORTANT: For crisp text, configure the font SIZE in the font asset itself (Font Editor > Size),
 * not via FontScaleMultiplier. Canvas text scaling stretches pre-rasterized glyphs and looks blurry.
 * Use a larger font (32-64pt) and set FontScaleMultiplier < 1.0 if you need smaller text.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicDamageNumberConfig : public UDataAsset {
    GENERATED_BODY()

public:
    // Font to use for damage numbers. Set the font SIZE in the font asset itself for crisp rendering.
    // Recommended: Create a font at 32-64pt size. Scaling up at runtime looks blurry.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font")
    TObjectPtr<UFont> Font;

    // Scale multiplier (use sparingly - scaling UP looks blurry, scaling DOWN is fine)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float FontScaleMultiplier = 1.0f;

    // Enable text outline
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font")
    bool bEnableOutline = true;

    // Outline color
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font", meta = (EditCondition = "bEnableOutline"))
    FLinearColor OutlineColor = FLinearColor::Black;

    // Default lifetime in seconds
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float DefaultLifetime = 1.0f;

    // Vertical float speed (units per second in screen space)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float VerticalFloatSpeed = 50.0f;

    // Random horizontal offset range
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float RandomHorizontalOffsetRange = 30.0f;

    // Random extra vertical speed range
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float RandomVerticalSpeedRange = 20.0f;

    // Scale multiplier for critical hits
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Critical")
    float CriticalHitScaleMultiplier = 1.3f;

    // Burst Animation: Initial scale when damage number first appears (e.g., 2.0 = starts at 2x size)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Burst")
    float BurstScaleMultiplier = 1.5f;

    // Burst Animation: How long (in seconds) it takes to settle from burst size to normal size
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Burst")
    float BurstDuration = 0.15f;

    // Color mappings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors")
    FLinearColor DefaultColor = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors")
    FLinearColor CriticalHitColor = FLinearColor::Yellow;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors")
    FLinearColor HealColor = FLinearColor(0.0f, 1.0f, 0.3f); // Bright Green

    // Animation style per damage type
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle DefaultAnimStyle = EMythicDamageNumberAnimStyle::FloatUp;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle CriticalAnimStyle = EMythicDamageNumberAnimStyle::Bounce;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle HealAnimStyle = EMythicDamageNumberAnimStyle::Pulse;

    // Number formatting
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Formatting")
    bool bAbbreviateLargeNumbers = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Formatting", meta = (EditCondition = "bAbbreviateLargeNumbers"))
    float ThousandThreshold = 10000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Formatting", meta = (EditCondition = "bAbbreviateLargeNumbers"))
    float MillionThreshold = 1000000.0f;
};

/**
 * UMythicDamageNumberSubsystem
 *
 * High-performance world subsystem for managing and rendering damage numbers.
 * Uses a simple array pool with Canvas drawing for minimal overhead.
 */
UCLASS()
class MYTHIC_API UMythicDamageNumberSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    // USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    // ~USubsystem interface

    /**
     * Add a new damage number to the display pool.
     * This is the main entry point - call from gameplay cues or damage events.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|DamageNumbers")
    void AddDamageNumber(FVector WorldLocation, float Magnitude, const FGameplayEffectContextHandle &EffectContext, bool bIsHeal = false);

    /**
     * Add a damage number with explicit text and color.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|DamageNumbers")
    void AddDamageNumberCustom(FVector WorldLocation, const FString &Text, FLinearColor Color, float Lifetime = 1.0f);

    /**
     * Set the configuration data asset.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|DamageNumbers")
    void SetConfig(UMythicDamageNumberConfig *NewConfig);

    /**
     * Clear all active damage numbers.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|DamageNumbers")
    void ClearAll();

    /**
     * Called by HUD to draw all damage numbers. Do not call directly.
     */
    void DrawDamageNumbers(UCanvas *Canvas, APlayerController *PC);

    // Callback for HUD post-render delegate
    void OnHUDPostRender(AHUD *HUD, UCanvas *Canvas);

protected:
    // Formats magnitude to display string
    FString FormatMagnitude(float Magnitude) const;

    // Determines damage type from effect context
    EMythicDamageNumberType DetermineDamageType(const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) const;

    // Gets color for a specific damage type
    FLinearColor GetColorForType(EMythicDamageNumberType Type) const;

    // Gets animation style for a specific damage type
    EMythicDamageNumberAnimStyle GetAnimStyleForType(EMythicDamageNumberType Type) const;

    // Calculates screen-space offset based on animation style
    FVector2D CalculateAnimationOffset(const FMythicDamageNumberData &Data, float CurrentTime) const;

    // Calculates extra scale based on animation style (for Pulse, etc.)
    float CalculateAnimationScale(const FMythicDamageNumberData &Data, float CurrentTime) const;

    // Determines color from effect context (legacy, use GetColorForType instead)
    FLinearColor DetermineColor(const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) const;

    // Checks if this is a critical hit from context
    bool IsCriticalHit(const FGameplayEffectContextHandle &EffectContext) const;

    // Removes expired entries from the pool
    void CleanupExpired();

protected:
    // Active damage numbers
    UPROPERTY()
    TArray<FMythicDamageNumberData> ActiveDamageNumbers;

    // Configuration
    UPROPERTY()
    TObjectPtr<UMythicDamageNumberConfig> Config;

    // ID counter for unique damage number IDs
    int32 NextID = 0;

    // Delegate handle for HUD drawing
    FDelegateHandle HUDDrawDelegateHandle;
};
