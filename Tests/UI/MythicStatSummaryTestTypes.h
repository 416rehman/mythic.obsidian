#pragma once

#include "CoreMinimal.h"
#include "GAS/MythicStatSummary.h"
#include "MythicStatSummaryTestTypes.generated.h"

/** A calculation with a known answer, so a test can prove Compute() runs the authored class. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicStatSummaryCalculation_Fixed : public UMythicStatSummaryCalculation {
    GENERATED_BODY()

public:
    virtual float Calculate_Implementation(const UAbilitySystemComponent *ASC) const override { return 1234.5f; }
};
