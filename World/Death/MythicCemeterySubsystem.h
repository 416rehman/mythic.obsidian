
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/SubclassOf.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicCemeteryRules.h"
#include "MythicEpitaph.h"
#include "MythicCemeterySubsystem.generated.h"

class AMythicGrave;
class UMythicLivingWorldSubsystem;

USTRUCT()
struct FMythicCemeteryDeathRecord {
    GENERATED_BODY()

    uint32 SourceNameHash = 0;

    UPROPERTY()
    FText DisplayName;

    UPROPERTY()
    FGameplayTag RoleTag;

    UPROPERTY()
    FGameplayTag Faction;

    UPROPERTY()
    int32 SourceTier = 0;

    UPROPERTY()
    float Significance = 0.0f;

    UPROPERTY()
    double DeathTime = 0.0;

    UPROPERTY()
    FVector DeathLocation = FVector::ZeroVector;

    UPROPERTY()
    FMythicCellCoord HomeCell;

    UPROPERTY()
    uint32 KillerNameHash = 0;
};

UCLASS()
class MYTHIC_API UMythicCemeterySubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    void NotifyDeath(const FMythicCemeteryDeathRecord &Record);

    const FMythicCemeteryConfig &GetConfig() const { return Config; }

    int32 GetGraveCount(FName CemeteryKey) const { return GraveCounts.FindRef(CemeteryKey); }

    static bool ParseEpitaphTemplatesJson(const FString &JsonText, TArray<FMythicEpitaphTemplate> &Out);

private:
    void ResolveCemeteryAnchor(const FMythicCemeteryDeathRecord &Record, FName &OutKey, FVector &OutAnchor) const;

    FText ComposeEpitaph(const FMythicCemeteryDeathRecord &Record) const;

    void EnsureCountsSeeded();

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorldSubsystem;

    FMythicCemeteryConfig Config;

    TArray<FMythicEpitaphTemplate> EpitaphTemplates;

    UPROPERTY()
    TSubclassOf<AMythicGrave> GraveClass;

    TMap<FName, int32> GraveCounts;

    bool bCountsSeeded = false;
};
