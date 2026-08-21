
#include "MythicWarMapScreen.h"

#include "MythicWarMapSubsystem.h"
#include "Mythic.h"

#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicWarMapScreen)


UMythicWarMapScreen* UMythicWarMapScreen::OpenWarMap(const UObject*, APlayerController* OwningPlayer,
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

void UMythicWarMapScreen::CloseWarMap(const UObject*, APlayerController* OwningPlayer,
                                      UMythicWarMapScreen* Screen) {
    if (!Screen) {
        return;
    }
    APlayerController* PC = OwningPlayer ? OwningPlayer : Screen->GetOwningPlayer();
    UPrimaryGameLayout* Layout = PC ? UPrimaryGameLayout::GetPrimaryGameLayout(PC) : nullptr;
    if (Layout) {
        Layout->FindAndRemoveWidgetFromLayer(Screen);
        return;
    }
    Screen->DeactivateWidget();
}

void UMythicWarMapScreen::CloseSelf() {
    CloseWarMap(this, GetOwningPlayer(), this);
}


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


void UMythicWarMapScreen::NativeOnActivated() {
    Super::NativeOnActivated();

    UMythicWarMapSubsystem* Sub = ResolveSubsystem();
    if (Sub) {
        if (!bBound) {
            Sub->OnWarMapChanged.AddDynamic(this, &UMythicWarMapScreen::HandleWarMapChanged);
            BoundSubsystem = Sub;
            bBound = true;
        }
        Sub->RefreshNow();
    } else {
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


void UMythicWarMapScreen::HandleWarMapChanged() {
    PumpToBlueprint();
}

void UMythicWarMapScreen::RefreshFromSubsystem() {
    UMythicWarMapSubsystem* Sub = ResolveSubsystem();
    if (!Sub) {
        return;
    }
    if (!bBound) {
        Sub->OnWarMapChanged.AddDynamic(this, &UMythicWarMapScreen::HandleWarMapChanged);
        BoundSubsystem = Sub;
        bBound = true;
    }
    Sub->RefreshNow();
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

void UMythicWarMapScreen::OnWarMapTextureReady_Implementation(UTexture2D* Texture) {
}

void UMythicWarMapScreen::OnWarMapDataRefreshed_Implementation(const TArray<FMythicWarMapLegendEntry>& Legend,
                                                               const TArray<FMythicWarMapMarker>& Markers,
                                                               const FMythicWarMapMarker& PlayerMarker) {
}
