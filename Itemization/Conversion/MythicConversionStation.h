#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/IMythicInteractable.h"
#include "Itemization/InventoryProviderInterface.h"
#include "ViewModels/ConversionViewModels.h"
#include "MythicConversionStation.generated.h"

class UConversionStationComponent;
class UMythicInventoryComponent;
class UStaticMeshComponent;
class USceneComponent;
class UCommonActivatableWidget;
class UCommonGenericInputActionDataTable;

UCLASS()
class MYTHIC_API AMythicConversionStation : public AActor, public IMythicInteractable, public IInventoryProviderInterface {
    GENERATED_BODY()

public:
    AMythicConversionStation();
    void SetupLocalViewModel();

    virtual void BeginPlay() override;

    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;
    virtual UAbilitySystemComponent *GetSchematicsASC() const override;

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    UFUNCTION(BlueprintCallable, Category = "Conversion")
    UConversionStationComponent *GetConversionComponent() const { return ConversionComponent; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UMythicInventoryComponent *GetStationInventory() const { return StationInventory; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ViweModel")
    UConversionStationVM *StationViewModel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station")
    USceneComponent *SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station")
    UStaticMeshComponent *Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Conversion")
    UConversionStationComponent *ConversionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    UMythicInventoryComponent *StationInventory;

    // Interaction prompt data.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Use");

    // Fired on the local interacting client so the Blueprint can push the station WBP onto the CommonUI layer,
    // binding GetStationInventory()->GetViewModel() and a UConversionStationVM. (Editor handoff.)
    UFUNCTION(BlueprintImplementableEvent, Category = "Conversion")
    void OnStationOpened(APlayerController *Interactor);

    static class AController *ResolveController(AActor *Interactor);
};
