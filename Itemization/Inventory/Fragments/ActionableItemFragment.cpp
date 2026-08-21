#include "ActionableItemFragment.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "Settings/MythicDeveloperSettings.h"
#include "GAS/Abilities/Item/GA_GenericConsumable.h"
#include "GameplayAbilitySpec.h"


#if WITH_EDITOR
bool UActionableItemFragment::IsValidFragment(FText &OutErrorMessage) const {
    if (!this->InputTag.IsValid()) {
        OutErrorMessage = FText::FromString("ActionableItemFragment: InputTag is not set");
        return false;
    }

    return Super::IsValidFragment(OutErrorMessage);
}
#endif

bool UActionableItemFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) {
        return false;
    }

    auto otherActionable = Cast<UActionableItemFragment>(Other);
    if (!otherActionable) {
        return false;
    }

    if (InputTag != otherActionable->InputTag) {
        return false;
    }

    return true;
}

FGameplayAbilitySpecHandle UActionableItemFragment::GrantItemAbility(UMythicAbilitySystemComponent *ASC, UMythicItemInstance *ItemInstance,
                                                                     TSubclassOf<UMythicGameplayAbility> AbilityClass) {
    UE_LOG(Myth, Log, TEXT("UActionableItemFragment::GrantItemAbility: Fragment=%s, Item=%s, AbilityClass=%s, InputTag=%s"),
           *GetName(),
           *GetNameSafe(ItemInstance),
           *GetNameSafe(AbilityClass.Get()),
           *InputTag.ToString());

    if (!ASC || !ItemInstance) {
        UE_LOG(Myth, Error, TEXT("  -> FAILED: Invalid ASC=%s or ItemInstance=%s"),
               *GetNameSafe(ASC),
               *GetNameSafe(ItemInstance));
        return FGameplayAbilitySpecHandle();
    }

    UClass *TargetAbilityClass = AbilityClass.Get();
    UE_LOG(Myth, Log, TEXT("  -> Initial TargetAbilityClass: %s"), *GetNameSafe(TargetAbilityClass));

    if (!TargetAbilityClass && InputTag.IsValid()) {
        UE_LOG(Myth, Log, TEXT("  -> No ability class, but have InputTag. Trying to use Generic Consumable."));
        if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
            TargetAbilityClass = Settings->DefaultItemInputAbility.LoadSynchronous();
            UE_LOG(Myth, Log, TEXT("  -> From Settings: %s"), *GetNameSafe(TargetAbilityClass));
        }
        else {
            UE_LOG(Myth, Warning, TEXT("  -> Could not get MythicDeveloperSettings."));
        }

        if (!TargetAbilityClass) {
            TargetAbilityClass = UGA_GenericConsumable::StaticClass();
            UE_LOG(Myth, Log, TEXT("  -> Fallback to native GA_GenericConsumable: %s"), *GetNameSafe(TargetAbilityClass));
        }
    }

    if (!TargetAbilityClass) {
        UE_LOG(Myth, Error, TEXT("  -> FAILED: Could not resolve a valid Ability Class for Item: %s. InputTag: %s"), *ItemInstance->GetName(),
               *InputTag.ToString());
        return FGameplayAbilitySpecHandle();
    }

    FGameplayAbilitySpec Spec(TargetAbilityClass, 1, INDEX_NONE, this);
    UE_LOG(Myth, Log, TEXT("  -> Created AbilitySpec with SourceObject=%s"), *GetName());

    if (InputTag.IsValid()) {
        Spec.GetDynamicSpecSourceTags().AddTag(InputTag);
        UE_LOG(Myth, Log, TEXT("  -> Added InputTag %s to DynamicSpecSourceTags"), *InputTag.ToString());
    }
    else {
        UE_LOG(Myth, Warning, TEXT("  -> InputTag is NOT VALID! Ability will not respond to input."));
    }

    FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
    UE_LOG(Myth, Log, TEXT("  -> GiveAbility returned Handle (Valid: %s)"), Handle.IsValid() ? TEXT("YES") : TEXT("NO"));

    return Handle;
}

void UActionableItemFragment::ExecuteGenericAction(UMythicItemInstance *ItemInstance) {
    UE_LOG(Myth, Warning, TEXT("ExecuteGenericAction called on base ActionableItemFragment. Override this in subclasses."));
}
