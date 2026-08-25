
#include "MythicSkillComponent.h"

#include "MythicSkillDefinition.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Mythic/Input/MythicTags_Input.h"
#include "Mythic/Mythic.h"
#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Progression/MythicAchievementComponent.h"
#include "Mythic/Progression/MythicUnlockComponent.h"
#include "Net/UnrealNetwork.h"

UMythicSkillComponent::UMythicSkillComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    SlotInputTags = {INPUT_ACTION_SKILL_Q.GetTag(), INPUT_ACTION_SKILL_E.GetTag()};
}

void UMythicSkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, EquippedSkills, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicSkillComponent, UnlockedSlots, COND_OwnerOnly);
}

void UMythicSkillComponent::BeginPlay() {
    Super::BeginPlay();

    const AActor *Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) {
        SizeSlots();
    }
}

void UMythicSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    const AActor *Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) {
        for (int32 SlotIndex = 0; SlotIndex < GrantedHandles.Num(); SlotIndex++) {
            ClearSlotAbility(SlotIndex);
        }
    }
    Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent *UMythicSkillComponent::ResolveASC() const {
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner())) {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}

void UMythicSkillComponent::SizeSlots() {
    const int32 Slots = FMath::Max(MaxSlots, 1);
    // Lowering MaxSlots drops slots: hand their abilities back before the entries go.
    for (int32 SlotIndex = Slots; SlotIndex < GrantedHandles.Num(); SlotIndex++) {
        ClearSlotAbility(SlotIndex);
    }
    if (EquippedSkills.Num() != Slots) {
        EquippedSkills.SetNum(Slots);
    }
    if (GrantedHandles.Num() != Slots) {
        GrantedHandles.SetNum(Slots);
    }
}

void UMythicSkillComponent::ClearSlotAbility(int32 SlotIndex) {
    if (!GrantedHandles.IsValidIndex(SlotIndex) || !GrantedHandles[SlotIndex].IsValid()) {
        return;
    }
    if (UAbilitySystemComponent *ASC = ResolveASC()) {
        ASC->ClearAbility(GrantedHandles[SlotIndex]);
    }
    GrantedHandles[SlotIndex] = FGameplayAbilitySpecHandle();
}

FGameplayTag UMythicSkillComponent::GetSlotInputTag(int32 SlotIndex) const {
    return SlotInputTags.IsValidIndex(SlotIndex) ? SlotInputTags[SlotIndex] : FGameplayTag();
}

FGameplayAbilitySpecHandle UMythicSkillComponent::GrantSlotAbility(UAbilitySystemComponent *ASC, UMythicSkillDefinition *Skill, int32 SlotIndex) {
    FGameplayAbilitySpec Spec(Skill->Ability, 1, INDEX_NONE, Skill);

    // The slot IS the key binding: AbilityInputTagPressed activates whatever spec carries the pressed tag.
    const FGameplayTag InputTag = GetSlotInputTag(SlotIndex);
    if (InputTag.IsValid()) {
        Spec.GetDynamicSpecSourceTags().AddTag(InputTag);
    }
    else {
        UE_LOG(Myth, Warning, TEXT("Skills: slot %d has no input tag — '%s' is granted but no key will fire it."), SlotIndex,
               *GetNameSafe(Skill));
    }

    return ASC->GiveAbility(Spec);
}

bool UMythicSkillComponent::IsSlotUnlocked(int32 SlotIndex) const {
    return SlotIndex >= 0 && SlotIndex < GetUnlockedSlots();
}

bool UMythicSkillComponent::IsSkillUnlocked(const UMythicSkillDefinition *Skill) const {
    if (!Skill) {
        return false;
    }
    if (!Skill->RequiredTag.IsValid()) {
        return true;
    }
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        if (const UMythicAchievementComponent *Achievements = PS->GetAchievementComponent()) {
            if (Achievements->IsAchievementUnlocked(Skill->RequiredTag)) {
                return true;
            }
        }
        if (const UMythicUnlockComponent *Unlocks = PS->GetUnlockComponent()) {
            if (Unlocks->HasUnlockTag(Skill->RequiredTag)) {
                return true;
            }
        }
        if (const UMythicNarrativeStateComponent *Narrative = PS->GetNarrativeState()) {
            if (Narrative->HasStoryTag(Skill->RequiredTag)) {
                return true;
            }
        }
    }
    const UAbilitySystemComponent *ASC = ResolveASC();
    return ASC && ASC->HasMatchingGameplayTag(Skill->RequiredTag);
}

UMythicSkillDefinition *UMythicSkillComponent::GetSkillInSlot(int32 SlotIndex) const {
    return EquippedSkills.IsValidIndex(SlotIndex) ? EquippedSkills[SlotIndex].LoadSynchronous() : nullptr;
}

