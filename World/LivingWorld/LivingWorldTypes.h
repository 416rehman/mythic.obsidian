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

/** Moral axes used for moral signature evaluation. */
UENUM(BlueprintType)
enum class EMythicMoralAxis : uint8 {
    /** Physical harm. Positive: murder, assault, collateral damage. Negative: pacifism, non-lethal takedowns. */
    Violence UMETA(DisplayName = "Violence"),

    /** Taking what isn't yours. Positive: pickpocketing, burglary, looting homes. Negative: returning stolen goods. */
    Theft UMETA(DisplayName = "Theft"),

    /** Lies and manipulation. Positive: fraud, impersonation, forged documents. Negative: honest dealings, oath-keeping. */
    Deception UMETA(DisplayName = "Deception"),

    /** Compassion vs cruelty. Positive: sparing enemies, healing strangers, charity. Negative: torture, executing prisoners. */
    Mercy UMETA(DisplayName = "Mercy"),

    /** Allegiance and betrayal. Positive: keeping oaths, protecting allies. Negative: betrayal, desertion, selling out. */
    Loyalty UMETA(DisplayName = "Loyalty"),

    /** Purity of the sacred. Positive: blessing shrines, honoring the dead. Negative: grave robbing, defiling temples, cannibalism, blood rituals. */
    Sanctity UMETA(DisplayName = "Sanctity"),

    /** Power structures and rule of law. Positive: paying fines, turning in bounties alive, completing faction orders. Negative: assassinating leaders, freeing prisoners, inciting rebellion. */
    Authority UMETA(DisplayName = "Authority"),

    /** Use of supernatural power (distinct from Sanctity — a healing spell is high Arcane AND high Sanctity). Positive: casting spells, enchanting gear, brewing potions. Negative: destroying enchanted artifacts, silencing mages, anti-magic actions. */
    Arcane UMETA(DisplayName = "Arcane"),

    COUNT UMETA(Hidden)
};

static constexpr int32 MoralAxisCount = static_cast<int32>(EMythicMoralAxis::COUNT);

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
