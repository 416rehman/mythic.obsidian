// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_NPCCombat.generated.h"

/**
 * NPC Combat Attribute Set
 */
UCLASS()
class MYTHIC_API UMythicAttributeSet_NPCCombat : public UMythicAttributeSet {
    GENERATED_BODY()
protected:
    // The amount of damage the NPC can deal
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Damage)
    FGameplayAttributeData Damage;

public:
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_NPCCombat, Damage);

    UFUNCTION()
    virtual void OnRep_Damage(const FGameplayAttributeData &OldDamage);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
