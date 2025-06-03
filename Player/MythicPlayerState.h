

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "MythicPlayerState.generated.h"

class UMythicAttributeSet_Life;
class UMythicAttributeSet_Offense;
class UMythicAttributeSet_Proficiencies;
class UMythicAttributeSet_Exp;
class UMythicAttributeSet_Utility;
class UMythicAttributeSet_Defense;
/**
 * 
 */
UCLASS()
class MYTHIC_API AMythicPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System")
    TObjectPtr<UMythicAbilitySystemComponent> MythicAbilitySystemComponent;

    // Default Ability Set
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UMythicGameplayAbility>> DefaultAbilities;

    // Default Gameplay Effects - Can also be used to initialize attributes.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
    TArray<TSubclassOf<UGameplayEffect>> DefaultGameplayEffects;

    // Life attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Life* LifeAttributes;

    // Offense attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Offense* OffenseAttributes;

    // Defense attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Defense* DefenseAttributes;

    // Utility attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Utility* UtilityAttributes;

    // Experience attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Exp* ExperienceAttributes;

    // Proficiency attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Proficiencies* ProficiencyAttributes;

	AMythicPlayerState();

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UMythicAbilitySystemComponent* GetMythicAbilitySystemComponent() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void BeginPlay() override;
};
