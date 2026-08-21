

#include "AttributeReward.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mythic.h"
#include "GameFramework/PlayerController.h"
#include "UI/ViewModels/MythicStatDisplay.h"

bool UAttributeReward::Give(FRewardContext &Context) const {
    auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.PlayerController);
    checkf(ASC, TEXT("AbilitySystemComponent is null"));

    if (!Attribute.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Attribute is not valid"));
        return false;
    }

    if (!ASC->HasAttributeSetForAttribute(Attribute)) {
        UAttributeSet *AttributeSet = NewObject<UAttributeSet>(ASC->GetOwner(), Attribute.GetAttributeSetClass());
        ASC->AddSpawnedAttribute(AttributeSet);
        UE_LOG(Myth, Log, TEXT("Added attribute set for attribute %s"), *Attribute.GetName());
    }

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

    const FMythicStatRule Rule = MythicStatDisplay::GetRule(Attribute);
    const FText Label = FText::FromString(Rule.Label);

    switch (Modifier) {
        case EGameplayModOp::Additive: {
            const FText Bonus = MythicStatDisplay::FormatBonus(Magnitude, Rule.Format);
            return Bonus.IsEmpty()
                       ? FText::GetEmpty()
                       : FText::Format(NSLOCTEXT("Mythic", "AttrRewardAdd", "{0} {1}"), Label, Bonus);
        }
        case EGameplayModOp::Multiplicitive:
            return FText::Format(NSLOCTEXT("Mythic", "AttrRewardMul", "{0} x{1}"), Label,
                                 FText::FromString(FString::SanitizeFloat(Magnitude, 1)));
        case EGameplayModOp::Override:
            return FText::Format(NSLOCTEXT("Mythic", "AttrRewardSet", "{0} set to {1}"), Label,
                                 MythicStatDisplay::FormatValue(Magnitude, Rule.Format));
        default: {
            const FText Bonus = MythicStatDisplay::FormatBonus(Magnitude, Rule.Format);
            return Bonus.IsEmpty()
                       ? FText::GetEmpty()
                       : FText::Format(NSLOCTEXT("Mythic", "AttrRewardAdd", "{0} {1}"), Label, Bonus);
        }
    }
}
