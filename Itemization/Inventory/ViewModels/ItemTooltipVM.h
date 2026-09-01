
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Stats/MythicStatTypes.h"
#include "UI/ViewModels/MythicAffixViewData.h"
#include "ItemTooltipVM.generated.h"

class UMythicItemInstance;
class UMythicItemizationDataRegistrySubsystem;
class UTexture2D;

/**
 * Canonical item-local attack summary for a weapon tooltip.
 *
 * Values are composed from the weapon's immutable affix snapshots, their live Affix/Stat Definitions, the authored
 * attack montage cadence, and the same attack-speed clamp used by combat. Character stats and temporary effects are
 * deliberately excluded: this describes the inspected weapon, not the currently equipped character loadout.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicWeaponAttackViewData {
    GENERATED_BODY()

    /** True only after every numeric field and localized text field was built successfully and atomically. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    bool bIsValid = false;

    /** Lower endpoint of the item-local basic-hit range after DamagePerHit affixes are composed in gameplay order. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    float MinimumDamagePerHit = 0.0f;

    /** Upper endpoint of the item-local basic-hit range resolved by the shared combat weapon-roll model. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    float MaximumDamagePerHit = 0.0f;

    /** Expected item-local damage per hit for the uniform combat weapon-roll range. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    float AverageDamagePerHit = 0.0f;

    /** Item-local AttackSpeed bonus fraction after all affixes targeting that GAS attribute are composed. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    float AttackSpeedBonus = 0.0f;

    /** Expected authored attacks per second derived from the uniformly selected montage variant cycle. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    float BaseAttacksPerSecond = 0.0f;

    /** Effective item-local attacks per second after applying the canonical combat AttackSpeed clamp. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    float AttacksPerSecond = 0.0f;

    /** Effective seconds per attack after applying the canonical combat AttackSpeed clamp. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    float AttackTimeSeconds = 0.0f;

    /** Expected item-local sustained DPS: average damage per hit multiplied by effective attacks per second. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    float DamagePerSecond = 0.0f;

    /** Canonical live DamagePerHit formatting also used for average-hit and sustained-DPS comparison. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    FMythicStatNumberPresentation DamageNumberPresentation;

    /** Canonical formatting for the effective attacks-per-second item metric. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    FMythicStatNumberPresentation AttacksPerSecondNumberPresentation;

    /** Localized minimum-to-maximum hit range formatted by the live DamagePerHit Stat Definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    FText DamagePerHitText;

    /** Localized attacks-per-second text formatted for the dedicated weapon metric. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    FText AttacksPerSecondText;

    /** Localized DPS text formatted consistently with the live DamagePerHit Stat Definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Tooltip|Attack")
    FText DamagePerSecondText;

    bool operator==(const FMythicWeaponAttackViewData& Other) const {
        return bIsValid == Other.bIsValid &&
               FMath::IsNearlyEqual(MinimumDamagePerHit, Other.MinimumDamagePerHit) &&
               FMath::IsNearlyEqual(MaximumDamagePerHit, Other.MaximumDamagePerHit) &&
               FMath::IsNearlyEqual(AverageDamagePerHit, Other.AverageDamagePerHit) &&
               FMath::IsNearlyEqual(AttackSpeedBonus, Other.AttackSpeedBonus) &&
               FMath::IsNearlyEqual(BaseAttacksPerSecond, Other.BaseAttacksPerSecond) &&
               FMath::IsNearlyEqual(AttacksPerSecond, Other.AttacksPerSecond) &&
               FMath::IsNearlyEqual(AttackTimeSeconds, Other.AttackTimeSeconds) &&
               FMath::IsNearlyEqual(DamagePerSecond, Other.DamagePerSecond) &&
               DamageNumberPresentation.Format == Other.DamageNumberPresentation.Format &&
               DamageNumberPresentation.DecimalPlaces == Other.DamageNumberPresentation.DecimalPlaces &&
               DamageNumberPresentation.UnitSuffix.EqualTo(Other.DamageNumberPresentation.UnitSuffix) &&
               AttacksPerSecondNumberPresentation.Format == Other.AttacksPerSecondNumberPresentation.Format &&
               AttacksPerSecondNumberPresentation.DecimalPlaces == Other.AttacksPerSecondNumberPresentation.DecimalPlaces &&
               AttacksPerSecondNumberPresentation.UnitSuffix.EqualTo(Other.AttacksPerSecondNumberPresentation.UnitSuffix) &&
               DamagePerHitText.EqualTo(Other.DamagePerHitText) &&
               AttacksPerSecondText.EqualTo(Other.AttacksPerSecondText) &&
               DamagePerSecondText.EqualTo(Other.DamagePerSecondText);
    }
};

/** Flattened Blueprint-facing projection of one immutable rolled affix and its live semantic definition. */
USTRUCT(BlueprintType)
struct FAffixDisplayData {
    GENERATED_BODY()

