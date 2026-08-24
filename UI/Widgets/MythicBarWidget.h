// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FieldNotificationId.h"
#include "MythicBarWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;

UCLASS()
class MYTHIC_API UMythicBarWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** Set the fill directly. Clamped to 0-1. Cheap enough to call as often as you like. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Bar")
    void SetPercent(float InPercent);

    UFUNCTION(BlueprintPure, Category = "Mythic|Bar")
    float GetPercent() const { return Percent; }

    /**
     * Follow a float field on a ViewModel.
     *
     * @param InViewModel  Any object implementing INotifyFieldValueChanged (every UMVVMViewModelBase does).
     * @param InFieldName  The FieldNotify property name, e.g. "Progress" or "FuelFraction".
     *
     * Re-binding to a different ViewModel unhooks the old one first, so recycled list rows do not leak or follow
     * the wrong job. Reads the value once immediately so the bar is correct before the first change fires.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Bar")
    void BindToFloatField(UObject *InViewModel, FName InFieldName);

    /** Stop following whatever ViewModel this bar was bound to. Safe to call when nothing is bound. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Bar")
    void Unbind();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** The image whose material carries the fill parameter. Optional so a designer can preview the widget empty. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Fill;

    /** Scalar parameter to write. "Percent" matches M_UI_BarHand, which the vitals bars already use. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Bar")
    FName PercentParameter = FName("Percent");

private:
    void HandleFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId);

    void PullFromViewModel();

    void PushToMaterial();

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FillMID;

    UPROPERTY(Transient)
    TWeakObjectPtr<UObject> BoundViewModel;

    UE::FieldNotification::FFieldId BoundFieldId;
    FDelegateHandle BoundHandle;
    FName BoundFieldName;

    float Percent = 0.0f;
};
