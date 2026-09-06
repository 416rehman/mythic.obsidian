#include "MythicAbilityPickerWidget.h"
#include "MythicCharacterPageWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/PanelWidget.h"
#include "Components/RichTextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "InputAction.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Skills/MythicAbilityCatalog.h"
#include "System/MythicAssetManager.h"
#include "TimerManager.h"
#include "UI/MythicUIManagerSubsystem.h"
#include "UI/Widgets/MythicBoundActionButton.h"
#include "UI/Widgets/MythicInputGlyph.h"
#include "UI/Widgets/MythicSectionHeader.h"

namespace {
void AbilityButtonLabel(UCommonButtonBase* Button,const FText& Text) {
    if (auto* Bound=Cast<UMythicBoundActionButton>(Button)) { Bound->SetLabelOverride(Text); Bound->SetActionBarPromptOnly(false); }
}
bool AbilityHasFocus(const UWidget* Widget) {
    auto* Player=Widget ? Widget->GetOwningPlayer() : nullptr;
    return Player && (Widget->HasUserFocus(Player) || Widget->HasUserFocusedDescendants(Player));
}
}
void UMythicAbilityListEntry::NativeOnInitialized() {
    Super::NativeOnInitialized();
    OnPressed.AddUniqueDynamic(this,&ThisClass::Pressed);
    OnFocusChanged.AddUniqueDynamic(this,&ThisClass::Noticed);
    OnHoverChanged.AddUniqueDynamic(this,&ThisClass::Noticed);
}
void UMythicAbilityListEntry::NativeOnListItemObjectSet(UObject* Object) {
    Item=Cast<UMythicAbilityListItem>(Object); Refresh();
}
void UMythicAbilityListEntry::NativeOnEntryReleased() { Item.Reset(); }
void UMythicAbilityListEntry::Refresh() {
    if (Item.IsValid() && Item->Picker.IsValid()) { SetCellIndex(Item->Index); SetCellState(Item->Picker->DescribeCell(Item->Index)); }
}
void UMythicAbilityListEntry::Pressed(int32 Index) { if (Item.IsValid() && Item->Picker.IsValid()) Item->Picker->SelectAbility(Index); }
void UMythicAbilityListEntry::Noticed(int32 Index,bool bOn) { if (bOn) Pressed(Index); }

