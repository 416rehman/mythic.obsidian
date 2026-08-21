
#pragma once

#include "CoreMinimal.h"
#include "IMythicInteractable.h"
#include "Components/VerticalBox.h"
#include "Input/CommonBoundActionBar.h"
#include "Input/CommonBoundActionButtonInterface.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicInteractionPromptWidget.generated.h"

UCLASS()
class MYTHIC_API UMythicInteractionPromptWidget : public UMythicActivatableWidget {
    GENERATED_BODY()
public:
    // Vertical box for showing the interaction prompt
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox *VerticalBox;

    UPROPERTY(EditAnywhere, Category = EntryLayout, meta=(MustImplement = "/Script/CommonUI.CommonBoundActionButtonInterface"))
    TSubclassOf<UCommonButtonBase> ActionButtonClass;

    UPROPERTY(BlueprintReadOnly)
    FUIActionBindingHandle PrimaryInteractionHandle;

    UPROPERTY(BlueprintReadOnly)
    FUIActionBindingHandle SecondaryInteractionHandle;

    /**
     * How many action buttons the prompt keeps alive. Two covers primary + secondary, which is every interactable
     * in the game today; a third would be built on demand and then kept.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "1"))
    int32 ActionButtonPoolSize = 2;

    UFUNCTION()
    void Clear();

    // Use this function to update the Widget state based on the InteractionData
    UFUNCTION(BlueprintCallable)
    void SetInteractionData(FMythicInteractionData InInteractionData, AActor *InInteractableActor, APlayerController *InPlayerController,
                            UMythicActivatableWidget *UI_LayerRootWidget);

    // Called when InteractionData is updated
    UFUNCTION(BlueprintImplementableEvent)
    void OnInteractionDataUpdated(FMythicInteractionData InInteractionData, AActor *InInteractableActor);
    void OnInteractionDataUpdated_Implementation(FMythicInteractionData InInteractionData, AActor *InInteractableActor) {}

private:
    UPROPERTY()
    TArray<TObjectPtr<UCommonButtonBase>> ActionButtonPool;

    UPROPERTY()
    TObjectPtr<UWidget> ActiveComplimentaryWidget;

    UCommonButtonBase *GetOrCreateActionButton(int32 Index);

    void ShowActionButton(int32 Index, const FUIActionBindingHandle &Handle);

    void CollapseActionButtonsFrom(int32 Index);
};
