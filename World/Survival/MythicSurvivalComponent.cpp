#include "MythicSurvivalComponent.h"

#include "SurvivalCore.h"
#include "SurvivalTags.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Survival.h"
#include "Settings/MythicDeveloperSettings.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/EnvironmentController/EnvironmentTags.h"
#include "GAS/MythicTags_GAS.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerState.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Knowledge/MythicCodexComponent.h"
#include "Knowledge/MythicTags_Knowledge.h"

#define LOCTEXT_NAMESPACE "MythicSurvival"

UMythicSurvivalComponent::UMythicSurvivalComponent() {
    PrimaryComponentTick.bCanEverTick = false;
}

void UMythicSurvivalComponent::ClassifyWeather(const FGameplayTag &WeatherTag, bool &bOutCold, bool &bOutWet) {
    const bool bSnow = WeatherTag.IsValid() && WeatherTag.MatchesTag(Environment_Weather_Snow);
    const bool bRain = WeatherTag.IsValid() && WeatherTag.MatchesTag(Environment_Weather_Rain);
    bOutCold = bSnow;
    bOutWet = bSnow || bRain;
}

void UMythicSurvivalComponent::BeginPlay() {
    Super::BeginPlay();

    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bSurvivalNeedsEnabled) {
        return;
    }

    MappedStatuses.Reset();
    auto AddMapped = [this](uint8 Bit, const TSoftClassPtr<UGameplayEffect> &Soft, const FText &Name, const FGameplayTag &Term) {
        FMythicMappedSurvivalStatus M;
        M.Bit = Bit;
        M.Effect = Soft.IsNull() ? nullptr : Soft.LoadSynchronous();
        M.Name = Name;
        M.TermTag = Term;
        MappedStatuses.Add(M);
    };
    AddMapped(ESSB_Starving, Settings->SurvivalStarvingEffect, LOCTEXT("Starving", "Starving"), CODEX_TERM_STATUS_STARVING);
    AddMapped(ESSB_WellFed, Settings->SurvivalWellFedEffect, LOCTEXT("WellFed", "Well Fed"), CODEX_TERM_STATUS_WELLFED);
    AddMapped(ESSB_Dehydrated, Settings->SurvivalDehydratedEffect, LOCTEXT("Dehydrated", "Dehydrated"), CODEX_TERM_STATUS_DEHYDRATED);
    AddMapped(ESSB_Cold, Settings->SurvivalColdEffect, LOCTEXT("Cold", "Cold"), CODEX_TERM_STATUS_COLD);

    const float Interval = FMath::Max(0.05f, Settings->SurvivalTickInterval);
    GetWorld()->GetTimerManager().SetTimer(SurvivalTimerHandle, this, &UMythicSurvivalComponent::ServerSurvivalTick, Interval,true);
}

void UMythicSurvivalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(SurvivalTimerHandle);
    }
    if (UAbilitySystemComponent *ASC = ResolvePlayerASC()) {
        if (ASC == HandlesOwnerASC.Get()) {
            for (const TPair<uint8, FActiveGameplayEffectHandle> &Pair : ActiveStatusHandles) {
                ASC->RemoveActiveGameplayEffect(Pair.Value);
            }
        }
    }
    ActiveStatusHandles.Empty();
    HandlesOwnerASC.Reset();
    NotifiedStatuses.Empty();
    ActiveStatusMask = 0;

    Super::EndPlay(EndPlayReason);
}

