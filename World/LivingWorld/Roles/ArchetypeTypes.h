// Mythic Living World — Archetype catalog (sim-driven WHO spawns)
// An "archetype" is the data row that decides which ROLE an ambient NPC takes, weighted by the live world sim:
// faction wealth/military, the settlement's economy, the cell's biome, and time of day. The population spawner draws
// one archetype per NPC (deterministically, from the NPC's NameHash) and stamps its RoleTag onto the identity fragment.
//
// This is a strict DATA layer — zero behavior. It supersedes the hardcoded switch in DeriveRole with a weighted draw
// over an authored (or code-default) catalog, so designers can add/retune roles without code. The faction-requirement
// gate (UMythicRoleDatabase::RequiredFactionTags via ApplyFactionGate) stays authoritative on top of the draw.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "ArchetypeTypes.generated.h"

// Scoped-enum forward decls (fixed uint8 underlying type) — avoids pulling the Actor-bearing MythicSettlement.h and the
// territory grid header into every consumer of this data layer just for by-value weight indices. Real definitions live
// in Settlements/MythicSettlement.h (economy) and Territory/MythicBiome.h (biome).
enum class EMythicSettlementEconomy : uint8;
enum class EMythicBiome : uint8;

// ─────────────────────────────────────────────────────────────
// Archetype Row — one weighted spawn archetype
// ─────────────────────────────────────────────────────────────

/**
 * One ambient-NPC archetype. The population spawner computes an effective weight for each row from the live sim
 * context (FMythicArchetypeContext) and weighted-picks one per NPC. A row is authored in a DataTable (keyed by an
 * arbitrary RowName) or comes from MythicArchetypeDefaults::GetCodeDefaultArchetypes() when no catalog is assigned.
 *
 * All multipliers are >= 0; a zero in any active multiplier removes the row from this context entirely. The
 * EconomyWeights / BiomeWeights arrays are indexed by the respective enum's integer value and are IsValidIndex-guarded
 * so an empty (or short) array means "neutral" (weight 1.0) — designers only fill the indices they care about.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicArchetypeRow : public FTableRowBase {
    GENERATED_BODY()

    /** The role this archetype stamps onto FMythicIdentityFragment.RoleTag. Leaf name should preserve the substrings
     *  Guard/Soldier/Merchant where relevant so NPCGenerator::GeneratePersonality role modifiers still fire. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype", meta = (Categories = "NPC.Role"))
    FGameplayTag RoleTag;

    /** Human-readable label (debug/UI only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
    FName DisplayName;

    /** Base relative weight before any context multiplier. 0 = disabled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype", meta = (ClampMin = "0.0"))
    float BaseWeight = 1.0f;

    /** Multiplier applied in full at max wealth (lerped 1→WealthFavor by normalized wealth). >1 favors rich towns. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Economy", meta = (ClampMin = "0.0"))
    float WealthFavor = 1.0f;

    /** Multiplier applied in full at min wealth (lerped 1→WealthDisfavor by inverse normalized wealth). >1 favors poor
     *  towns (beggars, laborers). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Economy", meta = (ClampMin = "0.0"))
    float WealthDisfavor = 1.0f;

    /** Multiplier applied in full at max military strength (lerped 1→MilitaryFavor). >1 favors militarized factions
     *  (soldiers, guards). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Economy", meta = (ClampMin = "0.0"))
    float MilitaryFavor = 1.0f;

    /** Per-economy multiplier, indexed by EMythicSettlementEconomy. Empty/short = neutral (1.0). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Economy", meta = (ClampMin = "0.0"))
    TArray<float> EconomyWeights;

    /** Per-biome multiplier, indexed by EMythicBiome. Empty/short = neutral (1.0). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Biome", meta = (ClampMin = "0.0"))
    TArray<float> BiomeWeights;

    /** Multiplier applied in full at midday (lerped NightWeight→DayWeight by DayFactor). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Schedule", meta = (ClampMin = "0.0"))
    float DayWeight = 1.0f;

    /** Multiplier applied in full at midnight (lerped NightWeight→DayWeight by DayFactor). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Schedule", meta = (ClampMin = "0.0"))
    float NightWeight = 1.0f;

    /** If false, this archetype never appears as a lone ambient NPC — it only spawns as part of a group (Step 4).
     *  The ambient single-spawn draw skips these rows so a lone noble never appears without his retinue. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Group")
    bool bAllowedAlone = true;

    /** Minimum members when this archetype anchors a group (consumed by the group system, Step 4). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Group", meta = (ClampMin = "1"))
    uint8 MinGroupSize = 1;

    /** Maximum members when this archetype anchors a group (consumed by the group system, Step 4). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Group", meta = (ClampMin = "1"))
    uint8 MaxGroupSize = 1;

    /** Optional location-kind gate (consumed by spawn-point selection, Step 3). Empty = any. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Location")
    FGameplayTagContainer AllowedLocationTags;

    /** If true, this archetype only spawns inside a settlement (the wilderness/patrol path skips it). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Location")
    bool bRequiresSettlement = false;

    /** Faction-tag requirement (HasAny). Empty = any faction. A pre-filter mirroring the role-DB gate so a faction that
     *  can't field this archetype never even draws it. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Requirements")
    FGameplayTagContainer RequiredFactionTags;

    /** If true, this archetype may be placed in/near water (forwarded into FMythicPlacementParams by Step 3/4). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Location")
    bool bWaterCapable = false;
};

// ─────────────────────────────────────────────────────────────
// Archetype Context — the live sim snapshot a draw is weighted against
// ─────────────────────────────────────────────────────────────

/**
 * The per-spawn sim context the weighted draw consumes. Plain C++ (NOT a USTRUCT) — it is a transient, game-thread
 * value bundle assembled from already-locked snapshot copies (faction data, settlement, lock-free grid reads), never
 * serialized or exposed to Blueprint. DayFactor is passed in (never read from a wall clock inside DeriveArchetype) so
 * the draw is a pure function and unit tests can pin it.
 */
