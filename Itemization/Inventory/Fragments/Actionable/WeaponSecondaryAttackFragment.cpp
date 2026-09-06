#include "WeaponSecondaryAttackFragment.h"
#include "Input/MythicTags_Input.h"
UWeaponSecondaryAttackFragment::UWeaponSecondaryAttackFragment() { InputTag=INPUT_ACTION_ATTACK_ALT; }

bool UWeaponSecondaryAttackFragment::HasValidHeavyTuning() const {
    return FMath::IsFinite(DamageMultiplier) && DamageMultiplier>=1.0f && DamageMultiplier<=3.0f
        && FMath::IsFinite(AttackRateMultiplier) && AttackRateMultiplier>=0.35f && AttackRateMultiplier<=1.0f;
}
bool UWeaponSecondaryAttackFragment::CanBeStackedWith(const UItemFragment* Other) const {
    const auto* Heavy=Cast<UWeaponSecondaryAttackFragment>(Other);
    return Heavy && HasValidHeavyTuning() && Heavy->HasValidHeavyTuning()
        && DamageMultiplier==Heavy->DamageMultiplier && AttackRateMultiplier==Heavy->AttackRateMultiplier
        && Super::CanBeStackedWith(Other);
}
void UWeaponSecondaryAttackFragment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ThisClass,DamageMultiplier,COND_InitialOrOwner);
    DOREPLIFETIME_CONDITION(ThisClass,AttackRateMultiplier,COND_InitialOrOwner);
}
#if WITH_EDITOR
bool UWeaponSecondaryAttackFragment::IsValidFragment(FText& OutErrorMessage) const {
    if (!HasValidHeavyTuning()) {
        OutErrorMessage=NSLOCTEXT("WeaponSecondary","InvalidTuning","Heavy damage must be finite 1–3 and attack rate finite 0.35–1.");
        return false;
    }
    return Super::IsValidFragment(OutErrorMessage);
}
#endif
