#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonInputTypeEnum.h"
#include "Input/MythicControlsSettings.h"
#include "MythicControlGlyph.generated.h"

class UMythicControlsComponent;
class UMythicInputGlyph;
class UCommonTextBlock;

UCLASS(meta=(DisableNativeTick))
class MYTHIC_API UMythicControlGlyph : public UUserWidget {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void SetControl(EMythicControl InControl);
    void SetDisplayInputType(ECommonInputType Type);
    void ClearDisplayInputType();
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float GlyphHeight=22;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    void Refresh();
    void DeviceChanged(ECommonInputType);
    EMythicControl Control=EMythicControl::Count;
    TOptional<ECommonInputType> DisplayInputType;
    TWeakObjectPtr<UMythicControlsComponent> Controls;
    FDelegateHandle Changed;
    FDelegateHandle DeviceHandle;
    UPROPERTY() TObjectPtr<UMythicInputGlyph> Modifier;
    UPROPERTY() TObjectPtr<UMythicInputGlyph> Button;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Plus;
    UPROPERTY() TObjectPtr<UCommonTextBlock> Unbound;
};
