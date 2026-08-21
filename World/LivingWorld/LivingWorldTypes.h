
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LivingWorldTypes.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogMythLivingWorld, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythFaction, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythTerritory, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythMorality, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythCausalFabric, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythWorldSim, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythPopulation, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythSettlement, Log, All);


UENUM(BlueprintType)
enum class EMythicActionCategory : uint8 {
    Melee,
    Ranged,
    Magic_Damage,
    Magic_Healing,
    Magic_Forbidden,
    Environmental,
    COUNT UMETA(Hidden)
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicFactionId {
    GENERATED_BODY()

    static constexpr uint8 InvalidIndex = 0xFF;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8 Index = InvalidIndex;

    bool IsValid() const { return Index != InvalidIndex; }

    bool operator==(const FMythicFactionId &Other) const { return Index == Other.Index; }
    bool operator!=(const FMythicFactionId &Other) const { return Index != Other.Index; }

    friend uint32 GetTypeHash(const FMythicFactionId &Id) { return ::GetTypeHash(Id.Index); }
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCellCoord {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 X = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Y = 0;

    FMythicCellCoord() = default;
    FMythicCellCoord(int32 InX, int32 InY) : X(InX), Y(InY) {}

    bool operator==(const FMythicCellCoord &Other) const { return X == Other.X && Y == Other.Y; }
    bool operator!=(const FMythicCellCoord &Other) const { return !(*this == Other); }

    friend uint32 GetTypeHash(const FMythicCellCoord &Coord) {
        return HashCombine(::GetTypeHash(Coord.X), ::GetTypeHash(Coord.Y));
    }

    FString ToString() const { return FString::Printf(TEXT("(%d,%d)"), X, Y); }
};


UENUM(BlueprintType)
enum class EMythicMoralAxis : uint8 {
    Violence UMETA(DisplayName = "Violence"),

    Theft UMETA(DisplayName = "Theft"),

    Deception UMETA(DisplayName = "Deception"),

    Mercy UMETA(DisplayName = "Mercy"),

    Loyalty UMETA(DisplayName = "Loyalty"),

    Sanctity UMETA(DisplayName = "Sanctity"),

    Authority UMETA(DisplayName = "Authority"),

    Arcane UMETA(DisplayName = "Arcane"),

    COUNT UMETA(Hidden)
};

static constexpr int32 MoralAxisCount = static_cast<int32>(EMythicMoralAxis::COUNT);


UENUM(BlueprintType)
enum class EMythicResourceType : uint8 {
    Food UMETA(DisplayName = "Food"),

    Materials UMETA(DisplayName = "Materials"),

    Arms UMETA(DisplayName = "Arms"),

    Wealth UMETA(DisplayName = "Wealth"),

    COUNT UMETA(Hidden)
};

static constexpr int32 ResourceTypeCount = static_cast<int32>(EMythicResourceType::COUNT);


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicResourceStock {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Food = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Materials = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Arms = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Wealth = 0.0f;

    float GetResource(EMythicResourceType Type) const {
        switch (Type) {
        case EMythicResourceType::Food:
            return Food;
        case EMythicResourceType::Materials:
            return Materials;
        case EMythicResourceType::Arms:
            return Arms;
        case EMythicResourceType::Wealth:
            return Wealth;
        default:
            return 0.0f;
        }
    }

    float &GetResourceMutable(EMythicResourceType Type) {
        switch (Type) {
        case EMythicResourceType::Food:
            return Food;
        case EMythicResourceType::Materials:
            return Materials;
        case EMythicResourceType::Arms:
            return Arms;
        case EMythicResourceType::Wealth:
            return Wealth;
        default:
            static float Dummy = 0.0f;
            return Dummy;
        }
    }

    FMythicResourceStock &operator+=(const FMythicResourceStock &Other) {
        Food += Other.Food;
        Materials += Other.Materials;
        Arms += Other.Arms;
        Wealth += Other.Wealth;
        return *this;
    }

    FMythicResourceStock &operator-=(const FMythicResourceStock &Other) {
        Food -= Other.Food;
        Materials -= Other.Materials;
        Arms -= Other.Arms;
        Wealth -= Other.Wealth;
        return *this;
    }

    FMythicResourceStock &operator*=(float Scalar) {
        Food *= Scalar;
        Materials *= Scalar;
        Arms *= Scalar;
        Wealth *= Scalar;
        return *this;
    }

    void ClampAll(float Min, float Max) {
        Food = FMath::Clamp(Food, Min, Max);
        Materials = FMath::Clamp(Materials, Min, Max);
        Arms = FMath::Clamp(Arms, Min, Max);
        Wealth = FMath::Clamp(Wealth, Min, Max);
    }
};


UENUM(BlueprintType)
enum class EMythicPressureChannel : uint8 {
    Threat UMETA(DisplayName = "Threat"),
    Injustice UMETA(DisplayName = "Injustice"),
    Grief UMETA(DisplayName = "Grief"),
    Shame UMETA(DisplayName = "Shame"),
    Desire UMETA(DisplayName = "Desire"),
    Wrath UMETA(DisplayName = "Wrath"),

    COUNT UMETA(Hidden)
};

static constexpr int32 PressureChannelCount = static_cast<int32>(EMythicPressureChannel::COUNT);


UENUM(BlueprintType)
enum class EMythicVentChannel : uint8 {
    Fight UMETA(DisplayName = "Fight"),
    Flee UMETA(DisplayName = "Flee"),
    Enforce UMETA(DisplayName = "Enforce"),
    Report UMETA(DisplayName = "Report"),
    Exploit UMETA(DisplayName = "Exploit"),
    Tend UMETA(DisplayName = "Tend"),
    Rally UMETA(DisplayName = "Rally"),
    Submit UMETA(DisplayName = "Submit"),

    COUNT UMETA(Hidden)
};


UENUM(BlueprintType)
enum class EMythicSignificanceTier : uint8 {
    Tier0_Ambient UMETA(DisplayName = "Ambient"),

    Tier1_Reactive UMETA(DisplayName = "Reactive"),

    Tier2_Cognitive UMETA(DisplayName = "Cognitive"),

    Tier3_Persistent UMETA(DisplayName = "Persistent")
};


UENUM(BlueprintType)
enum class EMythicMoralSeverity : uint8 {
    Ignore UMETA(DisplayName = "Ignore"),
    Disapprove UMETA(DisplayName = "Disapprove"),
    Condemn UMETA(DisplayName = "Condemn"),
    Hostile UMETA(DisplayName = "Hostile")
};
