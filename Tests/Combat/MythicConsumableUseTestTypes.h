#pragma once
#include "CoreMinimal.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "MythicConsumableUseTestTypes.generated.h"

/** Test-only callback probe; real production inventory settlement invokes the normal fragment lifecycle. */
UCLASS(NotBlueprintable,Hidden)
class UMythicConsumableUseAuditFragment : public UItemFragment {
    GENERATED_BODY()
public:
#if WITH_DEV_AUTOMATION_TESTS
    TFunction<void(UMythicInventoryComponent*,int32)> InventoryChanged;
    virtual void OnInventorySlotChanged(UMythicInventoryComponent* Inventory,int32 Slot) override {
        Super::OnInventorySlotChanged(Inventory,Slot);
        if (InventoryChanged) InventoryChanged(Inventory,Slot);
    }
#endif
};
