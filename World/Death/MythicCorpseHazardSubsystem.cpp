
#include "MythicCorpseHazardSubsystem.h"

#include "MythicCorpse.h"
#include "MythicCorpseHazardConfig.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythCorpseHazard, Log, All);

static const FName GCorpseDiseasePowerMalusName(TEXT("CorpseDisease.PowerMalus"));
static const FName GCorpseDiseaseRegenMalusName(TEXT("CorpseDisease.RegenMalus"));
static const FName GCorpseDiseaseDurationName(TEXT("CorpseDisease.Duration"));


UMythicGE_CorpseDisease::UMythicGE_CorpseDisease() {
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    {
        FSetByCallerFloat DurSBC;
        DurSBC.DataName = GCorpseDiseaseDurationName;
        DurationMagnitude = FGameplayEffectModifierMagnitude(DurSBC);
    }

    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    StackingType = EGameplayEffectStackingType::AggregateByTarget;
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    StackLimitCount = 1;
    StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

    {
        FGameplayModifierInfo Mod;
        Mod.Attribute = UMythicAttributeSet_Offense::GetPowerAttribute();
        Mod.ModifierOp = EGameplayModOp::Additive;
        FSetByCallerFloat SBC;
        SBC.DataName = GCorpseDiseasePowerMalusName;
        Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);
        Modifiers.Add(Mod);
    }
    {
        FGameplayModifierInfo Mod;
        Mod.Attribute = UMythicAttributeSet_Defense::GetHealthRegenRateAttribute();
        Mod.ModifierOp = EGameplayModOp::Additive;
        FSetByCallerFloat SBC;
        SBC.DataName = GCorpseDiseaseRegenMalusName;
        Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);
        Modifiers.Add(Mod);
    }
}


bool UMythicCorpseHazardSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    return World->GetNetMode() != NM_Client;
}

void UMythicCorpseHazardSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }

    if (const UMythicCorpseHazardConfig *Cfg = GetDefault<UMythicCorpseHazardConfig>()) {
        Config = Cfg->BuildRuntimeConfig();
        DiseasePowerMalusPerSeverity = Cfg->DiseasePowerMalusPerSeverity;
        DiseaseHealthRegenMalusPerSeverity = Cfg->DiseaseHealthRegenMalusPerSeverity;
        DiseaseDurationSeconds = (Cfg->DiseaseDurationSeconds > 0.0f) ? Cfg->DiseaseDurationSeconds : Config.TickSeconds * 2.5f;
        MaxDiseaseCorpsesPerTick = FMath::Max(1, Cfg->MaxDiseaseCorpsesPerTick);
        MaxDiseaseTargetsPerCorpse = FMath::Max(1, Cfg->MaxDiseaseTargetsPerCorpse);
        MaxActiveCorpses = FMath::Max(0, Cfg->MaxActiveCorpses);
        ResolvedDiseaseClass = Cfg->DiseaseEffectClass.IsNull()
            ? UMythicGE_CorpseDisease::StaticClass()
            : Cfg->DiseaseEffectClass.LoadSynchronous();
    }
    if (!ResolvedDiseaseClass) {
        ResolvedDiseaseClass = UMythicGE_CorpseDisease::StaticClass();
    }

    if (const UGameInstance *GI = InWorld.GetGameInstance()) {
        LivingWorldSubsystem = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    }

    const float Interval = FMath::Max(0.5f, Config.TickSeconds);
    InWorld.GetTimerManager().SetTimer(HazardTimerHandle, this, &UMythicCorpseHazardSubsystem::TickHazards,
                                       Interval,true,Interval);

    UE_LOG(LogMythCorpseHazard, Log,
           TEXT("CorpseHazard: armed (tick=%.1fs, diseaseStart=%d, diseaseR=%.0f, sanitR=%.0f, budget=%d corpses×%d pawns)"),
           Interval, FMath::FloorToInt(Config.DiseaseStartStageInt), Config.DiseaseRadius, Config.SanitationRadius,
           MaxDiseaseCorpsesPerTick, MaxDiseaseTargetsPerCorpse);
}

void UMythicCorpseHazardSubsystem::Deinitialize() {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(HazardTimerHandle);
    }
    Corpses.Reset();
    Super::Deinitialize();
}


void UMythicCorpseHazardSubsystem::RegisterCorpse(AMythicCorpse *Corpse) {
    if (!Corpse) {
        return;
    }
    const UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) {
        return;
    }
    Corpses.AddUnique(Corpse);
    EnforceCorpseCap();
}

