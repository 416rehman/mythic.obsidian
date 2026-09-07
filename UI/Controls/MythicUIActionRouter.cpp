#include "UI/Controls/MythicUIActionRouter.h"
#include "Input/CommonAnalogCursor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"

namespace {
class FMythicAnalogCursor final : public FCommonAnalogCursor {
public:
    explicit FMythicAnalogCursor(const UCommonUIActionRouterBase& Router)
        : FCommonAnalogCursor(Router) {}

    virtual bool HandleKeyDownEvent(FSlateApplication& Slate, const FKeyEvent& Event) override {
        const auto KeepAlive = AsShared();
        if (bDeinitializing) return false;
        if (IsAccept(Event) && !Event.IsRepeat() && FCommonAnalogCursor::IsRelevantInput(Event)) {
            // A new non-repeat edge retires an earlier edge whose physical up was lost.
            const uint64 BeforeRelease = NextSerial;
            if (MousePress.IsSet()) ReleaseSyntheticMouse(Slate, Event, MousePress->Serial);
            if (bDeinitializing || NextSerial != BeforeRelease) return true;
            Presses.RemoveAll([&](const FPress& P) { return SamePress(P.Event, Event); });
            Presses.Add(FPress{Event, ActionRouter.GetActiveInputMode() != ECommonInputMode::Game, ++NextSerial});
        }
        return FCommonAnalogCursor::HandleKeyDownEvent(Slate, Event);
    }

    virtual bool HandleKeyUpEvent(FSlateApplication& Slate, const FKeyEvent& Event) override {
        const auto KeepAlive = AsShared();
        const FPress* Press = Presses.FindByPredicate([&](const FPress& P) { return SamePress(P.Event, Event); });
        const uint64 Serial = Press ? Press->Serial : 0;
        // Base clears its private virtual-pointer state and runs configured UI bindings.
        // Our policy below prevents base from emitting an up. A canceled UI hold may
        // legitimately create the down inside this call; the same owned release pairs it.
        const bool bHandled = FCommonAnalogCursor::HandleKeyUpEvent(Slate, Event);
        const bool bReleased = Serial != 0 && ReleaseSyntheticMouse(Slate, Event, Serial);
        Presses.RemoveAll([&](const FPress& P) { return P.Serial == Serial; });
        return bHandled || bReleased;
    }

    virtual void Deinitialize() override {
        bDeinitializing = true;
        Presses.Reset();
        if (MousePress.IsSet() && FSlateApplication::IsInitialized()) {
            const FKeyEvent Event = MousePress->Event;
            ReleaseSyntheticMouse(FSlateApplication::Get(), Event, MousePress->Serial);
        }
        MousePress.Reset();
        FCommonAnalogCursor::Deinitialize();
    }

    virtual bool ShouldVirtualAcceptSimulateMouseButton(
        const FKeyEvent& Event, EInputEvent InputEvent) const override {
        if (bDeinitializing || !IsAccept(Event)) return false;
        // This cursor alone retires a synthetic mouse lease after the base up handler.
        if (InputEvent == IE_Released || Event.IsRepeat()) return false;
        const FPress* Press = Presses.FindByPredicate([&](const FPress& P) { return SamePress(P.Event, Event); });
        if (!Press || !Press->bMenuOrigin || MousePress.IsSet()
            || !FCommonAnalogCursor::IsRelevantInput(Event)
            || !FCommonAnalogCursor::ShouldVirtualAcceptSimulateMouseButton(Event, InputEvent)) return false;
        const auto User = FSlateApplication::Get().GetUser(Event);
        if (!User) return false;
        // Admission immediately precedes the engine's offscreen or FAnalogCursor
        // down dispatch. Neither handled=false nor the global mouse set proves absence.
        MousePress.Emplace(FMousePress{Press->Event, User, Press->Serial});
        return true;
    }

protected:
    virtual bool IsRelevantInput(const FKeyEvent& Event) const override {
        return (ForcedRelease.IsSet() && SamePress(ForcedRelease.GetValue(), Event))
            || FCommonAnalogCursor::IsRelevantInput(Event);
    }

private:
    struct FPress {
        FKeyEvent Event;
        bool bMenuOrigin;
        uint64 Serial;
    };
    struct FMousePress {
        FKeyEvent Event;
        TWeakPtr<FSlateUser> User;
        uint64 Serial;
    };
    static bool IsAccept(const FKeyEvent& Event) {
        return Event.GetKey() == EKeys::Virtual_Gamepad_Accept.GetVirtualKey();
    }
    static bool SamePress(const FKeyEvent& A, const FKeyEvent& B) {
        return A.GetKey() == B.GetKey() && A.GetUserIndex() == B.GetUserIndex()
            && A.GetInputDeviceId() == B.GetInputDeviceId();
    }
    bool ReleaseSyntheticMouse(FSlateApplication& Slate, const FKeyEvent& Event, uint64 Serial) {
        if (!MousePress.IsSet() || MousePress->Serial != Serial || !SamePress(MousePress->Event, Event)) return false;
        const FMousePress Owned = MousePress.GetValue();
        MousePress.Reset(); // Retire before any synchronous UI callback can reenter.
        if (!Owned.User.IsValid() || Slate.GetUser(Owned.Event) != Owned.User.Pin()) return false;
        TGuardValue<TOptional<FKeyEvent>> ReleaseGuard(ForcedRelease, TOptional<FKeyEvent>(Owned.Event));
        // FAnalogCursor reproduces the engine's exact user/pointer/modifier fields.
        // Its ProcessMouseButtonUpEvent clears Slate's button state even if UI consumes it.
        // The scoped exact-event relevance override permits this after menu/focus changes.
        FAnalogCursor::HandleKeyUpEvent(Slate, Owned.Event);
        return true;
    }

    TArray<FPress> Presses;
    mutable TOptional<FMousePress> MousePress;
    TOptional<FKeyEvent> ForcedRelease;
    bool bDeinitializing = false;
    uint64 NextSerial = 0;
};
}

TSharedRef<FCommonAnalogCursor> UMythicUIActionRouter::MakeAnalogCursor() const {
    return FCommonAnalogCursor::CreateAnalogCursor<FMythicAnalogCursor>(*this);
}
