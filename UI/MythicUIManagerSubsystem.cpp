#include "MythicUIManagerSubsystem.h"

#include "CommonLocalPlayer.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
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

    bModalLayerAffectsOtherLayers = true;
    ModalLayer = FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Menu"));
    OtherLayers.Reset();
    OtherLayers.AddTag(FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Game")));
    OtherLayersVisibility = ESlateVisibility::Collapsed;
}

void UMythicUIManagerSubsystem::Deinitialize() {
    Super::Deinitialize();
}

void UMythicUIManagerSubsystem::NotifyPlayerAdded(UCommonLocalPlayer *NewLocalPlayer) {
    Super::NotifyPlayerAdded(NewLocalPlayer);
    if (GetCurrentUIPolicy()) {
        this->PrimaryLocalPlayer = NewLocalPlayer;
        BindModalLayerWatch();
    }
}

void UMythicUIManagerSubsystem::BindModalLayerWatch() {
    if (bModalWatchBound || !bModalLayerAffectsOtherLayers || !ModalLayer.IsValid()) {
        return;
    }

    auto RetryLater = [this](const TCHAR *Why) {
        if (++ModalWatchAttempts > 40) {
            UE_LOG(Myth, Warning, TEXT("BindModalLayerWatch: gave up after %d attempts (%s). The HUD will draw over menus."),
                   ModalWatchAttempts, Why);
            return;
        }
        if (UGameInstance *GI = GetGameInstance()) {
            GI->GetTimerManager().SetTimer(ModalWatchRetryTimer,
                                           FTimerDelegate::CreateUObject(this, &UMythicUIManagerSubsystem::BindModalLayerWatch),
                                           0.25f, false);
        }
    };

    const UGameUIPolicy *Policy = GetCurrentUIPolicy();
    if (!Policy || !PrimaryLocalPlayer) {
        RetryLater(TEXT("no policy or local player"));
        return;
    }
    UPrimaryGameLayout *RootLayout = Policy->GetRootLayout(PrimaryLocalPlayer);
    if (!RootLayout) {
        RetryLater(TEXT("root layout not built yet"));
        return;
    }
    UCommonActivatableWidgetContainerBase *Modal = RootLayout->GetLayerWidget(ModalLayer);
    if (!Modal) {
        RetryLater(TEXT("modal layer not registered yet"));
        return;
    }

    Modal->OnDisplayedWidgetChanged().AddUObject(this, &UMythicUIManagerSubsystem::HandleModalDisplayedWidgetChanged);
    bModalWatchBound = true;
    UE_LOG(Myth, Log, TEXT("BindModalLayerWatch: watching %s; %d layer(s) will collapse while it is showing"),
           *ModalLayer.ToString(), OtherLayers.Num());

    ApplyModalLayerEffect(Modal->GetActiveWidget() != nullptr);
}

void UMythicUIManagerSubsystem::HandleModalDisplayedWidgetChanged(UCommonActivatableWidget *Displayed) {
    ApplyModalLayerEffect(Displayed != nullptr);
}

void UMythicUIManagerSubsystem::ApplyModalLayerEffect(bool bModalShowing) {
    const UGameUIPolicy *Policy = GetCurrentUIPolicy();
    if (!Policy || !PrimaryLocalPlayer) {
        return;
    }
    UPrimaryGameLayout *RootLayout = Policy->GetRootLayout(PrimaryLocalPlayer);
    if (!RootLayout) {
        return;
    }

    if (bModalShowing) {
        for (const FGameplayTag &LayerTag : OtherLayers) {
            UCommonActivatableWidgetContainerBase *Layer = RootLayout->GetLayerWidget(LayerTag);
            if (!Layer) {
                continue;
            }
            if (!LayerVisibilityMap.Contains(LayerTag)) {
                LayerVisibilityMap.Add(LayerTag, Layer->GetVisibility());
            }
            /**
             * The content is hidden as well as the container that holds it.
             *
             * CommonUI picks the active input root by comparing each root's last paint layer, and it reads
             * that from the widget's OWN visibility - never its parents'. A widget left visible inside a
             * collapsed layer keeps reporting the layer it was painted at before it was hidden, and that
             * stale number ties with the menu that just opened. The comparison is strictly greater, so a
             * tie loses: the root stays on the HUD, no menu's bindings are ever active, and every bound
             * action bar in the game renders empty.
             */
            if (UCommonActivatableWidget *Content = Layer->GetActiveWidget()) {
                if (!LayerContentVisibilityMap.Contains(Content)) {
                    LayerContentVisibilityMap.Add(Content, Content->GetVisibility());
                }
                Content->SetVisibility(OtherLayersVisibility);
            }
            Layer->SetVisibility(OtherLayersVisibility);
        }
        return;
    }

    for (const TPair<FGameplayTag, ESlateVisibility> &Pair : LayerVisibilityMap) {
        if (UCommonActivatableWidgetContainerBase *Layer = RootLayout->GetLayerWidget(Pair.Key)) {
            Layer->SetVisibility(Pair.Value);
        }
    }
    LayerVisibilityMap.Empty();

    for (const TPair<TObjectPtr<UCommonActivatableWidget>, ESlateVisibility> &Pair : LayerContentVisibilityMap) {
        if (Pair.Key) {
            Pair.Key->SetVisibility(Pair.Value);
        }
    }
    LayerContentVisibilityMap.Empty();
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

    if (!Widget) {
        UE_LOG(Myth, Warning, TEXT("AddWidgetInstanceToLayer: null Widget"));
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

    if (!Widget) {
        UE_LOG(Myth, Warning, TEXT("RemoveWidgetInstanceFromLayer: null Widget"));
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

                auto ModalLayerChild = Modal->GetActiveWidget();
                if (!ModalLayerChild) {
                    if (LayerVisibilityMap.Num() > 0) {
                        for (const auto &LayerVisibilityPair : LayerVisibilityMap) {
                            auto Layer = RootLayout->GetLayerWidget(LayerVisibilityPair.Key);
                            if (!Layer) {
                                return;
                            }

                            Layer->SetVisibility(LayerVisibilityPair.Value);
                        }

                        LayerVisibilityMap.Empty();
                    }

                    return;
                }

                for (const FGameplayTag &LayerTag : this->OtherLayers) {
                    auto Layer = RootLayout->GetLayerWidget(LayerTag);
                    if (!Layer) {
                        return;
                    }

                    if (!LayerVisibilityMap.Contains(LayerTag)) {
                        LayerVisibilityMap.Add(LayerTag, Layer->GetVisibility());
                    }

                    Layer->SetVisibility(this->OtherLayersVisibility);
                }
            }
        }
    }
}
