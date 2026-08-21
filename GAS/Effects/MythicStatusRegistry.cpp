
#include "MythicStatusRegistry.h"

#include "Mythic.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Settings/MythicDeveloperSettings.h"

void UMythicStatusRegistry::BuildIndex() {
    bIndexed = true;

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

    TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    PlayStatusCue(TargetASC, Definition->OnsetCueTag);
    return true;
}

bool UMythicStatusRegistry::ApplyStatusToActor(AActor *Target, FGameplayTag StatusType, AActor *Instigator) {
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
