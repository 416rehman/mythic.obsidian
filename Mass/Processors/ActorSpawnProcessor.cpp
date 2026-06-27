// Mythic Living World — Actor Spawn Processor Implementation

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
    // Server/standalone only, game thread (spawns actors). Mirrors UMythicPopulationSpawnerProcessor.
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // The spawn-request archetype may not exist at startup; don't let MASS prune this processor.
    QueryBasedPruning = EMassQueryBasedPruning::Never;

    SpawnActorClass = AMythicNPCCharacter::StaticClass();
    SpawnCreatureClass = AMythicCreatureCharacter::StaticClass();
    SpawnRequestQuery.RegisterWithProcessor(*this);
    CreatureSpawnRequestQuery.RegisterWithProcessor(*this);
    DespawnRequestQuery.RegisterWithProcessor(*this);
}

void UMythicActorSpawnProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // Humanoid branch: Identity + spawn-request tag, but EXCLUDE creatures. Without the FMythicCreatureTag(None)
    // exclusion a creature entity (which also carries Identity + the spawn-request tag when force-embodied) would match
    // here and be embodied as the humanoid EmbodiedNPCClass — the bug this two-query split fixes.
    SpawnRequestQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    SpawnRequestQuery.AddTagRequirement<FMythicActorSpawnRequestTag>(EMassFragmentPresence::All);
    SpawnRequestQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::None);

    // Creature branch: Identity + spawn-request tag + the creature tag. Mutually exclusive with the humanoid query
    // above (one requires FMythicCreatureTag, the other excludes it), so each spawn request matches exactly one query.
    CreatureSpawnRequestQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    CreatureSpawnRequestQuery.AddTagRequirement<FMythicActorSpawnRequestTag>(EMassFragmentPresence::All);
    CreatureSpawnRequestQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);

    // Embodied entities that asked to despawn (de-promoted below Tier 2).
    DespawnRequestQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    DespawnRequestQuery.AddTagRequirement<FMythicCognitiveTag>(EMassFragmentPresence::All);
    DespawnRequestQuery.AddTagRequirement<FMythicActorDespawnRequestTag>(EMassFragmentPresence::All);
}

namespace {
    // Pull the capsule footprint from a resolved actor class's CDO so the placement overlap test uses the REAL body
    // size (a fat creature won't squeeze into a humanoid-sized gap). ACharacter creates its capsule in the ctor, so the
    // CDO's capsule is valid. Falls back to the placement-params defaults if the class isn't a character. Cheap; the
    // resolved class is fixed per processor run, but reading the CDO each spawn is a trivial pointer chase.
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
    // Build the placement params from the FROZEN settings contract. Capsule dims come from the resolved class CDO so a
    // creature and a humanoid use their own footprints.
    FMythicPlacementParams Params;
    Params.CellCenterXY = CellCenterXY;
    Params.ScatterRadius = Settings->SpawnScatterRadius;
    Params.NavExtent = Settings->NavProjectionExtent;
    Params.bRequireReachability = Settings->bRequireReachability;
    Params.RetryBudget = Settings->SpawnRetryBudget;
    Params.bWaterCapable = bWaterCapable;
    GetCapsuleDimsFromClass(ResolvedClass, Params.CapsuleRadius, Params.CapsuleHalfHeight);

    // Charge ONE validation against the per-tick no-hitch budget (the FindValidSpawn cost is bounded by RetryBudget,
    // counted as one unit here — the caller has already checked the budget isn't spent before calling).
    ++ValidationsThisTick;

    if (MythicPlacement::FindValidSpawn(World, Params, OutTransform)) {
        // Success — clear any stale defer stamp for this entity.
        SpawnDeferUntil.Remove(Entity);
        return true;
    }

    // No valid candidate within the retry budget. Stamp the per-entity cooldown so this failing cell is retried at most
    // once per SpawnDeferCooldownSeconds window — NOT every frame (the kill-switch for the infinite retry loop).
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

        // Beyond ViewGateMinSpawnDistance a pop/vanish is imperceptible — not gated.
        if (DistSq > MinDistSq) {
            continue;
        }

        // Within range: is the actor inside the (margin-widened) view cone? Half-angle = half horizontal FOV + margin.
        // Use the cached POV FOV (matches the locked fallback in §4 so this never depends on a possibly-hidden
        // GetFOVAngle()).
        const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
        const FVector CamFwd = CamRot.Vector();
        const float HalfFOVRad = FMath::DegreesToRadians(0.5f * PC->PlayerCameraManager->GetFOVAngle());
        const float HalfConeRad = FMath::Min(PI, HalfFOVRad + MarginRad);

