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
class UTexture2D;

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
    // Status-effect hits — each gets a distinct config-driven color so the player reads the effect at a glance.
    // Mirrors the FMythicGameplayEffectContext status flags (set in DamageCalculation, resistance-gated in
    // DamageApplication, replicated via NetSerialize), so this is pure consumption of a built+replicated contract.
    Bleed,
    Burn,
    Poison,
    Stun,
    Slow,
    Weaken,
    Freeze,
    Terrify,
    // A dodged (missed) attack. The enum/color/anim land here; spawning the "DODGE" pop is a follow-up slice
    // (the dodge path early-returns in DamageApplication before the damage cue fires).
    Dodge,
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

/**
 * Screen-space notification kinds for the unified feedback system.
 * Toast = minor non-combat beat (stacked HUD list); Banner = major beat (animated center-screen hero banner, added in a
 * follow-up slice). Both are HUD-anchored, NOT world-projected.
 */
UENUM(BlueprintType)
enum class EMythicScreenNotifyKind : uint8 {
    Toast,
    Banner,
};

/**
 * One screen-space notification (toast or hero banner). HUD-anchored — used for non-combat feedback (loot, quest, party,
 * ...) that is about the player, not a world point. Plain struct for cache-friendly iteration (same rationale as the
 * damage numbers).
 */
USTRUCT()
struct FMythicScreenNotification {
    GENERATED_BODY()

    // Label, cached as FText at creation (not per-frame) for draw performance.
    UPROPERTY()
    FText CachedText;

    // Optional second line, used by hero banners (e.g. "Reached level 12"). Empty for toasts.
    UPROPERTY()
    FText Subtitle;

    // Optional icon drawn to the left of the label (null = text only).
    UPROPERTY()
    TObjectPtr<UTexture2D> Icon = nullptr;

    // Tint for the label (and icon, if a white/mask texture).
    UPROPERTY()
    FLinearColor Color = FLinearColor::White;

    // World time this notification was spawned.
    UPROPERTY()
    float SpawnTime = 0.0f;

    // Seconds the notification is visible.
    UPROPERTY()
    float Lifetime = 3.0f;

    // Unique id.
    UPROPERTY()
    int32 ID = 0;

    // Toast (stacked) vs Banner (animated hero).
    UPROPERTY()
    EMythicScreenNotifyKind Kind = EMythicScreenNotifyKind::Toast;

    bool IsExpired(float CurrentTime) const {
        return (CurrentTime - SpawnTime) >= Lifetime;
    }
};

/**
 * One world-anchored NON-COMBAT callout (gather "3 left" / "Depleted" at the node, a hazard at its source). Projected
 * above a world point and drifting up — calmer than a combat damage number, no background panel. Plain struct for
 * cache-friendly iteration.
 */
USTRUCT()
struct FMythicWorldCallout {
    GENERATED_BODY()

    UPROPERTY()
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY()
    FText CachedText;

    UPROPERTY()
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY()
    FLinearColor Color = FLinearColor::White;

    UPROPERTY()
    float SpawnTime = 0.0f;

    UPROPERTY()
    float Lifetime = 2.0f;

    UPROPERTY()
    int32 ID = 0;

    bool IsExpired(float CurrentTime) const {
        return (CurrentTime - SpawnTime) >= Lifetime;
    }
};

/**
 * Per-entity nameplate render state — holds the SMOOTHED alpha so a nameplate fades in/out instead of popping when an
 * entity becomes (ir)relevant. Plain struct (NOT a UPROPERTY): the TWeakObjectPtr auto-nulls and must NOT keep the actor
 * alive; this is purely transient render bookkeeping rebuilt each frame.
 */
struct FMythicNameplateState {
    TWeakObjectPtr<AActor> Actor;
    float CurrentAlpha = 0.0f;
};

