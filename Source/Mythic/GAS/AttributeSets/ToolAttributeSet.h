#pragma once
#include "MythicAttributeSet.h"
#include "ToolAttributeSet.generated.h"

UCLASS()
class UToolAttributeSet : public UMythicAttributeSet
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_PickaxeSpeedPercent)
	FGameplayAttributeData PickaxeSpeedPercent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ScytheSpeedPercent)
	FGameplayAttributeData ScytheSpeedPercent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_HatchetSpeedPercent)
	FGameplayAttributeData HatchetSpeedPercent;

	/*	----  */
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_PickaxeYield)
	FGameplayAttributeData PickaxeYieldPercent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ScytheYield)
	FGameplayAttributeData ScytheYieldPercent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_HatchetYield)
	FGameplayAttributeData HatchetYieldPercent;
	
public:
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UToolAttributeSet, PickaxeSpeedPercent);
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PickaxeSpeedPercent);
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PickaxeSpeedPercent);
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PickaxeSpeedPercent);

	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UToolAttributeSet, ScytheSpeedPercent);
	GAMEPLAYATTRIBUTE_VALUE_GETTER(ScytheSpeedPercent);
	GAMEPLAYATTRIBUTE_VALUE_SETTER(ScytheSpeedPercent);
	GAMEPLAYATTRIBUTE_VALUE_INITTER(ScytheSpeedPercent);

	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UToolAttributeSet, HatchetSpeedPercent);
	GAMEPLAYATTRIBUTE_VALUE_GETTER(HatchetSpeedPercent);
	GAMEPLAYATTRIBUTE_VALUE_SETTER(HatchetSpeedPercent);
	GAMEPLAYATTRIBUTE_VALUE_INITTER(HatchetSpeedPercent);

	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UToolAttributeSet, PickaxeYieldPercent);
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PickaxeYieldPercent);
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PickaxeYieldPercent);
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PickaxeYieldPercent);

	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UToolAttributeSet, ScytheYieldPercent);
	GAMEPLAYATTRIBUTE_VALUE_GETTER(ScytheYieldPercent);
	GAMEPLAYATTRIBUTE_VALUE_SETTER(ScytheYieldPercent);
	GAMEPLAYATTRIBUTE_VALUE_INITTER(ScytheYieldPercent);

	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UToolAttributeSet, HatchetYieldPercent);
	GAMEPLAYATTRIBUTE_VALUE_GETTER(HatchetYieldPercent);
	GAMEPLAYATTRIBUTE_VALUE_SETTER(HatchetYieldPercent);
	GAMEPLAYATTRIBUTE_VALUE_INITTER(HatchetYieldPercent);

	UFUNCTION()
	virtual void OnRep_PickaxeSpeedPercent(const FGameplayAttributeData& OldPickaxeSpeedPercent);
	UFUNCTION()
	virtual void OnRep_ScytheSpeedPercent(const FGameplayAttributeData& OldScytheSpeedPercent);
	UFUNCTION()
	virtual void OnRep_HatchetSpeedPercent(const FGameplayAttributeData& OldHatchetSpeedPercent);

	UFUNCTION()
	virtual void OnRep_PickaxeYield(const FGameplayAttributeData& OldPickaxeYield);
	UFUNCTION()
	virtual void OnRep_ScytheYield(const FGameplayAttributeData& OldScytheYield);
	UFUNCTION()
	virtual void OnRep_HatchetYield(const FGameplayAttributeData& OldHatchetYield);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};