
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "ArchetypeTypes.generated.h"

enum class EMythicSettlementEconomy : uint8;
enum class EMythicBiome : uint8;


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


struct FMythicArchetypeContext {
    float WealthNorm = 0.0f;

    float Military = 0.0f;

    EMythicSettlementEconomy Economy;

    EMythicBiome Biome;

    float DayFactor = 1.0f;

    FGameplayTag FactionTag;

    bool bWildernessContext = false;

    FMythicArchetypeContext();
};


UCLASS(BlueprintType)
class MYTHIC_API UMythicArchetypeCatalog : public UDataAsset {
    GENERATED_BODY()

public:
    /** All archetype rows in this catalog. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Archetypes")
    TArray<FMythicArchetypeRow> Archetypes;

    const FMythicArchetypeRow *FindByRole(const FGameplayTag &RoleTag) const {
        for (const FMythicArchetypeRow &Row : Archetypes) {
            if (Row.RoleTag.MatchesTagExact(RoleTag)) {
                return &Row;
            }
        }
        return nullptr;
    }
};


namespace MythicArchetypeDefaults {
    MYTHIC_API TConstArrayView<FMythicArchetypeRow> GetCodeDefaultArchetypes();
}
