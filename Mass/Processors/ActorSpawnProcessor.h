
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "Mass/EntityHandle.h"
#include "ActorSpawnProcessor.generated.h"

class AMythicNPCCharacter;
class AMythicCreatureCharacter;
class UMythicLivingWorldSettings;

UCLASS()
class MYTHIC_API UMythicActorSpawnProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicActorSpawnProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

    // Fallback embodied-NPC class, used when UMythicLivingWorldSettings::EmbodiedNPCClass is unset. Point either at a
    // mesh-bearing NPC Blueprint; possession is inherited from AMythicNPCCharacter's default controller, so no
    // Blueprint wiring is required for the NPC to perceive + move.
    UPROPERTY(EditAnywhere, Category = "Living World|Spawn")
    TSubclassOf<AMythicNPCCharacter> SpawnActorClass;

    // Fallback embodied-CREATURE class, used when UMythicLivingWorldSettings::EmbodiedCreatureClass is unset AND a
    // creature is being embodied. Defaults to the bare C++ AMythicCreatureCharacter (AI-possessed, mesh-less). Point at
    // a mesh-bearing creature Blueprint to give wildlife a body.
    UPROPERTY(EditAnywhere, Category = "Living World|Spawn")
    TSubclassOf<AMythicCreatureCharacter> SpawnCreatureClass;

private:
    UPROPERTY(Transient)
    TSubclassOf<AMythicNPCCharacter> ResolvedSpawnClass;

    UPROPERTY(Transient)
    TSubclassOf<AMythicCreatureCharacter> ResolvedCreatureClass;

    bool bCreatureClassResolved = false;

    FMassEntityQuery SpawnRequestQuery;

    FMassEntityQuery CreatureSpawnRequestQuery;

    FMassEntityQuery DespawnRequestQuery;

    TMap<FMassEntityHandle, double> SpawnDeferUntil;

    bool TryFindSpawnTransform(
        UWorld *World,
        const UMythicLivingWorldSettings *Settings,
        const FVector &CellCenterXY,
        UClass *ResolvedClass,
        bool bWaterCapable,
        double Now,
        int32 &ValidationsThisTick,
        const FMassEntityHandle &Entity,
        FTransform &OutTransform);

    static bool IsActorInCloseView(UWorld *World, const AMythicNPCCharacter *Actor, const UMythicLivingWorldSettings *Settings);
};