UMythicSkillComponent* UMythicAbilityPickerWidget::FindSkills() const {
    const auto* PS=GetOwningPlayer() ? GetOwningPlayer()->GetPlayerState<AMythicPlayerState>() : nullptr;
    return PS ? PS->GetSkillComponent() : nullptr;
}
UMythicSkillDefinition* UMythicAbilityPickerWidget::SelectedDefinition() const { return Library.IsValidIndex(SelectedIndex) ? Library[SelectedIndex]->Definition.Get() : nullptr; }
void UMythicAbilityPickerWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();
    if (AbilityList) {
        AbilityList->OnItemSelectionChanged().AddWeakLambda(this,[this](UObject* Object) {
            if (const auto* Item=Cast<UMythicAbilityListItem>(Object)) SelectAbility(Item->Index);
        });
        AbilityList->OnEntryWidgetGenerated().AddUObject(this,&ThisClass::ConfigureEntryNavigation);
        AbilityList->OnItemScrolledIntoView().AddWeakLambda(this,[this](UObject* Item,UUserWidget& Entry) {
            ConfigureEntryNavigation(Entry);
            if (IsActivated() && !bClosing && PendingFocusItem.Get()==Item && AbilityHasFocus(AbilityList)) {
                PendingFocusItem.Reset();
                if (auto* Cell=Cast<UMythicAbilityListEntry>(&Entry)) if (auto* Focus=Cell->GetFocusWidget()) Focus->SetUserFocus(GetOwningPlayer());
            }
        });
    }
    if (SocketClass && SocketStrip) for (int32 i=0;i<UMythicSkillComponent::SlotCount;++i) {
        auto* Socket=CreateWidget<UMythicRuneSocketWidget>(this,SocketClass);
        Socket->SetSlotIndex(i); Socket->SetInteractive(true);
        Socket->OnPressed.AddUniqueDynamic(this,&ThisClass::SelectSocket);
        SocketStrip->AddChild(Socket); StripSockets.Add(Socket);
    }
    if (DetailSocket) DetailSocket->SetInteractive(false);
    if (Dismiss) Dismiss->OnClicked.AddUniqueDynamic(this,&ThisClass::Close);
    if (CloseButton) CloseButton->OnClicked().AddWeakLambda(this,[this](){ Close(); });
    if (FooterCloseButton) FooterCloseButton->OnClicked().AddWeakLambda(this,[this](){ Close(); });
    if (EquipButton) EquipButton->OnClicked().AddWeakLambda(this,[this](){ EquipCurrent(); });
    if (UnequipButton) UnequipButton->OnClicked().AddWeakLambda(this,[this](){ UnequipCurrent(); });
    AbilityButtonLabel(FooterCloseButton,NSLOCTEXT("Mythic","AbilityBack","Back"));
    AbilityButtonLabel(CloseButton,NSLOCTEXT("Mythic","AbilityClose","Close"));
    AbilityButtonLabel(EquipButton,NSLOCTEXT("Mythic","AbilityEquip","Equip"));
    AbilityButtonLabel(UnequipButton,NSLOCTEXT("Mythic","AbilityUnequip","Unequip"));
    auto* Primary=PrimaryAction.LoadSynchronous(); auto* Actions=ActionsAction.LoadSynchronous();
    if (Glyph_Select) Glyph_Select->SetEnhancedAction(Primary);
    if (Glyph_Actions) Glyph_Actions->SetEnhancedAction(Actions);
    if (auto* B=Cast<UMythicBoundActionButton>(EquipButton)) B->SetGlyphEnhancedAction(Primary);
    if (auto* B=Cast<UMythicBoundActionButton>(UnequipButton)) B->SetGlyphEnhancedAction(Actions);
    if (Lbl_Select) Lbl_Select->SetText(NSLOCTEXT("Mythic","AbilityInspect","Inspect"));
    if (Lbl_Actions) Lbl_Actions->SetText(NSLOCTEXT("Mythic","AbilityUnequip","Unequip"));
    if (Lbl_Back) Lbl_Back->SetText(NSLOCTEXT("Mythic","AbilityBack","Back"));
    if (OpenAnim) { FWidgetAnimationDynamicEvent Event; Event.BindDynamic(this,&ThisClass::FinishOpenAnimation); BindToAnimationFinished(OpenAnim,Event); }
    if (CloseAnim) { FWidgetAnimationDynamicEvent Event; Event.BindDynamic(this,&ThisClass::FinishClose); BindToAnimationFinished(CloseAnim,Event); }
    if (auto* GI=GetGameInstance()) if (auto* Catalog=GI->GetSubsystem<UMythicAbilityCatalog>()) {
        if (Catalog->IsReady()) BuildLibrary();
        else Catalog->OnReady.AddWeakLambda(this,[this](){ BuildLibrary(); });
    }
}
void UMythicAbilityPickerWidget::BuildLibrary() {
    auto* Catalog=GetGameInstance() ? GetGameInstance()->GetSubsystem<UMythicAbilityCatalog>() : nullptr;
    if (!Catalog || !Catalog->IsReady() || !Library.IsEmpty()) return;
    int32 MaximumModifiers=0;
    for (const auto& Definition:Catalog->GetDefinitions()) {
        auto* Item=NewObject<UMythicAbilityListItem>(this); Item->Definition=Definition; Item->Picker=this; Item->Index=Library.Num(); Library.Add(Item);
        MaximumModifiers=FMath::Max(MaximumModifiers,Definition->Modifiers.Num());
    }
    if (AbilityList) AbilityList->SetListItems(Library);
    // Detail rows are pooled once when metadata becomes ready; selection only retexts them.
    if (ModifierRowClass && ModifierHost) for (int32 i=0;i<FMath::Min(MaximumModifiers,50);++i) {
        auto* Row=CreateWidget<UUserWidget>(this,ModifierRowClass); ModifierHost->AddChild(Row); ModifierRows.Add(Row);
        auto* Button=Cast<UCommonButtonBase>(Row->GetWidgetFromName(TEXT("RowButton"))); ModifierButtons.Add(Button);
        if (Button) Button->OnClicked().AddWeakLambda(this,[this,i](){ ToggleModifier(i); });
    }
    if (SelectedIndex==INDEX_NONE && !Library.IsEmpty()) SelectedIndex=0;
    Refresh();
}
UTexture2D* UMythicAbilityPickerWidget::FindPreloadedIcon(const UMythicSkillDefinition* Definition) {
    if (!Definition || Definition->Icon.IsNull()) return nullptr;
    const auto Path=Definition->Icon.ToSoftObjectPath();
    if (const auto* Found=IconCache.Find(Path)) return Found->Get();
    if (!PendingIcons.Contains(Path)) {
        PendingIcons.Add(Path);
        UMythicAssetManager::LoadAsync(this,Definition->Icon,[this,Path](UTexture2D* Texture) {
            PendingIcons.Remove(Path);
            IconCache.Add(Path,Texture);
            if (Texture) {
                IconCache.Add(Path,Texture); IconOrder.AddUnique(Path);
                while (IconOrder.Num()>64) { IconCache.Remove(IconOrder[0]); IconOrder.RemoveAt(0); }
            }
            Refresh();
            if (Page.IsValid()) Page->RefreshAbilitySockets();
        });
    }
    return Definition->Icon.Get();
}
FMythicRuneCellState UMythicAbilityPickerWidget::DescribeCell(int32 Index) {
    FMythicRuneCellState State;
    if (!Library.IsValidIndex(Index)) return State;
    auto* Definition=Library[Index]->Definition.Get(); auto* Skills=FindSkills();
    State.Name=Definition->Name; State.Icon=FindPreloadedIcon(Definition); State.bUnlocked=Skills && Skills->IsSkillUnlocked(Definition);
    if (Skills) for (int32 SocketIndex=0;SocketIndex<UMythicSkillComponent::SlotCount;++SocketIndex) if (Skills->GetSkillInSlot(SocketIndex)==Definition) { State.Worn=SocketIndex==SelectedSlot ? EMythicRuneWorn::Here:EMythicRuneWorn::Elsewhere; State.WornSlot=SocketIndex; }
    return State;
}
void UMythicAbilityPickerWidget::BindSkills() {
    auto* Skills=FindSkills(); if (BoundSkills.Get()==Skills) return;
    UnbindSkills(); BoundSkills=Skills;
    if (Skills) { Skills->OnSkillsChanged.AddUniqueDynamic(this,&ThisClass::Refresh); Skills->OnActionResult.AddUniqueDynamic(this,&ThisClass::HandleResult); }
}
void UMythicAbilityPickerWidget::UnbindSkills() {
    if (BoundSkills.IsValid()) { BoundSkills->OnSkillsChanged.RemoveDynamic(this,&ThisClass::Refresh); BoundSkills->OnActionResult.RemoveDynamic(this,&ThisClass::HandleResult); }
    BoundSkills.Reset();
}
void UMythicAbilityPickerWidget::SetCharacterPage(UMythicCharacterPageWidget* InPage) {
    if (InPage && InPage->GetOwningPlayer()==GetOwningPlayer()) Page=InPage;
}
void UMythicAbilityPickerWidget::OpenForSlot(int32 Index,UMythicCharacterPageWidget* InPage) {
    if (Index < 0 || Index >= UMythicSkillComponent::SlotCount) return;
    Page=InPage; ReturnSlot=Index; SelectedSlot=ReturnSlot; Notice=FText::GetEmpty(); bClosing=false; BindSkills();
    if (auto* Skills=FindSkills()) {
        auto* Equipped=Skills->GetSkillInSlot(SelectedSlot);
        for (int32 i=0;i<Library.Num();++i) if (Library[i]->Definition==Equipped) { SelectedIndex=i; break; }
    }
    if (!bOnLayer && PickerLayerTag.IsValid()) if (auto* GI=GetGameInstance()) if (auto* Manager=GI->GetSubsystem<UMythicUIManagerSubsystem>()) {
        Manager->AddWidgetInstanceToLayer(PickerLayerTag,GetOwningPlayer(),this); bOnLayer=true;
    }
    if (!IsActivated()) ActivateWidget(); Refresh();
    if (AbilityList && Library.IsValidIndex(SelectedIndex)) AbilityList->ScrollIndexIntoView(SelectedIndex);
}
void UMythicAbilityPickerWidget::NativeOnActivated() {
    Super::NativeOnActivated(); BindSkills(); bClosing=false;
    AddUIInputContext();
    if (auto* Action=PrimaryAction.Get()) { FInputActionExecutedDelegate Callback; Callback.BindUFunction(this,GET_FUNCTION_NAME_CHECKED(ThisClass,HandlePrimary)); RegisterInputActionBinding(Action,IE_Pressed,Callback,true,PrimaryBinding); }
    if (auto* Action=ActionsAction.Get()) { FInputActionExecutedDelegate Callback; Callback.BindUFunction(this,GET_FUNCTION_NAME_CHECKED(ThisClass,UnequipCurrent)); RegisterInputActionBinding(Action,IE_Pressed,Callback,true,ActionsBinding); }
    Refresh();
    if (OpenAnim) { if (Card) Card->ForceVolatile(true); PlayAnimation(OpenAnim); }
}
void UMythicAbilityPickerWidget::NativeOnDeactivated() {
    PendingFocusItem.Reset(); RemoveUIInputContext();
    bClosing=false; if (!PendingRequest.IsValid()) UnbindSkills(); UnregisterAllBindings(); PrimaryBinding={}; ActionsBinding={};
    if (Card) Card->ForceVolatile(false); Super::NativeOnDeactivated();
}
void UMythicAbilityPickerWidget::NativeDestruct() {
    bOnLayer=false;
    UnbindSkills();
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PendingTimer);
    PendingRequest.Invalidate(); PendingRevision=INDEX_NONE;
    Super::NativeDestruct();
}
bool UMythicAbilityPickerWidget::NativeOnHandleBackAction() { Close(); return true; }
void UMythicAbilityPickerWidget::FinishOpenAnimation() { if (Card) Card->ForceVolatile(false); }
void UMythicAbilityPickerWidget::Close() {
    if (bClosing) return; bClosing=true;
    if (CloseAnim && IsActivated() && GetCachedWidget().IsValid()) { if (Card) Card->ForceVolatile(true); PlayAnimation(CloseAnim); }
    else FinishClose();
}
void UMythicAbilityPickerWidget::FinishClose() {
    const bool bWasShown=bOnLayer || IsActivated(); FinishOpenAnimation();
    if (bOnLayer) if (auto* GI=GetGameInstance()) if (auto* Manager=GI->GetSubsystem<UMythicUIManagerSubsystem>()) Manager->RemoveWidgetInstanceFromLayer(PickerLayerTag,GetOwningPlayer(),this);
    bOnLayer=false; bClosing=false; if (IsActivated()) DeactivateWidget();
    if (bWasShown && GetWorld()) { auto WeakPage=Page; const int32 SocketIndex=ReturnSlot; GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this,WeakPage,SocketIndex](){ if (!IsActivated() && !bOnLayer && WeakPage.IsValid()) WeakPage->FocusAbilitySocket(SocketIndex); })); }
}
UWidget* UMythicAbilityPickerWidget::NativeGetDesiredFocusTarget() const {
    if (AbilityList && Library.IsValidIndex(SelectedIndex)) if (auto* Entry=AbilityList->GetEntryWidgetFromItem<UMythicAbilityListEntry>(Library[SelectedIndex])) return Entry->GetFocusWidget();
    return StripSockets.IsValidIndex(SelectedSlot) ? StripSockets[SelectedSlot]->GetFocusWidget() : Super::NativeGetDesiredFocusTarget();
}
void UMythicAbilityPickerWidget::SelectSocket(int32 Index) { if (Index<0 || Index>=UMythicSkillComponent::SlotCount || PendingRequest.IsValid()) return; SelectedSlot=Index; Notice=FText::GetEmpty(); Refresh(); }
void UMythicAbilityPickerWidget::SelectAbility(int32 Index) {
    if (!Library.IsValidIndex(Index)) return;
    const bool Changed=SelectedIndex!=Index;
    SelectedIndex=Index;
    if (AbilityList && AbilityList->GetSelectedItem()!=Library[Index]) AbilityList->SetSelectedItem(Library[Index]);
    if (Changed) { Notice=FText::GetEmpty(); RefreshDetail(); }
}
void UMythicAbilityPickerWidget::SetNotice(const FText& Text) { Notice=Text; RefreshDetail(); }
void UMythicAbilityPickerWidget::Refresh() {
    SelectAbility(SelectedIndex);
    auto* Skills=FindSkills();
    if (PendingRequest.IsValid() && PendingRevision!=INDEX_NONE && Skills && Skills->GetRevision()>=PendingRevision) { PendingRequest.Invalidate(); PendingRevision=INDEX_NONE; Notice=FText::GetEmpty(); if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PendingTimer); }
    for (int32 i=0;i<StripSockets.Num();++i) {
        auto* Definition=Skills ? Skills->GetSkillInSlot(i):nullptr; const bool Unlocked=Skills && Skills->IsSlotUnlocked(i);
        StripSockets[i]->SetState(!Unlocked ? EMythicRuneSocketState::Sealed:Definition ? EMythicRuneSocketState::Filled:EMythicRuneSocketState::Empty,FindPreloadedIcon(Definition),FLinearColor::White);
        StripSockets[i]->SetSelected(i==SelectedSlot);
    }
    if (AbilityList) for (auto* Entry:AbilityList->GetDisplayedEntryWidgets()) if (auto* AbilityEntry=Cast<UMythicAbilityListEntry>(Entry)) AbilityEntry->Refresh();
    if (TitleHeader) TitleHeader->SetHeader(NSLOCTEXT("Mythic","AbilityLibraryTitle","Abilities"),FText::GetEmpty());
    if (SocketHeader) SocketHeader->SetHeader(NSLOCTEXT("Mythic","AbilityDestination","Equip to socket"),FText::AsNumber(SelectedSlot+1));
    if (GridHeader) GridHeader->SetHeader(NSLOCTEXT("Mythic","AbilityLibrary","Library"),FText::AsNumber(Library.Num()));
    RefreshDetail();
    if (!IsActivated() && !bOnLayer && !PendingRequest.IsValid()) UnbindSkills();
}
void UMythicAbilityPickerWidget::RefreshDetail() {
    auto* Definition=SelectedDefinition(); auto* Skills=FindSkills(); const bool Learned=Definition && Skills && Skills->IsSkillUnlocked(Definition);
    if (DetailName) DetailName->SetText(Definition ? Definition->Name:FText::GetEmpty());
    if (DetailDescription) DetailDescription->SetText(Definition ? Definition->Description:FText::GetEmpty());
    if (DetailSocket) DetailSocket->SetState(Learned ? EMythicRuneSocketState::Filled:EMythicRuneSocketState::Sealed,FindPreloadedIcon(Definition),FLinearColor::White);
    if (DetailCategory) DetailCategory->SetText(Learned ? NSLOCTEXT("Mythic","AbilityLearned","Learned"):NSLOCTEXT("Mythic","AbilityLocked","Not yet learned"));
    EMythicAbilityResult EquipResult=Skills ? Skills->Preflight(EMythicAbilityAction::Equip,SelectedSlot,Definition,NAME_None,false):EMythicAbilityResult::NotReady;
    FText Hint=Notice;
    if (PendingRequest.IsValid()) Hint=NSLOCTEXT("Mythic","AbilityPending","Waiting for your loadout to update...");
    else if (Hint.IsEmpty()) Hint=!Learned && Definition ? Definition->Hint : UMythicSkillComponent::DescribeResult(EquipResult);
    if (DetailHint) DetailHint->SetText(Hint);
    const bool EquippedHere=Skills && Definition && Skills->GetSkillInSlot(SelectedSlot)==Definition;
    const bool CanEquip=!PendingRequest.IsValid() && EquipResult==EMythicAbilityResult::Success && !EquippedHere;
    if (EquipButton) { if (!CanEquip) FocusBeforeDisabling(EquipButton); EquipButton->SetIsEnabled(CanEquip); }
    AbilityButtonLabel(EquipButton,EquippedHere ? NSLOCTEXT("Mythic","AbilityEquipped","Equipped"):NSLOCTEXT("Mythic","AbilityEquip","Equip"));
    const bool CanUnequip=!PendingRequest.IsValid() && Skills && Skills->GetSkillInSlot(SelectedSlot) && Skills->Preflight(EMythicAbilityAction::Unequip,SelectedSlot,nullptr,NAME_None,false)==EMythicAbilityResult::Success;
    if (UnequipButton) { if (!CanUnequip) FocusBeforeDisabling(UnequipButton); UnequipButton->SetIsEnabled(CanUnequip); }
    if (DetailProgress) {
        FText ProgressText;
        if (Learned && (Definition->bHasMasteryTrack || Definition->bUseLegacyModifierProgression)) {
            const int32 Next=Skills->GetUsesForNextLevel(Definition);
            ProgressText=Next>0 ? FText::Format(NSLOCTEXT("Mythic","AbilityIndependentPractice","Rank {0} · {1} / {2} practice"),Skills->GetSkillLevel(Definition),Skills->GetSkillUses(Definition),Next)
                : FText::Format(NSLOCTEXT("Mythic","AbilityMastered","Mastered · {0} casts"),Skills->GetSkillUses(Definition));
        }
        DetailProgress->SetText(ProgressText);
    }
    for (int32 i=0;i<ModifierRows.Num();++i) {
        auto* Row=ModifierRows[i].Get(); const bool Exists=Definition && Definition->Modifiers.IsValidIndex(i);
        if (!Exists || PendingRequest.IsValid()) FocusBeforeDisabling(Row);
        Row->SetVisibility(Exists ? ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed); if (!Exists) continue;
        const auto& Modifier=Definition->Modifiers[i]; const bool Active=Skills && Skills->IsModifierActive(Definition,i);
        const auto Result=Skills ? Skills->Preflight(EMythicAbilityAction::Modifier,INDEX_NONE,Definition,Modifier.ModifierId,!Active):EMythicAbilityResult::NotReady;
        if (auto* Text=Cast<UCommonTextBlock>(Row->GetWidgetFromName(TEXT("NameText")))) Text->SetText(Modifier.Name);
        if (auto* Text=Cast<URichTextBlock>(Row->GetWidgetFromName(TEXT("DescriptionText")))) Text->SetText(Modifier.Description);
        if (auto* Text=Cast<UCommonTextBlock>(Row->GetWidgetFromName(TEXT("HintText")))) Text->SetText(Result!=EMythicAbilityResult::Success ? UMythicSkillComponent::DescribeResult(Result):Active ? NSLOCTEXT("Mythic","AbilityModifierRemove","Active · Remove"):Definition->bUseLegacyModifierProgression ? FText::Format(NSLOCTEXT("Mythic","AbilityModifierChoose","Choose · {0} points"),Modifier.PointCost):NSLOCTEXT("Mythic","AbilityRefinementChoose","Choose"));
        // Keep refusal rows reachable for gamepad inspection; submission re-runs the same preflight.
        if (ModifierButtons[i]) ModifierButtons[i]->SetIsEnabled(!PendingRequest.IsValid());
    }
    RefreshNavigation();
}

