
#include "MythicRuneComponent.h"

#include "MythicRuneDefinition.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Mythic/Mythic.h"
#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Progression/MythicAchievementComponent.h"
#include "Mythic/Progression/MythicUnlockComponent.h"
#include "Net/UnrealNetwork.h"

UMythicRuneComponent::UMythicRuneComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicRuneComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicRuneComponent, EquippedRunes, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicRuneComponent, UnlockedSlots, COND_OwnerOnly);
}

void UMythicRuneComponent::BeginPlay() {
    Super::BeginPlay();

    const AActor *Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) {
        SizeSlots();
    }
}

void UMythicRuneComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    const AActor *Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) {
        for (int32 SlotIndex = 0; SlotIndex < GrantedHandles.Num(); SlotIndex++) {
            ClearSlotAbility(SlotIndex);
        }
    }
    Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent *UMythicRuneComponent::ResolveASC() const {
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner())) {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}

void UMythicRuneComponent::SizeSlots() {
    const int32 Slots = FMath::Max(MaxSlots, 1);
    // Lowering MaxSlots drops sockets: hand their abilities back before the entries go.
    for (int32 SlotIndex = Slots; SlotIndex < GrantedHandles.Num(); SlotIndex++) {
        ClearSlotAbility(SlotIndex);
    }
    if (EquippedRunes.Num() != Slots) {
        EquippedRunes.SetNum(Slots);
    }
    if (GrantedHandles.Num() != Slots) {
        GrantedHandles.SetNum(Slots);
    }
}

void UMythicRuneComponent::ClearSlotAbility(int32 SlotIndex) {
    if (!GrantedHandles.IsValidIndex(SlotIndex) || !GrantedHandles[SlotIndex].IsValid()) {
        return;
    }
    if (UAbilitySystemComponent *ASC = ResolveASC()) {
        ASC->ClearAbility(GrantedHandles[SlotIndex]);
    }
    GrantedHandles[SlotIndex] = FGameplayAbilitySpecHandle();
}

bool UMythicRuneComponent::IsSlotUnlocked(int32 SlotIndex) const {
    return SlotIndex >= 0 && SlotIndex < GetUnlockedSlots();
}

bool UMythicRuneComponent::IsRuneUnlocked(const UMythicRuneDefinition *Rune) const {
    if (!Rune) {
        return false;
    }
    if (!Rune->RequiredTag.IsValid()) {
        return true;
    }
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        if (const UMythicAchievementComponent *Achievements = PS->GetAchievementComponent()) {
            if (Achievements->IsAchievementUnlocked(Rune->RequiredTag)) {
                return true;
            }
        }
        if (const UMythicUnlockComponent *Unlocks = PS->GetUnlockComponent()) {
            if (Unlocks->HasUnlockTag(Rune->RequiredTag)) {
                return true;
            }
        }
        if (const UMythicNarrativeStateComponent *Narrative = PS->GetNarrativeState()) {
            if (Narrative->HasStoryTag(Rune->RequiredTag)) {
                return true;
            }
        }
    }
    const UAbilitySystemComponent *ASC = ResolveASC();
    return ASC && ASC->HasMatchingGameplayTag(Rune->RequiredTag);
}

UMythicRuneDefinition *UMythicRuneComponent::GetRuneInSlot(int32 SlotIndex) const {
    return EquippedRunes.IsValidIndex(SlotIndex) ? EquippedRunes[SlotIndex].LoadSynchronous() : nullptr;
}

