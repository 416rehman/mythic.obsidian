// Mythic Living World System — Core type definitions
// All data types used across the living world shared data layer

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LivingWorldTypes.generated.h"

// ─────────────────────────────────────────────────────────────
// Log Categories
// ─────────────────────────────────────────────────────────────

DECLARE_LOG_CATEGORY_EXTERN(LogMythLivingWorld, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythFaction, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythTerritory, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythMorality, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythCausalFabric, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythWorldSim, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythPopulation, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogMythSettlement, Log, All);

// ─────────────────────────────────────────────────────────────
// Faction ID — Lightweight typed index
// ─────────────────────────────────────────────────────────────

/** Index into the faction database. Not a GUID — factions are a small, capped set. */
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

// ─────────────────────────────────────────────────────────────
// Cell Coordinate — 2D grid position
// ─────────────────────────────────────────────────────────────

/** 2D cell coordinate in the territory grid. */
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

// ─────────────────────────────────────────────────────────────
// Moral Axis — The dimensions of morality
// ─────────────────────────────────────────────────────────────

/**
 * Moral axes used for moral signature evaluation and ideology profiling.
 * Each axis represents a dimension of morality. Player/NPC actions produce events
 * with moral vectors along these axes. Factions evaluate those vectors against
 * their ideology to determine reaction severity.
 */
UENUM(BlueprintType)
enum class EMythicMoralAxis : uint8 {
    /**
     * Physical harm and combat.
     * Positive actions: murder, assault, collateral damage, executing prisoners, attacking civilians.
     * Negative actions: using non-lethal takedowns, de-escalating combat, protecting bystanders.
     * A faction with high Violence tolerance won't care about killings; low tolerance triggers hostility.
     */
    Violence UMETA(DisplayName = "Violence"),

    /**
     * Taking what isn't yours.
     * Positive actions: pickpocketing, burglary, looting homes/corpses, highway robbery.
     * Negative actions: returning stolen goods, refusing to loot, reporting thieves.
     * Theft > RaidIdeologyThreshold on a faction enables raiding income in the economy sim.
     */
    Theft UMETA(DisplayName = "Theft"),

    /**
     * Lies, fraud, and manipulation.
     * Positive actions: impersonation, forged documents, bribing officials, false testimony.
     * Negative actions: oath-keeping, honest dealings, exposing corruption.
     * Primarily drives diplomatic trust and NPC reaction severity.
     */
    Deception UMETA(DisplayName = "Deception"),

    /**
     * Compassion vs cruelty.
     * Positive actions: sparing defeated enemies, healing strangers, charity, freeing slaves.
     * Negative actions: torture, executing prisoners, leaving wounded to die, poisoning wells.
     * Affects how factions react to combat outcomes and civilian casualties.
     */
    Mercy UMETA(DisplayName = "Mercy"),

    /**
     * Allegiance, oaths, and group cohesion.
     * Positive actions: keeping oaths, protecting allies under fire, honoring contracts.
     * Negative actions: betrayal, desertion, selling information, backstabbing allies.
     * High faction Loyalty makes schisms harder to trigger; low Loyalty = factions fracture easily.
     */
    Loyalty UMETA(DisplayName = "Loyalty"),

    /**
     * Religious purity and the sacred.
     * Positive actions: blessing shrines, honoring the dead with proper rites, tithing.
     * Negative actions: grave robbing, defiling temples, cannibalism, blood rituals, necromancy.
     * Factions with high Sanctity strongly condemn desecration events.
     */
    Sanctity UMETA(DisplayName = "Sanctity"),

    /**
     * Power structures, law, and hierarchy.
     * Positive actions: paying fines, obeying guards, turning in bounties alive, completing faction orders.
     * Negative actions: assassinating leaders, freeing prisoners, inciting rebellion, tax evasion.
     * Authority > TaxAuthorityThreshold enables tax income for the faction in the economy sim.
     */
    Authority UMETA(DisplayName = "Authority"),

    /**
     * Use of supernatural/magical power (distinct from Sanctity — a healing spell is high Arcane AND high Sanctity).
     * Positive actions: casting spells, enchanting gear, brewing potions, summoning creatures.
     * Negative actions: destroying enchanted artifacts, silencing mages, anti-magic purges.
     * Drives diplomatic distance between magic-embracing and magic-fearing factions.
     */
    Arcane UMETA(DisplayName = "Arcane"),

