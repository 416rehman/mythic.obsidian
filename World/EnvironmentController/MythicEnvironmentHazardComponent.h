// Mythic — Environmental Hazard Component
// Consumes the (already-simulated, replicated) weather/season/time backend: applies designer-authored
// GameplayEffects to the player when the world conditions match, and removes them when they no longer do.
// The weather state machine ran with zero gameplay consumers before this; this is the consumer.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "World/EnvironmentController/EnvironmentTypes.h"
#include "MythicEnvironmentHazardComponent.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class AMythicEnvironmentController;

/**
 * One designer-authored hazard rule: when the world matches ALL non-empty axes (weather AND season AND daytime),
 * HazardEffect is applied to the player at EffectLevel; it's removed when the rule stops matching. Empty axis =
 * unconstrained. All effect content (the GE, its magnitudes/duration/stacking, and the exact conditions) is
 * designer-authored — nothing is fabricated in code.
 */
USTRUCT(BlueprintType)
struct FEnvironmentHazardCondition {
    GENERATED_BODY()

    // Live weather matches if it equals or is a child of any listed tag (Environment.Weather.*). Empty = ignored.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (Categories = "Environment.Weather"))
    TArray<FGameplayTag> WeatherTags;

    // Seasons in which this rule is active (OR within the axis). Empty = ignored.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
    TArray<TEnumAsByte<ESeason>> Seasons;

    // Times of day in which this rule is active (OR within the axis). Empty = ignored.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
    TArray<TEnumAsByte<EDayTime>> DayTimes;

    // The GameplayEffect applied to the player while this rule holds (designer asset — e.g. GE_Cold).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
    TSubclassOf<UGameplayEffect> HazardEffect;

    // Level passed to the applied effect spec.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
    float EffectLevel = 1.0f;

    // Player-facing name announced when this hazard begins ("<Name>") and ends ("<Name> subsides"), e.g. "Freezing",
    // "Scorching Heat", "Toxic Fog". Empty = no callout (the GE, if any, still applies silently). Designer-authored —
    // nothing fabricated in code. Independent of HazardEffect: a feedback-only rule (name, no GE) or a silent rule
    // (GE, no name) are both valid.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
    FText DisplayName;
};

/**
 * Hosted on AMythicPlayerController (persistent across respawn; resolves the PlayerState ASC). Binds the existing
 * environment-controller weather/daytime/season delegates and, on any change, re-evaluates every rule against the
 * live world state, diffing against the set of currently-applied GE handles (apply newly-true, remove
 * newly-false). Server-authoritative — the GE replicates to the owning client via the PlayerState ASC.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicEnvironmentHazardComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicEnvironmentHazardComponent();

    // Designer-authored hazard rules.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment Hazard")
    TArray<FEnvironmentHazardCondition> Conditions;

    // Read-only debug view of the hazards CURRENTLY applied to the player: the rule index plus a readable label
    // (the rule's DisplayName when authored, else "Hazard[<idx>]"). Surfaces the private ActiveHazardHandles map
    // for the Living World gameplay debugger (Environment pane) without exposing the GE handles. Game thread.
    void GetActiveHazards(TArray<int32> &OutRuleIndices, TArray<FString> &OutLabels) const {
        OutRuleIndices.Reset();
        OutLabels.Reset();
        for (const TPair<int32, FActiveGameplayEffectHandle> &Pair : ActiveHazardHandles) {
            OutRuleIndices.Add(Pair.Key);
            const bool bNamed = Conditions.IsValidIndex(Pair.Key) && !Conditions[Pair.Key].DisplayName.IsEmpty();
            OutLabels.Add(bNamed ? Conditions[Pair.Key].DisplayName.ToString() : FString::Printf(TEXT("Hazard[%d]"), Pair.Key));
        }
    }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void OnEnvironmentControllerRegistered(AMythicEnvironmentController *Controller);

    // Delegate handlers — signatures MUST match the controller's delegates. They ignore the payload and just
    // re-evaluate against live state (single source of truth).
    UFUNCTION()
    void HandleWeatherChanged(FGameplayTag PreviousWeather, FGameplayTag NewWeather);

    UFUNCTION()
    void HandleDaytimeChanged(EDayTime PrevDayTime, EDayTime NewDayTime);

    UFUNCTION()
    void HandleMonthChanged(int32 PrevMonth, int32 NewMonth, ESeason PrevSeason, ESeason NewSeason);

private:
    void BindController(AMythicEnvironmentController *Controller);
    void ReevaluateAll();
    bool EvaluateCondition(const FEnvironmentHazardCondition &Condition) const;
    UAbilitySystemComponent *ResolvePlayerASC() const;

    // Float the onset / relief callout for a hazard over the owning player (no-op if the rule has no DisplayName, or
    // the owner isn't a player controller). Server-side; routes through the PC's Client RPC.
    void NotifyHazard(const FEnvironmentHazardCondition &Condition, bool bOnset) const;

    // Rule index -> the active GE handle currently applied for it (diff state; not replicated — the GE itself is).
    TMap<int32, FActiveGameplayEffectHandle> ActiveHazardHandles;

    // Rule indices the player has currently been ANNOUNCED (onset shown, relief pending). Tracked separately from
    // ActiveHazardHandles so a seamless-travel ASC swap — which clears/re-applies the GE handles without any real
    // world-state change — never re-announces a hazard the player already knows about. Drives the callouts only.
    TSet<int32> NotifiedConditions;

    // The ASC the current handles were issued against. The component lives on the (persistent) PlayerController
    // but the ASC lives on the PlayerState, which can be REPLACED (seamless travel). If the ASC identity changes,
    // the old handles are stale — we drop the map (without removing on the new ASC) and re-apply fresh.
    TWeakObjectPtr<UAbilitySystemComponent> HandlesOwnerASC;

    TWeakObjectPtr<AMythicEnvironmentController> BoundController;
};
