// Mythic Living World — Encounter Director Implementation
// Timer-driven encounter evaluation, spawning, and lifecycle management.

#include "World/LivingWorld/Encounters/EncounterDirector.h"
#include "World/LivingWorld/Encounters/EncounterTemplateDatabase.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h" // R18-M7-r1: shared global spawn-serial source
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h" // RequiredWorldState weather gate (GetWeather)
#include "MassEntitySubsystem.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "AI/NPCs/MythicNPCCharacter.h" // CleanupEncounter tears down embodied actors before freeing their entities
#include "World/LivingWorld/MythicTags_LivingWorld.h" // TAG_LIVINGWORLD_EVENT_ENCOUNTER_COMPLETED
#include "GAS/Executions/MythicCombatRoll.h"          // MythicCombat::RollSucceeds (shared 0/100% boundary rule)
#include "Mass/Tags/MythicMassTags.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogMythEncounter);

// ─────────────────────────────────────────────────────────────
// Subsystem Lifecycle
// ─────────────────────────────────────────────────────────────

bool UMythicEncounterDirector::ShouldCreateSubsystem(UObject *Outer) const {
    // Only create in game worlds, not editor previews
    if (const UWorld *World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicEncounterDirector::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    // Cache shared data references from the living world subsystem
    if (UGameInstance *GI = GetWorld()->GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            LivingWorld = LW; // game-thread event writes route through LW->SubmitWorldEvent (thread-safe queue)
            CausalFabric = LW->GetCausalFabric();
            FactionDB = LW->GetFactionDatabase();
            TerritoryGrid = LW->GetTerritoryGrid();
            Settings = LW->GetSettings();
        }
    }

    if (!CausalFabric || !FactionDB || !TerritoryGrid || !Settings) {
        UE_LOG(LogMythEncounter, Warning,
               TEXT("EncounterDirector: Could not cache living world references. Director disabled."));
        return;
    }

    // Load configuration from settings
    MaxActiveEncounters = Settings->MaxActiveEncounters;
    EvaluationInterval = Settings->EncounterEvaluationInterval;

    // Start evaluation timer
    GetWorld()->GetTimerManager().SetTimer(
        EvaluationTimerHandle,
        this,
        &UMythicEncounterDirector::EvaluationTick,
        EvaluationInterval,
        /*bLoop=*/true,
        /*InitialDelay=*/EvaluationInterval);

    ActiveEncounters.Reserve(MaxActiveEncounters);

    // Load encounter templates from data asset
    if (!Settings->EncounterTemplateDatabase.IsNull()) {
        if (UMythicEncounterTemplateDatabase *DB = Settings->EncounterTemplateDatabase.LoadSynchronous()) {
            for (const FMythicEncounterTemplate &Template : DB->Templates) {
                RegisterTemplate(Template);
            }
            UE_LOG(LogMythEncounter, Log,
                   TEXT("EncounterDirector: Loaded %d templates from data asset"), DB->GetTemplateCount());
        }
    }

    UE_LOG(LogMythEncounter, Log,
           TEXT("EncounterDirector initialized: EvalInterval=%.1fs, MaxActive=%d, Templates=%d"),
           EvaluationInterval, MaxActiveEncounters, Templates.Num());
}

void UMythicEncounterDirector::Deinitialize() {
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(EvaluationTimerHandle);
    }

    // Tear down every active encounter via CleanupEncounter (destroys MASS entities + any embodied actors + clears the
    // EmbodiedActors map entries) instead of just dropping the array — Empty() alone would orphan promoted actors and
    // leak their registry entries (the same leak the iter-105/108 fixes close on the timeout + population-despawn paths).
    // CleanupEncounter(last) RemoveAtSwaps the last index (no swap → clean pop); it null-checks GetWorld()/LivingWorld,
    // so it degrades safely if the world is already tearing down.
    while (ActiveEncounters.Num() > 0) {
        CleanupEncounter(ActiveEncounters.Num() - 1);
    }
    Templates.Empty();
    TemplateCooldowns.Empty();

    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────
