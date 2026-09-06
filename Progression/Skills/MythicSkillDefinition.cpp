
#include "MythicSkillDefinition.h"
#include "Misc/DataValidation.h"
#include "GameplayEffect.h"

int32 UMythicSkillDefinition::FindModifier(FName Id) const {
    return Id.IsNone() ? INDEX_NONE : Modifiers.IndexOfByPredicate([Id](const FMythicSkillModifier& M) { return M.ModifierId == Id; });
}

bool UMythicSkillDefinition::ValidateDefinition(TArray<FText>& Errors) const {
    auto Fail = [&Errors](const TCHAR* Message) { Errors.Add(FText::FromString(Message)); };
    if (Name.IsEmpty() || Description.IsEmpty() || Icon.IsNull()) Fail(TEXT("Ability requires a name, description and icon."));
    if (!HasPayload()) Fail(TEXT("Ability requires a runtime class."));
    if (ReusePolicy!=EMythicAbilityReusePolicy::TimedCooldown && ReusePolicy!=EMythicAbilityReusePolicy::NoTimedCooldown)
        Fail(TEXT("Ability requires a supported reuse policy."));
    if (HasTimedCooldown() && (!CooldownTag.IsValid() || !CooldownEffect || !FMath::IsFinite(CooldownSeconds) || CooldownSeconds <= 0 || CooldownSeconds > 3600))
        Fail(TEXT("Ability requires a unique cooldown tag, duration effect and bounded duration."));
    if (!HasTimedCooldown() && (CooldownTag.IsValid() || CooldownEffect || CooldownSeconds != 0))
        Fail(TEXT("No-timed-cooldown abilities must author zero seconds and no cooldown tag or effect."));
    if (CooldownEffect && CooldownEffect.GetDefaultObject()->DurationPolicy!=EGameplayEffectDurationType::HasDuration)
        Fail(TEXT("Ability cooldown effect must have a finite duration policy."));
    if (CooldownEffect && CooldownEffect.GetDefaultObject()->GetStackingType()!=EGameplayEffectStackingType::None)
        Fail(TEXT("Ability cooldown effects must not stack across definitions or restorations."));
    if (Acquisition == EMythicAbilityAcquisition::Prerequisite && !RequiredTag.IsValid()) Fail(TEXT("Prerequisite ability requires a permanent fact tag."));
    if (Acquisition != EMythicAbilityAcquisition::Starter && Hint.IsEmpty()) Fail(TEXT("Locked ability requires an acquisition hint."));
    TSet<FName> ActionIds, ParameterNames;
    if (Actions.Num()>64 || Parameters.Num()>128) Fail(TEXT("Ability action/parameter safety limit exceeded."));
    for (const auto& Action:Actions) {
        if (ActionIds.Contains(Action.ActionId)) Fail(TEXT("Duplicate ability action ID."));
        ActionIds.Add(Action.ActionId); Action.Validate(Errors);
    }
    for (const auto& Parameter:Parameters) {
        if (Parameter.Name.IsNone() || ParameterNames.Contains(Parameter.Name)
            || !FMath::IsFinite(Parameter.Value) || !FMath::IsFinite(Parameter.Minimum) || !FMath::IsFinite(Parameter.Maximum)
            || Parameter.Minimum>Parameter.Maximum || Parameter.Value<Parameter.Minimum || Parameter.Value>Parameter.Maximum)
            Fail(TEXT("Ability parameter requires a unique local name and finite ordered bounds."));
        ParameterNames.Add(Parameter.Name);
    }
    for (const auto& Modifier:Modifiers) {
        for (const auto& Delta:Modifier.ParameterDeltas)
            if (!ParameterNames.Contains(Delta.Key) || !FMath::IsFinite(Delta.Value)) Fail(TEXT("Refinement delta must refer to a declared parameter."));
        TSet<FName> Overrides;
        for (const auto& Action:Modifier.ActionOverrides) {
            if (!ActionIds.Contains(Action.ActionId) || Overrides.Contains(Action.ActionId)) Fail(TEXT("Refinement action override must uniquely replace a declared action."));
            Overrides.Add(Action.ActionId); Action.Validate(Errors);
        }
    }
    TSet<FName> Ids;
    int32 PreviousThreshold=0;
    if (MasteryUseThresholds.Num()>49 || RefinementSelectionLimit<0 || RefinementSelectionLimit>50) Fail(TEXT("Ability progression exceeds its content safety limits."));
    if (!bHasMasteryTrack && !MasteryUseThresholds.IsEmpty()) Fail(TEXT("A disabled mastery track cannot declare thresholds."));
    for (int32 Threshold:MasteryUseThresholds) {
        if (Threshold<=PreviousThreshold) Fail(TEXT("Mastery thresholds must be strictly increasing positive cumulative uses."));
        PreviousThreshold=Threshold;
    }
    for (const auto& M : Modifiers) {
        if (M.ModifierId.IsNone() || Ids.Contains(M.ModifierId) || !M.HasEffect() || (bUseLegacyModifierProgression && M.PointCost < 1))
            Fail(TEXT("Modifiers require unique stable IDs, positive costs and an effect."));
        Ids.Add(M.ModifierId);
        for (const auto& Delta : M.NumberDeltas) if (!Delta.Key.IsValid() || !FMath::IsFinite(Delta.Value)) Fail(TEXT("Invalid modifier numeric delta."));
    }
    FGameplayTagContainer Keys;
    for (const auto& N : Numbers) {
        if (!N.Key.IsValid() || Keys.HasTagExact(N.Key) || !FMath::IsFinite(N.Value) || !FMath::IsFinite(N.Minimum)
            || !FMath::IsFinite(N.Maximum) || N.Minimum > N.Maximum || N.Value < N.Minimum || N.Value > N.Maximum)
            Fail(TEXT("Ability number requires a unique key and finite ordered bounds."));
        Keys.AddTag(N.Key);
    }
    for (const auto& M : Modifiers) for (const auto& Delta : M.NumberDeltas)
        if (!Keys.HasTagExact(Delta.Key)) Fail(TEXT("Modifier refers to an undeclared numeric parameter."));
    return Errors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UMythicSkillDefinition::IsDataValid(FDataValidationContext& Context) const {
    TArray<FText> Errors;
    const bool Valid = ValidateDefinition(Errors);
    for (const auto& Error : Errors) Context.AddError(Error);
    return Valid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
