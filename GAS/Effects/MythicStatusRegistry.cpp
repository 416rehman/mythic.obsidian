
#include "MythicStatusRegistry.h"

#include "Mythic.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Settings/MythicDeveloperSettings.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_AILMENT_DAMAGE, "SetByCaller.Ailment.Damage",
                               "Per-tick damage handed to an authored status effect");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_AILMENT_DURATION, "SetByCaller.Ailment.Duration",
                               "Seconds handed to an authored status effect");

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

    // The applier's stats decide how hard and how long its ailments bite, so two players inflicting the same
    // status do not inflict the same number.
    float DamageMultiplier = 1.0f;
    float DurationMultiplier = 1.0f;
    if (const UAbilitySystemComponent *SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator)) {
        if (const UMythicAttributeSet_Offense *Offense = SourceASC->GetSet<UMythicAttributeSet_Offense>()) {
            DamageMultiplier = Offense->GetAilmentDamageMultiplier();
            DurationMultiplier = Offense->GetAilmentDurationMultiplier();
        }
    }

    const float Damage = RollScaledMagnitude(Definition->DamagePerTick, 0, DamageMultiplier, FMath::FRand());
    if (Damage > 0.0f) {
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_AILMENT_DAMAGE, Damage);
    }
    const float Duration = RollScaledMagnitude(Definition->DurationSeconds, 0, DurationMultiplier, FMath::FRand());
    if (Duration > 0.0f) {
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_AILMENT_DURATION, Duration);
    }

    TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    PlayStatusCue(TargetASC, Definition->OnsetCueTag);
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
