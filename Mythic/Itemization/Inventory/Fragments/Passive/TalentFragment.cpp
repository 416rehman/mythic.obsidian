// 

#include "TalentFragment.h"
#include "AbilitySystemComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"

bool UTalentFragment::GetRichText(FText &OutRichText) {
    return this->TalentConfig.TalentDefinition->AbilityDef.GetRichText(OutRichText, this->TalentRuntimeReplicatedData.AbilityRollSpec.RolledAttributes);
}

bool UTalentFragment::IsValidFragment(FText &OutErrorMessage) const {
    auto TalentDef = this->TalentConfig.TalentDefinition;
    if (!TalentDef) {
        OutErrorMessage = FText::FromString("Invalid Talent Definition");
        return false;
    }

    if (!TalentDef->AbilityDef.IsValid(OutErrorMessage)) {
        return false;
    }

    return Super::IsValidFragment(OutErrorMessage);
}

void UTalentFragment::OnInstanced(UMythicItemInstance *ItemInstance) {
    Super::OnInstanced(ItemInstance);

    // ItemInstance->GetInventoryOwner() might be invalid this early
    auto Owner = ItemInstance->GetOwningActor();
    if (!Owner) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::OnInstanced: Invalid owner."));
        return;
    }

    // Make sure the talent definition is valid
    auto TalentDef = this->TalentConfig.TalentDefinition;
    if (!TalentDef) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::OnInstanced: Invalid talent definition."));
        return;
    }

    // Make sure the ability definition is valid
    if (!TalentDef->AbilityDef.Ability) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::OnInstanced: Invalid ability definition."));
        return;
    }

    // Roll the ability attributes
    this->TalentRuntimeReplicatedData.AbilityRollSpec = FAbilityRollSpec(TalentDef->AbilityDef);
}

void UTalentFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    if (!ItemInstance) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::OnActiveItem: Invalid item instance."));
        return;
    }

    // Grant the ability
    ServerHandleGrantAbility();
}

void UTalentFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);

    if (!ItemInstance) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::OnInactiveItem: Invalid item instance."));
        return;
    }

    // Remove the ability
    if (this->TalentRuntimeReplicatedData.AbilityRollSpec.AbilitySpec.Handle.IsValid()) {
        ServerRemoveAbility();
    }
}

bool UTalentFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) {
        return false;
    }

    auto OtherTalent = Cast<UTalentFragment>(Other);
    if (!OtherTalent) {
        return false;
    }

    return this->TalentRuntimeReplicatedData.AbilityRollSpec == OtherTalent->TalentRuntimeReplicatedData.AbilityRollSpec &&
        this->TalentConfig.TalentDefinition == OtherTalent->TalentConfig.TalentDefinition;
}

void UTalentFragment::ServerRemoveAbility_Implementation() {
    auto AbilityRollSpec = &this->TalentRuntimeReplicatedData.AbilityRollSpec;
    if (!AbilityRollSpec->AbilitySpec.Handle.IsValid()) {
        UE_LOG(Mythic, Warning, TEXT("UTalentFragment::ServerHandleInHandRemoveAbility_Implementation: No ability to remove."));
        return;
    }

    if (!this->ParentItemInstance) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::ServerHandleInHandRemoveAbility_Implementation: Invalid item instance."));
        return;
    }

    auto ASC = this->GetOwningAbilitySystemComponent();
    if (!ASC) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::ServerHandleInHandRemoveAbility_Implementation: Invalid ASC."));
        return;
    }

    ASC->ClearAbility(AbilityRollSpec->AbilitySpec.Handle);
    UE_LOG(Mythic, Warning, TEXT("UGameplayAbilityFragment::OnInactiveItem: Canceled Ability"));

    // Clear the ability
    AbilityRollSpec->AbilitySpec = FGameplayAbilitySpec();
}

void UTalentFragment::ServerHandleGrantAbility_Implementation() {
    auto ItemInstance = this->ParentItemInstance;
    if (!ItemInstance) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Invalid item instance."));
        return;
    }

    auto ASC = this->GetOwningAbilitySystemComponent();
    if (!ASC) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Invalid ASC."));
        return;
    }
    auto TalentDefinition = this->TalentConfig.TalentDefinition;
    auto AbilitySpec = FGameplayAbilitySpec(TalentDefinition->AbilityDef.Ability, 1, INDEX_NONE, this);
    auto NewAbilityHandle = ASC->GiveAbility(AbilitySpec);
    if (!NewAbilityHandle.IsValid()) {
        UE_LOG(Mythic, Error, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Failed to grant ability."));
        return;
    }

    this->TalentRuntimeReplicatedData.AbilityRollSpec.AbilitySpec = AbilitySpec;
}
