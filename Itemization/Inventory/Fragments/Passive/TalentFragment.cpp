// 

#include "TalentFragment.h"
#include "AbilitySystemComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "System/MythicAssetManager.h"

#if WITH_EDITOR
bool UTalentFragment::IsValidFragment(FText &OutErrorMessage) const {
    auto TalentDef = this->TalentBuildData.TalentPool;

    UTalentPool *Pool = TalentDef.LoadSynchronous();
    if (!Pool) {
        OutErrorMessage = FText::FromString("Invalid Talent Pool");
        return false;
    }

    for (auto TDef : Pool->TalentDefs) {
        if (!TDef->AbilityDef.IsValid(OutErrorMessage)) {
            return false;
        }
    }

    return Super::IsValidFragment(OutErrorMessage);
}
#endif
void UTalentFragment::RollTalents(UTalentPool *TalentPool, int NumTalentsToRoll) {
    // In case, there are not enough talents to add, or the item level is too low, clamp to however many affixes there are.
    int ClampedQty = FMath::Min(NumTalentsToRoll, TalentPool->TalentDefs.Num());

    // Get ClampedQty unique random indexes
    TArray<int> RandomIndexes;
    TArray<int> AvailableIndexes;

    // Create array of all possible indexes
    for (int i = 0; i < TalentPool->TalentDefs.Num(); i++) {
        AvailableIndexes.Add(i);
    }

    // Sample without replacement
    RandomIndexes.Reserve(ClampedQty);
    for (int i = 0; i < ClampedQty && AvailableIndexes.Num() > 0; i++) {
        int RandomPos = FMath::RandRange(0, AvailableIndexes.Num() - 1);
        RandomIndexes.Add(AvailableIndexes[RandomPos]);
        AvailableIndexes.RemoveAtSwap(RandomPos);
    }

    for (auto RandIdx : RandomIndexes) {
        auto TalentAtIdx = TalentPool->TalentDefs[RandIdx];
        if (!TalentAtIdx) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::OnInstanced: Invalid talent definition."));
            continue;
        }

        auto Ability = TalentAtIdx->AbilityDef.Ability;
        if (!Ability) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::OnInstanced: Invalid ability definition."));
            continue;
        }

        // Add the ability
        this->TalentRuntimeReplicatedData.RolledTalents.Add(FTalentSpec(TalentAtIdx->AbilityDef, TalentAtIdx, false));
    }
}

void UTalentFragment::OnInstanced(UMythicItemInstance *ItemInstance) {
    Super::OnInstanced(ItemInstance);

    auto Owner = ItemInstance->GetOwningActor();
    if (!Owner) {
        UE_LOG(Myth, Error, TEXT("UTalentFragment::OnInstanced: Invalid owner."));
        return;
    }

    // LoadAsync handles already-loaded case internally (immediate callback)
    UMythicAssetManager::LoadAsync(this, this->TalentBuildData.TalentPool,
                                   [this, ItemInstance](UTalentPool *TalentPool) {
                                       if (!TalentPool) {
                                           UE_LOG(Myth, Error, TEXT("UTalentFragment::OnInstanced: Failed to load talent pool."));
                                           return;
                                       }

                                       if (!ItemInstance || !ItemInstance->GetItemDefinition()) {
                                           UE_LOG(Myth, Error, TEXT("UTalentFragment::OnInstanced: Item became invalid during async load."));
                                           return;
                                       }

                                       auto Rarity = ItemInstance->GetItemDefinition()->Rarity;
                                       auto NumTalentsToRoll = Rarity == Legendary ? 1 : Rarity == Mythic ? 2 : 0;
                                       RollTalents(TalentPool, NumTalentsToRoll);
                                   });
}

void UTalentFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    if (!ItemInstance) {
        UE_LOG(Myth, Error, TEXT("UTalentFragment::OnActiveItem: Invalid item instance."));
        return;
    }

    // Grant the ability
    ServerHandleGrantAbility();
}

void UTalentFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);

    if (!ItemInstance) {
        UE_LOG(Myth, Error, TEXT("UTalentFragment::OnInactiveItem: Invalid item instance."));
        return;
    }

    // Remove the ability
    ServerRemoveAbility();
}

// Items with Talents cannot be stacked
bool UTalentFragment::CanBeStackedWith(const UItemFragment *Other) const {
    return false;
}

void UTalentFragment::ServerRemoveAbility_Implementation() {
    for (auto &TalentSpec : this->TalentRuntimeReplicatedData.RolledTalents) {
        if (!TalentSpec.AbilitySpec.Handle.IsValid()) {
            UE_LOG(Myth, Warning, TEXT("UTalentFragment::ServerHandleInHandRemoveAbility_Implementation: No ability to remove."));
            return;
        }

        if (!this->ParentItemInstance) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::ServerHandleInHandRemoveAbility_Implementation: Invalid item instance."));
            return;
        }

        auto ASC = this->GetOwningAbilitySystemComponent();
        if (!ASC) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::ServerHandleInHandRemoveAbility_Implementation: Invalid ASC."));
            return;
        }

        ASC->ClearAbility(TalentSpec.AbilitySpec.Handle);
        UE_LOG(Myth, Warning, TEXT("UGameplayAbilityFragment::OnInactiveItem: Canceled Ability"));

        // Clear the ability
        TalentSpec.AbilitySpec = FGameplayAbilitySpec();
    }
}

void UTalentFragment::ServerHandleGrantAbility_Implementation() {
    auto ItemInstance = this->ParentItemInstance;
    if (!ItemInstance) {
        UE_LOG(Myth, Error, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Invalid item instance."));
        return;
    }

    auto ASC = this->GetOwningAbilitySystemComponent();
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Invalid ASC."));
        return;
    }

    for (auto &TalentSpec : this->TalentRuntimeReplicatedData.RolledTalents) {
        if (TalentSpec.AbilitySpec.Handle.IsValid()) {
            UE_LOG(Myth, Warning, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Already granted ability."));
            continue;
        }

        auto TalentDefinition = TalentSpec.TalentDef;
        if (!TalentDefinition) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Invalid talent definition."));
            continue;
        }

        auto AbilityDef = TalentDefinition->AbilityDef;
        if (!AbilityDef.Ability) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Invalid ability definition."));
            continue;
        }

        auto Ability = AbilityDef.Ability;
        if (!Ability) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Invalid ability."));
            continue;
        }

        auto AbilitySpec = FGameplayAbilitySpec(Ability, 1, INDEX_NONE, this);
        auto NewAbilityHandle = ASC->GiveAbility(AbilitySpec);
        if (!NewAbilityHandle.IsValid()) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::ServerHandleGrantAbility_Implementation: Failed to grant ability."));
            return;
        }

        TalentSpec.AbilitySpec = AbilitySpec;
    }
}