void UMythicSkillComponent::GrantSlot() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (UnlockedSlots >= MaxSlots) {
        UE_LOG(Myth, Verbose, TEXT("Skills: slot grant ignored — all %d slots are already open."), MaxSlots);
        return;
    }
    UnlockedSlots++;
    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::RestoreSkills(const TArray<TSoftObjectPtr<UMythicSkillDefinition>> &SavedSkills, int32 SavedUnlockedSlots) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    UnlockedSlots = FMath::Clamp(SavedUnlockedSlots, 1, FMath::Max(MaxSlots, 1));
    SizeSlots();

    for (int32 SlotIndex = 0; SlotIndex < EquippedSkills.Num(); SlotIndex++) {
        ClearSlotAbility(SlotIndex);
        EquippedSkills[SlotIndex].Reset();
    }

    UAbilitySystemComponent *ASC = ResolveASC();
    for (int32 SlotIndex = 0; SlotIndex < SavedSkills.Num(); SlotIndex++) {
        const FSoftObjectPath SavedPath = SavedSkills[SlotIndex].ToSoftObjectPath();
        if (SavedPath.IsNull()) {
            continue;
        }
        if (!EquippedSkills.IsValidIndex(SlotIndex) || !IsSlotUnlocked(SlotIndex)) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped saved skill '%s' — slot %d is no longer open (%d of %d)."),
                   *SavedPath.ToString(), SlotIndex, GetUnlockedSlots(), MaxSlots);
            continue;
        }

        bool bAlreadyBound = false;
        for (int32 Other = 0; Other < SlotIndex && !bAlreadyBound; Other++) {
            bAlreadyBound = EquippedSkills[Other].ToSoftObjectPath() == SavedPath;
        }
        if (bAlreadyBound) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped saved skill '%s' — already bound to an earlier slot."), *SavedPath.ToString());
            continue;
        }

        UMythicSkillDefinition *Skill = SavedSkills[SlotIndex].LoadSynchronous();
        if (!Skill || !Skill->HasPayload()) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped saved skill '%s' — it no longer resolves to a skill with an ability."),
                   *SavedPath.ToString());
            continue;
        }
        if (!IsSkillUnlocked(Skill)) {
            UE_LOG(Myth, Warning, TEXT("Skills: dropped saved skill '%s' — %s is no longer earned."), *GetNameSafe(Skill),
                   *Skill->RequiredTag.ToString());
            continue;
        }
        if (!ASC) {
            UE_LOG(Myth, Error, TEXT("Skills: restore failed — %s has no ability system."), *GetNameSafe(GetOwner()));
            break;
        }

        const FGameplayAbilitySpecHandle Handle = GrantSlotAbility(ASC, Skill, SlotIndex);
        if (!Handle.IsValid()) {
            UE_LOG(Myth, Error, TEXT("Skills: restore failed — could not grant '%s'."), *GetNameSafe(Skill));
            continue;
        }
        GrantedHandles[SlotIndex] = Handle;
        EquippedSkills[SlotIndex] = Skill;
    }

    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::ServerEquipSkill_Implementation(int32 SlotIndex, UMythicSkillDefinition *Skill) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    SizeSlots();

    if (!EquippedSkills.IsValidIndex(SlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — slot %d is outside 0..%d."), SlotIndex, EquippedSkills.Num() - 1);
        return;
    }
    if (!IsSlotUnlocked(SlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — slot %d is still locked (%d of %d open)."), SlotIndex,
               GetUnlockedSlots(), MaxSlots);
        return;
    }
    if (!Skill) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — no skill given for slot %d."), SlotIndex);
        return;
    }
    if (!Skill->HasPayload()) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — '%s' has no ability."), *GetNameSafe(Skill));
        return;
    }
    if (!IsSkillUnlocked(Skill)) {
        UE_LOG(Myth, Warning, TEXT("Skills: equip refused — '%s' needs %s, which this player has not earned."),
               *GetNameSafe(Skill), *Skill->RequiredTag.ToString());
        return;
    }

    // Compare by path: a skill can be bound while its asset is unloaded, and a null Get() would read as a free slot.
    const FSoftObjectPath SkillPath(Skill);
    if (EquippedSkills[SlotIndex].ToSoftObjectPath() == SkillPath) {
        return;
    }
    for (int32 Other = 0; Other < EquippedSkills.Num(); Other++) {
        if (Other != SlotIndex && EquippedSkills[Other].ToSoftObjectPath() == SkillPath) {
            UE_LOG(Myth, Warning, TEXT("Skills: equip refused — '%s' is already bound to slot %d."), *GetNameSafe(Skill), Other);
            return;
        }
    }

    UAbilitySystemComponent *ASC = ResolveASC();
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("Skills: equip failed — %s has no ability system."), *GetNameSafe(GetOwner()));
        return;
    }

    ClearSlotAbility(SlotIndex);

    const FGameplayAbilitySpecHandle Handle = GrantSlotAbility(ASC, Skill, SlotIndex);
    if (!Handle.IsValid()) {
        EquippedSkills[SlotIndex].Reset();
        OnSkillsChanged.Broadcast();
        UE_LOG(Myth, Error, TEXT("Skills: equip failed — could not grant '%s'."), *GetNameSafe(Skill));
        return;
    }

    GrantedHandles[SlotIndex] = Handle;
    EquippedSkills[SlotIndex] = Skill;
    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::ServerUnequipSkill_Implementation(int32 SlotIndex) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !EquippedSkills.IsValidIndex(SlotIndex)) {
        return;
    }
    if (EquippedSkills[SlotIndex].IsNull()) {
        return;
    }

    ClearSlotAbility(SlotIndex);
    EquippedSkills[SlotIndex].Reset();
    OnSkillsChanged.Broadcast();
}

void UMythicSkillComponent::OnRep_Skills() {
    OnSkillsChanged.Broadcast();
}
