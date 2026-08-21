
#include "TalentFragment.h"
#include "AbilitySystemComponent.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/MythicLootSettings.h"
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
        if (!TDef) {
            continue;
        }

        if (!TDef->HasAnyPayload()) {
            OutErrorMessage = FText::FromString(FString::Printf(
                TEXT("Talent '%s' does nothing: set AbilityDef.Ability."),
                *TDef->Name.ToString()));
            return false;
        }
        if (TDef->AbilityDef.Ability && !TDef->AbilityDef.IsValid(OutErrorMessage)) {
            return false;
        }

        if (const UMythicGameplayAbility *AbilityCDO =
                TDef->AbilityDef.Ability ? Cast<UMythicGameplayAbility>(TDef->AbilityDef.Ability->GetDefaultObject()) : nullptr) {
            if (AbilityCDO->GetActivationPolicy() != EMythicAbilityActivationPolicy::OnSpawn) {
                UE_LOG(Myth, Warning,
                       TEXT("UTalentFragment::IsValidFragment: talent ability '%s' ActivationPolicy is not OnSpawn — talents must be passive."),
                       *GetNameSafe(TDef->AbilityDef.Ability.Get()));
            }
        }
    }

    return Super::IsValidFragment(OutErrorMessage);
}
#endif
int32 UTalentFragment::ResolveTalentCount(int32 Rarity, const UMythicLootSettings *LootSettings) {
    if (LootSettings && LootSettings->TalentCountByRarity.IsValidIndex(Rarity)) {
        return LootSettings->TalentCountByRarity[Rarity];
    }
    if (Rarity >= Mythic) {
        return 2;
    }
    return Rarity >= Rare ? 1 : 0;
}

TArray<int32> UTalentFragment::SampleWithoutReplacement(const TArray<int32> &EligibleIndexes, int32 NumToPick, FRandomStream &Rng) {
    TArray<int32> Picked;
    const int32 ClampedQty = FMath::Clamp(NumToPick, 0, EligibleIndexes.Num());
    if (ClampedQty <= 0) {
        return Picked;
    }
    TArray<int32> Available = EligibleIndexes;
    Picked.Reserve(ClampedQty);
    for (int32 i = 0; i < ClampedQty && Available.Num() > 0; i++) {
        const int32 Pos = Rng.RandRange(0, Available.Num() - 1);
        Picked.Add(Available[Pos]);
        Available.RemoveAtSwap(Pos);
    }
    return Picked;
}

void UTalentFragment::RollTalents(UTalentPool *TalentPool, int NumTalentsToRoll, EItemRarity ItemRarity, const FGameplayTagContainer &TypeProbe) {
    if (!TalentPool) {
        return;
    }

    TArray<int32> EligibleIndexes;
    EligibleIndexes.Reserve(TalentPool->TalentDefs.Num());
    for (int32 i = 0; i < TalentPool->TalentDefs.Num(); i++) {
        const UTalentDefinition *Def = TalentPool->TalentDefs[i];
        if (Def && IsTalentEligible(ItemRarity, Def->MinRarity) && IsTalentAllowedOnItem(Def->AllowedItemTypes, TypeProbe)) {
            EligibleIndexes.Add(i);
        }
    }

    FRandomStream Rng(FMath::Rand());
    const TArray<int32> PickedIndexes = SampleWithoutReplacement(EligibleIndexes, NumTalentsToRoll, Rng);

    for (int32 RandIdx : PickedIndexes) {
        auto TalentAtIdx = TalentPool->TalentDefs[RandIdx];
        if (!TalentAtIdx) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::OnInstanced: Invalid talent definition."));
            continue;
        }

        if (!TalentAtIdx->HasAnyPayload()) {
            UE_LOG(Myth, Error, TEXT("UTalentFragment::OnInstanced: talent '%s' grants nothing (no ability)."),
                   *TalentAtIdx->Name.ToString());
            continue;
        }

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

                                       const int32 RarityValue = ItemInstance->GetItemDefinition()->Rarity;
                                       const UMythicLootSettings *LootSettings = GetDefault<UMythicLootSettings>();
                                       const int32 NumTalentsToRoll = ResolveTalentCount(RarityValue, LootSettings);

                                       FGameplayTagContainer TypeProbe;
                                       ItemInstance->GetTypeProbe(TypeProbe);
                                       RollTalents(TalentPool, NumTalentsToRoll, static_cast<EItemRarity>(RarityValue), TypeProbe);
                                   });
}

void UTalentFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    if (!ItemInstance) {
        UE_LOG(Myth, Error, TEXT("UTalentFragment::OnActiveItem: Invalid item instance."));
        return;
    }

    ServerHandleGrantAbility();
}

void UTalentFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);

    if (!ItemInstance) {
        UE_LOG(Myth, Error, TEXT("UTalentFragment::OnInactiveItem: Invalid item instance."));
        return;
    }

    ServerRemoveAbility();
}

bool UTalentFragment::CanBeStackedWith(const UItemFragment *Other) const {
    return false;
}

void UTalentFragment::ServerRemoveAbility_Implementation() {
    for (auto &TalentSpec : this->TalentRuntimeReplicatedData.RolledTalents) {
        if (!TalentSpec.AbilitySpec.Handle.IsValid()) {
            UE_LOG(Myth, Warning, TEXT("UTalentFragment::ServerHandleInHandRemoveAbility_Implementation: No ability to remove."));
            continue;
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
            continue;
        }

        AbilitySpec.Handle = NewAbilityHandle;
        TalentSpec.AbilitySpec = AbilitySpec;
    }
}
