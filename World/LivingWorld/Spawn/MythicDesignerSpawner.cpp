// Mythic Living World — Designer Conditional / Stateful Spawner Implementation

#include "World/LivingWorld/Spawn/MythicDesignerSpawner.h"

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
    // Timer-throttled — never per-frame.
    PrimaryActorTick.bCanEverTick = false;

    // Server-only logic actor: the spawned NPC replicates itself; this controller does not.
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

    // 100% server-authoritative.
    if (!HasAuthority()) {
        return;
    }

    if (DesignerId.IsNone()) {
        UE_LOG(LogMythLivingWorld, Warning,
               TEXT("AMythicDesignerSpawner '%s' has no DesignerId — disabled (persistence key required)."), *GetName());
        return;
    }

    // Mirror the AUTHORITATIVE registry state into the cache.
    if (UMythicLivingWorldSubsystem *LWS = GetLWS()) {
        if (UMythicDesignerSpawnerRegistry *Reg = LWS->GetDesignerSpawnerRegistry()) {
            const FMythicDesignerSpawnerState &State = Reg->FindOrAdd(DesignerId);
            CachedSpawnsEver = State.SpawnsEver;
            bCachedPermaDead = State.bPermaDead;
            CachedLastDeathTime = State.LastDeathTime;
        }
    }

    // Already terminal => stay dormant (NO timer).
    if (bCachedPermaDead || CachedSpawnsEver >= MaxSpawnsEver) {
        return;
    }

    GetWorldTimerManager().SetTimer(EvalTimerHandle, this, &AMythicDesignerSpawner::TickEvaluate,
                                    EvaluationIntervalSeconds, /*bLoop=*/true);
}

void AMythicDesignerSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (GetWorld()) {
        GetWorldTimerManager().ClearTimer(EvalTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
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
    for (int32 i = LiveNPCs.Num() - 1; i >= 0; --i) {
        if (!LiveNPCs[i].IsValid()) {
            LiveNameHashes.Remove(LiveNPCs[i]);
            LiveNPCs.RemoveAtSwap(i);
        }
    }
}

void AMythicDesignerSpawner::TickEvaluate() {
    if (!HasAuthority()) {
        return;
    }

    ReapLiveNPCs();

    // Cheap caps gate before the (slightly heavier) input gather.
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
    GatherInputs(Inputs); // fills resolved flags; unresolved-required gates fail-safe inside EvaluateConditions

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
                    // Defensive clamp into [0,24): GetHours() is already 0..23 for a normalized timespan.
                    Hour = FMath::Fmod(Hour, 24.0f);
                    if (Hour < 0.0f) {
                        Hour += 24.0f;
                    }
                    return Hour;
                }
            }
        }
        // Fallback: derive an hour from world time on a fixed 24-minute day (1440s) when no clock is present. This is
        // only hit if the environment controller hasn't registered; a time-gated spawner without a clock degrades to
        // a slowly-cycling hour rather than crashing.
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

        // Range gate (or the tag-gate proximity scope): the player must be within PlayerRangeCm.
        if (bRangeGate || bTagGate) {
            if (!Pawn) {
                continue; // no body => cannot be in range / cannot carry pawn tags meaningfully
            }
            const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), MyLoc);
            if ((bRangeGate || bTagGate) && DistSq > RangeSq) {
                continue;
            }
        }

        // Tag gate: the player's ASC must own ALL required tags.
        if (bTagGate) {
            AActor *TagActor = Pawn ? static_cast<AActor *>(Pawn) : static_cast<AActor *>(PC);
            UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TagActor);
            if (!ASC || !ASC->HasAllMatchingGameplayTags(Conditions.RequiredPlayerTags)) {
                continue;
            }
        }

        // This player satisfied every active part of the player gate.
        return true;
    }

    return false;
}

