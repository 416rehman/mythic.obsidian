// 

#pragma once

#include "CoreMinimal.h"
#include "FamilyDefinition.h"
#include "MythicAIController.h"
#include "MythicNPCData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicNPCManager.generated.h"

class AMythicNPCCharacter;
// Tracked NPC Struct
USTRUCT(BlueprintType, Blueprintable)
struct FMythicCachedNPCData {
    GENERATED_BODY()

    // The Data of the NPC
    UPROPERTY(BlueprintReadOnly)
    FMythicNPCData NPCData;

    // Last Seen Loc maybe?

    FMythicCachedNPCData() {
        NPCData = FMythicNPCData();
    }

    FMythicCachedNPCData(FMythicNPCData InNPCData) {
        NPCData = InNPCData;
    }
};

/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicNPCManager : public UGameInstanceSubsystem {
    GENERATED_BODY()

    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    AMythicNPCCharacter *GetFromPool(FGameplayTag NPCType);

    // Tracked NPCs - Should only track NPCs that the player has interacted with. Non-interacted NPCs are one of.
    // I.e if the player is running around in a village, and goes up to an NPC and interacts (i.e talk, recruit, fight, or trade with), then that NPC is tracked.
    // If the player leaves the village and comes back, when populating the village, tracked NPCs will be spawned first, and then randomly generated NPCs will be spawned.
    UPROPERTY()
    TMap<FGuid, FMythicCachedNPCData> CachedNPCs;

    // Tracked Families.
    UPROPERTY()
    TMap<FGuid, FFamilySpec> CachedFamilies;

    // This Data Table holds the NPC Data for each NPC Type
    UPROPERTY(EditAnywhere, Category = "NPC Manager", meta = (RowType = "/Script/Mythic.FNPCTypeDefinition"))
    TSoftObjectPtr<UDataTable> NPCTypeDataTable;

    // Active NPCs
    UPROPERTY()
    TMap<FGuid, AMythicNPCCharacter *> ActiveNPCs;

    // Active Family Specs
    UPROPERTY()
    TMap<FGuid, FFamilySpec> ActiveFamilySpecs;

protected: // Changed to protected for pool and helpers, or could be private
    UPROPERTY()
    TArray<TObjectPtr<AMythicNPCCharacter>> NPCCharacterPool;

    // Returns NPC to the pool, and if bShouldCache is true, caches the NPC's data before returning it to the pool
    void ReturnToPool(AMythicNPCCharacter *NPC, bool bShouldCache = true);

public:
    UFUNCTION(BlueprintCallable, Category = "NPC Manager|Spawning")
    AMythicNPCCharacter *SpawnPredefinedNPC(UNPCDefinition *NPCDef, FVector SpawnLocation, FRotator SpawnRotation);

    UFUNCTION(BlueprintCallable, Category = "NPC Manager|Spawning")
    AMythicNPCCharacter *SpawnRandomNPC(FGameplayTag NPCType, FVector SpawnLocation, FRotator SpawnRotation);

    UFUNCTION(BlueprintCallable, Category = "NPC Manager|Spawning")
    AMythicNPCCharacter *SpawnCachedNPC(FGuid NPCId, FVector SpawnLocation, FRotator SpawnRotation);

    UFUNCTION(BlueprintCallable, Category= "NPC Manager")
    bool GetCachedNPCData(FGuid NPCType, FMythicNPCData &NPCData);

    UFUNCTION(BlueprintCallable, Category = "NPC Manager")
    bool GetCachedFamily(FGuid FamilyId, FFamilySpec &FamilySpec);

    // Add an NPC to the tracked NPCs, when culling an NPC that the player has interacted with. Receives the NPC Data, and the player that interacted with it.
    void CacheNPC(FMythicNPCData NPCData);

    // Remove an NPC from the tracked NPCs, when the NPC dies.
    void RemoveCachedNPC(FGuid NPCId);

    // Get tracked NPCs for the given location
    TArray<FMythicCachedNPCData> GetCachedNPCsAtLocation(FVector Location, float Radius);
};
