
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AppearanceTypes.generated.h"


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

    uint8 PartForSlot(EMythicAppearanceSlot Slot) const {
        return const_cast<FMythicAppearance*>(this)->PartForSlot(Slot);
    }

    bool operator==(const FMythicAppearance& O) const {
        return BodyType == O.BodyType && HeadPart == O.HeadPart && HairPart == O.HairPart && TorsoPart == O.TorsoPart &&
               LegsPart == O.LegsPart && FeetPart == O.FeetPart && HandsPart == O.HandsPart && BackPart == O.BackPart &&
               SkinTone == O.SkinTone && HairTone == O.HairTone && AgeBracket == O.AgeBracket &&
               OutfitSetId == O.OutfitSetId && bIsFemale == O.bIsFemale && PrimaryColor == O.PrimaryColor &&
               SecondaryColor == O.SecondaryColor;
    }
    bool operator!=(const FMythicAppearance& O) const { return !(*this == O); }
};


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


UCLASS(BlueprintType)
class MYTHIC_API UMythicAppearanceLibrary : public UDataAsset {
    GENERATED_BODY()

public:
    /** All outfit sets in this library. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
    TArray<FMythicOutfitSet> OutfitSets;

    const FMythicOutfitSet* FindBestFor(const FGameplayTag& RoleTag, uint8 WealthTier) const;
};


namespace MythicAppearanceDefaults {
    MYTHIC_API TConstArrayView<FMythicOutfitSet> GetCodeDefaultOutfitSets();

    MYTHIC_API TConstArrayView<FColor> GetCodeDefaultSkinTonePalette();

    MYTHIC_API TConstArrayView<FColor> GetCodeDefaultHairTonePalette();
}


struct MYTHIC_API FMythicAppearanceResolver {
    static uint8 WealthTierFromHash(uint32 NameHash);

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
