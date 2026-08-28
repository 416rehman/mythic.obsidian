#include "ActionableItemFragment.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "Settings/MythicDeveloperSettings.h"
#include "GAS/Abilities/Item/GA_GenericConsumable.h"
#include "GameplayAbilitySpec.h"


#if WITH_EDITOR
bool UActionableItemFragment::IsValidFragment(FText &OutErrorMessage) const {
    // Some actionable fragments are invoked directly by inventory/hotbar UI and intentionally grant no input
    // ability. Concrete input-bound fragments (for example UAttackFragment) own the stricter tag contract.
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

FGameplayAbilitySpecHandle UActionableItemFragment::GrantItemAbility(
    UMythicAbilitySystemComponent *ASC,
    UMythicItemInstance *ItemInstance,
    TSubclassOf<UMythicGameplayAbility> AbilityClass,
    const bool bBindInputTag) {
    UE_LOG(Myth, Log, TEXT("UActionableItemFragment::GrantItemAbility: Fragment=%s, Item=%s, AbilityClass=%s, InputTag=%s, BindInput=%s"),
           *GetName(),
           *GetNameSafe(ItemInstance),
           *GetNameSafe(AbilityClass.Get()),
           *InputTag.ToString(),
           bBindInputTag ? TEXT("true") : TEXT("false"));

    if (!ASC || !ItemInstance) {
        UE_LOG(Myth, Error, TEXT("  -> FAILED: Invalid ASC=%s or ItemInstance=%s"),
               *GetNameSafe(ASC),
               *GetNameSafe(ItemInstance));
        return FGameplayAbilitySpecHandle();
    }

    UClass *TargetAbilityClass = AbilityClass.Get();
    UE_LOG(Myth, Log, TEXT("  -> Initial TargetAbilityClass: %s"), *GetNameSafe(TargetAbilityClass));

    if (!TargetAbilityClass && bBindInputTag && InputTag.IsValid()) {
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

    if (bBindInputTag && InputTag.IsValid()) {
        Spec.GetDynamicSpecSourceTags().AddTag(InputTag);
        UE_LOG(Myth, Log, TEXT("  -> Added InputTag %s to DynamicSpecSourceTags"), *InputTag.ToString());
    }
    else if (bBindInputTag) {
        UE_LOG(Myth, Warning, TEXT("  -> InputTag is NOT VALID! Ability will not respond to input."));
    }
    else {
        UE_LOG(Myth, Log, TEXT("  -> Generic input binding intentionally omitted; exact-source activation owns this spec."));
    }

    FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
    UE_LOG(Myth, Log, TEXT("  -> GiveAbility returned Handle (Valid: %s)"), Handle.IsValid() ? TEXT("YES") : TEXT("NO"));

    return Handle;
}

void UActionableItemFragment::ExecuteGenericAction(UMythicItemInstance *ItemInstance) {
    UE_LOG(Myth, Warning, TEXT("ExecuteGenericAction called on base ActionableItemFragment. Override this in subclasses."));
}