        const FVector DirToActor = ToActor.GetSafeNormal();
        if (DirToActor.IsNearlyZero()) {
            // Camera essentially on top of the actor — treat as in-view (don't vanish it).
            return true;
        }
        const float CosAngle = FVector::DotProduct(CamFwd, DirToActor);
        if (CosAngle >= FMath::Cos(HalfConeRad)) {
            return true; // In a player's close view → must not despawn this tick.
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
        // System is active so this is normally non-null; bail defensively rather than spawn with no placement contract.
        return;
    }

    // Per-Execute no-hitch budget: cap the number of navmesh/overlap validations across ALL spawn requests this tick.
    // When spent, the remaining requests re-queue (their spawn-request tag is re-added) for the next tick. Bounds the
    // cost of a town streaming in all at once to MaxPlacementValidationsPerTick * (per-validation cost).
    const int32 MaxValidations = FMath::Max(1, Settings->MaxPlacementValidationsPerTick);
    int32 ValidationsThisTick = 0;
    const double Now = World->GetTimeSeconds();

    // Track which entities are still requesting a spawn this tick, so SpawnDeferUntil entries for entities that no
    // longer want a body (despawned, demoted, destroyed) are pruned — bounding the map's growth.
    TSet<FMassEntityHandle> ActiveSpawnRequests;

    // Resolve the embodied-NPC class once: the designer-assigned mesh-bearing BP in the Living World settings wins,
    // else the processor's SpawnActorClass, else the bare C++ class. Cached so the soft class isn't re-resolved per
    // spawn (and the one-time synchronous BP load happens at most once, never inside the per-request loop). Execute
    // only reaches here once the system is active, so GetSettings() is non-null — no stale-fallback-cache risk.
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

    // Resolve the embodied-CREATURE class once (same hoist discipline as the humanoid class). The designer's
    // EmbodiedCreatureClass soft class wins; else the processor's SpawnCreatureClass; else the bare C++
    // AMythicCreatureCharacter. Unlike the humanoid class this is allowed to resolve to NULL: a designer can null
    // EmbodiedCreatureClass to keep creatures MASS-only (no visible actor). bCreatureClassResolved latches the attempt
    // so a deliberately-null setting doesn't re-trigger a load every tick. Settings is non-null here (system is active).
    if (!bCreatureClassResolved) {
        bCreatureClassResolved = true;
        ResolvedCreatureClass = SpawnCreatureClass;
        if (!ResolvedCreatureClass) {
            ResolvedCreatureClass = AMythicCreatureCharacter::StaticClass();
        }
        if (!Settings->EmbodiedCreatureClass.IsNull()) {
            // A SET-but-unloadable soft class is treated as an explicit "no creature actor": resolve to null so the
            // branch consumes the tag and the creature stays MASS-only, rather than silently falling back to the
            // C++ default (which would contradict the designer's authored choice).
            ResolvedCreatureClass = Settings->EmbodiedCreatureClass.LoadSynchronous();
        }
    }

    // Collect requests first. Spawning actors while iterating the entity chunks is unsafe, so gather (entity,
    // identity) pairs in the query, then spawn after the iteration completes.
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

        // This entity is (still) requesting a body — keep its defer stamp from being pruned below.
        ActiveSpawnRequests.Add(Entity);

        // Per-entity placement defer: a recently-failed cell waits SpawnDeferCooldownSeconds before retrying. KEEP the
        // spawn-request tag (do NOT consume) so the request survives to the next eligible tick, and skip this entity.
        if (const double *DeferUntil = SpawnDeferUntil.Find(Entity)) {
            if (*DeferUntil > Now) {
                continue;
            }
        }

        // Never embody a perma-dead NPC (the direct Tier0->Tier2 promotion path doesn't check this). This is a terminal
        // decision (the entity can never embody), so consume the tag + drop any defer stamp.
        if (Registry && Registry->IsPermaDead(Identity.NameHash)) {
            Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Remove(Entity);
            continue;
        }

        // Already embodied? Don't spawn a SECOND actor for one entity. The SpawnRequestQuery does NOT exclude the
        // FMythicCognitiveTag added below, so a re-issued spawn request (a future re-promotion / save-restore path)
        // would otherwise double-embody this entity. The entity→actor reverse link is the single source of truth for
        // "has an actor". Consume the tag (terminal — it already has a body) and clear any defer stamp.
        if (LWS->FindEmbodiedActor(Entity)) {
            Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Remove(Entity);
            continue;
        }

