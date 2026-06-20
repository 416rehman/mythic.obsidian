#include "MythicInteractionPromptWidget.h"

#include "Mythic.h"
#include "Input/CommonUIInputTypes.h"

void UMythicInteractionPromptWidget::SetInteractionData(FMythicInteractionData InInteractionData, AActor *InInteractableActor,
                                                        APlayerController *InPlayerController, UMythicActivatableWidget *UI_LayerRootWidget) {
    Clear();

    // Add the complimentary widget
    if (InInteractionData.ComplimentaryWidget) {
        this->VerticalBox->AddChild(InInteractionData.ComplimentaryWidget);
    }

    // Both input-action bindings require the UI layer root widget. Guard once here — the secondary block previously had
    // NO null-check (only the primary did), so a None primary + set secondary + null root dereferenced null below.
    if (!UI_LayerRootWidget) {
        UE_LOG(Myth, Error, TEXT("Interaction Error: UI_LayerRootWidget is nullptr"));
        return;
    }

    // Bind the primary input action
    if (InInteractionData.PrimaryInteractionName.IsNone() || InInteractionData.PrimaryInteractionName == FName("")) {
        UE_LOG(Myth, Error, TEXT("Interaction Warning: Actor %s's PrimaryInteractionName is None"), *InInteractableActor->GetName());
    }
    else {
        FDataTableRowHandle rowhandle;
        rowhandle.DataTable = Cast<UDataTable>(InInteractionData.InputActionDataTable);
        if (!rowhandle.DataTable) {
            UE_LOG(Myth, Error, TEXT("Interaction Error: DataTable is not valid"));
            return;
        }
        rowhandle.RowName = InInteractionData.PrimaryInteractionName;

        TWeakObjectPtr<AActor> WeakInteractable(InInteractableActor);
        FBindUIActionArgs BindArgs(rowhandle, false, FSimpleDelegate::CreateLambda([this, WeakInteractable, InPlayerController]() {
            if (AActor *Interactable = WeakInteractable.Get()) {
                IMythicInteractable::Execute_OnPrimaryInteract(Interactable, InPlayerController);
            }
        }));

        this->PrimaryInteractionHandle = UI_LayerRootWidget->RegisterUIActionBinding(BindArgs);
        if (this->PrimaryInteractionHandle.IsValid()) {
            // Create widget for the primary action from the ActionButtonClass
            if (UCommonButtonBase *PrimaryActionBtn = CreateWidget<UCommonButtonBase>(this, this->ActionButtonClass)) {
                ICommonBoundActionButtonInterface *ActionButtonInterface = Cast<ICommonBoundActionButtonInterface>(PrimaryActionBtn);
                if (ensure(ActionButtonInterface)) {
                    ActionButtonInterface->SetRepresentedAction(this->PrimaryInteractionHandle);
                }
                else {
                    UE_LOG(Myth, Error, TEXT("Interaction Error: PrimaryActionBtn does not implement ICommonBoundActionButtonInterface"));
                }

                this->VerticalBox->AddChild(PrimaryActionBtn);
            }
        }
        else {
            UE_LOG(Myth, Error, TEXT("Interaction Error: PrimaryInteractionHandle is not valid"));
        }
    }

    // Bind the Secondary input action
    if (InInteractionData.SecondaryInteractionName.IsNone() || InInteractionData.SecondaryInteractionName ==
        FName("")) {
        UE_LOG(Myth, Error, TEXT("Interaction Warning: Actor %s's SecondaryInteractionName is None"), *InInteractableActor->GetName());
    }
    else {
        FDataTableRowHandle rowhandle;
        rowhandle.DataTable = Cast<UDataTable>(InInteractionData.InputActionDataTable);
        rowhandle.RowName = InInteractionData.SecondaryInteractionName;

        TWeakObjectPtr<AActor> WeakInteractable(InInteractableActor);
        FInputActionBindingHandle _BindingHandle;
        FBindUIActionArgs BindArgs2(rowhandle, false, FSimpleDelegate::CreateLambda([this, WeakInteractable, InPlayerController]() {
            if (AActor *Interactable = WeakInteractable.Get()) {
                IMythicInteractable::Execute_OnSecondaryInteract(Interactable, InPlayerController);
            }
        }));

        this->SecondaryInteractionHandle = UI_LayerRootWidget->RegisterUIActionBinding(BindArgs2);

        if (this->SecondaryInteractionHandle.IsValid()) {
            // Create widget for the Secondary action from the ActionButtonClass
            if (UCommonButtonBase *SecondaryActionBtn = CreateWidget<UCommonButtonBase>(this, this->ActionButtonClass)) {
                ICommonBoundActionButtonInterface *ActionButtonInterface = Cast<ICommonBoundActionButtonInterface>(SecondaryActionBtn);
                if (ensure(ActionButtonInterface)) {
                    ActionButtonInterface->SetRepresentedAction(this->SecondaryInteractionHandle);
                }
                else {
                    UE_LOG(Myth, Error, TEXT("Interaction Error: SecondaryActionBtn does not implement ICommonBoundActionButtonInterface"));
                }

                this->VerticalBox->AddChild(SecondaryActionBtn);
            }
        }
        else {
            UE_LOG(Myth, Error, TEXT("Interaction Error: SecondaryInteractionHandle is not valid"));
        }
    }

    OnInteractionDataUpdated(InInteractionData, InInteractableActor);
}

void UMythicInteractionPromptWidget::Clear() {
    this->PrimaryInteractionHandle.Unregister();
    this->SecondaryInteractionHandle.Unregister();
    this->VerticalBox->ClearChildren();

    UE_LOG(Myth, Log, TEXT("Interaction Prompt Cleared"));
}
