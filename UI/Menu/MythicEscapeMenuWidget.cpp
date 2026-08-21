// Copyright Stellar Games. All Rights Reserved.

#include "MythicEscapeMenuWidget.h"

#include "CommonTextBlock.h"
#include "CommonUIExtensions.h"
#include "CommonButtonBase.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PrimaryGameLayout.h"
#include "UI/MythicHUDLayout.h"
#include "UI/MythicUIStyle.h"
#include "UObject/UnrealType.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace {
const TCHAR *QuitPromptClassPath = TEXT("/Game/Mythic/UI/Widgets/WBP_ConfirmModal.WBP_ConfirmModal_C");
const FName QuitPromptLayer(TEXT("UI.Layer.Modal"));
const FName QuitPromptSetText(TEXT("SetModalText"));
const FName QuitPromptOnConfirm(TEXT("OnConfirm"));
const FName SettingsPageId(TEXT("Settings"));
const FName GameLayerTag(TEXT("UI.Layer.Game"));
}

void UMythicEscapeClickProxy::HandleClicked() {
    UMythicEscapeMenuWidget *Owner = Menu.Get();
    if (!Owner) {
        return;
    }
    if (Action == EMythicEscapeAction::Quit && GetOuter() != Owner) {
        Owner->QuitNow();
        return;
    }
    Owner->RunAction(Action);
}

void UMythicEscapeMenuWidget::NativeConstruct() {
    if (!bBuilt && ButtonList) {
        bBuilt = true;

        if (Txt_Title) {
            Txt_Title->SetText(NSLOCTEXT("Mythic", "Paused", "Paused"));
        }

        AddEntry(EMythicEscapeAction::Resume, NSLOCTEXT("Mythic", "EscResume", "Resume"));
        if (SettingsScreenClass) {
            AddEntry(EMythicEscapeAction::Settings, NSLOCTEXT("Mythic", "EscSettings", "Settings"));
        }
        AddEntry(EMythicEscapeAction::Quit, NSLOCTEXT("Mythic", "EscQuit", "Quit to Desktop"));

        OnActivated().AddWeakLambda(this, [this]() { FocusFirstRow(); });
    }

    Super::NativeConstruct();

    if (IsActivated()) {
        FocusFirstRow();
    }
}

void UMythicEscapeMenuWidget::FocusFirstRow() {
    if (UWidget *Target = NativeGetDesiredFocusTarget()) {
        Target->SetFocus();
    }
}

UWidget *UMythicEscapeMenuWidget::NativeGetDesiredFocusTarget() const {
    for (const FMythicEscapeEntry &Entry : Entries) {
        if (Entry.Button && Entry.Button->GetVisibility() != ESlateVisibility::Collapsed) {
            return Entry.Button;
        }
    }
    return Super::NativeGetDesiredFocusTarget();
}

void UMythicEscapeMenuWidget::AddEntry(EMythicEscapeAction Action, const FText &Label) {
    FMythicEscapeEntry Entry;
    UCommonTextBlock *OutLabel = nullptr;
    Entry.Button = FMythicUIStyle::MakeButton(this, EMythicTextRole::Heading, OutLabel);
    Entry.Label = OutLabel;
    if (Entry.Label) {
        Entry.Label->SetText(Label);
        Entry.Label->SetJustification(ETextJustify::Center);
    }

    Entry.Proxy = NewObject<UMythicEscapeClickProxy>(this);
    Entry.Proxy->Menu = this;
    Entry.Proxy->Action = Action;
    FMythicUIStyle::BindButtonClicked(Entry.Button, Entry.Proxy,
                                      GET_FUNCTION_NAME_CHECKED(UMythicEscapeClickProxy, HandleClicked));

    if (UPanelSlot *Added = ButtonList->AddChild(Entry.Button)) {
        if (UVerticalBoxSlot *VSlot = Cast<UVerticalBoxSlot>(Added)) {
            VSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 6.0f));
            VSlot->SetHorizontalAlignment(HAlign_Fill);
        }
    }

    Entries.Add(Entry);
}

