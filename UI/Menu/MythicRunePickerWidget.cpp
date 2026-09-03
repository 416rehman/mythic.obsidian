#include "MythicRunePickerWidget.h"

#include "Animation/WidgetAnimation.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/RichTextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "TimerManager.h"

#include "Mythic/Mythic.h"
#include "Player/MythicPlayerState.h"
#include "Progression/MythicAchievementSet.h"
#include "Progression/MythicUnlockRuleSet.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "Settings/MythicDeveloperSettings.h"
#include "UI/Menu/MythicCharacterPageWidget.h"
#include "UI/Menu/MythicRuneSocketWidget.h"
#include "UI/MythicUIManagerSubsystem.h"
#include "UI/MythicUIStyle.h"
#include "UI/ViewModels/MythicRuneDescriber.h"
#include "UI/Widgets/MythicInputGlyph.h"
#include "UI/Widgets/MythicSectionHeader.h"

namespace {
FText RunePicker_SocketName(int32 SlotIndex) {
    return FText::Format(NSLOCTEXT("Mythic", "RunePickerSocketName", "Socket {0}"), FText::AsNumber(SlotIndex + 1));
}

FText RunePicker_SealedNotice(const FText &Deed) {
    return FText::Format(NSLOCTEXT("Mythic", "RunePickerSealed", "Sealed - earn {0}"), Deed);
}

FText RunePicker_SealedLine(int32 SlotIndex, FText &OutDeed) {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const UMythicUnlockRuleSet *Rules = Settings ? Settings->DefaultUnlockRuleSet.LoadSynchronous() : nullptr;
    const UMythicAchievementSet *Deeds = Settings ? Settings->DefaultAchievementSet.LoadSynchronous() : nullptr;
    return UMythicRuneDescriber::DescribeSealedSocket(SlotIndex, Rules, Deeds, OutDeed);
}
}

void UMythicRunePickerWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();

    // At initialise, not construct: the picker is in no tree until it opens, and a pool built on the first
    // click is a frame spike on the first click.
    BuildLibrary();
    BuildStrip();
    BuildCells();

    if (DetailSocket) {
        DetailSocket->SetInteractive(false);
    }
    if (Dismiss) {
        Dismiss->OnClicked.AddUniqueDynamic(this, &UMythicRunePickerWidget::HandleDismissClicked);
    }
    if (CloseButton) {
        CloseButton->OnClicked().AddWeakLambda(this, [this]() { Close(); });
    }
    if (FooterCloseButton) {
        FooterCloseButton->OnClicked().AddWeakLambda(this, [this]() { Close(); });
    }
    if (UnequipButton) {
        UnequipButton->OnClicked().AddWeakLambda(this, [this]() { UnequipCurrent(); });
    }
    if (Glyph_Select) {
        Glyph_Select->SetEnhancedAction(PrimaryAction.LoadSynchronous());
    }
    if (Glyph_Actions) {
        Glyph_Actions->SetEnhancedAction(ActionsAction.LoadSynchronous());
    }
    if (Lbl_Back) {
        Lbl_Back->SetText(NSLOCTEXT("Mythic", "RunePickerBack", "Back"));
    }
    if (OpenAnim) {
        FWidgetAnimationDynamicEvent Finished;
        Finished.BindDynamic(this, &UMythicRunePickerWidget::HandleOpenAnimFinished);
        BindToAnimationFinished(OpenAnim, Finished);
    }
    if (CloseAnim) {
        FWidgetAnimationDynamicEvent Finished;
        Finished.BindDynamic(this, &UMythicRunePickerWidget::HandleCloseAnimFinished);
        BindToAnimationFinished(CloseAnim, Finished);
    }
}

void UMythicRunePickerWidget::BuildLibrary() {
    if (Library.Num() > 0) {
        return;
    }
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    TArray<FAssetData> Assets;
    Module.Get().GetAssetsByClass(UMythicRuneDefinition::StaticClass()->GetClassPathName(), Assets);

    for (const FAssetData &Asset : Assets) {
        if (UMythicRuneDefinition *Rune = Cast<UMythicRuneDefinition>(Asset.GetAsset())) {
            Library.Add(Rune);
        }
    }
    Library.Sort([](const UMythicRuneDefinition &A, const UMythicRuneDefinition &B) {
        return A.Name.ToString() < B.Name.ToString();
    });

    // Twelve never-streamed UI textures, loaded once here and held by the array so no redraw pays for them.
    PreloadedIcons.Reset(Library.Num());
    for (const UMythicRuneDefinition *Rune : Library) {
        PreloadedIcons.Add(Rune->Icon.LoadSynchronous());
    }
}

