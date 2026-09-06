#include "MythicControlsPage.h"
#include "UI/Controls/MythicModalInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/MythicControlsComponent.h"
#include "Input/MythicControlUserSettings.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "UI/MythicUIStyle.h"
#include "UI/Controls/MythicControlGlyph.h"
#include "UI/Widgets/MythicInputGlyph.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

namespace {
FText ControlGroup(EMythicControl C) {
    if (C<=EMythicControl::MoveRight || C==EMythicControl::Jump || C==EMythicControl::Dash || C==EMythicControl::Crouch) return NSLOCTEXT("Mythic","ControlsMovement","Movement");
    if (C==EMythicControl::Heal || C==EMythicControl::Utility || C==EMythicControl::Ping || C==EMythicControl::Interact) return NSLOCTEXT("Mythic","ControlsItems","Items and communication");
    if (C<=EMythicControl::AbilityLayer || C==EMythicControl::LockTarget) return NSLOCTEXT("Mythic","ControlsCombat","Combat");
    if (C==EMythicControl::Inspect || C==EMythicControl::CameraMode) return NSLOCTEXT("Mythic","ControlsCamera","Camera");
    return NSLOCTEXT("Mythic","ControlsMenus","Menus");
}
}

UMythicControlsPage::UMythicControlsPage(const FObjectInitializer& Initializer):Super(Initializer) { SetIsFocusable(true); InputConfig=EMythicWidgetInputMode::Menu; GameMouseCaptureMode=EMouseCaptureMode::NoCapture; }
void UMythicControlsPage::NativeOnInitialized() { Super::NativeOnInitialized(); Build(); }
void UMythicControlsPage::InitializeControls(UMythicControlsComponent* Owner) {
    if (Controls.IsValid()) Controls->OnControlsChanged.Remove(Changed);
    Controls=Owner; Changed.Reset();
    if (Owner) Changed=Owner->OnControlsChanged.AddUObject(this,&ThisClass::Refresh);
    Build(); BindSaveState(); Refresh();
}
void UMythicControlsPage::NativeOnDeactivated() { CancelCaptureInternal(false); Super::NativeOnDeactivated(); }
void UMythicControlsPage::NativeDestruct() { CancelCaptureInternal(false); if (Controls.IsValid()) Controls->OnControlsChanged.Remove(Changed); Changed.Reset(); if (ControlUserSettings.IsValid()) ControlUserSettings->OnSaveState.Remove(SaveStateChanged); SaveStateChanged.Reset(); ControlUserSettings.Reset(); Super::NativeDestruct(); }

