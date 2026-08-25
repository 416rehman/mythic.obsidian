#include "MythicRunePickerWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

#include "Mythic/Mythic.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "UI/Menu/MythicCharacterPageWidget.h"
#include "UI/MythicUIManagerSubsystem.h"

void UMythicRunePickerRowProxy::HandleClicked() {
    if (UMythicRunePickerWidget *P = Picker.Get()) {
        P->EquipRow(RowIndex);
    }
}

void UMythicRunePickerWidget::NativeConstruct() {
    BuildRows();
    Super::NativeConstruct();
}

void UMythicRunePickerWidget::OpenForSlot(int32 InSlotIndex, UMythicCharacterPageWidget *InPage) {
    SlotIndex = InSlotIndex;
    Page = InPage;

    if (SlotLabel) {
        SlotLabel->SetText(FText::Format(NSLOCTEXT("Mythic", "RunePickerSlot", "Rune Socket {0}"),
                                         FText::AsNumber(SlotIndex + 1)));
    }

    BuildRows();
    RefreshRows();
    ShowOnLayer();
    if (!IsActivated()) {
        ActivateWidget();
    }
}

void UMythicRunePickerWidget::ShowOnLayer() {
    if (bOnLayer || !PickerLayerTag.IsValid()) {
        return;
    }
    UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicUIManagerSubsystem *UIManager = GI ? GI->GetSubsystem<UMythicUIManagerSubsystem>() : nullptr;
    if (!UIManager) {
        return;
    }
    UIManager->AddWidgetInstanceToLayer(PickerLayerTag, GetOwningPlayer(), this);
    bOnLayer = true;
}

void UMythicRunePickerWidget::HideFromLayer() {
    if (!bOnLayer) {
        return;
    }
    if (UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        if (UMythicUIManagerSubsystem *UIManager = GI->GetSubsystem<UMythicUIManagerSubsystem>()) {
            UIManager->RemoveWidgetInstanceFromLayer(PickerLayerTag, GetOwningPlayer(), this);
        }
    }
    bOnLayer = false;
}

void UMythicRunePickerWidget::BuildRows() {
    if (!RowHost || !RowClass || Rows.Num() > 0) {
        return;
    }

    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();

    TArray<FAssetData> Assets;
    Registry.GetAssetsByClass(UMythicRuneDefinition::StaticClass()->GetClassPathName(), Assets);

    Library.Reset();
    for (const FAssetData &Asset : Assets) {
        if (UMythicRuneDefinition *Rune = Cast<UMythicRuneDefinition>(Asset.GetAsset())) {
            Library.Add(Rune);
        }
    }
    Library.Sort([](const UMythicRuneDefinition &A, const UMythicRuneDefinition &B) {
        return A.Name.ToString() < B.Name.ToString();
    });

    for (int32 Index = 0; Index < Library.Num(); ++Index) {
        UUserWidget *RowWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), RowClass);
        if (!RowWidget) {
            continue;
        }

        FMythicRuneRow Row;
        Row.Widget = RowWidget;
        Row.Rune = Library[Index];
        Row.Proxy = NewObject<UMythicRunePickerRowProxy>(this);
        Row.Proxy->Picker = this;
        Row.Proxy->RowIndex = Index;

        // Any button in the row drives the row, so a designer can lay the row out however it reads best.
        TArray<UWidget *> Children;
        RowWidget->WidgetTree->GetAllWidgets(Children);
        for (UWidget *Child : Children) {
            if (UButton *Button = Cast<UButton>(Child)) {
                Button->OnClicked.AddDynamic(Row.Proxy, &UMythicRunePickerRowProxy::HandleClicked);
                break;
            }
        }

        RowHost->AddChild(RowWidget);
        Rows.Add(Row);
    }
}

void UMythicRunePickerWidget::RefreshRows() {
    const UMythicRuneComponent *Runes = nullptr;
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
            Runes = PS->GetRuneComponent();
        }
    }

    for (FMythicRuneRow &Row : Rows) {
        if (!Row.Widget || !Row.Rune) {
            continue;
        }
        Row.bUnlocked = Runes && Runes->IsRuneUnlocked(Row.Rune);

        if (UTextBlock *NameText = Cast<UTextBlock>(Row.Widget->GetWidgetFromName(RowNameText))) {
            NameText->SetText(Row.Rune->Name);
        }
        if (UTextBlock *DescText = Cast<UTextBlock>(Row.Widget->GetWidgetFromName(RowDescriptionText))) {
            DescText->SetText(Row.Rune->Description);
        }
        // The hint is the whole value of showing a locked rune, and noise once it is earned.
        if (UTextBlock *HintText = Cast<UTextBlock>(Row.Widget->GetWidgetFromName(RowHintText))) {
            HintText->SetText(Row.Rune->Hint);
            HintText->SetVisibility(Row.bUnlocked ? ESlateVisibility::Collapsed
                                                  : ESlateVisibility::HitTestInvisible);
        }
        if (UImage *Icon = Cast<UImage>(Row.Widget->GetWidgetFromName(RowIconImage))) {
            if (UTexture2D *Texture = Row.Rune->Icon.LoadSynchronous()) {
                Icon->SetBrushFromTexture(Texture, true);
            }
        }

        Row.Widget->SetRenderOpacity(Row.bUnlocked ? 1.0f : LockedRowOpacity);
        Row.Widget->SetIsEnabled(Row.bUnlocked);
    }
}

void UMythicRunePickerWidget::EquipRow(int32 RowIndex) {
    if (!Rows.IsValidIndex(RowIndex)) {
        return;
    }
    const FMythicRuneRow &Row = Rows[RowIndex];
    if (!Row.bUnlocked || !Row.Rune) {
        return;
    }

    APlayerController *PC = GetOwningPlayer();
    AMythicPlayerState *PS = PC ? PC->GetPlayerState<AMythicPlayerState>() : nullptr;
    UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        UE_LOG(Myth, Warning, TEXT("RunePicker: no rune component to equip into."));
        return;
    }

    // The server re-runs every slotting rule; this call is a request, not the decision.
    Runes->ServerEquipRune(SlotIndex, Row.Rune);

    if (Page) {
        Page->NotifyRunesChanged();
    }
    HideFromLayer();
    DeactivateWidget();
}
