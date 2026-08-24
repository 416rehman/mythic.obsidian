#include "GAS/MythicStatSummary.h"

#include "AbilitySystemComponent.h"

float UMythicStatSummaryDefinition::Compute(const UAbilitySystemComponent *ASC) const {
    if (!CalculationClass) {
        return 0.0f;
    }
    // The calculation is pure, so its class default object is a valid evaluator — no per-read allocation, nothing to
    // store, nothing to free.
    const UMythicStatSummaryCalculation *Calc = GetDefault<UMythicStatSummaryCalculation>(CalculationClass);
    return Calc ? Calc->Calculate(ASC) : 0.0f;
}
