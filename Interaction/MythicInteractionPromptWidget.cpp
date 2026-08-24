#include "MythicInteractionPromptWidget.h"

#include "Mythic.h"
#include "Input/CommonUIInputTypes.h"

UCommonButtonBase *UMythicInteractionPromptWidget::GetOrCreateActionButton(int32 Index) {
    if (ActionButtonPool.IsValidIndex(Index)) {
        return ActionButtonPool[Index];
    }
    if (!this->ActionButtonClass || !this->VerticalBox) {
        return nullptr;
    }
    UCommonButtonBase *Button = CreateWidget<UCommonButtonBase>(this, this->ActionButtonClass);
    if (!Button) {
        return nullptr;
    }
    Button->SetVisibility(ESlateVisibility::Collapsed);
    // The shared action button carries a 200px MinWidth for the action bar; a "press E" pill must hug its
    // key and verb instead, or the prompt renders as a wide grey bar across the top of the screen.
    Button->SetMinDimensions(0, 0);
    this->VerticalBox->AddChild(Button);
    ActionButtonPool.Add(Button);
    return Button;
}

void UMythicInteractionPromptWidget::ShowActionButton(int32 Index, const FUIActionBindingHandle &Handle) {
    UCommonButtonBase *Button = GetOrCreateActionButton(Index);
    if (!Button) {
        return;
    }
    if (ICommonBoundActionButtonInterface *ActionButtonInterface = Cast<ICommonBoundActionButtonInterface>(Button)) {
        ActionButtonInterface->SetRepresentedAction(Handle);
        Button->SetVisibility(ESlateVisibility::Visible);
    }
    else {
        UE_LOG(Myth, Error, TEXT("Interaction Error: action button does not implement ICommonBoundActionButtonInterface"));
    }
}

void UMythicInteractionPromptWidget::CollapseActionButtonsFrom(int32 Index) {
    for (int32 i = Index; i < ActionButtonPool.Num(); ++i) {
        if (ActionButtonPool[i]) {
            ActionButtonPool[i]->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UMythicInteractionPromptWidget::SetInteractionData(FMythicInteractionData InInteractionData, AActor *InInteractableActor,
                                                        APlayerController *InPlayerController, UMythicActivatableWidget *UI_LayerRootWidget) {
    Clear();

    if (InInteractionData.ComplimentaryWidget != ActiveComplimentaryWidget) {
        if (ActiveComplimentaryWidget) {
            this->VerticalBox->RemoveChild(ActiveComplimentaryWidget);
        }
        ActiveComplimentaryWidget = InInteractionData.ComplimentaryWidget;
        if (ActiveComplimentaryWidget) {
            this->VerticalBox->AddChildToVerticalBox(ActiveComplimentaryWidget);
        }
    }
    CollapseActionButtonsFrom(0);

    if (!UI_LayerRootWidget) {
        UE_LOG(Myth, Error, TEXT("Interaction Error: UI_LayerRootWidget is nullptr"));
        return;
    }

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
            ShowActionButton(0, this->PrimaryInteractionHandle);
        }
        else {
            UE_LOG(Myth, Error, TEXT("Interaction Error: PrimaryInteractionHandle is not valid"));
        }
    }

    if (InInteractionData.SecondaryInteractionName.IsNone() || InInteractionData.SecondaryInteractionName ==
        FName("")) {
        UE_LOG(Myth, Verbose, TEXT("Interaction: Actor %s has no SecondaryInteractionName; skipping the secondary bind."),
               *InInteractableActor->GetName());
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
            ShowActionButton(1, this->SecondaryInteractionHandle);
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

    CollapseActionButtonsFrom(0);
    if (ActiveComplimentaryWidget) {
        this->VerticalBox->RemoveChild(ActiveComplimentaryWidget);
        ActiveComplimentaryWidget = nullptr;
    }

    UE_LOG(Myth, Verbose, TEXT("Interaction Prompt Cleared"));
}
