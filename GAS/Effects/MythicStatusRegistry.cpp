
#include "MythicStatusRegistry.h"
#include "GAS/MythicStatDiminishing.h"
#include "Settings/MythicCombatSettings.h"

#include "Mythic.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Abilities/MythicAbilityRollSource.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicStatContribution.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "Knowledge/MythicCodexComponent.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Settings/MythicDeveloperSettings.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_STATUS_DAMAGE, "SetByCaller.Status.Damage",
                               "Per-tick damage handed to an authored status effect");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_STATUS_DURATION, "SetByCaller.Status.Duration",
                               "Seconds handed to an authored status effect");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_STATUS_CONTROL_MAGNITUDE, "SetByCaller.Status.ControlMagnitude",
                               "Rolled control strength (slow/weaken/terrify fraction) handed to a control status effect");

void UMythicStatusRegistry::BuildIndex() {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || Settings->StatusEffectLibrary.IsNull()) {
        UE_LOG(Myth, Warning, TEXT("StatusRegistry: no StatusEffectLibrary configured — no status effect can be applied."));
        return;
    }

    Library = Settings->StatusEffectLibrary.Get();
    if (!Library) {
        Library = Settings->StatusEffectLibrary.LoadSynchronous();
    }
    if (!Library) {
        UE_LOG(Myth, Warning, TEXT("StatusRegistry: StatusEffectLibrary failed to load (%s)"), *Settings->StatusEffectLibrary.ToString());
        return;
    }

    bIndexed = true;

    for (UMythicStatusEffectDefinition *Definition : Library->Statuses) {
        if (!Definition) {
            continue;
        }
        if (!Definition->StatusType.IsValid()) {
            UE_LOG(Myth, Warning, TEXT("StatusRegistry: %s has no StatusType tag and was skipped."), *Definition->GetName());
            continue;
        }
        if (const TObjectPtr<UMythicStatusEffectDefinition> *Existing = StatusByType.Find(Definition->StatusType)) {
            UE_LOG(Myth, Warning, TEXT("StatusRegistry: %s and %s both claim %s — keeping the first."),
                   *(*Existing)->GetName(), *Definition->GetName(), *Definition->StatusType.ToString());
            continue;
        }
        StatusByType.Add(Definition->StatusType, Definition);
    }
}

UMythicStatusEffectDefinition *UMythicStatusRegistry::FindStatus(FGameplayTag StatusType) const {
    EnsureIndexed();
    const TObjectPtr<UMythicStatusEffectDefinition> *Found = StatusByType.Find(StatusType);
    return Found ? *Found : nullptr;
}

UMythicStatusEffectDefinition *UMythicStatusRegistry::FindStatusByBuildupAttribute(const FGameplayAttribute &BuildupAttribute) const {
    EnsureIndexed();
    if (!BuildupAttribute.IsValid()) {
        return nullptr;
    }
    for (const TPair<FGameplayTag, TObjectPtr<UMythicStatusEffectDefinition>> &Pair : StatusByType) {
        if (Pair.Value && Pair.Value->BuildupAttribute == BuildupAttribute) {
            return Pair.Value;
        }
    }
    return nullptr;
}