// Template Registration
// ─────────────────────────────────────────────────────────────

void UMythicEncounterDirector::RegisterTemplate(const FMythicEncounterTemplate &Template) {
    if (!Template.EncounterTag.IsValid()) {
        UE_LOG(LogMythEncounter, Warning, TEXT("Attempted to register template with invalid tag"));
        return;
    }

    Templates.Add(Template);
    UE_LOG(LogMythEncounter, Log, TEXT("Registered encounter template: %s"), *Template.EncounterTag.ToString());
}

// ─────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────

bool UMythicEncounterDirector::HasEncounterInCell(const FMythicCellCoord &Cell) const {
    for (const FMythicActiveEncounter &E : ActiveEncounters) {
        if (E.Cell == Cell && E.State == EMythicEncounterState::Active) {
            return true;
        }
    }
    return false;
}

bool UMythicEncounterDirector::ForceCompleteEncounter(uint32 EncounterId) {
    for (int32 i = 0; i < ActiveEncounters.Num(); ++i) {
        if (ActiveEncounters[i].EncounterId == EncounterId) {
            ActiveEncounters[i].State = EMythicEncounterState::Completed;
            EmitEncounterCompletedEvent(ActiveEncounters[i], /*bDefeated*/ false); // forced close = generic Completed beat (not a player victory)
            CleanupEncounter(i);
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────
// Evaluation
// ─────────────────────────────────────────────────────────────

void UMythicEncounterDirector::EvaluationTick() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicEncounterDirector_Eval);

    if (!CausalFabric || !FactionDB || !TerritoryGrid) {
        return;
    }

    const double WorldTime = GetWorld()->GetTimeSeconds();

    // Step 1: Update existing encounters (timeout, lifecycle)
    UpdateActiveEncounters();

    // Step 2: Check global budget
    if (ActiveEncounters.Num() >= MaxActiveEncounters) {
        return;
    }

    // Step 3: Evaluate templates
    for (const FMythicEncounterTemplate &Template : Templates) {
        // Budget check
        if (ActiveEncounters.Num() >= MaxActiveEncounters) {
            break;
        }

        // Cooldown check
        if (IsOnCooldown(Template.EncounterTag, WorldTime, Template.CooldownSeconds)) {
            continue;
        }

        // Instance limit check
        if (CountActiveInstances(Template.EncounterTag) >= Template.MaxConcurrentInstances) {
            continue;
        }

        // Evaluate prerequisites
        FMythicCellCoord SpawnCell;
        FMythicFactionId SpawnFaction;
        if (!EvaluateTemplate(Template, SpawnCell, SpawnFaction)) {
            continue;
        }

        // Cell occupancy check
        if (HasEncounterInCell(SpawnCell)) {
            continue;
        }

        // Probability roll — shared boundary rule: BaseProbability <= 0 NEVER spawns (a disabled template must stay
        // disabled even on an exact-0.0 FRand()), >= 1 always spawns. The bare `FRand() > BaseProbability` skip let a
        // 0% template spawn when FRand() returned exactly 0.0.
        if (!MythicCombat::RollSucceeds(Template.BaseProbability, FMath::FRand())) {
            continue;
        }

        // Spawn the encounter
        SpawnEncounter(Template, SpawnCell, SpawnFaction);
        TemplateCooldowns.FindOrAdd(Template.EncounterTag) = WorldTime;
    }
}

bool UMythicEncounterDirector::EvaluateTemplate(
    const FMythicEncounterTemplate &Template,
    FMythicCellCoord &OutCell,
    FMythicFactionId &OutFaction) const {
    // World-state prerequisite (the RequiredWorldState tag query was declared but had zero consumers): the template
    // only spawns when its query matches the current world state. We surface the live WEATHER tag here (the env
    // subsystem owns the clock) so designers can author ambushes that only emerge in fog or raids that surge in storms;
    // time-of-day/season tags are a follow-up slice. An EMPTY query (the default) imposes no constraint, so every
    // existing template is backward-safe. If the env controller isn't available yet (a startup/teardown transient) the
    // world state is unknown — a weather-REQUIRING query then stays unmet (fail-closed for a prerequisite), while a
    // "NOT <weather>" query still passes.
    if (!Template.RequiredWorldState.IsEmpty()) {
        FGameplayTagContainer WorldState;
        if (const UWorld *World = GetWorld()) {
            if (const UGameInstance *GI = World->GetGameInstance()) {
                if (const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
                    // Controller-gate the read (GetWeather logs + returns EmptyTag when the controller is absent).
                    if (Env->GetEnvironmentController() != nullptr) {
                        const FGameplayTag Weather = Env->GetWeather();
                        if (Weather.IsValid()) {
                            WorldState.AddTag(Weather);
                        }
                        // Time-of-day + season round out the world state, so a designer can author e.g. a night-only
                        // ambush (Environment.Time.Night) or a winter raid (Environment.Season.Winter).
                        const FGameplayTag TimeOfDay = Env->GetDayTimeTag();
                        if (TimeOfDay.IsValid()) {
                            WorldState.AddTag(TimeOfDay);
                        }
                        const FGameplayTag Season = Env->GetSeasonTag();
                        if (Season.IsValid()) {
                            WorldState.AddTag(Season);
                        }
                    }
                    else {
                        // No EnvironmentController on this level → the world state is unknowable, so a query that
                        // REQUIRES a weather/time/season tag stays unmet below and the template is held back. This is
                        // deliberate fail-closed (we don't leak intentionally-rare env-gated content onto a clock-less
                        // level). On a normal map this self-heals once the controller's BeginPlay registers; on a
                        // PERSISTENTLY controller-less map (greybox/headless/forgotten) it is permanent — so warn ONCE
                        // per template to make the otherwise-silent suppression diagnosable.
                        static TSet<FGameplayTag> WarnedTemplates; // game-thread only (timer-driven EvaluateTemplate)
                        if (!WarnedTemplates.Contains(Template.EncounterTag)) {
                            WarnedTemplates.Add(Template.EncounterTag);
                            UE_LOG(LogMythEncounter, Warning,
                                   TEXT("EncounterDirector: template '%s' has a RequiredWorldState (weather/time/season) "
                                       "query but no EnvironmentController is present — it will NOT spawn until one "
                                       "registers. Place an AMythicEnvironmentController, or clear RequiredWorldState."),
                                   *Template.EncounterTag.ToString());
                        }
                    }
                }
            }
        }
        if (!Template.RequiredWorldState.Matches(WorldState)) {
            return false;
        }
    }

    // Snapshot alive factions ONCE (so the faction-relationship gate below doesn't nest ForEachAliveFaction —
    // GetRelationship is then read outside the iteration callback, avoiding any re-entrancy/lock concern).
    struct FCandidateFaction {
        FMythicFactionId Id;
        float Military;
        int32 Population;
    };
    TArray<FCandidateFaction> Candidates;
    FactionDB->ForEachAliveFaction([&](FMythicFactionId Id, const FMythicFactionData &Data) {
        Candidates.Add({Id, Data.MilitaryStrength, Data.Population});
    });

    // The relationship prerequisite only constrains CONFLICT encounters: MinFactionRelation more hostile than Neutral
    // (Unfriendly/Hostile — the enum is ascending hostility Allied=0..Hostile=4). Neutral (the default) / Friendly /
    // Allied impose no constraint (backward-safe), so existing templates are unaffected.
    const bool bRelationGated =
        static_cast<uint8>(Template.MinFactionRelation) > static_cast<uint8>(EMythicFactionRelation::Neutral);

    // Collect ALL qualifying factions, then pick one at random — previously this returned the FIRST qualifying faction,
    // so every encounter clustered in the lowest-index alive faction's territory (no spread across the world). Random
    // selection distributes encounters over all eligible factions. (Server-side only — no client determinism concern.)
    TArray<FMythicFactionId> QualifyingFactions;
    for (const FCandidateFaction &C : Candidates) {
        // Military strength check
        if (C.Military < Template.MinMilitaryStrength) {
            continue;
        }
        // Population check
        if (C.Population < Template.MinPopulation) {
            continue;
        }
        // Faction-relationship check: this faction must actually have a relationship at least as hostile as
        // MinFactionRelation with SOME other alive faction (e.g. a raid spawns only where a war exists).
        if (bRelationGated) {
            bool bHasQualifyingRelation = false;
            for (const FCandidateFaction &Other : Candidates) {
                if (Other.Id == C.Id) {
                    continue;
                }
                if (static_cast<uint8>(FactionDB->GetRelationship(C.Id, Other.Id)) >= static_cast<uint8>(Template.MinFactionRelation)) {
                    bHasQualifyingRelation = true;
                    break;
                }
            }
            if (!bHasQualifyingRelation) {
                continue;
            }
        }

        // Qualifies only if it actually holds territory to host the encounter (cheap existence probe — fetch 1 cell).
        TArray<FMythicCellCoord> Probe;
        TerritoryGrid->GetFactionCells(C.Id, 1, Probe);
        if (Probe.Num() > 0) {
            QualifyingFactions.Add(C.Id);
        }
    }

    if (QualifyingFactions.Num() == 0) {
        return false;
    }

    // Pick a random qualifying faction, then a random cell within its territory.
    const FMythicFactionId ChosenFaction = QualifyingFactions[FMath::RandRange(0, QualifyingFactions.Num() - 1)];
    TArray<FMythicCellCoord> ChosenCells;
    TerritoryGrid->GetFactionCells(ChosenFaction, 32, ChosenCells);
    if (ChosenCells.Num() == 0) {
        return false; // territory emptied between the qualify probe and now — bail safely
    }
    OutCell = ChosenCells[FMath::RandRange(0, ChosenCells.Num() - 1)];
    OutFaction = ChosenFaction;
    return true;
}

void UMythicEncounterDirector::SpawnEncounter(
    const FMythicEncounterTemplate &Template,
    const FMythicCellCoord &Cell,
    FMythicFactionId Faction) {
    FMythicActiveEncounter NewEncounter;
    NewEncounter.EncounterId = NextEncounterId++;
    NewEncounter.TemplateTag = Template.EncounterTag;
    NewEncounter.State = EMythicEncounterState::Active;
    NewEncounter.Cell = Cell;
    NewEncounter.OriginFaction = Faction;
    NewEncounter.ActivationTime = GetWorld()->GetTimeSeconds();
    NewEncounter.MaxDurationSeconds = Template.MaxDurationSeconds;
    NewEncounter.EntityCount = Template.EntityCount;

    // ─── Spawn MASS entities for this encounter ──────────
    UMassEntitySubsystem *MassSubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
    if (MassSubsystem) {
        FMassEntityManager &EntityManager = MassSubsystem->GetMutableEntityManager();

        // Build archetype: Identity + Schedule + Significance + NPC tag + Encounter tag. The Encounter tag keeps these
        // entities out of the ambient PopulationSpawnerProcessor (its density count + far-from-player despawn), so the
        // EncounterDirector alone manages their lifecycle (via CleanupEncounter) — without it the population despawner
        // silently destroyed encounter members the moment no player was nearby (phantom encounters).
        const UScriptStruct *Composition[] = {
            FMythicIdentityFragment::StaticStruct(),
            FMythicScheduleFragment::StaticStruct(),
            FMythicSignificanceFragment::StaticStruct(),
            FMythicNPCTag::StaticStruct(),
            FMythicEncounterEntityTag::StaticStruct()
        };
        FMassArchetypeHandle Archetype = EntityManager.CreateArchetype(MakeArrayView(Composition));

        // Batch-create entities
        TArray<FMassEntityHandle> SpawnedEntities;
        EntityManager.BatchCreateEntities(Archetype, Template.EntityCount, SpawnedEntities);

        // R18-M7-r1: the registry's global monotonic AllocateSpawnSerial is the shared collision-free NameHash source.
        UMythicPersistentNPCRegistry *Registry = LivingWorld ? LivingWorld->GetPersistentNPCRegistry() : nullptr;

        for (int32 i = 0; i < SpawnedEntities.Num(); ++i) {
            FMassEntityHandle Entity = SpawnedEntities[i];

            // Identity — faction + encounter cell + unique hash per entity
            FMythicIdentityFragment &Identity = EntityManager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity);
            Identity.Faction = Faction;
            Identity.Cell = Cell;
            // R18-M7-r1: unify the NameHash source with the population spawner — draw from the registry's global
            // monotonic serial via GenerateNameHash so encounter + ambient NPCs share ONE collision-free identity
            // source and can't alias in the shared DeadNPCHashes perma-death set. (Registry is created at LWS init
            // before encounters fire; the legacy formula stays only as a last-resort fallback.)
            Identity.NameHash = Registry
                ? FMythicNPCGenerator::GenerateNameHash(Faction.Index, Cell, Registry->AllocateSpawnSerial())
                : (GetTypeHash(Cell) ^ (NewEncounter.EncounterId * 2654435761u) ^ (i * 131071u));
            Identity.VisualArchetype = static_cast<uint8>(Identity.NameHash % 8);

            // Schedule — encounter NPCs have no home/work, they operate at the encounter cell
            FMythicScheduleFragment &Schedule = EntityManager.GetFragmentDataChecked<FMythicScheduleFragment>(Entity);
            Schedule.Phase = EMythicSchedulePhase::Idle;
            Schedule.HomeCell = Cell;
            Schedule.WorkCell = Cell;

            // Significance — start at Tier 0, significance system will promote if near player
            FMythicSignificanceFragment &Sig = EntityManager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity);
            Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
        }

        NewEncounter.SpawnedEntities = MoveTemp(SpawnedEntities);

        UE_LOG(LogMythEncounter, Log,
               TEXT("Encounter %d: spawned %d MASS entities at (%d,%d)"),
               NewEncounter.EncounterId, Template.EntityCount, Cell.X, Cell.Y);
    }
    else {
        UE_LOG(LogMythEncounter, Warning,
               TEXT("Encounter %d: MassEntitySubsystem not available — entities not spawned"),
               NewEncounter.EncounterId);
    }

    ActiveEncounters.Add(MoveTemp(NewEncounter));

    // Submit the encounter event through the subsystem's THREAD-SAFE queue — NOT CausalFabric->AppendEvent directly.
    // SpawnEncounter runs on the GAME thread (timer-driven), but the fabric is a lock-free single-writer owned by the
    // sim thread (which drains PendingEvents on its own thread before CommitWrites). A direct game-thread AppendEvent
    // would race the sim thread's ring/index mutation and corrupt the shared world log.
    if (LivingWorld) {
        FMythicWorldEvent Event;
        Event.EventTag = Template.EncounterTag;
        Event.Cell = Cell;
        Event.PrimaryFaction = Faction;
        Event.Significance = 0.5f;
        // An encounter is a spawned world threat, not a social interaction. Stamp it Encounter (a MacroMask category)
        // so the World Chronicle surfaces it as player-facing news (was Social, which MacroMask drops at the gate).
        Event.CategoryFlags = EMythicEventCategory::Encounter;

        LivingWorld->SubmitWorldEvent(Event);
    }

    UE_LOG(LogMythEncounter, Log,
           TEXT("Encounter %d activated: %s at (%d,%d) (Faction %d, %d entities)"),
           ActiveEncounters.Last().EncounterId,
           *Template.EncounterTag.ToString(),
           Cell.X, Cell.Y,
           Faction.Index,
           Template.EntityCount);
}

