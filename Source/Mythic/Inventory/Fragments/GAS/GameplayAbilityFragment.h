// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Mythic/Inventory/Fragments/ItemFragment.h"
#include "UObject/Object.h"
#include "GameplayAbilityFragment.generated.h"

/** Grants a gameplay ability */
UCLASS(BlueprintType, meta=(Categories="Itemization.ActionType"))
class MYTHIC_API UGameplayAbilityFragment : public UActionableItemFragment
{
	GENERATED_BODY()
protected:
	// The gameplay ability to grant
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Gameplay Ability")
	FGameplayAbilitySpec GrantedAbilitySpec; // the spec of the granted ability
public:
	// The gameplay ability to grant
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Gameplay Ability")
	TSubclassOf<class UGameplayAbility> GameplayAbility;
	
	// should this gameplay ability only be granted when the item is in hand? or should it be granted when the item is in the inventory?
	// i.e a sword that grants an attack ability should only be granted when the sword is in hand, but a bottle with "refill" ability should be granted when the bottle is in the inventory
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Gameplay Ability", meta=(Categories="Itemization.ActionType"))
	FGameplayTagContainer GrantConditionTags;
	
	virtual void Action(UItemInstance* ItemInstance) override;
	virtual void OnActiveItem(UItemInstance* ItemInstance) override;
	virtual void OnInactiveItem(UItemInstance* ItemInstance) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override {
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);
		DOREPLIFETIME(UGameplayAbilityFragment, GrantedAbilitySpec);
		DOREPLIFETIME(UGameplayAbilityFragment, GameplayAbility);
		DOREPLIFETIME(UGameplayAbilityFragment, GrantConditionTags);
	}
};