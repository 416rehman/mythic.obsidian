
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayEffect.h"
#include "MythicCorpseHazard.h"
#include "MythicCorpseHazardConfig.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic Corpse Hazards"))
class MYTHIC_API UMythicCorpseHazardConfig : public UDeveloperSettings {
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName("Mythic"); }


    // Decomp stage (Fresh=0..Skeletal=3) at/after which a corpse spreads the disease/weakness aura. Default Decayed(2).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Disease", meta = (ClampMin = "0", ClampMax = "3"))
    float DiseaseStartStageInt = 2.0f;

    // Radius (cm) of the disease/weakness aura around a rotted corpse.
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Disease", meta = (ClampMin = "0.0"))
    float DiseaseRadius = 600.0f;

    // Severity added per decomp stage at/after the disease start stage (scales the applied malus magnitude).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Disease", meta = (ClampMin = "0.0"))
    float DiseaseSeverityPerStage = 0.5f;

    // Radius (cm) within which an unburied corpse contributes a sanitation/morale penalty to a settlement.
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Sanitation", meta = (ClampMin = "0.0"))
    float SanitationRadius = 2000.0f;

    // Sanitation/morale penalty a FRESH corpse contributes at zero distance (decays with distance + decomp stage).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Sanitation", meta = (ClampMin = "0.0"))
    float SanitationPerFreshCorpse = 1.0f;

    // Overall scale of a corpse's carrion (scavenger-lure) attractiveness (× the mid-decay profile).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Carrion", meta = (ClampMin = "0.0"))
    float CarrionAttractPerStage = 1.0f;

    // Seconds between hazard-subsystem ticks (the single repeating timer cadence).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Tick", meta = (ClampMin = "0.5"))
    float TickSeconds = 5.0f;


    // Duration debuff applied to pawns in the disease aura. Null → the native UMythicGE_CorpseDisease is used.
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Disease")
    TSoftClassPtr<UGameplayEffect> DiseaseEffectClass;

    // Power (attack output) malus applied per point of disease severity (SetByCaller, applied negative).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Disease", meta = (ClampMin = "0.0"))
    float DiseasePowerMalusPerSeverity = 4.0f;

    // Health-regen malus applied per point of disease severity (SetByCaller, applied negative).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Disease", meta = (ClampMin = "0.0"))
    float DiseaseHealthRegenMalusPerSeverity = 1.0f;

    // Disease debuff duration (seconds). <= 0 → auto = TickSeconds × 2.5 (persists between ticks, lapses after leaving).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Disease", meta = (ClampMin = "0.0"))
    float DiseaseDurationSeconds = 0.0f;


    // Max rotted corpses that emit disease per tick (round-robin across the registry when there are more).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Budget", meta = (ClampMin = "1"))
    int32 MaxDiseaseCorpsesPerTick = 12;

    // Max pawns each emitting corpse debuffs per tick (bounds one aura's overlap fan-out).
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Budget", meta = (ClampMin = "1"))
    int32 MaxDiseaseTargetsPerCorpse = 16;

    // Hard cap on concurrently-live corpse actors world-wide (replication + registry cost bound). Registering a
    // corpse past the cap evicts the OLDEST body that is not currently being looted (FMythicCorpseHazardRules::
    // ShouldEvictForCap / PickEvictIndex). 0 disables the cap. Generous default — a co-op session rarely holds this
    // many un-decayed bodies at once. J5 perf pass.
    UPROPERTY(Config, EditAnywhere, Category = "Corpse Hazard|Budget", meta = (ClampMin = "0"))
    int32 MaxActiveCorpses = 64;

    FMythicCorpseHazardConfig BuildRuntimeConfig() const {
        FMythicCorpseHazardConfig Out;
        Out.DiseaseStartStageInt = DiseaseStartStageInt;
        Out.DiseaseRadius = DiseaseRadius;
        Out.DiseaseSeverityPerStage = DiseaseSeverityPerStage;
        Out.SanitationRadius = SanitationRadius;
        Out.SanitationPerFreshCorpse = SanitationPerFreshCorpse;
        Out.CarrionAttractPerStage = CarrionAttractPerStage;
        Out.TickSeconds = TickSeconds;
        return Out;
    }
};
