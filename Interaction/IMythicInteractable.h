
#pragma once

#include "CoreMinimal.h"
#include "Input/CommonGenericInputActionDataTable.h"
#include "UObject/Interface.h"
#include "IMythicInteractable.generated.h"

class UUserWidget;

USTRUCT(BlueprintType)
struct FMythicInteractionData {
    GENERATED_BODY()

    // The data table to use for the interactable
    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    // The name of the primary interaction
    UPROPERTY(BlueprintReadWrite)
    FName PrimaryInteractionName;

    // The name of the secondary interaction
    UPROPERTY(BlueprintReadWrite)
    FName SecondaryInteractionName;

    // The complimentary widget to display along with the interaction UI
    UPROPERTY(BlueprintReadWrite)
    UUserWidget* ComplimentaryWidget = nullptr;
};


UINTERFACE(MinimalAPI, Blueprintable)
class UMythicInteractable : public UInterface {
    GENERATED_BODY()
};

class MYTHIC_API IMythicInteractable {
    GENERATED_BODY()

public:
    // Called when this item has been interacted with using the primary interaction.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
    void OnPrimaryInteract(AActor* Interactor);

    // Called when this item has been interacted with using the secondary interaction.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
    void OnSecondaryInteract(AActor* Interactor);

    // Get the root component to attach the interaction widget to.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
    USceneComponent* GetWidgetAttachmentComponent() const;

    // Get the interaction data for this interactable. Return true/false to indicate if the interaction is possible at this time or not.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
    bool GetInteractionData(AActor* Interactor, FMythicInteractionData& OutInteractionData) const;

    // Called when this item is focused by the player. Visual feedback can be given here.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
    void OnFocused(AActor* Interactor);

    // Called when this item is no longer focused by the player. Visual feedback can be removed here.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
    void OnUnfocused(AActor* Interactor);
};
