
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "GroupSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicFactionDatabase;
struct FMythicGroupTemplate;
struct FMythicGroupMemberSpec;
struct FMythicFactionData;
enum class EMythicSettlementEconomy : uint8;

UCLASS()
class MYTHIC_API UMythicGroupSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicGroupSpawnerProcessor();

    static bool TemplateEligible(const FMythicGroupTemplate &Template, const FMythicFactionData &Faction,
                                 EMythicSettlementEconomy EffEconomy);

    static bool PickTemplateIndex(const TArray<FMythicGroupTemplate> &Templates, EMythicSettlementEconomy EffEconomy,
                                  const FMythicFactionData &Faction, uint32 Seed, int32 &OutIndex);

    static int32 RollMemberCount(const FMythicGroupMemberSpec &Spec, uint32 Seed);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery ExistingNPCQuery;

    FMassEntityQuery ExistingGroupQuery;

    float TimeSinceLastTick = 0.0f;
};