UCommonButtonBase* UMythicControlsPage::AddButton(UHorizontalBox* Host,const FText& Text,UCommonTextBlock*& Label) {
    auto* Button=Cast<UCommonButtonBase>(FMythicUIStyle::MakeButton(this,EMythicTextRole::Body,Label));
    if (!Button || !Label) return nullptr;
    Label->SetText(Text); Button->SetMinDimensions(0,36);
    Button->OnFocusReceived().AddUObject(this,&ThisClass::PublishFocusHelp,Button);
    auto* ButtonSlot=Host->AddChildToHorizontalBox(Button); ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ButtonSlot->SetPadding(FMargin(Host->GetChildrenCount()>1 ? 8:0,0,0,0));
    return Button;
}
UCommonButtonBase* UMythicControlsPage::AddBindingButton(UHorizontalBox* Host,EMythicControl Control,bool Pad,UCommonTextBlock*& Label,UMythicControlGlyph*& Glyph) {
    auto* Button=Cast<UCommonButtonBase>(FMythicUIStyle::MakeButton(this,EMythicTextRole::Body,Label));
    if (!Button || !Label) return nullptr;
    // Keep the house button and focus ring intact. Passive glyph content is a sibling overlay,
    // because its authored CommonButton widget does not expose a native content slot.
    Label->SetText(FText::GetEmpty()); Label->SetVisibility(ESlateVisibility::Hidden);
    Button->SetMinDimensions(0,36);
    Button->OnFocusReceived().AddUObject(this,&ThisClass::PublishFocusHelp,Button);
    auto* Overlay=WidgetTree->ConstructWidget<UOverlay>();
    auto* Column=Host->AddChildToHorizontalBox(Overlay); Column->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Column->SetPadding(FMargin(Host->GetChildrenCount()>1 ? 8:0,0,0,0));
    auto* Back=Overlay->AddChildToOverlay(Button); Back->SetHorizontalAlignment(HAlign_Fill); Back->SetVerticalAlignment(VAlign_Fill);
    Glyph=CreateWidget<UMythicControlGlyph>(GetOwningPlayer());
    if (Glyph) {
        Glyph->SetControl(Control); Glyph->SetDisplayInputType(Pad ? ECommonInputType::Gamepad:ECommonInputType::MouseAndKeyboard);
        auto* Front=Overlay->AddChildToOverlay(Glyph); Front->SetHorizontalAlignment(HAlign_Center); Front->SetVerticalAlignment(VAlign_Center);
    }
    return Button;
}
void UMythicControlsPage::Build() {
    if (!WidgetTree || Rows.Num()) return;
    auto* Root=WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget=Root;
    auto* Review=WidgetTree->ConstructWidget<UHorizontalBox>(); Root->AddChildToVerticalBox(Review);
    Status=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); Status->SetAutoWrapText(true);
    Review->AddChildToHorizontalBox(Status)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CandidateGlyph=WidgetTree->ConstructWidget<UMythicInputGlyph>(); CandidateGlyph->GlyphHeight=24;
    auto* CandidateSlot=Review->AddChildToHorizontalBox(CandidateGlyph); CandidateSlot->SetVerticalAlignment(VAlign_Center); CandidateSlot->SetPadding(FMargin(8,0,0,0));
    CandidateGlyph->SetExplicitKey(EKeys::Invalid);
    auto* SaveRow=WidgetTree->ConstructWidget<UHorizontalBox>(); Root->AddChildToVerticalBox(SaveRow)->SetPadding(FMargin(0,6,0,0));
    SaveStatus=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); SaveStatus->SetAutoWrapText(true); SaveRow->AddChildToHorizontalBox(SaveStatus)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UCommonTextBlock* RetryLabel=nullptr; Retry=AddButton(SaveRow,NSLOCTEXT("Mythic","RetryBindingsSave","Retry save"),RetryLabel);
    if (Retry) { Retry->OnClicked().AddUObject(this,&ThisClass::RetrySave); Retry->SetVisibility(ESlateVisibility::Collapsed); }
    const auto& Bindings=GetDefault<UMythicControlsSettings>()->Bindings;
    TArray<FText> Groups;
    for (const auto& Binding:Bindings) { const FText Group=ControlGroup(Binding.Control); if (!Groups.ContainsByPredicate([&](const FText& Old){return Old.EqualTo(Group);})) Groups.Add(Group); }
    for (const FText& Group:Groups) {
        auto* Heading=FMythicUIStyle::MakeText(this,EMythicTextRole::Heading); Heading->SetText(Group);
        Root->AddChildToVerticalBox(Heading)->SetPadding(FMargin(0,24,0,6));
        auto* Columns=WidgetTree->ConstructWidget<UHorizontalBox>(); Root->AddChildToVerticalBox(Columns);
        for (bool Pad:{false,true}) {
            auto* Caption=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle);
            Caption->SetText(Pad ? NSLOCTEXT("Mythic","BindingControllerColumn","Controller"):NSLOCTEXT("Mythic","BindingKeyboardColumn","Keyboard / mouse"));
            Caption->SetJustification(ETextJustify::Center);
            auto* CaptionSlot=Columns->AddChildToHorizontalBox(Caption); CaptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); CaptionSlot->SetPadding(FMargin(Pad ? 8:0,0,0,0));
        }
        for (const auto& Binding:Bindings) if (ControlGroup(Binding.Control).EqualTo(Group)) {
            auto* Label=FMythicUIStyle::MakeText(this,EMythicTextRole::Body); Label->SetText(Binding.Label);
            Root->AddChildToVerticalBox(Label)->SetPadding(FMargin(0,8,0,4));
            auto* Pair=WidgetTree->ConstructWidget<UHorizontalBox>(); Root->AddChildToVerticalBox(Pair);
            FMythicControlRowWidgets Row; Row.Control=Binding.Control;
            UCommonTextBlock* KeyboardLabel=nullptr; UCommonTextBlock* GamepadLabel=nullptr;
            UMythicControlGlyph* KeyboardGlyph=nullptr; UMythicControlGlyph* GamepadGlyph=nullptr;
            Row.Keyboard=AddBindingButton(Pair,Binding.Control,false,KeyboardLabel,KeyboardGlyph); Row.KeyboardLabel=KeyboardLabel; Row.KeyboardGlyph=KeyboardGlyph;
            Row.Gamepad=AddBindingButton(Pair,Binding.Control,true,GamepadLabel,GamepadGlyph); Row.GamepadLabel=GamepadLabel; Row.GamepadGlyph=GamepadGlyph;
            if (Row.Keyboard) Row.Keyboard->OnClicked().AddUObject(this,&ThisClass::BeginCapture,Binding.Control,false);
            if (Row.Gamepad) Row.Gamepad->OnClicked().AddUObject(this,&ThisClass::BeginCapture,Binding.Control,true);
            Rows.Add(Row);
        }
    }
    auto* Footer=WidgetTree->ConstructWidget<UHorizontalBox>(); Root->AddChildToVerticalBox(Footer)->SetPadding(FMargin(0,14,0,0));
    UCommonTextBlock* Label=nullptr;
    UCommonTextBlock* ApplyLabel=nullptr;
    Confirm=AddButton(Footer,NSLOCTEXT("Mythic","BindApply","Apply binding"),ApplyLabel); ConfirmLabel=ApplyLabel;
    Cancel=AddButton(Footer,NSLOCTEXT("Mythic","BindCancel","Cancel"),Label);
    Clear=AddButton(Footer,NSLOCTEXT("Mythic","BindClear","Clear"),Label);
    Reset=AddButton(Footer,NSLOCTEXT("Mythic","BindReset","Reset mappings"),Label);
    if (Confirm) Confirm->OnClicked().AddUObject(this,&ThisClass::ConfirmCapture);
    if (Cancel) Cancel->OnClicked().AddUObject(this,&ThisClass::CancelCapture);
    if (Clear) Clear->OnClicked().AddUObject(this,&ThisClass::ClearCapture);
    if (Reset) Reset->OnClicked().AddUObject(this,&ThisClass::ResetAll);
    Refresh();
}
UCommonButtonBase* UMythicControlsPage::FindRowButton(EMythicControl C,bool Pad) const {
    const auto* Row=Rows.FindByPredicate([&](const auto& R){return R.Control==C;}); return Row ? (Pad ? Row->Gamepad.Get():Row->Keyboard.Get()):nullptr;
}
void UMythicControlsPage::SetRailFocus(UWidget* Rail,UWidget* NextSetting) {
    FollowingSetting=NextSetting;
    if (NextSetting && Reset) NextSetting->SetNavigationRuleExplicit(EUINavigation::Up,Reset);
    for (int32 Index=0;Index<Rows.Num();++Index) {
        auto& Row=Rows[Index];
        if (Row.Keyboard && Row.Gamepad) {
            Row.Keyboard->SetNavigationRuleExplicit(EUINavigation::Right,Row.Gamepad);
            Row.Gamepad->SetNavigationRuleExplicit(EUINavigation::Left,Row.Keyboard);
            if (Rail) Row.Keyboard->SetNavigationRuleExplicit(EUINavigation::Left,Rail);
        }
        for (bool Pad:{false,true}) if (auto* Button=Pad ? Row.Gamepad.Get():Row.Keyboard.Get()) {
            if (Index>0) Button->SetNavigationRuleExplicit(EUINavigation::Up,Pad ? Rows[Index-1].Gamepad.Get():Rows[Index-1].Keyboard.Get());
            if (Index+1<Rows.Num()) Button->SetNavigationRuleExplicit(EUINavigation::Down,Pad ? Rows[Index+1].Gamepad.Get():Rows[Index+1].Keyboard.Get());
            else if (Reset) Button->SetNavigationRuleExplicit(EUINavigation::Down,Reset);
        }
    }
    RefreshFooterNavigation();
}