void UMythicEncounterDirector::UpdateActiveEncounters() {
    const double WorldTime = GetWorld()->GetTimeSeconds();

    // Resolved once for the defeat scan below (all game-thread — UpdateActiveEncounters is a timer callback).
    UMassEntitySubsystem *MassSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr;
    const UMythicPersistentNPCRegistry *Registry = LivingWorld ? LivingWorld->GetPersistentNPCRegistry() : nullptr;

    for (int32 i = ActiveEncounters.Num() - 1; i >= 0; --i) {
        FMythicActiveEncounter &Encounter = ActiveEncounters[i];
        bool bDefeated = false; // true = completed via player wipeout (below); false = timeout/withdrawal

        // Defeat detection: an Active encounter whose every spawned NPC is dead (in the perma-death registry) or gone
        // is RESOLVED — complete it now instead of lingering to MaxDuration (which would hold a per-cluster budget slot
        // and fire a stale "completed" beat minutes after the player actually won). The perma-death registry is the
        // CORRECT signal, not entity-validity: the EncounterDirector is the sole entity-lifecycle authority (it frees
        // them only on cleanup), and a dehydrated-but-ALIVE member (player engaged then walked away) is NOT perma-dead,
        // so it rightly keeps the encounter active until timeout. Skipped until something has spawned (an empty list
        // must not vacuously complete a just-created encounter).
        if (Encounter.State == EMythicEncounterState::Active && MassSubsystem && Registry && Encounter.SpawnedEntities.Num() > 0) {
            const FMassEntityManager &EntityManager = MassSubsystem->GetEntityManager();
            bool bAllDown = true;
            for (const FMassEntityHandle &Entity : Encounter.SpawnedEntities) {
                if (!EntityManager.IsEntityValid(Entity)) {
                    continue; // already-destroyed entity = gone = counts as down
                }
                const FMythicIdentityFragment *Id = EntityManager.GetFragmentDataPtr<FMythicIdentityFragment>(Entity);
                if (!Id || !Registry->IsPermaDead(Id->NameHash)) {
                    bAllDown = false; // a live, not-perma-dead member remains — the threat isn't resolved
                    break;
                }
            }
            if (bAllDown) {
                Encounter.State = EMythicEncounterState::Completed;
                bDefeated = true;
            }
        }

        // Check timeout (leaves bDefeated false — the threat withdrew rather than being beaten)
        if (Encounter.HasTimedOut(WorldTime)) {
            Encounter.State = EMythicEncounterState::Completed;
        }

        // Clean up completed encounters
        if (Encounter.State == EMythicEncounterState::Completed) {
            EmitEncounterCompletedEvent(Encounter, bDefeated); // Defeated vs Completed beat (single source — see the helper)
            CleanupEncounter(i);
        }
    }
}

