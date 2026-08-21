
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "PopulationSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicSettlementRegistry;
class UMythicFactionDatabase;
class UMythicRoleDatabase;
struct FMythicCellCoord;
struct FMythicFactionData;
struct FMythicResourceStock;
struct FMythicArchetypeRow;
struct FMythicArchetypeContext;
enum class EMythicSettlementEconomy : uint8;

UCLASS()
class MYTHIC_API UMythicPopulationSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicPopulationSpawnerProcessor();

    static int32 ComputeTargetDensity(int32 SettlementMaxDensity, int32 SystemMaxPerCell, int32 FactionPopulation, int32 FactionCapacity);

    static EMythicSettlementEconomy ResolveEconomy(EMythicSettlementEconomy Authored, const FMythicResourceStock& FactionBaseProduction);

    static FGameplayTag DeriveArchetype(TConstArrayView<FMythicArchetypeRow> Catalog, const FMythicArchetypeContext& Ctx,
                                        uint32 NameHash, const FMythicArchetypeRow*& OutChosen);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    static FGameplayTag ApplyFactionGate(const UMythicRoleDatabase* RoleDB, const FGameplayTag& DerivedRole, const FGameplayTag& FactionTag);

    FMassEntityQuery ExistingNPCQuery;

    float TimeSinceLastTick = 0.0f;
};
