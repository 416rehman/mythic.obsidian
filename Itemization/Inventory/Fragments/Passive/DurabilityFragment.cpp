
#include "DurabilityFragment.h"

#include "Mythic.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
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
    if (const UAffixesFragment *Affixes = Inst->GetFragment<UAffixesFragment>()) {
        const_cast<UAffixesFragment *>(Affixes)->OnDurabilityBrokenStateChanged(bBroken);
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
