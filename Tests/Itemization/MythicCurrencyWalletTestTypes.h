#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Player/MythicPlayerController.h"
#include "UI/HUD/MythicHudNotice.h"
#include "MythicCurrencyWalletTestTypes.generated.h"

/** A controller with a second purse, so a charge that must span inventories has two to span. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class AMythicCurrencyWalletTestController final : public AMythicPlayerController {
    GENERATED_BODY()

public:
    AMythicCurrencyWalletTestController() {
        SecondInventory = CreateDefaultSubobject<UMythicInventoryComponent>(TEXT("SecondInventory"));
    }

    UPROPERTY()
    TObjectPtr<UMythicInventoryComponent> SecondInventory;

    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override {
        return {InventoryComponent, SecondInventory};
    }
};

/** Counts the HUD notices a controller raises the way the feed would: through a UFUNCTION handler. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicCurrencyWalletNoticeListener final : public UObject {
    GENERATED_BODY()

public:
    int32 NoticeCount = 0;
    FMythicHudNotice LastNotice;

    void Bind(AMythicPlayerController *Controller) {
        Controller->OnHudNotice.AddUniqueDynamic(this, &UMythicCurrencyWalletNoticeListener::HandleNotice);
    }

    UFUNCTION()
    void HandleNotice(const FMythicHudNotice &Notice) {
        NoticeCount++;
        LastNotice = Notice;
    }
};
