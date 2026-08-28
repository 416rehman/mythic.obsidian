// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/Canvas.h"
#include "Fonts/SlateFontInfo.h"
#include "GAS/Feedback/MythicCombatTextTypes.h"
#include "Engine/DataAsset.h"
#include "MythicDamageNumberSubsystem.generated.h"

class AHUD;
class UFont;
class UMythicStatusEffectDefinition;
class UMythicStatusEffectLibrary;

/** Selects the local screen-space motion treatment for one resolved combat-text entry. */
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

/** Transient local presentation state for one resolved numeric or textual combat result. */
USTRUCT(BlueprintType)
struct FMythicDamageNumberData {
    GENERATED_BODY()

    UPROPERTY()
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector TargetOffset = FVector::ZeroVector;

    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;

    UPROPERTY()
    TWeakObjectPtr<AActor> SourceActor;

    UPROPERTY()
    TObjectPtr<UMythicStatusEffectDefinition> StatusDefinition = nullptr;

    UPROPERTY()
    EMythicCombatTextOrigin Origin = EMythicCombatTextOrigin::DirectDamage;

    UPROPERTY()
    float Magnitude = 0.0f;

    UPROPERTY()
    FText CachedText;

    UPROPERTY()
    FLinearColor Color = FLinearColor::White;

    UPROPERTY()
    float SpawnTime = 0.0f;

    UPROPERTY()
    float Lifetime = 1.0f;

    UPROPERTY()
    float ScaleMultiplier = 1.0f;

    UPROPERTY()
    int32 ID = 0;

    UPROPERTY()
    float RandomOffsetX = 0.0f;

    UPROPERTY()
    float ExtraVerticalSpeed = 0.0f;

    UPROPERTY()
    bool bIsCritical = false;

    UPROPERTY()
    bool bOutgoingForViewer = true;

    UPROPERTY()
    bool bResolvedEvent = false;

    UPROPERTY()
    EMythicDamageNumberAnimStyle AnimStyle = EMythicDamageNumberAnimStyle::FloatUp;

    bool IsExpired(float CurrentTime) const {
        return (CurrentTime - SpawnTime) >= Lifetime;
    }

