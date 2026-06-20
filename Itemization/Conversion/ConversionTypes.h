#pragma once

#include "CoreMinimal.h"
#include "ConversionTypes.generated.h"

class UItemDefinition;
class UMythicItemInstance;

/** How a recipe ingredient slot matches a concrete item. */
UENUM(BlueprintType)
enum class EConversionMatchMode : uint8 {
    ExactItem UMETA(DisplayName="Exact Item"),
    TypeQuery UMETA(DisplayName="Item Type / Tag Query")
};

/** Whether a product mints a new item or transforms a consumed input in place. */
UENUM(BlueprintType)
enum class EConversionProductMode : uint8 {
    Create UMETA(DisplayName="Create New Item"),
    Transform UMETA(DisplayName="Transform Input In-Place")
};

/** How a created product's item level is determined. */
UENUM(BlueprintType)
enum class EProductLevelMode : uint8 {
    FixedLevel UMETA(DisplayName="Fixed Level"),
    InheritInputLevel UMETA(DisplayName="Inherit Input Level"),
    InheritStationLevel UMETA(DisplayName="Inherit Station Level")
};

/** What initiates a recipe: a player selecting it, or the station auto-firing when inputs are present. */
UENUM(BlueprintType)
enum class EConversionTrigger : uint8 {
    ManualSelect UMETA(DisplayName="Manual Select"),
    Automatic UMETA(DisplayName="Automatic When Inputs Present")
};

/** The timing model of a recipe's processing. */
UENUM(BlueprintType)
enum class EConversionTiming : uint8 {
    Instant UMETA(DisplayName="Instant"),
    Timed UMETA(DisplayName="Timed"),
    Continuous UMETA(DisplayName="Continuous")
};

/** Where the products of a completed cycle are delivered. */
UENUM(BlueprintType)
enum class EConversionOutputRouting : uint8 {
    StationOutputSlots UMETA(DisplayName="Station Output Slots"),
    GiveToInstigator UMETA(DisplayName="Give To Player"),
    DropInWorld UMETA(DisplayName="Drop In World")
};

/** Lifecycle state of a queued conversion job. */
UENUM(BlueprintType)
enum class EConversionJobState : uint8 {
    Pending,
    Processing,
    Stalled,
    Completed
};

/** Why a job is currently stalled (surfaced to the UI). */
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
