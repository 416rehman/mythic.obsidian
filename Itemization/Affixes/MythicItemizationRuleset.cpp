#include "Itemization/Affixes/MythicItemizationRuleset.h"

#include "Itemization/Affixes/MythicAffixProfile.h"
#include "Misc/DataValidation.h"
#include "System/MythicAssetManager.h"

#define LOCTEXT_NAMESPACE "MythicItemizationRuleset"

FPrimaryAssetId UMythicItemizationRuleset::GetPrimaryAssetId() const {
    return RulesetTag.IsValid()
               ? FPrimaryAssetId(UMythicAssetManager::ItemizationRulesetType, RulesetTag.GetTagName())
               : FPrimaryAssetId();
}

#if WITH_EDITOR
EDataValidationResult UMythicItemizationRuleset::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Error = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    if (DeveloperName.IsNone() || DesignerPurpose.TrimStartAndEnd().IsEmpty() || Revision < 1) {
        Error(LOCTEXT("InvalidMetadata",
                      "Developer Name, Designer Purpose, and a positive Revision are required."));
    }
    if (!RulesetTag.IsValid() || !RulesetTag.ToString().StartsWith(TEXT("Itemization.Ruleset."))) {
        Error(LOCTEXT("InvalidTag", "Ruleset Tag must be a valid Itemization.Ruleset.* tag."));
    }
    if (Profiles.IsEmpty()) {
        Error(LOCTEXT("MissingProfiles", "A ruleset must directly reference at least one concrete Affix Profile."));
    }

    TSet<FSoftObjectPath> SeenProfiles;
    for (const TSoftObjectPtr<UMythicAffixProfile> &Profile : Profiles) {
        const FSoftObjectPath Path = Profile.ToSoftObjectPath();
        if (!Path.IsValid() || SeenProfiles.Contains(Path)) {
            Error(LOCTEXT("InvalidProfile",
                          "Every ruleset profile must be a valid, unique typed Affix Profile reference."));
            continue;
        }
        SeenProfiles.Add(Path);
    }
    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
