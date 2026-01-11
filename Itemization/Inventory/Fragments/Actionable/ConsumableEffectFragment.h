// ConsumableEffectFragment.h
// Fragment for consumable items that apply gameplay effects with full UI display support

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/Fragments/ActionableItemFragment.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "ConsumableEffectFragment.generated.h"

// How the magnitude value is calculated
UENUM(BlueprintType)
enum class EMythicMagnitudeSource : uint8 {
    Static, // ScalableFloat - known value
    SetByCaller, // From fragment config
    AttributeBased, // Scales with an attribute
    Custom // Custom calculation - use override text
};

// Structured modifier info for UI display
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEffectModifierDisplayData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    FGameplayAttribute Attribute;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    TEnumAsByte<EGameplayModOp::Type> ModifierOp = EGameplayModOp::Additive;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    float Magnitude = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    EMythicMagnitudeSource MagnitudeSource = EMythicMagnitudeSource::Static;

    // For SetByCaller
    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    FGameplayTag SetByCallerTag;

    // For AttributeBased
    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    FGameplayAttribute BackingAttribute;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    float BackingCoefficient = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    bool bIsPercentage = false;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    bool bIsPositive = true;

    // C++ helpers
    FText GetAttributeDisplayName() const;
    FText GetMagnitudeText() const;
    FText GetFormattedText() const;
    FText GetRichText() const;
    // With ASC: calculates actual magnitude for attribute-backed modifiers
    FText GetMagnitudeText(UAbilitySystemComponent *ASC, bool bShowContext = false) const;
    FText GetRichText(UAbilitySystemComponent *ASC, bool bShowContext = false) const;
};

// Duration info for UI display
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEffectDurationDisplayData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    EGameplayEffectDurationType DurationType = EGameplayEffectDurationType::Instant;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    float Duration = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    EMythicMagnitudeSource DurationSource = EMythicMagnitudeSource::Static;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    FGameplayAttribute DurationBackingAttribute;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    float DurationBackingCoefficient = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    float Period = 0.0f;

    bool IsPeriodic() const { return Period > 0.0f && DurationType != EGameplayEffectDurationType::Instant; }
    FText GetFormattedText() const;
    FText GetRichText() const;
    // With ASC: calculates actual duration for attribute-backed duration
    FText GetFormattedText(UAbilitySystemComponent *ASC, bool bShowContext = false) const;
    FText GetRichText(UAbilitySystemComponent *ASC, bool bShowContext = false) const;
};

// Stacking info
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEffectStackingDisplayData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    EGameplayEffectStackingType StackingType = EGameplayEffectStackingType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    int32 StackLimitCount = 0;

    bool CanStack() const { return StackingType != EGameplayEffectStackingType::None && StackLimitCount != 1; }
    FText GetFormattedText() const;
    FText GetRichText() const;
};

// Combined display data for a single GE
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEffectDisplayData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    TArray<FMythicEffectModifierDisplayData> Modifiers;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    FMythicEffectDurationDisplayData Duration;

    UPROPERTY(BlueprintReadOnly, Category = "Effect")
    FMythicEffectStackingDisplayData Stacking;

    // Full rich text for entire effect
    FText GetRichText() const;
    FText GetRichText(UAbilitySystemComponent *ASC, bool bShowContext = false) const;
};

// Runtime data
USTRUCT(BlueprintType)
struct FConsumableEffectRuntimeReplicatedData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    UAbilitySystemComponent *ASC = nullptr;

    UPROPERTY()
    FGameplayAbilitySpecHandle AbilityHandle;
};

// Entry for a single gameplay effect with SetByCaller support
USTRUCT(BlueprintType)
struct FConsumableEffectEntry {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TSubclassOf<UGameplayEffect> GameplayEffectClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float EffectLevel = 1.0f;

    // SetByCaller values for MODIFIERS
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|SetByCaller", SaveGame, meta = (Categories = "SetByCaller"))
    TMap<FGameplayTag, float> SetByCallerMagnitudes;

    // Override display text for complex effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Display")
    FText DisplayTextOverride;
};

USTRUCT(BlueprintType)
struct FConsumableEffectConfig {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", SaveGame)
    TArray<FConsumableEffectEntry> Effects;
};

/**
 * Fragment for consumable items that apply gameplay effects.
 * Supports full UI display with SetByCaller, attribute-backed, stacking, and rich text.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UConsumableEffectFragment : public UActionableItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(ConsumableEffect)

    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), SaveGame)
    FConsumableEffectConfig ConsumableEffectConfig;

    UPROPERTY(Replicated, BlueprintReadOnly, SaveGame)
    FConsumableEffectRuntimeReplicatedData ConsumableEffectRuntimeReplicatedData;

    virtual void OnClientActionEnd(UMythicItemInstance *ItemInstance) override;
    virtual void OnInstanced(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnInventorySlotChanged(UMythicInventoryComponent *Inventory, int32 SlotIndex) override;
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    virtual void ExecuteGenericAction(UMythicItemInstance *ItemInstance) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(ThisClass, ConsumableEffectRuntimeReplicatedData, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(ThisClass, ConsumableEffectConfig, COND_InitialOrOwner);
    }

    // ========== UI INTROSPECTION ==========

    // Get display data for an effect entry
    UFUNCTION(BlueprintPure, Category = "Mythic|Consumable")
    static FMythicEffectDisplayData GetEffectDisplayDataFromEntry(const FConsumableEffectEntry &Entry);

    // Get display data for a GE class (no SetByCaller)
    UFUNCTION(BlueprintPure, Category = "Mythic|Consumable")
    static FMythicEffectDisplayData GetEffectDisplayData(TSubclassOf<UGameplayEffect> EffectClass);

    // Get display data for the primary (first) effect
    UFUNCTION(BlueprintPure, Category = "Mythic|Consumable")
    FMythicEffectDisplayData GetPrimaryEffectDisplayData() const;

    // Get display data for all effects
    UFUNCTION(BlueprintPure, Category = "Mythic|Consumable")
    TArray<FMythicEffectDisplayData> GetAllEffectDisplayData() const;

    // Get combined rich text for all effects
    UFUNCTION(BlueprintPure, Category = "Mythic|Consumable")
    FText GetCombinedRichText() const;

    UFUNCTION(Server, Reliable)
    void ServerApplyEffects(UMythicItemInstance *ItemInstance);

protected:
    static FMythicEffectModifierDisplayData ParseModifier(
        const FGameplayModifierInfo &Modifier,
        const TMap<FGameplayTag, float> &SetByCallerValues);

    static FMythicEffectDurationDisplayData ParseDuration(
        const UGameplayEffect *GE,
        const TMap<FGameplayTag, float> &SetByCallerValues);

    static FMythicEffectStackingDisplayData ParseStacking(const UGameplayEffect *GE);
};