void UMythicControlsPage::RefreshFooterNavigation() {
    // Review disables every binding row and Reset. Spatial navigation cannot
    // reliably reach adjacent buttons in this scrolled footer, so connect only
    // the currently enabled rail without granting any disabled action focus.
    TArray<UCommonButtonBase*> Enabled;
    for (auto* Button:{Confirm.Get(),Cancel.Get(),Clear.Get(),Reset.Get()}) {
        if (!Button) continue;
        for (const auto Direction:{EUINavigation::Left,EUINavigation::Right,EUINavigation::Up,EUINavigation::Down})
            Button->SetNavigationRuleBase(Direction,EUINavigationRule::Stop);
        if (Button->GetIsEnabled() && Button->IsVisible()) Enabled.Add(Button);
    }
    for (int32 Index=0;Index<Enabled.Num();++Index) {
        auto* Button=Enabled[Index];
        if (Index>0) Button->SetNavigationRuleExplicit(EUINavigation::Left,Enabled[Index-1]);
        if (Index+1<Enabled.Num()) Button->SetNavigationRuleExplicit(EUINavigation::Right,Enabled[Index+1]);
        // Returning from Reset has a stable matching destination. During capture
        // the cells remain disabled until Cancel/Apply/Clear returns the owner.
        if (!IsCapturing() && !Rows.IsEmpty()) {
            auto* Last=Rows.Last().Keyboard.Get();
            if (Last && Last->GetIsEnabled()) Button->SetNavigationRuleExplicit(EUINavigation::Up,Last);
            if (Button==Reset && FollowingSetting.IsValid()) Button->SetNavigationRuleExplicit(EUINavigation::Down,FollowingSetting.Get());
        }
    }
}

