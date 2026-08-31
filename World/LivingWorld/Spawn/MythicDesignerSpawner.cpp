
#include "World/LivingWorld/Spawn/MythicDesignerSpawner.h"
#include "Settings/MythicCombatSettings.h"

#include "AI/NPCs/MythicNPCCharacter.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Spawn/DesignerSpawnerRegistry.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityIdentityDefinition.h"
#include "World/Entity/MythicEntityPresentationTags.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/EnvironmentController/MythicEnvironmentController.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

AMythicDesignerSpawner::AMythicDesignerSpawner() {
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = false;
}

UMythicLivingWorldSubsystem *AMythicDesignerSpawner::GetLWS() const {
    if (const UWorld *World = GetWorld()) {
        if (UGameInstance *GI = World->GetGameInstance()) {
            return GI->GetSubsystem<UMythicLivingWorldSubsystem>();
        }
    }
    return nullptr;
}

void AMythicDesignerSpawner::BeginPlay() {
    Super::BeginPlay();

    if (!HasAuthority()) {
        return;
    }

    if (DesignerId.IsNone()) {
        UE_LOG(LogMythLivingWorld, Warning,
               TEXT("AMythicDesignerSpawner '%s' has no DesignerId — disabled (persistence key required)."), *GetName());
        return;
    }

    if (UMythicLivingWorldSubsystem *LWS = GetLWS()) {
        if (UMythicDesignerSpawnerRegistry *Reg = LWS->GetDesignerSpawnerRegistry()) {
            const FMythicDesignerSpawnerState &State = Reg->FindOrAdd(DesignerId);
            CachedSpawnsEver = State.SpawnsEver;
            bCachedPermaDead = State.bPermaDead;
            CachedLastDeathTime = State.LastDeathTime;
        }
    }

    if (bCachedPermaDead || CachedSpawnsEver >= MaxSpawnsEver) {
        return;
    }

    GetWorldTimerManager().SetTimer(EvalTimerHandle, this, &AMythicDesignerSpawner::TickEvaluate,
                                    EvaluationIntervalSeconds,true);
}

void AMythicDesignerSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (GetWorld()) {
        GetWorldTimerManager().ClearTimer(EvalTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void AMythicDesignerSpawner::BeginLivingWorldRestore() {
    if (!HasAuthority()) {
        return;
    }
    StopEvaluation();
    for (const TWeakObjectPtr<AMythicNPCCharacter> &WeakNPC : LiveNPCs) {
        if (AMythicNPCCharacter *NPC = WeakNPC.Get()) {
            if (NPC->LifeComponent) {
                NPC->LifeComponent->OnDeath.RemoveDynamic(
                    this, &AMythicDesignerSpawner::OnDesignerNPCDeath);
            }
            NPC->Destroy();
        }
    }
    LiveNPCs.Reset();
    LiveEntityIds.Reset();
    CachedSpawnsEver = 0;
    bCachedPermaDead = false;
    CachedLastDeathTime = 0.0;
}

void AMythicDesignerSpawner::CompleteLivingWorldRestore() {
    if (!HasAuthority() || DesignerId.IsNone()) {
        return;
    }
    if (UMythicLivingWorldSubsystem *LWS = GetLWS()) {
        if (UMythicDesignerSpawnerRegistry *Registry =
                LWS->GetDesignerSpawnerRegistry()) {
            const FMythicDesignerSpawnerState &State =
                Registry->FindOrAdd(DesignerId);
            CachedSpawnsEver = State.SpawnsEver;
            bCachedPermaDead = State.bPermaDead;
            CachedLastDeathTime = State.LastDeathTime;
        }
    }
    if (!bCachedPermaDead && CachedSpawnsEver < MaxSpawnsEver) {
        GetWorldTimerManager().SetTimer(
            EvalTimerHandle, this, &AMythicDesignerSpawner::TickEvaluate,
            EvaluationIntervalSeconds, true);
    }
}

void AMythicDesignerSpawner::StopEvaluation() {
    if (GetWorld()) {
        GetWorldTimerManager().ClearTimer(EvalTimerHandle);
    }
}

int32 AMythicDesignerSpawner::GetLiveCount() const {
    int32 Count = 0;
    for (const TWeakObjectPtr<AMythicNPCCharacter> &Ptr : LiveNPCs) {
        if (Ptr.IsValid()) {
            ++Count;
        }
    }
    return Count;
}

void AMythicDesignerSpawner::ReapLiveNPCs() {
    UMythicLivingWorldSubsystem *LWS = GetLWS();
    UMythicPersistentNPCRegistry *IdentityRegistry =
        LWS ? LWS->GetPersistentNPCRegistry() : nullptr;
    for (int32 i = LiveNPCs.Num() - 1; i >= 0; --i) {
        if (!LiveNPCs[i].IsValid()) {
            if (LWS && IdentityRegistry) {
                if (const FMythicEntityId *EntityId = LiveEntityIds.Find(LiveNPCs[i])) {
                    LWS->TryRetireEntityIdentity(*EntityId);
                }
            }
            LiveEntityIds.Remove(LiveNPCs[i]);
            LiveNPCs.RemoveAtSwap(i);
        }
    }
}

void AMythicDesignerSpawner::TickEvaluate() {
    if (!HasAuthority()) {
        return;
    }

    ReapLiveNPCs();

    if (bCachedPermaDead || CachedSpawnsEver >= MaxSpawnsEver) {
        StopEvaluation();
        return;
    }
    if (GetLiveCount() >= MaxConcurrent) {
        return;
    }

    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (RespawnCooldownSeconds > 0.0f && (Now - CachedLastDeathTime) < RespawnCooldownSeconds) {
        return;
    }

    FMythicDesignerConditionInputs Inputs;
    GatherInputs(Inputs);

    if (MythicDesignerSpawner::EvaluateConditions(Conditions, Inputs)) {
        SpawnNPC();
    }
}

bool AMythicDesignerSpawner::AreConditionsMet() const {
    FMythicDesignerConditionInputs Inputs;
    GatherInputs(Inputs);
    return MythicDesignerSpawner::EvaluateConditions(Conditions, Inputs);
}

float AMythicDesignerSpawner::GetCurrentGameHour() const {
    if (const UWorld *World = GetWorld()) {
        if (UGameInstance *GI = World->GetGameInstance()) {
            if (UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
                if (AMythicEnvironmentController *Controller = Env->GetEnvironmentController()) {
                    const FTimespan Ts = Controller->GetTimespan();
                    float Hour = static_cast<float>(Ts.GetHours()) + static_cast<float>(Ts.GetMinutes()) / 60.0f;
                    Hour = FMath::Fmod(Hour, 24.0f);
                    if (Hour < 0.0f) {
                        Hour += 24.0f;
                    }
                    return Hour;
                }
            }
        }
        const double Seconds = World->GetTimeSeconds();
        const double DayLengthSeconds = 1440.0;
        const double Frac = FMath::Fmod(Seconds, DayLengthSeconds) / DayLengthSeconds;
        return static_cast<float>(Frac * 24.0);
    }
    return 0.0f;
}

bool AMythicDesignerSpawner::AnyPlayerSatisfiesPlayerGate() const {
    const UWorld *World = GetWorld();
    if (!World) {
        return false;
    }

    const bool bTagGate = !Conditions.RequiredPlayerTags.IsEmpty();
    const bool bRangeGate = Conditions.bRequireAnyPlayerInRange;
    const float RangeSq = Conditions.PlayerRangeCm * Conditions.PlayerRangeCm;
    const FVector MyLoc = GetActorLocation();

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        APlayerController *PC = It->Get();
        if (!PC) {
            continue;
        }
        APawn *Pawn = PC->GetPawn();

        if (bRangeGate || bTagGate) {
            if (!Pawn) {
                continue;
            }
            const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), MyLoc);
            if ((bRangeGate || bTagGate) && DistSq > RangeSq) {
                continue;
            }
        }

        if (bTagGate) {
            AActor *TagActor = Pawn ? static_cast<AActor *>(Pawn) : static_cast<AActor *>(PC);
            UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TagActor);
            if (!ASC || !ASC->HasAllMatchingGameplayTags(Conditions.RequiredPlayerTags)) {
                continue;
            }
        }

        return true;
    }

    return false;
}

bool AMythicDesignerSpawner::GatherInputs(FMythicDesignerConditionInputs &OutInputs) const {
    OutInputs.GameHour = GetCurrentGameHour();

    OutInputs.bAnyPlayerSatisfiesTags = AnyPlayerSatisfiesPlayerGate();

    UMythicLivingWorldSubsystem *LWS = GetLWS();
    UMythicFactionDatabase *DB = LWS ? LWS->GetFactionDatabase() : nullptr;

    if (DB && Conditions.GatingFactionTag.IsValid() &&
        Conditions.FactionState != EMythicDesignerFactionStatePredicate::Any) {
        FMythicFactionData Data;
        FMythicFactionId Id;
        if (DB->FindFactionByTag(Conditions.GatingFactionTag, Data, &Id) && Id.IsValid()) {
            OutInputs.GatingFactionStatus = Data.Status;
            OutInputs.bGatingFactionResolved = true;
        }
    }

    if (DB && Conditions.Relation != EMythicDesignerRelationPredicate::Ignore &&
        Conditions.RelationFactionA.IsValid() && Conditions.RelationFactionB.IsValid()) {
        const FMythicFactionId A = DB->FindFactionId(Conditions.RelationFactionA);
        const FMythicFactionId B = DB->FindFactionId(Conditions.RelationFactionB);
        if (A.IsValid() && B.IsValid()) {
            OutInputs.RelationAB = DB->GetRelationship(A, B);
            OutInputs.bRelationResolved = true;
        }
    }

    return true;
}

