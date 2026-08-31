
#include "World/LivingWorld/Encounters/EncounterDirector.h"
#include "World/LivingWorld/Encounters/EncounterTemplateDatabase.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "Mass/Tags/MythicMassTags.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "World/LivingWorld/Encounters/MythicEncounterObjectiveDefaults.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "GAS/Progression/MythicRenownComponent.h"
#include "World/LivingWorld/Vendetta/MythicVendettaSubsystem.h"
#include "World/LivingWorld/Encounters/MythicReputationEncounterMath.h"
#include "Settings/MythicDeveloperSettings.h"
#include "World/Trading/MythicTradeLedgerSubsystem.h"
#include "World/GameDirector/MythicPacingDirectorSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogMythEncounter);


bool UMythicEncounterDirector::ShouldCreateSubsystem(UObject *Outer) const {
    if (const UWorld *World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicEncounterDirector::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    if (UGameInstance *GI = GetWorld()->GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            LivingWorld = LW;
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

    MaxActiveEncounters = Settings->MaxActiveEncounters;
    EvaluationInterval = Settings->EncounterEvaluationInterval;

    GetWorld()->GetTimerManager().SetTimer(
        EvaluationTimerHandle,
        this,
        &UMythicEncounterDirector::EvaluationTick,
        EvaluationInterval,
true,
EvaluationInterval);

    ActiveEncounters.Reserve(MaxActiveEncounters);

    if (!Settings->EncounterTemplateDatabase.IsNull()) {
        if (UMythicEncounterTemplateDatabase *DB = Settings->EncounterTemplateDatabase.LoadSynchronous()) {
            for (const FMythicEncounterTemplate &Template : DB->Templates) {
                RegisterTemplate(Template);
            }
            UE_LOG(LogMythEncounter, Log,
                   TEXT("EncounterDirector: Loaded %d templates from data asset"), DB->GetTemplateCount());
        }
    }

    if (Templates.Num() == 0) {
        TArray<FMythicEncounterTemplate> DefaultTemplates;
        MythicEncounterDefaults::BuildDefaultTemplates(DefaultTemplates);
        for (const FMythicEncounterTemplate &Template : DefaultTemplates) {
            RegisterTemplate(Template);
        }
        UE_LOG(LogMythEncounter, Log,
               TEXT("EncounterDirector: no authored templates — registered %d CODE-DEFAULT templates (assign an "
                    "EncounterTemplateDatabase in LivingWorldSettings to override)."),
               DefaultTemplates.Num());
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


void UMythicEncounterDirector::RegisterTemplate(const FMythicEncounterTemplate &Template) {
    if (!Template.EncounterTag.IsValid()) {
        UE_LOG(LogMythEncounter, Warning, TEXT("Attempted to register template with invalid tag"));
        return;
    }

    Templates.Add(Template);
    UE_LOG(LogMythEncounter, Log, TEXT("Registered encounter template: %s"), *Template.EncounterTag.ToString());
}


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
            EmitEncounterCompletedEvent(ActiveEncounters[i], false);
            CleanupEncounter(i);
            return true;
        }
    }
    return false;
}

void UMythicEncounterDirector::ResetForLivingWorldRestore() {
    check(IsInGameThread());
    ActiveEncounters.Reset();
    TemplateCooldowns.Reset();
    NextEncounterId = 1;
}

UObjectiveDefinition *UMythicEncounterDirector::GetEncounterClearObjective() const {
    if (!Settings) {
        return nullptr;
    }

    if (!Settings->EncounterClearObjective.IsNull()) {
        if (UObjectiveDefinition *Authored = Settings->EncounterClearObjective.LoadSynchronous()) {
            return Authored;
        }
    }

    if (!DefaultClearObjective) {
        constexpr int32 DefaultClearKills = 3;
        UMythicEncounterDirector *MutableThis = const_cast<UMythicEncounterDirector *>(this);
        MutableThis->DefaultClearObjective =
            MythicEncounterObjectiveDefaults::BuildDefaultEncounterClearObjective(MutableThis, DefaultClearKills);
    }
    return DefaultClearObjective;
}


void UMythicEncounterDirector::EvaluationTick() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicEncounterDirector_Eval);

    if (!CausalFabric || !FactionDB || !TerritoryGrid) {
        return;
    }

    const double WorldTime = GetWorld()->GetTimeSeconds();

    UpdateActiveEncounters();

    MaybeOfferClearObjectives();

    if (ActiveEncounters.Num() >= MaxActiveEncounters) {
        return;
    }

    for (const FMythicEncounterTemplate &Template : Templates) {
        if (ActiveEncounters.Num() >= MaxActiveEncounters) {
            break;
        }

        if (IsOnCooldown(Template.EncounterTag, WorldTime, Template.CooldownSeconds)) {
            continue;
        }

        if (CountActiveInstances(Template.EncounterTag) >= Template.MaxConcurrentInstances) {
            continue;
        }

        FMythicCellCoord SpawnCell;
        FMythicFactionId SpawnFaction;
        float SpawnProbability = Template.BaseProbability;
        if (!EvaluateTemplate(Template, SpawnCell, SpawnFaction, SpawnProbability)) {
            continue;
        }

        if (HasEncounterInCell(SpawnCell)) {
            continue;
        }

        if (!MythicCombat::RollSucceeds(SpawnProbability, FMath::FRand())) {
            continue;
        }

        SpawnEncounter(Template, SpawnCell, SpawnFaction);
        TemplateCooldowns.FindOrAdd(Template.EncounterTag) = WorldTime;
    }
}

