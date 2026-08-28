#pragma once

#include "CoreMinimal.h"
#include "Itemization/MythicDataAsset.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "MythicAffixProfile.generated.h"

/** Selects whether a grant rolls an eligible affix tier or requires one exact authored tier rank. */
UENUM(BlueprintType)
enum class EMythicAffixGrantTierMode : uint8 {
    /** Randomly selects among tiers whose minimum item level and budget are eligible. */
    WeightedEligible,

    /** Selects the authored rank regardless of item level; the magnitude budget still applies. */
    ExactTier
};

namespace MythicAffixGrant {
/** Returns true only for a canonical semantic tier selection; exact grants use a one-based tier rank. */
FORCEINLINE bool IsTierSelectionValid(const EMythicAffixGrantTierMode Mode, const int32 ExactTierRank) {
    switch (Mode) {
    case EMythicAffixGrantTierMode::WeightedEligible:
        return ExactTierRank == 0;
    case EMythicAffixGrantTierMode::ExactTier:
        return ExactTierRank > 0;
    default:
        return false;
    }
}

}

/** Guaranteed affix grant that selects either an eligible tier or one exact authored tier rank. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixGrantSpec {
    GENERATED_BODY()

    /** Tool-authored stable identity used to derive deterministic rolls and record grant provenance. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid GrantGuid;

    /** Internal, non-localized grant label used in authoring, validation, and diagnostics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DeveloperName;

    /** Canonical Affix Definition guaranteed by this grant; that asset owns all selectable tiers. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FMythicAffixDefinitionHandle AffixDefinition;

    /** Chooses an item-level-eligible random tier or an explicit rank that overrides item level. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EMythicAffixGrantTierMode TierMode = EMythicAffixGrantTierMode::WeightedEligible;

    /** One-based rank used only for an exact grant; it overrides random-roll item-level availability. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (EditCondition = "TierMode == EMythicAffixGrantTierMode::ExactTier", EditConditionHides,
                      ClampMin = "1", DisplayName = "Exact Tier Rank"))
    int32 ExactTierRank = 0;

    /** Roll group recorded in provenance for traces and analytics; guaranteed grants do not consume random-roll caps. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.Affix.RollGroup"))
    FGameplayTag RollGroup;

    /** Provenance kind used to distinguish implicit, random, crafted, gem, and other affix sources. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.Affix.Source"))
    FGameplayTag SourceKind;

    /** When true, crafting operations cannot reroll or replace the generated affix. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bLocked = true;
};

/** Priority-selected, context-gated reference to one reusable affix pool. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixPoolSlice {
    GENERATED_BODY()

    /** Tool-authored stable identity used to derive deterministic selection and record slice provenance. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid SliceGuid;

    /** Internal, non-localized slice label used in authoring, validation, and diagnostics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DeveloperName;

    /** Candidate pool contributed by this slice. A profile may include each pool at most once. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FMythicAffixPoolHandle Pool;

    /** Provenance kind assigned to every random affix selected through this slice. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.Affix.Source"))
    FGameplayTag SourceKind;

    /** Eligibility tier for slice selection; only the highest eligible priority participates in a roll. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Priority = 0;

    /** Integer relative weight among eligible slices at the winning priority. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1")) int32 SliceWeight = 1;
};

/** Primary Data Asset defining one item family's roll policy, guaranteed grants, and contextual pool slices. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicAffixProfile : public UMythicDataAsset {
    GENERATED_BODY()
public:
    /** Internal, non-localized label used to find this profile in authoring and diagnostics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DeveloperName;

    /** Explains the item family and generation contract represented by this profile; never shown to players. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FString DesignerPurpose;

    /** Gameplay-semantic revision stored in every generated affix's provenance. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Revision = 1;

    /** Stable primary-asset identity selected by affix-bearing item definitions. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.AffixProfile"))
    FGameplayTag ProfileTag;

    /** Generation rules that determine rarity roll counts, roll-group caps, budgets, and conflict behavior. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FMythicAffixRollPolicyHandle RollPolicy;

    /** Affixes always generated before random pool rolls for every use of this profile. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMythicAffixGrantSpec> GuaranteedGrants;

    /** Prioritized, weighted pool sources used to satisfy the policy's random-roll count. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMythicAffixPoolSlice> RandomPoolSlices;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
