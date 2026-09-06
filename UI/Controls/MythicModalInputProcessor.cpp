#include "UI/Controls/MythicModalInputProcessor.h"
#include "CommonActivatableWidget.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "Layout/WidgetPath.h"

FMythicModalInputProcessor::FMythicModalInputProcessor(UCommonActivatableWidget* InWidget,FRoutes InRoutes)
     :Widget(InWidget),Routes(MoveTemp(InRoutes)),OriginalLocalPlayer(InWidget ? InWidget->GetOwningLocalPlayer():nullptr) {
    if (const auto* LP=OriginalLocalPlayer.Get()) if (const auto User=LP->GetSlateUser()) OriginalSlateUserIndex=User->GetUserIndex();
}

bool FMythicModalInputProcessor::Start() {
    if (bRegistered) return true;
    if (!FSlateApplication::IsInitialized() || !HasFocus()) return false;
    auto& Slate=FSlateApplication::Get();
    // CommonInput observes device changes at PreGame. Capture precedes CommonUI's Game2 analog cursor/Back routing.
    bRegistered=Slate.RegisterInputPreProcessor(AsShared(),FInputPreprocessorRegistrationKey{EInputPreProcessorType::Game,0});
    if (!bRegistered) return false;
    FocusHandle=Slate.OnFocusChanging().AddSP(AsShared(),&FMythicModalInputProcessor::HandleFocusChanging);
    ActivationHandle=Slate.OnApplicationActivationStateChanged().AddSP(AsShared(),&FMythicModalInputProcessor::HandleActivation);
    return true;
}
void FMythicModalInputProcessor::Stop() {
    if (FSlateApplication::IsInitialized()) {
        auto& Slate=FSlateApplication::Get();
        Slate.OnFocusChanging().Remove(FocusHandle);
        Slate.OnApplicationActivationStateChanged().Remove(ActivationHandle);
        if (bRegistered) Slate.UnregisterInputPreProcessor(AsShared());
    }
    bRegistered=false; FocusHandle.Reset(); ActivationHandle.Reset(); bReleaseOnly=false; ReleaseOnlyKeys.Reset();
}
bool FMythicModalInputProcessor::OwnsUser(uint32 Index) const {
    const auto* LP=OriginalLocalPlayer.Get();
    const auto User=LP ? LP->GetSlateUser() : nullptr;
    return User && OriginalSlateUserIndex!=INDEX_NONE && User->GetUserIndex()==OriginalSlateUserIndex
        && Index==static_cast<uint32>(OriginalSlateUserIndex);
}
void FMythicModalInputProcessor::KeepOwnedReleases(TSet<FKey> Keys) {
    if (Keys.IsEmpty() || !OriginalLocalPlayer.IsValid() || !FSlateApplication::IsInitialized()) { Stop(); return; }
    auto& Slate=FSlateApplication::Get();
    Slate.OnFocusChanging().Remove(FocusHandle); FocusHandle.Reset();
    Slate.OnApplicationActivationStateChanged().Remove(ActivationHandle); ActivationHandle.Reset();
    ReleaseOnlyKeys=MoveTemp(Keys); ReleaseOnlyDeadline=FPlatformTime::Seconds()+30.0; bReleaseOnly=true;
    // Do not destroy Routes here: a capture callback can transfer ownership from inside Routes.Key.
    if (!bRegistered) bRegistered=Slate.RegisterInputPreProcessor(AsShared(),FInputPreprocessorRegistrationKey{EInputPreProcessorType::Game,0});
    if (!bRegistered) Stop();
}
bool FMythicModalInputProcessor::DrainOwnedRelease(uint32 Index,FKey Key,EInputEvent Event,bool bRepeat) {
    if (!bRegistered || !bReleaseOnly || !OwnsUser(Index) || !ReleaseOnlyKeys.Contains(Key)) return false;
    if (Event==IE_Pressed && !bRepeat) {
        // The old up was lost. This is a new physical press and must reach its current owner unchanged.
        ReleaseOnlyKeys.Remove(Key); if (ReleaseOnlyKeys.IsEmpty()) Stop(); return false;
    }
    if (Event==IE_Released) { ReleaseOnlyKeys.Remove(Key); if (ReleaseOnlyKeys.IsEmpty()) Stop(); return true; }
    return bRepeat;
}
bool FMythicModalInputProcessor::HasFocus() const {
    const auto* Owner=Widget.Get(); auto* PC=Owner ? Owner->GetOwningPlayer() : nullptr;
    return Owner && Owner->IsActivated() && PC && PC->IsLocalController()
        && (Owner->HasUserFocus(PC) || Owner->HasUserFocusedDescendants(PC));
}
bool FMythicModalInputProcessor::MayRoute(uint32 Index) {
    if (!bRegistered || bReleaseOnly || !OwnsUser(Index)) return false;
    if (!HasFocus()) { LoseFocus(); return false; }
    return true;
}
void FMythicModalInputProcessor::LoseFocus() {
    if (bLosingFocus) return;
    const auto KeepAlive=AsShared(); TGuardValue<bool> Guard(bLosingFocus,true);
    if (Routes.FocusLost) Routes.FocusLost();
    // The callback may have transferred consumed releases to the exact-user quarantine.
    if (!bReleaseOnly) Stop();
}
void FMythicModalInputProcessor::Tick(float,FSlateApplication&,TSharedRef<ICursor>) {
    const auto KeepAlive=AsShared();
    if (bReleaseOnly) {
        if (!OriginalLocalPlayer.IsValid() || !OwnsUser(static_cast<uint32>(OriginalSlateUserIndex))
            || ReleaseOnlyKeys.IsEmpty() || FPlatformTime::Seconds()>=ReleaseOnlyDeadline) Stop();
        return;
    }
    if (!Widget.IsValid()) { Stop(); return; }
    if (!HasFocus()) LoseFocus();
}
void FMythicModalInputProcessor::HandleActivation(bool bActive) { if (!bActive) LoseFocus(); }
void FMythicModalInputProcessor::HandleFocusChanging(const FFocusEvent& Event,const FWeakWidgetPath&,
    const TSharedPtr<SWidget>&,const FWidgetPath& NewPath,const TSharedPtr<SWidget>&) {
    if (!bRegistered || bReleaseOnly || !OwnsUser(Event.GetUser())) return;
    const auto* Owner=Widget.Get(); const auto Cached=Owner ? Owner->GetCachedWidget() : nullptr;
    if (!Cached || !NewPath.ContainsWidget(Cached.Get())) LoseFocus();
}
bool FMythicModalInputProcessor::HandleKeyDownEvent(FSlateApplication&,const FKeyEvent& Event) {
    const auto KeepAlive=AsShared();
    const bool CanRoute=!bReleaseOnly && MayRoute(Event.GetUserIndex());
    // MayRoute can transfer this same event to release-only ownership on focus loss.
    if (bReleaseOnly) return DrainOwnedRelease(Event.GetUserIndex(),Event.GetKey(),IE_Pressed,Event.IsRepeat());
    return CanRoute && Routes.Key && Routes.Key(Event.GetKey(),IE_Pressed,Event.IsRepeat());
}
bool FMythicModalInputProcessor::HandleKeyUpEvent(FSlateApplication&,const FKeyEvent& Event) {
    const auto KeepAlive=AsShared();
    const bool CanRoute=!bReleaseOnly && MayRoute(Event.GetUserIndex());
    // MayRoute can transfer this same event to release-only ownership on focus loss.
    if (bReleaseOnly) return DrainOwnedRelease(Event.GetUserIndex(),Event.GetKey(),IE_Released,false);
    return CanRoute && Routes.Key && Routes.Key(Event.GetKey(),IE_Released,false);
}
bool FMythicModalInputProcessor::HandleAnalogInputEvent(FSlateApplication&,const FAnalogInputEvent& Event) {
    const auto KeepAlive=AsShared();
    return MayRoute(Event.GetUserIndex()) && Routes.Analog && Routes.Analog(Event);
}
bool FMythicModalInputProcessor::HandleMouseButtonDownEvent(FSlateApplication&,const FPointerEvent& Event) {
    const auto KeepAlive=AsShared();
    const bool CanRoute=!bReleaseOnly && MayRoute(Event.GetUserIndex());
    if (bReleaseOnly) return DrainOwnedRelease(Event.GetUserIndex(),Event.GetEffectingButton(),IE_Pressed,false);
    if (!CanRoute) return false;
    return Routes.MouseButton ? Routes.MouseButton(Event,IE_Pressed) : Routes.Key && Routes.Key(Event.GetEffectingButton(),IE_Pressed,false);
}
bool FMythicModalInputProcessor::HandleMouseButtonUpEvent(FSlateApplication&,const FPointerEvent& Event) {
    const auto KeepAlive=AsShared();
    const bool CanRoute=!bReleaseOnly && MayRoute(Event.GetUserIndex());
    if (bReleaseOnly) return DrainOwnedRelease(Event.GetUserIndex(),Event.GetEffectingButton(),IE_Released,false);
    if (!CanRoute) return false;
    return Routes.MouseButton ? Routes.MouseButton(Event,IE_Released) : Routes.Key && Routes.Key(Event.GetEffectingButton(),IE_Released,false);
}
bool FMythicModalInputProcessor::HandleMouseButtonDoubleClickEvent(FSlateApplication& Slate,const FPointerEvent& Event) {
    return HandleMouseButtonDownEvent(Slate,Event);
}
bool FMythicModalInputProcessor::HandleMouseMoveEvent(FSlateApplication&,const FPointerEvent& Event) {
    const auto KeepAlive=AsShared();
    return MayRoute(Event.GetUserIndex()) && Routes.MouseMove && Routes.MouseMove(Event);
}
bool FMythicModalInputProcessor::HandleMouseWheelOrGestureEvent(FSlateApplication&,const FPointerEvent& Event,const FPointerEvent*) {
    const auto KeepAlive=AsShared();
    return MayRoute(Event.GetUserIndex()) && Routes.MouseWheel && Routes.MouseWheel(Event);
}
