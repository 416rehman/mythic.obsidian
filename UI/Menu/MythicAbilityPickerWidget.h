#pragma once
#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UI/Menu/MythicRunePickerCellWidget.h"
#include "UI/MythicActivatableWidget.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "MythicAbilityPickerWidget.generated.h"

class UMythicAbilityPickerWidget;
class UMythicCharacterPageWidget;
class UMythicSectionHeader;
class UMythicInputGlyph;
class UListView;
class UPanelWidget;
class URichTextBlock;
class UButton;
class UInputAction;
class UWidgetAnimation;

UCLASS()
class MYTHIC_API UMythicAbilityListItem : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UMythicSkillDefinition> Definition;
    UPROPERTY() TWeakObjectPtr<UMythicAbilityPickerWidget> Picker;
    int32 Index = INDEX_NONE;
};

/** The existing house cell, hosted by UListView's bounded entry pool. */
UCLASS()
class MYTHIC_API UMythicAbilityListEntry : public UMythicRunePickerCellWidget, public IUserObjectListEntry {
    GENERATED_BODY()
public:
    void Refresh();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeOnListItemObjectSet(UObject* Object) override;
    virtual void NativeOnEntryReleased() override;
    UFUNCTION() void Pressed(int32 Index);
    UFUNCTION() void Noticed(int32 Index,bool bOn);
private:
    TWeakObjectPtr<UMythicAbilityListItem> Item;
};

UCLASS()
class MYTHIC_API UMythicAbilityPickerWidget : public UMythicActivatableWidget {
    GENERATED_BODY()
public:
    void OpenForSlot(int32 SlotIndex,UMythicCharacterPageWidget* InPage);
    void SetCharacterPage(UMythicCharacterPageWidget* InPage);
    UFUNCTION(BlueprintCallable) void Close();
    void SelectAbility(int32 Index);
    FMythicRuneCellState DescribeCell(int32 Index);
    UTexture2D* FindPreloadedIcon(const UMythicSkillDefinition* Definition);
    bool IsOnLayer() const { return bOnLayer; }
    UFUNCTION() void Refresh();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;
    virtual bool NativeOnHandleBackAction() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    UFUNCTION() void SelectSocket(int32 Index);
    UFUNCTION() void HandlePrimary();
    UFUNCTION() void UnequipCurrent();
    UFUNCTION() void EquipCurrent();
    UFUNCTION() void HandleResult(FGuid Id,EMythicAbilityResult Result,int32 Revision);
    UFUNCTION() void FinishClose();
    UFUNCTION() void FinishOpenAnimation();
    UFUNCTION() UWidget* NavigateLibrary(EUINavigation Direction);
    UFUNCTION() UWidget* NavigateDetail(EUINavigation Direction);
    UFUNCTION() UWidget* NavigateSocket(EUINavigation Direction);
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UMythicRuneSocketWidget> SocketClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UMythicRunePickerCellWidget> CellClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UMythicSectionHeader> SectionHeaderClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UUserWidget> ModifierRowClass;
    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UInputAction> PrimaryAction;
    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UInputAction> ActionsAction;
    UPROPERTY(EditDefaultsOnly) FGameplayTag PickerLayerTag;
    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UTexture2D> TitleEmblem;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> Card;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Dismiss;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UMythicSectionHeader> TitleHeader;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UMythicSectionHeader> SocketHeader;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UMythicSectionHeader> GridHeader;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonButtonBase> CloseButton;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonButtonBase> UnequipButton;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonButtonBase> EquipButton;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonButtonBase> FooterCloseButton;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> SocketStrip;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UListView> AbilityList;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UMythicRuneSocketWidget> DetailSocket;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonTextBlock> DetailName;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonTextBlock> DetailCategory;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<URichTextBlock> DetailDescription;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonTextBlock> DetailHint;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonTextBlock> DetailProgress;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UPanelWidget> ModifierHost;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UMythicInputGlyph> Glyph_Select;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UMythicInputGlyph> Glyph_Actions;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UMythicInputGlyph> Glyph_Back;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonTextBlock> Lbl_Select;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonTextBlock> Lbl_Actions;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCommonTextBlock> Lbl_Back;
    UPROPERTY(Transient,meta=(BindWidgetAnimOptional)) TObjectPtr<UWidgetAnimation> OpenAnim;
    UPROPERTY(Transient,meta=(BindWidgetAnimOptional)) TObjectPtr<UWidgetAnimation> CloseAnim;
private:
    void BuildLibrary();
    void RefreshDetail();
    void Submit(EMythicAbilityAction Action,FName ModifierId=NAME_None,bool bActive=false);
    void ToggleModifier(int32 Index);
    void BindSkills();
    void UnbindSkills();
    UMythicSkillComponent* FindSkills() const;
    UMythicSkillDefinition* SelectedDefinition() const;
    void SetNotice(const FText& Text);
    void RefreshNavigation();
    void ConfigureEntryNavigation(UUserWidget& Entry);
    void FocusBeforeDisabling(UWidget* Widget);
    TWeakObjectPtr<UMythicAbilityListItem> PendingFocusItem;
    UPROPERTY() TArray<TObjectPtr<UMythicAbilityListItem>> Library;
    UPROPERTY() TArray<TObjectPtr<UMythicRuneSocketWidget>> StripSockets;
    UPROPERTY() TArray<TObjectPtr<UUserWidget>> ModifierRows;
    UPROPERTY() TArray<TObjectPtr<UCommonButtonBase>> ModifierButtons;
    UPROPERTY() TMap<FSoftObjectPath,TObjectPtr<UTexture2D>> IconCache;
    TSet<FSoftObjectPath> PendingIcons;
    TArray<FSoftObjectPath> IconOrder;
    TWeakObjectPtr<UMythicSkillComponent> BoundSkills;
    TWeakObjectPtr<UMythicCharacterPageWidget> Page;
    FInputActionBindingHandle PrimaryBinding;
    FInputActionBindingHandle ActionsBinding;
    FGuid PendingRequest;
    int32 PendingRevision = INDEX_NONE;
    FTimerHandle PendingTimer;
    int32 SelectedSlot = 0;
    int32 ReturnSlot = 0;
    int32 SelectedIndex = INDEX_NONE;
    bool bOnLayer = false;
    bool bClosing = false;
    FText Notice;
};
