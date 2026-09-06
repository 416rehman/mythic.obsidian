#pragma once

#include "CoreMinimal.h"
#include "UI/MythicActivatableWidget.h"
#include "CommonInputTypeEnum.h"
#include "Input/MythicControlsSettings.h"
#include "Input/MythicQuickUseComponent.h"
#include "MythicGameplayWheel.generated.h"

class UMythicControlsComponent;
class UMythicControlGlyph;
class UVerticalBox;
class UCommonActivatableWidgetContainerBase;
struct FMythicWheelOpenToken;
class UImage;
class UCommonTextBlock;
class UCanvasPanel;
class USizeBox;
class UOverlay;
class FViewport;
class FMythicModalInputProcessor;
struct FStreamableHandle;

USTRUCT()
struct FMythicWheelCell {
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<UImage> Plate;
    UPROPERTY() TObjectPtr<UImage> Icon;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Count;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Label;
};

UCLASS(meta=(DisableNativeTick))
class MYTHIC_API UMythicGameplayWheel : public UMythicActivatableWidget {
    GENERATED_BODY()
public:
    UMythicGameplayWheel(const FObjectInitializer& Initializer);
    void OpenFor(UMythicControlsComponent* Owner, const FMythicWheelOpenToken& Token);
    void CommitHighlighted();
    void CloseWithoutCommit();

protected:
    virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
    virtual void NativeOnInitialized() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual bool NativeOnHandleBackAction() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
    virtual FReply NativeOnKeyUp(const FGeometry&,const FKeyEvent&) override;
    virtual FReply NativeOnAnalogValueChanged(const FGeometry&,const FAnalogInputEvent&) override;
    virtual FReply NativeOnMouseMove(const FGeometry&,const FPointerEvent&) override;
    virtual FReply NativeOnMouseWheel(const FGeometry&,const FPointerEvent&) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry&,const FPointerEvent&) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend struct FMythicGameplayWheelDirectionTestAccess;
#endif
    void Build();
    void QueueLayout();
    void RefreshLayout();
    void HandleViewportResize(FViewport*,uint32);
    void ReleaseObservers();
    bool StartModalInput();
    TSharedPtr<FMythicModalInputProcessor> ModalProcessor;
    void RefreshCounts();
    void RefreshPage();
    void Highlight(int32 Index);
    void SelectDirection(FVector2D Direction,bool bStick);
    void ChangePage(int32 Delta);
    void DeviceChanged(ECommonInputType Type);
    int32 ItemIndex(int32 Sector) const;
    int32 PageCount() const;
    bool OwnsUser(int32 UserIndex) const;
    FReply RouteKey(FKey Key,EInputEvent Event,bool bRepeat=false);

    TWeakObjectPtr<UMythicControlsComponent> Controls;
    TWeakObjectPtr<UMythicQuickUseComponent> QuickUse;
    EMythicControl Opener = EMythicControl::Heal;
    TArray<FMythicQuickUseEntry> Entries;
    UPROPERTY() TArray<FMythicWheelCell> Cells;
    UPROPERTY() TObjectPtr<UCanvasPanel> WheelCanvas;
    UPROPERTY() TObjectPtr<USizeBox> WheelSize;
    UPROPERTY() TObjectPtr<UOverlay> SafeContent;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Title;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Detail;
    UPROPERTY() TObjectPtr<UCommonTextBlock> PageLabel;
    UPROPERTY() TObjectPtr<UVerticalBox> Hint;
    UPROPERTY() TObjectPtr<UMythicControlGlyph> OpenerGlyph;
    TArray<TSharedPtr<FStreamableHandle>> IconLoads;
    FDelegateHandle InventoryChanged;
    FDelegateHandle DeviceHandle;
    FDelegateHandle ViewportHandle;
    FTimerHandle LayoutTimer;
    FVector2D WheelCenter=FVector2D(160,174);
    float PointerNeutralRadius=48;
    FVector2D Stick = FVector2D::ZeroVector;
    FVector2D PageDirection = FVector2D::ZeroVector;
    FVector2D LastSelectionDirection = FVector2D::ZeroVector;
    int32 Page = 0;
    int32 Selected = INDEX_NONE;
    uint32 IconRevision = 0;
    bool bAwaitFreshDirection = true;
    bool bOpen = false;
    bool bCommitted = false;
    bool bOnLayer = false;
    uint64 OpenSerial=0;
    TWeakObjectPtr<UCommonActivatableWidgetContainerBase> AttachedLayer;
};
