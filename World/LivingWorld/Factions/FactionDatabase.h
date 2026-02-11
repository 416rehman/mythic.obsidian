// Mythic Living World System — Faction Database
// Cached faction data snapshot. Background thread computes, game thread reads lock-free.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "FactionDatabase.generated.h"

// ─────────────────────────────────────────────────────────────
// Faction Relationship — Pairwise faction stance
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicFactionRelation : uint8 {
    Allied UMETA(DisplayName = "Allied"),
    Friendly UMETA(DisplayName = "Friendly"),
    Neutral UMETA(DisplayName = "Neutral"),
    Unfriendly UMETA(DisplayName = "Unfriendly"),
    Hostile UMETA(DisplayName = "Hostile")
};

// ─────────────────────────────────────────────────────────────
// Ideology Profile — Per-axis moral stance (designer-facing)
// ─────────────────────────────────────────────────────────────

/**
 * How strongly a faction feels about each moral axis.
 * All values range from -1.0 to +1.0.
 *   Positive = faction approves of this behavior.
 *   Negative = faction condemns this behavior.
 *   Zero = faction is indifferent.
 *
 * Used in dot-product evaluation against player/NPC moral signatures.
 * The result is checked against faction thresholds to determine reaction severity.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicIdeologyProfile {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Violence = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Theft = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Deception = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Mercy = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Loyalty = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Sanctity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Authority = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Arcane = 0.0f;

    /** Indexed access for dot-product evaluation loops. */
    float GetAxis(EMythicMoralAxis Axis) const {
        switch (Axis) {
        case EMythicMoralAxis::Violence:
            return Violence;
        case EMythicMoralAxis::Theft:
            return Theft;
        case EMythicMoralAxis::Deception:
            return Deception;
        case EMythicMoralAxis::Mercy:
            return Mercy;
        case EMythicMoralAxis::Loyalty:
            return Loyalty;
        case EMythicMoralAxis::Sanctity:
            return Sanctity;
        case EMythicMoralAxis::Authority:
            return Authority;
        case EMythicMoralAxis::Arcane:
            return Arcane;
        default:
            return 0.0f;
        }
    }

    /** Indexed access for simulation writes. */
    float &GetAxisMutable(EMythicMoralAxis Axis) {
        switch (Axis) {
        case EMythicMoralAxis::Violence:
            return Violence;
        case EMythicMoralAxis::Theft:
            return Theft;
        case EMythicMoralAxis::Deception:
            return Deception;
        case EMythicMoralAxis::Mercy:
            return Mercy;
        case EMythicMoralAxis::Loyalty:
            return Loyalty;
        case EMythicMoralAxis::Sanctity:
            return Sanctity;
        case EMythicMoralAxis::Authority:
            return Authority;
        case EMythicMoralAxis::Arcane:
            return Arcane;
        default:
            static float Dummy = 0.0f;
            return Dummy;
        }
    }
};

// ─────────────────────────────────────────────────────────────
// Faction Data — Runtime state for one faction
// ─────────────────────────────────────────────────────────────

/**
 * Runtime data for a single faction. Maintained by the background simulation thread.
 * Game thread reads a snapshot copy.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicFactionData {
    GENERATED_BODY()

    /** Faction display name */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    /** Gameplay tag identifying this faction (e.g. "Faction.Imperials") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag FactionTag;

    /** Is this faction still alive? False = annihilated */
    UPROPERTY(BlueprintReadOnly)
    bool bAlive = true;

    /**
     * This faction's moral stance on each axis.
     * Compared against player/NPC moral signatures to determine reaction severity.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology")
    FMythicIdeologyProfile Ideology;

    /**
     * How severe must an action be before the faction disapproves?
     * Actions scoring above this threshold trigger mild reputation loss, suspicious NPC dialogue, etc.
     * Lower values = faction is more easily offended. Range: 0.0 - 1.0.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0",
        Tooltip = "Severity threshold for mild disapproval (suspicious dialogue, minor rep loss). Lower = more easily offended."))
    float DisapproveThreshold = 0.2f;

    /**
     * Actions scoring above this threshold are considered criminal by this faction.
     * Triggers bounty, guard alerts, faction reputation penalties.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0",
        Tooltip = "Severity threshold for criminal condemnation (bounties, guard alerts, major rep loss)."))
    float CondemnThreshold = 0.5f;

    /**
     * Actions scoring above this threshold provoke an immediate hostile response.
     * Faction members will attack on sight, declare war, or escalate to lethal force.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0",
        Tooltip = "Severity threshold for hostile response (attack on sight, war declaration, lethal force)."))
    float HostileThreshold = 0.8f;

    // ─── Economy ──────────────────────────────────────────

    /** Aggregate economic strength (0.0 - 1.0). Affects merchant inventory, pricing, military funding. */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    float EconomicStrength = 0.5f;

    /** Military strength (0.0 - 1.0). Affects guard deployment, response capability. */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    float MilitaryStrength = 0.5f;

    // ─── Population ───────────────────────────────────────

    /** Total population count for background simulation */
    UPROPERTY(BlueprintReadOnly, Category = "Population")
    int32 Population = 0;

    /** Index of the current leader NPC entity (0 = no leader) */
    UPROPERTY(BlueprintReadOnly, Category = "Population")
    int32 LeaderEntityId = 0;

    // ─── Territory ────────────────────────────────────────

    /** Number of cells this faction currently controls */
    UPROPERTY(BlueprintReadOnly, Category = "Territory")
    int32 ControlledCellCount = 0;
};

