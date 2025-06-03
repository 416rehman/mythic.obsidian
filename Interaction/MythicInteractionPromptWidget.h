// 

#pragma once

#include "CoreMinimal.h"
#include "IMythicInteractable.h"
#include "Components/VerticalBox.h"
#include "Input/CommonBoundActionBar.h"
#include "Input/CommonBoundActionButtonInterface.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicInteractionPromptWidget.generated.h"

/**
 * 
 */
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
};
