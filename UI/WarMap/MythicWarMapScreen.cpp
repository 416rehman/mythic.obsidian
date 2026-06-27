// Mythic Living World — Strategic War-Map UI Screen (implementation)
// Client-only CommonUI screen. Binds the per-local-player war-map subsystem and pumps texture + legend + markers into
// BlueprintImplementableEvents. Open/close go through the CommonUI primary game layout (push/pull by class).

#include "MythicWarMapScreen.h"

#include "MythicWarMapSubsystem.h"
#include "Mythic.h"

#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicWarMapScreen)

// ─────────────────────────────────────────────────────────────
// Open / close
// ─────────────────────────────────────────────────────────────

UMythicWarMapScreen* UMythicWarMapScreen::OpenWarMap(const UObject* /*ContextObject*/, APlayerController* OwningPlayer,
                                                     FGameplayTag LayerTag, TSubclassOf<UMythicWarMapScreen> ScreenClass) {
    if (!OwningPlayer) {
        UE_LOG(Myth, Warning, TEXT("OpenWarMap: null OwningPlayer"));
        return nullptr;
    }
    if (!ScreenClass) {
        UE_LOG(Myth, Warning, TEXT("OpenWarMap: null ScreenClass"));
        return nullptr;
    }
    if (!LayerTag.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("OpenWarMap: invalid LayerTag (set a registered UI.Layer.* tag on the screen)"));
        return nullptr;
    }

    UPrimaryGameLayout* Layout = UPrimaryGameLayout::GetPrimaryGameLayout(OwningPlayer);
    if (!Layout) {
        UE_LOG(Myth, Warning, TEXT("OpenWarMap: no primary game layout for player"));
        return nullptr;
    }

    return Layout->PushWidgetToLayerStack<UMythicWarMapScreen>(LayerTag, ScreenClass.Get());
}

void UMythicWarMapScreen::CloseWarMap(const UObject* /*ContextObject*/, APlayerController* OwningPlayer,
                                      UMythicWarMapScreen* Screen) {
    if (!Screen) {
        return;
    }
    // Prefer the owning player's layout; fall back to the screen's own owning player so a WBP close button works even
    // if the caller passed a stale controller.
    APlayerController* PC = OwningPlayer ? OwningPlayer : Screen->GetOwningPlayer();
    UPrimaryGameLayout* Layout = PC ? UPrimaryGameLayout::GetPrimaryGameLayout(PC) : nullptr;
    if (Layout) {
        Layout->FindAndRemoveWidgetFromLayer(Screen);
        return;
    }
    // No layout reachable — deactivate directly so we at least tear down activation/binding.
    Screen->DeactivateWidget();
}

void UMythicWarMapScreen::CloseSelf() {
    CloseWarMap(this, GetOwningPlayer(), this);
}

// ─────────────────────────────────────────────────────────────
// Subsystem resolution
// ─────────────────────────────────────────────────────────────

UMythicWarMapSubsystem* UMythicWarMapScreen::ResolveSubsystem() const {
    if (const ULocalPlayer* LP = GetOwningLocalPlayer()) {
        return LP->GetSubsystem<UMythicWarMapSubsystem>();
    }
    return nullptr;
}

UMythicWarMapSubsystem* UMythicWarMapScreen::GetWarMapSubsystem() const {
    return ResolveSubsystem();
}

UTexture2D* UMythicWarMapScreen::GetWarMapTexture() const {
    if (UMythicWarMapSubsystem* Sub = ResolveSubsystem()) {
        return Sub->GetWarMapTexture();
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────
// Activation lifecycle
// ─────────────────────────────────────────────────────────────

void UMythicWarMapScreen::NativeOnActivated() {
    Super::NativeOnActivated();

    UMythicWarMapSubsystem* Sub = ResolveSubsystem();
    if (Sub) {
        if (!bBound) {
            Sub->OnWarMapChanged.AddDynamic(this, &UMythicWarMapScreen::HandleWarMapChanged);
            BoundSubsystem = Sub;
            bBound = true;
        }
        // Trigger a rebuild; RefreshNow broadcasts OnWarMapChanged -> HandleWarMapChanged -> PumpToBlueprint, so the WBP
        // receives the texture + data exactly once even on first open.
        Sub->RefreshNow();
    } else {
        // Subsystem not reachable yet (early activation). The WBP can call RefreshFromSubsystem later, and once a proxy
        // change arrives the next activation will bind. Nothing to pump now.
        UE_LOG(Myth, Verbose, TEXT("WarMapScreen activated before subsystem available"));
    }
}

void UMythicWarMapScreen::NativeOnDeactivated() {
    if (bBound) {
        if (UMythicWarMapSubsystem* Sub = BoundSubsystem.Get()) {
            Sub->OnWarMapChanged.RemoveAll(this);
        }
        BoundSubsystem = nullptr;
        bBound = false;
    }
    Super::NativeOnDeactivated();
}

// ─────────────────────────────────────────────────────────────
// Data pump
// ─────────────────────────────────────────────────────────────

void UMythicWarMapScreen::HandleWarMapChanged() {
    PumpToBlueprint();
}

void UMythicWarMapScreen::RefreshFromSubsystem() {
    UMythicWarMapSubsystem* Sub = ResolveSubsystem();
    if (!Sub) {
        return;
    }
    // Bind on demand if a prior activation happened before the subsystem existed.
    if (!bBound) {
        Sub->OnWarMapChanged.AddDynamic(this, &UMythicWarMapScreen::HandleWarMapChanged);
        BoundSubsystem = Sub;
        bBound = true;
    }
    Sub->RefreshNow(); // broadcasts -> HandleWarMapChanged -> PumpToBlueprint
}

void UMythicWarMapScreen::PumpToBlueprint() {
    UMythicWarMapSubsystem* Sub = ResolveSubsystem();
    if (!Sub) {
        return;
    }

    OnWarMapTextureReady(Sub->GetWarMapTexture());

    TArray<FMythicWarMapLegendEntry> Legend;
    Sub->GetLegendEntries(Legend);

    TArray<FMythicWarMapMarker> Markers;
    Sub->GetMarkers(Markers);

    const FMythicWarMapMarker PlayerMarker = Sub->GetPlayerMarker();

    OnWarMapDataRefreshed(Legend, Markers, PlayerMarker);
}