void UMythicAbilityPickerWidget::FocusBeforeDisabling(UWidget* Widget) {
    if (IsActivated() && !bClosing && AbilityHasFocus(Widget)) {
        if (auto* Target=NavigateSocket(EUINavigation::Up)) Target->SetUserFocus(GetOwningPlayer());
    }
}

UWidget* UMythicAbilityPickerWidget::NavigateSocket(EUINavigation Direction) {
    return StripSockets.IsValidIndex(SelectedSlot) ? StripSockets[SelectedSlot]->GetFocusWidget() : FooterCloseButton.Get();
}

UWidget* UMythicAbilityPickerWidget::NavigateLibrary(EUINavigation Direction) {
    if (!AbilityList || !Library.IsValidIndex(SelectedIndex)) return NavigateSocket(Direction);
    if (auto* Entry=AbilityList->GetEntryWidgetFromItem<UMythicAbilityListEntry>(Library[SelectedIndex])) return Entry->GetFocusWidget();
    // The selected row can be outside the virtualized pool. Keep focus in this region
    // until the requested row materializes; a subsequent move out cancels the handoff.
    PendingFocusItem=Library[SelectedIndex];
    AbilityList->ScrollIndexIntoView(SelectedIndex);
    return AbilityList;
}

UWidget* UMythicAbilityPickerWidget::NavigateDetail(EUINavigation Direction) {
    if (EquipButton && EquipButton->GetIsEnabled()) return EquipButton;
    if (UnequipButton && UnequipButton->GetIsEnabled()) return UnequipButton;
    auto* Definition=SelectedDefinition();
    if (Definition && !PendingRequest.IsValid()) for (int32 i=0;i<FMath::Min(Definition->Modifiers.Num(),ModifierButtons.Num());++i) if (ModifierButtons[i]) return ModifierButtons[i];
    return FooterCloseButton;
}