bool AMythicDesignerSpawner::GatherInputs(FMythicDesignerConditionInputs &OutInputs) const {
    OutInputs.GameHour = GetCurrentGameHour();

    // Player gate result (the actor folds tags + proximity into a single bool the pure evaluator consumes).
    OutInputs.bAnyPlayerSatisfiesTags = AnyPlayerSatisfiesPlayerGate();

    UMythicLivingWorldSubsystem *LWS = GetLWS();
    UMythicFactionDatabase *DB = LWS ? LWS->GetFactionDatabase() : nullptr;

    // Faction-state gate input.
    if (DB && Conditions.GatingFactionTag.IsValid() &&
        Conditions.FactionState != EMythicDesignerFactionStatePredicate::Any) {
        FMythicFactionData Data;
        FMythicFactionId Id;
        if (DB->FindFactionByTag(Conditions.GatingFactionTag, Data, &Id) && Id.IsValid()) {
            OutInputs.GatingFactionStatus = Data.Status;
            OutInputs.bGatingFactionResolved = true;
        }
    }

    // Relation gate input.
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

    // ── Resolve the spawn transform ──
    FTransform SpawnTransform;
    if (bUseExactPlacedTransform) {
        SpawnTransform = GetActorTransform();
    }
    else {
        // Build placement params from the NPC class CDO's capsule.
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
            // No valid placement this eval — leave the timer running to retry next interval.
            return;
        }
        SpawnTransform = Validated;
    }

    // ── Deferred spawn so we can read the placed transform's cell before BeginPlay-driven systems run ──
    AMythicNPCCharacter *NPC = World->SpawnActorDeferred<AMythicNPCCharacter>(
        NPCClass, SpawnTransform, /*Owner=*/this, /*Instigator=*/nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!NPC) {
        UE_LOG(LogMythLivingWorld, Warning, TEXT("DesignerSpawner '%s': SpawnActorDeferred returned null."),
               *DesignerId.ToString());
        return;
    }
    NPC->FinishSpawning(SpawnTransform);

    // ── Compute the stable NameHash (deterministic per DesignerId + cell + faction) for the perma-death record ──
    uint8 FactionIndex = 0;
    if (UMythicLivingWorldSubsystem *LWS = GetLWS()) {
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
    if (UMythicLivingWorldSubsystem *LWS = GetLWS()) {
        if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
            Cell = Grid->WorldToCell(SpawnTransform.GetLocation());
        }
    }

    // Seed the SpawnIndex from a hash of DesignerId mixed with the cell so two spawners in the same cell+faction don't
    // alias one NameHash. Stable across runs (DesignerId is authored, cell is the placed transform).
    const int32 SpawnIndex = static_cast<int32>(
        GetTypeHash(DesignerId) ^ (static_cast<uint32>(Cell.X) << 8) ^ static_cast<uint32>(Cell.Y));
    const uint32 NameHash = FMythicNPCGenerator::GenerateNameHash(FactionIndex, Cell, SpawnIndex);

    // ── Bind death + bookkeeping ──
    if (NPC->LifeComponent) {
        NPC->LifeComponent->OnDeath.AddDynamic(this, &AMythicDesignerSpawner::OnDesignerNPCDeath);
    }

    const TWeakObjectPtr<AMythicNPCCharacter> WeakNPC(NPC);
    LiveNPCs.Add(WeakNPC);
    LiveNameHashes.Add(WeakNPC, NameHash);

    // ── Registry + cache ──
    if (UMythicLivingWorldSubsystem *LWS = GetLWS()) {
        if (UMythicDesignerSpawnerRegistry *Reg = LWS->GetDesignerSpawnerRegistry()) {
            Reg->RecordSpawn(DesignerId);
        }
    }
    ++CachedSpawnsEver;

    UE_LOG(LogMythLivingWorld, Log, TEXT("DesignerSpawner '%s' spawned NPC #%d (NameHash=%u, Cell=%s)."),
           *DesignerId.ToString(), CachedSpawnsEver, NameHash, *Cell.ToString());
}

void AMythicDesignerSpawner::OnDesignerNPCDeath(AActor *DeadActor) {
    if (!HasAuthority()) {
        return;
    }

    AMythicNPCCharacter *DeadNPC = Cast<AMythicNPCCharacter>(DeadActor);
    const TWeakObjectPtr<AMythicNPCCharacter> WeakDead(DeadNPC);

    // Resolve the cached NameHash for the perma-death record BEFORE we drop it from the live maps.
    uint32 DeadNameHash = 0;
    if (const uint32 *Found = LiveNameHashes.Find(WeakDead)) {
        DeadNameHash = *Found;
    }

    // Drop from the live set.
    LiveNPCs.RemoveAllSwap([&](const TWeakObjectPtr<AMythicNPCCharacter> &Ptr) {
        return Ptr == WeakDead || !Ptr.IsValid();
    });
    LiveNameHashes.Remove(WeakDead);

    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    CachedLastDeathTime = Now;

    // This is the terminal (perma-death) death only when the spawner has exhausted MaxSpawnsEver.
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

    // Route a terminal death through the PersistentNPCRegistry so the dead NameHash is permanently blocked from reuse
    // (mirrors the embodied-NPC perma-death contract). Only on a terminal death — non-terminal respawns are NOT perma.
    if (bPerma && LWS && LWS->IsSystemActive() && DeadNameHash != 0) {
        FMythicFactionId Faction; // governing faction of the gating tag (best-effort; invalid = factionless)
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
        if (UMythicPersistentNPCRegistry *PReg = LWS->GetPersistentNPCRegistry()) {
            PReg->RegisterDeath(DeadNameHash, Faction, FGameplayTag() /*empty role*/, Cell, Now, LWS);
        }
    }

    // Terminal => stop forever.
    if (bCachedPermaDead || CachedSpawnsEver >= MaxSpawnsEver) {
        StopEvaluation();
    }
}
