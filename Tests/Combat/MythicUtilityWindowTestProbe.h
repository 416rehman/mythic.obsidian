#pragma once
#include "Tests/Combat/MythicAbilityWindowTestProbe.h"
#include "MythicUtilityWindowTestProbe.generated.h"

/** Test-only authoring substitute. Admission, payment, geometry and motion stay native production code. */
UCLASS(NotBlueprintable,Transient,Hidden)
class UMythicUtilityWindowTestProbe : public UMythicAbilityWindowTestProbe {
    GENERATED_BODY()
public:
    UMythicUtilityWindowTestProbe() { bRequiresCameraAim=false; }
    FGuid EscapePreparation;
    FGuid StatusReservation;
    bool bPrepareEscape=true;
    bool bReserveBurn=false;
protected:
    virtual bool ValidateAbilityIntent_Implementation() override {
        EscapePreparation={}; StatusReservation={};
        if (bPrepareEscape) {
            EscapePreparation=PrepareUtilityEscape(TEXT("Escape"),GetMovementIntentDirection(true));
            if (!EscapePreparation.IsValid()) return false;
        }
        if (bReserveBurn) StatusReservation=ReserveSelfStatusGroup({FGameplayTag::RequestGameplayTag(TEXT("Status.Type.Burn"))});
        return true;
    }
};
