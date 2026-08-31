#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Nameplate/MythicNameplateTypes.h"

#include "MythicNameplateActionRailWidget.generated.h"

class UBorder;
class UMythicInputGlyph;
class UMythicNameplateVisualStyle;
class USizeBox;
class UTextBlock;

/** Fixed-tree renderer for the one focused entity action rail owned by a LocalPlayer HUD. */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicNameplateActionRailWidget : public UUserWidget {
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;

    /** Applies one exact-subject, available-only projection without querying gameplay or rebuilding the widget tree. */
    void ApplyProjection(
        const FMythicNameplateActionRailProjection &InProjection);

    /** Applies project visual tokens and local accessibility scale to this prewarmed rail. */
    void SetPresentationStyle(UMythicNameplateVisualStyle *InStyle,
                              const FMythicNameplateRenderPreferences &InPreferences);

    /** Clears glyphs/text and collapses the rail before another focused embodiment can claim it. */
    void ResetForPool();

    /** Called after native code applies a new exact-subject rail projection. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Nameplate|Action Rail")
    void OnActionRailProjectionChanged();

    /** Called when the prewarmed rail is released or its exact subject becomes stale. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Nameplate|Action Rail")
    void OnActionRailReleased();

protected:
    /** Content-local low-alpha scrim; it must never be authored as a large opaque panel. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate|Action Rail",
              meta = (BindWidgetOptional))
    TObjectPtr<UBorder> RailSurface;

    /** Fixed maximum-bounds container for the one-line rail. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate|Action Rail",
              meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> RailSizeBox;

    /** Current-device glyph for the highest-priority available action. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate|Action Rail",
              meta = (BindWidgetOptional))
    TObjectPtr<UMythicInputGlyph> PrimaryGlyph;

    /** Localized verb for the highest-priority available action. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate|Action Rail",
              meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> PrimaryActionText;

    /** Current-device glyph for the second available action or Inspect. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate|Action Rail",
              meta = (BindWidgetOptional))
    TObjectPtr<UMythicInputGlyph> SecondaryGlyph;

    /** Localized verb for the second available action or Inspect. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate|Action Rail",
              meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SecondaryActionText;

private:
    void ApplyEntry(UMythicInputGlyph *Glyph, UTextBlock *Text,
                    const FGameplayTag &InputTag, const FText &Label);
    void ResetEntry(UMythicInputGlyph *Glyph, UTextBlock *Text);

    /** Last exact-subject DTO, retained only so stale placement/release checks remain deterministic. */
    UPROPERTY(Transient)
    FMythicNameplateActionRailProjection Projection;

    /** Project visual tokens copied from the owning layer; no gameplay policy is read from this asset. */
    UPROPERTY(Transient)
    TObjectPtr<UMythicNameplateVisualStyle> VisualStyle;

    FMythicNameplateRenderPreferences RenderPreferences;
};