void UMythicCorpseHazardSubsystem::EnforceCorpseCap() {
    Corpses.RemoveAllSwap([](const TWeakObjectPtr<AMythicCorpse> &P) { return !P.IsValid(); });

    int32 Guard = Corpses.Num();
    while (FMythicCorpseHazardRules::ShouldEvictForCap(Corpses.Num(), MaxActiveCorpses) && Guard-- > 0) {
        TArray<float> Ages;
        TArray<bool> Locked;
        Ages.Reserve(Corpses.Num());
        Locked.Reserve(Corpses.Num());
        for (const TWeakObjectPtr<AMythicCorpse> &P : Corpses) {
            const AMythicCorpse *C = P.Get();
            Ages.Add(C ? C->GetGameTimeSinceCreation() : -1.0f);
            Locked.Add(C ? (C->Server_HasOpeners() || C->Server_IsChannelLocked()) : true);
        }
        const int32 EvictIdx = FMythicCorpseHazardRules::PickEvictIndex(Ages, Locked);
        if (EvictIdx == INDEX_NONE) {
            break;
        }
        AMythicCorpse *Evict = Corpses[EvictIdx].Get();
        Corpses.RemoveAtSwap(EvictIdx);
        if (Evict) {
            UE_LOG(LogMythCorpseHazard, Log, TEXT("CorpseHazard: cap %d exceeded — evicting oldest corpse %s (age %.0fs)"),
                   MaxActiveCorpses, *GetNameSafe(Evict), Evict->GetGameTimeSinceCreation());
            Evict->Destroy();
        }
    }
}

void UMythicCorpseHazardSubsystem::UnregisterCorpse(AMythicCorpse *Corpse) {
    if (!Corpse) {
        return;
    }
    Corpses.RemoveAllSwap([Corpse](const TWeakObjectPtr<AMythicCorpse> &P) { return P.Get() == Corpse; });
}

int32 UMythicCorpseHazardSubsystem::GetRegisteredCorpseCount() const {
    int32 N = 0;
    for (const TWeakObjectPtr<AMythicCorpse> &P : Corpses) {
        if (P.IsValid()) {
            ++N;
        }
    }
    return N;
}


void UMythicCorpseHazardSubsystem::TickHazards() {
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) {
        return;
    }

    Corpses.RemoveAllSwap([](const TWeakObjectPtr<AMythicCorpse> &P) { return !P.IsValid(); });
    const int32 Num = Corpses.Num();
    if (Num == 0) {
        DiseaseCursor = 0;
        return;
    }
    if (DiseaseCursor >= Num) {
        DiseaseCursor = 0;
    }

    int32 Applied = 0;
    int32 Scanned = 0;
    for (; Scanned < Num && Applied < MaxDiseaseCorpsesPerTick; ++Scanned) {
        const int32 Idx = (DiseaseCursor + Scanned) % Num;
        AMythicCorpse *Corpse = Corpses[Idx].Get();
        if (!Corpse || Corpse->AreHazardSignalsMuted()) {
            continue;
        }
        const int32 StageInt = static_cast<int32>(Corpse->GetDecompStage());
        if (!FMythicCorpseHazardRules::ShouldEmitDisease(StageInt, Config)) {
            continue;
        }
        ApplyDiseaseFromCorpse(Corpse);
        ++Applied;
    }
    DiseaseCursor = (DiseaseCursor + Scanned) % Num;
}

void UMythicCorpseHazardSubsystem::ApplyDiseaseFromCorpse(AMythicCorpse *Corpse) {
    UWorld *World = GetWorld();
    if (!World || !Corpse || !ResolvedDiseaseClass) {
        return;
    }
    const int32 StageInt = static_cast<int32>(Corpse->GetDecompStage());
    const float Severity = FMythicCorpseHazardRules::DiseaseSeverity(StageInt, Config);
    if (Severity <= 0.0f) {
        return;
    }

    const FVector Origin = Corpse->GetActorLocation();
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params(FName(TEXT("CorpseDiseaseAura")), false, Corpse);
    World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
                                    FCollisionObjectQueryParams(ECC_Pawn),
                                    FCollisionShape::MakeSphere(FMath::Max(1.0f, Config.DiseaseRadius)), Params);
    if (Overlaps.Num() == 0) {
        return;
    }

    const float PowerMalus = Severity * DiseasePowerMalusPerSeverity;
    const float RegenMalus = Severity * DiseaseHealthRegenMalusPerSeverity;

    TSet<UAbilitySystemComponent *> Seen;
    int32 Applied = 0;
    for (const FOverlapResult &O : Overlaps) {
        if (Applied >= MaxDiseaseTargetsPerCorpse) {
            break;
        }
        AActor *Actor = O.GetActor();
        if (!Actor || Actor == Corpse) {
            continue;
        }
        UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
        if (!ASC) {
            continue;
        }
        bool bAlready = false;
        Seen.Add(ASC, &bAlready);
        if (bAlready) {
            continue;
        }

        FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
        Ctx.AddSourceObject(Corpse);
        const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ResolvedDiseaseClass, 1.0f, Ctx);
        if (!Spec.IsValid()) {
            continue;
        }
        Spec.Data->SetSetByCallerMagnitude(GCorpseDiseasePowerMalusName, -PowerMalus);
        Spec.Data->SetSetByCallerMagnitude(GCorpseDiseaseRegenMalusName, -RegenMalus);
        Spec.Data->SetSetByCallerMagnitude(GCorpseDiseaseDurationName, DiseaseDurationSeconds);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        ++Applied;
    }
}


