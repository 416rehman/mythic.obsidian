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

    // Apply the attribute change to the ASC
    ASC->ApplyModToAttribute(Attribute, Modifier, Magnitude);
    UE_LOG(Myth, Log, TEXT("Applied %s modifier of %f to attribute %s"), *UEnum::GetValueAsString(Modifier), Magnitude, *Attribute.GetName());

    return true;
}
