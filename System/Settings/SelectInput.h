#pragma once
#include "InputTypes.h"
#include "SelectInput.generated.h"


UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicSelectInput : public UMythicInput {
    GENERATED_BODY()

    UMythicSelectInput();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
    TArray<FOptionAndDescription> Options;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
    uint8 CurrentOptionIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
    uint8 IncomingOptionIndex;

    // Default state, current state, and incoming state that should be applied, it can be optional.
public:
    // Constructor
    UMythicSelectInput(FText InLabel, FText InDescription, TArray<FOptionAndDescription> InOptions);

    virtual void Apply() override;
    virtual void Reset() override;
    virtual void NextOption() override;
    virtual void PreviousOption() override;
    virtual bool IsDisabled(APlayerController *inPlayerController, FText &Reason) override;
    virtual TArray<FOptionAndDescription> GetOptionDescriptions() override;
    virtual bool IsDirty() const override;

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    const TArray<FOptionAndDescription> &GetOptions() const;

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void SetOptions(const TArray<FOptionAndDescription> &InOptions);

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void SetCurrentOptionIndex(uint8 InCurrentOptionIndex);

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    uint8 GetCurrentOptionIndex() const;

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void SetIncomingOptionIndex(uint8 InIncomingOptionIndex);

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    uint8 GetIncomingOptionIndex() const;
};
