
#include "AI/Creatures/MythicCreatureCharacter.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "World/Entity/MythicEntityPresentationTags.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "World/Entity/MythicEntityIdentityDefinition.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Creatures/CreatureSpeciesTypes.h"

namespace {
bool ResolveSpeciesRow(const UWorld *World, const uint8 SpeciesId,
                       FMythicCreatureSpeciesRow &OutRow) {
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    const UMythicLivingWorldSubsystem *LivingWorld = GameInstance
        ? GameInstance->GetSubsystem<UMythicLivingWorldSubsystem>()
        : nullptr;
    const UMythicLivingWorldSettings *Settings =
        LivingWorld ? LivingWorld->GetSettings() : nullptr;
    if (Settings && !Settings->CreatureSpeciesTable.IsNull()) {
        if (const UDataTable *Table = Settings->CreatureSpeciesTable.LoadSynchronous()) {
            for (const TPair<FName, uint8 *> &Pair : Table->GetRowMap()) {
                const FMythicCreatureSpeciesRow *Row =
                    reinterpret_cast<const FMythicCreatureSpeciesRow *>(Pair.Value);
                if (Row && Row->SpeciesId == SpeciesId) {
                    OutRow = *Row;
                    return true;
                }
            }
        }
    }
    for (const FMythicCreatureSpeciesRow &Row :
         MythicCreatureDefaults::GetCodeDefaultSpecies()) {
        if (Row.SpeciesId == SpeciesId) {
            OutRow = Row;
            return true;
        }
    }
    OutRow = FMythicCreatureSpeciesRow();
    return false;
}
}

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
        FMythicCreatureSpeciesRow Species;
        if (ResolveSpeciesRow(World, SpeciesId, Species)) {
            SpeciesTag = Species.SpeciesTag;
            PublicIdentityDefinitionId =
                UMythicEntityIdentityDefinition::ResolvePrimaryAssetId(
                    Species.PublicIdentityDefinition);
        }
    }

    // Common identity, brain, appearance, status, and combat initialization must execute exactly once for creatures.
    Super::InitializeFromMassEntity(InEntityHandle);

    OnCreatureInitialized(SpeciesId, PackId);
}

void AMythicCreatureCharacter::OnReturnedToPool() {
    SpeciesId = 0;
    SpeciesTag = FGameplayTag();
    PublicIdentityDefinitionId = FPrimaryAssetId();
    PackId = 0;
    CurrentAggression = 0.0f;
    Super::OnReturnedToPool();
}

void AMythicCreatureCharacter::BuildMassPublicIdentity(
    const FMythicIdentityFragment & /*Identity*/,
    FMythicPublicIdentitySnapshot &OutIdentity) const {
    OutIdentity.Reset();
    OutIdentity.PublicKindTag =
        MythicEntityPresentationTags::EntityKindCreature;
    OutIdentity.PublicIdentityDefinitionId = PublicIdentityDefinitionId;
}

void AMythicCreatureCharacter::BuildDirectPublicIdentity(
    FMythicPublicIdentitySnapshot &OutIdentity) const {
    OutIdentity.Reset();
    OutIdentity.PublicKindTag =
        MythicEntityPresentationTags::EntityKindCreature;
    OutIdentity.PublicIdentityDefinitionId = PublicIdentityDefinitionId;
}
