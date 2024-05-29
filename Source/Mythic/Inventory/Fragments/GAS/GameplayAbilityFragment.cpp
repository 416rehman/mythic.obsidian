// 


#include "GameplayAbilityFragment.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mythic/Mythic.h"
#include "Mythic/Inventory/MythicInventoryComponent.h"

void UGameplayAbilityFragment::Action(UMythicItemInstance* ItemInstance) {
	// activate the gameplay ability
	if (GameplayAbility && ItemInstance) {
		if (ItemInstance->GetSlot() == nullptr) {
			UE_LOG(Mythic, Warning, TEXT("UGameplayAbilityFragment::Action_Implementation: ItemInstance->currentSlot is null"));
			return;
		}

	    // TODO
		// get the owning actor
		// APlayerController* PC = ItemInstance->GetSlot()->GetInventory()->GetOwner<APlayerController>();
		// if (PC) {
		// 	APSMythic_Player* PS = Cast<APSMythic_Player>(PC->GetPlayerState<APSMythic_Player>());
		//
		// 	if (PS) {
		// 		// get the owning actor's ability system component
		// 		UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS);
		// 		if (AbilitySystemComponent) {
		// 			// activate the gameplay ability
		// 			if (AbilitySystemComponent->TryActivateAbilityByClass(GameplayAbility, true)) {
		// 				UE_LOG(Mythic, Warning, TEXT("UGameplayAbilityFragment::Action_Implementation: Activated Gameplay Ability"));
		// 			}
		// 			else {
		// 				UE_LOG(Mythic, Warning, TEXT("UGameplayAbilityFragment::Action_Implementation: Failed to activate Gameplay Ability"));
		// 			}
		// 		}
		// 	}
		// }
	};
}

void UGameplayAbilityFragment::OnActiveItem(UMythicItemInstance* ItemInstance) {
	Super::OnActiveItem(ItemInstance);

    // TODO
	// // if actionTypetags includes "InHand"
	// if (this->GrantConditionTags.HasTag(
	// 	FGameplayTag::RequestGameplayTag(FName("Itemization.ActionType.InHand")))) {
	//
	// 	// get the player state
	// 	if (ItemInstance->GetSlot() == nullptr || ItemInstance->GetSlot()->GetInventory() == nullptr) {
	// 		UE_LOG(Mythic, Error, TEXT("UGameplayAbilityFragment::OnActiveItem: ItemInstance->currentSlot is null"));
	// 		return;
	// 	}
	// 	
	// 	APSMythic_Player* PlayerState = ItemInstance->GetSlot()->GetInventory()->GetOwner<APCMythic>()->GetPlayerState<APSMythic_Player>();
	// 	if (!PlayerState) {
	// 		UE_LOG(Mythic, Error, TEXT("UGameplayAbilityFragment::OnActiveItem: PlayerState is null"));
	// 		return;
	// 	}
	// 	this->GrantedAbilitySpec = FGameplayAbilitySpec(this->GameplayAbility, 1, 0, this);
	// 	
	// 	PlayerState->GetAbilitySystemComponentMythic()->Server_TryGrantAbility(this->GrantedAbilitySpec);
	// 	UE_LOG(Mythic, Warning, TEXT("UGameplayAbilityFragment::OnActiveItem: Granted Ability"));
	// }
}

void UGameplayAbilityFragment::OnInactiveItem(UMythicItemInstance* ItemInstance) {
	Super::OnInactiveItem(ItemInstance);
	// TODO
	// // check if the handle is valid
	// if (!this->GrantedAbilitySpec.Handle.IsValid()) {
	// 	UE_LOG(Mythic, Error, TEXT("UGameplayAbilityFragment::OnInactiveItem: GrantedAbilitySpec.Handle is invalid"));
	// }
	//
	// if (this->GrantConditionTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Itemization.ActionType.InHand")))) {
	// 	// get the owning actor
	// 	APlayerController* PC = ItemInstance->GetSlot()->GetInventory()->GetOwner<APlayerController>();
	// 	if (!PC) {
	// 		UE_LOG(Mythic, Error, TEXT("UGameplayAbilityFragment::OnInactiveItem: PC is null"));
	// 		return;
	// 	}
	//
	// 	APSMythic_Player* PS = Cast<APSMythic_Player>(PC->GetPlayerState<APSMythic_Player>());
	// 	if (!PS) {
	// 		UE_LOG(Mythic, Error, TEXT("UGameplayAbilityFragment::OnInactiveItem: PS is null"));
	// 		return;
	// 	}
	//
	// 	auto AbilitySystemComponent = PS->GetAbilitySystemComponentMythic();
	// 	if (!AbilitySystemComponent) {
	// 		UE_LOG(Mythic, Error, TEXT("UGameplayAbilityFragment::OnInactiveItem: AbilitySystemComponent is null"));
	// 		return;
	// 	}
	//
	// 	AbilitySystemComponent->Server_TryRemoveAbility(this->GrantedAbilitySpec.Handle);
	// 	UE_LOG(Mythic, Warning, TEXT("UGameplayAbilityFragment::OnInactiveItem: Canceled Ability"));
	// }
}
