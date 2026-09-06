#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MythicAbilityReticleWidget.generated.h"

/** Passive pointer for the exact camera intent sampled by Q/E. Never takes CommonUI focus. */
UCLASS(Blueprintable)
class MYTHIC_API UMythicAbilityReticleWidget : public UUserWidget {
    GENERATED_BODY()
public:
    explicit UMythicAbilityReticleWidget(const FObjectInitializer& Initializer);
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,
        FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled) const override;
};