void UMythicRunePickerWidget::BuildStrip() {
    if (!SocketStrip || !SocketClass || StripSockets.Num() > 0) {
        return;
    }
    const float Gap = FMythicUIStyle::Get().SpaceL;
    const int32 Count = FMath::Clamp(StripSocketCount, 1, 8);
    for (int32 i = 0; i < Count; ++i) {
        UMythicRuneSocketWidget *Socket = CreateWidget<UMythicRuneSocketWidget>(this, SocketClass);
        if (!Socket) {
            continue;
        }
        Socket->SetSlotIndex(i);
        Socket->SetInteractive(true);
        Socket->OnPressed.AddUniqueDynamic(this, &UMythicRunePickerWidget::HandleStripPressed);
        Socket->OnHoverChanged.AddUniqueDynamic(this, &UMythicRunePickerWidget::HandleStripHoverChanged);
        Socket->OnFocusChanged.AddUniqueDynamic(this, &UMythicRunePickerWidget::HandleStripFocusChanged);
        if (UHorizontalBoxSlot *CellSlot = Cast<UHorizontalBoxSlot>(SocketStrip->AddChild(Socket))) {
            CellSlot->SetVerticalAlignment(VAlign_Center);
            CellSlot->SetPadding(FMargin(i == 0 ? 0.0f : Gap, 0.0f, 0.0f, 0.0f));
        }
        StripSockets.Add(Socket);
    }
    StripLastDrawn.SetNum(StripSockets.Num());
    StripLastUnlocked.SetNum(StripSockets.Num());
}

void UMythicRunePickerWidget::BuildCells() {
    if (!CellGrid || !CellClass || Cells.Num() > 0) {
        return;
    }
    const int32 Columns = FMath::Max(GridColumns, 1);
    // One tile more than there are runes: the first is "No Rune", which empties the socket.
    for (int32 i = 0; i < Library.Num() + 1; ++i) {
        UMythicRunePickerCellWidget *Cell = CreateWidget<UMythicRunePickerCellWidget>(this, CellClass);
        if (!Cell) {
            continue;
        }
        const int32 Index = Cells.Num();
        Cell->SetCellIndex(Index);
        Cell->OnPressed.AddUniqueDynamic(this, &UMythicRunePickerWidget::HandleCellPressed);
        Cell->OnHoverChanged.AddUniqueDynamic(this, &UMythicRunePickerWidget::HandleCellHoverChanged);
        Cell->OnFocusChanged.AddUniqueDynamic(this, &UMythicRunePickerWidget::HandleCellFocusChanged);
        if (UUniformGridSlot *GridSlot = CellGrid->AddChildToUniformGrid(Cell, Index / Columns, Index % Columns)) {
            GridSlot->SetHorizontalAlignment(HAlign_Fill);
            GridSlot->SetVerticalAlignment(VAlign_Fill);
        }
        Cells.Add(Cell);
    }
    CellState.SetNum(Cells.Num());
}

void UMythicRunePickerWidget::OpenForSlot(int32 SlotIndex, UMythicCharacterPageWidget *InPage) {
    Page = InPage;
    bClosing = false;
    HoveredCell = FocusedCell = HoveredStrip = FocusedStrip = INDEX_NONE;
    // The first draw after opening records state; only later changes animate the strip.
    bStripDrawn = false;

    SelectSocket(SlotIndex);

    ShowOnLayer();
    if (!IsActivated()) {
        ActivateWidget();
    }
}

void UMythicRunePickerWidget::SelectSocket(int32 SlotIndex) {
    SelectedSlot = SlotIndex;
    SortCells();
    RefreshAll();
}

void UMythicRunePickerWidget::SetRuneSource(UMythicRuneComponent *Runes) {
    RuneSource = Runes;
}

void UMythicRunePickerWidget::NativeOnActivated() {
    Super::NativeOnActivated();
    bClosing = false;
    BindRunes();

    if (UInputAction *Action = PrimaryAction.LoadSynchronous()) {
        FInputActionExecutedDelegate Callback;
        Callback.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UMythicRunePickerWidget, HandlePrimaryAction));
        RegisterInputActionBinding(Action, IE_Pressed, Callback, true, PrimaryBinding);
    }

    // The dedicated unequip binding belongs to SetActionsBindingActive alone. Registering it here as well would
    // orphan whichever registration the other one overwrote, leaving a prompt no unregister can reach.
    RefreshAll();

    if (OpenAnim) {
        if (Card) {
            Card->ForceVolatile(true);
        }
        PlayAnimation(OpenAnim);
    }
}

void UMythicRunePickerWidget::NativeOnDeactivated() {
    bOnLayer = false;
    bClosing = false;
    UnbindRunes();
    UnregisterAllBindings();
    PrimaryBinding = FInputActionBindingHandle();
    ActionsBinding = FInputActionBindingHandle();
    if (Card) {
        Card->ForceVolatile(false);
    }
    Super::NativeOnDeactivated();
}

bool UMythicRunePickerWidget::NativeOnHandleBackAction() {
    Close();
    return true;
}

