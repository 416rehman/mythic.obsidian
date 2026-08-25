
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "NPCDefinition.generated.h"

class UFamilyDefinition;
class AMythicNPCCharacter;
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

    // The actor class the manager spawns for this definition. The Blueprint class carries the mesh,
    // attack ability and default effects; the raw C++ base has none of them, so an unset class means an
    // invisible, unarmed pawn.
    UPROPERTY(EditAnywhere, Category = "Basic")
    TSubclassOf<AMythicNPCCharacter> NPCClass;

    // The faction whose brain, relations and standing this NPC carries. Unset = no cognitive faction:
    // the NPC is Neutral to the world and invisible to the diplomacy, witness and standing systems.
    UPROPERTY(EditAnywhere, Category = "Relations", meta = (Categories = "Faction"))
    FGameplayTag Faction;

    // Proficiencies - What the NPC is good/bad at. Goes on the NPC's ASC
    UPROPERTY(EditAnywhere, Category = "Stats")
    TArray<FRolledAttributeSpec> Proficiencies;

    // Traits - Influences the NPCs behavior. Goes on the NPC's ASC
    UPROPERTY(EditAnywhere, Category = "Personality", meta=(Categories = "NPC.Trait"))
    FGameplayTagContainer Traits;

    // Sight perception, per NPC type (a hawk sees farther than a boar). Defaults match the previous hardcoded
    // AIController values, so an un-tuned NPC behaves exactly as before. The AIController applies these on possess.
    UPROPERTY(EditAnywhere, Category = "Perception", meta = (ClampMin = "0.0"))
    float SightRadius = 1500.0f;

    UPROPERTY(EditAnywhere, Category = "Perception", meta = (ClampMin = "0.0"))
    float LoseSightRadius = 2000.0f;

    UPROPERTY(EditAnywhere, Category = "Perception", meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float PeripheralVisionAngleDegrees = 90.0f;

    // Per-faction stance delta on the -100..+100 standing scale, added on top of the live diplomacy
    // relation before banding. Authored data biases the living world; it never replaces it.
    UPROPERTY(EditAnywhere, Category = "Relations", meta = (Categories = "Faction"))
    TMap<FGameplayTag, float> AffiliationOverrides;

    // Family Definition - Should not be set manually.
    // Instead, create or open a FamilyDefinition asset, and add this NPC as a member to it.
    UPROPERTY(VisibleAnywhere, Category = "Relations")
    TSoftObjectPtr<const UFamilyDefinition> FamilyDef;

    // Per-faction probability [0,1] of standing to fight when threatened by that faction. Collapsed into
    // the brain's Fight/Flee vent weights at spawn until a per-faction threat consumer exists.
    UPROPERTY(EditAnywhere, Category = "Relations", meta = (Categories = "Faction", ClampMin = "0.0", ClampMax = "1.0"))
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
