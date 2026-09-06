
#include "MythicActivatableWidget.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Input/MythicControlsComponent.h"
#include "Player/MythicPlayerController.h"

#include "CommonUITypes.h"
#include "ICommonInputModule.h"
#include "Mythic.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "UI/MythicUIStyle.h"
#include "Editor/WidgetCompilerLog.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Input/UIActionBinding.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateUser.h"

#define LOCTEXT_NAMESPACE "Mythic"

void UMythicActivatableWidget::NativeConstruct() {
    FMythicUIStyle::WireFocusRings(this);
    Super::NativeConstruct();

    if (IsActivated() && Cast<UCommonActivatableWidgetSwitcher>(GetParent())) {
        if (UWidget *First = FMythicUIStyle::FindFirstFocusable(this)) {
            First->SetFocus();
        }
    }
}

void UMythicActivatableWidget::NativeOnAddedToFocusPath(const FFocusEvent &InFocusEvent) {
    Super::NativeOnAddedToFocusPath(InFocusEvent);
    if (UWidget *Ring = GetWidgetFromName(TEXT("FocusRing"))) {
        Ring->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void UMythicActivatableWidget::NativeOnRemovedFromFocusPath(const FFocusEvent &InFocusEvent) {
    Super::NativeOnRemovedFromFocusPath(InFocusEvent);
    if (UWidget *Ring = GetWidgetFromName(TEXT("FocusRing"))) {
        Ring->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMythicActivatableWidget::NativeDestruct() {
    MenuDismissBinding.Unregister(); MenuDismissBinding=FUIActionBindingHandle(); MenuDismissAction={};
    RemoveUIInputContext();
    for (FUIActionBindingHandle Handle : BindingHandles) {
        if (Handle.IsValid()) {
            Handle.Unregister();
        }
    }
    BindingHandles.Empty();

    Super::NativeDestruct();
}

void UMythicActivatableWidget::ConfigureMenuDismissAction(FGameplayTag Action) {
    if (Action==MenuDismissAction && MenuDismissBinding.IsValid()) return;
    MenuDismissBinding.Unregister(); MenuDismissBinding=FUIActionBindingHandle(); MenuDismissAction=Action;
    if (!Action.IsValid()) return;
    const auto Callback=FSimpleDelegate::CreateWeakLambda(this,[this](){ if (IsActivated()) NativeOnHandleBackAction(); });
    const auto* PC=GetOwningPlayer<AMythicPlayerController>();
    const auto* Controls=PC ? PC->ControlsComponent.Get():nullptr;
    const UInputAction* Enhanced=Controls && Action==FGameplayTag::RequestGameplayTag(TEXT("UI.Action.Menu"),false)
        ? Controls->GetInputAction(EMythicControl::CharacterMenu):nullptr;
    FBindUIActionArgs Args=Enhanced ? FBindUIActionArgs(Enhanced,false,Callback) : FBindUIActionArgs(FUIActionTag::ConvertChecked(Action),false,Callback);
    Args.InputMode=ECommonInputMode::Menu;
    MenuDismissBinding=RegisterUIActionBinding(Args);
}

FReply UMythicActivatableWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) {
    auto* LP=GetOwningLocalPlayer(); const auto User=LP ? LP->GetSlateUser():nullptr;
    const auto Binding=FUIActionBinding::FindBinding(MenuDismissBinding);
    const auto* PC=GetOwningPlayer<AMythicPlayerController>();
    const bool bControlsDismiss=PC && PC->ControlsComponent && MenuDismissAction==FGameplayTag::RequestGameplayTag(TEXT("UI.Action.Menu"),false)
        && PC->ControlsComponent->IsControlKey(Event.GetKey(),EMythicControl::CharacterMenu);
    if (IsActivated() && User && Binding && Binding->BoundWidget.Get()==this
        && Event.GetUserIndex()==User->GetUserIndex() && (bControlsDismiss || Binding->NormalMappings.ContainsByPredicate([&](const auto& Mapping){ return Mapping.Key.IsSameResolvedKey(Event.GetKey()); }))) {
        if (auto* Router=LP->GetSubsystem<UCommonUIActionRouterBase>(); Router && Router->IsWidgetInActiveRoot(this)
            && Router->GatherActiveBindings().Contains(MenuDismissBinding)) {
            if (Event.IsRepeat() || Router->ProcessInput(Event.GetKey(),IE_Pressed)==ERouteUIInputResult::Handled) return FReply::Handled();
        }
    }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}

void UMythicActivatableWidget::RegisterInputBinding(FGameplayTag InputTag, EInputEvent InputType, const FInputActionExecutedDelegate &Callback, bool ShowInActionBar,
                                                    FInputActionBindingHandle &BindingHandle) {
    if (!InputTag.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("RegisterInputBinding called with invalid tag"));
        return;
    }
    auto arg = FBindUIActionArgs(FUIActionTag::ConvertChecked(InputTag), ShowInActionBar, FSimpleDelegate::CreateLambda(
                                                                         [&, Callback]() {
                                                                             auto bDone = Callback.ExecuteIfBound();
                                                                         }));
    arg.KeyEvent = InputType;
    BindingHandle.Handle = RegisterUIActionBinding(arg);
    BindingHandles.Add(BindingHandle.Handle);
}

void UMythicActivatableWidget::RegisterInputActionBinding(UInputAction *InputAction, EInputEvent InputType,
                                                          const FInputActionExecutedDelegate &Callback,
                                                          bool ShowInActionBar,
                                                          FInputActionBindingHandle &BindingHandle) {
    if (!InputAction) {
        UE_LOG(Myth, Warning, TEXT("RegisterInputActionBinding called with no input action"));
        return;
    }
    FBindUIActionArgs Args(InputAction, ShowInActionBar, FSimpleDelegate::CreateLambda(
                               [Callback]() { Callback.ExecuteIfBound(); }));
    Args.KeyEvent = InputType;
    BindingHandle.Handle = RegisterUIActionBinding(Args);
    BindingHandles.Add(BindingHandle.Handle);
}

void UMythicActivatableWidget::AddUIInputContext() {
    if (AcquiredInputContext || UIInputContext.IsNull()) {
        return;
    }
    if (const ULocalPlayer *LP = GetOwningLocalPlayer()) {
        if (UEnhancedInputLocalPlayerSubsystem *Sub =
                LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
            if (UInputMappingContext *Context = UIInputContext.LoadSynchronous()) {
                /**
                 * Forced immediately, because Enhanced Input defers its control-mapping rebuild.
                 *
                 * Without this the context is registered but QueryKeysMappedToAction still answers zero
                 * for the rest of the frame - and the action bar, which filters out any binding it cannot
                 * map to a key on the current device, drops every prompt before the rebuild ever lands.
                 */
                FModifyContextOptions Options;
                Options.bForceImmediately = true;
                AcquiredInputSubsystem = Sub;
                AcquiredInputContext = Context;
                Sub->AddMappingContext(Context, UIInputContextPriority, Options);
                UE_LOG(Myth, Log, TEXT("UIInputContext '%s' added at priority %d; %d mappings"),
                       *Context->GetName(), UIInputContextPriority, Context->GetMappings().Num());
            }
            else {
                UE_LOG(Myth, Warning, TEXT("UIInputContext failed to load"));
            }
        }
    }
}

void UMythicActivatableWidget::RemoveUIInputContext() {
    auto* Context = AcquiredInputContext.Get();
    auto* Sub = AcquiredInputSubsystem.Get();
    AcquiredInputContext = nullptr;
    AcquiredInputSubsystem.Reset();
    if (Sub && Context) Sub->RemoveMappingContext(Context);
}

void UMythicActivatableWidget::UnregisterInputBinding(FInputActionBindingHandle BindingHandle) {
    if (BindingHandle.Handle.IsValid()) {
        BindingHandle.Handle.Unregister();
        BindingHandles.Remove(BindingHandle.Handle);
    }
}

void UMythicActivatableWidget::UnregisterAllBindings() {
    for (FUIActionBindingHandle Handle : BindingHandles) {
        Handle.Unregister();
    }
    BindingHandles.Empty();
}


TOptional<FUIInputConfig> UMythicActivatableWidget::GetDesiredInputConfig() const {
    switch (InputConfig) {
    case EMythicWidgetInputMode::GameAndMenu:
        return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode);
    case EMythicWidgetInputMode::Game:
        return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode);
    case EMythicWidgetInputMode::Menu:
        return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
    case EMythicWidgetInputMode::Default:
    default:
        return TOptional<FUIInputConfig>();
    }
}

#if WITH_EDITOR

void UMythicActivatableWidget::ValidateCompiledWidgetTree(const UWidgetTree &BlueprintWidgetTree, class IWidgetCompilerLog &CompileLog) const {
    Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);

    if (!GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UMythicActivatableWidget, BP_GetDesiredFocusTarget))) {
        if (GetParentNativeClass(GetClass()) == UMythicActivatableWidget::StaticClass()) {
            CompileLog.Warning(LOCTEXT("ValidateGetDesiredFocusTarget_Warning",
                                       "GetDesiredFocusTarget wasn't implemented, you're going to have trouble using gamepads on this screen."));
        }
        else {
            CompileLog.Note(LOCTEXT("ValidateGetDesiredFocusTarget_Note",
                                    "GetDesiredFocusTarget wasn't implemented, you're going to have trouble using gamepads on this screen.  If it was implemented in the native base class you can ignore this message."));
        }
    }
}

#endif

#undef LOCTEXT_NAMESPACE
