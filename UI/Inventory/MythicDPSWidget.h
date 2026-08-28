// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Itemization/Inventory/ViewModels/ItemTooltipVM.h"
#include "MythicDPSWidget.generated.h"

class UCommonTextBlock;

/**
 * Typed native presentation base for the dedicated weapon attack block in item details.
 *
 * This widget owns no combat math and holds no item or fragment reference. It renders one atomic, display-ready
 * projection so Blueprint layout can evolve without becoming a second gameplay authority.
 */
UCLASS(Abstract, Blueprintable)
class MYTHIC_API UMythicDPSWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** Replaces every displayed attack metric from one canonical item-local projection. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details|Attack")
    void SetAttackDisplayData(const FMythicWeaponAttackViewData &InAttackDisplayData);

    /** Clears all displayed attack metrics when the details card no longer represents an eligible weapon. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details|Attack")
    void ClearAttackDisplayData();

    /** Last canonical attack projection rendered by this pooled widget; invalid after it is cleared. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Mythic|Item Details|Attack")
    FMythicWeaponAttackViewData AttackDisplayData;

protected:
    /** Required text block that presents item-local sustained damage per second. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCommonTextBlock> DamagePerSecondText;

    /** Required text block that presents composed item-local damage dealt by one hit. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCommonTextBlock> DamagePerHitText;

    /** Required text block that presents effective item-local attacks per second. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCommonTextBlock> AttacksPerSecondText;

    /** Called after native text bindings and AttackDisplayData have been updated atomically. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Item Details|Attack",
              meta = (DisplayName = "On Attack Presentation Updated"))
    void OnAttackPresentationUpdated(const FMythicWeaponAttackViewData &InAttackDisplayData);
};
