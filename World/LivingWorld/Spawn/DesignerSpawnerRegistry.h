
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/Spawn/DesignerSpawnerTypes.h"
#include "DesignerSpawnerRegistry.generated.h"

UCLASS()
class MYTHIC_API UMythicDesignerSpawnerRegistry : public UObject {
    GENERATED_BODY()

public:
    FMythicDesignerSpawnerState& FindOrAdd(FName DesignerId) {
        return States.FindOrAdd(DesignerId);
    }

    const FMythicDesignerSpawnerState* Find(FName DesignerId) const {
        return States.Find(DesignerId);
    }

    void RecordSpawn(FName DesignerId) {
        FMythicDesignerSpawnerState& S = States.FindOrAdd(DesignerId);
        ++S.SpawnsEver;
    }

    void RecordDeath(FName DesignerId, double WorldTime, bool bPerma) {
        FMythicDesignerSpawnerState& S = States.FindOrAdd(DesignerId);
        S.LastDeathTime = WorldTime;
        S.bPermaDead |= bPerma;
    }

    void GetAllDesignerIds(TArray<FName>& OutIds) const {
        States.GetKeys(OutIds);
    }

    virtual void Serialize(FArchive& Ar) override;

private:
    UPROPERTY()
    TMap<FName, FMythicDesignerSpawnerState> States;
};
