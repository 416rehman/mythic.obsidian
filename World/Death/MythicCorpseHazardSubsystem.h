
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayEffect.h"
#include "Templates/SubclassOf.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicCorpseHazard.h"
#include "MythicCorpseHazardSubsystem.generated.h"

class AMythicCorpse;
class UMythicLivingWorldSubsystem;
class UAbilitySystemComponent;

UCLASS()
class MYTHIC_API UMythicGE_CorpseDisease : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_CorpseDisease();
};

struct FMythicCarrionPoint {
    FVector Location = FVector::ZeroVector;
    float Attractiveness = 0.0f;
    int32 DecompStageInt = 0;
};

UCLASS()
class MYTHIC_API UMythicCorpseHazardSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Deinitialize() override;


    void RegisterCorpse(AMythicCorpse *Corpse);

    void UnregisterCorpse(AMythicCorpse *Corpse);


    float GetSanitationPenaltyForLocation(const FVector &SettlementLocation) const;

    float GetSanitationPenaltyForSettlement(const FMythicCellCoord &SettlementCell) const;

    void GetCarrionPointsNear(const FVector &Location, float Radius, TArray<FMythicCarrionPoint> &Out) const;

    float GetTotalCarrionAttractivenessNear(const FVector &Location, float Radius) const;

    void GetCorpsesNear(const FVector &Origin, float Radius, TArray<AMythicCorpse *> &Out) const;

    const FMythicCorpseHazardConfig &GetConfig() const { return Config; }

    int32 GetRegisteredCorpseCount() const;

private:
    void TickHazards();

    void ApplyDiseaseFromCorpse(AMythicCorpse *Corpse);

    void EnforceCorpseCap();

    UMythicLivingWorldSubsystem *ResolveLivingWorld() const;

    UPROPERTY()
    TArray<TWeakObjectPtr<AMythicCorpse>> Corpses;

    int32 DiseaseCursor = 0;

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorldSubsystem;

    UPROPERTY()
    TSubclassOf<UGameplayEffect> ResolvedDiseaseClass;

    FMythicCorpseHazardConfig Config;
    float DiseasePowerMalusPerSeverity = 4.0f;
    float DiseaseHealthRegenMalusPerSeverity = 1.0f;
    float DiseaseDurationSeconds = 12.5f;
    int32 MaxDiseaseCorpsesPerTick = 12;
    int32 MaxDiseaseTargetsPerCorpse = 16;

    int32 MaxActiveCorpses = 64;

    FTimerHandle HazardTimerHandle;
};
