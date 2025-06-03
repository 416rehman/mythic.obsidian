#pragma once
#include "FamilyDefinition.h"
#include "GameplayTagContainer.h"
#include "NPCDefinition.h"
#include "MythicNPCData.generated.h"


// Struct containing the NPC Data that can be cached/tracked
USTRUCT(BlueprintType, Blueprintable)
struct FMythicNPCData {
    GENERATED_BODY()

    // NPC's Unique ID
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    FGuid NPCId = FGuid::NewGuid();

    // Name of the NPC
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    FString NPCName;

    // NPC Type
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data", meta = (Categories = "NPC.Type"))
    FGameplayTag NPCType;

    // Tags required to recruit
    UPROPERTY(EditAnywhere, Category = "NPC Data", meta = (Categories = "NPC.Recruitment"))
    FGameplayTagContainer TagsRequiredToRecruit;

    // Overrides for the NPC's Affiliations. Map of Faction: Value
    // These have a higher priority than the global AffiliationOverrides in the NPCTypesTable
    UPROPERTY(EditAnywhere, Category = "NPC Data", meta = (Categories = "NPC.Type"))
    TMap<FGameplayTag, float> AffiliationOverrides;

    // Overrides for the Fight or Flight values. Map of Faction: Value
    // These have a higher priority than the global FoF mappings in the NPCTypesTable
    // When this NPC encounters a Hostile NPC, this will be used to determine if this NPC will fight or flee
    UPROPERTY(EditAnywhere, Category = "NPC Data", meta = (Categories = "NPC.Type"))
    TMap<FGameplayTag, float> FlightOrFightOverrides;

    // The NPC's Family
    UPROPERTY(BlueprintReadOnly)
    FGuid NPCFamilyId = FGuid();

    void ClearAll() {
        NPCId.Invalidate();
        NPCName.Empty();
        NPCType = FGameplayTag();
        TagsRequiredToRecruit = FGameplayTagContainer();
        AffiliationOverrides.Empty();
        FlightOrFightOverrides.Empty();
        NPCFamilyId.Invalidate();
    }

    // Default Constructor
    FMythicNPCData() {
        ClearAll();
    }

    // Creates a new NPCData from an NPCDefinition
    FMythicNPCData(UNPCDefinition *NPCDef) {
        this->NPCId = NPCDef->NPCId;
        this->NPCName = NPCDef->Name;
        this->NPCType = NPCDef->NPCType;
        this->TagsRequiredToRecruit = NPCDef->TagsRequiredToRecruit;
        this->AffiliationOverrides = NPCDef->AffiliationOverrides;
        this->FlightOrFightOverrides = NPCDef->FlightOrFightOverrides;
        auto FamilyDef = NPCDef->FamilyDef.Get();
        if (FamilyDef) {
            this->NPCFamilyId = FamilyDef->FamilyId;
        }
    }

    //TODO: Creates a new random NPCData from an NPCTypeDefinition
};