void UMythicEncounterDirector::CleanupEncounter(int32 Index) {
    if (!ActiveEncounters.IsValidIndex(Index)) {
        return;
    }

    FMythicActiveEncounter &Encounter = ActiveEncounters[Index];

    // Destroy spawned MASS entities
    if (Encounter.SpawnedEntities.Num() > 0) {
        UMassEntitySubsystem *MassSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr;
        if (MassSubsystem) {
            FMassEntityManager &EntityManager = MassSubsystem->GetMutableEntityManager();
            int32 DestroyedCount = 0;
            for (const FMassEntityHandle &Entity : Encounter.SpawnedEntities) {
                // Tear down any embodied cognitive actor FIRST. An encounter member promoted to Tier2 near a player has
                // an AMythicNPCCharacter linked in the subsystem's EmbodiedActors map (the SignificanceProcessor promotes
                // encounter entities like any NPC — they're excluded only from the population despawner, not promotion).
                // Destroying just the MASS entity would orphan that actor (its CognitiveBrain SourceEntity left dangling)
                // AND leak the EmbodiedActors[Entity] entry — the link's only other teardown is the actor's own EndPlay
                // or the dehydration despawn loop, neither of which fires on a direct entity destroy. This makes the
                // EncounterDirector the TRUE sole lifecycle authority (mirrors ActorSpawnProcessor's dehydration).
                if (LivingWorld) {
                    if (AMythicNPCCharacter *Actor = LivingWorld->FindEmbodiedActor(Entity)) {
                        Actor->Destroy();
                    }
                    LivingWorld->UnregisterEmbodiedActor(Entity); // idempotent — also clears a stale key if no actor
                }
                if (EntityManager.IsEntityValid(Entity)) {
                    EntityManager.DestroyEntity(Entity);
                    ++DestroyedCount;
                }
            }
            UE_LOG(LogMythEncounter, Log,
                   TEXT("Encounter %d: destroyed %d/%d MASS entities"),
                   Encounter.EncounterId, DestroyedCount, Encounter.SpawnedEntities.Num());
        }
    }

    UE_LOG(LogMythEncounter, Log,
           TEXT("Encounter %d completed/cleaned up: %s"),
           Encounter.EncounterId,
           *Encounter.TemplateTag.ToString());

    ActiveEncounters.RemoveAtSwap(Index);
}