/**
 * UMythicDamageNumberSubsystem
 *
 * The unified, high-performance, NON-WBP feedback subsystem. Combat damage numbers (world-projected, floating off the
 * target) AND non-combat screen notifications (toasts / animated hero banners) are rendered in ONE immediate-mode Canvas
 * pass via AHUD::OnHUDPostRender, over pooled flat arrays. (Pending a mechanical rename to UMythicFeedbackSubsystem now
 * that the former UMythicWorldFeedbackSubsystem is fully merged in.)
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
     * COMBAT-ONLY: float arbitrary combat status text off a world point (e.g. "Shield Broken!", "Winded!", "DODGE").
     * Non-combat feedback must use AddScreenToast / AddScreenBanner / AddWorldCallout instead — this is deliberately not
     * a generic floating-text hatch.
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

    // ─── Screen-space notifications (non-combat) ───────────────────────────────────────────────────────────────────

    /**
     * Queue a screen-space TOAST (minor non-combat beat: loot, trade, durability, party). HUD-anchored stacked list,
     * NOT world-projected. Icon optional (null = text only). DurationOverride <= 0 uses a sensible default.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Feedback")
    void AddScreenToast(const FText &Text, FLinearColor Color, UTexture2D *Icon = nullptr, float DurationOverride = 0.0f);

    /**
     * Queue a screen-space HERO BANNER (major non-combat beat: level-up, objective complete, zone entry, faction shift).
     * Center-screen, procedurally animated (slide-in + scale-overshoot pop + fade + entrance specular sweep). Subtitle +
     * icon optional. DurationOverride <= 0 uses a sensible default.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Feedback")
    void AddScreenBanner(const FText &Title, const FText &Subtitle, FLinearColor AccentColor, UTexture2D *Icon = nullptr, float DurationOverride = 0.0f);

    /**
     * Queue a world-anchored NON-COMBAT callout (gather / hazard) — projected above WorldLocation, drifting up and fading.
     * Calmer than a combat number (no background panel). Icon optional; DurationOverride <= 0 uses a sensible default.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Feedback")
    void AddWorldCallout(FVector WorldLocation, const FText &Text, FLinearColor Color, UTexture2D *Icon = nullptr, float DurationOverride = 0.0f);

    // ─── Pure presentation math for screen notifications (public + static so it is unit-testable) ──────────────────

    /** Toast opacity over its life: ramp 0->1 over FadeInTime, hold 1, ramp 1->0 over FadeOutTime; clamp [0,1]; Lifetime<=0 -> 0. */
    static float ComputeToastAlpha(float Elapsed, float Lifetime, float FadeInTime, float FadeOutTime);

    /** Remaining horizontal slide-in distance, easing SlideDistance -> 0 over FadeInTime (cubic ease-out); 0 once settled. */
    static float ComputeToastSlideOffset(float Elapsed, float FadeInTime, float SlideDistance);

    /** Vertical pixel offset of a toast at stack slot SlotFromAnchor (0 = nearest the anchor edge); negative slots clamp to 0. */
    static float ComputeToastStackOffset(int32 SlotFromAnchor, float EntryStep);

    /** Cubic "ease-out-back" curve: 0 -> 1 with a slight overshoot past 1 before settling. EaseOutBack(0)=0, EaseOutBack(1)=1. */
    static float EaseOutBack(float T);

    /** Banner scale over its entrance: StartScale -> 1.0 with an overshoot pop (ease-out-back) across EntranceTime; 1.0 after. */
    static float ComputeBannerScale(float Elapsed, float EntranceTime, float StartScale);

    /** Horizontal position (0 -> Width) of the entrance specular sweep across EntranceTime; clamps to Width once done. */
    static float ComputeBannerSweepX(float Elapsed, float EntranceTime, float Width);

    // ─── Contextual nameplates (health bars shown ONLY for engaged entities; relevance-driven, smoothly faded) ──────

    /** Target nameplate opacity: 0 if not relevant or beyond CullDistance; 1 within FullDistance; linear fade between. */
    static float ComputeNameplateTargetAlpha(bool bRelevant, float Distance, float FullDistance, float CullDistance);

    /** Step CurrentAlpha toward TargetAlpha by FadeRate*DeltaSeconds without overshoot (the smooth fade in/out). */
    static float StepNameplateAlpha(float CurrentAlpha, float TargetAlpha, float DeltaSeconds, float FadeRate);

    /**
     * Called by HUD to draw all damage numbers. Do not call directly.
     */
    void DrawDamageNumbers(UCanvas *Canvas, APlayerController *PC);

    // Number of damage numbers currently alive in the pool (exposes the protected ActiveDamageNumbers size only).
    // For the Living World gameplay debugger header.
    int32 GetActiveDamageNumberCount() const { return ActiveDamageNumbers.Num(); }

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

    // Removes expired entries from the pool (both the damage numbers and the screen notifications).
    void CleanupExpired();

    // Draws the screen-space notifications (toasts/banners). Called from OnHUDPostRender after the damage numbers.
    void DrawScreenNotifications(UCanvas *Canvas, APlayerController *PC);

    // Draws the world-anchored non-combat callouts (gather/hazard). Called from OnHUDPostRender.
    void DrawWorldCallouts(UCanvas *Canvas, APlayerController *PC);

    // Draws contextual nameplates (engaged-only health bars, faded by relevance). Called from OnHUDPostRender.
    void DrawNameplates(UCanvas *Canvas, APlayerController *PC);

    // True if this NPC is currently ENGAGED with the local player (its AI is fighting us) — drives nameplate visibility.
    bool IsNameplateRelevant(AActor *Npc, AActor *LocalPawn) const;

protected:
    // Active damage numbers
    UPROPERTY()
    TArray<FMythicDamageNumberData> ActiveDamageNumbers;

    // Active screen-space notifications (non-combat toasts / hero banners).
    UPROPERTY()
    TArray<FMythicScreenNotification> ActiveNotifications;

    // Active world-anchored non-combat callouts (gather/hazard).
    UPROPERTY()
    TArray<FMythicWorldCallout> ActiveWorldCallouts;

    // Per-entity nameplate render state (smoothed alpha). Transient; NOT a UPROPERTY (TWeakObjectPtr auto-nulls).
    TArray<FMythicNameplateState> NameplateStates;

    // World time of the previous nameplate draw, for the per-frame fade delta (-1 = not drawn yet).
    float LastNameplateTime = -1.0f;

    // Configuration
    UPROPERTY()
    TObjectPtr<UMythicDamageNumberConfig> Config;

    // ID counter for unique damage number IDs
    int32 NextID = 0;

    // Delegate handle for HUD drawing
    FDelegateHandle HUDDrawDelegateHandle;
};
