#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "World/Harvesting/MythicHarvestFocusComponent.h"

#include "MythicHarvestPromptWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * World-anchored one-line answer to "can I harvest this". The focus component has already resolved tool-slot
 * occupancy, so this widget only renders the decision it was handed and never inspects inventory itself.
 */
UCLASS(Abstract)
class MYTHIC_API UMythicHarvestPromptWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** Applies one resolved focus snapshot; an unfocused snapshot collapses the prompt without touching gameplay. */
    UFUNCTION(BlueprintCallable, Category = "Harvest|Prompt")
    void SetFocusPresentation(const FMythicHarvestFocusPresentation &InFocus);

protected:
    /** Required label carrying the component-formatted line, e.g. "Requires Axe" or "Chop Oak". */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Prompt", meta = (BindWidget))
    TObjectPtr<UTextBlock> PromptLabel;

    /** Optional required-tool icon; absent art leaves the line readable on its own. */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Prompt", meta = (BindWidgetOptional))
    TObjectPtr<UImage> ToolIcon;

    /** Fires after the native fields are applied so Blueprint may restyle per availability without re-deriving it. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Harvest|Prompt")
    void OnFocusPresentationChanged(const FMythicHarvestFocusPresentation &Focus);
};