    float GetAlpha(float CurrentTime) const {
        const float Age = CurrentTime - SpawnTime;
        const float NormalizedAge = FMath::Clamp(Age / FMath::Max(Lifetime, SMALL_NUMBER), 0.0f, 1.0f);
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

/** Data Asset containing accessibility-aware formatting, color, animation, and performance tuning for combat text. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicDamageNumberConfig : public UDataAsset {
    GENERATED_BODY()

public:
    /** Font used for damage numbers. Author its point size in the font asset so runtime scaling remains crisp. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font")
    TObjectPtr<UFont> Font;

    /** Global font scale multiplier applied before accessibility and per-number presentation scales. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float FontScaleMultiplier = 1.0f;

    /** Whether damage-number glyphs render an outline. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font")
    bool bEnableOutline = true;

    /** Color of the damage-number glyph outline when outlining is enabled. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Font", meta = (EditCondition = "bEnableOutline"))
    FLinearColor OutlineColor = FLinearColor::Black;

    /** Base lifetime in seconds before per-kind lifetime multipliers are applied. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float DefaultLifetime = 1.0f;

    /** Base vertical animation speed in screen-space units per second. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float VerticalFloatSpeed = 50.0f;

    /** Symmetric random horizontal screen-space offset used to separate concurrent numbers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float RandomHorizontalOffsetRange = 30.0f;

    /** Maximum random vertical-speed variation added to an entry. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    float RandomVerticalSpeedRange = 20.0f;

    /** Additional scale multiplier for critical-hit numbers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Critical")
    float CriticalHitScaleMultiplier = 1.3f;

    /** Initial scale of the entry's spawn burst before it settles to its presentation scale. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Burst")
    float BurstScaleMultiplier = 1.5f;

    /** Seconds taken for the spawn burst to settle to the entry's presentation scale. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Burst")
    float BurstDuration = 0.15f;

    /** Client-side offset from a resolved event's target/fallback position; gameplay never authors presentation offsets. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
    FVector WorldOffset = FVector(0.0f, 0.0f, 50.0f);

    /** Maximum number of active numeric and combat-text entries retained by a local world. Oldest entries are evicted first. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Performance", meta = (ClampMin = "16", ClampMax = "2048"))
    int32 MaxActiveNumbers = 256;

    /** Same-target resolutions within this short window merge when status, source, origin, and viewer direction also match. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Performance", meta = (ClampMin = "0.0", ClampMax = "0.25", Units = "s"))
    float MergeWindowSeconds = 0.075f;

    /** Scale applied only to periodic status-tick numbers so they read as secondary damage. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float StatusScaleMultiplier = 0.85f;

    /** Lifetime multiplier applied only to periodic status-tick numbers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float StatusLifetimeMultiplier = 0.85f;

    /** Color for ordinary resolved damage with no more specific presentation identity. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors")
    FLinearColor DefaultColor = FLinearColor::White;

    /** Color for resolved critical-hit damage. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors")
    FLinearColor CriticalHitColor = FLinearColor::Yellow;

    /** Color for positive healing numbers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors")
    FLinearColor HealColor = FLinearColor(0.0f, 1.0f, 0.3f); // Bright Green

    /** Color for incoming damage absorbed by an energy shield. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors")
    FLinearColor ShieldAbsorptionColor = FLinearColor(0.4f, 0.7f, 1.0f);

    /** Color for a dodge combat-text callout. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colors")
    FLinearColor DodgeColor = FLinearColor(0.8f, 0.8f, 0.85f); // Light gray

    /** Animation used by ordinary damage numbers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle DefaultAnimStyle = EMythicDamageNumberAnimStyle::FloatUp;

    /** Animation used by critical-hit damage numbers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle CriticalAnimStyle = EMythicDamageNumberAnimStyle::Bounce;

    /** Animation used by healing numbers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle HealAnimStyle = EMythicDamageNumberAnimStyle::Pulse;

    /** Animation used by incoming energy-shield absorption numbers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle ShieldAbsorptionAnimStyle = EMythicDamageNumberAnimStyle::FloatUp;

    /** Shared motion style for definition-colored periodic status-tick numbers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle StatusAnimStyle = EMythicDamageNumberAnimStyle::Shake;

    /** Animation used by dodge callouts. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Styles")
    EMythicDamageNumberAnimStyle DodgeAnimStyle = EMythicDamageNumberAnimStyle::FloatUpSlow;

    /** Whether sufficiently large magnitudes use compact K/M suffixes. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Formatting")
    bool bAbbreviateLargeNumbers = false;

    /** Smallest absolute magnitude formatted with a K suffix when abbreviation is enabled. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Formatting", meta = (EditCondition = "bAbbreviateLargeNumbers"))
    float ThousandThreshold = 10000.0f;

    /** Smallest absolute magnitude formatted with an M suffix when abbreviation is enabled. */
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
     * Evaluates combat-number visibility without requiring live user settings: 0 hides all, 1 shows outgoing only,
     * and 2 shows outgoing and incoming events. Unknown modes fail closed.
     */
    static bool ShouldPresentResolvedEvent(uint8 DamageNumberMode, bool bOutgoingForViewer);

    /**
     * Presents one server-resolved combat result. Status ticks take their color from the exact StatusDefinition Data
     * Asset carried by the event, and same-frame bursts may merge without coalescing successive periodic ticks.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|DamageNumbers")
    void AddResolvedCombatText(const FMythicResolvedCombatTextEvent &Event);

    /**
     * COMBAT-ONLY: float arbitrary combat status text off a world point (e.g. "Shield Broken!", "Winded!", "DODGE").
     * Non-combat feedback belongs in the UMG/MVVM HUD — this is deliberately not a generic floating-text hatch.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|DamageNumbers")
    void AddCombatText(FVector WorldLocation, const FString &Text, FLinearColor Color, float Lifetime = 1.0f);

    /**
     * Floats a DODGE callout using the configured DodgeColor. The authority dodge path invokes this separately because
     * a dodge negates the hit before ordinary damage feedback can execute.
     */
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

    FVector2D CalculateAnimationOffset(const FMythicDamageNumberData &Data, float CurrentTime) const;

    float CalculateAnimationScale(const FMythicDamageNumberData &Data, float CurrentTime) const;

    void CleanupExpired();

    void EnforceActiveBudget(int32 PendingEntries = 0);

    FMythicDamageNumberData *FindMergeCandidate(const FMythicResolvedCombatTextEvent &Event, float CurrentTime,
                                                const FVector &ResolvedWorldLocation);

    void ApplyRandomMotion(FMythicDamageNumberData &Data) const;

    void AddCombatTextInternal(FVector WorldLocation, const FString &Text, FLinearColor Color, float Lifetime,
                               EMythicDamageNumberAnimStyle AnimStyle);

protected:
    UPROPERTY()
    TArray<FMythicDamageNumberData> ActiveDamageNumbers;

    UPROPERTY()
    TObjectPtr<UMythicDamageNumberConfig> Config;

    UPROPERTY()
    TObjectPtr<UMythicStatusEffectLibrary> LoadedStatusEffectLibrary;

    int32 NextID = 0;

    FDelegateHandle HUDDrawDelegateHandle;
};
