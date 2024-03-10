#include "AffixesFragment.h"
#include "AbilitySystemComponent.h"
#include "Mythic/Inventory/InventoryComponent.h"
#include "Mythic/Inventory/InventorySlot.h"
#include "Mythic/Inventory/ItemInstance.h"
#include "AbilitySystemGlobals.h"

void UAffixesFragment::OnInstanced(UItemInstance* Instance) {
    if (this->GEClass == nullptr) {
        UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::OnInstanced: GEClass is null."));
        return;
    }
    
	Super::OnInstanced(Instance);

	int AffixesAdded = 0;
	
    // Create an Affix Group for this GEClass
    this->InstancedAffixGroup = FAffixGroupInst();
    this->InstancedAffixGroup.GEClass = this->GEClass;

    // Loop over the AffixPoolMap and add all the guaranteed affixes to the AffixGroup
	for (auto& AffixSelection : this->AffixPoolMap) {
		if (AffixSelection.Value.bGuaranteed) {
		    // create a new magnitude name value for each magnitude in the affix
		    FMagnitudeNameValue MagnitudeNameValue;
		    MagnitudeNameValue.Tag = AffixSelection.Key;

		    //random between min and max value
		    MagnitudeNameValue.Value = FMath::RandRange(AffixSelection.Value.Min, AffixSelection.Value.Max);
		    this->InstancedAffixGroup.Entries.Add(MagnitudeNameValue);

		    AffixesAdded++;
		}
		
		if (AffixesAdded >= this->MaxAffixes) {
			break;
		}
	}	

    // If there are more affixes to add, add them randomly
	if (AffixesAdded < this->MaxAffixes && this->AffixPoolMap.Num() >= this->MaxAffixes) {
		// Key array of the affix pool map
	    TArray<FGameplayTag> AffixKeys;
	    this->AffixPoolMap.GetKeys(AffixKeys);

	    // Randomize the keys to get a random order
	    AffixKeys.Sort([](const FGameplayTag& A, const FGameplayTag& B) {
            return FMath::RandBool();
        });

	    // Loop over the keys and add the affixes to the group if not already added, if AffixesAdded >= MaxAffixes, break
	    for (auto& AffixKey : AffixKeys) {
            if (AffixesAdded >= this->MaxAffixes) {
                break;
            }
            
           // Check if this key is already added to the group
            bool bAlreadyAdded = false;
            for (auto& Affix : this->InstancedAffixGroup.Entries) {
                if (Affix.Tag == AffixKey) {
                    bAlreadyAdded = true;
                    break;
                }
            }

            if (bAlreadyAdded) {
                continue;
            }

            // create a new magnitude name value for each magnitude in the affix
            FMagnitudeNameValue MagnitudeNameValue;
            MagnitudeNameValue.Tag = AffixKey;

            //random between min and max value
            MagnitudeNameValue.Value = FMath::RandRange(this->AffixPoolMap[AffixKey].Min, this->AffixPoolMap[AffixKey].Max);
            this->InstancedAffixGroup.Entries.Add(MagnitudeNameValue);

            AffixesAdded++;
        }
	}
}

void UAffixesFragment::OnSlotChanged(UInventorySlot* newSlot, UInventorySlot* oldSlot) {
    // TODO
	// Super::OnSlotChanged(newSlot, oldSlot);
	// if (oldSlot == newSlot) return;
	//
	// auto oldInventory = oldSlot ? oldSlot->GetInventory() : nullptr;
	// auto newInventory = newSlot ? newSlot->GetInventory() : nullptr;
	// auto PrevASC = oldInventory ? oldInventory->GetOwner<APCMythic>()->GetPlayerState<APSMythic_Player>()->GetAbilitySystemComponent() : nullptr;
	// auto CurrASC = newInventory ? newInventory->GetOwner<APCMythic>()->GetPlayerState<APSMythic_Player>()->GetAbilitySystemComponent() : nullptr;
	//
	// // Slot type is not compatible with the affixes
	// if (newSlot && this->SlotType != newSlot->SlotType) {
	// 	UE_LOG(Mythic, Warning, TEXT("AffixesInstFragment::OnSlotChanged: Item Slot Type: %s not compatible with the slot type: %s"), *this->SlotType.ToString(), *newSlot->SlotType.ToString());
	// 	DeactivateAffixes(PrevASC);
	// 	return;
	// }
	//
	// // Same ASC, no need to remove and add affixes
	// if (PrevASC && CurrASC && PrevASC == CurrASC) {
	// 	UE_LOG(Mythic, Warning, TEXT("AffixesInstFragment::OnSlotChanged: ASC match, not removing and adding affixes."));
	// 	return;
	// }
	//
	// DeactivateAffixes(PrevASC);	// Remove from old ASC
	// ActivateAffixes(CurrASC);	// Add to new ASC
}

void UAffixesFragment::ActivateAffixes(UAbilitySystemComponent* ASC) {
	if (!ASC) {
		UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::ActivateAffixes: Invalid ASC."));
		return;
	}
    if (this->InstancedAffixGroup.ActiveGEHandle.IsValid()) {
        UE_LOG(Mythic, Warning, TEXT("AffixesInstFragment::ActivateAffixes: Affix already active, skipping."));
        return;
    }
    
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(this->InstancedAffixGroup.GEClass, 1, EffectContext);
	if (SpecHandle.IsValid()) {
		for (FMagnitudeNameValue TagValuePair : this->InstancedAffixGroup.Entries) {
			UE_LOG(Mythic, Warning, TEXT("AffixesInstFragment::OnSlotChanged: Setting magnitude %s to %f"), *TagValuePair.Tag.ToString(), TagValuePair.Value);
			SpecHandle.Data->SetSetByCallerMagnitude(TagValuePair.Tag, TagValuePair.Value);
		}
		auto GEHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		this->InstancedAffixGroup.ActiveGEHandle = GEHandle;
	}
}

void UAffixesFragment::DeactivateAffixes(UAbilitySystemComponent* ASC) {
	if (!ASC) {
		UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::DeactivateAffixes: Invalid ASC."));
		return;
	}
	

	ASC->RemoveActiveGameplayEffect(this->InstancedAffixGroup.ActiveGEHandle);
	this->InstancedAffixGroup.ActiveGEHandle = FActiveGameplayEffectHandle();
	this->InstancedAffixGroup.ActiveGEHandle.Invalidate();
}
