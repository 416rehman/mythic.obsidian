
#include "Mass/Processors/ActorSpawnProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/Creatures/MythicCreatureCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"

UMythicActorSpawnProcessor::UMythicActorSpawnProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    QueryBasedPruning = EMassQueryBasedPruning::Never;

    SpawnActorClass = AMythicNPCCharacter::StaticClass();
    SpawnCreatureClass = AMythicCreatureCharacter::StaticClass();
    SpawnRequestQuery.RegisterWithProcessor(*this);
    CreatureSpawnRequestQuery.RegisterWithProcessor(*this);
    DespawnRequestQuery.RegisterWithProcessor(*this);
}

void UMythicActorSpawnProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    SpawnRequestQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    SpawnRequestQuery.AddTagRequirement<FMythicActorSpawnRequestTag>(EMassFragmentPresence::All);
    SpawnRequestQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::None);

    CreatureSpawnRequestQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    CreatureSpawnRequestQuery.AddTagRequirement<FMythicActorSpawnRequestTag>(EMassFragmentPresence::All);
    CreatureSpawnRequestQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);

    DespawnRequestQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    DespawnRequestQuery.AddTagRequirement<FMythicCognitiveTag>(EMassFragmentPresence::All);
    DespawnRequestQuery.AddTagRequirement<FMythicActorDespawnRequestTag>(EMassFragmentPresence::All);
}

namespace {
    void GetCapsuleDimsFromClass(UClass *ActorClass, float &OutRadius, float &OutHalfHeight) {
        if (ActorClass) {
            if (const ACharacter *CDO = Cast<ACharacter>(ActorClass->GetDefaultObject())) {
                if (const UCapsuleComponent *Capsule = CDO->GetCapsuleComponent()) {
                    OutRadius = Capsule->GetUnscaledCapsuleRadius();
                    OutHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
                }
            }
        }
    }
}

bool UMythicActorSpawnProcessor::TryFindSpawnTransform(
    UWorld *World,
    const UMythicLivingWorldSettings *Settings,
    const FVector &CellCenterXY,
    UClass *ResolvedClass,
    bool bWaterCapable,
    double Now,
    int32 &ValidationsThisTick,
    const FMassEntityHandle &Entity,
    FTransform &OutTransform) {
    FMythicPlacementParams Params;
    Params.CellCenterXY = CellCenterXY;
    Params.ScatterRadius = Settings->SpawnScatterRadius;
    Params.NavExtent = Settings->NavProjectionExtent;
    Params.bRequireReachability = Settings->bRequireReachability;
    Params.RetryBudget = Settings->SpawnRetryBudget;
    Params.bWaterCapable = bWaterCapable;
    GetCapsuleDimsFromClass(ResolvedClass, Params.CapsuleRadius, Params.CapsuleHalfHeight);

    ++ValidationsThisTick;

    if (MythicPlacement::FindValidSpawn(World, Params, OutTransform)) {
        SpawnDeferUntil.Remove(Entity);
        return true;
    }

    SpawnDeferUntil.Add(Entity, Now + static_cast<double>(Settings->SpawnDeferCooldownSeconds));
    return false;
}

bool UMythicActorSpawnProcessor::IsActorInCloseView(UWorld *World, const AMythicNPCCharacter *Actor, const UMythicLivingWorldSettings *Settings) {
    if (!World || !Actor || !Settings || !Settings->bViewGateEmbodiment) {
        return false;
    }

    const FVector ActorLoc = Actor->GetActorLocation();
    const float MinDist = Settings->ViewGateMinSpawnDistance;
    const float MinDistSq = MinDist * MinDist;
    const float MarginRad = FMath::DegreesToRadians(FMath::Max(0.0f, Settings->ViewConeMarginDeg));

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        const APlayerController *PC = It->Get();
        if (!PC || !PC->PlayerCameraManager) {
            continue;
        }

        const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
        const FVector ToActor = ActorLoc - CamLoc;
        const float DistSq = ToActor.SizeSquared();

        if (DistSq > MinDistSq) {
            continue;
        }

        const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
        const FVector CamFwd = CamRot.Vector();
        const float HalfFOVRad = FMath::DegreesToRadians(0.5f * PC->PlayerCameraManager->GetFOVAngle());
        const float HalfConeRad = FMath::Min(PI, HalfFOVRad + MarginRad);

        const FVector DirToActor = ToActor.GetSafeNormal();
        if (DirToActor.IsNearlyZero()) {
            return true;
        }
        const float CosAngle = FVector::DotProduct(CamFwd, DirToActor);
        if (CosAngle >= FMath::Cos(HalfConeRad)) {
            return true;
        }
    }

    return false;
}

void UMythicActorSpawnProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicActorSpawn_Execute);

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    UGameInstance *GI = World->GetGameInstance();
    if (!GI) {
        return;
    }
    UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LWS || !LWS->IsSystemActive()) {
        return;
    }
    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    if (!Grid) {
        return;
    }
    UMythicPersistentNPCRegistry *Registry = LWS->GetPersistentNPCRegistry();

    const UMythicLivingWorldSettings *Settings = LWS->GetSettings();
    if (!Settings) {
        return;
    }

    const int32 MaxValidations = FMath::Max(1, Settings->MaxPlacementValidationsPerTick);
    int32 ValidationsThisTick = 0;
    const double Now = World->GetTimeSeconds();

    TSet<FMassEntityHandle> ActiveSpawnRequests;

    if (!ResolvedSpawnClass) {
        ResolvedSpawnClass = SpawnActorClass;
        if (!ResolvedSpawnClass) {
            ResolvedSpawnClass = AMythicNPCCharacter::StaticClass();
        }
        if (!Settings->EmbodiedNPCClass.IsNull()) {
            if (UClass *Loaded = Settings->EmbodiedNPCClass.LoadSynchronous()) {
                ResolvedSpawnClass = Loaded;
            }
        }
    }

    if (!bCreatureClassResolved) {
        bCreatureClassResolved = true;
        ResolvedCreatureClass = SpawnCreatureClass;
        if (!ResolvedCreatureClass) {
            ResolvedCreatureClass = AMythicCreatureCharacter::StaticClass();
        }
        if (!Settings->EmbodiedCreatureClass.IsNull()) {
            ResolvedCreatureClass = Settings->EmbodiedCreatureClass.LoadSynchronous();
        }
    }

    TArray<TPair<FMassEntityHandle, FMythicIdentityFragment>> Requests;
    SpawnRequestQuery.ForEachEntityChunk(Context, [&Requests](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            Requests.Emplace(ChunkContext.GetEntity(i), IdentityView[i]);
        }
    });

    for (const TPair<FMassEntityHandle, FMythicIdentityFragment> &Req : Requests) {
        const FMassEntityHandle Entity = Req.Key;
        const FMythicIdentityFragment &Identity = Req.Value;

        ActiveSpawnRequests.Add(Entity);

        if (const double *DeferUntil = SpawnDeferUntil.Find(Entity)) {
            if (*DeferUntil > Now) {
                continue;
            }
        }

        if (Registry && Registry->IsPermaDead(Identity.NameHash)) {
            Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Remove(Entity);
            continue;
        }

        if (LWS->FindEmbodiedActor(Entity)) {
            Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Remove(Entity);
            continue;
        }

        if (ValidationsThisTick >= MaxValidations) {
            break;
        }

        const FVector CellCenterXY = Grid->CellToWorld(Identity.Cell);
        FTransform SpawnTM;

        bool bPlaced = false;
        if (Identity.bHasSpawnOverride) {
            float CapRadius = 0.0f, CapHalfHeight = 0.0f;
            GetCapsuleDimsFromClass(ResolvedSpawnClass, CapRadius, CapHalfHeight);
            ++ValidationsThisTick;
            if (MythicPlacement::ValidateExistingPoint(World, Identity.SpawnOverridePos, CapRadius, CapHalfHeight,
false, SpawnTM)) {
                SpawnDeferUntil.Remove(Entity);
                bPlaced = true;
            }
        }

        if (!bPlaced &&
            !TryFindSpawnTransform(World, Settings, CellCenterXY, ResolvedSpawnClass,false, Now, ValidationsThisTick, Entity, SpawnTM)) {
            continue;
        }

        Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);

        AMythicNPCCharacter *NPC = LWS->AcquireEmbodiedActor(ResolvedSpawnClass, SpawnTM.GetLocation(), SpawnTM.Rotator());
        if (!NPC) {
            Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Add(Entity, Now + static_cast<double>(Settings->SpawnDeferCooldownSeconds));
            continue;
        }

        NPC->InitializeFromMassEntity(Entity);

        LWS->RegisterEmbodiedActor(Entity, NPC);

        Context.Defer().AddTag<FMythicCognitiveTag>(Entity);
    }

    TArray<TPair<FMassEntityHandle, FMythicIdentityFragment>> CreatureRequests;
    CreatureSpawnRequestQuery.ForEachEntityChunk(Context, [&CreatureRequests](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            CreatureRequests.Emplace(ChunkContext.GetEntity(i), IdentityView[i]);
        }
    });

    for (const TPair<FMassEntityHandle, FMythicIdentityFragment> &Req : CreatureRequests) {
        const FMassEntityHandle Entity = Req.Key;
        const FMythicIdentityFragment &Identity = Req.Value;

        ActiveSpawnRequests.Add(Entity);

        if (!ResolvedCreatureClass) {
            Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Remove(Entity);
            continue;
        }

        if (const double *DeferUntil = SpawnDeferUntil.Find(Entity)) {
            if (*DeferUntil > Now) {
                continue;
            }
        }

        if (LWS->FindEmbodiedActor(Entity)) {
            Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Remove(Entity);
            continue;
        }

        if (ValidationsThisTick >= MaxValidations) {
            break;
        }

        const FVector CellCenterXY = Grid->CellToWorld(Identity.Cell);
        FTransform SpawnTM;
        if (!TryFindSpawnTransform(World, Settings, CellCenterXY, ResolvedCreatureClass,false, Now, ValidationsThisTick, Entity, SpawnTM)) {
            continue;
        }

        Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);

        AMythicNPCCharacter *Creature = LWS->AcquireEmbodiedActor(ResolvedCreatureClass, SpawnTM.GetLocation(), SpawnTM.Rotator());
        if (!Creature) {
            Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Add(Entity, Now + static_cast<double>(Settings->SpawnDeferCooldownSeconds));
            continue;
        }

        Creature->InitializeFromMassEntity(Entity);

        LWS->RegisterEmbodiedActor(Entity, Creature);

        Context.Defer().AddTag<FMythicCognitiveTag>(Entity);
    }

    TArray<FMassEntityHandle> Despawns;
    DespawnRequestQuery.ForEachEntityChunk(Context, [&Despawns](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        for (int32 i = 0; i < NumEntities; ++i) {
            Despawns.Add(ChunkContext.GetEntity(i));
        }
    });

    for (const FMassEntityHandle Entity : Despawns) {
        AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(Entity);

        if (Actor && IsActorInCloseView(World, Actor, Settings)) {
            continue;
        }

        Context.Defer().RemoveTag<FMythicActorDespawnRequestTag>(Entity);
        Context.Defer().RemoveTag<FMythicCognitiveTag>(Entity);

        LWS->ReleaseEmbodiedActor(Entity, Actor);

        SpawnDeferUntil.Remove(Entity);
    }

    if (SpawnDeferUntil.Num() > 0) {
        for (auto It = SpawnDeferUntil.CreateIterator(); It; ++It) {
            if (!ActiveSpawnRequests.Contains(It.Key())) {
                It.RemoveCurrent();
            }
        }
    }
}
