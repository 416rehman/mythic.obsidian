
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "SchemeTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythScheme, Log, All);


UENUM(BlueprintType)
enum class EMythicSchemeType : uint8 {
    Assassination UMETA(DisplayName = "Assassination"),

    TradeDisruption UMETA(DisplayName = "TradeDisruption"),

    TerritoryReclaim UMETA(DisplayName = "TerritoryReclaim"),

    SpyInfiltration UMETA(DisplayName = "SpyInfiltration"),

    CompanionRecruitment UMETA(DisplayName = "CompanionRecruitment"),

    MilitaryRaid UMETA(DisplayName = "MilitaryRaid"),

    DiplomaticPressure UMETA(DisplayName = "DiplomaticPressure"),

    COUNT UMETA(Hidden)
};

static constexpr int32 SchemeTypeCount = static_cast<int32>(EMythicSchemeType::COUNT);


UENUM(BlueprintType)
enum class EMythicSchemeState : uint8 {
    Planning UMETA(DisplayName = "Planning"),

    InProgress UMETA(DisplayName = "InProgress"),

    Succeeded UMETA(DisplayName = "Succeeded"),

    Failed UMETA(DisplayName = "Failed"),

    Discovered UMETA(DisplayName = "Discovered")
};


USTRUCT()
struct FMythicScheme {
    GENERATED_BODY()

    uint32 SchemeId = 0;

    FMythicFactionId OriginFaction;

    FMythicFactionId TargetFaction;

    EMythicSchemeType Type = EMythicSchemeType::Assassination;

    EMythicSchemeState State = EMythicSchemeState::Planning;

    float Progress = 0.0f;

    float ProgressRate = 0.01f;

    float DetectionRisk = 0.05f;

    double StartGameTime = 0.0;

    FMythicCellCoord TargetCell;

    bool IsDiscovered() const { return State == EMythicSchemeState::Discovered; }

    bool IsActive() const {
        return State == EMythicSchemeState::Planning || State == EMythicSchemeState::InProgress;
    }
};
