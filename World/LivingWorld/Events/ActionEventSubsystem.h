
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/Crime/CrimeTypes.h"
#include "ActionEventSubsystem.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;

UCLASS()
class MYTHIC_API UMythicActionEventSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    void SubmitAction(const FMythicActionEvent &Action);


    TArray<FMythicPendingActionEvent> &GetPendingEvents() { return PendingEvents; }

    void FlushProcessedEvents();

    TArray<FMythicWitnessResult> &GetPendingWitnessResults() { return PendingWitnessResults; }

    void FlushProcessedWitnessResults();

    bool HasPendingWork() const { return PendingEvents.Num() > 0 || PendingWitnessResults.Num() > 0; }


    FMythicCrimeReportQueue& GetCrimeReportQueue() { return CrimeReports; }


    float GetPerceptionMultiplier() const { return PerceptionMultiplier; }
    void SetPerceptionMultiplier(float NewMultiplier) { PerceptionMultiplier = FMath::Clamp(NewMultiplier, 0.01f, 2.0f); }

private:
    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorldSubsystem;

    UMythicLivingWorldSubsystem *ResolveLivingWorld() const;

    TArray<FMythicPendingActionEvent> PendingEvents;

    TArray<FMythicWitnessResult> PendingWitnessResults;

    FMythicCrimeReportQueue CrimeReports;

    float PerceptionMultiplier = 1.0f;

    FMythicCellCoord ResolveActorCell(const AActor *Actor) const;

    FMythicFactionId ResolveActorFaction(const AActor *Actor) const;
};

