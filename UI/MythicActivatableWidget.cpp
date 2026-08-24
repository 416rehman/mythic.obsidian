
#include "MythicActivatableWidget.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

#include "CommonUITypes.h"
#include "ICommonInputModule.h"
#include "Mythic.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "UI/MythicUIStyle.h"
#include "Editor/WidgetCompilerLog.h"
#include "Input/CommonUIInputTypes.h"

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
    for (FUIActionBindingHandle Handle : BindingHandles) {
        if (Handle.IsValid()) {
            Handle.Unregister();
        }
    }
    BindingHandles.Empty();

    Super::NativeDestruct();
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
    if (UIInputContext.IsNull()) {
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
    if (UIInputContext.IsNull()) {
        return;
    }
    if (const ULocalPlayer *LP = GetOwningLocalPlayer()) {
        if (UEnhancedInputLocalPlayerSubsystem *Sub =
                LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
            if (UInputMappingContext *Context = UIInputContext.Get()) {
                Sub->RemoveMappingContext(Context);
            }
        }
    }
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