void AMythicDesignerSpawner::SpawnNPC() {
    UWorld *World = GetWorld();
    if (!World || !*NPCClass) {
        UE_LOG(LogMythLivingWorld, Warning, TEXT("DesignerSpawner '%s': cannot spawn — null world or NPCClass."),
               *DesignerId.ToString());
        return;
    }

    UMythicLivingWorldSubsystem *LWS = GetLWS();
    UMythicPersistentNPCRegistry *IdentityRegistry =
        LWS ? LWS->GetPersistentNPCRegistry() : nullptr;
    if (!IdentityRegistry) {
        UE_LOG(LogMythLivingWorld, Error,
               TEXT("DesignerSpawner '%s': canonical identity registry unavailable."),
               *DesignerId.ToString());
        return;
    }

    FTransform SpawnTransform;
    if (bUseExactPlacedTransform) {
        SpawnTransform = GetActorTransform();
    }
    else {
        FMythicPlacementParams Params;
        Params.CellCenterXY = GetActorLocation();
        if (const AMythicNPCCharacter *CDO = NPCClass.GetDefaultObject()) {
            if (const UCapsuleComponent *Capsule = CDO->GetCapsuleComponent()) {
                Params.CapsuleRadius = Capsule->GetScaledCapsuleRadius();
                Params.CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
            }
        }
        FTransform Validated;
        if (!MythicPlacement::FindValidSpawn(World, Params, Validated)) {
            return;
        }
        SpawnTransform = Validated;
    }

    AMythicNPCCharacter *NPC = World->SpawnActorDeferred<AMythicNPCCharacter>(
        NPCClass, SpawnTransform,this,nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!NPC) {
        UE_LOG(LogMythLivingWorld, Warning, TEXT("DesignerSpawner '%s': SpawnActorDeferred returned null."),
               *DesignerId.ToString());
        return;
    }
    NPC->SetActorHiddenInGame(true);
    NPC->SetActorEnableCollision(false);
    NPC->SetActorTickEnabled(false);

    uint8 FactionIndex = 0;
    if (LWS) {
        if (UMythicFactionDatabase *DB = LWS->GetFactionDatabase()) {
            if (Conditions.GatingFactionTag.IsValid()) {
                const FMythicFactionId Id = DB->FindFactionId(Conditions.GatingFactionTag);
                if (Id.IsValid()) {
                    FactionIndex = Id.Index;
                }
            }
        }
    }

    FMythicCellCoord Cell;
    if (LWS) {
        if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
            Cell = Grid->WorldToCell(SpawnTransform.GetLocation());
        }
    }

    const int32 SpawnIndex = static_cast<int32>(
        GetTypeHash(DesignerId) ^ (static_cast<uint32>(Cell.X) << 8)
        ^ static_cast<uint32>(Cell.Y) ^ static_cast<uint32>(CachedSpawnsEver));
    const uint32 NameSeed = FMythicNPCGenerator::GenerateNameHash(
        FactionIndex, Cell, SpawnIndex);
    const FMythicEntityId EntityId = IdentityRegistry->AllocateEntityIdentity(
        NameSeed, EMythicEntityIdentityProvenance::DesignerSpawner);
    if (!EntityId.IsValid()) {
        NPC->Destroy();
        return;
    }

    NPC->FinishSpawning(SpawnTransform);
    NPC->StampCombatLevel(MythicCombat::ResolveCombatLevelAt(World, SpawnTransform.GetLocation()));

    UMythicEntityPresentationComponent *Presentation =
        IMythicPresentableEntity::Execute_GetEntityPresentationComponent(NPC);
    FMythicPublicIdentitySnapshot SafeIdentity;
    SafeIdentity.PublicKindTag = MythicEntityPresentationTags::EntityKindHumanoid;
    SafeIdentity.PublicIdentityDefinitionId =
        UMythicEntityIdentityDefinition::ResolvePrimaryAssetId(
            PublicIdentityDefinition);
    if (!Presentation
        || !Presentation->AuthorityPrepareEmbodiment(EntityId, SafeIdentity)) {
        UE_LOG(LogMythLivingWorld, Error,
               TEXT("DesignerSpawner '%s': failed to prepare entity presentation."),
               *DesignerId.ToString());
        NPC->Destroy();
        LWS->TryRetireEntityIdentity(EntityId);
        return;
    }
    if (!NPC->ActivatePreparedEmbodiment()) {
        UE_LOG(LogMythLivingWorld, Error,
               TEXT("DesignerSpawner '%s': failed to activate prepared embodiment."),
               *DesignerId.ToString());
        NPC->Destroy();
        LWS->TryRetireEntityIdentity(EntityId);
        return;
    }

    if (NPC->LifeComponent) {
        NPC->LifeComponent->OnDeath.AddDynamic(this, &AMythicDesignerSpawner::OnDesignerNPCDeath);
    }

    const TWeakObjectPtr<AMythicNPCCharacter> WeakNPC(NPC);
    LiveNPCs.Add(WeakNPC);
    LiveEntityIds.Add(WeakNPC, EntityId);

    if (LWS) {
        if (UMythicDesignerSpawnerRegistry *Reg = LWS->GetDesignerSpawnerRegistry()) {
            Reg->RecordSpawn(DesignerId);
        }
    }
    ++CachedSpawnsEver;

    UE_LOG(LogMythLivingWorld, Log,
           TEXT("DesignerSpawner '%s' spawned NPC #%d (%s, NameSeed=%u, Cell=%s)."),
           *DesignerId.ToString(), CachedSpawnsEver, *EntityId.ToDebugString(), NameSeed,
           *Cell.ToString());
}

