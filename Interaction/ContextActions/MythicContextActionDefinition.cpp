#include "Interaction/ContextActions/MythicContextActionDefinition.h"

#include "Interaction/ContextActions/MythicContextActionProjectionPolicy.h"
#include "Interaction/ContextActions/MythicTags_ContextActions.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "MythicContextActionDefinition"

const FPrimaryAssetType UMythicContextActionDefinition::PrimaryAssetType(TEXT("MythicContextAction"));

FPrimaryAssetId UMythicContextActionDefinition::GetPrimaryAssetId() const {
    return ActionTag.IsValid() ? FPrimaryAssetId(PrimaryAssetType, ActionTag.GetTagName()) : FPrimaryAssetId();
}

#if WITH_EDITOR
EDataValidationResult UMythicContextActionDefinition::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto AddError = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    if (!ActionTag.IsValid() || ActionTag.MatchesTagExact(CONTEXT_ACTION_ROOT)
        || !ActionTag.MatchesTag(CONTEXT_ACTION_ROOT)
        || ActionTag.MatchesTag(CONTEXT_ACTION_REASON_ROOT)) {
        AddError(LOCTEXT("InvalidActionTag", "Action Tag must be a valid Context.Action.* gameplay tag."));
    }
    if (DisplayName.IsEmpty()) {
        AddError(LOCTEXT("MissingDisplayName", "Display Name must contain localized player-facing text."));
    }
    if (!CommonUIInputActionTag.IsValid()
        || CommonUIInputActionTag.MatchesTagExact(CONTEXT_ACTION_INPUT_ROOT)
        || !CommonUIInputActionTag.MatchesTag(CONTEXT_ACTION_INPUT_ROOT)) {
        AddError(LOCTEXT("InvalidInputTag", "CommonUI Input Action Tag must be a valid UI.Action.* gameplay tag."));
    }
    if (!FMythicContextActionProjectionRules::IsHoldDurationValid(
            HoldDurationSeconds)) {
        AddError(LOCTEXT(
            "InvalidHoldDuration",
            "Hold Duration Seconds must be zero for a tap, or between 0.10 and 10.0 seconds for a hold."));
    }
    if (!FMath::IsFinite(MaximumFocusAngleDegrees)
        || MaximumFocusAngleDegrees <= 0.0f
        || MaximumFocusAngleDegrees > 90.0f) {
        AddError(LOCTEXT("InvalidFocusAngle",
                         "Maximum Focus Angle Degrees must be finite and greater than zero up to 90 degrees."));
    }
    if (!FMath::IsFinite(MaximumRangeCentimeters) || MaximumRangeCentimeters < 0.0f) {
        AddError(LOCTEXT("InvalidRange", "Maximum Range Centimeters must be finite and nonnegative."));
    }
    if (RangePolicy == EMythicContextActionRangePolicy::DefinitionRange
        && MaximumRangeCentimeters <= 0.0f) {
        AddError(LOCTEXT("MissingDefinitionRange",
                         "Definition Range policy requires Maximum Range Centimeters greater than zero."));
    }
    for (const TPair<FGameplayTag, FText> &Reason : UnavailableReasonTextOverrides) {
        if (!Reason.Key.IsValid()
            || Reason.Key.MatchesTagExact(CONTEXT_ACTION_REASON_ROOT)
            || !Reason.Key.MatchesTag(CONTEXT_ACTION_REASON_ROOT)
            || Reason.Value.IsEmpty()) {
            AddError(LOCTEXT(
                "InvalidUnavailableReasonOverride",
                "Unavailable Reason Text Overrides require non-root Context.Action.Reason.* keys and nonempty localized text."));
            break;
        }
    }

    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
