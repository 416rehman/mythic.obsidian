
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

    // Counter-play: if the player's ASC has ANY of these tags (hierarchical), this hazard is SUPPRESSED — it won't apply,
    // and an already-applied one is removed. Empty (default) = no suppression (the hazard always applies when its axes
    // match — byte-identical to the prior behaviour). The tag SOURCE is designer content (e.g. a campfire aura GE grants
    // "Status.Warm", warm clothing grants it via an affix, being indoors grants "Status.Sheltered") — this is the GATE.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (Categories = "Status"))
    TArray<FGameplayTag> SuppressionTags;

    // Player-facing name announced when this hazard begins ("<Name>") and ends ("<Name> subsides"), e.g. "Freezing",
    // "Scorching Heat", "Toxic Fog". Empty = no callout (the GE, if any, still applies silently). Designer-authored —
    // nothing fabricated in code. Independent of HazardEffect: a feedback-only rule (name, no GE) or a silent rule
    // (GE, no name) are both valid.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
    FText DisplayName;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicEnvironmentHazardComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicEnvironmentHazardComponent();

    // Designer-authored hazard rules.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment Hazard")
    TArray<FEnvironmentHazardCondition> Conditions;

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

    UFUNCTION()
    void HandleWeatherChanged(FGameplayTag PreviousWeather, FGameplayTag NewWeather);

    UFUNCTION()
    void HandleDaytimeChanged(EDayTime PrevDayTime, EDayTime NewDayTime);

    UFUNCTION()
    void HandleMonthChanged(int32 PrevMonth, int32 NewMonth, ESeason PrevSeason, ESeason NewSeason);

    void OnSuppressionTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

public:
    static bool IsHazardSuppressed(const FGameplayTagContainer &PlayerOwnedTags, const TArray<FGameplayTag> &SuppressionTags);

private:
    void BindController(AMythicEnvironmentController *Controller);
    void ReevaluateAll();
    void ReevaluateAllOnce();
    bool EvaluateCondition(const FEnvironmentHazardCondition &Condition, const FGameplayTagContainer &PlayerOwnedTags) const;
    UAbilitySystemComponent *ResolvePlayerASC() const;

    bool bReevaluating = false;
    bool bReevaluatePending = false;

    void RebindSuppressionTags(UAbilitySystemComponent *ASC);
    void UnbindSuppressionTags();

    void NotifyHazard(const FEnvironmentHazardCondition &Condition, bool bOnset) const;

    TMap<int32, FActiveGameplayEffectHandle> ActiveHazardHandles;

    TSet<int32> NotifiedConditions;

    TWeakObjectPtr<UAbilitySystemComponent> HandlesOwnerASC;

    TWeakObjectPtr<UAbilitySystemComponent> SuppressionBoundASC;

    TArray<FGameplayTag> BoundSuppressionTags;

    TWeakObjectPtr<AMythicEnvironmentController> BoundController;
};