bool UMythicEncounterDirector::EvaluateTemplate(
    const FMythicEncounterTemplate &Template,
    FMythicCellCoord &OutCell,
    FMythicFactionId &OutFaction,
    float &OutSpawnProbability) const {
    OutSpawnProbability = Template.BaseProbability;
    if (!Template.RequiredWorldState.IsEmpty()) {
        FGameplayTagContainer WorldState;
        if (const UWorld *World = GetWorld()) {
            if (const UGameInstance *GI = World->GetGameInstance()) {
                if (const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
                    if (Env->GetEnvironmentController() != nullptr) {
                        const FGameplayTag Weather = Env->GetWeather();
                        if (Weather.IsValid()) {
                            WorldState.AddTag(Weather);
                        }
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
                        static TSet<FGameplayTag> WarnedTemplates;
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

    struct FCandidateFaction {
        FMythicFactionId Id;
        float Military;
        int32 Population;
    };
    TArray<FCandidateFaction> Candidates;
    FactionDB->ForEachAliveFaction([&](FMythicFactionId Id, const FMythicFactionData &Data) {
        Candidates.Add({Id, Data.MilitaryStrength, Data.Population});
    });

    const bool bRelationGated =
        static_cast<uint8>(Template.MinFactionRelation) > static_cast<uint8>(EMythicFactionRelation::Neutral);

    TArray<FMythicFactionId> QualifyingFactions;
    for (const FCandidateFaction &C : Candidates) {
        if (C.Military < Template.MinMilitaryStrength) {
            continue;
        }
        if (C.Population < Template.MinPopulation) {
            continue;
        }
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

        TArray<FMythicCellCoord> Probe;
        TerritoryGrid->GetFactionCells(C.Id, 1, Probe);
        if (Probe.Num() > 0) {
            QualifyingFactions.Add(C.Id);
        }
    }

    if (QualifyingFactions.Num() == 0) {
        return false;
    }

    const FMythicFactionId ChosenFaction = QualifyingFactions[FMath::RandRange(0, QualifyingFactions.Num() - 1)];
    TArray<FMythicCellCoord> ChosenCells;
    TerritoryGrid->GetFactionCells(ChosenFaction, 32, ChosenCells);
    if (ChosenCells.Num() == 0) {
        return false;
    }
    OutCell = ChosenCells[FMath::RandRange(0, ChosenCells.Num() - 1)];
    OutFaction = ChosenFaction;

    if (Template.RequiredReputationBand.IsValid()) {
        const UMythicDeveloperSettings *DevSettings = GetDefault<UMythicDeveloperSettings>();
        if (DevSettings && DevSettings->bReputationEncountersEnabled) {
            const FMythicPartyReputation PartyRep = ComputePartyReputation(OutCell);
            const float Scaled = FMythicReputationEncounterMath::ScaleWeight(
                Template.BaseProbability, Template.RequiredReputationBand, PartyRep, Template.ReputationWeightScale);
            if (Scaled <= 0.0f) {
                return false;
            }
            OutSpawnProbability = FMath::Clamp(Scaled, 0.0f, 1.0f);
        }
    }

    if (Template.bCargoHeatAmbush && TerritoryGrid) {
        if (const UWorld *World = GetWorld()) {
            if (const UMythicTradeLedgerSubsystem *Ledger = World->GetSubsystem<UMythicTradeLedgerSubsystem>()) {
                const float Heat = Ledger->GetMaxCargoHeatAt(TerritoryGrid->CellToWorld(OutCell));
                if (Heat > 0.0f) {
                    OutSpawnProbability = FMath::Clamp(OutSpawnProbability * (1.0f + Heat), 0.0f, 1.0f);
                }
            }
        }
    }

    if (const UWorld *World = GetWorld()) {
        if (const UMythicPacingDirectorSubsystem *Pacing = World->GetSubsystem<UMythicPacingDirectorSubsystem>()) {
            const float Intensity = Pacing->GetSpawnIntensityMultiplier();
            if (Intensity > 0.0f && !FMath::IsNearlyEqual(Intensity, 1.0f)) {
                OutSpawnProbability = FMath::Clamp(OutSpawnProbability * Intensity, 0.0f, 1.0f);
            }
        }
    }
    return true;
}

FMythicPartyReputation UMythicEncounterDirector::ComputePartyReputation(const FMythicCellCoord &Cell) const {
    (void)Cell;

    FMythicPartyReputation Rep;
    const UWorld *World = GetWorld();
    if (!World) {
        return Rep;
    }

    const UMythicVendettaSubsystem *Vendetta = World->GetSubsystem<UMythicVendettaSubsystem>();

    int32 TierSum = 0;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        const AMythicPlayerController *PC = Cast<AMythicPlayerController>(It->Get());
        if (!PC || !PC->HasAuthority()) {
            continue;
        }
        const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>();
        if (!PS) {
            continue;
        }
        const UMythicRenownComponent *Renown = PS->GetRenownComponent();
        if (!Renown) {
            continue;
        }
        const int32 TierIndex = static_cast<int32>(Renown->GetGlobalTier());

        Rep.MaxTier = FMath::Max(Rep.MaxTier, TierIndex);
        TierSum += TierIndex;
        ++Rep.NumPlayers;

        if (Vendetta) {
            Rep.VendettaHeat = FMath::Max(Rep.VendettaHeat, Vendetta->GetMaxThreatForPlayer(PS->GetCanonicalPlayerKey()));
        }
    }

    if (Rep.NumPlayers > 0) {
        Rep.AvgTier = static_cast<float>(TierSum) / static_cast<float>(Rep.NumPlayers);
    }
    return Rep;
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

    const EMythicDangerTier CellDanger =
        TerritoryGrid ? TerritoryGrid->GetCellDangerTier(Cell) : EMythicDangerTier::Safe;
    NewEncounter.EntityCount = MythicEncounterDefaults::DangerScaledEntityCount(Template.EntityCount, CellDanger);

    if (Template.RequiredReputationBand.IsValid()) {
        const UMythicDeveloperSettings *DevSettings = GetDefault<UMythicDeveloperSettings>();
        if (DevSettings && DevSettings->bReputationEncountersEnabled) {
            const FMythicPartyReputation PartyRep = ComputePartyReputation(Cell);
            NewEncounter.EntityCount = FMythicReputationEncounterMath::ScalePackSize(
                NewEncounter.EntityCount, Template.RequiredReputationBand, PartyRep, Template.ReputationWeightScale,
20);
        }
    }

    UMassEntitySubsystem *MassSubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
    UMythicPersistentNPCRegistry *Registry =
        LivingWorld ? LivingWorld->GetPersistentNPCRegistry() : nullptr;
    if (MassSubsystem && Registry) {
        FMassEntityManager &EntityManager = MassSubsystem->GetMutableEntityManager();

        const UScriptStruct *Composition[] = {
            FMythicIdentityFragment::StaticStruct(),
            FMythicScheduleFragment::StaticStruct(),
            FMythicSignificanceFragment::StaticStruct(),
            FMythicNPCTag::StaticStruct(),
            FMythicEncounterEntityTag::StaticStruct()
        };
        FMassArchetypeHandle Archetype = EntityManager.CreateArchetype(MakeArrayView(Composition));

        TArray<FMassEntityHandle> SpawnedEntities;
        EntityManager.BatchCreateEntities(Archetype, NewEncounter.EntityCount, SpawnedEntities);

        TArray<FMassEntityHandle> ValidSpawnedEntities;
        ValidSpawnedEntities.Reserve(SpawnedEntities.Num());

        for (int32 i = 0; i < SpawnedEntities.Num(); ++i) {
            FMassEntityHandle Entity = SpawnedEntities[i];

            FMythicIdentityFragment &Identity = EntityManager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity);
            Identity.Faction = Faction;
            Identity.Cell = Cell;
            Identity.NameSeed = FMythicNPCGenerator::GenerateNameHash(
                Faction.Index, Cell, Registry->AllocateNameSeedSerial());
            Identity.EntityId = Registry->AllocateEntityIdentity(
                Identity.NameSeed,
                EMythicEntityIdentityProvenance::Encounter);
            if (!Identity.EntityId.IsValid()) {
                EntityManager.DestroyEntity(Entity);
                continue;
            }
            Identity.VisualArchetype = static_cast<uint8>(Identity.NameSeed % 8);

            FMythicScheduleFragment &Schedule = EntityManager.GetFragmentDataChecked<FMythicScheduleFragment>(Entity);
            Schedule.Phase = EMythicSchedulePhase::Idle;
            Schedule.HomeCell = Cell;
            Schedule.WorkCell = Cell;

            FMythicSignificanceFragment &Sig = EntityManager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity);
            Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
            ValidSpawnedEntities.Add(Entity);
        }

        NewEncounter.SpawnedEntities = MoveTemp(ValidSpawnedEntities);
        NewEncounter.EntityCount = NewEncounter.SpawnedEntities.Num();

        UE_LOG(LogMythEncounter, Log,
               TEXT("Encounter %d: spawned %d MASS entities at (%d,%d)"),
               NewEncounter.EncounterId, NewEncounter.EntityCount, Cell.X, Cell.Y);
    }
    else {
        UE_LOG(LogMythEncounter, Warning,
               TEXT("Encounter %d: MassEntitySubsystem not available — entities not spawned"),
               NewEncounter.EncounterId);
    }

    ActiveEncounters.Add(MoveTemp(NewEncounter));

    if (LivingWorld) {
        FMythicWorldEvent Event;
        Event.EventTag = Template.EncounterTag;
        Event.Cell = Cell;
        Event.PrimaryFaction = Faction;
        Event.Significance = 0.5f;
        Event.CategoryFlags = EMythicEventCategory::Encounter;

        LivingWorld->SubmitWorldEvent(Event);
    }

    UE_LOG(LogMythEncounter, Log,
           TEXT("Encounter %d activated: %s at (%d,%d) (Faction %d, %d entities)"),
           ActiveEncounters.Last().EncounterId,
           *Template.EncounterTag.ToString(),
           Cell.X, Cell.Y,
           Faction.Index,
           NewEncounter.EntityCount);
}

void UMythicEncounterDirector::UpdateActiveEncounters() {
    const double WorldTime = GetWorld()->GetTimeSeconds();

    UMassEntitySubsystem *MassSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr;
    const UMythicPersistentNPCRegistry *Registry = LivingWorld ? LivingWorld->GetPersistentNPCRegistry() : nullptr;

    for (int32 i = ActiveEncounters.Num() - 1; i >= 0; --i) {
        FMythicActiveEncounter &Encounter = ActiveEncounters[i];
        bool bDefeated = false;

        if (Encounter.State == EMythicEncounterState::Active && MassSubsystem && Registry && Encounter.SpawnedEntities.Num() > 0) {
            const FMassEntityManager &EntityManager = MassSubsystem->GetEntityManager();
            bool bAllDown = true;
            for (const FMassEntityHandle &Entity : Encounter.SpawnedEntities) {
                if (!EntityManager.IsEntityValid(Entity)) {
                    continue;
                }
                const FMythicIdentityFragment *Id = EntityManager.GetFragmentDataPtr<FMythicIdentityFragment>(Entity);
                if (!Id || !Registry->IsPermaDead(Id->EntityId)) {
                    bAllDown = false;
                    break;
                }
            }
            if (bAllDown) {
                Encounter.State = EMythicEncounterState::Completed;
                bDefeated = true;
            }
        }

        if (Encounter.HasTimedOut(WorldTime)) {
            Encounter.State = EMythicEncounterState::Completed;
        }

        if (Encounter.State == EMythicEncounterState::Completed) {
            EmitEncounterCompletedEvent(Encounter, bDefeated);
            CleanupEncounter(i);
        }
    }
}

void UMythicEncounterDirector::MaybeOfferClearObjectives() {
    if (ActiveEncounters.Num() == 0) {
        return;
    }
    UObjectiveDefinition *ClearObjective = GetEncounterClearObjective();
    if (!ClearObjective) {
        return;
    }
    const UWorld *World = GetWorld();
    if (!World || !TerritoryGrid) {
        return;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        AMythicPlayerController *PC = Cast<AMythicPlayerController>(It->Get());
        if (!PC || !PC->HasAuthority()) {
            continue;
        }
        const APawn *Pawn = PC->GetPawn();
        if (!Pawn) {
            continue;
        }
        const FMythicCellCoord PawnCell = TerritoryGrid->WorldToCell(Pawn->GetActorLocation());
        if (!HasEncounterInCell(PawnCell)) {
            continue;
        }
        if (UObjectiveTracker *Tracker = PC->GetObjectiveTracker()) {
            Tracker->ServerAddObjective(ClearObjective);
        }
    }
}

void UMythicEncounterDirector::CleanupEncounter(int32 Index) {
    if (!ActiveEncounters.IsValidIndex(Index)) {
        return;
    }

    FMythicActiveEncounter &Encounter = ActiveEncounters[Index];

    if (Encounter.SpawnedEntities.Num() > 0) {
        UWorld *World = GetWorld();
        UMassEntitySubsystem *MassSubsystem = (World && !World->bIsTearingDown) ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
        if (MassSubsystem) {
            FMassEntityManager &EntityManager = MassSubsystem->GetMutableEntityManager();
            UMythicPersistentNPCRegistry *IdentityRegistry =
                LivingWorld ? LivingWorld->GetPersistentNPCRegistry() : nullptr;
            int32 DestroyedCount = 0;
            int32 RetainedCount = 0;
            TSharedPtr<FMassCommandBuffer> RetainedCommands =
                MakeShared<FMassCommandBuffer>();
            for (const FMassEntityHandle &Entity : Encounter.SpawnedEntities) {
                if (EntityManager.IsEntityValid(Entity)) {
                    EMythicEntityRetirementResult Retirement =
                        EMythicEntityRetirementResult::RejectedInvalid;
                    if (LivingWorld && IdentityRegistry) {
                        if (const FMythicIdentityFragment *Identity =
                                EntityManager.GetFragmentDataPtr<FMythicIdentityFragment>(Entity)) {
                            Retirement = LivingWorld->TryRetireEntityIdentity(
                                Identity->EntityId);
                        }
                    }

                    if (Retirement
                        == EMythicEntityRetirementResult::RetainedByDurableReference) {
                        if (AMythicNPCCharacter *Actor =
                                LivingWorld->FindEmbodiedActor(Entity)) {
                            LivingWorld->ReleaseEmbodiedActor(Entity, Actor);
                        }
                        RetainedCommands->RemoveTag<FMythicEncounterEntityTag>(Entity);
                        ++RetainedCount;
                        continue;
                    }

                    if (LivingWorld) {
                        if (AMythicNPCCharacter *Actor =
                                LivingWorld->FindEmbodiedActor(Entity)) {
                            Actor->Destroy();
                        }
                        LivingWorld->UnregisterEmbodiedActor(Entity);
                    }
                    EntityManager.DestroyEntity(Entity);
                    ++DestroyedCount;
                }
            }
            if (RetainedCount > 0) {
                EntityManager.FlushCommands(RetainedCommands);
            }
            UE_LOG(LogMythEncounter, Log,
                   TEXT("Encounter %d: destroyed %d and retained %d/%d MASS entities"),
                   Encounter.EncounterId, DestroyedCount, RetainedCount,
                   Encounter.SpawnedEntities.Num());
        }
    }

    UE_LOG(LogMythEncounter, Log,
           TEXT("Encounter %d completed/cleaned up: %s"),
           Encounter.EncounterId,
           *Encounter.TemplateTag.ToString());

    ActiveEncounters.RemoveAtSwap(Index);
}

void UMythicEncounterDirector::EmitEncounterCompletedEvent(const FMythicActiveEncounter &Encounter, bool bDefeated) const {
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