void UMythicControlsPage::PublishFocusHelp(UCommonButtonBase* Button) {
    if (!IsActivated() && !Button) return;
    auto* Player=GetOwningPlayer();
    if (Button && (!Player || !Button->GetIsEnabled() ||
        (!Button->HasUserFocus(Player) && !Button->HasUserFocusedDescendants(Player)))) return;
    const auto DescribeBinding=[this](EMythicControl Control,bool Pad) {
        const auto* Definition=GetDefault<UMythicControlsSettings>()->Find(Control);
        const FText Title=Definition ? Definition->Label:NSLOCTEXT("Mythic","ControlsFocusBinding","Binding");
        const FText Current=Controls.IsValid() ? Controls->GetBindingLabel(Control,Pad):FText::GetEmpty();
        const FText Value=Current.IsEmpty() ? NSLOCTEXT("Mythic","ControlsFocusUnbound","Unbound"):Current;
        const FText Body=FText::Format(Pad
            ? NSLOCTEXT("Mythic","ControlsPadFocusHelp","Controller binding: {0}. Choose a fresh button, release it, then review before applying.")
            : NSLOCTEXT("Mythic","ControlsKeyFocusHelp","Keyboard / mouse binding: {0}. Choose a fresh key, release it, then review before applying."),Value);
        OnFocusHelp.Broadcast(Title,Body);
    };
    if (!Button) {
        if (IsCapturing()) {
            const auto* Definition=GetDefault<UMythicControlsSettings>()->Find(CaptureControl);
            OnFocusHelp.Broadcast(Definition ? Definition->Label:FText::GetEmpty(),Status ? Status->GetText():FText::GetEmpty());
        }
        return;
    }
    for (const auto& Row:Rows) {
        if (Button==Row.Keyboard) { DescribeBinding(Row.Control,false); return; }
        if (Button==Row.Gamepad) { DescribeBinding(Row.Control,true); return; }
    }
    if (Button==Reset) OnFocusHelp.Broadcast(NSLOCTEXT("Mythic","BindReset","Reset mappings"),
        NSLOCTEXT("Mythic","ControlsResetFocusHelp","Restore all keyboard and controller bindings to their defaults."));
    else if (Button==Retry) OnFocusHelp.Broadcast(NSLOCTEXT("Mythic","RetryBindingsSave","Retry save"),
        NSLOCTEXT("Mythic","ControlsRetryFocusHelp","Try saving your current bindings again."));
    else if (Button==Confirm) {
        const auto* Definition=GetDefault<UMythicControlsSettings>()->Find(CaptureControl);
        OnFocusHelp.Broadcast(ConfirmLabel ? ConfirmLabel->GetText():FText::GetEmpty(),
            FText::Format(NSLOCTEXT("Mythic","ControlsApplyFocusHelp","Set {0} to {1}. {2}"),
                Definition ? Definition->Label:FText::GetEmpty(),Candidate.GetDisplayName(),Status ? Status->GetText():FText::GetEmpty()));
    }
    else if (Button==Cancel) OnFocusHelp.Broadcast(NSLOCTEXT("Mythic","BindCancel","Cancel"),
        NSLOCTEXT("Mythic","ControlsCancelFocusHelp","Discard the pending binding change and return to the selected action."));
    else if (Button==Clear) OnFocusHelp.Broadcast(NSLOCTEXT("Mythic","BindClear","Clear"),
        NSLOCTEXT("Mythic","ControlsClearFocusHelp","Remove the selected binding. Other bindings stay as they are."));
}

