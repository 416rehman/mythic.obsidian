
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MythicDamageApplication.generated.h"

UCLASS()
class MYTHIC_API UMythicDamageApplication : public UGameplayEffectExecutionCalculation {
    GENERATED_BODY()
public:
    UMythicDamageApplication();

    static bool ShouldNegateFriendlyFire(bool bSourceIsPlayer, bool bTargetIsPlayer, bool bSameActor, bool bFriendlyFireEnabled);

    static float ApplySkillDamageBonus(float Damage, bool bIsSkillHit, float BonusSkillDamage);

    // Buildup one landed proc contributes. Never negative: a hit must never drain the meter it is filling.
    static float ComputeBuildupPerProc(float BasePerProc, float SourceMultiplier);

    // Armour must not reduce a hit below the chip floor, but a hit already nullified stays nullified —
    // otherwise immunity through IncomingDamageMultiplier cannot be expressed.
    static float ApplyChipFloor(float Damage, float MinChipDamage);

    /** Suppresses Gameplay Cues owned by an application effect when damage is rejected before landing. */
    static void MarkDamageExecutionAborted(
        FGameplayEffectCustomExecutionOutput &OutExecutionOutput);

    /**
     * Applies the native Damage.Hit cue policy to a fully composed pre-mitigation damage value. Zero, negative, or
     * non-finite damage suppresses the application GE's automatic hit cue; positive damage leaves it enabled.
     * Returns true when the resolved hit carries no damage. Status buildup may still be processed by the caller.
     */
    static bool HandleResolvedDamageCuePolicy(
        float ResolvedDamage,
        FGameplayEffectCustomExecutionOutput &OutExecutionOutput);

protected:
    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
    TArray<FGameplayEffectAttributeCaptureDefinition> IncreasedComposeCaptures;
    TArray<FGameplayEffectAttributeCaptureDefinition> MoreComposeCaptures;
};