TArray<UMythicStatusEffectDefinition *> UMythicStatusRegistry::GetAllStatuses() const {
    EnsureIndexed();
    TArray<UMythicStatusEffectDefinition *> Out;
    Out.Reserve(StatusByType.Num());
    for (const TPair<FGameplayTag, TObjectPtr<UMythicStatusEffectDefinition>> &Pair : StatusByType) {
        if (Pair.Value) {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

void UMythicStatusRegistry::PlayStatusCue(UAbilitySystemComponent *TargetASC, const FGameplayTag &CueTag) {
    if (!TargetASC || !CueTag.IsValid()) {
        return;
    }
    UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(TargetASC);
    if (!MythicASC) {
        return;
    }
    FGameplayCueParameters CueParams;
    if (const AActor *TargetActor = TargetASC->GetAvatarActor()) {
        CueParams.Location = TargetActor->GetActorLocation();
    }
    MythicASC->ExecuteGameplayCueMulticast(CueTag, CueParams);
}

bool UMythicStatusRegistry::ShouldTeachStatus(bool bTargetIsPlayer, bool bAlreadyKnown, bool bHasDescription) {
    return bTargetIsPlayer && !bAlreadyKnown && bHasDescription;
}

namespace {
// Names a status the first time it happens to a player, and records it as known. The codex glossary is what
// remembers, so this survives a reload and cannot fire twice for the same character.
void TeachStatusIfNew(const UAbilitySystemComponent *TargetASC, const UMythicStatusEffectDefinition *Definition) {
    if (!TargetASC || !Definition || !Definition->StatusType.IsValid()) {
        return;
    }

    APawn *TargetPawn = nullptr;
    AController *TargetController = nullptr;
    APlayerState *TargetPS = nullptr;
    UMythicGameplayEffectContextLibrary::ResolveInstigator(TargetASC->GetAvatarActor(), TargetPawn, TargetController, TargetPS);

    AMythicPlayerController *PC = Cast<AMythicPlayerController>(TargetController);
    AMythicPlayerState *MythicPS = Cast<AMythicPlayerState>(TargetPS);
    UMythicCodexComponent *Codex = MythicPS ? MythicPS->GetCodexComponent() : nullptr;
    if (!PC || !Codex) {
        return;
    }

    const bool bKnown = Codex->HasDiscoveredTerm(Definition->StatusType);
    if (!UMythicStatusRegistry::ShouldTeachStatus(true, bKnown, !Definition->Description.IsEmpty())) {
        return;
    }

    Codex->ServerDiscoverTerm(Definition->StatusType);
    PC->ClientNotifyStatusLearned(Definition->DisplayName, Definition->Description, Definition->DisplayColor);
}
}

float UMythicStatusRegistry::RollScaledMagnitude(const FRollDefinition &Range, int32 Level, float SourceMultiplier, float Roll01) {
    // An unauthored range means the effect keeps whatever it authors itself.
    if (Range.Min <= 0.0f && Range.Max <= 0.0f) {
        return 0.0f;
    }
    const float ScaledMin = Range.GetScaledMin(Level);
    const float ScaledMax = Range.GetScaledMax(Level);
    const float Low = FMath::Min(ScaledMin, ScaledMax);
    const float High = FMath::Max(ScaledMin, ScaledMax);
    const float Rolled = FMath::Lerp(Low, High, FMath::Clamp(Roll01, 0.0f, 1.0f));
    return FMath::Max(0.0f, Rolled * FMath::Max(0.0f, SourceMultiplier));
}

bool UMythicStatusRegistry::TryReadApplierStat(const AActor *Instigator, const FGameplayAttribute &Attribute, float &OutRaw) {
    OutRaw = 0.0f;
    if (!Attribute.IsValid()) {
        return false;
    }
    const UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator);
    // An applier without the stat is not an applier with zero of it. Traps, hazards and scripted sources have no
    // ability system at all, and must inflict the authored band rather than nothing.
    if (!ASC || !ASC->HasAttributeSetForAttribute(Attribute)) {
        return false;
    }
    OutRaw = ASC->GetNumericAttribute(Attribute);
    return true;
}

float UMythicStatusRegistry::ResolveApplierMultiplier(const AActor *Instigator, const FGameplayAttribute &Attribute) {
    float Raw = 0.0f;
    if (!TryReadApplierStat(Instigator, Attribute, Raw)) {
        return 1.0f;
    }
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    return Settings ? FMythicStatDiminishingRules::Apply(Settings->StatDiminishing, Attribute, Raw)
                    : FMath::Max(0.0f, Raw);
}

float UMythicStatusRegistry::ResolveApplierBonus(const AActor *Instigator, const FGameplayAttribute &Attribute) {
    float Raw = 0.0f;
    if (!TryReadApplierStat(Instigator, Attribute, Raw)) {
        return 1.0f;
    }
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    return Settings ? FMythicStatDiminishingRules::ApplyToBonus(Settings->StatDiminishing, Attribute, Raw)
                    : 1.0f + FMath::Max(0.0f, Raw);
}

float UMythicStatusRegistry::ResolveApplierPowerMultiplier(const AActor *Instigator) {
    float Power = 0.0f;
    if (!TryReadApplierStat(Instigator, UMythicAttributeSet_Offense::GetPowerAttribute(), Power)) {
        return 1.0f;
    }
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings) {
        return 1.0f;
    }
    // Ride the authored Power -> weapon-damage contribution, so a status's base damage scales with Power exactly as a
    // weapon roll does - one curve for a designer to tune, and no second convention for Power to drift against.
    return FMythicStatContributionRules::ApplyToBase(
        Settings->StatContributions.Contributions, UMythicAttributeSet_Offense::GetDamagePerHitAttribute(), 1.0f,
        [Power](const FGameplayAttribute &Attr) -> float {
            return Attr == UMythicAttributeSet_Offense::GetPowerAttribute() ? Power : 0.0f;
        });
}

namespace {
// Walks the active control statuses of one kind on a target and folds their per-application ControlMagnitudes into a
// single ready-to-multiply factor. Applications that authored no band are ignored here; the caller substitutes the
// pre-band constant only when none carried a magnitude, so today's flat behaviour holds until content is tuned.
float CombineControlMagnitudes(const UAbilitySystemComponent *ASC, const FGameplayTag &StateTag, float FallbackMagnitude, bool bBonus) {
    if (!ASC || !StateTag.IsValid()) {
        return 1.0f;
    }
    const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(StateTag));
    const TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
    if (Handles.Num() == 0) {
        return 1.0f;
    }

    float Combined = 1.0f;
    bool bAnyAuthored = false;
    for (const FActiveGameplayEffectHandle &Handle : Handles) {
        const FActiveGameplayEffect *Effect = ASC->GetActiveGameplayEffect(Handle);
        if (!Effect) {
            continue;
        }
        const float Mag = Effect->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_CONTROL_MAGNITUDE, false, -1.0f);
        if (Mag < 0.0f) {
            continue;
        }
        bAnyAuthored = true;
        const float Clamped = FMath::Max(0.0f, Mag);
        // One application can never fully stop on its own; the stacked product is floored below.
        Combined *= bBonus ? (1.0f + Clamped) : (1.0f - FMath::Min(Clamped, 0.95f));
    }

    if (!bAnyAuthored) {
        const float Base = FMath::Max(0.0f, FallbackMagnitude);
        return bBonus ? (1.0f + Base) : FMath::Max(0.0f, 1.0f - FMath::Min(Base, 0.95f));
    }
    // A slow stack, however deep, always leaves the target a sliver of movement.
    return bBonus ? Combined : FMath::Max(0.05f, Combined);
}
}

