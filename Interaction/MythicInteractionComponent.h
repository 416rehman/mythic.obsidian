
#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "GameplayTagContainer.h"
#include "MythicInteractionPromptWidget.h"
#include "Components/ActorComponent.h"
#include "MythicInteractionComponent.generated.h"

struct FMythicInteractCandidate {
    bool bInRange = false;
    float Dot = -1.0f;
    float Distance = 0.0f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicInteractionComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicInteractionComponent();

    static int32 SelectFocusedInteractable(TConstArrayView<FMythicInteractCandidate> Candidates, float MinDot = -1.0f);

protected:
    void UpdateUILayerRootWidget(ACommonPlayerController *CommonPlayerController);

    virtual void BeginPlay() override;

    // Find actor to focus on for interaction - Repeatedly
    UFUNCTION(BlueprintCallable)
    void ScanForInteractableActors();

    UPROPERTY()
    AActor *CurrentFocusedActor = nullptr;

    UPROPERTY()
    bool IsCurrentActorReadyForInteraction = false;

    UPROPERTY()
    FTimerHandle InteractionScanTimerHandle;

    UPROPERTY()
    UMythicActivatableWidget *UI_LayerRootWidget;

    UPROPERTY()
    ACommonPlayerController *OwningController;

    UPROPERTY()
    UMythicInteractionPromptWidget *InteractionPromptWidget;

public:
    // The UI Layer responsible for input handling during interaction.
    // Initialized to the native UI_LAYER_GAME tag in the constructor (a header default-member-initializer
    // cannot reliably reference a native gameplay tag at static-init time).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    FGameplayTag GameUILayerName;

    // The class of the widget to display when interacting with an actor
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    TSubclassOf<UMythicInteractionPromptWidget> InteractionPromptWidgetClass;

    UFUNCTION(BlueprintCallable)
    void PauseInteractions(bool bPause);
    void InitializeInteraction(AActor *NewFocusedActor);
    void EndInteraction(AActor *OldFocusedActor);

    void EndStaleInteraction();

    UFUNCTION(BlueprintCallable, Client, Reliable)
    void OnFocusedActorChanged(AActor *NewFocusedActor, AActor *OldFocusedActor);
    void OnFocusedActorChanged_Implementation(AActor *NewFocusedActor, AActor *OldFocusedActor);

    // Any interactable actors within this range will be considered for interaction.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    float InteractionRange = 200.f;

    // Forward-cone gate for interaction targeting (passed as SelectFocusedInteractable's MinDot): a candidate is only
    // focusable if its alignment with the player's forward vector is >= this. -1 (default) = full sphere (focus anything
    // in range — current behaviour, zero regression); 0 = front hemisphere; higher = a tighter look-at cone (cos of the
    // half-angle). The "full-sphere by design today" policy is preserved by the default; a designer opts into a cone.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float InteractionConeMinDot = -1.0f;

    // Rate of scanning for interactable actors
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    float InteractionScanRate = 0.1f;
};
