// 

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"

#include "MythicInventoryComponent_ViewModel.generated.h"

class UMythicInventorySlot;
/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicInventoryComponent_ViewModel : public UMVVMViewModelBase {
    GENERATED_BODY()

private:
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 ActiveSlotIndex = 0;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 DefaultSize = 10;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TArray<UMythicInventorySlot *> Slots;

public:
    void Initialize (int32 NewActiveSlotIndex, int32 NewDefaultSize, TArray<UMythicInventorySlot *> NewSlots) {
        SetActiveSlotIndex(NewActiveSlotIndex);
        SetDefaultSize(NewDefaultSize);
        SetSlots(NewSlots);
    }
    
    void SetActiveSlotIndex(int32 NewActiveSlotIndex) {
        if (UE_MVVM_SET_PROPERTY_VALUE(ActiveSlotIndex, NewActiveSlotIndex)) {
            UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveSlotIndex);
        }
    }

    const int32 GetActiveSlotIndex() const {
        return ActiveSlotIndex;
    }

    void SetDefaultSize(int32 NewSize) {
        if (UE_MVVM_SET_PROPERTY_VALUE(DefaultSize, NewSize)) {
            UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DefaultSize);
        }
    }

    const int32 GetDefaultSize() const {
        return DefaultSize;
    }

    void SetSlots(TArray<UMythicInventorySlot *> NewSlots) {
        if (UE_MVVM_SET_PROPERTY_VALUE(Slots, NewSlots)) {
            UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
        }
    }

    const TArray<UMythicInventorySlot *> GetSlots() const {
        return Slots;
    }
};
