#pragma once

#include "Input/CommonUIActionRouterBase.h"
#include "MythicUIActionRouter.generated.h"

/** Keep CommonUI acceptance in menus; gameplay receives its original physical key. */
UCLASS()
class MYTHIC_API UMythicUIActionRouter : public UCommonUIActionRouterBase {
    GENERATED_BODY()

protected:
    virtual TSharedRef<FCommonAnalogCursor> MakeAnalogCursor() const override;
};
