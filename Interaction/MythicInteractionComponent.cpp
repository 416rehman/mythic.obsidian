// 


#include "MythicInteractionComponent.h"
#include "CommonPlayerController.h"
#include "IMythicInteractable.h"
#include "Mythic.h"
#include "PrimaryGameLayout.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"

////////////// HOW IT WORKS /////////////////////
/// InteractionComponent takes a Tag for the GameUILayerName and finds the first widget, UI_LayerRootWidget, in that layer (This layer should only have one widget, the HUD)
/// Anytime the player is within InteractionRange of an interactable actor, this component will display the InteractionPromptWidget above that actor
/// All input actions will be bound to the UI_LayerRootWidget, which is the HUD widget

// Sets default values for this component's properties
UMythicInteractionComponent::UMythicInteractionComponent(): UI_LayerRootWidget(nullptr) {
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;
}

void UMythicInteractionComponent::UpdateUILayerRootWidget(ACommonPlayerController *CommonPlayerController) {
    // Instantiate the interaction widget class
    UPrimaryGameLayout *RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(CommonPlayerController);
    auto UI_Layer = RootLayout->GetLayerWidget(this->GameUILayerName);
    auto widget_list = UI_Layer->GetWidgetList();
    if (widget_list.Num() < 1) {
        UE_LOG(Myth, Error, TEXT("UMythicInteractionComponent::UpdateUILayerRootWidget: UI_Layer %s has no widgets"), *this->GameUILayerName.ToString());
        return;
    }

    if (auto widget = Cast<UMythicActivatableWidget>(widget_list[0])) {
        this->UI_LayerRootWidget = widget;
        UE_LOG(Myth, Warning, TEXT("UMythicInteractionComponent::UpdateUILayerRootWidget: Using Widget %s in Layer %s for input handling"),
               *this->UI_LayerRootWidget->GetName(),
               *this->GameUILayerName.ToString());
    }
    else {
        UE_LOG(Myth, Error, TEXT("UMythicInteractionComponent::UpdateUILayerRootWidget: Widget %s in Layer %s is not a MythicActivatableWidget"),
               *widget_list[0]->GetName(),
               *this->GameUILayerName.ToString());
    }
}

// Called when the game starts
void UMythicInteractionComponent::BeginPlay() {
    Super::BeginPlay();

    this->OwningController = Cast<ACommonPlayerController>(GetOwner());
    if (!this->OwningController) {
        UE_LOG(Myth, Error, TEXT("InteractionComponent should only be attached to a CommonPlayerController"));
        return;
    }

    // If the player controller is local (i.e. not on a server), then create the InteractionPromptWidget
    if (!this->OwningController->IsLocalController()) {
        return;
    }

    if (InteractionPromptWidgetClass) {
        this->InteractionPromptWidget = CreateWidget<UMythicInteractionPromptWidget>(GetWorld(), InteractionPromptWidgetClass);
        if (!this->InteractionPromptWidget) {
            UE_LOG(Myth, Error, TEXT("Failed to create InteractionPromptWidget"));
            return;
        }
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("InteractionPromptWidgetClass not set for %s"), *this->OwningController->GetName());
    }

    UpdateUILayerRootWidget(this->OwningController);

    // Start scanning for interactable actors
    this->PauseInteractions(false);
}

void UMythicInteractionComponent::ScanForInteractableActors() {
    auto Pawn = this->OwningController->GetPawn();
    if (!Pawn) {
        return;
    }
    auto PlayerLoc = Pawn->GetActorLocation();
    auto PlayerForward = this->OwningController->GetPawn()->GetActorForwardVector();
    auto World = GetWorld();

    // If items are within this distance, then use dot product to pick the one closest to the player's forward vector
    auto bestDot = -1.f;
    AActor *bestActor = nullptr;

    // Check for interactable actors in a sphere around the owning actor
    TArray<FHitResult> HitResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this->OwningController);
    auto HasResults = World->SweepMultiByChannel(HitResults, PlayerLoc, PlayerLoc, FQuat::Identity, ECollisionChannel::ECC_Visibility,
                                                 FCollisionShape::MakeSphere(InteractionRange), QueryParams);

    if (HasResults) {
        // Iterate over all hit results
        for (auto &hit : HitResults) {
            auto actor = hit.GetActor();
            if (!actor) {
                continue;
            }

            // Check if the actor implements the IMythicInteractable interface
            if (actor->GetClass()->ImplementsInterface(UMythicInteractable::StaticClass())) {
                auto thisActorLocation = actor->GetActorLocation();

                // If the item is within the distance threshold, then use the dot product to pick the one closest to the player's forward vector
                auto distance = (thisActorLocation - PlayerLoc).Size();
                if (distance < InteractionRange) {
                    auto toActor = (thisActorLocation - PlayerLoc).GetSafeNormal();
                    auto dot = FVector::DotProduct(PlayerForward, toActor);
                    if (dot > bestDot) {
                        bestDot = dot;
                        bestActor = actor;
                    }
                }
                else {
                    // If the item is outside the distance threshold, then just pick the closest one
                    if (!bestActor || distance < (bestActor->GetActorLocation() - PlayerLoc).Size()) {
                        bestActor = actor;
                    }
                }
            }
        }
    }

    // If we found an actor to focus on
    AActor *newFocusedActor = CurrentFocusedActor;
    if (bestActor) {
        // Focus on the best actor
        if (!CurrentFocusedActor) {
            newFocusedActor = bestActor;
        }
        // If we found an actor to focus on, and it is different from the currently focused actor, choose the closest one
        else if (CurrentFocusedActor != bestActor) {
            auto currentDistance = (CurrentFocusedActor->GetActorLocation() - PlayerLoc).Size();
            auto newDistance = (bestActor->GetActorLocation() - PlayerLoc).Size();
            if (newDistance < currentDistance) {
                newFocusedActor = bestActor;
            }
        }
    }

    // Check if the currently focused actor is still within range
    else if (CurrentFocusedActor) {
        auto distance = (CurrentFocusedActor->GetActorLocation() - PlayerLoc).Size();
        if (distance > InteractionRange) {
            newFocusedActor = nullptr;
        }
    }

    // If the focused actor has changed, then call the OnFocusedActorChanged function
    if (newFocusedActor != CurrentFocusedActor) {
        OnFocusedActorChanged(newFocusedActor, CurrentFocusedActor);

        // Finally cache the new focused actor
        CurrentFocusedActor = newFocusedActor;
    }
    // Otherwise, if the focused actor is still the same, but the interaction data is not set, initialize the interaction
    else if (CurrentFocusedActor && !this->IsCurrentActorReadyForInteraction) {
        InitializeInteraction(CurrentFocusedActor);
    }
}