void UMythicEncounterDirector::EmitEncounterCompletedEvent(const FMythicActiveEncounter &Encounter, bool bDefeated) const {
    // Mirror the SpawnEncounter spawn beat: surface the encounter's END as a chronicle event (the symmetric close of
    // the story the spawn opened). Through the subsystem's THREAD-SAFE queue (game thread → sim-thread fabric), NOT a
    // direct AppendEvent. CategoryFlags=Encounter so the chronicle MacroMask surfaces it. The leaf reflects the OUTCOME:
    // DEFEATED (player wiped out every spawned member) renders "Encounter Defeated — <faction>", vs the generic
    // COMPLETED ("Encounter Completed — <faction>") for a timeout/withdrawal/force-complete — so the news shows agency.
    // Defeat is the more salient beat, so it carries slightly higher significance.
    if (!LivingWorld) {
        return;
    }
    FMythicWorldEvent CompletedEvent;
    CompletedEvent.EventTag = bDefeated ? TAG_LIVINGWORLD_EVENT_ENCOUNTER_DEFEATED : TAG_LIVINGWORLD_EVENT_ENCOUNTER_COMPLETED;
    CompletedEvent.Cell = Encounter.Cell;
    CompletedEvent.PrimaryFaction = Encounter.OriginFaction;
    CompletedEvent.Significance = bDefeated ? 0.6f : 0.5f;
    CompletedEvent.CategoryFlags = EMythicEventCategory::Encounter;
    LivingWorld->SubmitWorldEvent(CompletedEvent);
}

// ─────────────────────────────────────────────────────────────
// Cooldown Helpers
// ─────────────────────────────────────────────────────────────

bool UMythicEncounterDirector::IsOnCooldown(const FGameplayTag &TemplateTag, double WorldTime, float CooldownSeconds) const {
    const double *LastActivation = TemplateCooldowns.Find(TemplateTag);
    if (!LastActivation) {
        return false;
    }
    return (WorldTime - *LastActivation) < static_cast<double>(CooldownSeconds);
}

int32 UMythicEncounterDirector::CountActiveInstances(const FGameplayTag &TemplateTag) const {
    int32 Count = 0;
    for (const FMythicActiveEncounter &E : ActiveEncounters) {
        if (E.TemplateTag == TemplateTag && E.State == EMythicEncounterState::Active) {
            ++Count;
        }
    }
    return Count;
}
