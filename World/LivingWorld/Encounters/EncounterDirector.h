
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "EncounterDirector.generated.h"

class UMythicCausalFabric;
class UMythicFactionDatabase;
class UMythicTerritoryGrid;
class UMythicLivingWorldSettings;
class UMythicLivingWorldSubsystem;
class UObjectiveDefinition;
struct FMythicPartyReputation;

UCLASS()
class MYTHIC_API UMythicEncounterDirector : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;


    void RegisterTemplate(const FMythicEncounterTemplate &Template);


    const TArray<FMythicActiveEncounter> &GetActiveEncounters() const { return ActiveEncounters; }

    int32 GetActiveEncounterCount() const { return ActiveEncounters.Num(); }

    int32 GetMaxActiveEncounters() const { return MaxActiveEncounters; }

    bool HasEncounterInCell(const FMythicCellCoord &Cell) const;


    bool ForceCompleteEncounter(uint32 EncounterId);

    UObjectiveDefinition *GetEncounterClearObjective() const;

private:

    void EvaluationTick();

    bool EvaluateTemplate(const FMythicEncounterTemplate &Template, FMythicCellCoord &OutCell, FMythicFactionId &OutFaction,
                          float &OutSpawnProbability) const;

    FMythicPartyReputation ComputePartyReputation(const FMythicCellCoord &Cell) const;

    void SpawnEncounter(const FMythicEncounterTemplate &Template, const FMythicCellCoord &Cell, FMythicFactionId Faction);

    void UpdateActiveEncounters();

    void MaybeOfferClearObjectives();

    void CleanupEncounter(int32 Index);

    void EmitEncounterCompletedEvent(const FMythicActiveEncounter &Encounter, bool bDefeated) const;


    TMap<FGameplayTag, double> TemplateCooldowns;

    bool IsOnCooldown(const FGameplayTag &TemplateTag, double WorldTime, float CooldownSeconds) const;

    int32 CountActiveInstances(const FGameplayTag &TemplateTag) const;


    TArray<FMythicEncounterTemplate> Templates;

    TArray<FMythicActiveEncounter> ActiveEncounters;

    int32 MaxActiveEncounters = 10;

    float EvaluationInterval = 5.0f;

    uint32 NextEncounterId = 1;

    FTimerHandle EvaluationTimerHandle;


    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> CausalFabric;

    UPROPERTY()
    TObjectPtr<UMythicFactionDatabase> FactionDB;

    UPROPERTY()
    TObjectPtr<UMythicTerritoryGrid> TerritoryGrid;

    const UMythicLivingWorldSettings *Settings = nullptr;

    UPROPERTY()
    TObjectPtr<UObjectiveDefinition> DefaultClearObjective;

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorld;
};
