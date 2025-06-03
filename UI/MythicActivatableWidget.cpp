// 
#include "MythicActivatableWidget.h"

#include "CommonUITypes.h"
#include "ICommonInputModule.h"
#include "Mythic.h"
#include "Editor/WidgetCompilerLog.h"
#include "Input/CommonUIInputTypes.h"

#define LOCTEXT_NAMESPACE "Mythic"

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
        UE_LOG(Mythic, Warning, TEXT("RegisterInputBinding called with invalid tag"));
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
            //TODO - Note for now, because we can't guarantee it isn't implemented in a native subclass of this one.
            CompileLog.Note(LOCTEXT("ValidateGetDesiredFocusTarget_Note",
                                    "GetDesiredFocusTarget wasn't implemented, you're going to have trouble using gamepads on this screen.  If it was implemented in the native base class you can ignore this message."));
        }
    }
}

#endif

#undef LOCTEXT_NAMESPACE