void AMythicDesignerSpawner::OnDesignerNPCDeath(AActor *DeadActor) {
    if (!HasAuthority()) {
        return;
    }

    AMythicNPCCharacter *DeadNPC = Cast<AMythicNPCCharacter>(DeadActor);
    const TWeakObjectPtr<AMythicNPCCharacter> WeakDead(DeadNPC);

    FMythicEntityId DeadEntityId;
    if (const FMythicEntityId *Found = LiveEntityIds.Find(WeakDead)) {
        DeadEntityId = *Found;
    }

    LiveNPCs.RemoveAllSwap([&](const TWeakObjectPtr<AMythicNPCCharacter> &Ptr) {
        return Ptr == WeakDead || !Ptr.IsValid();
    });
    LiveEntityIds.Remove(WeakDead);

    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    CachedLastDeathTime = Now;

    const bool bPerma = bMarkPermaDeadOnDeath && (CachedSpawnsEver >= MaxSpawnsEver);

    UMythicLivingWorldSubsystem *LWS = GetLWS();

    if (LWS) {
        if (UMythicDesignerSpawnerRegistry *Reg = LWS->GetDesignerSpawnerRegistry()) {
            Reg->RecordDeath(DesignerId, Now, bPerma);
        }
    }
    if (bPerma) {
        bCachedPermaDead = true;
    }

    bool bCommittedPersonDeath = false;
    UMythicPersistentNPCRegistry *PersistentRegistry =
        LWS ? LWS->GetPersistentNPCRegistry() : nullptr;
    if (LWS && LWS->IsSystemActive() && PersistentRegistry
        && DeadEntityId.IsValid()) {
        FMythicFactionId Faction;
        FMythicCellCoord Cell;
        if (UMythicFactionDatabase *DB = LWS->GetFactionDatabase()) {
            if (Conditions.GatingFactionTag.IsValid()) {
                Faction = DB->FindFactionId(Conditions.GatingFactionTag);
            }
        }
        if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
            const FVector DeathLoc = DeadActor ? DeadActor->GetActorLocation() : GetActorLocation();
            Cell = Grid->WorldToCell(DeathLoc);
        }
        bCommittedPersonDeath = PersistentRegistry->RegisterDeath(
            DeadEntityId, Faction, FGameplayTag(), Cell, Now, LWS);
        if (bCommittedPersonDeath) {
            LWS->ReportNpcDeath(Faction, FGameplayTag());
        }
    }

    // Spawner terminal state governs whether another distinct person may spawn; the person that just died is always
    // terminal and keeps a canonical tombstone. If the death transaction could not commit, release only when no
    // durable owner exists rather than leaving an untracked identity record.
    if (PersistentRegistry && DeadEntityId.IsValid()
        && !bCommittedPersonDeath) {
        LWS->TryRetireEntityIdentity(DeadEntityId);
    }

    if (bCachedPermaDead || CachedSpawnsEver >= MaxSpawnsEver) {
        StopEvaluation();
    }
}
