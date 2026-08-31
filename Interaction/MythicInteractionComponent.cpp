

#include "MythicInteractionComponent.h"
#include "CommonPlayerController.h"
#include "IMythicInteractable.h"
#include "Mythic.h"
#include "PrimaryGameLayout.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "Interaction/Attention/MythicEntityAttentionSubsystem.h"
#include "Interaction/ContextActions/MythicContextActionProvider.h"
#include "UI/MythicTags_UI.h"
#include "World/Entity/MythicEntityPresentationComponent.h"


UMythicInteractionComponent::UMythicInteractionComponent() : UI_LayerRootWidget(nullptr) {
    PrimaryComponentTick.bCanEverTick = false;

    GameUILayerName = UI_LAYER_GAME;
}

void UMythicInteractionComponent::UpdateUILayerRootWidget(ACommonPlayerController *CommonPlayerController) {
    UPrimaryGameLayout *RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(CommonPlayerController);
    auto UI_Layer = RootLayout->GetLayerWidget(this->GameUILayerName);
    auto widget_list = UI_Layer->GetWidgetList();
    if (widget_list.Num() < 1) {
        UE_LOG(Myth, Verbose, TEXT("UMythicInteractionComponent::UpdateUILayerRootWidget: layer %s is not populated yet; will bind on a later call."),
               *this->GameUILayerName.ToString());
        return;
    }

    if (auto widget = Cast<UMythicActivatableWidget>(widget_list[0])) {
        this->UI_LayerRootWidget = widget;
        UE_LOG(Myth, Log, TEXT("UMythicInteractionComponent::UpdateUILayerRootWidget: Using Widget %s in Layer %s for input handling"),
               *this->UI_LayerRootWidget->GetName(),
               *this->GameUILayerName.ToString());
    }
    else {
        UE_LOG(Myth, Error, TEXT("UMythicInteractionComponent::UpdateUILayerRootWidget: Widget %s in Layer %s is not a MythicActivatableWidget"),
               *widget_list[0]->GetName(),
               *this->GameUILayerName.ToString());
    }
}

void UMythicInteractionComponent::BeginPlay() {
    Super::BeginPlay();

    this->OwningController = Cast<ACommonPlayerController>(GetOwner());
    if (!this->OwningController) {
        UE_LOG(Myth, Error, TEXT("InteractionComponent should only be attached to a CommonPlayerController"));
        return;
    }

    if (!this->OwningController->IsLocalController()) {
        return;
    }

    if (InteractionPromptWidgetClass) {
        this->InteractionPromptWidget = CreateWidget<UMythicInteractionPromptWidget>(
            this->OwningController, InteractionPromptWidgetClass);
        if (!this->InteractionPromptWidget) {
            UE_LOG(Myth, Error, TEXT("Failed to create InteractionPromptWidget"));
            return;
        }
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("InteractionPromptWidgetClass not set for %s"), *this->OwningController->GetName());
    }

    UpdateUILayerRootWidget(this->OwningController);

    this->PauseInteractions(false);
}

void UMythicInteractionComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
    PauseInteractions(true);
    Super::EndPlay(EndPlayReason);
}

int32 UMythicInteractionComponent::SelectFocusedInteractable(TConstArrayView<FMythicInteractCandidate> Candidates, float MinDot) {
    float BestDot = -1.0f;
    int32 Best = INDEX_NONE;
    for (int32 i = 0; i < Candidates.Num(); ++i) {
        const FMythicInteractCandidate &C = Candidates[i];
        if (C.Dot < MinDot) {
            continue;
        }
        if (C.bInRange) {
            if (C.Dot > BestDot) {
                BestDot = C.Dot;
                Best = i;
            }
        }
        else if (Best == INDEX_NONE || C.Distance < Candidates[Best].Distance) {
            Best = i;
        }
    }
    return Best;
}