void UMythicInteractionComponent::PauseInteractions(bool bPause) {
    if (bPause) {
        this->InteractionScanTimerHandle.Invalidate();
        UE_LOG(Myth, Warning, TEXT("Paused Interaction Scans"));
        return;
    }

    if (!this->InteractionScanTimerHandle.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("Started Interaction Scans"));
        GetWorld()->GetTimerManager().SetTimer(this->InteractionScanTimerHandle, this, &UMythicInteractionComponent::ScanForInteractableActors,
                                               InteractionScanRate, true);
    }
}

void UMythicInteractionComponent::InitializeInteraction(AActor *NewFocusedActor) {
    if (auto RootComp = IMythicInteractable::Execute_GetWidgetAttachmentComponent(NewFocusedActor)) {
        // Get the interaction widget from the interactable interface
        FMythicInteractionData InteractionData;
        this->IsCurrentActorReadyForInteraction = IMythicInteractable::Execute_GetInteractionData(NewFocusedActor, this->OwningController, InteractionData);

        // If the actor is not ready for interaction, then return
        if (!this->IsCurrentActorReadyForInteraction) {
            UE_LOG(Myth, Error, TEXT("Interaction Error: Actor %s's is not ready for interaction"), *NewFocusedActor->GetName());
            return;
        }

        // If the InputActionDataTable is not set, then return
        if (!InteractionData.InputActionDataTable) {
            UE_LOG(Myth, Error, TEXT("Interaction Error: Actor %s's InputActionDataTable is nullptr"), *NewFocusedActor->GetName());
            return;
        }

        // Update the LayerRootWidget if it is not set
        if (!this->UI_LayerRootWidget) {
            UpdateUILayerRootWidget(Cast<ACommonPlayerController>(this->OwningController));
        }

        // Update the prompt widget with the interaction data
        if (!this->InteractionPromptWidget) {
            if (this->InteractionPromptWidgetClass) {
                this->InteractionPromptWidget = CreateWidget<UMythicInteractionPromptWidget>(GetWorld(), InteractionPromptWidgetClass);
            }
            else {
                UE_LOG(Myth, Error, TEXT("Interaction Error: InteractionPromptWidget and InteractionPromptWidgetClass are nullptr"));
                return;
            }
        }

        this->InteractionPromptWidget->SetInteractionData(InteractionData, NewFocusedActor, this->OwningController, this->UI_LayerRootWidget);

        // Create the widget component and attach it to the root component
        UWidgetComponent *newWidgetComponent = NewObject<UWidgetComponent>(NewFocusedActor, UWidgetComponent::StaticClass());
        newWidgetComponent->SetDrawAtDesiredSize(true);
        newWidgetComponent->SetWidget(this->InteractionPromptWidget);
        newWidgetComponent->SetVisibility(true);
        newWidgetComponent->RegisterComponent();
        newWidgetComponent->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
        newWidgetComponent->ComponentTags.Add(FName("InteractionWidget"));

        newWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);

        // Call the interactable actor's OnFocused function
        IMythicInteractable::Execute_OnFocused(NewFocusedActor, this->OwningController);
    }
    else {
        UE_LOG(Myth, Error, TEXT("Interaction Error: Actor %s's GetWidgetAttachmentComponent() returned nullptr"), *NewFocusedActor->GetName());
    }
}

void UMythicInteractionComponent::EndInteraction(AActor *OldFocusedActor) {
    UE_LOG(Myth, Warning, TEXT("Ending Interaction with %s"), *OldFocusedActor->GetName());

    // Look for a UI widget on the old focused actor with the tag "InteractionWidget" and remove it
    if (auto UIWidget = OldFocusedActor->FindComponentByTag(UWidgetComponent::StaticClass(), FName("InteractionWidget"))) {
        UIWidget->DestroyComponent();
    }

    // Call the interactable actor's OnUnfocused function
    IMythicInteractable::Execute_OnUnfocused(OldFocusedActor, this->OwningController);

    // Reset the current interaction data
    this->IsCurrentActorReadyForInteraction = false;

    // Clear the prompt widget
    if (this->InteractionPromptWidget) {
        this->InteractionPromptWidget->Clear();
    }
}

void UMythicInteractionComponent::OnFocusedActorChanged_Implementation(AActor *NewFocusedActor, AActor *OldFocusedActor) {
    // Remove the UI widget from the old focused actor
    if (OldFocusedActor) {
        EndInteraction(OldFocusedActor);
    }

    // Add the UI widget to the new focused actor
    if (NewFocusedActor && NewFocusedActor->Implements<UMythicInteractable>()) {
        InitializeInteraction(NewFocusedActor);
    }
}