struct FMythicArchetypeContext {
    /** Normalized faction wealth in [0,1] (Reserves.Wealth / MaxReserve, clamped — wealth can be negative). */
    float WealthNorm = 0.0f;

    /** Faction MilitaryStrength in [0,1]. */
    float Military = 0.0f;

    /** Resolved settlement economy for this cell. */
    EMythicSettlementEconomy Economy;

    /** Biome at the candidate cell. */
    EMythicBiome Biome;

    /** Time-of-day factor in [0,1]: 1=midday, 0=midnight (cosine of game hour). */
    float DayFactor = 1.0f;

    /** Spawning faction's tag (for RequiredFactionTags pre-filter). */
    FGameplayTag FactionTag;

    /** True when drawing for a wilderness/patrol context (no settlement). Rows that require a settlement or disallow
     *  lone spawns are excluded. */
    bool bWildernessContext = false;

    FMythicArchetypeContext();
};

// ─────────────────────────────────────────────────────────────
// Archetype Catalog — designer-authored data asset
// ─────────────────────────────────────────────────────────────

/**
 * Designer-authored catalog of archetype rows. Referenced from DA_LivingWorldSettings → ArchetypeCatalog. When unset,
 * the population spawner falls back to MythicArchetypeDefaults::GetCodeDefaultArchetypes() so the system runs unauthored.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicArchetypeCatalog : public UDataAsset {
    GENERATED_BODY()

public:
    /** All archetype rows in this catalog. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Archetypes")
    TArray<FMythicArchetypeRow> Archetypes;

    /** Find an archetype row by its role tag (exact match). Returns nullptr if not present. Mirrors
     *  UMythicRoleDatabase::FindRole. */
    const FMythicArchetypeRow *FindByRole(const FGameplayTag &RoleTag) const {
        for (const FMythicArchetypeRow &Row : Archetypes) {
            if (Row.RoleTag.MatchesTagExact(RoleTag)) {
                return &Row;
            }
        }
        return nullptr;
    }
};

// ─────────────────────────────────────────────────────────────
// Code-default archetype set
// ─────────────────────────────────────────────────────────────

/**
 * Built-in archetype set so role-from-context runs with ZERO authored data. Covers civilian/farmer/merchant/laborer/
 * fisher/guard/soldier/beggar/socialite/noble/traveler. GUARANTEES an always-eligible all-neutral Civilian row (alone-
 * allowed, no faction requirement, no settlement requirement) so the total weight is > 0 for EVERY context — the draw
 * can never come up empty. Used whenever UMythicLivingWorldSettings::ArchetypeCatalog is unset.
 */
namespace MythicArchetypeDefaults {
    /** Returns a view over a static, immutable array of default archetype rows (safe to call from any thread). */
    MYTHIC_API TConstArrayView<FMythicArchetypeRow> GetCodeDefaultArchetypes();
}
