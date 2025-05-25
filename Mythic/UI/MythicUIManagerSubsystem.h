// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"
#include "Containers/Ticker.h"
#include "GameUIManagerSubsystem.h"
#include "GameplayTagContainer.h"
#include "Components/SlateWrapperTypes.h"

#include "MythicUIManagerSubsystem.generated.h"

class UPrimaryGameLayout;
class FSubsystemCollectionBase;
class UObject;

static FSlateBrush OverridenFocusBrush;

UENUM()
enum ESlateColorOverride: uint8 {
    Selector,
    Selection,
    InactiveSelection,
    PressedSelection,
};

UCLASS()
class UMythicUIManagerSubsystem : public UGameUIManagerSubsystem {
    GENERATED_BODY()

public:
    UMythicUIManagerSubsystem();

    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual void NotifyPlayerAdded(UCommonLocalPlayer *NewLocalPlayer) override;
    virtual void NotifyPlayerRemoved(UCommonLocalPlayer *OldLocalPlayer) override;
    virtual void NotifyPlayerDestroyed(UCommonLocalPlayer *Player) override;

    // Should pushing to modal layer modify the visibility of all other layers?
    UPROPERTY(EditDefaultsOnly, Category = "Mythic UI Manager")
    bool bModalLayerAffectsOtherLayers = false;

    // If this layer has any children, the other layers visibility will be modified.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic UI Manager", meta = (EditCondition = "bModalLayerAffectsOtherLayers"))
    FGameplayTag ModalLayer;

    // Layers to disable when the modal layer is pushed. Only show in BP if above is set to true.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic UI Manager", meta = (EditCondition = "bModalLayerAffectsOtherLayers"))
    FGameplayTagContainer OtherLayers;

    // The visibility property to set all the other layers to when a modal layer is pushed. Only show in BP if above is set to true.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic UI Manager", meta = (EditCondition = "bModalLayerAffectsOtherLayers"))
    ESlateVisibility OtherLayersVisibility = ESlateVisibility::HitTestInvisible;

    // The map of layers to their visibility state before the modal layer was pushed. Only show in BP if above is set to true.
    UPROPERTY(Transient)
    TMap<FGameplayTag, ESlateVisibility> LayerVisibilityMap;

    // Will set the visibility of Otherlayers to OtherLayersVisibility when ModalLayer has any active widgets.
    UFUNCTION(BlueprintCallable, Category = "Mythic UI Manager")
    void SetModalAffectsOtherLayers(bool bInModalAffectsOtherLayers, FGameplayTag InModalLayer, FGameplayTagContainer InOtherLayers,
                                    ESlateVisibility InOtherLayersVisibility);

    // Add a widget instance to the given layer. This will only work if the layer is registered with the primary layout.
    // Will call the Activate method on the widget instance.
    // To remove the widget, call RemoveWidgetInstanceFromLayer.
    UFUNCTION(BlueprintCallable, Category = "Mythic UI Manager")
    void AddWidgetInstanceToLayer(FGameplayTag LayerName, const APlayerController *Controller, UCommonActivatableWidget *Widget);

    // Remove a widget instance from the given layer. This will only work if the layer is registered with the primary layout.
    // Will call the Deactivate method on the widget instance.
    UFUNCTION(BlueprintCallable, Category = "Mythic UI Manager")
    void RemoveWidgetInstanceFromLayer(FGameplayTag LayerName, const APlayerController *Controller, UCommonActivatableWidget *Widget);

    /************** SLATE OVERRIDES **************/
    // Overrides the Focus brush in the FCoreStyle.
    UFUNCTION(BlueprintCallable, Category="Mythic UI Manager | Slate Overrides")
    static void SetFocusBrush(FSlateBrush InBrush);

    // Overrides the various colors in the FCoreStyle.
    UFUNCTION(BlueprintCallable, Category="Mythic UI Manager | Slate Overrides")
    static void SetColorOverride(ESlateColorOverride InColorOverride, FLinearColor InColor);

private:
    bool Tick(float DeltaTime);
    void SyncRootLayoutVisibilityToShowHUD();

    FTSTicker::FDelegateHandle TickHandle;

    UPROPERTY()
    UCommonLocalPlayer *PrimaryLocalPlayer = nullptr;
};
