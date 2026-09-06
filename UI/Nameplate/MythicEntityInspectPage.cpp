#include "UI/Nameplate/MythicEntityInspectPage.h"

#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ICommonInputModule.h"
#include "Input/MythicControlsComponent.h"
#include "UI/Controls/MythicModalInputProcessor.h"
#include "UI/MythicUIStyle.h"
#include "UI/Nameplate/MythicNameplateDirector.h"
#include "UI/Widgets/MythicBoundActionButton.h"
#include "UI/Nameplate/MythicEntityInspectViewModel.h"

#define LOCTEXT_NAMESPACE "MythicEntityInspectPage"

namespace {
void ApplyInspectOptionalText(UTextBlock *Widget, const FText &Text) {
    if (!Widget) {
        return;
    }
    Widget->SetText(Text);
    Widget->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed
                                        : ESlateVisibility::HitTestInvisible);
}

FText BuildFactSection(
    const FText &Heading,
    const TArray<FMythicEntityInspectFactProjection> &Facts) {
    TArray<FText> Rows;
    Rows.Reserve(Facts.Num());
    for (const FMythicEntityInspectFactProjection &Fact : Facts) {
        if (Fact.ResolvedLabel.IsEmpty()) {
            continue;
        }
        Rows.Add(Fact.ResolvedDescription.IsEmpty()
                     ? Fact.ResolvedLabel
                     : FText::Format(LOCTEXT("FactDescriptionFormat", "{0} — {1}"),
                                     Fact.ResolvedLabel,
                                     Fact.ResolvedDescription));
    }
    if (Rows.IsEmpty()) {
        return FText::GetEmpty();
    }
    return FText::Format(
        LOCTEXT("FactSectionFormat", "{0}\n{1}"), Heading,
        FText::Join(LOCTEXT("FactSeparator", "\n"), Rows));
}

FText BuildThreatText(const FMythicEntityInspectProjection &Projection) {
    if (Projection.bBoss) {
        return LOCTEXT("BossThreat", "Boss");
    }
    if (!Projection.bCombatCapable) {
        return FText::GetEmpty();
    }
    switch (Projection.ThreatBand) {
    case EMythicThreatBand::None:
        return LOCTEXT("ThreatNone", "Combat capable");
    case EMythicThreatBand::Risky:
        return LOCTEXT("ThreatRisky", "Risky");
    case EMythicThreatBand::Deadly:
        return LOCTEXT("ThreatDeadly", "Deadly");
    case EMythicThreatBand::Overwhelming:
        return LOCTEXT("ThreatOverwhelming", "Overwhelming");
    default:
        return LOCTEXT("ThreatUnknown", "Combat capable");
    }
}
}

UMythicEntityInspectPage::UMythicEntityInspectPage(const FObjectInitializer& Initializer):Super(Initializer) {
    SetIsFocusable(true); InputConfig=EMythicWidgetInputMode::Menu; GameMouseCaptureMode=EMouseCaptureMode::NoCapture;
    bIsBackHandler=true; bAutoRestoreFocus=false;
}

void UMythicEntityInspectPage::NativeOnInitialized() {
    // Enforce the scoped close contract even for existing Blueprint page defaults.
    bIsBackHandler=true; SetIsFocusable(true);
    Super::NativeOnInitialized();
    BuildCloseFooter();
    if (!ViewModel) {
        ViewModel = NewObject<UMythicEntityInspectViewModel>(this);
    }
}

void UMythicEntityInspectPage::NativeDestruct() {
    StopInspectInput();
    if (ViewModel) {
        ViewModel->Reset();
    }
    ResetNativeBindings();
    Super::NativeDestruct();
}

