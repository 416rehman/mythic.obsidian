#include "WeaponBlockFragment.h"
#include "MythicWeaponActionContract.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/MythicWeaponDefenseAbility.h"
#include "GAS/Effects/MythicCombatBuffs.h"
#include "Input/MythicTags_Input.h"
UWeaponBlockFragment::UWeaponBlockFragment() { InputTag=INPUT_ACTION_ATTACK_ALT; GuardEffect=UMythicGE_Block::StaticClass(); }
bool UWeaponBlockFragment::HasUsableEffect() const {
    const auto* GE=GuardEffect.GetDefaultObject();
    return GE && GE->DurationPolicy==EGameplayEffectDurationType::Infinite && GE->Period.GetValueAtLevel(1)==0
        && GE->StackingType==EGameplayEffectStackingType::None;
}
void UWeaponBlockFragment::OnItemActivated(UMythicItemInstance* Item) {
    Super::OnItemActivated(Item);
    auto* NewASC=GetOwningAbilitySystemComponent();
    if (GrantedASC!=NewASC) { OnItemDeactivated(Item); GrantedASC=NewASC; }
    if (!NewASC || !NewASC->IsOwnerActorAuthoritative() || !HasUsableEffect()
        || !MythicWeaponAction::MatchesDefinition(this)) return;
    // Persisted input tags never redefine the native secondary role.
    InputTag=INPUT_ACTION_ATTACK_ALT;
    if (const auto* Spec=NewASC->FindAbilitySpecFromHandle(GrantedHandle); Spec && Spec->SourceObject.Get()==this) return;
    GrantedHandle=GrantItemAbility(NewASC,Item,UMythicWeaponDefenseAbility::StaticClass(),true);
}
void UWeaponBlockFragment::OnItemDeactivated(UMythicItemInstance* Item) {
    const auto Handle=GrantedHandle; auto* Old=GrantedASC.Get(); GrantedHandle={}; GrantedASC=nullptr;
    if (Old && Handle.IsValid() && Old->IsOwnerActorAuthoritative()) { Old->CancelAbilityHandle(Handle); Old->ClearAbility(Handle); }
    Super::OnItemDeactivated(Item);
}
bool UWeaponBlockFragment::CanBeStackedWith(const UItemFragment* Other) const {
    const auto* Block=Cast<UWeaponBlockFragment>(Other);
    return Block && GetClass()==Block->GetClass() && Super::CanBeStackedWith(Other) && GuardEffect==Block->GuardEffect;
}
void UWeaponBlockFragment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ThisClass,GuardEffect,COND_InitialOrOwner);
}
#if WITH_EDITOR
bool UWeaponBlockFragment::IsValidFragment(FText& Error) const {
    if (InputTag!=INPUT_ACTION_ATTACK_ALT || !HasUsableEffect()) {
        Error=FText::FromString(TEXT("Held defense requires canonical AttackAlt and one infinite, nonperiodic, nonstacking self effect.")); return false;
    }
    return Super::IsValidFragment(Error);
}
#endif
