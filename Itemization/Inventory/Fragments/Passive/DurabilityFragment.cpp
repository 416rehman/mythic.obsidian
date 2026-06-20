//

#include "DurabilityFragment.h"

#include "Mythic.h"
#include "Itemization/Inventory/MythicItemInstance.h"

void UDurabilityFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance); // sets ParentItemInstance

    // Seed starting durability (OnInstanced runs server-side at item creation; the value replicates).
    DurabilityRuntimeReplicatedData.Current = FMath::Max(0, DurabilityConfig.MaxDurability);
    DurabilityRuntimeReplicatedData.bBroken = false;
}

void UDurabilityFragment::ServerApplyWear(int32 Amount) {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    // No-op if durability is disabled for this item, the amount is non-positive, or it's already broken.
    if (Amount <= 0 || DurabilityConfig.MaxDurability <= 0 || DurabilityRuntimeReplicatedData.bBroken) {
        return;
    }

    DurabilityRuntimeReplicatedData.Current = FMath::Max(0, DurabilityRuntimeReplicatedData.Current - Amount);
    if (DurabilityRuntimeReplicatedData.Current <= 0) {
        DurabilityRuntimeReplicatedData.bBroken = true;
        UE_LOG(Myth, Log, TEXT("UDurabilityFragment: item %s broke (durability hit 0)."), *GetNameSafe(GetOwningItemInstance()));
    }
}

void UDurabilityFragment::ServerRepair(int32 Amount) {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (Amount <= 0 || DurabilityConfig.MaxDurability <= 0) {
        return;
    }

    DurabilityRuntimeReplicatedData.Current = FMath::Min(DurabilityConfig.MaxDurability, DurabilityRuntimeReplicatedData.Current + Amount);
    if (DurabilityRuntimeReplicatedData.Current > 0 && DurabilityRuntimeReplicatedData.bBroken) {
        DurabilityRuntimeReplicatedData.bBroken = false;
        UE_LOG(Myth, Log, TEXT("UDurabilityFragment: item %s repaired (durability %d)."),
               *GetNameSafe(GetOwningItemInstance()), DurabilityRuntimeReplicatedData.Current);
    }
}
