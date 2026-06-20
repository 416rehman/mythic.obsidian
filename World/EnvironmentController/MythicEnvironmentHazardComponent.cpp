#include "World/EnvironmentController/MythicEnvironmentHazardComponent.h"

#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "Mythic.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"

UMythicEnvironmentHazardComponent::UMythicEnvironmentHazardComponent() {
    PrimaryComponentTick.bCanEverTick = false;
}

void UMythicEnvironmentHazardComponent::BeginPlay() {
    Super::BeginPlay();

    // Server-only: GEs are applied authority-side (and replicate down). The weather delegates fire on BOTH the
    // server and clients, so binding only on authority avoids double-application.
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }

    UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicEnvironmentSubsystem *EnvSubsystem = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
    if (!EnvSubsystem) {
        return;
    }

    // Bind now if the controller already exists, else catch it when it registers (mirrors UEnvironmentComponent).
    if (AMythicEnvironmentController *Controller = EnvSubsystem->GetEnvironmentController()) {
        OnEnvironmentControllerRegistered(Controller);
    }
    else {
        EnvSubsystem->OnEnvironmentControllerRegisterDelegate.AddUniqueDynamic(
            this, &UMythicEnvironmentHazardComponent::OnEnvironmentControllerRegistered);
    }
}

void UMythicEnvironmentHazardComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    // Remove every still-active hazard effect so nothing lingers on the (persistent) PlayerState ASC — but ONLY
    // if the current ASC is the one the handles were issued against (after a PlayerState swap the handles belong
    // to a different/destroyed ASC, so removing them on the new one would be wrong).
    if (UAbilitySystemComponent *ASC = ResolvePlayerASC()) {
        if (ASC == HandlesOwnerASC.Get()) {
            for (const TPair<int32, FActiveGameplayEffectHandle> &Pair : ActiveHazardHandles) {
                ASC->RemoveActiveGameplayEffect(Pair.Value);
            }
        }
    }
    ActiveHazardHandles.Empty();
    HandlesOwnerASC.Reset();

    if (AMythicEnvironmentController *Controller = BoundController.Get()) {
        Controller->WeatherChangeDelegate.RemoveDynamic(this, &UMythicEnvironmentHazardComponent::HandleWeatherChanged);
        Controller->DayTimeChangeDelegate.RemoveDynamic(this, &UMythicEnvironmentHazardComponent::HandleDaytimeChanged);
        Controller->MonthChangeDelegate.RemoveDynamic(this, &UMythicEnvironmentHazardComponent::HandleMonthChanged);
    }
    BoundController.Reset();

    Super::EndPlay(EndPlayReason);
}

void UMythicEnvironmentHazardComponent::OnEnvironmentControllerRegistered(AMythicEnvironmentController *Controller) {
    if (!Controller) {
        return;
    }
    BindController(Controller);
    // Catch-up: evaluate immediately so a player who joins mid-weather gets the right hazards.
    ReevaluateAll();
}

void UMythicEnvironmentHazardComponent::BindController(AMythicEnvironmentController *Controller) {
    BoundController = Controller;
    Controller->WeatherChangeDelegate.AddUniqueDynamic(this, &UMythicEnvironmentHazardComponent::HandleWeatherChanged);
    Controller->DayTimeChangeDelegate.AddUniqueDynamic(this, &UMythicEnvironmentHazardComponent::HandleDaytimeChanged);
    Controller->MonthChangeDelegate.AddUniqueDynamic(this, &UMythicEnvironmentHazardComponent::HandleMonthChanged);
}

void UMythicEnvironmentHazardComponent::HandleWeatherChanged(FGameplayTag, FGameplayTag) { ReevaluateAll(); }
void UMythicEnvironmentHazardComponent::HandleDaytimeChanged(EDayTime, EDayTime) { ReevaluateAll(); }
void UMythicEnvironmentHazardComponent::HandleMonthChanged(int32, int32, ESeason, ESeason) { ReevaluateAll(); }

void UMythicEnvironmentHazardComponent::ReevaluateAll() {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    UAbilitySystemComponent *ASC = ResolvePlayerASC();
    if (!ASC || !BoundController.IsValid()) {
        return;
    }

    // The ASC (on the PlayerState) may have been swapped under us (seamless travel) while this component (on the
    // PlayerController) survived. The old handles point at a different ASC — drop the bookkeeping WITHOUT removing
    // on the new ASC (the old one owns / has already discarded those effects), then re-apply from scratch below.
    if (HandlesOwnerASC.Get() != ASC) {
        ActiveHazardHandles.Empty();
        HandlesOwnerASC = ASC;
    }

    for (int32 i = 0; i < Conditions.Num(); ++i) {
        const FEnvironmentHazardCondition &Condition = Conditions[i];
        const bool bNowActive = EvaluateCondition(Condition);
        const bool bWasActive = ActiveHazardHandles.Contains(i);

        if (bNowActive && !bWasActive) {
            if (!Condition.HazardEffect) {
                continue;
            }
            FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
            Ctx.AddSourceObject(GetOwner());
            const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Condition.HazardEffect, Condition.EffectLevel, Ctx);
            if (Spec.IsValid()) {
                const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
                ActiveHazardHandles.Add(i, Handle);
            }
        }
        else if (!bNowActive && bWasActive) {
            ASC->RemoveActiveGameplayEffect(ActiveHazardHandles[i]);
            ActiveHazardHandles.Remove(i);
        }
        // Unchanged: leave as-is (no re-apply) — idempotent, no duplicate stacks.
    }
}

bool UMythicEnvironmentHazardComponent::EvaluateCondition(const FEnvironmentHazardCondition &Condition) const {
    const AMythicEnvironmentController *Controller = BoundController.Get();
    if (!Controller) {
        return false;
    }

    // Live world state, read from the real controller getters/helpers (single source of truth).
    const FGameplayTag LiveWeather = Controller->GetCurrentWeather() ? Controller->GetCurrentWeather()->Tag : FGameplayTag();
    const FTimespan Time = Controller->GetTimespan();
    const EDayTime LiveDayTime = HourAsDayTime(static_cast<uint8>(Time.GetHours()));
    const ESeason LiveSeason = MonthAsSeason(GetMonthOfYear(Time));

    // Weather axis: empty = unconstrained; else live weather must equal or be a child of a listed tag.
    if (Condition.WeatherTags.Num() > 0) {
        bool bMatch = false;
        for (const FGameplayTag &Tag : Condition.WeatherTags) {
            if (Tag.IsValid() && LiveWeather.MatchesTag(Tag)) {
                bMatch = true;
                break;
            }
        }
        if (!bMatch) {
            return false;
        }
    }

    // Season axis.
    if (Condition.Seasons.Num() > 0 && !Condition.Seasons.Contains(TEnumAsByte<ESeason>(LiveSeason))) {
        return false;
    }

    // Time-of-day axis.
    if (Condition.DayTimes.Num() > 0 && !Condition.DayTimes.Contains(TEnumAsByte<EDayTime>(LiveDayTime))) {
        return false;
    }

    return true;
}

UAbilitySystemComponent *UMythicEnvironmentHazardComponent::ResolvePlayerASC() const {
    if (IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner())) {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}