float UMythicStatusRegistry::GetControlReductionMultiplier(const UAbilitySystemComponent *TargetASC, FGameplayTag StateTag, float FallbackMagnitude) {
    return CombineControlMagnitudes(TargetASC, StateTag, FallbackMagnitude, false);
}

float UMythicStatusRegistry::GetControlBonusMultiplier(const UAbilitySystemComponent *TargetASC, FGameplayTag StateTag, float FallbackMagnitude) {
    return CombineControlMagnitudes(TargetASC, StateTag, FallbackMagnitude, true);
}

float UMythicStatusRegistry::RollMagnitudeOrBase(const FRollDefinition &Range, float BaseWhenUnauthored, float Scale, float Roll01) {
    const bool bAuthored = Range.Min > 0.0f || Range.Max > 0.0f;
    if (!bAuthored) {
        return FMath::Max(0.0f, BaseWhenUnauthored) * FMath::Max(0.0f, Scale);
    }
    return RollScaledMagnitude(Range, 0, Scale, Roll01);
}

bool UMythicStatusRegistry::ApplyStatusEffect(UAbilitySystemComponent *TargetASC, const UMythicStatusEffectDefinition *Definition, AActor *Instigator,
                                              AActor *Causer) {
    if (!TargetASC || !Definition || !Definition->EffectToApply) {
        return false;
    }

    FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
    Context.AddInstigator(Instigator, Causer ? Causer : Instigator);

    const FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(Definition->EffectToApply, 1.0f, Context);
    if (!Spec.IsValid()) {
        return false;
    }

    // Two applications differ by the roll AND by what the applier has stacked, so a poison build hits harder
    // than a passer-by inflicting the same poison.
    const UMythicCombatSettings *CombatSettings = GetDefault<UMythicCombatSettings>();
    const float BaseDamage = CombatSettings ? CombatSettings->StatusBaseDamagePerTick : 3.0f;
    const float BaseDuration = CombatSettings ? CombatSettings->StatusBaseDurationSeconds : 5.0f;

    // Two applications differ by the roll AND by what the applier has stacked, so a poison build hits harder than
    // a passer-by inflicting the same poison. The global scale is the one knob that moves every status at once, Power
    // keeps the base pacing with the character, and the applier's per-status bonus rewards specialising into it.
    const float DamageScale = (CombatSettings ? CombatSettings->StatusDamageScale : 1.0f)
        * ResolveApplierPowerMultiplier(Instigator)
        * ResolveApplierBonus(Instigator, Definition->BonusDamageAttribute);
    const float Damage = RollMagnitudeOrBase(Definition->DamagePerTick, BaseDamage, DamageScale, FMath::FRand());
    if (Damage > 0.0f) {
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DAMAGE, Damage);
    }
    const float DurationScale = (CombatSettings ? CombatSettings->StatusDurationScale : 1.0f)
        * ResolveApplierMultiplier(Instigator, Definition->DurationMultiplierAttribute);
    const float Duration = RollMagnitudeOrBase(Definition->DurationSeconds, BaseDuration, DurationScale, FMath::FRand());
    if (Duration > 0.0f) {
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DURATION, Duration);
    }

    // The control axis - a slow's bite, a weaken's penalty, a terrify's bump - rolled per application and scaled by
    // the applier's gear. Only handed over when a band is authored; without one the effect keeps its own constant, so
    // nothing regresses before content is tuned. Consumers apply their own bound (a slow can never reach a full stop).
    const bool bControlAuthored = Definition->ControlMagnitude.Min > 0.0f || Definition->ControlMagnitude.Max > 0.0f;
    if (bControlAuthored) {
        const float ControlScale = ResolveApplierBonus(Instigator, Definition->ControlMagnitudeAttribute);
        const float ControlMag = RollMagnitudeOrBase(Definition->ControlMagnitude, 0.0f, ControlScale, FMath::FRand());
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_CONTROL_MAGNITUDE, ControlMag);
    }

    TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    PlayStatusCue(TargetASC, Definition->OnsetCueTag);
    TeachStatusIfNew(TargetASC, Definition);
    return true;
}

bool UMythicStatusRegistry::ApplyStatusToActor(AActor *Target, FGameplayTag StatusType, AActor *Instigator) {
    if (!Target || !Target->HasAuthority()) {
        return false;
    }

    UAbilitySystemComponent *TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
    if (!TargetASC) {
        return false;
    }
    const UWorld *World = Target ? Target->GetWorld() : nullptr;
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    const UMythicStatusRegistry *Registry = GameInstance ? GameInstance->GetSubsystem<UMythicStatusRegistry>() : nullptr;
    if (!Registry) {
        return false;
    }
    const UMythicStatusEffectDefinition *Definition = Registry->FindStatus(StatusType);
    return ApplyStatusEffect(TargetASC, Definition, Instigator, Instigator);
}
