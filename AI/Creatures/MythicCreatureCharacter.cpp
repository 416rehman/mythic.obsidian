
#include "AI/Creatures/MythicCreatureCharacter.h"
#include "Settings/MythicCombatSettings.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Engine/World.h"

AMythicCreatureCharacter::AMythicCreatureCharacter() {
    PrimaryActorTick.bCanEverTick = false;
}

void AMythicCreatureCharacter::InitializeFromMassEntity(const FMassEntityHandle &InEntityHandle) {
    if (!HasAuthority()) {
        return;
    }

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    UMassEntitySubsystem *EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(World);
    if (!EntitySubsystem || !EntitySubsystem->GetEntityManager().IsEntityValid(InEntityHandle)) {
        return;
    }

    const FMassEntityManager &EntityManager = EntitySubsystem->GetEntityManager();

    const FMythicCreatureFragment *CreatureFrag = EntityManager.GetFragmentDataPtr<FMythicCreatureFragment>(InEntityHandle);
    if (CreatureFrag) {
        SpeciesId = CreatureFrag->SpeciesId;
        PackId = static_cast<int32>(CreatureFrag->PackId);
        CurrentAggression = CreatureFrag->CurrentAggression;
    }

    OnCreatureInitialized(SpeciesId, PackId);

    // Creatures embody through Mass like NPCs do, and their fights ride the same level ladder.
    StampCombatLevel(MythicCombat::ResolveCombatLevelAt(World, GetActorLocation()));
}
