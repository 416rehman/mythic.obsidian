// 

#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "GameplayTagContainer.h"
#include "MythicInteractionPromptWidget.h"
#include "Components/ActorComponent.h"
#include "MythicInteractionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicInteractionComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UMythicInteractionComponent();

protected:
    void UpdateUILayerRootWidget(ACommonPlayerController *CommonPlayerController);
    
    // Called when the game starts
    virtual void BeginPlay() override;

    // Find actor to focus on for interaction - Repeatedly 
    UFUNCTION(BlueprintCallable)
    void ScanForInteractableActors();

    // The actor that is currently focused for interaction
    UPROPERTY()
    AActor *CurrentFocusedActor = nullptr;

    // Whether or not the interaction
    UPROPERTY()
    bool IsCurrentActorReadyForInteraction = false;

    // Timer handle for scanning for interactable actors
    UPROPERTY()
    FTimerHandle InteractionScanTimerHandle;

    UPROPERTY()
    UMythicActivatableWidget * UI_LayerRootWidget;

    UPROPERTY()
    ACommonPlayerController * OwningController;

    UPROPERTY()
    UMythicInteractionPromptWidget * InteractionPromptWidget;

public:
    // The UI Layer responsible for input handling during interaction
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    FGameplayTag GameUILayerName = FGameplayTag::RequestGameplayTag("UI.Layer.Game");
    
    // The class of the widget to display when interacting with an actor
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    TSubclassOf<UMythicInteractionPromptWidget> InteractionPromptWidgetClass;
    
    UFUNCTION(BlueprintCallable)
    void PauseInteractions(bool bPause);
    void InitializeInteraction(AActor *NewFocusedActor);
    void EndInteraction(AActor *OldFocusedActor);

    UFUNCTION(BlueprintCallable, Client, Reliable)
    void OnFocusedActorChanged(AActor *NewFocusedActor, AActor *OldFocusedActor);
    void OnFocusedActorChanged_Implementation(AActor *NewFocusedActor, AActor *OldFocusedActor);

    // Any interactable actors within this range will be considered for interaction.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    float InteractionRange = 200.f;

    // Rate of scanning for interactable actors
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    float InteractionScanRate = 0.1f;
};
