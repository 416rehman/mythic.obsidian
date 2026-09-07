#pragma once
#include "CoreMinimal.h"
#include "UI/MythicActivatableWidget.h"
#include "Character/Appearance/MythicAppearanceCatalog.h"
#include "MythicAppearanceWidget.generated.h"

class UMythicAppearanceComponent;
class UMythicAppearancePreview;
class UMythicBoundActionButton;
class UMythicCharacterPageWidget;
class UCommonTextBlock;
class UImage;
class UVerticalBox;
class UPanelWidget;
class UScrollBox;
class UCommonActivatableWidgetContainerBase;
class FMythicModalInputProcessor;

/** CommonUI transaction editor. Browsing never changes the live pawn or canonical inventory. */
UCLASS()
class MYTHIC_API UMythicAppearanceWidget : public UMythicActivatableWidget {
    GENERATED_BODY()
public:
    UMythicAppearanceWidget(const FObjectInitializer& Initializer);
    void Open(UMythicCharacterPageWidget* Page,UWidget* ReturnFocus);
    UFUNCTION(BlueprintCallable) void Close();
    void ForceClose();
    bool IsOnLayer() const { return bOnLayer; }
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;
    virtual bool NativeOnHandleBackAction() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
    virtual FReply NativeOnAnalogValueChanged(const FGeometry& Geometry,const FAnalogInputEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& Event) override;
    virtual void NativeOnAddedToFocusPath(const FFocusEvent& Event) override;
private:
    struct FChoiceRow {
        FText Label;
        TArray<FName> Values;
        TArray<FText> Names;
        TFunction<FName()> Read;
        TFunction<void(FName)> Write;
        TWeakObjectPtr<UMythicBoundActionButton> Button;
        TWeakObjectPtr<UMythicBoundActionButton> Previous;
    };
    UMythicBoundActionButton* Button(UPanelWidget* Host,const FText& Label,TFunction<void()> Action,bool bPrimary=false);
    void Build();
    void BuildChoices();
    void AddChoice(FChoiceRow Row);
    void Cycle(int32 Row,int32 Direction);
    void RefreshChoices();
    void RefreshPreview();
    void ReadAccepted();
    bool IsDirty() const;
    bool IsConflict() const;
    void SetTab(bool bGear);
    void Apply(bool bCloseAfter=false);
    void RetrySave();
    void SetNotice(const FText& Text);
    void SetExitPrompt(bool bShow);
    void StopLocalWork();
    void ResetPreviewAxes();
    void StopPreviewInput();
    TSharedPtr<FMythicModalInputProcessor> PreviewInput;
    TSet<FKey> AxesAwaitingNeutral;
    FDelegateHandle DeviceHandle,InputActivationHandle,DeviceConnectionHandle;
    UFUNCTION() void StateChanged();
    UFUNCTION() void Result(const FMythicAppearanceReceipt& Receipt);
    TWeakObjectPtr<UMythicAppearanceComponent> Appearance;
    TWeakObjectPtr<UMythicCharacterPageWidget> Page;
    TWeakObjectPtr<UWidget> ReturnFocus;
    TWeakObjectPtr<UWidget> LastFocus;
    TWeakObjectPtr<UCommonActivatableWidgetContainerBase> AttachedLayer;
    UPROPERTY(Transient) TObjectPtr<UMythicAppearanceCatalog> Catalog;
    UPROPERTY(Transient) TObjectPtr<UMythicAppearancePreview> Preview;
    UPROPERTY(Transient) TObjectPtr<UImage> PreviewImage;
    UPROPERTY(Transient) TObjectPtr<UCommonTextBlock> Notice;
    UPROPERTY(Transient) TObjectPtr<UCommonTextBlock> PreviewStatus;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> ChoiceScroll;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Choices;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> ExitPrompt;
    UPROPERTY(Transient) TObjectPtr<UMythicBoundActionButton> BodyTab;
    UPROPERTY(Transient) TObjectPtr<UMythicBoundActionButton> GearTab;
    UPROPERTY(Transient) TObjectPtr<UMythicBoundActionButton> ApplyButton;
    UPROPERTY(Transient) TObjectPtr<UMythicBoundActionButton> RetryButton;
    UPROPERTY(Transient) TObjectPtr<UMythicBoundActionButton> ReloadButton;
    UPROPERTY(Transient) TObjectPtr<UMythicBoundActionButton> CancelButton;
    UPROPERTY(Transient) TObjectPtr<UMythicBoundActionButton> FirstViewControl;
    UPROPERTY(Transient) TArray<TObjectPtr<UPanelWidget>> EditingPanels;
    FUIActionBindingHandle PreviousTabBinding,NextTabBinding;
    FMythicAppearanceOwnerState Baseline;
    FMythicAppearanceRecipe Draft;
    TArray<FMythicAppearanceEquippedItem> Equipment;
    TMap<FGuid,FMythicItemAppearanceData> DraftItems;
    TMap<FGuid,int32> SubmittedItemRevisions;
    TArray<FChoiceRow> Rows;
    FGuid Request;
    FTimerHandle ResponseDeadline;
    int32 AppliedRevision=INDEX_NONE;
    bool bOnLayer=false,bGearTab=false,bAwaiting=false,bCloseAfter=false,bDragging=false,bUncertain=false,bCommitReconciled=false;
    bool bPreviewResolved=false,bPreviewWasReady=false;
    float OrbitAxis=0,ZoomAxis=0,LeftZoom=0,RightZoom=0;
};
