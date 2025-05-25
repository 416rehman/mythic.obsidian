// 

#pragma once
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "MythicActivatableWidget.generated.h"

struct FUIInputConfig;

DECLARE_DYNAMIC_DELEGATE(FInputActionExecutedDelegate);

USTRUCT(BlueprintType)
struct FInputActionBindingHandle
{
	GENERATED_BODY()
	
public:
	FUIActionBindingHandle Handle;
};

UENUM(BlueprintType)
enum class EMythicWidgetInputMode : uint8
{
    Default,
    GameAndMenu,
    Game,
    Menu
};

/**
 * Extends UCommonActivatableWidget with Blueprint-visible functions for registering additional input action bindings.
 */
UCLASS(Blueprintable)
class MYTHIC_API UMythicActivatableWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()
    
protected:
	virtual void NativeDestruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = MythicActivatableWidget)
	void RegisterInputBinding(FGameplayTag InputTag, EInputEvent InputType, const FInputActionExecutedDelegate& Callback, bool ShowInActionBar, FInputActionBindingHandle& BindingHandle);

	UFUNCTION(BlueprintCallable, Category = MythicActivatableWidget)
	void UnregisterInputBinding(FInputActionBindingHandle BindingHandle);

	UFUNCTION(BlueprintCallable, Category = MythicActivatableWidget)
	void UnregisterAllBindings();
    
    //~UCommonActivatableWidget interface
    virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
    //~End of UCommonActivatableWidget interface

private:
    UPROPERTY()
	TArray<FUIActionBindingHandle> BindingHandles;
    
#if WITH_EDITOR
    virtual void ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, class IWidgetCompilerLog& CompileLog) const override;
#endif
	
protected:
    /** The desired input mode to use while this UI is activated, for example do you want key presses to still reach the game/player controller? */
    UPROPERTY(EditDefaultsOnly, Category = Input)
    EMythicWidgetInputMode InputConfig = EMythicWidgetInputMode::GameAndMenu;

    /** The desired mouse behavior when the game gets input. */
    UPROPERTY(EditDefaultsOnly, Category = Input)
    EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
};