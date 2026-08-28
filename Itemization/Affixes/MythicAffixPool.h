#pragma once

#include "CoreMinimal.h"
#include "Itemization/MythicDataAsset.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "MythicAffixPool.generated.h"

/** Weighted direct reference to one canonical affix definition within a roll group and eligibility context. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixPoolEntry {
    GENERATED_BODY()

    /** Tool-authored stable identity for deterministic traces, provenance, and row-level revisions. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGuid PoolRowGuid;

    /** Internal, non-localized row label used in authoring, validation, and diagnostics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DeveloperName;

    /** Gameplay-semantic revision included in compiled content hashes and telemetry. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 RowRevision = 1;

    /** Canonical Affix Definition this row can roll; that asset owns all stat bindings and tiers. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FMythicAffixDefinitionHandle AffixDefinition;

    /** Roll group charged when this row is selected, such as an offensive or defensive slot. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.Affix.RollGroup"))
    FGameplayTag RollGroup;

    /** Relative deterministic-selection weight among eligible rows in the selected pool slice. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.000001")) float SelectionWeight = 1.0f;

    /** Optional query against the item-generation context; an empty query leaves the row universally eligible. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagQuery EligibilityQuery;
};

/** Primary Data Asset containing reusable weighted affix-definition entries for deterministic generation. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicAffixPool : public UMythicDataAsset {
    GENERATED_BODY()
public:
    /** Internal, non-localized label used to find this pool in authoring and diagnostics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DeveloperName;

    /** Explains the intended item family and roll-space responsibility; never shown to players. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FString DesignerPurpose;

    /** Gameplay-semantic revision used to invalidate compiled pool data after tuning changes. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Revision = 1;

    /** Stable primary-asset identity for this affix pool. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.AffixPool")) FGameplayTag PoolTag;

    /** Weighted, context-gated Affix Definition candidates available through this pool. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMythicAffixPoolEntry> Entries;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
