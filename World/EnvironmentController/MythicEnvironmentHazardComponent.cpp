#include "World/EnvironmentController/MythicEnvironmentHazardComponent.h"

#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "Mythic.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Player/MythicPlayerController.h" // hazard onset/relief callout (the component is hosted on the PC)

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
    NotifiedConditions.Empty(); // no relief callout on teardown/logout — this is not a real world-state relief

    // Drop the suppression-tag listeners off the (still-valid) ASC so this destroyed component isn't called back.
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
    // Re-entrancy guard + coalescing. ApplyGameplayEffectSpecToSelf / RemoveActiveGameplayEffect inside the diff fire the
    // ASC's tag-change delegates SYNCHRONOUSLY, and OnSuppressionTagChanged re-calls ReevaluateAll. A nested call mid-loop
    // would corrupt the handle bookkeeping (the apply->record pair isn't yet complete → a leaked, untracked GE handle on
    // the persistent PlayerState ASC; an unbounded recurse for a self-toggling suppressor). So a re-entrant call only FLAGS
    // a re-run and returns; the outer call re-runs the FULL diff until stable. Each pass thus completes atomically. The
    // pass cap is a backstop against a content error (a hazard GE that grants its OWN suppressor tag → genuine oscillation).
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

    // The ASC (on the PlayerState) may have been swapped under us (seamless travel) while this component (on the
    // PlayerController) survived. The old handles point at a different ASC — drop the bookkeeping WITHOUT removing
    // on the new ASC (the old one owns / has already discarded those effects), then re-apply from scratch below.
    if (HandlesOwnerASC.Get() != ASC) {
        ActiveHazardHandles.Empty();
        HandlesOwnerASC = ASC;
    }

    // Keep the suppression-tag listeners bound to the live ASC (idempotent; rebinds on a swap) so a campfire/clothing
    // toggle re-evaluates hazards immediately. Must run on the same authority + resolved-ASC path as the GE handles.
    RebindSuppressionTags(ASC);

    // Snapshot the player's owned tags ONCE for this pass (not per rule) — the suppression gate reads it. A cross-rule
    // tag change made by a GE applied below fires the bound listener → a coalesced re-run picks up the fresh snapshot.
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
        // Unchanged: leave as-is (no re-apply) — idempotent, no duplicate stacks.

        // Player-facing perception, tracked against NotifiedConditions (NOT the GE handle map) so an ASC swap that
        // re-applies handles without a real world-state change can't re-announce. Driven purely by the matched world
        // state + the rule's DisplayName, so a feedback-only rule (no GE) still announces and a silent rule (no name)
        // stays quiet (NotifyHazard no-ops on an empty name).
        const bool bNotified = NotifiedConditions.Contains(i);
        if (bNowActive && !bNotified) {
            NotifiedConditions.Add(i);
            NotifyHazard(Condition, /*bOnset=*/true);
        }
        else if (!bNowActive && bNotified) {
            NotifiedConditions.Remove(i);
            NotifyHazard(Condition, /*bOnset=*/false);
        }
    }
}

void UMythicEnvironmentHazardComponent::NotifyHazard(const FEnvironmentHazardCondition &Condition, bool bOnset) const {
    if (Condition.DisplayName.IsEmpty()) {
        return; // unnamed hazard -> no callout (the GE, if any, still applies)
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

    // Counter-play: the world axes match, but a sheltered/warm player suppresses the hazard. Checked LAST (the rarest
    // gate). PlayerOwnedTags was snapshotted once by the caller; OnSuppressionTagChanged re-evaluates when these tags
    // come/go, so the hazard lifts/returns immediately. The tag SOURCE (campfire aura / warm clothing / indoors) is content.
    if (Condition.SuppressionTags.Num() > 0 && IsHazardSuppressed(PlayerOwnedTags, Condition.SuppressionTags)) {
        return false;
    }

    return true;
}

bool UMythicEnvironmentHazardComponent::IsHazardSuppressed(const FGameplayTagContainer &PlayerOwnedTags,
                                                          const TArray<FGameplayTag> &SuppressionTags) {
    for (const FGameplayTag &Tag : SuppressionTags) {
        if (Tag.IsValid() && PlayerOwnedTags.HasTag(Tag)) {
            return true; // owns this suppressor (or a child of it) — hazard suppressed
        }
    }
    return false;
}

void UMythicEnvironmentHazardComponent::OnSuppressionTagChanged(const FGameplayTag, int32) {
    // A suppressor came or went on the player — re-diff every rule so the affected hazard lifts/returns now.
    ReevaluateAll();
}

void UMythicEnvironmentHazardComponent::RebindSuppressionTags(UAbilitySystemComponent *ASC) {
    if (SuppressionBoundASC.Get() == ASC) {
        return; // already bound to this ASC; the Conditions' tag set is fixed at runtime, so nothing to do
    }
    UnbindSuppressionTags(); // drop bindings on the previous ASC (seamless-travel swap), if any

    if (!ASC) {
        return;
    }
    // Bind each DISTINCT valid suppression tag across all rules exactly once.
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