void UMythicControlsPage::BindSaveState() {
    if (ControlUserSettings.IsValid()) ControlUserSettings->OnSaveState.Remove(SaveStateChanged);
    SaveStateChanged.Reset(); ControlUserSettings.Reset(); bObservedSaveSuccess=false;
    if (const auto* LP=GetOwningLocalPlayer()) if (const auto* Subsystem=LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        ControlUserSettings=Cast<UMythicControlUserSettings>(Subsystem->GetUserSettings());
    if (ControlUserSettings.IsValid()) SaveStateChanged=ControlUserSettings->OnSaveState.AddUObject(this,&ThisClass::HandleSaveState);
    RefreshSaveState();
}
void UMythicControlsPage::HandleSaveState(bool bSaving,bool bSuccess) {
    bObservedSaveSuccess=!bSaving && bSuccess; RefreshSaveState();
}
void UMythicControlsPage::RefreshSaveState() {
    if (!SaveStatus) return;
    const bool Saving=ControlUserSettings.IsValid() && ControlUserSettings->IsSaving();
    const bool Failed=ControlUserSettings.IsValid() && ControlUserSettings->DidSaveFail();
    SaveStatus->SetText(Saving ? NSLOCTEXT("Mythic","BindingsSaving","Saving bindings..."):
        Failed ? NSLOCTEXT("Mythic","BindingsSaveFailed","Bindings apply now, but could not be saved. Retry saving."):
        bObservedSaveSuccess ? NSLOCTEXT("Mythic","BindingsSaved","Bindings saved."):FText::GetEmpty());
    if (Retry) { Retry->SetVisibility(Failed && !Saving ? ESlateVisibility::Visible:ESlateVisibility::Collapsed); Retry->SetIsEnabled(Failed && !Saving && !IsCapturing()); }
}
void UMythicControlsPage::RetrySave() { if (ControlUserSettings.IsValid() && !ControlUserSettings->IsSaving() && !IsCapturing()) ControlUserSettings->AsyncSaveSettings(); }

void UMythicControlsPage::Refresh() {
    if (!Status) return;
    RefreshSaveState();
    for (auto& Row:Rows) for (bool Pad:{false,true}) {
        auto* Label=Pad ? Row.GamepadLabel.Get():Row.KeyboardLabel.Get(); auto* Button=Pad ? Row.Gamepad.Get():Row.Keyboard.Get();
        auto* Glyph=Pad ? Row.GamepadGlyph.Get():Row.KeyboardGlyph.Get();
        if (Glyph) { Glyph->SetControl(Row.Control); Glyph->SetDisplayInputType(Pad ? ECommonInputType::Gamepad:ECommonInputType::MouseAndKeyboard); }
        // Retain a truthful text fallback if widget allocation failed; ordinary rows use shared platform art.
        if (Label && !Glyph) { Label->SetVisibility(ESlateVisibility::HitTestInvisible); Label->SetText(Controls.IsValid() ? Controls->GetBindingLabel(Row.Control,Pad):NSLOCTEXT("Mythic","UnboundControl","Unbound")); }
        if (Button) {
            Button->SetIsEnabled(Controls.IsValid() && !IsCapturing());
            const auto* Definition=GetDefault<UMythicControlsSettings>()->Find(Row.Control);
            Button->SetToolTipText(FText::Format(Pad ? NSLOCTEXT("Mythic","ChangeControllerBinding","Change controller binding: {0}"):NSLOCTEXT("Mythic","ChangeKeyboardBinding","Change keyboard / mouse binding: {0}"),Definition ? Definition->Label:FText::GetEmpty()));
        }
    }
    if (CandidateGlyph) {
        CandidateGlyph->SetDisplayInputType(bCaptureGamepad ? ECommonInputType::Gamepad:ECommonInputType::MouseAndKeyboard);
        CandidateGlyph->SetExplicitKey(IsCapturing() ? Candidate:EKeys::Invalid);
    }
    if (Confirm) Confirm->SetIsEnabled(IsCapturing() && IsCaptureReviewReady());
    if (Cancel) Cancel->SetIsEnabled(IsCapturing() && CaptureConsumedDowns.IsEmpty() && !bCancelCaptureQueued);
    if (Clear) Clear->SetIsEnabled(IsCapturing() && CaptureConsumedDowns.IsEmpty() && !bCancelCaptureQueued);
    if (Reset) Reset->SetIsEnabled(Controls.IsValid() && !IsCapturing());
    RefreshFooterNavigation();
    if (ConfirmLabel) ConfirmLabel->SetText(Conflict!=EMythicControl::Count ? NSLOCTEXT("Mythic","BindSwap","Swap bindings"):NSLOCTEXT("Mythic","BindApply","Apply binding"));
    if (!IsCapturing()) Status->SetText(NSLOCTEXT("Mythic","BindHelp","Choose a keyboard or controller binding. Review changes before applying them."));
}
void UMythicControlsPage::BeginCapture(EMythicControl C,bool Pad) {
    if (!Controls.IsValid() || IsCapturing()) return;
    Controls->CancelHeldInput(); CaptureControl=C; bCaptureGamepad=Pad; Candidate=EKeys::Invalid; Conflict=EMythicControl::Count; bCandidateReleased=false;
    IgnoreUntilRelease.Reset(); TArray<FKey> Keys; EKeys::GetAllKeys(Keys);
    if (auto* PC=GetOwningPlayer()) for (const FKey& Key:Keys) if (PC->IsInputKeyDown(Key)) IgnoreUntilRelease.Add(Key);
    OnCaptureStateChanged.Broadcast(true); Refresh();
    Status->SetText(NSLOCTEXT("Mythic","BindCapture","Press a fresh key or button, release it, then apply. Escape cancels."));
    PublishFocusHelp(nullptr);
    SetUserFocus(GetOwningPlayer());
    if (!StartCaptureInput()) {
        CancelCaptureInternal(false);
        Status->SetText(NSLOCTEXT("Mythic","BindCaptureUnavailable","Binding capture could not claim this controls page. Focus it and try again."));
    }
}
bool UMythicControlsPage::StartCaptureInput() {
    StopCaptureInput();
    const TWeakObjectPtr<UMythicControlsPage> Weak(this);
    FMythicModalInputProcessor::FRoutes Routes;
    Routes.Key=[Weak](FKey Key,EInputEvent Event,bool Repeat) {
        auto* Self=Weak.Get(); return Self && Self->CaptureEvent(Key,Event,Repeat).IsEventHandled();
    };
    Routes.MouseButton=[Weak](const FPointerEvent& Event,EInputEvent Input) {
        auto* Self=Weak.Get(); return Self && Self->CapturePointerEvent(Event,Input).IsEventHandled();
    };
    Routes.Analog=[Weak](const FAnalogInputEvent&) {
        auto* Self=Weak.Get(); return Self && Self->IsCapturing() && !Self->IsCaptureReviewReady();
    };
    Routes.FocusLost=[Weak]() { if (auto* Self=Weak.Get()) Self->CancelCaptureInternal(false); };
    CaptureProcessor=MakeShared<FMythicModalInputProcessor>(this,MoveTemp(Routes));
    return CaptureProcessor->Start();
}
void UMythicControlsPage::StopCaptureInput() {
    auto Previous=MoveTemp(CaptureProcessor); CaptureProcessor.Reset();
    if (Previous) Previous->KeepOwnedReleases(MoveTemp(CaptureConsumedDowns));
    PassthroughCaptureMouse.Reset(); CaptureConsumedDowns.Reset();
}
FReply UMythicControlsPage::CapturePointerEvent(const FPointerEvent& Event,EInputEvent Input) {
    const FKey Key=Event.GetEffectingButton();
    if (Input==IE_Released && PassthroughCaptureMouse.Remove(Key)>0) return FReply::Unhandled();
    // Escape remains available on keyboard; real Cancel/Clear button clicks remain available while awaiting a key.
    const auto Over=[&](const UCommonButtonBase* Button) {
        return Button && Button->GetIsEnabled() && Button->IsVisible()
            && Button->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition());
    };
    if (Input==IE_Pressed && Key==EKeys::LeftMouseButton && (Over(Cancel)||Over(Clear))) {
        PassthroughCaptureMouse.Add(Key); return FReply::Unhandled();
    }
    return CaptureEvent(Key,Input);
}
void UMythicControlsPage::CaptureKey(FKey Key) {
    if (!Controls.IsValid() || !IsCapturing() || Key.IsGamepadKey()!=bCaptureGamepad || Key.IsAnalog()) return;
    Candidate=Key; bCandidateReleased=false; Conflict=EMythicControl::Count;
    const bool Layer=bCaptureGamepad && UMythicControlsComponent::IsSkill(CaptureControl);
    for (const auto& Row:GetDefault<UMythicControlsSettings>()->Bindings) if (Row.Control!=CaptureControl && (!bCaptureGamepad || Row.bGamepadAbilityLayer==Layer) && Controls->GetEffectiveKey(Row.Control,bCaptureGamepad)==Key) { Conflict=Row.Control; break; }
    const auto* Other=GetDefault<UMythicControlsSettings>()->Find(Conflict);
    Status->SetText(Other ? FText::Format(NSLOCTEXT("Mythic","BindConflictGlyph","This button is used by {0}. Release, then choose Swap or Cancel."),Other->Label):NSLOCTEXT("Mythic","BindReviewGlyph","Use this binding? Release it, then apply or cancel."));
    Refresh();
    PublishFocusHelp(nullptr);
}
void UMythicControlsPage::ConfirmCapture() {
    if (!Controls.IsValid() || !IsCapturing() || !IsCaptureReviewReady()) return;
    FText Error;
    const bool Success=Conflict==EMythicControl::Count ? Controls->RemapControl(CaptureControl,bCaptureGamepad,Candidate,Error):Controls->SwapControls(CaptureControl,Conflict,bCaptureGamepad,Error);
    if (Success) CancelCapture(); else { Status->SetText(Error); if (Cancel) Cancel->SetUserFocus(GetOwningPlayer()); }
}
void UMythicControlsPage::ClearCapture() { if (!Controls.IsValid() || !IsCapturing() || !CaptureConsumedDowns.IsEmpty() || bCancelCaptureQueued) return; FText Error; if (Controls->RemapControl(CaptureControl,bCaptureGamepad,EKeys::Invalid,Error)) CancelCapture(); else Status->SetText(Error); }
void UMythicControlsPage::CancelCapture() {
    if (IsCapturing() && !CaptureConsumedDowns.IsEmpty()) {
        bCancelCaptureQueued=true; Refresh();
        if (Status) Status->SetText(NSLOCTEXT("Mythic","BindCancelRelease","Release held buttons to cancel binding capture."));
        return;
    }
    CancelCaptureInternal(true);
}
void UMythicControlsPage::CancelCaptureInternal(bool bRestoreFocus) {
    StopCaptureInput(); bCancelCaptureQueued=false;
    if (!IsCapturing()) return;
    const auto C=CaptureControl; const bool Pad=bCaptureGamepad;
    CaptureControl=EMythicControl::Count; Candidate=EKeys::Invalid; Conflict=EMythicControl::Count; IgnoreUntilRelease.Reset();
    OnCaptureStateChanged.Broadcast(false); Refresh();
    if (auto* Button=FindRowButton(C,Pad); bRestoreFocus && Button && IsActivated()) Button->SetUserFocus(GetOwningPlayer());
}
void UMythicControlsPage::ResetAll() { if (Controls.IsValid() && !IsCapturing()) { Controls->ResetMappings(); Refresh(); } }
bool UMythicControlsPage::TryHandleNestedBackAction() { if (!IsCapturing()) return false; CancelCapture(); return true; }
UWidget* UMythicControlsPage::NativeGetDesiredFocusTarget() const {
    // CommonUI requests this again on input-device changes, before the triggering
    // navigation key is routed. Preserve this owner's current cell/footer rather
    // than moving to row zero and applying that key to the wrong binding.
    if (IsCapturing() && !IsCaptureReviewReady()) return const_cast<UMythicControlsPage*>(this);
    auto* Player=GetOwningPlayer();
    const auto IsCurrentFocus=[Player](const UCommonButtonBase* Button) {
        return Player && Button && Button->GetIsEnabled() && Button->IsVisible()
            && (Button->HasUserFocus(Player) || Button->HasUserFocusedDescendants(Player));
    };
    for (const auto& Row:Rows) {
        if (IsCurrentFocus(Row.Keyboard.Get())) return Row.Keyboard.Get();
        if (IsCurrentFocus(Row.Gamepad.Get())) return Row.Gamepad.Get();
    }
    for (auto* Button:{Confirm.Get(),Cancel.Get(),Clear.Get(),Reset.Get()}) {
        if (IsCurrentFocus(Button)) return Button;
    }
    if (IsCapturing()) return Confirm ? static_cast<UWidget*>(Confirm.Get()):const_cast<UMythicControlsPage*>(this);
    return Rows.IsEmpty() ? nullptr:Rows[0].Keyboard.Get();
}
bool UMythicControlsPage::OwnsUser(int32 Index) const { const auto* LP=GetOwningLocalPlayer(); const auto User=LP ? LP->GetSlateUser():nullptr; return User && User->GetUserIndex()==Index; }
FReply UMythicControlsPage::CaptureEvent(FKey Key,EInputEvent Event,bool Repeat) {
    if (!IsCapturing()) return FReply::Unhandled();
    // Drain an owned press before allowing CommonUI to interpret review input. In particular an A-up
    // cannot synthesize a mouse release after its A-down was swallowed while a different key was held.
    if (CaptureConsumedDowns.Contains(Key)) {
        if (Event==IE_Released) {
            CaptureConsumedDowns.Remove(Key); IgnoreUntilRelease.Remove(Key);
            if (Candidate==Key) bCandidateReleased=true;
            if (bCancelCaptureQueued && CaptureConsumedDowns.IsEmpty()) { CancelCaptureInternal(true); return FReply::Handled(); }
            Refresh();
            if (IsCaptureReviewReady() && Confirm) Confirm->SetUserFocus(GetOwningPlayer());
        }
        return FReply::Handled();
    }
    if (IsCaptureReviewReady()) return FReply::Unhandled();
    if (Event==IE_Released) { IgnoreUntilRelease.Remove(Key); return FReply::Handled(); }
    CaptureConsumedDowns.Add(Key); Refresh();
    if (bCancelCaptureQueued || Repeat || IgnoreUntilRelease.Contains(Key)) return FReply::Handled();
    if (Key==EKeys::Escape) { CancelCapture(); return FReply::Handled(); }
    if (Candidate.IsValid()) return FReply::Handled();
    CaptureKey(Key); return FReply::Handled();
}
FReply UMythicControlsPage::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E) { if (CaptureProcessor) return Super::NativeOnPreviewKeyDown(G,E); const auto R=OwnsUser(E.GetUserIndex()) ? CaptureEvent(E.GetKey(),IE_Pressed,E.IsRepeat()):FReply::Unhandled(); return R.IsEventHandled() ? R:Super::NativeOnPreviewKeyDown(G,E); }
FReply UMythicControlsPage::NativeOnKeyUp(const FGeometry& G,const FKeyEvent& E) { if (CaptureProcessor) return Super::NativeOnKeyUp(G,E); const auto R=OwnsUser(E.GetUserIndex()) ? CaptureEvent(E.GetKey(),IE_Released):FReply::Unhandled(); return R.IsEventHandled() ? R:Super::NativeOnKeyUp(G,E); }
FReply UMythicControlsPage::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) { if (CaptureProcessor) return Super::NativeOnMouseButtonDown(G,E); const auto R=OwnsUser(E.GetUserIndex()) ? CaptureEvent(E.GetEffectingButton(),IE_Pressed):FReply::Unhandled(); return R.IsEventHandled() ? R:Super::NativeOnMouseButtonDown(G,E); }
FReply UMythicControlsPage::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E) { if (CaptureProcessor) return Super::NativeOnMouseButtonUp(G,E); const auto R=OwnsUser(E.GetUserIndex()) ? CaptureEvent(E.GetEffectingButton(),IE_Released):FReply::Unhandled(); return R.IsEventHandled() ? R:Super::NativeOnMouseButtonUp(G,E); }