UWidget *UMythicRunePickerWidget::NativeGetDesiredFocusTarget() const {
    int32 FirstUnlocked = INDEX_NONE;
    for (int32 i = 0; i < Cells.Num() && i < CellState.Num(); ++i) {
        if (!Cells[i]) {
            continue;
        }
        // The clear tile is never the opening focus: a pad press on it would only shake an already-empty socket.
        if (CellState[i].bClear) {
            continue;
        }
        if (CellState[i].Worn == EMythicRuneWorn::Here) {
            if (UWidget *Hit = Cells[i]->GetFocusWidget()) {
                return Hit;
            }
        }
        if (FirstUnlocked == INDEX_NONE && CellState[i].bUnlocked && Cells[i]->GetFocusWidget()) {
            FirstUnlocked = i;
        }
    }
    if (FirstUnlocked != INDEX_NONE) {
        return Cells[FirstUnlocked]->GetFocusWidget();
    }
    if (UMythicRuneSocketWidget *Socket = GetStripSocket(SelectedSlot)) {
        if (UWidget *Hit = Socket->GetFocusWidget()) {
            return Hit;
        }
    }
    if (UWidget *First = FMythicUIStyle::FindFirstFocusable(const_cast<UMythicRunePickerWidget *>(this))) {
        return First;
    }
    return Super::NativeGetDesiredFocusTarget();
}

void UMythicRunePickerWidget::Close() {
    if (bClosing) {
        return;
    }
    bClosing = true;
    // Nothing on screen means nothing to animate; a close that waited on an animation no one ticks would strand
    // the modal.
    if (CloseAnim && IsActivated() && GetCachedWidget().IsValid()) {
        if (Card) {
            Card->ForceVolatile(true);
        }
        PlayAnimation(CloseAnim);
        return;
    }
    FinishClose();
}

void UMythicRunePickerWidget::HandleOpenAnimFinished() {
    if (Card) {
        Card->ForceVolatile(false);
    }
}

void UMythicRunePickerWidget::HandleCloseAnimFinished() {
    if (Card) {
        Card->ForceVolatile(false);
    }
    FinishClose();
}

void UMythicRunePickerWidget::FinishClose() {
    const bool bWasShown = bOnLayer || IsActivated();
    bClosing = false;
    HideFromLayer();
    if (IsActivated()) {
        DeactivateWidget();
    }
    if (bWasShown) {
        ReturnFocusToPage();
    }
}

void UMythicRunePickerWidget::ReturnFocusToPage() {
    // One tick late: the page is not back in a visible Slate path on the frame the modal leaves, and SetFocus on a
    // widget without one fails silently.
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    TWeakObjectPtr<UMythicCharacterPageWidget> WeakPage = Page;
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [WeakPage]() {
        UMythicCharacterPageWidget *Owner = WeakPage.Get();
        UMythicRuneSocketWidget *Socket = Owner ? Owner->GetSocketWidget(Owner->LastOpenedSocket) : nullptr;
        UWidget *Hit = Socket ? Socket->GetFocusWidget() : nullptr;
        if (Hit) {
            Hit->SetFocus();
        }
    }));
}

void UMythicRunePickerWidget::HandleDismissClicked() {
    Close();
}

void UMythicRunePickerWidget::HandlePrimaryAction() {
    const int32 Cell = FocusedCell != INDEX_NONE ? FocusedCell : HoveredCell;
    if (Cell != INDEX_NONE) {
        ActivateCell(Cell);
        return;
    }
    if (FocusedStrip != INDEX_NONE) {
        HandleStripPressed(FocusedStrip);
        return;
    }
    // The binding consumes pad Accept before Slate routes it, so a focused footer button never hears its own click.
    auto IsFocused = [](const UCommonButtonBase *Button) {
        return Button && (Button->HasAnyUserFocus() || Button->HasFocusedDescendants());
    };
    if (IsFocused(UnequipButton)) {
        UnequipCurrent();
    }
    else if (IsFocused(CloseButton) || IsFocused(FooterCloseButton)) {
        Close();
    }
}

