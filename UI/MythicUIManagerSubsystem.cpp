#include "MythicUIManagerSubsystem.h"

#include "CommonLocalPlayer.h"
#include "Engine/GameInstance.h"
#include "GameFramework/HUD.h"
#include "GameUIPolicy.h"
#include "Mythic.h"
#include "PrimaryGameLayout.h"
#include "Styling/StarshipCoreStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicUIManagerSubsystem)

class FSubsystemCollectionBase;

UMythicUIManagerSubsystem::UMythicUIManagerSubsystem() {}

void UMythicUIManagerSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    //TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UMythicUIManagerSubsystem::Tick), 0.0f);
}

void UMythicUIManagerSubsystem::Deinitialize() {
    Super::Deinitialize();

    //FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
}

void UMythicUIManagerSubsystem::NotifyPlayerAdded(UCommonLocalPlayer *NewLocalPlayer) {
    Super::NotifyPlayerAdded(NewLocalPlayer);
    if (GetCurrentUIPolicy()) {
        this->PrimaryLocalPlayer = NewLocalPlayer;
    }
}

void UMythicUIManagerSubsystem::NotifyPlayerRemoved(UCommonLocalPlayer *OldLocalPlayer) {
    Super::NotifyPlayerRemoved(OldLocalPlayer);
    if (OldLocalPlayer == this->PrimaryLocalPlayer) {
        this->PrimaryLocalPlayer = nullptr;
    }
}

void UMythicUIManagerSubsystem::NotifyPlayerDestroyed(UCommonLocalPlayer *Player) {
    Super::NotifyPlayerDestroyed(Player);
    if (Player == this->PrimaryLocalPlayer) {
        this->PrimaryLocalPlayer = nullptr;
    }
}

void UMythicUIManagerSubsystem::SetModalAffectsOtherLayers(bool bInModalAffectsOtherLayers, FGameplayTag InModalLayer, FGameplayTagContainer InOtherLayers,
                                                           ESlateVisibility InOtherLayersVisibility) {
    this->bModalLayerAffectsOtherLayers = bInModalAffectsOtherLayers;
    this->ModalLayer = InModalLayer;
    this->OtherLayers = InOtherLayers;
    this->OtherLayersVisibility = InOtherLayersVisibility;
}

void UMythicUIManagerSubsystem::AddWidgetInstanceToLayer(FGameplayTag LayerName, const APlayerController *Controller, UCommonActivatableWidget *Widget) {
    const UGameUIPolicy *Policy = GetCurrentUIPolicy();
    if (!Policy) {
        UE_LOG(Myth, Warning, TEXT("No policy found"));
        return;
    }

    UPrimaryGameLayout *RootLayout = Policy->GetRootLayout(PrimaryLocalPlayer);
    if (!RootLayout) {
        UE_LOG(Myth, Warning, TEXT("No root layout found"));
        return;
    }

    auto Layer = RootLayout->GetLayerWidget(LayerName);
    if (!Layer) {
        UE_LOG(Myth, Warning, TEXT("No layer found"));
        return;
    }

    Layer->AddWidgetInstance(*Widget);
}

void UMythicUIManagerSubsystem::RemoveWidgetInstanceFromLayer(FGameplayTag LayerName, const APlayerController *Controller, UCommonActivatableWidget *Widget) {
    const UGameUIPolicy *Policy = GetCurrentUIPolicy();
    if (!Policy) {
        UE_LOG(Myth, Warning, TEXT("No policy found"));
        return;
    }

    UPrimaryGameLayout *RootLayout = Policy->GetRootLayout(PrimaryLocalPlayer);
    if (!RootLayout) {
        UE_LOG(Myth, Warning, TEXT("No root layout found"));
        return;
    }

    auto Layer = RootLayout->GetLayerWidget(LayerName);
    if (!Layer) {
        UE_LOG(Myth, Warning, TEXT("No layer found"));
        return;
    }

    Layer->RemoveWidget(*Widget);
}

void UMythicUIManagerSubsystem::SetFocusBrush(FSlateBrush InBrush) {
    OverridenFocusBrush = InBrush;
    FStarshipCoreStyle::SetFocusBrush(&OverridenFocusBrush);
}

void UMythicUIManagerSubsystem::SetColorOverride(ESlateColorOverride InColorOverride, FLinearColor InColor) {
    switch (InColorOverride) {
    case Selector:
        FStarshipCoreStyle::SetSelectorColor(InColor);
        break;
    case Selection:
        FStarshipCoreStyle::SetSelectionColor(InColor);
        break;
    case InactiveSelection:
        FStarshipCoreStyle::SetInactiveSelectionColor(InColor);
        break;
    case PressedSelection:
        FStarshipCoreStyle::SetPressedSelectionColor(InColor);
        break;
    }
}

bool UMythicUIManagerSubsystem::Tick(float DeltaTime) {
    SyncRootLayoutVisibilityToShowHUD();

    return true;
}

void UMythicUIManagerSubsystem::SyncRootLayoutVisibilityToShowHUD() {
    const UGameUIPolicy *Policy = GetCurrentUIPolicy();
    if (!Policy) {
        return;
    }

    if (PrimaryLocalPlayer) {
        auto World = GetWorld();
        bool bShouldShowUI = true;

        if (const APlayerController *PC = PrimaryLocalPlayer->GetPlayerController(World)) {
            const AHUD *HUD = PC->GetHUD();

            if (HUD && !HUD->bShowHUD) {
                bShouldShowUI = false;
            }
        }

        if (UPrimaryGameLayout *RootLayout = Policy->GetRootLayout(PrimaryLocalPlayer)) {
            const ESlateVisibility DesiredVisibility = bShouldShowUI ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
            if (DesiredVisibility != RootLayout->GetVisibility()) {
                RootLayout->SetVisibility(DesiredVisibility);
            }

            if (this->bModalLayerAffectsOtherLayers) {
                auto Modal = RootLayout->GetLayerWidget(this->ModalLayer);
                if (!Modal) {
                    return;
                }

                // Modal's current child
                auto ModalLayerChild = Modal->GetActiveWidget();
                if (!ModalLayerChild) {
                    if (LayerVisibilityMap.Num() > 0) {
                        // Reset the visibility of the other layers
                        for (const auto &LayerVisibilityPair : LayerVisibilityMap) {
                            auto Layer = RootLayout->GetLayerWidget(LayerVisibilityPair.Key);
                            if (!Layer) {
                                return;
                            }

                            Layer->SetVisibility(LayerVisibilityPair.Value);
                        }

                        // Clear the map
                        LayerVisibilityMap.Empty();
                    }

                    return;
                }

                // Modal has a child so modify the visibility of the other layers
                for (const FGameplayTag &LayerTag : this->OtherLayers) {
                    auto Layer = RootLayout->GetLayerWidget(LayerTag);
                    if (!Layer) {
                        return;
                    }

                    // Save the current visibility of the layer
                    if (!LayerVisibilityMap.Contains(LayerTag)) {
                        LayerVisibilityMap.Add(LayerTag, Layer->GetVisibility());
                    }

                    // Set the visibility of the layer
                    Layer->SetVisibility(this->OtherLayersVisibility);
                }
            }
        }
    }
}