void UMythicAbilityPickerWidget::ConfigureEntryNavigation(UUserWidget& Entry) {
    auto* Cell=Cast<UMythicAbilityListEntry>(&Entry);
    if (!Cell || !Cell->GetFocusWidget()) return;
    FCustomWidgetNavigationDelegate Detail; Detail.BindDynamic(this,&ThisClass::NavigateDetail);
    FCustomWidgetNavigationDelegate Socket; Socket.BindDynamic(this,&ThisClass::NavigateSocket);
    Cell->GetFocusWidget()->SetNavigationRuleCustom(EUINavigation::Right,Detail);
    Cell->GetFocusWidget()->SetNavigationRuleCustom(EUINavigation::Left,Socket);
    // SListView's directional selection can focus the entry root instead of its nested CommonButton.
    Cell->SetNavigationRuleCustom(EUINavigation::Right,Detail);
    Cell->SetNavigationRuleCustom(EUINavigation::Left,Socket);
}

void UMythicAbilityPickerWidget::RefreshNavigation() {
    FCustomWidgetNavigationDelegate LibraryNav; LibraryNav.BindDynamic(this,&ThisClass::NavigateLibrary);
    FCustomWidgetNavigationDelegate DetailNav; DetailNav.BindDynamic(this,&ThisClass::NavigateDetail);
    for (int32 i=0;i<StripSockets.Num();++i) if (auto* Focus=StripSockets[i]->GetFocusWidget()) {
        Focus->SetNavigationRuleCustom(EUINavigation::Down,LibraryNav);
        if (i>0) Focus->SetNavigationRuleExplicit(EUINavigation::Left,StripSockets[i-1]->GetFocusWidget());
        if (i+1<StripSockets.Num()) Focus->SetNavigationRuleExplicit(EUINavigation::Right,StripSockets[i+1]->GetFocusWidget());
        else Focus->SetNavigationRuleCustom(EUINavigation::Right,DetailNav);
    }
    TArray<UWidget*,TInlineAllocator<16>> DetailControls;
    if (EquipButton && EquipButton->GetIsEnabled()) DetailControls.Add(EquipButton);
    if (UnequipButton && UnequipButton->GetIsEnabled()) DetailControls.Add(UnequipButton);
    auto* Definition=SelectedDefinition();
    if (Definition && !PendingRequest.IsValid()) for (int32 i=0;i<FMath::Min(Definition->Modifiers.Num(),ModifierButtons.Num());++i) if (ModifierButtons[i]) DetailControls.Add(ModifierButtons[i]);
    if (FooterCloseButton) DetailControls.Add(FooterCloseButton);
    if (CloseButton) DetailControls.Add(CloseButton);
    for (int32 i=0;i<DetailControls.Num();++i) {
        auto* Control=DetailControls[i];
        Control->SetNavigationRuleCustom(EUINavigation::Left,LibraryNav);
        Control->SetNavigationRuleExplicit(EUINavigation::Up,DetailControls[(i+DetailControls.Num()-1)%DetailControls.Num()]);
        Control->SetNavigationRuleExplicit(EUINavigation::Down,DetailControls[(i+1)%DetailControls.Num()]);
        Control->SetNavigationRuleBase(EUINavigation::Right,EUINavigationRule::Stop);
    }
}
void UMythicAbilityPickerWidget::HandlePrimary() {
    if (!IsActivated() || bClosing) return;
    for (int32 i=0;i<ModifierButtons.Num();++i) if (AbilityHasFocus(ModifierButtons[i])) { ToggleModifier(i); return; }
    if (AbilityHasFocus(EquipButton)) { EquipCurrent(); return; }
    if (AbilityHasFocus(UnequipButton)) { UnequipCurrent(); return; }
    if (AbilityHasFocus(CloseButton) || AbilityHasFocus(FooterCloseButton)) { Close(); return; }
    if (AbilityList) for (auto* Entry:AbilityList->GetDisplayedEntryWidgets()) if (auto* Cell=Cast<UMythicAbilityListEntry>(Entry); Cell && AbilityHasFocus(Cell)) { SelectAbility(Cell->GetCellIndex()); return; }
    for (int32 i=0;i<StripSockets.Num();++i) if (AbilityHasFocus(StripSockets[i])) { SelectSocket(i); return; }
}
void UMythicAbilityPickerWidget::EquipCurrent() { Submit(EMythicAbilityAction::Equip); }
void UMythicAbilityPickerWidget::UnequipCurrent() { Submit(EMythicAbilityAction::Unequip); }
void UMythicAbilityPickerWidget::ToggleModifier(int32 Index) {
    auto* Definition=SelectedDefinition(); auto* Skills=FindSkills();
    if (Definition && Skills && Definition->Modifiers.IsValidIndex(Index)) Submit(EMythicAbilityAction::Modifier,Definition->Modifiers[Index].ModifierId,!Skills->IsModifierActive(Definition,Index));
}
void UMythicAbilityPickerWidget::Submit(EMythicAbilityAction Action,FName Id,bool bActive) {
    if (PendingRequest.IsValid() || bClosing) return;
    auto* Skills=FindSkills(); auto* Definition=Action==EMythicAbilityAction::Unequip ? nullptr:SelectedDefinition();
    const auto Result=Skills ? Skills->Preflight(Action,SelectedSlot,Definition,Id,bActive):EMythicAbilityResult::NotReady;
    if (Result!=EMythicAbilityResult::Success) { SetNotice(UMythicSkillComponent::DescribeResult(Result)); return; }
    BindSkills(); PendingRequest=FGuid::NewGuid(); PendingRevision=INDEX_NONE; RefreshDetail();
    if (GetWorld()) GetWorld()->GetTimerManager().SetTimer(PendingTimer,FTimerDelegate::CreateWeakLambda(this,[this](){
        PendingRequest.Invalidate(); PendingRevision=INDEX_NONE; SetNotice(NSLOCTEXT("Mythic","AbilityRequestTimeout","The loadout update has not arrived. Check your connection and try again."));
        if (!IsActivated() && !bOnLayer) UnbindSkills();
    }),10.0f,false);
    Skills->ServerRequestAbilityAction(PendingRequest,Action,SelectedSlot,Definition,Id,bActive);
}
void UMythicAbilityPickerWidget::HandleResult(FGuid Id,EMythicAbilityResult Result,int32 Revision) {
    if (Id!=PendingRequest) return;
    if (Result==EMythicAbilityResult::Success) { PendingRevision=Revision; Refresh(); }
    else { PendingRequest.Invalidate(); PendingRevision=INDEX_NONE; if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PendingTimer); SetNotice(UMythicSkillComponent::DescribeResult(Result)); if (!IsActivated() && !bOnLayer) UnbindSkills(); }
}
