// Mythic Living World — Embodied Creature Actor Implementation

#include "AI/Creatures/MythicCreatureCharacter.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Engine/World.h"

AMythicCreatureCharacter::AMythicCreatureCharacter() {
    // Inherit AI possession (AMythicNPCAIController) + GAS/life wiring from AMythicNPCCharacter's ctor. Creatures are
    // wildlife, not townsfolk — turn OFF per-frame Tick by default (the bare creature has no humanoid tick needs; a
    // mesh-bearing BP subclass can re-enable it if its anim needs ticking). Default-off matches the project's
    // bCanEverTick=false rule for new actors.
    PrimaryActorTick.bCanEverTick = false;

    // Creatures are never recruitable and never merchants — leave bRecruitable / MerchantOffers at their inherited
    // defaults (false / empty). No further ctor wiring needed: the base already set AIControllerClass + AutoPossessAI.
}

void AMythicCreatureCharacter::InitializeFromMassEntity(const FMassEntityHandle &InEntityHandle) {
    // Server-authoritative, exactly like the base humanoid path.
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

    // A creature carries FMythicCreatureFragment + FMythicIdentityFragment but NOT Personality/Schedule, so we
    // deliberately do NOT call the base (humanoid) brain init. Read the creature data and cache it on the actor.
    const FMythicCreatureFragment *CreatureFrag = EntityManager.GetFragmentDataPtr<FMythicCreatureFragment>(InEntityHandle);
    if (CreatureFrag) {
        SpeciesId = CreatureFrag->SpeciesId;
        PackId = static_cast<int32>(CreatureFrag->PackId);
        CurrentAggression = CreatureFrag->CurrentAggression;
    }

    // Hand the resolved species to the mesh-bearing Blueprint so it can swap mesh/anim per species (authored data, not
    // C++ asset coupling). Safe no-op when no BP binds it (bare C++ creature stays invisible-but-real + AI-possessed).
    OnCreatureInitialized(SpeciesId, PackId);
}
