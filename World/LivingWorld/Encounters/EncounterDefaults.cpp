
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "World/LivingWorld/Encounters/MythicEncounterObjectiveDefaults.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "Objectives/ObjectiveDefinition.h"
#include "GAS/MythicTags_GAS.h"
#include "Rewards/LootReward.h"

namespace MythicEncounterDefaults {
void BuildDefaultTemplates(TArray<FMythicEncounterTemplate>& Out) {
    Out.Reserve(Out.Num() + 4);

    {
        FMythicEncounterTemplate T;
        T.EncounterTag = TAG_LIVINGWORLD_ENCOUNTER_PATROL;
        T.DisplayName = FText::FromString(TEXT("Faction Patrol"));
        T.MinFactionRelation = EMythicFactionRelation::Neutral;
        T.MinMilitaryStrength = 0.0f;
        T.MinPopulation = 0;
        T.CooldownSeconds = 180.0f;
        T.MaxConcurrentInstances = 2;
        T.BaseProbability = 0.25f;
        T.EntityCount = 3;
        T.MaxDurationSeconds = 600.0f;
        Out.Add(MoveTemp(T));
    }

    {
        FMythicEncounterTemplate T;
        T.EncounterTag = TAG_LIVINGWORLD_ENCOUNTER_BANDIT_AMBUSH;
        T.DisplayName = FText::FromString(TEXT("Bandit Ambush"));
        T.MinFactionRelation = EMythicFactionRelation::Neutral;
        T.MinMilitaryStrength = 0.0f;
        T.MinPopulation = 0;
        T.CooldownSeconds = 240.0f;
        T.MaxConcurrentInstances = 2;
        T.BaseProbability = 0.20f;
        T.EntityCount = 4;
        T.MaxDurationSeconds = 600.0f;
        Out.Add(MoveTemp(T));
    }

    {
        FMythicEncounterTemplate T;
        T.EncounterTag = TAG_LIVINGWORLD_ENCOUNTER_WILDLIFE;
        T.DisplayName = FText::FromString(TEXT("Wildlife"));
        T.MinFactionRelation = EMythicFactionRelation::Neutral;
        T.MinMilitaryStrength = 0.0f;
        T.MinPopulation = 0;
        T.CooldownSeconds = 150.0f;
        T.MaxConcurrentInstances = 2;
        T.BaseProbability = 0.30f;
        T.EntityCount = 3;
        T.MaxDurationSeconds = 600.0f;
        Out.Add(MoveTemp(T));
    }

    {
        FMythicEncounterTemplate T;
        T.EncounterTag = TAG_LIVINGWORLD_ENCOUNTER_RAID;
        T.DisplayName = FText::FromString(TEXT("Faction Raid"));
        T.MinFactionRelation = EMythicFactionRelation::Hostile;
        T.MinMilitaryStrength = 0.0f;
        T.MinPopulation = 0;
        T.CooldownSeconds = 360.0f;
        T.MaxConcurrentInstances = 2;
        T.BaseProbability = 0.15f;
        T.EntityCount = 6;
        T.MaxDurationSeconds = 600.0f;
        Out.Add(MoveTemp(T));
    }
}

int32 DangerScaledEntityCount(int32 BaseCount, EMythicDangerTier Tier, int32 MaxEntityCount) {
    const int32 DangerBonus = static_cast<int32>(Tier);
    return FMath::Clamp(BaseCount + DangerBonus, 1, FMath::Max(1, MaxEntityCount));
}
}

namespace MythicEncounterObjectiveDefaults {
UObjectiveDefinition *BuildDefaultEncounterClearObjective(UObject *Outer, int32 RequiredKills) {
    if (!Outer) {
        return nullptr;
    }

    UObjectiveDefinition *Def = NewObject<UObjectiveDefinition>(Outer, NAME_None, RF_Transient);
    if (!Def) {
        return nullptr;
    }

    Def->TriggerEventTag = GAS_EVENT_KILL;
    Def->RequiredCount = FMath::Max(1, RequiredKills);
    Def->bCountByEventMagnitude = false;
    Def->DisplayText = FText::FromString(TEXT("Clear the encounter"));
    Def->CompletedText = FText::FromString(TEXT("Encounter cleared!"));

    Def->Rewards.LootReward = NewObject<ULootReward>(Def);

    return Def;
}
}
