
#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "ItemTooltipVM.h"
#include "ItemComparisonVM.generated.h"

class UMythicItemInstance;
class UMythicInventoryComponent;

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

    // per-attribute diffs between inspected and equipped (key-union: rows for stats present on EITHER side; base
    // weapon damage min/max, attack speed and durability are included alongside the rolled affixes)
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Comparison")
    TArray<FAttributeDiff> AttributeDiffs;

    // net upgrade count across the diffs (+1 per upgraded stat, -1 per downgraded; see
    // FMythicStatDeltaCore::ComputeUpgradeScore) — drives the at-a-glance verdict arrow
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Comparison")
    int32 UpgradeScore = 0;

    // true when UpgradeScore > 0 (equipping is a net stat gain vs the slot it would occupy)
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter = "SetIsUpgradeOverall", Getter = "GetIsUpgradeOverall", Category="Mythic|Comparison")
    bool bIsUpgradeOverall = false;

    // factory: builds a comparison between an inspected item and the equipped item it would actually REPLACE.
    // TargetSlotIndex (an index into Inventory->GetAllSlots()) pins the candidate slot explicitly (e.g. hovering a
    // specific ring slot); pass -1 to auto-pick. Auto-pick rule for an item that fits multiple equipment slots:
    // a matching EMPTY slot wins first (equipping there replaces nothing — the comparison is vs nothing, a pure
    // gain), otherwise the occupied matching slot with the BEST upgrade score (the replacement a rational player
    // would make; also the old "first whitelisted slot" bug fix — see the .cpp).
    UFUNCTION(BlueprintCallable, Category="Mythic|Comparison")
    static UItemComparisonVM *CreateComparison(UObject *Outer, UMythicItemInstance *Inspected, UMythicInventoryComponent *Inventory,
                                               int32 TargetSlotIndex = -1);

    void SetInspectedItem(UItemTooltipVM *InInspectedItem);
    UItemTooltipVM *GetInspectedItem() const;
    void SetEquippedItem(UItemTooltipVM *InEquippedItem);
    UItemTooltipVM *GetEquippedItem() const;
    void SetAttributeDiffs(TArray<FAttributeDiff> InAttributeDiffs);
    TArray<FAttributeDiff> GetAttributeDiffs() const;
    void SetUpgradeScore(int32 InUpgradeScore);
    int32 GetUpgradeScore() const;
    void SetIsUpgradeOverall(bool bInIsUpgradeOverall);
    bool GetIsUpgradeOverall() const;
};
