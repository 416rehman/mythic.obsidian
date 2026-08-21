#include "Itemization/Inventory/Fragments/Passive/PerishableFragment.h"

#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Cooking/MythicColdStorageComponent.h"
#include "Itemization/Cooking/MythicFreshnessCore.h"
#include "GameFramework/Actor.h"

double UPerishableFragment::UtcNowSeconds() {
    return static_cast<double>(FDateTime::UtcNow().ToUnixTimestamp());
}

double UPerishableFragment::GetEffectiveAgedSeconds(double NowUtcSeconds) const {
    const FPerishableRuntimeReplicatedData &Data = PerishableRuntimeReplicatedData;
    if (Data.AnchorUtcSeconds <= 0.0) {
        return FMath::Max(0.0, Data.AgedBankedSeconds);
    }
    return MythicFreshness::AccrueAgedSeconds(Data.AgedBankedSeconds, Data.AnchorUtcSeconds, NowUtcSeconds, Data.CurrentPreservationMult);
}

float UPerishableFragment::GetFreshnessFraction(double NowUtcSeconds) const {
    return MythicFreshness::FreshnessFraction(GetEffectiveAgedSeconds(NowUtcSeconds), PerishableConfig.ShelfLifeSeconds);
}

bool UPerishableFragment::HasMutationAuthority() const {
    const AActor *Owner = GetOwningActor();
    return Owner && Owner->HasAuthority();
}

float UPerishableFragment::ResolvePreservationMultiplier(const UMythicInventoryComponent *Inventory) {
    const AActor *Owner = Inventory ? Inventory->GetOwner() : nullptr;
    if (Owner) {
        if (const UMythicColdStorageComponent *Cold = Owner->FindComponentByClass<UMythicColdStorageComponent>()) {
            return Cold->GetPreservationMultiplier();
        }
    }
    return 1.0f;
}

void UPerishableFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);
    FPerishableRuntimeReplicatedData &Data = PerishableRuntimeReplicatedData;
    if (Data.AnchorUtcSeconds <= 0.0) {
        Data.AnchorUtcSeconds = UtcNowSeconds();
        Data.CurrentPreservationMult = 1.0f;
    }
}

void UPerishableFragment::OnInventorySlotChanged(UMythicInventoryComponent *NewInventory, int32 NewSlot) {
    Super::OnInventorySlotChanged(NewInventory, NewSlot);
    if (!HasMutationAuthority()) {
        return;
    }
    FPerishableRuntimeReplicatedData &Data = PerishableRuntimeReplicatedData;
    const double Now = UtcNowSeconds();
    if (Data.AnchorUtcSeconds > 0.0) {
        Data.AgedBankedSeconds = MythicFreshness::AccrueAgedSeconds(Data.AgedBankedSeconds, Data.AnchorUtcSeconds, Now, Data.CurrentPreservationMult);
    }
    Data.AnchorUtcSeconds = Now;
    Data.CurrentPreservationMult = ResolvePreservationMultiplier(NewInventory);
}

bool UPerishableFragment::CanBeStackedWith(const UItemFragment *Other) const {
    const UPerishableFragment *OtherFrag = Cast<UPerishableFragment>(Other);
    if (!OtherFrag) {
        return false;
    }
    if (PerishableConfig.ShelfLifeSeconds != OtherFrag->PerishableConfig.ShelfLifeSeconds
        || PerishableConfig.BucketSeconds != OtherFrag->PerishableConfig.BucketSeconds) {
        return false;
    }
    const double Now = UtcNowSeconds();
    const FPerishableRuntimeReplicatedData &A = PerishableRuntimeReplicatedData;
    const FPerishableRuntimeReplicatedData &B = OtherFrag->PerishableRuntimeReplicatedData;
    const double AnchorA = A.AnchorUtcSeconds > 0.0 ? A.AnchorUtcSeconds : Now;
    const double AnchorB = B.AnchorUtcSeconds > 0.0 ? B.AnchorUtcSeconds : Now;
    const bool bStackable = MythicFreshness::CanStackPerishables(
        A.AgedBankedSeconds, AnchorA, A.CurrentPreservationMult,
        B.AgedBankedSeconds, AnchorB, B.CurrentPreservationMult,
        PerishableConfig.BucketSeconds, Now);
    return bStackable && Super::CanBeStackedWith(Other);
}
