
#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "GameplayTagContainer.h"
#include "MythicInteractionPromptWidget.h"
#include "Components/ActorComponent.h"
#include "MythicInteractionComponent.generated.h"

class UWidgetComponent;

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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Runs one bounded legacy-interaction nomination pass; presentable entities publish into LocalPlayer attention. */
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

    /** Legacy prompt component owned by this LocalPlayer interaction instance; never found or destroyed by actor tag. */
    UPROPERTY(Transient)
    TObjectPtr<UWidgetComponent> ActiveInteractionWidgetComponent;

public:
    /**
     * CommonUI layer that owns legacy interaction bindings. Initialized to UI.Layer.Game in the constructor because
     * native gameplay tags are not safe to reference from a header default-member initializer during static startup.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    FGameplayTag GameUILayerName;

    /** Legacy prompt class used only for non-presentable interactables; contextual entities render through the HUD. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    TSubclassOf<UMythicInteractionPromptWidget> InteractionPromptWidgetClass;

    /** Starts or stops the bounded interaction scan and clears current local focus when paused. */
    UFUNCTION(BlueprintCallable)
    void PauseInteractions(bool bPause);
    void InitializeInteraction(AActor *NewFocusedActor);
    void EndInteraction(AActor *OldFocusedActor);

    void EndStaleInteraction();

    /** Publishes a local focus transition, tears down the old prompt, and initializes the new interaction surface. */
    UFUNCTION(BlueprintCallable, Client, Reliable)
    void OnFocusedActorChanged(AActor *NewFocusedActor, AActor *OldFocusedActor);
    void OnFocusedActorChanged_Implementation(AActor *NewFocusedActor, AActor *OldFocusedActor);

    /** Maximum legacy interaction nomination radius in centimeters. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    float InteractionRange = 200.f;

    // Forward-cone gate for interaction targeting (passed as SelectFocusedInteractable's MinDot): a candidate is only
    // focusable if its alignment with the player's forward vector is >= this. -1 (default) = full sphere (focus anything
    // in range — current behaviour, zero regression); 0 = front hemisphere; higher = a tighter look-at cone (cos of the
    // half-angle). The "full-sphere by design today" policy is preserved by the default; a designer opts into a cone.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float InteractionConeMinDot = -1.0f;

    /** Seconds between bounded legacy-interaction nomination passes. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
    float InteractionScanRate = 0.1f;
};
