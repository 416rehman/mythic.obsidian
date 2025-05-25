#pragma once
#include "MVVMViewModelBase.h"
#include "InputTypes.generated.h"

/// ViewModels for Mythic Input Types
/// Toggle On/Off
/// Select Option
/// Slider
///
///

USTRUCT(BlueprintType)
struct FOptionAndDescription {
    GENERATED_BODY()

    FText Name;
    FText Description;
};

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicInput : public UMVVMViewModelBase {
    GENERATED_BODY()

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
    FText Description;

public:
    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual void SetLabel(FText InLabel) {
        Label = InLabel;
    };

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual const FText &GetLabel() const {
        return Label;
    };

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual void SetDescription(FText InDescription) {
        Description = InDescription;
    };

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual const FText &GetDescription() const {
        return Description;
    };


    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual void Apply() {
    };

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual void Reset() {
    };

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual void NextOption() {
    };

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual void PreviousOption() {
    };

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual bool IsDisabled(APlayerController *inPlayerController, FText &Reason) { return false; };

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual TArray<FOptionAndDescription> GetOptionDescriptions() { return TArray<FOptionAndDescription>(); };

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    virtual bool IsDirty() const { return false; };
};
