#pragma once
#include "ActiveGameplayEffectHandle.h"
#include "Engine/DataTable.h"
#include "Mythic/Inventory/ItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "AffixesFragment.generated.h"

class UGameplayEffect;

struct FAffixGroupInst;

USTRUCT(BlueprintType)
struct FMagnitudeNameValue
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag {};

	UPROPERTY()
	float Value {0};
};

USTRUCT(BlueprintType)
struct FAffixGroupInst
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UGameplayEffect> GEClass {};

	UPROPERTY()
	TArray<FMagnitudeNameValue> Entries {};

	UPROPERTY()
	FActiveGameplayEffectHandle ActiveGEHandle {};
};

USTRUCT(Blueprintable, BlueprintType)
struct MYTHIC_API FAffixEntryDefinition
{
	GENERATED_BODY()
    
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Min"))
	float Min = 0;
    
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Max"))
	float Max = 0;

    // if this is true, then this affix will be guaranteed to be applied to the item
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Guaranteed"))
    bool bGuaranteed = false;
};

UCLASS()
class MYTHIC_API UAffixesFragment : public UItemFragment
{
	GENERATED_BODY()
protected:	// Instance Data
	UPROPERTY(Replicated, EditInstanceOnly, BlueprintReadWrite)
	FAffixGroupInst InstancedAffixGroup;
    
public:
    // The global gameplay effect that will be applied to the character when this item is equipped
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="GE Asset", MakeStructureDefaultValue="None"))
    TSubclassOf<class UGameplayEffect> GEClass;
    
	// Affixes from this pool will be applied to this item when it is instanced
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(Categories="AttributeSets"))
	TMap<FGameplayTag,FAffixEntryDefinition> AffixPoolMap;

	// this is the number of random affixes that will be applied to this item from the affix pool when it is instanced
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	int32 MaxAffixes;

	// when slotted in a slot with this tag, the affixes will be applied to the character
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	FGameplayTag SlotType;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override {
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);
		DOREPLIFETIME(UAffixesFragment, InstancedAffixGroup);
		DOREPLIFETIME(UAffixesFragment, AffixPoolMap);
		DOREPLIFETIME(UAffixesFragment, MaxAffixes);
		DOREPLIFETIME(UAffixesFragment, SlotType);
	}

	void DeactivateAffixes(UAbilitySystemComponent* ASC);
	void ActivateAffixes(UAbilitySystemComponent* ASC);
	virtual void OnInstanced(UItemInstance* Instance) override;
	virtual void OnSlotChanged(UInventorySlot* newSlot, UInventorySlot* oldSlot) override;
};