void UMythicRuneComponent::GrantSlot() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (UnlockedSlots >= MaxSlots) {
        UE_LOG(Myth, Verbose, TEXT("Runes: slot grant ignored — all %d sockets are already open."), MaxSlots);
        return;
    }
    UnlockedSlots++;
    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::RestoreRunes(const TArray<TSoftObjectPtr<UMythicRuneDefinition>> &SavedRunes, int32 SavedUnlockedSlots) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    UnlockedSlots = FMath::Clamp(SavedUnlockedSlots, 1, FMath::Max(MaxSlots, 1));
    SizeSlots();

    for (int32 SlotIndex = 0; SlotIndex < EquippedRunes.Num(); SlotIndex++) {
        ClearSlotAbility(SlotIndex);
        EquippedRunes[SlotIndex].Reset();
    }

    UAbilitySystemComponent *ASC = ResolveASC();
    for (int32 SlotIndex = 0; SlotIndex < SavedRunes.Num(); SlotIndex++) {
        const FSoftObjectPath SavedPath = SavedRunes[SlotIndex].ToSoftObjectPath();
        if (SavedPath.IsNull()) {
            continue;
        }
        if (!EquippedRunes.IsValidIndex(SlotIndex) || !IsSlotUnlocked(SlotIndex)) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped saved rune '%s' — slot %d is no longer open (%d of %d)."),
                   *SavedPath.ToString(), SlotIndex, GetUnlockedSlots(), MaxSlots);
            continue;
        }

        bool bAlreadyWorn = false;
        for (int32 Other = 0; Other < SlotIndex && !bAlreadyWorn; Other++) {
            bAlreadyWorn = EquippedRunes[Other].ToSoftObjectPath() == SavedPath;
        }
        if (bAlreadyWorn) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped saved rune '%s' — already worn in an earlier slot."), *SavedPath.ToString());
            continue;
        }

        UMythicRuneDefinition *Rune = SavedRunes[SlotIndex].LoadSynchronous();
        if (!Rune || !Rune->HasPayload()) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped saved rune '%s' — it no longer resolves to a rune with an ability."),
                   *SavedPath.ToString());
            continue;
        }
        if (!IsRuneUnlocked(Rune)) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped saved rune '%s' — %s is no longer earned."), *GetNameSafe(Rune),
                   *Rune->RequiredTag.ToString());
            continue;
        }
        if (!ASC) {
            UE_LOG(Myth, Error, TEXT("Runes: restore failed — %s has no ability system."), *GetNameSafe(GetOwner()));
            break;
        }

        const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(Rune->Ability, 1, INDEX_NONE, Rune));
        if (!Handle.IsValid()) {
            UE_LOG(Myth, Error, TEXT("Runes: restore failed — could not grant '%s'."), *GetNameSafe(Rune));
            continue;
        }
        GrantedHandles[SlotIndex] = Handle;
        EquippedRunes[SlotIndex] = Rune;
    }

    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::ServerEquipRune_Implementation(int32 SlotIndex, UMythicRuneDefinition *Rune) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    SizeSlots();

    if (!EquippedRunes.IsValidIndex(SlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("Runes: equip refused — slot %d is outside 0..%d."), SlotIndex, EquippedRunes.Num() - 1);
        return;
    }
    if (!IsSlotUnlocked(SlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("Runes: equip refused — slot %d is still locked (%d of %d open)."), SlotIndex,
               GetUnlockedSlots(), MaxSlots);
        return;
    }
    if (!Rune) {
        UE_LOG(Myth, Warning, TEXT("Runes: equip refused — no rune given for slot %d."), SlotIndex);
        return;
    }
    if (!Rune->HasPayload()) {
        UE_LOG(Myth, Warning, TEXT("Runes: equip refused — '%s' has no ability."), *GetNameSafe(Rune));
        return;
    }
    if (!IsRuneUnlocked(Rune)) {
        UE_LOG(Myth, Warning, TEXT("Runes: equip refused — '%s' needs %s, which this player has not earned."),
               *GetNameSafe(Rune), *Rune->RequiredTag.ToString());
        return;
    }

    // Compare by path: a rune can be worn while its asset is unloaded, and a null Get() would read as a free slot.
    const FSoftObjectPath RunePath(Rune);
    if (EquippedRunes[SlotIndex].ToSoftObjectPath() == RunePath) {
        return;
    }
    for (int32 Other = 0; Other < EquippedRunes.Num(); Other++) {
        if (Other != SlotIndex && EquippedRunes[Other].ToSoftObjectPath() == RunePath) {
            UE_LOG(Myth, Warning, TEXT("Runes: equip refused — '%s' is already worn in slot %d."), *GetNameSafe(Rune), Other);
            return;
        }
    }

    UAbilitySystemComponent *ASC = ResolveASC();
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("Runes: equip failed — %s has no ability system."), *GetNameSafe(GetOwner()));
        return;
    }

    ClearSlotAbility(SlotIndex);

    const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(Rune->Ability, 1, INDEX_NONE, Rune));
    if (!Handle.IsValid()) {
        EquippedRunes[SlotIndex].Reset();
        OnRunesChanged.Broadcast();
        UE_LOG(Myth, Error, TEXT("Runes: equip failed — could not grant '%s'."), *GetNameSafe(Rune));
        return;
    }

    GrantedHandles[SlotIndex] = Handle;
    EquippedRunes[SlotIndex] = Rune;
    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::ServerUnequipRune_Implementation(int32 SlotIndex) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !EquippedRunes.IsValidIndex(SlotIndex)) {
        return;
    }
    if (EquippedRunes[SlotIndex].IsNull()) {
        return;
    }

    ClearSlotAbility(SlotIndex);
    EquippedRunes[SlotIndex].Reset();
    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::OnRep_Runes() {
    OnRunesChanged.Broadcast();
}
