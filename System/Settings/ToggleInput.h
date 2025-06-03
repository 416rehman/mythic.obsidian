#pragma once
#include "SelectInput.h"
#include "ToggleInput.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicToggleInput : public UMythicSelectInput {
    GENERATED_BODY()

    UMythicToggleInput() : UMythicSelectInput(FText::FromString("Toggle"), FText::FromString("Toggle Description"),
                                              {{FText::FromString("Off"), FText::FromString("Off")}, {FText::FromString("On"), FText::FromString("On")}}) {
    };

    // Constructor
    UMythicToggleInput(FText InLabel, FText InDescription, bool DefaultOn, FText OnDesc = FText::FromString("On"), FText OffDesc = FText::FromString("Off")) :
        UMythicSelectInput(InLabel, InDescription, {{OnDesc, FText::FromString("On")}, {OffDesc, FText::FromString("Off")}}) {
        this->Label = InLabel;
        this->Description = InDescription;

        this->CurrentOptionIndex = DefaultOn ? 1 : 0;
        this->IncomingOptionIndex = this->CurrentOptionIndex;

        this->Options.Add({FText::FromString("Off"), OffDesc});
        this->Options.Add({FText::FromString("On"), OnDesc});
    }

public:
    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    ECheckBoxState IsOn() const {
        return CurrentOptionIndex == 1 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
};
