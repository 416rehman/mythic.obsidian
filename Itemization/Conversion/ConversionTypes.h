#pragma once

#include "CoreMinimal.h"
#include "ConversionTypes.generated.h"

class UItemDefinition;
class UMythicItemInstance;

UENUM(BlueprintType)
enum class EConversionMatchMode : uint8 {
    ExactItem UMETA(DisplayName="Exact Item"),
    TypeQuery UMETA(DisplayName="Item Type / Tag Query")
};

UENUM(BlueprintType)
enum class EConversionProductMode : uint8 {
    Create UMETA(DisplayName="Create New Item"),
    Transform UMETA(DisplayName="Transform Input In-Place")
};

UENUM(BlueprintType)
enum class EProductLevelMode : uint8 {
    FixedLevel UMETA(DisplayName="Fixed Level"),
    InheritInputLevel UMETA(DisplayName="Inherit Input Level"),
    InheritStationLevel UMETA(DisplayName="Inherit Station Level"),
    ProficiencyScaled UMETA(DisplayName="Scaled By Crafter Proficiency")
};

UENUM(BlueprintType)
enum class EConversionTrigger : uint8 {
    ManualSelect UMETA(DisplayName="Manual Select"),
    Automatic UMETA(DisplayName="Automatic When Inputs Present")
};

UENUM(BlueprintType)
enum class EConversionTiming : uint8 {
    Instant UMETA(DisplayName="Instant"),
    Timed UMETA(DisplayName="Timed"),
    Continuous UMETA(DisplayName="Continuous")
};

UENUM(BlueprintType)
enum class EConversionOutputRouting : uint8 {
    StationOutputSlots UMETA(DisplayName="Station Output Slots"),
    GiveToInstigator UMETA(DisplayName="Give To Player"),
    DropInWorld UMETA(DisplayName="Drop In World")
};

UENUM(BlueprintType)
enum class EConversionJobState : uint8 {
    Pending,
    Processing,
    Stalled,
    Completed
};

UENUM(BlueprintType)
enum class EConversionStallReason : uint8 {
    None,
    NoFuel,
    OutputFull,
    MissingInput,
    MissingCatalyst,
    InstigatorGone,
    NotLoaded
};
