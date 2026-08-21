// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "MythicKitInputs.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UENUM()
enum class EMythicKitInputKind : uint8 {
    Slider = 0,
    Check = 1,
    Stepper = 2,
};

UCLASS(Abstract)
class MYTHIC_API UMythicKitInputBase : public UCommonUserWidget {
    GENERATED_BODY()

public:
    /** 0..1 for a slider, 0/1 for a check, step index normalised for a stepper. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Kit")
    void SetValue(float InValue);

    UFUNCTION(BlueprintPure, Category = "Mythic|Kit")
    float GetValue() const { return Value; }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicKitValueChanged, float, NewValue);

    UPROPERTY(BlueprintAssignable, Category = "Mythic|Kit")
    FMythicKitValueChanged OnValueChanged;

protected:
    virtual void NativeConstruct() override;
    virtual void NativePreConstruct() override;

    virtual void NativeOnMouseEnter(const FGeometry &Geo, const FPointerEvent &Event) override;
    virtual void NativeOnMouseLeave(const FPointerEvent &Event) override;
    virtual FReply NativeOnFocusReceived(const FGeometry &Geo, const FFocusEvent &Event) override;
    virtual void NativeOnFocusLost(const FFocusEvent &Event) override;
    virtual FReply NativeOnKeyDown(const FGeometry &Geo, const FKeyEvent &Event) override;

    EMythicKitInputKind Kind = EMythicKitInputKind::Slider;

    virtual void Step(int32 Delta) {}

    void PushToMaterial();

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Visual;

    /** Defaults to M_UI_HandInput. Overridable so a screen can supply a differently tinted instance. */
    UPROPERTY(EditAnywhere, Category = "Mythic|Kit")
    TObjectPtr<UMaterialInterface> InputMaterial;

    UPROPERTY(EditAnywhere, Category = "Mythic|Kit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Value = 0.5f;

    /** How far one press of left/right moves a slider. Steppers ignore this. */
    UPROPERTY(EditAnywhere, Category = "Mythic|Kit", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float StepSize = 0.05f;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> Material;

    bool bHovered = false;
    bool bFocused = false;
};

UCLASS()
class MYTHIC_API UMythicKitSlider : public UMythicKitInputBase {
    GENERATED_BODY()

public:
    UMythicKitSlider();

protected:
    virtual void Step(int32 Delta) override;
};

UCLASS()
class MYTHIC_API UMythicKitCheck : public UMythicKitInputBase {
    GENERATED_BODY()

public:
    UMythicKitCheck();

    UFUNCTION(BlueprintPure, Category = "Mythic|Kit")
    bool IsChecked() const { return Value > 0.5f; }

    UFUNCTION(BlueprintCallable, Category = "Mythic|Kit")
    void SetChecked(bool bInChecked);

protected:
    virtual void Step(int32 Delta) override;
};

UCLASS()
class MYTHIC_API UMythicKitSelect : public UMythicKitInputBase {
    GENERATED_BODY()

public:
    UMythicKitSelect();

    UFUNCTION(BlueprintCallable, Category = "Mythic|Kit")
    void SetOptionCount(int32 InCount);

    UFUNCTION(BlueprintPure, Category = "Mythic|Kit")
    int32 GetIndex() const;

protected:
    virtual void Step(int32 Delta) override;
    virtual void NativePreConstruct() override;

    UPROPERTY(EditAnywhere, Category = "Mythic|Kit", meta = (ClampMin = "2"))
    int32 OptionCount = 4;

    /**
     * Whether a press past the last option returns to the first.
     *
     * Off by default. A wrapping control gives a player holding right no way to tell they have reached the end --
     * the value just keeps changing. Stopping at the edge is what makes the range legible.
     */
    UPROPERTY(EditAnywhere, Category = "Mythic|Kit")
    bool bWrap = false;
};
