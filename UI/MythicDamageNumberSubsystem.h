// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/Canvas.h"
#include "Fonts/SlateFontInfo.h"
#include "GameplayEffectTypes.h"
#include "Engine/DataAsset.h"
#include "MythicDamageNumberSubsystem.generated.h"

class AHUD;
class UFont;

UENUM(BlueprintType)
enum class EMythicDamageNumberAnimStyle : uint8 {
    FloatUp UMETA(DisplayName = "Float Up"),

    FloatUpSlow UMETA(DisplayName = "Float Up (Slow)"),

    Bounce UMETA(DisplayName = "Bounce"),

    ArcLeft UMETA(DisplayName = "Arc Left"),

    ArcRight UMETA(DisplayName = "Arc Right"),

    Shake UMETA(DisplayName = "Shake"),

    Pulse UMETA(DisplayName = "Pulse"),
};

UENUM(BlueprintType)
enum class EMythicDamageNumberType : uint8 {
    Default,
    Critical,
    Heal,
    Bleed,
    Burn,
    Poison,
    Stun,
    Slow,
    Weaken,
    Freeze,
    Terrify,
    Dodge,
};

USTRUCT(BlueprintType)
struct FMythicDamageNumberData {
    GENERATED_BODY()

    UPROPERTY()
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY()
    FText CachedText;

    UPROPERTY()
    FLinearColor Color = FLinearColor::White;

    UPROPERTY()
    float SpawnTime = 0.0f;

    UPROPERTY()
    float Lifetime = 1.0f;

    UPROPERTY()
    int32 ID = 0;

    UPROPERTY()
    float RandomOffsetX = 0.0f;

    UPROPERTY()
    float ExtraVerticalSpeed = 0.0f;

    UPROPERTY()
    bool bIsCritical = false;

    UPROPERTY()
    EMythicDamageNumberAnimStyle AnimStyle = EMythicDamageNumberAnimStyle::FloatUp;

    UPROPERTY()
    EMythicDamageNumberType DamageType = EMythicDamageNumberType::Default;

    bool IsExpired(float CurrentTime) const {
        return (CurrentTime - SpawnTime) >= Lifetime;
    }

    float GetAlpha(float CurrentTime) const {
        const float Age = CurrentTime - SpawnTime;
        const float NormalizedAge = FMath::Clamp(Age / Lifetime, 0.0f, 1.0f);
        return (NormalizedAge > 0.7f) ? FMath::Lerp(1.0f, 0.0f, (NormalizedAge - 0.7f) / 0.3f) : 1.0f;
    }

    float GetVerticalOffset(float CurrentTime) const {
        const float Age = CurrentTime - SpawnTime;
        return Age * (50.0f + ExtraVerticalSpeed);
    }