        // Per-tick validation budget spent: do NOT consume the tag — leave the request queued so it's reconsidered
        // next tick. Breaking (not continuing) avoids charging the loop for entities we won't validate anyway.
        if (ValidationsThisTick >= MaxValidations) {
            break;
        }

        // Rigorous placement: find a navmesh-valid, reachable, non-overlapping, non-water transform near the cell
        // center (replaces the old air-drop at Grid->CellToWorld's Z=0). Humanoids are never water-capable.
        const FVector CellCenterXY = Grid->CellToWorld(Identity.Cell);
        FTransform SpawnTM;

        // Settlement spawn-point FAST-PATH (Step 3, additive — not a new embodiment branch): if the population spawner
        // stamped a precomputed, navmesh-valid anchor on this entity, re-validate just its occupancy (cheap capsule
        // overlap, no project/scatter) using the resolved class's REAL capsule. On success we skip the full pipeline; on
        // a now-occupied point we FALL THROUGH to TryFindSpawnTransform (the verbatim cell-center path). The fast-path
        // validation is itself charged against the per-tick budget so it can't be used to dodge the no-hitch cap.
        bool bPlaced = false;
        if (Identity.bHasSpawnOverride) {
            float CapRadius = 0.0f, CapHalfHeight = 0.0f;
            GetCapsuleDimsFromClass(ResolvedSpawnClass, CapRadius, CapHalfHeight);
            ++ValidationsThisTick;
            if (MythicPlacement::ValidateExistingPoint(World, Identity.SpawnOverridePos, CapRadius, CapHalfHeight,
                                                       /*bWaterCapable=*/false, SpawnTM)) {
                SpawnDeferUntil.Remove(Entity); // success — clear any stale defer stamp (mirrors TryFindSpawnTransform)
                bPlaced = true;
            }
        }

        if (!bPlaced &&
            !TryFindSpawnTransform(World, Settings, CellCenterXY, ResolvedSpawnClass, /*bWaterCapable=*/false, Now, ValidationsThisTick, Entity, SpawnTM)) {
            // Placement deferred (TryFindSpawnTransform stamped the cooldown). KEEP the spawn-request tag so the entity
            // retries after the cooldown — never spawn invalid, never busy-loop.
            continue;
        }

