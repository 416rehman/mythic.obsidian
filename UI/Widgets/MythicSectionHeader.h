// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "MythicSectionHeader.generated.h"

class UCommonTextBlock;
class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicSectionHeaderToggled, class UMythicSectionHeader *, Header);

/**
 * The one way a section announces itself: emblem, name, and the count or value that belongs to it.
 *
 * Every screen had grown its own - the settings group row, the stat sheet's heading row, the map page's
 * Hdr_Powers, the proficiency and socket panels - each with different type, spacing and rule art, so five
 * screens that do the same job read as five products. Callers now pass content and get the house header.
 *
 * Icon and trailing text collapse when nothing is given, so a bare "Defense" and an
 * "[emblem] Proficiencies 7/13" are the same widget.
 */
UCLASS(Abstract)
class MYTHIC_API UMythicSectionHeader : public UCommonUserWidget {
    GENERATED_BODY()

public:
    /** Sets everything the header shows. An empty icon or trailing collapses rather than leaving a gap. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|UI")
    void SetHeader(const FText &InLabel, const FText &InTrailing, UTexture2D *InIcon = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Mythic|UI")
    void SetLabel(const FText &InLabel) { SetHeader(InLabel, TrailingText, IconTexture); }

    /**
     * Makes the header a drawer handle: it takes the hit itself and broadcasts OnToggled, showing a chevron
     * when the optional Txt_Chevron exists. The owner keeps the open/closed state and calls SetCollapsed —
     * the header never hides anything itself.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|UI")
    void SetCollapsible(bool bInCollapsible);

    UFUNCTION(BlueprintCallable, Category = "Mythic|UI")
    void SetCollapsed(bool bInCollapsed);

    UFUNCTION(BlueprintPure, Category = "Mythic|UI")
    bool IsCollapsed() const { return bCollapsed; }

    UPROPERTY(BlueprintAssignable, Category = "Mythic|UI")
    FMythicSectionHeaderToggled OnToggled;

protected:
    virtual void NativePreConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent &InMouseEvent) override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Emblem;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Label;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Trailing;

    /** The kit's Rule.Section, drawn under the label. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Rule;

    /** Drawer chevron, shown only while collapsible. Fallback when no ChevronTexture is set. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Chevron;

    /** Textured chevron. With ChevronTexture set it replaces Txt_Chevron and rotates on toggle. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Chevron;

    /** Lit with the kit's Focus.Row while the pointer rests on a collapsible header. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_HoverBacking;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|UI")
    TSoftObjectPtr<UTexture2D> ChevronTexture;

    /** Previewed in the designer and used when a caller passes no icon. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|UI")
    FText LabelText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|UI")
    FText TrailingText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|UI")
    TObjectPtr<UTexture2D> IconTexture;

private:
    void Apply();
    void ApplyHoverBacking();

    bool bCollapsible = false;
    bool bCollapsed = false;
    bool bPointerOver = false;
};
