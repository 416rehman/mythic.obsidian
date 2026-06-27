// Mythic Living World — NPC Appearance (deterministic wardrobe resolution)
// Pure, data-driven appearance resolution: from an NPC's stable seed (NameHash + demographics + role + faction) produce a
// fully-specified FMythicAppearance descriptor (which modular part goes in each slot, skin/hair tone, faction tint). The
// descriptor is replicated to clients by the NPC actor (Step 4) and handed to Blueprint/art via OnApplyAppearance — this
// file is the SYSTEM (resolution + library + code defaults); the actual modular meshes/skeletal-merge are ART.
//
// DETERMINISM CONTRACT: Resolve() is byte-identical for a given NameHash + inputs, with NO UObject calls and NO heap
// allocation beyond function-static defaults. Because every input is itself NameHash-derived server-side (and WealthTier
// comes from NameHash, not live economy), an NPC's look is stable across re-embodiment, pool reuse, save/load, and across
// all clients (clients never re-resolve — they receive the replicated descriptor).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AppearanceTypes.generated.h"

// ─────────────────────────────────────────────────────────────
// Appearance slot — a wardrobe attachment point
// ─────────────────────────────────────────────────────────────

/**
 * Modular wardrobe slots. FROZEN ORDER — this is a Blueprint + replication contract; never reorder existing entries,
 * only append before COUNT. The resolver writes a part index per slot; the BP merge maps each slot to a mesh socket.
 */
UENUM(BlueprintType)
enum class EMythicAppearanceSlot : uint8 {
    Head = 0,
    Hair,
    Torso,
    Legs,
    Feet,
    Hands,
    Back,
    COUNT UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────
// Appearance descriptor — the fully-resolved per-NPC look
// ─────────────────────────────────────────────────────────────

/**
 * The complete, resolved description of one NPC's look. REPLICATED as a member of AMythicNPCCharacter (Step 4). Uses
 * NAMED uint8 slot fields (NOT a C-array) for UHT + replication safety — a UPROPERTY C-array does not net-serialize
 * cleanly. PartForSlot() bridges the slot-indexed resolver loop to the named storage so the resolver can write by slot.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAppearance {
    GENERATED_BODY()

    /** Base body mesh variant (build/proportions). */
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 BodyType = 0;

    /** Per-slot part indices (which modular mesh option fills each EMythicAppearanceSlot). */
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 HeadPart = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 HairPart = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 TorsoPart = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 LegsPart = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 FeetPart = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 HandsPart = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 BackPart = 0;

    /** Index into the skin-tone palette. */
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 SkinTone = 0;

    /** Index into the hair-tone palette. */
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 HairTone = 0;

    /** Age bracket unpacked from DemographicFlags (0=child..4=elder). Drives child/adult/elder mesh selection in BP. */
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 AgeBracket = 0;

    /** The chosen outfit set (index into the resolved outfit-set list). Cosmetic grouping + debugger surface. */
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 OutfitSetId = 0;

    /** Female flag unpacked from DemographicFlags (bit 4). */
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    uint8 bIsFemale : 1;

    /** Primary faction tint (e.g. tabard / livery). Copied from the faction color. */
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    FColor PrimaryColor = FColor::White;

    /** Secondary faction tint (trim / accents). Derived from the primary. */
    UPROPERTY(BlueprintReadOnly, Category = "Appearance")
    FColor SecondaryColor = FColor::White;

    FMythicAppearance() : bIsFemale(0) {}

    /** Slot-indexed accessor over the named part fields, so the resolver can write/read by EMythicAppearanceSlot. */
    uint8& PartForSlot(EMythicAppearanceSlot Slot) {
        switch (Slot) {
        case EMythicAppearanceSlot::Head:  return HeadPart;
        case EMythicAppearanceSlot::Hair:  return HairPart;
        case EMythicAppearanceSlot::Torso: return TorsoPart;
        case EMythicAppearanceSlot::Legs:  return LegsPart;
        case EMythicAppearanceSlot::Feet:  return FeetPart;
        case EMythicAppearanceSlot::Hands: return HandsPart;
        case EMythicAppearanceSlot::Back:  return BackPart;
        default:                           return HeadPart;
        }
    }

    /** const overload of PartForSlot. */
    uint8 PartForSlot(EMythicAppearanceSlot Slot) const {
        return const_cast<FMythicAppearance*>(this)->PartForSlot(Slot);
    }

    /** Value compare for OnRep dirty checks (and tests). */
    bool operator==(const FMythicAppearance& O) const {
        return BodyType == O.BodyType && HeadPart == O.HeadPart && HairPart == O.HairPart && TorsoPart == O.TorsoPart &&
               LegsPart == O.LegsPart && FeetPart == O.FeetPart && HandsPart == O.HandsPart && BackPart == O.BackPart &&
               SkinTone == O.SkinTone && HairTone == O.HairTone && AgeBracket == O.AgeBracket &&
               OutfitSetId == O.OutfitSetId && bIsFemale == O.bIsFemale && PrimaryColor == O.PrimaryColor &&
               SecondaryColor == O.SecondaryColor;
    }
    bool operator!=(const FMythicAppearance& O) const { return !(*this == O); }
};

// ─────────────────────────────────────────────────────────────
// Outfit set — a wardrobe recipe gated by role + wealth
// ─────────────────────────────────────────────────────────────