        // Valid spot found — consume the request and acquire a body (pooled reuse or fresh spawn).
        Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);

        AMythicNPCCharacter *NPC = LWS->AcquireEmbodiedActor(ResolvedSpawnClass, SpawnTM.GetLocation(), SpawnTM.Rotator());
        if (!NPC) {
            // Acquire genuinely failed (bad class / no world). Don't loop — defer like a placement failure so we don't
            // hammer a broken class every tick. Re-add the tag (already consumed above) so it can retry post-cooldown.
            Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Add(Entity, Now + static_cast<double>(Settings->SpawnDeferCooldownSeconds));
            continue;
        }

        // Bind the actor to its source entity (faction / personality / cognitive brain). On a pooled reuse the actor's
        // WakeFromPool already re-armed GAS/brain inside Acquire; InitializeFromMassEntity then re-binds entity scalars.
        NPC->InitializeFromMassEntity(Entity);

        // Record the entity->actor reverse link so dehydration can find + despawn this actor on de-promotion.
        LWS->RegisterEmbodiedActor(Entity, NPC);

        // Mark the entity embodied so it isn't re-spawned while an actor exists.
        Context.Defer().AddTag<FMythicCognitiveTag>(Entity);
    }

    // ─── Creature embodiment: gather creature spawn requests, then spawn AMythicCreatureCharacter ───
    // Same gather-then-spawn discipline as the humanoid branch (spawning actors during chunk iteration is unsafe).
    // Creatures register into the SAME EmbodiedActors map (a creature IS-A AMythicNPCCharacter), so dehydration below
    // and the population/encounter far-despawn paths tear them down with zero extra wiring.
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

        // This entity is (still) requesting a body — keep its defer stamp from being pruned below.
        ActiveSpawnRequests.Add(Entity);

        // No creature actor class configured (designer chose MASS-only wildlife): terminal — consume the tag, spawn
        // nothing, drop any defer stamp.
        if (!ResolvedCreatureClass) {
            Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Remove(Entity);
            continue;
        }

        // Per-entity placement defer: a recently-failed cell waits before retrying. KEEP the spawn-request tag.
        if (const double *DeferUntil = SpawnDeferUntil.Find(Entity)) {
            if (*DeferUntil > Now) {
                continue;
            }
        }

        // Already embodied? Don't spawn a second actor (mirrors the humanoid guard above). Terminal — consume tag.
        if (LWS->FindEmbodiedActor(Entity)) {
            Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Remove(Entity);
            continue;
        }

        // Per-tick validation budget spent: leave the request queued for next tick (do NOT consume the tag).
        if (ValidationsThisTick >= MaxValidations) {
            break;
        }

        // Rigorous placement near the cell center. v1: creatures are NOT water-capable (FMythicCreatureSpeciesRow has no
        // water flag yet, and IsOverWater is a stub) — when a per-species bWaterCapable lands, source it here from the
        // creature fragment/species row WITHOUT changing this call shape.
        const FVector CellCenterXY = Grid->CellToWorld(Identity.Cell);
        FTransform SpawnTM;
        if (!TryFindSpawnTransform(World, Settings, CellCenterXY, ResolvedCreatureClass, /*bWaterCapable=*/false, Now, ValidationsThisTick, Entity, SpawnTM)) {
            // Placement deferred — KEEP the spawn-request tag so the creature retries after the cooldown.
            continue;
        }

        // Valid spot — consume the request and acquire a creature body (pooled reuse or fresh spawn). The pool is keyed
        // by CONCRETE UClass, so the creature class has its own bucket and is never handed out as a humanoid.
        Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);

        AMythicNPCCharacter *Creature = LWS->AcquireEmbodiedActor(ResolvedCreatureClass, SpawnTM.GetLocation(), SpawnTM.Rotator());
        if (!Creature) {
            Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);
            SpawnDeferUntil.Add(Entity, Now + static_cast<double>(Settings->SpawnDeferCooldownSeconds));
            continue;
        }

        // Bind the actor to its source creature entity (species / pack / aggression — NO humanoid brain).
        Creature->InitializeFromMassEntity(Entity);

        // Record the entity->actor reverse link. The pool/map are typed on AMythicNPCCharacter* (creature IS-A NPC
        // character), so the EmbodiedActors map + the entire despawn path are reused unchanged.
        LWS->RegisterEmbodiedActor(Entity, Creature);

        // Mark embodied so it isn't re-spawned while an actor exists.
        Context.Defer().AddTag<FMythicCognitiveTag>(Entity);
    }

    // ─── Dehydration: despawn actors whose entity dropped below Tier 2 ───
    // Gather first (don't mutate during chunk iteration), then act.
    TArray<FMassEntityHandle> Despawns;
    DespawnRequestQuery.ForEachEntityChunk(Context, [&Despawns](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        for (int32 i = 0; i < NumEntities; ++i) {
            Despawns.Add(ChunkContext.GetEntity(i));
        }
    });

    for (const FMassEntityHandle Entity : Despawns) {
        AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(Entity);

        // no-vanish-in-front: if the actor is in a player's close view, SKIP the despawn this tick and KEEP both
        // tags so it's reconsidered next tick (it despawns the instant the player looks away or moves past
        // ViewGateMinSpawnDistance). Grace is NOT applied to despawn — once de-significant, only the in-view guard
        // holds it. If there is no actor (already gone), fall through and just clear the tags.
        if (Actor && IsActorInCloseView(World, Actor, Settings)) {
            continue;
        }

        // Consume both tags so the entity isn't re-visited and becomes re-embodiable later.
        Context.Defer().RemoveTag<FMythicActorDespawnRequestTag>(Entity);
        Context.Defer().RemoveTag<FMythicCognitiveTag>(Entity);

        // Return the body to the pool (or Destroy if pooling is off / over cap). ReleaseEmbodiedActor calls
        // UnregisterEmbodiedActor FIRST internally — so do NOT unregister again here (that was the old redundant call;
        // double-unregister is harmless but the release path is now the single sink). Idempotent on a null Actor.
        LWS->ReleaseEmbodiedActor(Entity, Actor);

        // This entity no longer holds a body and isn't requesting one — drop any stale placement-defer stamp.
        SpawnDeferUntil.Remove(Entity);
    }

    // ─── Prune the placement-defer map (bounded growth) ───
    // Drop cooldown stamps for entities that are NOT requesting a spawn this tick (they despawned, demoted, or were
    // destroyed). Without this the map would accumulate dead entity handles forever. O(map size); the map is small
    // (only currently-deferring entities). Done after the spawn loops so ActiveSpawnRequests is fully populated.
    if (SpawnDeferUntil.Num() > 0) {
        for (auto It = SpawnDeferUntil.CreateIterator(); It; ++It) {
            if (!ActiveSpawnRequests.Contains(It.Key())) {
                It.RemoveCurrent();
            }
        }
    }
}
