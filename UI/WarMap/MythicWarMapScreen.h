
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicWarMapTypes.h"
#include "MythicWarMapScreen.generated.h"

class UTexture2D;
class UMythicWarMapSubsystem;
class APlayerController;
class ULocalPlayer;

UCLASS(Blueprintable)
class MYTHIC_API UMythicWarMapScreen : public UMythicActivatableWidget {
    GENERATED_BODY()

public:

    /**
     * Open (push) the war-map screen onto MapLayerTag for the player's primary game layout and return the instance
     * (null on failure). Mirrors the CommonUI PushWidgetToLayerStack idiom. Bind this to the M-key input action in
     * BP/input data. LayerTag must be a registered UI.Layer tag (authored). Safe to call when already open — CommonUI
     * stacks another instance, so prefer toggling via the returned handle / IsWarMapOpen if you need single-instance.
     */
    UFUNCTION(BlueprintCallable, Category = "War Map", meta = (DefaultToSelf = "ContextObject", WorldContext = "ContextObject"))
    static UMythicWarMapScreen* OpenWarMap(const UObject* ContextObject, APlayerController* OwningPlayer, FGameplayTag LayerTag,
                                           TSubclassOf<UMythicWarMapScreen> ScreenClass);

    /** Remove (pull) the given war-map screen instance from whichever layer it is on. Deactivates it. */
    UFUNCTION(BlueprintCallable, Category = "War Map", meta = (DefaultToSelf = "ContextObject", WorldContext = "ContextObject"))
    static void CloseWarMap(const UObject* ContextObject, APlayerController* OwningPlayer, UMythicWarMapScreen* Screen);

    /** Close THIS screen instance (convenience for a WBP close button / back action). */
    UFUNCTION(BlueprintCallable, Category = "War Map")
    void CloseSelf();


    /** Ask the subsystem to rebuild now, then re-pump texture + data into the BP events. Safe if the subsystem is null
     *  (no-op until it exists; the bound OnWarMapChanged will pump once data arrives). */
    UFUNCTION(BlueprintCallable, Category = "War Map")
    void RefreshFromSubsystem();

    /** The war-map texture for this local player (lazily built). Convenience pass-through for the WBP. May be null. */
    UFUNCTION(BlueprintCallable, Category = "War Map")
    UTexture2D* GetWarMapTexture() const;

    /** The resolved per-local-player war-map subsystem (may be null very early in activation). */
    UFUNCTION(BlueprintPure, Category = "War Map")
    UMythicWarMapSubsystem* GetWarMapSubsystem() const;


    /**
     * Fired when the texture is (re)available — bind the WBP's map image brush to it here.
     * NativeEvent, not ImplementableEvent: a C++ subclass can paint the map itself (pooled, no graph), and a WBP that
     * overrides it still wins. That keeps the fully-authored path open without forcing every map screen through
     * Blueprint.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "War Map")
    void OnWarMapTextureReady(UTexture2D* Texture);

    /** Fired whenever the map data refreshes — rebuild the WBP's legend rows + marker pins from these. */
    UFUNCTION(BlueprintNativeEvent, Category = "War Map")
    void OnWarMapDataRefreshed(const TArray<FMythicWarMapLegendEntry>& Legend, const TArray<FMythicWarMapMarker>& Markers,
                               const FMythicWarMapMarker& PlayerMarker);


    /** UI layer the screen is pushed to (a registered UI.Layer.* gameplay tag). NOT hardcoded — authored per project. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "War Map", meta = (Categories = "UI.Layer"))
    FGameplayTag MapLayerTag;

protected:
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;

private:
    UMythicWarMapSubsystem* ResolveSubsystem() const;

    UFUNCTION()
    void HandleWarMapChanged();

    void PumpToBlueprint();

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicWarMapSubsystem> BoundSubsystem;

    bool bBound = false;
};
