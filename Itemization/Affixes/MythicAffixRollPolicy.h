#pragma once

#include "CoreMinimal.h"
#include "Itemization/MythicDataAsset.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "MythicAffixRollPolicy.generated.h"

/** Controls whether an affix roll may ship below its requested count when the eligible pool is exhausted. */
UENUM(BlueprintType)
enum class EMythicAffixShortfallMode : uint8 { AllowPartial, FailGeneration };

/** Maximum random-affix count and shared magnitude budget for one semantic roll group. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixRollGroupBudget {
    GENERATED_BODY()

    /** Affix roll group whose selections share this cap; this is not a Stat Modifier Key. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (Categories = "Itemization.Affix.RollGroup", DisplayName = "Roll Group"))
    FGameplayTag RollGroup;

    /** Maximum random affixes that may be selected from this roll group on one item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) int32 MaxRolls = 0;
};

/** Complete random-roll and magnitude budgets applied to one item rarity. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixRarityBudget {
    GENERATED_BODY()

    /** Item rarity to which this complete roll and magnitude budget applies. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TEnumAsByte<EItemRarity> Rarity = EItemRarity::Common;

    /** Number of random pool affixes generation attempts to land after guaranteed grants. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) int32 RandomRollCount = 0;

    /** Per-group caps that partition the random rolls into offensive, defensive, utility, or other buckets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Roll Group Caps"))
    TArray<FMythicAffixRollGroupBudget> RollGroupBudgets;

    /** When true, eligible tiers ignore Magnitude Budget while still respecting all roll-group caps. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bUnlimitedMagnitudeBudget = true;

    /** Total tier Budget Cost available across random rolls when the magnitude budget is limited. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0")) float MagnitudeBudget = 0.0f;
};

/** Primary Data Asset owning deterministic affix-generation budgets and duplicate/shortfall policy. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicAffixRollPolicy : public UMythicDataAsset {
    GENERATED_BODY()
public:
    /** Internal, non-localized label used to find this policy in authoring and diagnostics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DeveloperName;

    /** Explains the loot-echelon and economy role of this generation policy; never shown to players. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FString DesignerPurpose;

    /** Gameplay-semantic revision stored in every generated affix's provenance. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Revision = 1;

    /** Stable primary-asset identity referenced by affix profiles. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.AffixRollPolicy"))
    FGameplayTag PolicyTag;

    /** Deterministic generation implementation version; changing it requires explicit save/seed compatibility. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1")) int32 AlgorithmVersion = 1;

    /** Complete random-roll, roll-group, and magnitude budgets authored once for each supported rarity. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMythicAffixRarityBudget> BudgetsByRarity;

    /** When enabled, guaranteed grants spend tier Budget Cost from the rarity's shared magnitude budget. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bGuaranteedConsumesMagnitudeBudget = false;

    /** Prevents two rolled entries that resolve to the same canonical affix definition on one item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDisallowDuplicateAffixDefinition = true;

    /** Prevents separate affixes from modifying the same canonical stat on one item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDisallowDuplicateTargetStat = true;

    /**
     * Chooses whether generation accepts fewer random affixes or fails when the full count cannot be satisfied.
     * Fail Generation requires a context-independent pool backbone because profiles do not author a finite set of
     * supported item-context tags; conditional rows may enrich that backbone but cannot be its only support.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EMythicAffixShortfallMode ShortfallMode = EMythicAffixShortfallMode::AllowPartial;

    /** Allows selection to descend to a lower-priority pool slice when every higher-priority slice is exhausted. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bAllowLowerPriorityFallback = false;

    const FMythicAffixRarityBudget *FindBudget(EItemRarity Rarity) const;
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
