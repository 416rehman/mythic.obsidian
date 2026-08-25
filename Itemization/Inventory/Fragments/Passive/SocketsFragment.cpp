#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"

void USocketsFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);

    if (!Instance || !Instance->GetItemDefinition()) {
        return;
    }

    const FMythicSocketCountTable &Table =
        (CountTableOverride.Rules.Num() > 0) ? CountTableOverride : FMythicSocketMath::DefaultSocketCountTable();

    const FGameplayTag ItemType = Instance->GetItemDefinition()->ItemType;
    const int32 Rarity = static_cast<int32>(Instance->GetItemDefinition()->Rarity);
    const int32 Count = FMythicSocketMath::RollSocketCount(ItemType, Instance->GetItemLevel(), Rarity, Table, FMath::FRand());

    Sockets.Reset();
    Sockets.Reserve(Count);
    for (int32 i = 0; i < Count; ++i) {
        FMythicSocketSlot Slot;
        Slot.SocketColor = RolledSocketColor;
        Slot.bFilled = false;
        Sockets.Add(MoveTemp(Slot));
    }
}

int32 USocketsFragment::GetFilledSocketCount() const {
    int32 N = 0;
    for (const FMythicSocketSlot &S : Sockets) {
        if (S.bFilled) {
            ++N;
        }
    }
    return N;
}

void USocketsFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    AActor *Owner = ItemInstance ? ItemInstance->GetInventoryOwner() : nullptr;
    if (!Owner) {
        UE_LOG(Myth, Error, TEXT("SocketsFragment::OnItemActivated: invalid inventory owner."));
        return;
    }
    UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("SocketsFragment::OnItemActivated: owner has no ASC."));
        return;
    }

    ActiveASC = ASC;
    ApplyToActiveASC();
}

void USocketsFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);

    if (!ActiveASC) {
        return;
    }
    RemoveFromActiveASC();
    ActiveASC = nullptr;
}

void USocketsFragment::ApplyToActiveASC() {
    if (!ActiveASC) {
        return;
    }

    for (FMythicSocketSlot &Slot : Sockets) {
        if (Slot.bFilled && Slot.SocketedAffixes.Num() > 0) {
            UAffixesFragment::ApplyAffixes(ActiveASC, Slot.SocketedAffixes);
        }
    }
}

void USocketsFragment::RemoveFromActiveASC() {
    if (!ActiveASC) {
        return;
    }
    for (int32 i = Sockets.Num() - 1; i >= 0; --i) {
        FMythicSocketSlot &Slot = Sockets[i];
        if (Slot.bFilled && Slot.SocketedAffixes.Num() > 0) {
            UAffixesFragment::RemoveAffixes(ActiveASC, Slot.SocketedAffixes);
        }
    }
}

void USocketsFragment::ServerSocketGem(int32 SocketIndex, const FGameplayTag &GemType, const TArray<FRolledAffix> &GemAffixes) {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (!Sockets.IsValidIndex(SocketIndex) || Sockets[SocketIndex].bFilled) {
        UE_LOG(Myth, Warning, TEXT("SocketsFragment::ServerSocketGem: invalid or already-filled socket %d."), SocketIndex);
        return;
    }

    const bool bWasActive = (ActiveASC != nullptr);
    if (bWasActive) {
        RemoveFromActiveASC();
    }

    FMythicSocketSlot &Slot = Sockets[SocketIndex];
    Slot.SocketedGemType = GemType;
    Slot.SocketedAffixes = GemAffixes;
    for (FRolledAffix &A : Slot.SocketedAffixes) {
        A.bIsApplied = false;
    }
    Slot.bFilled = true;

    if (bWasActive) {
        ApplyToActiveASC();
    }
}

FGameplayTag USocketsFragment::ServerUnsocketGem(int32 SocketIndex) {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return FGameplayTag();
    }
    if (!Sockets.IsValidIndex(SocketIndex) || !Sockets[SocketIndex].bFilled) {
        return FGameplayTag();
    }

    const bool bWasActive = (ActiveASC != nullptr);
    if (bWasActive) {
        RemoveFromActiveASC();
    }

    FMythicSocketSlot &Slot = Sockets[SocketIndex];
    const FGameplayTag Removed = Slot.SocketedGemType;
    Slot.SocketedGemType = FGameplayTag();
    Slot.SocketedAffixes.Reset();
    Slot.bFilled = false;

    if (bWasActive) {
        ApplyToActiveASC();
    }
    return Removed;
}

bool USocketsFragment::ServerAddSocket() {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return false;
    }

    const FMythicSocketCountTable &Table =
        (CountTableOverride.Rules.Num() > 0) ? CountTableOverride : FMythicSocketMath::DefaultSocketCountTable();
    const int32 HardCap = FMath::Max(0, Table.HardCap);
    if (Sockets.Num() >= HardCap) {
        return false;
    }

    FMythicSocketSlot Slot;
    Slot.SocketColor = RolledSocketColor;
    Slot.bFilled = false;
    Sockets.Add(MoveTemp(Slot));
    return true;
}

bool USocketsFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) {
        return false;
    }
    const USocketsFragment *OtherSockets = Cast<USocketsFragment>(Other);
    if (!OtherSockets) {
        return false;
    }
    if (Sockets.Num() != OtherSockets->Sockets.Num()) {
        return false;
    }
    for (int32 i = 0; i < Sockets.Num(); ++i) {
        const FMythicSocketSlot &A = Sockets[i];
        const FMythicSocketSlot &B = OtherSockets->Sockets[i];
        if (A.bFilled != B.bFilled || A.SocketColor != B.SocketColor || A.SocketedGemType != B.SocketedGemType) {
            return false;
        }
    }
    return true;
}
