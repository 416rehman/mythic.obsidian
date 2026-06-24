// 

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "ItemTooltipVM.h"
#include "ItemComparisonVM.generated.h"

class UMythicItemInstance;
class UMythicInventoryComponent;

// the delta for a single attribute between an inspected item and the currently equipped one
USTRUCT(BlueprintType)
struct FAttributeDiff {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    FText AttributeName;

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    float CurrentValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    float NewValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    float Delta = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    bool bIsUpgrade = false;

    // compare two attribute diff structures for equality
    bool operator==(const FAttributeDiff& Other) const {
        return AttributeName.EqualTo(Other.AttributeName) &&
               FMath::IsNearlyEqual(CurrentValue, Other.CurrentValue) &&
               FMath::IsNearlyEqual(NewValue, Other.NewValue) &&
               FMath::IsNearlyEqual(Delta, Other.Delta) &&
               bIsUpgrade == Other.bIsUpgrade;
    }
};

UCLASS()
class MYTHIC_API UItemComparisonVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // tooltip vm for the item being inspected
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Comparison")
    UItemTooltipVM *InspectedItem = nullptr;

    // tooltip vm for the currently equipped item (null if slot is empty)
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Comparison")
    UItemTooltipVM *EquippedItem = nullptr;

    // per-attribute diffs between inspected and equipped
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Comparison")
    TArray<FAttributeDiff> AttributeDiffs;

    // factory: builds a comparison between an inspected item and whatever is equipped in its matching slot
    UFUNCTION(BlueprintCallable, Category="Mythic|Comparison")
    static UItemComparisonVM *CreateComparison(UObject *Outer, UMythicItemInstance *Inspected, UMythicInventoryComponent *Inventory);

    void SetInspectedItem(UItemTooltipVM *InInspectedItem);
    UItemTooltipVM *GetInspectedItem() const;
    void SetEquippedItem(UItemTooltipVM *InEquippedItem);
    UItemTooltipVM *GetEquippedItem() const;
    void SetAttributeDiffs(TArray<FAttributeDiff> InAttributeDiffs);
    TArray<FAttributeDiff> GetAttributeDiffs() const;
};
