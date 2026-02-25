// Mythic Living World — Encounter Director Implementation
// Timer-driven encounter evaluation, spawning, and lifecycle management.

#include "World/LivingWorld/Encounters/EncounterDirector.h"
#include "World/LivingWorld/Encounters/EncounterTemplateDatabase.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "MassEntitySubsystem.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogMythEncounter);

// ─────────────────────────────────────────────────────────────
// Subsystem Lifecycle
// ─────────────────────────────────────────────────────────────

bool UMythicEncounterDirector::ShouldCreateSubsystem(UObject* Outer) const {
    // Only create in game worlds, not editor previews
    if (const UWorld* World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicEncounterDirector::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);

    // Cache shared data references from the living world subsystem
    if (UGameInstance* GI = GetWorld()->GetGameInstance()) {
        if (UMythicLivingWorldSubsystem* LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
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
        if (UMythicEncounterTemplateDatabase* DB = Settings->EncounterTemplateDatabase.LoadSynchronous()) {
            for (const FMythicEncounterTemplate& Template : DB->Templates) {
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

    ActiveEncounters.Empty();
    Templates.Empty();
    TemplateCooldowns.Empty();

    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────
// Template Registration
// ─────────────────────────────────────────────────────────────

void UMythicEncounterDirector::RegisterTemplate(const FMythicEncounterTemplate& Template) {
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

bool UMythicEncounterDirector::HasEncounterInCell(const FMythicCellCoord& Cell) const {
    for (const FMythicActiveEncounter& E : ActiveEncounters) {
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
    for (const FMythicEncounterTemplate& Template : Templates) {
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

        // Probability roll
        if (FMath::FRand() > Template.BaseProbability) {
            continue;
        }

        // Spawn the encounter
        SpawnEncounter(Template, SpawnCell, SpawnFaction);
        TemplateCooldowns.FindOrAdd(Template.EncounterTag) = WorldTime;
    }
}

bool UMythicEncounterDirector::EvaluateTemplate(
    const FMythicEncounterTemplate& Template,
    FMythicCellCoord& OutCell,
    FMythicFactionId& OutFaction) const {
    // Iterate alive factions to find one meeting prerequisites
    bool bFound = false;

    FactionDB->ForEachAliveFaction([&](FMythicFactionId Id, const FMythicFactionData& Data) {
        if (bFound) return;

        // Military strength check
        if (Data.MilitaryStrength < Template.MinMilitaryStrength) {
            return;
        }

        // Population check
        if (Data.Population < Template.MinPopulation) {
            return;
        }

        // This faction qualifies — find a cell in their territory
        TArray<FMythicCellCoord> FactionCells;
        TerritoryGrid->GetFactionCells(Id, 32, FactionCells);

        if (FactionCells.Num() > 0) {
            // Pick a random cell in their territory
            OutCell = FactionCells[FMath::RandRange(0, FactionCells.Num() - 1)];
            OutFaction = Id;
            bFound = true;
        }
    });

    return bFound;
}

void UMythicEncounterDirector::SpawnEncounter(
    const FMythicEncounterTemplate& Template,
    const FMythicCellCoord& Cell,
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
    UMassEntitySubsystem* MassSubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
    if (MassSubsystem) {
        FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();

        // Build archetype: Identity + Schedule + Significance + NPC tag
        const UScriptStruct* Composition[] = {
            FMythicIdentityFragment::StaticStruct(),
            FMythicScheduleFragment::StaticStruct(),
            FMythicSignificanceFragment::StaticStruct(),
            FMythicNPCTag::StaticStruct()
        };
        FMassArchetypeHandle Archetype = EntityManager.CreateArchetype(MakeArrayView(Composition));

        // Batch-create entities
        TArray<FMassEntityHandle> SpawnedEntities;
        EntityManager.BatchCreateEntities(Archetype, Template.EntityCount, SpawnedEntities);

        for (int32 i = 0; i < SpawnedEntities.Num(); ++i) {
            FMassEntityHandle Entity = SpawnedEntities[i];

            // Identity — faction + encounter cell + unique hash per entity
            FMythicIdentityFragment& Identity = EntityManager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity);
            Identity.Faction = Faction;
            Identity.Cell = Cell;
            Identity.NameHash = GetTypeHash(Cell) ^ (NewEncounter.EncounterId * 2654435761u) ^ (i * 131071u);
            Identity.VisualArchetype = static_cast<uint8>(Identity.NameHash % 8);

            // Schedule — encounter NPCs have no home/work, they operate at the encounter cell
            FMythicScheduleFragment& Schedule = EntityManager.GetFragmentDataChecked<FMythicScheduleFragment>(Entity);
            Schedule.Phase = EMythicSchedulePhase::Idle;
            Schedule.HomeCell = Cell;
            Schedule.WorkCell = Cell;

            // Significance — start at Tier 0, significance system will promote if near player
            FMythicSignificanceFragment& Sig = EntityManager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity);
            Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
        }

        NewEncounter.SpawnedEntities = MoveTemp(SpawnedEntities);

        UE_LOG(LogMythEncounter, Log,
               TEXT("Encounter %d: spawned %d MASS entities at (%d,%d)"),
               NewEncounter.EncounterId, Template.EntityCount, Cell.X, Cell.Y);
    } else {
        UE_LOG(LogMythEncounter, Warning,
               TEXT("Encounter %d: MassEntitySubsystem not available — entities not spawned"),
               NewEncounter.EncounterId);
    }

    ActiveEncounters.Add(MoveTemp(NewEncounter));

    // Write event to Causal Fabric
    if (CausalFabric) {
        FMythicWorldEvent Event;
        Event.EventTag = Template.EncounterTag;
        Event.Cell = Cell;
        Event.PrimaryFaction = Faction;
        Event.Significance = 0.5f;
        Event.CategoryFlags = EMythicEventCategory::Social;

        CausalFabric->AppendEvent(Event);
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

    for (int32 i = ActiveEncounters.Num() - 1; i >= 0; --i) {
        FMythicActiveEncounter& Encounter = ActiveEncounters[i];

        // Check timeout
        if (Encounter.HasTimedOut(WorldTime)) {
            Encounter.State = EMythicEncounterState::Completed;
        }

        // Clean up completed encounters
        if (Encounter.State == EMythicEncounterState::Completed) {
            CleanupEncounter(i);
        }
    }
}

void UMythicEncounterDirector::CleanupEncounter(int32 Index) {
    if (!ActiveEncounters.IsValidIndex(Index)) {
        return;
    }

    FMythicActiveEncounter& Encounter = ActiveEncounters[Index];

    // Destroy spawned MASS entities
    if (Encounter.SpawnedEntities.Num() > 0) {
        UMassEntitySubsystem* MassSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr;
        if (MassSubsystem) {
            FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();
            int32 DestroyedCount = 0;
            for (const FMassEntityHandle& Entity : Encounter.SpawnedEntities) {
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

// ─────────────────────────────────────────────────────────────
// Cooldown Helpers
// ─────────────────────────────────────────────────────────────

bool UMythicEncounterDirector::IsOnCooldown(const FGameplayTag& TemplateTag, double WorldTime, float CooldownSeconds) const {
    const double* LastActivation = TemplateCooldowns.Find(TemplateTag);
    if (!LastActivation) {
        return false;
    }
    return (WorldTime - *LastActivation) < static_cast<double>(CooldownSeconds);
}

int32 UMythicEncounterDirector::CountActiveInstances(const FGameplayTag& TemplateTag) const {
    int32 Count = 0;
    for (const FMythicActiveEncounter& E : ActiveEncounters) {
        if (E.TemplateTag == TemplateTag && E.State == EMythicEncounterState::Active) {
            ++Count;
        }
    }
    return Count;
}
