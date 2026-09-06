#pragma once
#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "MythicChordValueModifier.generated.h"

class UInputAction;
class UInputTriggerChordAction;

/** Keep raw camera-action values consistent with the same Enhanced Input chord that gates events. */
UCLASS(meta=(DisplayName="Mythic Chord Value Gate"))
class MYTHIC_API UMythicChordValueModifier : public UInputModifier {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chord") TObjectPtr<UInputAction> ChordAction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chord") bool bPassWhenChordActive = true;
    /** Opt in ONLY on actual IA_Look camera-axis gates, never ability, reticle, zoom or movement mappings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chord") bool bAllowDuringCameraAdjustment = false;
protected:
    virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput,FInputActionValue CurrentValue,float DeltaTime) override;
private:
    UPROPERTY(Transient) TObjectPtr<UInputTriggerChordAction> ChordProbe;
};
