
#include "DurabilityFragment.h"

#include "Mythic.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Player/MythicPlayerController.h"

void UDurabilityFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);

    DurabilityRuntimeReplicatedData.Current = FMath::Max(0, DurabilityConfig.MaxDurability);
    DurabilityRuntimeReplicatedData.bBroken = false;
}

void UDurabilityFragment::ServerApplyWear(int32 Amount) {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (Amount <= 0 || DurabilityConfig.MaxDurability <= 0 || DurabilityRuntimeReplicatedData.bBroken) {
        return;
    }

    DurabilityRuntimeReplicatedData.Current = FMath::Max(0, DurabilityRuntimeReplicatedData.Current - Amount);
    if (DurabilityRuntimeReplicatedData.Current <= 0) {
        DurabilityRuntimeReplicatedData.bBroken = true;
        UE_LOG(Myth, Log, TEXT("UDurabilityFragment: item %s broke (durability hit 0)."), *GetNameSafe(GetOwningItemInstance()));
        NotifyDurabilityBeat(EMythicItemDurabilityBeat::Broken);
        NotifyAffixesOfBrokenState(true);
        return;
    }

    if (!bLowWarningFired && DurabilityConfig.LowDurabilityWarnFraction > 0.0f) {
        const int32 WarnThreshold = FMath::CeilToInt(DurabilityConfig.MaxDurability * DurabilityConfig.LowDurabilityWarnFraction);
        if (DurabilityRuntimeReplicatedData.Current <= WarnThreshold) {
            bLowWarningFired = true;
            NotifyDurabilityBeat(EMythicItemDurabilityBeat::LowWarning);
        }
    }
}

bool UDurabilityFragment::ServerConsumeReceiptWear(const int64 Amount) {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority() || Amount <= 0
        || DurabilityConfig.MaxDurability <= 0) {
        return false;
    }
    // A recovered cumulative receipt is a semantic cost interval, while the
    // physical durability pool is saturating. If another valid authority cost
    // already broke the item, this interval is still consumed and must never
    // reappear after repair.
    if (DurabilityRuntimeReplicatedData.bBroken
        || DurabilityRuntimeReplicatedData.Current <= 0) {
        return true;
    }
    ServerApplyWear(static_cast<int32>(FMath::Min<int64>(Amount,
                                                        MAX_int32)));
    return true;
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
        NotifyAffixesOfBrokenState(false);
    }

    if (bLowWarningFired && DurabilityConfig.LowDurabilityWarnFraction > 0.0f) {
        const int32 WarnThreshold = FMath::CeilToInt(DurabilityConfig.MaxDurability * DurabilityConfig.LowDurabilityWarnFraction);
        if (DurabilityRuntimeReplicatedData.Current > WarnThreshold) {
            bLowWarningFired = false;
        }
    }
}

void UDurabilityFragment::NotifyDurabilityBeat(EMythicItemDurabilityBeat Beat) const {
    const UMythicItemInstance *Inst = GetOwningItemInstance();
    if (!Inst) {
        return;
    }
    UMythicInventoryComponent *Inv = Inst->GetInventoryComponent();
    if (!Inv) {
        return;
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(Inv->GetOwner());
    if (!PC) {
        return;
    }
    FText ItemName;
    if (const UItemDefinition *Def = Inst->GetItemDefinition()) {
        ItemName = Def->Name;
    }
    PC->ClientNotifyItemDurability(ItemName, Beat);
}

void UDurabilityFragment::NotifyAffixesOfBrokenState(bool bBroken) const {
    UMythicItemInstance *Inst = GetOwningItemInstance();
    if (!Inst) {
        return;
    }
    if (UMythicInventoryComponent *Inventory = Inst->GetInventoryComponent();
        Inventory && Inst->GetSlot() != INDEX_NONE) {
        // The application component re-enumerates the whole host here. Breaking disables base and socket/gem
        // snapshots together; repair restores the exact authoritative set and recomputes cross-item winners.
        Inventory->NotifyItemInstanceUpdated(Inst->GetSlot());
    }
}

bool UDurabilityFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) {
        return false;
    }
    const UDurabilityFragment *OtherFragment = Cast<UDurabilityFragment>(Other);
    if (!OtherFragment) {
        return false;
    }
    return DurabilityConfig.MaxDurability == OtherFragment->DurabilityConfig.MaxDurability
        && DurabilityRuntimeReplicatedData.Current == OtherFragment->DurabilityRuntimeReplicatedData.Current
        && DurabilityRuntimeReplicatedData.bBroken == OtherFragment->DurabilityRuntimeReplicatedData.bBroken;
}
