// 


#include "AttributeReward.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mythic.h"

bool UAttributeReward::Give(FRewardContext &Context) const {
    // Check if the context is an ability system component
    auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.PlayerController);
    checkf(ASC, TEXT("AbilitySystemComponent is null"));

    // Check if the attribute is valid
    if (!Attribute.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Attribute is not valid"));
        return false;
    }

    // If the attributeset is not on the ASC, give it first
    if (!ASC->HasAttributeSetForAttribute(Attribute)) {
        auto AttributeSet = Attribute.GetAttributeSetClass()->GetDefaultObject<UAttributeSet>();
        ASC->AddSpawnedAttribute(AttributeSet);
        UE_LOG(Myth, Log, TEXT("Added attribute set for attribute %s"), *Attribute.GetName());
    }

    // Modify the BASE value so the reward persists through saves
    float CurrentBase = ASC->GetNumericAttributeBase(Attribute);
    float NewBase = CurrentBase;

    switch (Modifier) {
    case EGameplayModOp::Additive:
        NewBase = CurrentBase + Magnitude;
        break;
    case EGameplayModOp::Multiplicitive:
        NewBase = CurrentBase * Magnitude;
        break;
    case EGameplayModOp::Override:
        NewBase = Magnitude;
        break;
    default:
        NewBase = CurrentBase + Magnitude;
        break;
    }

    ASC->SetNumericAttributeBase(Attribute, NewBase);
    UE_LOG(Myth, Log, TEXT("Set base value of %s: %.2f -> %.2f"), *Attribute.GetName(), CurrentBase, NewBase);

    return true;
}
