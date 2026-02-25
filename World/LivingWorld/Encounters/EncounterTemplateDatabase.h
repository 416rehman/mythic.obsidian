// Mythic Living World — Encounter Template Database
// Designer-authored data asset containing encounter template definitions.
// Referenced by LivingWorldSettings, auto-loaded by EncounterDirector.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "EncounterTemplateDatabase.generated.h"

/**
 * Data asset containing all encounter template definitions.
 * Designers create and edit templates directly in the editor.
 *
 * Setup:
 * 1. Content Browser → Right-click → Miscellaneous → Data Asset → MythicEncounterTemplateDatabase
 * 2. Add entries to the Templates array
 * 3. Reference this asset in DA_LivingWorldSettings → Encounter Template Database
 *
 * The Encounter Director automatically loads all templates from this asset on initialization.
 * Additional templates can still be registered at runtime via UMythicEncounterDirector::RegisterTemplate().
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicEncounterTemplateDatabase : public UDataAsset {
    GENERATED_BODY()

public:
    /**
     * All encounter templates available in this world.
     * Each entry defines the prerequisites, probability, entity count, and timing for one encounter type.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Encounter Templates",
              meta = (TitleProperty = "DisplayName"))
    TArray<FMythicEncounterTemplate> Templates;

    /** Get the number of templates in this database */
    int32 GetTemplateCount() const { return Templates.Num(); }
};
