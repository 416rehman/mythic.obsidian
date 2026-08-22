

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

UCLASS(Blueprintable)
class MYTHIC_API UMythicActivatableWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = MythicActivatableWidget)
	void RegisterInputBinding(FGameplayTag InputTag, EInputEvent InputType, const FInputActionExecutedDelegate& Callback, bool ShowInActionBar, FInputActionBindingHandle& BindingHandle);

	/**
	 * Bind a real Enhanced Input action, which is what the bound action bar can actually display.
	 *
	 * The bar drops any binding it cannot prove is reachable on the current device, and it proves that by
	 * asking Enhanced Input which keys map to the binding's UInputAction. A tag with ini key rows is the
	 * legacy path: it drives the press fine and shows nothing, which is why no prompt has ever appeared.
	 */
	UFUNCTION(BlueprintCallable, Category = MythicActivatableWidget)
	void RegisterInputActionBinding(UInputAction* InputAction, EInputEvent InputType,
	                                const FInputActionExecutedDelegate& Callback, bool ShowInActionBar,
	                                FInputActionBindingHandle& BindingHandle);

	UFUNCTION(BlueprintCallable, Category = MythicActivatableWidget)
	void UnregisterInputBinding(FInputActionBindingHandle BindingHandle);

	UFUNCTION(BlueprintCallable, Category = MythicActivatableWidget)
	void UnregisterAllBindings();

    virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
    /**
     * Mapping context added while this screen is up, removed when it closes.
     *
     * Enhanced Input only reports keys for an action inside an ACTIVE context, so a screen whose actions
     * are never mapped anywhere has prompts the bar refuses to draw. Opt-in per screen: leave it unset and
     * nothing changes.
     */
    UPROPERTY(EditDefaultsOnly, Category = Input)
    TSoftObjectPtr<class UInputMappingContext> UIInputContext;

    /** Priority for the context above. Above gameplay, because a menu is on top of the world. */
    UPROPERTY(EditDefaultsOnly, Category = Input)
    int32 UIInputContextPriority = 100;

    void AddUIInputContext();
    void RemoveUIInputContext();

public:

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