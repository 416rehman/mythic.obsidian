// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "ItemTooltipVM.generated.h"

class UMythicItemInstance;
class UTexture2D;

// display data for a single rolled affix
USTRUCT(BlueprintType)
struct FAffixDisplayData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    FText AttributeName;

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    float Value = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    bool bIsPercentage = false;

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    bool bLowerIsBetter = false;

    // compare two affix display structures for equality
    bool operator==(const FAffixDisplayData& Other) const {
        return AttributeName.EqualTo(Other.AttributeName) &&
               FMath::IsNearlyEqual(Value, Other.Value) &&
               bIsPercentage == Other.bIsPercentage &&
               bLowerIsBetter == Other.bLowerIsBetter;
    }
};

// display data for a single rolled talent
USTRUCT(BlueprintType)
struct FTalentDisplayData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    FText Name;

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    UTexture2D *Icon = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Tooltip")
    FText Description;

    // compare two talent display structures for equality
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
    // identity
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FText Name;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FText Description;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    TEnumAsByte<EItemRarity> Rarity = EItemRarity::Common;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    int32 ItemLevel = 1;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    UTexture2D *Icon = nullptr;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FGameplayTag ItemType;

    // affixes
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    TArray<FAffixDisplayData> Affixes;

    // talents
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    TArray<FTalentDisplayData> Talents;

    // durability
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    float CurrentDurability = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    float MaxDurability = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    float DurabilityPercent = 0.0f;

    // attack
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FText DamageRange;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    float AttackSpeed = 0.0f;

    // economics
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    float Weight = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    int32 Value = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    int32 StackSize = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    int32 StackMax = 1;

    // equip requirement
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Tooltip")
    FGameplayTag RequiredEquipTag;

    // factory
    UFUNCTION(BlueprintCallable, Category="Mythic|Tooltip")
    static UItemTooltipVM *CreateFromItemInstance(UObject *Outer, UMythicItemInstance *Item);

    // setters and getters
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
    void SetDamageRange(FText InDamageRange);
    FText GetDamageRange() const;
    void SetAttackSpeed(float InAttackSpeed);
    float GetAttackSpeed() const;
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
