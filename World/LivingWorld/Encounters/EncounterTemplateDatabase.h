
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "EncounterTemplateDatabase.generated.h"

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

    int32 GetTemplateCount() const { return Templates.Num(); }
};
