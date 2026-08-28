#pragma once

#include "CoreMinimal.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "MythicItemFactoryTypes.generated.h"

class AActor;
class UItemDefinition;
class UMythicItemInstance;

/** The complete, caller-visible result of a readiness-aware item creation request. */
UENUM(BlueprintType)
enum class EMythicCreateItemStatus : uint8 {
    Success,
    NotReady,
    InvalidData,
    GenerationFailed
};

/** Internal initialization outcome. NotReady is owned by the factory, before an item is allocated. */
UENUM()
enum class EMythicItemInitializeStatus : uint8 {
    Success,
    InvalidData,
    GenerationFailed
};

/**
 * Authoritative creation input. ItemDefinition is already loaded; the factory never resolves an asset path.
 * ServerSeed == 0 selects the canonical seed derived from the newly assigned ItemInstanceGuid and profile ID.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCreateItemRequest {
    GENERATED_BODY()

    /** Loaded item template to instantiate; the factory never resolves an asset path supplied by Blueprint. */
    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<UItemDefinition> ItemDefinition = nullptr;

    /** Number of items in the created stack; individual non-stackable definitions may constrain the final quantity. */
    UPROPERTY(BlueprintReadWrite, meta = (ClampMin = "1"))
    int32 Quantity = 1;

    /** Gameplay level used by authoritative affix tiers, scaling curves and other fragment initialization. */
    UPROPERTY(BlueprintReadWrite, meta = (ClampMin = "1"))
    int32 ItemLevel = 1;

    /** Actor that will own the new item instance; authority and fragment initialization are derived from this context. */
    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<AActor> OwningActor = nullptr;

    // Native-only: Blueprint cannot represent an unsigned 64-bit deterministic seed without loss.
    uint64 ServerSeed = 0;

    // Native-only immutable construction override. The definition must own exactly one YieldQualityFragment.
    TOptional<EMythicYieldQuality> YieldQualityOverride;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCreateItemResult {
    GENERATED_BODY()

    /** Final factory outcome; only Success guarantees that Item contains a fully initialized committed instance. */
    UPROPERTY(BlueprintReadOnly)
    EMythicCreateItemStatus Status = EMythicCreateItemStatus::InvalidData;

    /** Newly created item on Success, otherwise null; callers must not retain a failed staging instance. */
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UMythicItemInstance> Item = nullptr;

    /** Stable developer-facing diagnostic identity; never player-facing text. */
    UPROPERTY(BlueprintReadOnly)
    FName DiagnosticCode;

    bool IsSuccess() const { return Status == EMythicCreateItemStatus::Success && Item != nullptr; }
};

struct MYTHIC_API FMythicItemInitializeResult {
    EMythicItemInitializeStatus Status = EMythicItemInitializeStatus::InvalidData;
    FName DiagnosticCode;

    bool IsSuccess() const { return Status == EMythicItemInitializeStatus::Success; }
};

DECLARE_DELEGATE_OneParam(FOnMythicItemCreated, FMythicCreateItemResult);
DECLARE_DELEGATE_OneParam(FOnMythicItemDefinitionReady, bool /* bSuccess */);