void UMythicEntityInspectPage::BuildCloseFooter() {
    if (!WidgetTree || CloseButton) return;
    const auto& Style=FMythicUIStyle::Get();
    UClass* ButtonClass=Style.ActionButtonClass.LoadSynchronous();
    if (!ButtonClass || !ButtonClass->IsChildOf(UMythicBoundActionButton::StaticClass())) return;
    // Preserve the authored sanitized dossier tree. The native footer adds only a real close affordance.
    UWidget* Existing=WidgetTree->RootWidget;
    auto* Root=WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget=Root;
    if (Existing) { Existing->RemoveFromParent(); Root->AddChildToVerticalBox(Existing)->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
    CloseButton=WidgetTree->ConstructWidget<UMythicBoundActionButton>(ButtonClass);
    if (auto* ButtonStyle=Style.QuietActionButtonStyle.LoadSynchronous()) CloseButton->SetStyle(ButtonStyle);
    auto* FooterSlot=Root->AddChildToVerticalBox(CloseButton); FooterSlot->SetHorizontalAlignment(HAlign_Right); FooterSlot->SetPadding(FMargin(0,8,0,0));
    CloseButton->SetActionBarPromptOnly(false);
    CloseButton->SetLabelOverride(LOCTEXT("InspectClose","Close inspection"));
    CloseButton->SetGlyphEnhancedAction(ICommonInputModule::GetSettings().GetEnhancedInputBackAction());
    CloseButton->OnClicked().AddUObject(this,&ThisClass::RequestInspectClose);
}

TOptional<FUIInputConfig> UMythicEntityInspectPage::GetDesiredInputConfig() const {
    FUIInputConfig Config(ECommonInputMode::Menu,EMouseCaptureMode::NoCapture);
    Config.bIgnoreLookInput=true; Config.bIgnoreMoveInput=true; return Config;
}
UWidget* UMythicEntityInspectPage::NativeGetDesiredFocusTarget() const { if (CloseButton) return CloseButton.Get(); return const_cast<UMythicEntityInspectPage*>(this); }
void UMythicEntityInspectPage::BeginInspectInputOwnership() {
    StopInspectInput();
    if (!IsActivated() || !GetOwningLocalPlayer() || !GetOwningPlayer()) { RequestInspectClose(); return; }
    if (auto* Controls=GetOwningPlayer()->FindComponentByClass<UMythicControlsComponent>()) Controls->CancelHeldInput();
    NativeGetDesiredFocusTarget()->SetUserFocus(GetOwningPlayer());
    const TWeakObjectPtr<UMythicEntityInspectPage> Weak(this);
    FMythicModalInputProcessor::FRoutes Routes;
    Routes.Key=[Weak](FKey Key,EInputEvent Event,bool Repeat) { auto* Page=Weak.Get(); return Page && Page->RouteInspectKey(Key,Event,Repeat); };
    Routes.FocusLost=[Weak]() { if (auto* Page=Weak.Get()) Page->RequestInspectClose(); };
    InspectInput=MakeShared<FMythicModalInputProcessor>(this,MoveTemp(Routes));
    if (!InspectInput->Start()) RequestInspectClose();
}
void UMythicEntityInspectPage::StopInspectInput() {
    auto Previous=MoveTemp(InspectInput); InspectInput.Reset(); if (Previous) Previous->Stop();
}
void UMythicEntityInspectPage::RequestInspectClose() {
    StopInspectInput();
    if (auto* LP=GetOwningLocalPlayer()) if (auto* Director=LP->GetSubsystem<UMythicNameplateDirector>())
        if (Director->CloseEntityInspectForPage(this)) return;
    // A stale page may deactivate itself, but may never clear another page's viewer projection.
    if (IsActivated()) DeactivateWidget();
}
void UMythicEntityInspectPage::NativeOnDeactivated() { StopInspectInput(); Super::NativeOnDeactivated(); }
bool UMythicEntityInspectPage::NativeOnHandleBackAction() { RequestInspectClose(); return true; }
bool UMythicEntityInspectPage::RouteInspectKey(FKey Key,EInputEvent Event,bool bRepeat) {
    auto* PC=GetOwningPlayer(); auto* Controls=PC ? PC->FindComponentByClass<UMythicControlsComponent>():nullptr;
    if (!Controls) return false;
    const bool Inspect=Controls->IsControlKey(Key,EMythicControl::Inspect);
    const bool Menu=Controls->IsControlKey(Key,EMythicControl::CharacterMenu) || Controls->IsControlKey(Key,EMythicControl::SystemMenu);
    const bool Utility=Controls->IsControlKey(Key,EMythicControl::Utility);
    if (!Inspect && !Menu && !Utility) return false;
    if (Event==IE_Released) { Controls->HandleKey(Key,Event); return true; }
    if (Event!=IE_Pressed || bRepeat) return true;
    RequestInspectClose();
    // Only menu navigation is handed off. Utility closes this local state without using an item;
    // another fresh press can open its wheel. Inspect never manufactures a permanently held Alt.
    if (Menu) Controls->HandleKey(Key,Event);
    return true;
}

void UMythicEntityInspectPage::ApplyInspectProjection(
    const FMythicEntityInspectProjection &Projection) {
    if (!ViewModel) {
        ViewModel = NewObject<UMythicEntityInspectViewModel>(this);
    }
    ViewModel->Apply(Projection);
    RefreshNativeBindings();
    OnInspectProjectionChanged();
}

void UMythicEntityInspectPage::RefreshNativeBindings() {
    if (!ViewModel) {
        ResetNativeBindings();
        return;
    }

    const FMythicEntityInspectProjection &Projection =
        ViewModel->GetProjection();
    ApplyInspectOptionalText(NameText, Projection.ResolvedName);
    ApplyInspectOptionalText(RoleText, Projection.ResolvedRole);
    ApplyInspectOptionalText(FactionText, Projection.ResolvedFaction);
    ApplyInspectOptionalText(RelationshipText, Projection.ResolvedRelationship);
    ApplyInspectOptionalText(StandingText, Projection.ResolvedStanding);
    ApplyInspectOptionalText(ThreatText, BuildThreatText(Projection));
    ApplyInspectOptionalText(
        LevelText,
        Projection.bShowExactCombatLevel
            ? FText::Format(LOCTEXT("LevelFormat", "Level {0}"),
                            FText::AsNumber(Projection.ExactCombatLevel))
            : FText::GetEmpty());
    ApplyInspectOptionalText(
        TraitsText, BuildFactSection(LOCTEXT("TraitsHeading", "Traits"),
                                     Projection.Traits));
    ApplyInspectOptionalText(
        HistoryText, BuildFactSection(LOCTEXT("HistoryHeading", "History"),
                                      Projection.History));
    ApplyInspectOptionalText(
        LikesText, BuildFactSection(LOCTEXT("LikesHeading", "Likes"),
                                    Projection.Likes));
    ApplyInspectOptionalText(
        DislikesText, BuildFactSection(LOCTEXT("DislikesHeading", "Dislikes"),
                                       Projection.Dislikes));
    ApplyInspectOptionalText(ConnectionsText,
                             BuildFactSection(
                                 LOCTEXT("ConnectionsHeading", "Connections"),
                                 Projection.Connections));
}

void UMythicEntityInspectPage::ResetNativeBindings() {
    ApplyInspectOptionalText(NameText, FText::GetEmpty());
    ApplyInspectOptionalText(RoleText, FText::GetEmpty());
    ApplyInspectOptionalText(FactionText, FText::GetEmpty());
    ApplyInspectOptionalText(RelationshipText, FText::GetEmpty());
    ApplyInspectOptionalText(StandingText, FText::GetEmpty());
    ApplyInspectOptionalText(ThreatText, FText::GetEmpty());
    ApplyInspectOptionalText(LevelText, FText::GetEmpty());
    ApplyInspectOptionalText(TraitsText, FText::GetEmpty());
    ApplyInspectOptionalText(HistoryText, FText::GetEmpty());
    ApplyInspectOptionalText(LikesText, FText::GetEmpty());
    ApplyInspectOptionalText(DislikesText, FText::GetEmpty());
    ApplyInspectOptionalText(ConnectionsText, FText::GetEmpty());
}

#undef LOCTEXT_NAMESPACE
