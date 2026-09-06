#include "MythicCharacterPageWidget.h"
#include "MythicAbilityPickerWidget.h"
#include "MythicRuneSocketWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/MythicInputConfig.h"
#include "Player/MythicCharacter.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "UI/MythicUIStyle.h"
#include "UI/MythicHUDLayout.h"
#include "UI/Controls/MythicControlGlyph.h"
#include "UI/Widgets/MythicSectionHeader.h"

void UMythicCharacterPageWidget::BuildAbilitySockets() {
    if (!AbilitySocketStrip || !WidgetTree || !SocketClass || !GetOwningPlayer() || !AbilitySockets.IsEmpty()) return;
    if (AbilityHeaderHost && SocketHeaderClass) {
        AbilityHeader=CreateWidget<UMythicSectionHeader>(GetOwningPlayer(),SocketHeaderClass);
        AbilityHeaderHost->AddChild(AbilityHeader);
        AbilityHeader->SetHeader(NSLOCTEXT("Mythic","CharacterAbilities","Abilities"),FText::GetEmpty());
    }
    for (int32 i=0;i<UMythicSkillComponent::SlotCount;++i) {
        auto* Column=WidgetTree->ConstructWidget<UVerticalBox>();
        if (auto* ColumnSlot=Cast<UHorizontalBoxSlot>(AbilitySocketStrip->AddChild(Column))) {
            ColumnSlot->SetPadding(FMargin(i==0 ? 0:FMythicUIStyle::Get().SpaceS,0,0,0));
            ColumnSlot->SetVerticalAlignment(VAlign_Center);
        }
        auto* Socket=CreateWidget<UMythicRuneSocketWidget>(GetOwningPlayer(),SocketClass);
        Socket->SetSlotIndex(i); Socket->SetInteractive(true);
        Socket->OnPressed.AddUniqueDynamic(this,&ThisClass::OpenAbilityPicker);
        Column->AddChildToVerticalBox(Socket)->SetHorizontalAlignment(HAlign_Center); AbilitySockets.Add(Socket);
        auto* Glyph=WidgetTree->ConstructWidget<UMythicControlGlyph>();
        Glyph->GlyphHeight=22;
        auto* GlyphSlot=Column->AddChildToVerticalBox(Glyph); GlyphSlot->SetHorizontalAlignment(HAlign_Center); GlyphSlot->SetPadding(FMargin(0,FMythicUIStyle::Get().SpaceS,0,0)); AbilityGlyphs.Add(Glyph);
    }
    RefreshAbilitySockets();
}
void UMythicCharacterPageWidget::UnbindSkills() {
    if (BoundSkills.IsValid()) BoundSkills->OnSkillsChanged.RemoveDynamic(this,&ThisClass::RefreshAbilitySockets);
    BoundSkills.Reset();
}
void UMythicCharacterPageWidget::RefreshAbilitySockets() {
    // The first icon request can complete before the picker has ever opened.
    if (AbilityPicker) AbilityPicker->SetCharacterPage(this);
    const auto* PS=GetOwningPlayer() ? GetOwningPlayer()->GetPlayerState<AMythicPlayerState>():nullptr;
    auto* Skills=PS ? PS->GetSkillComponent():nullptr;
    if (BoundSkills.Get()!=Skills) { UnbindSkills(); BoundSkills=Skills; if (Skills) Skills->OnSkillsChanged.AddUniqueDynamic(this,&ThisClass::RefreshAbilitySockets); }
    int32 Equipped=0;
    for (int32 i=0;i<AbilitySockets.Num();++i) {
        auto* Definition=Skills ? Skills->GetSkillInSlot(i):nullptr;
        const bool Unlocked=Skills && Skills->IsSlotUnlocked(i);
        AbilitySockets[i]->SetState(!Unlocked ? EMythicRuneSocketState::Sealed:Definition ? EMythicRuneSocketState::Filled:EMythicRuneSocketState::Empty,
            AbilityPicker ? AbilityPicker->FindPreloadedIcon(Definition):nullptr,FLinearColor::White);
        AbilitySockets[i]->SetToolTipText(Definition ? Definition->Name:NSLOCTEXT("Mythic","AbilitySocketEmpty","Choose an ability"));
        if (AbilityGlyphs.IsValidIndex(i)) AbilityGlyphs[i]->SetControl(static_cast<EMythicControl>(static_cast<uint8>(EMythicControl::SkillQ)+i));
        if (Definition) ++Equipped;
    }
    if (AbilityHeader) AbilityHeader->SetHeader(NSLOCTEXT("Mythic","CharacterAbilities","Abilities"),FText::Format(NSLOCTEXT("Mythic","AbilitySocketCountFour","{0} / {1}"),Equipped,UMythicSkillComponent::SlotCount));
}
void UMythicCharacterPageWidget::OpenAbilityPicker(int32 Index) {
    if (!AbilityPicker || !IsActivated() || AbilityPicker->IsOnLayer() || Index<0 || Index>=UMythicSkillComponent::SlotCount) return;
    LastOpenedAbilitySocket=Index; LastOpenedSocket=INDEX_NONE;
    if (auto* HUD=FindHUDLayout()) AbilityPicker->ConfigureMenuDismissAction(HUD->GetOpenMenuAction());
    AbilityPicker->OpenForSlot(Index,this);
}
void UMythicCharacterPageWidget::FocusAbilitySocket(int32 Index) {
    if (!IsActivated() || !GetOwningPlayer() || (AbilityPicker && AbilityPicker->IsOnLayer())) return;
    LastOpenedAbilitySocket=Index;
    if (AbilitySockets.IsValidIndex(Index)) if (auto* Hit=AbilitySockets[Index]->GetFocusWidget()) Hit->SetUserFocus(GetOwningPlayer());
}
