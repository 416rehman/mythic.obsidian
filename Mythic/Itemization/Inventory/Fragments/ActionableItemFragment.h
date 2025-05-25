// 

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "ActionableItemFragment.generated.h"

/*
 * UActionableItemFragment
 * 
 * This fragment handles input binding and action execution for items.
 * 
 * Activation Flow:
 * 1. Server: MythicInventoryComponent.SetActiveSlotIndex(index)
 * 2. Server: MythicInventorySlot.ActivateSlot()
 * 3. Server: MythicItemInstance.OnActiveItem() -> Activates all fragments
 * 4. Server: Replicates activation state to client
 * 5. Client: MythicInventorySlot.ClientActivateSlot()
 * 6. Client: MythicItemInstance.OnClientActiveItem() -> Activates all fragments
 * 7. Client: ActionableItemFragment.OnClientItemActivated()
 *    - Gets local player controller
 *    - Finds EnhancedInputComponent
 *    - Binds InputAction to InputStarted and InputEnded
 * 
 * Deactivation follows the same flow in reverse.
 * The fragment only implements client-side input binding/unbinding.
 * Game logic should be implemented in OnClientActionBegin/OnClientActionEnd.
 */
UCLASS(BlueprintType, Abstract, Blueprintable, EditInlineNew, meta=(ShowOnlyInnerProperties))
class MYTHIC_API UActionableItemFragment : public UItemFragment {
    GENERATED_BODY()

public:
    UPROPERTY(Replicated, EditAnywhere, meta=(ShowOnlyInnerProperties))
    UInputAction *InputAction;

    TArray<uint32> InputBindings;

    /** Fragment Overrides */
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
    virtual void OnClientItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnClientItemDeactivated(UMythicItemInstance *ItemInstance) override;
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    /** Called when action input starts. Override to implement item-specific behavior. */
    UFUNCTION(BlueprintCallable)
    virtual void OnClientActionBegin(UMythicItemInstance *ItemInst) {}

    /** Called when action input ends. Override to implement item-specific behavior. */
    UFUNCTION(BlueprintCallable)
    virtual void OnClientActionEnd(UMythicItemInstance *ItemInstance) {}

    void InputStarted(const FInputActionInstance &InputActionInstance);

    void InputEnded(const FInputActionValue &InputActionValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UActionableItemFragment, InputAction);
    }
};
