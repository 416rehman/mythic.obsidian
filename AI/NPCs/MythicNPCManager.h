
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

USTRUCT()
struct FMythicNPCDefinitionBucket {
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<UNPCDefinition>> Defs;
};

USTRUCT()
struct FMythicNPCPoolBucket {
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<AMythicNPCCharacter>> NPCs;
};

UCLASS()
class MYTHIC_API UMythicNPCManager : public UGameInstanceSubsystem {
    GENERATED_BODY()

    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    AMythicNPCCharacter *GetFromPool(UClass *NPCClass);

    // The spawn API is BlueprintCallable and the subsystem exists on clients too; a client-side SpawnActor
    // would make a locally-authoritative phantom NPC only that machine can see. Checked per call because
    // Initialize runs against the pre-travel dummy world in packaged builds.
    bool IsAuthoritativeWorld() const;

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
    // Pooled by actor class: a recycled body must match the class the definition asks for, or a bandit
    // definition could come back wearing the merchant's Blueprint.
    UPROPERTY()
    TMap<TObjectPtr<UClass>, FMythicNPCPoolBucket> NPCCharacterPool;

    void ReturnToPool(AMythicNPCCharacter *NPC, bool bShouldCache = true);

    AMythicNPCCharacter *AcquireNPC(UClass *NPCClass, const FVector &SpawnLocation, const FRotator &SpawnRotation);

    /**
     * Definitions bucketed by NPCType, built lazily from the asset registry on the first random spawn.
     * One scan for the world's lifetime, so emergent spawns cost a map lookup and never a registry walk.
     */
    UPROPERTY()
    TMap<FGameplayTag, FMythicNPCDefinitionBucket> DefinitionsByType;

    bool bDefinitionIndexBuilt = false;

    void BuildDefinitionIndex();

public:
    /** Spawns one authored NPC definition through the canonical pooled preparation and combat-commit path. */
    UFUNCTION(BlueprintCallable, Category = "NPC Manager|Spawning")
    AMythicNPCCharacter *SpawnPredefinedNPC(UNPCDefinition *NPCDef, FVector SpawnLocation, FRotator SpawnRotation);

    /** Selects an authored definition under NPCType, then spawns it through the canonical pooled runtime path. */
    UFUNCTION(BlueprintCallable, Category = "NPC Manager|Spawning")
    AMythicNPCCharacter *SpawnRandomNPC(FGameplayTag NPCType, FVector SpawnLocation, FRotator SpawnRotation);

    /** Re-embodies one cached logical NPC by stable ID, preserving its persisted identity and authored state. */
    UFUNCTION(BlueprintCallable, Category = "NPC Manager|Spawning")
    AMythicNPCCharacter *SpawnCachedNPC(FGuid NPCId, FVector SpawnLocation, FRotator SpawnRotation);

    /**
     * The combat level an enemy spawning here should carry: the authored level for the territory danger tier at
     * this position, lifted by the world tier's ItemLevelBase. The one place spawn danger becomes a number.
     */
    UFUNCTION(BlueprintCallable, Category = "NPC Manager|Spawning")
    int32 ResolveCombatLevelAt(const FVector &SpawnLocation) const;

    /**
     * Take a finished NPC back into the pool instead of destroying it. True when this manager owned the
     * actor; false means another system (Mass embodiment) owns its lifecycle and the caller should proceed
     * with its own teardown.
     */
    bool ReclaimNPC(AMythicNPCCharacter *NPC);

    /** Authored definitions whose type sits under this tag. Builds the index on first use. */
    UFUNCTION(BlueprintCallable, Category = "NPC Manager|Spawning")
    int32 CountDefinitionsForType(FGameplayTag NPCType);

    /**
     * Re-runs combat scaling on every active NPC. Called when the session's player count changes, so
     * long-lived NPCs stamped for a full party do not keep party-sized health against a lone survivor.
     */
    void RefreshCombatScalingOnActive();

    /** Invalidates manager-owned logical NPCs whose canonical identities are about to be replaced by world restore. */
    void ResetForLivingWorldRestore();

    /** Copies cached logical NPC data for NPCId without spawning or mutating the cached record. */
    UFUNCTION(BlueprintCallable, Category= "NPC Manager")
    bool GetCachedNPCData(FGuid NPCId, FMythicNPCData &NPCData);

    /** Copies the cached family specification for FamilyId without mutating the authoritative cache. */
    UFUNCTION(BlueprintCallable, Category = "NPC Manager")
    bool GetCachedFamily(FGuid FamilyId, FFamilySpec &FamilySpec);

    void CacheNPC(FMythicNPCData NPCData);

    void RemoveCachedNPC(FGuid NPCId);

    int32 GetActiveNPCCount() const { return ActiveNPCs.Num(); }
    int32 GetCachedNPCCount() const { return CachedNPCs.Num(); }
    int32 GetCachedFamilyCount() const { return CachedFamilies.Num(); }
    int32 GetActiveFamilyCount() const { return ActiveFamilySpecs.Num(); }
    int32 GetPooledNPCCount() const {
        int32 Count = 0;
        for (const TPair<TObjectPtr<UClass>, FMythicNPCPoolBucket> &Bucket : NPCCharacterPool) {
            Count += Bucket.Value.NPCs.Num();
        }
        return Count;
    }
};
