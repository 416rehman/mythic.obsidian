// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "NPCDefinition.generated.h"

class UFamilyDefinition;
/**
 * NPC Definition - This is used to pre-define a SPECIFIC non-generic/random NPC. I.e., The King of the North, because he should always stay the same.
 * As opposed to randomly generating an NPC at runtime, this is a preset of a SPECIFIC NPC.
 */
UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API UNPCDefinition : public UDataAsset {
    GENERATED_BODY()

public:
    // When the NPC is spawned, this ID is used to track them throughout the game's systems
    UPROPERTY(BlueprintReadOnly, Category = "Basic")
    FGuid NPCId = FGuid::NewGuid();

    // The name of the NPC
    UPROPERTY(EditAnywhere, Category = "Basic")
    FString Name;

    // NPC Type
    UPROPERTY(EditAnywhere, Category = "Basic", meta=(Categories = "NPC.Type"))
    FGameplayTag NPCType;

    // Proficiencies - What the NPC is good/bad at. Goes on the NPC's ASC
    UPROPERTY(EditAnywhere, Category = "Stats")
    TArray<FRolledAttributeSpec> Proficiencies;

    // Traits - Influences the NPCs behavior. Goes on the NPC's ASC
    UPROPERTY(EditAnywhere, Category = "Personality", meta=(Categories = "NPC.Trait"))
    FGameplayTagContainer Traits;

    // Affiliation Overrides - Instead of using the global "per NPCType" affiliations from the NPCTypesTable
    UPROPERTY(EditAnywhere, Category = "Relations")
    TMap<FGameplayTag, float> AffiliationOverrides;

    // Family Definition - Should not be set manually.
    // Instead, create or open a FamilyDefinition asset, and add this NPC as a member to it.
    UPROPERTY(VisibleAnywhere, Category = "Relations")
    TSoftObjectPtr<const UFamilyDefinition> FamilyDef;

    // Fight or Flight Overrides - Instead of using the global "per NPCType" FoF mappings from the NPCTypesTable
    UPROPERTY(EditAnywhere, Category = "Relations")
    TMap<FGameplayTag, float> FlightOrFightOverrides;

    // Tags the player must have if they want to recruit this NPC.
    // Note: The player's affiliation with the NPC must also be positive (70%>) for them to be able to recruit them.
    UPROPERTY(EditAnywhere, Category = "Recruitment")
    FGameplayTagContainer TagsRequiredToRecruit;

    // Visual overrides
    UPROPERTY(EditAnywhere, Category = "Visuals")
    bool bUseFixedVisuals = false;

    // The mesh that will be used for the NPC
    UPROPERTY(EditAnywhere, Category = "Visuals", meta = (EditCondition = "bUseFixedVisuals"))
    TSoftObjectPtr<USkeletalMesh> FixedMesh;

    // The materials that will be used for the NPC's clothing
    UPROPERTY(EditAnywhere, Category = "Visuals", meta = (EditCondition = "bUseFixedVisuals"))
    TArray<TSoftObjectPtr<UMaterialInterface>> FixedMaterials;

    UPROPERTY(EditAnywhere, Category = "Visuals", meta = (EditCondition = "bUseFixedVisuals"))
    TArray<TSoftObjectPtr<USkeletalMesh>> FixedClothing;
};
