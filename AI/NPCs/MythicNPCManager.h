
#pragma once

#include "CoreMinimal.h"
#include "FamilyDefinition.h"
#include "MythicAIController.h"
#include "MythicNPCData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicNPCManager.generated.h"

class AMythicNPCCharacter;
USTRUCT(BlueprintType, Blueprintable)
struct FMythicCachedNPCData {
    GENERATED_BODY()

    // The Data of the NPC
    UPROPERTY(BlueprintReadOnly)
    FMythicNPCData NPCData;


    FMythicCachedNPCData() {
        NPCData = FMythicNPCData();
    }

    FMythicCachedNPCData(FMythicNPCData InNPCData) {
        NPCData = InNPCData;
    }
};

UCLASS()
class MYTHIC_API UMythicNPCManager : public UGameInstanceSubsystem {
    GENERATED_BODY()

    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    AMythicNPCCharacter *GetFromPool(FGameplayTag NPCType);

    UPROPERTY()
    TMap<FGuid, FMythicCachedNPCData> CachedNPCs;

    UPROPERTY()
    TMap<FGuid, FFamilySpec> CachedFamilies;

    // This Data Table holds the NPC Data for each NPC Type
    UPROPERTY(EditAnywhere, Category = "NPC Manager", meta = (RowType = "/Script/Mythic.FNPCTypeDefinition"))
    TSoftObjectPtr<UDataTable> NPCTypeDataTable;

    UPROPERTY()
    TMap<FGuid, AMythicNPCCharacter *> ActiveNPCs;

    UPROPERTY()
    TMap<FGuid, FFamilySpec> ActiveFamilySpecs;

protected:
    UPROPERTY()
    TArray<TObjectPtr<AMythicNPCCharacter>> NPCCharacterPool;

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

    void CacheNPC(FMythicNPCData NPCData);

    void RemoveCachedNPC(FGuid NPCId);

    int32 GetActiveNPCCount() const { return ActiveNPCs.Num(); }
    int32 GetCachedNPCCount() const { return CachedNPCs.Num(); }
    int32 GetCachedFamilyCount() const { return CachedFamilies.Num(); }
    int32 GetActiveFamilyCount() const { return ActiveFamilySpecs.Num(); }
    int32 GetPooledNPCCount() const { return NPCCharacterPool.Num(); }
};
