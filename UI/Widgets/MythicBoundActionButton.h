// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/CommonBoundActionButton.h"
#include "MythicBoundActionButton.generated.h"

class UMythicInputGlyph;

UCLASS(Abstract, meta = (DisableNativeTick))
class MYTHIC_API UMythicBoundActionButton : public UCommonBoundActionButton {
    GENERATED_BODY()

public:
    virtual void SetRepresentedAction(FUIActionBindingHandle InBindingHandle) override;

protected:
    virtual void UpdateInputActionWidget() override;

    /** Optional: a button that only wants a label simply leaves this out. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicInputGlyph> Glyph;

private:
    FUIActionBindingHandle MythicBindingHandle;
};