void UMythicEscapeMenuWidget::RunAction(EMythicEscapeAction Action) {
    switch (Action) {
        case EMythicEscapeAction::Resume:
            DeactivateWidget();
            break;

        case EMythicEscapeAction::Settings: {
            UMythicHUDLayout *Layout = nullptr;
            if (UPrimaryGameLayout *Root = UPrimaryGameLayout::GetPrimaryGameLayout(GetOwningPlayer())) {
                if (UCommonActivatableWidgetContainerBase *GameLayer =
                        Root->GetLayerWidget(FGameplayTag::RequestGameplayTag(GameLayerTag, false))) {
                    Layout = Cast<UMythicHUDLayout>(GameLayer->GetActiveWidget());
                }
            }
            if (Layout) {
                DeactivateWidget();
                Layout->OpenMenuOnPage(SettingsPageId);
            }
            else if (SettingsScreenClass && SettingsLayerTag.IsValid()) {
                UCommonUIExtensions::PushContentToLayer_ForPlayer(GetOwningLocalPlayer(), SettingsLayerTag,
                                                                  SettingsScreenClass);
            }
            break;
        }

        case EMythicEscapeAction::Quit:
            OpenQuitPrompt();
            break;
    }
}

void UMythicEscapeMenuWidget::OpenQuitPrompt() {
    UClass *PromptClass = QuitPromptClass ? QuitPromptClass.Get()
                                          : LoadClass<UCommonActivatableWidget>(nullptr, QuitPromptClassPath);
    const FGameplayTag Layer = QuitPromptLayerTag.IsValid() ? QuitPromptLayerTag
                                                            : FGameplayTag::RequestGameplayTag(QuitPromptLayer, false);
    if (!PromptClass || !Layer.IsValid()) {
        QuitNow();
        return;
    }

    UCommonActivatableWidget *Prompt =
        UCommonUIExtensions::PushContentToLayer_ForPlayer(GetOwningLocalPlayer(), Layer, PromptClass);
    if (!Prompt) {
        QuitNow();
        return;
    }

    if (UFunction *SetText = Prompt->FindFunction(QuitPromptSetText)) {
        const FText Values[] = {
            NSLOCTEXT("Mythic", "QuitPromptTitle", "Quit to Desktop?"),
            NSLOCTEXT("Mythic", "QuitPromptBody", "Anything since your last save will be lost."),
            NSLOCTEXT("Mythic", "QuitPromptYes", "Quit"),
            NSLOCTEXT("Mythic", "QuitPromptNo", "Stay"),
        };
        uint8 *Params = static_cast<uint8 *>(FMemory_Alloca(SetText->ParmsSize));
        FMemory::Memzero(Params, SetText->ParmsSize);
        int32 Index = 0;
        for (TFieldIterator<FProperty> It(SetText); It && (It->PropertyFlags & CPF_Parm); ++It) {
            if (FTextProperty *TextProp = CastField<FTextProperty>(*It)) {
                if (Index < UE_ARRAY_COUNT(Values)) {
                    TextProp->InitializeValue_InContainer(Params);
                    TextProp->SetPropertyValue_InContainer(Params, Values[Index++]);
                }
            }
        }
        Prompt->ProcessEvent(SetText, Params);
        for (TFieldIterator<FProperty> It(SetText); It && (It->PropertyFlags & CPF_Parm); ++It) {
            It->DestroyValue_InContainer(Params);
        }
    }

    if (FMulticastDelegateProperty *OnConfirm =
            CastField<FMulticastDelegateProperty>(Prompt->GetClass()->FindPropertyByName(QuitPromptOnConfirm))) {
        UMythicEscapeClickProxy *Proxy = NewObject<UMythicEscapeClickProxy>(Prompt);
        Proxy->Menu = this;
        Proxy->Action = EMythicEscapeAction::Quit;
        FScriptDelegate Yes;
        Yes.BindUFunction(Proxy, GET_FUNCTION_NAME_CHECKED(UMythicEscapeClickProxy, HandleClicked));
        OnConfirm->AddDelegate(MoveTemp(Yes), Prompt);
    }
}

void UMythicEscapeMenuWidget::QuitNow() {
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
