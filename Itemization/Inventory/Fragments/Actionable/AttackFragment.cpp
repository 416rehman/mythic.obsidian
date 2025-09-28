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

    if (!this->AttackConfig.AttackBeginEventTag.IsValid()) {
        OutErrorMessage = FText::FromString("AttackFragment: AttackBeginEventTag is not set");
        return false;
    }

    if (!this->AttackConfig.AttackEndEventTag.IsValid()) {
        OutErrorMessage = FText::FromString("AttackFragment: AttackEndEventTag is not set");
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

    // Roll the min/max damage values, make sure min is less than max
    this->AttackRuntimeReplicatedData.RolledDamageSpec = FRolledAttributeSpec(AttackBuildData.DamageAttribute, Instance->GetItemLevel(),
                                                                              AttackBuildData.DamageRollDefinition);
}

void UAttackFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    this->AttackRuntimeReplicatedData.ASC = this->GetOwningAbilitySystemComponent();
    if (!this->AttackRuntimeReplicatedData.ASC) {
        UE_LOG(Myth, Error, TEXT("UAttackFragment::OnItemActivated: ASC is null"));
        return;
    }

    auto Roll = &this->AttackRuntimeReplicatedData.RolledDamageSpec;
    const auto ASC = this->AttackRuntimeReplicatedData.ASC;
    ASC->ApplyModToAttribute(Roll->Attribute, EGameplayModOp::AddBase, Roll->Value);
    Roll->bIsApplied = true;

    // Give the player the ability to attack
    if (AttackConfig.TriggerAbility) {
        this->AttackRuntimeReplicatedData.AbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(AttackConfig.TriggerAbility, 1, INDEX_NONE, this));
    }
}

void UAttackFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);
    auto ASC = this->AttackRuntimeReplicatedData.ASC;
    if (!ASC) {
        return;
    }

    const auto Roll = &this->AttackRuntimeReplicatedData.RolledDamageSpec;
    if (!Roll->bIsApplied) {
        return;
    }

    // Reverse the damage attribute
    ASC->ApplyModToAttribute(Roll->Attribute, EGameplayModOp::AddBase, -Roll->Value);

    // Remove the ability
    auto AbilityHandle = this->AttackRuntimeReplicatedData.AbilityHandle;
    if (AttackConfig.TriggerAbility && AbilityHandle.IsValid()) {
        ASC->ClearAbility(AbilityHandle);
    }

    // Remove the ability component
    ASC = nullptr;
}

void UAttackFragment::TriggerAbilityWithEvent(FGameplayTag Tag) {
    auto ASC = this->AttackRuntimeReplicatedData.ASC;

    // Payload to pass to the ability to be triggered
    auto Payload = FGameplayEventData();
    Payload.EventTag = Tag;
    Payload.Instigator = this->GetOwningActor();
    Payload.OptionalObject = this; // OptionalObject is the fragment itself.

    // Pass current item instance to the ability system component
    ASC->TriggerAbilityFromGameplayEvent(this->AttackRuntimeReplicatedData.AbilityHandle, nullptr, Tag, &Payload, *ASC);
}

void UAttackFragment::OnClientActionBegin(UMythicItemInstance *ItemInst) {
    Super::OnClientActionBegin(ItemInst);

    // Trigger the begin event of the attack ability via the event tag
    if (AttackConfig.AttackBeginEventTag.IsValid()) {
        TriggerAbilityWithEvent(AttackConfig.AttackBeginEventTag);
    }
}

void UAttackFragment::OnClientActionEnd(UMythicItemInstance *ItemInst) {
    Super::OnClientActionEnd(ItemInst);

    // Trigger the end event of the attack ability via the event tag
    if (AttackConfig.AttackEndEventTag.IsValid()) {
        TriggerAbilityWithEvent(AttackConfig.AttackEndEventTag);
    }
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
        AttackConfig.AttackBeginEventTag == OtherConfig.AttackBeginEventTag &&
        AttackConfig.AttackEndEventTag == OtherConfig.AttackEndEventTag &&
        AttackConfig.AttackMontage == OtherConfig.AttackMontage;
}
