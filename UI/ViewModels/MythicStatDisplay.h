// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "MythicStatDisplay.generated.h"

class UMythicStatSummaryLibrary;
class UTexture2D;

UENUM(BlueprintType)
enum class EMythicStatFormat : uint8 {
    Flat,
    Integer,
    Percent,
    Multiplier,
    PerSecond,
    Bipolar
};

UENUM(BlueprintType)
enum class EMythicStatCategory : uint8 {
    Vitality,
    Offense,
    Defense,
    Utility,
    Proficiency,
    Survival,
    Hidden,

    /**
     * A stat the player invests in that derives others, rather than one derived from something else.
     *
     * Appended rather than inserted at the top: the values are serialised in config and assets, so
     * renumbering would silently repoint existing rows. The panel orders tiers explicitly instead.
     */
    Primary
};

/**
 * One row of the stat display registry, authored as data. The row NAME is the attribute name; everything
 * the sheet needs to place and format that attribute lives in the row. This table is the single source -
 * moving a stat between sections or renaming its label is a cell edit, never a code change.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatDisplayRow : public FTableRowBase {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat Display")
    FString Label;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat Display")
    EMythicStatCategory Category = EMythicStatCategory::Hidden;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat Display")
    EMythicStatFormat Format = EMythicStatFormat::Flat;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat Display")
    int32 SortOrder = 0;

    // The paired capacity attribute for "current / max" rows, empty for plain values.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat Display")
    FString MaxAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat Display")
    bool bHidden = false;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatLine {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Label;

    // Fully formatted, unit-correct. "5%", "+25%", "3.0/s", "42".
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Value;

    // "+12" / "-3" — what gear, talents and buffs are adding on top of the base right now. Empty when there is no
    // difference, so an unmodified stat stays visually quiet.
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText BonusText;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float BaseValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float CurrentValue = 0.0f;

    // CurrentValue - BaseValue. Positive is an improvement for every attribute except IncomingDamageMultiplier.
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float BonusValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bHasBonus = false;

    // 0..1 for current/max pairs so the view can draw a bar. -1 when this stat is not a pair.
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float BarPercent = -1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    EMythicStatCategory Category = EMythicStatCategory::Utility;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    EMythicStatFormat Format = EMythicStatFormat::Flat;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    int32 SortOrder = 1000;

    // Which attribute this line shows, so the view can ask follow-up questions (contribution tooltips) about it.
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayAttribute Attribute;
};

/** One headline card: a summary's display-ready state, computed by its authored calculation. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatSummaryLine {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayTag SummaryId;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Label;

    // Preformatted through the same formatter as every other stat on the sheet.
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Value;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Description;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float RawValue = 0.0f;
};

USTRUCT()
struct MYTHIC_API FMythicStatRule {
    GENERATED_BODY()

    // Matches FGameplayAttribute::GetName(), i.e. the C++ property name.
    UPROPERTY(EditAnywhere, Config, Category = "Stat")
    FString Attribute;

    // Left empty to fall back to MakeFriendlyLabel(Attribute).
    UPROPERTY(EditAnywhere, Config, Category = "Stat")
    FString Label;

    UPROPERTY(EditAnywhere, Config, Category = "Stat")
    EMythicStatCategory Category = EMythicStatCategory::Utility;

    UPROPERTY(EditAnywhere, Config, Category = "Stat")
    EMythicStatFormat Format = EMythicStatFormat::Flat;

    // Ascending within a category. Sparse (10, 20, 30…) so rules can be inserted between without renumbering.
    UPROPERTY(EditAnywhere, Config, Category = "Stat")
    int32 SortOrder = 1000;

    // Property name of this attribute's "max" partner, for current/max pairs. The pair renders as one row with a bar
    // and the max attribute is suppressed as its own row. Naming is inconsistent across the sets (MaxHealth is a
    // prefix, CombatProficiencyMax is a suffix), which is exactly why this is explicit rather than inferred.
    UPROPERTY(EditAnywhere, Config, Category = "Stat")
    FString MaxAttribute;

    UPROPERTY(EditAnywhere, Config, Category = "Stat")
    bool bHidden = false;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic Stat Display"))
class MYTHIC_API UMythicStatDisplaySettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override {
        return FName(TEXT("Game"));
    }

    // Per-attribute overrides. Empty by default — the C++ table already covers every shipped attribute.
    UPROPERTY(EditAnywhere, Config, Category = "Stat Display")
    TArray<FMythicStatRule> Overrides;

    // Attributes whose live value equals their base and whose base is 0 are dropped from the sheet. Keeps ~30 inert
    // rows (unrolled magic find, unused weapon-family bonuses) out of the player's face until something grants them.
    UPROPERTY(EditAnywhere, Config, Category = "Stat Display")
    bool bHideUnmodifiedZeroStats = true;

    // The headline summaries the sheet shows, in display order. Unset means no cards — the section collapses.
    UPROPERTY(EditAnywhere, Config, Category = "Stat Display")
    TSoftObjectPtr<UMythicStatSummaryLibrary> SummaryLibrary;

    /**
     * The stat display registry as data (rows of FMythicStatDisplayRow, row name = attribute name). When
     * authored this is the single source; the compiled-in list is only the fallback for a project without
     * the table, and its use is logged so it can never be mistaken for the real thing.
     */
    UPROPERTY(config, EditAnywhere, Category = "Stats", meta = (RequiredAssetDataTags = "RowStructure=/Script/Mythic.MythicStatDisplayRow"))
    TSoftObjectPtr<UDataTable> DisplayTable;
};

/** One line of a primary stat's tooltip: what it feeds, and how much it is feeding it right now. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatContributionLine {
    GENERATED_BODY()

    /** Name of the derived value, taken from the same rule table that names it everywhere else. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Label;

    /** Preformatted, e.g. "+38%". */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Value;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float Fraction = 0.0f;

    /** True when the diminishing curve is measurably cutting this contribution, so the UI can say so. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bDiminished = false;
};

namespace MythicStatDisplay {
    MYTHIC_API FMythicStatRule GetRule(const FGameplayAttribute &Attribute);

    /**
     * How one rolled or granted modifier should read, given the attribute's registry format and the op that
     * applies it. Every affix, effect line and comparison row resolves through here so a value and the range
     * printed beside it can never disagree.
     *
     * bForcePercent is the authored FRollDefinition override, honoured only for Flat attributes.
     */
    MYTHIC_API EMythicStatFormat ResolveRollFormat(const FGameplayAttribute &Attribute,
                                                   TEnumAsByte<EGameplayModOp::Type> Op,
                                                   bool bForcePercent = false);

    MYTHIC_API FString MakeFriendlyLabel(const FString &PropertyName);

    MYTHIC_API FText FormatValue(float Value, EMythicStatFormat Format);

    MYTHIC_API FText FormatBonus(float Delta, EMythicStatFormat Format);

    MYTHIC_API FText GetCategoryLabel(EMythicStatCategory Category);

    MYTHIC_API void InvalidateCache();
}
