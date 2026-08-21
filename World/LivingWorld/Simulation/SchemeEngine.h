
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/Simulation/SchemeTypes.h"
#include "SchemeEngine.generated.h"

class UMythicFactionDatabase;
class UMythicCausalFabric;
class UMythicTerritoryGrid;
class UMythicLivingWorldSettings;

UCLASS()
class MYTHIC_API UMythicSchemeEngine : public UObject {
    GENERATED_BODY()

public:
    void Initialize(
        UMythicFactionDatabase *InFactionDB,
        UMythicCausalFabric *InFabric,
        UMythicTerritoryGrid *InTerritoryGrid,
        const UMythicLivingWorldSettings *InSettings);


    void TickSchemes(float SimDeltaTime, uint32 SimTickIndex);


    TArray<FMythicScheme> GetActiveSchemes() const;

    TArray<FMythicScheme> GetSchemesByFaction(FMythicFactionId Faction) const;

    int32 GetActiveSchemeCount() const;


    virtual void Serialize(FArchive &Ar) override;

private:

    void GenerateSchemes(float SimDeltaTime, uint32 SimTickIndex);

    void GetEligibleSchemeTypes(int32 FactionIndex, TArray<EMythicSchemeType> &OutEligibleTypes) const;

    float CalculateProgressRate(const FMythicScheme &Scheme, int32 FactionIndex) const;

    float CalculateDetectionRisk(const FMythicScheme &Scheme, int32 TargetFactionIndex) const;


    void ProgressScheme(FMythicScheme &Scheme, float SimDeltaTime);

    void ExecuteScheme(FMythicScheme &Scheme);

    void ApplySchemeEffects(const FMythicScheme &Scheme);

    void OnSchemeDiscovered(FMythicScheme &Scheme);


    UPROPERTY()
    TObjectPtr<UMythicFactionDatabase> FactionDB;

    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> Fabric;

    UPROPERTY()
    TObjectPtr<UMythicTerritoryGrid> TerritoryGrid;

    const UMythicLivingWorldSettings *Settings = nullptr;


    TArray<FMythicScheme> ActiveSchemes;

    mutable FCriticalSection SchemeLock;

    uint32 NextSchemeId = 1;

    int32 GenerationTickInterval = 10;

    int32 MaxSchemesPerFaction = 5;

    int32 MaxTotalSchemes = 50;

    float SchemeBaseProbability = 0.05f;
};