    /** Canonical one-affix/one-stat presentation resolved from live Affix and Stat Definitions. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    FMythicAffixViewData ViewData;

    /** Flattened canonical stat identity used by item-comparison aggregation. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    FGameplayTag StatTag;

    /** Flattened localized affix label for simple MVVM bindings. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    FText AttributeName;

    /** Flattened immutable roll magnitude used by item-comparison aggregation. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    float Value = 0.0f;

    /** Presentation hint derived from the live Stat Definition and Affix Definition operation. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    bool bIsPercentage = false;

    /** Comparison hint derived from the live Stat Definition; never independent gameplay authority. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    bool bLowerIsBetter = false;

    /** True when this implicit core stat is routed into the dedicated weapon attack presentation. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    bool bOwnedByWeaponAttackPresentation = false;

    /** Complete localized affix line in project rich-text markup for simple tooltip bindings. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    FText RichText;

    bool operator==(const FAffixDisplayData& Other) const {
        return ViewData.RollGuid == Other.ViewData.RollGuid &&
               StatTag == Other.StatTag &&
               AttributeName.EqualTo(Other.AttributeName) &&
               FMath::IsNearlyEqual(Value, Other.Value) &&
               bIsPercentage == Other.bIsPercentage &&
               bLowerIsBetter == Other.bLowerIsBetter &&
               bOwnedByWeaponAttackPresentation == Other.bOwnedByWeaponAttackPresentation &&
               RichText.EqualTo(Other.RichText);
    }
};

/** Blueprint-facing localized presentation projection of one item talent. */
USTRUCT(BlueprintType)
struct FTalentDisplayData {
    GENERATED_BODY()

    /** Localized player-facing talent name projected from the item's authoritative talent data. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    FText Name;

    /** Optional presentation icon for the talent; it has no gameplay authority. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    UTexture2D *Icon = nullptr;

    /** Localized player-facing explanation of the talent's projected effects. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    FText Description;

    bool operator==(const FTalentDisplayData& Other) const {
        return Name.EqualTo(Other.Name) &&
               Icon == Other.Icon &&
               Description.EqualTo(Other.Description);
    }
};

UCLASS()
class MYTHIC_API UItemTooltipVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    /** Localized item name projected from the immutable item definition for MVVM binding. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FText Name;

    /** Localized item description projected from the immutable item definition for MVVM binding. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FText Description;

    /** Authoritative generated rarity used by tooltip styling and tier presentation. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    TEnumAsByte<EItemRarity> Rarity = EItemRarity::Common;

    /** Authoritative item level captured by the instance and used to resolve level-scaled item data. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    int32 ItemLevel = 1;

    /** Presentation icon projected from the item definition; changing it does not mutate the item. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    UTexture2D *Icon = nullptr;

    /** Canonical hierarchical item-type identity used by widgets for layout and equipment context. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FGameplayTag ItemType;

    /** Atomic display projection of all immutable affix snapshots currently owned by the item. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    TArray<FAffixDisplayData> Affixes;

    /** Display-only projection of talents attached to the item; talent fragments remain gameplay authority. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    TArray<FTalentDisplayData> Talents;

    /** Current authoritative durability value, or zero when the item has no durability fragment. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    float CurrentDurability = 0.0f;

    /** Maximum authoritative durability value, or zero when the item has no durability fragment. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    float MaxDurability = 0.0f;

    /** Clamped current/max durability ratio prepared for progress-bar binding; it is presentation-only derived data. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    float DurabilityPercent = 0.0f;

    /**
     * Atomic item-local weapon projection shared by the DPS block and comparison UI. It is invalid for non-weapons
     * and whenever live affix/stat semantics are unavailable, so consumers never fall back to partial attack math.
     */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FMythicWeaponAttackViewData WeaponAttack;

