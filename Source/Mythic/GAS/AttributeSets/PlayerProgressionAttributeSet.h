
#pragma once
#include "MythicAttributeSet.h"
#include "PlayerProgressionAttributeSet.generated.h"

UCLASS()
class MYTHIC_API UPlayerProgressionAttributeSet : public UMythicAttributeSet
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_XP)
	FGameplayAttributeData XP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Level)
	FGameplayAttributeData Level;

public:
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UPlayerProgressionAttributeSet, XP);
	GAMEPLAYATTRIBUTE_VALUE_GETTER(XP);
	GAMEPLAYATTRIBUTE_VALUE_SETTER(XP);
	GAMEPLAYATTRIBUTE_VALUE_INITTER(XP);

	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UPlayerProgressionAttributeSet, Level);
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Level);
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Level);
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Level);

	UFUNCTION()
	virtual void OnRep_XP(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Level(const FGameplayAttributeData& OldValue);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
