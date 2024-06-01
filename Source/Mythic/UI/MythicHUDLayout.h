// 

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MythicHUDLayout.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class MYTHIC_API UMythicHUDLayout : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    UMythicHUDLayout(const FObjectInitializer& ObjectInitializer);
    void NativeOnInitialized() override;

protected:
    void HandleEscapeAction();

    UPROPERTY(EditDefaultsOnly)
    TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;
};
