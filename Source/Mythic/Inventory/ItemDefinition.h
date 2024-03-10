

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Fragments\ItemFragment.h"
#include "ItemDefinition.generated.h"

UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API UItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	
	/** The name of the item */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta=(DisplayName="Name"))
	FText Name;

	/** Short description of the item */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Description"))
	FText Description;

	/** The type of the item, stored in gameplay tag Itemization.Type */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(Categories="Itemization.Type", DisplayName="ItemType"))
	FGameplayTag ItemType = FGameplayTag::RequestGameplayTag("Itemization.Type.Other");

	/** Rarity of the item, stored in gameplay tag Itemization.Rarity */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(Categories="Itemization.Rarity", DisplayName="Rarity"))
	FGameplayTag Rarity = FGameplayTag::RequestGameplayTag("Itemization.Rarity.Common");

	/** Static mesh to use for the item when it is in the world */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="WorldMesh", MakeStructureDefaultValue="None"))
	TObjectPtr<UStaticMesh> WorldMesh;

	/** 2d icon to use for the item when it is in the inventory */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="2dIcon", MakeStructureDefaultValue="None"))
	TObjectPtr<UTexture2D> Icon2d;

	/** Stack size of the item. If greater than 1, the item can be stacked in the inventory */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="StackSizeMax", MakeStructureDefaultValue="0"))
	int32 StackSizeMax;

	// Set of ItemDefinitionFragments that define the item
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="ItemDefinitionFragments", MakeStructureDefaultValue="None"), Instanced)
	TArray<TObjectPtr<UItemFragment>> Fragments;
};