    float GetBurstScale(float CurrentTime, float BurstScale, float BurstDuration) const {
        const float Age = CurrentTime - SpawnTime;
        if (Age >= BurstDuration || BurstDuration <= 0.0f) {
            return 1.0f;
        }
        const float T = Age / BurstDuration;
        const float EaseOut = 1.0f - FMath::Pow(1.0f - T, 3.0f);
        return FMath::Lerp(BurstScale, 1.0f, EaseOut);
    }
};

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

    // Status-effect colors — one per status so each effect reads at a glance. Author-time visual defaults
    // (designers can retune in the data asset); no gameplay magnitudes here.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor BleedColor = FLinearColor(0.7f, 0.0f, 0.0f); // Dark red

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor BurnColor = FLinearColor(1.0f, 0.45f, 0.0f); // Orange

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor PoisonColor = FLinearColor(0.4f, 0.85f, 0.1f); // Sickly green

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor StunColor = FLinearColor(1.0f, 0.9f, 0.4f); // Pale yellow

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor SlowColor = FLinearColor(0.4f, 0.6f, 0.9f); // Steel blue

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor WeakenColor = FLinearColor(0.6f, 0.45f, 0.7f); // Muted purple

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor FreezeColor = FLinearColor(0.5f, 0.9f, 1.0f); // Cyan

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor TerrifyColor = FLinearColor(0.7f, 0.2f, 0.7f); // Dark magenta

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor DodgeColor = FLinearColor(0.8f, 0.8f, 0.85f); // Light gray

    // Animation style per damage type
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle DefaultAnimStyle = EMythicDamageNumberAnimStyle::FloatUp;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle CriticalAnimStyle = EMythicDamageNumberAnimStyle::Bounce;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle HealAnimStyle = EMythicDamageNumberAnimStyle::Pulse;

    // Shared style for all status-effect numbers (Shake reads as a lingering "status" hit).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle StatusAnimStyle = EMythicDamageNumberAnimStyle::Shake;

    // Style for a dodged attack (a gentle drift — it's a miss, not a hit).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle DodgeAnimStyle = EMythicDamageNumberAnimStyle::FloatUpSlow;

    // Number formatting
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Formatting")
    bool bAbbreviateLargeNumbers = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Formatting", meta = (EditCondition = "bAbbreviateLargeNumbers"))
    float ThousandThreshold = 10000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Formatting", meta = (EditCondition = "bAbbreviateLargeNumbers"))
    float MillionThreshold = 1000000.0f;
};

UCLASS()
class MYTHIC_API UMythicDamageNumberSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;

    /**
     * Add a new damage number to the display pool.
     * This is the main entry point - call from gameplay cues or damage events.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|DamageNumbers")
    void AddDamageNumber(FVector WorldLocation, float Magnitude, const FGameplayEffectContextHandle &EffectContext, bool bIsHeal = false);

    /**
     * COMBAT-ONLY: float arbitrary combat status text off a world point (e.g. "Shield Broken!", "Winded!", "DODGE").
     * Non-combat feedback belongs in the UMG/MVVM HUD — this is deliberately not a generic floating-text hatch.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|DamageNumbers")
    void AddCombatText(FVector WorldLocation, const FString &Text, FLinearColor Color, float Lifetime = 1.0f);

    // Float a "DODGE" callout using the configured DodgeColor (single source — no duplicated literal). Used by the
    // dodge feedback path, which must fire from the authority damage execution because a dodge negates the hit BEFORE
    // the normal damage cue runs, so the built Dodge color would otherwise never appear.
    UFUNCTION(BlueprintCallable, Category = "Mythic|DamageNumbers")
    void AddDodgeNumber(FVector WorldLocation);

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

    void DrawDamageNumbers(UCanvas *Canvas, APlayerController *PC);

    int32 GetActiveDamageNumberCount() const { return ActiveDamageNumbers.Num(); }

    void OnHUDPostRender(AHUD *HUD, UCanvas *Canvas);

protected:
    FString FormatMagnitude(float Magnitude) const;

    EMythicDamageNumberType DetermineDamageType(const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) const;

    FLinearColor GetColorForType(EMythicDamageNumberType Type) const;

    EMythicDamageNumberAnimStyle GetAnimStyleForType(EMythicDamageNumberType Type) const;

    FVector2D CalculateAnimationOffset(const FMythicDamageNumberData &Data, float CurrentTime) const;

    float CalculateAnimationScale(const FMythicDamageNumberData &Data, float CurrentTime) const;

    FLinearColor DetermineColor(const FGameplayEffectContextHandle &EffectContext, bool bIsHeal) const;

    bool IsCriticalHit(const FGameplayEffectContextHandle &EffectContext) const;

    void CleanupExpired();

protected:
    UPROPERTY()
    TArray<FMythicDamageNumberData> ActiveDamageNumbers;

    UPROPERTY()
    TObjectPtr<UMythicDamageNumberConfig> Config;

    int32 NextID = 0;

    FDelegateHandle HUDDrawDelegateHandle;
};