UMythicLivingWorldSubsystem *UMythicCorpseHazardSubsystem::ResolveLivingWorld() const {
    if (LivingWorldSubsystem) {
        return LivingWorldSubsystem;
    }
    const UWorld *World = GetWorld();
    const UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
}

float UMythicCorpseHazardSubsystem::GetSanitationPenaltyForLocation(const FVector &SettlementLocation) const {
    float Total = 0.0f;
    for (const TWeakObjectPtr<AMythicCorpse> &P : Corpses) {
        const AMythicCorpse *Corpse = P.Get();
        if (!Corpse || Corpse->AreHazardSignalsMuted()) {
            continue;
        }
        const int32 StageInt = static_cast<int32>(Corpse->GetDecompStage());
        const float Dist = FVector::Dist(Corpse->GetActorLocation(), SettlementLocation);
        Total += FMythicCorpseHazardRules::SanitationPenalty(StageInt, Dist, Config);
    }
    return Total;
}

float UMythicCorpseHazardSubsystem::GetSanitationPenaltyForSettlement(const FMythicCellCoord &SettlementCell) const {
    UMythicLivingWorldSubsystem *LW = ResolveLivingWorld();
    if (!LW) {
        return 0.0f;
    }
    const UMythicTerritoryGrid *Grid = LW->GetTerritoryGrid();
    if (!Grid) {
        return 0.0f;
    }
    return GetSanitationPenaltyForLocation(Grid->CellToWorld(SettlementCell));
}

void UMythicCorpseHazardSubsystem::GetCarrionPointsNear(const FVector &Location, float Radius, TArray<FMythicCarrionPoint> &Out) const {
    Out.Reset();
    const float RadiusSq = Radius * Radius;
    for (const TWeakObjectPtr<AMythicCorpse> &P : Corpses) {
        const AMythicCorpse *Corpse = P.Get();
        if (!Corpse || Corpse->AreHazardSignalsMuted()) {
            continue;
        }
        const FVector CorpseLoc = Corpse->GetActorLocation();
        if (FVector::DistSquared(CorpseLoc, Location) > RadiusSq) {
            continue;
        }
        const int32 StageInt = static_cast<int32>(Corpse->GetDecompStage());
        const float Attract = FMythicCorpseHazardRules::CarrionAttractiveness(StageInt, Config);
        if (Attract <= 0.0f) {
            continue;
        }
        FMythicCarrionPoint Point;
        Point.Location = CorpseLoc;
        Point.Attractiveness = Attract;
        Point.DecompStageInt = StageInt;
        Out.Add(Point);
    }
}

float UMythicCorpseHazardSubsystem::GetTotalCarrionAttractivenessNear(const FVector &Location, float Radius) const {
    const float RadiusSq = Radius * Radius;
    float Total = 0.0f;
    for (const TWeakObjectPtr<AMythicCorpse> &P : Corpses) {
        const AMythicCorpse *Corpse = P.Get();
        if (!Corpse || Corpse->AreHazardSignalsMuted()) {
            continue;
        }
        if (FVector::DistSquared(Corpse->GetActorLocation(), Location) > RadiusSq) {
            continue;
        }
        Total += FMythicCorpseHazardRules::CarrionAttractiveness(static_cast<int32>(Corpse->GetDecompStage()), Config);
    }
    return Total;
}

void UMythicCorpseHazardSubsystem::GetCorpsesNear(const FVector &Origin, float Radius, TArray<AMythicCorpse *> &Out) const {
    Out.Reset();
    if (Radius <= 0.0f) {
        return;
    }
    const float RadiusSq = Radius * Radius;
    for (const TWeakObjectPtr<AMythicCorpse> &P : Corpses) {
        AMythicCorpse *Corpse = P.Get();
        if (!Corpse) {
            continue;
        }
        if (FVector::DistSquared(Corpse->GetActorLocation(), Origin) > RadiusSq) {
            continue;
        }
        Out.Add(Corpse);
    }
}
