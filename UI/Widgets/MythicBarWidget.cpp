// Copyright Stellar Games. All Rights Reserved.

#include "MythicBarWidget.h"

#include "Components/Image.h"
#include "INotifyFieldValueChanged.h"
#include "Materials/MaterialInstanceDynamic.h"

void UMythicBarWidget::NativeConstruct() {
    Super::NativeConstruct();

    if (Fill) {
        FillMID = Fill->GetDynamicMaterial();
    }
    PushToMaterial();
}

void UMythicBarWidget::NativeDestruct() {
    Unbind();
    FillMID = nullptr;
    Super::NativeDestruct();
}

void UMythicBarWidget::SetPercent(float InPercent) {
    const float Clamped = FMath::Clamp(InPercent, 0.0f, 1.0f);
    if (FMath::IsNearlyEqual(Clamped, Percent)) {
        return;
    }
    Percent = Clamped;
    PushToMaterial();
}

void UMythicBarWidget::PushToMaterial() {
    if (!FillMID && Fill) {
        FillMID = Fill->GetDynamicMaterial();
    }
    if (FillMID) {
        FillMID->SetScalarParameterValue(PercentParameter, Percent);
    }
}

void UMythicBarWidget::BindToFloatField(UObject *InViewModel, FName InFieldName) {
    Unbind();

    if (!InViewModel || InFieldName.IsNone()) {
        return;
    }

    INotifyFieldValueChanged *Notify = Cast<INotifyFieldValueChanged>(InViewModel);
    if (!Notify) {
        UE_LOG(LogTemp, Warning, TEXT("MythicBarWidget: %s does not implement INotifyFieldValueChanged"),
               *InViewModel->GetName());
        return;
    }

    const UE::FieldNotification::IClassDescriptor &Descriptor = Notify->GetFieldNotificationDescriptor();
    const UE::FieldNotification::FFieldId FieldId = Descriptor.GetField(InViewModel->GetClass(), InFieldName);
    if (!FieldId.IsValid()) {
        UE_LOG(LogTemp, Warning, TEXT("MythicBarWidget: '%s' is not a FieldNotify field on %s"),
               *InFieldName.ToString(), *InViewModel->GetClass()->GetName());
        return;
    }

    const INotifyFieldValueChanged::FFieldValueChangedDelegate Delegate =
        INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UMythicBarWidget::HandleFieldChanged);

    BoundHandle = Notify->AddFieldValueChangedDelegate(FieldId, Delegate);
    BoundViewModel = InViewModel;
    BoundFieldId = FieldId;
    BoundFieldName = InFieldName;

    PullFromViewModel();
}

void UMythicBarWidget::Unbind() {
    if (UObject *VM = BoundViewModel.Get()) {
        if (INotifyFieldValueChanged *Notify = Cast<INotifyFieldValueChanged>(VM)) {
            if (BoundFieldId.IsValid() && BoundHandle.IsValid()) {
                Notify->RemoveFieldValueChangedDelegate(BoundFieldId, BoundHandle);
            }
        }
    }
    BoundViewModel = nullptr;
    BoundFieldId = UE::FieldNotification::FFieldId();
    BoundHandle.Reset();
    BoundFieldName = NAME_None;
}

void UMythicBarWidget::HandleFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId) {
    if (FieldId == BoundFieldId) {
        PullFromViewModel();
    }
}

void UMythicBarWidget::PullFromViewModel() {
    UObject *VM = BoundViewModel.Get();
    if (!VM || BoundFieldName.IsNone()) {
        return;
    }

    if (const FFloatProperty *FloatProp = FindFProperty<FFloatProperty>(VM->GetClass(), BoundFieldName)) {
        SetPercent(FloatProp->GetPropertyValue_InContainer(VM));
        return;
    }
    if (const FDoubleProperty *DoubleProp = FindFProperty<FDoubleProperty>(VM->GetClass(), BoundFieldName)) {
        SetPercent(static_cast<float>(DoubleProp->GetPropertyValue_InContainer(VM)));
    }
}
