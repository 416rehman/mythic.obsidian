// 

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MythicActivatableWidget.h"
#include "MythicHUDLayout.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class MYTHIC_API UMythicHUDLayout : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    UMythicHUDLayout(const FObjectInitializer &ObjectInitializer);


    void NativeOnInitialized() override;

public:
    // Return true to skip the menu opening
    UFUNCTION(BlueprintImplementableEvent, Category = MythicHUDLayout)
    bool PreEscapeMenuOpen();

protected:
    void HandleEscapeAction();

    UPROPERTY(EditDefaultsOnly)
    TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;
};
