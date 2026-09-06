#pragma once
#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h"

class UCommonActivatableWidget;
class ULocalPlayer;
class FWidgetPath;
class FWeakWidgetPath;
class SWidget;
struct FFocusEvent;

/** Temporary input lease for a focused, exact-local-player modal. It never synthesizes an input event. */
class FMythicModalInputProcessor final : public IInputProcessor, public TSharedFromThis<FMythicModalInputProcessor> {
public:
    struct FRoutes {
        TFunction<bool(FKey,EInputEvent,bool)> Key;
        TFunction<bool(const FPointerEvent&,EInputEvent)> MouseButton;
        TFunction<bool(const FAnalogInputEvent&)> Analog;
        TFunction<bool(const FPointerEvent&)> MouseMove;
        TFunction<bool(const FPointerEvent&)> MouseWheel;
        TFunction<void()> FocusLost;
    };
    FMythicModalInputProcessor(UCommonActivatableWidget* InWidget,FRoutes InRoutes);
    bool Start();
    void Stop();
    /** Transfers only consumed press tails past teardown. No fresh down, axis, pointer or gameplay is routed.
     * Same original LocalPlayer/Slate user only; drains on up, retires on a fresh down, expires after 30s. */
    void KeepOwnedReleases(TSet<FKey> Keys);
    virtual void Tick(float,FSlateApplication&,TSharedRef<ICursor>) override;
    virtual bool HandleKeyDownEvent(FSlateApplication&,const FKeyEvent&) override;
    virtual bool HandleKeyUpEvent(FSlateApplication&,const FKeyEvent&) override;
    virtual bool HandleAnalogInputEvent(FSlateApplication&,const FAnalogInputEvent&) override;
    virtual bool HandleMouseButtonDownEvent(FSlateApplication&,const FPointerEvent&) override;
    virtual bool HandleMouseButtonUpEvent(FSlateApplication&,const FPointerEvent&) override;
    virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication&,const FPointerEvent&) override;
    virtual bool HandleMouseMoveEvent(FSlateApplication&,const FPointerEvent&) override;
    virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication&,const FPointerEvent&,const FPointerEvent*) override;
    virtual const TCHAR* GetDebugName() const override { return TEXT("MythicFocusedModalInput"); }
private:
    bool OwnsUser(uint32 UserIndex) const;
    bool DrainOwnedRelease(uint32 UserIndex,FKey Key,EInputEvent Event,bool bRepeat);
    bool HasFocus() const;
    bool MayRoute(uint32 UserIndex);
    void LoseFocus();
    void HandleActivation(bool bActive);
    void HandleFocusChanging(const FFocusEvent&,const FWeakWidgetPath&,const TSharedPtr<SWidget>&,
        const FWidgetPath&,const TSharedPtr<SWidget>&);
    TWeakObjectPtr<UCommonActivatableWidget> Widget;
    FRoutes Routes;
    TWeakObjectPtr<ULocalPlayer> OriginalLocalPlayer;
    TSet<FKey> ReleaseOnlyKeys;
    int32 OriginalSlateUserIndex=INDEX_NONE;
    double ReleaseOnlyDeadline=0;
    bool bReleaseOnly=false;
    FDelegateHandle FocusHandle,ActivationHandle;
    bool bRegistered=false;
    bool bLosingFocus=false;
};
