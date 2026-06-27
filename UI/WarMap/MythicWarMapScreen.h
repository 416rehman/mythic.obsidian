// Mythic Living World — Strategic War-Map UI Screen (client-side CommonUI)
// The CommonUI screen that renders the war-map: the territory texture (from UMythicWarMapSubsystem), the settlement /
// encounter / player markers (via the pure world->map transform), and the faction legend. This is the C++ base the
// user's WBP reparents to: it owns activation, subsystem binding, and the map-data pump (texture + legend + markers +
// player marker), and exposes them to the WBP through BlueprintImplementableEvents. The visual layout (where the image
// goes, how a marker/legend row looks) is the authored WBP handoff — this class never builds Slate widgets itself.
//
// 100% client UI. Nothing here is replicated. The screen reads the per-local-player UMythicWarMapSubsystem, which in
// turn reads the already-replicated living-world proxies. Open/close go through the CommonUI primary game layout
// (push/pull by class on a designer-authored layer tag), so the M-key input action + the layer tag are authored in
// BP/input data — see MapLayerTag and the static Open/Close helpers.

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

/**
 * Strategic war-map screen. Reparent the authored WBP to this class. On activation it binds the per-local-player
 * UMythicWarMapSubsystem's change delegate and pumps the current map data into BlueprintImplementableEvents; on
 * deactivation it unbinds. Everything is client-local and event-driven (no per-frame work).
 */
UCLASS(Blueprintable)
class MYTHIC_API UMythicWarMapScreen : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    // ─── Open / close helpers (CommonUI primary-layout push/pull by class) ───

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

    // ─── Refresh API ──────────────────────────────────────

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

    // ─── BP visual hooks (the WBP implements these to paint) ───

    /** Fired when the texture is (re)available — bind the WBP's map image brush to it here. */
    UFUNCTION(BlueprintImplementableEvent, Category = "War Map")
    void OnWarMapTextureReady(UTexture2D* Texture);

    /** Fired whenever the map data refreshes — rebuild the WBP's legend rows + marker pins from these. */
    UFUNCTION(BlueprintImplementableEvent, Category = "War Map")
    void OnWarMapDataRefreshed(const TArray<FMythicWarMapLegendEntry>& Legend, const TArray<FMythicWarMapMarker>& Markers,
                               const FMythicWarMapMarker& PlayerMarker);

    // ─── Designer-authored config ─────────────────────────

    /** UI layer the screen is pushed to (a registered UI.Layer.* gameplay tag). NOT hardcoded — authored per project. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "War Map", meta = (Categories = "UI.Layer"))
    FGameplayTag MapLayerTag;

protected:
    //~UCommonActivatableWidget
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    //~End UCommonActivatableWidget

private:
    /** Resolve the subsystem from this widget's owning local player. Null if unavailable. */
    UMythicWarMapSubsystem* ResolveSubsystem() const;

    /** Bound to UMythicWarMapSubsystem::OnWarMapChanged — re-pumps texture + data into the BP events. */
    UFUNCTION()
    void HandleWarMapChanged();

    /** Push the current texture + legend + markers + player marker into the BP events. */
    void PumpToBlueprint();

    /** Weak ref to the subsystem we bound to (for a clean unbind in NativeOnDeactivated even if the player changed). */
    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicWarMapSubsystem> BoundSubsystem;

    /** Whether OnWarMapChanged is currently bound (idempotent bind/unbind guard). */
    bool bBound = false;
};