void UMythicInteractionComponent::ScanForInteractableActors() {
    auto Pawn = this->OwningController->GetPawn();
    if (!Pawn) {
        return;
    }
    auto PlayerLoc = Pawn->GetActorLocation();
    auto PlayerForward = this->OwningController->GetPawn()->GetActorForwardVector();
    auto World = GetWorld();

    TArray<FHitResult> HitResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this->OwningController);
    auto HasResults = World->SweepMultiByChannel(HitResults, PlayerLoc, PlayerLoc, FQuat::Identity, ECollisionChannel::ECC_Visibility,
                                                 FCollisionShape::MakeSphere(InteractionRange), QueryParams);

    TArray<FMythicInteractCandidate> Candidates;
    TArray<AActor *> CandidateActors;
    if (HasResults) {
        for (auto &hit : HitResults) {
            auto actor = hit.GetActor();
            if (!actor || OwningController->GetPawn() == actor) {
                continue;
            }
            if (actor->GetClass()->ImplementsInterface(UMythicInteractable::StaticClass())) {
                const FVector thisActorLocation = actor->GetActorLocation();
                const float distance = (thisActorLocation - PlayerLoc).Size();
                FMythicInteractCandidate Cand;
                Cand.bInRange = distance < InteractionRange;
                Cand.Dot = FVector::DotProduct(PlayerForward, (thisActorLocation - PlayerLoc).GetSafeNormal());
                Cand.Distance = distance;
                Candidates.Add(Cand);
                CandidateActors.Add(actor);
            }
        }
    }
    const int32 BestIdx = SelectFocusedInteractable(Candidates, InteractionConeMinDot);
    AActor *bestActor = (BestIdx != INDEX_NONE) ? CandidateActors[BestIdx] : nullptr;

    if (CurrentFocusedActor && !IsValid(CurrentFocusedActor)) {
        EndInteraction(CurrentFocusedActor);
        CurrentFocusedActor = nullptr;
    }
    else if (!CurrentFocusedActor && IsCurrentActorReadyForInteraction) {
        EndStaleInteraction();
    }

    AActor *newFocusedActor = CurrentFocusedActor;
    if (bestActor) {
        if (!CurrentFocusedActor) {
            newFocusedActor = bestActor;
        }
        else if (CurrentFocusedActor != bestActor) {
            auto currentDistance = (CurrentFocusedActor->GetActorLocation() - PlayerLoc).Size();
            auto newDistance = (bestActor->GetActorLocation() - PlayerLoc).Size();
            if (newDistance < currentDistance) {
                newFocusedActor = bestActor;
            }
        }
    }

    else if (CurrentFocusedActor) {
        auto distance = (CurrentFocusedActor->GetActorLocation() - PlayerLoc).Size();
        if (distance > InteractionRange) {
            newFocusedActor = nullptr;
        }
    }

    if (newFocusedActor != CurrentFocusedActor) {
        OnFocusedActorChanged(newFocusedActor, CurrentFocusedActor);

        CurrentFocusedActor = newFocusedActor;
    }
    else if (CurrentFocusedActor && !this->IsCurrentActorReadyForInteraction) {
        InitializeInteraction(CurrentFocusedActor);
    }
}

void UMythicInteractionComponent::PauseInteractions(bool bPause) {
    if (bPause) {
        if (UWorld *World = GetWorld()) {
            World->GetTimerManager().ClearTimer(InteractionScanTimerHandle);
        } else {
            InteractionScanTimerHandle.Invalidate();
        }
        if (CurrentFocusedActor) {
            OnFocusedActorChanged_Implementation(nullptr, CurrentFocusedActor);
            CurrentFocusedActor = nullptr;
        } else if (IsCurrentActorReadyForInteraction) {
            EndStaleInteraction();
        }
        UE_LOG(Myth, Warning, TEXT("Paused Interaction Scans"));
        return;
    }

    if (!this->InteractionScanTimerHandle.IsValid()) {
        UE_LOG(Myth, Log, TEXT("Started Interaction Scans"));
        GetWorld()->GetTimerManager().SetTimer(this->InteractionScanTimerHandle, this, &UMythicInteractionComponent::ScanForInteractableActors,
                                               InteractionScanRate, true);
    }
}

