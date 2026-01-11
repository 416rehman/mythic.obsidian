// 


#include "AttackFragment.h"

#include "GameModes/GameState/MythicGameState.h"

class AMythicGameState;

#if WITH_EDITOR
bool UAttackFragment::IsValidFragment(FText &OutErrorMessage) const {
    if (!this->AttackConfig.TriggerAbility) {
        OutErrorMessage = FText::FromString("AttackFragment: TriggerAbility is not set");
        return false;
    }

    if (!this->AttackConfig.AttackMontage) {
        OutErrorMessage = FText::FromString("AttackFragment: AttackMontage is not set");
        return false;
    }

    if (!this->AttackBuildData.DamageRollDefinition.IsValid(OutErrorMessage)) {
        return false;
    }

    return Super::IsValidFragment(OutErrorMessage);
}
#endif

void UAttackFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);

    UE_LOG(Myth, Log, TEXT("UAttackFragment::OnInstanced: Fragment=%s, Item=%s, InputTag=%s, TriggerAbility=%s"),
           *GetName(),
           *GetNameSafe(Instance),
           *InputTag.ToString(),
           *GetNameSafe(AttackConfig.TriggerAbility));

    // Roll the min/max damage values, make sure min is less than max
    this->AttackRuntimeReplicatedData.RolledDamageSpec = FRolledAttributeSpec(AttackBuildData.DamageAttribute, Instance->GetItemLevel(),
                                                                              AttackBuildData.DamageRollDefinition);
}

void UAttackFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    UE_LOG(Myth, Log, TEXT("UAttackFragment::OnItemActivated: Fragment=%s, Item=%s, InputTag=%s"),
           *GetName(),
           *GetNameSafe(ItemInstance),
           *InputTag.ToString());

    this->AttackRuntimeReplicatedData.ASC = this->GetOwningAbilitySystemComponent();
    if (!this->AttackRuntimeReplicatedData.ASC) {
        UE_LOG(Myth, Error, TEXT("UAttackFragment::OnItemActivated: ASC is null! Cannot grant ability."));
        return;
    }

    UE_LOG(Myth, Log, TEXT("  -> Got ASC: %s (Owner: %s)"),
           *GetNameSafe(AttackRuntimeReplicatedData.ASC),
           *GetNameSafe(AttackRuntimeReplicatedData.ASC->GetOwnerActor()));

    auto Roll = &this->AttackRuntimeReplicatedData.RolledDamageSpec;
    if (Roll->bIsApplied) {
        UE_LOG(Myth, Log, TEXT("  -> Damage already applied, skipping attribute modification"));
        return;
    }

    const auto ASC = this->AttackRuntimeReplicatedData.ASC;
    ASC->ApplyModToAttribute(Roll->Attribute, EGameplayModOp::AddBase, Roll->Value);
    Roll->bIsApplied = true;
    UE_LOG(Myth, Log, TEXT("  -> Applied damage attribute: %s = %f"), *Roll->Attribute.GetName(), Roll->Value);

    // Give the player the ability to attack
    UE_LOG(Myth, Log, TEXT("  -> Granting ability %s with InputTag %s"), *GetNameSafe(AttackConfig.TriggerAbility), *InputTag.ToString());
    this->AttackRuntimeReplicatedData.AbilityHandle = GrantItemAbility(ASC, ItemInstance, AttackConfig.TriggerAbility);

    if (!this->AttackRuntimeReplicatedData.AbilityHandle.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UAttackFragment::OnItemActivated: Failed to grant attack ability %s"), *GetNameSafe(AttackConfig.TriggerAbility));
    }
    else {
        UE_LOG(Myth, Log, TEXT("  -> SUCCESS! Granted ability"));

        // Log the granted ability's dynamic tags to verify the InputTag was applied
        if (FGameplayAbilitySpec *Spec = ASC->FindAbilitySpecFromHandle(this->AttackRuntimeReplicatedData.AbilityHandle)) {
            FGameplayTagContainer DynamicTags = Spec->GetDynamicSpecSourceTags();
            UE_LOG(Myth, Log, TEXT("  -> Granted ability DynamicSpecSourceTags: %s"), *DynamicTags.ToString());
        }
    }
}

void UAttackFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);

    UE_LOG(Myth, Log, TEXT("UAttackFragment::OnItemDeactivated: Fragment=%s, Item=%s"),
           *GetName(),
           *GetNameSafe(ItemInstance));

    auto ASC = this->AttackRuntimeReplicatedData.ASC;
    if (!ASC) {
        UE_LOG(Myth, Log, TEXT("  -> No ASC cached, nothing to clean up"));
        return;
    }

    const auto Roll = &this->AttackRuntimeReplicatedData.RolledDamageSpec;
    if (!Roll->bIsApplied) {
        UE_LOG(Myth, Log, TEXT("  -> Damage not applied, nothing to reverse"));
        return;
    }

    // Reverse the damage attribute
    ASC->ApplyModToAttribute(Roll->Attribute, EGameplayModOp::AddBase, -Roll->Value);
    Roll->bIsApplied = false;
    UE_LOG(Myth, Log, TEXT("  -> Reversed damage attribute"));

    // Remove the ability
    auto AbilityHandle = this->AttackRuntimeReplicatedData.AbilityHandle;
    if (AttackConfig.TriggerAbility && AbilityHandle.IsValid()) {
        ASC->ClearAbility(AbilityHandle);
        UE_LOG(Myth, Log, TEXT("  -> Cleared ability"));
    }

    // Remove the ability component
    this->AttackRuntimeReplicatedData.ASC = nullptr;
}


bool UAttackFragment::CanBeStackedWith(const UItemFragment *Other) const {
    auto OtherFragment = Cast<UAttackFragment>(Other);
    if (!OtherFragment) {
        return false;
    }

    auto Roll = &this->AttackRuntimeReplicatedData.RolledDamageSpec;
    auto OtherRoll = &OtherFragment->AttackRuntimeReplicatedData.RolledDamageSpec;

    auto OtherConfig = OtherFragment->AttackConfig;

    return Super::CanBeStackedWith(Other) &&
        Roll->Attribute == OtherRoll->Attribute &&
        Roll->Value == OtherRoll->Value &&
        AttackConfig.TriggerAbility == OtherConfig.TriggerAbility &&
        AttackConfig.AttackMontage == OtherConfig.AttackMontage;
}
