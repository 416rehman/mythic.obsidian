
#include "MythicStatusRegistry.h"

#include "Mythic.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Abilities/MythicAbilityRollSource.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "Knowledge/MythicCodexComponent.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Settings/MythicDeveloperSettings.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_STATUS_DAMAGE, "SetByCaller.Status.Damage",
                               "Per-tick damage handed to an authored status effect");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_STATUS_DURATION, "SetByCaller.Status.Duration",
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

    // The authored band is the whole story: two applications differ by the roll, not by a stat on the applier.
    const float Damage = RollScaledMagnitude(Definition->DamagePerTick, 0, 1.0f, FMath::FRand());
    if (Damage > 0.0f) {
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DAMAGE, Damage);
    }
    const float Duration = RollScaledMagnitude(Definition->DurationSeconds, 0, 1.0f, FMath::FRand());
    if (Duration > 0.0f) {
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DURATION, Duration);
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
