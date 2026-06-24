// 

#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "GameplayTagContainer.h"
#include "MythicInteractionPromptWidget.h"
#include "Components/ActorComponent.h"
#include "MythicInteractionComponent.generated.h"

// One interaction-scan candidate reduced to the values the focus-priority decision needs.
struct FMythicInteractCandidate {
    bool bInRange = false;  // actor-origin distance is within InteractionRange of the player
    float Dot = -1.0f;      // alignment of (player→actor) with the player's forward vector, [-1, 1]
    float Distance = 0.0f;  // actor-origin distance from the player
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicInteractionComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UMythicInteractionComponent();

    // Pick the focused interactable from scan candidates: among IN-RANGE candidates, the one best aligned with the
    // player's forward vector (highest Dot) wins; if NONE are in range (only large actors whose body was swept but
    // whose origin lies beyond InteractionRange), fall back to the closest by origin distance. An in-range candidate
    // always beats an out-of-range one (its origin distance is < InteractionRange <= any out-of-range distance), so
    // ordering doesn't matter. Returns INDEX_NONE for no candidates. Pure + static — the player-facing "what does E
    // target" priority is unit-testable. MinDot is the forward-cone eligibility gate: a candidate whose Dot is below it
    // is never focusable (applies to BOTH the in-range and out-of-range-fallback tiers). MinDot = -1 (default) admits
    // the full sphere — exactly the prior behaviour; higher values (0 = front hemisphere, up toward 1 = a tight cone)
    // require the player to look toward the target.
    static int32 SelectFocusedInteractable(TConstArrayView<FMythicInteractCandidate> Candidates, float MinDot = -1.0f);

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
    UMythicActivatableWidget *UI_LayerRootWidget;

    UPROPERTY()
    ACommonPlayerController *OwningController;

    UPROPERTY()
    UMythicInteractionPromptWidget *InteractionPromptWidget;

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

    // Teardown when the focused actor has been destroyed and its pointer is no longer available
    // (GC nulled CurrentFocusedActor). Clears the prompt-widget input bindings and resets the
    // ready flag without dereferencing the dead actor (its attached widget component died with it).
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
