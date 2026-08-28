// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicDataAsset.h"
#include "Stats/MythicStatTypes.h"
#include "MythicStatDefinition.generated.h"

/** Canonical Primary Data Asset binding one player-facing stat to its GAS attribute and presentation rules. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicStatDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    /** Stable, non-localized name used by designers, validation logs, and content tooling. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FName DeveloperName;

    /** Internal explanation of what the stat represents and where it is expected to be used. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
    FString DesignerPurpose;

    /** Version of gameplay-relevant stat semantics, incremented when those semantics change. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Identity")
    int32 Revision = 1;

    /** Version of player-facing stat presentation, incremented when display data changes. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Identity")
    int32 PresentationRevision = 1;

    /** Canonical gameplay tag identity used by itemization, UI, telemetry, and save snapshots. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat", meta = (Categories = "Stat.Attribute"))
    FGameplayTag StatTag;

    /** Gameplay Ability System attribute that stores the live value of this stat. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    FGameplayAttribute Attribute;

    /** Localized player-facing name used on stat sheets, affixes, and tooltips. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    FText DisplayName;

    /** Localized explanation of the stat's gameplay effect. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (MultiLine = true))
    FText Description;

    /** Category definition that controls this stat's grouping, order, and category-level presentation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    FMythicStatCategoryDefinitionHandle Category;

    /** Ascending order of this stat within its category on the stat sheet. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    int32 SheetOrder = 0;

    /** Canonical rules for formatting this stat and affix values that target it. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    FMythicStatNumberPresentation NumberPresentation;

    /** Defines whether increases, decreases, or neither direction should be presented as beneficial. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    EMythicStatComparisonDirection ComparisonDirection = EMythicStatComparisonDirection::HigherIsBetter;

    /** Baseline value treated as unmodified for visibility and comparison presentation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    float NeutralValue = 0.0f;

    /** Rule controlling whether this stat is included in the data-driven stat sheet. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    EMythicStatSheetVisibility SheetVisibility = EMythicStatSheetVisibility::Always;

    /** Optional role used to render a current value and capacity definition as one paired stat row. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    EMythicStatPairRole PairRole = EMythicStatPairRole::None;

    /** Other stat definition in this current/capacity pair; both definitions must reference each other. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat",
              meta = (EditCondition = "PairRole != EMythicStatPairRole::None", EditConditionHides))
    FMythicStatDefinitionHandle PairedStat;

    /** Allows affix definitions to target this stat; disable for runtime-only or unsafe attributes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    bool bCanBeAffixTarget = false;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    EMythicStatFormat GetNumberFormat() const {
        return NumberPresentation.Format;
    }

    /** Runtime-safe local validation used by both editor validation and the compiled registry. */
    bool AppendValidationErrors(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
