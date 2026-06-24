//

#include "DurabilityFragment.h"

#include "Mythic.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Player/MythicPlayerController.h"

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
        NotifyDurabilityBeat(EMythicItemDurabilityBeat::Broken);
        return;
    }

    // One-shot low-durability warning the first time this wear crosses to/below the designer-tunable fraction. Latched
    // so a worn item doesn't re-warn on every subsequent hit; ServerRepair re-arms it once repaired back above.
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
    }

    // Re-arm the one-shot low warning once durability climbs back above the threshold, so a later wear-down warns again.
    if (bLowWarningFired && DurabilityConfig.LowDurabilityWarnFraction > 0.0f) {
        const int32 WarnThreshold = FMath::CeilToInt(DurabilityConfig.MaxDurability * DurabilityConfig.LowDurabilityWarnFraction);
        if (DurabilityRuntimeReplicatedData.Current > WarnThreshold) {
            bLowWarningFired = false;
        }
    }

    // NOTE: the "repaired" recovery callout is fired by the repair INITIATOR (today the conversion/repair station),
    // not here. ServerRepair's only caller repairs a DETACHED instance (held mid-routing), so the fragment cannot
    // resolve the owning player from it — but the station knows the instigator. See UConversionStationComponent::
    // ProduceAndRoute. (Wear/break warnings DO fire here: those run while the item is equipped, so the fragment can
    // resolve the player.)
}

void UDurabilityFragment::NotifyDurabilityBeat(EMythicItemDurabilityBeat Beat) const {
    const UMythicItemInstance *Inst = GetOwningItemInstance();
    if (!Inst) {
        return;
    }
    // The player's inventory component is a subobject of the owning AMythicPlayerController (the PlayerState delegates
    // its IInventoryProviderInterface to it), so the inventory's owner IS the controller to notify. Container /
    // merchant / station / world-drop inventories are owned by non-PC actors, so this cleanly no-ops for them
    // (mirrors the loot-pickup callout's Cast<AMythicPlayerController>(GetOwner()) guard).
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

bool UDurabilityFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) {
        return false;
    }
    const UDurabilityFragment *OtherFragment = Cast<UDurabilityFragment>(Other);
    if (!OtherFragment) {
        return false;
    }
    // Identical wear state only — max, current durability, and the broken latch must all match, or a stack-merge would
    // discard one instance's durability (AddToAnySlot keeps the survivor's fragment and Destroy()s the incoming one).
    return DurabilityConfig.MaxDurability == OtherFragment->DurabilityConfig.MaxDurability
        && DurabilityRuntimeReplicatedData.Current == OtherFragment->DurabilityRuntimeReplicatedData.Current
        && DurabilityRuntimeReplicatedData.bBroken == OtherFragment->DurabilityRuntimeReplicatedData.bBroken;
}