/**
 * One authorable wardrobe recipe: how many modular options exist per slot (so the resolver can pick within range), gated
 * to a role family and a wealth-tier band. The resolver weighted-picks one eligible set per NPC, then picks a part within
 * each slot's count. PartCountPerSlot is indexed by EMythicAppearanceSlot; a missing/short entry is treated as 1 option.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicOutfitSet {
    GENERATED_BODY()

    /** Role family this set is for ("NPC.Role.*"). Empty = eligible for any role. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit", meta = (Categories = "NPC.Role"))
    FGameplayTag RoleTag;

    /** Inclusive wealth-tier band this set covers (0=destitute .. 3=wealthy). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit", meta = (ClampMin = "0", ClampMax = "3"))
    uint8 MinWealthTier = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit", meta = (ClampMin = "0", ClampMax = "3"))
    uint8 MaxWealthTier = 3;

    /** Number of modular options per slot, indexed by EMythicAppearanceSlot. Missing/short => 1 option for that slot. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit")
    TArray<uint8> PartCountPerSlot;

    /** Number of body-type variants this set supports. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit", meta = (ClampMin = "1"))
    uint8 BodyTypeCount = 1;

    /** Relative weight among eligible sets in the weighted pick. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outfit", meta = (ClampMin = "0.0"))
    float RelativeWeight = 1.0f;
};

// ─────────────────────────────────────────────────────────────
// Appearance library — designer-authored outfit catalog
// ─────────────────────────────────────────────────────────────

/**
 * Designer-authored catalog of outfit sets. Referenced from DA_LivingWorldSettings → AppearanceLibrary (Step 4). When
 * unset, the resolver falls back to MythicAppearanceDefaults::GetCodeDefaultOutfitSets() so wardrobe runs unauthored.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicAppearanceLibrary : public UDataAsset {
    GENERATED_BODY()

public:
    /** All outfit sets in this library. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
    TArray<FMythicOutfitSet> OutfitSets;

    /**
     * Find the best outfit set for a role + wealth tier: the highest-weight set whose RoleTag matches (or is empty) AND
     * whose wealth band contains WealthTier. Returns nullptr if none match. Mirrors the catalog FindBy* idiom.
     */
    const FMythicOutfitSet* FindBestFor(const FGameplayTag& RoleTag, uint8 WealthTier) const;
};

// ─────────────────────────────────────────────────────────────
// Code-default outfit sets + palettes
// ─────────────────────────────────────────────────────────────

/**
 * Built-in wardrobe so appearance runs with ZERO authored data. MUST include at least one always-eligible set (empty
 * RoleTag, wealth 0..3) so every NPC resolves to a valid look. All helpers are pure + thread-safe (function-static data
 * built once).
 */
namespace MythicAppearanceDefaults {
    /** Built-in outfit sets. View over a function-static TArray (built once). At least one is always eligible. */
    MYTHIC_API TConstArrayView<FMythicOutfitSet> GetCodeDefaultOutfitSets();

    /** Built-in skin-tone palette (non-empty). View over a function-static TArray. */
    MYTHIC_API TConstArrayView<FColor> GetCodeDefaultSkinTonePalette();

    /** Built-in hair-tone palette (non-empty). View over a function-static TArray. */
    MYTHIC_API TConstArrayView<FColor> GetCodeDefaultHairTonePalette();
}

// ─────────────────────────────────────────────────────────────
// Appearance resolver — the pure, deterministic core
// ─────────────────────────────────────────────────────────────

/**
 * The deterministic appearance resolver. Single static entry point — no state. Called server-side once per embodiment
 * (Step 4). Pure + unit-testable: identical inputs => identical FMythicAppearance, no UObject access, no allocation.
 */
struct MYTHIC_API FMythicAppearanceResolver {
    /**
     * Derive the STABLE wealth tier [0,3] for an NPC purely from its NameHash. NOT from live faction reserves — an NPC's
     * clothes must not change as the sim economy moves; appearance must be identical across re-embody / pool reuse /
     * save-load. Live Reserves.Wealth is reserved for the war-map economy signal, not wardrobe.
     */
    static uint8 WealthTierFromHash(uint32 NameHash);

    /**
     * Resolve the full appearance descriptor.
     *
     * @param NameHash         The NPC's stable identity seed.
     * @param DemographicFlags Packed [3b age][1b gender] (see FMythicNPCGenerator::GenerateDemographicFlags).
     * @param RoleTag          NPC role (gates outfit-set eligibility).
     * @param FactionIndex     Faction index (reserved for future per-faction wardrobe; currently mixed into nothing
     *                         beyond the colors the caller passes in — kept for signature stability).
     * @param WealthTier       Stable wealth tier [0,3] (use WealthTierFromHash).
     * @param FactionPrimary   Resolved faction primary color (from MythicFactionColor::GetFactionColor).
     * @param FactionSecondary Resolved faction secondary/accent color.
     * @param OutfitSets       Candidate outfit sets (authored library or code defaults).
     * @param SkinPalette      Skin-tone palette (authored or code default; resolver clamps to it).
     * @param HairPalette      Hair-tone palette (authored or code default).
     * @return Fully-resolved, deterministic appearance descriptor.
     */
    static FMythicAppearance Resolve(
        uint32 NameHash,
        uint8 DemographicFlags,
        const FGameplayTag& RoleTag,
        uint8 FactionIndex,
        uint8 WealthTier,
        const FColor& FactionPrimary,
        const FColor& FactionSecondary,
        TConstArrayView<FMythicOutfitSet> OutfitSets,
        TConstArrayView<FColor> SkinPalette,
        TConstArrayView<FColor> HairPalette);
};
