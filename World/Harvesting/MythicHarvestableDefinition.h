#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "MythicHarvestableDefinition.generated.h"

class UItemDefinition;
class UMythicHarvestToolTypeDefinition;
class UNiagaraSystem;
class UProficiencyDefinition;
class USoundBase;
class UTexture2D;

/** Determines how the deterministic reward transaction assigns item quality. */
UENUM(BlueprintType)
enum class EMythicHarvestRewardQualityPolicy : uint8 {
    DefinitionDefault,
    Fixed,
    ContributorProficiency,
};

/** One optional presentation threshold crossed as authoritative completed-work fraction increases. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHarvestWorkStage {
    GENERATED_BODY()

    /**
     * Definition-owned normalized completed-work threshold evaluated by the server and mirrored for presentation;
     * reading is side-effect free, invalid/nonascending values fail validation, and units are a fraction in (0,1).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Work", meta = (ClampMin = "0.0001", ClampMax = "0.9999"))
    float CompletedWorkFraction = 0.5f;

    /**
     * Definition-owned optional effect shown after this committed threshold is crossed; Blueprint only presents it,
     * null/load failure is mechanically inert, and its location is supplied separately in centimeters.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Work")
    TSoftObjectPtr<UNiagaraSystem> StageEffect;

    /**
     * Definition-owned optional sound shown after this committed threshold is crossed; Blueprint only presents it,
     * null/load failure is mechanically inert, and the reference carries no gameplay units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Work")
    TSoftObjectPtr<USoundBase> StageSound;
};

/** One direct item row in either the primary-material or bonus-loot completion channel. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHarvestRewardEntry {
    GENERATED_BODY()

    /**
     * Definition-owned direct item asset granted by the server transaction; clients may read it for presentation,
     * null fails validation, delivery failure queues the same deterministic grant, and quantity is authored below.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Reward")
    TObjectPtr<UItemDefinition> ItemDefinition = nullptr;

    /**
     * Definition-owned inclusive minimum quantity rolled once by the server; reading is side-effect free, invalid
     * values fail validation, and units are whole item-stack units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Reward", meta = (ClampMin = "1"))
    int32 MinQuantity = 1;

    /**
     * Definition-owned inclusive maximum quantity rolled once by the server; reading is side-effect free, values
     * below MinQuantity fail validation, and units are whole item-stack units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Reward", meta = (ClampMin = "1"))
    int32 MaxQuantity = 1;

    /**
     * Definition-owned independent deterministic eligibility probability evaluated only on authority; clients may
     * display it, invalid values fail validation, and units are a normalized probability in [0,1].
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Reward", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Probability = 1.0f;

    /**
     * Definition-owned positive relative selection weight consumed by deterministic channel selection on authority;
     * reading has no side effects, nonfinite/nonpositive values fail validation, and the value is unitless.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Reward", meta = (ClampMin = "0.000001"))
    float SelectionWeight = 1.0f;

    /**
     * Definition-owned quality rule evaluated by the server reward plan; Blueprint may describe but not execute it,
     * an unknown enum fails validation, and the rule has no numeric units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Reward")
    EMythicHarvestRewardQualityPolicy QualityPolicy = EMythicHarvestRewardQualityPolicy::DefinitionDefault;

    /**
     * Definition-owned quality used only by the Fixed policy; Blueprint may display but not apply it, an invalid enum
     * fails validation, and the discrete quality tier has no physical units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Reward",
              meta = (EditCondition = "QualityPolicy == EMythicHarvestRewardQualityPolicy::Fixed", EditConditionHides))
    EMythicYieldQuality FixedQuality = EMythicYieldQuality::Common;
};

/** Typed quest-credit behavior emitted after a committed completion. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHarvestQuestCreditPolicy {
    GENERATED_BODY()

    /**
     * Definition-owned opt-in for a native completion-credit event carrying this exact Harvestable Definition;
     * Blueprint cannot emit it, false is inert, and the flag has no units or tag/string authorization fallback.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Quest")
    bool bEmitCompletionCredit = true;

    /**
     * Definition-owned credit count emitted once per eligible contributor after commit; clients may display it,
     * nonpositive values fail validation when enabled, and units are whole objective credits.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Quest", meta = (ClampMin = "1", EditCondition = "bEmitCompletionCredit"))
    int32 CreditCount = 1;
};

/** Typed regional-pressure behavior emitted after a committed completion. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHarvestPressurePolicy {
    GENERATED_BODY()

    /**
     * Definition-owned opt-in for the native Harvest pressure channel after server commit; Blueprint cannot emit it,
     * false is inert, and no caller-selected tag or string is used as authority.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Pressure")
    bool bEmitPressure = true;

    /**
     * Definition-owned pressure added once for a committed node generation; clients may display it, negative or
     * nonfinite values fail validation, and units are regional Harvest-pressure units per completion.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest|Pressure", meta = (ClampMin = "0.0", EditCondition = "bEmitPressure"))
    float PressurePerCompletion = 1.0f;
};

/**
 * Single direct source of work, eligibility, yield, progression, lifecycle, and presentation for one harvestable
 * semantic type. Runtime components reference this asset and must not duplicate its gameplay fields.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicHarvestableDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    /**
     * Definition-owned localized node name readable on every peer; reading has no side effects, an empty value fails
     * validation, and the text carries no gameplay units or authorization evidence.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Presentation")
    FText DisplayName;

    /**
     * Definition-owned localized action verb such as Chop or Mine; Blueprint may present but not interpret it,
     * an empty value fails validation, and the text has no gameplay units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Presentation")
    FText HarvestVerb;

    /**
     * Definition-owned optional UI icon loaded for presentation on any peer; reading is side-effect free, null/load
     * failure hides only the icon, and this reference carries no gameplay units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Presentation")
    TSoftObjectPtr<UTexture2D> Icon;

    /**
     * Definition-owned analytics classification; it is optional and side-effect free, has no units, and must never
     * authorize eligibility or replace the direct RequiredToolType reference.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Analytics")
    FGameplayTag ResourceTaxonomyTag;

    /**
     * Definition-owned exact tool-family asset compared by pointer identity on authority; launch content requires a
     * non-null family (including Sickle for shrubs), clients may read it for prompts, and missing references fail cook.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Eligibility")
    TObjectPtr<UMythicHarvestToolTypeDefinition> RequiredToolType = nullptr;

    /**
     * Definition-owned minimum tier checked against the exact active tool on authority; clients may present it,
     * negative values fail validation, and units are integer tool tiers.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Eligibility", meta = (ClampMin = "0"))
    int32 MinimumToolTier = 0;

    /**
     * Definition-owned total work converted to fixed quanta by authority; clients may display it, nonfinite or
     * nonpositive/unquantizable values fail validation, and units are continuous harvest-work units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Work", meta = (ClampMin = "0.0001"))
    float MaxWork = 1.0f;

    /**
     * Definition-owned ascending normalized presentation thresholds; Blueprint may render committed stages only,
     * invalid/duplicate ordering fails validation, and each threshold is a completed-work fraction in (0,1).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Work")
    TArray<FMythicHarvestWorkStage> WorkStages;

    /**
     * Definition-owned direct progression track resolved by authority; clients may display it, null fails validation,
     * and no resource-tag map or stat-sheet scan is used as fallback.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Progression")
    TObjectPtr<UProficiencyDefinition> ProficiencyDefinition = nullptr;

    /**
     * Definition-owned base proficiency XP accrued per accepted applied work; authority grants it idempotently,
     * nonfinite/negative values fail validation, and units are base XP per harvest-work unit.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Progression", meta = (ClampMin = "0.0"))
    float ProficiencyXPPerAppliedWork = 0.0f;

    /**
     * Definition-owned base proficiency XP distributed once at completion; authority grants it idempotently,
     * nonfinite/negative values fail validation, and units are base XP per node completion.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Progression", meta = (ClampMin = "0.0"))
    float CompletionProficiencyXP = 0.0f;

    /**
     * Definition-owned direct primary-material rows rolled and contribution-split once by authority; Blueprint may
     * describe them, invalid/missing/duplicate rows fail validation, and quantities are whole item units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Rewards")
    TArray<FMythicHarvestRewardEntry> PrimaryMaterials;

    /**
     * Definition-owned direct bonus-loot rows rolled separately once by authority; Blueprint may describe them,
     * invalid/missing/duplicate rows fail validation, and quantities are whole item units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Rewards")
    TArray<FMythicHarvestRewardEntry> BonusLoot;

    /**
     * Definition-owned typed quest hook consumed only after authoritative completion; Blueprint may inspect but not
     * emit credit, invalid enabled counts fail validation, and units are documented inside the policy.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Hooks")
    FMythicHarvestQuestCreditPolicy QuestCredit;

    /**
     * Definition-owned typed pressure hook consumed only after authoritative completion; Blueprint may inspect but
     * not emit pressure, invalid values fail validation, and units are documented inside the policy.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Hooks")
    FMythicHarvestPressurePolicy Pressure;

    /**
     * Definition-owned regrowth delay scheduled from server world time after completion; clients may display it,
     * nonfinite/negative values fail validation, and units are seconds excluding offline time.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Lifecycle", meta = (ClampMin = "0.0", Units = "s"))
    float RespawnDelaySeconds = 60.0f;

    /**
     * Definition-owned policy allowing authority to defer visible restoration near living players; Blueprint may
     * explain it but cannot restore a node, false restores exactly at deadline, and the flag has no units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Lifecycle")
    bool bDeferRegrowWhileVisible = true;

    /**
     * Definition-owned optional accepted-hit effect emitted only from post-commit feedback; Blueprint may present it,
     * null/load failure is mechanically inert, and the effect location is supplied in centimeters.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Feedback")
    TSoftObjectPtr<UNiagaraSystem> AcceptedHitEffect;

    /**
     * Definition-owned optional accepted-hit sound emitted only from post-commit feedback; Blueprint may present it,
     * null/load failure is mechanically inert, and the reference has no gameplay units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Feedback")
    TSoftObjectPtr<USoundBase> AcceptedHitSound;

    /**
     * Definition-owned optional completion effect emitted only after the native completion commit; Blueprint may
     * present it, null/load failure is mechanically inert, and the effect location is supplied in centimeters.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Feedback")
    TSoftObjectPtr<UNiagaraSystem> CompletionEffect;

    /**
     * Definition-owned optional completion sound emitted only after the native completion commit; Blueprint may
     * present it, null/load failure is mechanically inert, and the reference has no gameplay units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Feedback")
    TSoftObjectPtr<USoundBase> CompletionSound;

    /**
     * Definition-owned optional rejection effect emitted from owner feedback without mutation; Blueprint may present
     * it, null/load failure is mechanically inert, and the effect location is supplied in centimeters.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Feedback")
    TSoftObjectPtr<UNiagaraSystem> RejectionEffect;

    /**
     * Definition-owned optional rejection sound emitted from owner feedback without mutation; Blueprint may present
     * it, null/load failure is mechanically inert, and the reference has no gameplay units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Feedback")
    TSoftObjectPtr<USoundBase> RejectionSound;

    /**
     * Definition-owned optional regrow effect emitted after authoritative restoration; Blueprint may present it,
     * null/load failure is mechanically inert, and the effect location is supplied in centimeters.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Feedback")
    TSoftObjectPtr<UNiagaraSystem> RegrowEffect;

    /**
     * Definition-owned optional regrow sound emitted after authoritative restoration; Blueprint may present it,
     * null/load failure is mechanically inert, and the reference has no gameplay units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvestable|Feedback")
    TSoftObjectPtr<USoundBase> RegrowSound;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    bool AppendValidationErrors(TArray<FText> &OutErrors) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