    COUNT UMETA(Hidden)
};

static constexpr int32 MoralAxisCount = static_cast<int32>(EMythicMoralAxis::COUNT);

// ─────────────────────────────────────────────────────────────
// Resource Type — Economic resource categories
// ─────────────────────────────────────────────────────────────

/**
 * Aggregate resource categories for faction-level economic simulation.
 * Individual items map to these categories at the systems level.
 */
UENUM(BlueprintType)
enum class EMythicResourceType : uint8 {
    /** Sustains population. Produced by agricultural territory. */
    Food UMETA(DisplayName = "Food"),

    /** Raw materials (ore, wood, stone). Consumed by arms production. */
    Materials UMETA(DisplayName = "Materials"),

    /** Weapons, armor, equipment. Consumed by military upkeep. */
    Arms UMETA(DisplayName = "Arms"),

    /** Gold, trade goods. Accumulated from trade surplus and taxation. */
    Wealth UMETA(DisplayName = "Wealth"),

    COUNT UMETA(Hidden)
};

static constexpr int32 ResourceTypeCount = static_cast<int32>(EMythicResourceType::COUNT);

// ─────────────────────────────────────────────────────────────
// Resource Stock — Per-resource values with indexed access
// ─────────────────────────────────────────────────────────────

/**
 * Named resource values with indexed accessor for sim loops.
 * Same UHT-safe pattern as FMythicIdeologyProfile.
 */
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

    /** Indexed access for simulation loops. */
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

    /** Mutable indexed access for simulation writes. */
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

    /** Add another stock's values to this one. */
    FMythicResourceStock &operator+=(const FMythicResourceStock &Other) {
        Food += Other.Food;
        Materials += Other.Materials;
        Arms += Other.Arms;
        Wealth += Other.Wealth;
        return *this;
    }

    /** Subtract another stock's values from this one. */
    FMythicResourceStock &operator-=(const FMythicResourceStock &Other) {
        Food -= Other.Food;
        Materials -= Other.Materials;
        Arms -= Other.Arms;
        Wealth -= Other.Wealth;
        return *this;
    }

    /** Scale all values by a multiplier. */
    FMythicResourceStock &operator*=(float Scalar) {
        Food *= Scalar;
        Materials *= Scalar;
        Arms *= Scalar;
        Wealth *= Scalar;
        return *this;
    }

    /** Clamp all values to a range. */
    void ClampAll(float Min, float Max) {
        Food = FMath::Clamp(Food, Min, Max);
        Materials = FMath::Clamp(Materials, Min, Max);
        Arms = FMath::Clamp(Arms, Min, Max);
        Wealth = FMath::Clamp(Wealth, Min, Max);
    }
};

// ─────────────────────────────────────────────────────────────
// Pressure Channels — What stress an NPC accumulates
// ─────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────
// Venting Channels — How an NPC releases pressure
// ─────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────
// Significance Tiers — NPC complexity levels
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicSignificanceTier : uint8 {
    /** Pure visual crowd. No pressure, no personality, no cognition. Minimal cost. */
    Tier0_Ambient UMETA(DisplayName = "Ambient"),

    /** Hydrated — has pressure + personality fragments. Reacts to nearby events. */
    Tier1_Reactive UMETA(DisplayName = "Reactive"),

    /** Cognitive actor. Has BDI brain, queries causal fabric, generates schemes. */
    Tier2_Cognitive UMETA(DisplayName = "Cognitive"),

    /** Persistent cognitive NPC. Saved, has memories, crystallization. */
    Tier3_Persistent UMETA(DisplayName = "Persistent")
};

// ─────────────────────────────────────────────────────────────
// Moral Severity — Result of moral evaluation
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicMoralSeverity : uint8 {
    Ignore UMETA(DisplayName = "Ignore"),
    Disapprove UMETA(DisplayName = "Disapprove"),
    Condemn UMETA(DisplayName = "Condemn"),
    Hostile UMETA(DisplayName = "Hostile")
};