void UMythicRunePickerWidget::HandleActionsAction() {
    UnequipCurrent();
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

UMythicRuneComponent *UMythicRunePickerWidget::FindRuneComponent() const {
    if (UMythicRuneComponent *Source = RuneSource.Get()) {
        return Source;
    }
    const APlayerController *PC = GetOwningPlayer();
    const AMythicPlayerState *PS = PC ? PC->GetPlayerState<AMythicPlayerState>() : nullptr;
    return PS ? PS->GetRuneComponent() : nullptr;
}

void UMythicRunePickerWidget::BindRunes() {
    UMythicRuneComponent *Runes = FindRuneComponent();
    if (!Runes) {
        return;
    }
    if (BoundRunes.IsValid() && BoundRunes.Get() != Runes) {
        UnbindRunes();
    }
    Runes->OnRunesChanged.AddUniqueDynamic(this, &UMythicRunePickerWidget::HandleRunesChanged);
    Runes->OnRuneRefused.AddUniqueDynamic(this, &UMythicRunePickerWidget::HandleRuneRefused);
    BoundRunes = Runes;
}

void UMythicRunePickerWidget::UnbindRunes() {
    if (UMythicRuneComponent *Runes = BoundRunes.Get()) {
        Runes->OnRunesChanged.RemoveDynamic(this, &UMythicRunePickerWidget::HandleRunesChanged);
        Runes->OnRuneRefused.RemoveDynamic(this, &UMythicRunePickerWidget::HandleRuneRefused);
    }
    BoundRunes.Reset();
}

void UMythicRunePickerWidget::HandleRunesChanged() {
    RefreshAll();
}

void UMythicRunePickerWidget::HandleRuneRefused(int32 SlotIndex, EMythicRuneRefusal Reason) {
    UMythicRuneSocketWidget *Socket = GetStripSocket(SlotIndex);
    if (!Socket) {
        Socket = GetStripSocket(SelectedSlot);
    }
    if (Socket) {
        Socket->PlayRefuse();
    }
    SetSocketHeaderTrailing(UMythicRuneComponent::DescribeRefusal(Reason, INDEX_NONE));
}

void UMythicRunePickerWidget::RefreshAll() {
    RefreshStrip();
    RefreshCells();
    RefreshHeaders();
    RefreshDetail();
    RefreshPrompts();
}

int32 UMythicRunePickerWidget::FindWornSlot(const UMythicRuneComponent *Runes,
                                            const UMythicRuneDefinition *Rune) const {
    if (!Runes || !Rune) {
        return INDEX_NONE;
    }
    for (int32 SocketIndex = 0; SocketIndex < Runes->MaxSlots; ++SocketIndex) {
        if (Runes->GetRuneInSlot(SocketIndex) == Rune) {
            return SocketIndex;
        }
    }
    return INDEX_NONE;
}

void UMythicRunePickerWidget::RefreshStrip() {
    const UMythicRuneComponent *Runes = FindRuneComponent();
    const UMythicCharacterPageWidget *Owner = Page.Get();

    for (int32 i = 0; i < StripSockets.Num(); ++i) {
        UMythicRuneSocketWidget *Socket = StripSockets[i];
        if (!Socket) {
            continue;
        }
        const bool bUnlocked = Runes && Runes->IsSlotUnlocked(i);
        const UMythicRuneDefinition *Worn = Runes ? Runes->GetRuneInSlot(i) : nullptr;
        const FSoftObjectPath Now = Worn ? FSoftObjectPath(Worn) : FSoftObjectPath();
        const EMythicRuneSocketState State = !bUnlocked ? EMythicRuneSocketState::Sealed
                                             : Worn      ? EMythicRuneSocketState::Filled
                                                         : EMythicRuneSocketState::Empty;
        Socket->SetState(State, FindPreloadedIcon(Worn),
                         Owner ? Owner->RuneCategoryColour(Worn) : FLinearColor::White);
        Socket->SetSelected(i == SelectedSlot);

        // Same diff the page runs, so a landing plays here and on the plinth from one broadcast.
        if (bStripDrawn && StripLastDrawn.IsValidIndex(i)) {
            if (bUnlocked && !StripLastUnlocked[i]) {
                Socket->PlayUnseal();
            }
            if (Now != StripLastDrawn[i]) {
                if (Now.IsValid()) {
                    Socket->PlayLand();
                }
                else if (StripLastDrawn[i].IsValid()) {
                    Socket->PlayUnland();
                }
            }
        }
        if (StripLastDrawn.IsValidIndex(i)) {
            StripLastDrawn[i] = Now;
            StripLastUnlocked[i] = bUnlocked;
        }
    }
    bStripDrawn = StripSockets.Num() > 0;
}

int32 UMythicRunePickerWidget::CategoryRank(const UMythicRuneDefinition *Rune) const {
    const UMythicCharacterPageWidget *Owner = Page.Get();
    if (!Owner || !Rune) {
        return MAX_int32;
    }
    const TArray<FMythicRuneCategoryColour> &Order = Owner->GetRuneCategoryColours();
    for (int32 i = 0; i < Order.Num(); ++i) {
        if (Order[i].Category.IsValid() && Rune->CategoryTags.HasTag(Order[i].Category)) {
            return i;
        }
    }
    return MAX_int32;
}

void UMythicRunePickerWidget::SortCells() {
    const UMythicRuneComponent *Runes = FindRuneComponent();

    struct FSortKey {
        int32 LibraryIndex = INDEX_NONE;
        int32 Category = 0;
        int32 Band = 0;
        FString Name;
    };
    TArray<FSortKey> Keys;
    Keys.Reserve(Library.Num());
    for (int32 i = 0; i < Library.Num(); ++i) {
        const UMythicRuneDefinition *Rune = Library[i];
        if (!Rune) {
            continue;
        }
        FSortKey Key;
        Key.LibraryIndex = i;
        Key.Category = CategoryRank(Rune);
        const bool bUnlocked = Runes && Runes->IsRuneUnlocked(Rune);
        Key.Band = FindWornSlot(Runes, Rune) == SelectedSlot && SelectedSlot != INDEX_NONE ? 0 : bUnlocked ? 1 : 2;
        Key.Name = Rune->Name.ToString();
        Keys.Add(MoveTemp(Key));
    }
    Keys.Sort([](const FSortKey &A, const FSortKey &B) {
        if (A.Category != B.Category) {
            return A.Category < B.Category;
        }
        if (A.Band != B.Band) {
            return A.Band < B.Band;
        }
        return A.Name < B.Name;
    });

    // Cells keep their place in the grid and are re-pointed, because reordering children is the most expensive
    // thing a widget tree can do. The order then holds until the target changes: a re-sort on every
    // OnRunesChanged would move the rune out from under pad focus and the pointer, and the next press would
    // equip its neighbour.
    for (int32 CellIndex = 0; CellIndex < CellState.Num(); ++CellIndex) {
        // Tile 0 is always "No Rune", so the way to empty a socket sits in the same place every time.
        const bool bClearTile = CellIndex == 0;
        const int32 KeyIndex = CellIndex - 1;
        CellState[CellIndex].bClear = bClearTile;
        CellState[CellIndex].Rune = !bClearTile && Keys.IsValidIndex(KeyIndex)
            ? Library[Keys[KeyIndex].LibraryIndex]
            : nullptr;
    }
}

void UMythicRunePickerWidget::RefreshCells() {
    const UMythicRuneComponent *Runes = FindRuneComponent();
    const UMythicCharacterPageWidget *Owner = Page.Get();

    for (int32 CellIndex = 0; CellIndex < Cells.Num(); ++CellIndex) {
        UMythicRunePickerCellWidget *Cell = Cells[CellIndex];
        if (!Cell || !CellState.IsValidIndex(CellIndex)) {
            continue;
        }
        FMythicRuneCellEntry &Entry = CellState[CellIndex];
        if (Entry.bClear) {
            const bool bSocketEmpty = Runes && SelectedSlot != INDEX_NONE && !Runes->GetRuneInSlot(SelectedSlot);
            Entry.bUnlocked = true;
            Entry.WornSlot = INDEX_NONE;
            // Reads as the socket's current choice while it stands empty, the same as a worn rune does.
            Entry.Worn = bSocketEmpty ? EMythicRuneWorn::Here : EMythicRuneWorn::None;

            FMythicRuneCellState ClearState;
            ClearState.Name = NSLOCTEXT("Mythic", "RunePickerNoRune", "No Rune");
            ClearState.Icon = nullptr;
            ClearState.Tint = FLinearColor::White;
            ClearState.bUnlocked = true;
            ClearState.bClear = true;
            ClearState.Worn = Entry.Worn;
            ClearState.WornSlot = INDEX_NONE;
            Cell->SetCellState(ClearState);
            Cell->SetRenderOpacity(1.0f);
            Cell->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            continue;
        }
        UMythicRuneDefinition *Rune = Entry.Rune;
        if (!Rune) {
            Entry = FMythicRuneCellEntry();
            Cell->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }
        Entry.bUnlocked = Runes && Runes->IsRuneUnlocked(Rune);
        Entry.WornSlot = FindWornSlot(Runes, Rune);
        Entry.Worn = Entry.WornSlot == INDEX_NONE ? EMythicRuneWorn::None
                     : Entry.WornSlot == SelectedSlot ? EMythicRuneWorn::Here
                                                      : EMythicRuneWorn::Elsewhere;

        FMythicRuneCellState State;
        State.Name = Rune->Name;
        State.Icon = FindPreloadedIcon(Rune);
        State.Tint = Owner ? Owner->RuneCategoryColour(Rune) : FLinearColor::White;
        State.bUnlocked = Entry.bUnlocked;
        State.Worn = Entry.Worn;
        State.WornSlot = Entry.WornSlot;
        Cell->SetCellState(State);
        // Faded, never disabled: a locked rune must still answer hover and pad focus with its deed.
        Cell->SetRenderOpacity(Entry.bUnlocked ? 1.0f : LockedCellOpacity);
        Cell->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
}

FText UMythicRunePickerWidget::SocketCountText(const UMythicRuneComponent *Runes) const {
    const int32 Open = Runes ? Runes->GetUnlockedSlots() : 0;
    int32 WornCount = 0;
    for (int32 SocketIndex = 0; SocketIndex < Open; ++SocketIndex) {
        WornCount += Runes->GetRuneInSlot(SocketIndex) ? 1 : 0;
    }
    return FText::Format(NSLOCTEXT("Mythic", "SocketCount", "{0} / {1} sockets"), FText::AsNumber(WornCount),
                         FText::AsNumber(Open));
}

void UMythicRunePickerWidget::RefreshHeaders() {
    const UMythicRuneComponent *Runes = FindRuneComponent();
    if (TitleHeader) {
        TitleHeader->SetHeader(NSLOCTEXT("Mythic", "RunePickerTitle", "Runes"), FText::GetEmpty(),
                               TitleEmblem.LoadSynchronous());
    }
    SetSocketHeaderTrailing(SocketCountText(Runes));
    if (GridHeader) {
        int32 Earned = 0;
        for (const UMythicRuneDefinition *Rune : Library) {
            Earned += (Runes && Runes->IsRuneUnlocked(Rune)) ? 1 : 0;
        }
        GridHeader->SetHeader(NSLOCTEXT("Mythic", "RunePickerGridHeading", "Available runes"),
                              FText::Format(NSLOCTEXT("Mythic", "RunePickerEarned", "{0} / {1} earned"),
                                            FText::AsNumber(Earned), FText::AsNumber(Library.Num())),
                              nullptr);
    }
}

void UMythicRunePickerWidget::SetSocketHeaderTrailing(const FText &Trailing) {
    if (SocketHeader) {
        SocketHeader->SetHeader(NSLOCTEXT("Mythic", "RunePickerSocketHeading", "Choose a rune socket"), Trailing,
                                nullptr);
    }
}

void UMythicRunePickerWidget::ShowSealedNotice(int32 SlotIndex) {
    FText Deed;
    RunePicker_SealedLine(SlotIndex, Deed);
    SetSocketHeaderTrailing(RunePicker_SealedNotice(Deed));
}

void UMythicRunePickerWidget::RefreshDetail() {
    // Hover wins over focus so the mouse reads what it is over; with nothing under either, the selected socket.
    const int32 Cell = HoveredCell != INDEX_NONE ? HoveredCell : FocusedCell;
    if (Cell != INDEX_NONE && CellState.IsValidIndex(Cell) && CellState[Cell].Rune) {
        ShowDetailForRune(CellState[Cell].Rune, CellState[Cell].bUnlocked);
        return;
    }
    const int32 Strip = HoveredStrip != INDEX_NONE ? HoveredStrip : FocusedStrip;
    ShowDetailForSocket(Strip != INDEX_NONE ? Strip : SelectedSlot);
}

void UMythicRunePickerWidget::ShowDetailForRune(const UMythicRuneDefinition *Rune, bool bUnlocked) {
    if (!Rune) {
        return;
    }
    const UMythicCharacterPageWidget *Owner = Page.Get();
    const FLinearColor Tint = Owner ? Owner->RuneCategoryColour(Rune) : FLinearColor::White;

    if (DetailSocket) {
        DetailSocket->SetState(bUnlocked ? EMythicRuneSocketState::Filled : EMythicRuneSocketState::Sealed,
                               FindPreloadedIcon(Rune), Tint);
    }
    if (DetailName) {
        DetailName->SetText(Rune->Name);
    }
    // The category is already read by the cell's colour, so spelling it out here says it twice. Rolled numbers come
    // from this owner's roll set, so two players wearing the same rune read their own values.
    if (DetailCategory) {
        DetailCategory->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (DetailDescription) {
        DetailDescription->SetText(UMythicRuneDescriber::DescribeBehaviour(Rune, FindRuneComponent()));
    }
    if (DetailHint) {
        DetailHint->SetText(Rune->Hint);
        DetailHint->SetVisibility(bUnlocked || Rune->Hint.IsEmpty() ? ESlateVisibility::Collapsed
                                                                    : ESlateVisibility::HitTestInvisible);
    }
}

void UMythicRunePickerWidget::ShowDetailForSocket(int32 SlotIndex) {
    const UMythicRuneComponent *Runes = FindRuneComponent();
    const bool bUnlocked = Runes && Runes->IsSlotUnlocked(SlotIndex);
    if (const UMythicRuneDefinition *Worn = bUnlocked ? Runes->GetRuneInSlot(SlotIndex) : nullptr) {
        ShowDetailForRune(Worn, true);
        return;
    }

    FText Deed;
    const FText Body = bUnlocked
        ? FText::Format(NSLOCTEXT("Mythic", "RunePickerChoose", "Choose a rune for socket {0}"),
                        FText::AsNumber(SlotIndex + 1))
        : RunePicker_SealedLine(SlotIndex, Deed);

    if (DetailSocket) {
        DetailSocket->SetState(bUnlocked ? EMythicRuneSocketState::Empty : EMythicRuneSocketState::Sealed, nullptr,
                               FLinearColor::White);
    }
    if (DetailName) {
        DetailName->SetText(RunePicker_SocketName(SlotIndex));
    }
    if (DetailCategory) {
        DetailCategory->SetText(bUnlocked ? NSLOCTEXT("Mythic", "RunePickerEmpty", "Empty")
                                          : NSLOCTEXT("Mythic", "RunePickerSealedWord", "Sealed"));
        DetailCategory->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().InkLabel));
        DetailCategory->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    if (DetailDescription) {
        DetailDescription->SetText(Body);
    }
    if (DetailHint) {
        DetailHint->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMythicRunePickerWidget::SetActionsBindingActive(bool bActive) {
    // OpenForSlot refreshes before it activates, and a binding registered off an inactive widget never reaches the
    // action bar, so hold it until activation puts the refresh back on the same footing.
    const bool bWanted = bActive && IsActivated();
    if (bWanted == ActionsBinding.Handle.IsValid()) {
        return;
    }
    bActive = bWanted;
    if (!bActive) {
        UnregisterInputBinding(ActionsBinding);
        ActionsBinding = FInputActionBindingHandle();
        return;
    }
    UInputAction *Action = ActionsAction.LoadSynchronous();
    if (!Action) {
        return;
    }
    FInputActionExecutedDelegate Callback;
    Callback.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UMythicRunePickerWidget, HandleActionsAction));
    RegisterInputActionBinding(Action, IE_Pressed, Callback, true, ActionsBinding);
}

void UMythicRunePickerWidget::RefreshPrompts() {
    const UMythicRuneComponent *Runes = FindRuneComponent();
    const bool bFilled = Runes && Runes->GetRuneInSlot(SelectedSlot) != nullptr;

    FText Select = NSLOCTEXT("Mythic", "RunePickerSelect", "Select");
    bool bPrimaryUnequips = false;
    const int32 Cell = FocusedCell != INDEX_NONE ? FocusedCell : HoveredCell;
    if (Cell != INDEX_NONE && CellState.IsValidIndex(Cell) && CellState[Cell].bClear) {
        Select = NSLOCTEXT("Mythic", "RunePickerClear", "Empty the socket");
        bPrimaryUnequips = true;
    }
    else if (Cell != INDEX_NONE && CellState.IsValidIndex(Cell) && CellState[Cell].Rune) {
        switch (CellState[Cell].Worn) {
        case EMythicRuneWorn::Here:
            Select = NSLOCTEXT("Mythic", "RunePickerUnequip", "Unequip");
            bPrimaryUnequips = true;
            break;
        case EMythicRuneWorn::Elsewhere:
            Select = NSLOCTEXT("Mythic", "RunePickerMove", "Move here");
            break;
        default:
            Select = NSLOCTEXT("Mythic", "RunePickerEquip", "Equip");
            break;
        }
    }
    const FText Unequip = NSLOCTEXT("Mythic", "RunePickerUnequip", "Unequip");

    if (Lbl_Select) {
        Lbl_Select->SetText(Select);
    }
    if (PrimaryBinding.Handle.IsValid()) {
        PrimaryBinding.Handle.SetDisplayName(Select);
    }
    // One verb never appears twice in the action bar: while the primary action already reads Unequip, the
    // dedicated binding is dropped, which is the only way its prompt leaves the bar.
    const bool bShowActions = bFilled && !bPrimaryUnequips;
    SetActionsBindingActive(bShowActions);
    if (Lbl_Actions) {
        Lbl_Actions->SetText(Unequip);
        Lbl_Actions->SetVisibility(bShowActions ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (Glyph_Actions) {
        Glyph_Actions->SetVisibility(bShowActions ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (ActionsBinding.Handle.IsValid()) {
        ActionsBinding.Handle.SetDisplayName(Unequip);
    }
    if (UnequipButton) {
        UnequipButton->SetIsEnabled(bFilled);
    }
}

void UMythicRunePickerWidget::HandleCellPressed(int32 CellIndex) {
    ActivateCell(CellIndex);
}

void UMythicRunePickerWidget::HandleCellHoverChanged(int32 CellIndex, bool bOn) {
    if (bOn) {
        HoveredCell = CellIndex;
    }
    else if (HoveredCell == CellIndex) {
        HoveredCell = INDEX_NONE;
    }
    RefreshDetail();
    RefreshPrompts();
}

void UMythicRunePickerWidget::HandleCellFocusChanged(int32 CellIndex, bool bOn) {
    if (bOn) {
        FocusedCell = CellIndex;
        FocusedStrip = INDEX_NONE;
    }
    else if (FocusedCell == CellIndex) {
        FocusedCell = INDEX_NONE;
    }
    RefreshDetail();
    RefreshPrompts();
}

void UMythicRunePickerWidget::HandleStripPressed(int32 SlotIndex) {
    UMythicRuneSocketWidget *Socket = GetStripSocket(SlotIndex);
    if (!Socket) {
        return;
    }
    if (Socket->GetState() == EMythicRuneSocketState::Sealed) {
        Socket->PlayRefuse();
        ShowSealedNotice(SlotIndex);
        return;
    }
    SelectSocket(SlotIndex);
}

void UMythicRunePickerWidget::NoticeStrip(int32 SlotIndex, bool bOn) {
    // Tooltips are mouse-only, so the header trailing is how a pad reads the deed behind a sealed socket.
    UMythicRuneSocketWidget *Socket = GetStripSocket(SlotIndex);
    if (bOn && Socket && Socket->GetState() == EMythicRuneSocketState::Sealed) {
        ShowSealedNotice(SlotIndex);
    }
    else {
        SetSocketHeaderTrailing(SocketCountText(FindRuneComponent()));
    }
    RefreshDetail();
}

void UMythicRunePickerWidget::HandleStripHoverChanged(int32 SlotIndex, bool bOn) {
    if (bOn) {
        HoveredStrip = SlotIndex;
    }
    else if (HoveredStrip == SlotIndex) {
        HoveredStrip = INDEX_NONE;
    }
    NoticeStrip(SlotIndex, bOn);
}

void UMythicRunePickerWidget::HandleStripFocusChanged(int32 SlotIndex, bool bOn) {
    if (bOn) {
        FocusedStrip = SlotIndex;
        FocusedCell = INDEX_NONE;
    }
    else if (FocusedStrip == SlotIndex) {
        FocusedStrip = INDEX_NONE;
    }
    NoticeStrip(SlotIndex, bOn);
    RefreshPrompts();
}

void UMythicRunePickerWidget::RefuseLocally(int32 CellIndex, EMythicRuneRefusal Reason, int32 OtherSlot) {
    if (UMythicRunePickerCellWidget *Cell = GetCellWidget(CellIndex)) {
        Cell->PlayRefuse();
    }
    SetSocketHeaderTrailing(UMythicRuneComponent::DescribeRefusal(Reason, OtherSlot));
}

void UMythicRunePickerWidget::ActivateCell(int32 CellIndex) {
    if (!CellState.IsValidIndex(CellIndex)) {
        return;
    }
    if (CellState[CellIndex].bClear) {
        UnequipCurrent();
        return;
    }
    if (!CellState[CellIndex].Rune) {
        return;
    }
    const FMythicRuneCellEntry &Entry = CellState[CellIndex];
    UMythicRuneComponent *Runes = FindRuneComponent();
    if (!Runes) {
        UE_LOG(Myth, Warning, TEXT("Runes: picker has no rune component to equip into."));
        return;
    }
    if (!Entry.bUnlocked) {
        RefuseLocally(CellIndex, EMythicRuneRefusal::DeedMissing, INDEX_NONE);
        ShowDetailForRune(Entry.Rune, false);
        return;
    }

    // The same gate the server runs, asked first so a refused click shakes at once instead of waiting a round
    // trip; the server still re-runs it and owns the answer.
    switch (Entry.Worn) {
    case EMythicRuneWorn::Here:
        Runes->ServerUnequipRune(SelectedSlot);
        return;
    case EMythicRuneWorn::Elsewhere: {
        int32 Other = INDEX_NONE;
        const EMythicRuneRefusal Reason = Runes->CanEquipRune(SelectedSlot, Entry.Rune, Other);
        const bool bOnlyWornWhereExpected = Reason == EMythicRuneRefusal::WornElsewhere && Other == Entry.WornSlot;
        if (Reason != EMythicRuneRefusal::None && !bOnlyWornWhereExpected) {
            RefuseLocally(CellIndex, Reason, Other);
            return;
        }
        Runes->ServerMoveRune(Entry.WornSlot, SelectedSlot);
        return;
    }
    default: {
        int32 Other = INDEX_NONE;
        const EMythicRuneRefusal Reason = Runes->CanEquipRune(SelectedSlot, Entry.Rune, Other);
        if (Reason != EMythicRuneRefusal::None) {
            RefuseLocally(CellIndex, Reason, Other);
            return;
        }
        Runes->ServerEquipRune(SelectedSlot, Entry.Rune);
        return;
    }
    }
}

void UMythicRunePickerWidget::UnequipCurrent() {
    UMythicRuneComponent *Runes = FindRuneComponent();
    if (Runes && Runes->GetRuneInSlot(SelectedSlot)) {
        Runes->ServerUnequipRune(SelectedSlot);
        return;
    }
    if (UMythicRuneSocketWidget *Socket = GetStripSocket(SelectedSlot)) {
        Socket->PlayRefuse();
    }
}

UTexture2D *UMythicRunePickerWidget::FindPreloadedIcon(const UMythicRuneDefinition *Rune) const {
    if (!Rune) {
        return nullptr;
    }
    const int32 Index = Library.IndexOfByKey(Rune);
    return PreloadedIcons.IsValidIndex(Index) ? PreloadedIcons[Index].Get() : nullptr;
}

UMythicRunePickerCellWidget *UMythicRunePickerWidget::GetCellWidget(int32 CellIndex) const {
    return Cells.IsValidIndex(CellIndex) ? Cells[CellIndex].Get() : nullptr;
}

UMythicRuneSocketWidget *UMythicRunePickerWidget::GetStripSocket(int32 SlotIndex) const {
    return StripSockets.IsValidIndex(SlotIndex) ? StripSockets[SlotIndex].Get() : nullptr;
}

const UMythicRuneDefinition *UMythicRunePickerWidget::GetCellRune(int32 CellIndex) const {
    return CellState.IsValidIndex(CellIndex) ? CellState[CellIndex].Rune.Get() : nullptr;
}

bool UMythicRunePickerWidget::IsCellUnlocked(int32 CellIndex) const {
    return CellState.IsValidIndex(CellIndex) && CellState[CellIndex].bUnlocked;
}

EMythicRuneWorn UMythicRunePickerWidget::GetCellWorn(int32 CellIndex) const {
    return CellState.IsValidIndex(CellIndex) ? CellState[CellIndex].Worn : EMythicRuneWorn::None;
}
