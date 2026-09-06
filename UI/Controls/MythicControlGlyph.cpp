#include "MythicControlGlyph.h"
#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Input/MythicControlsComponent.h"
#include "UI/MythicUIStyle.h"
#include "UI/Widgets/MythicInputGlyph.h"

void UMythicControlGlyph::NativeOnInitialized() {
    Super::NativeOnInitialized();
    auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>(); WidgetTree->RootWidget=Row;
    Modifier=WidgetTree->ConstructWidget<UMythicInputGlyph>(); Modifier->GlyphHeight=GlyphHeight;
    Button=WidgetTree->ConstructWidget<UMythicInputGlyph>(); Button->GlyphHeight=GlyphHeight;
    Plus=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); Plus->SetText(FText::FromString(TEXT("+")));
    Unbound=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); Unbound->SetText(NSLOCTEXT("Mythic","GlyphUnbound","Unbound"));
    Row->AddChildToHorizontalBox(Modifier)->SetVerticalAlignment(VAlign_Center);
    Row->AddChildToHorizontalBox(Plus)->SetPadding(FMargin(2,0,2,0));
    Row->AddChildToHorizontalBox(Button)->SetVerticalAlignment(VAlign_Center);
    Row->AddChildToHorizontalBox(Unbound)->SetVerticalAlignment(VAlign_Center);
    SetVisibility(ESlateVisibility::HitTestInvisible); Refresh();
}
void UMythicControlGlyph::NativeConstruct() {
    Super::NativeConstruct();
    if (Controls.IsValid()) Controls->OnControlsChanged.Remove(Changed);
    Controls=GetOwningPlayer() ? GetOwningPlayer()->FindComponentByClass<UMythicControlsComponent>():nullptr;
    if (Controls.IsValid()) Changed=Controls->OnControlsChanged.AddUObject(this,&ThisClass::Refresh);
    if (auto* Input=UCommonInputSubsystem::Get(GetOwningLocalPlayer())) { Input->OnInputMethodChangedNative.Remove(DeviceHandle); DeviceHandle=Input->OnInputMethodChangedNative.AddUObject(this,&ThisClass::DeviceChanged); }
    Refresh();
}
void UMythicControlGlyph::NativeDestruct() {
    if (Controls.IsValid()) Controls->OnControlsChanged.Remove(Changed);
    if (auto* Input=UCommonInputSubsystem::Get(GetOwningLocalPlayer())) Input->OnInputMethodChangedNative.Remove(DeviceHandle);
    Changed.Reset(); DeviceHandle.Reset(); Super::NativeDestruct();
}
void UMythicControlGlyph::SetControl(EMythicControl InControl) { Control=InControl; Refresh(); }
void UMythicControlGlyph::SetDisplayInputType(ECommonInputType Type) {
    DisplayInputType=Type; Refresh();
}
void UMythicControlGlyph::ClearDisplayInputType() {
    DisplayInputType.Reset(); Refresh();
}
void UMythicControlGlyph::DeviceChanged(ECommonInputType) { Refresh(); }
void UMythicControlGlyph::Refresh() {
    if (!Button || !Modifier) return;
    const auto* Input=UCommonInputSubsystem::Get(GetOwningLocalPlayer());
    const ECommonInputType Type=DisplayInputType.Get(Input ? Input->GetCurrentInputType():ECommonInputType::MouseAndKeyboard);
    const bool Pad=Type==ECommonInputType::Gamepad;
    if (DisplayInputType.IsSet()) {
        Button->SetDisplayInputType(Type); Modifier->SetDisplayInputType(Type);
    } else {
        Button->ClearDisplayInputType(); Modifier->ClearDisplayInputType();
    }
    const FKey Key=Controls.IsValid() ? Controls->GetEffectiveKey(Control,Pad):EKeys::Invalid;
    const FKey Layer=Controls.IsValid() ? Controls->GetEffectiveKey(EMythicControl::AbilityLayer,true):EKeys::Invalid;
    const bool Chord=Pad && UMythicControlsComponent::IsSkill(Control);
    const bool Bound=Key.IsValid() && (!Chord || Layer.IsValid());
    Button->GlyphHeight=GlyphHeight; Modifier->GlyphHeight=GlyphHeight;
    Button->SetExplicitKey(Bound ? Key:EKeys::Invalid); Modifier->SetExplicitKey(Bound && Chord ? Layer:EKeys::Invalid);
    Plus->SetVisibility(Bound && Chord ? ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    Unbound->SetVisibility(Bound ? ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    SetToolTipText(Controls.IsValid() ? Controls->GetBindingLabel(Control,Pad):NSLOCTEXT("Mythic","GlyphUnbound","Unbound"));
}
