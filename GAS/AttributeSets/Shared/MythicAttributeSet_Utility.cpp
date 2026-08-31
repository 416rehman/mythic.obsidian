

#include "MythicAttributeSet_Utility.h"

#include "Settings/MythicCombatSettings.h"
#include "Net/UnrealNetwork.h"
#include "World/Harvesting/MythicHarvestSettings.h"

namespace {

void ClampHarvestWorkAttribute(float &InOutValue) {
    double ClampedValue = 0.0;
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    InOutValue = Settings
        && Settings->TryClampHarvestWorkMultiplier(
            static_cast<double>(InOutValue), ClampedValue)
        ? static_cast<float>(ClampedValue)
        : 1.0f;
}

} // namespace

UMythicAttributeSet_Utility::UMythicAttributeSet_Utility() {
    InitMaxStamina(100.0f);
    InitCurrentStamina(100.0f);
    InitStaminaRegenRate(10.0f);

    InitMaxCooldownReduction(0.60f);
    InitMovementSpeedMultiplier(1.0f);
    InitItemRarityFind(1.0f);
    InitItemQuantityFind(1.0f);
    InitHarvestWorkMultiplier(1.0f);
}

TConstArrayView<FMythicBoundedAttributePair>
UMythicAttributeSet_Utility::GetBoundedAttributePairs() const {
    static const FMythicBoundedAttributePair Pairs[] = {
        {GetCurrentStaminaAttribute(), GetMaxStaminaAttribute(), 0.0f,
         0.0f, EMythicAttributeBaseOverflowPolicy::Discard},
        // Cooldown reduction is an investment stat: over-cap base investment
        // remains stored so later cap increases can expose it again.
        {GetCooldownReductionAttribute(), GetMaxCooldownReductionAttribute(),
         0.0f, 0.0f, EMythicAttributeBaseOverflowPolicy::Preserve}
    };
    return Pairs;
}

bool UMythicAttributeSet_Utility::IsReductionFractionAttribute(const FGameplayAttribute &Attribute) {
    return Attribute == GetStaminaCostReductionAttribute() || Attribute == GetCooldownReductionAttribute();
}

void UMythicAttributeSet_Utility::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetStaminaCostReductionAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
    // A full stop is the crowd-control path's job (Stunned and Frozen disable movement outright), so stacked slows
    // bottom out at the authored floor instead: never negative, and never an accidental standstill.
    else if (Attribute == GetMovementSpeedMultiplierAttribute()) {
        NewValue = FMath::Max(MythicCombat::GetMinSpeedScale(), NewValue);
    }
    else if (Attribute == GetHarvestWorkMultiplierAttribute()) {
        ClampHarvestWorkAttribute(NewValue);
    }
}

void UMythicAttributeSet_Utility::PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const {
    Super::PreAttributeBaseChange(Attribute, NewValue);

    if (Attribute == GetHarvestWorkMultiplierAttribute()) {
        ClampHarvestWorkAttribute(NewValue);
    }
}

void UMythicAttributeSet_Utility::OnRep_Resolve(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, Resolve, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_MaxStamina(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, MaxStamina, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_CurrentStamina(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, CurrentStamina, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_StaminaRegenRate(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, StaminaRegenRate, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_StaminaCostReduction(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, StaminaCostReduction, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_CooldownReduction(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, CooldownReduction, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_MaxCooldownReduction(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, MaxCooldownReduction, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_ProficiencyXPBonus(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, ProficiencyXPBonus, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_MovementSpeedMultiplier(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, MovementSpeedMultiplier, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_ItemRarityFind(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, ItemRarityFind, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_ItemQuantityFind(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, ItemQuantityFind, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_HarvestWorkMultiplier(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, HarvestWorkMultiplier, OldValue);
}

void UMythicAttributeSet_Utility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, Resolve, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, MaxStamina, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, CurrentStamina, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, StaminaRegenRate, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, StaminaCostReduction, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, CooldownReduction, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, MaxCooldownReduction, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, ProficiencyXPBonus, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, MovementSpeedMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, ItemRarityFind, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, ItemQuantityFind, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, HarvestWorkMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
}
