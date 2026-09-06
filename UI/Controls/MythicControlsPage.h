#pragma once

#include "CoreMinimal.h"
#include "UI/MythicActivatableWidget.h"
#include "Input/MythicControlsSettings.h"
#include "MythicControlsPage.generated.h"

class UMythicControlsComponent;
class UMythicControlGlyph;
class UMythicInputGlyph;
class UMythicControlUserSettings;
class UCommonButtonBase;
class UCommonTextBlock;
class UVerticalBox;
class UHorizontalBox;
class FMythicModalInputProcessor;

DECLARE_MULTICAST_DELEGATE_OneParam(FMythicCaptureStateChanged,bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMythicControlFocusHelp,const FText&,const FText&);

USTRUCT()
struct FMythicControlRowWidgets {
    GENERATED_BODY()
    EMythicControl Control=EMythicControl::Count;
    UPROPERTY() TObjectPtr<UCommonButtonBase> Keyboard;
    UPROPERTY() TObjectPtr<UCommonButtonBase> Gamepad;
    UPROPERTY() TObjectPtr<UCommonTextBlock> KeyboardLabel;
    UPROPERTY() TObjectPtr<UCommonTextBlock> GamepadLabel;
    UPROPERTY() TObjectPtr<UMythicControlGlyph> KeyboardGlyph;
    UPROPERTY() TObjectPtr<UMythicControlGlyph> GamepadGlyph;
};

/** Embedded under the existing Settings Controls category; never a new shell page. */
UCLASS(meta=(DisableNativeTick))
class MYTHIC_API UMythicControlsPage : public UMythicActivatableWidget {
    GENERATED_BODY()
public:
    UMythicControlsPage(const FObjectInitializer& Initializer);
    void InitializeControls(UMythicControlsComponent* Owner);
    bool IsCapturing() const { return CaptureControl!=EMythicControl::Count; }
    void CancelCapture();
    UWidget* GetBindingFocus() const { return NativeGetDesiredFocusTarget(); }
    void SetRailFocus(UWidget* Rail,UWidget* FollowingSetting=nullptr);
    FMythicControlFocusHelp OnFocusHelp;
    FMythicCaptureStateChanged OnCaptureStateChanged;
    virtual bool TryHandleNestedBackAction() override;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
    virtual FReply NativeOnKeyUp(const FGeometry&,const FKeyEvent&) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry&,const FPointerEvent&) override;

private:
    void Build();
    void Refresh();
    void RefreshFooterNavigation();
    void PublishFocusHelp(UCommonButtonBase* Button);
    TWeakObjectPtr<UWidget> FollowingSetting;
    void BeginCapture(EMythicControl Control,bool bGamepad);
    bool StartCaptureInput();
    void StopCaptureInput();
    void CancelCaptureInternal(bool bRestoreFocus);
    FReply CapturePointerEvent(const FPointerEvent& Event,EInputEvent InputEvent);
    TSharedPtr<FMythicModalInputProcessor> CaptureProcessor;
    TSet<FKey> PassthroughCaptureMouse;
    void CaptureKey(FKey Key);
    void ConfirmCapture();
    void ClearCapture();
    void ResetAll();
    void BindSaveState();
    void RefreshSaveState();
    void HandleSaveState(bool bSaving,bool bSuccess);
    void RetrySave();
    FReply CaptureEvent(FKey Key,EInputEvent Event,bool bRepeat=false);
    bool OwnsUser(int32 Index) const;
    UCommonButtonBase* AddButton(UHorizontalBox* Host,const FText& Text,UCommonTextBlock*& Label);
    UCommonButtonBase* AddBindingButton(UHorizontalBox* Host,EMythicControl Control,bool bGamepad,UCommonTextBlock*& Label,UMythicControlGlyph*& Glyph);
    UCommonButtonBase* FindRowButton(EMythicControl Control,bool bGamepad) const;
    TWeakObjectPtr<UMythicControlsComponent> Controls;
    FDelegateHandle Changed;
    TWeakObjectPtr<UMythicControlUserSettings> ControlUserSettings;
    FDelegateHandle SaveStateChanged;
    bool bObservedSaveSuccess=false;
    UPROPERTY() TArray<FMythicControlRowWidgets> Rows;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Status;
    UPROPERTY() TObjectPtr<UMythicInputGlyph> CandidateGlyph;
    UPROPERTY() TObjectPtr<UCommonTextBlock> SaveStatus;
    UPROPERTY() TObjectPtr<UCommonButtonBase> Retry;
    UPROPERTY() TObjectPtr<UCommonButtonBase> Confirm;
    UPROPERTY() TObjectPtr<UCommonTextBlock> ConfirmLabel;
    UPROPERTY() TObjectPtr<UCommonButtonBase> Cancel;
    UPROPERTY() TObjectPtr<UCommonButtonBase> Clear;
    UPROPERTY() TObjectPtr<UCommonButtonBase> Reset;
    EMythicControl CaptureControl=EMythicControl::Count;
    EMythicControl Conflict=EMythicControl::Count;
    FKey Candidate;
    TSet<FKey> IgnoreUntilRelease;
    // Every consumed down retains its repeat/up until review can safely accept a fresh press.
    TSet<FKey> CaptureConsumedDowns;
    bool IsCaptureReviewReady() const { return !bCancelCaptureQueued && Candidate.IsValid() && bCandidateReleased && CaptureConsumedDowns.IsEmpty(); }
    bool bCaptureGamepad=false;
    bool bCandidateReleased=false;
    bool bCancelCaptureQueued=false;
};
