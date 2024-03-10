
#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "MythicAttributeSet.h"
#include "PlayerAttributeSet.generated.h"

UCLASS()
class MYTHIC_API UPlayerAttributeSet: public UMythicAttributeSet {
    GENERATED_BODY()
protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MovementSpeed)
    FGameplayAttributeData MovementSpeed;

public:
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UPlayerAttributeSet, MaxHealth);
    GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxHealth);
    GAMEPLAYATTRIBUTE_VALUE_SETTER(MaxHealth);
    GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxHealth);
    
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UPlayerAttributeSet, Health);
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Health);
    GAMEPLAYATTRIBUTE_VALUE_SETTER(Health);
    GAMEPLAYATTRIBUTE_VALUE_INITTER(Health);

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UPlayerAttributeSet, MovementSpeed);
    GAMEPLAYATTRIBUTE_VALUE_GETTER(MovementSpeed);
    GAMEPLAYATTRIBUTE_VALUE_SETTER(MovementSpeed);
    GAMEPLAYATTRIBUTE_VALUE_INITTER(MovementSpeed);

    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
    
    UFUNCTION()
    virtual void OnRep_MovementSpeed(const FGameplayAttributeData& OldMovementSpeed);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // PreAttributeChange is called just before any modification to attributes takes place.
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};
