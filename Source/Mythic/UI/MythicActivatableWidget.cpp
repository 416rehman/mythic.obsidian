// 
#include "MythicActivatableWidget.h"
#include "Input/CommonUIInputTypes.h"

void UMythicActivatableWidget::NativeDestruct() {
    for (FUIActionBindingHandle Handle : BindingHandles) {
        if (Handle.IsValid()) {
            Handle.Unregister();
        }
    }
    BindingHandles.Empty();

    Super::NativeDestruct();
}

void UMythicActivatableWidget::RegisterBinding(FDataTableRowHandle InputAction, const FInputActionExecutedDelegate &Callback,
                                               FInputActionBindingHandle &BindingHandle) {
    FBindUIActionArgs BindArgs(InputAction, FSimpleDelegate::CreateLambda([InputAction, Callback]() {
        Callback.ExecuteIfBound(InputAction.RowName);
    }));
    BindArgs.bDisplayInActionBar = true;

    BindingHandle.Handle = RegisterUIActionBinding(BindArgs);
    BindingHandles.Add(BindingHandle.Handle);
}

void UMythicActivatableWidget::UnregisterBinding(FInputActionBindingHandle BindingHandle) {
    if (BindingHandle.Handle.IsValid()) {
        BindingHandle.Handle.Unregister();
        BindingHandles.Remove(BindingHandle.Handle);
    }
}

void UMythicActivatableWidget::UnregisterAllBindings() {
    for (FUIActionBindingHandle Handle : BindingHandles) {
        Handle.Unregister();
    }
    BindingHandles.Empty();
}