// ─────────────────────────────────────────────────────────────
// Faction Database Settings — Data asset
// ─────────────────────────────────────────────────────────────

UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicFactionDatabaseSettings : public UDataAsset {
    GENERATED_BODY()

public:
    /** Maximum number of factions the system supports. Drives array sizing. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "2", ClampMax = "64"))
    int32 MaxFactions = 20;

    /** Initial faction definitions. Loaded at startup. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<FMythicFactionData> InitialFactions;
};

// ─────────────────────────────────────────────────────────────
// Faction Database — Double-buffered faction state
// ─────────────────────────────────────────────────────────────

/**
 * Central faction database. Background thread maintains the write copy,
 * game thread reads from the committed snapshot.
 *
 * Faction relationships are stored as a flat array (NxN matrix, packed upper triangle).
 */
UCLASS()
class MYTHIC_API UMythicFactionDatabase : public UObject {
    GENERATED_BODY()

public:
    /** Initialize from settings. Must be called once before use. */
    void Initialize(const UMythicFactionDatabaseSettings *Settings);

    // ─── Write Interface (Background Thread Only) ─────────

    /** Get mutable reference to a faction's data in the write buffer */
    FMythicFactionData *GetFactionMutable(FMythicFactionId Id);

    /** Set the relationship between two factions */
    void SetRelationship(FMythicFactionId A, FMythicFactionId B, EMythicFactionRelation Relation);

    /**
     * Register a new faction at runtime (faction creation from schism/conquest).
     * Returns the assigned FactionId, or invalid if the cap is reached.
     */
    FMythicFactionId RegisterFaction(const FMythicFactionData &Data);

    /** Mark a faction as annihilated */
    void AnnihilateFaction(FMythicFactionId Id);

    /** Commit the write buffer — game thread snapshot swap */
    void CommitWrites();

    // ─── Read Interface (Game Thread — Lock-Free) ─────────

    /** Get faction data by ID (read snapshot). Returns nullptr if ID is invalid. */
    const FMythicFactionData *GetFaction(FMythicFactionId Id) const;

    /** Get faction by gameplay tag. Linear scan — use sparingly, cache the ID. */
    const FMythicFactionData *FindFactionByTag(const FGameplayTag &Tag, FMythicFactionId *OutId = nullptr) const;

    /** Get the relationship between two factions */
    EMythicFactionRelation GetRelationship(FMythicFactionId A, FMythicFactionId B) const;

    /** Get the number of active (alive) factions */
    int32 GetActiveFactionCount() const;

    /** Get the maximum faction capacity */
    int32 GetMaxFactions() const { return MaxFactions; }

    /** Iterate all alive factions */
    void ForEachAliveFaction(TFunctionRef<void(FMythicFactionId, const FMythicFactionData &)> Callback) const;

private:
    int32 MaxFactions = 0;

    /** Write buffer — background thread only */
    TArray<FMythicFactionData> WriteFactions;

    /** Read buffer — game thread snapshot */
    TArray<FMythicFactionData> ReadFactions;

    /** Relationship matrix — flat array, indexed by RelationIndex(A, B) */
    TArray<EMythicFactionRelation> WriteRelationships;
    TArray<EMythicFactionRelation> ReadRelationships;

    /** Number of factions currently registered */
    int32 RegisteredCount = 0;

    /** Flatten two faction IDs into a relationship matrix index (upper triangle) */
    int32 RelationIndex(FMythicFactionId A, FMythicFactionId B) const {
        const int32 Low = FMath::Min(A.Index, B.Index);
        const int32 High = FMath::Max(A.Index, B.Index);
        return Low * MaxFactions + High;
    }
};