    /** Per-stack or per-item weight projected from authoritative inventory data for UI display. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    float Weight = 0.0f;

    /** Current authoritative economic value projected for vendor and tooltip presentation. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    int32 Value = 0;

    /** Current authoritative quantity in this item instance's stack. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    int32 StackSize = 0;

    /** Maximum stack quantity allowed by the immutable item definition. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    int32 StackMax = 1;

    /** Gameplay tag requirement that the owning character must satisfy before this item can be equipped. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FGameplayTag RequiredEquipTag;

    /** Builds a display-only tooltip projection from an existing item; it never mutates authoritative item state. */
    UFUNCTION(BlueprintCallable, Category="Mythic|Tooltip")
    static UItemTooltipVM *CreateFromItemInstance(UObject *Outer, UMythicItemInstance *Item);

    /**
     * Builds only the affix presentation projection. This is the hot-path-safe shared seam for item-detail rows:
     * it uses the item's canonical type probe and level to resolve live progression ranges, then consumes immutable
     * snapshots and already-loaded semantic data without loading unrelated assets. The result is atomic.
     */
    static bool BuildAffixDisplayData(UMythicItemInstance *Item,
                                      const UMythicItemizationDataRegistrySubsystem *Registry,
                                      TArray<FAffixDisplayData> &OutAffixes);

    /** Returns true only for equipment weapons that own a typed Attack Fragment. */
    static bool ShouldUseWeaponAttackPresentation(UMythicItemInstance *Item);

    /** Returns true for the two GAS attributes owned by the dedicated weapon attack block. */
    static bool IsWeaponAttackAttribute(const FGameplayAttribute &Attribute);

    /**
     * Pure numeric weapon-metric calculation shared by runtime presentation and automation tests. It applies the
     * exact combat play-rate clamp and rejects non-finite or non-physical inputs without producing partial output.
     */
    static bool CalculateWeaponAttackMetrics(float DamagePerHit,
                                             float AttackSpeedBonus,
                                             float MontageDurationSeconds,
                                             float MinAttackSpeedPlayRate,
                                             float MaxAttackSpeedPlayRate,
                                             FMythicWeaponAttackViewData &OutAttackData);

    /**
     * Builds the dedicated attack block from an already-atomic canonical affix projection. Modifier operations are
     * folded through the permanent-stat ledger, so editing an Affix Definition cannot leave tooltip math stale.
     */
    static bool BuildWeaponAttackDisplayData(
        UMythicItemInstance *Item,
        const UMythicItemizationDataRegistrySubsystem *Registry,
        TConstArrayView<FAffixDisplayData> DisplayAffixes,
        FMythicWeaponAttackViewData &OutAttackData);

    void SetName(FText InName);
    FText GetName() const;
    void SetDescription(FText InDescription);
    FText GetDescription() const;
    void SetRarity(EItemRarity InRarity);
    EItemRarity GetRarity() const;
    void SetItemLevel(int32 InItemLevel);
    int32 GetItemLevel() const;
    void SetIcon(UTexture2D *InIcon);
    UTexture2D *GetIcon() const;
    void SetItemType(FGameplayTag InItemType);
    FGameplayTag GetItemType() const;
    void SetAffixes(TArray<FAffixDisplayData> InAffixes);
    TArray<FAffixDisplayData> GetAffixes() const;
    void SetTalents(TArray<FTalentDisplayData> InTalents);
    TArray<FTalentDisplayData> GetTalents() const;
    void SetCurrentDurability(float InCurrentDurability);
    float GetCurrentDurability() const;
    void SetMaxDurability(float InMaxDurability);
    float GetMaxDurability() const;
    void SetDurabilityPercent(float InDurabilityPercent);
    float GetDurabilityPercent() const;
    void SetWeaponAttack(FMythicWeaponAttackViewData InWeaponAttack);
    FMythicWeaponAttackViewData GetWeaponAttack() const;
    void SetWeight(float InWeight);
    float GetWeight() const;
    void SetValue(int32 InValue);
    int32 GetValue() const;
    void SetStackSize(int32 InStackSize);
    int32 GetStackSize() const;
    void SetStackMax(int32 InStackMax);
    int32 GetStackMax() const;
    void SetRequiredEquipTag(FGameplayTag InRequiredEquipTag);
    FGameplayTag GetRequiredEquipTag() const;
};
