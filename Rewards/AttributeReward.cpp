// 


#include "AttributeReward.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mythic.h"
#include "GameFramework/PlayerController.h"

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
        // Create an owner-Outered per-instance attribute set (the engine idiom — same as the protected
        // GetOrCreateAttributeSubobject does internally) — NOT GetDefaultObject<>(), which returns the shared class CDO:
        // AddSpawnedAttribute would register the CDO as this ASC's live set, so the SetNumericAttributeBase below would
        // corrupt the attribute defaults for EVERY actor/ASC of the class (and the CDO, Outered to its package, cannot
        // replicate as a per-actor subobject).
        UAttributeSet *AttributeSet = NewObject<UAttributeSet>(ASC->GetOwner(), Attribute.GetAttributeSetClass());
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

FText UAttributeReward::GetPreviewText() const {
    if (!Attribute.IsValid()) {
        return FText::GetEmpty();
    }
    return FText::FromString(FString::Printf(TEXT("+%.1f %s"), Magnitude, *Attribute.GetName()));
}
