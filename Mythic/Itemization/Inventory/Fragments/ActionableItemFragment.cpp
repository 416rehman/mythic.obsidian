#include "ActionableItemFragment.h"

#include "EnhancedInputComponent.h"

bool UActionableItemFragment::IsValidFragment(FText &OutErrorMessage) const {
    if (!this->InputAction) {
        OutErrorMessage = FText::FromString("ActionableItemFragment: InputAction is not set");
        return false;
    }

    return Super::IsValidFragment(OutErrorMessage);
}

void UActionableItemFragment::OnClientItemActivated(UMythicItemInstance *ItemInstance) {
    if (!InputAction) {
        UE_LOG(LogTemp, Error, TEXT("ActionableItemFragment::OnClientItemActivated: InputAction is not set"));
        return;
    }

    auto owner = ItemInstance->GetInventoryOwner();
    auto OwnerController = Cast<APlayerController>(owner);
    if (!OwnerController || !OwnerController->IsLocalController()) {
        UE_LOG(LogTemp, Warning, TEXT("ActionableItemFragment::ClientBindInput: Owner is not a local player controller"));
        return;
    }

    if (auto EnhancedInputComp = Cast<UEnhancedInputComponent>(OwnerController->InputComponent)) {
        // Bind the input action
        FEnhancedInputActionEventBinding &binding = EnhancedInputComp->BindAction(InputAction, ETriggerEvent::Started, this,
                                                                                  &UActionableItemFragment::InputStarted);
        if (auto handle = binding.GetHandle()) {
            InputBindings.Add(handle);
        }

        FEnhancedInputActionEventBinding &binding2 = EnhancedInputComp->BindAction(InputAction, ETriggerEvent::Completed, this,
                                                                                   &UActionableItemFragment::InputEnded);
        if (auto handle = binding2.GetHandle()) {
            InputBindings.Add(handle);
        }
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("ActionableItemFragment::ClientBindInput: EnhancedInputComponent not found"));
    }
}

void UActionableItemFragment::OnClientItemDeactivated(UMythicItemInstance *ItemInstance) {
    // Player input component
    auto owner = ItemInstance->GetInventoryOwner();
    auto OwnerController = Cast<APlayerController>(owner);
    if (auto EnhancedInputComp = Cast<UEnhancedInputComponent>(OwnerController->InputComponent)) {
        // Unbind the input action
        for (auto handle : InputBindings) {
            EnhancedInputComp->RemoveBindingByHandle(handle);
        }
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("ActionableItemFragment::ClientUnbindInput: EnhancedInputComponent not found"));
    }
    // Clear regardless of component existence
    InputBindings.Empty();
}

void UActionableItemFragment::InputStarted(const FInputActionInstance &InputActionInstance) {
    OnClientActionBegin(this->ParentItemInstance);
}

void UActionableItemFragment::InputEnded(const FInputActionValue &InputActionValue) {
    OnClientActionEnd(this->ParentItemInstance);
}

bool UActionableItemFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) {
        return false;
    }

    auto otherActionable = Cast<UActionableItemFragment>(Other);
    if (!otherActionable) {
        return false;
    }

    if (InputAction != otherActionable->InputAction) {
        return false;
    }

    return true;
}