void UMythicSurvivalComponent::ServerSurvivalTick() {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    UAbilitySystemComponent *ASC = ResolvePlayerASC();
    const UMythicAttributeSet_Survival *Set = ASC ? ASC->GetSet<UMythicAttributeSet_Survival>() : nullptr;
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();

    const bool bDead = ASC && ASC->HasMatchingGameplayTag(GAS_STATE_DEAD);
    const bool bMaster = Settings && Settings->bSurvivalNeedsEnabled;
    if (!FMythicSurvivalCore::IsSurvivalActive(bMaster, Set != nullptr, bDead)) {
        ClearActiveStatusEffects(ASC);
        return;
    }

    const float Dt = FMath::Max(0.05f, Settings->SurvivalTickInterval);

    const float NewNourishment = FMythicSurvivalCore::ComputeDecayStep(
        Set->GetNourishment(), Settings->NourishmentDecayPerSecond, Dt, Set->GetMaxNourishment());
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Survival::GetNourishmentAttribute(), NewNourishment);

    const float NewHydration = FMythicSurvivalCore::ComputeDecayStep(
        Set->GetHydration(), Settings->HydrationDecayPerSecond, Dt, Set->GetMaxHydration());
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Survival::GetHydrationAttribute(), NewHydration);

    bool bColdWeather = false, bWetWeather = false;
    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        if (const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
            ClassifyWeather(Env->GetWeather(), bColdWeather, bWetWeather);
        }
    }
    const bool bWarmSource = ASC->HasMatchingGameplayTag(TAG_Status_Warm);
    const bool bSheltered = ASC->HasMatchingGameplayTag(TAG_Status_Sheltered);

    FWarmthWetnessRates Rates;
    Rates.WarmSourceWarmthPerSecond = Settings->WarmSourceWarmthPerSecond;
    Rates.ColdWeatherWarmthPerSecond = Settings->ColdWeatherWarmthPerSecond;
    Rates.WetChillWarmthPerSecond = Settings->WetChillWarmthPerSecond;
    Rates.PassiveWarmthRegenPerSecond = Settings->PassiveWarmthRegenPerSecond;
    Rates.WettingPerSecond = Settings->WettingPerSecond;
    Rates.DryingPerSecond = Settings->DryingPerSecond;
    Rates.MaxWarmth = Set->GetMaxWarmth();
    Rates.MaxWetness = Set->GetMaxWetness();
    Rates.NeutralWarmth = FMath::Clamp(Settings->NeutralWarmthFraction, 0.0f, 1.0f) * Set->GetMaxWarmth();

    const FSurvivalWarmthWetnessResult Net = FMythicSurvivalCore::ComputeWarmthWetnessNet(
        bWarmSource, bSheltered, bColdWeather, bWetWeather, Set->GetWarmth(), Set->GetWetness(), Rates, Dt);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Survival::GetWarmthAttribute(), Set->GetWarmth() + Net.NetWarmthDelta);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Survival::GetWetnessAttribute(), Set->GetWetness() + Net.NetWetnessDelta);

    FSurvivalThresholds T;
    const float Band = FMath::Max(0.0f, Settings->SurvivalHysteresisBand);
    T.StarvingEnter = Settings->StarvingThreshold;
    T.StarvingExit = Settings->StarvingThreshold + Band;
    T.WellFedEnter = Settings->WellFedThreshold;
    T.WellFedExit = FMath::Max(0.0f, Settings->WellFedThreshold - Band);
    T.DehydratedEnter = Settings->DehydratedThreshold;
    T.DehydratedExit = Settings->DehydratedThreshold + Band;
    T.ColdEnter = Settings->ColdThreshold;
    T.ColdExit = Settings->ColdThreshold + Band;
    T.WetColdAggravation = Settings->WetColdAggravation;

    auto SafeFrac = [](float Cur, float Max) { return Max > 0.0f ? FMath::Clamp(Cur / Max, 0.0f, 1.0f) : 0.0f; };
    const uint8 NewMask = FMythicSurvivalCore::ResolveStatus(
        SafeFrac(Set->GetNourishment(), Set->GetMaxNourishment()),
        SafeFrac(Set->GetHydration(), Set->GetMaxHydration()),
        SafeFrac(Set->GetWarmth(), Set->GetMaxWarmth()),
        SafeFrac(Set->GetWetness(), Set->GetMaxWetness()),
        T, ActiveStatusMask);
    ActiveStatusMask = NewMask;

    ApplyStatusDiff(ASC, NewMask);
}

void UMythicSurvivalComponent::ApplyStatusDiff(UAbilitySystemComponent *ASC, uint8 NewMask) {
    if (!ASC) {
        return;
    }
    if (HandlesOwnerASC.Get() != ASC) {
        ActiveStatusHandles.Empty();
        HandlesOwnerASC = ASC;
    }

    for (const FMythicMappedSurvivalStatus &Mapped : MappedStatuses) {
        const bool bNowActive = (NewMask & Mapped.Bit) != 0;
        const bool bWasActive = ActiveStatusHandles.Contains(Mapped.Bit);

        if (bNowActive && !bWasActive) {
            if (Mapped.Effect) {
                FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
                Ctx.AddSourceObject(GetOwner());
                const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Mapped.Effect, 1.0f, Ctx);
                if (Spec.IsValid()) {
                    const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
                    ActiveStatusHandles.Add(Mapped.Bit, Handle);
                }
            }
        }
        else if (!bNowActive && bWasActive) {
            ASC->RemoveActiveGameplayEffect(ActiveStatusHandles[Mapped.Bit]);
            ActiveStatusHandles.Remove(Mapped.Bit);
        }

        const bool bNotified = NotifiedStatuses.Contains(Mapped.Bit);
        if (bNowActive && !bNotified) {
            NotifiedStatuses.Add(Mapped.Bit);
            NotifyStatus(Mapped.Name,true);
            if (Mapped.TermTag.IsValid()) {
                if (const AMythicPlayerState *MythPS = Cast<AMythicPlayerState>(GetOwner())) {
                    if (UMythicCodexComponent *CodexComp = MythPS->GetCodexComponent()) {
                        CodexComp->ServerDiscoverTerm(Mapped.TermTag);
                    }
                }
            }
        }
        else if (!bNowActive && bNotified) {
            NotifiedStatuses.Remove(Mapped.Bit);
            NotifyStatus(Mapped.Name,false);
        }
    }
}

void UMythicSurvivalComponent::ClearActiveStatusEffects(UAbilitySystemComponent *ASC) {
    if (ASC && ASC == HandlesOwnerASC.Get()) {
        for (const TPair<uint8, FActiveGameplayEffectHandle> &Pair : ActiveStatusHandles) {
            ASC->RemoveActiveGameplayEffect(Pair.Value);
        }
    }
    ActiveStatusHandles.Empty();
    NotifiedStatuses.Empty();
    ActiveStatusMask = 0;
}

void UMythicSurvivalComponent::NotifyStatus(const FText &StatusName, bool bOnset) const {
    if (StatusName.IsEmpty()) {
        return;
    }
    if (const APlayerState *PS = Cast<APlayerState>(GetOwner())) {
        if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(PS->GetPlayerController())) {
            PC->ClientNotifyEnvironmentHazard(StatusName, bOnset);
        }
    }
}

UAbilitySystemComponent *UMythicSurvivalComponent::ResolvePlayerASC() const {
    if (IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner())) {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}

#undef LOCTEXT_NAMESPACE
