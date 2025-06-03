#pragma once
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Exp.generated.h"

/// Player Progression ///
/// Overall player progression through the game.
UCLASS()
class MYTHIC_API UMythicAttributeSet_Exp : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", ReplicatedUsing = OnRep_XP)
    FGameplayAttributeData XP;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", ReplicatedUsing = OnRep_Level)
    FGameplayAttributeData Level;

public:
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Exp, XP);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Exp, Level);

    UFUNCTION()
    virtual void OnRep_XP(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_Level(const FGameplayAttributeData &OldValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;
};
