// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicEscapeMenuWidget.generated.h"

class UButton;
class UCommonActivatableWidget;
class UCommonTextBlock;
class UMythicEscapeMenuWidget;
class UPanelWidget;

UENUM()
enum class EMythicEscapeAction : uint8 {
    Resume,
    Settings,
    Quit,
};

UCLASS()
class MYTHIC_API UMythicEscapeClickProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicEscapeMenuWidget> Menu;

    UPROPERTY()
    EMythicEscapeAction Action = EMythicEscapeAction::Resume;

    UFUNCTION()
    void HandleClicked();
};

USTRUCT()
struct FMythicEscapeEntry {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UWidget> Button;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Label;

    UPROPERTY()
    TObjectPtr<UMythicEscapeClickProxy> Proxy;
};

UCLASS()
class MYTHIC_API UMythicEscapeMenuWidget : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    void RunAction(EMythicEscapeAction Action);

    void QuitNow();

protected:
    virtual void NativeConstruct() override;
    virtual UWidget *NativeGetDesiredFocusTarget() const override;

    /** Buttons go here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> ButtonList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Title;

public:
    /** The screen Escape opens for Settings. Public for the same reachability reason as the menu shell. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Escape")
    TSubclassOf<UCommonActivatableWidget> GetSettingsScreenClass() const { return SettingsScreenClass; }

protected:
    /** The settings screen, pushed onto SettingsLayerTag when Settings is chosen. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Escape")
    TSubclassOf<UCommonActivatableWidget> SettingsScreenClass;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Escape", meta = (Categories = "UI.Layer"))
    FGameplayTag SettingsLayerTag;

    /**
     * The confirm prompt Quit opens (WBP_ConfirmModal). Quit is the one thing on this screen you cannot take back, so
     * it is never one click away. Unset falls back to the kit's prompt by path, so an unconfigured menu still asks.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Escape")
    TSubclassOf<UCommonActivatableWidget> QuitPromptClass;

    /** Layer the prompt is pushed to. Unset falls back to UI.Layer.Modal. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Escape", meta = (Categories = "UI.Layer"))
    FGameplayTag QuitPromptLayerTag;

private:
    void AddEntry(EMythicEscapeAction Action, const FText &Label);

    void OpenQuitPrompt();

    void FocusFirstRow();

    UPROPERTY()
    TArray<FMythicEscapeEntry> Entries;

    bool bBuilt = false;
};