void UMythicInteractionComponent::InitializeInteraction(AActor *NewFocusedActor) {
    if (!IsValid(NewFocusedActor)) {
        return;
    }

    // Presentable context-action providers render and bind through the LocalPlayer HUD. They must never attach a
    // WidgetComponent to a shared world actor, which is ambiguous and destructive under split-screen.
    if (NewFocusedActor->GetClass()->ImplementsInterface(
            UMythicContextActionProvider::StaticClass())) {
        if (const UMythicEntityPresentationComponent *Presentation =
                NewFocusedActor->FindComponentByClass<
                    UMythicEntityPresentationComponent>()) {
            // Registration may arrive after the actor under replication. Never fall back to the legacy shared-world
            // prompt during that window: retry on the next scan and let the LocalPlayer projection become authoritative.
            IsCurrentActorReadyForInteraction =
                Presentation->GetPresentationInstance().IsValid();
            if (IsCurrentActorReadyForInteraction) {
                IMythicInteractable::Execute_OnFocused(NewFocusedActor,
                                                       OwningController);
            }
        }
        // Providers fail closed even when misconfigured: binding the legacy actor-owned prompt would bypass the grant,
        // revision, privacy, and LocalPlayer ownership contract that this interface promises.
        return;
    }

    if (auto RootComp = IMythicInteractable::Execute_GetWidgetAttachmentComponent(NewFocusedActor)) {
        FMythicInteractionData InteractionData;
        this->IsCurrentActorReadyForInteraction = IMythicInteractable::Execute_GetInteractionData(NewFocusedActor, this->OwningController, InteractionData);

        if (!this->IsCurrentActorReadyForInteraction) {
            UE_LOG(Myth, Error, TEXT("Interaction Error: Actor %s's is not ready for interaction"), *NewFocusedActor->GetName());
            return;
        }

        if (!InteractionData.InputActionDataTable) {
            UE_LOG(Myth, Error, TEXT("Interaction Error: Actor %s's InputActionDataTable is nullptr"), *NewFocusedActor->GetName());
            return;
        }

        if (!this->UI_LayerRootWidget) {
            UpdateUILayerRootWidget(Cast<ACommonPlayerController>(this->OwningController));
        }

        if (!this->UI_LayerRootWidget) {
            return;
        }

        if (!this->InteractionPromptWidget) {
            if (this->InteractionPromptWidgetClass) {
                this->InteractionPromptWidget =
                    CreateWidget<UMythicInteractionPromptWidget>(
                        this->OwningController,
                        InteractionPromptWidgetClass);
            }
            else {
                UE_LOG(Myth, Error, TEXT("Interaction Error: InteractionPromptWidget and InteractionPromptWidgetClass are nullptr"));
                return;
            }
        }

        this->InteractionPromptWidget->SetInteractionData(InteractionData, NewFocusedActor, this->OwningController, this->UI_LayerRootWidget);

        if (ActiveInteractionWidgetComponent) {
            ActiveInteractionWidgetComponent->DestroyComponent();
            ActiveInteractionWidgetComponent = nullptr;
        }
        UWidgetComponent *newWidgetComponent = NewObject<UWidgetComponent>(
            NewFocusedActor, UWidgetComponent::StaticClass());
        // A compact pill, not a draw-at-desired-size widget: the prompt's desired width resolved to ~600px
        // and rendered as a grey bar across the top of the screen. A fixed screen-space draw size keeps it a
        // tidy "press E" callout; the plate and content fill it.
        newWidgetComponent->SetDrawAtDesiredSize(false);
        newWidgetComponent->SetDrawSize(FVector2D(240.0f, 60.0f));
        newWidgetComponent->SetWidget(this->InteractionPromptWidget);
        newWidgetComponent->SetOwnerPlayer(
            this->OwningController->GetLocalPlayer());
        newWidgetComponent->SetVisibility(true);
        newWidgetComponent->RegisterComponent();
        newWidgetComponent->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
        newWidgetComponent->ComponentTags.Add(FName("InteractionWidget"));

        newWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
        ActiveInteractionWidgetComponent = newWidgetComponent;

        IMythicInteractable::Execute_OnFocused(NewFocusedActor, this->OwningController);
    }
    else {
        UE_LOG(Myth, Error, TEXT("Interaction Error: Actor %s's GetWidgetAttachmentComponent() returned nullptr"), *NewFocusedActor->GetName());
    }
}

void UMythicInteractionComponent::EndInteraction(AActor *OldFocusedActor) {
    UE_LOG(Myth, Warning, TEXT("Ending Interaction with %s"),
           *GetNameSafe(OldFocusedActor));

    if (ActiveInteractionWidgetComponent) {
        ActiveInteractionWidgetComponent->DestroyComponent();
        ActiveInteractionWidgetComponent = nullptr;
    }

    if (IsValid(OldFocusedActor)) {
        IMythicInteractable::Execute_OnUnfocused(OldFocusedActor,
                                                 this->OwningController);
    }

    this->IsCurrentActorReadyForInteraction = false;

    if (this->InteractionPromptWidget) {
        this->InteractionPromptWidget->Clear();
    }
}

void UMythicInteractionComponent::EndStaleInteraction() {
    UE_LOG(Myth, Warning, TEXT("Ending stale interaction: focused actor was destroyed while focused"));

    this->IsCurrentActorReadyForInteraction = false;

    if (ActiveInteractionWidgetComponent) {
        ActiveInteractionWidgetComponent->DestroyComponent();
        ActiveInteractionWidgetComponent = nullptr;
    }

    if (this->InteractionPromptWidget) {
        this->InteractionPromptWidget->Clear();
    }
}

void UMythicInteractionComponent::OnFocusedActorChanged_Implementation(AActor *NewFocusedActor, AActor *OldFocusedActor) {
    // Transitional bridge: legacy interaction retains its proven sweep/UI path while publishing the selected public
    // embodiment into the shared LocalPlayer attention service. Once interaction consumes that service directly this
    // bridge disappears without changing attention, targeting, or nameplate contracts.
    if (OwningController && OwningController->IsLocalController()) {
        if (ULocalPlayer *LocalPlayer = OwningController->GetLocalPlayer()) {
            if (UMythicEntityAttentionSubsystem *Attention =
                    LocalPlayer->GetSubsystem<UMythicEntityAttentionSubsystem>()) {
                FMythicEntityPresentationInstance InteractionInstance;
                if (UMythicEntityPresentationComponent *Presentation =
                        NewFocusedActor
                            ? NewFocusedActor->FindComponentByClass<
                                  UMythicEntityPresentationComponent>()
                            : nullptr) {
                    InteractionInstance = Presentation->GetPresentationInstance();
                }
                Attention->SetInteractionTarget(InteractionInstance);
            }
        }
    }

    if (OldFocusedActor) {
        EndInteraction(OldFocusedActor);
    }

    if (NewFocusedActor && NewFocusedActor->Implements<UMythicInteractable>()) {
        InitializeInteraction(NewFocusedActor);
    }
}
