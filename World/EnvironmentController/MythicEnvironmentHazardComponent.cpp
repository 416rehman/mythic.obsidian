#include "World/EnvironmentController/MythicEnvironmentHazardComponent.h"

#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "Mythic.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Player/MythicPlayerController.h"

UMythicEnvironmentHazardComponent::UMythicEnvironmentHazardComponent() {
    PrimaryComponentTick.bCanEverTick = false;
}

void UMythicEnvironmentHazardComponent::BeginPlay() {
    Super::BeginPlay();

    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }

    UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicEnvironmentSubsystem *EnvSubsystem = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
    if (!EnvSubsystem) {
        return;
    }

    if (AMythicEnvironmentController *Controller = EnvSubsystem->GetEnvironmentController()) {
        OnEnvironmentControllerRegistered(Controller);
    }
    else {
        EnvSubsystem->OnEnvironmentControllerRegisterDelegate.AddUniqueDynamic(
            this, &UMythicEnvironmentHazardComponent::OnEnvironmentControllerRegistered);
    }
}

void UMythicEnvironmentHazardComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UAbilitySystemComponent *ASC = ResolvePlayerASC()) {
        if (ASC == HandlesOwnerASC.Get()) {
            for (const TPair<int32, FActiveGameplayEffectHandle> &Pair : ActiveHazardHandles) {
                ASC->RemoveActiveGameplayEffect(Pair.Value);
            }
        }
    }
    ActiveHazardHandles.Empty();
    HandlesOwnerASC.Reset();
    NotifiedConditions.Empty();

    UnbindSuppressionTags();

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
    if (bReevaluating) {
        bReevaluatePending = true;
        return;
    }
    bReevaluating = true;
    int32 PassGuard = 0;
    do {
        bReevaluatePending = false;
        ReevaluateAllOnce();
    } while (bReevaluatePending && ++PassGuard < 8);
    bReevaluating = false;
}

void UMythicEnvironmentHazardComponent::ReevaluateAllOnce() {
    UAbilitySystemComponent *ASC = ResolvePlayerASC();
    if (!ASC || !BoundController.IsValid()) {
        return;
    }

    if (HandlesOwnerASC.Get() != ASC) {
        ActiveHazardHandles.Empty();
        HandlesOwnerASC = ASC;
    }

    RebindSuppressionTags(ASC);

    FGameplayTagContainer OwnedTags;
    ASC->GetOwnedGameplayTags(OwnedTags);

    for (int32 i = 0; i < Conditions.Num(); ++i) {
        const FEnvironmentHazardCondition &Condition = Conditions[i];
        const bool bNowActive = EvaluateCondition(Condition, OwnedTags);
        const bool bWasActive = ActiveHazardHandles.Contains(i);

        if (bNowActive && !bWasActive) {
            if (Condition.HazardEffect) {
                FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
                Ctx.AddSourceObject(GetOwner());
                const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Condition.HazardEffect, Condition.EffectLevel, Ctx);
                if (Spec.IsValid()) {
                    const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
                    ActiveHazardHandles.Add(i, Handle);
                }
            }
        }
        else if (!bNowActive && bWasActive) {
            ASC->RemoveActiveGameplayEffect(ActiveHazardHandles[i]);
            ActiveHazardHandles.Remove(i);
        }

        const bool bNotified = NotifiedConditions.Contains(i);
        if (bNowActive && !bNotified) {
            NotifiedConditions.Add(i);
            NotifyHazard(Condition,true);
        }
        else if (!bNowActive && bNotified) {
            NotifiedConditions.Remove(i);
            NotifyHazard(Condition,false);
        }
    }
}

void UMythicEnvironmentHazardComponent::NotifyHazard(const FEnvironmentHazardCondition &Condition, bool bOnset) const {
    if (Condition.DisplayName.IsEmpty()) {
        return;
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwner())) {
        PC->ClientNotifyEnvironmentHazard(Condition.DisplayName, bOnset);
    }
}

bool UMythicEnvironmentHazardComponent::EvaluateCondition(const FEnvironmentHazardCondition &Condition,
                                                          const FGameplayTagContainer &PlayerOwnedTags) const {
    const AMythicEnvironmentController *Controller = BoundController.Get();
    if (!Controller) {
        return false;
    }

    const FGameplayTag LiveWeather = Controller->GetCurrentWeather() ? Controller->GetCurrentWeather()->Tag : FGameplayTag();
    const FTimespan Time = Controller->GetTimespan();
    const EDayTime LiveDayTime = HourAsDayTime(static_cast<uint8>(Time.GetHours()));
    const ESeason LiveSeason = MonthAsSeason(GetMonthOfYear(Time));

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

    if (Condition.Seasons.Num() > 0 && !Condition.Seasons.Contains(TEnumAsByte<ESeason>(LiveSeason))) {
        return false;
    }

    if (Condition.DayTimes.Num() > 0 && !Condition.DayTimes.Contains(TEnumAsByte<EDayTime>(LiveDayTime))) {
        return false;
    }

    if (Condition.SuppressionTags.Num() > 0 && IsHazardSuppressed(PlayerOwnedTags, Condition.SuppressionTags)) {
        return false;
    }

    return true;
}

bool UMythicEnvironmentHazardComponent::IsHazardSuppressed(const FGameplayTagContainer &PlayerOwnedTags,
                                                          const TArray<FGameplayTag> &SuppressionTags) {
    for (const FGameplayTag &Tag : SuppressionTags) {
        if (Tag.IsValid() && PlayerOwnedTags.HasTag(Tag)) {
            return true;
        }
    }
    return false;
}

void UMythicEnvironmentHazardComponent::OnSuppressionTagChanged(const FGameplayTag, int32) {
    ReevaluateAll();
}

void UMythicEnvironmentHazardComponent::RebindSuppressionTags(UAbilitySystemComponent *ASC) {
    if (SuppressionBoundASC.Get() == ASC) {
        return;
    }
    UnbindSuppressionTags();

    if (!ASC) {
        return;
    }
    for (const FEnvironmentHazardCondition &Condition : Conditions) {
        for (const FGameplayTag &Tag : Condition.SuppressionTags) {
            if (Tag.IsValid() && !BoundSuppressionTags.Contains(Tag)) {
                ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
                   .AddUObject(this, &UMythicEnvironmentHazardComponent::OnSuppressionTagChanged);
                BoundSuppressionTags.Add(Tag);
            }
        }
    }
    SuppressionBoundASC = ASC;
}

void UMythicEnvironmentHazardComponent::UnbindSuppressionTags() {
    if (UAbilitySystemComponent *ASC = SuppressionBoundASC.Get()) {
        for (const FGameplayTag &Tag : BoundSuppressionTags) {
            ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
        }
    }
    BoundSuppressionTags.Reset();
    SuppressionBoundASC.Reset();
}

UAbilitySystemComponent *UMythicEnvironmentHazardComponent::ResolvePlayerASC() const {
    if (IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner())) {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}
